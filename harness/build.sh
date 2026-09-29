#!/bin/bash
# Build the hdsfx machines on Linux, with user mode networking.
#   MAME_SRC=/path/to/mame SLIRP_PREFIX=/usr harness/build.sh
# SLIRP_PREFIX is where include/slirp/libslirp.h and the libslirp library are; the
# libslirp-dev package, even just unpacked with dpkg -x, is enough. The result is
# $MAME_SRC/netos.
set -e
: "${MAME_SRC:?set MAME_SRC to a MAME source tree}"
SLIRP_PREFIX=${SLIRP_PREFIX:-/usr}
LIBDIR=$SLIRP_PREFIX/lib/x86_64-linux-gnu
[ -d "$LIBDIR" ] || LIBDIR=$SLIRP_PREFIX/lib
cd "$MAME_SRC"
make SUBTARGET=netos SOURCES=src/mame/hds/viewstation.cpp REGENIE=1 NOWERROR=1 \
	OPTIMIZE=2 SYMBOLS=0 USE_SLIRP=1 ARCHOPTS="-I$SLIRP_PREFIX/include" LDOPTS="-L$LIBDIR" \
	-j"$(nproc)"
ls -l netos
