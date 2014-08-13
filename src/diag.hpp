#ifndef CPPVIEWS_SRC_DIAG_HPP_
#define CPPVIEWS_SRC_DIAG_HPP_

#include "list.hpp"
#include "map.hpp"  // TODO: move to list.hpp

#include "util/bit_twiddling.hpp"
#include "util/intseq.hpp"

#ifndef V_DIAG_NLIBDIVIDE
#include "util/libdivide.h"
#endif  // !defined(V_DIAG_NLIBDIVIDE)

#include <tuple>
#include <type_traits>

namespace v {

namespace detail {

// A Helper class for performing fast division (and multiplication):
// Use libdivide to perform generic division (using bit shifts and mults)
// For powers of 2, use bit shifts only (see the specialization below).
// Finally, use specialization for scaling by 1 to avoid any overhead.
template<typename Size, Size rhs, class Enable = void>
class DiagDimScaler
#ifndef V_DIAG_NLIBDIVIDE
    : protected libdivide::divider<Size>
#endif  // !defined(V_DIAG_NLIBDIVIDE)
{
 public:
#ifndef V_DIAG_NLIBDIVIDE
  DiagDimScaler() : libdivide::divider<Size>(rhs) {}
#endif  // !defined(V_DIAG_NLIBDIVIDE)

  friend Size operator*(const Size& lhs, const DiagDimScaler& s) {
    return lhs * rhs;
  }
  friend Size operator/(const Size& lhs, const DiagDimScaler& s) {
#ifndef V_DIAG_NLIBDIVIDE
    return lhs / static_cast<libdivide::divider<Size> >(s);
#else
    return lhs / rhs;
#endif  // !defined(V_DIAG_NLIBDIVIDE)
  }
};

template<typename Size, Size rhs>
class DiagDimScaler<Size, rhs,
                    typename std::enable_if<(rhs > 1 && IsPow2(rhs))>::type> {
  static constexpr Size rhs_lg = FindFirstSet(rhs) - 1;
 public:
  friend Size operator*(const Size& lhs, const DiagDimScaler& s) {
    return lhs << rhs_lg;
  }
  friend Size operator/(const Size& lhs, const DiagDimScaler& s) {
    return lhs >> rhs_lg;
  }
};

template<typename Size, Size rhs>
class DiagDimScaler<Size, rhs, typename std::enable_if<rhs == 1>::type> {
 public:
  friend Size operator*(const Size& lhs, const DiagDimScaler&) { return lhs; }
  friend Size operator/(const Size& lhs, const DiagDimScaler&) { return lhs; }
};

// A multi-dimensional array wrapper with operator()(indexes...) indexing
template<typename T, typename BlockSize, BlockSize block_size,
         BlockSize... block_sizes>
class DiagBlock {
  using Nested = DiagBlock<T, BlockSize, block_sizes...>;
  using Type = Nested[block_size];
 public:
  using NativeNested = typename
      DiagBlock<T, BlockSize, block_sizes...>::NativeType;
  using NativeType = NativeNested[block_size];

  template<typename Index, typename... Indexes>
  T& operator()(Index index, Indexes... indexes) {
    return b_[index](indexes...);
  }

 private:
  Type b_;
};

template<typename T, typename BlockSize, BlockSize block_size>
class DiagBlock<T, BlockSize, block_size> {
 public:
  using NativeType = T[block_size];

  template<typename Index>
  T& operator()(Index index) { return b_[index]; }

