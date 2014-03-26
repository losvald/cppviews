#ifndef CPPVIEWS_SRC_UTIL_IMMUTABLE_SKIP_LIST_HPP_
#define CPPVIEWS_SRC_UTIL_IMMUTABLE_SKIP_LIST_HPP_

#include "bit_twiddling.hpp"

#include <algorithm>
#include <vector>

#include <cassert>

namespace {

struct SingleElementBucketSizeGetter {
  inline size_t operator()(size_t bucket_index) const {
    return 1;
  }
};

}  // namespace

template<class BucketSizeGetter = SingleElementBucketSizeGetter>
class ImmutableSkipList {
 public:
  ImmutableSkipList(
      size_t bucket_count,
      const BucketSizeGetter& bucket_size_getter = BucketSizeGetter())
      : bkt_cnt_(bucket_count),
        size_getter_(bucket_size_getter),
        skip_cnts_(Pow2RoundUp(bucket_count)) {
    // fast-path for an empty skip list
    if (bucket_count == 0)
      return;

    // Avoid unnecessary branching (and zero-assignment) overhead by using:
    // - using size_getter_ directly to initialize the last level of the tree
    // - zeroing out the skip counts for out-of-range bucket indexes efficiently
    auto skip_cnts_noskip_cnt = (skip_cnts_.size() - bkt_cnt_) >> 1;
    auto skip_cnts_it = skip_cnts_.end();
    std::fill_n(skip_cnts_it - skip_cnts_noskip_cnt, skip_cnts_noskip_cnt, 0);
    skip_cnts_it -= skip_cnts_noskip_cnt;
    size_t bkt_ind = bkt_cnt_;
    if (bkt_cnt_ & 1)
      *--skip_cnts_it = size_getter_(--bkt_ind);
    while (bkt_ind >= 2)
      *--skip_cnts_it = size_getter_(--bkt_ind) + size_getter_(--bkt_ind);

    // initialize the skip counts in the remaining levels in linear time
    assert(skip_cnts_it - skip_cnts_.begin() == (skip_cnts_.size() >> 1));
    auto skip_cnts_fast_it = skip_cnts_.cend();
    for (auto lvl_size = skip_cnts_.size() >> 1; lvl_size >>= 1; ) {
      for (auto itr = lvl_size; itr--; )
        *--skip_cnts_it = *--skip_cnts_fast_it + *--skip_cnts_fast_it;
    }
  }

  inline size_t bucket_count() const { return bkt_cnt_; }

  // Returns the skip count for a specific tree node index.
  size_t skip_count(size_t index) const { return skip_cnts_[index]; }

  // Returns the maximum index for skip counts (min is 1).
  size_t skip_count_max_index() const { return skip_cnts_.size() - 1; }

private:
  inline size_t GetSize(size_t bkt_ind) const {
    return bkt_ind < skip_cnts_.size() ? size_getter_(bkt_ind) : 0;
  }

  size_t bkt_cnt_;
  BucketSizeGetter size_getter_;
  std::vector<size_t> skip_cnts_;
};

#endif  /* CPPVIEWS_SRC_UTIL_IMMUTABLE_SKIP_LIST_HPP_ */
