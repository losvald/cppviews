#include "test.hpp"

#include "../../src/util/intseq.hpp"

#include <array>

using namespace std;

constexpr int Sum() { return 0; }

template<typename T, typename... Args>
constexpr T Sum(T arg, Args... args) { return arg + Sum(args...); }

template<typename T, size_t N, T... Is>
constexpr T ArraySum(const array<T, N>& a, cpp14::integer_sequence<T, Is...>) {
  return Sum(std::get<Is>(a)...);
}

template<typename T, size_t N>
constexpr T ArraySum(const array<T, N>& a) {
  return ArraySum(a, cpp14::make_integer_sequence<T, N>());
}

TEST(Intseq, MakeIntegerSequence) {
  ASSERT_EQ(6, Sum(3, 1, 2));

  array<size_t, 4> a4 = {5, 3, 1, 2};
  EXPECT_EQ(11, ArraySum(a4));

  array<int, 1> a1 = {6};
  EXPECT_EQ(6, ArraySum(a1));

  array<unsigned, 2> a2 = {7, 3};
  EXPECT_EQ(10, ArraySum(a2));
}
