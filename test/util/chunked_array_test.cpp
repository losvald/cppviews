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

TYPED_TEST(ChunkedArrayTest, ReadWriteFixed) {
  ChunkedArray<int, TypeParam::value, 100> a;
  ASSERT_EQ(100, a.size());
  for (int i = 0; i < a.size(); ++i)
    ASSERT_EQ(i + 100, a[i] = i + 100) << "Failed read/write at index " << i;

  ChunkedArray<int, TypeParam::value, 1> singleton;
  EXPECT_EQ(1, singleton.size());
  EXPECT_EQ(42, singleton[0] = 42);
}

TYPED_TEST(ChunkedArrayTest, FillFixed) {
  ChunkedArray<int, TypeParam::value, 100> a;
  a.Fill(13);
  ASSERT_EQ(100, a.size());
  for (int i = 0; i < a.size(); ++i)
    ASSERT_EQ(13, a[i]);
}

TYPED_TEST(ChunkedArrayTest, ReadWriteResizeable) {
  constexpr auto kChunkSizeLog = TypeParam::value;
  ChunkedArray<int, kChunkSizeLog> a;
  a.Resize(2);
  ASSERT_EQ(2 << kChunkSizeLog, a.size());
  EXPECT_EQ(-3, a[0] = -3);
  EXPECT_EQ(-17, a[1] = -17);

  // expand
  a.Resize(5);
  ASSERT_EQ(5 << kChunkSizeLog, a.size());
  EXPECT_EQ(15, a[2] = 15);
  EXPECT_EQ(-17, a[1]);  // verify not overwritten
  EXPECT_EQ(-1, a[4] = -1);

  // shrink
  a.Resize(1);
  EXPECT_EQ(-3, a[0]);  // verify not overwritten
  EXPECT_EQ(10, a[0] = 10);  // check overwriting works

  // expand it again
  a.Resize(100);
  ASSERT_EQ(100 << kChunkSizeLog, a.size());
  for (int i = 0; i < a.size(); ++i)
    ASSERT_EQ(i + 100, a[i] = i + 100) << "Failed read/write at index " << i;
}

TYPED_TEST(ChunkedArrayTest, FillResizeable) {
  constexpr auto kChunkSizeLog = TypeParam::value;
  ChunkedArray<int, kChunkSizeLog> a;
  a.Fill(-1);  // verify robustness by filling empty array

  a.Resize(100);
  a.Fill(13);
  ASSERT_EQ(100 << kChunkSizeLog, a.size());
  for (int i = 0; i < a.size(); ++i)
    ASSERT_EQ(13, a[i]);

  // shrink it
  a.Resize(10);
  for (int i = 0; i < 10; ++i)
    ASSERT_EQ(13, a[i]);

  // expand it again
  a.Resize(73);
  ASSERT_EQ(73 << kChunkSizeLog, a.size());
  for (int i = 0; i < 10; ++i)
    ASSERT_EQ(13, a[i]);

  a.Fill(5);
  for (int i = 0; i < a.size(); ++i)
    ASSERT_EQ(5, a[i]);
}
