#ifndef CPPVIEWS_BENCH_SMV_FACADE_HPP_
#define CPPVIEWS_BENCH_SMV_FACADE_HPP_

#include "smv.hpp"

#include <array>
#include <iterator>
#include <map>
#include <utility>

template<typename AdapteeType>
class SmvFacade;

namespace detail {

template<typename T, typename Coord = unsigned>
class SmvFacadeHelper {
  typedef std::array<size_t, 2> SizeArray;

 public:
  typedef T DataType;
  typedef Coord CoordType;

  SmvFacadeHelper(DataType* default_value,
                  const CoordType& row_count, const CoordType& col_count)
      : size_(row_count * col_count),
        sizes_({row_count, col_count}),
        default_value_(default_value) {}

  const size_t size() const { return size_; }
  const SizeArray& sizes() const { return sizes_; }

 protected:
  size_t size_;
  SizeArray sizes_;
  DataType* default_value_;
};

template<typename Map>
class SmvFacadeMapHelper
#define V_THIS_BASE_TYPE                                \
  SmvFacadeHelper<typename Map::mapped_type, typename Map::key_type::first_type>
    : public V_THIS_BASE_TYPE {
  typedef typename Map::key_type KeyType;
 public:
  typedef typename SmvFacadeMapHelper::DataType DataType;
  typedef typename SmvFacadeMapHelper::CoordType CoordType;

  class ValuesView {
    typedef typename Map::iterator MapIter;
   public:
    // don't use iterator.hpp to avoid bias in benchmarks and be transparent
    class Iterator : public std::iterator<std::forward_iterator_tag,
                                          DataType,
                                          typename MapIter::difference_type> {
     public:
      Iterator(MapIter it) : it_(it) {}
      Iterator& operator++() { ++it_; return *this; }
      Iterator& operator++(int) { Iterator old(it_); ++it_; return old; }
      DataType& operator*() const { return it_->second; }
      DataType* operator->() const { return &this->operator*(); }
      bool operator==(const Iterator& other) const {
        return this->it_ == other.it_;
      }
      bool operator!=(const Iterator& other) const { return !(*this == other); }
     private:
      MapIter it_;
    };

    ValuesView(Map* map) : map_(map) {}
    Iterator begin() const { return map_->begin(); }
    Iterator end() const { return map_->end(); }
    size_t size() const { return map_->size(); }
   private:
    Map* map_;
  };

  SmvFacadeMapHelper(DataType* default_value,
                     const CoordType& row_count, const CoordType& col_count)
      : V_THIS_BASE_TYPE(default_value, row_count, col_count),
        values_(&map_) {}
#undef V_THIS_BASE_TYPE

  DataType& operator()(const CoordType& row, const CoordType& col) {
    return map_[KeyType(row, col)];
  }

  DataType& operator()(const CoordType& row, const CoordType& col) const {
    const auto it(map_.find(KeyType(row, col)));
    return it != map_.cend() ? it->second : *this->default_value_;
  }

 protected:
  mutable Map map_;
  ValuesView values_;
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

  const typename SmvFacade::ValuesView& values() const { return this->values_; }
};

#endif  /* CPPVIEWS_BENCH_SMV_FACADE_HPP_ */
