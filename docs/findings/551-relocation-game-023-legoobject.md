# Finding 551 — GAME-023 (LegoObject.c) relocated, after two source fixes and a native-only bounds guard

Issue #521, following finding 548 (#517). Stage S3 of the
[#388 plan](../plans/388-original-link-order.md),
[ADR 0006](../adr/0006-original-link-order.md) decisions 3–4, entropy rules
2–3 and the reshape-commits addendum. Stacked on #528 (issue #525, finding
552), which relocates GAME-017, GAME-020, G98-011 and G98-014. Measured on
#528's head `3f15129c`; every figure below is a delta against it.

The branch was first measured on `main` `b5d7be57`. It was then rebuilt on
#528, as ADR 0006 rule 3 requires:
- The source-fix and guard commits were cherry-picked without conflicts and
  proved again with `reshape_check`. The results were the same.
- The relocation was generated again by the tool, with the same plan.
- The lint and generated-file commits were regenerated, not merged.

## Summary

1. **Two source fixes, each proved with `tools/reshape_check.py`** (one
   commit per file):
   - the self-nested `namespace unported_ledger_stubs` in `entity.cpp` and
     `view.cpp`;
   - the dead `const` overload of `entity_action_kind_supported` in
     `entity.cpp`, renamed.
2. **A native-only bounds guard on the terrain SFX grid store** in
   `terrain_object_sfx_grid_config_load` (`0x0044acc0`), as the owner
   decided (#521, option 1). The matching build is unchanged: its token
   stream and object are identical, and the function keeps 80.00%.
3. **GAME-023 moves** to `src/game/legoobject.cpp` in one tool-generated
   commit: 251 functions, 116 owned globals and 112 closure items, from 14
   files. No fix-ups to the generated code were needed.
4. **Gate:** two drops and two rises, all compile drift in moved functions.
   **No exact match is lost.** Means: game-only 52.48 → 52.50%, full
   57.08 → 57.09%. One listing in an earlier unit file (GAME-049) changes at
   link time only.
5. **Layout:** adjacent function-order inversions fall from 326 to 231, and
   pairs from 77,707 to 52,084. Unit contiguity is unchanged at 89.04%
   (195 units); see "Layout" for why GAME-023 is still not contiguous.

## 1. Source fixes

All three are measured against their parent commit on #528 with
`python3 tools/reshape_check.py <file> --base <commit>~1 --head <commit>`.

| Commit | File | Native / matching / MinGW tokens | Matching object |
| --- | --- | --- | --- |
| Close the self-nested namespace | `entity.cpp` (inner block around `ledger_stub_00444460_FlaggedVectorStepCommandIssue`) | differ: the two deleted lines | every non-debug section's bytes, size and flags are identical. `.text` relocations by name and the symbol table differ for one name: `…::unported_ledger_stubs::unported_ledger_stubs::f` becomes `…::unported_ledger_stubs::f` |
| Close the self-nested namespace | `view.cpp` (inner block around `ledger_stub_004756d0_VideoObjectAspectRatioGet`) | differ: the two deleted lines | **identical**. The stub is `extern "C"`, so its symbol does not change |
| Rename the dead static to `entity_action_kind_supported_const_unused` | `entity.cpp` | differ: the identifier | **identical**. The static is never emitted |

- As finding 548 noted, none of these is token-identical. So none is a
  reshape in the narrow sense of the ADR 0006 addendum, rule 2. Each is a
  separate commit, and none changes code bytes.
- The rename moves one lint-ratchet entry to the new name, in the same
  commit. The ratchet's size does not change.
- With these fixes, `relocate_unit.py GAME-023 --dry-run` plans the move as
  finding 548 predicted, and no declaration split is needed.

## 2. The out-of-window store: a native-only guard

`terrain_object_sfx_grid_config_load` (`0x0044acc0`) computes
`cell = obj_y + obj_x * 15` and stores into the 300-entry
`s_terrain_sfx_grid[cell]` without a check, as the original does.
`test_level`'s data reaches `cell == -15`, a store 60 bytes before the grid.
- **Before the relocation**, the native layout put nothing that mattered
  there.
- **After the relocation**, the store lands on `s_pool_chunks[28]`.
  `pooled_record_storage_clear` frees that value, and the `level` test
  crashes (finding 548's hardware-watchpoint result).
  - Reproduced here: on the relocated tree, with the guard disabled
    (`#if 0`), native `level` fails with SEGFAULT.
  - With the guard, all 37 tests pass.

**The guard.** It is wrapped in `#if !(defined(_MSC_VER) && defined(_M_IX86))`,
the condition the codebase already uses to pick the matching path. One
example is `terrain_object_type_sfx_at_cell_get` (`0x0044ad80`), in the same
unit. An out-of-window cell is skipped with `continue`. Cells inside the
window are stored as before.
- The guard also applies to the MinGW reference build, which shares the
  non-MSVC branch.
- `test_level`'s data is unchanged, and the layout is not padded.
- The block carries the long comment the owner asked for. It covers:
  - that the unchecked store is the original's behaviour, kept on purpose;
  - why the matching build keeps it;
  - what the store hits after the relocation;
  - why the guard is native-only;
  - references to #521 and finding 548.

**Matching build: no drift.**
- `reshape_check` on `level.cpp`: the matching token stream is identical,
  and the matching object is identical (sections, relocations, symbol
  table). The native and MinGW streams differ by the guard, as intended.
- `make match-function ADDR=0x0044acc0`: 80.00%, baseline 80.00%. The store
  is still the original's blind `mov [eax*4 + s_terrain_sfx_grid], ecx`.
  The same score holds after the relocation, and in the full
  `make reccmp-check`.
- The guard was committed before the relocation, in `level.cpp`, and moved
  with its function verbatim.

## 3. The relocation

`python3 tools/relocate_unit.py GAME-023`. Stem `legoobject` (the default).

| Unit | File | Functions | Owned globals | Closure | From (functions, globals) |
| --- | --- | ---: | ---: | ---: | --- |
| GAME-023 | `game/legoobject.cpp` (LegoObject.c) | 251 | 116 | 112 | `entity.cpp` (176, 8), `ai.cpp` (28, 97), `spawn.cpp` (14, 2), `level.cpp` (12, 6), `cmd.cpp` (5), `terrain_grid.cpp` (3, 3), `gods98/view.cpp` (3), `effects.cpp` (2), `select.cpp` (2), `gods98/simtick.cpp` (2), `frontend.cpp` (1), `objective.cpp` (1), `gods98/camera.cpp` (1), `gods98/record.cpp` (1) |

- **Private headers written (16):**
  - `ai_for_`/`ai_from_legoobject.hpp`, `cmd_for_`, `effects_for_`,
    `entity_for_`/`entity_from_`, `frontend_for_`, `level_for_`/`level_from_`,
    `select_from_`, `spawn_for_`, `terrain_grid_for_`/`terrain_grid_from_`;
  - `gods98/camera_for_`, `simtick_for_` and `view_for_legoobject.hpp`.
- **Shared inline function:** `unported_ledger_stubs::entity_movement_scale_impl`
  goes to `entity_for_legoobject.hpp`, as finding 548's scratch run showed. It
  no longer needs its lint-ratchet entry, which is removed in its own commit
  (4,122 → 4,121 entries).
- **Statics made external:** 21 owned globals and 6 functions (`static`
  removed because remaining code still uses them), and 15 items that stay
  behind. All are recorded in `docs/coverage/relocations/GAME-023.csv`.
- **Link order:**
  - Five legacy files lose their last markers and move to the port tail:
    `effects.cpp`, `frontend.cpp`, `objective.cpp`, `terrain_grid.cpp` and
    `gods98/camera.cpp`. On `main`, only `terrain_grid.cpp` and `camera.cpp`
    retired, because #528 moves the other three files' remaining markers.
  - The other legacy files are re-keyed by their remaining code.
  - Entries: 103 → 99.
- **Native build:** compiles with no fix-ups. Both `cmake --build` and
  `ctest` pass, 37/37, including `level`.

## Gate (ADR 0006 rule 2)

| Check | Result |
| --- | --- |
| Native build + `ctest` | 37/37, including `level` |
| `make tools-test` | OK. Lint ratchet 4,122 → 4,121 |
| `link_order.py check`, Meson parity | OK, OK (in `tools-test`) |
| `make reccmp-check` | 2 drops, 2 rises, no exact match lost. The baseline is refreshed by `make match-progress` |
| Means | game-only 52.48 → 52.50% (2,166 mapped); full 57.08 → 57.09% (2,402) |
| Exact-function code bytes | 67,562, unchanged. Fully identical functions at original addresses: 8, unchanged |
| Function-order inversions | full: 326 → **231** adjacent, 77,707 → **52,084** pairs. Game-only: 265 → 166, 62,635 → 36,962 |
| Unit order and contiguity | unit-order inversions: 61 → 65 adjacent, 3,080 → 3,104 pairs. Contiguity: 89.04% (195 units), unchanged |
| Link-order entries | 103 → 99 |
| Identical bytes | `.text` at original addresses: 20,396 → 20,408. `.data`: 13,462 → 12,871. `.rdata`: 1,458 → 1,159 |
| `make unit-data-offsets` | full 0, gaps-only 2, partial 24 → 25, n/a 199 → 198. GAME-023 now has its own link-order object, so it leaves n/a and becomes partial |

The `binary-progress` step of `make match-progress` was run on the host
(`make binary-progress-compare`), because the distrobox lacks `pefile`. It
reads the same `build-reccmp/` build and report.

### Changed listings

All 8 changed listing digests (the same set in both scopes) are in functions
whose source is unchanged. Each was compiled as a single TU on #528's head
and on this branch (`tools/match_function.py`).

| Function | File (here) | Score | Instructions | Label | Single TU | What changed |
| --- | --- | ---: | ---: | --- | --- | --- |
| `0x00444680` entity_step_movement_vector_resolve | `legoobject.cpp` (moved) | 4.89 → **4.88** | 275 → 276 (+1) | `drift-review` | reproduces | one more `fxch`, x87 `fmul`/`fld` order; branch offsets shift; +2 bytes |
| `0x0044a070` entity_follow_target_pose_get | `legoobject.cpp` (moved) | 28.69 → **22.49** | 124 → 126 (+2) | `drift-review` | reproduces | x87 scheduling (`fld`/`fxch`/`faddp`); +12 bytes |
| `0x004382c0` pooled_record_allocate_and_reset | `legoobject.cpp` (moved) | 35.09 → 62.75 | 28 → 24 (−4) | `drift-review` | reproduces | register allocation (`edx` → `eax`); `push esi`/`pop esi` gone; −6 bytes |
| `0x004388b0` entity_action_kind_prerequisite_check | `legoobject.cpp` (moved) | 14.49 → 18.12 | 152 = | `drift-reordered` | reproduces | `div`/`neg`/`sbb` register choice |
| `0x00438770` building_upgrade_resource_admission_check | `legoobject.cpp` (moved) | 13.95 = | 116 = | `drift-reordered` | reproduces | register choice (`ecx`/`eax`); −1 byte |
| `0x00446610` ledger_stub_00446610_EntityMovementTickScaleAndDeltaGet | `legoobject.cpp` (moved) | 12.07 = | 193 = | `drift-review` | reproduces | x87 `fld`/`fmul` operands swap |
| `0x00447950` entity_directional_activity_start | `legoobject.cpp` (moved) | 33.87 = | 124 = | `drift-review` | reproduces | x87 `fxch`/`fdiv` order; +2 bytes |
| `0x0046db90` terrain_cell_quad_run_stage_level | `cell_flood_passes.cpp` (GAME-049, earlier unit; unchanged) | 11.11 = | 117 = | `drift-review` | **identical** | link-time only |

- **Drops.**
  - `0x0044a070` reverses the rise that finding 548 recorded (22.49 → 28.69,
    "restores the score it had before #516"). It is back at 22.49.
  - `0x00444680` is new on this base. It falls by 0.01 points, with one more
    instruction.
  - On `main`, the first measurement of this branch also dropped
    `0x00444fb0` (26.04 → 25.56). #528 already makes that change, so it is
    not a drop here: its single-TU listing is identical on both sides.
  - `0x004388b0` also returns to its pre-#517 score, 18.12. These functions'
    codegen follows the context of whichever translation unit holds them.
- **Earlier unit files.** No earlier relocated or converted unit file, and
  none of their private headers, is edited. That includes #528's four new
  units.
  - One listing changes at link time only: `0x0046db90` in GAME-049
    (`cell_flood_passes.cpp`). Its single-TU listing is identical on both
    sides, and its score does not change.
  - This is the same effect as `0x004078c0` in finding 548. Data moved, so
    the full-link listing labels an operand differently. The exact operand
    was not inspected here.
  - No other earlier unit's listing changed. The three GAME-005
    (`bubbles.cpp`) link-only changes seen on `main` do not occur on #528.
- **`0x0044acc0`** (the guarded function, now in `legoobject.cpp`): 80.00%
  with 60 instructions, and its single-TU listing is identical to #528's.
  Its baseline and listing digest are unchanged.

**Flagged for the owner (rule 2):**
- the two drops, `0x00444680` and `0x0044a070`;
- every instruction-count change: `0x00444680` (+1), `0x0044a070` (+2) and
  `0x004382c0` (−4).

## Layout

GAME-023 is still not contiguous. This comes from the rebuilt binary's
order, not from a gap in the move: every annotated function in the unit's
range is in `legoobject.cpp`, in address order.
- **Five callbacks outside the run.** Five of the unit's annotated
  callbacks sit in address order in `legoobject.cpp`, but the rebuilt
  binary maps them well after the rest of the unit, among later units.
  Why the linker places them there was not investigated:
  - `entity_linked_route_clear_callback` (`0x00437510`);
  - `entity_route_marker_candidate_visit` (`0x00438fb0`);
  - `spatial_entity_damage_radius_callback` (`0x00440800`);
  - `spatial_entity_vector_step_callback` (`0x00440920`);
  - `selection_candidate_filter_pool_refresh_callback` (`0x0044c500`).

  `translation-units.csv` does not list them, so the contiguity metric
  counts them as foreign functions that split the unit's run.
- **One swapped pair.** `activity_is_special` (`0x0044b350`, `static`) is
  emitted after `ai_activity_grant` (`0x0044b370`).
- **Not in `src/`.** Twelve of GAME-023's 258 original functions have no
  source yet, for example `EntityRename` and `EntityCreateByType`.
