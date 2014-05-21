#ifndef CPPVIEWS_SRC_UTIL_ITERATOR_HPP_
#define CPPVIEWS_SRC_UTIL_ITERATOR_HPP_

#include <iterator>
#include <type_traits>

#include <cstddef>

namespace detail {

constexpr bool Or2(bool cond1, bool cond2) { return cond1 || cond2; }

template<class Iter1, class Iter2, class Return>
using EnableIfInteroperable = typename std::enable_if<
  Or2(std::is_convertible<Iter1, Iter2>::value,
      std::is_convertible<Iter2, Iter1>::value),
  Return>::type;

template<class InputIter>
struct InputIterHelper {
  bool operator==(const InputIterHelper& rhs) const { return it_ == rhs.it_; }
  bool operator!=(const InputIterHelper& rhs) const { return it_ != rhs.it_; }

  // expose this helper accessor to allow for non-const to const conversion
  const InputIter& it() const { return it_; }
 protected:
  InputIter it_;
};

template<class ForwardIter, class Derived>
struct ForwardIterHelper : public InputIterHelper<ForwardIter> {
  Derived& operator++() { ++this->it_; return static_cast<Derived&>(*this); }
  Derived operator++(int) { Derived old(this->it_); ++this->it_; return old; }
};

template<class BidirIter, class Derived>
struct BidirectionalIterHelper : public ForwardIterHelper<BidirIter, Derived> {
  Derived& operator--() { --this->it_; return static_cast<Derived&>(*this); }
  Derived operator--(int) { Derived old(this->it_); --this->it_; return old; }
};

template<class RandomIter, class Derived>
class RandomAccessIterHelper
    : public BidirectionalIterHelper<RandomIter, Derived> {
  typedef typename std::iterator_traits<RandomIter>::difference_type DiffType;
 public:
  Derived operator+(DiffType n) const { Derived it(this->it_); return it += n; }
  Derived operator-(DiffType n) const { Derived it(this->it_); return it -= n; }
  Derived& operator+=(DiffType n) {
    this->it_ += n;
    return static_cast<Derived&>(*this);
  }
  Derived& operator-=(DiffType n) {
    this->it_ -= n;
    return static_cast<Derived&>(*this);
  }
  friend Derived operator+(DiffType n, const Derived& it) { return it + n; }
  friend Derived operator-(DiffType n, const Derived& it) { return it - n; }
  bool operator<(const Derived& rhs) const { return this->it_ < rhs.it_; }
  bool operator<=(const Derived& rhs) const { return this->it_ <= rhs.it_; }
  bool operator>(const Derived& rhs) const { return this->it_ > rhs.it_; }
  bool operator>=(const Derived& rhs) const { return this->it_ >= rhs.it_; }
};

template<class InputIter>
using IterCat = typename std::iterator_traits<InputIter>::iterator_category;

template<class InputIter, class IterTag>
using HasIterCat = typename std::is_same<IterCat<InputIter>, IterTag>;

template<class InputIter, class IterTag, class True, class False>
using IfHasIterCatType = typename std::conditional<
  HasIterCat<InputIter, IterTag>::value, True, False>::type;

template<class InputIter, class Derived>
using MakeIterHelper = typename detail::IfHasIterCatType<
  InputIter, std::random_access_iterator_tag,
  detail::RandomAccessIterHelper<InputIter, Derived>,
  detail::IfHasIterCatType<
    InputIter, std::bidirectional_iterator_tag,
    detail::BidirectionalIterHelper<InputIter, Derived>,
    detail::IfHasIterCatType<
      InputIter, std::forward_iterator_tag,
      detail::ForwardIterHelper<InputIter, Derived>,
      detail::InputIterHelper<InputIter> > > >;

template<class InputIter, class Func>
using TransformedIterRef = typename std::result_of<
  Func(typename std::iterator_traits<InputIter>::reference)>::type;

template<class InputIter>
class IndirectIterTransformer {
  typedef typename std::iterator_traits<InputIter>::value_type Pointer;
  typedef typename std::remove_pointer<Pointer>::type Pointee;
public:
  Pointee& operator()(Pointer ptr) const { return *ptr; }
};

}  // namespace detail

template<class Iter>
using IsConstIter = std::is_const<
  typename std::remove_reference<
    typename std::iterator_traits<Iter>::reference
    >::type
  >;

template<class FromValue, class ToValue, class Return>
using EnableIfIterConvertible = typename std::enable_if<
  std::is_convertible<FromValue*, ToValue*>::value, Return>::type;

template<class InputIter, class Func, class NonConstInputIter = InputIter>
#define V_THIS_REF_TYPE detail::TransformedIterRef<InputIter, Func>
class TransformedIterator : public std::iterator<
  typename std::iterator_traits<InputIter>::iterator_category,
  typename std::iterator_traits<InputIter>::value_type,
  typename std::iterator_traits<InputIter>::difference_type,
  typename std::add_pointer<V_THIS_REF_TYPE>::type,
  V_THIS_REF_TYPE
  >, public detail::MakeIterHelper<InputIter,
                                   TransformedIterator<InputIter, Func> > {
#undef V_THIS_REF_TYPE
  typedef typename std::iterator_traits<TransformedIterator> Traits;
 public:
  TransformedIterator() = default;
  TransformedIterator(InputIter it) { this->it_ = it; }

  // it's legit (and more generic) to allow const to non-const conversion
  // from iterators with different transformer functions
  TransformedIterator(const detail::InputIterHelper<NonConstInputIter>& h)
      : TransformedIterator(h.it()) {}

  typename Traits::reference operator*() const { return func_(*this->it_); }
  typename Traits::pointer operator->() const { return &this->operator*(); }
 private:
  // hide the helper accessor used for non-const to const conversion
  const InputIter& it() const { return this->it_; }

  Func func_;
};

