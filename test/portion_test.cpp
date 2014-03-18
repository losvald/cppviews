#include "../src/portion.hpp"
#include "test.hpp"

#include <vector>
#include <set>

TEST(PortionTest, MetaTest) {
  int arr[] = {};
  EXPECT_TRUE((std::is_same<int,
               decltype(MakePortion(arr, 3))::ValueType>
               ::value));
  EXPECT_TRUE((std::is_same<int&,
               decltype(MakePortion(arr, 3))::RefType>
               ::value));

  std::vector<int> vec;
  EXPECT_TRUE((std::is_same<int,
               decltype(MakePortion(vec.begin(), 3))::ValueType>
               ::value));
  EXPECT_TRUE((std::is_same<int&,
               decltype(MakePortion(vec.begin(), 3))::RefType>
               ::value));

  std::set<int> set;
  EXPECT_TRUE((std::is_same<const int,
               decltype(MakePortion(set.begin(), 3))::ValueType>
               ::value));
  EXPECT_TRUE((std::is_same<const int&,
               decltype(MakePortion(set.begin(), 3))::RefType>
               ::value));
  EXPECT_TRUE((std::is_same<const int,
               decltype(MakePortion(set.cbegin(), 3))::ValueType>
               ::value));
  EXPECT_TRUE((std::is_same<const int&,
               decltype(MakePortion(set.cbegin(), 3))::RefType>
               ::value));
}


TEST(PortionTest, Pointer) {
  int a[] = {0, 10, 20, 30, 40, 50};
  Portion<int*> f(a + 1, 3);
  EXPECT_EQ(3, f.size());
  EXPECT_EQ(10, f[0]);
  EXPECT_EQ(20, f[1]);
  EXPECT_EQ(30, f[2]);

  f[0] = 100;
  EXPECT_EQ(100, f[0]);
  EXPECT_EQ(a[1], f[0]);

  EXPECT_EQ(20, f.get(1));
  EXPECT_EQ(30, f.get(2));

  f.set(0, 10);
  EXPECT_EQ(10, f.get(0));
  EXPECT_EQ(a[1], f.get(0));

  PortionBase<int>& fb = f;
  EXPECT_EQ(10, fb.get(0));
  EXPECT_EQ(20, fb.get(1));
  EXPECT_EQ(30, fb.get(2));

  std::reverse_iterator<int*> r(a + 1 + fb.size());
  Portion<decltype(r)> b(r, fb.size());
  EXPECT_EQ(3, b.size());
  EXPECT_EQ(30, b[0]);
  EXPECT_EQ(20, b[1]);
  EXPECT_EQ(10, b[2]);
}

TEST(PortionTest, RandomIter) {
  std::vector<int> v = {10, 20, 30};
  // Portion<decltype(v.cbegin())> fc(v.cbegin(), 2); // compile error - OK
  Portion<decltype(v.begin())> f(v.begin(), 2);
  EXPECT_EQ(10, f[0]);
  EXPECT_EQ(20, f[1]);
  EXPECT_EQ(30, f[2]);
  f[1] = 200;
  EXPECT_EQ(200, f[1]);
  EXPECT_EQ(200, v[1]);

  PortionBase<int>& fb = f;
  fb.set(1, 20);
  EXPECT_EQ(10, fb.get(0));
  EXPECT_EQ(20, fb.get(1));
  EXPECT_EQ(30, fb.get(2));

  // Test construction of a (read-only) portion using MakePortion

  auto fa = MakePortion(v.begin(), 3);
  EXPECT_EQ(10, fa[0]);
  EXPECT_EQ(20, fa[1]);
  EXPECT_EQ(30, fa[2]);

  auto fac = MakePortion(v.cbegin(), 3);
  EXPECT_EQ(10, fac[0]);
  EXPECT_EQ(20, fac[1]);
  EXPECT_EQ(30, fac[2]);

  auto far = MakePortion(v.rbegin(), 3);
  EXPECT_EQ(30, far[0]);
  EXPECT_EQ(20, far[1]);
  EXPECT_EQ(10, far[2]);
  far[2] = 100;
  EXPECT_EQ(100, v[0]);

  auto facr = MakePortion(v.crbegin(), 3);
  for (int index = v.size(); --index; )
    EXPECT_EQ(far[index], facr[index]);
  // facr[0] = 3000;         // compile error - OK
}

TEST(PortionTest, BidirIter) {
  std::set<int> s = {50, 20, 40, 10, 0, 30};
  // Portion<decltype(s.begin())> f(s.begin(), 2);   // compile error!
  // Portion<decltype(s.cbegin())> f(s.cbegin(), 2); // compile error!
  Portion<decltype(s.cbegin()), const int> f(++ ++ s.cbegin(), 3);
  EXPECT_EQ(20, f[0]);
  EXPECT_EQ(30, f[1]);
  EXPECT_EQ(40, f[2]);
  EXPECT_EQ(50, f[3]);
  // f[0] = 200;                           // compile error - OK

  ReadonlyPortion<decltype(s.cbegin())> rf(++ ++ s.cbegin(), 3);
  EXPECT_EQ(20, f[0]);
  EXPECT_EQ(30, f[1]);
  EXPECT_EQ(40, f[2]);
  EXPECT_EQ(50, f[3]);
  // f[0] = 200;                           // compile error - OK

  // Test construction of a read-only portion using MakePortion

  auto af = MakePortion(++ ++ s.begin(), 2);
  EXPECT_EQ(20, af[0]);
  EXPECT_EQ(30, af[1]);
  EXPECT_EQ(40, af[2]);
  EXPECT_EQ(50, af[3]);
  // af[0] = 100;         // compile error - OK

  auto af2 = MakePortion(++ ++ s.cbegin(), 2);
  EXPECT_EQ(20, af2[0]);
  EXPECT_EQ(30, af2[1]);
  EXPECT_EQ(40, af2[2]);
  EXPECT_EQ(50, af2[3]);
  // af[0] = 100;         // compile error - OK
}
