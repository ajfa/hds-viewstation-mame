@echo off
rem HDS / Neoware @workStation (Intel i960CA) with netOS 3.2 on a 1152x900 screen, booting from its local disk.
cd /d "%~dp0."
"%~dp0hdsfx.exe" hdsfxv16 -rompath "%~dp0roms" -inipath "%~dp0ini" -hard "%~dp0disk\netos.hd" ^
    -nvram_directory "%~dp0state" -cfg_directory "%~dp0state\cfg" ^
    -snapshot_directory "%~dp0snapshots" -window -nomaximize -skip_gameinfo -mouse %*
if errorlevel 1 pause
