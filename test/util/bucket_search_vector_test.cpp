#include "bucket_search_vector.hpp"

#include "../src/util/bit_twiddling.hpp"
#include "test.hpp"

TEST(BucketSearchVectorTest, GetOffsetUniqueSizes) {
  struct BucketSizeGetter {
    size_t operator()(size_t bkt_ind) const { return 1 << bkt_ind; }
  } bsg;
  BucketSearchVector<BucketSizeGetter> sv(13, bsg);

  for (size_t off = 0; off < sv.bucket_count(); ++off) {
    size_t ind_to = (1U << sv.bucket_count()) - (1 << off);
    for (size_t ind = 0; ind < ind_to; ++ind) {
      auto p_act = sv.get(ind, off);
      size_t ind2 = ind + ((1 << off) - 1);
      size_t bkt_ind = FindFirstSet(ind2 + 1) - 1;
      decltype(p_act) p_exp(bkt_ind, ind2 - ((1 << bkt_ind) - 1));
      ASSERT_EQ(p_exp, p_act) << "get(" << ind << ", " << off << ") incorrect";
    }
  }
}
