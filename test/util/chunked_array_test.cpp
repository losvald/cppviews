#include "../src/util/chunked_array.hpp"
#include "test.hpp"

#include <type_traits>

namespace {

template<unsigned k>
using Unsigned = std::integral_constant<unsigned, k>;

}  // namespace

template<class Unsigned>
class ChunkedArrayTest : public ::testing::Test {};

typedef ::testing::Types<
  Unsigned<0>,
  Unsigned<1>,
  Unsigned<4>
  > ChunkSizeLog;
TYPED_TEST_CASE(ChunkedArrayTest, ChunkSizeLog);

TYPED_TEST(ChunkedArrayTest, ReadWrite) {
  ChunkedArray<int, 100, TypeParam::value> a;
  ASSERT_EQ(100, a.size());
  for (int i = 0; i < a.size(); ++i)
    ASSERT_EQ(i + 100, a[i] = i + 100) << "Failed read/write at index " << i;

  ChunkedArray<int, 1, TypeParam::value> singleton;
  EXPECT_EQ(1, singleton.size());
  EXPECT_EQ(42, singleton[0] = 42);
}

TYPED_TEST(ChunkedArrayTest, Fill) {
  ChunkedArray<int, 100, TypeParam::value> a;
  a.Fill(13);
  ASSERT_EQ(100, a.size());
  for (int i = 0; i < a.size(); ++i)
    ASSERT_EQ(13, a[i]);
}
