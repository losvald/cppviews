#ifndef CPPVIEWS_SRC_PORTION_HPP_
#define CPPVIEWS_SRC_PORTION_HPP_

#include <iterator>
#include <type_traits>

namespace v {

template<typename T>
class PortionBase {
 public:
  typedef T ValueType;

  virtual void set(size_t index, const T& value) = 0;
  virtual const T& get(size_t index) const = 0;
  virtual size_t size() const = 0;
};

template<typename T>
struct PortionSetHelper {
  template<class P>
  inline void Set(const P& portion, size_t index, const T& value) {
    portion[index] = value;
  }
};

template<typename T>
struct PortionSetHelper<const T> {
  template<class P>
  inline void Set(const P& portion, size_t index, const T& value) {}
};

template<class InputIter,
         typename T = typename std::iterator_traits<InputIter>::value_type>
class Portion : public PortionBase<T>, protected PortionSetHelper<T> {
 public:
  typedef InputIter Iterator;
  typedef typename std::iterator_traits<InputIter>::difference_type DiffType;
  typedef typename std::iterator_traits<InputIter>::reference RefType;

  constexpr Portion(const InputIter& begin, DiffType size)
      : begin_(begin),
        size_(size) {}
  constexpr Portion(DiffType size, const InputIter& end)
      : begin_(std::next(end, -size)),
        size_(size) {}

  Portion() {}

  constexpr const RefType operator[](DiffType index) const {
    return *std::next(begin_, index);
  }

  inline RefType operator[](DiffType index) {
    return *std::next(begin_, index);
  }

  inline void set(size_t index, const T& value) {
    this->Set(*this, index, value);
  }

  constexpr const T& get(size_t index) const { return operator[](index); }
  constexpr size_t size() const {
    return static_cast<decltype(size())>(size_);
  }

 private:
  InputIter begin_;
  DiffType size_;
};

template<class InputIter>
using ReadonlyPortion = Portion<
  InputIter, const typename std::iterator_traits<InputIter>::value_type>;

namespace {

template<class InputIter>
using IsReadOnly = std::is_const<
  typename std::remove_reference<
    typename std::iterator_traits<InputIter>::reference
    >::type
  >;

}  // namespace

template<class InputIter>
typename std::enable_if<
  !IsReadOnly<InputIter>::value,
  Portion<InputIter> >::type constexpr MakePortion(
      InputIter begin,
      typename Portion<InputIter>::DiffType size) {
  return Portion<InputIter>(begin, size);
}

template<class InputIter>
typename std::enable_if<
  IsReadOnly<InputIter>::value,
  ReadonlyPortion<InputIter> >::type constexpr MakePortion(
      InputIter begin,
      typename ReadonlyPortion<InputIter>::DiffType size) {
  return ReadonlyPortion<InputIter>(begin, size);
}

template<class InputIter>
typename std::enable_if<
  !IsReadOnly<InputIter>::value,
  Portion<InputIter>* >::type constexpr AllocPortion(
      InputIter begin,
      typename Portion<InputIter>::DiffType size) {
  return new Portion<InputIter>(begin, size);
}

template<class InputIter>
typename std::enable_if<
  IsReadOnly<InputIter>::value,
  ReadonlyPortion<InputIter>* >::type constexpr AllocPortion(
      InputIter begin,
      typename ReadonlyPortion<InputIter>::DiffType size) {
  return new ReadonlyPortion<InputIter>(begin, size);
}

}  // namespace v

#endif  /* CPPVIEWS_SRC_PORTION_HPP_ */
