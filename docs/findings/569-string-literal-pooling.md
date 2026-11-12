# Finding 569 — String-literal pooling: `/O2` already pools, as the original did

Issue #553. Finding 560 put GAME-037's string-data gap down to "the
matching build doesn't merge repeated string literals" and suggested `/Gf`
or `/GF`. That diagnosis is wrong. The flag experiments ran on `main` at `63da6269`; the PR is stacked on PR #569 (`convert/556-game-013`, `9d81c971`), and its results below are measured there. Finding 563 is the whole-binary audit.

## Summary

- **The original pools literals across objects, in `.data`.** That is
  `/Gf`, not `/GF`.
- **The matching build already does the same.** In MSVC 5.0, `/O1` and
  `/O2` emit every literal as a `??_C@` COMDAT in `_DATA`, which is `/Gf`'s
  output. The linker keeps the copy from the first object in link order.
  **No flag is adopted.** An explicit `/Gf` changes nothing, and `/GF` moves
  every literal into `.rdata`, which the original's strings never use.
- **GAME-037's gap has two causes, and neither is a flag:**
  - **A tool artefact.** `unit_data_offsets.py` laid out each object's
    sections itself, and counted string COMDATs that the linker had taken
    from an earlier object. This tool is fixed: it now agrees with the
    linked addresses on 795 of 795 offsets, against 49 disagreements before.
  - **One real divergence.** The original pools `"Text"` (`004a26b0`) into
    GAME-017 (FrontEnd.c, the `Menu::Overwrite` lookup in `LoadMenuBuild`,
    `004161f0`). The rebuilt `frontend_unit.cpp` has no such literal, so
    `"Text"` lands in `reward.c.obj`: 8 bytes before `"Flics"`
    (`004a9394`). Converting GAME-017 fixes this.
- **The `// STRING` rule is relaxed.** A pooled literal gets **one**
  marker, in the unit that owns its address. Before, it got none. Reccmp
  then labels every unit's use of that literal. Marking the four pooled
  literals that converted units own (`","`, `"NULL"`, `"Reward"`,
  `"Quota"`) changed 22 functions on top of PR #569:
  - 21 improve, four of them to 100%;
  - exact functions go from 1,022 to 1,026, with none lost;
  - exact code bytes go from 87,662 to 88,637;
  - one low-scoring port function drifts by about one point (below).
- **A gate fix.** Reccmp colours its log level (`[\x1b[31mERROR\x1b[0m]`),
  so `reccmp_diagnostics.py` never saw Reccmp's `[ERROR]` lines. One of the
  lines it missed is the duplicate-marker error that this rule depends on.
  With the fix the gate caught four duplicate markers already on the stack,
  which this PR removes (see the gate fix).

## Evidence from the original

Sections of `LegoRR.exe`: `.rdata` is `0x2be0` bytes (11,232 bytes of
floats, doubles and tables), and `.data` holds every string literal.

`tools/experiments/553/pooled_strings.py` scans `.text` for every 4-byte
value that points at the start of a printable, non-empty, NUL-terminated
string. It attributes each referencing site to its function and unit
through `docs/coverage/translation-units.csv`.

| Measure | Value |
| --- | --- |
| referenced string addresses | 2,103 (2,076 `.data`; 27 `.rdata`, all of them constants that happen to read as text) |
| addresses used by more than one function | 121 |
| addresses used by more than one **unit** | 83 |
| texts stored at more than one referenced address | 6, none of them two game-code literals with one text |

The largest cross-unit shares:

| Address | Text | Sites | Units |
| --- | --- | --- | --- |
| `004a1cd4` | `":"` | 61 | 13 |
| `004a1d60` | `"%s"` | 55 | 17 |
| `004a13e4` | `","` | 36 | 17 |
| `004a1eac` | `"r"` | 21 | 7 |
| `004a13e0` | `"LWS"` | 17 | 6 |
| `004a2378` | `"\|"` | 17 | 5 |
| `004a13f4` | `"NULL"` | 7 | 5 |

The six repeated texts are:
- CRT copies (`"\n"` and `"."` in LIBC objects);
- float constants that happen to read as one character (`"("` at
  `004aa454` and `004a6764`, `"0"` in GAME-023, `"AN"` in `.rdata`);
