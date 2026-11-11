# Finding 505 — Original translation units and link order

Issue #383. Maps every function in the shipped `LegoRR.exe`
(SHA-256 `c3d8ce23…e581e0f`) to an **original translation unit**, in link
order. A unit is the contiguous range of the binary built from one source
file of the original 1999 build. This is analysis only; nothing under `src/`
moved.

- Per-function map:
  [`docs/coverage/translation-units.csv`](../coverage/translation-units.csv).
  Columns: address, function name, unit order, unit id, unit name, unit kind,
  confidence, evidence, and the `src/` file that currently carries its Reccmp
  marker.
- Per-unit table:
  [`docs/coverage/translation-unit-ranges.csv`](../coverage/translation-unit-ranges.csv).
  Columns: text range, name and name basis, boundary confidence and evidence,
  slack, attributed `.rdata`/`.data`/literal/`.bss` ranges, and the curator's note.
- Curated input: [`docs/coverage/translation-units.spec.csv`](../coverage/translation-units.spec.csv).
  It holds one row per unit (start address, id, name, basis, note).
- Tool: [`tools/translation_units.py`](../../tools/translation_units.py).
  It extracts the evidence, proposes boundaries, validates the spec and
  writes both CSVs.

## Summary

| Segment | Text | Units | Named | Functions |
| --- | --- | ---: | ---: | ---: |
| Game objects | `00401000`–`004721e0` | 50 | 20 | 1,681 |
| gods98 engine | `004721e0`–`0048d920` | 31 | 9 | 575 |
| Static CRT (LIBC.LIB) | `0048d920`–`0049b6b0` | 134 | 130 | 267 |
| Import thunks (+ dinput data) | `0049b6b0`–`0049c7c0` | 6 | 6 | 14 |
| Late-linked library members | `0049c7c0`–`0049ddf5` | 6 | 4 | 27 |
| **Total** | | **227** | **169 + 4 symbol-only** | **2,564** |

How the names are supported:

- 5 are `attested` by a `__FILE__` path in the binary: `Viewports.c`,
  `Main.c`, `Images.c`, `Input.c` and `Files.c`.
- 15 are `string-prefix`: the unit first uses identifiers carrying the module
  stem, such as `Advisor_*`, `AITask_*`, `Info_*` or `Flic File…`.
- 7 are `config-key`: the unit owns the config section named after the
  module, such as `Dependencies`, `Encyclopedia`, `HelpWindow`, `FrontEnd`,
  `LightEffects`, `Objective…` and `Priorities`.
- 2 are `inferred` stems: `Lws.c` from the LWS scene strings and `Wad.c` from
  the `WWAD` magic.
- 144 are `library-symbol` names: CRT and import-library objects identified
  by the public symbols they define. Four of these have known symbols but an
  unknown VC5 file name.
- The other 54 units are numbered only. Each note says what the unit does.

String-prefix and config-key names give the module **stem** plus `.c`. The
exact spelling (for example `Pointer.c` against `Pointers.c`) is not attested.

Confidence:

| | High | Medium | Low |
| --- | ---: | ---: | ---: |
| Boundaries | 61 | 123 | 43 |
| Functions | 1,997 | 211 | 356 |

Data ranges are attributed to 116 units:

| Region | Units |
| --- | ---: |
| `.rdata` | 74 |
| String literals | 59 |
| Initialised `.data` | 37 |
| Per-object `.bss` | 68 |

Units with no references of their own in a region get no range there; the
map does not guess one.

## How MSVC 5.0 laid this binary out

These rules were measured on this binary. The method depends on them.

1. **One contiguous run per object in every section, in the same order.**
   The order of code in `.text` matches the order of each object's
   contributions to `.rdata`, `.data` and `.bss`.
2. **String literals are pooled across objects and placed with their first
   user.** A literal lives in the `.data` block of the first object in link
   order that uses it. Later users point backwards. In address order, the
   first users of 2,021 referenced literals are 99.4% non-decreasing.
3. **Section regions.**
   - `.data` `004a1000`–`004b2db0` holds initialised tables and literals.
   - `004b2db0`–`00549520` is per-object `.bss` (statics and zero-initialised
     globals) in link order.
   - From `00549520` the C *common* symbols follow, in no link order. These
     are tentative definitions such as module globals structs.

   The boundary at `00549520` is where the CRT's own `.bss` ends and
   references jump back to game code.
4. **`.rdata`.**
   - `.rdata` constants are mostly private per-object copies; the same float
     appears many times.
   - The first user (and usually only user) owns each one, and first users
     keep 88% address order.
   - A 16-byte `{1,2,4,-1}` table opens the `.rdata` block of 19 gods98
     objects and one game object (`0049e034`). Each copy counts one object.
