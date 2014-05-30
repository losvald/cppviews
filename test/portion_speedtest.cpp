#include "../src/portion.hpp"
#include "test.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <set>
#include <type_traits>
#include <vector>

namespace {

template<class P>
struct PortionValue { typedef typename P::DataType Type; };

template<class P>
struct PortionValue<P*> { typedef P Type; };

template<class P, typename T>
struct Helper {
  inline const T& Get(const P& portion, size_t index) const {
    return portion[index];
  }

  inline void Set(P& portion, size_t index, const T& value) {
    portion[index] = value;
  }
};

template<typename T>
struct Helper<PortionBase<T>, T> {
  inline const T& Get(const PortionBase<T>& portion, size_t index) const {
    return portion.get(index);
  }

  inline void Set(PortionBase<T>& portion, size_t index, const T& value) {
    portion.get(index) = value;
  }
};

}  // namespace

template<class P>
class PortionSpeedTest : public ::testing::Test,
                         public Helper<P, typename PortionValue<P>::Type> {
 protected:
  typedef P PortionType;
  typedef typename PortionValue<P>::Type DataType;

  static const int gArrSize = 1 << 10;

  void SetUp() {
    hash_ = kHashMagic;
  }

  void TearDown() {
    EXPECT_NE(hash_, kHashMagic);
  }

  void InitArray(DataType* arr, unsigned size) const {
    std::fill(arr, arr + size, 0);
    for (int i = 1; i < size; ++i)
      arr[i] = 2104087157 * arr[i - 1] + i;
  }

  inline void Read(const P& portion, size_t index) {
    hash_ = hash_ * kHashPrime + hasher_(this->Get(portion, index));
  }

  inline void Write(P& portion, size_t index, const DataType& value) {
    this->Set(portion, index, value);
    hash_ = hash_ * kHashPrime + hasher_(value);
  }

  const DataType kHashPrime = 31;
  const DataType kHashMagic = DataType();

  std::hash<DataType> hasher_;
  DataType hash_;
};

namespace {

template<class P, typename T>
struct MyPortionFactory {
  MyPortionFactory(T* arr, size_t size);
  P operator()();
};

template<typename T>
struct MyPortionFactory<T*, T> {
  MyPortionFactory(T* arr, size_t size) : arr_(arr) {}
  T* operator()() { return arr_; }
 private:
  T* arr_;
};

template<typename T>
struct MyPortionFactory<Portion<T*>, T> {
  MyPortionFactory(T* arr, size_t size) : arr_(arr), size_(size) {}
  Portion<T*> operator()() { return MakePortion(arr_, size_); }
 private:
  T* arr_;
  size_t size_;
};

template<typename T>
struct MyPortionFactory<Portion<std::reverse_iterator<T*> >, T> {
  MyPortionFactory(T* arr, size_t size) : rarr_(arr + size), size_(size) {}
  Portion<std::reverse_iterator<T*> > operator()() {
    return MakePortion(rarr_, size_);
  }
 private:
  std::reverse_iterator<T*> rarr_;
  size_t size_;
};

template<typename T>
struct MyPortionFactory<Portion<typename std::vector<T>::iterator>, T> {
  MyPortionFactory(T* arr, size_t size) : c_(arr, arr + size) {}
  Portion<typename std::vector<T>::iterator> operator()() {
    return MakePortion(c_.begin(), c_.size());
  }
 private:
  std::vector<T> c_;
};

template<typename T>
struct MyPortionFactory<Portion<typename std::vector<T>::reverse_iterator>, T> {
  MyPortionFactory(T* arr, size_t size) : c_(arr, arr + size) {}
  Portion<typename std::vector<T>::reverse_iterator> operator()() {
    return MakePortion(c_.rbegin(), c_.size());
  }
 private:
  std::vector<T> c_;
};

}  // namespace

typedef ::testing::Types<
  int*,
  Portion<int*>,
  Portion<std::reverse_iterator<int*> >,
  Portion<std::vector<int>::iterator>,
  Portion<std::vector<int>::reverse_iterator>
  > PortionTypes;
TYPED_TEST_CASE(PortionSpeedTest, PortionTypes);

const int gLastPrime = 65521;

class PortionSpeedTestOverhead : public ::testing::Test {};

TEST_F(PortionSpeedTestOverhead, Read100MOverhead) {
  int last = 1;
  for (int itr = 0; itr < 100000000; ++itr) {
    last = itr + last * gLastPrime;
  }
  EXPECT_NE(last, 0);
}

TEST_F(PortionSpeedTestOverhead, Write100MOverhead) {
  int last = 1;
  for (int itr = 0; itr < 100000000; ++itr) {
    last = itr + last * gLastPrime;
  }
  EXPECT_NE(last, 0);
}

