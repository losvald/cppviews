#include "../../../bench/util/sparse_matrix.hpp"

#include "test.hpp"

#include <sstream>

using namespace std;

TEST(SparseMatrixTest, Small) {
  //  +--+--+--+--+
  //  |  |10|  |20|
  //  +--+--+--+--+
  //  |30|40|  |50|
  //  +--+--+--+--+
  //  |  |  |  |60|
  //  +--+--+--+--+
  istringstream ss("3 4 6\n"
                   "2 1 30\n"
                   "1 2 10\n"
                   "3 4 60\n"
                   "2 2 40\n"
                   "1 4 20\n"
                   "2 4 50\n");
  SparseMatrix<int> sm;
  sm.Init(ss);
  EXPECT_EQ(3, sm.row_count());
  EXPECT_EQ(4, sm.col_count());
  EXPECT_EQ(6, sm.nonzero_count());
  EXPECT_EQ(3 * 4, sm.element_count());

  // validate values
  EXPECT_EQ(20, sm(0, 3));
  EXPECT_EQ(30, sm(1, 0));
  EXPECT_EQ(0, sm(2, 2));
  EXPECT_EQ(0, sm(3, 1));

  // validate iteration over columns
  auto r = sm.nonzero_col_range(0, 0, 4);
  EXPECT_EQ(1, *r.first++);
  EXPECT_EQ(3, *r.first++);
  EXPECT_EQ(r.first, r.second);
  r = sm.nonzero_col_range(1, 1, 3);
  EXPECT_EQ(1, *r.first++);
  EXPECT_EQ(r.first, r.second);

  // validate iteration over rows
  r = sm.nonzero_row_range(3, 2, 3);
  EXPECT_EQ(2, *r.first++);
  EXPECT_EQ(r.first, r.second);
  r = sm.nonzero_row_range(2, 0, 3);
  EXPECT_EQ(r.first, r.second);
}

TEST(SparseMatrixTest, AllZeroes) {
  istringstream ss("13 7 0");
  SparseMatrix<> sm;
  sm.Init(ss);
  EXPECT_EQ(13, sm.row_count());
  EXPECT_EQ(7, sm.col_count());
  EXPECT_EQ(0, sm.nonzero_count());
  EXPECT_EQ(13 * 7, sm.element_count());
  typedef decltype(sm.row_count()) Coord;
  for (Coord row = 0; row < sm.row_count(); ++row)
    for (Coord col = 0; col < sm.col_count(); ++col)
      ASSERT_EQ(0, sm(row, col));

  auto r = sm.nonzero_col_range(2, 3, 4);
  EXPECT_EQ(r.first, r.second);
}

TEST(SparseMatrixTest, Huge) {
  istringstream ss("123456789 987654321 2\n"
                   "111111112 222222223 5\n"
                   "333333334 444444445 7");
  SparseMatrix<double, size_t> sm;
  ASSERT_NO_THROW({
      sm.Init(ss);
      ASSERT_EQ(5, sm(111111111, 222222222));
      ASSERT_EQ(7, sm(333333333, 444444444));
    });
}
