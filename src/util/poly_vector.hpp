#ifndef CPPVIEWS_SRC_UTIL_POLY_VECTOR_HPP_
#define CPPVIEWS_SRC_UTIL_POLY_VECTOR_HPP_

#include "fake_pointer.hpp"

#include <memory>
#include <type_traits>
#include <vector>

template<class T, class Base = T>
using PolyUniqPtr = typename std::conditional<
  std::is_same<T, Base>::value,
  std::unique_ptr<Base>,
  FakePointer<T>
  >::type;

template<class T, class Base>
struct PolyVectorEmplaceHelper {
  template<class Derived, typename... Args>
  inline void Emplace(std::vector<PolyUniqPtr<T, Base> >& v,
                      Args&&... args) const {
    v.emplace_back(std::forward<Args>(args)...);
  }
};

template<typename T>
struct PolyVectorDefaultFactory {
  template<typename... Args>
  constexpr T operator()(Args&&... args) const { return T(); }
};

template<class T, class Base = T, class Factory = PolyVectorDefaultFactory<T> >
class PolyVector {
 public:
  typedef PolyUniqPtr<T, Base> Pointer;

  PolyVector(size_t capacity) { v_.reserve(capacity); }
  // PolyVector(const PolyVector& pv) = default;
  // PolyVector(PolyVector&& pv) = default;
  PolyVector() = default;

  inline const T& operator[](size_t index) const {
    return *v_[index];
  }

  inline T& operator[](size_t index) {
    return *v_[index];
  }

  template<typename... Args>
  inline PolyVector& AppendThis(Args&&... args) {
    return this->template Append<T>(std::forward<Args>(args)...);
  }

  template<typename... Args>
  inline PolyVector&& Append(Args&&... args) {
    return this->Append<decltype(Factory()(std::forward<Args>(args)...))>(
        std::forward<Args>(args)...);
  }

  template<class Derived, typename... Args>
  inline PolyVector&& Append(Args&&... args) {
    PolyVectorEmplaceHelper<T, Base> h;
    h.template Emplace<Derived>(v_, std::forward<Args>(args)...);
    return std::move(*this);
  }

  void Shrink() { v_.shrink_to_fit(); }

  size_t size() const { return v_.size(); }
  size_t max_size() const { return v_.max_size(); }

 private:
  std::vector<Pointer> v_;
};

template<typename T>
struct PolyVectorEmplaceHelper<T, T> {
  template<class Derived, typename... Args>
  inline void Emplace(
      std::vector<PolyUniqPtr<T, T> >& v, Args&&... args) const {
    // TODO: optimize using placement new (to ensure locality of pointed data)
    v.emplace_back(new Derived(std::forward<Args>(args)...));
  }
};

#endif  /* CPPVIEWS_SRC_UTIL_POLY_VECTOR_HPP_ */
