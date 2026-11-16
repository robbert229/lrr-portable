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
| Accuracy: mean Reccmp score of mapped functions, game-only / full | 98.04% / 98.23% |
| Mapped functions, game-only / full | 2,723 / 3,019 |
| Exact (100%) functions, game-only / full | 2,418 / 2,707 |

## What works, what remains

The focus so far has been on roughly implementing every single function. As a
result the game is not stable. You can play through some of the missions, however
the game is likely to crash before you end up completing a mission. We are nearing 100% 

<img width="2242" height="1440" alt="image" src="https://github.com/user-attachments/assets/9dae6667-cb3c-4ea5-a882-a837f23f350a" />
<img width="2242" height="1440" alt="image" src="https://github.com/user-attachments/assets/714ff741-c2b2-4093-8257-8386c593f194" />
<img width="2037" height="1440" alt="image" src="https://github.com/user-attachments/assets/7c544e7b-fb49-4450-9ac1-0ef29ecafcb2" />


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
