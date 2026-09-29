#!/usr/bin/env python3
# Build a raw IDE disk for the HDS/Neoware boot PROM.
#
# Sector 0 holds a 4.4BSD disklabel (little endian, 8 partitions). Partition index 0 is
# never offset by the driver, so it is left as the whole disk.
# With an FFS image (layout of the netOS disk partitioning command):
#   a (fstype 9)   one sector
#   b (fstype 10)  boot code
#   c (fstype 11)  server code and devf
#   d (fstype 7)   4.2BSD file system, mounted by netOS as /disk1 (writable: home, settings)
# Without it (older layout): a and c whole disk, b boot code, d server code, e the rest.
#   b (fstype 10)  boot code: a boot PROM file (magic 0x48680342 at +4), loaded by name
#   d (fstype 11)  server code (32-byte header, magic 0x24681357 at +12) followed by a
#                  read-only "devf" file system:
#                    32-byte header: dir offset, magic 0x01122334, entry count, dir size
#                    directory: name NUL, u32 offset, u32 size (offsets from data start)
#                    file data
#   e (fstype 9)   the rest
# usage: mkdisk.py out.hd bootprom servercode tree[:overlay...] [size_mb [ffs.img [e]]]
# ("e": keep the older layout and put the FFS in partition e)
import os, struct, sys

out, boot, code, tree = sys.argv[1:5]
size_mb = int(sys.argv[5]) if len(sys.argv) > 5 else 256
ffs = open(sys.argv[6], 'rb').read() if len(sys.argv) > 6 else None
ffs_on_e = len(sys.argv) > 7 and sys.argv[7] == 'e'
SEC = 512
HEADS, SPT = 16, 32
CYL = HEADS * SPT
total = size_mb * 1024 * 1024 // SEC
cyls = total // CYL

def cyl_round(n):
    return (n + CYL - 1) // CYL * CYL

bimg = open(boot, 'rb').read()
assert struct.unpack_from('<I', bimg, 4)[0] == 0x48680342, 'not a boot PROM image'
assert sum(bimg[:struct.unpack_from('<I', bimg, 8)[0]]) & 0xff == 0, 'boot image checksum'
cimg = open(code, 'rb').read()
assert struct.unpack_from('<I', cimg, 12)[0] == 0x24681357, 'not a server code image'

# devf file system with the netOS tree as /netOS/...
# (several trees separated by ':' are merged, later ones replacing files of earlier ones)
source = {}
for t in tree.split(':'):
    for root, dirs, names in os.walk(t):
        dirs[:] = sorted(d for d in dirs if d != 'ppc8xx')
        for n in sorted(names):
            full = os.path.join(root, n)
            if os.path.isfile(full) and not os.path.islink(full):
                source[('/netOS/' + os.path.relpath(full, t)).replace(os.sep, '/')] = full
files = list(source)
directory, data = bytearray(), bytearray()
for name in files:
    blob = open(source[name], 'rb').read()
    directory += name.encode() + b'\0' + struct.pack('<II', len(data), len(blob))
    data += blob
    data += b'\0' * (-len(data) % 4)
fs = struct.pack('<IIII', 32, 0x01122334, len(files), len(directory)) + b'\0' * 16 + directory + data
servpart = cimg + fs

FS_FLASH, FS_BOOT, FS_SERVER = 9, 10, 11
boot_off, boot_len = CYL, cyl_round((len(bimg) + SEC - 1) // SEC)
code_off, code_len = boot_off + boot_len, cyl_round((len(servpart) + SEC - 1) // SEC)
rest_off = code_off + code_len
assert rest_off < total, 'disk too small'

FS_BSDFFS = 7
parts = [(0, 0, 0)] * 8
if ffs is None or ffs_on_e:
    parts[0] = (total, 0, 0)
    parts[1] = (boot_len, boot_off, FS_BOOT)
    parts[2] = (total, 0, 0)
    parts[3] = (code_len, code_off, FS_SERVER)
    parts[4] = (total - rest_off, rest_off, FS_BSDFFS if ffs_on_e else FS_FLASH)
else:
    assert len(ffs) <= (total - rest_off) * SEC, 'FFS image does not fit'
    parts[0] = (1, 0, FS_FLASH)
    parts[1] = (boot_len, boot_off, FS_BOOT)
    parts[2] = (code_len, code_off, FS_SERVER)
    parts[3] = (total - rest_off, rest_off, FS_BSDFFS)

label = bytearray(148 + 16 * 8)
struct.pack_into('<I', label, 0, 0x82564557)
struct.pack_into('<HH', label, 4, 5, 0)
label[8:24] = b'ESDI'.ljust(16, b'\0')
label[24:40] = b'netOS'.ljust(16, b'\0')
struct.pack_into('<IIIIII', label, 40, SEC, SPT, HEADS, cyls, CYL, total)
struct.pack_into('<HHHH', label, 72, 3600, 1, 0, 0)
struct.pack_into('<I', label, 132, 0x82564557)
struct.pack_into('<HHII', label, 136, 0, 8, 8192, 8192)
for i, (sz, off, fst) in enumerate(parts):
    # FFS: 1 KB fragments, 8 per block, 16 cylinders per group (as makefs lays it out)
    fsize, frag, cpg = (1024, 8, 16) if fst == FS_BSDFFS else (0, 0, 0)
    struct.pack_into('<IIIBBH', label, 148 + 16 * i, sz, off, fsize, fst, frag, cpg)
ck = 0
for i in range(0, len(label), 2):
    ck ^= struct.unpack_from('<H', label, i)[0]
struct.pack_into('<H', label, 136, ck)

with open(out, 'wb') as f:
    f.truncate(total * SEC)
    f.seek(0); f.write(label)
    f.seek(boot_off * SEC); f.write(bimg)
    f.seek(code_off * SEC); f.write(servpart)
    if ffs is not None:
        f.seek(rest_off * SEC); f.write(ffs)
print('%s: %d MB, %d files (%d KB) in the devf file system; boot code at %d, server code at %d'
      % (out, size_mb, len(files), len(data) // 1024, boot_off, code_off))
