# Build and run

[Project overview](../README.md) · [Implementation notes](implementation.md)

Run commands from the repository root. Supply a legally owned game directory
containing `LegoRR0.wad`, `LegoRR1.wad`, and the loose `Data/` files. The
[repository layout](repository-layout.md) describes the reference files.

## Native Linux build

Install CMake 3.20 or later, a C++17 compiler, SDL3 development headers, and Git.
`make setup` bootstraps the sparse `3rdparty/isle-portable` miniwin submodule,
applies the project's patches from `patches/miniwin` to it, and configures
CMake; `make build` also runs setup. The configuration file reader,
[iniparser](https://gitlab.com/iniparser/iniparser), comes from an installed
CMake package when there is one; otherwise configure fetches release v4.2.6,
the version isle-portable uses.

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
always reports no movies; it is off already on Emscripten, iOS, the Nintendo
targets, Vita and DOS.

miniwin emulates a 16-bit RGB565 display and describes its software renderer
as a hardware device, which is what `-best` looks for (see the
`MINIWIN_PIXELFORMAT` and `MINIWIN_SOFTWARE_HAL` definitions in
`CMakeLists.txt`). For a headless run, use SDL's offscreen driver and have
miniwin write each presented frame to a file:

```sh
cd /path/to/game
SDL_VIDEODRIVER=offscreen MINIWIN_DUMP_FRAME=/tmp/frame.png /path/to/build/LegoRR -best -window
```

Run the binary from a copy of the game directory, or from one whose WADs are
symbolic links, so that nothing the program writes lands in your reference
data. The front end ignores close requests, so stop a headless run with
`timeout`.

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

## Continuous integration

The `native-build` workflow (`.github/workflows/native-build.yml`) runs on pull
requests and pushes to `main` and `develop`. Its check is required for merging. It checks out
the repository without submodules, runs `tools/setup-miniwin.sh` to fetch only
`miniwin/` and apply `patches/miniwin`, and then runs the native commands from [Tests](#tests). It also
lints the units (no `LEGORRR_BACKEND_*`, `MINIWIN`, `_MSC_VER`, `_M_IX86` or
`_WIN32` guards and no `lpVtbl` in `src/`) and runs the Python tool tests in `tools/tests`.

SDL3 comes from a pinned source build of release-3.4.18 (commit `829a65d7`),
configured headless with `SDL_UNIX_CONSOLE_BUILD` and without X11 or Wayland.
The tests use SDL's dummy and offscreen drivers. The build is cached by SDL
version. `ubuntu-24.04` has no SDL3 development package, so a distro package is
not used. When you change the SDL version or its build recipe, update
`SDL_VERSION`, `SDL_COMMIT`, or `SDL_CACHE_VERSION` in the workflow.


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

develop has no matching build, Reccmp gate or Windows reference backend
(ADR 0007). They remain on `main` until the decomp tag; see `main`'s
`docs/building.md`.
