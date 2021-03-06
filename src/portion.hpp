#include <iostream>
#ifndef CPPVIEWS_SRC_PORTION_HPP_
#define CPPVIEWS_SRC_PORTION_HPP_

#include "util/iterator.hpp"

#include <deque>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <type_traits>

namespace v {

namespace detail {

template<typename T>
class PortionBaseIter;

}  // namespace detail

template<typename T>
class PortionBase {
 public:
  typedef T DataType;  // used by List
  typedef detail::PortionBaseIter<T> Iterator;

  static constexpr unsigned kDims = 1;  // used by List

  virtual ~PortionBase() noexcept = default;

  virtual void ShrinkToFirst() = 0;

  virtual T& get(size_t index) const = 0;
  virtual size_t size() const = 0;
  virtual size_t max_size() const = 0;

  size_t dim_size(unsigned dim) const { return size(); }  // used by List

  Iterator begin() const { return Iterator(*this, 0); }
  Iterator end() const { return Iterator(*this, size()); }
};

template<class ForwardIter,
         typename T = typename std::conditional<
           IsConstIter<ForwardIter>::value,
           const typename std::iterator_traits<ForwardIter>::value_type,
           typename std::iterator_traits<ForwardIter>::value_type
           >::type
         >
class Portion : public PortionBase<T> {
 public:
  typedef ForwardIter Iterator;
  // TODO: support ConstIter (requires major refactoring)
  typedef typename std::iterator_traits<ForwardIter>::difference_type DiffType;
  typedef typename std::iterator_traits<ForwardIter>::reference RefType;

  constexpr Portion(const ForwardIter& begin, DiffType size)
      : begin_(begin),
        size_(size) {}
  constexpr Portion(DiffType size, const ForwardIter& end)
      : begin_(std::next(end, -size)),
        size_(size) {}
  Portion() = default;

  void ShrinkToFirst() override { size_ = 1; }

  constexpr RefType operator[](DiffType index) const {
    return *std::next(begin_, index);
  }

  constexpr T& get(size_t index) const override {
    return operator[](index);
  }
  constexpr size_t size() const override { return size_; }

  constexpr size_t max_size() const override {
    return std::numeric_limits<DiffType>::max();
  }

  Iterator begin() const { return begin_; }
  Iterator end() const { return std::next(begin_, size_); }

 private:
  ForwardIter begin_;
  DiffType size_;
};

template<class ForwardIter>
using ReadonlyPortion = Portion<
  ForwardIter, const typename std::iterator_traits<ForwardIter>::value_type>;

// specialization for singleton portion

template<typename T>
using SingletonPortion = Portion<T, T>;

template<typename T>
using ReadonlySingletonPortion = SingletonPortion<const T>;

namespace detail {

template<typename T>
class SingletonPortionIter;

template<typename T>
class SingletonMultiportionIter;

}  // namespace detail

template<typename T>
class Portion<T, T> : public PortionBase<T> {
 public:
  typedef detail::SingletonPortionIter<T> Iterator;
  typedef detail::SingletonPortionIter<const T> ConstIterator;

  constexpr Portion(T& value) : ptr_(&value) {}
  Portion(T&& value) = delete;          // disallow references to temporaries
  Portion() = default;

  void ShrinkToFirst() override {}

  constexpr operator T&() const { return *ptr_; }

  // for consistency
  constexpr T& operator[](size_t) const { return *ptr_; }

  constexpr T& get(size_t index) const override { return *ptr_; }
  constexpr size_t size() const override { return this->max_size(); }
  constexpr size_t max_size() const override { return 1; }

  Iterator begin() const { return Iterator(ptr_, false); }
  Iterator end() const { return Iterator(ptr_, true); }
  ConstIterator cbegin() const { return ConstIterator(ptr_, false); }
  ConstIterator cend() const { return ConstIterator(ptr_, true); }

