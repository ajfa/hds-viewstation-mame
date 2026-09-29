#!/bin/bash
# Put the driver and the patches into a MAME source tree.
#   MAME_SRC=/path/to/mame harness/install.sh
# The patches are against MAME 60e07cb1 (2026-09-23) and apply with small offsets to nearby
# versions. ui-skip-warnings.patch is optional: it lets skip_warnings in ui.ini also hide
# the startup warnings on a machine MAME has not shown them on before (as in a fresh pack).
set -e
here=$(cd "$(dirname "$0")/.." && pwd)
: "${MAME_SRC:?set MAME_SRC to a MAME source tree}"
mkdir -p "$MAME_SRC/src/mame/hds"
cp "$here/driver/viewstation.cpp" "$MAME_SRC/src/mame/hds/viewstation.cpp"
cd "$MAME_SRC"
for p in i960-ca i82596-self-test slirp-network mame-lst ui-skip-warnings; do
	if git apply --check "$here/patches/$p.patch" 2> /dev/null; then
		git apply "$here/patches/$p.patch"
		echo "applied $p"
	elif git apply --reverse --check "$here/patches/$p.patch" 2> /dev/null; then
		echo "already applied $p"
	else
		echo "FAILED $p"
		exit 1
	fi
done
