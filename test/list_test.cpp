#include "../src/list.hpp"
#include "../src/util/memory.hpp"
#include "test.hpp"

#include <algorithm>
#include <vector>
#include <type_traits>

TEST(ListTest, Dummy) {
  DummyList<int, 3> dl;
  typedef ListBase<int, 3>::SizeArray SizeArray;
  EXPECT_EQ((SizeArray({0, 0, 0})), dl.sizes());

  EXPECT_EQ(dl.lbegin(), dl.lend());
  EXPECT_EQ(dl.fbegin(), dl.fend());
  EXPECT_EQ(dl.begin(), dl.end());

  EXPECT_EQ(dl.lbegin() + 0, dl.lbegin());
  EXPECT_EQ(dl.end() -= 0, dl.end());

  EXPECT_EQ(dl.begin(), dl.cbegin());
  EXPECT_EQ(dl.cend(), dl.end());
  decltype(dl.clend()) cit = dl.lend();
  EXPECT_EQ(cit, dl.clend());
  // decltype(dl.lend()) it = dl.clend();  // compile error - OK
}

TEST(ListTest, PortionVector) {
  int arr[] = {0, 10, 20, 30, 40, 50};
  std::vector<int> v(arr + 3, arr + 5);

  PortionVector<int, PortionBase<int> > pv;
  pv.Append(arr, 2);
  pv.Append(arr[5]);
  pv.Append(v.begin(), 2);

  EXPECT_EQ(10, pv[0].get(1));
  EXPECT_EQ(50, pv[1].get(0));
  EXPECT_EQ(40, pv[2].get(1));
}

TEST(ListTest, SimpleNonPoly) {
  int arr[] = {0, 10, 20, 30, 40, 50};
  PolyVector<Portion<int*>, PortionBase<int>, PortionFactory > v;
  v.Append(arr, 1);
  v.AppendDerived<Portion<int*> >(arr + 1, 2);
  PortionVector<int, Portion<int*> > w = v;
  SimpleList<Portion<int*> > l(std::move(v));

  // EXPECT_EQ("foobar", std::string("f").append(2, 'o').append("bar"));

  SimpleList<Portion<int*> > l2(
      PortionVector<int, Portion<int*> >()
      .Append(arr + 4, 1)
      .AppendDerived<int>(arr + 1, 2));
  EXPECT_EQ(40, l2.get(0));
  EXPECT_EQ(10, l2.get(1));
  EXPECT_EQ(20, l2.get(2));

  const View<int>& v2 = l2;
  ASSERT_EQ(v2.begin(), l2.fbegin());
  ASSERT_EQ(v2.begin(), l2.fbegin());
  ASSERT_EQ(v2.end(), l2.fend());
  ASSERT_EQ(3, v2.size());
  EXPECT_EQ(v2.begin(), v2.begin());
  EXPECT_EQ(v2.end(), v2.end());
  EXPECT_NE(v2.begin(), v2.end());

  auto v2it = v2.begin();
  EXPECT_EQ(v2.begin(), v2it);
  EXPECT_NE(v2.end(), v2it);
  EXPECT_EQ(40, *v2it++);
  EXPECT_NE(v2.begin(), v2it);
  EXPECT_NE(v2.end(), v2it);
  EXPECT_EQ(10, *v2it);
  EXPECT_NE(v2.begin(), v2it);
  EXPECT_NE(v2.end(), v2it);
  EXPECT_EQ(20, *++v2it);
  EXPECT_NE(v2.begin(), v2it);
  EXPECT_NE(v2.end(), v2it);
  EXPECT_EQ(v2.end(), ++v2it);
  EXPECT_NE(v2.begin(), v2it);

  auto l3 = MakeList(
      PortionVector<int, Portion<int*> >()
      .Append(arr + 4, 1)
      .AppendDerived<int>(arr + 1, 2));
  EXPECT_EQ(40, l2.get(0));
  EXPECT_EQ(10, l2.get(1));
  EXPECT_EQ(20, l2.get(2));

  auto l3_it = l3.begin();
  EXPECT_EQ(40, *l3_it++);
  EXPECT_EQ(10, *l3_it);
  *l3_it = 100;
  EXPECT_EQ(20, *++l3_it);
  EXPECT_EQ(100, l.get(1));
}

