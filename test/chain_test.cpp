#include "../src/chain.hpp"
#include "test.hpp"

TEST(ChainTest, PolyNoGapNoOffsets) {
  //    0 1 2 3 4 5 6 7
  //   +-----+-------+-+
  // 0 |1 0 0|4 5 0 0|0|
  // 1 |0 2 0|0 0 6 7|8|
  // 2 |0 0 3+-------+0|
  // 3 +-----+       |9|
  //                 +-+
  int digits[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1};
  typedef ListBase<int, 2> ListBaseType;
  Chain<ListBase<int, 2>, 0> c2_0_nogap(
      ListVector<ListBase<int, 2> >()
      .Append([&](unsigned col, unsigned row) {
          return digits + (1 + col) * (row == col);
        }, 3, 3)
      .Append([&](unsigned col, unsigned row) {
          return digits + (4 + col) * ((!row && col < 2) || (row && col >= 2));
        }, 4, 2)
      .Append([&](unsigned col, unsigned row) {
          return digits + (7 + (row + 1) / 2) * (row % 2);
        }, 1, 4),
      &digits[10]);  // use -1 as default
  typedef ListBaseType::SizeArray SizeArray;
  EXPECT_EQ((SizeArray({8, 4})), c2_0_nogap.sizes());
  EXPECT_EQ((SizeArray({0, 0})), c2_0_nogap.nesting_offset(0));
  EXPECT_EQ((SizeArray({3, 0})), c2_0_nogap.nesting_offset(1));
  EXPECT_EQ((SizeArray({7, 0})), c2_0_nogap.nesting_offset(2));

  // validate all non-zero entries and a few other corner cases
  EXPECT_EQ(1, c2_0_nogap.get({0, 0}));
  EXPECT_EQ(2, c2_0_nogap.get({1, 1}));
  EXPECT_EQ(3, c2_0_nogap.get({2, 2}));
  EXPECT_EQ(4, c2_0_nogap.get({3, 0}));
  EXPECT_EQ(5, c2_0_nogap.get({4, 0}));
  EXPECT_EQ(6, c2_0_nogap.get({5, 1}));
  EXPECT_EQ(7, c2_0_nogap.get({6, 1}));
  EXPECT_EQ(8, c2_0_nogap.get({7, 1}));
  EXPECT_EQ(9, c2_0_nogap.get({7, 3}));
  EXPECT_EQ(0, c2_0_nogap.get({3, 1}));
  EXPECT_EQ(0, c2_0_nogap.get({7, 2}));
  EXPECT_EQ(-1, c2_0_nogap.get({3, 2}));
  EXPECT_EQ(-1, c2_0_nogap.get({0, 3}));
  EXPECT_EQ(-1, c2_0_nogap.get({6, 2}));

  // validate the number of zero entries is correct
  unsigned zero_cnt = 0;
  for (unsigned col = 0; col < c2_0_nogap.sizes()[0]; ++col)
    for (unsigned row = 0; row < c2_0_nogap.sizes()[1]; ++row)
      zero_cnt += c2_0_nogap.get({col, row}) == 0;
  EXPECT_EQ(12, zero_cnt);
}