5. **Padding does not separate C objects.**
   - Every C function is 16-byte aligned with `0x90` fill, inside and between
     objects (2,503 of 2,564 functions are aligned).
   - `int3` fill follows assembly objects, and also C functions that contain
     inline assembly (`trnsctrl.cpp`, `stricmp`).
   - Unaligned, packed functions mark an object compiled differently. The
     game camera unit `00435b20`–`00436b87` is one.
6. **Unreferenced header inline functions are emitted at the end of an
   object.** Two examples:
   - The `DisplayWidth`/`DisplayHeight` getters and two `FormattedText`
     printf wrappers close Advisor. Advisor never calls them.
   - Two getters called only from gods98 close AITask.

   Referenced inlines (`Vector3*`) sit between the functions that use them.
7. **Link order.**
   1. Game objects, in case-insensitive alphabetical project order. The 20
      named game units are strictly sorted, from `Advisor.c` and `AITask.c`
      to `ToolTip.c`.
   2. The gods98 engine. It is not alphabetical: `Viewports.c` comes before
      `Main.c`, which comes before `Images.c`, `Input.c` and `Files.c`. That
      fits library-member pull order.
   3. LIBC.LIB.
   4. The import libraries, in this order: d3drm, ddraw, avifil32, dinput,
      dsound, kernel32.
   5. Late library members, found by a second resolution pass. These are an
      RNC-style WAD decompressor, a display-mode dialog object, and the CRT
      `_strnicmp`, `_itoa`, `_strlwr` and `_strupr`. Those four are reached
      through `oldnames.lib` aliases.
8. **C++ is present in gods98 only.** Two objects use `thiscall` methods,
   vtables and exception-handling frames: the DirectShow player at
   `004721e0` and the AVI player at `0047e460`. Their `.text$x` unwind
   funclets sit at the very end of `.text` (`0049dd90`–`0049ddf4`).

## Method

1. **Extract.**
   - Disassemble every manifest function with Capstone, following branches
     and in-function jump tables to find its end, fill bytes and absolute
     references.
   - Classify each reference into `.rdata`, literal, initialised data,
     per-object `.bss` or common.
   - Record direct calls.
2. **Propose.** `translation_units.py propose` runs a contiguous
   (modularity) segmentation of a cohesion graph. Its edges are shared data
   items, data-order adjacency between first users, and calls. The result
   only seeded curation.
3. **Curate.** Units were fixed by:
   - plots of text index against data rank;
   - `dossier` listings of first-used literals, `.bss` blocks and caller
     locality;
   - the alphabetical constraint for game names;
   - `__FILE__` anchors, the `.rdata` table count and C++ structure for
     gods98;
   - CRT public symbols, `int3` and packed padding, and the CRT `.bss` order
     for the libraries.
4. **Validate.** `build` assigns every data item a home unit by a monotone
   dynamic programme. Home units must not decrease with address (link order),
   and the programme maximises the items homed with one of their users.
   - **Anchors.** A function is anchored when it first uses an item homed in
     its own unit.
   - **Conflicts.** A literal or `.rdata` item that a function owns but that
     is homed in another unit is a conflict. The published spec has zero
     conflicts.
5. **Score boundaries.** The evidence kinds are padding or alignment regime,
   library-status change, segment change, `__FILE__`, library-symbol and
   data steps per region.
   - **Slack** is the number of unanchored functions between the last anchor
     on the left and the first on the right.
   - Unanchored functions in the slack vote by call locality.
   - **High:** structural evidence, or a data step with zero slack.
   - **Medium:** a data step with small slack or unanimous votes, or both
     sides identified by library symbols.
   - **Low:** everything else.
6. **Function confidence.**
   - **High:** the function is anchored or bracketed by anchors of its own
     unit.
   - Otherwise it takes the confidence of the boundary whose slack zone it
     sits in.

To reproduce, with `pip install pefile capstone numpy` and `reference/LegoRR.exe` present:

    python3 tools/translation_units.py check     # validate the spec
    python3 tools/translation_units.py build     # regenerate both CSVs
    python3 tools/translation_units.py layout    # compare with src/
    python3 tools/translation_units.py dossier 401000 402000

## Unit list

Units in link order are below. The CRT, import and late units are in the
ranges CSV with symbols in their notes.

### Game objects

