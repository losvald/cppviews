#ifndef CPPVIEWS_BENCH_SMV_FACADE_HPP_
#define CPPVIEWS_BENCH_SMV_FACADE_HPP_

#include <array>
#include <map>
#include <utility>

template<typename AdapteeType>
class SmvFacade;

namespace detail {

template<typename Map>
struct SmvFacadeMapHelper {
 public:
  typedef Map MapType;
  typedef typename Map::mapped_type DataType;
  typedef std::array<size_t, 2> SizeArray;
  DataType default_val_ = 0;

 protected:
  typedef typename MapType::key_type KeyType;
  typedef typename KeyType::first_type CoordType;

 public:
  SmvFacadeMapHelper(DataType* default_value,
                     const CoordType& row_count, const CoordType& col_count)
      : size_(row_count * col_count),
        sizes_({row_count, col_count}),
        default_value_(default_value) {}

  DataType& operator()(const CoordType& row, const CoordType& col) {
    return map_[KeyType(row, col)];
  }

  DataType& operator()(const CoordType& row, const CoordType& col) const {
    const auto it(map_.find(KeyType(row, col)));
    return it != map_.cend() ? it->second : *default_value_;
  }

  const size_t size() const { return size_; }
  const SizeArray& sizes() const { return sizes_; }

 protected:
  size_t size_;
  SizeArray sizes_;
  mutable MapType map_;
  DataType* default_value_;
};

}  // namespace detail

template<typename T, typename CoordType>
#define V_THIS_ADAPTEE_TYPE                     \
  std::map<std::pair<CoordType, CoordType>, T>
class SmvFacade<V_THIS_ADAPTEE_TYPE>
#define V_THIS_BASE_TYPE                                \
  detail::SmvFacadeMapHelper<V_THIS_ADAPTEE_TYPE>
    : public V_THIS_BASE_TYPE {
  typedef V_THIS_BASE_TYPE Helper;
#undef V_THIS_BASE_TYPE
#undef V_THIS_ADAPTEE_TYPE
 public:
  SmvFacade(typename Helper::DataType* default_value,
            const CoordType& row_count, const CoordType& col_count)
      : Helper(default_value, row_count, col_count) {}
};

#endif  /* CPPVIEWS_BENCH_SMV_FACADE_HPP_ */
