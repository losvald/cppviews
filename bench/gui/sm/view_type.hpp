#ifndef CPPVIEWS_BENCH_GUI_SM_VIEW_TYPE_HPP_
#define CPPVIEWS_BENCH_GUI_SM_VIEW_TYPE_HPP_

#include <array>
#include <limits>

enum ViewType : unsigned char {
  /**/kViewTypeChain = 0,
      kViewTypeMono,
      kViewTypeFull,
      kViewTypeSparse,
      kViewTypeDiag,
      kViewTypeImpl,
      kViewTypeMax_ = kViewTypeImpl
};

namespace std {

template<>
struct numeric_limits<ViewType> {
  static constexpr int max() { return kViewTypeMax_; }
};

}  // namespace std

template<typename T>
using ViewTypeSparseArray = std::array<T,
                                       std::numeric_limits<ViewType>::max() + 1
                                       >;

template<unsigned char view_type>
class ViewParams {
 public:
  enum Dir : unsigned char { kRightDown, kLeftDown, kLeftUp, kRightUp };
};

template<>
class ViewParams<kViewTypeChain> {
 public:
  enum Dir : unsigned char { kRight, kDown };
};

#endif  /* CPPVIEWS_BENCH_GUI_SM_VIEW_TYPE_HPP_ */
