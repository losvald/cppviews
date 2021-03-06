#ifndef CPPVIEWS_SRC_UTIL_POLY_VECTOR_HPP_
#define CPPVIEWS_SRC_UTIL_POLY_VECTOR_HPP_

#include "fake_pointer.hpp"
#include "iterator.hpp"

#include <memory>
#include <type_traits>
#include <vector>

namespace detail {

template<class T, class Base = T>
using PolyUniqPtr = typename std::conditional<
  std::is_same<T, Base>::value,
  std::unique_ptr<Base>,
  FakePointer<T>
  >::type;

template<class T, class Base>
struct PolyVectorEmplaceHelper {
  typedef std::vector<PolyUniqPtr<T, Base> > Vector;

  template<class Derived, typename... Args>
  inline void EmplaceBack(Vector& v, Args&&... args) const {
    v.emplace_back(std::forward<Args>(args)...);
  }

  template<class Derived, typename... Args>
  inline typename Vector::iterator
  Emplace(typename Vector::iterator pos, Vector& v, Args&&... args) const {
    return v.emplace(pos, std::forward<Args>(args)...);
  }
};

template<typename T>
struct PolyVectorEmplaceHelper<T, T> {
  typedef std::vector<PolyUniqPtr<T, T> > Vector;

  template<class Derived, typename... Args>
  inline void EmplaceBack(Vector& v, Args&&... args) const {
    v.emplace_back(Create<Derived>(std::forward<Args>(args)...));
  }

  template<class Derived, typename... Args>
  inline typename Vector::iterator
  Emplace(typename Vector::iterator pos, Vector& v, Args&&... args) const {
    // TODO: g++ 4.8 doesn't support const_iterator in emplace, so use iterator
    // (for compilers that support const_iterator, iterator will convert to it)
    return v.emplace(pos, Create<Derived>(std::forward<Args>(args)...));
  }

 private:
  template<class Derived, typename... Args>
  static Derived* Create(Args&&... args) {
    // TODO: optimize using placement new (to ensure locality of pointed data)
    return new Derived(std::forward<Args>(args)...);
  }
};

template<typename T>
struct PolyVectorDefaultFactory {
  template<typename... Args>
  constexpr T operator()(Args&&... args) const {
    return T(std::forward<Args>(args)...);
  }
};

template<class T, class Base, class Factory>
struct PolyVectorFactoryProvider {
  typedef typename std::conditional<
    std::is_same<T, Base>::value,
    Factory,
    PolyVectorDefaultFactory<T> >::type FactoryType;
};

}  // namespace detail

template<class T, class Base = T,
         class Factory = detail::PolyVectorDefaultFactory<T> >
class PolyVector {
  // for convenience, allow user to always specify a polymorphic factory,
  // so use default (which generates derived) in the non-poly case
  typedef typename detail::PolyVectorFactoryProvider<T, Base, Factory>
  ::FactoryType FactoryType;
  typedef detail::PolyUniqPtr<T, Base> Pointer;
  typedef std::vector<Pointer> WrappedType;

  struct IteratorTransformer;
  struct ConstIteratorTransformer;

 public:
  typedef typename Pointer::element_type ValueType;
  typedef ValueType& Ref;
  typedef const Ref ConstRef;
  typedef typename WrappedType::size_type SizeType;

  typedef TransformedIterator<typename WrappedType::iterator,
                              IteratorTransformer
                              > Iterator;
  typedef TransformedIterator<typename WrappedType::const_iterator,
                              ConstIteratorTransformer,
                              typename WrappedType::iterator
                              > ConstIterator;

  PolyVector(SizeType capacity) { v_.reserve(capacity); }
  // PolyVector(const PolyVector& pv) = default;
  // PolyVector(PolyVector&& pv) = default;
  PolyVector() = default;

  inline ConstRef operator[](SizeType index) const { return *v_[index]; }
  inline Ref operator[](SizeType index) { return *v_[index]; }

  template<typename... Args>
  inline PolyVector&& Append(Args&&... args) {
    return this->AppendDerived<
      decltype(FactoryType()(std::forward<Args>(args)...))>(
          std::forward<Args>(args)...);
  }

  template<class Derived, typename... Args>
  inline PolyVector&& AppendDerived(Args&&... args) {
    detail::PolyVectorEmplaceHelper<T, Base> h;
    h.template EmplaceBack<Derived>(v_, std::forward<Args>(args)...);
    return std::move(*this);
  }

  template<typename... Args>
  inline Iterator Add(Iterator pos, Args&&... args) {
    return this->Add<decltype(FactoryType()(std::forward<Args>(args)...))>(
        pos, std::forward<Args>(args)...);
  }

  template<class Derived, typename... Args>
  Iterator Add(Iterator pos, Args&&... args) {
    detail::PolyVectorEmplaceHelper<T, Base> h;
    return h.template Emplace<Derived>(pos.iterator(), v_,
                                       std::forward<Args>(args)...);
  }

  void Shrink() { v_.shrink_to_fit(); }

  Iterator Erase(Iterator first, Iterator last) {
    // TODO: g++ 4.8 doesn't support const_iterator in erase, so use iterator
    // (for compilers that support const_iterator, iterator will convert to it)
    return v_.erase(first.iterator(), last.iterator());
  }

  void reset(SizeType index, T* ptr) { v_[index].reset(ptr); }

  Iterator begin() { return v_.begin(); }
  Iterator end() { return v_.end(); }
  ConstIterator begin() const { return v_.begin(); }
  ConstIterator end() const { return v_.end(); }
  ConstIterator cbegin() const { return v_.cbegin(); }
  ConstIterator cend() const { return v_.cend(); }

  Ref front() { return *v_.front(); }
  ConstRef front() const { return *v_.front(); }
  Ref back() { return *v_.back(); }
  ConstRef back() const { return *v_.back(); }

  SizeType size() const { return v_.size(); }
  SizeType max_size() const { return v_.max_size(); }
  bool empty() const { return v_.empty(); }

 private:
  struct IteratorTransformer {
    Ref operator()(Pointer& ptr) const { return *ptr; }
  };

  struct ConstIteratorTransformer {
    ConstRef operator()(const Pointer& ptr) const { return *ptr; }
  };

  WrappedType v_;
};

#endif  /* CPPVIEWS_SRC_UTIL_POLY_VECTOR_HPP_ */