  template<typename Index>
  const T& operator()(Index index) const {
    return const_cast<DiagBlock*>(this)->operator()(index);
  }
 private:
  NativeType b_;
};


template<typename Int>
constexpr Int BitwiseOr(Int&& i) { return std::forward<Int>(i); }

template<typename Int, typename... Ints>
constexpr typename std::common_type<Int, Ints...>::type
BitwiseOr(Int&& i, Ints&&... is) {
  return i & BitwiseOr(std::forward<Ints>(is)...);
}

template<typename Size, Size... sizes>
constexpr bool IsSimpleDiagBlock() {
  using namespace list_detail;
  return GetNth<0, Size, sizes...>::value == 1 && SameSize(sizes...);
}

template<typename ListType,  // use CRTP to statically inject ops and funcs
         typename DataType, typename BlockSize, BlockSize... block_sizes>
class DiagHelper
#define V_THIS_BASE_TYPE ListBase<DataType, sizeof...(block_sizes)>
    : public V_THIS_BASE_TYPE {
  typedef V_THIS_BASE_TYPE ListBaseType;
#undef V_THIS_BASE_TYPE

  static_assert(sizeof...(block_sizes) >= 2, "Too few dimensions");

 protected:
  struct Disabler {};  // used for SFINAE (to disable constructors)

  template<typename... Sizes>
  DiagHelper(typename std::conditional<
             1 + sizeof...(Sizes) == sizeof...(block_sizes),
             size_t, Disabler>::type size,
             Sizes... sizes)
    : ListBase<DataType, sizeof...(block_sizes)>(size, sizes...) {}

  DiagHelper() = default;

  template<size_t... Is>
  DataType& get0(const typename ListBaseType::SizeArray& indexes,
                 cpp14::index_sequence<Is...>) const {
    return this->operator()(std::get<Is>(indexes)...);
  }

 public:
  template<typename... Indexes>
  DataType& operator()(const Indexes&... indexes) const {
    return static_cast<const ListType*>(this)->get0(
        cpp14::index_sequence_for<Indexes...>(),
        indexes...);
  }

  DataType& get(typename ListBaseType::SizeArray&& indexes) const override {
    return get0(indexes, cpp14::make_index_sequence<DiagHelper::kDims>());
  }

  template<unsigned dim>
  static constexpr BlockSize block_size() {
    return list_detail::GetNth<dim, BlockSize, block_sizes...>::value;
  }

  static BlockSize block_size(unsigned dim) {
    static const BlockSize kBlockSizes[] = {block_sizes...};
    return kBlockSizes[dim];
  }
};

template<typename DataType, typename BlockSize, BlockSize... block_sizes>
class DiagValueIter
    : public DefaultIterator<DiagValueIter<DataType, BlockSize, block_sizes...>,
                             std::forward_iterator_tag, DataType>,
      public View<DataType>::IteratorBase {
  V_DEFAULT_ITERATOR_DERIVED_HEAD(DiagValueIter);

  typedef detail::DiagBlock<DataType, BlockSize, block_sizes...> DiagBlockType;
  typedef typename std::vector<DiagBlockType>::iterator OuterIter;

  static constexpr unsigned dims() { return sizeof...(block_sizes); }

  typedef typename ListBase<DataType, DiagValueIter::dims()>::SizeArray
  SizeArray;

 public:
  DiagValueIter(OuterIter outer, bool last_block_full, const size_t& block_cnt,
                const SizeArray* sizes)
      : DiagValueIter(outer, last_block_full, block_cnt,
                      cpp14::make_index_sequence<dims()>(),
                      *sizes) {}

 protected:
  V_DEF_VIEW_ITER_IS_EQUAL(DataType, DiagValueIter)

  bool IsEqual(const DiagValueIter& other) const {
    return outer_ == other.outer_ && inner_ == other.inner_;
  }

  void Increment() override {
    static constexpr auto block_size = block_size_product();
    do {
      if (++inner_ == block_size) {
        inner_ = 0;
        ++outer_;
      }
      // try short-circuiting via a cheap check of the global_ value
      // to prevent potentially expenseive O(dims) overhead of WithinLastBlock()
    } while (IsSignedNegative(--global_) && !WithinLastBlock());
  }

  DataType& ref() const override {
    return reinterpret_cast<DataType*>(&*outer_)[inner_];
  }

 private:
  constexpr bool WithinLastBlock() const {
    return WithinLastBlock(inner_, cpp14::make_index_sequence<dims()>());
  }

  template<size_t... Is>
  constexpr bool WithinLastBlock(size_t pos, cpp14::index_sequence<Is...>) const
  {
    // TODO: messy, because the block has the reverse memory order
    return list_detail::WithinLast<BlockSize,
                                   list_detail::GetNth<
                                     static_cast<BlockSize>(dims() - Is - 1),
                                     BlockSize, block_sizes...>::value...>
        (pos, std::get<dims() - Is - 1>(last_block_sizes_)...);
  }

  template<typename Size, BlockSize size>
  static const detail::DiagDimScaler<Size, size>& dim_scaler() {
    static const detail::DiagDimScaler<Size, size> instance;
    return instance;
  }

  template<size_t... Is>
  DiagValueIter(OuterIter outer, bool last_block_full, const size_t& block_cnt,
                cpp14::index_sequence<Is...>,
                const SizeArray& sizes)
      : full_block_cnt_(block_cnt - !last_block_full),
        global_(full_block_cnt_ * this->block_size_product() +
                // add one to avoid Within() check upon past-the-end increment
                // in case last block is full (optimizes the common case)
                last_block_full),
        outer_(outer),
        inner_(0),
        last_block_sizes_({static_cast<BlockSize>(
            std::get<Is>(sizes) - block_sizes * full_block_cnt_)...}) {
    // TODO: uncomment when memory order of DiagBlock is reversed
    // if (!last_block_full)
    //   global_ += list_detail::SizeProduct(static_cast<size_t>(
    //       std::min(std::get<Is>(last_block_sizes_), block_sizes))...);
  }

  static constexpr size_t block_size_product() {
    return list_detail::SizeProduct(static_cast<size_t>(block_sizes)...);
  }

  size_t full_block_cnt_;
  size_t global_;
  OuterIter outer_;  // block iterator
  size_t inner_;    // linearized index within the block

  std::array<BlockSize, dims()> last_block_sizes_;
};

template<typename DataType>
class SimpleDiagValueIter
    : public DefaultIterator<SimpleDiagValueIter<DataType>,
                             std::forward_iterator_tag, DataType>,
      public View<DataType>::IteratorBase {
  V_DEFAULT_ITERATOR_DERIVED_HEAD(SimpleDiagValueIter);
  typedef typename std::vector<DataType>::iterator OuterIter;

 public:
  SimpleDiagValueIter(OuterIter outer) : outer_(outer) {}

 protected:
  V_DEF_VIEW_ITER_IS_EQUAL(DataType, SimpleDiagValueIter)

  bool IsEqual(const SimpleDiagValueIter& other) const {
    return outer_ == other.outer_;
  }

  void Increment() override { ++outer_; }

  DataType& ref() const override {
    return *reinterpret_cast<DataType*>(&*outer_);
  }

 private:
  OuterIter outer_;  // block iterator
};

}  // namespace detail

// For the MakeList overload, we need to somehow encode the block sizes.
template<typename BlockSize, BlockSize... block_sizes>
struct DiagTag {};

template<typename DataType, typename BlockSize, BlockSize... block_sizes>
using Diag = List<cpp14::integer_sequence<BlockSize, block_sizes...>,
                  sizeof...(block_sizes),
                  kListOpVector,
                  DataType,
                  void>;

template<typename DataType, unsigned dims, typename BlockSize,
         BlockSize... block_sizes>
#define V_LIST_TYPE                                                     \
  List<cpp14::integer_sequence<BlockSize, block_sizes...>,              \
       dims,                                                            \
       kListOpVector,                                                   \
       DataType,                                                        \
       void,                                                            \
       typename std::enable_if<(                                        \
           dims == sizeof...(block_sizes) &&                            \
           !detail::IsSimpleDiagBlock<BlockSize, block_sizes...>())>::type>
class V_LIST_TYPE
#define V_THIS_BASE_TYPE \
  detail::DiagHelper<V_LIST_TYPE, DataType, BlockSize, block_sizes...>
    : public V_THIS_BASE_TYPE,
      public Map<std::array<size_t, dims>, DataType> {
  friend class V_THIS_BASE_TYPE;
  typedef V_THIS_BASE_TYPE DiagHelper;
#undef V_THIS_BASE_TYPE
#undef V_LIST_TYPE

 public:
  typedef BlockSize BlockSizeType;
  typedef detail::DiagValueIter<DataType, BlockSize, block_sizes...> ValueIter;

  class ValuesView : public View<DataType> {
   public:
    typedef ValueIter Iterator;

    ValuesView(List* list)
        : View<DataType>(list->nondefault_value_cnt_),
          list_(list) {}
    typename View<DataType>::Iterator iterator_begin() const {
      return new Iterator(begin());
    }
    typename View<DataType>::Iterator iterator_end() const {
      return new Iterator(end());
    }
    Iterator begin() const {
      return Iterator(list_->blocks_.begin(), list_->last_block_full_,
                      list_->blocks_.size(), &list_->sizes());
    }
    Iterator end() const {
      return Iterator(list_->blocks_.end(), list_->last_block_full_,
                      list_->blocks_.size(), &list_->sizes());
    }
   private:
    List* list_;
  };
  friend class ValuesView;

  template<typename... Sizes>
  List(DataType* default_value, size_t size, Sizes... sizes)
      : DiagHelper(size, sizes...),
        blocks_(ComputeBlockCount(cpp14::index_sequence_for<size_t, Sizes...>(),
                                  size, sizes...)),
        default_value_(default_value),
        last_block_full_(IsLastBlockFull(
            blocks_.size(),
            cpp14::index_sequence_for<size_t, Sizes...>(), size, sizes...)),
        nondefault_value_cnt_(ComputeNonDefaultValueCount(blocks_.size(),
                                                          last_block_full_,
                                                          size, sizes...)),
        values_(this) {}

  List(size_t block_count, DataType* default_value = nullptr)
      : DiagHelper((block_count * block_sizes)...),
        blocks_(block_count),
        default_value_(default_value),
        last_block_full_(true),
        nondefault_value_cnt_(
            list_detail::SizeProduct(static_cast<size_t>(block_sizes)...) *
            block_count),
        values_(this) {}

  // used by MakeList
  template<typename... Args>
  List(DiagTag<BlockSize, block_sizes...>, Args&&... args)
      : List(std::forward<Args>(args)...) {}

  friend List MakeList(List&& list) { return std::forward<List>(list); }

  typename View<DataType>::Iterator iterator_begin() const override {
    // TODO: implement
    return *reinterpret_cast<typename View<DataType>::Iterator*>(NULL);
  }

  ValuesView& values() const override { return values_; }

  size_t block_count() const { return blocks_.size(); }

 private:
  typedef detail::DiagBlock<DataType, BlockSize, block_sizes...> DiagBlockType;

  template<size_t... Is, typename... Sizes>
  static size_t ComputeBlockCount(cpp14::index_sequence<Is...>, Sizes... sizes)
  {
    return list_detail::MinSize(((sizes + (block_sizes - 1)) /
                                 std::get<Is>(dim_scalers()))...);
  }

  template<typename... Sizes>
  static constexpr size_t ComputeNonDefaultValueCount(const size_t& block_count,
                                                      bool last_block_full,
                                                      Sizes&&... sizes) {
    using namespace list_detail;
    return block_size_product() * (block_count - !last_block_full) +
        SizeProduct(MinSize(
            sizes - block_sizes * (block_count - !last_block_full),
            block_sizes)...);
  }

  template<size_t... Is, typename... Sizes>
  static bool IsLastBlockFull(const size_t& block_cnt,
                              cpp14::index_sequence<Is...>,
                              const Sizes&... sizes) {
    using namespace detail;
    return BitwiseOr((block_cnt * std::get<Is>(dim_scalers()) > sizes)...);
  }

  static constexpr size_t block_size_product() {
    return list_detail::SizeProduct(static_cast<size_t>(block_sizes)...);
  }

  // use static singleton accessor to avoid a global definition
  static const std::tuple<detail::DiagDimScaler<BlockSize, block_sizes>...>&
  dim_scalers() {
    static const std::tuple<
      detail::DiagDimScaler<BlockSize, block_sizes>...> instance;
    return instance;
  }

  template<size_t I, size_t... Is, typename Index, typename... Indexes>
  DataType& get0(cpp14::index_sequence<I, Is...>, const Index& index,
                 const Indexes&... indexes) const {
    typename decltype(blocks_)::size_type block_index;
    if (!list_detail::SameSize(block_index = index / std::get<I>(dim_scalers()),
                               (indexes / std::get<Is>(dim_scalers()))...))
      return *default_value_;

    return blocks_[block_index](
        index - block_index * std::get<I>(dim_scalers()),
        (indexes - block_index * std::get<Is>(dim_scalers()))...);
  }

  mutable std::vector<DiagBlockType> blocks_;
  DataType* default_value_;
  bool last_block_full_;
  size_t nondefault_value_cnt_;
  mutable ValuesView values_;
};

// specialization for diagonal with block sizes all 1

template<typename DataType, unsigned dims, typename BlockSize,
         BlockSize... block_sizes>
#define V_LIST_TYPE                                                     \
  List<cpp14::integer_sequence<BlockSize, block_sizes...>,              \
       dims,                                                            \
       kListOpVector,                                                   \
       DataType,                                                        \
       void,                                                            \
       typename std::enable_if<(                                        \
           dims == sizeof...(block_sizes) &&                            \
           detail::IsSimpleDiagBlock<BlockSize, block_sizes...>())>::type>
class V_LIST_TYPE
#define V_THIS_BASE_TYPE                                                \
  detail::DiagHelper<V_LIST_TYPE, DataType, BlockSize, block_sizes...>
    : public V_THIS_BASE_TYPE,
      public Map<std::array<size_t, dims>, DataType> {
  friend class V_THIS_BASE_TYPE;
  typedef V_THIS_BASE_TYPE DiagHelper;
#undef V_THIS_BASE_TYPE
#undef V_LIST_TYPE

 public:
  class ValuesView : public View<DataType> {
   public:
    typedef detail::SimpleDiagValueIter<DataType> Iterator;

    ValuesView(typename std::vector<DataType>::iterator begin,
               typename std::vector<DataType>::iterator end)
        : View<DataType>(end - begin),
          begin_(begin),
          end_(end) {}
    typename View<DataType>::Iterator iterator_begin() const {
      return new Iterator(begin());
    }
    typename View<DataType>::Iterator iterator_end() const {
      return new Iterator(end());
    }
    Iterator begin() const { return begin_; }
    Iterator end() const { return end_; }

   private:
    Iterator begin_;
    Iterator end_;
  };

  template<typename... Sizes>
  List(DataType* default_value, size_t size, Sizes... sizes)
      : DiagHelper(size, sizes...),
        blocks_(list_detail::MinSize(size, sizes...)),
        default_value_(default_value),
        values_(blocks_.begin(), blocks_.end()) {}

  List(size_t block_count, DataType* default_value = nullptr)
      : DiagHelper((block_count * block_sizes)...),
        blocks_(block_count),
        default_value_(default_value),
        values_(blocks_.begin(), blocks_.end()) {}

  // used by MakeList
  template<typename... Args>
  List(DiagTag<BlockSize, block_sizes...>, Args&&... args)
      : List(std::forward<Args>(args)...) {}

  friend List MakeList(List&& list) { return std::forward<List>(list); }

  typename View<DataType>::Iterator iterator_begin() const override {
    // TODO: implement
    return *reinterpret_cast<typename View<DataType>::Iterator*>(NULL);
  }

  ValuesView& values() const override { return values_; }

  size_t block_count() const { return blocks_.size(); }

 private:
  template<size_t I, size_t... Is, typename Index, typename... Indexes>
  DataType& get0(cpp14::index_sequence<I, Is...>, const Index& index,
                 const Indexes&... indexes) const {
    return list_detail::SameSize(index, indexes...)
        ? blocks_[index]
        : *default_value_;
  }

  mutable std::vector<DataType> blocks_;
  DataType* default_value_;
  mutable ValuesView values_;
};

template<typename DataType, typename BlockSize, BlockSize... block_sizes,
         typename... Sizes>
auto MakeList(DiagTag<BlockSize, block_sizes...>,
              DataType* default_value, Sizes... sizes)
#define V_LIST_TYPE Diag<DataType, BlockSize, block_sizes...>
    -> V_LIST_TYPE { return V_LIST_TYPE(default_value, sizes...); }
#undef V_LIST_TYPE

}  // namespace v

#endif  /* CPPVIEWS_SRC_DIAG_HPP_ */
