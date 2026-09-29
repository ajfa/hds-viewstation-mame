#!/usr/bin/env python3
# Render ROMLOG lines from error.log using the format strings in the decompressed boot PROM
# usage: romlog.py V32-boot.dec error.log   (the boot PROM decompressed with lzrw3a.py)
import re, sys
img = open(sys.argv[1], 'rb').read()
B = 0x30000000
def cstr(a):
    o = a - B
    if 0 <= o < len(img):
        return img[o:o + 200].split(b'\0')[0].decode('latin1')
    return None
for l in open(sys.argv[2], errors='replace'):
    m = re.search(r'ROMLOG (\d+) ([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+)', l)
    if not m:
        continue
    lvl, fmt, *args = int(m.group(1)), *(int(x, 16) for x in m.groups()[1:])
    f = cstr(fmt)
    if f is None:
        print(lvl, hex(fmt), [hex(a) for a in args]); continue
    out, ai = '', 0
    for part in re.split(r'(%[-0-9.l]*[a-zA-Z%])', f):
        if part.startswith('%') and len(part) > 1:
            if part == '%%': out += '%'; continue
            v = args[ai] if ai < len(args) else 0; ai += 1
            if part.endswith('s'):
                s = cstr(v); out += s if s is not None else '<%x>' % v
            elif part.endswith('x') or part.endswith('X'):
                out += '%x' % v
            elif part.endswith('c'):
                out += chr(v & 0xff)
            else:
                out += str(v if v < 0x80000000 else v - (1 << 32))
        else:
            out += part
    print(lvl, out.rstrip())
