#ifndef CPPVIEWS_SRC_UTIL_CHUNKED_ARRAY_HPP_
#define CPPVIEWS_SRC_UTIL_CHUNKED_ARRAY_HPP_

#include <array>

template<typename T, size_t kSize, unsigned kChunkSizeLog>
class ChunkedArray {
 public:
  static constexpr size_t kChunkSize = size_t(1) << kChunkSizeLog;
  static constexpr size_t kChunkCount = (kSize + kChunkSize) >> kChunkSizeLog;

  ChunkedArray() {
    for (auto i = 0; i < kFullChunkCount; ++i)
      chunks_[i] = new T[kChunkSize];
    if (kFullChunkCount != kChunkCount)
      chunks_.back() = new T[kSize & (kChunkSize - 1)];
  }

  ~ChunkedArray() {
    for (auto c : chunks_)
      delete[] c;
  }

  const T& operator[](size_t index) const {
    return chunks_[index >> kChunkSizeLog][index & (kChunkSize - 1)];
  }

  T& operator[](size_t index) {
    return const_cast<T&>((*static_cast<const ChunkedArray*>(this))[index]);
  }

  void Fill(const T& value) {
    for (auto i = 0; i < kFullChunkCount; ++i)
      std::fill(chunks_[i], chunks_[i] + kChunkSize, value);
    if (kFullChunkCount != kChunkCount)
      std::fill(chunks_.back(), chunks_.back() + (kSize & (kChunkSize - 1)),
                value);
  }

  static constexpr size_t size() { return kSize; }

 private:
  static constexpr size_t kFullChunkCount = kSize >> kChunkSizeLog;

  std::array<T*, kChunkCount> chunks_;
};

#endif  /* CPPVIEWS_SRC_UTIL_CHUNKED_ARRAY_HPP_ */
