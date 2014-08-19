#ifndef CPPVIEWS_SRC_CHAIN_HPP_
#define CPPVIEWS_SRC_CHAIN_HPP_

#include "list.hpp"

#include <algorithm>
#include <numeric>
#include <utility>
#include <vector>

namespace v {

namespace detail {

template<class SublistType, unsigned chain_dim>
class ChainedListVector : public ListVector<SublistType> {
  typedef ListVector<SublistType> ListVectorType;
  typedef typename ListVectorType::SizeType SizeType;
 public:
  ChainedListVector(ListVectorType&& sv) : ListVectorType(std::move(sv)) {}
  ChainedListVector(SizeType capacity) : ListVectorType(capacity) {}
  ChainedListVector() = default;
  static constexpr unsigned chain_dimension() { return chain_dim; }
};

template<class SublistType>
struct ChainHelper {
  static void InsertDummies(ListVector<SublistType>* c) {
    // add a sentinel list at the front (copy first and shrink it to size 0)
    c->Add(c->begin(), c->front());
    c->front().ShrinkToFirst();

    // add a sentinel list at the back (copy last and shrink it to size 0)
    c->Append(c->back());
    c->back().ShrinkToFirst();
  }
};

template<typename T, unsigned dims>
struct ChainHelper<ListBase<T, dims> > {
  static void InsertDummies(ListVector<ListBase<T, dims> >* c) {
    c->template Add<DummyList<T, dims> >(c->begin());
    c->template AppendDerived<DummyList<T, dims> >();
  }
};

template<class SublistType>
class ChainValues : public View<typename SublistType::DataType> {
  typedef ListVector<SublistType> ListVectorType;

 public:
  using typename View<typename SublistType::DataType>::DataType;

  class Iterator
      : public DefaultIterator<Iterator, std::forward_iterator_tag, DataType>,
        public View<DataType>::IteratorBase {
    V_DEFAULT_ITERATOR_DERIVED_HEAD(Iterator);

    typedef typename ListVectorType::ConstIterator OuterIter;
    typedef typename std::decay<
      decltype(std::declval<SublistType>().values())
      >::type::Iterator InnerIter;

   public:
    Iterator(const OuterIter& outer_cur, const OuterIter& outer_max)
        : inner_cur_(outer_cur->values().begin()),
          inner_end_(outer_cur->values().end()),
          outer_cur_(outer_cur),
          outer_max_(outer_max) {}

   protected:
    V_DEF_VIEW_ITER_IS_EQUAL(DataType, Iterator)

    bool IsEqual(const Iterator& other) const {
      return outer_cur_ == other.outer_cur_ && inner_cur_ == other.inner_cur_;
    }

    void Increment() override {
      if (++inner_cur_ == inner_end_)
        do {
          const auto& values = (++outer_cur_)->values();
          inner_end_ = values.end();
          inner_cur_ = values.begin();
        } while (inner_cur_ == inner_end_ && outer_cur_ != outer_max_);
    }

    DataType& ref() const override { return *inner_cur_; }

   private:
    InnerIter inner_cur_;
    InnerIter inner_end_;  // value iterator creation can be expensive
    OuterIter outer_cur_;
    OuterIter outer_max_;
  };

 public:
  ChainValues(const ListVectorType& lists) : lists_(&lists) {
    this->size_ = 0;
    for (const auto& list : *lists_)
      this->size_ += list.values().size();
  }
  typename View<DataType>::Iterator iterator_begin() const {
    return new Iterator(begin());
  }
  typename View<DataType>::Iterator iterator_end() const {
    return new Iterator(end());
  }
  Iterator begin() const {
    auto it = ++lists_->begin(), it_max = --lists_->end();
    if (!it->values().size() && it != it_max) {
      // unroll once to boost performance if first is not empty (common case)
      do ++it; while (!it->values().size() && it != it_max);
    }
    return Iterator(it, it_max);
  }
  Iterator end() const {
    auto it_max = --lists_->end();
    return Iterator(it_max, it_max);
  }