- `004aaf44`, the `"r"` tail of `"Error"` that an inlined `strcpy` reads as
  a word.

So the original pools across objects, and its pooled literals are writable
`.data`, which is `/Gf`. With `/GF` they would sit in `.rdata`. Each literal
sits in the first object that uses it, as finding 511 found for DxError.c.

## What MSVC 5.0 emits

`a.c` uses `","` twice in one function and once in another, plus `"NULL"`.
It was compiled with the toolchain's `cl`, `/ML /DNDEBUG /FAs`:

| Flags | Literal output |
| --- | --- |
| `/Od` | one anonymous `$SGnn` per use: three `","` copies |
| `/Od /Gf` | `??_C@_01KMAE@?0?$AA@` COMDAT in `_DATA`, used by every `","` |
| `/O1` | as `/Od /Gf` |
| `/O2 /Ob1` (the matching build) | as `/Od /Gf` |
| `/O2 /Ob1 /GF` | the same COMDATs in `CONST` (read-only, `.rdata`) |

The matching build's `LegoRR.map` confirms the link-time merge. `","` is
stored once, at `0055f3e4` in `advisor.c.obj`. That is GAME-001, which also
owns `004a13e4` in the original. `"Reward"`, `"Quota"` and `"Crystals"` sit
in `panel_unit.c.obj` (GAME-032), again as in the original.

Flags of the matching build: `CMAKE_C_FLAGS` `/DWIN32 /D_WINDOWS /ML`,
`CMAKE_CXX_FLAGS` `/DWIN32 /D_WINDOWS /GX /ML`, and the RelWithDebInfo
defaults `/Zi /O2 /Ob1 /DNDEBUG`. These come from `CMakeLists.txt` (the
classic-profile block) and `tools/reccmp.sh`.

## Flag builds

Each is a full `make reccmp-check` (MSVC 5.0, developer distrobox) on
`63da6269`, with the flag appended in `CMakeLists.txt`'s classic-profile
block.

| Build | Reccmp mean (full / game) | Drops | `unit-data-offsets` | exact code bytes | `.data` / `.rdata` identical |
| --- | --- | --- | --- | --- | --- |
| `main` | 62.20 / 58.20 | — | full 0, gaps-only 2, partial 30 | 79,550 | 18.01% / 10.32% |
| `+ /Gf` | 62.20 / 58.20 | 0 | full 0, gaps-only 2, partial 30 | 79,550 | unchanged |
| `+ /GF` | 62.20 / 58.20 | 0 | full 0, gaps-only 1, partial 28 | 79,550 | 27.25% / 6.31% |

- **`/Gf`**: `.rdata` and every string byte are identical to `main`. The
  other differences are 28 `.bss` symbols in `upgrades.cpp` that trade
  places, and the operands that refer to them. Those statics are in an
  anonymous namespace whose per-build hash (`lego.cpp1928227770@…`) changes
  from build to build. This is build nondeterminism, not the flag.
- **`/GF`**: the strings move to `.rdata`, which grows by 52 KB, and data
  alignment gets worse. `.data`'s identical share rises only because the
  measure compares bytes position by position: the strings left `.data`,
  so more zero bytes line up. Without `/GF` it is rejected. With `/GF`,
  every object also warns `D4025: overriding '/Gf' with '/GF'`. That
  warning came from a cached `/Gf` left over from the previous experiment,
  not from `/O2`. The classic-profile block writes its flags back to the
  CMake cache with `FORCE`, so a flag you add there persists until
  `build-reccmp/CMakeCache.txt` is deleted.

## The tool fix: discarded COMDATs

`unit_data_offsets.py` computed an item's object offset as the section's
base plus the symbol's value. The base summed every earlier section of the
same group in the object. A string COMDAT that the link took from another
object (because that object came first) still advanced the base, so every
later literal looked shifted.

`coff()` now takes a `discarded(name)` predicate. A COMDAT section is
skipped when the map places its leader symbol (the first external
definition in the section) in another object. `measure()` builds that
predicate from the map.

Measured on the `main` build. Disagreements are object offset gaps that
differ from the linked address gaps:

