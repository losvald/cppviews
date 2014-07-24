#ifndef CPPVIEWS_BENCH_SMV_RUN_HPP_
#define CPPVIEWS_BENCH_SMV_RUN_HPP_

// This can be overridden by specifying command line arguments for compiler
// E.g., for GCC:
//      g++ ... -DSM_NAME=p0040_cdd -include sm/view/p0040_cdd.hpp
#ifndef SM_NAME
// We use a default that always exist here not to trigger compiler errors
#define SM_NAME p0040_cdd_manual
#include "sm/view/p0040_cdd_manual.hpp"

#endif  // !defined(SM_NAME)

#endif  /* CPPVIEWS_BENCH_SMV_RUN_HPP_ */
