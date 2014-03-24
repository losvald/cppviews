#ifndef CPPVIEWS_SRC_PORTION_HELPER_HPP_
#define CPPVIEWS_SRC_PORTION_HELPER_HPP_

#include "portion.hpp"

namespace v {

template<class P, typename T>
struct PortionHelper {
  inline const T& Get(const P& portion, size_t index) const {
    return portion[index];
  }

  inline void Set(P& portion, size_t index, const T& value) {
    portion[index] = value;
  }
};

template<typename T>
struct PortionHelper<PortionBase<T>, T> {
  inline const T& Get(const PortionBase<T>& portion, size_t index) const {
    return portion.get(index);
  }

  inline void Set(PortionBase<T>& portion, size_t index, const T& value) {
    portion.set(index, value);
  }
};

}  // namespace v

#endif  /* CPPVIEWS_SRC_PORTION_HELPER_HPP_ */
