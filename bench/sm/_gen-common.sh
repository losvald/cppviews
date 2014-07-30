#!/usr/bin/env bash

_gen_common_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$_gen_common_dir/_common.sh"

function mtx_data_type {
    local mtx="$1"
    if grep -q -m 1 '^[^%].*\(\.\|e\).*' "$mtx"; then
	# real iff 'e' or '-' in mtx body
	echo double
    else
	echo int
    fi
}

function mtx_sizes {
    local mtx="$1"
    grep -m 1 '^[^%][0-9]' "$mtx" | awk '{print $1 ", " $2}'
}

function mtx_path {
    local path
    path="$(readlink -e "$1")"
    exit_code=$?
    if (( $exit_code != 0 )); then
	echo "mtx file does not exist: $1" >&2
	return 1
    fi
    echo $path
}

function mtx_name {
    local name=$(basename "$1")
    echo ${name%.*}
}

function sm_name {
    local name="$(basename "$1")"
    echo ${name%.*} | tr '-' '_'  # symbols must not contain '-' ('_' is fine)
}

function include_guard {
    local hpp="$1"
    local name=$(sm_name "$hpp")
    echo -n "CPPVIEWS_BENCH_SM_"
    echo "$(basename $(dirname "$(readlink -e "$hpp")"))_${name}_HPP" \ |
    tr a-z A-Z
}
