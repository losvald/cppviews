#ifndef CPPVIEWS_SRC_UTIL_IMMUTABLE_SKIP_LIST_HPP_
#define CPPVIEWS_SRC_UTIL_IMMUTABLE_SKIP_LIST_HPP_

#include "bit_twiddling.hpp"

#include <algorithm>
#include <utility>
#include <vector>

#include <cassert>

#include <iostream>

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
  // position in the skiplist: (bucket_index, local_index)
  typedef std::pair<size_t, size_t> Position;

  ImmutableSkipList(
      size_t bucket_count,
      const BucketSizeGetter& bucket_size_getter = BucketSizeGetter())
      : bkt_cnt_(bucket_count),
        size_getter_(bucket_size_getter),
        skip_cnts_(Pow2RoundUp(bucket_count) + (bucket_count == 0)) {
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

  Position get(size_t global_index) const {
    size_t ind = skip_cnts_.size() >> 1;
    if (skip_cnts_[ind] <= global_index) {
      bool asc;
      do {
        // unsigned height = 0;
        size_t ind_next = ind;
        while (skip_cnts_[ind_next >>= 1] <= global_index) {
          assert(ind_next);
          ind = ind_next;
          // std::cout << "up " << skip_cnts_[ind] << std::endl;
          // ++height;
        }

        // ind_next != (ind >> height)
        if ((ind_next << 1) != ind)
          break;

        // std::cout << "skip " << ind + 1 << ' ' << skip_cnts_[ind] << std::endl;
        assert(skip_cnts_[ind] <= global_index);
        global_index -= skip_cnts_[ind++];
      } while (true);

      const size_t ind_to = skip_cnts_.size();
      do {
        while ((ind <<= 1) < ind_to && skip_cnts_[ind] > global_index) {
          // std::cout << "down " << ind << ' ' << skip_cnts_[ind] << std::endl;
        }
        if (ind >= ind_to) {
          ind >>= 1;
          break;
        }

        // std::cout << "skip " << ind + 1 << ' ' << skip_cnts_[ind] << std::endl;
        assert(skip_cnts_[ind] <= global_index);
        global_index -= skip_cnts_[ind++];
      } while (true);
    }

    assert(ind >= (skip_cnts_.size() >> 1));
    assert(ind < skip_cnts_.size());
    assert(global_index < skip_cnts_.at(ind));
    // std::cout << ind << ' ' << global_index << ' ' << std::endl;
    ind = (ind << 1) - skip_cnts_.size();

    size_t left_size = GetSize(ind);
    unsigned left = (global_index < left_size);
    ind += !left;
    global_index -= (left_size << 1) - (left_size << left);
    // the above is equivalent to the following but without branching and mult
    // if (!left) ind += !left, global_index -= left_size;
    return Position(ind, global_index);
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
