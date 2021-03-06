#include "bit_twiddling.hpp"

#include <cstdint>

template<typename T>
class BitTwiddlingInlineTest : public BitTwiddlingTestBase<T> {};

TYPED_TEST_CASE(BitTwiddlingInlineTest, Inline);

TYPED_TEST(BitTwiddlingInlineTest, Pow2RoundUpSizeT) {
  Pow2RoundUpWrapper<TypeParam::value> pow2_round_up;
  EXPECT_EQ(1, pow2_round_up(1));
  EXPECT_EQ(2, pow2_round_up(2));
  EXPECT_EQ(4, pow2_round_up(3));
  EXPECT_EQ(4, pow2_round_up(4));
  EXPECT_EQ(8, pow2_round_up(7));
  EXPECT_EQ(8, pow2_round_up(8));
  EXPECT_EQ(16, pow2_round_up(9));
  EXPECT_EQ(2097152, pow2_round_up(1234567));
  EXPECT_EQ(2097152, pow2_round_up(2097152));
  EXPECT_EQ(2097152 * 2, pow2_round_up(2097153));
  EXPECT_EQ(134217728, pow2_round_up(100000000));

  const size_t bit_count = std::numeric_limits<size_t>::digits;
  const size_t one = 1;
  EXPECT_EQ(0, pow2_round_up((one << (bit_count - 1)) + 1));
}

TYPED_TEST(BitTwiddlingInlineTest, Pow2RoundUpUnsignedShort) {
  using namespace std;
  Pow2RoundUpWrapper<TypeParam::value> pow2_round_up;
  EXPECT_EQ(0, pow2_round_up(0));

  unsigned short i;
  auto i_max_half = (numeric_limits<decltype(i)>::max() >> 1) + 1;
  for (i = 1; i <= i_max_half; ++i) {
    auto p = pow2_round_up(i);
    ASSERT_FALSE(p & (p - 1)) << "not power of 2: " << p << " (" << i << ')';
    ASSERT_LE(i, p);
    ASSERT_GT(i, p >> 1);
  }

  for (i = i_max_half + 1; i < (i_max_half - 1) + i_max_half; ++i)
    ASSERT_EQ(0, pow2_round_up(i));
}

TYPED_TEST(BitTwiddlingInlineTest, Pow2RoundUpShort) {
  using namespace std;
  Pow2RoundUpWrapper<TypeParam::value> pow2_round_up;
  EXPECT_EQ(0, pow2_round_up(0));

  short i;
  auto i_max_half = (numeric_limits<decltype(i)>::max() >> 1) + 1;
  for (i = 1; i <= i_max_half; ++i) {
    auto p = pow2_round_up(i);
    ASSERT_FALSE(p & (p - 1)) << "not power of 2: " << p << " (" << i << ')';
    ASSERT_LE(i, p);
    ASSERT_GT(i, p >> 1);
  }

  for (i = 1; i <= i_max_half + 1; ++i)
    ASSERT_EQ(0, pow2_round_up(-i));
}

TEST(BitTwiddlingTest, FindFirstSetSizeT) {
  EXPECT_EQ(1, FindFirstSet<size_t>(1));
  EXPECT_EQ(2, FindFirstSet<size_t>(3));
  EXPECT_EQ(3, FindFirstSet<size_t>(6));
  EXPECT_EQ(53, FindFirstSet<size_t>(size_t(1) << 52));
  EXPECT_EQ(64, FindFirstSet<size_t>(-1));
}

TEST(BitTwiddlingTest, FindFirstSetUnsignedShort) {
  const unsigned max_bits = std::numeric_limits<unsigned short>::digits;
  for (unsigned b = 1; b < max_bits; ++b) {
    for (unsigned short i = 1 << (b - 1); i < (1 << b); ++i)
      ASSERT_EQ(b, FindFirstSet(i)) << "Failed for: " << i;
  }
}

TEST(BitTwiddlingTest, IsSignedNegative) {
  EXPECT_FALSE(IsSignedNegative(static_cast<std::uint8_t>(17)));
  EXPECT_FALSE(IsSignedNegative(static_cast<std::uint8_t>(0)));
  EXPECT_FALSE(IsSignedNegative(static_cast<std::uint8_t>(127)));
  EXPECT_TRUE(IsSignedNegative(static_cast<std::uint8_t>(128)));
  EXPECT_TRUE(IsSignedNegative(static_cast<std::uint8_t>(197)));
  EXPECT_TRUE(IsSignedNegative(static_cast<std::uint8_t>(255)));

  EXPECT_FALSE(IsSignedNegative(static_cast<std::uint32_t>(987654321)));
  EXPECT_TRUE(IsSignedNegative(static_cast<std::uint32_t>(2314567890)));

  EXPECT_FALSE(IsSignedNegative(false));
  EXPECT_TRUE(IsSignedNegative(true));
}
