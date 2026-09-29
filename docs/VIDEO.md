# Video

The boot PROM and netOS support several video boards and tell them apart at run time.
The driver emulates two: the G300 board and the colour FX board with a TLC34075 palette
DAC (the default, CONFIG "Video board" in MAME's machine configuration menu).

## Video memory

- 0x21800000 to 0x219FFFFF: 2 MB of VRAM, 8 bits per pixel, lines 2048 bytes apart.
- 0x21A00000 to 0x21BFFFFF: off-screen memory (not a mirror of the VRAM).
- 0x21C00000 to 0x21DFFFFF: **block write window**.
- 0x21E00000 and 0x21FFFFFC: block write colour registers, one per VRAM bank (the banks
  interleave every 4 bytes).

### Block write

A word written to the window paints the block colour into the pixels whose mask bits are
set, at the same offset in VRAM. Each word covers **16 pixels**, with the low 16 bits as
the mask (bit 0 is the leftmost pixel). The second half of a 32-pixel group is reached
three ways, and all three occur:

- netOS's X server writes a pair of words with `stl` at a 32-byte aligned address: the
  word at +4 covers pixels 16 to 31;
- its edge masks are single stores at +0x00 and +0x10;
- the boot PROM uses +0x00 and +0x14.

The words of a quad store (`stq`) step 16 pixels each, so a quad covers 64 pixels. The
driver maps a window address `w` to the pixel address

    (w & ~0x1f) + 32 * bit3(w) + 16 * (bit2(w) | bit4(w))

This one rule fits every access the boot PROM and netOS make. Without block write,
netOS fills and text come out as vertical bands.

## G300 board

An INMOS G300 style controller at 0xC0000000, also answering at 0xC8000000:

- words 0x000 to 0x0FF: palette, each entry `0x00bbggrr` (red in the low byte);
- 0x121 to 0x12C: timing; 0x140: pixel mask; 0x160: control; 0x180: top of screen.

## FX board, TLC34075

The TLC34075 sits in the low bytes of the same window, 0xC0000000 to 0xC000000F (register
number = byte offset), and also answers at 0xC8000000: palette write address at 0, data
at 1 (red, green, blue, 8 bits each), pixel mask at 2, read address at 3, general
control, clock selection and multiplexing at 8 to B. Writing 3 to the test register at
0xE makes it read back the ID 0x75.

On the FX board the boot PROM programs only the DAC; nothing is written to the G300
timing registers. A second device with an index/data pair at 0xDE000002/0xDE000003 is
programmed in the same routine and is probably the clock or timing generator; it is not
emulated.

### How software identifies the board

The boot PROM (at 0x30034D50) and netOS (at 0x30177C50) run the same test:

1. write 3 to 0xC000000E and read it back. Neither 0x75 nor 0x74: go to 4;
2. write 0x5A5A5A5A to both block write colour registers, clear 32 bytes of VRAM,
   write 0xFFFFFFFF to the first window word and see which bytes turned 0x5A. The
   pattern 0x0F0F0F0F (bytes 0 to 3, 8 to 11, 16 to 19, 24 to 27) means **type 2**;
3. write 0x55 to 0xC0000002 and 0xAA to 0xC8000002, read 0xC0000002: 0x55 means a
   second, separate DAC (**type 3**), anything else a mirrored one (**type 1**);
4. a 16-bit test at 0x90000004 gives **type 4**; nothing gives **type 0**, the G300.

netOS starts its X server with `-fb g3` for type 0, `-fb fxi` for type 2, `-fb nc1` for
types 5 and 6 and `-fb fxa` for the rest. The emulated FX board is type 1, `fxa`.

## Monitor and mode switches

The monitor is chosen by switch banks read through the SCN2681 DUART's input port (IP3 to
IP6, register 0xD). Address lines A23 and A24 pick the bank, so the same register is read
at 0xD200000D, 0xD280000D, 0xD300000D and 0xD380000D:

| bank | meaning |
|---|---|
| 0 | monitor type (hex switch) |
| 1 | video mode, inverted |
| 2, 3 | jumpers, inverted |

The boot PROM keeps a mode table at 0x30034570, 28-byte entries: name index at +0, width
at +2, height at +4, clock at +6 and monitor type at +8. On the FX board it takes the
first entry of the monitor type whose clock field is 0xFFFF, or 0xFFFE when bit 3 of the
video mode is set. The monitor names, by type:

| type | name | first FX mode |
|---|---|---|
| 0 | V19C | 1280x1024 |
| 1 | V19MM | 1280x1024 |
| 2 | V17M | 1024x864 |
| 3 | V16C | 1152x900 |
| 4 | V15M | 1024x864 |
| 5 | V14C | 1024x768 |
| 6 | V19MS | 1152x900 |
| 7 | V19CT | 1280x1024 |
| 8 | VGA | 640x480 (mode bit 3) |
| 9 | VESA | 800x600 (mode bit 3) |

netOS follows whatever the boot PROM chose. MAME sizes a window before it reads its
settings, so the monitor is a separate system rather than a menu setting: `hdsfx` (V19C),
`hdsfxv16` (V16C), `hdsfxv14` (V14C) and `hdsfxvesa` (VESA with mode 8). At 640x480 the
netOS login dialog is wider than the screen, so there is no VGA system.