TEST(ListTest, SimpleNonPolyRandomAccessIterator) {
  //           0  [1   2   3   4   5][6   7]  8   9[10  11  12]
  int arr[] = {0, 10, 20, 20, 10, 50, 0, 10, 80, 90, 0, 10, 20};
  //               0   1   2   3   4  5   6          7   8   9

  SimpleList<Portion<int*> > l(
      PortionVector<int, Portion<int*> >()
      .Append(arr + 1, 5)
      .Append(arr + 6, 2)
      .Append(arr + 10, 3));

  typedef decltype(l)::Iterator Iter;
  typedef decltype(l)::ConstIterator ConstIter;
  Iter it = l.lend();
  EXPECT_EQ(20, *--it);
  EXPECT_EQ(0, *(it -= 2));
  it = l.lbegin();
  EXPECT_EQ(20, *++it);
  EXPECT_EQ(10, *--it);
  it += 2;
  EXPECT_EQ(20, *it);
  EXPECT_EQ(10, *(it - 2));
  it += 4;
  EXPECT_EQ(10, *it--);
  EXPECT_EQ(50, *(it -= 1));
  EXPECT_EQ(50, *(it += 0));
  EXPECT_EQ(50, *(it - 0));

  // validate interoperability
  Iter it0 = 4 + l.lbegin();
  EXPECT_EQ(50, *it0);
  it0 += 1;
  EXPECT_EQ(0, *it0);
  ConstIter cit0 = it0;
  // it0 = cit0;  // compile error - OK
  EXPECT_EQ(cit0, it0);
  EXPECT_EQ(it0, cit0);
  EXPECT_EQ(0, *cit0);
  cit0 += 2;
  for (int i = 0; i < 2; ++i) {
    EXPECT_EQ(*it0, *cit0) << "i = " << i;
    EXPECT_NE(it0, cit0) << "i = " << i;
    EXPECT_EQ(2, cit0 - it0) << "i = " << i;
    ++it0, ++cit0;
  }
  EXPECT_NE(*it0, *cit0);

  // validate modifications through list iterators
  it = l.lbegin() + 2;
  *it = 30;
  EXPECT_EQ(30, l.get(2));
}

TEST(ListTest, SimplePolyRandomAccessIterator) {
  std::vector<int> every10 = {0, 10, 20};
  int fifty = 50;

  //    [ 0   1][ 2   3][ 4][ 5   6][ 7   8   9]
  // =   10, 20, 20, 10, 50,  0, 10,  0, 10, 20
  SimpleList<PortionBase<int> > l(
      PortionVector<int>()
      .Append(every10.begin() + 1, 2)
      .Append(every10.rbegin(), 2)
      .Append(static_cast<int&>(fifty))
      .Append(every10.begin(), 2)
      .Append(every10.data(), 3));

  typedef decltype(l)::Iterator Iter;
  typedef decltype(l)::ConstIterator ConstIter;
  Iter it = l.lend();
  EXPECT_EQ(20, *--it);
  EXPECT_EQ(0, *(it -= 2));
  it = l.lbegin();
  EXPECT_EQ(20, *++it);
  EXPECT_EQ(10, *--it);
  it += 2;
  EXPECT_EQ(20, *it);
  EXPECT_EQ(10, *(it - 2));
  it += 4;
  EXPECT_EQ(10, *it--);
  EXPECT_EQ(50, *(it -= 1));
  EXPECT_EQ(50, *(it += 0));
  EXPECT_EQ(50, *(it - 0));

  // validate interoperability
  Iter it0 = 4 + l.lbegin();
  EXPECT_EQ(50, *it0);
  it0 += 1;
  EXPECT_EQ(0, *it0);
  ConstIter cit0 = it0;
  // it0 = cit0;  // compile error - OK
  EXPECT_EQ(cit0, it0);
  EXPECT_EQ(it0, cit0);
  EXPECT_EQ(0, *cit0);
  cit0 += 2;
  for (int i = 0; i < 2; ++i) {
    EXPECT_EQ(*it0, *cit0) << "i = " << i;
    EXPECT_NE(it0, cit0) << "i = " << i;
    EXPECT_EQ(2, cit0 - it0) << "i = " << i;
    ++it0, ++cit0;
  }
  EXPECT_NE(*it0, *cit0);

  // validate modifications through list iterators
  it = l.lbegin() + 2;
  *it = 30;
  EXPECT_EQ(30, l.get(2));
}

