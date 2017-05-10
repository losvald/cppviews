#ifndef CPPVIEWS_SRC_SPARSE_LIST_HPP_
#define CPPVIEWS_SRC_SPARSE_LIST_HPP_

#include "list.hpp"
#include "util/intseq.hpp"
#include "util/iterator.hpp"

#include <array>
#include <cstdint>
#include <map>

namespace v {

// For the MakeList overload, we need to somehow encode the block sizes.
template<unsigned dim_count>
struct SparseListTag {};

template<typename T, unsigned dims>
using SparseHashList = List<void, dims, kListOpMutable, void, T>;

template<class MapType>
class SparseListForwardIter
    : public DefaultIterator<SparseListForwardIter<MapType>,
                             std::forward_iterator_tag,
                             typename MapType::mapped_type>,
      public View<typename MapType::mapped_type>::IteratorBase {

  V_DEFAULT_ITERATOR_DERIVED_HEAD(SparseListForwardIter);
  template<typename> friend class SparseListForwardIter;

  using Enabler = typename SparseListForwardIter::Enabler;
  using DataType = typename MapType::mapped_type;

 public:
  template<typename DataType2>
  SparseListForwardIter(const SparseListForwardIter<DataType2>& other,
                        EnableIfIterConvertible<DataType2, DataType,
                        Enabler> = Enabler()) {}

  SparseListForwardIter(typename MapType::iterator it)
      : it_(std::move(it)) {}

 protected:
  V_DEF_VIEW_ITER_IS_EQUAL(DataType, SparseListForwardIter);

  template<typename DataType2>
  bool IsEqual(const SparseListForwardIter<DataType2>& other) const {
    return true;  // conforms to C++ spec. (this->it_ == other.it_ is no better)
  }

  void Increment() override { ++it_; }

  DataType& ref() const override { return it_->second; }

 private:
  typename MapType::iterator it_;
};

template<typename T, unsigned dims>
class List<void, dims, kListOpMutable, void, T>
    : public ListBase<T, dims> {

  using ListBaseType = ListBase<T, dims>;
  using typename ListBaseType::SizeArray;
  using ListBaseType::kDims;

  // template<typename E>
  // struct Get2ndTransformer {
  //   auto operator=(const E& e) -> decltype(e.second) { return e.second; }
  // };

  using MapType = std::map<std::array<size_t, dims>, T>;

 public:
  using ForwardIterator = SparseListForwardIter<MapType>;

  // TODO use SFINAE to enable only if dims == sizeof...(Sizes)
  template<typename... Sizes>
  explicit List(SparseListTag<sizeof...(Sizes)>,
       T* default_value,
       Sizes&&... sizes)
      : ListBaseType(std::forward<Sizes>(sizes)...) {
  }

  friend List MakeList(List&& list) { return std::forward<List>(list); }

  template<typename... Indexes>
  const T& operator()(Indexes&&... indexes) const {
    // TODO this does not type-check if used as mutable
    return static_cast<const decltype(this)>(this)->get(
        std::forward<Indexes>(indexes)...);
  }

  template<typename... Indexes>
  const T& get(Indexes&&... indexes) const {
    try {
      return map_.at(SizeArray{{std::forward<Indexes>(indexes)...}});
    } catch (const std::out_of_range&) {
      return *default_val_;
    }
  }

  T& get(SizeArray&& indexes) const override { return map_[indexes]; }

  const List& values() const override { return *this; }

  size_t nondefault_count() const { return map_.size(); }

  // non-polymorphic iterators (no dynamic allocation)

  ForwardIterator begin() const { return ForwardIterator(map_.begin()); }
  ForwardIterator end() const { return ForwardIterator(map_.end()); }

 protected:

  // polymorphic iterators

  typename View<T>::Iterator iterator_begin() const override {
    return new ForwardIterator(map_.begin());
  }

  // overrides View<DataType>::Iterator to provide O(1) time complexity
  typename View<T>::Iterator iterator_end() const override {
    return new ForwardIterator(map_.end());
  }

 private:
  mutable MapType map_;
  T* default_val_;
};

template<typename T, unsigned dims, typename... Sizes>
auto MakeList(SparseListTag<dims>, T* default_value, Sizes... sizes)
#define V_LIST_TYPE SparseHashList<T, sizeof...(sizes)>
    -> V_LIST_TYPE {
  return V_LIST_TYPE(SparseListTag<dims>(), default_value, sizes...); }
#undef V_LIST_TYPE

}  // namespace v

#endif  /* CPPVIEWS_SRC_SPARSE_LIST_HPP_ */
