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

  // Returns the position of the global_index-th element,
  // starting at offset-th bucket.
  Position get(size_t global_index, size_t offset = 0) const {
#ifdef DEBUG
    auto global_index_init = global_index;
    size_t dir_cnts = 0, kOne = 1;
    enum DirShiftDbg {
      kUpShift = 0,
      kAscShift = (sizeof(dir_cnts) * CHAR_BIT) >> 2,
      kDownShift = kAscShift << 1,
      kDescShift = kAscShift + kDownShift
    };
#define INC_DIR_DBG(dir_shift)                  \
    (dir_cnts += kOne << dir_shift)
#define GET_DIR_DBG(dir_shift)                                  \
    ((dir_cnts >> (dir_shift)) & ((kOne << kAscShift) - 1))
#ifdef PRINT_DIR_DBG
#define CERR_DIR_DBG(ch) std::cerr << ch;
#else
#define CERR_DIR_DBG(ch)
#endif  // PRINT_DIR_DBG
#endif  // DEBUG

    size_t right = (offset & 1);
    offset &= ~1;
    global_index += right * size_getter_(offset);
    offset >>= 1;

    size_t ind = (skip_cnts_.size() >> 1) + offset;
    if (skip_cnts_[ind] <= global_index) {
      // unsigned height = 0;
      size_t ind_next = ind;
      while (skip_cnts_[ind_next >>= 1] <= global_index) {
        global_index += (ind & 1) * skip_cnts_[ind & ~1];
        ind = ind_next;
#ifdef DEBUG
        INC_DIR_DBG(kUpShift); CERR_DIR_DBG('>');
#endif  // DEBUG
      }

#ifdef DEBUG
      assert(skip_cnts_[ind] <= global_index);
      INC_DIR_DBG(kAscShift); CERR_DIR_DBG('a');
#endif  // DEBUG

      global_index -= skip_cnts_[ind++];

      const size_t ind_to = skip_cnts_.size();
      do {
        while ((ind <<= 1) < ind_to && skip_cnts_[ind] > global_index) {
#ifdef DEBUG
          INC_DIR_DBG(kDownShift); CERR_DIR_DBG('<');
#endif  // DEBUG
        }

        if (ind >= ind_to) {
          ind >>= 1;
          break;
        }

#ifdef DEBUG
        assert(skip_cnts_[ind] <= global_index);
        INC_DIR_DBG(kDescShift); CERR_DIR_DBG('d');
#endif  // DEBUG

        global_index -= skip_cnts_[ind++];
      } while (true);

#ifdef DEBUG
      unsigned steps = GET_DIR_DBG(kUpShift) + GET_DIR_DBG(kAscShift) +
          GET_DIR_DBG(kDownShift) + GET_DIR_DBG(kDescShift);
#ifdef PRINT_DIR_DBG
      std::cerr << " (" << steps << ")"
                << ' ' << GET_DIR_DBG(kUpShift)
                << ' ' << GET_DIR_DBG(kAscShift)
                << ' ' << GET_DIR_DBG(kDownShift)
                << ' ' << GET_DIR_DBG(kDescShift)
                << std::endl;
#endif  // PRINT_DIR_DBG
      // Verify the debug direction counts invariant
      assert(steps == 2 * GET_DIR_DBG(kUpShift) + 1);

      // Verify that the number of step is at most lg(global_index)
      assert((1 << (steps / 2)) <= global_index_init);

#undef INC_DIR_DBG
#undef GET_DIR_DBG
#endif  // DEBUG
    }

#ifdef MACRO
    assert(ind >= (skip_cnts_.size() >> 1));
    assert(global_index < skip_cnts_.at(ind));
#endif  // MACRO

    // normalize the position to be relative to a left bucket leaf
    ind = (ind << 1) - skip_cnts_.size();  // left bucket index (left of leaf)
    size_t left_size = size_getter_(ind);  // left bucket exists, so no GetSize
    right = (left_size <= global_index);
    return Position(ind + right, global_index - right * left_size);
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
