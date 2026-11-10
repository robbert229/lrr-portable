# Native Linux (miniwin) is the primary target; Win32/MinGW is the reference build

The end goal is a **native Linux port running `lrr-portable` with no Wine
layer**. The sequencing answer to "Windows-first or port-first" was: *flip to
native-first*, because the port adapter is not something we must build from
scratch. We adopt **miniwin** from the isle-decomp/isle-portable project — a
source-compatible reimplementation of the DX6-era WinAPI surface (DirectDraw,
D3DRM skeleton, DInput, etc.) over SDL3, LGPL-3.0, maintained by a sibling
LEGO-game decompilation project — as the adapter that satisfies the finding-15
API surface on host platforms.

Decisions that hang off this:

- **One source tree, two adapters** — game code is written once against the
  DX6-era surface (finding 15's import table is the contract, Q-B). The
  `native` adapter compiles it against `miniwin`; the `winapi` adapter
  compiles it against MinGW-w64's real DirectX headers.
- **The Win32/MinGW build is retained as the *reference build*.** It is the
  only way to run our rebuild and the shipped binary in the *same* harness
  (the A.2 `--run-port` oracle, ADR 0002) and diff behavior directly. It is a
  validation citizen, not a shipping path.
- **Backend choice is a Meson `platform` option** (`native` default,
  `winapi`) fixed at setup; the arch (i686 cross file) applies only to the
  winapi adapter.
- **miniwin comes in as a sparse git submodule** (limited to `miniwin/`,
  only root metadata checked out) — the sibling game's decompiled source
  never lands in our working tree. Wired via `cmake.subproject('miniwin')`
  (Q-A, Q-F).
- **C++17 for the reconstruction, with discipline** (Q-E; amended by
  [ADR 0005](0005-original-shape-c-units.md): code rebuilt as original
  translation units is a C-compatible subset, compiled as C in the matching
  build and as C++ here): plain structs,
  no heavy STL — C++ only where miniwin's interface demands it. The corpus's
  C-shaped code ports with negligible friction.

**Facts driving the flip** (verified 2026-10-20):
- miniwin is active and CI-green across ~12 platforms; its DirectDraw→SDL3 is
  complete; D3DRM is a real (skeleton) reimplementation — the D3DRM surface our
  3D path needs (findings 11/13) is extension work with a head start, not a
  blank page.
- miniwin headers are C++-only (`virtual`, `class`, `bool`) — hence Q-E.
- miniwin tops out at `IDirectDraw` / `IDirectDraw2` / `IDirectDrawSurface3`;
  the shipped binary climbs to `IDirectDraw4` under Wine (finding 16). The
  game's exact QI target is still open (finding 15 §5) — a potential miniwin
  QI-bridge gap for contributors upstream.
- AVIFIL32 (movies) has no miniwin shim; `DontPlayAvis` remains the escape
  hatch (finding 15) until a native AVI path exists.

**Consequences:** the A.2 harness / Wine findings (16–18) reframe as
"how we validate," not "how we ship." README wording updated accordingly.
Growth gaps in miniwin (D3DRM surfaces, DDraw4 bridge, AVI) are upstream
contributions, not local forks.