#!/bin/bash
# Build the hdsfx machines on Windows with MSYS2 (run from any MSYS2 shell).
#   MAME_SRC=/c/path/to/mame SLIRP_PREFIX=/c/path/to/deps/mingw64 harness/build-windows.sh
#
# SLIRP_PREFIX holds the MinGW-w64 libslirp and glib2 packages, and their pcre2 and libffi
# dependencies, unpacked (they need not be installed):
#   pacman -Sw --cachedir deps mingw-w64-x86_64-libslirp mingw-w64-x86_64-glib2
#   for p in libslirp glib2 pcre2 libffi; do tar --zstd -xf deps/mingw-w64-x86_64-$p-*.pkg.tar.zst -C deps; done
# They are linked statically, so netos.exe needs no DLL beyond those of Windows.
set -u
: "${MAME_SRC:?set MAME_SRC to a MAME source tree}"
: "${SLIRP_PREFIX:?set SLIRP_PREFIX to the unpacked mingw64 tree}"
export OS=Windows_NT MSYSTEM=MINGW64 MINGW_PREFIX=/mingw64 MINGW64=C:/msys64/mingw64
export PATH=/mingw64/bin:/usr/bin:$PATH
PY=${PYTHON:-python}
cd "$MAME_SRC" || exit 1
mingw32-make SUBTARGET=netos SOURCES=src/mame/hds/viewstation.cpp \
	PYTHON_EXECUTABLE="$PY" MINGW64=C:/msys64/mingw64 PTR64=1 REGENIE=1 \
	USE_SLIRP=1 ARCHOPTS="-I$SLIRP_PREFIX/include -DLIBSLIRP_STATIC" LDOPTS="-L$SLIRP_PREFIX/lib" \
	TOOLCHAIN=C:/msys64/mingw64/bin/ NOWERROR=1 OPTIMIZE=2 SYMBOLS=0 TOOLS=0 -j"$(nproc)"
ls -l netos.exe
