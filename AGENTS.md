# Agent workflow rules

This is the `develop` branch. It follows
[ADR 0007](docs/adr/0007-port-on-develop-units-on-miniwin.md): the port runs
the original-shape units on miniwin, and becomes `main` at the *decomp tag*.
Open PRs for this work against `develop`.

- **The units are the game.** All game and engine logic lives in the units
  (`src/link_order.cmake`, kind `unit`). Port-only code lives in exactly two
  places: in miniwin, below the seam, and in library substitutions inside a
  unit, each call site marked `// [library:audio]`, `// [library:time]`,
  `// [library:video]`, `// [library:config]`, `// [library:filesystem]` or
  `// [library:input]`.
  A substitution replaces only the call it marks.
- **Units keep the original's shape:** their files, function order, names,
  signatures and globals. Within that shape, change code freely for
  correctness, portability and safety.
- Write C++17 that builds and runs 64-bit. Records read from or written to
  disk keep fixed-width fields and get explicit parse and serialize
  functions; every other record holds real pointers.
- A decomp change on `main` reaches `develop` only when it fixes behavior,
  reapplied by hand.
- Write every commit message and PR title in
  [Conventional Commits 1.0.0](https://www.conventionalcommits.org/en/v1.0.0/)
  form: `type(scope): summary`, for example
  `fix(sound3d): stop looping samples on level exit`. Use the types `feat`,
  `fix`, `docs`, `refactor`, `perf`, `test`, `build`, `ci` and `chore`. The
  scope is optional and names the unit, subsystem or tool. Mark a breaking
  change with `!` after the type or scope, or with a `BREAKING CHANGE:`
  footer. A squash merge uses the PR title as the commit subject, so the PR
  title must follow the form too.
- Never merge, squash-merge, or auto-merge a PR — open the PR, report the link,
  and leave merging to the human. This applies to every branch and every repo
  state, including after all checks are green.

# Project guide

- A C++17 reconstruction of Lego Rock Raiders. Engine code is in
  `src/gods98/`, game logic in `src/game/`, and native entry points in
  `src/platform/native/`. Reverse-engineering findings and architecture
  decisions are in `docs/findings/` and `docs/adr/`; tooling is in `tools/`.
- CMake is the build. It needs SDL3 and the sparse
  `3rdparty/isle-portable` miniwin submodule, which is upstream isle-portable
  with `patches/miniwin` applied by `tools/setup-miniwin.sh`. To change
  miniwin, commit in the submodule and run `tools/miniwin-patches.sh export`
  ([ADR 0009](docs/adr/0009-miniwin-patches-in-this-repository.md)). `make setup`, `make build`, and
  `make run` configure, build, and launch; `make run` starts `LegoRR` in
  `WAD_DIR` (the game data directory) with `RUN_ARGS` (default
  `-best -window`). For tests, run `cmake -S . -B build`, then
  `cmake --build build` and `ctest --test-dir build --output-on-failure`
  (`make test` does the same).
- CI runs one required check, `native-build`
  (`.github/workflows/native-build.yml`), on pull requests and on pushes to
  `main` and `develop`. It builds the native port, runs ctest with synthetic
  fixtures only, and runs the Python tool tests in `tools/tests`. Real-data
  tests need licensed inputs, so run them locally.
- Source files carry SPDX headers; keep them, and follow the surrounding C++
  style. There is no repository-wide formatter or linter.
- The `// FUNCTION:`, `// GLOBAL:` and `// LIBRARY:` annotations and
  `// PORT-ONLY:` tags in the source come from the matching build on `main`.
  Keep the annotations as a record of original addresses; nothing on
  `develop` checks them.
