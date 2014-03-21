#ifndef CPPVIEWS_SRC_PORTION_HPP_
#define CPPVIEWS_SRC_PORTION_HPP_

#include <initializer_list>
#include <iterator>
#include <limits>
#include <type_traits>
#include <deque>

namespace v {

template<typename T>
class PortionBase {
 public:
  typedef T ValueType;

  virtual void set(size_t index, const T& value) const = 0;
  virtual const T& get(size_t index) const = 0;
  virtual size_t size() const = 0;
  virtual size_t max_size() const = 0;
};

template<typename T>
struct PortionSetHelper {
  template<class P>
  inline void Set(const P& portion, size_t index, const T& value) const {
    portion[index] = value;
  }
};

template<typename T>
struct PortionSetHelper<const T> {
  template<class P>
  inline void Set(const P& portion, size_t index, const T& value) const {}
};

namespace {

template<class InputIter>
using IsReadOnly = std::is_const<
  typename std::remove_reference<
    typename std::iterator_traits<InputIter>::reference
    >::type
  >;

}  // namespace

template<class InputIter,
         typename T = typename std::conditional<
           IsReadOnly<InputIter>::value,
           const typename std::iterator_traits<InputIter>::value_type,
           typename std::iterator_traits<InputIter>::value_type
           >::type
         >
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

  constexpr RefType operator[](DiffType index) const {
    return *std::next(begin_, index);
  }

  inline void set(size_t index, const T& value) const {
    this->Set(*this, index, value);
  }

  constexpr const T& get(size_t index) const { return operator[](index); }
  constexpr size_t size() const {
    return static_cast<decltype(size())>(size_);
  }

  constexpr size_t max_size() const {
    return std::numeric_limits<DiffType>::max();
  }

 private:
  InputIter begin_;
  DiffType size_;
};

template<class InputIter>
using ReadonlyPortion = Portion<
  InputIter, const typename std::iterator_traits<InputIter>::value_type>;

template<typename T>
class Portion<T, T> : public PortionBase<T>, protected PortionSetHelper<T> {
 public:
  constexpr Portion(T& value) : ptr_(&value) {}
  Portion(T&& value) = delete;          // disallow references to temporaries
  Portion() {}

  constexpr operator T&() const { return *ptr_; }

  // for consistency
  constexpr T& operator[](size_t index) const { return *ptr_; }

  inline void set(size_t index, const T& value) const {
    this->Set(*this, index, value);
  }

  constexpr const T& get(size_t index) const { return *ptr_; }
  constexpr size_t size() const { return static_cast<size_t>(1); }
  constexpr size_t max_size() const {
    return std::numeric_limits<size_t>::max();
  }

 private:
  T* ptr_;
};

template<typename T>
using SingletonPortion = Portion<T, T>;

template<typename T>
using ReadonlySingletonPortion = SingletonPortion<const T>;

template<typename T>
class Portion<T[], T> : public PortionBase<T>, protected PortionSetHelper<T> {
 public:
  Portion(std::initializer_list<std::reference_wrapper<T> > init)
      : ptrs_(init.size()) {
    auto ptr_it = ptrs_.begin();
    for (auto& value : init)
      *ptr_it++ = &value.get();
  }
  Portion() {}

  constexpr T& operator[](size_t index) const { return *ptrs_[index]; }

  inline void PushFront(T& value) { ptrs_.push_front(&value); }
  inline void PushBack(T& value) { ptrs_.push_back(&value); }
  inline void PopFront() { ptrs_.pop_front(); }
  inline void PopBack() { ptrs_.pop_back(); }
  inline void Shrink() { ptrs_.shrink_to_fit(); }

  inline void set(size_t index, const T& value) const {
    this->Set(*this, index, value);
  }

  constexpr const T& get(size_t index) const { return (*this)[index]; }
  constexpr size_t size() const { return static_cast<size_t>(ptrs_.size()); }

  constexpr T& front() const { return *ptrs_.front(); }
  constexpr T& back() const { return *ptrs_.back(); }
  constexpr size_t max_size() const { return ptrs_.max_size(); }

 private:
  // TODO: optimize for smaller memory footprint for small sizes
  // (because sizeof(deque) itself is 11 * sizeof(size_t), which is big!)
  std::deque<T*> ptrs_;
};

template<typename T>
using SingletonMultiportion = Portion<T[], T>;

template<typename T>
using ReadonlySingletonMultiportion = Portion<const T[], const T>;

template<class InputIter>
constexpr Portion<InputIter> MakePortion(
    InputIter begin,
    typename Portion<InputIter>::DiffType size) {
  return Portion<InputIter>(begin, size);
}

template<typename T>
constexpr SingletonMultiportion<T> MakePortion(
    std::initializer_list<std::reference_wrapper<T> > init) {
  return SingletonMultiportion<T>(init);
}

template<class InputIter>
constexpr Portion<InputIter>* AllocPortion(
    InputIter begin,
    typename Portion<InputIter>::DiffType size) {
  return new Portion<InputIter>(begin, size);
}

}  // namespace v

#endif  /* CPPVIEWS_SRC_PORTION_HPP_ */
