# Divergences from the original game

The port aims to behave like the reference binary, the 1999 `LegoRR.exe`.
This file lists every place where it deliberately does not, and why.
AGENTS.md requires an entry here for each such change. New divergences
should, where they can:

- come with a `LegoRR.ini` key in the `[tweaks]` section that restores the
  original;
- mark their call site `// [divergence:<name>]`.

Older ones carry a `PORT-ONLY:` comment instead.

The list has three parts:

1. Divergences a player can notice.
2. Fixes for bugs in the original that the port would otherwise reproduce
   or turn into crashes.
3. Platform substitutions that are meant to behave the same.

A change that only restores the original's behaviour, such as a 64-bit
layout fix, is not a divergence. Part 2 lists the larger ones for
reference.

## 1. Divergences a player can notice

### Save after a training mission

- **Original:** a level whose `Lego.cfg` block has no `Reward` section shows
  no rewards screen, and so no Save button. The eight training missions
  (`Levels::Tutorial01` to `Tutorial08`, the first entries on the mission
  select screen) are such levels. `Reward_CheckSkip` (0x00462780) goes
  straight to `Reward_GoToAdvance` (0x00462270):
  - In a new game, the progress goes to slot 5, the scratch slot. The Load
    menu never lists slot 5, so finishing a training mission and quitting
    loses that progress.
  - With a slot already loaded, the progress is kept only in memory until a
    later rewards screen's Save.
- **Port:** when such a level is completed, the game opens the Save Game
  screen that the Save button opens (`Reward_SaveMenu`, 0x00462030). If the
  player picks a slot, the level is recorded there and written to disk. If
  they back out, play goes on as in the original. Levels started from the
  separate Tutorials menu are not offered a save, the same guard the
  original's save menu has.
- **Why:** the reference binary's behaviour looks like an oversight in the
  game data rather than a design choice. Players lose the progress they have
  just made, and nothing tells them so.
- **Configurable:** `[tweaks]` `Training Save = prompt` (the default) or `off`, which
  keeps the original.
- **Where:** `reward_check_skip` in `src/game/reward.c`;
  `LRR_GetTrainingSave` in `src/platform/native/config.h`.

### Saves in a per-user directory

- **Original:** keeps `Saves\<n>.sav`, `Saves\<n>.osf` and the `.dat`
  thumbnails in `Data\Saves` under the install.
