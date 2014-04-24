#ifndef CPPVIEWS_SRC_VIEW_HPP_
#define CPPVIEWS_SRC_VIEW_HPP_

#include <iterator>
#include <memory>
#include <numeric>

#include <cstddef>

namespace v {

template<typename T>
class ViewIterator {
  // TODO: Make it inner protected and change the access modifiers to protected
  // (for now, subclasses restrict them to private)
 public:
  virtual ViewIterator* Clone() const = 0;
  virtual void Increment() = 0;
  virtual T& ref() const = 0;
};

template<typename T>
class View {
 public:
  typedef T DataType;

  // C++ operators cannot be virtual, so to avoid boilerplate code duplication,
  // we define this macro that defines those operators using virtual functions
  // (the subclasses should pass "this" to 2nd arg unless they are decorators)
#define V_DEF_VIEW_ITER_METHODS(Iter, it)                               \
  protected:                                                            \
  ViewIterator<T>* Clone() const {                                      \
    return new Iter(*this);                                             \
  } public:                                                             \
  typename std::iterator_traits<Iter>::reference operator*() const {    \
    return it->ref();                                                   \
  }                                                                     \
  typename std::iterator_traits<Iter>::pointer operator->() const {     \
    return &it->ref();                                                  \
  }                                                                     \
  Iter& operator++() { it->Increment(); return *this; }                 \
  Iter operator++(int) { Iter old(*this); it->Increment(); return old; }

  class Iterator : public std::iterator<std::forward_iterator_tag, T> {
   public:
    Iterator(ViewIterator<T>*&& it)
        : it_(std::forward<ViewIterator<T>*&&>(it)) {}
    Iterator(const Iterator& copy) : it_(copy.it_->Clone()) {}
    V_DEF_VIEW_ITER_METHODS(Iterator, it_);
   private:
    std::unique_ptr<ViewIterator<T> > it_;
  };

  View(size_t size) : size_(size) {}
  View() = default;

  size_t size() const { return size_; }
  virtual size_t max_size() const { return std::numeric_limits<size_t>::max(); }

  virtual Iterator begin() const = 0;
  virtual Iterator end() const {
    Iterator it(begin());
    std::advance(it, size_);
    return it;
  }

 protected:
  size_t size_;
};

}  // namespace v

#endif  /* CPPVIEWS_SRC_VIEW_HPP_ */
