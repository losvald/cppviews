#ifndef CPPVIEWS_BENCH_SM_3X5ALL7_HPP_
#define CPPVIEWS_BENCH_SM_3X5ALL7_HPP_

#include "../smv_factory.hpp"

using all7 = SameValuesMatrix<int, 7>;

template<>
class SmvFactory<all7> {
 public:
  typedef all7 ListType;
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

#endif  /* CPPVIEWS_BENCH_SM_3X5ALL7_HPP_ */
