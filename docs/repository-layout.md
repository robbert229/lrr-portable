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
docs/adr/           Architecture decision records (build/port decisions)
reference/          (gitignored) the original, unmodified game files
unpacked/           (gitignored) 5,845 assets extracted from the .wad archives
cross/              MinGW-w64 Win32 toolchain inputs
CMakeLists.txt      Active CMake build (PLATFORM=native|winapi)
src/link_order.cmake
                    The link-order list (ADR 0006): every game, gods98,
                    platform and program source in original link order, with
                    its library; the matching build links them in this order
meson.build         Existing Meson build (kept during the CMake migration)
src/                Reconstruction source tree: game/ and gods98/ (original
                    units: one <stem>.c or <stem>.cpp per unit, plus legacy
                    mixed files and their private <file>_shared.hpp headers
                    from relocation), platform/ (backend entry points), crt/
                    (runtime stand-ins), main.cpp
subprojects/        miniwin CMake bridge (sparse isle-portable submodule)
3rdparty/           Dependency submodules (isle-portable, sparse → miniwin/)
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
