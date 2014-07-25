#!/usr/bin/env bash

function find_mtx {
    name="$1"
    # Consider all prefixes that precede '_' or '-' as a *.mtx basename
    # (since view variants have suffices beginning with those two characters)
    local src_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    for ((i=${#name}; i > 0; --i)); do
	suffix="${name:i}"
	regex='^_|-'
	if [[ -z $suffix || $suffix =~ $regex ]]; then
	    mtx="$src_dir/${name:0:i}.mtx"
	    # pick only the longest matching *.mtx
	    if [[ -e "$mtx" ]]; then
		echo "$mtx"
		return 0
	    fi
	fi
    done
    echo "Could not find a *.mtx file whose basename is a prefix of $name" >&2
    return 1
}
