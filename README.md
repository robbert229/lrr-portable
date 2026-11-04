# lrr-portable

A portable reconstruction of **Lego Rock Raiders**, the 1999 Windows strategy
game about mining underground caverns with LEGO crews, vehicles, and buildings.

The goal is to recover the game's behavior from its original executable and
build a **native Linux version without Wine**, using an SDL3-based compatibility
layer called miniwin. A Windows reference build lets us compare the same
reconstructed source against the original game.

## How far along is it?

**The reconstruction is extensive, but work remains on faithful behavior and
end-to-end gameplay.** Recovering a function, matching its compiled code, and
showing that it behaves correctly in a mission are separate milestones.

As of **2026-11-04**, the checked-in [coverage ledger](docs/coverage/README.md)
tracks **2,564 functions** from the original executable:

- **2,285 (89.1%) have source entry-point citations**, classified as “ported.”
- **271 are runtime library code or import support**, tracked separately from
  game logic.
- **8 remain in the ledger's other categories**, and no entries are classified
  as stubs.

These are function counts, not a “percent of the game finished.” A citation
connects reconstructed source to an original function; it does not establish
that every branch is implemented or that the result matches the original.
The ledger's [definitions and caveats](docs/coverage/README.md#caveats) explain
the measurement and how to regenerate it.

There is already more to explore than a collection of function names:

- **Game data and asset tooling:** documented archive and asset formats, WAD
  unpacking/repacking, and LightWave model conversion. The loader tests include
  checks against the original models, textures, sounds, and animations when
  game data is available.
- **A working reconstruction of major game systems:** menus and level loading,
  terrain, entities, selection and commands, objectives, savegames, audio, and
  movies all have reconstructed implementations and tests. The documented native tutorial
  path includes a textured cave, Raiders, a mission brief, and selection/movement.
  See the [implementation notes](docs/implementation.md) for the evidence and
  limitations of individual systems.
- **Recent gameplay work:** [PR #374](https://github.com/robbert229/lrr-portable/pull/374)
  added building scenes and GetTool routes to the Wine gameplay runtime.
  This is progress toward connecting recovered systems in the running game;
  full campaign parity still needs validation.

Binary matching is also underway using **Microsoft Visual C++ 5.0**, the original
compiler generation, and Reccmp. A recorded
[compiler-flag experiment](docs/findings/486-msvc5-optimization-flag-reccmp-sweep.md)
reported **32.21% game-only Reccmp progress** for its baseline. That is a
historical measurement of compiled-code matching, not today's score or a
playability rating; it uses a different function set from the source ledger.

The remaining work includes improving binary matches, replacing or validating
portable platform seams, and checking menus, rendering, saves, and mission
behavior against the original with real game data. There is no claim here that
the native reconstruction can yet play the entire campaign faithfully.

The **original executable** has separately been played through its first mission
under Proton ([session notes](docs/findings/17-interactive-a2-session.md)).
That provides a useful reference for comparison with the reconstruction.

## Try it or contribute

You need a legally owned copy of the original game; assets are not included.
For a native build, install CMake, a C++ compiler, and SDL3 development headers,
then run from the repository root:

```sh
make build
make run WAD_DIR=/path/to/game
```

The [build and run guide](docs/building.md) covers dependencies, tests, movies,
the Windows reference backend, and Reccmp comparisons.

Useful places to explore:

- [Implementation notes](docs/implementation.md): subsystem behavior, test
  evidence, and known gaps.
- [Reverse-engineering findings](docs/findings/README.md) and
  [decompiled pseudocode](docs/decompiles/README.md): how the original works.
- [Function coverage](docs/coverage/README.md),
  [subsystem map](docs/coverage/function-areas.md), and
  [global-variable research](docs/dat/README.md): what has been identified.
- [Tools](tools/README.md): archives and asset conversion;
  [compatibility harness](tools/compat/README.md): running the original.
- [Repository layout](docs/repository-layout.md),
  [architecture decisions](docs/adr/0004-miniwin-native-port.md), and
  [early research history](docs/research-history.md).

Please keep original game files in the ignored `reference/` directory and
commit research notes and tooling rather than game content. Source changes
must remain compatible with MSVC 5.0; see [AGENTS.md](AGENTS.md).

## Legal note

This project is not affiliated with or endorsed by the LEGO Group.

Lego Rock Raiders is © 1999 LEGO Media International. All original game assets are the
property of their respective owners and remain copyright LEGO. This repository exists
solely for interoperability research, documentation, and preservation purposes and
contains no original game content. Do not upload copies of the game anywhere.

The reconstruction source, build system, tooling and documentation in this
repository are licensed **LGPL-3.0-or-later** (see `LICENSE`); the
miniwin adapter is similarly LGPL-3.0. That license covers *our code*, not the
game's assets or binary.
