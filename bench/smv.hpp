#ifndef CPPVIEWS_BENCH_SMV_HPP_
#define CPPVIEWS_BENCH_SMV_HPP_

#include "../src/list.hpp"

typedef unsigned DefaultCoord;

namespace detail {

template<typename T, T value, typename Coord = DefaultCoord>
struct SameValueAccessor {
  const T* operator()(Coord row, Coord col) const {
    static const T kValue = value;
    return &kValue;
  }
};

}  // namespace detail

template<typename T, class Accessor>
using ImplicitMatrix = v::ImplicitList<const T, 2, Accessor>;

template<typename T, T value, typename Coord = DefaultCoord>
using SameValuesMatrix = ImplicitMatrix<
  T,
  detail::SameValueAccessor<T, value, Coord> >;

template<typename T>
constexpr T Zero() { return T(); }

template<typename T>
using AllZeroesMatrix = SameValuesMatrix<T, Zero<T>()>;

template<typename T>
using AllOnesMatrix = SameValuesMatrix<T, T() + 1>;

template<class List>
size_t RowCount(const List& smv) { return smv.sizes()[0]; }

template<class List>
size_t ColCount(const List& smv) { return smv.sizes()[1]; }

#endif  /* CPPVIEWS_BENCH_SMV_HPP_ */
