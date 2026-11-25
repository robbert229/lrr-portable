# Build and run

[Project overview](../README.md) · [Implementation notes](implementation.md)

Run commands from the repository root. Supply a legally owned game directory
containing `LegoRR0.wad`, `LegoRR1.wad`, and the loose `Data/` files. The
[repository layout](repository-layout.md) describes the reference files.

## Native Linux build

Install CMake 3.20 or later, a C++17 compiler, and Git.
`make setup` bootstraps the sparse `3rdparty/isle-portable` miniwin submodule,
applies the project's patches from `patches/miniwin` to it, and configures
CMake; `make build` also runs setup. The configuration file reader,
[iniparser](https://gitlab.com/iniparser/iniparser), comes from an installed
CMake package when there is one; otherwise configure fetches release v4.2.6,
the version isle-portable uses. SDL3 works the same way: an installed package
is used when CMake finds one (point `CMAKE_PREFIX_PATH` at it if it is
elsewhere), and otherwise configure fetches and builds a static SDL at the
commit CI uses (release-3.4.18, `829a65d7`). On Xbox (`WINDOWS_STORE`) and
Switch it fetches isle-portable's SDL forks, and on Android it reads the SDL
package Gradle builds. The fetched SDL needs SDL's own
[build dependencies](https://wiki.libsdl.org/SDL3/README-linux#build-dependencies);
on a host without X11 or Wayland development libraries, configure with
`-DSDL_X11=OFF -DSDL_WAYLAND=OFF -DSDL_UNIX_CONSOLE_BUILD=ON`. The
`LRR_SDL3_GIT_REPO` and `LRR_SDL3_GIT_TAG` cache variables pick another SDL
source.

```sh
make build
make run WAD_DIR=/path/to/game
```

`LegoRR` is the units' `WinMain` (ADR 0007): it runs in the game directory,
opens `LegoRR0.wad` and `LegoRR1.wad` there, and takes the shipped exe's flags
(`-best`, `-window`, `-nosound`, `-debug`, ...). `make run` passes `RUN_ARGS`,
none by default. Where the original showed its "Mode Selection" dialog, the
port reads the display settings from `LegoRR.ini`, as isle-portable reads
`isle.ini` in place of Lego Island's `CONFIG.EXE`; `-best` still picks a mode
by itself, and `-window` then runs it in a window. miniwin supplies the
Windows-compatible interfaces over SDL3 on every host, Windows included; the
build does not require Wine. The file layer's substitution, including
directory listings and the save directory, is in
`src/platform/native/files_native.cpp`; the port's configuration file
(`LegoRR.ini`, see the README) is read by `src/platform/native/config.cpp`
before `WinMain`, and `--ini <path>` names another one. The imports still
outside miniwin's scope (the registry among them) are logging stubs in
`src/platform/native/seam_stubs.cpp` until their library substitutions land.
`BUILD_DIR` selects another build directory.

For direct CMake use, initialize miniwin first. Rerun `tools/setup-miniwin.sh`
whenever `patches/miniwin` or the submodule pin changes; the build stops with
that advice when the applied patches are out of date.

```sh
tools/setup-miniwin.sh
cmake -S . -B build
cmake --build build
cd /path/to/game && /path/to/build/LegoRR -best -window
```

The game's movies need FFmpeg 7 or later at run time, not at build time:
`src/library/video_ffmpeg.cpp` loads libavformat, libavcodec, libavutil and
libswscale with `SDL_LoadObject`, trying the names `CMakeLists.txt` lists for
the host (`LRR_VIDEO_LIB*`: `.so`, `.dylib` or `.dll`). Without them the game
skips its movies. `-DLRR_MOVIES=OFF` builds the video library as a stub that
always reports no movies; it is off already on Emscripten, iOS, Android, the Nintendo
targets, Vita and DOS.

miniwin emulates a 16-bit RGB565 display and describes its software renderer
as a hardware device, which is what `-best` looks for (see the
`MINIWIN_PIXELFORMAT` and `MINIWIN_SOFTWARE_HAL` definitions in
`CMakeLists.txt`). miniwin builds through upstream's `miniwin/CMakeLists.txt`,
which adds each platform's renderer backends: SDL GPU, plus OpenGL 1.x and
OpenGL ES 2 and 3 when CMake finds their libraries, DirectX 9 on Windows, and
Citro3D, GXM and Glide on 3DS, Vita and DOS. Only the software, SDL GPU and
OpenGL ES 3 backends draw the level (immediate mode, viewport rectangles and
rectangle clears; ADR 0008, step 6), and miniwin reports which do
(`Miniwin_DeviceDrawsImmediateMode`). With an empty `3d device` in
`LegoRR.ini`, the game picks the platform's default device if it can draw,
else the first device that can (a device the file names wins): on the desktop SDL GPU, which needs a
Vulkan, Metal or Direct3D 12 device, falling back to the software renderer
(`3d device = Miniwin driver (Miniwin Emulation)` selects it); on Android
OpenGL ES 3, and on the web the software renderer. The
per-platform defaults are in `src/platform/native/<platform>/config.h`, and
`device_choice_test` checks them against each platform's device list.

The OpenGL backends draw only into an OpenGL window, and the window exists
before the game lists its devices. So the port asks miniwin for an OpenGL
window (`Miniwin_SetOpenGLWindows`) when `3d device` names an OpenGL device,
or is empty and the platform's default is one; `3d device = Miniwin driver
(OpenGL ES 3.0 HAL)` selects OpenGL ES 3, which needs an OpenGL ES 3.0
context (a desktop OpenGL 4.3 or later driver provides one). On an OpenGL
window miniwin does not list SDL GPU, which cannot share the window with EGL
(on Wayland the attempt closes the display connection). OpenGL ES 2 and
OpenGL 1.x are listed there too but cannot draw the level: OpenGL ES 3 is
the oldest OpenGL the port draws with. miniwin tries OpenGL ES 3 on a hidden
window of its own before listing it; where a platform has only one window
(Android), it tries it on the game's OpenGL window instead. The `miniwin_*_gles3` tests run the
rendering checks on OpenGL ES 3 where SDL's offscreen driver offers an
OpenGL ES 3 context through EGL (`LRR_MINIWIN_GLES3_VIDEO_DRIVER` picks
another driver), and are skipped elsewhere, as on CI.

For a headless run, use SDL's offscreen driver and have miniwin write each
presented frame to a file:

```sh
cd /path/to/game
SDL_VIDEODRIVER=offscreen MINIWIN_DUMP_FRAME=/tmp/frame.png /path/to/build/LegoRR -best -window
```

The frame is written at the resolution the picture was drawn at: the game's
640x480 at the render target's scale, such as 1600x1200 in a 1600x1200
window on SDL GPU.

miniwin fits the game's 640x480 picture into a window of any size, as
isle-portable does: the largest 4:3 area of the window, with black bars at
the sides or above and below. SDL GPU draws the 3D view at the window's
resolution and scales the interface up, nearest-neighbour at whole-number
scales and bilinear otherwise; the software renderer draws the whole picture
at 640x480 and scales it. The mouse follows the fit. `[tweaks]` `display =
original` keeps a fixed window at the mode's size
([divergences](divergences.md)), and `miniwin_presentation_test` checks the
fit, the mouse and the frame-buffer locks headlessly.

Run the binary from a copy of the game directory, or from one whose WADs are
symbolic links, so that nothing the program writes lands in your reference
data. The front end ignores close requests, so stop a headless run with
`timeout`.

## Web build (Emscripten)

The web build (issue #926) needs [emsdk](https://emscripten.org/docs/getting_started/downloads.html)
6.0.10, the release isle-portable builds with, and gives `LegoRR.html`,
`LegoRR.js` and `LegoRR.wasm`. The tests are native programs, so configure
without them:

```sh
tools/setup-miniwin.sh
emcmake cmake -S . -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build-web --target program
```

`-sPROXY_TO_PTHREAD` runs `main`, and so `WinMain`'s loop and the front
end's nested modal loops, on a worker thread, where blocking is allowed; the
browser's main thread stays free. A canvas drawn with WebGL shows a frame
only when its thread returns to the browser, which `WinMain` never does, so
the web build draws with miniwin's software renderer through SDL's software
render driver (`entry.cpp`), which puts each frame on the page's canvas from
the main browser thread. miniwin's OpenGL ES backends are left out
(`ISLE_RENDERER_OPENGLES`), as enumerating them would take the canvas for
WebGL. The OpenGL ES 3 backend draws the level, but WebGL 2 from this
worker would need `-sOFFSCREEN_FRAMEBUFFER`, which runs every GL call on
the main thread synchronously, thousands of calls a frame, and an explicit
commit of each frame; a web build that draws with WebGL 2 needs a main loop
that returns to the browser between frames instead. SDL's DOM event handlers queue each mouse
and key event for the worker, and miniwin's `PeekMessageA` runs that queue
once per message pump (`patches/miniwin`). The page,
`src/platform/native/emscripten/shell.html`, sizes the canvas with CSS at
4:3; the game keeps its 640x480 display and SDL scales it to the canvas.
wasm-ld's warnings are errors in this build: a unit that declares another
unit's function with other parameters would otherwise link to a trap.

The page needs cross-origin isolation (for `SharedArrayBuffer`), which
`tools/serve_web.py` gives it:

```sh
tools/serve_web.py build-web        # http://127.0.0.1:8000/LegoRR.html
```

The game reads its data from `/LegoRR` (`legorr:diskpath`), and its
configuration and saves live in the browser's Origin Private File System
(`src/platform/native/emscripten/`).

### Streaming the game data

The page brings no game data: the game streams it from a host over a WasmFS
fetch backend (issue #908), as isle-portable does with
`ISLE_EMSCRIPTEN_HOST` and `/LEGO`. The data is at `<host>/LegoRR/`, which
holds a copy of the game directory's WADs and `Data` tree and a manifest,
`LegoRR.manifest`, that lists the files to register.
`tools/web_manifest.py` writes the manifest from a game directory when you
deploy, so the build holds neither a file list nor game data:

```sh
tools/web_manifest.py /path/to/game -o deploy/LegoRR/LegoRR.manifest
# then publish the game directory's LegoRR*.wad and Data beside it, at <host>/LegoRR/
```

The manifest lists the top-level WADs and everything under `Data` except
`Data/Saves` (the player's saves live in the browser) and the `*.avi` movies
(the web build has no FFmpeg, and they are most of the game's 520 MB;
`--include-movies` and `--exclude GLOB` change that). The game fetches the
manifest first, makes a directory tree and a file on the fetch backend for
each line, and fetches a file when it first reads or sizes it. The settings
are CMake cache variables, given to `emcmake cmake`:

- `LRR_EMSCRIPTEN_HOST`: the host's URL prefix, such as
  `https://example.com`, as `ISLE_EMSCRIPTEN_HOST`. Empty (the default) is
  the page's own origin, so `<page>/LegoRR/` holds the data.
- `LRR_EMSCRIPTEN_CHUNK_SIZE`: the bytes per range request (default 524288,
  isle-portable's 512 KiB; at most 2147483647, which the backend reads as a
  signed number). The backend fetches a file of at most twice this, or from a
  host that does not send `Accept-Ranges: bytes`, whole on first use, and a
  larger file in chunks. With the default that makes the two WADs and a few
  streamed sounds and FLICs chunked and every other file whole.
- `LRR_EMSCRIPTEN_PRELOAD_WADS` (on): see below.

The WADs are fetched in chunks, and at start-up they are read through, in
requests of eight chunks, with progress events for the page. The game does not
leave them mostly unread: reaching the main menu already reads 122 of
`LegoRR0.wad`'s 151 chunks (61 MiB of 75), and a level then reads the rest,
so chunks save little bandwidth. Chunks still beat one whole-file request: the
page shows "Fetching LegoRR0.wad: 58%" where the game, blocked in its first
read, would show nothing, a host without range support still works (the
backend then fetches the file whole), and the data arrives in requests
that a CDN caches. Measured with the web build through a throttled local
server (40 ms round trip, 50 Mbit/s; one run each), the time from loading the
page to the main menu was 42.3 s with 512 KiB chunks and the game's own
reads, 40.8 s with the start-up read (19 requests for `LegoRR0.wad`, not 104),
and 38.8 s with every file whole; at a 150 ms round trip, 85.0 s against
72.9 s with the start-up read. `LRR_EMSCRIPTEN_PRELOAD_WADS=OFF`
leaves the WADs to the game's own reads, and a chunk size of 2147483647
fetches every file whole.

The page listens for events the game dispatches on the canvas: `dataProgress`
(`detail.name`, `detail.progress` in percent) while a WAD is read through,
`dataReady` once the data is set up, and `dataError` (`detail.message`) when
it cannot be (no manifest on the host, or one that lists no files). `shell.html` shows them in its status
line.

The host must serve `LegoRR/` with:

- `Accept-Ranges: bytes` and range responses (`206`) for the WADs, and a
  `HEAD` request with `Range: bytes=0-` that answers with `Content-Length`.
- The URLs the backend asks for, which have a doubled slash
  (`<host>/LegoRR//Data/...`, as isle-portable's do): the host must treat it
  as one slash, as most servers and CDNs do.
- When the host is not the page's origin: CORS (`Access-Control-Allow-Origin`
  for the page's origin) with `Access-Control-Expose-Headers:
  Accept-Ranges, Content-Length, Content-Range`, because the backend reads
  `Accept-Ranges` and, without it, fetches every file whole; and, since the
  page is cross-origin isolated, `Cross-Origin-Resource-Policy:
  cross-origin` or CORS on every response.

`tools/serve_web.py` is such a host for a local run. With `--data` it serves
a game directory at `/LegoRR/`, with the manifest generated on the fly, so
nothing is written into the game directory:

```sh
tools/serve_web.py build-web --data /path/to/game          # http://127.0.0.1:8000/LegoRR.html
```

Add `--port 8001 --cors-origin http://127.0.0.1:8000` to serve the data from
another origin than the page, with `-DLRR_EMSCRIPTEN_HOST=http://127.0.0.1:8001`
in the build. The licensed data stays on that machine; keep it out of any
repository and any CI run, which use synthetic data only.

To deploy, build with `LRR_EMSCRIPTEN_HOST` set (or empty, for one origin),
then publish three things: `LegoRR.html`, `LegoRR.js` and `LegoRR.wasm` from
the build directory (the page's origin, served with
`Cross-Origin-Opener-Policy: same-origin` and
`Cross-Origin-Embedder-Policy: require-corp`, as `tools/serve_web.py` does,
because the game's threads need `SharedArrayBuffer`); the game directory's
`LegoRR*.wad` and `Data` (minus what the manifest leaves out) at
`<host>/LegoRR/`; and the manifest `tools/web_manifest.py` wrote at
`<host>/LegoRR/LegoRR.manifest`. The page and the data may share an origin or
not. Regenerate the manifest whenever the data changes, and give the data a
cache lifetime that suits it, since the backend never revalidates a file it
has fetched during a visit.

A page that bundles the data itself still works: when `/LegoRR` already
exists in the page's filesystem, the game streams nothing (a fetch backend
can only make files in its own directories, so it cannot add what a bundle
lacks). For a local run without a server for the data, preload a game
directory into the build; the result holds the licensed data, so keep it to
yourself:

```sh
emcmake cmake -S . -B build-web-data -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF \
  "-DCMAKE_EXE_LINKER_FLAGS=--preload-file /path/to/game@/LegoRR --exclude-file *.avi --exclude-file *.exe"
cmake --build build-web-data --target program
tools/serve_web.py build-web-data
```

## Nintendo 3DS build (devkitARM)

The 3DS build (issue #931) uses devkitPro's devkitARM toolchain, from the
`devkitpro/devkitarm` container image as isle-portable's CI does. It gives
`LegoRR.3dsx`, for the Homebrew Launcher, and, when `bannertool` and
`makerom` are on the `PATH`, `LegoRR.cia` (`packaging/3ds`). The tests are
native programs, so configure without them:

```sh
tools/setup-miniwin.sh
podman run --rm -v "$PWD:$PWD" -w "$PWD" docker.io/devkitpro/devkitarm:latest \
  sh -c 'cmake -S . -B build-3ds -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF \
           -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/3DS.cmake &&
         cmake --build build-3ds'
```

A worktree's `.git` file names its repository's git directory, which the
configure step's patch check reads, so mount that too. The game reads its
data, `Data` and the WADs, from `sdmc:/3ds/LegoRR`, and keeps `LegoRR.ini`
and its saves there (`src/platform/native/3ds/config.cpp`). SDL writes its
log to `sdmc:/3ds/SDL_Log.txt`. Movies are off (`LRR_MOVIES`).

It needs a New 3DS. In Azahar on Citro3D, Level01 holds about 39 MB of heap
besides libctru's 32 MB linear heap (some 14 MB of it in use: textures, the
GPU command buffer and the frame's vertices), more than an Old 3DS gives an
application, so the `.cia` asks for the New 3DS's 124 MB mode
(`template.rsf`); the `.3dsx` gets what the Homebrew Launcher gives it.

miniwin lists Citro3D, the palette software renderer and the software
renderer here; the default device is Citro3D (`3ds/config.h`), which draws
the level: immediate mode is lit on the CPU and rasterized by the GPU, at
about 15 frames a second in Azahar. The game draws on the 320x240 bottom
screen, the touch screen; the top screen shows libctru's console. A 3DS has
no windows, so the display is always full screen, at the configured mode
(640x480), which Citro3D fits onto the bottom screen; the software devices,
if chosen in `LegoRR.ini`, still show their frame unscaled on the top
screen. Citro3D has no second vertex colour, so a textured draw leaves out
its specular colour (the game turns specular off). A frame's GPU commands
get 1 MiB, about twice what Level01 needs; a draw that would overflow them
is dropped, with a warning in the log.
Touch on the 3DS: SDL makes mouse events from a touch only for the window
it reports the touch to, and on the 3DS that is the fullscreen window of the
touch screen's display, which SDL does not set for the game's window. So
miniwin creates the window on that display, and turns each finger event
into the pointer moving there and the left button pressing and releasing
(`windowmessages.cpp`, in `patches/miniwin`). A touch is a mouse click at the
touched point and nothing more: no right click, scrolling, zooming or
keys, which #986 tracks for Android and iOS, and the 3DS's buttons and
circle pad are not mapped to anything.
On the SD card, where every lookup is a slow service call, the
path mapping (`files_native.cpp`) reads each directory once and matches
case from that listing.

Sleep mode, the HOME menu and the system's exit request reach the game
through libctru's APT hooks (`src/platform/native/3ds/apthooks.cpp`, adapted
from isle-portable's). SDL's event pump runs them, because it calls
`aptMainLoop` every time the game pumps its window messages. They become the
application lifecycle (`src/platform/native/lifecycle.h`), which the game
takes as the paths it already has:

- Suspend and resume are losing and regaining focus. A full-screen game
  never loses focus in the original, so the main loop reads the lifecycle
  there (`[divergence:suspend-pause]`). The first frame after a resume is
  inactive, and the level pauses on an inactive frame when
  `LoseFocusAndPause` is on, which also stops its sounds
  (docs/divergences.md).
- Exit is the close request that the window's close button gives, which
  `WinMain`'s message loop handles, so saves and settings are written as on
  the desktop.

The main loop already clamps a frame's step to 3 (0.12 s at the original's
25 frames a second), so the time spent suspended is one clamped step, not a
jump. Azahar without a HOME menu does not deliver the events, so they have
been checked only on the desktop (`lifecycle` in ctest). Android's and iOS's
`SDL_EVENT_WILL_ENTER_BACKGROUND` and `SDL_EVENT_DID_ENTER_FOREGROUND` could
report the same events.

The icon and banner show the app icon's 40x40 pixel art at whole-number
scales, so it stays sharp, and the banner sound is original
(`packaging/3ds/regenerate.py` renders all three). An emulator such as [Azahar](https://azahar-emu.org/) runs
the `.3dsx` with the game data in its emulated SD card's `3ds/LegoRR`.

## Android

The Android app (issue #927) is the Gradle project in `packaging/android`,
adapted from isle-portable's `android-project`. Building it needs JDK 17
(Temurin is what CI uses) and the Android SDK with platform 35, NDK
27.3.13750724 and the SDK's CMake 3.31.5; Gradle installs a missing NDK or
CMake when the SDK licences are accepted, or install them first:

```sh
sdkmanager "platforms;android-35" "ndk;27.3.13750724" "cmake;3.31.5"
tools/setup-miniwin.sh
cd packaging/android
ANDROID_HOME=/path/to/sdk ./gradlew assembleDebug
# app/build/outputs/apk/debug/app-debug.apk
```

The wrapper fetches Gradle 8.12 and checks its SHA-256. The app's package
is `io.github.robbert229.LegoRR` (`LRR_APP_ID`), named "LRR Portable",
with launcher icons that `packaging/icons/regenerate.py` renders from the
project's own icon. Gradle builds the repository's CMake project once per
ABI; on Android, `CMakeLists.txt` makes the program the shared library
`libLegoRR.so`, which SDL's Java activity (`LegoRRActivity`) loads before
calling its `SDL_main` (`entry.cpp`) on SDL's main thread. `WinMain`
blocks there, as it does on the desktop. SDL3 is SDL's own Android archive
(AAR) from the 3.4.18 release, which is also the release CMake fetches
elsewhere. `app/build.gradle` downloads it into `app/libs`, checks its
SHA-256, and links it with `find_package(SDL3 ... COMPONENTS Shared)`.
Movies are off (`LRR_MOVIES`), as Android has no FFmpeg.
`-PcmakeArgs="..."` passes more CMake arguments.

**ABIs.** The APK holds `arm64-v8a` and `x86_64`: 64-bit ARM is every
phone and tablet sold in recent years (Google Play requires 64-bit code),
and `x86_64` runs in the emulator and on Chromebooks. `armeabi-v7a` and
`x86` are left out: devices that run only 32-bit code are old or low-end
and a shrinking share, and each ABI adds about 3 MB to the APK and another
full build to CI. Both still build:
`./gradlew assembleDebug -PabiFilters=arm64-v8a,armeabi-v7a,x86_64,x86`.

**Display.** The game is landscape and full screen (`full screen = true`
is Android's default in `LegoRR.ini`): an Android window always fills the
screen, and the full-screen mode is what lets miniwin fit the game's 640x480
into it. The default 3D device is OpenGL ES 3 (`ANDROID_DEFAULT_3D_DEVICE`),
which every device from Android 4.3 on supports, in the OpenGL window its
name asks for. The software renderer is the fallback when OpenGL ES 3 cannot
be set up, and `3d device = Miniwin driver (Miniwin Emulation)` selects it.
SDL GPU is usually not listed on Android: it cannot share the OpenGL window,
and upstream lists it there only once DirectDraw has a window, after the game
has listed its devices.

**Game data.** The app reads the game data from its external files
directory, `/sdcard/Android/data/io.github.robbert229.LegoRR/files/`
(`legorr:diskpath`), which also holds `LegoRR.ini` and the `saves`
directory. No permission is needed. A folder picker on first run is #909;
until then, copy your own game files in with `adb`:

```sh
P=io.github.robbert229.LegoRR
adb install app-debug.apk
adb shell am start -W -n $P/.LegoRRActivity   # creates the directory
adb shell am force-stop $P
cd /path/to/game
adb push LegoRR0.wad LegoRR1.wad Data /sdcard/Android/data/$P/files/
adb shell chmod -R a+rwX /sdcard/Android/data/$P/files/Data
```

Push `Music` the same way, and `chmod` it too. The `chmod` matters: a
directory that `adb push` creates belongs to the shell user, and the app
cannot read it until the directory is opened to all users. The files pushed
straight into `files/`, like the WADs, are readable without it. Without the
`chmod` the game still runs, but without what it reads from `Data`, such as
the menu's animations from `Data/AVI` and the sounds in `Data/Sounds`. #909
tracks a better way in. Movies are off, so the `.avi` files in `Data/AVI` (about
370 MB) can be left out; keep the `.flh` files beside them. Don't edit
`LegoRR.ini` in place with `adb shell`, because the file it writes belongs
to the shell user and the app can no longer write it. Pull the file, edit
it, and push it back, or delete it to get the defaults again. Uninstalling
the app deletes the directory and everything in it, including the saves.

**Touch.** SDL turns a touch into mouse events: a tap moves the pointer to
the finger and presses and releases the left button, and miniwin passes
those on as `WM_MOUSEMOVE` and `WM_LBUTTON*`. A press of a frame or more
clicks, in the menus and in a level; a tap shorter than one game frame can
be missed, because the game reads the button state once per frame. A mouse
and keyboard work. #986 tracks what touch still lacks: a right click,
scrolling and zooming the map, the keys the game asks for (Space, Escape,
typing a save's name) and Android's Back button.

**Signing.** CI builds only the debug APK, signed with Android's debug key,
so it installs for testing. `./gradlew assembleRelease` gives an unsigned,
minified `app-release-unsigned.apk`. Signing a release is the owner's
decision, and it needs:

- a release keystore that the owner generates once (`keytool -genkeypair`)
  and keeps safe. Every update must be signed with the same key.
- four repository secrets: the base64 keystore, its password, the key alias
  and the key's password.
- a CI step that decodes the keystore and signs the APK, either with
  `apksigner sign` after `assembleRelease` or with a `signingConfig` in
  `app/build.gradle` that reads the four values from the environment, as
  isle-portable's does.
- for Google Play, an Android App Bundle (`bundleRelease`) and Play App
  Signing.

## Tests

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

`make test` runs the same build and ctest in `BUILD_DIR`. The tests call
units directly (bezier, steering, animation_object, draw, registry) or
miniwin (miniwin_dx6), on synthetic data only. Passing tests establishes the
behavior covered by those tests; gameplay comparison against the original is
a separate check.

The miniwin rendering tests (`miniwin_immediate`, `_viewports`,
`_textures`, `_lights` and `_fog`) draw with the software renderer. Their
`_sdlgpu` twins run the same checks on the SDL GPU backend, and
`miniwin_backends` draws one scene on both backends and compares them pixel
by pixel, with the tolerance its source describes. These need an SDL GPU
device that can draw on SDL's offscreen video driver, and are skipped
without one, as on CI. Mesa's lavapipe is such a device without a GPU:

```sh
VK_DRIVER_FILES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json \
  ctest --test-dir build -L sdlgpu --output-on-failure
```

A GPU driver that cannot present to an offscreen surface (NVIDIA's, for
one) runs them on the desktop instead:
`MINIWIN_TEST_DEVICE="SDL3 GPU HAL" MINIWIN_TEST_VIDEO_DRIVER=wayland
build/miniwin_immediate_test`, and `MINIWIN_TEST_VIDEO_DRIVER=wayland
build/miniwin_backends_test`.

The SDL GPU backend's shaders are HLSL in miniwin's
`shaders/src`, compiled into the headers in `shaders/generated`.
`tools/miniwin-shaders/regenerate.sh` regenerates them on Linux with
SDL_shadercross and vkd3d-utils; its header lists what it needs.

### Coverage

```sh
make coverage
```

builds the port in `build-coverage` (`COVERAGE_BUILD_DIR`) with
`-DLRR_COVERAGE=ON` and `CMAKE_BUILD_TYPE=Debug`, clears the counts of any
earlier run, runs ctest, and then runs `tools/coverage.py`. That writes the
line-by-line report to `build-coverage/report/html/index.html`
(`COVERAGE_DIR`), and `comment.md` beside it: the coverage of the lines the
branch changes against `origin/main`, and coverage by area of `src/`.
`LRR_COVERAGE` compiles and links every target with `--coverage`, and needs
GCC; the report needs `gcovr` and `diff-cover` on `PATH` (CI pins the
versions in `.github/workflows/coverage.yml`). The report counts `src/` less
the tests themselves (`src/test/` and `test_*` sources). With the game data
in place, the real-data tests count too, so a local report runs higher than
CI's.

### Writing a test

A test is one source file, `test_<name>.cpp`, registered with
`lrr_unit_test` (`cmake/LrrTesting.cmake`, whose header lists every option)
in the `tests.cmake` of the test's own directory: `src/game/tests.cmake`,
`src/gods98/tests.cmake`, `src/platform/native/tests.cmake`,
`src/library/tests.cmake` or `src/crt/tests.cmake`. The root
`CMakeLists.txt` only includes those files; a new test leaves it alone. A
test in another directory gets a `tests.cmake` there, added to the root's
list.

```cmake
# What the test establishes, in a sentence or two.
lrr_unit_test(block_grid SOURCES ${GAME_DIR}/test_block_grid.cpp
  WRAP ddiscene_object_position_get entity_state_flags_get
  LABELS block_grid)
```

That builds `block_grid_test`, links it with `units` (`LINK` replaces that),
and adds the ctest test `block_grid`. `WRAP` reroutes the units' calls to a
symbol to the test's `__wrap_<symbol>` through GNU ld's `--wrap`; where the
toolchain has no `--wrap` (Apple, Windows, compilers other than GCC and
Clang), `lrr_unit_test` leaves such a test out, so it needs no platform
check around it. `ARGS`, `ENVIRONMENT`, `SKIP_RETURN_CODE` (77 for a test
that skips), `INCLUDES` and `DEFINES` cover the rest. The `WRAP` list is the
test's dependency surface on the other units, so it stays beside the source
it stubs.

Another run of the same executable, with other arguments or environment, is
`lrr_unit_test_variant`, which takes the same `ARGS`, `LABELS`,
`ENVIRONMENT` and `SKIP_RETURN_CODE`:

```cmake
lrr_unit_test_variant(audio_mix OF audio
  ARGS disk
  LABELS audio
  SKIP_RETURN_CODE 77)
```

The assertions are in `src/test/lrr_test.h`, header-only, on every test's
include path:

```cpp
#include "lrr_test.h"

int main(void)
{
    CHECK(grid != NULL, "the grid is allocated");
    CHECK_EQ(block_index(grid, x, y), 0u);
    CHECK_NEAR(position.x, 1.2f, 1e-6);
    CHECK_STREQ(record.name, "Building3");
    return lrr_test_result("block_grid");
}
```

A failed check prints its file and line, and for the `CHECK_EQ`, `_NEAR`
and `_STREQ` forms the value it got and the one it expected, then the test
carries on. `lrr_test_result` returns 1 if any check failed. Scratch
directories for tests that write files are in
`src/platform/native/test_fixture.h`, and a miniwin device for drawing
tests in `src/platform/native/test_miniwin_device.h`.

## Continuous integration

The `native-build` workflow (`.github/workflows/native-build.yml`) runs on pull
requests and pushes to `main`. Its check is required for merging. It checks out
the repository without submodules, runs `tools/setup-miniwin.sh` to fetch only
`miniwin/` and apply `patches/miniwin`, and then runs the native commands from [Tests](#tests). It also
lints the units (no `LEGORRR_BACKEND_*`, `MINIWIN`, `_MSC_VER`, `_M_IX86` or
`_WIN32` guards and no `lpVtbl` in `src/`) and runs the Python tool tests in `tools/tests`.
The build itself treats the compiler warnings in `LRR_LINT_WARNINGS`
(`CMakeLists.txt`) as errors in the units, `seam_stubs` and `program`, so a
change that adds one fails the check. Today that is `-Wparentheses` (MSVC
C4706): an assignment used as a condition. Add a warning to the list only
once the code builds cleanly with it.

SDL3 comes from a pinned source build of release-3.4.18 (commit `829a65d7`),
configured headless with `SDL_UNIX_CONSOLE_BUILD` and without X11 or Wayland,
by the `setup-sdl3` action (`.github/actions/setup-sdl3`), which the
`coverage` workflow shares. The tests use SDL's dummy and offscreen drivers.
The build is cached by SDL version. `ubuntu-24.04` has no SDL3 development
package, so a distro package is not used. When you change the SDL version or
its build recipe, update `SDL_VERSION`, `SDL_COMMIT`, or `SDL_CACHE_VERSION`
in that action, and
keep `LRR_SDL3_GIT_TAG` in `CMakeLists.txt` on the same commit. An existing
build directory keeps its cached `LRR_SDL3_GIT_TAG`; delete that entry, or
the directory, to pick up a new one.

The `coverage` workflow (`.github/workflows/coverage.yml`) runs on the same
events and is not a required check. On `ubuntu-24.04` it builds as
[Coverage](#coverage) describes, with the pinned SDL3, and runs ctest and
`tools/coverage.py`. On a pull request it posts one comment, which later
pushes update: the coverage of the lines the pull request changes, with the
lines the tests miss, and coverage by area of `src/` against the last
`coverage` run on `main`. A push to `main` uploads that run's `summary.json`
as the `coverage-summary` artifact for later pull requests to compare
against. Every run writes the comment to its job summary and uploads the
HTML report as the `coverage-report` artifact. A pull request from a fork
gets a read-only token, so it gets no comment; its coverage is in the job
summary. Nothing is sent outside GitHub.

The `native-packages` workflow (`.github/workflows/native-packages.yml`) builds
the Linux packages: a TGZ and an AppImage on `ubuntu-24.04` (with the AppImage
checks and a smoke run of the image), and the Flatpak on x86_64 and aarch64
in the `flatpak-github-actions:freedesktop-24.08` container. It is not a
required check. Every package workflow uploads its packages as artifacts;
there is no continuous release. The `release` workflow
(`.github/workflows/release.yml`) calls `native-hosts` and `native-packages`
on a `v*` tag, or by hand, and attaches their packages to a draft GitHub
release; see [Releasing](releasing.md).

The `native-hosts` workflow (`.github/workflows/native-hosts.yml`) runs on the
same events and builds the port on macOS arm64 (`macos-latest`), macOS Intel
(`macos-15-intel`), both with `CMAKE_OSX_DEPLOYMENT_TARGET=12.0`, FreeBSD
15.1 in a VM, and Windows: MSVC x86 and x64 (`windows-2025`, x86 running
under WOW64), MSVC arm64 (`windows-11-arm`), and msys2 MinGW64. Its jobs are
not required checks. The macOS and Windows jobs also build the CPack package
and upload it. Each runs `tools/setup-miniwin.sh`, then the commands
from [Tests](#tests) with Ninja and a Release build; SDL3 is the static build
configure fetches at `LRR_SDL3_GIT_TAG`, with SDL's default drivers, except on
MinGW64, which uses msys2's `mingw-w64-x86_64-sdl3` package. The `--wrap`
tests need GNU ld options and are not built on macOS or Windows. The build
turns off fused multiply-add (`-ffp-contract=off`), which Clang otherwise uses
on arm64; MSVC builds with `/fp:precise`, which does not contract.

On Windows, `config` skips: SDL keeps the preference directory in the user's
AppData, which the test cannot move into its fixture. The MSVC jobs set the
checkout's `core.autocrlf` to false, so `patches/miniwin` applies, and run
CMake from PowerShell with `-DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl`
inside a developer environment (`ilammy/msvc-dev-cmd`), because Git Bash's
`link` would shadow MSVC's linker. MinGW links its C and C++ runtimes
statically, and a build against a shared SDL3 copies `SDL3.dll` beside
`LegoRR.exe` (and the tests).

The same workflow's `nintendo-3ds` job, also not required, builds the 3DS
target in the `devkitpro/devkitarm` image (pinned by digest), with
`bannertool` 1.2.0 and `makerom` 0.18 downloaded and checked against a
SHA-256, and uploads `LegoRR.3dsx` and `LegoRR.cia` as the `LegoRR-3ds`
artifact.

The same workflow's `android` job, also not required, builds the debug APK
on `ubuntu-24.04` (see [Android](#android)). It checks the Gradle
wrapper's jar (`gradle/actions/wrapper-validation`), sets up Temurin JDK 17,
installs the pinned NDK and CMake with `sdkmanager`, runs `./gradlew
assembleDebug`, and uploads the APK as the `LegoRR-android-debug` artifact.
Every download it makes is pinned and checked: Gradle by the wrapper's
`distributionSha256Sum`, SDL's AAR by `app/build.gradle`, and the NDK and
CMake by the SDK repository's checksums. It signs nothing and uses no
secrets.

The same workflow's `web-emscripten` job, also not required, sets up emsdk
6.0.10 (`emscripten-core/setup-emsdk`), builds the web target as in
[Web build](#web-build-emscripten) and uploads `LegoRR.html`, `.js` and
`.wasm` as the `LegoRR-web` artifact.


## Packages

`cmake --build build --target package` (or `cpack` in the build directory)
writes `lrr-portable-<version>-<system>-<arch>` to `build/dist`; the
`LRR_PACKAGING` option, on for Linux, macOS, FreeBSD and Windows, adds the
install rules and CPack settings from `packaging/`. It follows
isle-portable's `packaging/` tree, and the same files hold what each host
needs:

- **Linux:** a TGZ with `LegoRR`, a desktop entry and AppStream metadata
  (generated from the templates in `packaging/linux`) and the icon. The
  AppImage is `packaging/linux/appimage/build.sh build`: it installs into
  `build/AppDir`, lets linuxdeploy copy in the libraries `LegoRR` needs,
  swaps in the project's `AppRun`, and packs it with appimagetool. Both tools
  are pinned releases, checked against a SHA-256. `check.sh` then fails the
  build when anything in the image needs a glibc newer than 2.39 (Ubuntu
  24.04's, the CI host) or a CPU above baseline x86-64. The AppImage is
  x86_64 only. The Flatpak
  (`packaging/linux/flatpak/io.github.robbert229.LegoRR.yml`, x86_64 and
  aarch64) builds on the freedesktop 24.08 runtime. It copies the working
  tree, so run `tools/setup-miniwin.sh` first, and it fetches SDL3 during the
  build. Movies in the Flatpak need the runtime's
  `org.freedesktop.Platform.ffmpeg-full//24.08` extension.
- **macOS:** a DMG holding `LegoRR.app` (`packaging/macos`): `Info.plist`, the
  icon, and an ad-hoc signature, which Apple silicon needs to run the binary.
  It is neither Developer ID signed nor notarized, so Gatekeeper asks on the
  first launch (right-click, Open). With packaging on, the build tree's
  program is `build/LegoRR.app/Contents/MacOS/LegoRR`, which `make run` finds.
  `packaging/macos/verify.sh` checks a DMG.
- **Windows:** a ZIP with `LegoRR.exe`, `LICENSE.txt`,
  `THIRD-PARTY-NOTICES.txt`, the Visual C++ runtime DLLs in an MSVC build
  (`InstallRequiredSystemLibraries`), and `SDL3.dll` for a shared SDL3.

Every package carries `LICENSE` and `THIRD-PARTY-NOTICES.txt`.
`tools/check_package.py` checks a package for them, and for anything outside
its allow-list, such as game data. Each package job runs it, except the
Flatpak jobs, whose bundles the release workflow checks; and
[Releasing](releasing.md) covers the release workflow that ships them.

The icon is original pixel artwork drawn for this repository
(`packaging/icons/lrr-portable.svg`, LGPL-3.0-or-later): a drill boring into
rock and ore. Its composition is styled after the original game's drill icon,
but it holds none of the game's image data, and the repository ships no game
data in any package. `packaging/icons/regenerate.py` renders the SVG into the
macOS bundle's `AppIcon.icns` and the Android launcher icons, and
`packaging/3ds/regenerate.py` renders the 3DS icon; all are committed.

The application id is `io.github.robbert229.LegoRR`, set once as `LRR_APP_ID`
in `packaging/CMakeLists.txt` and used by the desktop entry, the AppStream
component, the Flatpak and the bundle identifier. It follows Flathub's
`io.github.<owner>.<name>` convention for a project on GitHub.

Every package expects the owner's own game files: set `diskpath` in
`LegoRR.ini` (see the README) to the directory that holds `Data` and the WADs,
because a launcher or an AppImage does not start in it. The Flatpak reads
`/run/media`, `/media`, `/mnt` and the home directory read-only.

## Sources

`src/link_order.cmake` is the source list: one
`lrr_link(<kind> <key> <library> <source>)` line per source, with the library
(`game`, `gods98` or `platform`) it belongs to. Its header explains the
fields. Its entries of kind `unit` are the 83 original-shape units, which
compile as C++17 into one library; kind `platform` is the process entry and
the seam stubs.

## Running the original

`make reference-run` runs the shipped `LegoRR.exe` from `REFERENCE_DIR`
(default `reference/`) under Proton, with the
[compatibility harness](../tools/compat/README.md)'s runner and prefix. Set
`PROTON_BIN` (or `RR_A2_PROTON`) and `COMPATDATA` to override them.

## The matching build

`main` has no matching build, Reccmp gate or Windows reference backend
(ADR 0007). They live on the `decomp` branch, where the decomp tag will be
cut; see `decomp`'s `docs/building.md`.

### Synthetic headless level fixtures

Tests can add `${PROJECT_SOURCE_DIR}/src/test/synthetic_level_fixture.cpp`
to their `lrr_unit_test(... SOURCES ...)` and include
`test/synthetic_level_fixture.h`. The helpers are test-only:

- `make_fixture("unique_test_fixture")` replaces the named scratch
  directory beneath the test working directory, enters it, and writes
  artificial files under Data/L. Use a different directory for each
  concurrently running test.
- `write_text`, `write_map`, `write_bmp`, and `write_triangle` also
  work independently with caller-supplied paths (parent directories must
  exist). Maps take dimensions and a cell-value callback; BMPs are
  grayscale, and the LWOB is a single triangle.
- `initialise_headless("UniqueTestName")` initializes files, SDL and the
  software device. Register the test with
  `ENVIRONMENT "SDL_VIDEO_DRIVER=dummy"`.
- `boot_game()` initializes the minimal subsystems needed by the existing
  synthetic level loader. It is not the complete startup routine.
  After it succeeds, call `lego_level_load("Level01")` and inspect the
  resulting state. Run each scenario in a separate process: the helpers
  do not reset all global state or perform shutdown.

No installed game data is needed. All input files are generated in the
scratch directory; the fixture is an artificial two-cave grid, not a
shipped level.

The separate `synthetic_lifecycle` test exercises two real load/tick/free
cycles, including empty-level teardown. The `synthetic_missing_config`
variant calls the complete startup entry point in a fresh process without
a configuration file and checks that it rejects startup without installing
a level. These tests do not establish successful complete initialization
or full engine shutdown: the successful load uses the partial setup above,
which lacks the complete object and interface catalogs. Coverage of those
paths remains outstanding.

#### Startup and shutdown subsystem tests

The four `synthetic_startup*` scenarios use separate working directories
and the software device, with only the grayscale BMP inputs that each
scenario needs. They exercise real functions without replacing the game
units:

- Reward initialization and shutdown: a generated image and font, a
  short synthetic text record, and the allocation of the reward item table.
- Reward initialization with a missing image: the item retains the text
  and font, but has no image pointer or image-present flag; shutdown
  still completes.
- Interface initialization and shutdown: menu name registration,
  allocation of the menu items and initial position.
- Scene initialization and shutdown: a root container, a first-person
  camera, two viewports, and release of the viewports, camera, container
  pool and software DirectDraw device in that order.

Full game startup and shutdown are **not** covered by these tests.
In a local synthetic probe, `lego_engine_shutdown` after the partial
boot originally failed in `reward_shutdown` because reward initialization
was omitted. After real reward and interface initialization, it reached
camera cleanup and crashed in `Direct3DRMFrameImpl::GetParent` while
freeing a camera after root destruction. A separate probe of
`lego_game_config_init` with a minimal on-disk config and generated
font, message and loader BMPs reached the failure-path
`ddiscene_object_pool_shutdown` and crashed in the same frame method.
The first-person scene test deliberately releases its camera before the
root; the overhead camera's cleanup also did not complete in a probe.
These probes do not establish whether the crashes occur after a complete
production startup. No production changes or stubbed scene cleanup
were made to bypass them. The existing missing-config test still
exercises the early startup rejection.

Local coverage measurement (GCC Debug build, `LRR_COVERAGE=ON`,
gcovr 8.8, repository's source filters excluding test code): with counters
cleared, the 144 pre-existing CTest cases yielded 15,835/43,166 lines
(36.7%), 1,540/3,115 functions (49.4%) and 9,107/43,738 branches (20.8%).
Adding the four subsystem scenarios in the same instrumented build
yielded 15,980 lines (37.0%), 1,558 functions (50.0%) and 9,186 branches
(21.0%). These are local incremental results, not a comparison against
a separate main CI run.

#### Positive startup configuration and resource loading

`positive_startup` loads all five object type catalogs through the real
catalog functions, using two generic vehicle entries and one entry each
for the figure, creature, building, and upgrade catalogs. The only model
geometry is a triangle; `write_null_scene(path)` adds a reusable LWS
fixture containing one moving null (Anchor) with two keyframes and no
external mesh or sound. The test verifies the activity selection, wheel
mesh and explicit radius, upgrade-part binding, attachment metadata,
building shape and power scene, and both display-name tables.

Three separate process variants test additional startup paths:

- `startup_missing_catalog`: an absent upgrade section and missing
  model files in each of the other four catalogs are rejected.
- `startup_misc_resources`: maps required resource slots to the same
  triangle or null scene (with a minimal activity wrapper when required),
  verifying that the loaded templates are hidden. None represents the
  named game object or its appearance.
- `startup_config_loader`: checks graphics quality acceptance and
  rejection, configured tooltip and upgrade names, and the real software
  loading and shutdown screens, font, progress callback, and profile
  write-back.

The catalog and miscellaneous resource tests are process-isolated rather
than exercising full scene shutdown (see the preceding teardown probes).
A local probe calling the real `lego_game_config_init` with the new catalog
and generic font, message and loader BMPs progressed past all five catalog
gates, then crashed in `container_load` at its filename copy in
`lego_misc_object_resources_load` because a required filename was absent.
With the generic miscellaneous resource inputs supplied, the same probe
progressed further, to `info_set_overflow_image_file`, and crashed in
`Image_SetPenZeroTrans` when the required overflow image was missing.
Neither probe is registered as a passing test. Complete initialization
still requires additional interface resources and configuration, and the
earlier shutdown ordering problems remain unresolved. No production
path was replaced or failure masked.

Local matched coverage on the stacked building-lifecycle base (GCC Debug,
`LRR_COVERAGE=ON`, gcovr 8.8, test sources excluded): the 158 existing
cases covered 16,645/43,166 lines (38.6%), 1,611/3,115 functions (51.7%)
and 9,618/43,738 branches (22.0%). Adding the four startup scenarios
reached 17,338 lines (40.2%), 1,648 functions (52.9%) and 10,110 branches
(23.1%). The 693 additional covered lines include 265 in
`lego_unit.c`, 160 in `container.c`, 62 in `vehicle_model.c`,
53 in `building_model.c` and 20 in `animation_object.c`.
The full suite registered 162 cases, with 150 passing and the 12
optional renderer-backend cases skipped.

#### Scene ownership and teardown regression

The `scene_teardown*` scenarios exercise generic retained-mode frames,
all three camera types and their viewports, and real resource cleanup.
The former scene teardown crashes above were reproduced without any
model or level files: after `Container_Remove(root)`, calling
`Container_GetParent(child)` segfaulted in
`Direct3DRMFrameImpl::GetParent`. The camera-first and root-first
teardown orders also crashed at the same frame method in
`camera_free`. The parent frame owns a reference to its children
through its child array, whereas each child has a non-owning parent
pointer. Destroying the parent released its array without clearing
that pointer, leaving a surviving child pointing to freed storage.

The miniwin patch clears those back links before releasing the child
array. It does not change the game units, scene destruction order or
player-visible behaviour. The regression retains the root child array
across root destruction, checks that the child's parent becomes null
and the grandchild still refers to its live parent, then removes the
remaining containers. The camera scenarios test both viewport-first
cleanup and the engine's root-first order (primary viewport, root,
cameras, container pool, remaining viewports).

Additional cases verify:

- Missing mesh, animation and activity files do not leave extra child
  frames; valid triangle and null-scene resources can load afterwards,
  the live pool shuts down, and a fresh pool can initialise.
- The real `lego_game_config_init` rejects an incomplete on-disk config
  after camera setup. Only solid grayscale placeholders are supplied
  for early font/message/loader readers. The scene pool cleanup returns
  safely and can be initialised again after releasing the remaining
  viewports. This does not establish that every startup allocation is
  reclaimed.
- The real `lego_engine_shutdown` completes after the existing partial
  boot fixture loads and ticks its artificial level. The test first
  initialises reward records, owned font placeholders, interface and
  blank panel buttons omitted by partial boot, and checks that shutdown
  clears the current level.

These results supersede the earlier scene teardown crash probes. A
successful complete `lego_game_config_init` is still not established;
the later interface-resource requirements remain untested. Full memory
leak accounting and successful full-initializer shutdown are outside
these regression assertions.

Local matched coverage on the `d00f4af1` stack tip (GCC Debug,
`LRR_COVERAGE=ON`, gcovr 8.8, test code and miniwin excluded): the
164 base cases covered 17,674/43,166 lines (40.9%), 1,685/3,115
functions (54.1%) and 10,358/43,738 branches (23.7%). With the patch
and six new scenarios the 170-case suite covered 17,972 lines
(41.6%), 1,706 functions (54.8%) and 10,606 branches (24.2%):
298 additional lines, including 177 in `lego_unit.c`. Both
instrumented runs had zero failures and 12 optional backend skips.

#### Distinct-grid level transitions

`make_transition_fixture(directory)` extends the existing fixture without
changing `make_fixture` or the no-argument `boot_game()`. It writes
`Data/transition.cfg` (open with `boot_game("transition.cfg")`) and two
independent abstract grid records: GridA (8x8, block size 40, vertical
barrier, two resource objects) and GridB (10x9, block size 32, horizontal
barrier, one resource object). GridB has its own map layers, empty PTL and
object list in Data/L2; `write_transition_surface()` restores its surface
file after a negative test. Both records specify fixed random seeds.
No shipped map or object artwork is involved.

The three `distinct_level*` cases run in separate scratch directories
with no save slot selected:

- A real `lego_keep_running` call follows GridA's configured next level
  to GridB. A second call follows GridB's self-reference to exercise
  restart. Both grids are ticked and their barriers and resource
  counters deliberately modified before leaving them.
- Removing GridB's PTL causes the real transition from GridA to fail.
  The test checks that no level or live object survives and restores
  the file before loading GridB again.
- Unloading GridA explicitly and removing GridB's surface causes a
  direct load failure. Restoring the generated map allows recovery.

All three cases check distinct dimensions, block size, terrain flags,
resource census and positions, object registry, oxygen and resource
balances, per-grid option flags, upgrade/train times, and explicit vs.
default configuration values. Restart also checks that a seeded random
block field is reproduced. Each recovered/current grid is ticked and
then passed to the actual engine shutdown, with assertions that the
current level and object registry are cleared.

These assertions establish logical state isolation, not that every
allocation has been reclaimed. The failure cases are before successful
map allocation; later failures such as an absent object list after
map loading are not covered. The existing complete-initializer
limitations remain; this uses the real level/transition entry points
after the documented partial setup, with no test-only production mode
or mocked startup calls.

Local matched coverage on the `5cc40e8f` stack tip (GCC Debug,
`LRR_COVERAGE=ON`, gcovr 8.8, test code and miniwin excluded):
171 base cases covered 18,529/43,166 lines (42.9%), 1,743/3,115
functions (56.0%) and 11,035/43,738 branches (25.2%).
Adding the three transition cases covered 18,579 lines (43.0%),
1,746 functions (56.1%) and 11,112 branches (25.4%): 50 additional
lines, 3 functions and 77 branches. The complete 174-case suite had
162 passing, 12 optional backend skips and zero failures.

#### Independent startup resources and host options

The `startup_resources` test uses two grayscale BMPs and a small,
independent configuration record. It runs the real panel and button
loaders (including a missing panel and a missing button image), checks
resolution fallback and bounds, loads a tutorial icon, registers tooltips
and their sound aliases, and loads text and info messages. Replacing an
image-bearing text message releases that image; the test releases its
font and the image pool in the real shutdown functions. Panel button
storage and text strings are freed explicitly where there is no exposed
subsystem shutdown function. The process is isolated; this does not
establish complete ownership or leak accounting for the static info
and tooltip records.

The two `startup_advisor` variants initialise a camera and viewport,
then load a single generic animated null into two configured animation
slots, with a third missing scene. They verify the actual child-frame
count, parsed positions with resolution fallback, invalid position
rejection, animation admission and end cleanup, then release the
viewport, camera, container pool and device. The reduced-animation
variant verifies that an optional `!key` is not loaded. The normal
optional animation fallback was observed to re-query the unprefixed key
instead of the generated prefixed key in `advisor_config_slots_load_target`;
this existing defect is not changed or asserted as desired behaviour.

`startup_options` exercises the existing WinMain option parser and
start-level override getter without invoking WinMain or the game
initializer. The `startup_device_fit`, `startup_device_original` and
`startup_device_best` variants load isolated explicit preferences (or
skip if the host cannot isolate its preference directory), initialise the
real dummy-driver window and DirectDraw selection and setup, check the
chosen device, mode and dimensions, then shut down DirectDraw and the
window. The hardware flag in the best-device test is the flag returned
by miniwin, not a claim that the dummy driver uses physical hardware.

Three additional rejection tests use linker wrapping solely at the
external driver/device enumeration and setup boundary: no driver,
no hardware-flagged device, and all setup attempts failing. The real
`init_initialise` must reject the first two without attempting setup and
try each enumerated device once in the last case. No successful device
setup or game callback is fabricated. The wrapping cases are registered
where the toolchain supports the existing GNU linker-wrap mechanism.

All input is generated in scratch directories. These tests do not
exercise or claim successful complete game initialization, and they do
not require a substitute game asset pack. Malformed bitmap decoding is
not covered by the missing-file tests.

Matched local coverage on the `913cbcfa` stack tip (GCC Debug,
`LRR_COVERAGE=ON`, gcovr 8.8, test code and miniwin excluded):
178 base cases covered 19,810/43,166 lines (45.9%), 1,834/3,115
functions (58.9%) and 12,108/43,738 branches (27.7%). The ten new
cases raised these to 20,052 lines (46.5%), 1,850 functions (59.4%)
and 12,380 branches (28.3%), an increase of 242 lines, 16 functions
and 272 branches. Newly covered lines include 83 in `init.c`, 48
in `lego_unit.c`, 41 in `advisor.c` and 27 in `directdraw.c`.
The complete 188-case suite had 176 passing, 12 optional renderer
skips and zero failures.

#### Native engine callback lifecycle

The `engine_lifecycle*` tests exercise the existing WinMain as a host for
generic application callbacks, not for the game. Each process supplies
only an isolated host INI and one trivial application configuration text
file, under its own generated scratch directory. The launch options
explicitly direct both game-data and preferences paths into that fixture.
No game models, levels or artwork are present.

The test executable compiles the unchanged `main.c` unit with just the
application-registration symbol renamed to the test's generic registry
function. It still calls the real `window_runtime_callback_triplet_set`,
and the window, software device, input, file layer, config pool, renderer
and loop are real. This test-only link seam avoids the GNU linker-wrap
restriction, so the tests are registered on Windows and other native
toolchains as well.

Seven separate processes cover:

- Fixed timestep, paused timestep, deterministic timestep precedence and
  resumption, plus the per-frame renderer update latch.
- Real-time stepping with a deliberately long first tick and the
  three-frame clamp; an enabled FPS overlay with no font exercises the
  missing-resource path.
- SDL focus loss/gain, keyboard acquire/unacquire, middle button and
  wheel state, and a platform quit event; the last callback does not
  request exit, so only the real window message terminates the loop.
- Fullscreen host suspend/resume, including the delayed active frame and
  system-key/focus filtering.
- A rejected init that releases its own config, with no subsequent tick
  or shutdown callback and all three callback pointers cleared.
- A successfully initialised application without a loop callback, which
  still receives shutdown.
- A null registration, which does not install or invoke a callback state.

Shutdown assertions verify the callback runs before the window and
configuration pool are destroyed, the generic application's config is
released, the pool is no longer resident, input is unacquired and the
window ID no longer resolves. An exit guard fails if file initialisation
or another engine path exits the process before the assertions complete.
The scenarios intentionally use `-nosound` and do not establish
successful game startup or exhaustive resource leak accounting.

Matched local coverage on the `0420c8b7` base (GCC Debug,
`LRR_COVERAGE=ON`, gcovr 8.8, test sources and miniwin excluded):
the 190 pre-existing tests covered 20,440/43,172 lines (47.3%),
1,877/3,115 functions (60.3%) and 12,722/43,748 branches (29.1%).
Adding these seven scenarios covered 20,593 lines (47.7%), 1,886
functions (60.5%) and 12,869 branches (29.4%): 153 new lines,
9 functions and 147 branches, including 126 lines in `main.c`.
The before/after denominator is from the same instrumented build,
which includes the second compilation of `main.c`. The complete
197-case suite had 185 passing, 12 optional renderer skips and
zero failures.

The host configuration variants add three more independent processes,
using the same generic callbacks and no game callbacks:

- `fit-config`: a 713x521 host window with the 640x480 virtual mode,
  window-resolution rendering, and combined left/right mouse handling.
- `debug-config`: an 800x600 mode accepted with `-debug`, virtual-mode
  rendering, and independent mouse buttons with `-dualmouse`.
- `resource-reject`: an unsupported window resolution falls back to
  640x480; the application's config marks a missing resource as required,
  so the callback releases its partial scene and rejects initialization.
  The host must not call either the loop or the application shutdown.

These variants create only an empty generic mesh and a spotlight, with
no geometry or texture files. They exercise real container parenting,
light setup, missing activity/animation/mesh resources, a valid config
without the optional activity group and unknown resource types. Both
successful and rejected callbacks release their scene before the host
destroys its device and window. The successful cases also assert bounded
mouse-event progression through the real native loop.

Matched coverage from `d6e074b2` (GCC Debug, gcovr 8.8, same production
source filters) increased from 23,987/43,178 lines (55.6%) to
24,049/43,178 (55.7%), from 2,115 to 2,116 of 3,115 functions, and
from 15,471 to 15,506 of 43,752 branches. The 62 additional lines are
36 in `main.c` and 26 in `container.c`; three additional branches
are in `init.c`. Full CTest: 207 passed, 12 existing optional renderer
skips, zero failures. This is independent host coverage, not a successful
game initialization or exhaustive leak check.
The three new cases also passed in a local MinGW64 cross-build (GCC 14,
SDL3 3.4.18) under Wine; native Windows CI is a separate verification.

#### Real initializer smoke test

`synthetic_initializer_smoke` exercises the actual `lego_game_config_init`
entry point rather than the partial-boot helper. It combines the existing
abstract grids with the primitive catalog and generates uniform grayscale
placeholder inputs only for the resource slots necessary to initialize.
The fixture is temporary; it is not a data pack or a reproduction of
shipped artwork, sounds, models or levels. The frontend and intro media
are disabled through configuration so this test exercises direct level
startup rather than menu interaction.

The test checks that complete initialization installs GridA and the
configured catalog, counts the two expected resource objects, advances
three ticks, follows the real level transition to GridB, verifies the new
grid and its independent accounting and object count, advances another
tick, and calls the real engine shutdown before releasing DirectDraw.
This verifies the success path previously missing from the startup
probes. It is not a complete frontend boot or memory-leak accounting test.

#### Generic menu resource loading

`menu_resource_load` parses two original abstract menu definitions with
uniform placeholder textures and fonts. It verifies cache reuse, menu
position/anchor fields, cycle labels and callback bindings, integer and
real slider ranges, trigger ownership, optional overlay resources and
links between the two generated menu records. No shipped menu layout or
artwork is reproduced. It also releases the loaded menu items through
the real cleanup routine; the item array must remain live until those
items have been released. The fixture is process-isolated and does not
establish comprehensive cache or frontend teardown accounting.
