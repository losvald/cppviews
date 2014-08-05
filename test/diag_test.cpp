#include "../src/diag.hpp"
#include "test.hpp"

#include <type_traits>

typedef ::testing::Types<
  std::integral_constant<unsigned, 3>,
  std::integral_constant<size_t, 4>,
  std::integral_constant<unsigned, 1> > Rhs;
TYPED_TEST_CASE(DiagDimScalerTest, Rhs);

template<class IC>
class DiagDimScalerTest : public ::testing::Test {
};

TYPED_TEST(DiagDimScalerTest, Mult) {
  v::detail::DiagDimScaler<typename TypeParam::value_type, TypeParam::value> s;
#define EXPECT_MULTIPLIES(x) EXPECT_EQ((x) * TypeParam::value, (x) * s)
  EXPECT_MULTIPLIES(0u);
  EXPECT_MULTIPLIES(1);
  EXPECT_MULTIPLIES(2);
  EXPECT_MULTIPLIES(3);
  EXPECT_MULTIPLIES(5);
  EXPECT_MULTIPLIES(7);
  EXPECT_MULTIPLIES(42);
  EXPECT_MULTIPLIES(19937);
  EXPECT_MULTIPLIES(static_cast<typename TypeParam::value_type>(-1));
#undef EXPECT_MULTIPLIES
}

TYPED_TEST(DiagDimScalerTest, Div) {
  v::detail::DiagDimScaler<typename TypeParam::value_type, TypeParam::value> s;
#define EXPECT_DIVIDES(x) EXPECT_EQ((x) / TypeParam::value, (x) / s)
  EXPECT_DIVIDES(0);
  EXPECT_DIVIDES(1);
  EXPECT_DIVIDES(2);
  EXPECT_DIVIDES(3);
  EXPECT_DIVIDES(5);
  EXPECT_DIVIDES(7);
  EXPECT_DIVIDES(14);
  EXPECT_DIVIDES(16);
  EXPECT_DIVIDES(42);
  EXPECT_DIVIDES(19937);
  EXPECT_DIVIDES(static_cast<typename TypeParam::value_type>(-1));
#undef EXPECT_DIVIDES
}

