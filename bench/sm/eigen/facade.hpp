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
 public:
  using typename Helper::DataType;
  using typename Helper::CoordType;

  class ValuesView {
    typedef typename V_THIS_ADAPTEE_TYPE SparseMatrixType;
    typedef typename SparseMatrixType::InnerIterator SparseMatrixIter;
    const SparseMatrixType* sm_;

   public:
    class Iterator : public SparseMatrixIter {
     public:
      explicit Iterator(const SparseMatrixType& sm, const size_t& outer = 0,
                        const size_t& skip = 0)
          : SparseMatrixIter(sm, outer) {
        this->m_id += skip;
      }
      DataType& operator*() const {
        return const_cast<Iterator*>(this)->valueRef();
      }
      DataType* operator->() const { return &this->operator*(); }
      bool operator==(const Iterator& other) const {
        return this->m_id == other.m_id;
      }
      bool operator!=(const Iterator& other) const { return !(*this == other); }
    };

    ValuesView(const SparseMatrixType& sm) : sm_(&sm) {}
    Iterator begin() const { return Iterator(*sm_); }
    Iterator end() const {
      auto last_row = sm_->row(sm_->rows() - 1);
      // auto last_row = sm_->block(sm_->rows() - 1, 0, 1, sm_->cols());
      // A dirty workaround for [row()|block()].nonZeros() which segfaults (?)
      // TODO: replace with a O(1) workaround (this is O(sqrt N))
      size_t last_row_nnz = last_row.template cast<bool>()
          .template cast<int>().sum();
      return Iterator(*sm_, sm_->rows() - 1, last_row_nnz);
    }
    size_t size() const { return sm_->nonZeros(); }
  };
#undef V_THIS_ADAPTEE_TYPE

  SmvFacade(DataType* default_value,
            const CoordType& row_count, const CoordType& col_count)
      : Helper(default_value, row_count, col_count),
        sm_(row_count, col_count),
        values_(sm_) {}

  DataType& operator()(unsigned row, unsigned col) {
    return sm_.coeffRef(row, col);
  }

  DataType& operator()(unsigned row, unsigned col) const {
    return sm_.coeff(row, col) ? sm_.coeffRef(row, col) : *this->default_value_;
  }

  const ValuesView& values() const { return values_; }

 protected:
  mutable Eigen::SparseMatrix<DataType, Eigen::RowMajor> sm_;

 private:
  ValuesView values_;
};

#endif  /* CPPVIEWS_BENCH_SM_EIGEN_FACADE_HPP_ */
