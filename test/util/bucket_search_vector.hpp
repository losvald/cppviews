#ifndef CPPVIEWS_TEST_UTIL_BUCKET_SEARCH_VECTOR_HPP_
#define CPPVIEWS_TEST_UTIL_BUCKET_SEARCH_VECTOR_HPP_

#include "../src/util/immutable_skip_list.hpp"
#include "test.hpp"

#include <utility>
#include <vector>

namespace bsv {

struct SingleElementBucketSizeGetter {
  inline size_t operator()(size_t bucket_index) const {
    return 1;
  }
};

}  // namespace bsv

template<class BucketSizeGetter = bsv::SingleElementBucketSizeGetter>
class BucketSearchVector {
 public:
  typedef std::pair<size_t, size_t> Position;

  BucketSearchVector(
      size_t bucket_count,
      const BucketSizeGetter& bucket_size_getter = BucketSizeGetter())
      : bkt_cnt_(bucket_count),
        size_getter_(bucket_size_getter),
        cum_sizes_((bucket_count + 1) >> 1) {
    if (bkt_cnt_) {
      cum_sizes_[0] = 0;
      for (size_t i = 0; i + 1 < cum_sizes_.size(); ++i)
        cum_sizes_[i + 1] = cum_sizes_[i] +
            size_getter_(i << 1) + size_getter_((i << 1) | 1);
    }
  }

  Position get(size_t global_index) const {
    size_t ind = 0;
    for (size_t ind_hi = cum_sizes_.size() - 1; ind < ind_hi; ) {
      size_t ind_mid = (ind + ind_hi + 1) >> 1;
      if (cum_sizes_[ind_mid] <= global_index) ind = ind_mid;
      else ind_hi = ind_mid - 1;
    }

    // normalize the position to be relative to a left bucket leaf
    global_index -= cum_sizes_[ind];
    ind <<= 1;                     // left bucket index (left of leaf)
    size_t left_size = size_getter_(ind);  // left bucket exists, so no GetSize
    size_t right = (left_size <= global_index);
    return Position(ind + right, global_index - right * left_size);
  }

  inline Position get(size_t global_index, size_t offset) const {
    return get(global_index + (offset & 1) * size_getter_(offset & ~1) +
               cum_sizes_[offset >> 1]);
  }

  inline size_t bucket_count() const { return bkt_cnt_; }

 private:
  size_t bkt_cnt_;
  BucketSizeGetter size_getter_;
  std::vector<size_t> cum_sizes_;
};

#endif  /* CPPVIEWS_TEST_UTIL_BUCKET_SEARCH_VECTOR_HPP_ */
