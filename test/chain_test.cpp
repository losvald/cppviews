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
          return digits + (8 + (row + 1) / 2) * (col % 2);
        }, 1, 4),
      &digits[10]);  // use -1 as default
  typedef ListBaseType::SizeArray SizeArray;
  EXPECT_EQ((SizeArray({8, 4})), c2_0_nogap.sizes());
  EXPECT_EQ((SizeArray({0, 0})), c2_0_nogap.nesting_offset(0));
  EXPECT_EQ((SizeArray({3, 0})), c2_0_nogap.nesting_offset(1));
  EXPECT_EQ((SizeArray({7, 0})), c2_0_nogap.nesting_offset(2));
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
  Chain<ListBaseType, 1> c2_1(
      ListVector<ListBaseType>()
      .Append([&](unsigned col, unsigned row) {
          return digits + (1 + col) * (row + col == 2);
        }, 3, 3)
      .Append([&](unsigned col, unsigned row) {
          return digits + (4 + col) * (row == col % 2);
        }, 4, 2)
      .Append([&](unsigned col, unsigned row) {
          return digits + 8 * (row == 1);
        }, 1, 2)
      .Append([&](unsigned col, unsigned row) {
          return digits + 9 * (row == 1 && col == 0);
        }, 2, 2),
      &digits[10],  // use -1 as default
      {{0, {2}}, {2, {1}}, {3, {3}}},
      {{2, 1}});
  typedef ListBaseType::SizeArray SizeArray;
  EXPECT_EQ((SizeArray({5, 10})), c2_1.sizes());
  EXPECT_EQ((SizeArray({2, 0})), c2_1.nesting_offset(0));
  EXPECT_EQ((SizeArray({0, 3})), c2_1.nesting_offset(1));
  EXPECT_EQ((SizeArray({1, 6})), c2_1.nesting_offset(2));
  EXPECT_EQ((SizeArray({3, 8})), c2_1.nesting_offset(3));
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
}
