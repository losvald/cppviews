#ifndef CPPVIEWS_SRC_UTIL_CHUNKED_ARRAY_HPP_
#define CPPVIEWS_SRC_UTIL_CHUNKED_ARRAY_HPP_

#include <algorithm>
#include <array>

template<typename T, unsigned kChunkSizeLog, size_t kSize = 0,
         class Alloc = std::allocator<T> >
class ChunkedArray {
 public:
  static constexpr size_t kChunkSize = size_t(1) << kChunkSizeLog;
  static constexpr size_t kChunkCount = (kSize + kChunkSize) >> kChunkSizeLog;

  ChunkedArray() {
    for (auto i = 0; i < kFullChunkCount; ++i)
      chunks_[i] = alloc_.allocate(kChunkSize);
    if (kFullChunkCount != kChunkCount)
      chunks_.back() = alloc_.allocate(kSize & (kChunkSize - 1));
  }

  ~ChunkedArray() {
    for (auto i = 0; i < kFullChunkCount; ++i)
      alloc_.deallocate(chunks_[i], kChunkSize);
    if (kFullChunkCount != kChunkCount)
      alloc_.deallocate(chunks_.back(), (kSize & (kChunkSize - 1)));
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
  Alloc alloc_;

  std::array<T*, kChunkCount> chunks_;
};

// specialize for case of runtime size (0)
template<typename T, unsigned kChunkSizeLog, class Alloc>
class ChunkedArray<T, kChunkSizeLog, 0, Alloc> {
 public:
  static constexpr size_t kChunkSize = size_t(1) << kChunkSizeLog;

  ChunkedArray() : chunks_(NULL), chunk_count_(0) {}
  ChunkedArray(size_t chunk_count) {
    this->Resize(chunk_count);
  }

  ~ChunkedArray() {
    this->DeallocateChunks(0);
    delete[] chunks_;
  }

  const T& operator[](size_t index) const {
    return chunks_[index >> kChunkSizeLog][index & (kChunkSize - 1)];
  }

  T& operator[](size_t index) {
    return const_cast<T&>((*static_cast<const ChunkedArray*>(this))[index]);
  }

  void Fill(const T& value) {
    for (decltype(chunk_count_) i = 0; i < chunk_count_; ++i)
      std::fill(chunks_[i], chunks_[i] + kChunkSize, value);
  }

  void Resize(size_t chunk_count) {
    this->DeallocateChunks(chunk_count);
    T** chunks = new T*[chunk_count];
    for (auto i = chunk_count_; i < chunk_count; ++i)
      chunks[i] = alloc_.allocate(kChunkSize);
    std::copy_n(chunks_, std::min(chunk_count_, chunk_count), chunks);
    delete[] chunks_;
    chunks_ = chunks;
    chunk_count_ = chunk_count;
  }

  size_t size() const { return chunk_count_ << kChunkSizeLog; }

 private:
  void DeallocateChunks(size_t offset) {
    for (auto i = offset; i < chunk_count_; ++i)
      alloc_.deallocate(chunks_[i], kChunkSize);
  }

  Alloc alloc_;
  T** chunks_;
  size_t chunk_count_;
};

#endif  /* CPPVIEWS_SRC_UTIL_CHUNKED_ARRAY_HPP_ */