TYPED_TEST(PortionSpeedTest, Read100M) {
  int arr[this->gArrSize];
  this->InitArray(arr, this->gArrSize);
  MyPortionFactory<TypeParam, int> pf(arr, this->gArrSize);
  TypeParam p = pf();

  int last = 1;
  for (int itr = 0; itr < 100000000; ++itr) {
    this->Read(p, last & (this->gArrSize - 1));
    last = itr + last * gLastPrime;
  }
}

TYPED_TEST(PortionSpeedTest, Write100M) {
  int arr[this->gArrSize];
  this->InitArray(arr, this->gArrSize);
  MyPortionFactory<TypeParam, int> pf(arr, this->gArrSize);
  TypeParam p = pf();

  int last = 1;
  for (int itr = 0; itr < 100000000; ++itr) {
    this->Write(p, last & (this->gArrSize - 1), last ^ itr);
    last = itr + last * gLastPrime;
  }

  // ensure the array writes are not optimized away
  for (int i = 0; i < this->gArrSize; ++i)
    this->Read(p, i);
}

class PortionSpeedPolyTest : public PortionSpeedTest<PortionBase<int> > {
 protected:

  static const int gPolyPortionCount = 1 << 6;
  static const int gPolyArrSize = gArrSize / gPolyPortionCount;

  static void ClassSetup() {
    static_assert((gPolyPortionCount & -gPolyPortionCount) ==
                  gPolyPortionCount,
                  "Must be a power of 2");
    static_assert((gPolyArrSize & -gPolyArrSize) == gPolyArrSize,
                  "Must be a power of 2");
  }

  template<class InputIter>
  static PortionBase<int>* AllocPortion(InputIter iter) {
    auto p = MakePortion(iter, gPolyArrSize);
    return new decltype(p)(p);
  }

  void InitPortions(std::function<PortionBase<int>*(int index)> f) {
    for (int i = 0; i < gPolyPortionCount; ++i)
      ps[i].reset(f(i));
  }

  void Read100MTimes() {
      int last = 1;
      for (int itr = 0; itr < 100000000; ++itr) {
        this->Read(*ps[last & (gPolyPortionCount - 1)],
                   last & (gPolyArrSize - 1));
        last = itr + last * gLastPrime;
      }
  }

  void Write100MTimes() {
      int last = 1;
      for (int itr = 0; itr < 100000000; ++itr) {
        this->Write(*ps[last & (gPolyPortionCount - 1)],
                    last & (gPolyArrSize - 1),
                    last);
        last = itr + last * gLastPrime;
      }

      // ensure the array writes are not optimized away
      for (int pi = 0; pi < this->gPolyPortionCount; ++pi)
        for (int i = 0; i < this->gArrSize; ++i)
          this->Read(*ps[pi], i);
  }

  std::unique_ptr<PortionBase<int> > ps[gPolyPortionCount];
};

TEST_F(PortionSpeedPolyTest, Read100M1Class) {
  int arr[gPolyArrSize];
  this->InitArray(arr, gPolyArrSize);
  InitPortions([&](int i) { return AllocPortion(arr); });
  Read100MTimes();
}

TEST_F(PortionSpeedPolyTest, Read100M4Classes) {
  int arr[gPolyArrSize];
  this->InitArray(arr, gPolyArrSize);
  std::vector<int> v(arr, arr + gPolyArrSize);
  std::reverse_iterator<int*> rarr(arr + gPolyArrSize);
  InitPortions([&](int i) {
      if (i < gPolyPortionCount / 4)
        return AllocPortion(v.begin());
      if (i >= gPolyPortionCount - gPolyPortionCount / 4)
        return AllocPortion(v.rbegin());
      if (i < gPolyPortionCount / 2)
        return AllocPortion(rarr);
      return AllocPortion(arr);
      // for some reason the partitioning above results in much worse
      // than the following
      // switch (i % 4) {
      //   case 0: return AllocPortion(arr);
      //   case 1: return AllocPortion(v.begin());
      //   case 2: return AllocPortion(v.rend());
      //   default: return AllocPortion(rarr);
      // }
    });

  Read100MTimes();
}

TEST_F(PortionSpeedPolyTest, Write100M1Class) {
  int arr[gPolyArrSize];
  this->InitArray(arr, gPolyArrSize);
  InitPortions([&](int i) { return AllocPortion(arr); });
  Write100MTimes();
}

TEST_F(PortionSpeedPolyTest, Write100M4Classes) {
  int arr[gPolyArrSize];
  this->InitArray(arr, gPolyArrSize);
  std::vector<int> v(arr, arr + gPolyArrSize);
  std::reverse_iterator<int*> rarr(arr + gPolyArrSize);
  InitPortions([&](int i) {
      if (i < gPolyPortionCount / 4)
        return AllocPortion(v.begin());
      if (i >= gPolyPortionCount - gPolyPortionCount / 4)
        return AllocPortion(v.rbegin());
      if (i < gPolyPortionCount / 2)
        return AllocPortion(rarr);
      return AllocPortion(arr);
    });

  Write100MTimes();
}
