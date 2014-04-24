#ifndef CPP14_INTSEQ_HPP_
#define CPP14_INTSEQ_HPP_

// [make_](integer|index)_sequence and index_sequence_for from C++14 draft

namespace cpp14 {

template<class T, T... Is>
struct integer_sequence {
  static_assert(std::is_integral<T>::value, "Not an integral type");

  typedef integer_sequence type;
  typedef T value_type;
  static constexpr size_t size() noexcept { return sizeof...(Is); }

  template<T I>
  using append = integer_sequence<T, Is..., I>;
};

namespace detail {

template<class T, class IS1, class IS2> struct concat_iota;

template<class T, T... Is1, T... Is2>
struct concat_iota<T, integer_sequence<T, Is1...>, integer_sequence<T, Is2...> >
    : integer_sequence<T, Is1..., (sizeof...(Is1) + Is2)...> {};

template<class T, T N>
struct iota : concat_iota<T, typename iota<T, N/2>::type,
                          typename iota<T, N - N/2>::type >::type {};

// cannot partially specialize because type T depends on template parameter,
// so we use the following macro to fully specialize for common integral types
#define INTSEQ_SPEC_IOTA(type)                                          \
  template<> struct iota<type, 0> : integer_sequence<type> {};          \
  template<> struct iota<type, 1> : integer_sequence<type, 0> {}

INTSEQ_SPEC_IOTA(int);
INTSEQ_SPEC_IOTA(unsigned);
INTSEQ_SPEC_IOTA(size_t);

#undef INTSEQ_SPEC_IOTA

}  // namespace detail

template<size_t... Is>
using index_sequence = integer_sequence<size_t, Is...>;

template<class T, T N>
using make_integer_sequence = detail::iota<T, N>;
template<size_t N>
using make_index_sequence = make_integer_sequence<size_t, N>;

template<class... T>
using index_sequence_for = make_index_sequence<sizeof...(T)>;

}  // namespace cpp14

#endif  /* CPP14_INTSEQ_HPP_ */
