#ifndef CPPVIEWS_BENCH_GUI_UTIL_SPARSE_MATRIX_HPP_
#define CPPVIEWS_BENCH_GUI_UTIL_SPARSE_MATRIX_HPP_

#include <algorithm>
#include <limits>
#include <unordered_map>
#include <utility>
#include <vector>

template<typename T = double, class Coord = int>
class SparseMatrix {
  struct PointHasher;
  typedef std::vector< std::vector<Coord> > NonZeroList;

 public:
  typedef std::pair<Coord, Coord> Point;
  typedef typename NonZeroList::value_type::const_iterator CoordIter;
  typedef std::pair<CoordIter, CoordIter> CoordIterRange;

  SparseMatrix() = default;

  template<class InputStream>
  void Init(InputStream& is) {
    size_t n;
    Coord row_cnt, col_cnt;
    is >> row_cnt >> col_cnt >> n;
    rows_.resize(col_cnt);
    cols_.resize(row_cnt);
    values_.reserve(n);
    while (n--) {
      Coord row, col; T val;
      is >> row >> col >> val;
      values_[Point(--row, --col)] = val;
      rows_[col].push_back(row);
      cols_[row].push_back(col);
    }
    SortAndShrink(rows_);
    SortAndShrink(cols_);
  }

  const T& operator()(const Coord& row, const Coord& col) const {
    static const T kZero = T();
    auto it = values_.find(Point(row, col));
    if (it != values_.end())
      return it->second;
    return kZero;
  }

  CoordIterRange
  nonzero_col_range(Coord row, Coord col_from, Coord col_to) const {
    CoordIterRange r;
    GetRange(cols_, row, col_from, col_to, &r);
    return r;
  }

  CoordIterRange
  nonzero_row_range(Coord col, Coord row_from, Coord row_to) const {
    CoordIterRange r;
    GetRange(rows_, col, row_from, row_to, &r);
    return r;
  }

  Coord row_count() const { return cols_.size(); }
  Coord col_count() const { return rows_.size(); }
  size_t nonzero_count() const { return values_.size(); }
  size_t element_count() const { return rows_.size() * cols_.size(); }

 private:
  typedef std::unordered_map<Point, T, PointHasher> ValueMap;

  struct PointHasher {
    size_t operator()(const Point& p) const {
      return p.first << (std::numeric_limits<Coord>::digits >> 1) | p.second;
    }
  };

  static void SortAndShrink(NonZeroList& list) {
    for (auto& indices : list) {
      indices.shrink_to_fit();
      std::sort(indices.begin(), indices.end());
    }
  }

  static void GetRange(const NonZeroList& list, Coord i, Coord from, Coord to,
                       CoordIterRange* r) {
    auto begin = list[i].begin() + from, end = list[i].end();
    r->first = lower_bound(begin, end, from);
    r->second = lower_bound(r->first, end, to);
  }

  ValueMap values_;
  NonZeroList rows_;
  NonZeroList cols_;
};

#endif  /* CPPVIEWS_BENCH_GUI_UTIL_SPARSE_MATRIX_HPP_ */
