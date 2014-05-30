#ifndef CPPVIEWS_SRC_VIEW_HPP_
#define CPPVIEWS_SRC_VIEW_HPP_

#include <iterator>
#include <memory>
#include <numeric>

#include <cstddef>

namespace v {

template<typename T>
class View {
 public:
  typedef T DataType;

  class Iterator;

  class IteratorBase {
   protected:
    typedef bool (*IsEqualPointer)(const IteratorBase& lhs,
                                   const IteratorBase& rhs);

    virtual IteratorBase* Clone() const = 0;
    virtual void Increment() = 0;
    virtual T& ref() const = 0;
    virtual IsEqualPointer equal() const = 0;

    // operators cannot be virtual, so to avoid boilerplate code duplication,
    // we define this macro that defines those operators using virtual functions
    // (the subclasses should pass "this" to 2nd arg unless they are decorators)
#define V_DEF_VIEW_ITER_OPERATORS(Iter, it)                             \
    typename std::iterator_traits<Iter>::reference operator*() const {  \
      return it->ref();                                                 \
    }                                                                   \
    typename std::iterator_traits<Iter>::pointer operator->() const {   \
      return &it->ref();                                                \
    }                                                                   \
    Iter& operator++() { it->Increment(); return *this; }               \
    Iter operator++(int) { Iter old(*this); it->Increment(); return old; } \
    bool operator!=(const Iter& rhs) const { return !(*this == rhs); }

#define V_DEF_VIEW_ITER_IS_EQUAL(T, Iter)                               \
  typename View<T>::IteratorBase::IsEqualPointer equal() const override { \
      return &IsEqual;                                                  \
  }                                                                     \
  static bool IsEqual(const typename View<T>::IteratorBase& lhs,        \
                      const typename View<T>::IteratorBase& rhs) {      \
    return static_cast<const Iter&>(lhs) == static_cast<const Iter&>(rhs); \
  }                                                                     \
  Iter* Clone() const override { return new Iter(*this); }

    friend class Iterator;
  };  // class IteratorBase

  class Iterator : public std::iterator<std::forward_iterator_tag, T> {
   public:
    Iterator(IteratorBase*&& it) : it_(std::forward<IteratorBase*&&>(it)) {}
    Iterator(const Iterator& copy) : it_(copy.it_->Clone()) {}
    Iterator(Iterator&& other) = default;
    Iterator& operator=(const Iterator& rhs) = default;
    Iterator& operator=(Iterator&& rhs) = default;
    bool operator==(const Iterator& rhs) const {
      return it_->equal() == rhs.it_->equal() && it_->equal()(*it_, *rhs.it_);
    }
    V_DEF_VIEW_ITER_OPERATORS(Iterator, it_);

   private:
    std::unique_ptr<IteratorBase> it_;
  };

  View(size_t size) : size_(size) {}
  View() = default;

  size_t size() const { return size_; }
  virtual size_t max_size() const { return std::numeric_limits<size_t>::max(); }

  Iterator begin() const { return iterator_begin(); }
  Iterator end() const { return iterator_end(); }

 protected:
  virtual Iterator iterator_begin() const = 0;
  virtual Iterator iterator_end() const {
    return std::next(this->iterator_begin(), this->size_);
  }

  size_t size_;
};

}  // namespace v

#endif  /* CPPVIEWS_SRC_VIEW_HPP_ */