 private:
  T* ptr_;
};

template<typename T>
using SingletonMultiportion = Portion<T[], T>;

template<typename T>
using ReadonlySingletonMultiportion = Portion<const T[], const T>;

template<typename T>
class Portion<T[], T> : public PortionBase<T> {
  typedef std::deque<T*> PointerDeque;
 public:
  typedef IndirectIterator<typename PointerDeque::iterator> Iterator;
  typedef IndirectIterator<typename PointerDeque::const_iterator,
                           typename PointerDeque::iterator> ConstIterator;

  Portion(std::initializer_list<std::reference_wrapper<T> > init)
      : ptrs_(init.size()) {
    auto ptr_it = ptrs_.begin();
    for (auto& value : init)
      *ptr_it++ = &value.get();
  }
  Portion() = default;

  void ShrinkToFirst() {
    ptrs_.resize(1);
    ptrs_.shrink_to_fit();
  }

  constexpr T& operator[](size_t index) const { return *ptrs_[index]; }

  void PushFront(T& value) { ptrs_.push_front(&value); }
  void PushBack(T& value) { ptrs_.push_back(&value); }
  void PopFront() { ptrs_.pop_front(); }
  void PopBack() { ptrs_.pop_back(); }
  void ShrinkToFit() { ptrs_.shrink_to_fit(); }

  constexpr T& get(size_t index) const override { return (*this)[index]; }
  constexpr size_t size() const override { return ptrs_.size(); }

  constexpr T& front() const { return *ptrs_.front(); }
  constexpr T& back() const { return *ptrs_.back(); }
  constexpr size_t max_size() const { return ptrs_.max_size(); }

  Iterator begin() { // XXX should be const, but need non-const deque iter
    return Iterator(ptrs_.begin());
  }

  Iterator end() { // XXX should be const, but need non-const deque iter
    return Iterator(ptrs_.end());
  }
  ConstIterator cbegin() const { return ConstIterator(ptrs_.cbegin()); }
  ConstIterator cend() const { return ConstIterator(ptrs_.cend()); }

 private:
  // TODO: optimize for smaller memory footprint for small sizes
  // (because sizeof(deque) itself is 11 * sizeof(size_t), which is big!)
  PointerDeque ptrs_;
};

// specialization for dummy portion

template<typename T>
using DummyPortion = Portion<void, T>;

template<typename T>
class Portion<void, T> : public PortionBase<T> {
 public:
  Portion(const Portion&) = delete;
  Portion(Portion&&) = delete;
  Portion() = default;

  void* operator new(size_t sz) noexcept { return instance(); }
  void operator delete(void* ptr) noexcept {
    // XXX: is this thread-safe?
    // delete calls destructor, so we need to re-construct the object
    ::new(ptr) Portion;  // TODO: double-check that it's not Undefined Behavior
  }

  void ShrinkToFirst() override {}

  // get() should never be called on a dummy portion
  T& get(size_t index) const override {
    return *static_cast<T*>(nullptr);  // Undefined Behavior, but fast
  }

  size_t size() const override { return 1; }
  size_t max_size() const override { return 0; }

