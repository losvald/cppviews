#include "../src/util/fake_pointer.hpp"
#include "test.hpp"

#include <vector>

template<typename T>
struct Helper {
  inline void Reset(T& p, T& value) const { p = value; }
  inline T& Dereference(T& p) const { return p; }
};

template<typename T>
struct Helper<FakePointer<T> > {
  inline void Reset(FakePointer<T>& p, T& value) const { *p = value; }
  inline T& Dereference(FakePointer<T>& p) const { return *p; }
};

template<typename T>
class FakePointerSpeedtest : public ::testing::Test,
                             protected Helper<T> {
 public:
  const int gArrSize = 1 << 10;

  FakePointerSpeedtest() {
  }
};

typedef ::testing::Types<
  int,
  FakePointer<int>
  > ObjectTypes;
TYPED_TEST_CASE(FakePointerSpeedtest, ObjectTypes);

TYPED_TEST(FakePointerSpeedtest, Dereference100M) {
  std::vector<TypeParam> objs(this->gArrSize);
  for (int i = 0; i < this->gArrSize; ++i)
    this->Reset(objs[i], i);

  int hash = 1;
  for (int itr = 0; itr < 100000000; ++itr) {
    hash = this->Dereference(objs[itr & (this->gArrSize - 1)]) + hash * 31;
  }
  EXPECT_NE(0, hash);
}