| Order | Unit | Name (basis) | Text | Fns | Boundary |
| ---: | --- | --- | --- | ---: | --- |
| ---: | --- | --- | --- | ---: | --- |
| 0 | GAME-001 | `Advisor.c` (string-prefix) | `00401000`–`00401cb0` | 32 | high |
| 1 | GAME-002 | `AITask.c` (string-prefix) | `00401cb0`–`004065f0` | 94 | low |
| 2 | GAME-003 | — | `004065f0`–`00406990` | 7 | medium |
| 3 | GAME-004 | — | `00406990`–`004070b0` | 21 | medium |
| 4 | GAME-005 | `Bubbles.c` (string-prefix) | `004070b0`–`00407d60` | 15 | medium |
| 5 | GAME-006 | — | `00407d60`–`004089d0` | 22 | medium |
| 6 | GAME-007 | — | `004089d0`–`00409d40` | 31 | low |
| 7 | GAME-008 | — | `00409d40`–`0040a0c0` | 4 | medium |
| 8 | GAME-009 | — | `0040a0c0`–`0040ab70` | 11 | medium |
| 9 | GAME-010 | `Dependencies.c` (config-key) | `0040ab70`–`0040b310` | 9 | low |
| 10 | GAME-011 | — | `0040b310`–`0040bdc0` | 11 | high |
| 11 | GAME-012 | — | `0040bdc0`–`0040cdc0` | 27 | low |
| 12 | GAME-013 | — | `0040cdc0`–`0040e490` | 24 | medium |
| 13 | GAME-014 | `Encyclopedia.c` (config-key) | `0040e490`–`0040e930` | 6 | high |
| 14 | GAME-015 | — | `0040e930`–`0040f6c0` | 15 | high |
| 15 | GAME-016 | — | `0040f6c0`–`004102b0` | 23 | low |
| 16 | GAME-017 | `FrontEnd.c` (config-key) | `004102b0`–`00418190` | 132 | low |
| 17 | GAME-018 | `HelpWindow.c` (config-key) | `00418190`–`00419230` | 19 | low |
| 18 | GAME-019 | `Info.c` (string-prefix) | `00419230`–`0041a300` | 36 | high |
| 19 | GAME-020 | `Interface.c` (string-prefix) | `0041a300`–`0041f870` | 73 | medium |
| 20 | GAME-021 | `Lego.c` (string-prefix) | `0041f870`–`00435b20` | 216 | low |
| 21 | GAME-022 | — | `00435b20`–`00436c20` | 27 | high |
| 22 | GAME-023 | `LegoObject.c` (string-prefix) | `00436c20`–`0044c710` | 258 | high |
| 23 | GAME-024 | `LightEffects.c` (config-key) | `0044c710`–`0044db50` | 20 | high |
| 24 | GAME-025 | — | `0044db50`–`0044e0c0` | 5 | high |
| 25 | GAME-026 | — | `0044e0c0`–`00451cd0` | 62 | medium |
| 26 | GAME-027 | `Message.c` (string-prefix) | `00451cd0`–`00452df0` | 26 | low |
| 27 | GAME-028 | — | `00452df0`–`004574e0` | 30 | high |
| 28 | GAME-029 | `Objective.c` (config-key) | `004574e0`–`00459190` | 14 | low |
| 29 | GAME-030 | — | `00459190`–`00459530` | 8 | high |
| 30 | GAME-031 | — | `00459530`–`0045a030` | 8 | medium |
| 31 | GAME-032 | `Panel.c` (string-prefix) | `0045a030`–`0045c830` | 52 | high |
| 32 | GAME-033 | `Pointer.c` (string-prefix) | `0045c830`–`0045cdc0` | 9 | medium |
| 33 | GAME-034 | `Priorities.c` (config-key) | `0045cdc0`–`0045d7e0` | 14 | high |
| 34 | GAME-035 | — | `0045d7e0`–`0045d8b0` | 3 | low |
| 35 | GAME-036 | — | `0045d8b0`–`0045f030` | 11 | low |
| 36 | GAME-037 | — | `0045f030`–`00462da0` | 52 | high |
| 37 | GAME-038 | — | `00462da0`–`004636b0` | 9 | low |
| 38 | GAME-039 | — | `004636b0`–`00464110` | 12 | low |
| 39 | GAME-040 | — | `00464110`–`00464740` | 3 | low |
| 40 | GAME-041 | `SFX.c` (string-prefix) | `00464740`–`004661c0` | 35 | medium |
| 41 | GAME-042 | — | `004661c0`–`004667e0` | 9 | low |
| 42 | GAME-043 | — | `004667e0`–`0046a370` | 63 | low |
| 43 | GAME-044 | — | `0046a370`–`0046a7f0` | 9 | low |
| 44 | GAME-045 | `Text.c` (string-prefix) | `0046a7f0`–`0046b1d0` | 10 | medium |
| 45 | GAME-046 | `ToolTip.c` (string-prefix) | `0046b1d0`–`0046c160` | 16 | high |
| 46 | GAME-047 | — | `0046c160`–`0046c6f0` | 5 | medium |
| 47 | GAME-048 | — | `0046c6f0`–`0046db90` | 26 | low |
| 48 | GAME-049 | — | `0046db90`–`0046eb80` | 12 | low |
| 49 | GAME-050 | — | `0046eb80`–`004721e0` | 45 | low |

