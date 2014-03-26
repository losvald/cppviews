#include "../src/util/immutable_skip_list.hpp"
#include "test.hpp"

TEST(ImmutableSkipListTest, ConstructFull) {
  ImmutableSkipList<> sl(8);
  EXPECT_EQ(8, sl.bucket_count());
  ASSERT_EQ(7, sl.skip_count_max_index());
  EXPECT_EQ(2, sl.skip_count(7));
  EXPECT_EQ(2, sl.skip_count(6));
  EXPECT_EQ(2, sl.skip_count(5));
  EXPECT_EQ(2, sl.skip_count(4));
  EXPECT_EQ(4, sl.skip_count(3));
  EXPECT_EQ(4, sl.skip_count(2));
  EXPECT_EQ(8, sl.skip_count(1));

  ImmutableSkipList<> sl2(2);
  EXPECT_EQ(2, sl2.bucket_count());
  ASSERT_EQ(1, sl2.skip_count_max_index());
  EXPECT_EQ(2, sl2.skip_count(1));
}

TEST(ImmutableSkipListTest, ConstructCompleteEven) {
  ImmutableSkipList<> sl(6);
  EXPECT_EQ(6, sl.bucket_count());
  ASSERT_EQ(7, sl.skip_count_max_index());
  EXPECT_EQ(0, sl.skip_count(7));
  EXPECT_EQ(2, sl.skip_count(6));
  EXPECT_EQ(2, sl.skip_count(5));
  EXPECT_EQ(2, sl.skip_count(4));
  EXPECT_EQ(2, sl.skip_count(3));
  EXPECT_EQ(4, sl.skip_count(2));
  EXPECT_EQ(6, sl.skip_count(1));
}

TEST(ImmutableSkipListTest, ConstructCompleteOdd) {
  ImmutableSkipList<> sl(5);
  EXPECT_EQ(5, sl.bucket_count());
  ASSERT_EQ(7, sl.skip_count_max_index());
  EXPECT_EQ(0, sl.skip_count(7));
  EXPECT_EQ(1, sl.skip_count(6));
  EXPECT_EQ(2, sl.skip_count(5));
  EXPECT_EQ(2, sl.skip_count(4));
  EXPECT_EQ(1, sl.skip_count(3));
  EXPECT_EQ(4, sl.skip_count(2));
  EXPECT_EQ(5, sl.skip_count(1));

  ImmutableSkipList<> sl1(1);
  EXPECT_EQ(1, sl1.bucket_count());
  ASSERT_EQ(0, sl1.skip_count_max_index()); // TODO is this desired???
}

TEST(ImmutableSkipListTest, ConstructEmpty) {
  ImmutableSkipList<> sl(0);
  EXPECT_EQ(0, sl.bucket_count());
  ASSERT_EQ(-1, sl.skip_count_max_index()); // TODO is this desired???
}
