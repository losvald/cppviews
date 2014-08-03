#include "../../src/util/poly_vector.hpp"
#include "../test.hpp"

#include <algorithm>
#include <string>
#include <utility>

struct String : public std::string {
  template<typename... Args>
  String(Args&&... args) : std::string(//args...) {}
      std::forward<Args>(args)...) {}
  virtual ~String() {}          // make String polymorphic
};

struct ReversedString : public String {
  ReversedString(size_type count, char ch)
      : ReversedString(std::string(count, ch)) {}
  ReversedString(const std::string& s) : String(s.rbegin(), s.rend()) {}
  ReversedString() {}
};

struct SortedString : public String {
  SortedString(size_type count, char ch) : String(count, ch) {}
  SortedString(const std::string& s) : String(s) {
    std::sort(begin(), end());
  }
  SortedString() {}
};

typedef ::testing::Types<
  String,
  ReversedString,
  SortedString
  > StringTypes;
TYPED_TEST_CASE(PolyVectorTest, StringTypes);

template<typename T>
struct PolyVectorTest : public ::testing::Test {
};

#define V__POLYVEC_APP(type, arg, ...)               \
  template Append/*<type>*/(arg, ##__VA_ARGS__)

TYPED_TEST(PolyVectorTest, DefaultFactory) {
  PolyVector<TypeParam, String> pv;
  pv.template Append/*<TypeParam>*/(3, 'a');
  EXPECT_EQ("aaa", pv[0]);
  pv.V__POLYVEC_APP(TypeParam, "bb");
  EXPECT_EQ("bb", pv[1]);

  typename PolyVector<TypeParam, String>::ConstIterator cit = pv.cbegin();
  EXPECT_EQ(pv[0], *cit++);
  EXPECT_EQ(pv[1], *cit--);
  EXPECT_EQ(pv[0], *cit);
  EXPECT_EQ(pv[1], *++cit);
  EXPECT_EQ(pv[0], *--cit);
  EXPECT_EQ(pv.cbegin(), cit);
  EXPECT_EQ(pv.cend(), cit + 2);

  typename PolyVector<TypeParam, String>::Iterator it = pv.begin();
  *it = SortedString("a");
  EXPECT_EQ("a", pv[0]);
  cit = it;  // validate conversion to const iterator
  EXPECT_EQ("bb", *++cit);
  // *cit = SortedString("b");  // compile error - OK
  // cit->clear();  // compile error - OK

  const char* anagram = "slot";
  TypeParam s(anagram);
  PolyVector<TypeParam, TypeParam> pv_same;
  pv_same.V__POLYVEC_APP(TypeParam, anagram);
  EXPECT_EQ(s, pv_same[0]);
}

struct ReversedStringFactory {
  template<typename... Args>
  ReversedString operator()(Args&&... args) const {
    ADD_FAILURE();  // verify this functor is not ever called
  }
};

TEST(PolyVectorSameClassTest, FactoryNoOverload) {
  PolyVector<String, String, ReversedStringFactory> pvr;
  pvr.Append("bats");
  EXPECT_EQ("stab", pvr[0]);

  SortedString abot("boat");
  PolyVector<SortedString, SortedString> pv;
  pv.Append(abot);
  EXPECT_EQ("abot", pv[0]);
  // pv.Add(pv.cbegin(),
}

struct StringFactory {
  ReversedString operator()(const std::string& other) const {
    ADD_FAILURE();           // verify this functor is not ever called
    return ReversedString(other);
  }
  SortedString operator()() const {
    ADD_FAILURE();           // verify this functor is not ever called
    return SortedString();
  }
  String operator()(String::size_type count, char ch) const {
    ADD_FAILURE();           // verify this functor is not ever called
    return String();
  }
};

TEST(PolyVectorSameClassTest, FactoryOverload) {
  PolyVector<String, String, StringFactory> pv;

  pv.Append();
  EXPECT_NO_THROW({
      EXPECT_EQ("", dynamic_cast<SortedString&>(pv[0]));
    });
  EXPECT_ANY_THROW(dynamic_cast<ReversedString&>(pv[0]));

  pv.Append(std::string("bats"));
  EXPECT_NO_THROW({
      EXPECT_EQ("stab", dynamic_cast<ReversedString&>(pv[1]));
    });
  EXPECT_ANY_THROW(dynamic_cast<SortedString&>(pv[1]));

  pv.Append(3, 'X');
  EXPECT_ANY_THROW(dynamic_cast<ReversedString&>(pv[2]));
  EXPECT_ANY_THROW(dynamic_cast<SortedString&>(pv[2]));
  EXPECT_EQ("XXX", pv[2]);
}

struct AbstractRange {
  virtual int from() const = 0;
  virtual int to() const = 0;
  friend bool operator==(const AbstractRange& lhs, const AbstractRange& rhs) {
    return lhs.from() == rhs.from() && lhs.to() == rhs.to();
  }
  friend bool operator!=(const AbstractRange& lhs, const AbstractRange& rhs) {
    return !(lhs == rhs);
  }
};

struct SingletonRange : public AbstractRange {
  SingletonRange(int from) : from_(from) {}
  int from() const override { return from_; }
  int to() const override { return from_; }
  friend std::ostream& operator<<(::std::ostream& os, const SingletonRange& r) {
    return os << '[' << r.from_ << ']';
  }
 private:
  int from_;
};

struct Range : public AbstractRange {
  Range(int from, int to) : from_(from), to_(to) {}
  int from() const override { return from_; }
  int to() const override { return to_; }
  friend std::ostream& operator<<(::std::ostream& os, const Range& r) {
    return os << '[' << r.from_ << ' ' << r.to_ << ']';
  }
 private:
  int from_, to_;
};

struct RangeFactory {
  Range operator()(int from, int to) const { return Range(from, to); }
  SingletonRange operator()(int from) const { return SingletonRange(from); }
};

typedef ::testing::Types<
  AbstractRange,
  Range
  > RangeTypes;
TYPED_TEST_CASE(PolyVectorAbstractBaseTest, RangeTypes);

template<typename T>
struct PolyVectorAbstractBaseTest : public ::testing::Test {
};

TYPED_TEST(PolyVectorAbstractBaseTest, InsertAndErase) {
  PolyVector<TypeParam, AbstractRange, RangeFactory> pv;
  pv.Add(pv.begin(), 13, 42);
  EXPECT_EQ(Range(1, 10), *pv.Add(pv.begin(), 1, 10));
  EXPECT_EQ(Range(2, 20), *pv.Add(++pv.begin(), 2, 20));
  EXPECT_EQ(3, pv.size());
  EXPECT_EQ(Range(1, 10), pv[0]);
  EXPECT_EQ(Range(2, 20), pv[1]);
  EXPECT_EQ(Range(13, 42), pv[2]);

  pv.Add(pv.end(), 99, 999);
  pv.Erase(pv.begin() + 1, pv.begin() + 3);
  EXPECT_EQ(2, pv.size());
  EXPECT_EQ(Range(1, 10), pv.front());
  EXPECT_EQ(Range(99, 999), pv.back());

  pv.Erase(pv.begin(), pv.begin());
  EXPECT_EQ(2, pv.size());

  pv.Erase(pv.begin(), pv.end());
  EXPECT_EQ(0, pv.size());
}
