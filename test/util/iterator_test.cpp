#include "../../src/util/iterator.hpp"
#include "../test.hpp"

#include <initializer_list>
#include <list>
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

  // validate non-const to const conversion
  typedef IndirectIterator<std::vector<int*>::const_iterator,
                           std::vector<int*>::iterator> CTI;
  ti = a.begin();
  CTI cti = ti;
  EXPECT_EQ(10, *cti);
  cti = a.cbegin(); // validate construction from non-transformed const iter
  EXPECT_EQ(10, *cti);
  // ti = cti;  // compile error - OK

  // validate change via reference
  typedef std::iterator_traits<TI>::reference TIRef;
  TIRef r = *TI(a.begin());
  cti = a.cbegin();
  r = 100;
  EXPECT_EQ(100, *a.front());
  EXPECT_EQ(100, *cti); // validate the change is seen by the const iterator

  // validate change via pointer
  typedef std::pair<int, int> IntPair;
  IntPair p12(1, 2), p21(2, 1);
  std::vector<IntPair*> ap = {&p12, &p21};
  typedef IndirectIterator<decltype(ap.begin())> PTI;
  PTI p = --ap.end();
  p->first = 3, p->second = 4;
  EXPECT_EQ(IntPair(3, 4), *ap.back());
}

template<typename T>
void Minus1TransformerHelper(T* this_v, T v) { *this_v = v + 1; }
template<typename T>
void Minus1TransformerHelper(const T* this_v, T v) {}

template<typename T>
struct Minus1Transformer {
  struct Proxy {
    Proxy& operator=(T v) { Minus1TransformerHelper(this->v, v); return *this; }
    operator T() const { return *v - 1; }
    T* v;
  };
  Proxy operator()(T& v) const { return Proxy{&v}; }
};


TEST(IteratorTest, TransformedCustomProxy) {
  typedef TransformedIterator<int*, Minus1Transformer<int> > TI;
  int x = 5;
  TI ti = &x;
  int act = *ti;
  EXPECT_EQ(4, act);
  *ti = 9;
  EXPECT_EQ(9, *ti);
  EXPECT_EQ(10, x);  // validate the pointed data is set to +1

  // validate non-const to const conversion
  typedef TransformedIterator<const int*, Minus1Transformer<const int>,
                              int*
                              // Minus1Transformer<const int>
                              > CTI;
  ti = &x;
  CTI cti = ti;
  EXPECT_EQ(9, *cti);
  cti = &x;  // validate construction from non-transformed const iter
  *cti++ = 10;
  EXPECT_EQ(9, *ti);
  // ti = cti;  // compile error - OK
}

