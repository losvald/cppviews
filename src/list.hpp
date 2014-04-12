#ifndef CPPVIEWS_SRC_LIST_HPP_
#define CPPVIEWS_SRC_LIST_HPP_

#include "portion.hpp"
#include "portion_helper.hpp"
#include "view.hpp"
#include "util/immutable_skip_list.hpp"
#include "util/poly_vector.hpp"

#include <array>
#include <functional>
#include <type_traits>
#include <utility>

namespace v {

// Sub = (<offset>, <size>)
typedef std::pair<size_t, size_t> Sub;

template<typename T, class P = PortionBase<T> >
using PortionVector = PolyVector<P, PortionBase<T>, PortionFactory>;

namespace list_detail {

// a variadic function that is inlined as (size1 * ... * sizeN) at compile time
template<typename Size, typename... Sizes>
typename std::enable_if<std::is_same<Size, size_t>::value, Size>::type
constexpr SizeProduct(Size size, Sizes... sizes) {
  return size * SizeProduct<Sizes...>(sizes...);
}

template<>
size_t constexpr SizeProduct<size_t>(size_t size) { return size; }

template<typename T>
constexpr size_t ZeroSize() {
  return size_t{};
}

template <typename... Ns>
struct Pairwise;

template <>
struct Pairwise<> {
  template <typename R, typename F, typename T, typename... Args>
  static R Sum0(F f, const T&, Args... sums) { return f(sums...); }
};

template <typename N0, typename... Ns>
struct Pairwise<N0, Ns...> {
  // returns F(num0 + std::get<0>(a), ...)
  template <typename F, typename T>
  static typename std::result_of<F(N0, Ns...)>::type
  Sum(F f, N0 num0, Ns... nums, const T& a) {
    return Pairwise<N0, Ns...>::template
        Sum0<typename std::result_of<F(N0, Ns...)>::type>(f, num0, nums..., a);
  }

  template <typename R, typename F, typename T, typename... Args>
  static R Sum0(F f, N0 num0, Ns... nums, const T& a, Args&&... sums) {
    return Pairwise<Ns...>::template
        Sum0<R>(f, nums..., a, sums...,
                num0 + static_cast<N0>(std::get<sizeof...(Args)>(a)));
  }
};

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

}  // namespace list_detail

template<typename T, class P = PortionBase<T>, unsigned dims = 1,
         class Indexer = list_detail::LogarithmicIndexer<PortionVector<T, P> > >
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

// specialization for Implicit Lists (those that do not store portions)
template<typename T, unsigned dims, class Accessor>
class List<T, void, dims, Accessor> : public View<T> {
 public:
  typedef std::array<size_t, dims> Strides;

  template<typename... Sizes>
  List(Accessor accessor, size_t size, Sizes... sizes)
      : View<T>(list_detail::SizeProduct(size, static_cast<size_t>(sizes)...)),
        accessor_(accessor),
        strides_{{size, static_cast<size_t>(sizes)...}},
    offsets_{{list_detail::ZeroSize<Sizes>()...}} {
      static_assert(1 + sizeof...(Sizes) == dims,
                    "The number of sizes does not match dims");
  }

  template<typename... Subs>
  List(Accessor accessor, Sub sub, Subs... subs)
      : View<T>(list_detail::SizeProduct(
          sub.second, static_cast<Sub>(subs).second...)),
        accessor_(accessor),
        strides_{{sub.second, static_cast<Sub>(subs).second...}},
    offsets_{{sub.first, static_cast<Sub>(subs).first...}} {
      static_assert(1 + sizeof...(Subs) == dims,
                    "The number of subs does not match dims");
    }

  template<typename... Subs>
  List(const List& list, Sub sub, Subs... subs)
      : List(list.accessor_, sub, subs...) {
    // TODO make the constructor trivial by expanding offset sum at compile time
    auto it = list.offsets_.cbegin();
    for (auto& offset : offsets_)
      offset += *it++;
  }

  template<typename... Indexes>
  T& operator()(Indexes... indexes) const {
    return *(list_detail::Pairwise<Indexes...>::template
             Sum(accessor_, indexes..., offsets_));
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

  const std::array<size_t, dims>& offsets() { return offsets_; }

 private:
  Accessor accessor_;
  Strides strides_;
  std::array<size_t, dims> offsets_;
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

template<class Accessor, typename... Sizes>
constexpr auto MakeList(Accessor accessor, size_t size, Sizes... sizes)
    -> List<typename std::remove_pointer<
  decltype(accessor(size, sizes...))>::type, void, (1 + sizeof...(Sizes)),
            Accessor> {
  return List<typename std::remove_pointer<
    decltype(accessor(size, sizes...))>::type, void, (1 + sizeof...(Sizes)),
              Accessor>(accessor, size, sizes...);
}

template<class Accessor, typename... Subs>
constexpr auto MakeList(Accessor accessor, Sub sub, Subs... subs)
    -> List<typename std::remove_pointer<
  decltype(accessor(sub.first, subs.first...))>::type,
            void, (1 + sizeof...(Subs)), Accessor> {
  return List<typename std::remove_pointer<
    decltype(accessor(sub.first, subs.first...))>::type,
              void, (1 + sizeof...(Subs)), Accessor>(
                  accessor, sub, subs...);
}

}  // namespace v

#endif  /* CPPVIEWS_SRC_LIST_HPP_ */
