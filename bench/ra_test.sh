#!/bin/bash

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
hpcc_dir="$script_dir/hpcc"

for ra_view in ""; do

    # touch all files containing the RA_VIEW_ macro and trigger make to compile
    find "$hpcc_dir" \( -name '*.c' -o -name '*.h' \) \
	| xargs egrep -l '\bRA_VIEW_' | xargs touch
    # compile silently (if case no errors)
    make_args="arch=Linux RA_VIEW=$ra_view"
    if ! make $make_args >/dev/null 2>/dev/null; then
	make $make_args
	exit 1
    fi

    # run on 1 proc with a clear output file and parse RandomAccess section
    (cd "$hpcc_dir" &&
	sed 's/2\(\s\+[PQ]s\)/1\1/' _hpccinf.txt >hpccinf.txt &&
	>hpccoutf.txt &&
	./hpcc && python "$script_dir/ra_parse.py" hpccoutf.txt $ra_view
    ) || exit $?
done
