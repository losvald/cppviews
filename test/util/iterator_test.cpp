#include "../../src/util/iterator.hpp"
#include "../test.hpp"

#include <vector>

#include <cctype>

TEST(IteratorTest, IndirectRandomAccess) {
  int x = 10, y = 20, z = 30;
  std::vector<int*> a = {&x, &y, &z};
  typedef IndirectIterator<std::vector<int*>::iterator> TI;

  // validate forward iterator operations
  TI ti = a.begin();
  ASSERT_EQ(ti, TI(a.begin()));
  ASSERT_FALSE(ti != TI(a.begin()));
  ASSERT_NE(ti, TI(a.end()));
  EXPECT_EQ(10, *ti++);
  EXPECT_EQ(20, *ti);
  EXPECT_EQ(30, *++ti);
  EXPECT_EQ(ti, TI(--a.end()));
  EXPECT_EQ(++ti, TI(a.end()));

  // validate bidirectional iterator operations
  EXPECT_EQ(30, *--ti);
  EXPECT_NE(ti, TI(a.end()));
  --ti;
  EXPECT_EQ(20, *ti--);
  EXPECT_EQ(10, *ti);
  EXPECT_EQ(ti, TI(a.begin()));

  // validate bidirectional iterator operations
  ti = a.begin();
  EXPECT_LT(ti, ti + 1);
  EXPECT_FALSE(ti < ti);
  EXPECT_LE(ti, ti);
  EXPECT_LE(ti, ti + 1);
  EXPECT_FALSE(ti + 1 <= ti);
  EXPECT_GT(ti + 1, ti);
  EXPECT_FALSE(ti > ti);
  EXPECT_GE(ti, ti);
  EXPECT_GE(ti + 1, ti);
  EXPECT_FALSE(ti >= ti + 1);
  EXPECT_EQ(30, *(ti += 2));
  EXPECT_EQ(10, *(ti -= 2));
  EXPECT_EQ(30, *(TI(a.begin()) + 2));
  EXPECT_EQ(10, *(TI(a.end()) - 3));
  EXPECT_EQ(30, *(2 + TI(a.begin())));
  EXPECT_EQ(10, *(-3 + TI(a.end())));

  // validate change via reference
  typedef std::iterator_traits<TI>::reference TIRef;
  TIRef r = *TI(a.begin());
  r = 100;
  EXPECT_EQ(100, *a.front());

  // validate change via pointer
  typedef std::pair<int, int> IntPair;
  IntPair p12(1, 2), p21(2, 1);
  std::vector<IntPair*> ap = {&p12, &p21};
  typedef IndirectIterator<decltype(ap.begin())> PTI;
  PTI p = --ap.end();
  p->first = 3, p->second = 4;
  EXPECT_EQ(IntPair(3, 4), *ap.back());
}

TEST(IteratorTest, CustomProxy) {
  struct Minus1Transformer {
    struct Proxy {
      Proxy& operator=(int v) { *this->v = v + 1; return *this; }
      operator int() const { return *v - 1; }
      int* v;
    };
    Proxy operator()(int& v) const { return Proxy{&v}; }
  };
  typedef TransformedIterator<int*, Minus1Transformer> TI;
  int x = 5;
  TI ti = &x;
  int act = *ti;
  EXPECT_EQ(4, act);
  *ti = 9;
  EXPECT_EQ(9, *ti);
  EXPECT_EQ(10, x);  // validate the pointed data is set to +1
}

TEST(IteratorTest, Const) {
  struct Iter : public std::iterator<std::forward_iterator_tag,
                                    const char> {
    Iter() = default;
    Iter(const char* str) : str_(str), cur_(0) {}
    const char operator*() const { return str_[cur_]; }
    Iter& operator++() { ++cur_; return *this; }
    const char* str_;
    size_t cur_;
  };

  struct UpcaseTransformer {
    const char operator()(const char& c) const {
      return toupper(c);
    }
  };

  typedef TransformedIterator<Iter, UpcaseTransformer> TI;
  TI it("ftw");
  EXPECT_EQ('F', *it);
  EXPECT_EQ('T', *++it);
  EXPECT_EQ('W', *++it);
  // *it = 'a';  // compile error - OK

  std::vector<char> v = {'f', 'o', 'o'};
  typedef TransformedIterator<decltype(v.cbegin()),
                              UpcaseTransformer> VTI;
  VTI vti = v.cbegin();
  EXPECT_EQ('F', *vti);
  EXPECT_EQ('f', v.front());  // validate the vector is unmodified
  // *vti = 'a';  // compile error - OK
}
