#ifndef CPPVIEWS_SRC_LIST_HPP_
#define CPPVIEWS_SRC_LIST_HPP_

#include "map.hpp"
#include "portion.hpp"
#include "portion_helper.hpp"
#include "view.hpp"
#include "util/immutable_skip_list.hpp"
#include "util/intseq.hpp"
#include "util/poly_vector.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace v {

// Sub = (<offset>, <size>)
typedef std::pair<size_t, size_t> Sub;

namespace list_detail {

// a variadic function that is inlined as (size1 * ... * sizeN) at compile time
template<typename Size, typename... Sizes>
typename std::enable_if<std::is_same<Size, size_t>::value, Size>::type
constexpr SizeProduct(const Size& size, Sizes&&... sizes) {
  return size * SizeProduct<Sizes...>(std::forward<Sizes>(sizes)...);
}

template<>
size_t constexpr SizeProduct<size_t>(const size_t& size) { return size; }

template<typename T>
constexpr size_t ZeroSize() {
  return size_t{};
}

template<size_t T>
constexpr size_t ZeroSize() { return size_t{}; }

template<size_t dummy>
constexpr size_t ConstSize(const size_t& size) { return size; }

template<class Container>
size_t SizeSum(const Container& subcontainers) {
  size_t sum = 0;
  for (const auto& subcontainer : subcontainers)
    sum += subcontainer.size();
  return sum;
}

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
constexpr bool SameSize(Size&&) { return true; }

template<typename Size1, typename Size2, typename... Sizes>
constexpr bool SameSize(const Size1& size1, const Size2& size2,
                        Sizes&&... sizes) {
  using namespace std;
  return (size1 == size2) & SameSize(size1, forward<Sizes>(sizes)...);
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
        Sum0<typename std::result_of<F(N0, Ns...)>::type>(
            f, std::move(num0), std::forward<Ns>(nums)..., a);
  }

  template <typename R, typename F, typename T, typename... Args>
  static R Sum0(F f, N0&& num0, Ns&&... nums, const T& a, Args&&... sums) {
    return Pairwise<Ns...>::template
        Sum0<R>(f, std::forward<Ns>(nums)..., a, sums...,
                num0 + static_cast<N0>(std::get<sizeof...(Args)>(a)));
  }
};

// useful for evaluating an expression on a parameter pack, e.g.
// Ignore(std::get<Index...>(array) += 2)...
template<typename... Args>
void Ignore(Args&&... args) {}

template<size_t N, typename T, T... Ts>
struct GetNth {
  static constexpr T values[sizeof...(Ts)] = {Ts...};
  static const T value = values[N];
};

template<size_t N, typename T, T... Ts>
constexpr T GetNth<N, T, Ts...>::values[sizeof...(Ts)];

template<typename DataType>
class DummyListIter
    : public DefaultIterator<DummyListIter<DataType>,
                             std::random_access_iterator_tag,
                             DataType>,
      public View<DataType>::IteratorBase {
  V_DEFAULT_ITERATOR_DERIVED_HEAD(DummyListIter)
  template<typename> friend class DummyListIter;
  using Enabler = typename DummyListIter::Enabler;

 public:
  explicit DummyListIter() {
  }

  template<typename DataType2>
  DummyListIter(const DummyListIter<DataType2>& other,
                typename std::enable_if<
                std::is_convertible<DataType2*, DataType*>::value,
                Enabler>::type = Enabler()) {}

 protected:
  V_DEF_VIEW_ITER_IS_EQUAL(DataType, DummyListIter)
  template<typename DataType2>
  bool IsEqual(const DummyListIter<DataType2>& other) const {
    return true;
  }
  void Increment() override {}

  template<typename DataType2>
  typename DummyListIter::difference_type
  DistanceTo(const DummyListIter<DataType2>& to) const { return 0; }
  void Advance(typename DummyListIter::difference_type n) {}

  DataType& ref() const override {
    return *static_cast<DataType*>(nullptr);  // Undefined Behavior, but fast
  }
};

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
  size_t bucket_size(const size_t& index) const { return bsg_(index); }
  const ImmutableSkipListType& fwd() const { return fwd_; }
 private:
  ListSizeGetter<SubviewContainer, dim> bsg_;
  ImmutableSkipListType fwd_;
  // TODO optimize short jumps (need also a "backward" skip list)
  // ImmutableSkipList<ReverseBucketSizeGetter> bwd_;
};

