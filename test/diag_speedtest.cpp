#include "../src/diag.hpp"
#include "test.hpp"

TEST(DiagSpeedtest, SmallHalfDefault) {
  // 1 1 0
  // 1 1 0
  // 1 1 0
  // 0 0 1
  static int zero = 0;
  Diag<int, unsigned, 2, 3> d(&zero, 3, 4);
  for (unsigned r = 0; r < d.block_size(0); ++r)
    for (unsigned c = 0; c < d.block_size(1); ++c)
      d(r, c) = 1;

  int chksum = 0;
  for (int i = 0; i < 50000000; ++i) {
    int rnd = rand();
    for (int j = 0; j < 4; ++j, rnd >>= 4) {
      int c = rnd & 0xF, r = c >> 2;
      if (c >= 12) continue;
      c &= 3;
      chksum += d(r, c);
      // chksum += d(size_t(r), size_t(c));
    }
  }
  EXPECT_NE(0, chksum);
}
