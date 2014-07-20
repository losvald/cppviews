#ifndef CPPVIEWS_BENCH_SM_ALL7_MANUAL_HPP_
#define CPPVIEWS_BENCH_SM_ALL7_MANUAL_HPP_

#include "../smv_factory.hpp"

using all7_manual = SameValuesMatrix<int, 7>;

template<>
class SmvFactory<all7_manual> {
 public:
  typedef all7_manual ListType;
  typedef unsigned CoordType;

  static const ListType& Get() {
    struct StaticInitializer {
      ListType instance;
      StaticInitializer() : instance(3, 5) {}
    };
    static StaticInitializer si;
    return si.instance;
  }
};

#endif  /* CPPVIEWS_BENCH_SM_ALL7_MANUAL_HPP_ */
