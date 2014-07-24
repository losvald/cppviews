#!/bin/bash

src_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ $# -lt 1 ]; then
    echo "Usage: $0 MTX_FILE" >&2
    exit 1
fi
mtx="$(readlink -e $1)"
exit_code=$?
if (( $exit_code != 0 )); then
    echo "mtx file does not exist: $mtx" >&2
    exit 2
fi

name=$(basename "$mtx")
name=${name%.*}
hpp="$src_dir/$name.hpp"

# sanity check
if [ $(dirname "$hpp") != "$src_dir" ]; then
    echo "refusing to write to a wrong directory: $hpp" >&2
    exit 3
fi

name=$(echo $name | tr '-' '_')  # symbols must not contain '-' ('_' is fine)
guard="HPPVIEWS_BENCH_SM_STL_MAP_$(echo $name | tr 'a-z' 'A-Z')_HPP_"
sizes=$(grep -m 1 '^[^%].[0-9]' "$mtx" | awk '{print $1 ", " $2}')
if grep -m 1 '^[^%].*\(\.\|e\).*' "$mtx"; then
    # real iff 'e' or '-' in mtx body
    coord=double
else
    coord=int
fi
cat >"$hpp" <<EOF
#ifndef $guard
#define $guard

#include "../../smv_facade.hpp"
#include "../../smv_factory.hpp"
#include "../../util/sparse_matrix.hpp"

#include <exception>
#include <fstream>
#include <string>

class $name
#define SM_BASE_TYPE \\
  SmvFacade<std::map<std::pair<unsigned, unsigned>, $coord> >
    : public SM_BASE_TYPE {
  typedef SM_BASE_TYPE BaseType;
#undef SM_BASE_TYPE

 public:
  $name() : BaseType(ZeroPtr<$coord>(), $sizes) {
    // Since the path depends on the path from which the compiled binary is run
    // and there are several such binaries, we need to specify an absolute path
    std::ifstream ifs("$mtx");
    if (!ifs)
      throw std::invalid_argument("invalid path to .mtx file");

    SparseMatrix<$coord, unsigned> sm;

    sm.Init(ifs);
    const unsigned row_to = RowCount(*this);
    const unsigned col_to = ColCount(*this);
    for (unsigned row = 0; row < row_to; ++row)
      for (auto col_range = sm.nonzero_col_range(row, 0, col_to);
         col_range.first != col_range.second; ++col_range.first) {
        auto col = *col_range.first;
        (*this)(row, col) = sm(row, col);
      }
  }
};

#endif  // $guard
EOF

echo "Generated code in $hpp" >&2
cat "$hpp"
