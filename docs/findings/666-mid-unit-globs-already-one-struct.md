# Finding 666 — Interface.c, Reward.c, Panel.c, Objective.c and NERPs already keep their state in its original form

Issue #785, part of #388 and ADR 0005. Findings 653 (`legoGlobs`) and 662
(`objectGlobs`) made Lego.c's and LegoObject.c's state one struct each.
The issue asked for the same in five more units, on the premise that their
state was still separate stand-in globals.

**Result: the premise does not hold, and no source change is made.**

- Interface.c, Panel.c, Objective.c and Reward.c each already define one
  typed `*Globs` struct, at its original address, of its original size and
  layout. Their conversions (findings 505, 554 and 586, and the GAME-029 and
  GAME-037 conversions) built them that way.
- NERPs has its two original structs. Its `.data` scalars are separate
  globals in the original too, so there is nothing to fold.
- No other unit and no port file defines an annotated global inside any of
  the six structs' ranges.
- Of the 18 NERPs functions that read the `.data` scalars, three are not
  exact. Their differences are operand order, statement order and control
  flow, not data layout.

Measured on `main` `b928af37` with one full matching build and `make
reccmp-check`:

| Measure | `main` `b928af37` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 336,831 (52.42%) | 336,831 (52.42%) |
| Accuracy, full / game-only | 97.07% / 96.74% | 97.07% / 96.74% |
| Functions that rose / fell | | 0 / 0 |
| `unit-data-offsets`: the five units' rows | | unchanged |
| `unit-data-offsets`: unit summary (full / gaps-only / partial / n/a) | 0 / 17 / 13 / 197 | unchanged |
| `unit-data-offsets`: unattributed annotations | 79 | 79 |
| Image end (`make pe-audit`) | `0x7f5000` | `0x7f5000` |

`make reccmp-check` passes with no function lower. `make match-progress`,
`make unit-data-offsets` and `make pe-audit` were run. The baseline and
listing digests are unchanged. `binary-progress.json` changes in one
place: one 32-byte function moves from `original_annotated` (2,533 to
2,532) to `port_unit`. That is the `/OPT:NOREF` relink flip of findings
654 and 663, with no source change. `make tools-test` (no
`--allow-growth`), `make winapi-link-check` and native ctest (37 of 37)
pass.

## 1. The five units' state

| Unit | Struct | Address, size | Defined in | `unit-data-offsets` (`.bss` / commons) |
| --- | --- | --- | --- | --- |
| GAME-020 Interface.c | `interfaceGlobs` | `004dcd58`, `0x14ac` (to `004de204`) | `interface_unit.c` | gaps-only (2 items) / - |
| GAME-032 Panel.c | `panelGlobs` | `005000e0`, `0x7b8` (to `00500898`) | `panel_unit.c` | n/a (1 item) / - |
| GAME-029 Objective.c | `objectiveGlobs` | `004ffbc0`, `0x28c` (to `004ffe4c`) | `objective_unit.c` | n/a (1 item) / - |
| GAME-037 Reward.c | `rewardGlobs` | `00552980`, `0x3250` (common) | `reward.c` | n/a / `_rewardGlobs(size=12880)` |
| GAME-028 NERPs | `nerpsruntimeGlobs`, `nerpsGlobs` | `004ff958`, `0x68`; `00555d40`, `0xb4` (common) | `tutorial_script.c` | n/a (1 item) / `_nerpsGlobs(size=180)` |

The `before` and `after` rows are the same, because no input changed.
Every extent ends where the next unit's annotated state begins:

- Interface.c's struct ends at `004de204`, and Lego.c's `004de208`
  follows.
- Panel.c's ends at `00500898`.
- Objective.c's ends at `004ffe4c`, the GAME-029 range's `.bss` end.
- Reward.c's ends at `00555bd0`, and PTL.c's `ptlGlobs` starts at
  `00555be0`.
- NERPs' `.bss` ends at `004ff9c0`.

The other annotated items in these units are not struct members:

- `004dcd50` `s_lastDependencyObjIndex`, and the two initialised function
  statics `004a2b58` and `004a2b5c` (Interface.c);
- the NERPs function table (`004a5948`), the operator names (`004a6710`)
  and the scalars `004a673c`-`004a676c`;
- `0076ab48`, Input.c's `lClicked`, which `objective_unit.c` defines as a
  common until Input.c is converted.

### NERPs' `.data` scalars stay separate

Folding `004a673c`-`004a676c` (9 globals, 0x34 bytes) into one initialised
struct was the only candidate. There are three reasons not to:

- **OpenLRR names them separately.** Its `NERPsFile.cpp` (an exe build whose
  addresses sit 0x1000 higher) declares `nerpsHasNextButton`,
  `nerpsBOOL_004a7740`, `nerpsMessageTimerValues[3]`, `nerpsIconPos`,
  `nerpsIconWidth`, `nerpsIconSpace`, `nerpsUnkSampleIndex` and
  `c_NERPsRuntime_TutorialActionCallback` as separate globals.
- **The two zero dwords are already explained.** `004a6744` and `004a6754`
  are the zero tails of two small initialised objects, `NERPs_Next` and
  `NERPs_MessageTime` (finding 586). One struct would not explain them
  better.
- **The readers' differences are not data layout.** `make match-function`
  on the three readers that are not exact:

| Address | Function | Baseline | What differs |
| --- | --- | ---: | --- |
| `00453bb0` | nerps_runtime_update_timers | 96.97 | effective match: `fld`/`fadd` operand order in the timer loop (rule A/E) |
| `00457170` | nerps_set_tutorial_pointer | 95.12 | `imul` operand order, `[esi+4]` against `[edi+0x14]` (rule B phase, finding 663) |
| `00456830` | nerps_parse_message | 67.59 | branch layout and register roles throughout; the icon scalars are read in the same order on both sides |

A struct would cost or save symbols ahead of 100 or more NERPs functions,
and so re-roll their phases (finding 538 rule B), for no expected rise.

## 2. Readers outside the units

Only one reader outside the units reaches a field through anything but the
struct:

- **Lego.c's `extern uint32_t rewardGlobs[]`**
  (`lego_unit_imports.h`, finding 626). It is a word view with an offset
  macro for `itemsArray[5].startCount` (`00555118`), and it already names
  the original's object. Replacing it with the struct tag would add one
  symbol before Lego.c's functions. This is the cost finding 626 avoided.
  It stays.
- **The other readers** use the struct by its real type:
  - `lego_unit_imports.h`'s `panelGlobs`;
  - `lego_unit.c`'s `interfaceGlobs`;
  - Info.c, Priorities.c, ScrollInfo.c and Encyclopedia.c.
- **Port mirrors** do not reach the matching units:
  - `simtick.cpp`'s copies of `004de1f4`/`004de1fc`/`004de200`;
  - `objective.cpp`'s copies of `nerpsGlobs.messageLines`/`messageCount`.

  Both are unannotated C++ port state. No matching-build C unit reads them
  in place of a field. The panel tool-tip gate, the case finding 662
  closed, is already the field.

## 3. Mismatch causes

The issue says data layout appears in these units' recorded mismatch
causes. No below-100 function in the five units that reads their state
differs in a data operand of that state. Finding 660 records the largest
Interface.c near misses as register roles and MSVC 5.0 cross-jump choices,
and section 1 gives NERPs' readers.

## Follow-up

None for data layout in these five units. The next units with state still
spread over several definitions are the ones `unit-data-offsets` reports
`partial` in `.bss`, for example GAME-017's `004d7b08` gap.