| Tool | Disagreements | GAME-037 | G98-020 `.data` |
| --- | --- | --- | --- |
| before | 49 of 795, in G98-020, GAME-032 and GAME-037 | gap 36, first `004a9370` (`"Sounds"`) | gap 9, first `004ab670` |
| after | 0 of 795 | gap 32, first `004a9394` (`"Flics"`) | gap 3, first `004ab6bc` |

On `main` the unit summary stays at full 0, gaps-only 2, partial 30, n/a
193. On PR #569's base build, the fix moves G98-004 from `partial` to
`gaps-only` (full 0, gaps-only 2 to 3, partial 29 to 28, n/a 194). It also
corrects the first gap reported for GAME-020, GAME-032, GAME-037 and
G98-020. The other units that the artefact pushed to `partial` still have
real gaps. GAME-037's
remaining gap is `"Text"` (above).

## The `// STRING` rule

The old rule (findings 549, 550, 553, 554, 560, 561) was: a literal pooled
with another unit gets no marker. Its stated reason was that a marker
"relabels their operands" (the `advisor.c` comment). Relabelling is the
point: the original side of a use has only a bare address (`push
0x4a8a00`) until some marker names it. Tried with `match-function`:

| Function | No marker | Marker in owning unit | Second marker in a using unit |
| --- | --- | --- | --- |
| `0x0045c040` mission_crystal_quota_setup (`panel_unit.c`) | 85.42 | 93.75 (`"Reward"`, `"Quota"`) | — |
| `0x0045f530` reward_load_item_text (`reward.c`) | 93.44 | 95.08 | 96.72 (`"Reward"` + `"Text"`) |

A second marker for the same address is not allowed. Full Reccmp keeps one
`STRING` per address and logs `[ERROR] Dropped duplicate address 0x4a89f8 on
STRING annotation in file: …/reward.c`. Before this change, that error did
not fail the gate (see the gate fix). Hence the rule:

- Mark each literal address **once** in the whole tree, in the unit that
  owns it, even when other units reuse it.
- Leave uses in other units unmarked. A literal owned by an unconverted
  unit (such as `"Text"`) stays unmarked until that unit's conversion marks
  it.
- One literal per marked line. A marker binds to the first literal on the
  next line, and two stacked markers bind only one. A call can be split
  inside its argument list: the function annotation still loads.

`tools/convert_unit.py`'s scaffold comment now says this.

**Applied** to every pooled literal that a converted (`.c`) unit owns and
does not yet mark. On `main` the script's last list found two, in
`advisor.c` (`","`
`004a13e4`, `"NULL"` `004a13f4`), and `panel_unit.c` adds `"Reward"`
`004a89f8` and `"Quota"` `004a8a00`. `"Crystals"` (`004a8a08`, mission.cpp's
annotated `s_task_name_crystals`) and `""` (`004b8a14`, config.c's
`s_config_angle_empty`) are annotated `GLOBAL`s and stay as they are.

**Stacked conversions.** This PR sits on top of PR #572 (GAME-019),
PR #571 (GAME-021 part 1) and PR #569 (GAME-013). Those three conversions followed
the old rule, and this PR does not retrofit their markers. On the stack's
base, `pooled_strings.py` lists eight pooled literals that GAME-021 owns
but leaves unmarked: `"Pilot"` `004a356c`, `"Freezer"` `004a3f08`,
`"Pusher"` `004a3f10`, `"MESH"` `004a4014`, `"Boulder"` `004a401c`,
`"%s%s%s"` `004a4024`, `"UpgradeTime"` `004a4418` and `"FlapInWind"`
`004a46a0`. GAME-019 and GAME-013 own none. Conversions after this one follow
the new rule; GAME-021's remaining parts can mark these eight.

## Results

`make reccmp-check`, `make match-progress` and `make unit-data-offsets`, on
this branch, against PR #569's head (`9d81c971`, built and measured as the
base):

