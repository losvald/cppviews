#ifndef CPPVIEWS_TEST_HPP_
#define CPPVIEWS_TEST_HPP_

#ifdef __clang__
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

#include <gtest/gtest.h>

#include <cstdlib>

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <string>
#include <utility>

namespace v {}
using namespace v;

template<class InputIterator1, class InputIterator2, class Equals>
bool CheckEq(InputIterator1 exp_begin, InputIterator1 exp_end,
             InputIterator2 act_begin, InputIterator2 act_end,
             const Equals& equals) {
  InputIterator1 exp_it = exp_begin;
  InputIterator2 act_it = act_begin;
  for (; exp_it != exp_end && act_it != act_end ; ++exp_it, ++act_it) {
    if (!equals(*exp_it, *act_it))
      return false;
  }
  return exp_it == exp_end && act_it == act_end;
}

template<class InputIterator1, class InputIterator2, class Compare,
class Equals>
bool CheckSortedEq(InputIterator1 exp_begin, InputIterator1 exp_end,
                   InputIterator2 act_begin, InputIterator2 act_end,
                   const Compare& comp, const Equals& equals) {
  std::sort(exp_begin, exp_end, comp);
  std::sort(act_begin, act_end, comp);
  return CheckEq(exp_begin, exp_end, act_begin, act_end, equals);
}

#endif  // CPPVIEWS_TEST_HPP_
