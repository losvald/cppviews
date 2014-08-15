#include "../src/chain.hpp"
#include "test.hpp"

namespace {

template<typename ValueType, typename KeyType>
void AssertValuesEmpty(const Map<ValueType, KeyType>& map) {
  ASSERT_EQ(0, map.values().size());
  ASSERT_EQ(map.values().end(), map.values().begin());
}

}  // namespace

TEST(ChainTest, PolyNoGapNoOffsets) {
  //    0 1 2 3 4 5 6 7
  //   +-----+-------+-+
  // 0 |1 0 0|4 5 0 0|0|
  // 1 |0 2 0|0 0 6 7|8|
  // 2 |0 0 3+-------+0|
  // 3 +-----+       |9|
  //                 +-+
  static int digits[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1};
  typedef ListBase<int, 2> ListBaseType;
  Chain<ListBase<int, 2>, 0> c2_0_nogap(
      ListVector<ListBase<int, 2> >()
      .Append([&](unsigned col, unsigned row) {
          return digits + (1 + col) * (row == col);
        }, 3, 3)
      .Append([&](unsigned col, unsigned row) {
          return digits + (4 + col) * ((!row && col < 2) || (row && col >= 2));
        }, 4, 2)
      .Append([&](unsigned col, unsigned row) {
          return digits + (7 + (row + 1) / 2) * (row % 2);
        }, 1, 4),
      &digits[10]);  // use -1 as default
  typedef ListBaseType::SizeArray SizeArray;
  EXPECT_EQ((SizeArray({8, 4})), c2_0_nogap.sizes());
  EXPECT_EQ((SizeArray({0, 0})), c2_0_nogap.nesting_offset(0));
  EXPECT_EQ((SizeArray({3, 0})), c2_0_nogap.nesting_offset(1));
  EXPECT_EQ((SizeArray({7, 0})), c2_0_nogap.nesting_offset(2));

  // validate all non-zero entries and a few other corner cases
  EXPECT_EQ(1, c2_0_nogap.get({0, 0}));
  EXPECT_EQ(2, c2_0_nogap.get({1, 1}));
  EXPECT_EQ(3, c2_0_nogap.get({2, 2}));
  EXPECT_EQ(4, c2_0_nogap.get({3, 0}));
  EXPECT_EQ(5, c2_0_nogap.get({4, 0}));
  EXPECT_EQ(6, c2_0_nogap.get({5, 1}));
  EXPECT_EQ(7, c2_0_nogap.get({6, 1}));
  EXPECT_EQ(8, c2_0_nogap.get({7, 1}));
  EXPECT_EQ(9, c2_0_nogap.get({7, 3}));
  EXPECT_EQ(0, c2_0_nogap.get({3, 1}));
  EXPECT_EQ(0, c2_0_nogap.get({7, 2}));
  EXPECT_EQ(-1, c2_0_nogap.get({3, 2}));
  EXPECT_EQ(-1, c2_0_nogap.get({0, 3}));
  EXPECT_EQ(-1, c2_0_nogap.get({6, 2}));

  // validate the number of zero entries is correct
  unsigned zero_cnt = 0;
  for (unsigned col = 0; col < c2_0_nogap.sizes()[0]; ++col)
    for (unsigned row = 0; row < c2_0_nogap.sizes()[1]; ++row)
      zero_cnt += c2_0_nogap.get({col, row}) == 0;
  EXPECT_EQ(12, zero_cnt);

  AssertValuesEmpty(c2_0_nogap);
}

