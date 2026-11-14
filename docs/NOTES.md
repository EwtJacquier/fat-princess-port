# Fat Princess: Fistful of Cake (PSP) — technical notes

## The disc
- UCUS-98740, "FAT PRINCESS: FISTFUL OF CAKE" (USA, v1.00, firmware 6.20), ISO 311 MB
  (`Fat Princess - Fistful of Cake (USA).iso`, SHA-1 8076e844870ba4169f07c5844a249b497bde2bc3).
- `PSP_GAME/SYSDIR/EBOOT.BIN` is encrypted (~PSP); `BOOT.BIN` is all zeros. Decrypted with
  `re/tools/prxdec` (PPSSPP's PrxDecrypter + libkirk, built from a PPSSPP source tree):
  `EBOOT_decrypted.elf`, 3,840,113 bytes, SHA-1 17fd9fad55e7b6e865e20ca4116f75b671c03acf, module
  "s_app_name", .text 0x2dc128 bytes (a mid-size PSP game).
- `USRDIR/FP.wad` (60 MB): "WAD0" archive (magic stored as `0DAW`), header 0x28 bytes, then
  entries of (name hash, offset, size), 200 at the top; nested WADs inside (152 found on a first
  walk; some nested tables don't parse yet with that layout). File kinds seen: ~2,950 "GTIF"
  (`46495447` read as bytes: the textures, presumably), 24 PNG, 24 "STRB" string banks (`BRTS`).
  Script code: `wad/scripts.wad`, `scripts/%s.o` (a "fusescript" engine, FSM scripts).
- `USRDIR/modules/*.prx`: SDK helper libraries (libfont, libdeflt, libsha*, libsfmt*, psmf,
  scan, show_macaddr, pspnet_ap_dialog_dummy...). The EBOOT imports none of libfont's functions.
- Music `streams/music/*.stm.psp` (17), voices `streams/vo/`, intro movie `movies/intro.pmf`.

## FP.wad, decoded (re/tools/wad.py)
- WAD0 header (0x28): magic, u16 2, u16 4, u32 entries, u32 data start, u32 alignment, u32 data
  size, u32 stored size, 3 x 0; entries (u32 name hash, u32 offset from the WAD's start, u32 size).
  Stored size < data size: zlib, decompressing in place after the header. 15,976 files in all.
- Name hash: CRC-32 without the final inversion (~zlib.crc32) of the LOWER-CASE path, e.g.
  "wad/ui_vram.wad", "images/fonts/fatfingers_26.fnt.psp". CRC is linear, so "name + suffix"
  hashes can be tested without the name.
- Textures: a header (a file, or inside a material/model): u32 2, u32 pixel CRC, w, h, bpp,
  then for CLUT ones (4/8 bpp) palette entries, 32, palette bytes, the RGBA8888 palette; then
  u32 1, u32 pixel CRC, u32 pixel bytes. 32-bit ones: 2, CRC, w, h, 32, 1, CRC, bytes. The pixels
  are a separate file found BY CONTENT: its ~CRC-32 equals the header's CRC. Stored GE-swizzled.
  1,245 distinct textures; every texture seen in a match decodes identically to the GE's.
- "GTIF" files (2,979) are scene/material data, not textures.

## Text: not baked into textures
- Strings live in the STRB banks as UTF-16LE, several languages (seen: Spanish), with inline codes
  for button icons and colors (`Pulsa /c4/b2/c0 para recoger sombreros y cambiar de clase.`).
- Drawn by the game with its own bitmap fonts: `images/fonts/%s.fnt.psp` (metrics) and pages
  `images/fonts/%s.img.%s` (glyph textures; per-language variants `%s_%c%c`), named fonts such as
  `title_font`, `footer_font`, `GetLargeFont`.
- English fonts: images/fonts/fatfingers_26.fnt.psp (body text, 512x128 8-bit page, key e19a3795)
  and fp_bold_30.fnt.psp (titles, 512x256 4-bit page, key ace9bafb); others for Russian/CJK.
  .fnt.psp: u16 glyph count, ..., u16 height at +6; from 0x12, 23-byte records: u16 code, f32
  u0 u1 v0 v1 (page-normalized), u32 advance, u8 flag; codes 0x80-0x8b are the button icons.
- Sharp text comes from the HD texture pack (no runtime code): its glyph pages have each glyph
  cut out of its coverage (bicubic 8x, 0.3-texel smoothing, threshold with a 1-pixel ramp, box
  down to 4x; glyph rectangles from the .fnt tables): the game's own lettering, sharp at any render
  scale. The button icons on those pages keep the AI upscale.

## Imports (what the runtime must provide)
sceCtrl 4, sceDisplay 6, sceDmac 1, sceGe_user 10, sceImpose 1, sceAtrac3plus 11, sceSasCore 27,
sceAudio 13, sceMpeg 25, sceUmdUser 7, scePower 4, sceRtc 1, sceUtility 21, ThreadManForUser 36,
IoFileMgrForUser 15, ModuleMgrForUser 7 (it loads the modules/ PRXs), SysMemUserForUser 6, ...
Network:
- Ad hoc: sceNetAdhoc = Init, Term, PdpCreate, PdpDelete, PdpSend, PdpRecv (datagrams only, no
  PTP); sceNetAdhocctl = Init, Term, Connect (a network by name), Disconnect, AddHandler,
  DelHandler. Lobby and discovery are the game's own, over PDP.
- Infrastructure (PSN, gone): sceNpMatching2 (rooms, signaling), sceNp*, sceNetInet (UDP/TCP
  sockets), sceNetResolver, sceHttp, sceSsl, sceNetUpnp, sceNetApctl.
- Netcode is host-based (a host with clients): "Framework Host", "HostMigrate",
  "str:msg_host_migrating", "ReturnToHostedRoom", "CreateHostLevelWaitScreen".

## The game (reviews)
- Players (menu_server_settings.csl, the host's options; texts from the string tables): Ad Hoc
  matches of 2, 4, 6, 8 characters, then "10/12/14/16 PLAYERS (8 HUMANS)" (str:player_N_8h): at
  most 8 humans, bots fill the rest. Infrastructure: "10/12 PLAYERS (6 HUMANS)", "14/16 PLAYERS
  (4 HUMANS)". NetGame.StartServer{mode, map, max_players = the option, num_humans}. Unused texts
  "10 PLAYERS" ... "16 PLAYERS" (no human cap) exist too.
- Modes: Rescue the Princess, Snatch 'n Grab, Team Deathmatch, Invasion, plus PSP-only Jailbreak,
  Demolition, Grim Reaper, Queen's Rule (names vary by source); story "Legend of the Fat Princess"
  (7 chapters). Classes come from hats (worker, warrior, ranger, mage, priest...).
- PPSSPP: ad hoc reported working with its built-in PRO ad hoc server (knulli.org guide).

## Boot and runtime findings (2026-11-14, the port running)
- Boots to the title, menus, tutorial and matches (Mess About, Black Forest) natively at 60 fps.
- The game waits for the UMD drive callback (registered with sceUmdRegisterUMDCallBack) before it
  reads anything: sceUmdActivate notifies it (PRESENT|READY|READABLE = 0x32), as on the PSP.
- File reads go through sceIoOpenAsync/ReadAsync/WaitAsyncCB (threads Async_Manager, WAD_Manager).
- Sound: SCREAM (Sony's sound library): sceSas (VAG and PCM voices) mixed into sceAudioOutput2
  ("SimpleAudio"); music is ATRAC3+ streams (.stm.psp) with sceAtracGetNextDecodePosition &co.
- Intro movie (movies/intro.pmf, the only one) uses the low-level sceMpeg API (not implemented
  yet: the game skips it).
- Rendering needs two things the runtime didn't have yet: the stencil (shadow volumes: z-fail incr/decr,
  then a darkening quad where stencil >= 1) and face culling (PSP cull bit 0 = cull GL_FRONT in our
  FBO orientation, as PPSSPP).
- About 30 VFPU forms the runtime didn't have yet (half floats, vcmov, vsin/vcos, vrsq, vmidt, vqmul...),
  checked against PPSSPP's interpreter.
- "PLAY WITH OTHERS" appears in the main menu only after the online user agreement (shown after
  the title) is scrolled to the end and accepted. Then: Infrastructure (fails cleanly: "Connection
  to the PlayStation Network was cancelled") or Ad-Hoc: Jump In! / Find a Game (join),
  Control Freaks > Start Server (host: lobby with a 60 s countdown, bots fill the teams).
- Ad hoc: sceNetAdhocctlConnect with an empty group name, one PDP socket on port 0x2222. A
  searching client broadcasts one query (71 B); hosts answer by unicast. A lone host sends nothing.
- Frame loop (smooth motion): the main thread ("user_main") ticks the game clock once a frame
  (FUN_089bc4dc: ms from sceKernelGetSystemTimeWide, step = the difference, capped and scaled,
  everything moves by it), then waits in FUN_08977864 (CpuGpuSync flag 0x20) for the "swap thread"
  (FUN_08977924), which waits for the frame's end (0x10) and the GE (0x1), sets the framebuffer
  (sync 0, no vblank wait of its own), enqueues the list and sets 0x20. On the PSP the GE's drawing
  time spaces all this; here: (1) display.c paces busy frames to one per vblank, (2) mods/frameclock.c
  gives the clock tick the vblank of the frame on the screen when the frame began (its step is
  exactly the time between frames on the screen: 16/17 ms; it was 3-30 ms, the camera wobbled and the
  player shook), (3) in the browser the vblanks follow the display's refresh (platform_web.c), else
  two clocks drift and some refreshes show two frames' worth or none. FP_FRAME_CLOCK=0, FP_VSYNC=0,
  FP_FRAME_PACE=0 turn each off; FP_FPS=1 logs vblanks/frames and, in the browser, the refreshes.
- Glow post effect: its blur buffers (0x174000, 0x17c000, 128x272) are cleared by untextured draws
  while their own texture is still bound; WebGL refuses such draws ("feedback loop"), so ge.c binds a
  blank texture for untextured draws then (FP_GE_FEEDBACK=1 logs them).

## Traffic, measured (two native copies, host + 1 client, 16-player match = 2 humans + 14 bots)
| phase | host out | client out |
| --- | --- | --- |
| join burst (~5 s) | 9.6 msg/s, 6 KB/s (messages up to 1245 B) | 3.8 msg/s, 130 B/s |
| lobby | ~3.4 msg/s, ~160 B/s | ~2.5 msg/s, ~60 B/s |
| match | ~14 msg/s, 2-3 KB/s per client (messages up to ~420 B) | ~10 msg/s, ~200 B/s (23-35 B) |
- All unicast (no broadcasts after discovery). The host sends world state at ~14 Hz, not 60.
- Extrapolated to 8 humans: host upload ~7 x 2.5 KB/s = ~18 KB/s (~140 kbit/s), ~100 msg/s;
  each client ~3 KB/s down. Small for LiveKit data channels; use lossy (unreliable) delivery.
- FP_NETSTATS=5 prints these numbers every 5 s. FP_PDP_PACE=1 brings back the
  one-datagram-per-sender-per-frame pacing (off here: the host sends several per frame).

## Traffic over LiveKit
- PDP is UDP-like: the transport should use LiveKit's lossy (unreliable, unordered) data, not the
  reliable one a turn-based game could afford, or a lost packet stalls the rest.
- Host-based star: each client sends its input/state to the host; the host sends the world to every
  client (7 copies with 8 players): the host's upload is the bottleneck.

## Scripts (CSL) and the mods
- The game's flow (boot screens, title, menus, save dialogs) is compiled script: CSL V0.7.1 by Game
  Toolworks ("FITG"/"MLSC" images, version float 0.35) in FP.wad's scripts archive, ~950 of them,
  each naming its source (c:\projects_fp\fp\data\scripts\NAME.csl). `re/tools/csldis.py`
  disassembles one: tables of identifiers, strings, ints (self-relative pointers), function records
  (13 words, +0x30 -> u32 length + code). Instruction: byte < 0x80 = opcode without operand (0x09
  nop, 0x0a return, 0x0c pop, 0x0e/0x0f call with/without result, argc on top); byte >= 0x80 =
  opcode (low 5 bits) with a signed operand (1 byte, +1 with bit 0x20, +2 with bit 0x40): 0x04 jmp,
  0x05/0x06 jf/jt, 0x13 identifier, 0x14 string, 0x15 global, 0x16 int. Calls: `global f; nil;
  int argc; callv`, methods `global m; getmember2 name; args; int argc; callv`. Interpreter
  FUN_08a8252c, image loader FUN_08a8963c (this+0x14 = the MLSC block).
- Natives are a table of (name, function) per module, e.g. util at 0x08b01894: LoadSaveData
  FUN_08924ed0, AutoSave FUN_0892536c, HandleAutoLoadResult FUN_089257d0, SystemDialogShowing.
- Text: string tables "BRTS" (strings/<name><language>.sf), entries (hash, offset) sorted by
  hash = ~crc32("str:<id>"), UTF-16; lookup FUN_089fdb34(table, hash). English menus:
  str:play_with_yourself, str:play, str:twiddly_knobs, str:legend, str:mess_about, str:gladiate,
  str:get_fabulous, str:confused (How To Play!), str:option_credits_button.
- Boot: startup.csl startup_screen_sequence -> epilepsy -> ESRB -> Sony (-> SMS in Japan) ->
  play_intro_movie; the engine runs load_game_assets, which on the first run shows
  fp_ui_messages.ShowBootupMemoryStickWarning (Yes = load_save_data = util.LoadSaveData; No = "disable
  saving?"). The load's result (FUN_089257d0) creates the title. pressed_start: begin_menus when
  agreed_to_eula, else show_eula. A memory stick without a save gets one only at the first
  util.AutoSave (settings, customization, pause menu).
- Mods (port/mods): scripts.c patches instructions as images load (same length; checks the
  original bytes): boot = no logos, the save loaded without the dialog, Start accepts the agreement;
  menu = 3 main options (no LOAD GAME), PLAY ONLINE straight to ad hoc (FIND A GAME / HOST A GAME,
  no JUMP IN!), network errors back to the main menu, no copyright line under the menus, and the
  options changed with left/right shown as "< VALUE >": the vmenu (menu_fp_vmenu.csl) asks for their
  text as "_mid" .. name instead of "str:" .. name; strings.c's hook of the text manager
  (FUN_089fe1d4) finds the "str:" id again from the CRC (same bytes after another start: the
  difference only depends on the name's length, 1 to 64 tried) and returns "< TEXT >". save.c: a boot without a save makes one (what util.AutoSave does). strings.c: the
  menus' new names (English tables only). FP_MOD_BOOT=0 / FP_MOD_MENU=0 keep the original.
- Headless tests with the mods: title at frame ~270; "400:start" -> main menu; "500:cross" PLAY
  OFFLINE (STORY / CASUAL / ARENA MODE); casual match: "400:start,500:cross,600:down,660:cross,
  780:right,840:cross,960-3000/120:cross". Environment switches are FP_* (FP_INPUT, FP_DUMP,
  FP_DUMP_FULL, FP_MS0, FP_SCALE, FP_TEXTURES, FP_NETSTATS...).
