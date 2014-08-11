#ifndef CPPVIEWS_SRC_MAP_HPP_
#define CPPVIEWS_SRC_MAP_HPP_

#include "view.hpp"

#include <utility>

namespace v {

template<typename Key, typename Value>
class Map {
 public:
  typedef Key KeyType;
  typedef Value ValueType;
  typedef std::pair<Key, Value> EntryType;

  virtual View<Value>& values() const = 0;
  // TODO: support keys view
};

}  // namespace v

#endif  /* CPPVIEWS_SRC_MAP_HPP_ */