TEST(ChainTest, PolyConst) {
  //    0 1 2 3 4
  //       +-----+
  // 0     |0 0 1|
  // 1     |0 2 0|
  // 2     |3 0 0|
  //   +---+---+-+
  // 3 |4 0 6 0|
  // 4 |0 5 0 7|
  //   +-------+
  // 5
  //     +-+
  // 6   |0|
  // 7   |8|
  //     +-+ +---+
  // 8       |0 0|
  // 9       |9 0|
  //         +---+
  static const int digits[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1};
  typedef ListBase<const int, 2> ListBaseType;
  auto c2_1_lists_factory = [&]() {
    return ListVector<ListBaseType>()
    .Append([&](unsigned col, unsigned row) {
        return digits + (1 + row) * (row + col == 2);
      }, 3, 3)
    .Append([&](unsigned col, unsigned row) {
        return digits + (4 + col) * (row == col % 2);
      }, 4, 2)
    .Append([&](unsigned col, unsigned row) {
        return digits + 8 * (row == 1);
      }, 1, 2)
    .Append([&](unsigned col, unsigned row) {
        return digits + 9 * (row == 1 && col == 0);
      }, 2, 2);
  };
  Chain<ListBaseType, 1> c2_1(
      c2_1_lists_factory(),
      &digits[10],  // use -1 as default
      {{0, {2}}, {2, {1}}, {3, {3}}},
      {{2, 1}});
  typedef ListBaseType::SizeArray SizeArray;
  EXPECT_EQ((SizeArray({5, 10})), c2_1.sizes());
  EXPECT_EQ((SizeArray({2, 0})), c2_1.nesting_offset(0));
  EXPECT_EQ((SizeArray({0, 3})), c2_1.nesting_offset(1));
  EXPECT_EQ((SizeArray({1, 6})), c2_1.nesting_offset(2));
  EXPECT_EQ((SizeArray({3, 8})), c2_1.nesting_offset(3));

  EXPECT_EQ(1, c2_1.get({4, 0}));
  EXPECT_EQ(2, c2_1.get({3, 1}));
  EXPECT_EQ(3, c2_1.get({2, 2}));
  EXPECT_EQ(4, c2_1.get({0, 3}));
  EXPECT_EQ(5, c2_1.get({1, 4}));
  EXPECT_EQ(6, c2_1.get({2, 3}));
  EXPECT_EQ(7, c2_1.get({3, 4}));
  EXPECT_EQ(8, c2_1.get({1, 7}));
  EXPECT_EQ(9, c2_1.get({3, 9}));
  EXPECT_EQ(0, c2_1.get({1, 3}));
  EXPECT_EQ(0, c2_1.get({4, 9}));
  EXPECT_EQ(-1, c2_1.get({0, 0}));
  EXPECT_EQ(-1, c2_1.get({1, 2}));
  EXPECT_EQ(-1, c2_1.get({1, 5}));

  // verify construction via explicitly provided offsets
  Chain<ListBaseType, 1> c2_1_explicit(
      c2_1_lists_factory(),
      &digits[10],
      {{2, 0}, {0, 3}, {1, 6}, {3, 8}},
      5, 10);
  ASSERT_EQ((SizeArray({5, 10})), c2_1_explicit.sizes());
  EXPECT_EQ((SizeArray({2, 0})), c2_1_explicit.nesting_offset(0));
  EXPECT_EQ((SizeArray({0, 3})), c2_1_explicit.nesting_offset(1));
  EXPECT_EQ((SizeArray({1, 6})), c2_1_explicit.nesting_offset(2));
  EXPECT_EQ((SizeArray({3, 8})), c2_1_explicit.nesting_offset(3));
  for (unsigned col = 0; col < c2_1.sizes()[0]; ++col)
    for (unsigned row = 0; row < c2_1.sizes()[1]; ++row)
      EXPECT_EQ(c2_1.get({col, row}), c2_1_explicit.get({col, row})) <<
          "Incorrect get(" << col << ", " << row << ')';

  AssertValuesEmpty(c2_1);
}

TEST(ChainTest, Poly1DLeadingGap) {
  static const char* value = "value";
  static const char* default_value = nullptr;
  typedef ListBase<const char*, 1> ListBaseType;
  Chain<ListBaseType, 0> c1(
      ListVector<ListBaseType>()
      .Append([&](unsigned) {
          return &value;
        }, 1),
      &default_value,
      {},
      {{0, 3}});
  EXPECT_EQ(4, c1.sizes().front());
  EXPECT_EQ(3, c1.nesting_offset(0).front());
  EXPECT_EQ(value, c1.get({3}));
  EXPECT_EQ(default_value, c1.get({0}));
  EXPECT_EQ(default_value, c1.get({1}));
  EXPECT_EQ(default_value, c1.get({2}));

  AssertValuesEmpty(c1);
}

TEST(ChainTest, PolyTrailingGap) {
  static const char* value = "value";
  static const char* default_value = nullptr;
  typedef ListBase<const char*, 3> ListBaseType;
  Chain<ListBaseType, 1> c3_1(
      ListVector<ListBaseType>()
      .Append([&](unsigned, unsigned, unsigned) {
          return &value;
        }, 1, 1, 1),
      &default_value,
      {},
      {{1, 2}});
  typedef ListBaseType::SizeArray SizeArray;
  EXPECT_EQ((SizeArray({1, 3, 1})), c3_1.sizes());
  EXPECT_EQ((SizeArray({0, 0, 0})), c3_1.nesting_offset(0));
  EXPECT_EQ(value, c3_1.get({0, 0, 0}));
  EXPECT_EQ(default_value, c3_1.get({0, 1, 0}));
  EXPECT_EQ(default_value, c3_1.get({0, 2, 0}));

  AssertValuesEmpty(c3_1);
}

