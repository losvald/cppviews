#!/bin/bash

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
hpcc_dir="$script_dir/hpcc"

(cd "$hpcc_dir" &&
    sed 's/2\(\s\+[PQ]s\)/1\1/' _hpccinf.txt >hpccinf.txt && >hpccoutf.txt &&
    ./hpcc && python "$script_dir/ra_parse.py" hpccoutf.txt
) || exit $?
