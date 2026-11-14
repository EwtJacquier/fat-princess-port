#!/usr/bin/env python3
"""FP.wad: nested "WAD0" archives (magic stored as 0DAW).

Header (0x28 bytes): magic, u16 version 2, u16 4, u32 entries, u32 data start (header + table),
u32 alignment, u32 data size, u32 stored size, 3 x u32 0. Then `entries` x (u32 name hash, u32
offset, u32 size), offsets from the WAD's own start. When the stored size is smaller than the data
size the data is zlib-compressed: it decompresses to what follows the header, in place (offsets
count from the WAD's start as if it were stored uncompressed). An entry may be a WAD0 itself.

    python3 -I re/tools/wad.py ISO          (summary of what is inside)
"""
import struct
import sys
import zlib
from collections import Counter

MAGIC = b'0DAW'


def iso_file(f, path):
    f.seek(16 * 2048)
    pvd = f.read(2048)
    lba, size = struct.unpack_from('<I', pvd, 158)[0], struct.unpack_from('<I', pvd, 166)[0]
    for part in path.split('/'):
        f.seek(lba * 2048)
        d = f.read(size)
        p, found = 0, None
        while p < len(d):
            n = d[p]
            if n == 0:
                p = (p // 2048 + 1) * 2048
                continue
            name = d[p + 33:p + 33 + d[p + 32]].decode('latin1').split(';')[0]
            if name.upper() == part.upper():
                found = struct.unpack_from('<I', d, p + 2)[0], struct.unpack_from('<I', d, p + 10)[0]
                break
            p += n
        if not found:
            raise SystemExit(f'{path}: not in the ISO')
        lba, size = found
    return lba * 2048, size


def read_wad(iso_path):
    with open(iso_path, 'rb') as f:
        off, size = iso_file(f, 'PSP_GAME/USRDIR/FP.wad')
        f.seek(off)
        return f.read(size)


def open_wad(d, at=0):
    """The WAD at d[at:] as one uncompressed buffer (header included) and its entries
    [(hash, start, end)] in that buffer."""
    n, data, _align, usize, csize = struct.unpack_from('<IIIII', d, at + 8)
    if csize < usize:
        body = bytes(data) + zlib.decompress(d[at + data:at + data + csize])
        buf = d[at:at + data] + body[data:]
    else:
        buf = d[at:at + data + usize]
    out = []
    for i in range(n):
        h, o, s = struct.unpack_from('<III', buf, 0x28 + 12 * i)
        out.append((h, o, o + s))
    return buf, out


def walk(d, at=0, path=()):
    """Every leaf file: (path of hashes, bytes)."""
    buf, ents = open_wad(d, at)
    for h, s, e in ents:
        if buf[s:s + 4] == MAGIC:
            yield from walk(buf, s, path + (h,))
        else:
            yield path + (h,), buf[s:e]


if __name__ == '__main__':
    d = read_wad(sys.argv[1])
    kinds = Counter()
    n = 0
    for p, b in walk(d):
        n += 1
        kinds[b[:4]] += 1
    print(n, 'files')
    for k, c in kinds.most_common(30):
        print(c, k)