TEST(ChainTest, NestingMakeList) {
  static int digits[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1};
  ListVector<ListBase<int, 2> > lists_root;

  ListVector<ListBase<int, 2> > lists_1;
  lists_1.Append([&](unsigned col, unsigned row) {
      return digits + 1;
    }, 1, 3);
  lists_root.Append(ChainTag<1>(),
                    std::move(lists_1),
                    &digits[9],
                    ChainOffsetVector<2>({{0, 2}}), // {} cannot be deduced !
                    1, 5);

  auto l = MakeList(
      ChainTag<1>(),
      ListVector<ListBase<int, 2> >()
      .Append([&](unsigned col, unsigned row) { return digits + 2; }, 1, 1)
      .Append([&](unsigned col, unsigned row) { return digits + 3; }, 1, 1),
      &digits[8],
      {{0, 0}, {0, 1}},  // OK, since MakeList is called explicitly
      1, 2);
  lists_root.Append(std::move(l));

  lists_root.Append(Chain<ListBase<int, 2>, 0>(
      ListVector<ListBase<int, 2> >()  // need to repeat the nested type
      .Append([&](unsigned col, unsigned row) { return digits + 4; }, 1, 1),
      &digits[7],
    {{0, 0}},
      1, 4));

  Chain<ListBase<int, 2>, 1> nested(
      std::move(lists_root),
      &digits[0]);

  typedef decltype(nested)::SizeArray SizeArray;
  EXPECT_EQ(SizeArray({1, 11}), nested.sizes());
  EXPECT_EQ(SizeArray({0, 0}), nested.nesting_offset(0));
  EXPECT_EQ(SizeArray({0, 5}), nested.nesting_offset(1));
  EXPECT_EQ(SizeArray({0, 7}), nested.nesting_offset(2));

  EXPECT_EQ(9, nested.get({0, 0}));
  EXPECT_EQ(9, nested.get({0, 1}));
  EXPECT_EQ(1, nested.get({0, 2}));
  EXPECT_EQ(1, nested.get({0, 4}));

  EXPECT_EQ(2, nested.get({0, 5}));
  EXPECT_EQ(3, nested.get({0, 6}));
  EXPECT_EQ(4, nested.get({0, 7}));
  EXPECT_EQ(7, nested.get({0, 10}));
}

TEST(UniformChainTest, PolyNoGaps) {
  //    0 1 2 3 4 5 6 7 8
  //   +-----+-----+-----+
  // 0 |1 0 0|4 5 0|0 0 0|
  // 1 |0 2 0|0 0 6|7 8 9|
  //   +-----+-----+-----+
  static int digits[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1};
  typedef ListBase<int, 2> ListBaseType;
  UniformChain<ListBase<int, 2>, 0, 3> uc2_0_3_nogap(
      ListVector<ListBase<int, 2> >()
      .Append([&](unsigned col, unsigned row) {
          return digits + (1 + col) * (row == col);
        }, 3, 2)
      .Append([&](unsigned col, unsigned row) {
          return digits + (4 + col) * ((!row && col < 2) || (row && col >= 2));
        }, 3, 2)
      .Append([&](unsigned col, unsigned row) {
          return digits + (7 + col) * (row == 1);
        }, 3, 2),
      &digits[10]);  // use -1 as default
  typedef ListBaseType::SizeArray SizeArray;
  EXPECT_EQ((SizeArray({9, 2})), uc2_0_3_nogap.sizes());

  // validate all non-zero entries
  EXPECT_EQ(1, uc2_0_3_nogap.get({0, 0}));
  EXPECT_EQ(2, uc2_0_3_nogap.get({1, 1}));
  EXPECT_EQ(4, uc2_0_3_nogap.get({3, 0}));
  EXPECT_EQ(5, uc2_0_3_nogap.get({4, 0}));
  EXPECT_EQ(6, uc2_0_3_nogap.get({5, 1}));
  EXPECT_EQ(7, uc2_0_3_nogap.get({6, 1}));
  EXPECT_EQ(8, uc2_0_3_nogap.get({7, 1}));
  EXPECT_EQ(9, uc2_0_3_nogap.get({8, 1}));

  // validate a few other zero entries (corner cases)
  EXPECT_EQ(0, uc2_0_3_nogap.get({0, 1}));
  EXPECT_EQ(0, uc2_0_3_nogap.get({1, 0}));
  EXPECT_EQ(0, uc2_0_3_nogap.get({3, 1}));
  EXPECT_EQ(0, uc2_0_3_nogap.get({6, 0}));
  EXPECT_EQ(0, uc2_0_3_nogap.get({8, 0}));

  AssertValuesEmpty(uc2_0_3_nogap);
}

