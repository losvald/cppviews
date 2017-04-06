#ifndef CPPVIEWS_BENCH_SM_STL_MAP_FACADE_HPP_
#define CPPVIEWS_BENCH_SM_STL_MAP_FACADE_HPP_

#include "../../smv_facade.hpp"

#include <map>

template<typename T, typename CoordType>
#define V_THIS_ADAPTEE_TYPE                     \
  std::map<std::pair<CoordType, CoordType>, T>
class SmvFacade<V_THIS_ADAPTEE_TYPE>
#define V_THIS_BASE_TYPE                                \
  detail::SmvFacadeMapHelper<V_THIS_ADAPTEE_TYPE>
    : public V_THIS_BASE_TYPE {
  typedef V_THIS_BASE_TYPE Helper;
#undef V_THIS_BASE_TYPE
#undef V_THIS_ADAPTEE_TYPE
 public:
  SmvFacade(typename Helper::DataType* default_value,
            const CoordType& row_count, const CoordType& col_count)
      : Helper(default_value, row_count, col_count) {}

  const typename SmvFacade::ValuesView& values() const { return this->values_; }
};

#endif  /* CPPVIEWS_BENCH_SM_STL_MAP_FACADE_HPP_ */
