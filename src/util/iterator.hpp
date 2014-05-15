#ifndef CPPVIEWS_SRC_UTIL_ITERATOR_HPP_
#define CPPVIEWS_SRC_UTIL_ITERATOR_HPP_

#include <iterator>
#include <type_traits>

namespace detail {

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

template<class InputIter, class Func, class NonConstInputIter = InputIter>
#define V_THIS_TYPE TransformedIterator<InputIter, Func>
#define V_THIS_REF_TYPE detail::TransformedIterRef<InputIter, Func>
class TransformedIterator : public std::iterator<
  typename std::iterator_traits<InputIter>::iterator_category,
  typename std::iterator_traits<InputIter>::value_type,
  typename std::iterator_traits<InputIter>::difference_type,
  typename std::add_pointer<V_THIS_REF_TYPE>::type,
  V_THIS_REF_TYPE
  >, public detail::IfHasIterCatType<
  InputIter, std::random_access_iterator_tag,
  detail::RandomAccessIterHelper<InputIter, V_THIS_TYPE>,
  detail::IfHasIterCatType<
    InputIter, std::bidirectional_iterator_tag,
    detail::BidirectionalIterHelper<InputIter, V_THIS_TYPE>,
    detail::IfHasIterCatType<
      InputIter, std::forward_iterator_tag,
      detail::ForwardIterHelper<InputIter, V_THIS_TYPE>,
      detail::InputIterHelper<InputIter> > >
  > {
#undef V_THIS_REF_TYPE
#undef V_THIS_TYPE
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

#endif  /* CPPVIEWS_SRC_UTIL_ITERATOR_HPP_ */
