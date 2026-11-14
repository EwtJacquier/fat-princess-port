#!/usr/bin/env python3
"""The PSP system functions a decrypted EBOOT imports, as the recompiler wants them (imports.json):
each stub's address (relocated at the load base), library, NID, and the name, return and argument
kinds from PPSSPP's HLE tables ({NID, &Wrap..., "name", 'r', "args"}).

    python3 -I gen_imports.py EBOOT_decrypted.elf PPSSPP_SOURCE_DIR imports.json [BASE]
"""
import glob
import json
import re
import struct
import sys

BASE = 0x08804000


def main():
    elf, ppsspp, out = sys.argv[1], sys.argv[2], sys.argv[3]
    base = int(sys.argv[4], 16) if len(sys.argv) > 4 else BASE
    d = open(elf, 'rb').read()
    table = {}
    pat = re.compile(r'\{\s*0[xX]([0-9A-Fa-f]{8})\s*,[^"\n]*"([^"]+)"\s*,\s*\'(.)\'\s*,\s*"([^"]*)"')
    for f in glob.glob(ppsspp + '/Core/HLE/*.cpp'):
        for m in pat.finditer(open(f, errors='replace').read()):
            table.setdefault(int(m.group(1), 16), (m.group(2), m.group(3), m.group(4)))
    e_shoff, = struct.unpack_from('<I', d, 0x20)
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from('<HHH', d, 0x2E)
    secs = [struct.unpack_from('<IIIIII', d, e_shoff + i * e_shentsize) for i in range(e_shnum)]
    stro = secs[e_shstrndx][4]
    name_of = lambda nm: d[stro + nm:d.index(b'\0', stro + nm)].decode()

    def at(addr):
        for nm, typ, fl, a, off, size in secs:
            if a <= addr < a + size and off and typ != 8:  # not .bss
                return off + addr - a
        raise ValueError(hex(addr))

    mi = [s for s in secs if name_of(s[0]) == '.rodata.sceModuleInfo'][0]
    stub_top, stub_end = struct.unpack_from('<II', d, mi[4] + 32 + 12)
    imports, unknown = [], 0
    a = stub_top
    while a < stub_end:
        lname_p, ver, attr, entsz, nvar, nfunc, nids, funcs = struct.unpack_from('<IHHBBHII', d, at(a))
        lib = d[at(lname_p):d.index(b'\0', at(lname_p))].decode()
        for i in range(nfunc):
            nid = struct.unpack_from('<I', d, at(nids) + 4 * i)[0]
            name, ret, args = table.get(nid, (None, 'x', ''))
            if not name:
                name, unknown = '%s_%08X' % (lib, nid), unknown + 1
            imports.append({'addr': '%08x' % (base + funcs + 8 * i), 'lib': lib, 'nid': '%08X' % nid, 'name': name, 'ret': ret, 'args': args})
        a += entsz * 4
    json.dump(imports, open(out, 'w'), indent=1)
    print('%d imports (%d not in PPSSPP\'s tables) -> %s' % (len(imports), unknown, out))


if __name__ == '__main__':
    main()
