#include "../../src/util/poly_vector.hpp"
#include "../test.hpp"

#include <algorithm>
#include <string>

struct ReversedString : public std::string {
  ReversedString(size_type count, char ch)
      : ReversedString(std::string(count, ch)) {}
  ReversedString(const std::string& s) : std::string(s.rbegin(), s.rend()) {}
  ReversedString() {}
};

struct SortedString : public std::string {
  SortedString(size_type count, char ch)
      : std::string(count, ch) {}
  SortedString(const std::string& s) : std::string(s) {
    std::sort(begin(), end());
  }
  SortedString() {}
};

typedef ::testing::Types<
  std::string,
  ReversedString,
  SortedString
  > StringTypes;
TYPED_TEST_CASE(PolyVectorTest, StringTypes);

template<typename T>
struct PolyVectorTest : public ::testing::Test {
};

#define V__POLYVEC_APP(type, arg, ...)               \
  template Append<type>(arg, ##__VA_ARGS__)

TYPED_TEST(PolyVectorTest, StringTypes) {
  PolyVector<TypeParam, std::string> pv;

  pv.template Append<TypeParam>(3, 'a');
  EXPECT_EQ("aaa", pv[0]);

  pv.V__POLYVEC_APP(TypeParam, "bb");
  EXPECT_EQ("bb", pv[1]);
}

TEST(PolyVectorSameClassTest, ReversedString) {
  ReversedString foo("foo");
  PolyVector<ReversedString, std::string> pv;
  pv.template Append<ReversedString>("bats");
  EXPECT_EQ("stab", pv[0]);
}