TEST(UniformChainTest, PolyGapsBeforeOnly) {
  //    0 1 2 3 4 5
  //       +-+   +-+
  // 0     |4|   |0|
  // 1     |0|   |2|
  //       +-+   +-+
  static int four = 4, two = 2, zero = 0, minus_one = -1;
  typedef ListBase<int, 2> ListBaseType;
  UniformChain<ListBase<int, 2>, 0, 1, 2> uc2_0_1_2(
      ListVector<ListBase<int, 2> >()
      .Append([&](unsigned col, unsigned row) {
          if (row >= 2) ADD_FAILURE() << "row = " << row;
          return row == 0 ? &four : &zero;
        }, 1, 2)
      .Append([&](unsigned col, unsigned row) {
          if (row >= 2) ADD_FAILURE() << "row = " << row;
          return row == 1 ? &two : &zero;
        }, 1, 2),
      &minus_one);
  typedef ListBaseType::SizeArray SizeArray;
  EXPECT_EQ((SizeArray({6, 2})), uc2_0_1_2.sizes());

  // validate non-default values
  EXPECT_EQ(4, uc2_0_1_2.get({2, 0}));
  EXPECT_EQ(2, uc2_0_1_2.get({5, 1}));
  EXPECT_EQ(0, uc2_0_1_2.get({5, 0}));
  EXPECT_EQ(0, uc2_0_1_2.get({2, 1}));

  // validate a few default values (corner cases)
  EXPECT_EQ(-1, uc2_0_1_2.get({0, 0}));
  EXPECT_EQ(-1, uc2_0_1_2.get({1, 0}));
  EXPECT_EQ(-1, uc2_0_1_2.get({3, 1}));
  EXPECT_EQ(-1, uc2_0_1_2.get({4, 1}));

  AssertValuesEmpty(uc2_0_1_2);
}

TEST(UniformChainTest, PolyGapsAfterOnly) {
  //    0 1 2 3 4 5 6 7
  //   +-+     +-+
  // 0 |1|     |3|
  //   +-+     +-+
  static int one = 1, three = 3, zero = 0;
  typedef ListBase<int, 2> ListBaseType;
  UniformChain<ListBase<int, 2>, 0, 1, 0, 3> uc2_0_1_0_3(
      ListVector<ListBase<int, 2> >()
      .Append([&](unsigned col, unsigned row) {
          if (row) ADD_FAILURE() << "row = " << row;
          return &one;
        }, 1, 1)
      .Append([&](unsigned col, unsigned row) {
          if (row) ADD_FAILURE() << "row = " << row;
          return &three;
        }, 1, 1),
      &zero);
  typedef ListBaseType::SizeArray SizeArray;
  EXPECT_EQ((SizeArray({8, 1})), uc2_0_1_0_3.sizes());

  // validate non-default values
  EXPECT_EQ(1, uc2_0_1_0_3.get({0, 0}));
  EXPECT_EQ(3, uc2_0_1_0_3.get({4, 0}));

  // validate default values
  EXPECT_EQ(0, uc2_0_1_0_3.get({1, 0}));
  EXPECT_EQ(0, uc2_0_1_0_3.get({2, 0}));
  EXPECT_EQ(0, uc2_0_1_0_3.get({3, 0}));
  EXPECT_EQ(0, uc2_0_1_0_3.get({5, 0}));
  EXPECT_EQ(0, uc2_0_1_0_3.get({6, 0}));
  EXPECT_EQ(0, uc2_0_1_0_3.get({7, 0}));

  AssertValuesEmpty(uc2_0_1_0_3);
}