| Measure | PR #569 | this branch |
| --- | --- | --- |
| Reccmp mean, full / game-only | 66.58 / 63.13 | 66.60 / 63.15 |
| exact functions (baseline at 100%) | 1,022 | 1,026 |
| exact function code bytes | 87,662 (13.64%) | 88,637 (13.79%) |
| `unit-data-offsets` | full 0, gaps-only 2, partial 29, n/a 194 | full 0, gaps-only 3, partial 28, n/a 194 (G98-004, from the tool fix); GAME-001 `.data` gaps-only with 38 items (was 36) |
| `binary-progress` data-offset agreement | 8 of 49 | 8 of 49 |

The changed functions:
- **New exact matches:** `0x00401530` advisor_position_config_load,
  `0x0045f1f0` reward_load_item_sound, `0x0045f230` reward_load_item_font
  and `0x004877e0` lws_sound_triggers_setup.
- **Improved:**
  - most of Reward.c: `0x0045f7f0` 92.11 to 98.25, `0x00460140` 89.09 to
    96.36, `0x0045fda0` 92.82 to 96.65;
  - `0x0045c040` 85.42 to 93.75;
  - `0x0041afd0` interface_load_item_panels, 77.04 to 78.52 (GAME-020,
    converted in PR #559);
  - `0x0045b9d0`, `0x0045d060`, `0x00401270`, `0x004869f0`, `0x00434710`
    and `0x00464d00`.
- **Drop (accepted):** `0x004345f0` frontend_config::panels_config_entry_apply,
  38.46 to 37.50 (`lego.cpp`). On `main` this PR also drifted `0x0041afd0`
  (then the port's `interface.cpp`, 12.53 to 11.49); GAME-020's conversion
  turned that into a gain.

  The source didn't change, and the listing has the same instructions
  (`drift; instructions +0`). The only difference is that `push "NULL"`
  now resolves to a `STRING` label on both sides. That moves Reccmp's
  alignment of this port-shaped function, which is far from the
  original's shape. This is the operand-level entropy of ADR 0005 rule 2,
  reported here as the net effect.

## The gate fix

`tools/reccmp_diagnostics.py` rejects `[ERROR]`, `[WARNING]` and
`[CRITICAL]` lines from the matching build and Reccmp. Reccmp writes the
level in colour, so the pattern never matched. It now removes ANSI escapes
before matching. A test in `tools/tests/test_reccmp_diagnostics.py` covers
the escapes.

Clean `main` logs have no coloured diagnostics. The stack below this PR
does: once the gate could see them, it failed on four STRING markers that
Reccmp had been dropping silently, each a second marker for one address:

| Address | Text | Kept | Removed |
| --- | --- | --- | --- |
| `004a41dc` | `"\n%s: %i/%i"` | first use, `lego_unit.c` | two more uses in the same function |
| `004a3acc` | `"LoaderProfile.txt"` | `lego_unit.c` (loader start) | `lego_unit.c` (loader shutdown) |
| `004a3ab0` | `"LoaderProfileNoSound.txt"` | `lego_unit.c` (loader start) | `lego_unit.c` (loader shutdown) |
| `004a2b54` | `"%i"` | `info_unit.c` (GAME-019, its owner) | `interface_unit.c` |

Each removed marker is the one Reccmp reported dropping, so no score
changes. They are the only change this PR makes to the three conversions
below it.

## Checks

- `make reccmp-check`: no diagnostics. One drop (above), accepted by the
  baseline refresh. Link-order check-map OK.
- `make match-progress`: regenerated `docs/coverage/match-baseline.txt`,
  `docs/coverage/match-listing-digests.txt` and
  `docs/coverage/binary-progress.json`.
- `make tools-test`: passes, including the new `unit_data_offsets` and
  `reccmp_diagnostics` tests; the lint ledger does not grow.
- Native: `cmake --build build` and `ctest --test-dir build`: 37/37 pass.
- `pooled_strings.py` on this branch lists only GAME-021's eight literals
  (above) as owned by a converted unit but unmarked.

## Reproducing

    python3 -I tools/experiments/553/pooled_strings.py .
    make reccmp-check && make unit-data-offsets
    python3 tools/match_function.py 0x0045c040 --json
    # flag experiment: append /Gf or /GF to the classic-profile flags in
    # CMakeLists.txt, delete build-reccmp/CMakeCache.txt, rerun reccmp-check
