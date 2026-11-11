# Build, run, and compare

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

The default backend is `native`. miniwin supplies the Windows-compatible
interfaces over SDL3; this backend does not require Wine. `BUILD_DIR` selects
another build directory. `MOVIES=auto|enabled|disabled` controls detection of
FFmpeg runtime libraries; the default is `auto`. Movie playback requires the
runtime libraries, rather than FFmpeg headers at build time.

For direct CMake use, initialize miniwin first:

```sh
tools/setup-miniwin.sh
cmake -S . -B build -DPLATFORM=native -DMOVIES=auto
cmake --build build
LEGORRR_WAD_DIR=/path/to/game ./build/LegoRR
```

## Tests

```sh
cmake -S . -B build -DTEST_WADS_DIR=/path/to/game
cmake --build build
ctest --test-dir build --output-on-failure
```

Omit `TEST_WADS_DIR` for synthetic-fixture tests only. Real-data probes need
original game files and are skipped when those files are not configured.
Passing tests establishes the behavior covered by those tests; gameplay
comparison against the original is a separate check.

## Continuous integration

The `native-build` workflow (`.github/workflows/native-build.yml`) runs on pull
requests and pushes to `main`. Its check is required for merging. It checks out
the repository without submodules, runs `tools/setup-miniwin.sh` to fetch only
`miniwin/`, and then runs the native commands from [Tests](#tests) with
`TEST_WADS_DIR` unset, so only the synthetic-fixture tests run.

SDL3 comes from a pinned source build of release-3.4.18 (commit `829a65d7`),
configured headless with `SDL_UNIX_CONSOLE_BUILD` and without X11 or Wayland.
The tests use SDL's dummy and offscreen drivers. The build is cached by SDL
version. `ubuntu-24.04` has no SDL3 development package, so a distro package is
not used. When you change the SDL version or its build recipe, update
`SDL_VERSION`, `SDL_COMMIT`, or `SDL_CACHE_VERSION` in the workflow.

The matching build, the Reccmp gate (`make reccmp-check`), and real-data tests
need licensed inputs and the MSVC 5.0 toolchain. They stay local and are not run
in CI.

## Windows reference backend

The reference backend builds a 32-bit Windows executable for comparison with
the shipped game. Install the i686 MinGW-w64 compiler, resource compiler, and
archive tools before configuring:

```sh
make setup BUILD_DIR=build-winapi PLATFORM=winapi \
  CMAKE_TOOLCHAIN_FILE=cross/i686-toolchain.cmake
cmake --build build-winapi
```

See [ADR 0001](adr/0001-minwg-cross-toolchain.md),
[ADR 0002](adr/0002-run-port-integrity-bypass.md),
[ADR 0004](adr/0004-miniwin-native-port.md), and
[ADR 0005](adr/0005-original-shape-c-units.md) for the toolchain and comparison
rationale. Units rebuilt in original shape (`LRR_ORIGINAL_SHAPE_SOURCES`) are
compiled as C by the matching build and as C++ by every other build. The [compatibility harness](../tools/compat/README.md) documents
running the original executable under Proton.

## MSVC 5.0 and Reccmp

Keep source changes compatible with MSVC 5.0. With the configured classic
toolchain, Wine, and Reccmp available, build the candidate and generate a report:

```sh
make reccmp-report RECCMP_WINE_BIN=/path/to/wine \
  RECCMP_RECCMP=/path/to/reccmp-reccmp
```

`make reccmp` is an alias. The toolchain defaults to
`~/.local/share/reprobit/toolchains/msvc_5_0_rtm`; override it with
`REPROBIT_MSVC_5_0_RTM_ROOT=/path/to/toolchain`. Reports go to
`build-reccmp/reccmp-report/index.html` and `results.json`.

There is one matching build. It links with `/INCREMENTAL:NO` (ADR 0006
decision 2), so the executable has no incremental-link thunks and, like the
original, no `.reloc` section. `tools/reccmp.sh` then removes the PE debug
directory, which the original also lacks; the linker PDB stays next to the
executable for Reccmp. The link also writes a map to `build-reccmp/LegoRR.map`
(gitignored with the rest of `build-reccmp/`): the section layout, then every
public symbol with its address and contributing `library:object`, sorted by
address, which shows the link order. The gate, `make binary-progress` and
`make match-progress` all read this one build.

### Match baseline gate

`docs/coverage/match-baseline.txt` is the committed *match baseline*: every
function's Reccmp similarity score (percent, two decimals, sorted by address)
for the game-only and full scopes. Parallel changes can silently erase each
other's matches, so check before handing work back:

```sh
make reccmp-check      # rebuild, then compare against the baseline
make reccmp-compare    # compare an existing build-reccmp/reccmp-report only
```

Both exit non-zero and list every function that fell below baseline as
`old -> new`. Improvements, newly mapped and newly unmapped functions are
reported but do not fail. To accept a deliberate change (an improvement, or an
intended drop), rebuild with `make reccmp-report`, run
`make reccmp-baseline-update`, and commit the baseline in the same change.
`RECCMP_BASELINE=<file>` and `RECCMP_BUILD_DIR=<dir>` override the paths. The
logic lives in `tools/match_baseline.py`.

```sh
make reccmp-lint
make reference-run
make reccmp-run
```

The run targets use the same Proton runner, compatibility prefix, and reference
working directory. `reccmp-run` requires an already-built candidate. Set
`RECCMP_PROTON_BIN` and `RECCMP_COMPATDATA` to override the runner and prefix;
`RECCMP_REFERENCE_DIR` selects the original game directory.

The [coverage ledger](coverage/README.md) measures source citations. Reccmp
compares compiled functions with the original executable; its percentages
measure something different. Record the build configuration and report scope
when publishing a score, as in the
[optimization-flag sweep](findings/486-msvc5-optimization-flag-reccmp-sweep.md).

### Single-function loop

`make match-function ADDR=0x00478e60` compiles only the translation unit that
holds the function, with the same flags, and scores it with Reccmp's own
comparator against the original in about a second, without linking. It
prints the similarity, the baseline delta and a diff (`JSON=1` for JSON).
See [`tools/README.md`](../tools/README.md#match_functionpy--single-function-match-loop)
for options, timings and how its scores relate to a full report.

## Legacy build notes

The root Meson build remains in the repository. The
[implementation notes](implementation.md) preserve its subsystem-specific
commands and historical test results. CMake through `make` is the current
entry point.

### Whole-binary progress

`make binary-progress` rebuilds the matching build (`make reccmp-report`) and
compares its executable with the original, refreshing
`docs/coverage/binary-progress.json`; `make binary-progress-compare` compares
an existing build without rebuilding. Install Python `pefile` and `capstone`
for the report; `BINARY_PROGRESS_PYTHON` selects their interpreter. See [tool
documentation](../tools/README.md#whole-binary-progress) for definitions and
input hashing. Run `make reccmp-check` for the match baseline gate.

### PR matching progress

For any change to matching-build inputs, run `make reccmp-check`, investigate
any drops under the ADR 0005 entropy policy, then run `make match-progress`
and commit `docs/coverage/match-baseline.txt`,
`docs/coverage/match-listing-digests.txt` and
`docs/coverage/binary-progress.json`. This reruns `make reccmp-report`,
refreshes the baseline and listing digests from it and compares the same
executable for the whole-binary report. There is no second build: after `make reccmp-check`, CMake has nothing
to recompile or relink, so only Reccmp runs again. The hash alone cannot
establish that the baseline is current. CI checks the progress and baseline
fingerprints and baseline digest in
`matching-progress-freshness` without the toolchain;
it reports progress in a single comment for same-repository PRs. Fork PRs
get the freshness check but no comment. This check verifies freshness, not
measurement correctness. On a generated-file conflict, take either side and
regenerate all three with `make match-progress`; never hand-merge them.
