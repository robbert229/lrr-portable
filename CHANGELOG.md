# Changelog

What changed in each release of lrr-portable, written for players. The
format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
versions follow [Semantic Versioning](https://semver.org/) (0.x while the
port is young). Each section below is also the text of that release on
GitHub. [docs/releasing.md](docs/releasing.md) explains how a release is
made.

## [Unreleased]

Nothing yet.

## [0.1.0] - unreleased

The first release of lrr-portable: LEGO Rock Raiders (1999), rebuilt to run
natively on Windows, macOS and Linux.

**You need your own copy of LEGO Rock Raiders for PC.** No download
contains any game data. lrr-portable uses `LegoRR0.wad`, `LegoRR1.wad` and the
`Data` folder from your installed game; the
[README](https://github.com/robbert229/lrr-portable#getting-the-game-running)
explains how to point it at them.

**This is an early release.** Every part of the game has been rebuilt, and
you can start missions, dig, build and explore, but crashes are still
common and you may not reach the end of a mission. Please
[report what breaks](https://github.com/robbert229/lrr-portable/issues).

### Downloads

| System | File |
| --- | --- |
| Windows 10 and 11, 64-bit | `lrr-portable-0.1.0-Windows-x64.zip` |
| Windows on Arm | `lrr-portable-0.1.0-Windows-arm64.zip` |
| Windows, 32-bit | `lrr-portable-0.1.0-Windows-x86.zip` |
| macOS 12 or later, Apple silicon | `lrr-portable-0.1.0-Darwin-arm64.dmg` |
| macOS 12 or later, Intel | `lrr-portable-0.1.0-Darwin-x86_64.dmg` |
| Linux, x86_64 (glibc 2.39 or later, such as Ubuntu 24.04) | `lrr-portable-0.1.0-Linux-x86_64.AppImage` or `.tar.gz` |
| Linux, Flatpak (x86_64 or aarch64) | `lrr-portable-0.1.0-Linux-x86_64.flatpak` or `-aarch64.flatpak` |

`SHA256SUMS` lists every file's checksum. Each download carries the licence
(`LICENSE`) and the notices for the libraries it includes
(`THIRD-PARTY-NOTICES.txt`). `LegoRR --version` prints the version.

### What works

- The whole game, rebuilt from the original program: the front end, the
  Chief's briefings, missions, digging, building and the rewards
  screens.
- Saving and loading. Old saves from the original game load unchanged; copy
  them from its `Data\Saves` folder into the new saves folder.
- Graphics through your graphics card (SDL GPU), with a software renderer
  when the card can't be used. A window or full screen at the original
  640x480.
- Sound and 3D sound. Music, from the game CD's tracks ripped to files
  (`Music/Track02.ogg` and so on; OGG, FLAC, MP3 or WAV).
- The intro and cutscene movies, when FFmpeg 7 or newer is installed.
- The original's command-line options: `-window`, `-nosound`, `-best` and
  `-debug`.

### Changes from the original

The aim is the game as it was, so changes are few. The full list, with the
details, is in [docs/divergences.md](docs/divergences.md).

- **Save after a training mission.** The original never offered to save
  after a training mission. lrr-portable opens the Save Game screen when you
  finish one. Set `training save = off` in `LegoRR.ini` for the original
  behaviour.
- **Saves and settings in your user folder.** Saves and `LegoRR.ini` live in
  a per-user folder (`%APPDATA%\lrr-portable\LegoRR\` on Windows,
  `~/Library/Application Support/lrr-portable/LegoRR/` on macOS,
  `~/.local/share/lrr-portable/LegoRR/` on Linux), not in the game folder,
  and nothing is written to the registry.
- **Settings in `LegoRR.ini`.** The screen mode, full screen or window, the
  game folder (`diskpath`) and the 3D device are set there, in place of the
  Mode Selection box the original showed at every start.
- **Skip the LEGO logo movie** with a key or click, like the other intro
  movies.
- **Closing the window quits** from menus, movies and loading screens.
- **Menus run at the original's speed** on fast screens.
- **Music from files** instead of the CD drive, and **movies through
  FFmpeg** instead of Windows' video codecs.
- **Text from mods and custom levels can't crash the game** through printf
  format codes.
- **Crash fixes** for bugs in the original that Windows 98 happened to hide.

### Known issues

- **Crashes are common.** Missions can crash before you finish them.
- **640x480 only.** Other screen modes need `-debug` and can crash at
  start-up; higher resolutions and widescreen are coming (see below).
- **Some menu effects run too fast.** Front-end effects that haven't yet
  been moved to elapsed time still count frames, so they can run faster than
  in the original.
- **A carried object can get a broken facing** when the Rock Raider or
  vehicle carrying it stands still; a fix is in progress.
- **Movies need FFmpeg 7 or newer**, which no download includes. Without it
  the game skips the movies. The Flatpak uses the
  `org.freedesktop.Platform.ffmpeg-full` extension.
- **The macOS app isn't notarized.** The first time, right-click
  `LegoRR.app` and choose **Open**.
- **The Windows program isn't signed.** Windows SmartScreen may warn the
  first time; choose **More info**, then **Run anyway**.
- **The AppImage needs glibc 2.39 or later** (Ubuntu 24.04, Fedora 40 or
  newer) and an x86_64 processor. On older systems, use the Flatpak.
- **The Flatpak reads your game files** only from your home folder,
  `/media`, `/run/media` and `/mnt`.
- **No web version** in this release.

### Coming next

These are in progress and not in 0.1.0:

- Higher resolutions, a resizable window and widescreen
  ([#915](https://github.com/robbert229/lrr-portable/issues/915)).
- The web version streaming its game data
  ([#908](https://github.com/robbert229/lrr-portable/issues/908)).
- A Nintendo 3DS build
  ([#931](https://github.com/robbert229/lrr-portable/issues/931)).
- The fix for a carried object's facing
  ([#961](https://github.com/robbert229/lrr-portable/issues/961),
  [#976](https://github.com/robbert229/lrr-portable/pull/976)).

[Unreleased]: https://github.com/robbert229/lrr-portable/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/robbert229/lrr-portable/releases/tag/v0.1.0
