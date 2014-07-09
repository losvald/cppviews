#ifndef CPPVIEWS_SRC_CHAIN_HPP_
#define CPPVIEWS_SRC_CHAIN_HPP_

#include "list.hpp"

#include <algorithm>
#include <numeric>

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
  template<typename... Sizes>
  List(ListVector<SublistType>&& lists, DataType* default_value,
       typename std::conditional<1 + sizeof...(Sizes) == dims,
       size_t, Disabler>::type size,
       Sizes... sizes)
      : ListBaseType(size, sizes...),
        default_value_(default_value),
        lists_(std::move(lists)) {}

  List(ListVector<SublistType>&& lists,
       DataType* default_value = nullptr)
      : lists_(std::move(lists)),
        default_value_(default_value) {
    ClearSizeArray(ListBaseType::offsets_);
    ComputeSizes(lists_, ListBaseType::sizes_);
  }

  DataType& get(const SizeArray& indexes) const override {
    static DataType todo = DataType();
    return todo;  // TODO: implement
  }

  typename View<DataType>::Iterator iterator_begin() const override {
    // TODO: implement
    return *reinterpret_cast<typename View<DataType>::Iterator*>(NULL);
  }

 private:
  static void ClearSizeArray(typename List::SizeArray& a) {
    std::fill(a.begin(), a.end(), typename List::SizeArray::size_type());
  }

  static void ComputeSizes(const Container& lists,
                           typename List::SizeArray& sizes) {
    typedef typename List::SizeArray::size_type Size;
    Size kZeroSize = Size();
    ClearSizeArray(sizes);
    for (unsigned dim = 0; dim < dims; ++dim) {
#define V_THIS_ACCUMULATE(ret)      \
      sizes[dim] = std::accumulate(                                     \
          lists.begin(), lists.end(), kZeroSize,                        \
          [&](const Size& size, const SublistType& l) { return (ret); })
      if (dim + chain_dim + 1u == dims) {  // XXX:
        V_THIS_ACCUMULATE(size + l.sizes()[dim]);
      } else {
        V_THIS_ACCUMULATE(std::max(size, l.sizes()[dim]));
      }
    }
  }

  Container lists_;
  DataType* default_value_;
};

}  // namespace v

#endif  /* CPPVIEWS_SRC_CHAIN_HPP_ */
