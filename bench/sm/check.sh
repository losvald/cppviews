#!/bin/bash

src_dir=$(basename "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)")

if [ $# -lt 1 ]; then
    echo "Usage: $0 name" >&2
    exit 1
fi
name=$(basename $1)
name=${name%.*}  # for convenience, ignore extension
shift

bin="$src_dir/${name}_checker"
cpp="$bin.cpp"

trap "rm -f '$bin' '$cpp'" EXIT
>"$cpp"
echo "#include \"$name.hpp\"" >>"$cpp"
echo "typedef SmvFactory<$name> SmvFactoryType;" >>"$cpp"
echo "#include \"../smv_checker.cpp.tpl\"" >>"$cpp"

# parse CXXFLAGS from Makefile.am and use as default
[ -z "$CXXFLAGS" ] && \
    CXXFLAGS=$(grep 'AM_CXXFLAGS' "$src_dir/../Makefile.am" | sed 's/[^=]*=//')
[ -z $CC ] && CC=g++-4.8

echo "Compiling ..."
compile_cmd="$CC $CXXFLAGS -o '$bin' '$cpp'"
echo "$compile_cmd"
eval "$compile_cmd"
echo "Running checker ..."
run_cmd="'./$bin' $@ $src_dir/$name.mtx"
echo "$run_cmd"
eval "$run_cmd"
exit_code=$?
if (( $exit_code != 0 )); then
    echo "FAILED (exit_code = $exit_code)"
    exit $exit_code
fi
