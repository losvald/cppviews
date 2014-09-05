#ifndef CPPVIEWS_SRC_UTIL_PROXY_POINTER_HPP_
#define CPPVIEWS_SRC_UTIL_PROXY_POINTER_HPP_

#include <type_traits>

template<class Proxy>
class ProxyPointer {
 public:
  ProxyPointer() = default;
  // ProxyPointer(const Proxy& copy)
  // noexcept(std::is_nothrow_copy_constructible<Proxy>::value)
  //     : proxy_(copy) {}

  ProxyPointer(Proxy&& src)
  noexcept(std::is_nothrow_move_constructible<Proxy>::value)
      : proxy_(std::move(src)) {}

  // Proxy& operator=(Proxy&& src) = default;
  // Proxy& operator=(Proxy&& src) = default;
  Proxy& operator*() const noexcept { return proxy_; }
  Proxy* operator->() const { return &proxy_; }

 private:
  Proxy proxy_;
};

#endif  /* CPPVIEWS_SRC_UTIL_PROXY_POINTER_HPP_ */
