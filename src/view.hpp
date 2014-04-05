#ifndef CPPVIEWS_SRC_VIEW_HPP_
#define CPPVIEWS_SRC_VIEW_HPP_

#include <cstddef>

namespace v {

// TODO:

template<typename T>
class View {
 public:
  View(size_t size) : size_(size) {}
  View() = default;

  size_t size() const { return size_; }
  virtual size_t max_size() const = 0;

 protected:
  size_t size_;
};


}  // namespace v

#endif  /* CPPVIEWS_SRC_VIEW_HPP_ */