constexpr bool And2(bool cond1, bool cond2) { return cond1 && cond2; }

// Checks whether the unlinearized position "pos" is in the range [0, to_index).
template<typename Size, Size size, typename LastIndex>
constexpr bool WithinLast(const size_t& pos, LastIndex&& to_index) {
  return pos < to_index;
}

// Checks whether the unlinearized position "pos" is in the range
// (0, ...) (inclusive) to (to_index, to_indexes...) (exclusive).
template<typename Size, Size size, Size... sizes, typename Index,
         typename... Indexes>
constexpr bool WithinLast(const size_t& pos, Index&& to_index,
                          Indexes&&... to_indexes) {
  return WithinLast<Size, sizes...>(pos / size,
                                    std::forward<Indexes>(to_indexes)...) &
      (pos - pos / size * size < to_index);
}

template<class OuterIter, class ListType, typename DataType>
class List1DForwardIter
    : public DefaultIterator<List1DForwardIter<OuterIter, ListType, DataType>,
                             std::forward_iterator_tag,
                             DataType>,
      public View<DataType>::IteratorBase {
  V_DEFAULT_ITERATOR_DERIVED_HEAD(List1DForwardIter);
  template<class, class, class> friend class List1DForwardIter;
  using typename List1DForwardIter::DefaultIterator_::Enabler;

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
  {
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
  using typename List1DIter::DefaultIterator_::Enabler;

 public:
  explicit List1DIter(const OuterIter& outer_begin, const size_t& index,
                      const Indexer& indexer)
      : indexer_(&indexer),
        outer_begin_(outer_begin),
        global_index_(index) {
    std::tie(bucket_index_, local_index_) = indexer_->fwd().get(index);
    UpdateInner(local_index_);
  }
  template<class OuterIter2, typename DataType2>
  List1DIter(const List1DIter<OuterIter2, Indexer, DataType2>& other,
             typename std::enable_if<And2(
                 std::is_convertible<OuterIter2, OuterIter>::value,
                 std::is_convertible<DataType2*, DataType*>::value),
             Enabler>::type = Enabler())
      : indexer_(indexer_),
        outer_begin_(other.outer_begin_),
        inner_cur_(other.inner_cur_),
        bucket_index_(other.bucket_index_),
        local_index_(other.local_index_),
        global_index_(other.global_index_) {}
  List1DIter() = default;

 protected:
  V_DEF_VIEW_ITER_IS_EQUAL(DataType, List1DIter)

  template<class OuterIter2, typename DataType2>
  bool IsEqual(const List1DIter<OuterIter2, Indexer, DataType2>& other) const
  {
    return global_index_ == other.global_index_;
  }

  void Increment() override {
    ++global_index_;
    if (++local_index_ == indexer_->bucket_size(bucket_index_)) {
      // rely on the invariant that list contains no empty portions/subviews,
      // except the last one which is used as a sentinel
      // (this reduces the time complexity of this operation)
      UpdateInner(0);
    } else
      ++inner_cur_;
  }

  void Advance(typename List1DIter::difference_type n) {
    global_index_ += n;

    // TODO implement indexer_->bwd() and use it
    if (n <= 0) {
      std::tie(bucket_index_, local_index_) = indexer_->fwd()
          .get(global_index_);
      UpdateInner(local_index_);
      return;
    }

    std::tie(bucket_index_, local_index_) = indexer_->fwd()
        .get(local_index_ + n, bucket_index_);
    UpdateInner(local_index_);
  }

  template<class OuterIter2, typename DataType2>
  typename List1DIter::difference_type
  DistanceTo(const List1DIter<OuterIter2, Indexer, DataType2>& to) const {
    return to.global_index_ - global_index_;
  }

  DataType& ref() const override { return *inner_cur_; }

 private:
  // XXX: IndirectIterator value_type can be a proxy object type, so use pointer
  typedef typename std::remove_pointer<
   typename std::iterator_traits<OuterIter>::pointer>::type SubviewType;
  typedef typename SubviewType::Iterator InnerIter;

  void UpdateInner(size_t local_index) {
    inner_cur_ = (outer_begin_ + bucket_index_)->begin() + local_index;
  }

  const Indexer* indexer_;
  OuterIter outer_begin_;
  InnerIter inner_cur_;
  size_t bucket_index_;
  size_t local_index_;
  size_t global_index_;
};

}  // namespace list_detail