TEST(UniformChainTest, PolyGapsBeforeAndAfter) {
  //   0
  //    0 1 2 3
  //     +-------+
  // 0  /       /|
  //   +-------+ +
  // 1 |9 8 7 6|/
  //   +-------+
  // 2
  // 3   +-------+
  // 4  /       /|
  //   +-------+ +
  // 5 |2 3 4 5|/
  //   +-------+
  // 6
  // 7   +-------+
  // 8  /       /|
  //   +-------+ +
  // 9 |1 1 1 1|/
  //   +-------+
  // 10
  // 11
  static int digits[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1};
  typedef ListBase<int, 3> ListBaseType;
  UniformChain<ListBase<int, 3>, 1, 1, 1, 2> uc3_1_1_1_2(
      ListVector<ListBase<int, 3> >()
      .Append([&](unsigned c1, unsigned c2, unsigned c3) {
          return &digits[9 - c1];
        }, 4, 1, 1)
      .Append([&](unsigned c1, unsigned c2, unsigned c3) {
          return &digits[2 + c1];
        }, 4, 1, 1)
      .Append([&](unsigned c1, unsigned c2, unsigned c3) {
          return &digits[1];
        }, 4, 1, 1),
      &digits[10]);  // use -1 as default
  typedef ListBaseType::SizeArray SizeArray;
  EXPECT_EQ((SizeArray({4, 12, 1})), uc3_1_1_1_2.sizes());

  // validate all non-zero entries
  EXPECT_EQ(9, uc3_1_1_1_2.get({0, 1}));
  EXPECT_EQ(8, uc3_1_1_1_2.get({1, 1}));
  EXPECT_EQ(7, uc3_1_1_1_2.get({2, 1}));
  EXPECT_EQ(6, uc3_1_1_1_2.get({3, 1}));
  EXPECT_EQ(2, uc3_1_1_1_2.get({0, 5}));
  EXPECT_EQ(3, uc3_1_1_1_2.get({1, 5}));
  EXPECT_EQ(4, uc3_1_1_1_2.get({2, 5}));
  EXPECT_EQ(5, uc3_1_1_1_2.get({3, 5}));
  for (size_t i = 0; i < 4; ++i)
    EXPECT_EQ(1, uc3_1_1_1_2.get({i, 9, 0})) << "get(" << i << ", 9, 0)";

  // validate a few other zero entries (corner cases)
  for (size_t c2 = 0; c2 <= 9; ++c2)
    if (c2 != 1 && c2 != 5 && c2 != 9)
      for (size_t c1 = 0; c1 <= 3; ++c1)
        EXPECT_EQ(-1, uc3_1_1_1_2.get({c1, c2, 0}))
            << "get(" << c1 << ", " << c2 << ", 0)";

  AssertValuesEmpty(uc3_1_1_1_2);
}

TEST(UniformChainTest, MakeList) {
  //    0 1 2 3 4 5 6 7 8
  //     +-+     +-+   +-+
  // 0   |1|     |1|   |2|
  //     | |     | |   | |
  // 1   |1|     |1|   |2|
  //     | |     | |   +-+
  // 2   |1|     |1|   |2|
  //     | |     | |   | |
  // 3   |1|     |1|   |2|
  //     +-+     +-+   +-+
  static int zero = 0, minus_one = -1, minus_two = -2;
  Chain<ListBase<int, 2>, 0> nested(
      ListVector<ListBase<int, 2> >()
      .Append(MakeList(
          UniformChainTag<0, 1, 1, 2>(),
          ListVector<ListBase<int, 2> >()
          .Append([](unsigned, unsigned) {
              static int one = 1;
              return &one;
            }, 1, 4)
          .Append([](unsigned, unsigned) {
              static int one = 1;
              return &one;
            }, 1, 4)
          , &minus_one))
      .Append(
          UniformChainTag<1, 2>(),
          ListVector<ListBase<int, 2> >()
          .Append([](unsigned, unsigned) {
              static int two = 2;
              return &two;
            }, 1, 2)
          .Append([](unsigned, unsigned) {
              static int two = 2;
              return &two;
            }, 1, 2)
          , &minus_two)
      , &zero);

  ASSERT_EQ(9, nested.sizes()[0]);
  ASSERT_EQ(4, nested.sizes()[1]);
  for (size_t c1 = 0; c1 <= 8; ++c1)
    for (size_t c2 = 0; c2 <= 3; ++c2) {
      int exp = -1;
      if (c1 == 1 || c1 == 5) exp = 1;
      else if (c1 == 8) exp = 2;
      EXPECT_EQ(exp, nested.get({c1, c2}))
          << "get(" << c1 << ", " << c2 << ')';
    }
}
