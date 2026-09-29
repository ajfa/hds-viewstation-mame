#!/bin/bash
# Build dasm960, a command line i960 disassembler around MAME's own i960dis.
#   MAME_SRC=/path/to/mame tools/dasm960/build.sh
# usage of the result: dasm960 file.bin load_address [start end] > file.dis
set -e
here=$(cd "$(dirname "$0")" && pwd)
: "${MAME_SRC:?set MAME_SRC to a MAME source tree}"
g++ -O2 -std=c++20 -I "$here" -I "$MAME_SRC/src/devices/cpu/i960" -I "$MAME_SRC/src/lib/util" \
	-I "$MAME_SRC/src/osd" -o "$here/dasm960" "$here/dasm960.cpp" \
	"$MAME_SRC/src/devices/cpu/i960/i960dis.cpp" "$MAME_SRC/src/lib/util/disasmintf.cpp" \
	"$MAME_SRC/src/lib/util/strformat.cpp"
