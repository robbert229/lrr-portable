# Repository layout and reference files

[Project overview](../README.md) · [Build and run](building.md)

```
README.md           This file
docs/findings/      Reverse-engineering findings (see findings/README.md)
tools/              WAD tooling (rrwad.py: unpack/pack/verify — see tools/README.md)
                    + model converter (tools/rrlwo.py: .lwo → glTF 2.0)
                    + compat harness (tools/compat/a2.py — see tools/compat/README.md)
docs/decompiles/    Canonical and legacy Ghidra pseudocode; see
                    docs/decompiles/README.md
docs/evidence/      Exported REA evidence bundle (address-level citations)
tools/              Analysis tooling: unwad.py, Ghidra function-naming script
docs/coverage/      Original translation units, function areas and call graph
                    of LegoRR.exe (frozen at the develop fork; their
                    generators are on the decomp branch)
docs/adr/           Architecture decision records (build/port decisions)
reference/          (gitignored) the original, unmodified game files
unpacked/           (gitignored) 5,845 assets extracted from the .wad archives
CMakeLists.txt      The CMake build (native, SDL3 + miniwin); its
                    project(VERSION) is the version (see releasing.md)
CHANGELOG.md        Release notes for players, one section per release
LICENSE             LGPL-3.0-or-later
THIRD-PARTY-NOTICES.txt
                    Licences of the libraries the program includes; shipped
                    in every package with LICENSE
packaging/          CPack packages, the Linux desktop entry and AppStream
                    metadata, the AppImage and Flatpak recipes, the macOS
                    bundle, the Nintendo 3DS .3dsx and .cia, the Android
                    Gradle project and the app icon (see building.md#packages
                    and building.md#android)
src/link_order.cmake
                    The source list: every game, gods98, platform and
                    program source, in the original's unit order, with its
                    library
src/                Reconstruction source tree: game/ and gods98/ (original
                    units: one <stem>.c or <stem>.cpp per unit, plus legacy
                    mixed files and their private <file>_shared.hpp headers
                    from relocation), platform/ (native entry points), crt/
                    (stand-ins for the original's static C runtime), main.cpp;
                    each directory with tests registers them in its
                    tests.cmake (building.md#writing-a-test)
cmake/LrrTesting.cmake
                    lrr_unit_test and lrr_unit_test_variant, which the
                    tests.cmake files call
3rdparty/           Dependency submodules (isle-portable, sparse → miniwin/)
patches/miniwin/    Our miniwin changes, applied on top of the submodule (ADR 0009)
```

Everything under `reference/` is the original game as shipped. It is **not** committed to git
(copyright), and is expected to be provided by each person working in this repo from a
legally obtained copy of the game.

### What's in `reference/`

| Path | Description |
| --- | --- |
| `LegoRR.exe` | The main game executable — primary reverse engineering target |
| `LegoRR0.wad`, `LegoRR1.wad` | Game data archives (assets and possibly logic/data) |
| `LegoRR.icd` | Installation/configuration data |
| `D3DRM.DLL` | DirectX 5 render-mesh library dependency |
| `CLGen.exe`, `CLGen.dat` | Install-time generator component |
| `Data/` | Loose runtime data — streamed sounds (`Data/Sounds/Streamed/`), video (`Data/AVI/`) |
| `Readme.txt` | Original publisher readme |
