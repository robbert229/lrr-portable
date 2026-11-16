# The port runs the original-shape units on miniwin, 64-bit, on a long-lived `develop` branch

**Status:** accepted (2026-11-15). **Supersedes** [ADR 0004](0004-miniwin-native-port.md)
on how the native port is structured and where it lives. **Amends**
[ADR 0005](0005-original-shape-c-units.md) and [ADR 0006](0006-original-link-order.md)
on the *develop branch* only; both still govern `main` until the *decomp tag*.

## Context

The native build and the Win32 builds run different programs. The *matching
build* runs the original `WinMain` and the *original translation units*. The
native build runs `LegorrrRun`, a port-only rewrite spread across about 136k
lines of *port tail* (`frameloop.cpp`, `lego_port.cpp`, `legoobject_port.cpp`,
`scene_render.cpp`, the `*_port.cpp` files and about 230 hook setters, most
of which no production code ever sets). The native build "compiles but never
calls" the units (`lego_unit.h`). Every gameplay defect therefore has to be
fixed twice. The more complete copy of the game is the one that matches,
scoring 98.2% mean Reccmp accuracy at #848, and it runs under Wine with
Microsoft's `D3DRM.DLL`.

The owner has decided that the port becomes the project's future `main`. The
decompilation becomes a tagged snapshot once it is byte-identical. The port
does **not** need byte-identical code, but it keeps the original's overall
shape.

## Decision

1. **The units are the game.** The port compiles the 83 original-shape units
   (`src/link_order.cmake`, kind `unit`) and the original `WinMain`. No game
   or engine logic lives outside a unit. Port-only code exists in only two
   places: in miniwin, below the *seam* (the DX6/Win32 import surface of
   finding 15), and in marked *library substitutions* inside the units.
2. **Delete the port shell.** The `LegorrrRun` rewrite, the hook tables and
   every port-tail file that duplicates unit behavior are deleted, not
   migrated. Before deletion, any behavior they encode that the units lack
   becomes a unit fix.
3. **Follow isle-portable: miniwin for its scope, library substitutions for
   the rest.** miniwin is the only adapter on every host, Windows included.
   The `winapi` platform and its MinGW/DirectX build are dropped.
   - **miniwin** covers windows, DirectDraw, Direct3D, D3DRM and DirectInput.
     The units call it through C++ interface syntax (`obj->Method(...)`), not
     C `lpVtbl->` calls. miniwin grows the interface versions the game uses
     that it lacks:
     - DirectDraw: `IDirectDraw4`, `IDirectDrawSurface4`.
     - D3DRM3: Frame3, Device3, MeshBuilder2/3, AnimationSet2, UserVisual,
       Viewport2, Texture3, Material2, and fog.
     - D3DIM: Device3, Viewport3, Texture2.
     - DirectInput device 2.

     These changes go upstream to isle-portable first. Patches still
     awaiting review are carried on a fork that the submodule points at, and
     dropped as upstream accepts them.
   - **Library substitutions** replace the Windows-only APIs outside
     miniwin's scope, directly in the units that call them, in the same way
     as isle-portable's library-substitution table:

     | Original | Substitution | Marker |
     | --- | --- | --- |
     | DirectSound, 3D sound | SDL3 audio and miniaudio | `// [library:audio]` |
     | winmm time | SDL3 | `// [library:time]` |
     | winmm MCI CD audio | audio files through miniaudio | `// [library:audio]` |
     | AVIFile movies | FFmpeg | `// [library:video]` |
     | Registry | ini file (libiniparser) | `// [library:config]` |
     | Files | SDL3 | `// [library:filesystem]` |

     Each replaced call site carries its marker, so a search for one marker
     finds every place a subsystem was substituted. A substitution changes
     only the call it replaces and leaves the unit's shape intact.
4. **64-bit from the start.** Unit records hold real pointers. Offset comments
   remain as documentation of the original's layout, but nothing enforces
   them. Only records read from or written to disk (WAD, save games, `.lws`,
   `.lwo`, `.map`) keep fixed-width fields, and they get explicit parse and
   serialize functions instead of `memcpy`. Pointer-to-`uint32_t` image
   macros such as `LEGO_LEVEL_PTR` go away.
5. **Original shape is kept, matching rules are not.** On the develop branch:
   - Units keep their files, function order, names, signatures and globals.
   - Units compile as C++17. The C89 subset rule, the MSVC 5.0 compatibility
     rule and `make reccmp-check` do not apply.
   - Code is free to change for correctness, portability and safety.
6. **One branch, then a hand-over.**
   - `develop` forks from `main` now. Decomposition work continues on `main` until
     it is byte-identical, then `main` is tagged (the *decomp tag*) and `develop`
     becomes `main`.
   - Decomp changes made after the fork reach `develop` only when they fix
     behavior, and only by hand. Changes made purely for score are not ported.

## Validation

The oracle is the shipped `LegoRR.exe` itself, run under the *A.2 harness*
with Microsoft's `D3DRM.DLL` (Wine's built-in d3drm stubs
`CreateUserVisual`).

- **Recording the original:** proxy DLLs for ddraw, d3drm, dinput, dsound
  and winmm sit in front of the shipped binary. They log every seam call with
  its arguments and return value, and inject a *scenario*'s input and timer
  readings.
- **Logging native:** miniwin logs the same calls in the same normalized
  form. A substituted call site logs the original call it replaced (for
  example `IDirectSoundBuffer::Play` with its arguments), so the shipped
  binary's dsound and winmm calls still line up one to one.
