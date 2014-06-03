#ifndef CPPVIEWS_SRC_LIST_HPP_
#define CPPVIEWS_SRC_LIST_HPP_

#include "portion.hpp"
#include "portion_helper.hpp"
#include "view.hpp"
#include "util/immutable_skip_list.hpp"
#include "util/intseq.hpp"
#include "util/poly_vector.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

namespace v {

// Sub = (<offset>, <size>)
typedef std::pair<size_t, size_t> Sub;

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

template<size_t T>
constexpr size_t ZeroSize() { return size_t{}; }

template<class Container>
size_t SizeSum(const Container& subcontainers) {
  size_t sum = 0;
  for (const auto& subcontainer : subcontainers)
    sum += subcontainer.size();
  return sum;
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

// useful for evaluating an expression on a parameter pack, e.g.
// Ignore(std::get<Index...>(array) += 2)...
template<typename... Args>
void Ignore(Args&&... args) {}

template<class SubviewContainer>
using SubviewContainerElementType = typename
    SubviewContainer::ValueType::DataType;

template<class ForwardIter>
using IsForwardIter = std::is_same<
  typename std::iterator_traits<ForwardIter>::iterator_category,
  std::forward_iterator_tag>;

template<class SubviewContainer, unsigned dim>
struct ListSizeGetter {
  ListSizeGetter(const SubviewContainer& c) : c_(c) {}
  size_t operator()(size_t bucket_index) const {
    return c_[bucket_index].dim_size(dim);
  }
 private:
  const SubviewContainer& c_;
};

template<class SubviewContainer, unsigned dim = 1>
class LinearizedMonotonicIndexer {
  typedef ImmutableSkipList<ListSizeGetter<SubviewContainer, dim> >
  ImmutableSkipListType;
 public:
  LinearizedMonotonicIndexer(const SubviewContainer& c) :
      bsg_(c),
      fwd_(c.size(), bsg_) {}
  const ImmutableSkipListType& fwd() const { return fwd_; }
 private:
  ListSizeGetter<SubviewContainer, dim> bsg_;
  ImmutableSkipListType fwd_;
  // TODO optimize short jumps (need also a "backward" skip list)
  // ImmutableSkipList<ReverseBucketSizeGetter> bwd_;
};

constexpr bool And2(bool cond1, bool cond2) { return cond1 && cond2; }

template<class OuterIter, class ListType, typename DataType>
class List1DForwardIter
    : public DefaultIterator<List1DForwardIter<OuterIter, ListType, DataType>,
                             std::forward_iterator_tag,
                             DataType>,
      public View<DataType>::IteratorBase {
  V_DEFAULT_ITERATOR_DERIVED_HEAD(List1DForwardIter);
  template<class, class, class> friend class List1DForwardIter;
 public:
  explicit List1DForwardIter(const OuterIter& outer_cur, const ListType& list)
      : outer_cur_(outer_cur),
        list_(&list) {
    if (list.size()) {
      inner_cur_ = outer_cur->begin();
      inner_end_ = outer_cur->end();
    }
  }
  template<class OuterIter2, typename DataType2>
  List1DForwardIter(
      const List1DForwardIter<OuterIter2, ListType, DataType2>& other,
      typename std::enable_if<And2(
          std::is_convertible<OuterIter2, OuterIter>::value,
          std::is_convertible<DataType2*, DataType*>::value),
      Enabler>::type = Enabler())
      : outer_cur_(other.outer_cur_),
        inner_cur_(other.inner_cur_),
        inner_end_(other.inner_end_) {}
  List1DForwardIter() = default;

 protected:
  V_DEF_VIEW_ITER_IS_EQUAL(DataType, List1DForwardIter)

  template<class OuterIter2, typename DataType2>
  bool
  IsEqual(const List1DForwardIter<OuterIter2, ListType, DataType2>& other) const
  override {
    return outer_cur_ == other.outer_cur_ &&
        (list_->size() == 0 || inner_cur_ == other.inner_cur_);
  }

  void Increment() override {
    ++inner_cur_;
    if (inner_cur_ == inner_end_) {
      // rely on the invariant that list contains no empty portions/subviews,
      // except the last one which is used as a sentinel
      // (this reduces the time complexity of this operation)
      inner_end_ = (++outer_cur_)->end();
      inner_cur_ = outer_cur_->begin();
    }
  }
  DataType& ref() const override { return *inner_cur_; }

 private:
  // XXX: IndirectIterator value_type can be a proxy object type, so use pointer
  typedef typename std::remove_pointer<
   typename std::iterator_traits<OuterIter>::pointer>::type SubviewType;
  typedef typename SubviewType::Iterator InnerIter;

  OuterIter outer_cur_;
  InnerIter inner_cur_;
  InnerIter inner_end_;
  const ListType* list_;
};

template<class OuterIter, class Indexer, typename DataType>
class List1DIter
    : public DefaultIterator<List1DIter<OuterIter, Indexer, DataType>,
                             std::random_access_iterator_tag,
                             DataType>,
      public View<DataType>::IteratorBase {
  V_DEFAULT_ITERATOR_DERIVED_HEAD(List1DIter);
  template<class, class, class> friend class List1DIter;
 public:
  explicit List1DIter(const OuterIter& outer_begin, size_t index,
                      const Indexer& indexer)
      : indexer_(&indexer),
        outer_begin_(outer_begin),
        global_index_(index) {}
  template<class OuterIter2, typename DataType2>
  List1DIter(const List1DIter<OuterIter2, Indexer, DataType2>& other,
             typename std::enable_if<And2(
                 std::is_convertible<OuterIter2, OuterIter>::value,
                 std::is_convertible<DataType2*, DataType*>::value),
             Enabler>::type = Enabler())
      : indexer_(indexer_),
        outer_begin_(other.outer_begin_),
        global_index_(other.global_index_) {}
  List1DIter() = default;

 protected:
  V_DEF_VIEW_ITER_IS_EQUAL(DataType, List1DIter)

  template<class OuterIter2, typename DataType2>
  bool IsEqual(const List1DIter<OuterIter2, Indexer, DataType2>& other) const
  override {
    return global_index_ == other.global_index_;
  }

  void Increment() override { ++global_index_; }
  void Advance(typename DefaultIterator_::difference_type n) {
    global_index_ += n;
  }

  template<class OuterIter2, typename DataType2>
  typename DefaultIterator_::difference_type
  DistanceTo(const List1DIter<OuterIter2, Indexer, DataType2>& to) const {
    return to.global_index_ - global_index_;
  }

  DataType& ref() const override {
    // TODO: make more efficient by using inner iterators and bucket index
    auto pos = indexer_->fwd().get(global_index_);
    return *((outer_begin_ + pos.first)->begin() + pos.second);
  }

 private:
  // XXX: IndirectIterator value_type can be a proxy object type, so use pointer
  typedef typename std::remove_pointer<
   typename std::iterator_traits<OuterIter>::pointer>::type SubviewType;
  typedef typename SubviewType::Iterator InnerIter;

  const Indexer* indexer_;
  OuterIter outer_begin_;
  InnerIter inner_cur_;
  size_t global_index_;
};

}  // namespace list_detail

template<typename T, unsigned dims = 1>
class ListBase : public View<T> {
  // struct Disabler {};  // used for SFINAE (to disable constructors
 public:
  typedef std::array<size_t, dims> SizeArray;

