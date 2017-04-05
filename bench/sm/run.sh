#!/bin/bash

src_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ $# -lt 1 ]; then
    echo "Usage: $0 header <smv_run_arg>..." >&2
    exit 1
fi
hpp="$(readlink -e $1)"
shift

name=$(basename "$hpp")
name=${name%.*}
bin="${hpp%.*}_runner"

cleanup_cmd="rm -f '$bin'"
trap "$cleanup_cmd" EXIT

# parse CXXFLAGS from Makefile.am and use as default
[ -z "$CXXFLAGS" ] && \
    CXXFLAGS=$(grep 'AM_CXXFLAGS' "$src_dir/../Makefile.am" | \
    sed 's/[^=]*=//')
[ -z "$CC" ] && CC=g++
CXXFLAGS+=" -DSM_NAME=$name -include '$hpp'"

echo "Compiling ..." >&2
compile_cmd="$CC $CXXFLAGS -O2 -o '$bin' '$src_dir/../smv_run.cpp'"
echo "$compile_cmd"
eval "$compile_cmd 1>&2"
exit_code=$?
(( $exit_code != 0 )) && exit $exit_code

echo "Running benchmark ..." >&2
run_cmd="'$bin' $@"
echo "$run_cmd"
eval "$run_cmd"
exit_code=$?
if (( $exit_code != 0 )); then
    echo "FAILED (exit_code = $exit_code)" >&2
    exit $exit_code
fi

eval "$cleanup_cmd"
