#!/bin/bash
# Run a machine headless for a number of emulated seconds, as fast as the host allows.
#   MAME_BIN=/path/to/netos ROMS=/path/to/roms DISK=/path/to/netos.hd harness/run.sh <seconds> [system] [mame args]
# Output goes to run.out and error.log in the current directory. Examples:
#   harness/run.sh 60 hdsfx -autoboot_script harness/lua/pcwatch.lua
#   BPS=harness/lua/romlog.txt harness/run.sh 60 hdsfx -debug -debugger none -autoboot_script harness/lua/bps.lua
#   CLICKS='963,479,236' SHOTS='240' harness/run.sh 241 hdsfx -autoboot_script harness/lua/clickseq.lua
set -e
secs=${1:?seconds}; shift
sys=${1:-hdsfx}; [ $# -gt 0 ] && shift
: "${MAME_BIN:?set MAME_BIN to the built netos binary}" "${ROMS:?set ROMS to the rompath}" "${DISK:?set DISK to the disk image}"
# no window, no sound device: SDL must not see a display at all, or it still opens one
env -u DISPLAY -u WAYLAND_DISPLAY -u XDG_SESSION_TYPE SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
	"$MAME_BIN" "$sys" -rompath "$ROMS" -hard "$DISK" -video none -sound none -skip_gameinfo \
	-seconds_to_run "$secs" -nothrottle -log "$@" > run.out 2>&1
echo "exit $?"