TEST(IteratorTest, TransformedConst) {
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

template<typename T>
class EverySecondArrayIter
    : public DefaultIterator<EverySecondArrayIter<T>,
                             std::random_access_iterator_tag, T> {
  V_DEFAULT_ITERATOR_DERIVED_HEAD(EverySecondArrayIter);

 public:
  EverySecondArrayIter(T* arr) : cur_(arr) {}

  template<typename T2>
  EverySecondArrayIter(const EverySecondArrayIter<T2>& other,
                       EnableIfIterConvertible<T2, T, Enabler> = Enabler())
      : cur_(other.cur_) {}

 protected:
  void Advance(int n) {
    cur_ += 2 * n;
  }
  template<typename T2>
  typename DefaultIterator_::difference_type
  DistanceTo(const EverySecondArrayIter<T2>& to) const {
    return (to.cur_ - cur_) / 2;
  }
  template<typename T2>
  bool IsEqual(const EverySecondArrayIter<T2>& other) const {
    return cur_ == other.cur_;
  }
  T& ref() const { return *cur_; }

 private:
  template<class> friend class EverySecondArrayIter;
  T* cur_;
};

TEST(IteratorTest, DefaultRandomAccess) {
  std::array<int, 5> a{ {10, 20, 30, 40, 50} };
  typedef EverySecondArrayIter<int> ESI;

  ESI esi(&a.front());
  EXPECT_EQ(10, *esi);
  EXPECT_EQ(30, *++esi);
  EXPECT_EQ(30, *esi++);
  EXPECT_EQ(50, *esi);

  // validate modification through iterator are transient
  *esi = 500;
  EXPECT_EQ(500, a[4]);
  *esi = 50;
  EXPECT_EQ(50, a[4]);

  // validate forward iterator operations
  esi = ESI(&a.front());
  ASSERT_EQ(esi, ESI(&a.front()));
  ASSERT_FALSE(esi != ESI(&a.front()));
  ASSERT_NE(esi, ESI(&a.back()));
  EXPECT_EQ(10, *esi++);
  EXPECT_EQ(30, *esi);
  EXPECT_EQ(50, *++esi);
  EXPECT_EQ(esi, ESI(&a.back()));

  // validate bidirectional iterator operations
  EXPECT_EQ(50, *esi--);
  EXPECT_NE(esi, ESI(&a.back()));
  EXPECT_EQ(30, *esi);
  EXPECT_EQ(10, *--esi);
  EXPECT_EQ(esi, ESI(&a.front()));

  // validate random access iterator operations
  esi = &a.front();
  EXPECT_LT(esi, esi + 1);
  EXPECT_FALSE(esi < esi);
  EXPECT_LE(esi, esi);
  EXPECT_LE(esi, esi + 1);
  EXPECT_FALSE(esi + 1 <= esi);
  EXPECT_GT(esi + 1, esi);
  EXPECT_FALSE(esi > esi);
  EXPECT_GE(esi, esi);
  EXPECT_GE(esi + 1, esi);
  EXPECT_FALSE(esi >= esi + 1);
  EXPECT_EQ(50, *(esi += 2));
  EXPECT_EQ(10, *(esi -= 2));
  EXPECT_EQ(50, *(ESI(&a.front()) + 2));
  EXPECT_EQ(10, *(ESI(&a.back()) - 2));
  EXPECT_EQ(50, *(2 + ESI(&a.front())));
  EXPECT_EQ(10, *(-2 + ESI(&a.back())));
  EXPECT_EQ(10, *(-2 + ESI(&a.back())));
  EXPECT_EQ(ESI(&a.back()) - ESI(&a.front()), 2);
  EXPECT_EQ(ESI(&a.front()) - ESI(&a.front() + 2), -1);

  // validate non-const to const conversion and some interoperability
  typedef EverySecondArrayIter<const int> CESI;
  static_assert(std::is_convertible<ESI, CESI>::value, "non-const -> const");
  static_assert(!std::is_convertible<CESI, ESI>::value, "! const -> non-const");

  // EverySecondArrayIt<char> cesi2 = static_cast<char*>(
  //     static_cast<void*>(a));
  // EXPECT_NE(cesi2, esi);  // compile error - OK: no match for operator!=
  // EXPECT_NE(esi, cesi2);  // compile error - OK: no match for operator!=

  CESI cesi(&a.front());
  esi = ESI(&a.back());
  EXPECT_NE(cesi, esi);
  EXPECT_NE(esi, cesi);
  ++cesi;
  EXPECT_EQ(30, *cesi);
  cesi = esi;
  ASSERT_EQ(50, *cesi);
  EXPECT_EQ(cesi, esi);
  EXPECT_EQ(esi, cesi);

  // validate more interoperability
  esi = &a.front();
  cesi = &a.front();
  EXPECT_LT(cesi, esi + 1);
  EXPECT_LT(esi, cesi + 1);
  EXPECT_LE(esi, cesi);
  EXPECT_LE(cesi, esi);
  EXPECT_GT(1 + esi, cesi);
  EXPECT_GT(1 + cesi, esi);
  EXPECT_GE(esi, cesi);
  EXPECT_GE(cesi, esi);
  EXPECT_EQ(2, ESI(&a.back()) - cesi);
  EXPECT_EQ(2, CESI(&a.back()) - esi);
}

// it is also possible to use DefaultIter with inner classes, but it's messy

template<typename T>
class PointerList {
 public:
  class Iterator : public DefaultIterator<
    Iterator,
    std::forward_iterator_tag, T> {
    V_DEFAULT_ITERATOR_DERIVED_HEAD(Iterator);

    typedef typename std::conditional<
      std::is_const<T>::value,
      typename std::list<typename std::remove_cv<T>::type*>::const_iterator,
      typename std::list<T*>::iterator>::type InnerIter;
    InnerIter cur_;
   public:
    Iterator(InnerIter cur) : cur_(cur) {}
    Iterator(const Iterator& other) : cur_(other.cur_) {}
    // TODO: support non-const to const conversion
    // (PointerList<...>::Iterator fails because of incomplete outer class)
   protected:
    void Increment() { ++cur_; }
    bool IsEqual(const Iterator& other) const { return cur_ == other.cur_; }
    T& ref() const { return **cur_; }
  };
  typedef typename PointerList<const T>::Iterator ConstIterator;

  PointerList(std::initializer_list<T*> init_list) : list_(init_list) {}
  Iterator begin() { return Iterator(list_.begin()); }
  Iterator end() { return Iterator(list_.end()); }
  ConstIterator cbegin() { return ConstIterator(list_.cbegin()); }
  ConstIterator cend() { return ConstIterator(list_.cend()); }

 private:
  std::list<T*> list_;
};

TEST(IteratorTest, DefaultForward) {
  int x = 10, y = 20, z = 30;
  PointerList<int> l({&x, &y, &z});

  // validate forward iterator operations
  typedef decltype(l.begin()) PLI;
  PLI pli(l.begin());
  ASSERT_EQ(pli, PLI(l.begin()));
  ASSERT_FALSE(pli != PLI(l.begin()));
  ASSERT_NE(pli, PLI(l.end()));
  EXPECT_EQ(10, *pli++);
  EXPECT_EQ(20, *pli);
  EXPECT_EQ(30, *++pli);
  EXPECT_EQ(++pli, PLI(l.end()));

  // validate change via reference
  typedef decltype(l.cbegin()) CPLI;
  typedef std::iterator_traits<PLI>::reference PLIRef;
  PLIRef r = *PLI(l.begin());
  CPLI cpli = l.cbegin();
  pli = l.begin();
  r = 100;
  EXPECT_EQ(100, *pli);
  EXPECT_EQ(100, *cpli); // validate the change is seen by the const iterator

  // TODO: validate non-const to const conversion
}