template<class ListType>
struct ListTraits {
  static const unsigned kDims = ListType::kDims;
};

template<typename T, unsigned dims = 1>
class ListBase : public View<T>,
                 public Map<std::array<size_t, dims>, T> {
  // struct Disabler {};  // used for SFINAE (to disable constructors

 protected:
  template<class Derived, typename V, class Category, typename Distance>
  class EntryDimIteratorBase;

 public:
  typedef std::array<size_t, dims> SizeArray;
  template<typename V>
  struct Entry {
    explicit Entry(V* value, SizeArray&& indexes = SizeArray{})
        : val_(value),
          indexes_(std::move(indexes)) {}
    Entry() = default;
    const SizeArray& indexes() const { return indexes_; }
    V& value() { return *val_; }
   private:
    V* val_;
    SizeArray indexes_;
    template<class, typename, class, typename>
    friend class EntryDimIteratorBase;
  };
  static_assert(std::is_move_constructible<Entry<T> >::value, "");
  static_assert(std::is_move_constructible<Entry<const T> >::value, "");

  static constexpr unsigned kDims = dims;

  template<typename... Sizes>
  ListBase(const size_t& size,  // TODO: use SFINAE as below
           // const typename std::conditional<1 + sizeof...(Sizes) == dims,
           // size_t, Disabler>::type& size,
           Sizes&&... sizes) :
      View<T>(list_detail::SizeProduct(size, static_cast<size_t>(sizes)...)),
      sizes_{{size, static_cast<size_t>(sizes)...}},
    offsets_{{list_detail::ZeroSize<Sizes>()...}} {
      // XXX: this *sometimes* fails for some reason ... (so does SFINAE above)
      // static_assert(1 + sizeof...(Sizes) == dims,
      //               "The number of sizes does not match dims");
    }

  ListBase() = default;
  virtual ~ListBase() noexcept = default;

  virtual void ShrinkToFirst() {
    sizes_.fill(0);
  }

  virtual T& get(SizeArray&& indexes) const = 0;
  T& get(const SizeArray& indexes) const { return get(SizeArray(indexes)); }

  const SizeArray& sizes() const { return sizes_; }

  typename View<T>::Iterator fbegin() const {
    return this->begin();
  }
  typename View<T>::Iterator fend() const {
    return this->end();
  }

 protected:
  template<typename V>
  class PolyDimIterator {
   protected:
    typedef bool (*IsEqualPointer)(const PolyDimIterator& lhs,
                                   const PolyDimIterator& rhs);

    virtual PolyDimIterator* Clone() const = 0;
    virtual void Increment() = 0;
    virtual V& ref() const = 0;
    virtual IsEqualPointer equal() const = 0;
  };

  // helper class for easy derivation of PolyDimIterator by the subclasses
  template<class Derived, typename V, class Category,
           typename Distance = std::ptrdiff_t>
  class DimIteratorBase
      : public DefaultIterator<Derived, Category, V, Distance>,
        public PolyDimIterator<V> {
   protected:
    PolyDimIterator<V>* Clone() const override {
      return new Derived(this->derived());
    }
    static bool IsEqual(const PolyDimIterator<V>& lhs,
                        const PolyDimIterator<V>& rhs) {
      return static_cast<const Derived&>(lhs) ==
          static_cast<const Derived&>(rhs);
    }
    typename DimIteratorBase::IsEqualPointer equal() const override {
      return &IsEqual;
    }
  };

  template<class Derived, typename V, class Category,
           typename Distance = std::ptrdiff_t>
  class EntryDimIteratorBase : public DimIteratorBase<Derived, Entry<V>,
                                                      Category, Distance> {
   protected:
    // protected proxy functions to allow changes to Entry's indexes_
    // (since friendship is not inheritable)
    template<typename V2>
    static constexpr SizeArray& GetEntryIndexes(Entry<V2>& e) {
      return e.indexes_;
    }
    template<unsigned dim>
    static constexpr void SetEntryIndex(const size_t& index, Entry<V>& e) {
      std::get<dim>(e.indexes_) = index;
    }
  };

  template<typename V>
  class DimIterator : public DimIteratorBase<DimIterator<V>,
                                             V,
                                             std::forward_iterator_tag,
                                             std::ptrdiff_t> {
    V_DEFAULT_ITERATOR_DERIVED_HEAD(DimIterator);
   public:
    DimIterator(PolyDimIterator<V>*&& it) :
        it_(std::forward<PolyDimIterator<V>*&&>(it)) {}
    DimIterator(const DimIterator& copy) : it_(copy.it_->Clone()) {}
    DimIterator(DimIterator&& other) = default;
    DimIterator& operator=(const DimIterator& rhs) = default;
    DimIterator& operator=(DimIterator&& rhs) = default;
   protected:
    void Increment() override { it_->Increment(); }
    bool IsEqual(const DimIterator& rhs) const override {
      return it_->equal() == rhs.it_->equal() && it_->equal()(*it_, *rhs.it_);
    }
    virtual V& ref() const override { return it_->ref(); }
   private:
    std::unique_ptr<PolyDimIterator<V> > it_;
  };

  SizeArray sizes_;
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

struct ListFactory;

template<class ListType>
using ListVector = PolyVector<ListType,
                              ListBase<typename ListType::DataType,
                                       ListTraits<ListType>::kDims>,
                              ListFactory>;

template<class SubviewType, unsigned dims = 1,
         ListFlags flags = kListNoFlags,
         class Indexer = list_detail::LinearizedMonotonicIndexer<
           ListVector<SubviewType> >,
         typename T = typename SubviewType::DataType,
         class Enable = void  // used for SFINAE (in specialization)
         >
class List;

// specialization for dummy list (all-zero sizes)
template<typename T, unsigned dims>
using DummyList = List<void, dims, kListNoFlags, void, T>;

template<unsigned dims, typename T>
class List<void, dims, kListNoFlags, void, T> : public ListBase<T, dims> {
  typedef ListBase<T, dims> ListBaseType;
 public:
  typedef list_detail::DummyListIter<T> Iterator;
  typedef list_detail::DummyListIter<const T> ConstIterator;

  List() : List(cpp14::make_index_sequence<dims>()) {}
  void ShrinkToFirst() override {}

  T& get(typename ListBaseType::SizeArray&& indexes) const override {
    return *static_cast<T*>(nullptr);  // Undefined Behavior, but fast
  }

  typename View<T>::Iterator iterator_begin() const override {
    return new Iterator();
  }

  // define iterator accessors of the following form:
  //   [Const]Iterator [prefix]begin|end() const { return [Const]Iterator(); }
  // where prefix is either "" or "l", e.g. lbegin(), cend(), clbegin() etc.
#define V_THIS_DEF_ITER_ACCESSOR(type, method)  \
  type method() const { return type(); }
#define V_THIS_DEF_ITER_ACCESSORS(prefix)                       \
  V_THIS_DEF_ITER_ACCESSOR(Iterator, prefix ## begin)           \
  V_THIS_DEF_ITER_ACCESSOR(Iterator, prefix ## end)             \
  V_THIS_DEF_ITER_ACCESSOR(ConstIterator, c ## prefix ## begin) \
  V_THIS_DEF_ITER_ACCESSOR(ConstIterator, c ## prefix ## end)

  V_THIS_DEF_ITER_ACCESSORS()
  V_THIS_DEF_ITER_ACCESSORS(l)
#undef V_THIS_DEF_ITER_ACCESSOR
#undef V_THIS_DEF_ITER_ACCESSORS

  const List& values() const override { return *this; }

 private:
  template<size_t... Is>
  List(cpp14::index_sequence<Is...>)
  : ListBaseType(std::forward<decltype(Is)>(
        list_detail::ZeroSize<decltype(Is)>())...) {}
};

// specialization for 1D lists

namespace detail {

template<class P>
struct SimpleListHelper {
  template<class Container>
  static void AppendEmpty(Container* c) {
    c->Append(c->back());
    c->back().ShrinkToFirst();
  }
};

template<typename T>
struct SimpleListHelper<PortionBase<T> > {
  template<class Container>
  static void AppendEmpty(Container* c) {
    c->template AppendDerived<DummyPortion<T> >();
  }
};

}  // namespace detail

template<typename T, class P = PortionBase<T> >
using PortionVector = PolyVector<P, PortionBase<T>, PortionFactory>;

#define V_LIST_INDEXER_TYPE list_detail:: \
  LinearizedMonotonicIndexer<PortionVector<typename P::DataType, P> >

template<class P>
using SimpleList = List<P, 1, kListOpVector, V_LIST_INDEXER_TYPE>;

template<class P>
class List<P, 1, kListOpVector, V_LIST_INDEXER_TYPE>
    : public ListBase<typename P::DataType>,
      public PortionHelper<P, typename P::DataType>,
      protected detail::SimpleListHelper<P> {
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
        indexer_(AppendDummy(container_)) {

    container_.Shrink();
  }

  void ShrinkToFirst() override {
    // remove everything except the dummy portion
    ListBase<DataType>::ShrinkToFirst();
    container_.Erase(++container_.begin(), container_.end());
  }

  inline void set(const DataType& value, const size_t& index) const {
    const_cast<DataType&>(this->get(index)) = value;
  }

  inline const DataType& get(const size_t& index) const {
    auto pos = indexer_.fwd().get(index);
    return container_[pos.first].get(pos.second);
  }

  DataType& get(SizeArray&& indexes) const override {
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

  const List& values() const override { return *this; }

 protected:
  typename View<DataType>::Iterator iterator_begin() const override {
    return new ForwardIterator(this->begin());
  }

  // overrides View<DataType>::Iterator to provide O(1) time complexity
  typename View<DataType>::Iterator iterator_end() const override {
    return new ForwardIterator(this->end());
  }

 private:
  static Container& AppendDummy(Container& container) {
    // put a sentinel empty portion at the back
    if (container.size())
      detail::SimpleListHelper<P>::template AppendEmpty(&container);
    return container;
  }

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
    Iterator(const List* list, const size_t& index, Sizes&&... indexes)
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
      if (++indexes_.back() >= list_->sizes_.back()) {
        // TODO: use a constexpr function that unrolls the loop at compile time
        for (int dim = dims - 1; dim > 0; ) {
          indexes_[dim--] = 0;
          if (++indexes_[dim] < list_->sizes_[dim])
            break;
        }
      }
    }

    T& ref() const { return ref(cpp14::make_index_sequence<dims>()); }

   private:
    template<size_t... Is>
    T& ref(cpp14::index_sequence<Is...>) const {
      return (*list_)(std::get<Is>(indexes_)...);
    }

    const List* list_;
    std::array<size_t, dims> indexes_;
  };

  template<typename... Sizes>
  List(Accessor accessor,
       const typename std::conditional<1 + sizeof...(Sizes) == dims,
       size_t, Disabler>::type& size,
       Sizes&&... sizes)
      : ListBase<T, dims>(size, std::forward<Sizes>(sizes)...),
        accessor_(accessor),
        sizes_{{size, static_cast<size_t>(sizes)...}},
    offsets_{{list_detail::ZeroSize<Sizes>()...}} {}

  template<typename... Sizes>
  List(const typename std::conditional<1 + sizeof...(Sizes) == dims,
       size_t, Disabler>::type& size,
       Sizes&&... sizes)
      : List(Accessor(), size, std::forward<Sizes>(sizes)...) {}

  template<typename... Subs>
  List(Accessor accessor, const Sub& sub, Subs&&... subs)
      : ListBase<T, dims>(list_detail::SizeProduct(
          sub.second, static_cast<Sub>(subs).second...)),
        accessor_(accessor),
        sizes_{{sub.second, static_cast<Sub>(subs).second...}},
    offsets_{{sub.first, static_cast<Sub>(subs).first...}} {
      static_assert(1 + sizeof...(Subs) == dims,
                    "The number of subs does not match dims");
    }

  template<typename... Subs>
  List(const List& list, const Sub& sub, Subs&&... subs)
      : List(list.accessor_, sub, std::forward<Subs>(subs)...) {
    // TODO make the constructor trivial by expanding offset sum at compile time
    auto it = list.offsets_.cbegin();
    for (auto& offset : offsets_)
      offset += *it++;
  }

  // template<typename... Sizes>
  // List(Accessor accessor) : accessor_(accessor) {}

  // List() = default;

  template<typename... Indexes>
  T& operator()(Indexes&&... indexes) const {
    return *(list_detail::Pairwise<
             typename std::remove_reference<Indexes>::type...>::template
             Sum(accessor_, std::forward<Indexes>(indexes)..., offsets_));
  }

  template<typename... Indexes>
  const T& get(Indexes&&... indexes) const {
    return this->operator()(std::forward<Indexes>(indexes)...);
  }
  template<typename... Indexes>
  void set(const T& value, Indexes&&... indexes) const {
    this->operator()(std::forward<Indexes>(indexes)...) = value;
  }

  T& get(SizeArray&& indexes) const override {
    return this->get0(std::move(indexes), cpp14::make_index_sequence<dims>());
  }

  Accessor& accessor() { return accessor_; }
  const Accessor& accessor() const { return accessor_; }

  const SizeArray& sizes() const { return sizes_; }

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

  const DummyList<T, dims>& values() const override {
    static DummyList<T, dims> instance;
    return instance;
  }

 private:
  template<size_t... Is>
  T& get0(const SizeArray& indexes, cpp14::index_sequence<Is...>) const {
    return this->operator()(std::get<Is>(std::move(indexes))...);
  }

  template<size_t... Is>
  Iterator* begin(cpp14::index_sequence<Is...>) const {
    return new Iterator(this, list_detail::ZeroSize<Is>()...);
  }

  template<size_t I, size_t... Is>
  Iterator* end(cpp14::index_sequence<I, Is...>) const {
    return new Iterator(this, sizes_.front(), list_detail::ZeroSize<Is>()...);
  }

  Accessor accessor_;
  SizeArray sizes_;
  SizeArray offsets_;
};

template<typename T, class P>
constexpr SimpleList<P> MakeList(PortionVector<T, P>&& pv) {
  return SimpleList<P>(std::forward<PortionVector<T, P> >(pv));
}

template<class Accessor, typename... Sizes>
constexpr auto MakeList(Accessor a, const size_t& size, Sizes&&... sizes)
#define V_LIST_TYPE                                                       \
    ImplicitList<typename                                               \
                 std::remove_pointer<decltype(a(size, sizes...))>       \
                 ::type, (1 + sizeof...(Sizes)), Accessor>
    -> V_LIST_TYPE {                                            \
  return V_LIST_TYPE(a, size, std::forward<Sizes>(sizes)...); }
#undef V_LIST_TYPE

template<class Accessor, typename... Subs>
constexpr auto MakeList(Accessor a, const Sub& sub, Subs&&... subs)
#define V_LIST_TYPE                                                     \
    ImplicitList<typename                                               \
                 std::remove_pointer<decltype(a(sub.first, subs.first...))> \
                 ::type, (1 + sizeof...(Subs)), Accessor>
    -> V_LIST_TYPE { return V_LIST_TYPE(a, sub, std::forward<Subs>(subs)...); }
#undef V_LIST_TYPE

struct ListFactory {
  template<typename... Args>
  constexpr auto operator()(Args&&... args) const ->
      decltype(MakeList(std::forward<Args>(args)...)) {
    return MakeList(std::forward<Args>(args)...);
  }
};

}  // namespace v

#endif  /* CPPVIEWS_SRC_LIST_HPP_ */
