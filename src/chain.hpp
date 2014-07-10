#ifndef CPPVIEWS_SRC_CHAIN_HPP_
#define CPPVIEWS_SRC_CHAIN_HPP_

#include "list.hpp"

#include <algorithm>
#include <numeric>
#include <utility>
#include <vector>

namespace v {

namespace detail {

template<unsigned chain_dim, typename T, unsigned dims,
         class SublistType = ListBase<T, dims> >
class ChainedListVector : public ListVector<SublistType> {
  typedef ListVector<SublistType> ListVectorType;
  typedef typename ListVectorType::SizeType SizeType;
 public:
  static const unsigned kChainDim = chain_dim;
  ChainedListVector(ListVectorType&& sv) : ListVectorType(std::move(sv)) {}
  ChainedListVector(SizeType capacity) : ListVectorType(capacity) {}
  ChainedListVector() = default;
};

}  // namespace detail

template<class SublistType,
         unsigned chain_dim = 0,
         ListFlags flags = kListOpVector,
         unsigned dims = ListTraits<SublistType>::kDims>  // it can also be >
using Chain = List<SublistType, dims,
                   flags,
                   detail::ChainedListVector<chain_dim,
                                             typename SublistType::DataType,
                                             ListTraits<SublistType>::kDims,
                                             SublistType>,
                   typename SublistType::DataType>;

template<class SublistType, unsigned dims, unsigned chain_dim>
class List<
  SublistType,
  dims,
  kListOpVector,
  detail::ChainedListVector<chain_dim, typename SublistType::DataType, dims,
                            SublistType>,
  typename SublistType::DataType,
  typename std::enable_if<dims >= ListTraits<SublistType>::kDims>::type
#define V_THIS_BASE ListBase<typename SublistType::DataType, dims>
  > : public V_THIS_BASE {
  static_assert(chain_dim < dims, "chain_dim < dims");
  typedef V_THIS_BASE ListBaseType;
#undef V_THIS_BASE
  typedef typename SublistType::DataType DataType;
  typedef typename ListBaseType::SizeArray SizeArray;
  typedef detail::ChainedListVector<chain_dim, DataType, dims, SublistType>
  Container;
  struct Disabler {};  // used for SFINAE (to disable constructors)

 public:
  typedef std::array<size_t, dims - 1> LateralOffset;
  typedef std::pair<size_t, LateralOffset> LateralOffsetEntry;
  typedef std::pair<size_t, size_t> GapEntry;

  template<typename... Sizes>
  List(ListVector<SublistType>&& lists, DataType* default_value,
       typename std::conditional<1 + sizeof...(Sizes) == dims,
       size_t, Disabler>::type size,
       Sizes... sizes)
      : ListBaseType(size, sizes...),
        default_value_(default_value),
        lists_(std::move(lists)),
        nesting_offsets_(lists_.size()) {}

  List(ListVector<SublistType>&& lists,
       DataType* default_value = nullptr,
       const std::vector<LateralOffsetEntry>& lateral_offsets = {},
       const std::vector<GapEntry>& sorted_gaps = {})
      : lists_(std::move(lists)),
        default_value_(default_value),
        nesting_offsets_(lists_.size()) {
    for (const auto& entry : lateral_offsets)
      SetLateralOffset(entry.first, entry.second);

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

    ClearSizeArray(ListBaseType::offsets_);

    // compute non-lateral size
    auto& sizes_ = ListBaseType::sizes_;
    const size_t trailing_gap =
        (sorted_gaps.empty() || sorted_gaps.back().first != lists_.size()) ? 0
        : sorted_gaps.back().second;
    sizes_[chain_dim] =
        nesting_offsets_.back()[chain_dim] +
        lists_.back().sizes()[chain_dim] +
        trailing_gap;

    for (unsigned dim = 0; dim < chain_dim; ++dim)
      ComputeLateralSize(lists_, nesting_offsets_, sizes_, dim);
    for (unsigned dim = chain_dim + 1; dim < dims; ++dim)
      ComputeLateralSize(lists_, nesting_offsets_, sizes_, dim);
  }

  DataType& get(const SizeArray& indexes) const override {
    static DataType todo = DataType();
    return todo;  // TODO: implement
  }

  typename View<DataType>::Iterator iterator_begin() const override {
    // TODO: implement
    return *reinterpret_cast<typename View<DataType>::Iterator*>(NULL);
  }

  const SizeArray& nesting_offset(size_t index) const {
    return nesting_offsets_[index];
  }

 private:
  static void ClearSizeArray(typename List::SizeArray& a) {
    std::fill(a.begin(), a.end(), typename List::SizeArray::size_type());
  }

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

  void SetLateralOffset(size_t index, const LateralOffset& offset) {
    auto& nesting_offset = nesting_offsets_[index];
    std::copy(offset.begin() + chain_dim, offset.end(),
              1+std::copy_n(offset.begin(), chain_dim, nesting_offset.begin()));
  }

  Container lists_;
  DataType* default_value_;
  std::vector<SizeArray> nesting_offsets_;
};

}  // namespace v

#endif  /* CPPVIEWS_SRC_CHAIN_HPP_ */
