#ifndef CPPVIEWS_BENCH_SM_SCALAVIEW_FACADE_HPP_
#define CPPVIEWS_BENCH_SM_SCALAVIEW_FACADE_HPP_

#include "../../smv_facade.hpp"

#include <cstdint>

// tag class which holds static information about the view
template<typename T, uint32_t row_cnt_, uint32_t col_cnt_, class Derived>
struct Scalaview {};

template<typename T, uint32_t row_cnt_, uint32_t col_cnt_, class Derived>
class SmvFacade<Scalaview<T, row_cnt_, col_cnt_, Derived>>
#define V_THIS_BASE_TYPE detail::SmvFacadeHelper<T, uint32_t>
    : public V_THIS_BASE_TYPE {
  typedef V_THIS_BASE_TYPE Helper;
#undef V_THIS_BASE_TYPE
 public:
  using typename Helper::DataType;
  using typename Helper::CoordType;

  struct ValuesView {
    struct Iterator
        : public std::iterator<std::forward_iterator_tag, DataType> {
      DataType& operator*() const { return *ZeroPtr<T>(); }  // TODO: implement
      DataType* operator->() const { return &this->operator*(); }
      bool operator==(const Iterator& other) const { return true; } // TODO:
      bool operator!=(const Iterator& other) const { return !(*this == other); }
      Iterator& operator++() { return *this; }  // TODO: implement
      Iterator operator++(int) { Iterator old(*this); ++*this; return old; }
    };
    Iterator begin() const { return Iterator(); }  // TODO: implement
    Iterator end() const { return Iterator(); }    // TODO: implement
    size_t size() const { return 0; }              // TODO: implement
  } fake_values_;  // TODO: make non-empty

  void VecMult(const VecInfo<DataType, CoordType>& vi,
               std::vector<DataType>* res) const {
    DataType* v = new DataType[col_cnt_]();  // zero-initialize
    for (const auto& e : vi)
      v[e.first] = e.second;
    auto r = derived().CVecMult(v);  // no overhead since injected via CRTP
    res->assign(r, r + row_cnt_);
  }

  DataType& operator()(const CoordType& row, const CoordType& col) {
    return *ZeroPtr<T>();  // TODO: implement
  }
  DataType& operator()(const CoordType& row, const CoordType& col) const {
    return *ZeroPtr<T>();  // TODO: implement
  }

  const ValuesView& values() const { return fake_values_; }  // TODO: implement

 protected:
  SmvFacade() : Helper(ZeroPtr<T>(), row_cnt_, col_cnt_) {}
  const Derived& derived() const { return *static_cast<const Derived*>(this); }
};

#endif  /* CPPVIEWS_BENCH_SM_SCALAVIEW_FACADE_HPP_ */
