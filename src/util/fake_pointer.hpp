#ifndef CPPVIEWS_SRC_UTIL_FAKE_POINTER_HPP_
#define CPPVIEWS_SRC_UTIL_FAKE_POINTER_HPP_

#include <cstddef>
#include <utility>

template<typename T>
class FakePointer {
 public:
  typedef T element_type;

  template<typename ... Args>
  FakePointer(Args&&... args) : data_(std::forward<Args>(args)...) {}

  // this is ignored in overload resolution, so a hack below is used instead
  // FakePointer(FakePointer&& p) {}//: data_(std::move(p.data_)) {}

  FakePointer(FakePointer& p) : data_(p.data_) {}
  FakePointer() {}

  inline T* operator->() const {
    return &data_;
  }

  inline T& operator*() const noexcept {
    return data_;
  }

  friend inline bool operator==(const FakePointer& lhs,
                                const FakePointer& rhs) noexcept {
    return &lhs.data_ == &rhs.data_;
  }

  friend inline bool operator!=(const FakePointer& lhs,
                                const FakePointer& rhs) noexcept {
    return &lhs.data_ != &rhs.data_;

  }

  friend inline bool operator==(const FakePointer& lhs,
                                nullptr_t rhs) noexcept {
    return false;
  }

  inline friend bool operator!=(const FakePointer& lhs,
                                nullptr_t rhs) noexcept {
    return true;
  }

 private:
  inline operator T&&() const {     // hack to enable proper move ctor
    return std::move(data_);
  }

  mutable T data_;
};

#endif  /* CPPVIEWS_SRC_UTIL_FAKE_POINTER_HPP_ */
