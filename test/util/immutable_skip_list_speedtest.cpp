#include "../src/util/immutable_skip_list.hpp"
#include "test.hpp"

#include <algorithm>
#include <utility>
#include <vector>
#include <type_traits>

template<size_t bucket_count_, size_t sizes_sum_>
struct BucketSizeGetterBase {
  constexpr size_t bucket_count() const { return bucket_count_; }
  constexpr size_t sizes_sum() const { return sizes_sum_; }
 protected:
  BucketSizeGetterBase() : sizes_(bucket_count_) {}
  std::vector<size_t> sizes_;
};

template<size_t bucket_count, size_t sizes_sum, size_t min_size = 1>
struct UniformBucketSizeGetter
    : public BucketSizeGetterBase<bucket_count, sizes_sum> {
  UniformBucketSizeGetter() {
    static_assert(sizes_sum / bucket_count >= min_size, "avg_size >= min_size");

    // find max and period s.t. min + min+1 + ... + max + min + ... <= sum
    size_t max_size = min_size, period = 1;
    for (size_t iota_sum = max_size;
         (iota_sum += ++max_size) <= ++period * sizes_sum / bucket_count; )
      ;
    --max_size;
    --period;

    // fill the sizes as follows: min, min+1, ..., max, min, ...
    auto it = this->sizes_.begin();
    assert(this->sizes_.size() == bucket_count);
    for (auto itr = bucket_count / period; itr--; it += period)
      std::iota(it, it + period, min_size);
    if (it < this->sizes_.end())
      std::iota(it, this->sizes_.end(), min_size);

    // adjust the last element s.t. sum of sizes == sizes_sum
    auto sum = std::accumulate(this->sizes_.begin(), this->sizes_.end(),
                               size_t(0));
    assert(sum <= sizes_sum);
    this->sizes_.back() += sizes_sum - sum;
  }

  inline size_t operator()(size_t index) const { return this->sizes_[index]; }
};

TEST(ImmutableSkipListMetatest, UniformBucketSizeGetter) {
  UniformBucketSizeGetter<8, 12> ubsg_4_12;
  EXPECT_EQ(1, ubsg_4_12(0));
  EXPECT_EQ(2, ubsg_4_12(1));
  EXPECT_EQ(1, ubsg_4_12(2));
  EXPECT_EQ(1, ubsg_4_12(6));
  EXPECT_EQ(2, ubsg_4_12(7));

  UniformBucketSizeGetter<5, 15, 2> ubsg_5_15_2;
  EXPECT_EQ(2, ubsg_5_15_2(0));
  EXPECT_EQ(3, ubsg_5_15_2(1));
  EXPECT_EQ(4, ubsg_5_15_2(2));
  EXPECT_EQ(2, ubsg_5_15_2(3));
  EXPECT_EQ(4, ubsg_5_15_2(4));

  UniformBucketSizeGetter<3, 100, 32> ubsg_3_100_32;
  EXPECT_EQ(32, ubsg_3_100_32(0));
  EXPECT_EQ(33, ubsg_3_100_32(1));
  EXPECT_EQ(35, ubsg_3_100_32(2));

  UniformBucketSizeGetter<3, 100, 33> ubsg_3_100_33;
  EXPECT_EQ(33, ubsg_3_100_33(0));
  EXPECT_EQ(33, ubsg_3_100_33(1));
  EXPECT_EQ(34, ubsg_3_100_33(2));
}

template<size_t bucket_count, size_t sizes_sum>
struct ConstBucketSizeGetter
    : public BucketSizeGetterBase<bucket_count, sizes_sum> {
  ConstBucketSizeGetter() {
    static_assert(sizes_sum % bucket_count == 0 && sizes_sum >= bucket_count,
                  "sizes_sum not a multiple of bucket_count");
    std::fill_n(this->sizes_.begin(), bucket_count, sizes_sum / bucket_count);
  }

  inline size_t operator()(size_t index) const { return this->sizes_[index]; }
};

template<size_t bucket_count, size_t sizes_sum>
struct CachedConstBucketSizeGetter
    : public BucketSizeGetterBase<bucket_count, sizes_sum> {
  inline size_t operator()(size_t index) const { return kSize; }
 private:
  const size_t kSize = sizes_sum / bucket_count;
};

