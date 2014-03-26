#include "bit_twiddling.hpp"

#include <cstdlib>
#include <functional>

template<typename T>
class BitTwiddlingSpeedtest : public BitTwiddlingTestBase<T> {
};

TYPED_TEST_CASE(BitTwiddlingSpeedtest, Inline);

TYPED_TEST(BitTwiddlingSpeedtest, UnsignedShort1K) {
  using namespace std;
  Pow2RoundUpWrapper<TypeParam::value> pow2_round_up;
  // EXPECT_EQ(1, pow2_round_up(0));
  unsigned short i;
  auto i_max_half = (numeric_limits<decltype(i)>::max() >> 1) + 1;

  size_t checksum = 0;
  hash<decltype(i)> h;
  for (int itr = 0; itr < 1000; ++itr) {
    for (i = 1; i <= i_max_half; ++i) {
      auto r = static_cast<decltype(i)>(rand());
      checksum += h(pow2_round_up(r));
    }
  }
  ASSERT_NE(1000, checksum);
}
