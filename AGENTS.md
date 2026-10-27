# Agent workflow rules

- Never merge, squash-merge, or auto-merge a PR — open the PR, report the link,
  and leave merging to the human. This applies to every branch and every repo
  state, including after all checks are green.

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
