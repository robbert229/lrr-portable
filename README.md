# lrr-portable

A decompilation and portable reconstruction of **Lego Rock Raiders** (1999).
The goal is a native Linux port using SDL3/miniwin, with a Windows reference
build for comparison against the original.

## Decompilation status

Verified **2026-11-04** with MSVC 5.0 and Reccmp:

**47.77% game-only binary-matching progress.** Reccmp maps **2,116 / 2,138
functions (98.97%)**, with **48.27% accuracy** among mapped functions.
[Report details and reproduction](docs/coverage/reccmp.md).

The [source ledger](docs/coverage/README.md) records **2,288 / 2,564 functions
(89.2%) with source citations**, plus 271 runtime/import functions and 5 other
entries. Source coverage and binary similarity measure different things;
neither guarantees correct gameplay.

## What works, what remains

Reconstructed systems include menus, level loading, terrain, entities,
commands, objectives, saves, audio, and movies. Asset tools unpack WAD archives
and convert LightWave models. See [implementation notes](docs/implementation.md)
for subsystem evidence and limitations.

Work continues on binary matching, platform integration, and verifying ordinary
mission behavior against the original. Full campaign parity remains unverified.

## Build and run

Requires a legally owned game copy, CMake, a C++ compiler, and SDL3 development
headers. Game assets are not included.

```sh
make build
make run WAD_DIR=/path/to/game
```

[Build guide](docs/building.md) · [Findings](docs/findings/README.md) ·
[Pseudocode](docs/decompiles/README.md) · [Tools](tools/README.md) ·
[Compatibility harness](tools/compat/README.md) ·
[Repository layout](docs/repository-layout.md) · [Contributor rules](AGENTS.md)

## License

Code, tooling, and documentation: [LGPL-3.0-or-later](LICENSE). Original game
assets and binaries belong to their respective owners and are not included.
Keep your game files in the ignored `reference/` directory. This project is
not affiliated with or endorsed by the LEGO Group.