TEST(ChainTest, PolyConst) {
  //    0 1 2 3 4
  //       +-----+
  // 0     |0 0 1|
  // 1     |0 2 0|
  // 2     |3 0 0|
  //   +---+---+-+
  // 3 |4 0 6 0|
  // 4 |0 5 0 7|
  //   +-------+
  // 5
  //     +-+
  // 6   |0|
  // 7   |8|
  //     +-+ +---+
  // 8       |0 0|
  // 9       |9 0|
  //         +---+
  const int digits[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1};
  typedef ListBase<const int, 2> ListBaseType;
  auto&& c2_1_lists_factory = [&]() {
    return std::move(ListVector<ListBaseType>()
     .Append([&](unsigned col, unsigned row) {
          return digits + (1 + row) * (row + col == 2);
        }, 3, 3)
      .Append([&](unsigned col, unsigned row) {
          return digits + (4 + col) * (row == col % 2);
        }, 4, 2)
      .Append([&](unsigned col, unsigned row) {
          return digits + 8 * (row == 1);
        }, 1, 2)
      .Append([&](unsigned col, unsigned row) {
          return digits + 9 * (row == 1 && col == 0);
        }, 2, 2));
  };
  Chain<ListBaseType, 1> c2_1(
      c2_1_lists_factory(),
      &digits[10],  // use -1 as default
      {{0, {2}}, {2, {1}}, {3, {3}}},
      {{2, 1}});
  typedef ListBaseType::SizeArray SizeArray;
  EXPECT_EQ((SizeArray({5, 10})), c2_1.sizes());
  EXPECT_EQ((SizeArray({2, 0})), c2_1.nesting_offset(0));
  EXPECT_EQ((SizeArray({0, 3})), c2_1.nesting_offset(1));
  EXPECT_EQ((SizeArray({1, 6})), c2_1.nesting_offset(2));
  EXPECT_EQ((SizeArray({3, 8})), c2_1.nesting_offset(3));

  EXPECT_EQ(1, c2_1.get({4, 0}));
  EXPECT_EQ(2, c2_1.get({3, 1}));
  EXPECT_EQ(3, c2_1.get({2, 2}));
  EXPECT_EQ(4, c2_1.get({0, 3}));
  EXPECT_EQ(5, c2_1.get({1, 4}));
  EXPECT_EQ(6, c2_1.get({2, 3}));
  EXPECT_EQ(7, c2_1.get({3, 4}));
  EXPECT_EQ(8, c2_1.get({1, 7}));
  EXPECT_EQ(9, c2_1.get({3, 9}));
  EXPECT_EQ(0, c2_1.get({1, 3}));
  EXPECT_EQ(0, c2_1.get({4, 9}));
  EXPECT_EQ(-1, c2_1.get({0, 0}));
  EXPECT_EQ(-1, c2_1.get({1, 2}));
  EXPECT_EQ(-1, c2_1.get({1, 5}));

  // verify construction via explicitly provided offsets
  Chain<ListBaseType, 1> c2_1_explicit(
      c2_1_lists_factory(),
      &digits[10],
      {{2, 0}, {0, 3}, {1, 6}, {3, 8}},
      5, 10);
  ASSERT_EQ((SizeArray({5, 10})), c2_1_explicit.sizes());
  EXPECT_EQ((SizeArray({2, 0})), c2_1_explicit.nesting_offset(0));
  EXPECT_EQ((SizeArray({0, 3})), c2_1_explicit.nesting_offset(1));
  EXPECT_EQ((SizeArray({1, 6})), c2_1_explicit.nesting_offset(2));
  EXPECT_EQ((SizeArray({3, 8})), c2_1_explicit.nesting_offset(3));
  for (unsigned col = 0; col < c2_1.sizes()[0]; ++col)
    for (unsigned row = 0; row < c2_1.sizes()[1]; ++row)
      EXPECT_EQ(c2_1.get({col, row}), c2_1_explicit.get({col, row})) <<
          "Incorrect get(" << col << ", " << row << ')';
}

TEST(ChainTest, Poly1DLeadingGap) {
  const char* value = "value";
  typedef ListBase<const char*, 1> ListBaseType;
  Chain<ListBaseType, 0> c1(
      ListVector<ListBaseType>()
      .Append([&](unsigned) {
          return &value;
        }, 1),
      nullptr,
      {},
      {{0, 3}});
  EXPECT_EQ(4, c1.sizes().front());
  EXPECT_EQ(3, c1.nesting_offset(0).front());
  EXPECT_EQ("value", c1.get({3}));
}

TEST(ChainTest, PolyTrailingGap) {
  const char* value = "value";
  typedef ListBase<const char*, 3> ListBaseType;
  Chain<ListBaseType, 1> c3_1(
      ListVector<ListBaseType>()
      .Append([&](unsigned, unsigned, unsigned) {
          return &value;
        }, 1, 1, 1),
      nullptr,
      {},
      {{1, 2}});
  typedef ListBaseType::SizeArray SizeArray;
  EXPECT_EQ((SizeArray({1, 3, 1})), c3_1.sizes());
  EXPECT_EQ((SizeArray({0, 0, 0})), c3_1.nesting_offset(0));
  EXPECT_EQ("value", c3_1.get({0, 0, 0}));
}
