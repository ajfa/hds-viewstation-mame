# Hardware

What the driver emulates, as worked out from the boot PROM and netOS 3.2. The memory map
is also at the top of `driver/viewstation.cpp`.

| address | device |
|---|---|
| 00000000-000003FF | 80960CA internal data RAM |
| 10000000, 10000004 | 82596CA Ethernet: PORT, channel attention (see NETWORK.md) |
| 21800000-21FFFFFF | video memory, off-screen memory, block write (see VIDEO.md) |
| 30000000 | DRAM, 8 MB, repeats through its 128 MB window |
| 40000000 | SIMM, 16 MB, or 32 or 64 MB with `-ram 40m` or `-ram 72m`; likewise |
| A8000000, AC000000 | board control registers, 16 bit (not emulated) |
| C0000000, C8000000 | video controller or palette DAC (see VIDEO.md) |
| D0000000, D0000001 | 8042-style keyboard and mouse controller |
| D2000000 | SCN2681 DUART, and the switch banks on its input port |
| D6000000 | 8254 timer at 3.6864 MHz; counter 0 is the 100 Hz clock |
| D8000000 | setup memory (NVRAM); the boot PROM reports 8 KB |
| DA000000 | Ethernet address PROM |
| E00001F0, E00005F0 | IDE data, 8 bits wide, with a latch for the high byte |
| E02001F0-E02001F7 | IDE task file |
| FEFC0000, FFFC0000 | boot PROM (256 KB), at both addresses |

The real board took up to 132 MB: 4 MB on the main board on some models, and two SIMM
sockets of up to 64 MB each. The driver has 8 MB on the main board and one SIMM bank.
A single 128 MB bank at 40000000 is not what the boot PROM expects: it reports 0 MB of
expansion memory, so how the second socket is decoded is still to be worked out.

## Interrupts

The 80960CA runs its external interrupt pins in dedicated mode:

| pin | source |
|---|---|
| XINT0 | DUART |
| XINT1 | a device at D4000000 with an index/data pair, most likely the PCMCIA controller (not emulated) |
| XINT2 | keyboard |
| XINT3 | mouse |
| XINT5, XINT7 | watchdog |
| XINT6 | 8254 counter 0, inverted: the 100 Hz clock |

## Keyboard and mouse

MAME's high level `kbdc8042` delivers keyboard codes at 100 Hz and, with its interrupts
wired, hangs the boot PROM, and the low level 8042 device needs the 8042's own ROM. The
driver has its own small controller: a queue of controller replies, one for the mouse and the MAME PC
keyboard (scan code set 3), with keyboard and mouse interrupts enabled by bits 0 and 1 of
the command byte. The mouse is a PS/2 mouse polled at 100 Hz; while the guest is behind,
movement is kept for a later packet instead of being dropped.

## Boot PROM

`V32-bootprom.bin` on the CD is the same file for the socketed PROM and for the boot code
partition of the disk. The image is LZRW3-A compressed; `tools/lzrw3a.py` unpacks it,
and the result runs from 0x30000080. Its log function is at 0x3000C2F0:
`harness/lua/romlog.txt` puts a breakpoint there that writes each call to MAME's error
log, and `tools/romlog.py` prints those lines with the PROM's own format strings.
