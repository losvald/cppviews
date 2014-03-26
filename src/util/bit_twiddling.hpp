#ifndef CPPVIEWS_SRC_UTIL_BIT_TWIDDLING_HPP_
#define CPPVIEWS_SRC_UTIL_BIT_TWIDDLING_HPP_

#include <climits>
#include <cstddef>
#include <type_traits>

namespace {

template<typename T, unsigned bits = sizeof(T) * CHAR_BIT, unsigned rs = 1>
struct Pow2RoundUpFunc {
  constexpr T operator()(T x) const {
    return Pow2RoundUpFunc<T, bits, (rs << 1)>()(x | (x >> rs));
  }
};

template<typename T, unsigned bits>
struct Pow2RoundUpFunc<T, bits, bits> {
  constexpr T operator()(T x) const { return x; }
};

}  // namespace

// Computes the smallest power of 2 greater than or equal to an integral type x,
// or 0 if x is non-positive.
template<typename T>
constexpr T Pow2RoundUp(T x) {
  static_assert(std::is_integral<T>::value, "Not an integral type.");
  return Pow2RoundUpFunc<T>()(x - 1) + 1;
}

#endif  /* CPPVIEWS_SRC_UTIL_BIT_TWIDDLING_HPP_ */
