#ifndef CPPVIEWS_SRC_DIAG_HPP_
#define CPPVIEWS_SRC_DIAG_HPP_

#include "list.hpp"

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

template<typename Size>
constexpr Size MinSize(Size&& size) { return std::forward<Size>(size); }

template<typename Size1, typename Size2, typename... Sizes>
constexpr typename std::common_type<Size1, Size2, Sizes...>::type
MinSize(Size1&& size1, Size2&& size2, Sizes&&... sizes) {
  return size1 < size2
                 ? MinSize(size1, std::forward<Sizes>(sizes)...)
                 : MinSize(size2, std::forward<Sizes>(sizes)...);
}

template<typename Size>
constexpr bool SameSize(const Size& size) { return true; }

template<typename Size1, typename Size2, typename... Sizes>
constexpr bool SameSize(const Size1& size1, const Size2& size2,
                        const Sizes&... sizes) {
  return (size1 == size2) & SameSize(size2, sizes...);
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
    : public V_THIS_BASE_TYPE {
  friend class V_THIS_BASE_TYPE;
  typedef V_THIS_BASE_TYPE DiagHelper;
#undef V_THIS_BASE_TYPE
#undef V_LIST_TYPE

 public:
  template<typename... Sizes>
  List(DataType* default_value, size_t size, Sizes... sizes)
      : DiagHelper(size, sizes...),
        blocks_(ComputeBlockCount(cpp14::index_sequence_for<size_t, Sizes...>(),
                                  size, sizes...)),
        default_value_(default_value) {}

  List(size_t block_count, DataType* default_value = nullptr)
      : DiagHelper((block_count * block_sizes)...),
        blocks_(block_count),
        default_value_(default_value) {}

  // used by MakeList
  template<typename... Args>
  List(DiagTag<BlockSize, block_sizes...>, Args&&... args)
      : List(std::forward<Args>(args)...) {}

  friend List MakeList(List&& list) { return std::forward<List>(list); }

  typename View<DataType>::Iterator iterator_begin() const override {
    // TODO: implement
    return *reinterpret_cast<typename View<DataType>::Iterator*>(NULL);
  }

  size_t block_count() const { return blocks_.size(); }

 private:
  typedef detail::DiagBlock<DataType, BlockSize, block_sizes...> DiagBlockType;

  template<size_t... Is, typename... Sizes>
  static size_t ComputeBlockCount(cpp14::index_sequence<Is...>, Sizes... sizes)
  {
    return detail::MinSize(((sizes + block_sizes - 1) /
                            std::get<Is>(dim_scalers()))...);
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
    if (!detail::SameSize(block_index = index / std::get<I>(dim_scalers()),
                          (indexes / std::get<Is>(dim_scalers()))...))
      return *default_value_;

    return blocks_[block_index](
        index - block_index * std::get<I>(dim_scalers()),
        (indexes - block_index * std::get<Is>(dim_scalers()))...);
  }

  mutable std::vector<DiagBlockType> blocks_;
  DataType* default_value_;
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
    : public V_THIS_BASE_TYPE {
  friend class V_THIS_BASE_TYPE;
  typedef V_THIS_BASE_TYPE DiagHelper;
#undef V_THIS_BASE_TYPE
#undef V_LIST_TYPE

 public:
  template<typename... Sizes>
  List(DataType* default_value, size_t size, Sizes... sizes)
      : DiagHelper(size, sizes...),
        blocks_(detail::MinSize(size, sizes...)),
        default_value_(default_value) {}

  List(size_t block_count, DataType* default_value = nullptr)
      : DiagHelper((block_count * block_sizes)...),
        blocks_(block_count),
        default_value_(default_value) {}

  // used by MakeList
  template<typename... Args>
  List(DiagTag<BlockSize, block_sizes...>, Args&&... args)
      : List(std::forward<Args>(args)...) {}

  friend List MakeList(List&& list) { return std::forward<List>(list); }

  typename View<DataType>::Iterator iterator_begin() const override {
    // TODO: implement
    return *reinterpret_cast<typename View<DataType>::Iterator*>(NULL);
  }

  size_t block_count() const { return blocks_.size(); }

 private:
  template<size_t I, size_t... Is, typename Index, typename... Indexes>
  DataType& get0(cpp14::index_sequence<I, Is...>, const Index& index,
                 const Indexes&... indexes) const {
    if (!detail::SameSize(index, indexes...))
      return *default_value_;
    return blocks_[index];
  }

  mutable std::vector<DataType> blocks_;
  DataType* default_value_;
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
