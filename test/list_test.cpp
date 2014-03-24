#include "../src/list.hpp"
#include "../src/util/memory.hpp"
#include "test.hpp"

#include <vector>
#include <memory>
#include <type_traits>

template<typename T>
using PolyList = List<T, PortionBase<T> >;

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

  // EXPECT_EQ(10, pv[1]);
  // EXPECT_EQ(50, pv[2]);
  // EXPECT_EQ(40, pv[4]);
}

TEST(ListTest, ConstructorNonPoly) {
  int arr[] = {0, 10, 20, 30, 40, 50};
  PolyVector<Portion<int*>, PortionBase<int>, PortionFactory > v;
  v.Append(arr, 1);
  v.Append<Portion<int*> >(arr + 1, 2);
  PortionVector<int, Portion<int*> > w = v;
  List<int, Portion<int*> > l(std::move(v));

  // EXPECT_EQ("foobar", std::string("f").append(2, 'o').append("bar"));

  List<int, Portion<int*> > l2(
      PortionVector<int, Portion<int*> >()
      .Append(arr + 4, 1)
      .Append<int>(arr + 1, 2));
  EXPECT_EQ(40, l2.get(0));
  EXPECT_EQ(10, l2.get(1));
  EXPECT_EQ(20, l2.get(2));

  auto l3 = MakeList(
      PortionVector<int, Portion<int*> >()
      .Append(arr + 4, 1)
      .Append<int>(arr + 1, 2));
  EXPECT_EQ(40, l2.get(0));
  EXPECT_EQ(10, l2.get(1));
  EXPECT_EQ(20, l2.get(2));
}

void AssertEqualYellow(const List<const char>& l) {
  EXPECT_EQ('y', l.get(0));
  EXPECT_EQ('e', l.get(1));
  EXPECT_EQ('l', l.get(2));
  EXPECT_EQ('l', l.get(3));
  EXPECT_EQ('o', l.get(4));
  EXPECT_EQ('w', l.get(5));
}

TEST(ListTest, ConstructorPoly) {
  char y = 'y';
  const char *hello_world_str = "hello world";
  std::string world(hello_world_str + 6, 5);

  List<const char> l_verbose(
      PortionVector<const char>()
      .Append<SingletonPortion<const char> >(y)
      .Append<Portion<const char*> >(hello_world_str + 1, 4)
      .Append<Portion<std::string::const_iterator> >(world.cbegin(), 1)
      .Append<decltype(MakePortion(hello_world_str[5]))>(hello_world_str[5])
      .Append<decltype(MakePortion(world.crbegin() + 2, 4))>(
          world.crbegin() + 2, 4));
  AssertEqualYellow(l_verbose);

  List<const char> l(
      PortionVector<const char>()
      .Append(static_cast<const char&>(y))
      .Append(hello_world_str + 1, 4)
      .Append(world.cbegin(), 1)
      .Append(hello_world_str + 5, 1)
      .Append(world.crbegin() + 2, 4));
  AssertEqualYellow(l);

  // using a macro, we might simplify it to
  // V_LIST(const char, l,
  //        (hello_world_str + 1, 4)
  //        (world.cbegin(), 1)
  //        (hello_world_str + 5, 1)
  //        (world.crbegin() + 2, 4));
}