- **Only the game's own calls are logged.** Microsoft's `D3DRM.DLL` calls
  ddraw and d3dim internally, and miniwin's implementation will not make the
  same calls. Each log therefore records only calls made from the game's
  code: the proxies filter by return address, and miniwin logs at its public
  entry points.
- **Reading the first divergence:** there are only two cases.
  - An earlier call returned something different, which is a bug in miniwin
    or in a substitution.
  - The game made a different call after identical returns, which is a unit
    bug.

The guardrails are:

- **CI (`native-build`):** builds x86-64 native and runs ctest. It also lints
  the units: no `LEGORRR_BACKEND_*`/`MINIWIN` guards in a unit, and no
  `lpVtbl`.
- **Seam golden tests in ctest:** each miniwin extension is tested on
  synthetic data only, for example blit and colour key, frame transforms,
  mesh builder output and a 3D-sound pan.
- **Local `make port-check`:**
  - Replays recorded scenarios headlessly on native, under ASan and UBSan.
    A scenario is the recorded input and timer readings.
  - Diffs the native seam log against the shipped binary's recorded log for
    the same scenario. Floating-point
    arguments are compared with a tolerance, because x87 and SSE differ.
  - Compares framebuffer checkpoints with a tolerance.
  - Ratchets a committed baseline of each scenario's first divergent frame,
    in the same way as the *match baseline*: a change may not move a
    divergence earlier unless it refreshes the baseline.

Replays depend on the game reading time and input only through the seam. Any
unit that reads them some other way is fixed so that it goes through the seam.

## Considered options

- **Keep and finish the port shell.** Rejected. It is a second implementation
  of the game, and every gameplay fix would have to be made twice.
- **Our own C-ABI DX6 layer, with units kept in matching form.** Rejected
  once byte identity stopped binding the port. The units' `lpVtbl` calls
  would be the only thing it made possible, and it would throw away
  miniwin's working DirectDraw and SDL3 code.
- **A sibling shim library supplying `dsound.h`, `mmsystem.h`, `vfw.h` and
  the registry in miniwin's style.** Rejected. It would keep these calls
  unchanged in the units, but it would be a second emulation layer for APIs
  the game uses in only a few places, and it would diverge from the sibling
  project's convention. The substitution markers and logging at the
  substituted call sites keep the trace comparison working.
- **Put those APIs into miniwin.** Rejected. They lie outside miniwin's
  scope, and upstream has no use for them, because isle-portable replaced
  them in its game code.
- **Keep an i686 MinGW reference build on real DirectX.** isle-portable keeps
  exactly one such configuration, MSVC x86 against its bundled DirectX 5 SDK,
  as a compile check. Rejected. It would
  run the port's own source on real DirectX, but the shipped binary is a
  better oracle, and seam logs with return values already separate adapter
  bugs from unit bugs. Keeping it would cost a second header set the units
  must compile against, 32-bit builds, and Wine in CI. The *matching build*
  at the decomp tag still exists if a rebuilt-from-source binary on real
  DirectX is ever needed.
- **i386 first, 64-bit later.** Rejected. It would make an i686 SDL3 and
  multilib dependency the first milestone, and every unit would need
  reworking a second time for macOS and ARM.
- **Hand `main` to the port now, or wait for the decomp tag.** Rejected.
  Handing it over now strands the decomp work. Waiting stalls the port on a
  goal it no longer depends on.

## Consequences

- The units fork on the develop branch. Each decomp change after the fork that
  fixes behavior has to be reapplied by hand, and conflicts are expected
  wherever a unit was rewritten for 64-bit or the seam syntax.
- The develop branch drops the *matching build*, Reccmp tooling, the *link-order
  list*, the port tail's `lrr_tail.lib` arrangement, the `winapi` platform
  and the *reference build*. The decomp tag preserves them.
- *Port runs* (ADR 0002) are no longer part of port validation. The A.2
  harness runs only the shipped binary, behind the recording proxy DLLs.
- The `AGENTS.md` on the develop branch states its own rules. The MSVC 5.0 and
  `reccmp-check` rules stay on `main`.
- ADR 0004's "upstream contributions, not forks" becomes "upstream first, with
  a fork for patches still in review", because the miniwin gap is too large
  to wait on review.

## Order of work

1. Fork `develop` and drop the `winapi` platform. Switch the units to
   `obj->Method(...)` seam syntax. Separately, build the recording proxy DLLs
   and record the first scenarios from the shipped binary. That work needs
   no port code, so it can start at once.
2. Make the units 64-bit clean and give the on-disk records explicit parsers.
   Native then links the units against stubbed seam calls that log and abort,
   and boots as far as the first missing piece.
3. Delete the port shell and its hook tables.
4. Fill the seam in boot-path order:
   - window, message pump and time;
   - registry and files;
   - DirectInput (SDL scancode → DIK table);
   - DirectDraw4, which brings up the front end and menus.
5. Add D3DRM3 and D3DIM, the largest piece. This brings up the level view.
6. Substitute audio and video: DirectSound and 3D sound, MCI CD from audio
   files, and AVI through FFmpeg. The time, config and filesystem
   substitutions are already needed in step 4.
7. Add `make port-check` once the first scenario reaches gameplay, and
   ratchet it from there.
8. At the decomp tag, `develop` becomes `main`.
