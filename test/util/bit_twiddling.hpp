#include "../src/util/bit_twiddling.hpp"
#include "test.hpp"

#include <numeric>

namespace {

template<typename T, unsigned bits>
struct Pow2RoundUpInlineFunc {
  T operator()(T x) const;
};

#define POW2RUI_SPEC_BEGIN(bits) \
  template<typename T>                    \
  struct Pow2RoundUpInlineFunc<T, bits> { \
  T operator()(T x) const {               \
  --x;

#define POW2RUI_XORSR(bits) x |= x >> bits
#define POW2RUI_SPEC_BODY_1
#define POW2RUI_SPEC_BODY_2 POW2RUI_SPEC_BODY_1 POW2RUI_XORSR(1);
#define POW2RUI_SPEC_BODY_4 POW2RUI_SPEC_BODY_2 POW2RUI_XORSR(2);
#define POW2RUI_SPEC_BODY_8 POW2RUI_SPEC_BODY_4 POW2RUI_XORSR(4);
#define POW2RUI_SPEC_BODY_16 POW2RUI_SPEC_BODY_8 POW2RUI_XORSR(8);
#define POW2RUI_SPEC_BODY_32 POW2RUI_SPEC_BODY_16 POW2RUI_XORSR(16);
#define POW2RUI_SPEC_BODY_64 POW2RUI_SPEC_BODY_32 POW2RUI_XORSR(32);

#define POW2RUI_SPEC_END \
  return ++x;            \
}                        \
};

POW2RUI_SPEC_BEGIN(1) POW2RUI_SPEC_BODY_1 POW2RUI_SPEC_END
POW2RUI_SPEC_BEGIN(2) POW2RUI_SPEC_BODY_2 POW2RUI_SPEC_END
POW2RUI_SPEC_BEGIN(4) POW2RUI_SPEC_BODY_4 POW2RUI_SPEC_END
POW2RUI_SPEC_BEGIN(8) POW2RUI_SPEC_BODY_8 POW2RUI_SPEC_END
POW2RUI_SPEC_BEGIN(16) POW2RUI_SPEC_BODY_16 POW2RUI_SPEC_END
POW2RUI_SPEC_BEGIN(32) POW2RUI_SPEC_BODY_32 POW2RUI_SPEC_END
POW2RUI_SPEC_BEGIN(64) POW2RUI_SPEC_BODY_64 POW2RUI_SPEC_END

#undef POW2RUI_XORSR
#undef POW2RUI_SPEC_BODY_1
#undef POW2RUI_SPEC_BODY_2
#undef POW2RUI_SPEC_BODY_4
#undef POW2RUI_SPEC_BODY_8
#undef POW2RUI_SPEC_BODY_16
#undef POW2RUI_SPEC_BODY_32
#undef POW2RUI_SPEC_BEGIN
#undef POW2RUI_SPEC_END

}  // namespace

template<typename T>
inline T Pow2RoundUpInline(T x) {
  static_assert(std::is_integral<T>::value, "Not an integral type.");
  return Pow2RoundUpInlineFunc<T, sizeof(T) * CHAR_BIT>()(x);
}

template<bool inln>
struct Pow2RoundUpWrapper {};

template<>
struct Pow2RoundUpWrapper<false> {
  template<typename T>
  constexpr T operator()(T x) const { return Pow2RoundUp(x); }
};

template<>
struct Pow2RoundUpWrapper<true> {
  template<typename T>
  T operator()(T x) const { return Pow2RoundUpInline(x); }
};

typedef ::testing::Types<
  std::false_type,
  std::true_type> Inline;

template<typename T>
class BitTwiddlingTestBase : public ::testing::Test {};
