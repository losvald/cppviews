#ifndef CPPVIEWS_BENCH_SM_ARMADILLO_FACADE_HPP_
#define CPPVIEWS_BENCH_SM_ARMADILLO_FACADE_HPP_

#include "../../smv_facade.hpp"

#include <armadillo>

#include <array>
#include <type_traits>

template<typename T>
#define V_THIS_ADAPTEE_TYPE                     \
  arma::SpMat<T>
class SmvFacade<V_THIS_ADAPTEE_TYPE>
#define V_THIS_BASE_TYPE                                \
  detail::SmvFacadeHelper<T>
    : public V_THIS_BASE_TYPE {
  typedef V_THIS_BASE_TYPE Helper;
#undef V_THIS_BASE_TYPE
 public:
  using typename Helper::DataType;
  using typename Helper::CoordType;

  SmvFacade(DataType* default_value,
            const CoordType& row_count, const CoordType& col_count)
      : Helper(default_value, row_count, col_count),
        sm_(row_count, col_count) {}

  auto operator()(unsigned row, unsigned col)
      -> decltype(std::declval<V_THIS_ADAPTEE_TYPE>().at(row, col)) {
    return sm_.at(row, col);
  }

  auto operator()(unsigned row, unsigned col) const
      -> decltype(std::declval<V_THIS_ADAPTEE_TYPE>().at(row, col)) {
    // TODO: we are a bit cheating here: what if default_value_ != 0 ???
    return sm_.at(row, col);
  }

 protected:
  mutable V_THIS_ADAPTEE_TYPE sm_;
#undef V_THIS_ADAPTEE_TYPE
};

#endif  /* CPPVIEWS_BENCH_SM_ARMADILLO_FACADE_HPP_ */
