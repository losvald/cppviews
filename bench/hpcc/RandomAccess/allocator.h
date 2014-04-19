#ifndef CPPVIEWS_BENCH_HPCC_RANDOMACCESS_ALLOCATOR_H_
#define CPPVIEWS_BENCH_HPCC_RANDOMACCESS_ALLOCATOR_H_

#include "../hpl/include/hpccmema.h"

#include <memory>

// minimum C++11 allocator (http://en.cppreference.com/w/cpp/concept/Allocator)
// which uses HPCC_XMALLOC and HPCC_free
template<class T>
struct HpccXAllocator {
  typedef T value_type;

  HpccXAllocator() = default;

  template<class U>
  HpccXAllocator(const HpccXAllocator& other) {} // stateless (nothing to copy)

  T* allocate(std::size_t n) { return HPCC_XMALLOC(T, n); }
  void deallocate(T* p, std::size_t n) { HPCC_free(p); }
};

template<class T, class U>
bool operator==(const HpccXAllocator<T>&, const HpccXAllocator<U>&) {
  return true;
}

template<class T, class U>
bool operator!=(const HpccXAllocator<T>& lhs, const HpccXAllocator<U>& rhs) {
  return !(lhs == rhs);
}

#endif  /* CPPVIEWS_BENCH_HPCC_RANDOMACCESS_ALLOCATOR_H_ */