 private:
  const ListVectorType* lists_;
};

}  // namespace detail

// For the MakeList overload, we need to somehow encode the chaining dimension.
// One way would be to expose detail::ChainedListVector; however, this would
// require reimplementation of a lot of methods to support the fluent interface.
// An easier (and perhaps cleaner) way is to provide a tag which encodes it.
template<unsigned chain_dim>
struct ChainTag {};

template<class SublistType,
         unsigned chain_dim,
         ListFlags flags = kListOpVector,
         unsigned dims = ListTraits<SublistType>::kDims>  // it can also be >
using Chain = List<SublistType, dims, flags,
                   detail::ChainedListVector<SublistType, chain_dim>,
                   typename SublistType::DataType>;

template<class SublistType, unsigned dims, unsigned chain_dim>
class List<
  SublistType,
  dims,
  kListOpVector,
#define V_THIS_DATA_TYPE typename SublistType::DataType
  detail::ChainedListVector<SublistType, chain_dim>,
  V_THIS_DATA_TYPE,
  typename std::enable_if<dims >= ListTraits<SublistType>::kDims>::type>
    : public ListBase<V_THIS_DATA_TYPE, dims>,
      protected detail::ChainHelper<SublistType> {
  static_assert(chain_dim < dims, "chain_dim < dims");
#define V_CHAIN_FOR_LATERAL_DIM(dim, body)                              \
  for (unsigned dim = 0; dim < chain_dim; ++dim) do body while (false); \
  for (unsigned dim = chain_dim + 1; dim < dims; ++dim) do body while (false);

  typedef ListBase<V_THIS_DATA_TYPE, dims> ListBaseType;
  typedef detail::ChainedListVector<SublistType, chain_dim> Container;
  struct Disabler {};  // used for SFINAE (to disable constructors)

 public:
  typedef std::array<size_t, dims - 1> LateralOffset;
  typedef std::pair<size_t, LateralOffset> LateralOffsetEntry;
  typedef std::pair<size_t, size_t> GapEntry;

  // redeclared for convenience of the implementation
  typedef V_THIS_DATA_TYPE DataType;
  typedef typename ListBaseType::SizeArray SizeArray;
#undef V_THIS_DATA_TYPE

  typedef detail::ChainValues<SublistType> ValuesView;

  template<typename... Sizes>
  List(ListVector<SublistType>&& lists, DataType* default_value,
       std::vector<SizeArray>&& nesting_offsets,
       const typename std::conditional<1 + sizeof...(Sizes) == dims,
       size_t, Disabler>::type& size, Sizes&&... sizes)
      : ListBaseType(size, std::forward<Sizes>(sizes)...),
        lists_(std::move(lists)),
        default_value_(default_value),
        nesting_offsets_(InsertDummies(lists_, this->sizes_,
                                       std::move(nesting_offsets))),
        fwd_skip_list_(nesting_offsets_.size() - 1, &nesting_offsets_),
        values_(lists_) {}

  explicit List(ListVector<SublistType>&& lists,
                DataType* default_value = nullptr,
                const std::vector<LateralOffsetEntry>& lateral_offsets = {},
                const std::vector<GapEntry>& sorted_gaps = {})
      : lists_(std::move(lists)),
        default_value_(default_value),
        nesting_offsets_(lists_.size()),
        fwd_skip_list_(0),
        values_(lists_) {
    for (const auto& entry : lateral_offsets)
      SetLateralOffset(entry.second, &nesting_offsets_[entry.first]);

    // compute non-lateral nesting offsets
    size_t last_gap_index = 0;
    size_t cur_nesting_offset = 0;
    auto nesting_offset = nesting_offsets_.begin();
    auto list = lists_.cbegin();
#define V_THIS_COMPUTE_NESTING_OFFSETS(index_to)                \
    { const size_t gap_index_to = (index_to);                   \
      for (size_t i = last_gap_index; i < gap_index_to; ++i) {  \
        (*nesting_offset++)[chain_dim] = cur_nesting_offset;    \
        cur_nesting_offset += (list++)->sizes()[chain_dim];     \
      }                                                         \
    }
    for (const auto& gap_entry : sorted_gaps) {
      V_THIS_COMPUTE_NESTING_OFFSETS(gap_entry.first);
      last_gap_index = gap_entry.first;
      cur_nesting_offset += gap_entry.second;
    }
    V_THIS_COMPUTE_NESTING_OFFSETS(lists_.size());
#undef V_THIS_COMPUTE_NESTING_OFFSETS

    this->offsets_.fill(0);

    // compute non-lateral size
    auto& sizes_ = this->sizes_;
    const size_t trailing_gap =
        (sorted_gaps.empty() || sorted_gaps.back().first != lists_.size()) ? 0
        : sorted_gaps.back().second;
    sizes_[chain_dim] =
        nesting_offsets_.back()[chain_dim] +
        lists_.back().sizes()[chain_dim] +
        trailing_gap;

    V_CHAIN_FOR_LATERAL_DIM(dim, {
        ComputeLateralSize(lists_, nesting_offsets_, sizes_, dim);
            });

    // TODO: resizing nesting_offsets_ twice and moving skip list isn't ideal,
    // but it makes the initialization simple
    InsertDummies(lists_, this->sizes_, std::move(nesting_offsets_));
    fwd_skip_list_ = decltype(fwd_skip_list_)(nesting_offsets_.size() - 1,
                                              &nesting_offsets_);
  }

  // List(const List&) = delete;  // redundant since ListVector is not copyable
  List(List&& src)
      : ListBaseType(std::move(src)),
        lists_(std::move(src.lists_)),
        default_value_(src.default_value_),
        nesting_offsets_(std::move(src.nesting_offsets_)),
        fwd_skip_list_(std::move(src.fwd_skip_list_)),
        values_(lists_) {
    // the size getter points to nesting_offsets_ that is moved, so update it
    fwd_skip_list_.bucket_size_getter().o_ = &nesting_offsets_;
  }

  // used by MakeList
  template<typename... Args>
  List(ChainTag<chain_dim>, Args&&... args)
      : List(std::forward<Args>(args)...) {}

  friend List MakeList(List&& list) {
    return std::forward<List>(List(std::move(list)));
  }

  template<typename... Indexes>
  DataType& operator()(Indexes&&... indexes) const {
    // TODO: terribly inefficient
    return get(SizeArray{static_cast<size_t>(indexes)...});
  }

  void ShrinkToFirst() override {
    ListBaseType::ShrinkToFirst();
    lists_.Erase(++lists_.begin(), lists_.end());
  }

  DataType& get(SizeArray&& indexes) const override {
    auto nonlateral_pos = fwd_skip_list_.get(indexes[chain_dim]);
    const auto& list = lists_[nonlateral_pos.first];
    bool within = nonlateral_pos.second < list.sizes()[chain_dim];
    indexes[chain_dim] = nonlateral_pos.second;
    const auto& offset = nesting_offsets_[nonlateral_pos.first];
    V_CHAIN_FOR_LATERAL_DIM(dim, {
        within &= (offset[dim] <= indexes[dim]) &
            (indexes[dim] < offset[dim] + list.sizes()[dim]);
        indexes[dim] -= offset[dim];
      });
    return within ? list.get(std::move(indexes)) : *default_value_;
  }

  typename View<DataType>::Iterator iterator_begin() const override {
    // TODO: implement
    return *reinterpret_cast<typename View<DataType>::Iterator*>(NULL);
  }

  const ValuesView& values() const override { return values_; }

  const SizeArray& nesting_offset(size_t index) const {
    return nesting_offsets_[++index];
  }

  static constexpr unsigned chain_dimension() { return chain_dim; }

 private:
  struct NonLateralBucketSizeGetter {
    NonLateralBucketSizeGetter(const std::vector<List::SizeArray>* o) : o_(o) {}
    NonLateralBucketSizeGetter() = default;
    size_t operator()(size_t index) const {
      return (*o_)[index + 1][chain_dim] - (*o_)[index][chain_dim];
    }

    mutable const std::vector<List::SizeArray>* o_;
  };

  typedef ImmutableSkipList<NonLateralBucketSizeGetter> SkipListType;

  static void ComputeLateralSize(const Container& lists,
                                 const std::vector<SizeArray>& nesting_offsets,
                                 typename List::SizeArray& sizes,
                                 int dim) {
    auto& size = sizes[dim];
    auto list = lists.cbegin();
    auto nesting_offset = nesting_offsets.cbegin();
    typename List::SizeArray::size_type min_nesting_offset = -1u;
    size = 0;
    for (size_t i = lists.size(); i--; ++list, ++nesting_offset) {
      const auto& cur_nesting_offset = (*nesting_offset)[dim];
      min_nesting_offset = std::min(min_nesting_offset, cur_nesting_offset);
      size = std::max(size, cur_nesting_offset + list->sizes()[dim]);
    }
    size -= min_nesting_offset;
  }

  static void SetLateralOffset(const LateralOffset& offset,
                               SizeArray* nesting_offset) {
    using namespace std;
    copy(offset.begin() + chain_dim, offset.end(),
         1 + copy_n(offset.begin(), chain_dim, nesting_offset->begin()));
  }

  static std::vector<SizeArray>&&
  InsertDummies(Container& lists_, const List::SizeArray& sizes,
                std::vector<SizeArray>&& nesting_offsets_) {
    if (!lists_.empty()) {
      detail::ChainHelper<SublistType>::InsertDummies(&lists_);
      lists_.Shrink();
    }

    nesting_offsets_.reserve(nesting_offsets_.size() + 2);
    nesting_offsets_.emplace(nesting_offsets_.begin());
    nesting_offsets_.front().fill(0);
    nesting_offsets_.emplace_back();
    nesting_offsets_.back().fill(0);
    nesting_offsets_.back()[chain_dim] = sizes[chain_dim];
    nesting_offsets_.shrink_to_fit();
    return std::move(nesting_offsets_);
  }
#undef V_CHAIN_FOR_LATERAL_DIM

  Container lists_;
  DataType* default_value_;
  std::vector<SizeArray> nesting_offsets_;
  SkipListType fwd_skip_list_;
  ValuesView values_;
};

// specialization for Uniform Chain (same sublist sizes + no lateral offsets)

namespace detail {

template<class SublistType, unsigned chain_dim, size_t uniform_size_,
         size_t gap_before_, size_t gap_after_>
class UniformlyChainedListVector
#define V_THIS_BASE_TYPE ChainedListVector<SublistType, chain_dim>
    : public V_THIS_BASE_TYPE {
  typedef V_THIS_BASE_TYPE ChainedListVectorType;
#undef V_THIS_BASE_TYPE
  typedef ListVector<SublistType> ListVectorType;
  typedef typename ListVectorType::SizeType SizeType;
 public:
  using ChainedListVectorType::ChainedListVectorType;

  UniformlyChainedListVector(ListVectorType&& sv)
      : ChainedListVectorType(std::move(sv)) {}
  static constexpr size_t uniform_size() { return uniform_size_; }
  static constexpr size_t gap_before() { return gap_before_; }
  static constexpr size_t gap_after() { return gap_after_; }
};

}  // namespace detail

template<unsigned chain_dim, size_t uniform_size_,
         size_t gap_before_ = 0, size_t gap_after_ = 0>
struct UniformChainTag {};

template<class SublistType,
         unsigned chain_dim,
         size_t uniform_size_,
         size_t gap_before_ = 0,
         size_t gap_after_ = 0,
         ListFlags flags = kListOpVector,
         unsigned dims = ListTraits<SublistType>::kDims>  // it can also be >
using UniformChain = List<SublistType, dims, flags,
                          detail::UniformlyChainedListVector<
                            SublistType, chain_dim, uniform_size_,
                            gap_before_, gap_after_>,
                          typename SublistType::DataType>;

template<class SublistType, unsigned dims, unsigned chain_dim,
         size_t uniform_size_, size_t gap_before_, size_t gap_after_>
class List<
  SublistType,
  dims,
  kListOpVector,
#define V_THIS_DATA_TYPE typename SublistType::DataType
  detail::UniformlyChainedListVector<SublistType, chain_dim, uniform_size_,
                                     gap_before_, gap_after_>,
  V_THIS_DATA_TYPE,
  typename std::enable_if<dims >= ListTraits<SublistType>::kDims>::type>
    : public ListBase<V_THIS_DATA_TYPE, dims>,
      protected detail::ChainHelper<SublistType> {
  static_assert(chain_dim < dims, "chain_dim < dims");

  typedef ListBase<V_THIS_DATA_TYPE, dims> ListBaseType;
  typedef detail::UniformlyChainedListVector<
    SublistType, chain_dim, uniform_size_, gap_before_, gap_after_> Container;

  // redeclared for convenience of the implementation
  typedef V_THIS_DATA_TYPE DataType;
  typedef typename ListBaseType::SizeArray SizeArray;
#undef V_THIS_DATA_TYPE

 public:
  typedef detail::ChainValues<SublistType> ValuesView;

  template<typename... Sizes>
  List(ListVector<SublistType>&& lists, DataType* default_value)
      : lists_(std::move(lists)),
        default_value_(default_value),
        values_(lists_) {
    const auto& first_sizes = lists_[0].sizes();
    std::copy(first_sizes.begin(), first_sizes.end(), this->sizes_.begin());
    this->sizes_[chain_dim] = uniform_size_ + gap_before_ + gap_after_;
    this->sizes_[chain_dim] *= lists_.size();
    this->offsets_.fill(0);
    this->InsertDummies(&lists_);
  }

  // List(const List&) = delete;  // redundant since ListVector is not copyable
  // List(const List&) = default;

  // used by MakeList
  template<typename... Args>
  List(UniformChainTag<chain_dim, uniform_size_, gap_before_, gap_after_>,
       Args&&... args)
      : List(std::forward<Args>(args)...) {}

  friend List MakeList(List&& list) {
    return std::forward<List>(List(std::move(list)));
  }

  template<typename... Indexes>
  DataType& operator()(Indexes&&... indexes) const {
    // TODO: terribly inefficient
    return get(SizeArray{static_cast<size_t>(indexes)...});
  }

  void ShrinkToFirst() override {
    ListBaseType::ShrinkToFirst();
    lists_.Erase(++lists_.begin(), lists_.end());
  }

  DataType& get(SizeArray&& indexes) const override {
    if (indexes[chain_dim] < gap_before_)
      return *default_value_;

    auto& nonlateral_index = indexes[chain_dim];
    const auto list_index = (nonlateral_index -= gap_before_) / bucket_size();
    nonlateral_index -= list_index * bucket_size();
    return nonlateral_index < uniform_size_
                              ? lists_[1 + list_index].get(std::move(indexes))
                              : *default_value_;
  }

  typename View<DataType>::Iterator iterator_begin() const override {
    // TODO: implement
    return *static_cast<typename View<DataType>::Iterator*>(nullptr);
  }

  const ValuesView& values() const override { return values_; }

 private:
  static constexpr size_t bucket_size() {
    return uniform_size_ + gap_before_ + gap_after_;
  }

  Container lists_;
  DataType* default_value_;
  ValuesView values_;
};

// functions

template<unsigned dims>
using ChainOffsetVector = std::vector<std::array<size_t, dims> >;

template<class Sublist, unsigned chain_dim, typename... Sizes>
auto MakeList(ChainTag<chain_dim>,
              ListVector<Sublist>&& lists,
              typename Sublist::DataType* default_value,
              ChainOffsetVector<Sublist::kDims>&& nesting_offsets,
              Sizes&&... sizes)
#define V_LIST_TYPE \
    Chain<Sublist, chain_dim>
    -> V_LIST_TYPE {
  using namespace std;
  return V_LIST_TYPE(move(lists), default_value, move(nesting_offsets),
                     forward<Sizes>(sizes)...);
}
#undef V_LIST_TYPE

template<class Sublist, unsigned chain_dim, size_t uniform_size,
         size_t gap_before, size_t gap_after>
auto MakeList(UniformChainTag<chain_dim, uniform_size, gap_before, gap_after>,
              ListVector<Sublist>&& lists,
              typename Sublist::DataType* default_value)
#define V_LIST_TYPE \
    UniformChain<Sublist, chain_dim, uniform_size, gap_before, gap_after>
    -> V_LIST_TYPE {
  return V_LIST_TYPE(std::move(lists), default_value);
}
#undef V_LIST_TYPE

}  // namespace v

#endif  /* CPPVIEWS_SRC_CHAIN_HPP_ */
