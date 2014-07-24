#ifndef CPPVIEWS_BENCH_SM_VIEW_P0040_CDD_MANUAL_HPP_
#define CPPVIEWS_BENCH_SM_VIEW_P0040_CDD_MANUAL_HPP_

#include "../../smv_factory.hpp"
#include "../../gui/sm/view_type.hpp"

#include "../../../src/chain.hpp"
#include "../../../src/diag.hpp"

class p0040_cdd_manual
#define SM_BASE_TYPE                                            \
  v::Chain<v::ListBase<int, 2>, 1>  // avoid type repetition
    : public SM_BASE_TYPE {
  typedef SM_BASE_TYPE BaseType;
#undef SM_BASE_TYPE

 public:
  p0040_cdd_manual() : BaseType(
      v::ChainTag<1>(), v::ListVector<v::ListBase<int, 2> >()
      .Append([] {
          v::Diag<int, unsigned, 1, 1> v(ZeroPtr<int>(), 23, 23);
          // Here, 23 lines would be generated,
          //    v(0, 0) = 1;
          //    v(1, 1) = 1;
          //    ...
          // We use a loop to make it less verbose
          for (unsigned i = v.sizes()[0]; i--; )
            v(i, i) = 1;
          return v;
        }())
      .Append(
          v::ChainTag<0>(), v::ListVector<v::ListBase<int, 2> >()
          .Append([] {
              v::Diag<int, unsigned, 2, 4> v(ZeroPtr<int>(), 20, 40);
              // Here, 80 lines would be generated,
              //        v(0, 0) = -1;
              //        v(1, 0) = 1;
              //        ...
              // We use a loop to make it less verbose
              for (unsigned b = v.block_count(); b--; )
                for (unsigned i = v.block_size(1); i--; ) {
                  unsigned row = b * v.block_size(0);
                  unsigned col = b * v.block_size(1) + i;
                  v(row, col) = -1;
                  v(row + 1, col) = 1;
                }
              return v;
            }())
          .Append(
              v::ChainTag<1>(), v::ListVector<v::ListBase<int, 2> >()
              .Append([] {
                  v::Diag<int, unsigned, 1, 1> v(ZeroPtr<int>(), 3, 3);
                  v(0, 0) = -1669;
                  v(1, 1) = -1669;
                  v(2, 2) = -1669;
                  return v;
                }())
              // the rest of the Diag views have the same structure
              .Append([] {
                  v::Diag<int, unsigned, 1, 1> v(ZeroPtr<int>(), 3, 3);
                  for (unsigned i = v.sizes()[0]; i--; )
                    v(i, i) = -2191;
                  return v;
                }())
              .Append([] {
                  v::Diag<int, unsigned, 1, 1> v(ZeroPtr<int>(), 3, 3);
                  for (unsigned i = v.sizes()[0]; i--; )
                    v(i, i) = -1553;
                  return v;
                }())
              .Append([] {
                  v::Diag<int, unsigned, 1, 1> v(ZeroPtr<int>(), 3, 3);
                  for (unsigned i = v.sizes()[0]; i--; )
                    v(i, i) = -1829;
                  return v;
                }())
              .Append([] {
                  v::Diag<int, unsigned, 1, 1> v(ZeroPtr<int>(), 3, 3);
                  for (unsigned i = v.sizes()[0]; i--; )
                    v(i, i) = -1772;
                  return v;
                }())
              .Append([] {
                  v::Diag<int, unsigned, 1, 1> v(ZeroPtr<int>(), 3, 3);
                  for (unsigned i = v.sizes()[0]; i--; )
                    v(i, i) = -1665;
                  return v;
                }())
              .Append([] {
                  v::Diag<int, unsigned, 1, 1> v(ZeroPtr<int>(), 3, 3);
                  for (unsigned i = v.sizes()[0]; i--; )
                    v(i, i) = -2220;
                  return v;
                }())
              .Append([] {
                  v::Diag<int, unsigned, 1, 1> v(ZeroPtr<int>(), 3, 3);
                  for (unsigned i = v.sizes()[0]; i--; )
                    v(i, i) = -1634;
                  return v;
                }())
              .Append([] {
                  v::Diag<int, unsigned, 1, 1> v(ZeroPtr<int>(), 3, 3);
                  for (unsigned i = v.sizes()[0]; i--; )
                    v(i, i) = -2211;
                  return v;
                }())
              .Append([] {
                  v::Diag<int, unsigned, 1, 1> v(ZeroPtr<int>(), 3, 3);
                  for (unsigned i = v.sizes()[0]; i--; )
                    v(i, i) = -2142;
                  return v;
                }())
              , ZeroPtr<int>(), v::ChainOffsetVector<2>({
                  {0, 0},
                  {0, 4},
                  {0, 8},
                  {0, 12},
                  {0, 16},
                  {0, 20},
                  {0, 24},
                  {0, 28},
                  {0, 32},
                  {0, 36},
                      })
              , 3, 40)
          , ZeroPtr<int>(), v::ChainOffsetVector<2>({
              {0, 0},
              {20, 0},
                  })
          , 23, 40)
      , ZeroPtr<int>(), v::ChainOffsetVector<2>({
          {0, 0},
          {0, 23},
              })
      , 23, 63) {
  }
};

#endif  // CPPVIEWS_BENCH_SM_VIEW_P0040_CDD_MANUAL_HPP_