### gods98 engine

| Order | Unit | Name (basis) | Text | Fns | Boundary |
| ---: | --- | --- | --- | ---: | --- |
| ---: | --- | --- | --- | ---: | --- |
| 50 | G98-001 | — | `004721e0`–`00472710` | 11 | high |
| 51 | G98-002 | — | `00472710`–`00476d50` | 107 | low |
| 52 | G98-003 | `Viewports.c` (attested) | `00476d50`–`00477440` | 22 | low |
| 53 | G98-004 | — | `00477440`–`004777a0` | 7 | low |
| 54 | G98-005 | `Main.c` (attested) | `004777a0`–`00478df0` | 24 | high |
| 55 | G98-006 | — | `00478df0`–`00479500` | 16 | high |
| 56 | G98-007 | — | `00479500`–`00479ee0` | 20 | medium |
| 57 | G98-008 | — | `00479ee0`–`0047a640` | 11 | low |
| 58 | G98-009 | — | `0047a640`–`0047c170` | 42 | high |
| 59 | G98-010 | — | `0047c170`–`0047d410` | 17 | low |
| 60 | G98-011 | `Images.c` (attested) | `0047d410`–`0047e460` | 27 | low |
| 61 | G98-012 | — | `0047e460`–`0047ed90` | 25 | medium |
| 62 | G98-013 | `Input.c` (attested) | `0047ed90`–`0047f130` | 6 | low |
| 63 | G98-014 | `Files.c` (attested) | `0047f130`–`004805b0` | 36 | low |
| 64 | G98-015 | — | `004805b0`–`00483c80` | 58 | medium |
| 65 | G98-016 | `Flic.c` (string-prefix) | `00483c80`–`004850f0` | 24 | high |
| 66 | G98-017 | — | `004850f0`–`00485a20` | 13 | low |
| 67 | G98-018 | `Keys.c` (string-prefix) | `00485a20`–`00485e80` | 2 | low |
| 68 | G98-019 | — | `00485e80`–`004869f0` | 12 | low |
| 69 | G98-020 | `Lws.c` (inferred) | `004869f0`–`00488b50` | 25 | high |
| 70 | G98-021 | — | `00488b50`–`00488c70` | 5 | low |
| 71 | G98-022 | — | `00488c70`–`00489210` | 6 | low |
| 72 | G98-023 | — | `00489210`–`004893f0` | 5 | low |
| 73 | G98-024 | — | `004893f0`–`004894c0` | 4 | high |
| 74 | G98-025 | — | `004894c0`–`00489c30` | 11 | low |
| 75 | G98-026 | — | `00489c30`–`00489d90` | 2 | low |
| 76 | G98-027 | — | `00489d90`–`0048b260` | 2 | low |
| 77 | G98-028 | — | `0048b260`–`0048b330` | 4 | low |
| 78 | G98-029 | — | `0048b330`–`0048b4a0` | 3 | low |
| 79 | G98-030 | `Wad.c` (inferred) | `0048b4a0`–`0048c040` | 15 | low |
| 80 | G98-031 | — | `0048c040`–`0048d920` | 13 | low |

