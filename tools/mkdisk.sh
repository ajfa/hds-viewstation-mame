#!/bin/bash
# Build the IDE disk image from the netOS 3.2 CD.
#
#   NETOS_CD=/path/to/extracted/cd tools/mkdisk.sh [out.hd]
#
# Partition b holds the boot PROM image, d netOS 3.2 (V32.bin) and the CD's netOS tree plus
# the overlay (Netscape) in the read-only devf file system, and e an empty 4.2BSD file
# system that netOS mounts as /disk1.
#
# disk1-empty.img.gz is that empty file system as netOS itself made it: its own newfs, run
# inside the emulator on a blank partition, then cut out of the disk image. The FFS layout
# of current tools (NetBSD makefs, for one) is too new for netOS, whose fsck then stops the
# boot asking questions on the console.
set -e
here=$(cd "$(dirname "$0")" && pwd)
: "${NETOS_CD:?set NETOS_CD to the extracted netOS 3.2 CD}"
out=${1:-netos.hd}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
python3 "$here/overlay.py" "$NETOS_CD" "$work/overlay" > /dev/null
gzip -dc "$here/disk1-empty.img.gz" > "$work/disk1.img"
python3 "$here/mkdisk.py" "$out" "$NETOS_CD/netos32/netOS/image/i960/V32-bootprom.bin" \
	"$NETOS_CD/netos32/netOS/image_u/i960/V32.bin" \
	"$NETOS_CD/netos32/netOS:$work/overlay" 256 "$work/disk1.img" e