  static constexpr unsigned kDims = dims;

  template<typename... Sizes>
  ListBase(size_t size,  // TODO: use SFINAE as below
           // typename std::conditional<1 + sizeof...(Sizes) == dims,
           // size_t, Disabler>::type size,
           Sizes... sizes) :
      View<T>(list_detail::SizeProduct(size, static_cast<size_t>(sizes)...)),
      strides_{{size, static_cast<size_t>(sizes)...}},
    offsets_{{list_detail::ZeroSize<Sizes>()...}} {
      // XXX: this *sometimes* fails for some reason ... (so does SFINAE above)
      // static_assert(1 + sizeof...(Sizes) == dims,
      //               "The number of sizes does not match dims");
    }
  virtual ~ListBase() noexcept = default;

  virtual T& get(const SizeArray& indexes) const = 0;

  const SizeArray& strides() const { return strides_; }

  typename View<T>::Iterator fbegin() const {
    return this->begin();
  }
  typename View<T>::Iterator fend() const {
    return this->end();
  }

 protected:
  SizeArray strides_;
  SizeArray offsets_;
};

enum ListFlags : uint8_t {
  kListNoFlags,
      kListOpMin,

      // mutation operations must be first
      kListOpInsBack = kListOpMin << 0,
      kListOpInsFront = kListOpMin << 1,
      kListOpInsMid = kListOpMin << 2,
      kListOpDelBack = kListOpMin << 3,
      kListOpDelFront = kListOpMin << 4,
      kListOpDelMid = kListOpMin << 5,
      kListOpMutable,  // works because the previous element has value 2^k

