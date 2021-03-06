#!/usr/bin/env bash

src_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$src_dir/../_gen-common.sh"

if [ $# -lt 1 ]; then
    echo "Usage: $0 MTX_FILE" >&2
    exit 1
fi
mtx="$(mtx_path "$1")"
(( $? != 0 )) && exit 1
hpp="$src_dir/$(mtx_name "$mtx").hpp"

sm=$(sm_name $hpp)
guard=$(include_guard "$hpp")
sizes=$(mtx_sizes "$mtx")
data_type=$(mtx_data_type "$mtx")
base_type="SmvFacade<arma::SpMat<$data_type> >"
cat >"$hpp" <<EOF
#ifndef $guard
#define $guard

#include "facade.hpp"

#include "../../smv_factory.hpp"
#include "../../util/sparse_matrix.hpp"

#include <exception>
#include <fstream>
#include <string>

class $sm
#define SM_BASE_TYPE \\
  $base_type  // avoid type repetition
    : public SM_BASE_TYPE {
  typedef SM_BASE_TYPE BaseType;
#undef SM_BASE_TYPE

 public:
  $sm() : BaseType(ZeroPtr<$data_type>(), $sizes) {
    // Since the path depends on the path from which the compiled binary is run
    // and there are several such binaries, we need to specify an absolute path
    std::ifstream ifs("$mtx");
    if (!ifs)
      throw std::invalid_argument("invalid path to .mtx file");

    SparseMatrix<$data_type, unsigned> sm;
    sm.Init(ifs);
    const unsigned row_to = RowCount(*this);
    const unsigned col_to = ColCount(*this);
    for (unsigned row = 0; row < row_to; ++row)
      for (auto col_range = sm.nonzero_col_range(row, 0, col_to);
         col_range.first != col_range.second; ++col_range.first) {
        auto col = *col_range.first;
        (*this)(row, col) = sm(row, col);
      }
    nondefault_cnt_ = sm.nonzero_count();
  }

  void VecMult(const VecInfo<DataType, CoordType>& vi,
               std::vector<DataType>* res) const {
    arma::SpMat<$data_type> v(ColCount(*this), 1);
    for (const auto& e : vi)
      v(e.first) = e.second;
    decltype(v) v_res = sm_ * v;
    res->resize(RowCount(*this));
    for (CoordType r = 0; r < res->size(); ++r)
      (*res)[r] = v_res.at(r);
  }
};

#endif  // $guard
EOF

echo "Generated code in $hpp" >&2
cat "$hpp"