  static Portion* instance() noexcept {
    static Portion kInstance;
    return &kInstance;
  }
};

template<class ForwardIter>
constexpr Portion<ForwardIter> MakePortion(
    ForwardIter begin,
    typename Portion<ForwardIter>::DiffType size) {
  return Portion<ForwardIter>(begin, size);
}

template<class ForwardIter, typename T>
constexpr Portion<ForwardIter, T>
MakePortion(const Portion<ForwardIter, T>& other) {
  return Portion<ForwardIter, T>(other);
}

template<typename T>
constexpr SingletonMultiportion<T> MakePortion(
    std::initializer_list<std::reference_wrapper<T> > init) {
  return SingletonMultiportion<T>(init);
}

template<typename T>
constexpr SingletonPortion<T> MakePortion(T& value) {
  return SingletonPortion<T>(value);
}

template<typename T>
constexpr DummyPortion<T> MakePortion() {
  return DummyPortion<T>();
}

template<class ForwardIter>
constexpr Portion<ForwardIter>* AllocPortion(
    ForwardIter begin,
    typename Portion<ForwardIter>::DiffType size) {
  return new Portion<ForwardIter>(begin, size);
}

template<typename T>
constexpr SingletonMultiportion<T>* AllocPortion(
    std::initializer_list<std::reference_wrapper<T> > init) {
  return new SingletonMultiportion<T>(init);
}

template<typename T>
constexpr SingletonPortion<T>* AllocPortion(T& value) {
  return SingletonPortion<T>(value);
}

template<typename T>
inline DummyPortion<T>* AllocPortion() {
  return DummyPortion<T>::instance();
}

struct PortionFactory {
  template<typename... Args>
  constexpr auto operator()(Args&&... args) const ->
      decltype(MakePortion(std::forward<Args>(args)...)) {
    return MakePortion(std::forward<Args>(args)...);
  }
};

struct PortionAllocator {
  template<typename... Args>
  constexpr auto operator()(Args&&... args) const ->
      decltype(AllocPortion(std::forward<Args>(args)...)) {
    return AllocPortion(std::forward<Args>(args)...);
  }
};

namespace detail {

template<typename T>
class PortionBaseIter
    : public DefaultIterator<PortionBaseIter<T>,
                             std::random_access_iterator_tag,
                             T> {
  V_DEFAULT_ITERATOR_DERIVED_HEAD(PortionBaseIter);
  template<class, class, class, class> friend class DefaultIterator;

 public:
  explicit PortionBaseIter(const PortionBase<T>& p, size_t index)
      : p_(&p),
        index_(index) {}
  template<typename T2>
  PortionBaseIter(const PortionBaseIter<T2>& other,
                  EnableIfIterConvertible<T2, T,
                  typename PortionBaseIter::Enabler> =
                  typename PortionBaseIter::Enabler())
      : p_(other.p_),
        index_(other.index_) {}
  PortionBaseIter() = default;

 protected:
  void Increment() { ++index_; }
  void Decrement() { --index_; }
  void Advance(typename PortionBaseIter::difference_type distance) {
    index_ += distance;
  }
  template<typename T2>
  typename PortionBaseIter::difference_type DistanceTo(
      const PortionBaseIter<T2>& it) const {
    return it.index_ - index_;
  }
  template<typename T2>
  bool IsEqual(const PortionBaseIter<T2>& other) const {
    return index_ == other.index_;
  }
  typename PortionBaseIter::reference ref() const { return p_->get(index_); }

 private:
  const PortionBase<T>* p_;
  size_t index_;
};

template<typename T>
class SingletonPortionIter
    : public DefaultIterator<SingletonPortionIter<T>,
                             std::random_access_iterator_tag,
                             T> {
  V_DEFAULT_ITERATOR_DERIVED_HEAD(SingletonPortionIter);
 public:
  explicit SingletonPortionIter(T* datum, bool end)
      : datum_(datum),
        end_(end) {}
  template<typename T2>
  SingletonPortionIter(const SingletonPortionIter<T2>& other,
                       EnableIfIterConvertible<T2, T,
                       typename SingletonPortionIter::Enabler> =
                       typename SingletonPortionIter::Enabler())
      : datum_(other.datum_),
        end_(other.end_) {}
  SingletonPortionIter() = default;

 protected:
  void Increment() { end_ = true; }
  void Decrement() { end_ = false; }
  void Advance(typename SingletonPortionIter::difference_type distance) {
    // avoid branching by using bitwise operators
    end_ = (end_ & (distance >= 0)) | (distance > 0);
  }
  template<typename T2>
  typename SingletonPortionIter::difference_type DistanceTo(
      const SingletonPortionIter<T2>& it) const {
    return it.end_ - end_;
  }
  template<typename T2>
  bool IsEqual(const SingletonPortionIter<T2>& other) const {
    return end_ == other.end_;
  }
  typename SingletonPortionIter::reference ref() const { return *datum_; }

 private:
  template<typename> friend class SingletonPortionIter;
  T* datum_;
  bool end_;
};

}  // namespace detail

}  // namespace v

#endif  /* CPPVIEWS_SRC_PORTION_HPP_ */