template<class L>
void AssertEqualYellow(const L& l) {
  EXPECT_EQ('y', l.get(0));
  EXPECT_EQ('e', l.get(1));
  EXPECT_EQ('l', l.get(2));
  EXPECT_EQ('l', l.get(3));
  EXPECT_EQ('o', l.get(4));
  EXPECT_EQ('w', l.get(5));
}

TEST(ListTest, ConstructorPoly) {
  static char y = 'y';
  static const char *hello_world_str = "hello world";
  static std::string world(hello_world_str + 6, 5);

  SimpleList<PortionBase<const char> > l_verbose(
      PortionVector<const char>()
      .AppendDerived<SingletonPortion<const char> >(y)
      .AppendDerived<Portion<const char*> >(hello_world_str + 1, 4)
      .AppendDerived<Portion<std::string::const_iterator> >(world.cbegin(), 1)
      .AppendDerived<decltype(MakePortion(hello_world_str[5]))>(
          hello_world_str[5])
      .AppendDerived<decltype(MakePortion(world.crbegin() + 2, 4))>(
          world.crbegin() + 2, 4));
  AssertEqualYellow(l_verbose);

  SimpleList<PortionBase<const char> > l(
      PortionVector<const char>()
      .Append(static_cast<const char&>(y))
      .Append(hello_world_str + 1, 4)
      .Append(world.cbegin(), 1)
      .Append(hello_world_str + 5, 1)
      .Append(world.crbegin() + 2, 4));
  AssertEqualYellow(l);

  View<const char>::Iterator l_vit(l.fbegin());
  EXPECT_EQ('y', *l_vit++);
  EXPECT_EQ('e', *l_vit);
  EXPECT_EQ('l', *++l_vit);
  EXPECT_EQ('l', *++l_vit);
  EXPECT_EQ('o', *++l_vit);
  EXPECT_EQ('w', *++l_vit);

  // using a macro, we might simplify it to
  // V_LIST(const char, l,
  //        (hello_world_str + 1, 4)
  //        (world.cbegin(), 1)
  //        (hello_world_str + 5, 1)
  //        (world.crbegin() + 2, 4));

  ImplicitList<const char> il([&](size_t index) -> const char* {
      switch (index) {
        case 0: return &y;
        case 1: return hello_world_str + 1;
        case 2: return hello_world_str + 2;
        case 3: return hello_world_str + 3;
        case 4: return hello_world_str + 4;
        default: return &world[0];
      }
    }, 6);
  AssertEqualYellow(il);

  auto il2 = MakeList([&](size_t index) -> const char* {
      switch (index) {
        case 0: return &y;
        case 1: return hello_world_str + 1;
        case 2: return hello_world_str + 2;
        case 3: return hello_world_str + 3;
        case 4: return hello_world_str + 4;
        default: return &world[0];
      }
    }, 6);
  AssertEqualYellow(il2);

  auto it2 = il2.begin();
  EXPECT_EQ(y, *it2);
  EXPECT_EQ(&y, &*it2++);
  EXPECT_EQ(hello_world_str + 1, &*it2);
  EXPECT_EQ(hello_world_str + 2, &*++it2);
  ++it2;
  ++it2;
  EXPECT_EQ(&world[0], &*++it2);
  EXPECT_EQ(il2.end(), ++it2);
}

TEST(ListTest, SizesAndSize) {
  static int var = 2;
  auto il1d = MakeList([&](size_t index) { return &var; }, 7);
  EXPECT_EQ(7, il1d.sizes()[0]);
  EXPECT_EQ(7, il1d.size());

  auto il3d = MakeList([&](int x, size_t y, char z) { return &var; },
                       5, size_t(3), 2);
  EXPECT_EQ((std::array<size_t, 3>({5, 3, 2})), il3d.sizes());
  EXPECT_EQ(30, il3d.size());
}

