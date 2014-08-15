#include "../src/chain.hpp"
#include "../src/diag.hpp"
#include "test.hpp"

TEST(DiagChainTest, PolyValueIterNoEmpty) {
  static int default_val = -1;
  Chain<ListBase<int, 2>, 0> c(
      ListVector<ListBase<int, 2> >()
      .Append(DiagTag<unsigned, 1, 1>(), &default_val, 3, 3)
      .Append(DiagTag<unsigned, 1, 1>(), &default_val, 1, 3)
      .Append(DiagTag<unsigned, 1, 1>(), &default_val, 2, 3)
      , &default_val);
  c(0, 0) = 10, c(1, 1) = 20, c(2, 2) = 30;
  c(3, 0) = 40;
  c(4, 0) = 50, c(5, 1) = 60;

  EXPECT_EQ(6, c.values().size());
  auto vit = c.values().begin();
  EXPECT_EQ(c.values().begin(), vit);
  EXPECT_NE(++c.values().begin(), vit);
  EXPECT_EQ(10, *vit++);
  EXPECT_EQ(20, *vit);
  EXPECT_EQ(30, *++vit);
  EXPECT_EQ(40, *++vit);
  EXPECT_EQ(50, *++vit);
  EXPECT_EQ(60, *++vit);
  EXPECT_NE(c.values().end(), vit);
  EXPECT_EQ(c.values().end(), ++vit);
  EXPECT_EQ(c.values().end(), c.values().end());
}
