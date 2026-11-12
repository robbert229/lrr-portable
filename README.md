# lrr-portable

A decompilation and portable reconstruction of **Lego Rock Raiders** (1999).
The goal is a native Linux port using SDL3/miniwin, with a Windows reference
build for comparison against the original. I would currently classify it as a
vibe-coded slop fest, however things are developing rapidly. I hope to have it
in a playable state before the end of November 2026.

## Decompilation status

<!-- Figures come from the committed reports in docs/coverage/. Refresh them
     with `python3 tools/readme_status.py` (no build needed); recount the
     source ledger with `python3 tools/coverage_ledger.py`. -->

Verified **2026-11-12** on `main` at `3791f0dd`, with MSVC 5.0 and Reccmp:

| Measure | Value |
| --- | ---: |
| **Exact-function code bytes**: original code in functions Reccmp scores at 100% | **105,485 / 642,549 bytes (16.42%)** |
| Accuracy: mean Reccmp score of mapped functions, game-only / full | 68.70% / 71.59% |
| Mapped functions, game-only / full | 2,260 / 2,496 |
| Exact (100%) functions, game-only / full | 974 / 1,174 |
| Functions byte-identical at their original address | 8 (384 bytes) |

The old headline, Reccmp's game-only "progress" (47.77% on 2026-11-04), is no
longer tracked; exact-function code bytes replaced it, and accuracy is the same
Reccmp measure as before (then 48.27%).
[Report details and reproduction](docs/coverage/reccmp.md).

**How it's measured.** Conversions now rebuild the original translation units
in their original shape ([ADR 0005](docs/adr/0005-original-shape-c-units.md))
and link order ([ADR 0006](docs/adr/0006-original-link-order.md)). The matching build
compiles them with MSVC 5.0 and scores each function with Reccmp. The
whole-binary gap is still large: `.text` is 1,105,428 bytes against the
original's 642,549, `.rdata` 24,376 against 11,232 and `.data` 1,004,544
against 73,216. A second link in the original's mode (no `/DEBUG`, so
`/OPT:REF`) drops 414,654 bytes of unreferenced code and brings `.text` to
690,682. These figures are in
[`binary-progress.json`](docs/coverage/binary-progress.json); see findings
[563](docs/findings/563-whole-binary-audit.md) and
[576](docs/findings/576-link-mode-and-unreferenced-code.md).

The [source ledger](docs/coverage/README.md) records **2,282 / 2,564 functions
(89.0%) with source citations**, plus 280 runtime/import functions and 2 named
functions not yet cited. Source coverage and binary similarity measure
different things; neither guarantees correct gameplay.

## What works, what remains

The focus so far has been on roughly implementing every single function. As a
result the game is currently unplayable. Most systems are roughly implemented,
but not in a functional state.

The menus partially work, however defects exist in scrolling, and animations.

<img width="2242" height="1440" alt="image" src="https://github.com/user-attachments/assets/9dae6667-cb3c-4ea5-a882-a837f23f350a" />
<img width="2242" height="1440" alt="image" src="https://github.com/user-attachments/assets/714ff741-c2b2-4093-8257-8386c593f194" />

GamePlay has a lot of work that needs to be done. 

<img width="2242" height="1440" alt="image" src="https://github.com/user-attachments/assets/9eded253-8540-4b51-ae4b-cb2f1440bccb" />

## Build and run

Requires a legally owned game copy, CMake, a C++ compiler, and SDL3 development
headers. Game assets are not included. Place the game assets into the 
`reference` folder.

```sh
$ ls reference/
CLGen.dat  D3DRM.DLL  LegoRR0.wad  LegoRR.exe  legoRR-stub.log  Saves
CLGen.exe  Data       LegoRR1.wad  LegoRR.icd  Readme.txt
```

You can then generate the report, and run:

```sh
make reccmp-report
make reccmp-run
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
