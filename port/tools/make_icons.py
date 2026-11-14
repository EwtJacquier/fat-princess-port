"""The page's icons (web/favicon-32.png, web/apple-touch-icon.png): a slice of cake with a cherry,
drawn here (no game art), 8x larger than needed and scaled down.

    python3 -I tools/make_icons.py
"""
from pathlib import Path

from PIL import Image, ImageDraw

OUT = Path(__file__).resolve().parents[1] / 'web'
INK = (58, 31, 11, 255)


def cake(size):
    S = size * 8
    im = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    u = S / 100  # drawing units
    w = 3.2 * u  # outline
    P = lambda pts: [(x * u, y * u) for x, y in pts]
    # the slice, seen from the side: a wedge (point on the left), sponge, cream layer, frosting
    body = P([(8, 62), (88, 40), (92, 44), (92, 86), (12, 86), (8, 82)])
    d.polygon(body, fill=(243, 196, 104, 255))
    d.polygon(P([(10, 70), (90, 56), (90, 63), (10, 77)]), fill=(255, 246, 220, 255))   # cream between the layers
    d.polygon(P([(90, 70), (92, 70), (92, 78), (90, 78)]), fill=(140, 70, 24, 255))
    d.line(body + [body[0]], fill=INK, width=int(w), joint='curve')
    # frosting on top, with drips
    top = P([(6, 62), (86, 36), (95, 41), (95, 47), (88, 50), (84, 56), (80, 50), (66, 53), (61, 61), (56, 55), (40, 59), (34, 68),
             (29, 61), (14, 66), (6, 66)])
    d.polygon(top, fill=(255, 251, 238, 255))
    d.line(top + [top[0]], fill=INK, width=int(w * .8), joint='curve')
    # the cherry and its stem
    d.line(P([(63, 30), (66, 19), (74, 10)]), fill=INK, width=int(w * .9), joint='curve')
    cx, cy, r = 60 * u, 37 * u, 11 * u
    d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=(214, 40, 34, 255), outline=INK, width=int(w * .8))
    d.ellipse((cx - r * .55, cy - r * .6, cx - r * .05, cy - r * .1), fill=(255, 190, 180, 255))
    return im.resize((size, size), Image.LANCZOS)


if __name__ == '__main__':
    cake(32).save(OUT / 'favicon-32.png')
    icon = Image.new('RGBA', (180, 180), (253, 233, 170, 255))  # iOS shows the touch icon on a square: a warm parchment one
    icon.alpha_composite(cake(156), (12, 14))
    icon.convert('RGB').save(OUT / 'apple-touch-icon.png')
    print('ok')
