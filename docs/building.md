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
On the SD card, where every lookup is a slow service call, the
path mapping (`files_native.cpp`) reads each directory once and matches
case from that listing.

The icon, banner and banner sound are original (`packaging/3ds/regenerate.py`
renders them). An emulator such as [Azahar](https://azahar-emu.org/) runs
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

### Writing a test

A test is one source file registered in the `if(BUILD_TESTING)` block of
`CMakeLists.txt` with `lrr_unit_test` (`cmake/LrrTesting.cmake`, whose header
lists every option):

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
that skips), `INCLUDES` and `DEFINES` cover the rest.

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
configured headless with `SDL_UNIX_CONSOLE_BUILD` and without X11 or Wayland.
The tests use SDL's dummy and offscreen drivers. The build is cached by SDL
version. `ubuntu-24.04` has no SDL3 development package, so a distro package is
not used. When you change the SDL version or its build recipe, update
`SDL_VERSION`, `SDL_COMMIT`, or `SDL_CACHE_VERSION` in the workflow, and
keep `LRR_SDL3_GIT_TAG` in `CMakeLists.txt` on the same commit. An existing
build directory keeps its cached `LRR_SDL3_GIT_TAG`; delete that entry, or
the directory, to pick up a new one.

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

The icon is original artwork drawn for this repository
(`packaging/icons/lrr-portable.svg`, LGPL-3.0-or-later); it shares nothing
with the game's own art, and the repository ships no game data in any package.
`packaging/icons/regenerate.py` renders the SVG into the macOS bundle's
`AppIcon.icns`, which is committed.

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
