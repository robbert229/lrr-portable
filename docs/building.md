# Build and run

[Project overview](../README.md) · [Implementation notes](implementation.md)

Run commands from the repository root. Supply a legally owned game directory
containing `LegoRR0.wad`, `LegoRR1.wad`, and the loose `Data/` files. The
[repository layout](repository-layout.md) describes the reference files.

## Native Linux build

Install CMake 3.20 or later, a C++17 compiler, SDL3 development headers, and Git.
`make setup` bootstraps the sparse `3rdparty/isle-portable` miniwin submodule
and configures CMake; `make build` also runs setup.

```sh
make build
make run WAD_DIR=/path/to/game
```

miniwin supplies the Windows-compatible interfaces over SDL3 on every host,
Windows included (ADR 0007); the build does not require Wine. `BUILD_DIR` selects
another build directory. `MOVIES=auto|enabled|disabled` controls detection of
FFmpeg runtime libraries; the default is `auto`. Movie playback requires the
runtime libraries, rather than FFmpeg headers at build time.

For direct CMake use, initialize miniwin first:

```sh
tools/setup-miniwin.sh
cmake -S . -B build -DMOVIES=auto
cmake --build build
LEGORRR_WAD_DIR=/path/to/game ./build/LegoRR
```

## Tests

```sh
cmake -S . -B build -DTEST_WADS_DIR=/path/to/game
cmake --build build
ctest --test-dir build --output-on-failure
```

`make test` runs the same build and ctest in `BUILD_DIR`. Omit
`TEST_WADS_DIR` for synthetic-fixture tests only. Real-data probes need
original game files and are skipped when those files are not configured.
Passing tests establishes the behavior covered by those tests; gameplay
comparison against the original is a separate check.

## Continuous integration

The `native-build` workflow (`.github/workflows/native-build.yml`) runs on pull
requests and pushes to `main` and `develop`. Its check is required for merging. It checks out
the repository without submodules, runs `tools/setup-miniwin.sh` to fetch only
`miniwin/`, and then runs the native commands from [Tests](#tests) with
`TEST_WADS_DIR` unset, so only the synthetic-fixture tests run. It also runs
the Python tool tests in `tools/tests`.

SDL3 comes from a pinned source build of release-3.4.18 (commit `829a65d7`),
configured headless with `SDL_UNIX_CONSOLE_BUILD` and without X11 or Wayland.
The tests use SDL's dummy and offscreen drivers. The build is cached by SDL
version. `ubuntu-24.04` has no SDL3 development package, so a distro package is
not used. When you change the SDL version or its build recipe, update
`SDL_VERSION`, `SDL_COMMIT`, or `SDL_CACHE_VERSION` in the workflow.

Real-data tests need licensed game files, so they stay local and are not run
in CI.

## Sources

`src/link_order.cmake` is the source list: one
`lrr_link(<kind> <key> <library> <source>)` line per source, with the library
(`game`, `gods98`, `platform_glue` or `program`) it is built into. Its header
explains the fields. It is the only place that adds a game, gods98, platform
or program source to the build. The units rebuilt in original shape are its
`.c` files and its `.cpp` entries of kind `unit`; they compile as C++17.

## Running the original

`make reference-run` runs the shipped `LegoRR.exe` from `REFERENCE_DIR`
(default `reference/`) under Proton, with the
[compatibility harness](../tools/compat/README.md)'s runner and prefix. Set
`PROTON_BIN` (or `RR_A2_PROTON`) and `COMPATDATA` to override them.

## The matching build

develop has no matching build, Reccmp gate or Windows reference backend
(ADR 0007). They remain on `main` until the decomp tag; see `main`'s
`docs/building.md`.