template<class InputIter, class NonConstInputIter = InputIter>
using IndirectIterator = TransformedIterator<
  InputIter,
  detail::IndirectIterTransformer<InputIter>,
  NonConstInputIter>;

// Adapter which adapts the following simplified interface:
// V& ref() const;  // dereferences the iterator
// for forward iterators:
//   void Increment();
//   template<class...> IsEqual(const Iterator<...>& other) const;
// for bidirectional iterators:
//   void Decrement();
// for random-access iterators:
//   void Advance(Distance n);
//   template<class...> Distance DistanceTo(const Iterator<...>& other) const;
//
// Typical usage is like this:
// template<class T>
// class Iterator : public DefaultIterator<Iterator,
//                                         std::forward_iterator_tag,
//                                         T> {
//   V_DEFAULT_ITERATOR_DERIVED_HEAD(Iterator);
//  public:
//   Iterator(/* main ctor */);
//   Iterator(const Iterator<T2>& other,
//            EnableIfIterConvertible<T2, T, Enabler> = Enabler()) const;
//            // for convenience, V_DEFAULT_ITERATOR_DISABLE_NONCONVERTIBLE(T2)
//            // can be put as a second parameter, but it's less readable
//  protected:
//   void Increment();
//   template<class T2>
//   bool IsEqual(const Iterator<T2>& other) const;
//   typename DefaultIterator_::reference ref() const;
//   ...
// };
template<class Derived,
         class Category,
         class V,
         class Distance = std::ptrdiff_t>
struct DefaultIterator : public std::iterator<Category, V, Distance> {

#define V_DEFAULT_ITERATOR_DERIVED_HEAD(Iter)                           \
  typedef typename Iter::DefaultIterator_ DefaultIterator_;             \
  typedef typename DefaultIterator_::Enabler Enabler;                   \
  template<class, class, class, class> friend class DefaultIterator;

#define V_DEFAULT_ITERATOR_DISABLE_NONCONVERTIBLE(FromValue)            \
  typename DefaultIterator_::template                                   \
  EnablerIfConvertibleFrom<T2>::type = typename DefaultIterator_::Enabler()

 public:
  typedef DefaultIterator<Derived, Category, V> DefaultIterator_;

  V& operator*() const {
    return this->derived().ref();
  }
  V* operator->() const {
    return &this->operator*();
  }

  Derived& operator++() {
    this->derived().Increment();
    return this->derived();
  }
  Derived operator++(int) {
    Derived old(this->derived());
    ++this->derived();
    return old;
  }

  template<class Derived2>
  detail::EnableIfInteroperable<Derived, Derived2, bool>
  operator==(const Derived2& rhs) const {
    return this->derived().IsEqual(rhs);
  }

  template<class Derived2>
  detail::EnableIfInteroperable<Derived, Derived2, bool>
  operator!=(const Derived2& rhs) const {
    return !this->derived().IsEqual(rhs);
  }

 protected:
  struct Enabler {};
  template<typename T2>
  using EnablerIfConvertibleFrom = typename std::enable_if<
    std::is_convertible<T2*, V*>::value,
    Enabler>;

  Derived& derived() { return *static_cast<Derived*>(this); }
  const Derived& derived() const { return *static_cast<const Derived*>(this); }
};

template<class Derived, class V, typename Distance>
class DefaultIterator<Derived, std::bidirectional_iterator_tag, V, Distance>
    : public DefaultIterator<Derived, std::forward_iterator_tag, V, Distance> {
 public:
  Derived& operator--() {
    this->derived().Decrement();
    return this->derived();
  }
  Derived operator--(int) {
    Derived old(this->derived());
    --this->derived();
    return old;
  }
};

template<class Derived, class V, typename Distance>
class DefaultIterator<Derived, std::random_access_iterator_tag, V, Distance>
    : public DefaultIterator<Derived, std::bidirectional_iterator_tag,
                             V, Distance> {
 public:
  Derived& operator+=(Distance n) {
    this->derived().Advance(n);
    return this->derived();
  }
  Derived& operator-=(Distance n) { return *this += -n; }
  Derived operator+(Distance n) const {
    Derived it(this->derived());
    it.Advance(n);
    return it;
  }
  Derived operator-(Distance n) const { return *this + -n; }

  template<class Derived2>
  detail::EnableIfInteroperable<Derived, Derived2, Distance>
  operator-(const Derived2& rhs) const {
    return rhs.DistanceTo(this->derived());
  }

  // define relations <, <=, >, >=
#define V_DEF_ITERATOR_INTEROP_RELATION(rel)                          \
  template<class Derived2>                                            \
  detail::EnableIfInteroperable<Derived, Derived2, bool>              \
  operator rel(const Derived2& rhs) const {                           \
    return 0 rel this->derived().DistanceTo(rhs);                     \
  }

  V_DEF_ITERATOR_INTEROP_RELATION(<);
  V_DEF_ITERATOR_INTEROP_RELATION(<=);
  V_DEF_ITERATOR_INTEROP_RELATION(>);
  V_DEF_ITERATOR_INTEROP_RELATION(>=);
#undef V_DEF_ITERATOR_INTEROP_RELATION

  friend Derived operator+(Distance n, const Derived& rhs) { return rhs + n; }
  friend Derived operator-(Distance n, const Derived& rhs) { return rhs - n; }

 protected:
  // provide default implementation of Increment()/Decrement() using Advance()
  // the derived class can, however, override them with a more efficient impl
  void Increment() { this->derived().Advance(1); }
  void Decrement() { this->derived().Advance(-1); }

  template<class, class, class, class> friend class DefaultIterator;
};

#endif  /* CPPVIEWS_SRC_UTIL_ITERATOR_HPP_ */
