#include "../src/util/immutable_skip_list.hpp"

#include "../src/util/bit_twiddling.hpp"
#include "test.hpp"

#include <utility>

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

  struct Getter {             // returns 2 and 3 for index 0 and 1, respectively
    size_t operator()(size_t index) const { return 2 + !!index; }
  };
  ImmutableSkipList<Getter> sl2(2);
  EXPECT_EQ(2, sl2.bucket_count());
  ASSERT_EQ(1, sl2.skip_count_max_index());
  EXPECT_EQ(5, sl2.skip_count(1));
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
  ASSERT_EQ(1, sl1.skip_count_max_index());
}

TEST(ImmutableSkipListTest, ConstructEmpty) {
  ImmutableSkipList<> sl(0);
  EXPECT_EQ(0, sl.bucket_count());
  ASSERT_EQ(0, sl.skip_count_max_index());
}

TEST(ImmutableSkipListTest, GetConstSizes1) {
  ImmutableSkipList<> sl(1 << 16);
  for (size_t ind = 0; ind < sl.bucket_count(); ++ind) {
    auto p_act = sl.get(ind);
    decltype(p_act) p_exp(ind, 0);
    ASSERT_EQ(p_exp, p_act) << "get(" << ind << ") incorrect";
  }
}

TEST(ImmutableSkipListTest, GetUniqueSizes) {
  struct BucketSizeGetter {
    size_t operator()(size_t bkt_ind) const { return 1 << bkt_ind; }
  } bsg;
  ImmutableSkipList<BucketSizeGetter> sl(16, bsg);

  for (size_t ind = 0; ind < (1U << sl.bucket_count()) - 1; ++ind) {
    auto p_act = sl.get(ind);
    size_t bkt_ind = FindFirstSet(ind + 1) - 1;
    decltype(p_act) p_exp(bkt_ind, ind - ((1 << bkt_ind) - 1));
    ASSERT_EQ(p_exp, p_act) << "get(" << ind << ") incorrect";
  }
}

TEST(ImmutableSkipListTest, GetOffsetConstSizes1) {
  ImmutableSkipList<> sl(1 << 9);
  for (size_t off = 0; off < sl.bucket_count(); ++off) {
    for (size_t ind = 0; ind < sl.bucket_count() - off; ++ind) {
      auto p_act = sl.get(ind, off);
      decltype(p_act) p_exp(ind + off, 0);
      ASSERT_EQ(p_exp, p_act) << "get(" << ind << ", " << off << ") incorrect";
    }
  }
}

TEST(ImmutableSkipListTest, GetOffsetUniqueSizes) {
  struct BucketSizeGetter {
    size_t operator()(size_t bkt_ind) const { return 1 << bkt_ind; }
  } bsg;
  ImmutableSkipList<BucketSizeGetter> sl(13, bsg);

  for (size_t off = 0; off < sl.bucket_count(); ++off) {
    size_t ind_to = (1U << sl.bucket_count()) - (1 << off);
    for (size_t ind = 0; ind < ind_to; ++ind) {
      auto p_act = sl.get(ind, off);
      size_t ind2 = ind + ((1 << off) - 1);
      size_t bkt_ind = FindFirstSet(ind2 + 1) - 1;
      decltype(p_act) p_exp(bkt_ind, ind2 - ((1 << bkt_ind) - 1));
      ASSERT_EQ(p_exp, p_act) << "get(" << ind << ", " << off << ") incorrect";
    }
  }
}

TEST(ImmutableSkipListTest, GetZeroSize) {
  struct BucketSizeGetter {
    size_t operator()(size_t bkt_ind) const {
      switch (bkt_ind) {
        case 0: return 0;
        case 1: return 2;
        case 2: return 1;
        case 3: return 0;
        case 4: return 0;
        case 5: return 2;
        case 6: return 0;
        default:
          ADD_FAILURE();
      }
    }
  };
  ImmutableSkipList<BucketSizeGetter> sl(7);
  typedef decltype(sl)::Position Pos;

  // validate it skips if the first bucket has size 0
  EXPECT_EQ(Pos(1, 0), sl.get(0));
  EXPECT_EQ(Pos(1, 1), sl.get(1));

  // validate it skips multiple zero buckets
  EXPECT_EQ(Pos(5, 0), sl.get(3));
  EXPECT_EQ(Pos(5, 1), sl.get(4));

  // validate the position is the beginning of the past-the-last bucket
  EXPECT_EQ(Pos(7, 0), sl.get(5));
}