      kListOpInsEnd = kListOpInsBack | kListOpInsFront,
      kListOpIns = kListOpInsEnd | kListOpInsMid,
      kListOpDelEnd = kListOpDelBack | kListOpDelFront,
      kListOpDel = kListOpDelEnd | kListOpDelMid,
      kListOpVector = kListOpInsBack | kListOpDelBack,
      kListOpQueue = kListOpInsBack | kListOpDelFront,
      kListOpDeque = kListOpInsEnd | kListOpDelEnd
      };

struct ListFactory {
  template<typename... Args>
  constexpr auto operator()(Args&&... args) const ->
      decltype(MakeList(std::forward<Args>(args)...)) {
    return MakeList(std::forward<Args>(args)...);
  }
};

template<typename T, class L = ListBase<T> >
using SubviewVector = PolyVector<L, ListBase<T, L::kDims>, ListFactory>;

template<class SubviewType, unsigned dims = 1,
         ListFlags flags = kListNoFlags,
         class Indexer = list_detail::LinearizedMonotonicIndexer<
           SubviewVector<typename SubviewType::DataType, SubviewType>
           >,
         typename T = typename SubviewType::DataType,
         class Enable = void  // used for SFINAE (in specialization)
         >
class List;
//     : public ListBase<T, dims> {
//   // typedef List<T, SubviewVector<List>, dims, Indexer> CompositeType;

//   class Iterator : public std::iterator<std::forward_iterator_tag, T>,
//                    public View<T>::IteratorBase {
//    public:
//     Iterator(const Indexer* indexer, size_t index)
//         : indexer_(indexer),
//           index_(index) {}
//     Iterator(const Iterator&) = default;

//     V_DEF_VIEW_ITER_OPERATORS(Iterator, this)

//     bool operator==(const Iterator& other) const {
//       return indexer_ == other.indexer_ && index_ == other.index_;
//     }

//    protected:
//     V_DEF_VIEW_ITER_IS_EQUAL(Iterator)

//     void Increment() override { ++index_; }
//     T& ref() const override {
//       // TODO
//     }

//    private:
//     const Indexer* indexer_;

//     size_t index_;
//   };

//   // List() : indexer_(portions_) {}
//   // List(C&& subviews)
//   //     : ListBase<T, dims>(subviews.size()),
//   //       subviews_(std::forward<C>(subviews)),
//   //       indexer_(subviews_) {}

//   // List(typename C::ValueType... subviews)
//   //     : ListBase<T, dims>(c.size()),
//   //       subviews_(std::forward<C>(c)),
//   //       indexer_(subviews_) {}

//   inline void set(const T& value, size_t index) const {
//     const_cast<T&>(this->get(index)) = value;
//   }

//   inline const T& get(size_t index) const {
//     // return indexer_(index);
//   }

//   inline size_t max_size() const { return subviews_.max_size(); }

//  private:
//   C subviews_;
// };

// template<typename T, unsigned dims = 1>
// using CompositeList<T, dims> = List<T, ListBase<T, dims>, dims>;

// specializations for 1D lists

template<typename T, class P = PortionBase<T> >
using PortionVector = PolyVector<P, PortionBase<T>, PortionFactory>;

#define V_LIST_INDEXER_TYPE \
  list_detail::LinearizedMonotonicIndexer<PortionVector<typename P::DataType, P> >

template<class P>
using SimpleList = List<P, 1, kListOpVector, V_LIST_INDEXER_TYPE>;

template<class P>
struct SimpleListHelper {
  template<class Container>
  static void AppendEmpty(Container* c) {
    c->Append(c->back());
    c->back().Clear();
  }
};

template<typename T>
struct SimpleListHelper<PortionBase<T> > {
  template<class Container>
  static void AppendEmpty(Container* c) {
    typename Container::SizeType size = c->size();
    // c->Append();  // XXX: this should work after fixing a bug
    c->GrowBack();
    c->reset(size, nullptr);
  }
};

template<class P>
class List<P, 1, kListOpVector, V_LIST_INDEXER_TYPE>
    : public ListBase<typename P::DataType>,
      public PortionHelper<P, typename P::DataType>,
      protected SimpleListHelper<P> {
  typedef V_LIST_INDEXER_TYPE Indexer;
  typedef typename P::DataType DataType;
  typedef PortionVector<DataType, P> Container;
#undef V_LIST_INDEXER_TYPE

  typedef typename ListBase<DataType>::SizeArray SizeArray;

 public:
  typedef list_detail::List1DForwardIter<typename Container::Iterator,
                                         List,
                                         DataType> ForwardIterator;

  typedef list_detail::List1DIter<typename Container::Iterator,
                                  Indexer,
                                  DataType> Iterator;
  // XXX: change to ConstIterator after adding conversions for FakePointer
  typedef list_detail::List1DIter<typename Container::Iterator,
                                  Indexer,
                                  /* const */ DataType> ConstIterator;

  // List() : indexer_(portions_) {}
  List(Container&& pv)
      : ListBase<DataType>(list_detail::SizeSum<Container>(pv)),
        container_(std::forward<Container>(pv)),
        indexer_(container_) {
    // put a sentinel empty portion at the back
    if (container_.size())
      SimpleListHelper<P>::template AppendEmpty(&container_);

    container_.Shrink();
  }

  inline void set(const DataType& value, size_t index) const {
    const_cast<DataType&>(this->get(index)) = value;
  }

  inline const DataType& get(size_t index) const {
    auto pos = indexer_.fwd().get(index);
    return container_[pos.first].get(pos.second);
  }

  DataType& get(const SizeArray& indexes) const {
    return const_cast<DataType&>(this->get(indexes.front()));
  }
  void set(const DataType& value, const SizeArray& indexes) const {
    set(value, indexes.front());
  }

  inline size_t max_size() const { return container_.max_size(); }

  ForwardIterator begin() const {
    return ForwardIterator(const_cast<Container&>(container_).begin(), *this);
  }

  ForwardIterator end() const {
    auto it = const_cast<Container&>(container_).end();
    if (this->size_)
      --it;
    return ForwardIterator(it, *this);
  }

  Iterator lbegin() const {
    return Iterator(const_cast<Container&>(container_).begin(), 0, indexer_);
  }

  Iterator lend() const {
    return Iterator(const_cast<Container&>(container_).begin(), this->size(),
                    indexer_);
  }

 protected:
  typename View<DataType>::Iterator iterator_begin() const override {
    return new ForwardIterator(this->begin());
  }

  // overrides View<DataType>::Iterator to provide O(1) time complexity
  typename View<DataType>::Iterator iterator_end() const override {
    return new ForwardIterator(this->end());
  }

 private:
  Container container_;
  Indexer indexer_;
};

// specialization for Implicit Lists (those that do not store portions)

template<typename T>
using ListAccessorFunction = std::function<T* const(size_t)>;

template<typename T, unsigned dims = 1,
         class Accessor = ListAccessorFunction<T> >
using ImplicitList = List<void, dims, kListNoFlags, Accessor, T>;

template<typename T, unsigned dims, class Accessor>
class List<void, dims, kListNoFlags, Accessor, T>
    : public ListBase<T, dims> {
  typedef typename ListBase<T, dims>::SizeArray SizeArray;
  struct Disabler {};  // used for SFINAE (to disable constructors)

 public:
  class Iterator : public std::iterator<std::random_access_iterator_tag, T>,
                   public View<T>::IteratorBase {
   public:
    Iterator(const Iterator&) = default;
    Iterator(Iterator&&) = default;
    template<typename... Sizes>
    Iterator(const List* list, size_t index, Sizes... indexes)
        : list_(list),
          indexes_{{index, static_cast<size_t>(indexes)...}} {
    }

    bool operator==(const Iterator& other) const {
      // XXX this is terribly inefficient and is never equal to a subview iter
      // (should compare offsets in reverse order)
      return list_ == other.list_ && indexes_ == other.indexes_;
    }

    V_DEF_VIEW_ITER_OPERATORS(Iterator, this)

   protected:
    V_DEF_VIEW_ITER_IS_EQUAL(T, Iterator)

    void Increment() {
      if (++indexes_.back() >= list_->strides_.back()) {
        // TODO: use a constexpr function that unrolls the loop at compile time
        for (int dim = dims - 1; dim > 0; ) {
          indexes_[dim--] = 0;
          if (++indexes_[dim] < list_->strides_[dim])
            break;
        }
      }
    }

    T& ref() const { return ref(cpp14::make_index_sequence<dims>()); }

   private:
    template<size_t... Indexes>
    T& ref(cpp14::index_sequence<Indexes...>) const {
      return (*list_)(std::get<Indexes>(indexes_)...);
    }

    const List* list_;
    std::array<size_t, dims> indexes_;
  };

  template<typename... Sizes>
  List(Accessor accessor,
       typename std::conditional<1 + sizeof...(Sizes) == dims,
       size_t, Disabler>::type size,
       Sizes... sizes)
      : ListBase<T, dims>(size, sizes...),
        accessor_(accessor),
        strides_{{size, static_cast<size_t>(sizes)...}},
    offsets_{{list_detail::ZeroSize<Sizes>()...}} {}

  template<typename... Subs>
  List(Accessor accessor, Sub sub, Subs... subs)
      : ListBase<T, dims>(list_detail::SizeProduct(
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
  const T& get(Indexes... indexes) const {
    return this->operator()(indexes...);
  }
  template<typename... Indexes>
  void set(const T& value, Indexes... indexes) const {
    this->operator()(indexes...) = value;
  }

  T& get(const SizeArray& indexes) const {
    return this->get0(indexes, cpp14::make_index_sequence<dims>());
  }

  const SizeArray& strides() const { return strides_; }

  typename View<T>::Iterator begin() const {
    return this->iterator_begin();
  }
  typename View<T>::Iterator end() const {
    return this->iterator_end();
  }

  typename View<T>::Iterator iterator_begin() const {
    return begin(cpp14::make_index_sequence<dims>());
  }

  // overrides View<T>::Iterator to provide O(1) time complexity
  typename View<T>::Iterator iterator_end() const {
    return end(cpp14::make_index_sequence<dims>());
  }

 private:
  template<size_t... Indexes>
  T& get0(const SizeArray& indexes, cpp14::index_sequence<Indexes...>) const {
    return this->operator()(std::get<Indexes>(indexes)...);
  }

  template<size_t... Indexes>
  Iterator* begin(cpp14::index_sequence<Indexes...>) const {
    return new Iterator(this, list_detail::ZeroSize<Indexes>()...);
  }

  template<size_t Index, size_t... Indexes>
  Iterator* end(cpp14::index_sequence<Index, Indexes...>) const {
    return new Iterator(this, strides_.front(),
                        list_detail::ZeroSize<Indexes>()...);
  }

  Accessor accessor_;
  SizeArray strides_;
  SizeArray offsets_;
};

template<typename T, class P>
constexpr SimpleList<P> MakeList(PortionVector<T, P>&& pv) {
  return SimpleList<P>(std::forward<PortionVector<T, P> >(pv));
}

template<class Accessor, typename... Sizes>
constexpr auto MakeList(Accessor a, size_t size, Sizes... sizes)
#define V_LIST_TYPE                                                       \
    ImplicitList<typename                                               \
                 std::remove_pointer<decltype(a(size, sizes...))>       \
                 ::type, (1 + sizeof...(Sizes)), Accessor>
    -> V_LIST_TYPE { return V_LIST_TYPE(a, size, sizes...); }
#undef V_LIST_TYPE


template<class Accessor, typename... Subs>
constexpr auto MakeList(Accessor a, Sub sub, Subs... subs)
#define V_LIST_TYPE                                                     \
    ImplicitList<typename                                               \
                 std::remove_pointer<decltype(a(sub.first, subs.first...))> \
                 ::type, (1 + sizeof...(Subs)), Accessor>
    -> V_LIST_TYPE { return V_LIST_TYPE(a, sub, subs...); }
#undef V_LIST_TYPE

}  // namespace v

#endif  /* CPPVIEWS_SRC_LIST_HPP_ */
