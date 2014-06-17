#ifndef CPPVIEWS_BENCH_GUI_UTIL_SPARSE_MATRIX_HPP_
#define CPPVIEWS_BENCH_GUI_UTIL_SPARSE_MATRIX_HPP_

#include <algorithm>
#include <limits>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

template<typename T = double, class Coord = int>
class SparseMatrix {
  struct PointHasher;
  typedef std::map< Coord, std::vector<Coord> > NonZeroList;

 public:
  typedef std::pair<Coord, Coord> Point;
  typedef typename NonZeroList::mapped_type::const_iterator CoordIter;
  typedef std::pair<CoordIter, CoordIter> CoordIterRange;

  SparseMatrix() = default;

  template<class InputStream, size_t max_line_length = 1024>
  void Init(InputStream& is) {
    // skip the header (lines beginning with '%', if any)
    decltype(is.tellg()) offset = 0;
    for (char buf[max_line_length + 1];
         is.getline(buf, sizeof(buf)) && buf[0] == '%'; )
      offset = is.tellg();
    is.seekg(offset);

    size_t n;
    is >> row_count_ >> col_count_ >> n;
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

  Coord row_count() const { return row_count_; }
  Coord col_count() const { return col_count_; }
  size_t nonzero_count() const { return values_.size(); }
  size_t element_count() const { return size_t(row_count_) * col_count_; }

 private:
  typedef std::unordered_map<Point, T, PointHasher> ValueMap;

  struct PointHasher {
    size_t operator()(const Point& p) const {
      return p.first << (std::numeric_limits<Coord>::digits >> 1) | p.second;
    }
  };

  static void SortAndShrink(NonZeroList& list) {
    for (auto& it : list) {
      auto& indices = it.second;
      indices.shrink_to_fit();
      std::sort(indices.begin(), indices.end());
    }

    // insert a sentinel vector to handle the case of all zeroes
    if (list.empty())
      list.emplace(Coord(), std::vector<Coord>(Coord()));
  }

  static void GetRange(const NonZeroList& list, Coord i, Coord from, Coord to,
                       CoordIterRange* r) {
    auto lr = list.equal_range(i);
    if (lr.first == lr.second) {
      r->first = r->second = list.begin()->second.end();
      return;
    }

    auto begin = lr.first->second.begin(), end = lr.first->second.end();
    r->first = lower_bound(begin, end, from);
    r->second = lower_bound(r->first, end, to);
  }

  ValueMap values_;
  NonZeroList rows_;
  NonZeroList cols_;
  Coord row_count_;
  Coord col_count_;
};

#endif  /* CPPVIEWS_BENCH_GUI_UTIL_SPARSE_MATRIX_HPP_ */
