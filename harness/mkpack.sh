#!/bin/bash
# Assemble the Windows pack folder and zip.
#   harness/mkpack.sh <netos.exe> <netos.hd> <V32-bootprom.bin> [outdir]
# The executable is stripped and renamed hdsfx.exe; the boot PROM goes in roms/hdsfx.
set -e
here=$(cd "$(dirname "$0")/.." && pwd)
exe=$1; disk=$2; rom=$3; out=${4:-.}
p=$out/HDS-netOS-Windows
rm -rf "$p" "$out/HDS-netOS-Windows.zip"
mkdir -p "$p/roms/hdsfx" "$p/disk" "$p/ini"
if command -v x86_64-w64-mingw32-strip > /dev/null; then
	x86_64-w64-mingw32-strip -o "$p/hdsfx.exe" "$exe"
else
	strip -o "$p/hdsfx.exe" "$exe"
fi
cp "$disk" "$p/disk/netos.hd"
cp "$rom" "$p/roms/hdsfx/v32-bootprom.bin"
cp "$here"/pack/NETOS*.bat "$p/"
cp "$here/pack/ui.ini" "$p/ini/"
sed 's/$/\r/' "$here/pack/README.txt" > "$p/README.txt"
(cd "$out" && python3 -c "
import os, zipfile
z = zipfile.ZipFile('HDS-netOS-Windows.zip', 'w', zipfile.ZIP_DEFLATED, compresslevel=9)
for root, dirs, files in os.walk('HDS-netOS-Windows'):
    dirs.sort()
    for f in sorted(files):
        z.write(os.path.join(root, f))
")
ls -l "$out/HDS-netOS-Windows.zip"
