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
  Chain<ListBase<int, 2>, 0> c2_0_nogap(
      ListVector<ListBase<int, 2> >()
      .Append([&](unsigned row, unsigned col) {
          return digits + (1 + col) * (row == col);
        }, 3, 3)
      .Append([&](unsigned row, unsigned col) {
          return digits + (4 + col) * ((!row && col < 2) || (row && col >= 2));
        }, 2, 4)
      .Append([&](unsigned row, unsigned col) {
          return digits + (8 + (row + 1) / 2) * (col % 2);
        }, 4, 1),
      &digits[10]);  // use -1 as default
  EXPECT_EQ((std::array<size_t, 2>({4, 8})), c2_0_nogap.sizes());
}
