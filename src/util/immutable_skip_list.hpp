#ifndef CPPVIEWS_SRC_UTIL_IMMUTABLE_SKIP_LIST_HPP_
#define CPPVIEWS_SRC_UTIL_IMMUTABLE_SKIP_LIST_HPP_

#include "bit_twiddling.hpp"

#include <algorithm>
#include <utility>
#include <vector>

#include <cassert>

namespace isl {

struct SingleElementBucketSizeGetter {
  inline size_t operator()(size_t bucket_index) const {
    return 1;
  }
};

}  // namespace isl

template<class BucketSizeGetter = isl::SingleElementBucketSizeGetter>
class ImmutableSkipList {
 public:
  // position in the skiplist: (bucket_index, local_index)
  typedef std::pair<size_t, size_t> Position;

  ImmutableSkipList(
      size_t bucket_count,
      const BucketSizeGetter& bucket_size_getter = BucketSizeGetter())
      : bkt_cnt_(bucket_count),
        size_getter_(bucket_size_getter),
        skip_cnts_(Pow2RoundUp(bucket_count) + (bucket_count < 2)),
        skip_lvl_max_(FindFirstSet(skip_cnts_.size()) - 3u) {
        // Note: skip_lvl_max_ is size_t(-1) for bucket_count == 0
    // fast-path for nearly empty skip list
    if (bucket_count <= 2) {
      auto skip_cnts_it = skip_cnts_.end();
      switch (bucket_count) {
        case 2:
          *--skip_cnts_it = size_getter_(0) + size_getter_(1);
          // fall through
        default:
          *--skip_cnts_it = -1;         // sentinel used for searching
      }
      return;
    }

    // Avoid unnecessary branching (and zero-assignment) overhead by using:
    // - using size_getter_ directly to initialize the last level of the tree
    // - zeroing out skip counts for out-of-range bucket indexes efficiently
    auto skip_cnts_noskip_cnt = (skip_cnts_.size() - bkt_cnt_) >> 1;
    auto skip_cnts_it = skip_cnts_.end();
    std::fill_n(skip_cnts_it - skip_cnts_noskip_cnt, skip_cnts_noskip_cnt, 0);
    skip_cnts_it -= skip_cnts_noskip_cnt;
    size_t bkt_ind = bkt_cnt_;
    if (bkt_cnt_ & 1)
      *--skip_cnts_it = size_getter_(--bkt_ind);
    while (bkt_ind >= 2) {
      size_t a = size_getter_(--bkt_ind), b = size_getter_(--bkt_ind);
      *--skip_cnts_it = a + b;
    }

    // initialize the skip counts in the remaining levels in linear time
    assert(skip_cnts_it - skip_cnts_.begin() == (skip_cnts_.size() >> 1));
    auto skip_cnts_fast_it = skip_cnts_.cend();
    for (auto lvl_size = skip_cnts_.size() >> 1; lvl_size >>= 1; ) {
      for (auto itr = lvl_size; itr--; )
        *--skip_cnts_it = *--skip_cnts_fast_it + *--skip_cnts_fast_it;
    }
    assert(skip_cnts_it == ++skip_cnts_.begin());
    *--skip_cnts_it = -1;               // sentinel used for searching
  }

  // Returns the position of the global_index-th element,
  // starting at offset-th bucket.
  Position get(size_t global_index, size_t offset = 0) const {
    // normalize the position to be relative to the bucket that is a left child
    global_index += -(offset & 1) & size_getter_(offset & ~1);
    const size_t ind_to = skip_cnts_.size();
    size_t ind = (ind_to + offset) >> 1;
    if (skip_cnts_[ind] > global_index) // in offset-th bucket or its sibling
      ind <<= 1;
    else {                              // otherwise, follow skip lists
      if (offset && (offset & 0x1FFu)) { // if < 8 levels, ascend linearly
        // (note: "offset &&" is an optimization)
        // ascend the tree by moving up (or up-back) - O(lg N) steps
        for (auto ind_next = ind; skip_cnts_[ind_next >>= 1] <= global_index;
             ind = ind_next) {
          // adjust the global_index if it was a right child (an up-back skip)
          global_index += -(ind & 1) & skip_cnts_[ind_next << 1];
        }
      } else {                   // otherwise, binary search is faster
        // ascend the tree using at most lg(levels) jumps - O(lg lg N) steps
        decltype(skip_lvl_max_) ind_rs_lo(0);
        for (auto ind_rs_hi = skip_lvl_max_; ind_rs_lo < ind_rs_hi;) {
          decltype(ind_rs_hi) ind_rs_mid = (ind_rs_lo + ind_rs_hi + 1u) >> 1;
          if (skip_cnts_[ind >> ind_rs_mid] <= global_index)
            ind_rs_lo = ind_rs_mid;
          else
            ind_rs_hi = ind_rs_mid - 1u;
        }
        ind >>= ind_rs_lo;
      }

      // descend the tree by moving forward-down - O(lg N) steps
      global_index -= skip_cnts_[ind++];
      if ((ind <<= 1) < ind_to) {       // optimize by unrolling 1st iteration
        do {
          if (skip_cnts_[ind] <= global_index) {
            global_index -= skip_cnts_[ind++];
          }
        } while ((ind <<= 1) < ind_to);
      }
    } // for random skip of N: at most 1/4 lg lg N + 7/4 lg N steps - O(lg N)

    // normalize the position to be relative to the bucket that is a left child
    ind -= ind_to;         // compute bucket index (out of leaf index)
    size_t left_size = size_getter_(ind); // left bucket always exists
    size_t right = -(left_size <= global_index);
    return Position(ind - right, global_index - (right & left_size));
  }

  inline size_t bucket_count() const { return bkt_cnt_; }

  // Returns the skip count for a specific tree node index.
  size_t skip_count(size_t index) const { return skip_cnts_[index]; }

  // Returns the maximum index for skip counts (min is 1).
  size_t skip_count_max_index() const { return skip_cnts_.size() - 1; }

private:
  size_t bkt_cnt_;
  BucketSizeGetter size_getter_;
  std::vector<size_t> skip_cnts_;
  unsigned skip_lvl_max_;
};

#endif  /* CPPVIEWS_SRC_UTIL_IMMUTABLE_SKIP_LIST_HPP_ */
