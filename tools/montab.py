#!/usr/bin/env python3
# dump the boot PROM monitor/mode table at 0x30034570 (28-byte entries)
# usage: montab.py V32-boot.dec
import struct, sys
img = open(sys.argv[1], 'rb').read()
o = 0x34570
i = 0
while True:
    e = img[o:o + 28]
    if struct.unpack_from('<H', e, 2)[0] == 0 and i > 0:
        break
    h = struct.unpack_from('<14H', e)
    print('%2d %08x  ' % (i, 0x30000000 + o) + ' '.join('%04x' % x for x in h), ' cat', e[8])
    o += 28; i += 1
    if i > 80: break
