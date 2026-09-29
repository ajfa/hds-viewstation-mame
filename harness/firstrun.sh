#!/bin/bash
# Run Netscape once on a new disk, headless, so its license is accepted and its cache and
# preferences exist in ///disk1/home/.netscape; the disk then opens straight into the
# browser. It boots, presses "Don't log in", opens netOS menu > Applications > Internet >
# The Old Net, accepts the license and the cache notice with Enter, and stops at 600 s.
# It needs the network (user mode, built in) for the page, not for the rest.
#   MAME_BIN=... ROMS=... DISK=netos.hd harness/firstrun.sh
set -e
here=$(cd "$(dirname "$0")" && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cd "$work"
CLICKS='963,479,190;40,1008,215;134,874,222m;300,892,226m;420,897,232m;485,915,236' \
TYPES='420=\n;460=\n' SHOTS='600' \
	"$here/run.sh" 601 hdsfx -nvram_directory nv -cfg_directory cfg -snapshot_directory snap \
	-autoboot_script "$here/lua/clicktype.lua"
ls snap/hdsfx
