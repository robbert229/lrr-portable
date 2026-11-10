# Agent workflow rules

- Keep all C/C++ source compatible with Microsoft Visual C++ 5.0 (MSVC 5.0).
  Avoid language and CRT features it does not support; validate source changes
  with `make reccmp-report` when the configured toolchain is available.
- Never merge, squash-merge, or auto-merge a PR — open the PR, report the link,
  and leave merging to the human. This applies to every branch and every repo
  state, including after all checks are green.
- Before handing work back, run `make reccmp-check` (matching build; see the
  *match baseline* in `GLOSSARY.md`) whenever the configured toolchain is
  available, including for apparently unrelated C/C++ or `#include` changes.
  Do not let a function's score drop below the baseline. If a change
  intentionally improves matches, refresh the baseline in the same change with
  `make reccmp-baseline-update`. Use `make reccmp-compare` to re-check an
  existing report without rebuilding.

# Project guide

- This is a C++17 reconstruction of Lego Rock Raiders. Engine code is in
  `src/gods98/`, game logic in `src/game/`, and backend entry points in
  `src/platform/`. Reverse-engineering findings and architecture decisions are
  in `docs/findings/` and `docs/adr/`; tooling is in `tools/`.
- CMake is the active build (the root also retains a Meson build). Native is
  the default backend and requires SDL3 plus the sparse `3rdparty/isle-portable`
  miniwin submodule. `make setup`, `make build`, and `make run` configure,
  build, and launch; `WAD_DIR` selects the game data directory. For tests, run
  `cmake -S . -B build` then `cmake --build build` and `ctest --test-dir build
  --output-on-failure`. Set `TEST_WADS_DIR` at configure time to enable
  real-data tests when the shipped WADs are available.
- `PLATFORM=winapi` selects the 32-bit Windows reference backend; see
  `cross/i686.ini` and the build documentation before attempting a cross-build.
  `MOVIES=auto|enabled|disabled` controls native FFmpeg runtime detection.
- No separate repository-wide formatter, linter, pre-commit configuration, or
  CI workflow is present. The Makefile exposes `make reccmp-lint` for the
  Reccmp annotation ledger. Follow the surrounding C++ style and retain the
  SPDX headers used by source files.