TEST(ListTest, Offsets2D) {
  static int m[3][3]; std::iota(&m[0][0], &m[0][0] + 9, 1);
  auto il2d = MakeList([&](unsigned row, unsigned col) { return &m[row][col]; },
                       Sub(2, 5), Sub(1, 3));
  EXPECT_EQ(8, il2d(0, 0));
  EXPECT_EQ(9, il2d(0, 1));

  // test modifications
  il2d(0, 1) = 90;
  EXPECT_EQ(90, il2d(0, 1));
  ListBase<int, 2>& b = il2d;
  b.get({0, 1}) = 900;
  EXPECT_EQ(900, b.get({0, 1}));
  EXPECT_EQ(900, il2d(0, 1));
}

TEST(ListTest, Offsets3DSub) {
  static const int n = 5;
  static int m[n][n][n];
  for (int x = 0, px = 1; x < n; ++x, px *= 2)
    for (int y = 0, py = 1; y < n; ++y, py *= 3)
      for (int z = 0, pz = 1; z < n; ++z, pz *= 5) {
        m[x][y][z] = px * py * pz;
      }

  auto il3d = MakeList([&](int x, size_t y, unsigned char z) {
      return &m[x][y][z];
    }, Sub(2, 5), Sub(3, 3), Sub(1, 2));

  EXPECT_EQ(m[2][3][1], il3d(0, 0, 0));
  EXPECT_EQ(m[3][4][1], il3d.get(1, 1, 0));
  EXPECT_EQ(m[4][3][4], il3d.get({2, 0, 3}));

  auto it = il3d.begin();
  for (int i = 0; i < 3; ++i)
    ++it;
  EXPECT_EQ(il3d(0, 1, 1), *it);
  for (int i = 0; i < 3; ++i)
    ++it;
  EXPECT_EQ(il3d(1, 0, 0), *it);

  decltype(il3d) il3d_sub(il3d, Sub(1, 2), Sub(0, 2), Sub(2, 2));
  EXPECT_EQ(m[3][3][3], il3d_sub(0, 0, 0));
  EXPECT_EQ(m[4][4][4], il3d_sub(1, 1, 1));
  EXPECT_EQ(m[4][3][4], il3d_sub(1, 0, 1));
  EXPECT_EQ(m[3][4][3], il3d_sub(0, 1, 0));

  auto sub_it = il3d_sub.begin();
  EXPECT_EQ(il3d_sub.begin(), sub_it);
  EXPECT_EQ(il3d_sub(0, 0, 0), *sub_it++);
  EXPECT_NE(il3d_sub.begin(), sub_it);
  EXPECT_EQ(il3d_sub(0, 0, 1), *sub_it++);
  EXPECT_EQ(il3d_sub(0, 1, 0), *sub_it++);
  EXPECT_EQ(il3d_sub(0, 1, 1), *sub_it++);
  EXPECT_EQ(il3d_sub(1, 0, 0), *sub_it++);
  EXPECT_EQ(il3d_sub(1, 0, 1), *sub_it++);
  EXPECT_EQ(il3d_sub(1, 1, 0), *sub_it++);
  EXPECT_EQ(il3d_sub(1, 1, 1), *sub_it++);
  EXPECT_EQ(il3d_sub.end(), sub_it);
  EXPECT_EQ(il3d_sub.end(), il3d_sub.end());
  EXPECT_NE(il3d_sub.begin(), sub_it);

  sub_it = il3d_sub.begin();
  std::fill_n(sub_it, 3, 7);
  for (int i = 0; i < 3; ++i)
    ASSERT_EQ(7, *sub_it++);
  ASSERT_NE(7, *sub_it);
}

TEST(ListTest, IterEmpty) {
  auto l = MakeList(PortionVector<int, Portion<int*> >());
  EXPECT_EQ(l.fbegin(), l.fbegin());
  EXPECT_EQ(l.fend(), l.fend());
  EXPECT_EQ(l.fbegin(), l.fend());
  EXPECT_EQ(l.begin(), l.begin());
  EXPECT_EQ(l.end(), l.end());
  EXPECT_EQ(l.begin(), l.end());

  static int datum;
  auto il = MakeList([&](int index) { return &datum; }, 0);
  EXPECT_EQ(il.fbegin(), il.fbegin());
  EXPECT_EQ(il.fend(), il.fend());
  EXPECT_EQ(il.fbegin(), il.fend());
  EXPECT_EQ(l.begin(), l.begin());
  EXPECT_EQ(l.end(), l.end());
  EXPECT_EQ(l.begin(), l.end());
}
