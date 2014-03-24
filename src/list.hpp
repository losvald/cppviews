#ifndef CPPVIEWS_SRC_LIST_HPP_
#define CPPVIEWS_SRC_LIST_HPP_

#include "portion.hpp"
#include "portion_helper.hpp"
#include "view.hpp"
#include "util/poly_vector.hpp"

#include <vector>

namespace v {

template<typename T, class P = PortionBase<T> >
using PortionVector = PolyVector<P, PortionBase<T>, PortionFactory>;

namespace {

template<class PolyContainer,
         typename T = typename PolyContainer::Pointer::element_type::ValueType>
struct LinearIndexer {
  const T& operator()(const PolyContainer& portions, size_t index) const {
    size_t cum_size = 0, i_to = portions.size();
    for (size_t i = 0; i < i_to; ++i) {
      const auto& p = portions[i];
      if ((cum_size += p.size()) > index)
        return p.get(index - (cum_size - p.size()));
    }
    return portions[i_to - 1].get(portions[i_to - 1].size()); // not found
  }
};

}  // namespace

template<typename T, class P = PortionBase<T> >
class List : public View<T>, protected PortionHelper<P, T> {
 public:
  typedef typename PortionVector<T, P>::Pointer PortionPointer;

  List() {}
  List(PortionVector<T, P>&& pv)
      : portions_(std::forward<PortionVector<T, P> >(pv)) {
    portions_.Shrink();
  }

  inline void set(size_t index, const T& value) const {
    const_cast<T&>(this->get(index)) = value;
  }

  inline const T& get(size_t index) const {
    return indexer_(portions_, index);
  }

  size_t max_size() const { return portions_.max_size(); }

 protected:
  PortionVector<T, P> portions_;
  LinearIndexer<decltype(portions_)> indexer_;

 private:
  // T& get_linear(size_t index) const {
  //   size_t cum_size = 0, i_to = portions_.size();
  //   for (size_t i = 0; i < i_to; ++i) {
  //     const P& p = portions_[i];
  //     if ((cum_size += p.size()) > index)
  //       return p.get(index - (cum_size - p.size()));
  //   }
  //   return portions_[i_to - 1].get(portions_[i_to - 1].size());
  // }
};

template<typename T, class P>
constexpr List<T, P> MakeList(PortionVector<T, P>&& pv) {
  return List<T, P>(std::forward<PortionVector<T, P> >(pv));
}

}  // namespace v

#endif  /* CPPVIEWS_SRC_LIST_HPP_ */
