#!/bin/bash

src_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$src_dir/_common.sh"

if [ $# -lt 1 ]; then
    echo "Usage: $0 HEADER" >&2
    exit 1
fi
hpp="$1"
shift

if ! [ -e "$hpp" ]; then
    exit "No such header file: '$hpp'" >&2
    exit 1
fi

bin="${hpp%.*}_checker"
cpp="$bin.cpp"
name=$(basename "$hpp")
name=${name%.*}
mtx=$(find_mtx $name)

cleanup_cmd="rm -f '$bin' '$cpp'"
trap "$cleanup_cmd" EXIT

>"$cpp"
echo "#include \"$(basename $hpp)\"" >>"$cpp"
echo "typedef SmvFactory<$name> SmvFactoryType;" >>"$cpp"
echo "#include \"../../smv_checker.cpp.tpl\"" >>"$cpp"

# parse CXXFLAGS from Makefile.am and use as default
[ -z "$CXXFLAGS" ] && \
    CXXFLAGS=$(grep 'AM_CXXFLAGS' "$src_dir/../Makefile.am" | \
    sed 's/[^=]*=//')
[ -z $CC ] && CC=g++-4.8

echo "Compiling ..."
compile_cmd="$CC $CXXFLAGS -o '$bin' '$cpp'"
echo "$compile_cmd"
eval "$compile_cmd"
exit_code=$?
(( $exit_code != 0 )) && exit $exit_code

echo "Running checker ..."
run_cmd="'$bin' $@ '$mtx'"
echo "$run_cmd"
eval "$run_cmd"
exit_code=$?
if (( $exit_code != 0 )); then
    echo "FAILED (exit_code = $exit_code)"
    exit $exit_code
fi

eval "$cleanup_cmd"
