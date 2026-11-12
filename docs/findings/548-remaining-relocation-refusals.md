# Finding 548 — The remaining relocation refusals: what the tool now handles, and the source fixes left

Issue #514, part of #388, following #485, #496 and #501. It covers each
refusal type the survey still reported on `main`:
- name clashes;
- a nested namespace with a shared `inline`;
- out-of-line class members;
- anonymous-namespace items, including the two that blocked GAME-021 after
  its declaration split (PR #515, finding 546);
- G98-009's conditional COM helpers.

Measured on #518's head `d4fa60af`: `main` `02ee3fcf` plus #515 (GAME-032)
and the GAME-001 and GAME-004 conversions (#519, #518). This change is stacked
on #518, and every figure below is its delta on top of #518.

## Summary

| Refusal | Units | Answer |
| --- | --- | --- |
| name clash with another file | GAME-002, GAME-027 | **source fix** (rename a static). The tool now reads both definitions' annotations and says which fix applies |
| nested namespace, plus a shared `inline` | GAME-023 | **tool**: nested named namespaces and shared unannotated `inline` functions. **Source fixes** for GAME-023 itself: its namespace is nested *in itself*, and two more problems sit behind it |
| out-of-line class members | G98-012 | **tool**. G98-012 stays blocked by a 1,400-line backend region (a P5-style reshape) |
| anonymous-namespace items used before their definition | GAME-050; GAME-021 after its split (#515) | **tool**: forward declarations inside an anonymous namespace (`namespace lego { namespace { … } }`). **GAME-050 and GAME-021 relocated** |
| conditional group of `static` COM helpers | G98-009 | **tool**: whole groups may hold unannotated helper functions. **G98-009 relocated**, after its tool-generated declaration split |

- **Survey on #518** (15 units left): 0 → 1 units relocate as they are
  (GAME-050), plus G98-009 and GAME-021 after `--split-declarations`. Every
  other refusal type in the issue now names the source fix it needs.
- **The survey's blind spot is fixed.** A unit that needs
  `--split-declarations` is now "ok after" only if its relocation plans on
  the split text, in a scratch copy. Otherwise it is "refused after", with
  the reasons. With `main`'s tool, this check reports GAME-021 as refused
  after the split, which is what #515 found by hand.
- **Gate:** the three relocations together change 18 listings. 17 are
  compile drift (each reproduces in a single-TU compile). The 18th,
  `0x004078c0` in the earlier unit file `bubbles.cpp`, is identical as a
  single TU and keeps its score: only a link-time operand label changed.
  Four scores fall and eight rise, two of them with unchanged listings. No
  exact match is lost. Means: game-only 52.519 → 52.525%, full 57.114 →
  57.119%.
- **GAME-023 was not feasible.** With its two code fixes below applied in a
  scratch tree, it relocates (251 functions, 116 owned globals) and builds.
  But the native `level` test then crashes on an out-of-bounds store that
  the port makes on purpose (below).

## What the tool now does

All are covered by fixture tests in `tools/tests/test_relocate_unit.py`
(`RemainingRefusalsTest` and the updated refusal tests).

1. **Nested named namespaces.**
   - `namespace A { namespace B { } }` is scope `A::B`. The unit file, the
     private headers and forward declarations write nested blocks: MSVC 5.0
     has no `namespace A::B {`.
   - Name lookup walks the enclosing namespaces: `B`, then `A`, then global.
   - A linkage block inside a namespace still refuses.
   - So does a **namespace nested in itself** (`namespace X { namespace X { } }`),
     which is what the two real cases are. Inside the outer `X`, once the inner
     one is declared, a qualified `X::name` names the inner namespace. Lookups
     therefore depend on position, and a relocation reorders items.
     GAME-023's scratch build showed it: five calls in the unit file stopped
     resolving.
2. **Shared `inline` functions.**
   - An unannotated inline function that both files use is header-safe. It
     goes verbatim to the `for` header, after the header's declarations.
     Every file that uses an inline function must see its definition anyway.
   - An annotated inline function used by both files refuses. This includes a
     non-`static` one, which previously got a declaration instead: a
     declaration is not enough for an inline function.
   - GAME-023's scratch relocation moved `entity_movement_scale_impl` into
     `entity_for_legoobject.hpp`; `entity.cpp` and the unit both compiled.
3. **Out-of-line class members.**
   - `T C::f() { }` is the symbol `C::f`, never the free function `f`, and
     its class `C` is one of its uses.
   - The class definition follows the members like a type: into the unit
     file, or into the `for` header when the legacy file still uses it.
   - Members get no declarations of their own, because their class declares
     them.
4. **Anonymous-namespace functions used before their definition** get a
   forward declaration inside an anonymous namespace, for example
   `namespace lego { namespace { int ascii_case_equal(const char* a, const char* b); } }`.
   An unnamed namespace is the same one throughout a translation unit, so the
   later definition matches. This was GAME-021's blocker after its split
   (#515): `lego::prefix_equal` and `lego::upgrade_type_tables_release` from
   `upgrades.cpp`. A fixture covers that shape.
5. **Helper functions in whole `#if` groups.**
   - A whole group (#501) may now hold *unannotated* function definitions,
     such as G98-009's four `static` COM-call helpers, with `__asm` in one
     branch and C plus `typedef`s in the other.
   - Like header-safe items, the helpers count as one cluster with the group.
   - A group with functions lands in the unit file before the unit's
     functions, as it sat before its users in the source, so no forward
     declarations are needed.
   - Shared, the group refuses if a later branch defines a shared helper (as
     guarded declarations require, #501). An annotated function in a group
     still refuses, naming the group.
6. **The survey looks behind a split.** It applies a unit's declaration
   splits in a scratch copy of `src/` and `docs/coverage/`, then plans the
   relocation there. The status is "ok after `--split-declarations`" or
   "refused after `--split-declarations`" with the reasons; the real tree is
   never written. G98-009 (#508) and GAME-021 (#515) were both reported "ok
   after" but refused once split. A fixture covers this.
7. **Name clashes.** The refusal reads both definitions' annotations, as in
   #482:

   | Annotations | Advice |
   | --- | --- |
   | the same address | one original global defined twice: **unify** them |
   | different addresses | two original globals: **rename the static** |
   | only one annotated | the other is port-only: **rename it** |

   Either way it is a source fix, not a relocation. The survey labels each
   case with its fix.

## Source fixes, for follow-up tickets

Each was measured with `tools/reshape_check.py` against `HEAD`:
- the dead-static rename leaves the object identical;
- the namespace fix and the `cmd.cpp` rename leave the code identical and
  change only symbol names;
- the `ai.cpp` rename changes `.bss`.

None is token-identical, because each changes an identifier. So none is a
reshape under the ADR 0006 addendum, whose allowed renames are of file-local
macros only. Each is a **fix commit** under rule 3 with its own gate. The
evidence below shows how small each one is.

| Unit | Fix | `reshape_check` object result |
| --- | --- | --- |
| GAME-023, G98-002 | Delete the inner `namespace unported_ledger_stubs {` and its `}` in `entity.cpp` (lines 14575 and 14675) and `view.cpp` (5723 and 5729). Each is a block pasted inside an already-open one. Only the moved stub's mangled name changes (`…::unported_ledger_stubs::unported_ledger_stubs::f` → `…::unported_ledger_stubs::f`), and every qualified call to it still resolves | code identical; `.text` relocations by name (`entity.cpp`) and the symbol table differ, for that one name |
| GAME-023 | Rename `entity.cpp:7174`'s port-only `static int entity_action_kind_supported(const EntitySlot*, …)`. It is dead code: both of `entity.cpp`'s calls pass a non-`const` `EntitySlot*`, so overload resolution picks `cmd.cpp`'s external `0x0043a7c0` (declared in `cmd.hpp`). The tool resolves names without overload resolution, sees a second definition, and refuses: the two would meet in the unit file | **identical** (the static is never emitted) |
| GAME-002 | Rename `ai.cpp`'s `static int s_active_count` (`0x004b3424`); `sfx.cpp` has `int s_active_count` (`0x00502bd4`), which frozen private headers already name | `.bss` bytes, size and flags differ: renaming a static datum **reorders MSVC 5.0's `.bss`**. Relocations and the symbol table differ by name. This needs a full gate |
| GAME-027 | Rename `cmd.cpp`'s `static int s_queue_count[2]` (`0x004fef20`); `info.cpp` has `0x004dcd38` | relocations and the symbol table differ by name only |
| GAME-023 | `terrain_object_sfx_grid_config_load` (`0x0044acc0`) "stores blind past the 300-entry window", on purpose. `test_level`'s data reaches `cell = −15`, which writes 60 bytes before `s_terrain_sfx_grid`. On `main` that lands harmlessly; with GAME-023 relocated, it lands on `s_pool_chunks[28]`, and `pooled_record_storage_clear` then frees the value (found with a hardware watchpoint). **An owner decision:** guard the port path, or change the test data | — |
| G98-012 | `movie.cpp:92`–`1469` is one backend region of three branches (FFmpeg, VFW, movies disabled), with an `#undef` and alternative definitions of annotated functions. This is the plan's P5 shape: a region split (B3-style), designed against this tool | not measured |

With GAME-023's two code fixes (the namespace and the dead static) applied
in a scratch tree, GAME-023 plans with 251 functions, 116 owned globals and
112 closure items, and the native build compiles. Only the `level` test
fails, on the blind store.

## Survey before and after

`python3 tools/relocate_unit.py --survey` on #518's head `d4fa60af` (15
units left), with the base's tool and with this one. Only changed rows are shown:

| Unit | Before | After |
| --- | --- | --- |
| GAME-002 | name clash with another file | name clash: two original globals (rename, source fix) |
| GAME-023 | nested namespace ×1, shared inline ×1 | namespace nested in itself (source fix) |
| GAME-027 | name clash with another file | name clash: two original globals (rename, source fix) |
| GAME-050 | anonymous namespace shared ×2 | **ok** (45 functions), relocated here |
| G98-002 | `#if` ×5, nested namespace ×1 | namespace nested in itself (source fix); its `#if` groups now merge |
| G98-009 | needs `--split-declarations` (and, behind it, the COM-helper group) | **ok after `--split-declarations`**, relocated here |
| GAME-021 | "ok after `--split-declarations`", wrongly (the old survey did not look behind the split) | ok after `--split-declarations`, relocated here. `main`'s tool with only the survey fix reports it as "refused after `--split-declarations`": anonymous namespace ×2 |
| G98-012 | `#if` ×10, out-of-line class member ×11 | `#if` ×10 (the backend region) |

Totals: 0 → 1 units relocate as they are, plus G98-009 and GAME-021 after
`--split-declarations`. Blocked by an `#if` block: 8 → 7.

## Proof relocations

Three units, committed in address order, each after its own split commit.
GAME-023 was not feasible (above).

### GAME-021 (Lego.c) → `game/lego.cpp`

- First, `relocate_unit.py GAME-021 --split-declarations --check-layout`,
  committed on its own. It splits `simtick.cpp`'s
  `static int s_minion_actions = 0, s_minion_fails = 0;`, and the matching
  object's data layout is unchanged (213 data symbols, identical section
  sizes).
- Then `relocate_unit.py GAME-021`: 197 functions, 12 owned globals and 128
  closure items from `crt.cpp`, `ai.cpp`, `cmd.cpp`, `entity.cpp`,
  `entity_config.cpp`, `frontend.cpp` and others.
- The two helpers that blocked #515 get
  `namespace lego { namespace { int prefix_equal(const char* value, const char* prefix); } }`
  and the same for `upgrade_type_tables_release`.
- Five legacy files lose their last markers and become port-tail entries:
  `entity_config.cpp`, `scene_render.cpp`, `engine.cpp`, `mainsim.cpp` and,
  with GAME-050, `weapon.cpp`.

### GAME-050 → `game/weapons.cpp`

- `relocate_unit.py GAME-050 --stem weapons`: 45 functions, 1 owned global
  and 13 closure items from `cmd.cpp`, `effects.cpp`, `entity.cpp`,
  `weapon.cpp`, `gods98/record.cpp` and `gods98/simtick.cpp`.
- Private headers: `{cmd,effects,entity,weapon}_for_weapons.hpp`,
  `simtick_for_weapons.hpp`, `cmd_from_weapons.hpp` and
  `weapon_from_weapons.hpp`.
- The new forward declarations:
  `namespace lego { namespace { void weapon_definition_reset(WeaponDefinition* definition); } }`
  and the same for `ascii_case_equal`.

### G98-009 → `gods98/sound3d.cpp`

- First, `relocate_unit.py G98-009 --stem sound3d --split-declarations
  --check-layout`, committed on its own. It splits `simtick.cpp`'s
  `static int s_sound_on = 0, s_sound_active = 0;`, and the matching
  object's data layout is unchanged (203 data symbols after GAME-021,
  identical section sizes).
- Then the relocation: 38 functions, 7 owned globals and 16 closure items
  from `crt.cpp`, `audio.cpp`, `record.cpp`, `resource.cpp`, `simtick.cpp`,
  `sound.cpp` and `view.cpp`.
- `sound.cpp`'s COM-helper group moves whole, with its comment, and lands
  before the unit's first function.

### Gate (ADR 0006 rule 2)

| Check | Result |
| --- | --- |
| Native build + `ctest` | 37/37 |
| `make tools-test` | OK. The lint ratchet shrinks 4,123 → 4,122: `entity_minifigure_type_table_load` no longer needs its entry after it moved with GAME-021 |
| `link_order.py check` / `check-map`, Meson parity | OK / OK / OK |
| `make reccmp-check` (distrobox) | 4 falls, 8 rises, no exact match lost; the baseline is refreshed under rule 2 |
| Means | game-only 52.519 → 52.525% (2,164); full 57.114 → 57.119% (2,400) |
| `make match-progress` | function-order inversions: 536 → **430** adjacent, 210,224 → **165,240** pairs. Unit contiguity: 87.67 → 88.13% (192 → 193 units). Unit-order inversion pairs: 3,128 → **3,155** (+27). Link-order entries: 109 → 107 (3 units added, 5 legacy files retired to the tail). Exact-function code bytes: 67,562, unchanged. Identical `.text` at original addresses: 20,802 → 20,551 bytes; identical `.data`: 11,302 → 10,791 bytes |
| `make unit-data-offsets` | full 0, gaps-only 2, partial 20, n/a 203 |

Every function whose listing digest changed is labelled by
`match_listings.classify`. All of them were compiled as single TUs on #518
and on this branch. Each of the 17 compile-drift changes reproduces there;
`0x004078c0` does not, because only its link-time label changed.

| Function | File (here) | Score | Size (bytes) | Label | What changed |
| --- | --- | ---: | ---: | --- | --- |
| `0x004388b0` entity_action_kind_prerequisite_check | `entity.cpp` | 18.12 → **14.49** | = | `drift-reordered` | integer `mul`/`div`/`neg` register choice |
| `0x00444460` ledger_stub_00444460_FlaggedVectorStepCommandIssue | `entity.cpp` | 23.61 → **22.72** | = | `drift-review` | x87 `fld`/`fmul` operands swap |
| `0x00471370` cmd_place_ray_candidate_check | `weapons.cpp` (moved) | 15.85 → **15.76** | +2 (+1 instruction) | `source-changed` | the recorded `static` removal (shared with `cmd.cpp`); one more `fxch` |
| `0x0047bdb0` sound_stream_ring_buffer_tick | `sound3d.cpp` (moved) | 10.81 → **8.11** | = | `drift-reordered` | register choice re-rolled (`mov`/`xor`/`setne`) |
| `0x0042d600` entity_object_names_load | `lego.cpp` (moved) | 5.86 → 5.97 | −3 (−2) | `drift-review` | register allocation |
| `0x0042da20` entity_object_the_names_load | `lego.cpp` (moved) | 5.86 → 5.97 | −3 (−2) | `drift-review` | register allocation (same shape) |
| `0x00444fb0` ledger_stub_00444fb0_EntityTurnToVectorActivityIssue | `entity.cpp` | 25.56 → 26.04 | +2 (+1) | `drift-review` | one `fxch` |
| `0x0044a070` entity_follow_target_pose_get | `entity.cpp` | 22.49 → 28.69 | −12 (−2) | `drift-review` | x87 scheduling. This restores the score it had before #516 |
| `0x0047ba20` sound_stream_stop_and_release | `sound3d.cpp` (moved) | 8.70 → 9.38 | −4 (−3) | `drift-review` | register allocation |
| `0x00483420` screen_effect_item_opacity_set_target | `record.cpp` | 30.51 → 36.16 | = | `drift-reordered` | register choice (`mov`/`shl`) |
| `0x00411360` menu_mouse_coordinate_update | `frontend.cpp` | 21.12 = | = | `drift-reordered` | argument pushes reordered |
| `0x0041c1c0` advisor_overlay_hit_resolve | `frontend.cpp` | 11.49 = | = | `drift-review` | x87 `fadd` operands |
| `0x00435230` random_variant_cell_scatter | `lego.cpp` (moved) | 33.78 = | = | `drift-reordered` | register choice |
| `0x00446610` ledger_stub_00446610_EntityMovementTickScaleAndDeltaGet | `entity.cpp` | 12.07 = | = | `drift-review` | x87 `fld`/`fmul` operands swap |
| `0x00447950` entity_directional_activity_start | `entity.cpp` | 33.87 = | −2 (0) | `drift-review` | x87 scheduling |
| `0x0046ffb0` ledger_stub_0046ffb0_LaserSpatialSoundAndImpact | `weapons.cpp` (moved) | 10.47 = | = | `drift-review` | x87 `fld`/`fmul` operands swap |
| `0x0047bc30` sound_stream_drain_check | `sound3d.cpp` (moved) | 12.77 = | = | `drift-reordered` | register choice |
| `0x004078c0` ledger_stub_004078c0_EntityCursorTargetStep | `bubbles.cpp` (an earlier unit; unchanged) | 10.26 = | = | `drift-review` | link only: the single-TU listing is identical. Reccmp labels an operand differently after the data moved, as in this same function during #496's replay |

Two more functions rise with unchanged listing digests:
- `0x0041fa80` ledger_stub_0041fa80_StartLevelPilots (moved with GAME-021,
  `spawn.cpp` → `lego.cpp`): 54.41 → 55.88. Its listing is unchanged (75
  instructions), so only operand addresses differ after the move.
- `0x00482620` mesh_segment_vertex_fill (`record.cpp`): 26.09 → 31.30, a
  register re-roll that the digest's normalisation hides. It reproduces in a
  single-TU compile.

No other earlier unit file (relocated or converted) changes. This PR edits
none of them or their private headers; it adds only the three new unit
files.

**Flagged for the owner (rule 2):**
- the instruction-count changes: `0x00471370` (+1), `0x00444fb0` (+1),
  `0x0042d600` (−2), `0x0042da20` (−2), `0x0044a070` (−2) and
  `0x0047ba20` (−3);
- all four falls.

## Limits

- **Overload resolution.** The tool resolves a name to the file's own
  definition; it does not do overload resolution. GAME-023's dead static
  shows the cost: the refusal is right, but the cause is a function nobody
  calls.
- **Nested namespaces in practice.** The only real nested namespaces are
  self-nested, and they refuse. The general support has fixtures but no
  relocation yet.
- **Class members in practice.** Out-of-line class members have no proof
  relocation either. G98-012, their only user, waits on its backend region.
