#ifndef CPPVIEWS_SRC_UTIL_BIT_TWIDDLING_HPP_
#define CPPVIEWS_SRC_UTIL_BIT_TWIDDLING_HPP_

#include <limits>
#include <type_traits>

#include <climits>
#include <cstddef>

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

// Inspired by HD nlz2: http://www.hackersdelight.org/hdcodetxt/nlz.c.txt
template<typename T, unsigned rs = (sizeof(T) >> 1) * CHAR_BIT>
struct FindFirstSetFunc {
  constexpr unsigned operator()(T x) const {
    return x >> rs ?
        FindFirstSetFunc<T, (rs >> 1)>()(x >> rs) + rs :
        FindFirstSetFunc<T, (rs >> 1)>()(x);
  }
};

template<typename T>
struct FindFirstSetFunc<T, 1> {
  constexpr unsigned operator()(T x) const { return x >> 1 ? 2 : x; }
};

}  // namespace

// Computes the smallest power of 2 greater than or equal to an integral type x,
// or 0 if x is non-positive.
template<typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type
constexpr Pow2RoundUp(T x) {
  return Pow2RoundUpFunc<T>()(x - 1) + 1;
}

// Computes the position of the Least Significant Bit set in x.
// Note: undefined if x is zero
template<typename T>
typename std::enable_if<std::is_unsigned<T>::value, unsigned>::type
constexpr FindFirstSet(T x) { return FindFirstSetFunc<T>()(x); }

// Checks whether x is a power of 2 or zero.
template<typename T>
typename std::enable_if<std::is_unsigned<T>::value, bool>::type
constexpr IsPow2(T x) { return (x & (x - 1)) == 0; }

// Checks whether x is a power of 2 (but not zero, unlike IsPow2).
template<typename T>
typename std::enable_if<std::is_unsigned<T>::value, bool>::type
constexpr IsPow2Zero(T x) { return x && IsPow2(x); }

// Checks whether the conversion to the same-size signed type is less than 0.
template<typename T>
typename std::enable_if<std::is_unsigned<T>::value, T>::type
constexpr IsSignedNegative(const T& x) {
  return x & (T(1) << (std::numeric_limits<T>::digits - 1));
}

#endif  /* CPPVIEWS_SRC_UTIL_BIT_TWIDDLING_HPP_ */
