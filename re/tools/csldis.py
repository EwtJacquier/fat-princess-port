#!/usr/bin/env python3
"""Disassembler for the game's compiled scripts (CSL V0.7.1 by Game Toolworks; images "FITG"/"MLSC").

Image (after the 8-byte FITG header): "MLSC", size, f32 0.35, 3 x u32, u32 hash, then four
self-relative pointers to tables (each: u32 count, entries): idents (self-relative pointers to
names), strings (same), ints (u32), 64-bit constants. At +0x2c the module: u32, a pointer to the
function list, a pointer to a second list. A function record is 13 words: +4 index, +8 number of
parameters, ..., +0x28 pointer to a word, +0x30 pointer to its code (u32 length, bytes).

Instructions: a byte below 0x80 is an opcode without operand (some read one more byte); a byte
with bit 7 set carries an operand: one signed byte, two bytes (big-endian) with bit 5, two more
with bit 6; its opcode is the low 5 bits.

    python3 -I re/tools/csldis.py SCRIPT.csl [FUNCTION]
"""
import struct
import sys

OPS = {  # operand opcodes (byte & 0x1f), from the interpreter FUN_08a8252c
    0x00: 'getmember', 0x01: 'getmember1', 0x02: 'getmember2', 0x03: 'getmember3', 0x04: 'jmp', 0x05: 'jf',
    0x06: 'jt', 0x07: 'and_jmp', 0x08: 'or_jmp', 0x09: 'op09', 0x0a: 'foreach', 0x0b: 'foreach_next',
    0x0c: 'for', 0x0d: 'for_next', 0x0e: 'closure', 0x0f: 'closure_local', 0x10: 'local_nil',
    0x11: 'name', 0x12: 'import', 0x13: 'ident', 0x14: 'str', 0x15: 'global', 0x16: 'int',
    0x17: 'intc', 0x18: 'float', 0x19: 'floatc', 0x1a: 'enum', 0x1b: 'enumc',
}
EXTRA_BYTE_OPERAND = {0x0a, 0x0b, 0x0c, 0x0d, 0x0f, 0x10, 0x11, 0x12}  # one more byte after the operand
SIMPLE = {  # opcodes without operand: name, extra bytes
    0x01: ('bnot', 0), 0x02: ('op02', 0), 0x03: ('op03', 0), 0x04: ('not', 0), 0x05: ('land', 0),
    0x06: ('lor', 0), 0x07: ('neg', 0), 0x08: ('op08', 0), 0x09: ('nop', 0), 0x0a: ('op0a', 0),
    0x0b: ('dup', 0), 0x0c: ('pop', 0), 0x0d: ('ret', 0), 0x0e: ('call', 0), 0x0f: ('callv', 0),
    0x10: ('tset', 0), 0x11: ('nil', 0), 0x12: ('false', 0), 0x13: ('true', 0), 0x14: ('local', 1),
    0x15: ('arg', 1), 0x16: ('self', 0), 0x17: ('bool', 0), 0x4c: ('op4c', 1), 0x50: ('op50', 1),
    0x51: ('op51', 1), 0x52: ('op52', 1), 0x53: ('op53', 1), 0x55: ('op55', 1), 0x57: ('op57', 1),
}


class Image:
    def __init__(self, data):
        self.f = data
        assert data[:4] == b'FITG' and data[8:12] == b'MLSC', 'not a CSL image'
        M = 8
        self.idents = self._names(self._rel(M + 0x1c))
        self.strings = self._names(self._rel(M + 0x20))
        o = self._rel(M + 0x24)
        self.ints = [self.u(o + 4 + 4 * i) for i in range(self.u(o))] if o else []
        P = M + 0x2c
        fo = self._rel(P + 4)
        self.funcs = []
        for i in range(self.u(fo)):
            r = self._rel(fo + 4 + 4 * i)
            code = self._rel(r + 0x30)
            if code is None:  # a function without code here (declared only)
                continue
            n = self.u(code)
            self.funcs.append(dict(rec=r, index=self.u(r + 4), nparams=self.u(r + 8), code=code + 4, size=n,
                                   words=[self.i(r + k) for k in range(0, 0x34, 4)]))

    def u(self, o):
        return struct.unpack_from('<I', self.f, o)[0]

    def i(self, o):
        return struct.unpack_from('<i', self.f, o)[0]

    def _rel(self, o):
        v = self.i(o)
        return o + v if v else None

    def cstr(self, o):
        e = self.f.find(b'\0', o)
        return self.f[o:e].decode('latin1')

    def _names(self, o):
        if not o:
            return []
        return [self.cstr(o + 4 + 4 * k + self.i(o + 4 + 4 * k)) for k in range(self.u(o))]

    def decode(self, fn):
        """(offset in the file, length, mnemonic, operand text) for each instruction of a function."""
        f, p, end = self.f, fn['code'], fn['code'] + fn['size']
        while p < end:
            b = f[p]
            if b & 0x80:
                q = p + 1
                v = struct.unpack_from('b', f, q)[0]
                q += 1
                if b & 0x20:
                    v = (v << 8) | f[q]
                    q += 1
                if b & 0x40:
                    v = (v << 16) | (f[q] << 8) | f[q + 1]
                    q += 2
                op = b & 0x1f
                extra = None
                if op in EXTRA_BYTE_OPERAND:
                    extra = f[q]
                    q += 1
                name = OPS.get(op, f'op{op:02x}')
                if op in (0, 1, 2, 3, 0x11, 0x12, 0x13, 0x15):
                    txt = self.idents[v] if 0 <= v < len(self.idents) else f'#{v}'
                elif op == 0x14:
                    txt = repr(self.strings[v]) if 0 <= v < len(self.strings) else f'#{v}'
                elif op in (4, 5, 6, 7, 8, 0x0a, 0x0b, 0x0c, 0x0d):
                    txt = f'-> {q + v - fn["code"]:#x}'
                elif op == 0x17:
                    txt = str(self.ints[v]) if 0 <= v < len(self.ints) else f'#{v}'
                elif op == 0x18:
                    txt = str(struct.unpack('<f', struct.pack('<i', v))[0]) if b & 0x40 else str(v)
                else:
                    txt = str(v)
                if extra is not None:
                    txt += f' [{extra}]'
                yield p, q - p, name, txt
                p = q
            else:
                name, n = SIMPLE.get(b, (f'op{b:02x}', 0))
                txt = ' '.join(str(x) for x in f[p + 1:p + 1 + n])
                yield p, 1 + n, name, txt
                p += 1 + n


def main():
    img = Image(open(sys.argv[1], 'rb').read())
    want = sys.argv[2] if len(sys.argv) > 2 else None
    for fn in img.funcs:
        name = img.idents[fn['index']] if fn['index'] < len(img.idents) else f'#{fn["index"]}'
        if want and name != want:
            continue
        print(f'== {name} (params {fn["nparams"]}, code {fn["code"]:#x}+{fn["size"]:#x}) {fn["words"]}')
        for p, n, m, t in img.decode(fn):
            print(f'  {p - fn["code"]:04x}  {img.f[p:p + n].hex():12s} {m:10s} {t}')


if __name__ == '__main__':
    main()
