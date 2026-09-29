#!/usr/bin/env python3
# LZRW3-A decompressor (Ross Williams, 1991), as used by the HDS/Neoware boot PROM.
# usage: lzrw3a.py bootprom.bin out.bin
import struct, sys

START = b'123456789012345678'   # hash table entries initially point at this string

def hash3(b0, b1, b2):
    return (((40543 * ((b0 << 8) ^ (b1 << 4) ^ b2)) >> 4) & 0x1ff) << 3

def decompress(src):
    flag = struct.unpack_from('<I', src, 0)[0]
    if flag == 1:
        return bytes(src[4:])
    out = bytearray()
    buf = lambda p: START[p[1]] if p[0] == 's' else out[p[1]]
    table = [('s', 0)] * 4096
    p = 4
    end = len(src)
    control = 1
    literals = 0
    cycle = 0
    while p < end:
        if control == 1:
            control = 0x10000 | src[p] | (src[p + 1] << 8)
            p += 2
        unroll = 16 if p <= end - 32 else 1
        while unroll:
            unroll -= 1
            if control & 1:
                lenmt = src[p]; p += 1
                index = ((lenmt & 0xf0) << 4) | src[p]; p += 1
                kind, zp = table[index]
                start = len(out)
                for i in range(3 + (lenmt & 0xf)):
                    out.append(START[zp + i] if kind == 's' else out[zp + i])
                if literals > 0:
                    r = start - literals
                    table[hash3(out[r], out[r + 1], out[r + 2]) + cycle] = ('o', r)
                    cycle = (cycle + 1) & 7
                    if literals == 2:
                        r += 1
                        table[hash3(out[r], out[r + 1], out[r + 2]) + cycle] = ('o', r)
                        cycle = (cycle + 1) & 7
                    literals = 0
                table[(index & 0xff8) + cycle] = ('o', start)
                cycle = (cycle + 1) & 7
            else:
                out.append(src[p]); p += 1
                literals += 1
                if literals == 3:
                    r = len(out) - 3
                    table[hash3(out[r], out[r + 1], out[r + 2]) + cycle] = ('o', r)
                    cycle = (cycle + 1) & 7
                    literals = 2
            control >>= 1
            if p >= end:
                break
    return bytes(out)

rom = open(sys.argv[1], 'rb').read()
hdr = 0xa0
hlen, = struct.unpack_from('<H', rom, hdr)
load, entry = struct.unpack_from('<II', rom, hdr + 12)
size = 0x3f035
data = rom[hdr + hlen:hdr + size]
out = decompress(data)
print('header len %#x load %#x entry %#x in %#x out %#x' % (hlen, load, entry, len(data), len(out)))
open(sys.argv[2], 'wb').write(rom[hdr:hdr + hlen] + out if False else out)
