"""Pictures for the web page, read from the player's own ISO (Fat Princess: Fistful of Cake): the
title logo, the menus' sunburst behind the page, and a storybook picture beside the sign-in.

Every one is a texture of FP.wad (docs/NOTES.md has the formats): found by its
pixel data's CRC, which its texture header carries. When the texture pack (build/textures) has an
upscaled version of a picture, that one is used. Python's standard library only (serve_web.py
imports this).

    python3 -I tools/isoart.py ISO NAME OUT.png [PACK]     # NAME: logo, scene, backdrop
"""
import struct
import sys
import zlib
from pathlib import Path

ART = {  # name -> pixel CRC of the texture
    'logo': 0x70b1519e,      # FAT PRINCESS / FISTFUL OF CAKE (the English title screen's)
    'scene': 0x41897d4a,     # the two princesses and the giant cake slice (the story's pictures)
    'backdrop': 0xe3e72074,  # the menus' sunburst
}
CROP = {'logo': (138, 22, 374, 233)}  # the logo's own area of its 512x256 texture
SHOWN = ('logo', 'scene', 'backdrop')  # what the page uses (made when the server starts)


# ---- ISO 9660 and FP.wad -----------------------------------------------------------------------------
def iso_file(f, path):
    """(byte offset, size) of a file on the ISO."""
    f.seek(16 * 2048)
    pvd = f.read(2048)
    lba, size = struct.unpack_from('<I', pvd, 158)[0], struct.unpack_from('<I', pvd, 166)[0]
    for part in path.split('/'):
        f.seek(lba * 2048)
        data = f.read(size)
        p, found = 0, None
        while p < len(data):
            n = data[p]
            if n == 0:
                p = (p // 2048 + 1) * 2048
                continue
            name = data[p + 33:p + 33 + data[p + 32]].decode('latin1').split(';')[0]
            if name.upper() == part.upper():
                found = struct.unpack_from('<I', data, p + 2)[0], struct.unpack_from('<I', data, p + 10)[0]
                break
            p += n
        if not found:
            raise FileNotFoundError(path)
        lba, size = found
    return lba * 2048, size


def _leaves(d, at=0):
    """Every file of the WAD tree at d[at:] (nested WADs zlib-compressed, in place after their header)."""
    n, data, _align, usize, csize = struct.unpack_from('<IIIII', d, at + 8)
    buf = d[at:at + data] + zlib.decompress(d[at + data:at + data + csize]) if csize < usize else d[at:at + data + usize]
    for i in range(n):
        _, o, s = struct.unpack_from('<III', buf, 0x28 + 12 * i)
        if buf[o:o + 4] == b'0DAW':
            yield from _leaves(buf, o)
        else:
            yield buf[o:o + s]


def _header(b, crc):
    """(w, h, bpp, palette) of the texture header for this pixel CRC inside b, or None."""
    at = b.find(b'\x02\0\0\0' + struct.pack('<I', crc))
    while at >= 0:
        if at % 4 == 0 and at + 20 <= len(b):
            w, h, bpp = struct.unpack_from('<III', b, at + 8)
            if bpp == 32:
                return w, h, 32, b''
            if bpp in (4, 8) and at + 32 <= len(b):
                ncol, _, pbytes = struct.unpack_from('<III', b, at + 20)
                return w, h, bpp, b[at + 32:at + 32 + pbytes]
        at = b.find(b'\x02\0\0\0' + struct.pack('<I', crc), at + 1)
    return None


def _unswizzle(raw, pitch, rows):
    """GE swizzle: blocks of 16 bytes x 8 rows, left to right, then the next band of 8 rows."""
    out = bytearray(pitch * rows)
    bw = pitch // 16
    for by in range((rows + 7) // 8):
        band = raw[by * bw * 128:(by + 1) * bw * 128]
        for r in range(min(8, rows - by * 8)):
            y = by * 8 + r
            for k in range(16):
                out[y * pitch + k:(y + 1) * pitch:16] = band[r * 16 + k::128][:bw]
    return bytes(out)


def texture(iso, crc):
    """(width, height, RGBA bytes) of the FP.wad texture whose pixel data has this CRC."""
    with open(iso, 'rb') as f:
        off, size = iso_file(f, 'PSP_GAME/USRDIR/FP.wad')
        f.seek(off)
        d = f.read(size)
    hdr, pixels = None, None
    for b in _leaves(d):
        if hdr is None:
            hdr = _header(b, crc)
        if pixels is None and (~zlib.crc32(b) & 0xffffffff) == crc:
            pixels = b
        if hdr and pixels:
            break
    if not (hdr and pixels):
        raise KeyError(f'texture {crc:08x} not in FP.wad')
    w, h, bpp, pal = hdr
    pitch = w * bpp // 8
    raw = _unswizzle(pixels.ljust(pitch * ((h + 7) // 8 * 8), b'\0'), pitch, h) if pitch % 16 == 0 else pixels
    if bpp == 32:
        return w, h, bytes(raw[:w * h * 4])
    colors = [pal[i:i + 4] for i in range(0, len(pal), 4)]
    colors += [b'\0\0\0\0'] * (256 - len(colors))
    if bpp == 8:
        return w, h, b''.join(colors[i] for i in raw[:w * h])
    return w, h, b''.join(colors[n] for b in raw[:w * h // 2] for n in (b & 15, b >> 4))


def key(w, h, rgba):
    """The texture pack's key of a picture: CRC-32 and Adler-32 of its RGBA texels."""
    return f'{zlib.crc32(rgba) & 0xffffffff:08x}{zlib.adler32(rgba) & 0xffffffff:08x}'


# ---- QOI (the texture pack's pictures) and PNG ------------------------------------------------------
def qoi(d):
    """(width, height, RGBA bytes) of a QOI picture."""
    if d[:4] != b'qoif':
        raise ValueError('not a QOI picture')
    w, h = struct.unpack_from('>II', d, 4)
    n = w * h
    out = bytearray(n * 4)
    index = [b'\0\0\0\0'] * 64
    r, g, b, a = 0, 0, 0, 255
    p, i = 14, 0
    while i < n:
        c = d[p]
        p += 1
        if c == 0xfe:
            r, g, b = d[p], d[p + 1], d[p + 2]
            p += 3
        elif c == 0xff:
            r, g, b, a = d[p], d[p + 1], d[p + 2], d[p + 3]
            p += 4
        elif c >> 6 == 0:
            r, g, b, a = index[c]
        elif c >> 6 == 1:
            r, g, b = (r + (c >> 4 & 3) - 2) & 255, (g + (c >> 2 & 3) - 2) & 255, (b + (c & 3) - 2) & 255
        elif c >> 6 == 2:
            dg = (c & 63) - 32
            c2 = d[p]
            p += 1
            r, g, b = (r + dg - 8 + (c2 >> 4)) & 255, (g + dg) & 255, (b + dg - 8 + (c2 & 15)) & 255
        else:
            run = (c & 63) + 1
            px = bytes((r, g, b, a))
            out[i * 4:(i + run) * 4] = px * run
            i += run
            continue
        px = bytes((r, g, b, a))
        index[(r * 3 + g * 5 + b * 7 + a * 11) & 63] = px
        out[i * 4:i * 4 + 4] = px
        i += 1
    return w, h, bytes(out)


def png(w, h, rgba):
    """PNG bytes: RGB when every texel is opaque, each row filtered against the one above (PNG's
    "Up" filter: much smaller for pictures, still the standard library only)."""
    opaque = rgba[3::4] == b'\xff' * (w * h)
    px, bpp = (bytes(b for i, b in enumerate(rgba) if i & 3 != 3), 3) if opaque else (rgba, 4)
    stride = w * bpp
    out, prev = [], bytes(stride)
    for y in range(h):
        row = px[y * stride:(y + 1) * stride]
        out.append(b'\2' + bytes((a - b) & 255 for a, b in zip(row, prev)) if y else b'\0' + row)
        prev = row
    chunk = lambda t, d: struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2 if opaque else 6, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress(b''.join(out), 6)) + chunk(b'IEND', b''))


# ---- what the page asks for --------------------------------------------------------------------------
def original(iso, name):
    """(width, height, RGBA bytes, sprites) of one of ART, as the ISO has it (no sprite lists here)."""
    w, h, rgba = texture(iso, ART[name])
    return w, h, rgba, {}


def pack_file(pack, w, h, rgba):
    """The texture pack's file for this picture (its upscaled version), or None."""
    k = key(w, h, rgba)
    try:
        for line in (Path(pack) / 'index.txt').read_text().splitlines():
            parts = line.split()
            if len(parts) == 4 and parts[0] == k and parts[1] == str(w) and parts[2] == str(h):
                return Path(pack) / parts[3]
    except OSError:
        pass
    return None


def _crop(w, rgba, box, scale):
    """(width, height, RGBA) of box (in the original's texels) of a picture `scale` times larger."""
    x0, y0, x1, y1 = (v * scale for v in box)
    stride = w * 4
    return x1 - x0, y1 - y0, b''.join(rgba[y * stride + x0 * 4:y * stride + x1 * 4] for y in range(y0, y1))


def picture(w, h, rgba, upscaled=None, crop=None):
    """PNG bytes of the picture, or of its upscaled version (a pack file) when there is one, and
    the scale used; crop: (x0, y0, x1, y1) in the original's texels."""
    if upscaled:
        try:
            pw, ph, big = qoi(Path(upscaled).read_bytes())
            if pw % w == 0 and pw // w == ph // h:
                s = pw // w
                return png(*_crop(pw, big, crop, s)) if crop else png(pw, ph, big), s
        except (OSError, ValueError):
            pass
    return png(*_crop(w, rgba, crop, 1)) if crop else png(w, h, rgba), 1


if __name__ == '__main__':
    w, h, rgba, sprites = original(sys.argv[1], sys.argv[2])
    data, scale = picture(w, h, rgba, pack_file(sys.argv[4], w, h, rgba) if len(sys.argv) > 4 else None, CROP.get(sys.argv[2]))
    Path(sys.argv[3]).write_bytes(data)
    print(dict(w=w, h=h, scale=scale, sprites=sprites))
