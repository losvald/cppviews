#!/usr/bin/env bash

_gen_common_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$_gen_common_dir/_common.sh"

function mtx_data_type {
    mtx="$1"
    if grep -q -m 1 '^[^%].*\(\.\|e\).*' "$mtx"; then
	# real iff 'e' or '-' in mtx body
	echo double
    else
	echo int
    fi
}
