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

TEST(DiagTest, Unsigned2DLastNotFull) {
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

  // validate forward iteration over block values
  auto vit = d_2_3.values().begin();
  EXPECT_EQ(0, *vit++);
  EXPECT_EQ(1, *vit++);
  EXPECT_EQ(2, *vit++);
  for (int i = 0; i < 3; ++i)  // check the rest of the 1st block
    EXPECT_EQ(3 + i, *vit++);
  auto vit2 = d_2_3.values().begin();
  for (int i = 0; i < 6; ++i) // advance to the beginning of the 2nd block
    ++vit2;
  EXPECT_EQ(vit, vit2);
  for (int i = 6; i < 12; ++i) // advance to the beginning of the 3rd block
    EXPECT_EQ(i, *vit2++) << "*(d_2_3.values().begin() + " << i << ")";
  for (int i = 12; i < 18; ++i)  // advance to the last, non-full block
    ++vit2;
  d_2_3(6, 9) = 123;
  d_2_3(7, 9) = 456;

  EXPECT_EQ(123, *vit2++);
  EXPECT_EQ(456, *vit2++);
  EXPECT_EQ(d_2_3.values().end(), vit2);
  EXPECT_EQ(20, d_2_3.values().size());

  // validate changes to the default value
  EXPECT_EQ(0, d_2_3(2, 2));
  d_2_3(2, 2) = 22;
  EXPECT_EQ(22, d_2_3(2, 2));
  d_2_3(4, 1) = 41;  // change the default value by access to another element
  EXPECT_EQ(41, d_2_3(2, 2));
}

TEST(DiagTest, Unsigned2DSingleFullBlock) {
  Diag<double, unsigned, 4, 1> d_4_1(nullptr, 4, 1);
  for (unsigned i = 0; i < 4; ++i) d_4_1(i, 0) = 10 * (i + 1);

  // validate forward iteration over block values
  auto vit = d_4_1.values().begin();
  EXPECT_EQ(10, *vit++);
  EXPECT_EQ(20, *vit++);
  EXPECT_EQ(30, *vit++);
  EXPECT_EQ(40, *vit++);
  EXPECT_EQ(d_4_1.values().end(), vit);
  EXPECT_EQ(4, d_4_1.values().size());
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

  // validate forward iteration over block values
  auto vit = d_1_1.values().begin();
  EXPECT_EQ(0, *vit++);
  EXPECT_EQ(1, *vit);
  EXPECT_EQ(2, *++vit);
  EXPECT_NE(d_1_1.values().end(), vit++);
  EXPECT_EQ(d_1_1.values().end(), vit);
  EXPECT_EQ(3, d_1_1.values().size());

  // validate dimension iteration
  // d_1_1.dim_begin<0>(0) != d_1_1.dim_begin<1>(0);  // compile error - OK
  auto dit = d_1_1.dim_begin<0>(0);
  EXPECT_EQ(d_1_1.dim_begin<0>(0), dit);
  EXPECT_EQ(0, *dit++);
  EXPECT_EQ(-1, *dit);
  EXPECT_EQ(*d_1_1.dim_begin<0>(1), *dit);
  EXPECT_EQ(-1, *++dit);
  EXPECT_NE(d_1_1.dim_begin<0>(0), dit);
  auto cdit = d_1_1.dim_cbegin<0>(0);
  ++ ++ cdit;
  EXPECT_EQ(-1, *cdit);
  EXPECT_EQ(dit, cdit);
  ++dit;
  EXPECT_NE(dit, cdit);
  EXPECT_EQ(dit, d_1_1.dim_end<0>(0));
  cdit = dit;
  EXPECT_EQ(cdit, dit);
  EXPECT_EQ(cdit, d_1_1.dim_cend<0>(0));
  typedef decltype(dit) DIT;
  static_assert(!std::is_convertible<decltype(cdit), DIT>(), "");
  static_assert(!std::is_assignable<DIT, decltype(default_val)>(), "");
}

TEST(DiagTest, DiagonalDimIteration3D) {
  static char default_val = 0;
  Diag<char, unsigned, 1, 1, 1> d_1_1_1(&default_val, 5, 3, 4);
  for (unsigned i = 0; i < 3; ++i)
    d_1_1_1(i, i, i) = 1;
  auto begin0 = [&](unsigned c0, unsigned c1) {
    return d_1_1_1.dim_begin<0>(c0, c1);
  };
  auto end0 = [&](unsigned c0, unsigned c1) {
    return d_1_1_1.dim_end<0>(c0, c1);
  };

  // verify value iterations that pass through the diagonal
  for (unsigned c1 = 0; c1 < 3; ++c1) {
    auto dit = begin0(c1, c1), dit_end = end0(c1, c1);
    for (unsigned c2 = 0; c2 < 3; ++c2) {
      EXPECT_NE(dit, dit_end);
      EXPECT_EQ(c1 == c2, *dit++) << "dim_begin<0>(" << c1 << ", " << c1
                                  << ") + " << c2;
    }
    EXPECT_NE(dit, dit_end);
    EXPECT_NE(++dit, dit_end);
    EXPECT_EQ(++dit, dit_end);
  }

  // verify value iterations that never passes through the diagonal
  for (unsigned c1 = 0; c1 < 3; ++c1)
    for (unsigned c2 = 0; c2 < 3; ++c2) {
      if (c1 == c2)
        continue;
      auto dit = begin0(c1, c2), dit_end = end0(c1, c2);
      for (unsigned j = 0; j < 3; ++j) {
        EXPECT_NE(dit, dit_end);
        EXPECT_FALSE(*dit++) << "(dim_begin<0>(" << c1 << ", " << c2 << ") + "
                             << j << ") != 0";
      }
      EXPECT_NE(dit, dit_end);
      EXPECT_NE(++dit, dit_end);
      EXPECT_EQ(++dit, dit_end);
    }

  // verify value iteration that passess outside of the diagonal's bounding box
  {
    const unsigned c0 = 4, c1 = 2;
    auto dit = d_1_1_1.dim_begin<2>(c0, c1);
    auto dit_end = d_1_1_1.dim_end<2>(c0, c1);
    for (int i = 0; i < 4; ++i) {
      EXPECT_EQ(0, *dit);
      EXPECT_NE(dit++, dit_end) << "i = " << i;
    }
    EXPECT_EQ(dit, dit_end);
  }
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
