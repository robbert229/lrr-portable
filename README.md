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

Figures from the reports committed on `main` at `77f65f61` (#848,
**2026-11-15**), built with MSVC 5.0 and compared with Reccmp:

| Measure | Value |
| --- | ---: |
| **Exact-function code bytes**: original code in functions Reccmp scores at 100% | **365,396 / 642,549 bytes (56.87%)** |
| Accuracy: mean Reccmp score of mapped functions, game-only / full | 98.04% / 98.23% |
| Mapped functions, game-only / full | 2,723 / 3,019 |
| Exact (100%) functions, game-only / full | 2,418 / 2,707 |
| Functions byte-identical at their original address | 114 (8,094 bytes) |
| `.text` bytes identical at their original address | 9.42% (chance 2.83%) |

The old headline, Reccmp's game-only "progress" (47.77% on 2026-11-04), is no
longer tracked; exact-function code bytes replaced it, and accuracy is the same
Reccmp measure as before (then 48.27%).
[Report details and reproduction](docs/coverage/reccmp.md).

The goal is now a byte-identical image: the original-mode matching link should
equal the original `LegoRR.exe` apart from the PE header's `TimeDateStamp` and
`CheckSum`, as checked by `python3 tools/image_identity.py`. #848 started that
work. Every common now sits at its original address (47 of 47), `.bss` starts
at its original address with 118 of 164 items in place, and 2,401 of 2,458
`.data` items sit at their original offsets in a `.data` section of the
original's size
([finding 687](docs/findings/687-original-mode-measures-advisor-constants-bss-gaps.md),
[finding 690](docs/findings/690-data-commons-order-and-frontend-arrays.md),
[finding 691](docs/findings/691-lego-bss-statics-then-externals.md)).
The first differing byte is still near the start of `.text`: 365 bytes in, or
7,187 bytes in when address operands are masked.

`LegoObject.c`, `Lego.c`, `Main.c` and `Input.c` are converted to their
original shape. The matching build links in the original's mode, `/DEBUG
/OPT:REF /OPT:NOICF`, and its `/OPT:REF` drops no annotated function
([finding 642](docs/findings/642-gate-takes-the-original-link-mode.md)). Its
C runtime comes from `LIBC.LIB` alone, as the original's did
([finding 643](docs/findings/643-crt-from-libc-and-late-members.md)). The image
stays below the original's flag constant `0x800000`, so scores come from plain
Reccmp with no plain-immediate wrapper; `make pe-audit` and the gate fail if
the image grows past it again
([finding 647](docs/findings/647-retire-plain-immediate-wrapper.md)).
The link also follows the original's object and library layout, and the port
tail is now empty. Function-order inversions stand at 222 pairs (1 in gods98
code, none in game code), 224 of 226 units are contiguous, and the entry point
is `0x8ecf0` against the original's `0x8f000`
([finding 657](docs/findings/657-function-order-and-unit-contiguity-breakdown.md),
[finding 658](docs/findings/658-libc-member-order-gods98-members-and-tail-library.md),
[finding 692](docs/findings/692-entry-point-sqrt-member-unit-size-sweep.md)).
The resources and PE header stamps match the original's
([finding 651](docs/findings/651-original-resources-and-header-stamps.md)).

The [source ledger](docs/coverage/README.md) records **2,288 / 2,564 functions
(89.2%) with source citations**, plus 271 runtime/import functions, and 4 named
and 1 anonymous function not yet cited. Source coverage and binary similarity measure
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
