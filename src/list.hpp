#ifndef CPPVIEWS_SRC_LIST_HPP_
#define CPPVIEWS_SRC_LIST_HPP_

#include "portion.hpp"
#include "portion_helper.hpp"
#include "view.hpp"
#include "util/immutable_skip_list.hpp"
#include "util/poly_vector.hpp"

#include <vector>

namespace v {

template<typename T, class P = PortionBase<T> >
using PortionVector = PolyVector<P, PortionBase<T>, PortionFactory>;

namespace {

template<class PolyContainer,
         typename T = typename PolyContainer::Pointer::element_type::ValueType>
struct LinearIndexer {
  LinearIndexer(const PolyContainer& portions) : portions_(portions) {}

  const T& operator()(size_t index) const {
    size_t cum_size = 0, i_to = portions_.size();
    for (size_t i = 0; i < i_to; ++i) {
      const auto& p = portions_[i];
      if ((cum_size += p.size()) > index)
        return p.get(index - (cum_size - p.size()));
    }
    return portions_[i_to - 1].get(portions_[i_to - 1].size()); // not found
  }

 private:
  const PolyContainer& portions_;
};

template<class PolyContainer,
         typename T = typename PolyContainer::Pointer::element_type::ValueType>
struct LogarithmicIndexer {
  LogarithmicIndexer(const PolyContainer& portions) :
      bsg_(portions),
      fwd_(portions.size(), bsg_) {
  }

  inline const T& operator()(size_t index) const {
    auto pos = fwd_.get(index);
    return bsg_.portions_[pos.first].get(pos.second);
  }

 private:
  struct BucketSizeGetter {
    const PolyContainer& portions_;
    BucketSizeGetter(const PolyContainer& portions) : portions_(portions) {}
    inline size_t operator()(size_t bucket_index) const {
      return portions_[bucket_index].size();
    }
  } bsg_;
  ImmutableSkipList<BucketSizeGetter> fwd_;
  // TODO optimize short jumps (need also a "backard" skip list)
  // ImmutableSkipList<ReverseBucketSizeGetter> bwd_;
};

}  // namespace

template<typename T, class P = PortionBase<T>,
         class Indexer = LogarithmicIndexer<PortionVector<T, P> > >
class List : public View<T>, protected PortionHelper<P, T> {
 public:
  typedef typename PortionVector<T, P>::Pointer PortionPointer;

  // List() : indexer_(portions_) {}
  List(PortionVector<T, P>&& pv)
      : View<T>(portions_.size()),
        portions_(std::forward<PortionVector<T, P> >(pv)),
        indexer_(portions_) {
    portions_.Shrink();
  }

  inline void set(size_t index, const T& value) const {
    const_cast<T&>(this->get(index)) = value;
  }

  inline const T& get(size_t index) const {
    return indexer_(index);
  }

  inline size_t max_size() const { return portions_.max_size(); }

 private:
  PortionVector<T, P> portions_;
  Indexer indexer_;
};

template<typename T, class P>
constexpr List<T, P> MakeList(PortionVector<T, P>&& pv) {
  return List<T, P>(std::forward<PortionVector<T, P> >(pv));
}

}  // namespace v

#endif  /* CPPVIEWS_SRC_LIST_HPP_ */
