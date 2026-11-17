# Fat Princess: Fistful of Cake — PC & browser port

If you enjoy this port and feel like it, you can [buy me a coffee](https://buymeacoffee.com/ewtjacquier). Thank you!

A fan-made port of **Fat Princess: Fistful of Cake** (PSP, 2010) that runs natively on Linux and
**in the browser**, at high resolution, with HD textures, accounts for each player and **online
matches with friends** (the game's Ad Hoc mode). It is not an emulator: the game's own program was
translated, function by function, into C code that compiles for your PC or for WebAssembly.

> **Bring your own game.** This repository has no part of the game: no ISO, no game code, no game
> files (only a wallpaper of the game, with its logo, for the sign-in page). You build the port
> from your own copy, dumped from your own UMD. Not affiliated with Sony Interactive Entertainment,
> Fun Bits Interactive, Titan Studios or anyone who owns Fat Princess.

Made by fans, for fans: to keep a game we love alive, playable on today's machines and in a
browser tab, and to storm each other's castles again.

## Contents

1. [What you get](#what-you-get)
2. [How it works](#how-it-works)
3. [Technologies](#technologies)
4. [What you need](#what-you-need)
5. [Step by step](#step-by-step)
   1. [Get the code](#1-get-the-code)
   2. [Get the decrypted EBOOT](#2-get-the-decrypted-eboot)
   3. [Translate the game to C](#3-translate-the-game-to-c)
   4. [Get FFmpeg](#4-get-ffmpeg)
   5. [Build and play on Linux](#5-build-and-play-on-linux)
   6. [Build and play in the browser](#6-build-and-play-in-the-browser)
   7. [HD textures](#7-hd-textures)
6. [Online play: Ad Hoc only, no game server](#online-play-ad-hoc-only-no-game-server)
7. [Sharing it with friends (Cloudflare Tunnel)](#sharing-it-with-friends-cloudflare-tunnel)
8. [Accounts and saves: where everything is kept](#accounts-and-saves-where-everything-is-kept)
9. [Things to know](#things-to-know)
10. [Repository layout](#repository-layout)
11. [Credits](#credits)

## What you get

- **The whole game**, translated to native code: the story (Legend of the Fat Princess), the
  casual matches and the arena against the computer, every map, class and hat, customization,
  music and voices.
- **4x internal resolution** (1920x1088) at a smooth 60 frames per second: the camera follows the
  running princess-snatchers without a jolt.
- **HD textures**: every one of the game's 1,245 textures was remade at 4x, most with the
  [Real-ESRGAN](https://github.com/xinntao/Real-ESRGAN) anime model; the menu art, icons, the HUD
  hearts and others where that fell short were redrawn with OpenAI's image models. Each one was
  checked against its original. The game's lettering is redrawn sharp: its glyph pages were cut out again
  with clean edges at 4x, so menus, tips and the HUD keep the game's own fonts at any resolution.
  They come as a separate download and never change the game's files: turn them off and on in the
  settings menu.
- **A browser version**: the game in a tab, with a settings menu (Esc): fullscreen, volume,
  keyboard keys, gamepad buttons and which stick is the PSP's analog stick, internal resolution, HD
  textures, how the original textures are filtered.
- **Players**: each player logs in and keeps their own save; their name is their name online.
- **Online matches** through the game's **Ad Hoc** mode: one player hosts, the others join.
  **Up to 16 people** per match (the original game stopped at 8: "16 PLAYERS (8 HUMANS)"; the
  players mod lifts it), bots fill the places nobody takes. There is no game server: see
  [Online play](#online-play-ad-hoc-only-no-game-server).
- **Mods** (C code in `port/mods/`, each can be turned off):
  - straight to the title: no logos, no notices; the save loads by itself (or is made, the first
    time) and autosave stays on; Start on the title accepts the online agreement; the game's tips
    (the hint balloons during a match too) start off on every boot (Settings turns them on);
  - clearer menus: **PLAY OFFLINE** / **PLAY ONLINE** / **CUSTOMIZE** (no LOAD GAME: the save is
    already loaded); STORY MODE / CASUAL MODE / ARENA MODE; CHANGE LOOK / SETTINGS / HOW TO PLAY /
    CREDITS; PLAY ONLINE goes straight to the Ad Hoc menu: **HOST A GAME** / **FIND A GAME**;
    options changed with left and right show it: `< GAME TIPS OFF >`; the analog stick moves through the
    menus too; no copyright line under them;
  - **16 people online**: a match of 10 to 16 places takes that many people, not 8 (the host also
    lets those joining in one a second instead of one every 5 seconds, so a crowd arriving
    together all gets in); the size options read `< 16 PLAYERS >`;
  - **steadier online**: a player whose messages stop arriving is dropped after 15 seconds of
    silence, not 4 (a connection that hiccups and comes back stays in the match);
  - smooth motion: the game's clock steps exactly with the frames on the screen (see
    [Things to know](#things-to-know)).

## How it works

The PSP runs MIPS machine code. Emulators interpret it (or compile it on the fly) while the game
runs. This port does it once, ahead of time: **static recompilation**.

```
your UMD ─▶ ISO ─▶ EBOOT.BIN (encrypted) ─▶ EBOOT_decrypted.elf
                                              │
           re/export/program.json ────────────┤  where each function and jump table is
           (made once with Ghidra)            │  (addresses only: no game code)
                                              ▼
                            tools/prx_load.py + tools/recomp.py
                                              │
                                              ▼
                  port/generated/*.c  ── about 9,150 functions as C code (stays on your PC)
                                              │
              port/runtime/*.c  ──────────────┤  the PSP's system, rewritten: threads, files,
              port/mods/*.c                   │  graphics, sound, saves, network
                                              ▼
                        native binary (GCC)  or  WebAssembly (Emscripten)
```

- Each MIPS function becomes a C function; registers live in a struct, the PSP's memory is one
  block of memory, branches become `goto`s. The VFPU (the PSP's vector unit) is translated inline.
- The PSP system functions the game calls are implemented by this project's runtime (nothing comes
  from an emulator): graphics go to OpenGL ES 3 / WebGL 2, sound to SDL or WebAudio, the game's
  files are read straight from your ISO.
- Mods replace game functions by address (`port/hooks.txt`) with hand-written C that can still
  call the original; the menu and boot mods also patch the game's own scripts as they load (Game
  Toolworks' CSL, checked byte by byte before any change).

`docs/NOTES.md` has the technical notes: the disc, `FP.wad`, the scripts, the frame loop, the
network traffic.

## Technologies

| What | Used for |
|---|---|
| **C** (GNU C11), **GCC**, **Make** | the generated game code, the runtime, the mods |
| **Python 3** | the recompiler, the build tools, the local web server (standard library only) |
| **SDL2** | window, input and sound in the native build |
| **OpenGL ES 3 / WebGL 2** | the PSP's graphics chip, rendered at 4x |
| **Emscripten** (WebAssembly, Asyncify, SIMD) | the browser build; the PSP's threads become fibers |
| **FFmpeg** (PPSSPP's builds) | ATRAC3+ music and voices |
| **LiveKit** (WebRTC) | online play: the game's Ad Hoc messages between players |
| **Ghidra** (optional) | the analysis that found every function (already done: `re/export/`) |
| **Real-ESRGAN**, **OpenAI image models** | how the HD textures were made (`realesrgan-x4plus-anime` at 4x; `gpt-image` redraws) |

## What you need

**The game**: *Fat Princess: Fistful of Cake* (USA), **UCUS-98740**, as an ISO dumped from your own
UMD (with a PSP running custom firmware, for example).

**To build** (Linux; on Windows use WSL2 with Ubuntu):

```bash
sudo apt install build-essential pkg-config git python3 libsdl2-dev libgles-dev
```

- For the browser build, the **Emscripten SDK** (tested with 3.1.46):
  ```bash
  git clone https://github.com/emscripten-core/emsdk ~/emsdk
  cd ~/emsdk && ./emsdk install 3.1.46 && ./emsdk activate 3.1.46
  export EMSDK=~/emsdk          # the build scripts use it (or: source ~/emsdk/emsdk_env.sh)
  ```
- About **8 GB of RAM** and **2 GB of disk** for building (plus ~400 MB for the HD textures).

**To play**: Linux for the native build; for the browser build, any desktop browser with WebGL 2
(Chrome, Edge, Firefox) on any system. A gamepad is optional.

## Step by step

### 1. Get the code

```bash
git clone https://github.com/<you>/fat-princess-port.git
```

Put your ISO **next to** the `fat-princess-port` folder, named
`Fat Princess - Fistful of Cake (USA).iso` (or pass its path to the commands below):

```
somewhere/
├── Fat Princess - Fistful of Cake (USA).iso
└── fat-princess-port/
    ├── EBOOT_decrypted.elf      ← step 2
    ├── port/
    └── re/
```

### 2. Get the decrypted EBOOT

The game's program is `PSP_GAME/SYSDIR/EBOOT.BIN` inside the ISO, and it is encrypted. Ways to
decrypt it:

- **With [pspdecrypt](https://github.com/John-K/pspdecrypt)**: take `EBOOT.BIN` out of the ISO
  (any ISO tool, e.g. `7z x "Fat Princess - Fistful of Cake (USA).iso" PSP_GAME/SYSDIR/EBOOT.BIN`),
  then `pspdecrypt EBOOT.BIN` writes the decrypted file.
- **With [PPSSPP](https://www.ppsspp.org)**: in *Settings → Tools → Developer tools*, turn on the
  option that dumps the decrypted EBOOT when a game boots, then start the game once; the file
  lands in PPSSPP's memory stick folder, under `PSP/SYSTEM/DUMP/`.
- **With `re/tools/prxdec`**: PPSSPP's own decrypter built into a small command line tool
  (`PPSSPP=path/to/ppsspp-source re/tools/prxdec/build.sh`, then
  `re/tools/prxdec/prxdec EBOOT.BIN EBOOT_decrypted.elf`).

Save it as `EBOOT_decrypted.elf` in the `fat-princess-port` folder (git ignores it). Its SHA-1
must be `17fd9fad55e7b6e865e20ca4116f75b671c03acf` (step 3 checks it).

### 3. Translate the game to C

```bash
cd fat-princess-port/port
tools/generate.sh            # or: tools/generate.sh path/to/EBOOT_decrypted.elf
```

This is the "decompiling" part, done for you: it loads the program the way the PSP's kernel
would (`tools/prx_load.py`: relocations, imports) and translates every function to C
(`tools/recomp.py`), using the map of the program in `re/export/program.json`. It takes about a
minute and writes `port/generated/` (about 30 MB of C). **That folder is the game's code: it is
yours to build, never commit or share it.**

Redoing the analysis yourself is optional: it needs Ghidra, see `re/README.md`.

### 4. Get FFmpeg

```bash
tools/get_ffmpeg.sh
```

It downloads [PPSSPP's FFmpeg builds](https://github.com/hrydgard/ppsspp-ffmpeg) (only the
decoding libraries), uses the prebuilt ones for Linux and builds the WebAssembly ones with
Emscripten (a few minutes, once).

### 5. Build and play on Linux

```bash
make -j8
./build/fatprincess "../../Fat Princess - Fistful of Cake (USA).iso"
```

The first build takes a while (9,150 functions; `-j8` is fine with 16 GB of RAM, use fewer jobs
with less). Keys: **arrows** = the analog stick (walking, and the menus), **I J K L** = the d-pad
(the helpers' commands and the taunts), **Z** = ✕, **X** = ◯, **A** = □, **S** = △,
**Q**/**W** = L/R, **Enter** = Start, **Backspace** = Select; USB gamepads work too. Saves go to
`build/ms0/PSP/SAVEDATA`.

### 6. Build and play in the browser

```bash
tools/build_web.sh                 # writes build/web/
python3 tools/serve_web.py         # http://127.0.0.1:8643/
```

Open **http://127.0.0.1:8643/**. The server gives the page the ISO (only the parts the game
reads), keeps the saves in the project folder and handles the players' accounts.

- **Log in** or create a player (a new one starts from a copy of the main save), or "Play without
  an account" (the main save). Each browser *window* is one player: two windows, two players. The
  sign-in screen shows the game's art, read from your ISO.
- **Esc** (or the gear in the corner, or Home on a gamepad) opens the settings: fullscreen (also
  F11 or a double click), the volume, internal resolution (1x to 4x), how the original textures
  are filtered (smooth, sharp or pixels), HD textures, keys, gamepad buttons and which stick is
  the PSP's analog stick (the other one works as the d-pad).
- Opened from another computer (a friend, through a tunnel), the page first copies the game into
  that browser: the ISO and the HD textures, about 700 MB once, with a progress bar, so the game
  never waits on the network after that (`?preload=0` in the address skips it).
- Without the server (the page opened some other way) the page asks for the ISO file and keeps
  the save in the browser.

### 7. HD textures

Download the texture pack (the `fatprincess-hd-textures*.zip` file on this repository's Releases
page) and install it, from `port/`:

```bash
python3 tools/textures.py install ~/Downloads/fatprincess-hd-textures*.zip
python3 tools/textures.py status
```

Every texture of the game was upscaled 4x and compared with its original. Most (1,081) come from
Real-ESRGAN's anime model (`realesrgan-x4plus-anime`); 162 (menu art, icons, the HUD hearts and
others where the plain upscale fell short) were redrawn with OpenAI's image models (`gpt-image`),
fitted back onto the original's place and outline and reviewed one by one. The font pages were
redone with clean edges instead (an AI upscale blurs thin letters): the lettering stays the game's
own, sharp. The pack only **adds** files in `build/textures`: the game looks each
texture up while it runs and draws the HD version in its place, so your ISO and the game's files
never change.

**Going back to the originals** (the rollback):

- in the browser, Esc → **HD textures: Off** (instant, and back On whenever you like);
- or remove the pack: `python3 tools/textures.py uninstall`.

No copy of the original textures is needed for that: they always come from your ISO.

## Online play: Ad Hoc only, no game server

**What works**: the game's **Ad Hoc** mode, the one PSPs played side by side. One player opens
**PLAY ONLINE → HOST A GAME** and starts the server *in their own game* (the match's host: its
lobby, its countdown, bots for the empty places); the others open **PLAY ONLINE → FIND A GAME**
and join it.

**How many**: up to **16 people** per match. The host picks a match of 2 to 16 places; people take
them as they join and bots take the rest. The original game stopped at 8 people ("16 PLAYERS (8
HUMANS)"; its PlayStation Network mode at 4 to 6 in big matches); the players mod lifts that (with
`FP_MOD_PLAYERS=0` the game keeps its 8). Tested with 16 copies of the game, all of them in the
same match. The host's game sends the most: a few KB per second per player. A player with a poor
connection no longer slows everyone else down: tested with four players, one of them on a very
bad link (a quarter of a second late, 10% of the messages lost), and every game held 60 frames a
second.

**What doesn't**: there is **no game server** anywhere. The game's *Infrastructure* mode (matches
through the PlayStation Network) needed Sony's servers, which are gone; the menu mod goes straight
to Ad Hoc and never shows it. Nobody runs a server for this port, and it doesn't need one: the
host player's own game is the match's server, exactly as on the PSP.

**How the players reach each other**: Ad Hoc is Wi-Fi between consoles in the same room. The port
emulates that network (`port/runtime/net.c`) and, in the browser, carries its messages through a
**LiveKit room**: LiveKit is a service for real-time apps (WebRTC); here it is only the "Wi-Fi",
passing the game's small messages along (the host sends about 3 KB/s to each player, the others
far less), so its **free plan** is plenty.

1. Create a free account at **[cloud.livekit.io](https://cloud.livekit.io)** and a project.
2. In the project's settings, create an **API key**. Note the three values: the project's
   **URL** (`wss://<your-project>.livekit.cloud`), the **API Key** and the **API Secret**.
3. Write them in `port/.env` (the file is ignored by git: never commit it):
   ```ini
   LIVEKIT_URL=wss://your-project.livekit.cloud
   LIVEKIT_API_KEY=<your API key>
   LIVEKIT_API_SECRET=<your API secret>
   ```
4. Restart `serve_web.py`. The login screen shows **Network play: ready**.
5. In the game: **PLAY ONLINE**, one player **HOST A GAME**, the others **FIND A GAME**.

How it fits together: the page asks *your* server for a pass (`/livekit-token`), and your server
signs it with the secret, which never leaves your PC. Everyone who opens the game from the same
server meets in the same room (`?FP_ROOM=name` in the address makes a separate one). To try it
alone, open two browser windows and log in as two players.

## Sharing it with friends (Cloudflare Tunnel)

To play with friends who are not on your network, one person (**the host of the page**) runs the
server and shares it through a **[Cloudflare Tunnel](https://developers.cloudflare.com/cloudflare-one/connections/connect-networks/)**:
a secure HTTPS address for your local server, with no router setup.

What happens then: your friends open the address; their browser loads the game from your server
and runs it **on their own computer**; their saves and accounts live on yours; online matches go
through LiveKit with passes your server signs (they need no LiveKit account).

**Quick test** (a temporary random address):

```bash
# install cloudflared: https://developers.cloudflare.com/cloudflare-one/connections/connect-networks/downloads/
cloudflared tunnel --url http://127.0.0.1:8643
```

It prints an address like `https://something-random.trycloudflare.com`: send it to your friends.
It lasts while the command runs.

**Your own address, open only to your friends** (free, needs a domain on Cloudflare):

```bash
cloudflared tunnel login                                         # pick your domain
cloudflared tunnel create fatprincess                            # prints the tunnel's id
cloudflared tunnel route dns fatprincess fatprincess.your-domain.com
```

`~/.cloudflared/config.yml`:

```yaml
tunnel: fatprincess
credentials-file: /home/<you>/.cloudflared/<tunnel-id>.json
ingress:
  - hostname: fatprincess.your-domain.com
    service: http://127.0.0.1:8643
  - service: http_status:404
```

```bash
cloudflared tunnel run fatprincess
```

Then, in Cloudflare's dashboard, **Zero Trust → Access → Applications → Add a self-hosted
application** for `fatprincess.your-domain.com`, with a policy that allows your friends' e-mail
addresses: each one logs in with a code sent to their e-mail before the game opens.

**Please read this before sharing:**

- Whoever opens the address downloads the game's data from **your** ISO and the translated game
  code. Share it **only with friends who own the game**, never publicly: use the Cloudflare Access
  policy above for anything longer than a quick test.
- Anyone who reaches the page can create a player on your server: another reason for Access.
- The game streams from your PC: your upload speed is everyone's first loading time (about 700 MB
  per friend, once; after that their browser keeps it).

## Accounts and saves: where everything is kept

There is no database server: everything is plain files in your `port/build/` folder, written by
`serve_web.py` (and by the native build, for the main save).

| What | Where |
|---|---|
| Main save (native build, and "Play without an account") | `build/ms0/PSP/SAVEDATA/` |
| A player's account (name, password hash) | `build/profiles/<name>/account.json` |
| A player's save | `build/profiles/<name>/ms0/PSP/SAVEDATA/` |
| Logged-in sessions | `build/profiles/sessions.json` |
| HD textures | `build/textures/` |
| Settings of the page (keys, gamepad, switches) | the browser's `localStorage` |
| The game copied into a friend's browser | that browser's Cache Storage (site data) |
| Saves when the page runs without the server | the browser's IndexedDB |

- Passwords are never stored: only a salted **PBKDF2-SHA256** hash (200,000 rounds).
- Every time a save is written, the previous one is kept as `.bak` next to it.
- A new player starts from a copy of the main save, with their name written into it.
- **Back up**: copy `build/ms0` and `build/profiles`.
- **Forgotten password**: the host deletes `build/profiles/<name>/account.json`; the player then
  creates the same name again with a new password, and keeps their save.

## Things to know

- **Legal**: this repository holds only the port's own code, the analysis data (addresses and
  names), a wallpaper of the game (with its logo) for the sign-in page, and icons drawn for the
  project (a slice of cake). The game, its code and its art belong to their owners. Don't publish
  builds, generated code, ISOs or EBOOTs. The HD texture pack is derived from the game's art and
  meant for people who own the game.
- **Native builds**: Linux only for now (the browser build runs everywhere). Native online play
  only works between copies on the same machine (for testing); online play is a browser feature.
- **Smooth motion**: on the PSP the graphics chip's drawing time spaced the frames; here it takes
  no time, so the port paces them itself (one frame per screen refresh) and feeds the game's clock
  the time of the frames on the screen. In the browser the frames follow the monitor's refresh: 60
  and 120 Hz monitors are perfectly smooth, others (144 Hz) get a steady cadence.
- **Performance**: full speed at 4x on a laptop with integrated graphics (where it was made). The
  HD textures need more video memory; lower the internal resolution if a weak machine stutters.
- **The intro movie** is not played yet (the game goes straight on without it).
- **Phones and tablets** get a note asking for a computer instead of the game (nothing of the
  game is downloaded there): it needs a keyboard or a gamepad, and a computer's memory.
- **Sound** starts after the first click or key press (browsers' rule).
- **Saves** are the game's own files: the native build and the browser build share the main one.
- **Switches** (environment variables natively, `?NAME=value` in the browser's address):
  `FP_MOD_BOOT=0`, `FP_MOD_MENU=0`, `FP_MOD_PLAYERS=0` and `FP_MOD_ONLINE=0` turn the mods off, `FP_FRAME_CLOCK=0` gives the game the
  real clock again, `FP_SCALE=1..4` sets the render scale, `FP_FPS=1` logs the frame rate.
- Something wrong? In the browser, **F9** writes a diagnostic of the current frame into
  `build/webdump/` on the host (only from the host's own browser). The browser's console (F12)
  shows the game's log.

## Repository layout

```
port/
  runtime/        the PSP's system, rewritten (CPU helpers, VFPU, threads, files, graphics,
                  sound, saves, network, texture packs)
  mods/           the mods (C), hooked by address in hooks.txt
  web/            the browser page (shell.html), audio and LiveKit glue, the sign-in wallpaper
  tools/          generate.sh, recomp.py, prx_load.py, serve_web.py, textures.py, build scripts
  assets/fonts/   TeX Gyre Adventor (a vector font the runtime can draw text with; off here)
  third_party/    FFmpeg's headers (the libraries come from tools/get_ffmpeg.sh), stb_truetype
docs/
  NOTES.md        technical notes: the disc, FP.wad, the scripts, the frame loop, the traffic
re/
  export/         the analysis: program.json (functions, jump tables), imports, fixups, WAD names
  ghidra_scripts/ how program.json was made (Ghidra)
  tools/          FP.wad reader, the scripts' disassembler, the EBOOT decrypter, Ghidra helpers
```

## Credits

- **Fun Bits Interactive** and **Sony Computer Entertainment**, for Fat Princess: Fistful of
  Cake, and **Titan Studios**, for the original Fat Princess.
- [PPSSPP](https://www.ppsspp.org): the reference for how the PSP behaves, its FFmpeg builds, its
  tables of PSP system function names and its EBOOT decrypter.
- [Ghidra](https://ghidra-sre.org), [Emscripten](https://emscripten.org),
  [SDL](https://www.libsdl.org), [FFmpeg](https://ffmpeg.org), [LiveKit](https://livekit.io)
  (client library, Apache-2.0), [Real-ESRGAN](https://github.com/xinntao/Real-ESRGAN) (BSD-3-Clause),
  [TeX Gyre Adventor](https://www.gust.org.pl/projects/e-foundry/tex-gyre/adventor) (GUST Font
  License), [stb_truetype](https://github.com/nothings/stb) (public domain / MIT).
