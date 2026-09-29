# The IDE disk

Built by `tools/mkdisk.sh` (which calls `tools/mkdisk.py`) from the netOS 3.2 CD. 256 MB,
16 heads, 32 sectors per track.

## Label

Sector 0 holds a 4.4BSD disklabel, little endian, 8 partitions. The boot PROM's disk
driver never adds the partition offset to partition 0, so that partition starts at 0.

| partition | fstype | contents |
|---|---|---|
| a | 0 | the whole disk |
| b | 10 | boot code |
| c | 0 | the whole disk |
| d | 11 | server code (netOS) followed by the devf file system |
| e | 7 (4.2BSD) | /disk1, writable |

## Boot code (b)

A boot PROM image: the file `V32-bootprom.bin` from the CD, magic 0x48680342 at +4, bytes
summing to 0. The boot PROM loads it by name, `/dev/dsk/wd0b`, and uses it for the rest
of the mass storage boot. The image is LZRW3-A compressed (`tools/lzrw3a.py`), and runs
from 0x30000080.

## Server code and devf (d)

`V32.bin` from the CD (a 32-byte header with magic 0x24681357 at +12, then an i960 COFF
image), directly followed by a read-only file system that the boot PROM and netOS call
devf, and netOS mounts as `/dev/flash`:

- 32-byte header: offset of the directory, magic 0x01122334, number of entries, size
  of the directory;
- the directory: for each file, its name, a NUL, and two 32-bit words, the offset of
  its data (from the start of the data) and its size;
- the data, each file padded to 4 bytes.

Names are full paths, `/netOS/config/default.cfg` and so on. The tree is the CD's netOS
tree without the PowerPC (`ppc8xx`) binaries, plus the overlay that `tools/overlay.py`
makes: Netscape and the configuration changes below.

netOS 3.2 reads its configuration and programs from here: `%netOSdir%` resolves to it.
netOS only ever mounts `wd0d` by itself. If that partition were 4.2BSD, netOS would take
it as `/disk1` and look for its programs there, which is how a disk installation made
with the CD's `diskinstall.cmds` works.

## /disk1 (e)

A 4.2BSD file system, written as it comes out of **netOS's own newfs**
(`tools/disk1-empty.img.gz`, made by running `newfs /dev/dsk/wd0d` inside the emulator on
a blank partition and cutting it out of the image). Current tools make an FFS layout
that is too new for this netOS: `fsck` then runs at boot and stops on the console asking
questions.

The overlay's `config/startup.cmd`, set as `setup.startup.command.file` in
`default.cfg`, mounts it and prepares the home directory before the usual start:

    mount /dev/dsk/wd0e /disk1
    mkdir /disk1/home
    rm /disk1/home/.netscape/lock
    starter &

`starter` is what netOS runs when there is no startup file. The `rm` clears the lock a
Netscape leaves behind when the emulator is closed while it runs.

netOS keeps the file system marked clean while it is mounted and writes changes out
within a second, so closing the emulator does not trigger `fsck` on the next boot.

## Netscape

`hds-fx.3.2.1/hds-fx/bin/netscape` on the CD is Netscape Navigator 3.0 for the i960, in
the same b.out format as the netOS 3.2 programs, and runs unchanged. The overlay puts it
in `/netOS/bin/i960`, gives it an environment in `apps.cfg` with `HOME=///disk1/home`,
and adds an Internet menu to `system.netOSwmrc` with window manager functions that open
it maximized.

`harness/firstrun.sh` runs it once on a new disk so that its license is accepted and its
preferences and cache exist.
