#include "../src/sparse_list.hpp"
#include "test.hpp"

// typedef ::testing::Types<
//   std::integral_constant<unsigned, 3>,
//   std::integral_constant<size_t, 4>,
//   std::integral_constant<unsigned, 1> > Rhs;
// TYPED_TEST_CASE(SparseListTest, Rhs);


TEST(SparseListTest, 1D) {
  // auto l = MakeList(SparseListTag<1>(), 5);
  int zero;
  SparseHashList<int, 1> sl(SparseListTag<1>(), &zero, 5);

  EXPECT_EQ((std::array<size_t, 1>{{5}}), sl.sizes());

  sl.get({3}) = 30;
  EXPECT_EQ(30, sl.get(3));
  EXPECT_EQ(1, sl.nondefault_count());

  sl.get(2);
  sl(1);
  EXPECT_EQ(1, sl.nondefault_count());

  auto it = sl.begin();
  EXPECT_EQ(sl.begin(), it);
  EXPECT_EQ(30, *it);
  EXPECT_EQ(sl.end(), ++it);

  // TODO complete
}

TEST(SparseListTest, 2D) {
  double zero;
  auto sm = MakeList(SparseListTag<2>(), &zero, 2, 3);

  sm.get({2, 3}) = 4.5;
  EXPECT_EQ(4.5, sm(2, 3));

  // TODO complete
}
