# Changelog

What changed in each release of lrr-portable, written for players. The
format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
versions follow [Semantic Versioning](https://semver.org/); a pre-release
such as `1.0.0-beta1` comes before the release it names. Each section below is also the text of that release on
GitHub. [docs/releasing.md](docs/releasing.md) explains how a release is
made. The section for the version `main` is working towards is dated
`unreleased` until its release.

## [1.0.0-beta1] - 2026-11-22

The first release of lrr-portable: LEGO Rock Raiders (1999), rebuilt to run
natively on Windows, macOS and Linux, with early test builds for Android and
the Nintendo 3DS.

**You need your own copy of LEGO Rock Raiders for PC.** No download
contains any game data. lrr-portable uses `LegoRR0.wad`, `LegoRR1.wad` and the
`Data` folder from your installed game; the
[README](https://github.com/robbert229/lrr-portable#getting-the-game-running)
explains how to point it at them.

**This is a beta, the first pre-release of 1.0.0.** Every part of the game
has been rebuilt, and you can start missions, dig, build and explore, but
crashes are still common and you may not reach the end of a mission. Please
[report what breaks](https://github.com/robbert229/lrr-portable/issues).

### Downloads

| System | File |
| --- | --- |
| Windows 10 and 11, 64-bit | `lrr-portable-1.0.0-beta1-Windows-x64.zip` |
| Windows on Arm | `lrr-portable-1.0.0-beta1-Windows-arm64.zip` |
| Windows, 32-bit | `lrr-portable-1.0.0-beta1-Windows-x86.zip` |
| macOS 12 or later, Apple silicon | `lrr-portable-1.0.0-beta1-Darwin-arm64.dmg` |
| macOS 12 or later, Intel | `lrr-portable-1.0.0-beta1-Darwin-x86_64.dmg` |
| Linux, x86_64 (glibc 2.39 or later, such as Ubuntu 24.04) | `lrr-portable-1.0.0-beta1-Linux-x86_64.AppImage` or `.tar.gz` |
| Linux, Flatpak (x86_64 or aarch64) | `lrr-portable-1.0.0-beta1-Linux-x86_64.flatpak` or `-aarch64.flatpak` |
| Android 5 or later, **test build** | `lrr-portable-1.0.0-beta1-Android-debug-test.apk` |
| New Nintendo 3DS (homebrew), **test build** | `lrr-portable-1.0.0-beta1-3DS-homebrew-test.zip` (`LegoRR.3dsx` and `LegoRR.cia`) |

`SHA256SUMS` lists every file's checksum. Each download carries the licence
(`LICENSE`) and the notices for the libraries it includes
(`THIRD-PARTY-NOTICES.txt`). On the desktop, `LegoRR --version` prints the
version (on Windows, run `LegoRR.exe --version | more` to see it); every
system also writes it to the log at start-up.

The **Android and 3DS test builds** are for trying the port out, not for
keeping:

- The APK is signed with a throwaway debug key, so a later build can't
  install over it. You have to uninstall first, and uninstalling deletes
  the app's folder, saves included. Copy your saves out first.
- The 3DS build is homebrew: it runs from the Homebrew Launcher
  (`LegoRR.3dsx`) or installs with a CIA installer (`LegoRR.cia`) on a
  console with custom firmware.

### What works

- The whole game, rebuilt from the original program: the front end, the
  Chief's briefings, missions, digging, building and the rewards
  screens.
- Saving and loading. Old saves from the original game load unchanged; copy
  them from its `Data\Saves` folder into the new saves folder.
- Graphics through your graphics card: SDL GPU on the desktop, with OpenGL
  ES 3 available, and a software renderer when neither can be used.
- **Any window size and a sharper 3D view.** Resize the window as you like,
  or play full screen at your desktop's resolution. The game's 640x480
  picture fills as much of the window as it can, with black bars when the
  window isn't 4:3. The 3D view is drawn at the window's resolution, and the
  menus and panels are scaled up. Set `display = original` in `LegoRR.ini`
  for the original's fixed 640x480 window.
- Sound and 3D sound. Music, from the game CD's tracks ripped to files
  (`Music/Track02.ogg` and so on; OGG, FLAC, MP3 or WAV).
- The intro and cutscene movies, when FFmpeg 7 or newer is installed.
- The original's command-line options: `-window`, `-nosound`, `-best` and
  `-debug`.
- **Android (test build):** the game runs full screen in landscape, drawn
  with OpenGL ES 3, on `arm64-v8a` and `x86_64` devices.
- **New Nintendo 3DS (test build):** the game draws the level on the 3DS's
  GPU, with the whole picture fitted to the bottom screen.
- **The web build** streams the game data from a server, so you can host it
  yourself together with your own copy of the game files. No hosted version
  or download comes with this release; see
  [docs/building.md](docs/building.md#web-build-emscripten).

### Changes from the original

The aim is the game as it was, so changes are few. The full list, with the
details, is in [docs/divergences.md](docs/divergences.md).

- **Save after a training mission.** The original never offered to save
  after a training mission. lrr-portable opens the Save Game screen when you
  finish one. Set `training save = off` in `LegoRR.ini` for the original
  behaviour.
- **A resizable window**, with the 3D view drawn at the window's resolution
  (above). Set `display = original` to turn it off.
- **Saves and settings in your user folder.** Saves and `LegoRR.ini` live in
  a per-user folder, not in the game folder, and nothing is written to the
  registry:
  - Windows: `%APPDATA%\lrr-portable\LegoRR\`
  - macOS: `~/Library/Application Support/lrr-portable/LegoRR/`
  - Linux: `~/.local/share/lrr-portable/LegoRR/`
  - Android: `/sdcard/Android/data/io.github.robbert229.LegoRR/files/`
- **Settings in `LegoRR.ini`.** The window size, full screen or window, the
  game folder (`diskpath`) and the 3D device are set there, in place of the
  Mode Selection box the original showed at every start.
- **Skip the LEGO logo movie** with a key or click, like the other intro
  movies.
- **Movies fill the screen** in the larger debug screen modes.
- **Closing the window quits** from menus, movies and loading screens.
- **Menus run at the original's speed** on fast screens.
- **Music from files** instead of the CD drive, and **movies through
  FFmpeg** instead of Windows' video codecs.
- **Text from mods and custom levels can't crash the game** through printf
  format codes.
- **Crash fixes** for bugs in the original that Windows 98 happened to hide:
  - a missing menu animation no longer crashes the main menu;
  - a vehicle or Rock Raider carrying something while standing still no
    longer gets a broken facing;
  - the larger debug screen modes start, using the 640x480 interface
    layout.

### Known issues

- **Crashes are common.** Missions can crash before you finish them.
- **The interface stays at the original's 640x480 layout**, scaled up.
  Widescreen layouts are still to come.
- **Some menu effects run too fast.** Front-end effects that haven't yet
  been moved to elapsed time still count frames, so they can run faster than
  in the original.
- **The software renderer stays at 640x480** and is slow in a large window.
  The game uses it only where graphics-card rendering isn't available.
- **Movies need FFmpeg 7 or newer**, which no download includes. Without it
  the game skips the movies. The Flatpak uses the
  `org.freedesktop.Platform.ffmpeg-full` extension. Android and the 3DS
  have no movies.
- **The macOS app isn't notarized.** The first time, right-click
  `LegoRR.app` and choose **Open**.
- **The Windows program isn't signed.** Windows SmartScreen may warn the
  first time; choose **More info**, then **Run anyway**.
- **The AppImage needs glibc 2.39 or later** (Ubuntu 24.04, Fedora 40 or
  newer) and an x86_64 processor. On older systems, use the Flatpak.
- **The Flatpak reads your game files** only from your home folder,
  `/media`, `/run/media` and `/mnt`.
- **The web build uses the software renderer**, and you have to host it
  and your own game files yourself.
- **Android:**
  - a tap is a left click, and there are no touch controls yet for right
    click, scrolling, zoom, keys or Back
    ([#986](https://github.com/robbert229/lrr-portable/issues/986));
  - you copy the game files onto the device with `adb push`
    ([#909](https://github.com/robbert229/lrr-portable/issues/909) will add
    a folder picker); [docs/building.md](docs/building.md#android) shows how.
- **Nintendo 3DS:**
  - it needs a **New** 3DS or New 2DS: the level needs more memory than an
    original 3DS gives a program;
  - it runs at about 15 frames a second in a level, and hasn't yet been
    played past the mission brief or tried on real hardware;
  - the top screen shows a text console;
  - a software 3D device chosen in `LegoRR.ini` draws its picture unscaled
    on the top screen;
  - the HOME button and sleep don't pause or close the game yet
    ([#910](https://github.com/robbert229/lrr-portable/issues/910)).

### Coming next

These are in progress and not in 1.0.0-beta1:

- Widescreen interface layouts
  ([#915](https://github.com/robbert229/lrr-portable/issues/915)).
- Touch controls and a folder picker on Android
  ([#986](https://github.com/robbert229/lrr-portable/issues/986),
  [#909](https://github.com/robbert229/lrr-portable/issues/909)).
- Pause, resume and exit on the 3DS's HOME button
  ([#910](https://github.com/robbert229/lrr-portable/issues/910)).

[1.0.0-beta1]: https://github.com/robbert229/lrr-portable/releases/tag/v1.0.0-beta1
