#!/usr/bin/env bash

src_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$src_dir/../_gen-common.sh"

if [ $# -lt 2 ]; then
    echo "Usage: $0 SMVD_FILE SMV_GEN_PATH [SMV_GEN_OPTION]..." >&2
    exit 1
fi
smvd="$1"
smv_gen="$2"
shift 2

# Sanity check to reduce the risk of dangerous behavior (e.g. accidental rm -rf)
if [ "$(basename "$smv_gen")" != smv_gen ]; then
    echo "Invalid smv_gen path: $smv_gen"
    exit 2
fi

name=$(basename "$smvd")
name=${name%.*}
mtx=$(find_mtx "$name")
(( $? != 0 )) && exit 3

hpp="$src_dir/$name.hpp"
data_type=$(mtx_data_type "$mtx")
smv_gen_cmd="'$smv_gen' $@ '$smvd' '$mtx' $data_type"
echo "$smv_gen_cmd > '$hpp'" >&2
eval "$smv_gen_cmd > '$hpp'"
