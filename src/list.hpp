#ifndef CPPVIEWS_SRC_LIST_HPP_
#define CPPVIEWS_SRC_LIST_HPP_

#include "portion.hpp"
#include "portion_helper.hpp"
#include "view.hpp"
#include "util/immutable_skip_list.hpp"
#include "util/poly_vector.hpp"

#include <array>
#include <functional>
#include <vector>
#include <type_traits>

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

template<typename T, class P = PortionBase<T>, unsigned dims = 1,
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

  inline void set(const T& value, size_t index) const {
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

// a variadic function that is inlined as (size1 * ... * sizeN) at compile time
template<typename Size, typename... Sizes>
typename std::enable_if<std::is_same<Size, size_t>::value, Size>::type
constexpr SizeProduct(Size size, Sizes... sizes) {
  return size * SizeProduct<Sizes...>(sizes...);
}

template<>
size_t constexpr SizeProduct<size_t>(size_t size) { return size; }

// specialization for Implicit Lists (those that do not store portions)
template<typename T, unsigned dims, class Accessor>
class List<T, void, dims, Accessor> : public View<T> {
 public:
  typedef std::array<size_t, dims> Strides;

  template<typename... DimSizes>
  List(Accessor accessor, size_t size, DimSizes... sizes)
      : View<T>(SizeProduct(size, static_cast<size_t>(sizes)...)),
        accessor_(accessor),
        strides_({size, static_cast<size_t>(sizes)...}) {}

  template<typename... Indexes>
  T& operator()(Indexes... indexes) const {
    return *accessor_(indexes...);
  }

  template<typename... Indexes>
  void set(const T& value, Indexes... indexes) const {
    this->operator()(indexes...) = value;
  }

  template<typename... Indexes>
  const T& get(Indexes... indexes) const {
    return this->operator()(indexes...);
  }

  const Strides& strides() const { return strides_; }

 private:
  Accessor accessor_;
  Strides strides_;
};

template<typename T>
using ListAccessorFunction = std::function<T* const(size_t)>;

template<typename T, unsigned dims = 1,
         class Accessor = ListAccessorFunction<T> >
using ImplicitList = List<T, void, dims, Accessor>;


template<typename T, class P>
constexpr List<T, P> MakeList(PortionVector<T, P>&& pv) {
  return List<T, P>(std::forward<PortionVector<T, P> >(pv));
}

template<class Accessor, typename... DimSizes>
constexpr auto MakeList(Accessor accessor, size_t dim_size_fst,
                        DimSizes... dim_size_rest)
    -> List<typename std::remove_pointer<
  decltype(accessor(dim_size_fst, dim_size_rest...))>::type,
            void, (1 + sizeof...(DimSizes)), Accessor> {
  return List<typename std::remove_pointer<
    decltype(accessor(dim_size_fst, dim_size_rest...))>::type,
              void, (1 + sizeof...(DimSizes)), Accessor>(
                  accessor, dim_size_fst, dim_size_rest...);
}

}  // namespace v

#endif  /* CPPVIEWS_SRC_LIST_HPP_ */
