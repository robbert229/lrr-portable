# Changelog

What changed in each release of lrr-portable, written for players. The
format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
versions follow [Semantic Versioning](https://semver.org/); a pre-release
such as `1.0.0-beta1` comes before the release it names. Each section below is also the text of that release on
GitHub. [docs/releasing.md](docs/releasing.md) explains how a release is
made. The section for the version `main` is working towards is dated
`unreleased` until its release.

## [1.0.0-beta3] - 2026-11-25

The third beta of lrr-portable: LEGO Rock Raiders (1999), rebuilt to run
natively on Windows, macOS and Linux, with early test builds for Android and
the Nintendo 3DS. This beta fills widescreen displays, adds touch controls
on Android, lets RecompLauncher set the game up on macOS, and fixes full
screen on a computer with more than one display.

**You need your own copy of LEGO Rock Raiders for PC.** No download
contains any game data. lrr-portable uses `LegoRR0.wad`, `LegoRR1.wad` and the
`Data` folder from your installed game; the
[README](https://github.com/robbert229/lrr-portable#getting-the-game-running)
explains how to point it at them.

**This is a beta, a pre-release of 1.0.0.** Every part of the game has been
rebuilt, and you can start missions, dig, build and explore, but crashes
are still common and you may not reach the end of a mission. Please
[report what breaks](https://github.com/robbert229/lrr-portable/issues).
The [1.0.0-beta2 notes](https://github.com/robbert229/lrr-portable/releases/tag/v1.0.0-beta2)
and the [1.0.0-beta1 notes](https://github.com/robbert229/lrr-portable/releases/tag/v1.0.0-beta1)
describe everything that already worked.

### Downloads

| System | File |
| --- | --- |
| Windows 10 and 11, 64-bit | `lrr-portable-1.0.0-beta3-Windows-x64.zip` |
| Windows on Arm | `lrr-portable-1.0.0-beta3-Windows-arm64.zip` |
| Windows, 32-bit | `lrr-portable-1.0.0-beta3-Windows-x86.zip` |
| macOS 12 or later, Apple silicon | `lrr-portable-1.0.0-beta3-Darwin-arm64.dmg` |
| macOS 12 or later, Intel | `lrr-portable-1.0.0-beta3-Darwin-x86_64.dmg` |
| macOS, for RecompLauncher | `lrr-portable-1.0.0-beta3-Darwin-arm64.zip` or `-Darwin-x86_64.zip` |
| Linux, x86_64 (glibc 2.39 or later, such as Ubuntu 24.04) | `lrr-portable-1.0.0-beta3-Linux-x86_64.AppImage` or `.tar.gz` |
| Linux, Flatpak (x86_64 or aarch64) | `lrr-portable-1.0.0-beta3-Linux-x86_64.flatpak` or `-aarch64.flatpak` |
| Android 5 or later, **test build** | `lrr-portable-1.0.0-beta3-Android-debug-test.apk` |
| New Nintendo 3DS (homebrew), **test build** | `lrr-portable-1.0.0-beta3-3DS-homebrew-test.zip` (`LegoRR.3dsx` and `LegoRR.cia`) |

`SHA256SUMS` lists every file's checksum. Each download carries the licence
(`LICENSE`) and the notices for the libraries it includes
(`THIRD-PARTY-NOTICES.txt`). On the desktop, `LegoRR --version` prints the
version (on Windows, run `LegoRR.exe --version | more` to see it); every
system also writes it to the log at start-up.

The **Android and 3DS test builds** are for trying the port out, not for
keeping:

- The APK is signed with a throwaway debug key, so it can't install over
  an earlier APK. You have to uninstall first, and uninstalling deletes the
  app's folder, saves included. Copy your saves out first.
- The 3DS build is homebrew: it runs from the Homebrew Launcher
  (`LegoRR.3dsx`) or installs with a CIA installer (`LegoRR.cia`) on a
  console with custom firmware.

### What's new

- **Widescreen.** In a window wider than 4:3, or in full screen on a wide
  display, a level now fills the screen instead of showing black bars at
  the sides. The 3D view shows more of the level at the sides, and the
  interface keeps to the edges of the screen: the radar, messages and
  information panel at the left, the top panel, icons, crystal bar and
  priorities at the right, and the briefing, help and menus in the middle.
  The screens around the levels (the front end, level select, rewards and
  loading screens, and the movies) are still drawn at 4:3, centred, with
  black at the sides. It's on by default and needs `display = fit`; set
  `widescreen = false` under `[tweaks]` in `LegoRR.ini` to keep the 4:3
  picture.
- **Touch controls (Android test build).** A tap clicks and a drag holds
  the button. Holding a finger still moves the pointer without clicking,
  so tooltips show. Two fingers scroll the map, pinch to zoom and twist to
  turn the view. A two-finger tap is Space, and a three-finger tap opens
  or closes the on-screen keyboard. The keyboard also opens by itself while
  you rename a raider, and Back is Escape. Each part can be turned off under
  `[tweaks]`; `touch = true` turns them on with a desktop touch screen.
- **[RecompLauncher](https://recomplauncher.com) on macOS.** The new macOS
  ZIPs are the downloads the launcher installs, so it can now ask for your
  Rock Raiders folder, start the game and show its settings on macOS too.
  To play without the launcher, use the disk image as before.
- **Nintendo 3DS (test build):** the game now runs a New 3DS at its full
  clock speed with its extra cache, which it didn't before, and waits less
  between frames.

An existing `LegoRR.ini` isn't rewritten, so add the new keys by hand
under their section headings. `LegoRR.ini` also accepts a few more
spellings: for example, `full screen = on` means full screen.

### Fixes

- **Full screen opens on the main display.** On a computer with more than
  one display, where the main display isn't the leftmost, the game used to
  go full screen on the other display.
- **Vehicles carrying more than one thing.** After a vehicle took only part
  of a raider's load, the rest could be lost track of.
- **Leaving a vehicle** away from a dock keeps the way the raider faced.
- **Naming a raider.** A name of the full 11 letters now fits in its
  storage, and a raider that is recycled no longer keeps a freed name.
- **Sound effects play at the volume the game's data gives them.** Before,
  a sample's volume could pick up stray digits.
- **Fewer crashes and leaks when quitting a level or the game:** menus,
  smoke effects, parts of the 3D scene and objects waiting to be placed are
  released properly.

### Changes from the original

New in this release; the full list, with the details, is in
[docs/divergences.md](docs/divergences.md).

- **Widescreen** above (`[tweaks]` `widescreen`).
- **The touch controls** above (`[tweaks]` `touch` and the `touch_` keys),
  on by default on Android only.
- **The on-screen keyboard** opens while you type a raider's name, and
  Android's Back button is Escape.
- **Smoke effects** release their 3D frames when they're removed, and
  **sound volumes** are read without the original's stray digits. Neither
  has a `[tweaks]` key: these fix the original's handling of its own
  resources.

### Known issues

- **Crashes are common.** Missions can crash before you finish them.
- **The interface is still the original's 640x480 layout, scaled up.**
  On a wide screen it moves to the edges, but its parts keep their size and
  shape. The screens around the levels stay at 4:3 with bars.
- **The screen's shape is chosen when the game starts.** If you resize the
  window to another shape afterwards, the picture is fitted into it with
  bars until you restart.
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
- **Android:** you copy the game files onto the device with `adb push`
  ([#909](https://github.com/robbert229/lrr-portable/issues/909) will add a
  folder picker); [docs/building.md](docs/building.md#android) shows how.
  The touch controls are new and have had little testing on devices.
- **Nintendo 3DS:**
  - it needs a **New** 3DS or New 2DS: the level needs more memory than an
    original 3DS gives a program;
  - it has barely been tried on a console, and its speed there hasn't
    been measured since the clock speed fix;
  - a touch is a left click and nothing more: there's no right click,
    scrolling or zoom, and the buttons and circle pad do nothing;
  - the top screen shows a text console;
  - it keeps the 4:3 picture: widescreen is off there by default, since a
    wider view draws more of the level each frame;
  - a software 3D device chosen in `LegoRR.ini` draws its picture unscaled
    on the top screen.

### Coming next

These are in progress and not in 1.0.0-beta3:

- A folder picker on Android
  ([#909](https://github.com/robbert229/lrr-portable/issues/909)).
- Builds for more of the systems isle-portable runs on
  ([#934](https://github.com/robbert229/lrr-portable/issues/934)).

## [1.0.0-beta2] - 2026-11-24

The second beta of lrr-portable: LEGO Rock Raiders (1999), rebuilt to run
natively on Windows, macOS and Linux, with early test builds for Android and
the Nintendo 3DS. This beta fixes the Flatpak, adds keyboard and mouse
camera controls, fixes the raiders' "idleness syndrome", adds two cheats,
and lets you tap through the 3DS build's menus.

**You need your own copy of LEGO Rock Raiders for PC.** No download
contains any game data. lrr-portable uses `LegoRR0.wad`, `LegoRR1.wad` and the
`Data` folder from your installed game; the
[README](https://github.com/robbert229/lrr-portable#getting-the-game-running)
explains how to point it at them.

**This is a beta, a pre-release of 1.0.0.** Every part of the game has been
rebuilt, and you can start missions, dig, build and explore, but crashes
are still common and you may not reach the end of a mission. Please
[report what breaks](https://github.com/robbert229/lrr-portable/issues).
The [1.0.0-beta1 notes](https://github.com/robbert229/lrr-portable/releases/tag/v1.0.0-beta1)
describe everything that already worked.

### Downloads

| System | File |
| --- | --- |
| Windows 10 and 11, 64-bit | `lrr-portable-1.0.0-beta2-Windows-x64.zip` |
| Windows on Arm | `lrr-portable-1.0.0-beta2-Windows-arm64.zip` |
| Windows, 32-bit | `lrr-portable-1.0.0-beta2-Windows-x86.zip` |
| macOS 12 or later, Apple silicon | `lrr-portable-1.0.0-beta2-Darwin-arm64.dmg` |
| macOS 12 or later, Intel | `lrr-portable-1.0.0-beta2-Darwin-x86_64.dmg` |
| Linux, x86_64 (glibc 2.39 or later, such as Ubuntu 24.04) | `lrr-portable-1.0.0-beta2-Linux-x86_64.AppImage` or `.tar.gz` |
| Linux, Flatpak (x86_64 or aarch64) | `lrr-portable-1.0.0-beta2-Linux-x86_64.flatpak` or `-aarch64.flatpak` |
| Android 5 or later, **test build** | `lrr-portable-1.0.0-beta2-Android-debug-test.apk` |
| New Nintendo 3DS (homebrew), **test build** | `lrr-portable-1.0.0-beta2-3DS-homebrew-test.zip` (`LegoRR.3dsx` and `LegoRR.cia`) |

`SHA256SUMS` lists every file's checksum. Each download carries the licence
(`LICENSE`) and the notices for the libraries it includes
(`THIRD-PARTY-NOTICES.txt`). On the desktop, `LegoRR --version` prints the
version (on Windows, run `LegoRR.exe --version | more` to see it); every
system also writes it to the log at start-up.

The **Android and 3DS test builds** are for trying the port out, not for
keeping:

- The APK is signed with a throwaway debug key, so it can't install over
  the 1.0.0-beta1 APK. You have to uninstall first, and uninstalling deletes
  the app's folder, saves included. Copy your saves out first.
- The 3DS build is homebrew: it runs from the Homebrew Launcher
  (`LegoRR.3dsx`) or installs with a CIA installer (`LegoRR.cia`) on a
  console with custom firmware.

### What's new

- **Keyboard and mouse camera controls.** In a level, `W`, `A`, `S` and `D`
  pan the view, `Q` and `E` rotate it, the mouse wheel zooms, and dragging
  with the middle button pans. Edge panning and the arrow keys still work.
  They're on by default; set `controls = original` under `[tweaks]` in
  `LegoRR.ini` for the original's controls alone.
- **Raiders no longer stand idle while jobs wait.** The "idleness
  syndrome", a bug in the original's job assignment that left raiders
  standing around, worst with many raiders or on maps with parts they can't
  reach, is fixed. It's on by default; set
  `fix_idleness_syndrome = false` under `[tweaks]` for the original's
  behaviour.
- **Cheats.** A new `[cheats]` section in `LegoRR.ini`, off unless you turn
  it on:
  - `unlimited_raiders = true` lets you teleport in raiders past the limit
    your Support Stations set;
  - `unlimited_oxygen = true` keeps the air full, so a level can't be lost
    to it.
- **A frame rate counter.** Set `fps_overlay = true` under `[tweaks]` to
  show the frames a second in the top right corner.
- **[RecompLauncher](https://recomplauncher.com) support.** The launcher can
  set lrr-portable up for you: it asks for your Rock Raiders folder, starts
  the game, and shows its settings on the game's page. Two new command-line
  options make this work, and change nothing unless you pass them:
  `--game <folder>` reads the game from that folder for one run, and
  `--data <folder>` keeps `LegoRR.ini` and your saves in that folder.
- **Nintendo 3DS (test build):**
  - touching the bottom screen clicks where you touch, so you can get
    through the menus;
  - the HOME menu and sleep mode pause the level, and closing the game from
    the HOME menu saves your settings as quitting does;
  - long sounds are read from the SD card as they play instead of loaded
    whole; loading them whole ran the game out of memory at the main menu;
  - a crash on the level-select screen is fixed.

An existing `LegoRR.ini` isn't rewritten, so add the new keys by hand
under their section headings.

### Fixes

- **The Flatpak starts again.** In 1.0.0-beta1, starting the Flatpak from
  the menu or with `flatpak run` closed it straight away. The same happened
  to any install started as `LegoRR` from the `PATH`.

### Changes from the original

New in this release; the full list, with the details, is in
[docs/divergences.md](docs/divergences.md).

- **The keyboard and mouse camera controls** above (`[tweaks]`
  `controls`).
- **The idleness syndrome fix** above (`[tweaks]`
  `fix_idleness_syndrome`).
- **The cheats** above (`[cheats]`), off by default.
- **The frame rate counter** above (`[tweaks]` `fps_overlay`), off by
  default.
- **Suspending the 3DS pauses the level**, as losing focus does in the
  original when `LoseFocusAndPause` is on in `Lego.cfg`.

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
- **RecompLauncher on macOS:** the launcher installs a zipped app, and the
  macOS download is a disk image, so the launcher can't set it up yet.
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
  - it runs at about 15 frames a second in a level in an emulator, and has
    barely been tried on a console. Pausing on the HOME menu and sleep, and
    the fixes above, haven't been confirmed on one yet;
  - a touch is a left click and nothing more: there's no right click,
    scrolling or zoom, and the buttons and circle pad do nothing;
  - the top screen shows a text console;
  - a software 3D device chosen in `LegoRR.ini` draws its picture unscaled
    on the top screen.

### Coming next

These are in progress and not in 1.0.0-beta2:

- Widescreen interface layouts
  ([#915](https://github.com/robbert229/lrr-portable/issues/915)).
- Touch controls and a folder picker on Android
  ([#986](https://github.com/robbert229/lrr-portable/issues/986),
  [#909](https://github.com/robbert229/lrr-portable/issues/909)).

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
  with OpenGL ES 3, on `arm64-v8a` and `x86_64` devices, with touch
  controls: tap to click, hold still to see a tooltip, drag with two fingers
  to scroll, pinch to zoom, twist to turn the view, tap with two fingers for
  Space and with three for the on-screen keyboard. Back is Escape.
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
- **Touch controls** on Android (above), each with a `LegoRR.ini` setting
  to turn it off. A tap or key shorter than a frame is no longer missed.
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
- A folder picker on Android
  ([#909](https://github.com/robbert229/lrr-portable/issues/909)).
- Pause, resume and exit on the 3DS's HOME button
  ([#910](https://github.com/robbert229/lrr-portable/issues/910)).

[1.0.0-beta3]: https://github.com/robbert229/lrr-portable/releases/tag/v1.0.0-beta3
[1.0.0-beta2]: https://github.com/robbert229/lrr-portable/releases/tag/v1.0.0-beta2
[1.0.0-beta1]: https://github.com/robbert229/lrr-portable/releases/tag/v1.0.0-beta1
