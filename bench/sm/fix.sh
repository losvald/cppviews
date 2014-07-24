#!/usr/bin/env bash
src_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if (( $# < 1 )); then
    echo "Usage: $0 MTX_FILE..." >&2
    exit 1
fi

for mtx in "$@"; do
    mtx_dir="$(cd "$(dirname "$mtx")" && pwd)"
    mtx_fix="$mtx_dir/.fix.$(basename "$mtx")"
    if [[ "$mtx_dir" == "$src_dir" && "${mtx##*.}" == "mtx" &&
		-e "$mtx" && -e "$mtx_fix" ]]; then
	eval "bash '$mtx_fix' '$mtx'"
    else
	echo "Skipping file: $mtx" >&2
    fi
done
