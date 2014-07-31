#ifndef CPPVIEWS_BENCH_SM_EIGEN_FACADE_HPP_
#define CPPVIEWS_BENCH_SM_EIGEN_FACADE_HPP_

#include "../../smv_facade.hpp"

#include <eigen3/Eigen/Sparse>

#include <array>

template<typename T>
#define V_THIS_ADAPTEE_TYPE                     \
  Eigen::SparseMatrix<T, Eigen::RowMajor>
class SmvFacade<V_THIS_ADAPTEE_TYPE>
#define V_THIS_BASE_TYPE                                \
  detail::SmvFacadeHelper<T>
    : public V_THIS_BASE_TYPE {
  typedef V_THIS_BASE_TYPE Helper;
#undef V_THIS_BASE_TYPE
#undef V_THIS_ADAPTEE_TYPE
 public:
  using typename Helper::DataType;
  using typename Helper::CoordType;

  SmvFacade(DataType* default_value,
            const CoordType& row_count, const CoordType& col_count)
      : Helper(default_value, row_count, col_count),
        sm_(row_count, col_count) {}

  DataType& operator()(unsigned row, unsigned col) {
    return sm_.coeffRef(row, col);
  }

  DataType& operator()(unsigned row, unsigned col) const {
    return sm_.coeff(row, col) ? sm_.coeffRef(row, col) : *ZeroPtr<int>();
  }

 protected:
  mutable Eigen::SparseMatrix<int, Eigen::RowMajor> sm_;
};

#endif  /* CPPVIEWS_BENCH_SM_EIGEN_FACADE_HPP_ */