class ImmutableSkipListSpeedtestBase : public ::testing::Test {
 protected:
  void TearDown() { EXPECT_NE(hash_, 0xcafebabe); }

  template<class L>
  inline void Get(const L& l,
                  size_t global_index, size_t offset = 0) {
    auto p = l.get(global_index, offset);
    hash_ = hasher_((p.first << 4) ^ p.second) + hash_ * 31;
  }

  std::hash<size_t> hasher_;
  size_t hash_;
};

// Speed test case to validate the hypothesis that the distribution of
// bucket sizes does not have impact on the search time.

template<class BucketSizeGetter>
class ImmutableSkipListSizesSpeedtest : public ImmutableSkipListSpeedtestBase {
};

#define kBucketCount (size_t(1) << 6)
static_assert(IsPow2(kBucketCount), "not a power of 2");
#define kSizesSum (kBucketCount << 2)
static_assert(IsPow2(kSizesSum), "not a power of 2");

typedef ::testing::Types<
  CachedConstBucketSizeGetter<kBucketCount, kSizesSum>,  // 4, 4, ... (no read)
  ConstBucketSizeGetter<kBucketCount, kSizesSum>,  // 4, 4, ...
  UniformBucketSizeGetter<kBucketCount, kSizesSum, 3>,  // 3, 4, 5, 3, ...
  UniformBucketSizeGetter<kBucketCount, kSizesSum, 1>,  // 1, 2, ..., 7, 1, ...
#define RS 2
  CachedConstBucketSizeGetter<kBucketCount, (kSizesSum << RS)>,
  ConstBucketSizeGetter<kBucketCount, (kSizesSum << RS)>,
  UniformBucketSizeGetter<kBucketCount, (kSizesSum << RS), ((3 + 1) << RS) - 1>,
  UniformBucketSizeGetter<kBucketCount, (kSizesSum << RS), 1>,
#undef RS
  CachedConstBucketSizeGetter<kBucketCount, (1 << 20)>
  > BucketSizeGetterTypes;
TYPED_TEST_CASE(ImmutableSkipListSizesSpeedtest, BucketSizeGetterTypes);
#undef kBucketCount
#undef kSizesSum

TYPED_TEST(ImmutableSkipListSizesSpeedtest, Get10MSequential) {
  TypeParam bsg;
  ImmutableSkipList<TypeParam> l(bsg.bucket_count(), bsg);
  for (int itr = 0; itr < 10000000; ++itr) {
    this->Get(l, itr & (bsg.sizes_sum() - 1));
  }
}

TYPED_TEST(ImmutableSkipListSizesSpeedtest, Get10MRandom) {
  TypeParam bsg;
  ImmutableSkipList<TypeParam> l(bsg.bucket_count(), bsg);
  size_t last = 1;
  for (int itr = 0; itr < 10000000; ++itr) {
    this->Get(l, last & (bsg.sizes_sum() - 1));
    last = itr + last * 65521;
  }
}

const int gLastPrime = 65521;

class ImmutableSkipListSpeedtestOverhead
    : public ImmutableSkipListSpeedtestBase {};

struct FakeImmutableSkipList {
  std::pair<size_t, size_t> get(size_t global_index, size_t offset) const {
    return std::make_pair(global_index, offset);
  }
};

TEST_F(ImmutableSkipListSpeedtestOverhead, Get10RandomOverhead) {
  FakeImmutableSkipList l;
  size_t last = 1;
  for (int itr = 0; itr < 10000000; ++itr) {
    Get(l, last & ((1 << 5) - 1));
    last = itr + last * gLastPrime;
  }
}

