// Usage:
// 1) provide a specialization for SmvFactory<SmvTypeToCheck>
// 2) typedef SmvFactory<SmvTypeToCheck> SmvFactoryType;
// 3) #include "smv_checker.cpp.tpl"
// e.g.

// #define TEST_SMV_CHECKER  // uncomment for a quick test
#ifdef TEST_SMV_CHECKER

#include "../../bench/smv_factory.hpp"

// // A quick and dirty alternative to the below:
// struct ThreeByFiveAllSevens : SameValuesMatrix<int, 7> {
//   ThreeByFiveAllSevens() : SameValuesMatrix<int, 7>(3, 5) {}
// };

using ThreeByFiveAllSevens = SameValuesMatrix<int, 7>;

template<>
class SmvFactory<ThreeByFiveAllSevens> {
 public:
  typedef ThreeByFiveAllSevens ListType;
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

typedef SmvFactory<ThreeByFiveAllSevens> SmvFactoryType;

#endif  // defined(TEST_SMV_CHECKER)


#include "util/sparse_matrix.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>

#include <cmath>
#include <cstdio>
#include <cstdlib>

typedef SmvFactoryType::CoordType CoordType;
typedef SmvFactoryType::ListType::DataType DataType;
typedef SparseMatrix<DataType, CoordType> SM;

#define SMV_FOR_ROW_COL(smv, r, c)                                      \
  for (CoordType r = 0, r ## _to = RowCount(smv); r < r ## _to; ++r)    \
    for (CoordType c = 0, c ## _to = ColCount(smv); c < c ## _to; ++c)

template<class List>
size_t CountNonZeroes(const List& smv) {
  size_t cnt = 0;
  SMV_FOR_ROW_COL(smv, r, c) {
    cnt += (smv(r, c) == typename List::DataType());
  }
  return cnt;
}

int main(int argc, char* argv[]) {
  const char* prog = argv[0];
  const bool quit = [&]{  // a flag whether to quit after first unequal values
    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'q') {
      --argc, ++argv;
      return true;
    }
    return false;
  }();
  if (argc < 2) {
    fprintf(stderr, "Usage: %s [-q] SM_MTX\n", prog);
    return 1;
  }

  SM sm;
  std::ifstream ifs(argv[1]);
  sm.Init(ifs);
  auto& smv = SmvFactoryType::Get();
  if (RowCount(smv) != static_cast<size_t>(sm.row_count()) ||
      ColCount(smv) != static_cast<size_t>(sm.col_count())) {
    std::cerr << "Size not equal\n"
        "  Actual: " << RowCount(smv) << " x " << ColCount(smv) << "\n"
        "Expected: " << sm.row_count() << " x " << sm.col_count() << std::endl;
    return 2;
  }

  int exit_code = 0;  // bit mask which
  static const double kEps = 1e-9;
#define SMV_CHECK_ALMOST_EQ(exp, act, row, col, exit_code_lg)           \
  do {                                                                  \
    if (fabs(exp - act) > kEps) {                                       \
        printf("(%lld, %lld) not equal\n  Actual: %g\nExpected: %g\n",  \
               (long long)row, (long long)col, (double)act, (double)exp); \
        if ((exit_code |= 1 << exit_code_lg) && quit)                   \
          return exit_code;                                             \
    }                                                                   \
  } while (0)

  // check non-zero elements
  for (auto row = sm.row_count(); row--; )
    for (auto col_range = sm.nonzero_col_range(row, 0, sm.col_count());
         col_range.first != col_range.second; ++col_range.first) {
      auto col = *col_range.first;
      SMV_CHECK_ALMOST_EQ(sm(row, col), smv(row, col), row, col, 3);
    }

  // check up to twice as many elements (to check some zero elements)
  size_t n = std::min(smv.size(), 2 * CountNonZeroes(smv));
  SMV_FOR_ROW_COL(smv, row, col) {
    if (n-- == 0)
      break;
    auto expected = sm(row, col);
    if (fabs(expected) <= kEps)
      SMV_CHECK_ALMOST_EQ(expected, smv(row, col), row, col, 4);
  }
#undef SMV_CHECK_ALMOST_EQ

  return exit_code;
}
