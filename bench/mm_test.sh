#!/bin/bash

src_dir=$(basename "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)")
prog=mm

for i in $(seq 5 10) $@; do
    n=$((1 << i))
    printf "N = 2^%-2d = %d\n" $i $n

    if ! $src_dir/${prog}_checker -q \
	<($src_dir/${prog} $n _ 2>/dev/null) \
	<($src_dir/${prog}_views $n _ 2>/dev/null); then
	echo 'FAILED'
	exit 1
    fi

    for impl in \
	'      ' \
	'_views'; do
	echo -ne "$prog$impl\t"
	/usr/bin/time -f "$prog$impl $n\t\t%e\t%U\t%S" \
	    $src_dir/$prog$impl $n 2>&1 | sed '2d'
    done
    echo
done
