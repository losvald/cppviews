#ifndef CPPVIEWS_BENCH_SMV_FACTORY_HPP_
#define CPPVIEWS_BENCH_SMV_FACTORY_HPP_

#include "smv.hpp"

// The generic factory class for Sparse Matrix View Factories - uses no-arg ctor
// Should provide specialization with the same interface otherwise
template<class List>
class SmvFactory {
 public:
  typedef List ListType;  // must be v::List or derived from it
  typedef DefaultCoord CoordType;

  static const ListType& Get() {
    static ListType instance;
    return instance;
  }
};

#endif  /* CPPVIEWS_BENCH_SMV_FACTORY_HPP_ */
