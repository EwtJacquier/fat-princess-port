# Fat Princess: Fistful of Cake (UCUS-98740) — reverse engineering

Everything the port needs from here is already in `export/`: you only need this folder to redo the
analysis, or to look into the game. Addresses are always in the PSP's runtime form (base
`0x08804000`): the same in Ghidra, in PPSSPP's debugger and in the port (`f_08977864` is the
function at `0x08977864`).

## Files

| Path | What it is |
|---|---|
| `../EBOOT_decrypted.elf` | the decrypted EBOOT.BIN (MIPS/Allegrex module), yours, never committed |
| `export/program.json` | where every function and jump table is: what the recompiler reads |
| `export/imports.json` | the PSP system functions the game imports (NID → name, from PPSSPP's tables) |
| `export/fixups.json` | what Ghidra's analysis missed: functions reached only through vtables, jump tables' cases |
| `export/vfpu_mnemonics.txt` | the VFPU instructions found in the program |
| `export/wad_names.json` | names of FP.wad's files (the archive keeps only their hashes) |
| `ghidra_scripts/` | the scripts that made and use the analysis (export, fixups, decompile) |
| `ghidra_project/` | the Ghidra project `analyze.sh` makes (not committed) |
| `tools/wad.py` | reads FP.wad, the game's archive of archives (`python3 -I tools/wad.py ISO`) |
| `tools/csldis.py` | disassembles the game's compiled scripts (CSL by Game Toolworks) |
| `tools/decomp.sh` | prints Ghidra's pseudo-C for the functions at some addresses |
| `tools/gen_imports.py` | makes `imports.json` from the EBOOT and PPSSPP's source tree |
| `tools/prxdec/` | decrypts EBOOT.BIN with PPSSPP's decrypter (`PPSSPP=path/to/ppsspp build.sh`) |

## Redoing the analysis

It needs [Ghidra](https://ghidra-sre.org) 12.2 with the
[ghidra-allegrex](https://github.com/kotcrab/ghidra-allegrex) extension (the PSP's CPU and its
ELF loader) and a JDK 21 or newer. The scripts look for them in `~/tools/` (`ghidra_12.2_PUBLIC`,
`jdk-25.0.5+8`); point them elsewhere with `GHIDRA=...` and `JAVA_HOME=...`.

```bash
re/analyze.sh                # import + analyze EBOOT_decrypted.elf, export program.json (a few minutes)
port/tools/rebuild.sh        # apply export/fixups.json, export again, translate to C and build
re/tools/decomp.sh 08977864  # Ghidra's pseudo-C of a function (read-only)
```

## Findings

`../docs/NOTES.md` has the notes: the disc and FP.wad's format, the textures, the text and its
fonts, the imports, the frame loop, the network messages and their traffic, the scripts (CSL) and
how the mods change them.