- **Port:** maps every name under `Saves` into `savepath`. The default is
  SDL's preference directory, e.g. `~/.local/share/lrr-portable/LegoRR/` or
  `%APPDATA%\lrr-portable\LegoRR\`.
- **Why:** an install directory is often read-only. Following
  isle-portable keeps the two ports alike.
- **Configurable:** `savepath`. An empty value keeps `Data\Saves`.
- **Where:** b4af5d1f (#911); `src/platform/native/files_native.cpp`,
  `src/platform/native/config.cpp`.

### Display settings from LegoRR.ini, not the Mode Selection dialog

- **Original:** unless started with `-best`, `Init.c` asks for the driver,
  device, screen mode and window or full screen in a dialog.
- **Port:** takes those choices from the ini file. The dialog code stays in
  the unit but is not called. An invalid resolution falls back to the
  dialog's default mode.
- **Why:** miniwin cannot show Win32 dialogs. This follows isle-portable,
  which replaces Lego Island's CONFIG.EXE with `isle.ini`.
- **Configurable:** `Full Screen`, `Horizontal Resolution`,
  `Vertical Resolution` and `3D Device`. `-best` and `-window` still work.
- **Where:** 7590243a (#913); `src/gods98/init.c` (`[library:config]`).

### LegoRR.ini and `--ini`

- **Original:** has neither.
- **Port:** writes `LegoRR.ini` with defaults on first run. `--ini <path>`
  names another file; on desktop a path that does not load is a fatal error.
- **Why:** this holds the settings above, as isle-portable does.
- **Where:** `src/platform/native/entry.cpp`, `src/platform/native/config.cpp`.

### No registry

- **Original:** reads `HKLM\SOFTWARE\LEGO Media\Games\Rock Raiders`:
  - `StandardParameters` adds extra command-line flags.
  - `NoHALMessage`, `CDMissing` and `SetupError` supply message text.
- **Port:** every registry read fails, so the flags never apply and the
  default messages are used.
- **Why:** there is no registry off Windows. ADR 0007 plans an ini
  substitution for these values.
- **Configurable:** no; pass the flags on the command line.
- **Where:** `src/platform/native/seam_stubs.cpp`.

### The LEGO logo movie can be skipped

- **Original:** a key or click skips `LegoAvi` only in programmer mode.
  `DDIAvi` and `RRAvi` can always be skipped.
- **Port:** `LegoAvi` can always be skipped too.
- **Why:** consistency with the other two intro movies.
- **Configurable:** no.
- **Where:** d8dda7c7 (#894); `src/game/lego_unit.c` (`PORT-ONLY`).

### Closing the window quits from menus, movies and splashes

- **Original:** keeps running a front-end menu after a close request. Its
  intro movies and splash screens never check for one.
- **Port:** a close request leaves the menu as Quit Game does, and ends
  movies and splashes. A close request is the close button, Alt+F4, or
  SIGINT/SIGTERM.
- **Why:** otherwise closing the window, or stopping the process, left the
  port sitting at the main menu.
- **Where:** 132071ef (#865), 5759c088 (#877);
  `src/game/frontend_unit.c`, `src/game/lego_unit.c`.

### Front-end timing on elapsed time

- **Original:** the full-screen flip waited for vblank, so per-frame
  front-end effects ran about 60 times a second.
- **Port:** the frame rate is not capped. These effects now run on elapsed
  time at the original's 60 Hz rate:
  - the menu overlay roll
  - the text cursor blink
  - the menu edge scroll
  - the rewards screen scroll and its debug speed keys

  Front-end effects that have not been converted still count frames, so
  they can run faster than in the original.
- **Why:** to match the original's speed at any frame rate.
- **Where:** 55fa915f (#871), 6f1fc773 (#873).

### Music from track files

- **Original:** plays the CD's Red Book audio through MCI.
- **Port:** plays `Music/TrackNN.{ogg,flac,mp3,wav}`. Without the files there
  is no music, as with no CD in the drive.
- **Why:** few hosts have a CD drive.
- **Where:** 7c0872d1 (#874); `src/library/audio_sdl.cpp`.

### Movies need FFmpeg

- **Port:** loads FFmpeg 7 or newer at run time. Without it the movies are
  skipped.
- **Why:** it replaces DirectShow and AVIFile.
- **Where:** e6a2510d (#868); `src/library/video_ffmpeg.cpp`.

### Debug-build assert on the .osf magic

- **Port:** a debug build asserts the `RROS` magic when loading a `.osf`, as
  isle-portable asserts its save version. Release builds keep the
  original's check.
- **Where:** b4af5d1f; `src/game/objectrecall.c`.

## 2. Bug fixes in the original

These change the original's code where it misbehaves. Each one is marked
`PORT-ONLY` at its site.

| Fix | Original | Port | Where |
| --- | --- | --- | --- |
| Restart Mission level name | frees `levelName`, which the new level keeps as its name; Win32's heap hid this | doesn't free it | 62f44679 (#903), `lego_unit.c` |
| NaN move probe | a NaN facing gives `INT_MIN` block coordinates, which 32-bit arithmetic wraps to block 0 | clamps out-of-range coordinates to block 0, the original's result, instead of reading outside the grid | 62f44679, `lego_unit.c` |
| Animated cursor flic | walks past the drawn frame and over-reads | stops after the frame; bounds the walk by the loaded size | e04b5f1b (#895), `flic_unit.c` |
| Texture free order | releases the surface before the texture that uses it; leaks the record | releases the texture first; frees the record | 16d561af (#896) |
| Unopened movie size | reads a NULL surface | guards it | `movie_unit.cpp` |
| Region data on failure | frees it twice after a `SetClipList` failure | frees it once | `directdraw.c` |
| RNC unpacked length | read as a signed long | read as unsigned | `rnc.c` |
| Info text image tokens | `info_compile_text` writes an uninitialised byte for a `$name$` token that names no image, overruns its image slots past the eighth token and its name buffer past 31 characters, and reads past the text for an unclosed token; Font draws any `@[0x` + 8 characters + `]` as an image | writes nothing for an imageless token, keeps five images and drops the rest, truncates the name, stops at the text's end; Font draws only a full hex address (#904) | `info_unit.c`, `font.c` |
| `Data\Saves` directory | (n/a: only POSIX paths are affected) | makes `Data/Saves`, not a directory named with a backslash | b4af5d1f |

Many 64-bit and ABI fixes are not listed here. These include reused
`va_list`s, LP64 `RECT`s, struct offsets, linker-folded getters, misaligned
loads and declaration mismatches. They restore the original's behaviour
rather than change it; their commits (#863–#898) describe them.

## 3. Platform substitutions meant to behave the same

Each one is marked `// [library:<area>]` at its call site (ADR 0007).

| Area | Original | Port |
| --- | --- | --- |
| Audio | DirectSound, DirectSound 3D, winmm mixer | SDL3 audio streams |
| CD music | MCI cdaudio | track files through miniaudio |
| Time | winmm | `SDL_GetTicks` |
| Video | DirectShow, AVIFile | FFmpeg |
| Keyboard | DirectInput | SDL keyboard state through a scancode to `DIK_*` table |
| Filesystem | Windows paths | host paths, case-insensitive, `SDL_IOStream`, `SDL_GlobDirectory` |
| Config | registry, Mode Selection dialog | iniparser (`LegoRR.ini`) |
| Display | DirectDraw, Direct3DRM, USER32 | miniwin over SDL3 (ADR 0004, 0008, 0009) |

Known small differences in substitutions meant to behave the same:

- 3D sound pans with DirectSound's pan law. There is no Doppler, which the
  original never used either, because it sets no velocities.
- miniwin presents its software renderer as a hardware device, in RGB565.
- `entry.cpp` builds the Windows command line from `argv`, so the original's
  flags (`-best`, `-window`, `-nosound`, `-debug`, ...) work unchanged.
- `crt/vc5names.h` keeps the VC5 `rand`/`srand`, so random sequences match.
