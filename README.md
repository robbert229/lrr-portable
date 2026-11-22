# lrr-portable

**LEGO Rock Raiders (1999), rebuilt to run natively on modern computers.**

As a young child, I remember drilling through rock walls, hunting for Energy
Crystals, and yelling at Rock Raiders who walked straight past the ore I told
them to pick up. I did not understand how to play this game as a child, and I
never beat it. As an adult, I decided to revisit it, and was saddened by the
lack of a good way to run it on modern systems.

This project brings the original PC game back to life on today's Windows,
macOS and Linux. It doesn't need emulation, a Windows 98 virtual machine or
compatibility tricks.

<img width="2242" height="1440" alt="lrr-portable running a Rock Raiders mission" src="https://github.com/user-attachments/assets/9dae6667-cb3c-4ea5-a882-a837f23f350a" />
<img width="2242" height="1440" alt="lrr-portable gameplay" src="https://github.com/user-attachments/assets/714ff741-c2b2-4093-8257-8386c593f194" />
<img width="2037" height="1440" alt="lrr-portable gameplay" src="https://github.com/user-attachments/assets/7c544e7b-fb49-4450-9ac1-0ef29ecafcb2" />

## A huge thank you to the isledecomp team

**This port would not exist without the work of the
[isledecomp](https://github.com/isledecomp) team.** Their
[LEGO Island decompilation](https://github.com/isledecomp/isle) and its
portable follow-up, [isle-portable](https://github.com/isledecomp/isle-portable),
showed that a late-90s LEGO game could be brought back, piece by piece, and
run anywhere. They also built the tools to do it, and they shared them all.

lrr-portable stands directly on their shoulders:

- **miniwin**, isle-portable's layer that stands in for the old Windows and
  DirectX graphics so the game runs on SDL3, is what draws Rock Raiders on
  your screen.
- **[reccmp](https://github.com/isledecomp/reccmp)**, their decompilation
  comparison tool, is how every function of the game was checked against the
  original.
- Our save folders, `LegoRR.ini` settings file, packaging and console SDL
  builds all follow isle-portable's lead, so the two ports feel alike.

Thank you, isledecomp. If you enjoy this project, go play
[isle-portable](https://github.com/isledecomp/isle-portable) too.

## Is it playable?

**Not quite yet, but it's getting close.** Every part of the game has been
rebuilt, and you can start missions, dig, build and explore. The port is
still young, though, and **crashes are common**: you may not get to the end of
a mission yet. Bugs are being fixed quickly, and the aim is a fully playable
game by the end of November 2026.

If you try it and something breaks, please
[open an issue](https://github.com/robbert229/lrr-portable/issues) and say
what you were doing when it happened.

## What you need

- **Your own copy of LEGO Rock Raiders for PC** (the original 1999 release).
  lrr-portable contains **no game data**. It uses the files from your
  installed game:
  - `LegoRR0.wad` and `LegoRR1.wad`
  - the `Data` folder
- **Optional: the game's music.** The original played its soundtrack straight
  from the CD. To hear it, rip the CD's audio tracks into a `Music` folder
  next to the WAD files, named by CD track number: `Music/Track02.ogg`,
  `Music/Track03.ogg`, and so on. OGG, FLAC, MP3 and WAV all work. Without
  them the game plays without music, the same as the original with no CD in
  the drive.
- **Optional: FFmpeg 7 or newer, for the movies.** The intro and cutscene
  movies play through [FFmpeg](https://ffmpeg.org/). If it isn't installed,
  the game skips them and carries on.

## Getting the game running

1. **Get a build.** Ready-made builds are made for every change: an AppImage
   or Flatpak for Linux, a DMG for macOS and a ZIP for Windows. They're on
   the [Actions page](https://github.com/robbert229/lrr-portable/actions)
   (open the latest successful `native-packages` run and download one under
   *Artifacts*; you need to be signed in to GitHub). You can also
   [build it yourself](#building-it-yourself).
2. **Point it at your game.** Either start lrr-portable from inside your Rock
   Raiders folder (the one with `LegoRR0.wad`), or set `diskpath` in
   `LegoRR.ini` to that folder (see [Settings](#settings)). The settings file
   is created the first time you launch.
3. **Play.** Listen to the Chief's briefing, pick a mission and start digging.

> **macOS:** the app isn't notarized by Apple, so the first time you open it,
> right-click `lrr-portable.app` and choose **Open**.

## Your saves

Saves don't go in the game folder any more. They go in a per-user folder,
along with `LegoRR.ini`, so they stay put and work even if the game is
installed somewhere read-only:

| System | Saves and settings |
| --- | --- |
| Windows | `%APPDATA%\lrr-portable\LegoRR\` |
| macOS | `~/Library/Application Support/lrr-portable/LegoRR/` |
| Linux | `~/.local/share/lrr-portable/LegoRR/` |

**Have old saves?** The save format hasn't changed. Copy the files from your
original install's `Data\Saves` folder (`0.sav`, `0.osf`, the `.dat`
thumbnails and so on) into the folder above and they'll show up in the Load
Game menu.

## Settings

The original game asked about your screen in a "Mode Selection" box every
time it started. lrr-portable keeps those choices in `LegoRR.ini`, which sits
in the same folder as your saves. Open it in any text editor:

```ini
[legorr]
savepath = /home/me/.local/share/lrr-portable/LegoRR/
diskpath =
full screen = false
horizontal resolution = 640
vertical resolution = 480
3d device =

[tweaks]
training save = prompt
```

| Setting | What it does |
| --- | --- |
| `full screen` | `true` to play full screen, `false` (the default) for a window. |
| `horizontal resolution`, `vertical resolution` | The screen mode. The game runs at 640x480; 800x600 and 1024x768 work with the `-debug` option. |
| `diskpath` | Your Rock Raiders folder, so you can start the game from anywhere. Empty means "the folder I was started in". |
| `savepath` | Where your saves go. Empty keeps them in the game's own `Data\Saves` folder, like the original. |
| `3d device` | Leave empty to pick the best one automatically: your graphics card when it can, or a software renderer otherwise. `Miniwin driver (SDL3 GPU HAL)` forces the graphics card and `Miniwin driver (Miniwin Emulation)` forces the software renderer. |
| `training save` | See [Improvements](#small-improvements-over-the-original) below. |

The original game's command-line options still work too: `-window`,
`-nosound`, `-best` (skip the settings and pick a screen mode automatically)
and `-debug`. `--ini <file>` uses a different settings file.

## Small improvements over the original

The goal is the game as it was, so changes are rare, and where possible you
can switch them off.

- **Save after a training mission.** The original never let you save after
  finishing a training mission, so quitting threw that progress away without
  warning. lrr-portable opens the Save Game screen when you finish one. Don't
  want it? Set `training save = off`.
- **Skip the LEGO logo movie** with a key or click, like the other intro
  movies.
- **Closing the window quits** from menus, movies and loading screens,
  instead of being ignored.
- **Menus run at the right speed** on fast, modern screens.
- **Crash fixes:** several bugs in the original that Windows 98 happened to
  hide have been fixed so they don't crash the game on modern systems.

The full list, with the technical details, is in
[docs/divergences.md](docs/divergences.md).

## Building it yourself

You need CMake, a C++ compiler and Git. Put your game files in the
`reference` folder of this repository, then:

```sh
make run
```

SDL3 is used from your system if it's installed, and downloaded and built
otherwise. The [build guide](docs/building.md) covers everything else,
including packaging and running the tests.

## For developers and the curious

lrr-portable began as a decompilation of `LegoRR.exe`: the original program,
rebuilt as source code that compiles back into a near-identical copy of the
1999 executable. That matching decompilation continues on the
[`decomp`](https://github.com/robbert229/lrr-portable/tree/decomp) branch.
At `77f65f61` (#848, 2026-11-15), built with the original MSVC 5.0 compiler
and checked with reccmp:

| Measure | Value |
| --- | ---: |
| Accuracy: mean reccmp score of mapped functions, game-only / full | 98.04% / 98.23% |
| Mapped functions, game-only / full | 2,723 / 3,019 |
| Exact (100%) functions, game-only / full | 2,418 / 2,707 |

The `main` branch, the one you're looking at, takes that rebuilt code and
runs it on miniwin and SDL3
([ADR 0007](docs/adr/0007-port-on-develop-units-on-miniwin.md)).

[Build guide](docs/building.md) · [Findings](docs/findings/README.md) ·
[Pseudocode](docs/decompiles/README.md) · [Tools](tools/README.md) ·
[Compatibility harness](tools/compat/README.md) ·
[Repository layout](docs/repository-layout.md) · [Contributor rules](AGENTS.md)

## License

Code, tooling and documentation: [LGPL-3.0-or-later](LICENSE). LEGO Rock
Raiders, its assets and its original program belong to their respective
owners and are not included. This project is not affiliated with or endorsed
by the LEGO Group.
