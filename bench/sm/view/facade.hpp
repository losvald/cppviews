#ifndef CPPVIEWS_BENCH_SM_VIEW_FACADE_HPP_
#define CPPVIEWS_BENCH_SM_VIEW_FACADE_HPP_

#include "../../smv_facade.hpp"

template<class ViewType>
class SmvFacade {
 public:
  typedef unsigned CoordType;  // TODO: extract precisely from view (tricky!)

  template<typename DataType>
  void VecMult(const VecInfo<DataType, CoordType>& vi,
               std::vector<DataType>* res) const {
    auto& smv_ = *static_cast<const ViewType*>(this);
    res->resize(RowCount(smv_));
    for (CoordType r = 0, r_end = res->size(); r < r_end; ++r) {
      DataType val = 0;
      for (const auto& e : vi)
        val += smv_(r, e.first) * e.second;
      (*res)[r] = val;
    }
  }
};

#endif  /* CPPVIEWS_BENCH_SM_VIEW_FACADE_HPP_ */
