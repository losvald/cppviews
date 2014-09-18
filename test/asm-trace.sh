#!/bin/bash
if [ $# -lt 1 ]; then
    echo "Usage: $0 BINARY" >&2
    exit 1
fi
bin="$1"
scriptdir=$(dirname "$(readlink -f "$0")")
gdbinit="$scriptdir/$(basename "$bin").gdbinit"
asmtrace="$bin.asmtrace"
if ! [ -e "$gdbinit" ]; then
    echo "Using default GDB init file instead of missing: $gdbinit" >&2
    gdbinit="$scriptdir/asm-trace.gdbinit"
fi

RCPAT='^\s*\[\?Inferior'
rc=$(gdb -batch -x "$gdbinit" "$bin" 2>/dev/null \
    | grep -e "$RCPAT" -e '^=> ' | tee "$asmtrace" \
    | tail | grep "$RCPAT" | sed 's/^.* \(.*\).$/\1/')
if [ -z "$rc" ] || [ "$rc" == "killed" ]; then
    echo "Error: trace too long." >&2
    exit 3
fi

# strip off instructions after returning from main() (process cleanup by the OS)
mainend="$(tac "$asmtrace" | grep '<main+' -m1)"
sed -i -n "1,/$(echo "$mainend" | sed -e 's/[\/&]/\\&/g')/p" "$asmtrace"