Update (#409/#410, [finding 512](512-sound-unit-original-shape.md)): G98-021, G98-022 and G98-023
are one object and are now one unit, G98-021 (`00488b50`–`004893f0`, 16
functions). The current tables are in the coverage CSVs.

## Low-confidence areas (flagged, not guessed)

- **Game boundaries with no data step or large slack (21).** These are the
  starts of the following units:
  - AITask.c `00401cb0`: the Advisor inline tail is placed by the end-of-object
    inline rule, not by data.
  - `004089d0`, `Dependencies.c` `0040ab70`, `0040bdc0`, `0040f6c0`,
    `FrontEnd.c` `004102b0` and `HelpWindow.c` `00418190`.
  - `Lego.c` `0041f870`, `Message.c` `00451cd0` and `Objective.c` `004574e0`.
  - `0045d7e0`, `0045d8b0`, `00462da0`, `004636b0` and `00464110`.
  - `004661c0`, `004667e0`, `0046a370`, `0046c6f0`, `0046db90` and
    `0046eb80`.

  In each case the units themselves are evidenced (each owns literals,
  `.bss` or commons). What is uncertain is which side the few unanchored
  functions between them belong to.
- **Lego.c (216 functions) may be two objects.** An interior split near
  `0042f350` (terrain block helpers) cannot be excluded: the literals rise
  continuously and the unit's `.bss` is sparse.
- **Map mesh tail.** The model-record helpers `004519b0`–`00451cd0` have no
  data and are kept with the map mesh unit.
- **gods98 boundaries resting on behaviour only.**
  - The string/token helpers either side of `Viewports.c` and `Main.c`.
  - The text-window unit `004850f0`–`00485a20`.
  - The wave reader, MCI and sound front-end units `00488c70`–`004894c0`,
    which may be one file. (Settled for `00488b50`–`004893f0`: one object,
    finding 512.)
  - The file-list and registry units `0048b260`/`0048b330`, which may be one
    file.
- **Objects with no manifest function at their start** are absorbed into the
  previous unit, and the notes say so:
  - `__CIsqrt`/`_sqrt` (`0048e5c0`);
  - the Pentium FDIV workaround (real start `004923c0`);
  - `winxfltr.c` (`00495c00`);
  - `exsup3.asm` (`004967d0`);
  - the dinput `c_dfDI*` data (`0049b700`–`0049c7a7`, inside the avifil32
    unit's range).

  A matching build must split these even though the function manifest
  cannot.
- **CRT pairs with no data of their own.** `_open.c`/`stream.c`, and the
  `0049d030` boundary between the decompressor and the display-mode dialog.

## Where the current `src/` layout disagrees

Output of `translation_units.py layout`, using Reccmp marker locations:

- **74 of 227 units are spread over more than one source file.** 43 of the
  54 marked source files mix functions from more than one unit.
- **Engine and CRT files hold game code.**
  - `src/crt/crt.cpp` carries markers for 161 units. These include game
    functions such as 14 from `Panel.c` and 8 each from `FrontEnd.c` and the
    reward unit.
  - `src/gods98/record.cpp`, `view.cpp` and `simtick.cpp` draw from 31, 29
    and 14 units, mostly game objects. Examples are the camera unit, the
    Bezier and animation wrappers, and `FrontEnd.c`.
- **Game objects are split by responsibility rather than by original file.**
  - `Lego.c` spans `level.cpp` (103), `engine.cpp` (18), `entity.cpp` (14)
    and more.
  - `LegoObject.c` spans `entity.cpp` (172), `ai.cpp` (28), `spawn.cpp`
    (14) and `level.cpp` (12).
  - `FrontEnd.c` spans `frontend.cpp` (45) and `savegame.cpp` (28), among
    others.
  - `Info.c` lives mostly in `gods98/message.cpp` (25), and `SFX.c` in
    `gods98/sound.cpp` (23).
- **Single source files merge several original units.** For example,
  `game/frontend.cpp` holds `FrontEnd.c`, `Interface.c`, `Panel.c`,
  `HelpWindow.c`, `Lego.c` and others (23 units).
- **Some functions sit in the wrong segment.** The gods98 file-list, registry
  and WAD objects (G98-028–030) are partly in
  `game/startup_config.cpp`. The CRT `trnsctrl.cpp` routines are in
  `game/seh_runtime.cpp`.
- **Reccmp markers and ledger names that the map contradicts:**

  | Address | Current marker or ledger name | Actually |
  | --- | --- | --- |
  | `0048d9e0` | `_vsprintf` | `_sprintf` |
  | `0048e1e0` | `FormattedTextBufferBuild` | CRT `_vsprintf` |
  | `0048ead0` | `EngineCalloc` | `operator new` (`new.cpp`) |
  | `004907f0` | `ParseEntityHealthValue` | `_fltin` (`cfin.c`) |
  | `004907c0` | `StringPrefixReserve` | static `_shift` (`cvt.c`) |
  | `00490230` | `ProcessorFeatureFallbackFalse` | `_ms_p5_test_fdiv` |
  | `SehFrameUnwind`, `CallSettingFrame12`, `CrtExceptionFrame*`, `SehScopeTableLookup` | descriptive names | `trnsctrl.cpp` routines |

The full lists are reproducible with the `layout` command. Restructuring
(#388) should follow the per-unit table rather than these summaries.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