TEST(DiagTest, DiagBlock) {
  v::detail::DiagBlock<int*, unsigned, 3, 2> b;
  int digits[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  b(0, 0) = &digits[0], b(0, 1) = &digits[1];
  b(1, 0) = &digits[2], b(1, 1) = &digits[3];
  b(2, 0) = &digits[4], b(2, 1) = &digits[5];
  EXPECT_EQ(digits + 0, b(0, 0));
  EXPECT_EQ(digits + 1, b(0, 1));
  EXPECT_EQ(digits + 2, b(1, 0));
  EXPECT_EQ(digits + 3, b(1, 1));
  EXPECT_EQ(digits + 4, b(2, 0));
  EXPECT_EQ(digits + 5, b(2, 1));
}

TEST(DiagTest, ConstructorBlockCount4D) {
  Diag<void*, size_t, 7, 4, 1, 3> d_7_4_1_3(5);
  EXPECT_EQ(5, d_7_4_1_3.block_count());
  EXPECT_EQ(decltype(d_7_4_1_3)::SizeArray({35, 20, 5, 15}), d_7_4_1_3.sizes());
}

TEST(DiagTest, Unsigned2D) {
  static double default_val = 0;
  Diag<double, unsigned, 2, 3> d_2_3(&default_val, 10, 10);
  EXPECT_EQ(2, d_2_3.block_size<0>());
  EXPECT_EQ(3, d_2_3.block_size<1>());
  EXPECT_EQ(4, d_2_3.block_count());
  EXPECT_EQ(d_2_3.block_size(0), d_2_3.block_size<0>());
  EXPECT_EQ(d_2_3.block_size(1), d_2_3.block_size<1>());
  EXPECT_EQ(100, d_2_3.size());
  d_2_3(0, 0) = 0, d_2_3(0, 1) = 1, d_2_3(0, 2) = 2;
  d_2_3(1, 0) = 3, d_2_3(1, 1) = 4, d_2_3(1, 2) = 5;
  EXPECT_EQ(1, d_2_3(0, 1));
  EXPECT_EQ(4, d_2_3(1, 1));
  EXPECT_EQ(5, d_2_3(1, 2));

  // verify that writing to other blocks doesn't interfere with the first
  d_2_3(2, 3) = 6, d_2_3(2, 4) = 7, d_2_3(2, 5) = 8;
  d_2_3(3, 3) = 9, d_2_3(3, 4) = 10, d_2_3(3, 5) = 11;
  d_2_3(5, 8) = 99;
  EXPECT_EQ(1, d_2_3(0, 1));
  EXPECT_EQ(4, d_2_3(1, 1));
  EXPECT_EQ(5, d_2_3(1, 2));
  EXPECT_EQ(7, d_2_3(2, 4));
  EXPECT_EQ(10, d_2_3(3, 4));
  EXPECT_EQ(11, d_2_3(3, 5));

  // validate changes to the default value
  EXPECT_EQ(0, d_2_3(2, 2));
  d_2_3(2, 2) = 22;
  EXPECT_EQ(22, d_2_3(2, 2));
  d_2_3(4, 1) = 41;  // change the default value by access to another element
  EXPECT_EQ(41, d_2_3(2, 2));
}

TEST(DiagTest, SizeT3D) {
  static int default_val = -1;
  Diag<int, size_t, 4, 1, 99> d_99_1(&default_val, 24, 7, 100000);
  EXPECT_EQ(4, d_99_1.block_size<0>());
  EXPECT_EQ(1, d_99_1.block_size(1));
  EXPECT_EQ(99, d_99_1.block_size<2>());
  typedef decltype(d_99_1)::SizeArray SizeArray;
  EXPECT_EQ(SizeArray({24, 7, 100000}), d_99_1.sizes());
  EXPECT_EQ(24 * 7 * 100000, d_99_1.size());
  EXPECT_EQ(6, d_99_1.block_count());

  // verify some elements with default values
  EXPECT_EQ(-1, d_99_1(4, 0, 98));  // off by 1 in dim 0
  EXPECT_EQ(-1, d_99_1(3, 1, 98));  // off by 1 in dim 1
  EXPECT_EQ(-1, d_99_1(3, 0, 99));  // off by 1 in dim 2
  EXPECT_EQ(-1, d_99_1(8, 2, 197)); // off by -1 in dim 2, close to block 2
  EXPECT_EQ(-1, d_99_1(19, 5, 500));  // off by -1 in dim 0, middle dim 2
  EXPECT_EQ(-1, d_99_1(23, 6, 99999));  // max indexes (not on the diagonal)
}

TEST(DiagTest, SizeT2D1x1) {
  static int default_val = -1;
  Diag<int, size_t, 1, 1> d_1_1(&default_val, 3, 3);
  EXPECT_EQ(1, d_1_1.block_size<0>());
  EXPECT_EQ(1, d_1_1.block_size(1));
  for (int i = 0; i < 3; ++i)
    d_1_1(i, i) = i;

  EXPECT_EQ(0, d_1_1(0, 0));
  EXPECT_EQ(1, d_1_1(1, 1));
  EXPECT_EQ(2, d_1_1(2, 2));
}

TEST(DiagTest, MakeList) {
  static int default_val = -1;
  auto d = MakeList(DiagTag<unsigned, 5, 8>(), &default_val, 10, 40);
  EXPECT_EQ(400, d.size());
  EXPECT_EQ(5, d.block_size<0>());
  EXPECT_EQ(8, d.block_size(1));
}

TEST(DiagTest, PolyVectorAppend) {
  static int default_val = -1;
  ListVector<ListBase<int, 3> > lv;

  lv.Append(DiagTag<int, 1, 2, 4>(), &default_val, 2, 4, 8);
  EXPECT_EQ(2, (static_cast<Diag<int, int, 1, 2, 4>&>(lv[0]).block_count()));

  lv.Append(MakeList(DiagTag<int, 3, 3, 3>(), &default_val, 7, 7, 7));
  EXPECT_EQ(3, (static_cast<Diag<int, int, 3, 3, 3>&>(lv[1]).block_count()));
}