class ImmutableSkipListSpeedtest
    : public ::testing::WithParamInterface<size_t>,
      public ImmutableSkipListSpeedtestBase {
 protected:
  template<class L>
  void GetRandom(const L& l, int count) {
    size_t last = 1;
    for (int itr = 0; itr < count; ++itr) {
      this->Get(l, last & (sizes_sum() - 1));
      last = itr + last * gLastPrime;
    }
  }

  struct Pow2BucketSizeGetter {
    constexpr size_t operator()(size_t index) const {
      // ...00 -> 2 + 2     = 4
      // ...01 -> 2 + 3 + 1 = 6
      // ...10 -> 2 + 0     = 2
      // ...11 -> 2 + 1 + 1 = 4
      // ===== -> 8 + 6 + 2 = 16 = (1 << 4)
      return 2 + ((index & 0x3) ^ 0x2) + (index & 0x1);
    }
  };

  void SetUp() {
    constexpr size_t k4SizesSum =
        Pow2BucketSizeGetter()(0) +
        Pow2BucketSizeGetter()(1) +
        Pow2BucketSizeGetter()(2) +
        Pow2BucketSizeGetter()(3);
    static_assert(IsPow2(k4SizesSum), "sum of sizes not a power of 2");

    bucket_count_ = GetParam();
    sizes_sum_ = bucket_count_ * (k4SizesSum >> 2);
  }

  inline size_t bucket_count() const { return bucket_count_; }
  inline size_t sizes_sum() const { return sizes_sum_; }

  Pow2BucketSizeGetter bsg_;

 private:
  size_t bucket_count_;
  size_t sizes_sum_;
};
INSTANTIATE_TEST_CASE_P(LgBucketCount, ImmutableSkipListSpeedtest,
                        ::testing::Values(
                            size_t(1) << 0,
                            size_t(1) << 1,
                            size_t(1) << 2,
                            size_t(1) << 3,
                            size_t(1) << 4,
                            size_t(1) << 5,
                            size_t(1) << 6,
                            size_t(1) << 7,
                            size_t(1) << 8));

TEST_P(ImmutableSkipListSpeedtest, Get10MRandom) {
  auto l = ImmutableSkipList<decltype(bsg_)>(bucket_count(), bsg_);
  this->GetRandom(l, 10000000);
}

class BigImmutableSkipListSpeedtest : public ImmutableSkipListSpeedtest {};
INSTANTIATE_TEST_CASE_P(LgBucketCountMin14Div2, BigImmutableSkipListSpeedtest,
                        ::testing::Values(
                            size_t(1) << 14,
                            size_t(1) << 16,
                            size_t(1) << 18,
                            size_t(1) << 20,
                            size_t(1) << 22,
                            size_t(1) << 24,
                            size_t(1) << 26));

TEST_P(BigImmutableSkipListSpeedtest, Constructor) {
  auto l = ImmutableSkipList<decltype(bsg_)>(bucket_count(), bsg_);
  this->Get(l, bucket_count() / 2);     // use l to prevent optimization
}

TEST_P(BigImmutableSkipListSpeedtest, Get1MRandom) {
  auto l = ImmutableSkipList<decltype(bsg_)>(bucket_count(), bsg_);
  this->GetRandom(l, 1000000);
}

class HugeImmutableSkipListSpeedtest : public ImmutableSkipListSpeedtest {};
INSTANTIATE_TEST_CASE_P(LgBucketCountMin25, HugeImmutableSkipListSpeedtest,
                        ::testing::Values(
                            size_t(1) << 18,
                            size_t(1) << 19,
                            size_t(1) << 20,
                            size_t(1) << 21,
                            size_t(1) << 22));

TEST_P(HugeImmutableSkipListSpeedtest, Get500KRandom) {
  auto l = ImmutableSkipList<decltype(bsg_)>(bucket_count(), bsg_);
  this->GetRandom(l, 500000);
}

class TinyImmutableSkipListSpeedtest : public ImmutableSkipListSpeedtest {};
INSTANTIATE_TEST_CASE_P(LgBucketCount, TinyImmutableSkipListSpeedtest,
                        ::testing::Values(
                            size_t(1) << 1,
                            size_t(1) << 2,
                            size_t(1) << 3,
                            size_t(1) << 4));

TEST_P(TinyImmutableSkipListSpeedtest, Get200MRandom) {
  auto l = ImmutableSkipList<decltype(bsg_)>(bucket_count(), bsg_);
  this->GetRandom(l, 200000000);
}
