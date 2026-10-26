# Reccmp function annotation handoff

## Objective

Backfill `FUNCTION` / `STUB` address markers in production code when the source
tree contains defensible evidence that a ported function corresponds to a
function in the original `LegoRR.exe`. These markers let Reccmp compare the
original function at its known address with the rebuilt function.

This work is address attribution, not a requirement to label every C/C++
definition. A missing marker is preferable to an invented or ambiguous
address. Continue until the available source, decompilation notes, symbols,
and build artifacts have been audited for reliable mappings.

## Annotation format

Place the marker immediately before the function definition:

```cpp
/* ExampleFunction (FUN_00401234): documents the function's behavior. */
// FUNCTION: LEGORR 0x00401234
int example_function(int value)
{
    ...
}
```

Use `FUNCTION` for an implementation and `STUB` for an intentionally stubbed
implementation when its original address is known. The marker target name is
`LEGORR`, matching [`reccmp-project.yml`](../reccmp-project.yml).

## Evidence rules

Prefer, in order:

1. A function-specific comment immediately adjacent to the definition that
   names the exact `FUN_XXXXXXXX` address.
2. An explicit function-name/address pairing elsewhere in the same source file
   that unambiguously names this definition.
3. Authoritative symbol, decompiler, or Reccmp evidence that directly links the
   implementation and original address.

Before adding a marker, inspect the function body and its surrounding comments.
Do not use an address just because it appears nearby: comments may describe a
callee, a shared helper, a thunk, a related code path, or a subsystem section.
Do not assign the same address to multiple definitions. Some port helpers
intentionally model only a branch of a larger original function; mark those
only when the project’s evidence explicitly treats that implementation as the
corresponding Reccmp function.

Do not add markers to functions explicitly described as unported hooks or
unowned state merely because their comments mention an original function.
Keep intentionally unimplemented areas out of Reccmp’s comparison set unless
the existing source annotations establish a deliberate `STUB` mapping.

## Working procedure

1. Inspect the active branch, worktree, and existing edits. Preserve all
   uncommitted work; do not reset the tree or switch branches.
2. Search for candidate evidence, for example:

   ```sh
   rg -n 'FUN_[0-9a-fA-F]{6,8}|FUNCTION: LEGORR|STUB: LEGORR' src
   ```

3. For each candidate, identify the exact function definition and validate
   that the address describes that function rather than a call target or
   neighboring function.
4. Check that the address is not already annotated elsewhere under `src/`.
   Record only unambiguous mappings and keep the marker directly above the
   definition.
5. Review the diff, then run the focused checks below. Do not change function
   behavior as part of annotation-only work.

## Checks and Reccmp report

The project linter is [`tools/lint_reccmp_annotations.py`](../tools/lint_reccmp_annotations.py)
and can be run with:

```sh
make reccmp-lint
# or
python3 tools/lint_reccmp_annotations.py
```

The linter is a source-level heuristic. It reports definitions lacking
markers; it does not determine whether a valid original address exists and it
does not validate address correctness or uniqueness. A nonzero exit is
therefore expected while source functions without reliable address evidence
remain. Review its output rather than filling every reported line with a
guess.

Check formatting and address uniqueness independently:

```sh
git diff --check
```

The uniqueness audit should scan `FUNCTION` and `STUB` markers across all
production translation units in `src/`, normalize hexadecimal case and
leading zeros, and report every address with more than one definition.

To rebuild and refresh the report when Reccmp, Wine, and the configured MSVC
5.0 toolchain are available:

```sh
make reccmp RECCMP_WINE_BIN=/path/to/wine \
  RECCMP_RECCMP=/path/to/reccmp-reccmp
```

The report is written to `build-reccmp/reccmp-report/index.html` with JSON at
`build-reccmp/reccmp-report/results.json`. Reports already in the worktree may
predate annotation edits; rerun the command before treating them as evidence
for the current markers. The wrapper configures and builds the project, then
rewrites the generated local `reccmp-build.yml`.

## Current handoff state

- Active project worktree: `/home/johnrowl/.codex/worktrees/c71a/lrr-portable`.
- Current branch at handoff: `integration/reprobit`.
- The tree contains substantial pre-existing user changes across source,
  CMake, Makefile, README, Reccmp setup, and the annotation linter. Preserve
  them; inspect `git status --short` before editing.
- Current source audit found **1,044 `FUNCTION`/`STUB` annotations at 1,044 unique
  addresses**. The duplicate-address audit found no collisions.
- `git diff --check` passed at the last check.
- `python3 tools/lint_reccmp_annotations.py` last reported **981** functions
  without markers. This is not proof that 1,534 mappings remain available;
  many functions have no trustworthy original address, and the linter does
  not evaluate address evidence. `docs/coverage/ledger.csv` currently has
  **87** `ported` rows whose entry addresses lack markers. Per its README,
  `ported` means that the address or `FUN_*` is cited somewhere in `src/`;
  some citations are call sites, address ranges, or TODO hooks rather than a
  matching implementation boundary, so each still needs source/decompile
  review before annotation.
- No build or tests were run as part of the current annotation pass. The
  existing `build-reccmp/reccmp-report` may be stale relative to source.

### Candidates to resolve next

- The latest pass added exact-address readers from Finding 457's entity stats
  record family, plus the three documented stats-word bit tests. Their header
  prototypes supply the address/offset pairs and each body reads that exact
  offset through the shared record helper. `entity_stats_float_at24_indexed`
  remains unannotated because `ai_stats_speed` already owns `0x00469cc0` and
  the address audit forbids mapping both source implementations to that entry.
- `simtick.cpp` now marks the five tick functions whose header and Finding 03
  identify exact function boundaries: the table dispatcher, post-step
  cluster, mute timer, bobbin walk, and terrain marker sweep. Their helper
  callbacks remain separate stubs or TODO seams.
- The same pass marked the directly described `ConfigBooleanParse`, the two
  C-locale string case converters, and the camera-record accessors and X
  rotation matrix leaf. Their adjacent comments or header declarations name
  the exact addresses; null guards are documented port deviations.
- Additional exact declaration/body pairs were marked for the entity pointer
  registry operations and random activity attempt, camera bounds/distance
  operations, grid-cell buffer lifecycle, retry-enabled allocation, scroll
  speed mapping, and save-slot accessors. Their headers state the original
  addresses and behavior; marker placement and address uniqueness were
  rechecked.
- The entity activity band now also marks the exact record-flag operations,
  nested-float update, linked-entity update, and timer-registry operations
  whose header declarations and implementation comments identify one
  original entry point.
- The opaque-record reader and geometry leaves in `gods98/record.cpp` were
  backfilled from the matching record-header function/address descriptions;
  each implementation accesses the documented field or data window. The
  faithful UInt96 zero helper and pure-return CRT stub variant were also
  mapped from their exact function comments.
- The save-slot clear, five generic list operations, checkpoint progress
  fold, and reward-zone/button handlers now carry the addresses documented in
  their API comments. Their corresponding bodies operate on the described
  save or list records, with the documented port context and pointer guards.
- `fe_parse_menu` maps to `MenuBuild` (`0x004140c0`): it reads one menu
  group’s submenu count and constructs each submenu/item model through the
  parser helpers. `frontend_build` composes several such menu builds. The port
  writes into a pre-existing `FrontEnd` and returns a boolean rather than the
  corpus’s allocated-menu pointer; parser bounds and omitted render resources
  remain documented reconstruction differences.
- `fe_item_kind` maps to `MenuItemType` (`0x00413e60`): its five explicit
  item kinds and catch-all value correspond to the decompiled selector. The
  port maps unrecognized tokens to `Next` instead of the corpus’s `-1`, a
  deliberate parser tolerance that remains visible in the comparison.
- `sound_bank_entry_volume` maps to `IndexedRecordStride134WordGet`
  (`0x0047b0d0`): its indexed `BankSlot.volume` field represents corpus
  record offset `+0x110` with record stride `0x134`. The slot range guard is a
  documented port safety deviation.
- `texture_cache_name_slot_append` maps to `TextureCacheInsert`
  (`0x00483080`): the body duplicates the key, writes the three cache-entry
  slots at `count * 3 * sizeof(void*)`, and increments the count exactly as
  the decompile does. The widened native pointer stride and defensive guards
  are documented in `texture.hpp`.
- The objective load, checker, announcer, per-frame state machine, and
  committed voice-alias action are mapped from the exact addresses identified
  in `objective.hpp` and its module comment.
- The FLIC resource opener is mapped to `0x00483c80` from the documented API
  identity in `flic.hpp`; the remaining decoder/parser logic is part of the
  port's implementation of that opener.
- `flic_tick_loop` maps to the in-memory `FlicRamFrameAdvance` stepper
  (`0x004841d0`): it advances the current chunk index, decodes the next frame,
  and applies the corpus loop-to-frame-1 rule. The separate
  `FlicFrameAdvanceAndBlit` entry also manages DirectDraw presentation and is
  not assigned to this decoder function.
- A follow-up review found no safe direct mapping for `MenuScrollStateUpdate`
  (`0x00413970`): its pan arithmetic is embedded in `frontend_tick`, and the
  port intentionally uses different velocity and viewport rules. The entity
  pool callback walker (`0x004377d0`) remains an explicit unported seam; the
  scene action-name setter (`0x00473370`) is outside the implemented scene
  assembly pipeline. Do not mark the enclosing frontend tick or visibility
  hook as those original entry points.
- The high-citation `EntityBuildingIndexSet` (`0x00469c10`) requires the
  corpus per-type/per-row stats arrays and updates old/new index counters; the
  port has no compatible stats-record owner. `CurrentLevelHeightGet`
  (`0x00431570`) remains unmatched: despite the corpus name, it returns the
  current level block's `+4` field (the terrain handle in the recovered level
  layout), while the port's `level_height` reads `+0x18`. No matching getter
  for the original `+4` field has been identified.
  `GameConfigInit` (`0x0041fb50`) is a broad loader whose type-table and
  subsystem initialization paths are still absent or split across startup;
  none currently has a single corresponding source function to mark.
- An adjacency audit moved 18 existing markers out of explanatory comments or
  ahead of them so every marker now sits directly above its implementation.
- The post-step and FX-bank ticker implementations are now explicit `STUB`
  mappings (`0x004665c0` and `0x0046e390`). Their local comments identify
  those exact original entries, and their current bodies are placeholder
  behavior rather than faithful implementations. The telemetry accessor that
  shares the post-step comment remains unmapped.
- Six additional simulation ticker bodies carry their own exact implementation
  addresses (`0x004313b0`, `0x00424600`, `0x0043b270`, `0x0046f550`,
  `0x00425d90`, and `0x0040eab0`). The former `0x00402220` marker was removed:
  the full decompile identifies that address as `CmdTickGlobal`, whose routine
  is split between `simtick_frame_counter` and the scoped command-list cleanup;
  neither source function alone is a defensible one-to-one mapping.
- `block_find` is now mapped to `0x004792e0`: `block.hpp` identifies this as
  `BlockFindPath`, the function-specific path-lookup section describes the
  same entry, and the full decompile's function heading confirms
  `ConfigBlockPathFind @ 004792E0`. The port preserves its path traversal and
  first-token wildcard semantics in a tree representation.
- The Reward Text loader is mapped to `0x0045f530`. Its field, offset, and
  flag effects match the full decompile's `TaskBoxRect` body, including the
  string copy, underscore replacement, coordinates, and `0x31` flag mask.
- The block-text API now maps `block_open` (`0x00478e60`), `block_find`
  (`0x004792e0`), `block_subgroup` (`0x00478ff0`), and `block_value`
  (`0x004790b0`). These pairings are enumerated in `block.hpp`; the matching
  full-decompile headings and bodies confirm the open/scan, path traversal,
  child-gate, and value-field roles. `block_get_strptr` was not assigned the
  `0x00479050` address: that original allocates a copy, while the port returns
  a borrowed pointer.
- Four fixed-return CRT leaves now map to the addresses confirmed by their
  full-decompile headings: `constant_zero_float_get` (`0x00408930`),
  `constant_negative_10000_get` (`0x0047c160`),
  `processor_feature_fallback_false` (`0x00490230`), and
  `flh_magic_uu_noop` (`0x00456210`). These are faithful leaf behaviors, not
  unresolved global-state shims.
- The faithful pure-return `crt_no_op_stub` is explicitly marked as a `STUB`
  at `0x00491ff5`, and `byte_range_swap` maps to `0x0048e030` from its exact
  API comment and byte-swap implementation. Its same-pointer shortcut and
  NULL guard are documented port behavior.
- The heap allocation chain now maps the eight directly identified entry
  points: aligned/retry/plain allocation (`0x0048dc40`, `0x0048dbf0`,
  `0x0048dbd0`), free (`0x0048db80`), its engine/UI wrappers (`0x0047ff40`,
  `0x0047ff30`, `0x0045daf0`), and allocation-retry handler dispatch
  (`0x00491d90`). Their API comments specify those addresses and the bodies
  implement the documented call chain.
- The selection module's six exact accessors/operations are mapped from
  `select.hpp` and their matching bodies: count (`0x004525f0`), nonempty
  predicate (`0x004525b0`), array getter (`0x004525e0`), count reset
  (`0x00452600`), first selected entity (`0x004525c0`), and membership scan
  (`0x00452610`). The related `selection_add` remains unmapped: its comment
  calls `0x00452610` an analog, and that address belongs to the separate
  membership function. The candidate-already-selected leaf is also mapped to
  its separate address `0x00452690`; the eligibility leaf already maps at
  `0x00452870` in the entity module.
- Three field-grid entity operations now carry their exact `engine.hpp`
  addresses: set the entity in a cell (`0x00466380`), invalidate it and clear
  flags (`0x00466750`), and test whether a cell permits spawning
  (`0x004663f0`). The implementation comments and field accesses match the
  corresponding declarations.
- Three activity functions now carry their exact `ai.hpp` identities:
  setting an entity activity (`0x004409e0`), granting an activity
  (`0x0044b370`), and updating activity scene state (`0x00439990`). Their
  implementation bodies match the documented record/global effects; the
  other nearby address, `0x00437d00`, describes the larger entity creation
  dispatcher and was not assigned to its minion-state selector helper.
- Two engine-state operations are mapped to their function-specific entries:
  clear the low state byte and refresh visibility (`0x00435a20`), and apply
  the global position handle when present (`0x00435580`). Although the
  backing globals have no ported owner, these functions themselves implement
  the documented operations; the standalone state accessors remain hooks.
- `media_cd_audio_playback_available` now maps to `0x00488b90`. The full
  decompile and Finding 406 identify the exact two-global availability test;
  the port keeps the CD-device input behind a test seam because its original
  MCI opener is still unported.
- The InfoMessages config loader now maps to `0x00434c10`. Its API comment
  names `IncidentMessageConfigLoad`, and the full decompile confirms this
  function opens the InfoMessages section and populates the per-id message
  settings modeled by the port.
- `input_key_context` now maps to `0x00411f00`; `input.hpp` names
  `KeyContextCheck` and describes its direct filter lookup, and the port body
  performs that lookup with the documented bounds guard.
- The LWO model pipeline now maps `model_open` to `ModelObjectCreate`
  (`0x0048d7c0`) and `model_parse` to `LoadLWO` (`0x0048d2c0`). The three
  source chunk handlers map to `LwoPolygons` (`0x0048c3e0`),
  `LwoSurfaceNames` (`0x0048c690`), and `LwoSurfaceSettings` (`0x0048c820`);
  the header's handler list and full-decompile function headings identify
  each boundary. No separate PNTS handler exists in the port, so
  `LwoPoints` remains unmapped.
- `sound_alias_registration_set` now maps to `SoundDynamicAliasRegistrationSet`
  (`0x00464c50`); the full decompile and API notes identify the flag-bit
  setter, and the implementation sets or clears that exact bit.
- `rotated_pairs_scratch_fill` maps to `RotatedPairsScratchFill`
  (`0x00464110`); its header, Finding 253, and decompile agree on the pair
  rotation/translation contract. The marker was also moved from the unrelated
  `simtick_scene_transform_config_peek` getter to
  `simtick_release_all_group_records`, whose declaration and decompile identify
  `ReleaseAllGroupRecords` at `0x004099d0`.
- The CRT math-error slot setter and signed decimal formatter now map to
  `CrtMathErrorCodeSet` (`0x004939a0`) and `SignedDecimalToStringConvert`
  (`0x0049db60`). Their API comments give exact names and addresses, and the
  full-decompile bodies match the setter and negative-decimal dispatch. The
  signed formatter's marker had been misplaced inside the preceding unsigned
  formatter; it now sits directly above the correct definition.
- The four input flag-bit leaves map to `InputEntryFlag8SetClear`
  (`0x0041c9f0`), `PlayerInputFlag8SetClear` (`0x0041ca20`),
  `InputEntryFlag8Get` (`0x0041ca60`), and `PlayerInputFlag8Get`
  (`0x0041ca70`). Their full-decompile bodies and Finding 444 establish each
  separate boundary and exact bit operation. The port's bounded module-owned
  table storage changes the owner and adds range guards, but these are direct
  implementations of the four mapped operations, not placeholder hooks.
- Seven media state/dispatch functions map to their exact `media.hpp` and
  Finding 418 identities: `CDAudioWatchdogModeUpdate` (`0x004297b0`),
  `AudioPlaybackToggle` (`0x00429810`), `ActionFeedbackSoundDispatch`
  (`0x00428800`), `ActionCodeToSoundAlias` (`0x004351a0`),
  `SoundMinimumDistanceUpdateMode` (`0x004295f0`), `PointerCursorSelect`
  (`0x0045cc20`), and `PointerCursorOverrideSet` (`0x0045cc40`). The full
  decompile confirms the control flow, cue mapping, cursor state writes, and
  mode branches; port-only timer/render/audio seams remain documented.
- `fixed_record_slot_resource_release` and `fixed_record_slot_resource_get`
  map to `FixedRecordSlotResourceRelease` (`0x0048bfc0`) and
  `FixedRecordSlotResourceGet` (`0x0048bff0`). The header, Finding 375, and
  decompile give exact boundaries; each body performs the described release
  or installed-slot lookup. Their backing table is port-owned storage because
  the original writer is still unported, which is a data-model deviation, not
  a reason to omit these two implemented operations.
- Ten sound operations now carry the exact mappings documented in `sound.hpp`
  and confirmed by the full decompile: subsystem enabled getter (`0x00465370`),
  deferred timer set/expire (`0x00464ec0`, `0x00464e80`), sample-marker parser
  and variant selector (`0x00464d00`, `0x00464e20`), configured variant
  property set/read (`0x00465010`, `0x00465030`), streamed-wave step rate
  (`0x0047b8e0`), and level/tutorial stream helpers (`0x00416150`,
  `0x004161a0`). Their source bodies match the documented operations; the
  unported instance-stop and shared RNG details remain explicit deviations.
- Five engine-state accessors/setters map to their names and addresses in
  `engine.hpp`, Findings 295/392, and the full decompile: current mode is zero
  (`0x00425d80`), global scroll guard set (`0x00425cd0`), triple copy out
  (`0x00426230`), flag bits 6 and 9 set (`0x00426360`), and flag bit 7 set
  (`0x004263a0`). The port threads the former globals through `EngineContext`
  or module-owned state, but each implementation retains the documented leaf
  operation.
- Eight front-end leaves map to their exact names and addresses in the full
  decompile and Findings 23/372/438/250/436: the five flow-state probes
  (`0x00416d50`, `0x00416d60`, `0x00416dd0`, `0x00416de0`, `0x00416df0`),
  the menu audio interval draw (`0x00412190`), status-banner global getter
  (`0x0041b9b0`), and panel texture-slot blitter (`0x0045a9c0`). The panel
  code preserves the decompile's record/flag selection; its missing-record
  guard remains a documented port safety deviation.
- Eleven additional front-end operations map to the exact declaration and
  behavior descriptions in `frontend.hpp`: selection-coordinate and region
  stores (`0x0041cce0`, `0x0041e9d0`), objective-selection reset
  (`0x0041a990`), interaction timer reset (`0x0041ee60`), mode flag update
  (`0x00418800`), selected-map coordinate store (`0x0041ea50`), panel
  resource placement (`0x00459fd0`), scroll-offset update (`0x0041a250`),
  active-object word selection (`0x00416e00`), screen-effect clear/release
  (`0x0040a3d0`), and overlay animation advance (`0x0045c580`). The modeled
  state is module-owned and some calls use explicit port seams, as documented
  beside these APIs; those are implementation deviations rather than
  test-only hook functions.
- Six command/UI-chain implementations map to their exact `cmd.hpp` API
  identities: `any_object_can_occupy_terrain_cell` (`0x00431ab0`),
  `menu_selection_resource_update` (`0x0041b300`),
  `command_context_post_to_hud` (`0x0041b2d0`),
  `selection_clear_request_and_hud_reset` (`0x0041e750`),
  `level_resource_balance_add_and_sound` (`0x0041f9a0`), and
  `simulation_mode_counter_add` (`0x0043a910`). Their declarations describe
  the same record sweep, state update, command enqueue, counter, and overlay
  operations implemented in `cmd.cpp`; the documented port guards remain.
- Eight selection and UI-state leaves now map to the identities documented in
  `frontend.hpp`: current-selection-context get/set (`0x00417fe0`,
  `0x00417ff0`), selection bit-0 clear (`0x004181a0`), mode/index handle get
  (`0x00419150`), resource bit-0x40 get (`0x004191f0`), timed-effect update
  request (`0x0046b770`), UI-mode transition (`0x00418fc0`), and selection
  mode dispatch (`0x00418f80`). The exact bit operations, table lookup, and
  state transitions match the documented contracts; bounds guards and
  module-owned state are documented port deviations. `ui_mode_resource_positions_apply`
  remains unannotated because its reposition action is an intentionally
  unported seam.
- Twenty-seven CRT/state-record leaves map to exact `crt.hpp` declarations:
  the named flag/global get/set operations (`0x00401c20`, `0x00407340`,
  `0x00407360`, `0x00409d40`, `0x0040ab10`, `0x0040bfc0`, `0x0040e7e0`,
  `0x0040f5e0`, `0x00401c30`, `0x00401c40`, `0x0045c4a0`, `0x0045cc10`,
  `0x0045cc60`, and `0x00401b20`), request-record predicates
  (`0x0045ab30`, `0x0045ab60`, `0x0045ab90`), and the indexed pair, metrics,
  region, panel-flag, and reward-score operations (`0x0045a6e0`, `0x0045a700`,
  `0x00463730`, `0x00463ed0`, `0x00463ea0`, `0x0045d370`, `0x0045aac0`,
  `0x0045abb0`, `0x0045ab00`, `0x00462800`). These map the described direct
  word, bit, and record-field operations. Guarded array slots and module-owned
  state remain the header-documented port deviations; diagnostic setters and
  peeks stay unannotated.
- Twenty-one additional API implementations map to their exact documented
  identities: global list pushes/free (`0x0046a470`, `0x004792c0`,
  `0x0046a370`, `0x0046a390`); texture-directory, renderer-bit, and scene
  callback registrations (`0x004728c0`, `0x004728e0`, `0x00472900`,
  `0x00472920`); string registry and cache operations (`0x00410450`,
  `0x00410490`, `0x00410520`, `0x00410560`); selection-marker release
  (`0x0040ce30`); pair-table find/append (`0x00488730`, `0x00488790`); signal
  record find (`0x0049ab90`); and block token-pool destroy (`0x00478e20`),
  texture cache-name find (`0x00483030`), entity-kind lookup (`0x00475930`),
  advisor action-position get (`0x00401890`), and indexed linked-list entry
  get (`0x00419780`). The documented bounds, host-pointer layout, and missing
  corpus writers remain port deviations.
- Five additional exact implementations now carry the direct identities in
  their API notes: the `strtol`/`strtod` conversion wrappers
  (`0x0048db70`, `0x0048d980`), simtick sound keep-alive and editor-step
  dispatch (`0x0047aba0`, `0x0044cfa0`), and the bit-field tree operation
  (`0x0049cf90`). The scene-position reset marker (`0x0044cf70`) was moved
  from the state storage declarations onto its implementation. Its field
  copying and bit clear match the described operation; callbacks remain
  explicit port seams.
- Eleven terrain-record flag operations map to their explicit `level.hpp`
  function/address descriptions: the tracked 0x100 and visited-flag setters,
  visited clear, primary/secondary mask clear, three flag-set operations,
  the 0x40000000 clear/set leaf, bit-0x20 clear, edge-flag update, and filled
  work-flag circle (`0x00432100`, `0x00432270`, `0x00432300`, `0x00432360`,
  `0x004323f0`, `0x00432450`, `0x00432490`, `0x00432560`, `0x004310f0`,
  `0x0042f2e0`, `0x00435010`). Their flag mutations and tracking loops match
  the declared roles; the unavailable per-cell refresh remains a no-op seam.
- `cmd_bind_key` maps to `0x00452010` from its direct API comment and
  declaration. Two misplaced markers in `view.cpp` were moved to
  `view_grid_edge_get` (`0x00435f28`) and `polyline_segment_lengths_total`
  (`0x004067b0`), whose adjacent descriptions and bodies match those leaves.
- Nineteen terrain-grid operations now carry their explicit `level.hpp`
  identities: coordinate timestamp remember/exists (`0x0040ee00`,
  `0x0040efb0`), erosion timing/delay/neighbor check (`0x0040e930`,
  `0x0040e9c0`, `0x0040f000`), dirty queue push/clear (`0x00450960`), grid
  record mask/get and field-flag operations (`0x0040ab30`, `0x0040af90`,
  `0x0040b250`, `0x0040b2a0`, `0x0040b2e0`), record-present check
  (`0x0040e460`), terrain normal sampling (`0x004512d0`), surface-type name
  and value reads (`0x00435170`, `0x00435190`), neighbor classification
  (`0x0042f350`), and grid-record count clear/difference/get
  (`0x00437440`, `0x00437460`, `0x004374a0`). Header descriptions and
  implementations agree on their record offsets, masks, iteration, and
  return shapes; bounded access and missing loader-owned tables remain
  documented deviations.
- `flic_opcode27_fallback_noop` is marked `STUB` at `0x00484c90`; `flic.hpp`
  names the exact fallback and the full decompile confirms the body is a pure
  return, matching the implementation.
- Fifty-one additional command-layer implementations now carry their exact
  `cmd.hpp` identities, from the selection snapshot/dispatch and selection
  filters through record scheduling, registry operations, cell issuers, task
  queue producers, and entity-chain cleanup. The header documents each
  function-specific address and behavior; the matching `cmd.cpp` body performs
  the described operation, with port deviations and unported subcalls called
  out in the surrounding documentation. Two callback dispatchers remain
  unmarked because their implementations are explicit no-op seams. The audit
  now finds 952 markers at 952 unique addresses.
- A full-decompile name cross-check exposed and corrected displaced markers:
  `string_case_substring_find` now owns `0x0048c040`; `cmd_chain_walk` and
  `cmd_create_task` own `0x00404190` and `0x00402720`; and the record grid,
  indexed-flag, resource-handle, scene-owner, and indexed-field operations now
  match their exact `record.hpp`/decompile address pairs. This pass added the
  remaining 15 direct record operations whose bodies match the described field,
  grid, or geometry behavior. Forty-six more exact function-name/header pairs
  were mapped across the CRT state leaves, camera and view operations, message
  state, entity registry, UI, save accessors, and terrain-level operations.
  The source audit is now at 952 unique markers; `git diff --check` passes.
- The coverage-ledger cross-check exposed twelve additional exact `ported`
  boundaries and they are now annotated: `EntityFlag40000ThresholdCompare`
  (`0x00437cc0`), `EnginePreTick` (`0x004231f0`), `MainSimStep`
  (`0x004250c0`), `TerrainCellSurfaceAppearanceApply` (`0x0044f5c0`),
  `MessageDispatch` (`0x00419b80`), `frameloop_credits` / `ScrollingTextDisplay`
  (`0x0040a0c0`), `movie_avis_enabled` / `AvisEnabled` (`0x00414ce0`),
  `movie_video_playback_loop` / `DirectShowVideoPlaybackLoop` (`0x0042efc0`),
  and `map_pipeline` / `MapLoadPipeline` (`0x0042b510`), `cmd_init` /
  `CmdRegistryInit` (`0x00401cf0`), and the partial terrain half of
  `terrain_cell_refresh_dispatch` / `TerrainCellRefreshDispatch`
  (`0x00402ae0`), and `simtick_manager_tick` / `ManagerTick`
  (`0x0045a730`). The matching ledger rows, full-decompile headings, and
  source declarations/comments agree. `MessageDispatch` has port gaps in
  payload use, but is the direct implementation boundary and should appear in
  Reccmp for comparison. The terrain refresh mapping preserves its surface-reset
  half while its entity-pool callback half remains unported. At that audit
  point there were 952
  unique markers and no duplicate addresses; `git diff --check` passes.
- This continuation added three exact ledger mappings: the command array
  value search (`cmd_array_find_value_index`, `0x00452650`), the deliberately
  unimplemented quit-poll boundary (`EngineKeepRunning`, `STUB` at
  `0x00435940`), and the renderer global dword getter
  (`renderer_global_dword_get`, `0x004065d0`). The API identities match their
  implementations; the quit poll remains an explicit scaffold stub. Current
  totals at that point were 955 unique markers, 1,042 linter-reported
  unannotated functions, and 176 `ported` ledger rows without a marker.
- `frontend_enabled` now maps to `FrontEndActive` (`0x00416d00`) from the
  header's explicit API pairing and full-decompile body. The port preserves
  the config flag but omits the original loader-mode gate, so this is a
  deliberate partial mapping for comparison. Current totals at that point: 956 unique
  markers, 1,041 linter-reported unannotated functions, and 175 unannotated
  `ported` ledger rows.
- The next evidence pass added `level_random_seed_set` (`RandomSeedSet`,
  `0x0048e160`) from its explicit header identity and matching seed-store body,
  plus `object_lookup` (`STUB 0x0044a200`) and `target_blend`
  (`STUB 0x0044a070`) from the view module's direct hook comments and stub
  bodies. Current totals are 959 unique markers, 1,038 linter-reported
  unannotated functions, and 172 unannotated `ported` ledger rows.
- The terrain/view pass added six direct boundaries: the simple walkability
  branch (`level_walkable`, `0x00432770`), three bounded terrain-bit reads
  (`0x00432b00`, `0x00432b50`, `0x00432d90`), the occupied-flag setter
  (`level_tile_mark`, `0x00432e00`), and the per-entry screen-effect helper
  (`effect_helper`, `STUB 0x0040aa40`). Walkability is partial: the corpus's
  entity-state and speed gates remain documented gaps. Current totals are
  965 unique markers, 1,032 linter-reported unannotated functions, and 166
  unannotated `ported` ledger rows.
- `level_tile_type` maps to `TerrainCellMaterialGet` (`0x004319b0`): the
  full decompile reads the same terrain record byte at `+2`; the port adds a
  bounds guard and signed-byte conversion. Current totals are 966 unique
  markers, 1,031 linter-reported unannotated functions, and 165 unannotated
  `ported` ledger rows.
- `sound_positional_sound_submit` now maps to `PositionalSoundSubmit`
  (`0x00465090`). Its full decompile and body agree on the batch-mode gates,
  alias lookup, positional/coordinate route, deferred record fields, and
  zero return. Current totals are 967 unique markers, 1,030 linter-reported
  unannotated functions, and 164 unannotated `ported` ledger rows.
- The latest source/decompile comparison mapped four more boundaries:
  `idle_register` / `IdleQueueRegister` (`0x00407450`),
  `object_list_load_file` / `OListEntityRecordParse` (`0x0042e020`), the
  resolver core `entity_type_pair_for_name` / `ObjectListEntityTypeResolve`
  (`0x0042e8b0`), and `view_tick` / `ViewContextTick` (`0x00435f5c`). Their
  direct decompile entries and source comments establish the boundaries;
  omitted idle timestamps, extended object setup, optional resolver output,
  and unported view modes remain comparison gaps. Current totals are 971
  unique markers, 1,026 linter-reported unannotated functions, and 160
  unannotated `ported` ledger rows.
- The save-slot audit added `save_read_slot` / `SavReadSlot` (`0x00417a90`),
  the explicitly documented serialization branch `sav_write_slot_file` /
  `SavWriteSlot` (`0x00417bd0`), and `save_mark_record` / `SavMarkRecord`
  (`0x00417eb0`). The full decompilation confirms the file read, slot-file
  serialization, and record flag operations. The port has documented
  differences in slot-context handling, bootstrap/mode branches, and index
  guards; no wrapper shares these addresses. Current totals are 974 unique
  markers, 1,023 linter-reported unannotated functions, and 157 unannotated
  `ported` ledger rows.
- A source/decompile cross-check added `simtick_vector_within_axis_tolerance`
  / `VectorWithinAxisTolerance` (`0x0044d9a0`): all three axis comparisons
  match, with the NULL guard and separately modeled tolerance storage
  documented as port differences. It also added `three_surface_resources_load`
  / `ThreeSurfaceResourcesLoadAndInitialize` (`0x0045bfb0`): the three
  load-and-transform operations and the unusual second/third destination
  order match the full decompile. Current totals are 976 unique markers,
  1,021 linter-reported unannotated functions, and 155 unannotated `ported`
  ledger rows.
- `panel_resource_position_update` now maps to `PanelResourcePositionUpdateIfEnabled`
  (`0x0045c4b0`). The address-specific header description and full decompile
  agree on the glyph-present gate, indexed position plus stored offsets, the
  `0x50080c` and status-banner gates, and the conditional blit. Current totals
  are 977 unique markers, 1,020 linter-reported unannotated functions, and
  154 unannotated `ported` ledger rows.
- `cmd_queue_cancel_match` maps to `CommandQueueCancelOpcode` (`0x00451f60`):
  its active-bank walk, second-field comparison, swap-with-last removal, and
  forward cursor match the full decompile. The port's pointer-width comparison
  is the documented deviation. Current totals are 978 unique markers, 1,019
  linter-reported unannotated functions, and 153 unannotated `ported` ledger
  rows.
- `cmd_produce` now maps to `KeyStateGatedCommandEnqueue` (`0x00452060`). Its
  active-buffer capacity gate and five-dword record append match the decompile;
  the port flattens the original optional pair pointer into its final two
  scalar parameters. Current totals are 979 unique markers, 1,018
  linter-reported unannotated functions, and 152 unannotated `ported` ledger
  rows.
- `entity_at` maps the regular type 1–4 array lookup branch of
  `EntityPointer` (`0x0042ed70`). The full decompile uses the same typed-array
  bases and strides; the port adds bounds/null guards and groups the original
  special singleton kinds, so the source comment records that partial scope.
  Current totals are 980 unique markers, 1,017 linter-reported unannotated
  functions, and 151 unannotated `ported` ledger rows.
- `cmd_queue_tick` maps to the `SelectionRoot` dispatcher (`0x004520d0`): the
  keybind pre-pass, active-buffer swap/reset, and opcode walk follow the full
  decompile. Several manager opcodes remain explicit port stubs. Current
  totals are 981 unique markers, 1,016 linter-reported unannotated functions,
  and 150 unannotated `ported` ledger rows.
- `osf_read_state_reset` now maps to `OsfReadStateReset` (`0x004592a0`). The
  exact reset is extracted from the load path: the port resets its fixed-table
  count and capacity instead of freeing an allocated global table, while the
  reader separately clears the port-only shadow state. Current totals are
  982 unique markers, 1,016 linter-reported unannotated functions, and 149
  unannotated `ported` ledger rows.
- `ares_cd_fallback_path` now maps to `SoundPathFallbackFormat`
  (`0x0047f660`). The helper owns the drive-present gate, formatted output,
  and success result from the original routine; the existing caller's AUTO
  mode gate remains outside it. The port-specific mounted-root override and
  path bounds/separator handling are documented deviations. Current totals
  are 983 unique markers, 1,016 linter-reported unannotated functions, and
  148 unannotated `ported` ledger rows.
- The camera pass added direct timer-loop mappings: `camera_grid_timers` /
  `LevelRecordTimerCountdown` (`0x0042c440`) decrements the move-list timer
  at stride `0x4c`; `camera_grid_expiry` / `LavaErosionCellTimersAdvance`
  (`0x0042cb70`) scans the grid, advances busy-cell timers, and dispatches the
  expiry action. `camera_spawn_countdown` / `SpawnCoordinatePoolTick`
  (`0x004261c0`) decrements the level countdown, probes/issues a spawn, and
  rearms from the level interval. The context-based state/grid binding, guards,
  and stubbed spawn collaborators are port adaptations. Current totals are
  986 unique markers, 1,013 linter-reported unannotated functions, and 145
  unannotated `ported` ledger rows.
- The ManagerTick address is assigned to `simtick_manager_tick`, where the
  target selection, smoothing, mode routing, timer stop, and settle logic live.
  The `tickers.cpp` `ManagerTick` wrapper only supplies the context-derived
  `ingame` flag and calls this helper, so it remains unmarked.
- The same-name decompile audit left only known non-one-to-one cases unmarked:
  `cmd_chain_purge` currently clears an entity-owned chain while the corpus
  `CmdChainPurge` sorts a global queue; `cmd_tick_global` represents only part
  of `CmdTickGlobal`, split across the command and simtick modules; the named
  coordinate/argument callback dispatchers are unported no-op seams;
  `ui_mode_resource_positions_apply` has no implemented reposition action.
  These names alone do not justify assigning the original entry addresses.

- `FUN_0042e8b0` has one marker on `entity_type_pair_for_name`, the helper
  that contains the full resolver search and special-name logic. The public
  `entity_type_row_for_name` wrapper and `entity_type_for_name` convenience
  wrapper remain unmarked to avoid mapping the same original entry more than
  once.
- `FUN_00477440` is cited as a shared text/token split helper from several
  modules. The closest shared-looking API, `util_query_fields`, also opens a
  resource and tokenizes whitespace/pipe fields; it is not the corpus
  delimiter-driven, in-place splitter. Other call sites use specialized local
  splitters. Similar parsing outcomes do not establish one original function
  boundary, so none of these definitions receives that address.
- `RandomNext` (`0x0048e170`) maps only to `level_rand_u16`, paired with the
  already mapped `level_random_seed_set` (`0x0048e160`). Their source state
  and exact LCG operations correspond directly to the adjacent original
  seed/draw entries. Other modules keep separate local copies of the original
  shared RNG state and remain unmapped to avoid duplicate or misleading
  address assignments.
- Comments that say a corpus operation has “no ported owner,” or that a
  function is an unported hook, are not sufficient grounds for a marker.
- The latest audit of the remaining high-citation ledger rows found no
  additional one-to-one boundary among DirectDraw bitmap load/save and font
  record-creation operations (the port has software rendering and load seams), the
  shared RNG thunk (other module-local models), the scene-activity
  bookkeeping (folded into activity updates), or the terrain pick/height
  hooks (explicit stubs). The lava-crust tile probe and idle-table insertion
  are now extracted as a partial `EntityLavaCrustActivityRegister` mapping;
  its stats gate and other documented deviations remain. The basic
  `entity_create` slot allocator also does not implement the original's large
  type-specific spawn dispatcher. Keep these rows unmarked until a matching
  implementation boundary appears.
- A fresh same-name review confirmed `CmdChainPurge` (`0x00402310`) has no
  matching port implementation: the original removes flagged records from a
  global queue and performs per-record status updates, while
  `cmd_chain_purge` clears one entity's command chain. `StartLevelOverrideIfEnabledGet`
  (`0x00401c00`) returns a pointer into unowned startup state, for which the
  port has no owner. `SlotSummary` (`0x004152c0`) counts completed records,
  sums point values, and formats a completion string; `save_slot_state` only
  counts record flags and selects a resume index, so it is not the same
  boundary. `WadEntryMemoryStreamOpen` (`0x0048be40`) materializes an entry
  into the original fixed stream table; the port's resource handles defer
  payload reads, and its related slot helper is already mapped to the distinct
  `WadEntryReadIntoStreamSlot` entry (`0x0047fea0`). None of these is a safe
  additional marker.

### WinAPI annotation pass

The WinAPI entry helpers are marked as partial implementations of the
original `RegistryPathAccessor` (`0x0048b360`) and
`CommandLineOptionParse` (`0x00477bf0`). In `src/platform/winapi/frameloop.cpp`,
the known window-create, message-pump, and render-present seams are marked as
stubs for `WindowCreate` (`0x004783f0`), `Win32MessagePumpDispatch`
(`0x00478040`), and `RenderSurfacePresent` (`0x0047c8d0`). Their mappings are
documented in finding 398 and finding 431; the current WinAPI source explicitly
defers their real implementations.

The project-side implementations for the remaining adapter operations live
in `src/platform/native/frameloop.cpp`; the corresponding WinAPI definitions
are placeholders in `src/platform/winapi/frameloop.cpp`. That identifies where
the portable implementation comes from, but does not identify an original
`LegoRR.exe` entry point. `platform.h` describes event polling as a convenience
wrapper around the event pump, screenshot capture as an optional PPM
diagnostic, and audio as a PCM submit/loop interface over a platform stream.
The full decompile shows the three marked operations as distinct original
entries; it does not establish separate original entries for teardown,
HWND/dimension getters, `platform_poll_events`, screenshot capture, or each
PCM/audio operation. Those APIs may combine behavior that the original
implements in other routines or through DirectSound calls. Without a direct
one-to-one boundary, assigning an address would be a guess. Do not reuse the
three marked addresses for those methods.

The unresolved adapter methods were checked against their native definitions:
`platform_destroy_window` destroys the SDL window and frees the adapter
object; `platform_window_hwnd` exposes the SDL window handle;
`platform_window_width` / `platform_window_height` return dimensions stored at
creation; `platform_poll_events` filters the batch produced by
`platform_pump_events`; and `platform_screenshot` writes an opt-in PPM
diagnostic. These are backend lifecycle, convenience, and diagnostic APIs,
not source-identical original entry points. The audio methods create and
operate an SDL audio stream: open/close its device, submit mono/stereo samples,
loop a buffer, refill, set/read gain, report queued bytes, and stop playback.
The decompiled original instead distributes sound-device setup, DirectSound
buffer operations, sample submission, and playback control across separate
engine routines; the platform audio API was introduced by this port and no
matching original boundary is documented. Keep these definitions unmarked
until a specific original routine and corresponding implementation boundary
are established. The existing WinAPI bodies are intentional reference-build
stubs and do not supply additional mappings.

Continue searching for exact, source-backed names/addresses and validate each
new mapping against address uniqueness. Avoid broad renames, code behavior
changes, report interpretation without a fresh build, or claiming the
annotation work is complete solely because the linter’s remaining entries
cannot be guessed.

- The state-accessor audit added four direct mappings: `system_state_flags_get`
  / `SystemStateFlags` (`0x00456240`), `system_state_flags_set` /
  `SetSystemStateFlags` (`0x00456550`), `global_flag_004ff978_get` /
  `GlobalFlagGet_004ff978` (`0x00456640`), and
  `global_value_004ffe48_set` / `GlobalValueSet_004ffe48` (`0x00458660`). The
  API comments identify each exact corpus operation and address; the bodies
  directly read or write the corresponding module-owned word. The setter's
  null guard is already documented as a port deviation. Current totals are
  997 unique markers and 134 `ported` ledger rows without a marker.
- `mission_crystal_quota_setup` now maps to `CrystalQuotaSetup`
  (`0x0045c040`). Its extracted body reads `Reward Quota Crystals` and applies
  the documented zero/over-limit fallback before storing the context value;
  `mission_task_refresh` retains the same call order and result. Current totals
  are 998 unique markers and 133 `ported` ledger rows without a marker.
- Two mission config initializers now carry partial mappings explicitly
  identified by `mission.hpp`: `mission_task_init` / `TaskBlockInit`
  (`0x0045f030`) and `mission_reward_init` / `RewardInit` (`0x0045f7f0`). The
  former initializes the context-owned ten task records and runs the six field
  loaders; it differs from the original's session-global allocation, loader
  ordering, and extra refresh. The latter mirrors the reward display settings;
  render-resource ownership remains outside it. Current totals are 1,000
  unique markers and 131 `ported` ledger rows without a marker.
- The texture path adds two documented counterparts: `texture_load` /
  `CreateBmpSurface` (`0x00474050`) decodes a resource-backed BMP into the
  port's software surface, and `texture_get_or_load` / `TextureGetOrCreate`
  (`0x00482e70`) implements the name lookup, reference increment, miss load,
  and cache insertion. The software surface replaces DirectDraw, and the port
  has one cache rather than the corpus's primary/secondary caches. Current
  totals are 1,002 unique markers and 129 `ported` ledger rows without a
  marker.
- `model_apply_uv_text` now maps to `ParseUV` (`0x00481c50`). It parses the
  sidecar's buffered text and applies UV blocks to the model, matching the
  corpus parser's central operation; the caller supplies already-read text
  rather than the original stream handle. Current totals are 1,003 unique
  markers and 128 `ported` ledger rows without a marker.
- The model parser now gives `LwoPoints` (`0x0048c210`) its own `parse_points`
  helper, extracted from the inline `PNTS` branch in `model_parse`. It converts
  each big-endian float triple and appends it to the vertex buffer, matching
  the corpus handler's payload operation. The parse-failure path remains the
  same. Current totals are 1,004 unique markers and 127 `ported` ledger rows
  without a marker.
- `texture_bitmap_layout_prepare` now maps to `IndexedBitmapPaletteDataPrepare`
  (`0x00489c30`). It prepares bitmap dimensions, format, row stride, pixel
  offset, and the zero-padded RGB palette used by the decoder. File-bound
  checks and OS/2-header support are port safeguards; `texture_decode` still
  performs software pixel conversion after the extracted preparation step.
  Current totals are 1,005 unique markers and 126 `ported` ledger rows without
  a marker.
- `camera_move_pipeline` maps to `CameraMoveCommandRoute` (`0x0046a730`): it
  preserves the original move-list call and conditional post-step walk. The
  two called helpers are still explicit stubs, which is recorded at the
  definition. Current totals are 1,007 unique markers and 124 `ported` ledger
  rows without a marker.
- `resource_wad_lookup` maps to `WadEntryNameLookup` (`0x0048bda0`): both
  perform a case-insensitive name scan within one archive and return the entry
  index or `-1`. The mounted-archive index and count guards are port safeguards.
  Current totals are 1,006 unique markers and 125 `ported` ledger rows without
  a marker.
- `fe_build_chain` maps to `LevelChainBuild` (`0x004154b0`): both read the
  configured starting level, follow its `NextLevel` chain, and build ordered
  level names with display labels. The port writes into caller-owned bounded
  arrays and applies its own fallback label behavior; the original allocates
  its result records. Current totals are 1,008 unique markers and 123 `ported`
  ledger rows without a marker.
- `save_stream_length` maps to `GameFileLengthGet` (`0x0047fca0`). The original
  saves the current stream position, seeks to end for the length, then restores
  the saved position. The helper extracts the equivalent inline sequence from
  `.osf` loading, where the stream is newly opened at offset zero. Current totals
  are 1,009 unique markers and 122 `ported` ledger rows without a marker.
- `mission_replace_char` maps to `StrReplace` (`0x00413f00`): both replace one
  selected character in a NUL-terminated string. The port limits the scan to
  the modeled 0x20-byte reward-text field; the original scans to NUL. Current
  totals are 1,010 unique markers and 121 `ported` ledger rows without a marker.
- Reviewed two other cited candidates and left them unmarked: `main_hud_text`
  only writes an already-built string to stderr, while `TextBufferAppendFormat`
  formats varargs into the original engine text buffer; `save_write_thumbnail`
  emits a software framebuffer BMP, while `BitmapFileWrite` reads a DirectDraw
  surface, applies pixel masks, and writes its bitmap. The shared output format
  is not enough to establish the same function boundary.
- `texture_decode` maps to `BitmapSurfaceLoad` (`0x0047d9d0`): the decompile
  parses BMP dimensions/palette and creates a surface from its pixels; the
  decoder interprets the bitmap bytes and converts their rows into the port's
  software surface. Resource lookup and DirectDraw allocation remain in the
  surrounding original routine and are represented by the separate
  `texture_load` boundary. Current totals are 1,011 unique markers and 120
  `ported` ledger rows without a marker.
- `simtick_frame_counter` maps to `CmdTickGlobal` (`0x00402220`) for the
  matching frame-count clamp and increment at the start of the original body.
  The original then walks and purges command lists and dispatches callbacks;
  those later operations are not implemented by this function. The header and
  source explicitly identify this as its partial mapping. Current totals are
  1,012 unique markers and 119 `ported` ledger rows without a marker.
- `cmd_minion_companion` maps to `MinionCompanionCmd` (`0x00402850`). The
  original and extracted port helper both gate on entity type 6, allocate a
  kind-3 record, bind kind `0x15`, append it at the tail, and set flag `0x2000`.
  The port retains its allocation-failure guard. Current totals are 1,013
  unique markers and 118 `ported` ledger rows without a marker.
- `resource_open` maps to `GameFileOpenDispatch` (`0x0047f6e0`): both select
  the WAD or standard-file path according to the requested mode, including the
  standard-file fallback. The port returns a handle and defers payload reads to
  `resource_read`; the original dispatch opens its underlying stream inline.
  Current totals are 1,014 unique markers and 117 `ported` ledger rows without
  a marker.
- `resource_mount` maps to the archive-mount loop in `WadMountStartup`
  (`0x0047f130`): both try numbered `<prefix><index>.wad` paths for indices
  0–9 and mount each available archive. This is partial: the original also
  checks CD state and handles retry dialogs and missing-data exits. Current
  totals are 1,015 unique markers and 116 `ported` ledger rows without a
  marker.
- The extracted `ares_open_wad_path` helper maps to the all-archives branch of
  `WadPathOpen` (`0x0048bf70`): both search mounted archives newest-first,
  resolve the named entry, and open the first match. The port returns a lazy
  resource handle; the original fills its fixed stream slot immediately.
  Current totals are 1,016 unique markers and 115 `ported` ledger rows without
  a marker.
- `options_load_corpus_defaults` maps to the fallback assignments in
  `OptionsLoadOrDefault` (`0x00416940`): both set game speed 1, SFX volume 8,
  and music volume 5. The original's save-slot selection and state reset remain
  in the surrounding startup path; the port's other option defaults are
  port-specific. Current totals are 1,017 unique markers and 114 `ported`
  ledger rows without a marker.
- `ai_attached_entity_wake` maps to `AttachedEntityWake` (`0x0044c110`). It
  receives the already-resolved target and performs the same special-state
  test, 100.0 wake-timer assignment, and `0x800000` wake-flag set. The original
  resolves the target from the parent's `+0x364` pointer; the port's callback
  passes its `wake_target` field. Current totals are 1,018 unique markers and
  113 `ported` ledger rows without a marker.
- A follow-up of the high-citation rows left `CameraPanAxisUpdate`
  (`0x00436900`) unmarked: the original adds a delta to a camera tilt field,
  while the port's `camera_pointer_pan_update` accumulates a normalized
  pointer-pan vector. `TimedStatusAndEventTick` (`0x0041b930`) updates global
  timers and event bits absent from `message_update`; `WadReverseLowBits`
  (`0x0049d000`) belongs to the still-unported compressed-payload decoder.
  `TerrainGridPick` (`0x0044f6d0`) is only a forwarding wrapper to the original
  world-to-cell bounds helper, while the current `grid_pick` is an approximate
  floor-and-bounds shim. These remain unmatched rather than receiving guessed
  addresses.
- `FUN_0040d490` remains unmatched: the original walks a dynamically sized
  table of 20-byte self-linked records and invokes a supplied callback for
  each live record. The port's AI timer registries are fixed slot arrays with
  a different record layout and no corresponding callback walk; their shared
  “registry” terminology is insufficient evidence for this address.
- `options_scroll_index_store` now maps to `ScrollToSliderIndexStore`
  (`0x00414f90`). The helper was extracted from `scroll_slider_refresh` without
  changing behavior; its threshold result is stored in the context-owned
  speed-index field in place of the original module global. The separate
  `scroll_to_slider_index` and `scroll_slider_refresh` mappings remain at
  `0x00415030` and `0x00415360`. Current totals are 1,019 unique markers and
  112 `ported` ledger rows without a marker.
- `fe_slider_pointer_handle` maps the explicitly identified base-track drag
  branch of `MenuSliderPointerHandler` (`0x00411870`). The source comment and
  frontend interaction test identify the handler route; the extracted helper
  preserves the current pointer bounds, value calculation, and option update.
  The original's alternate arrow-region handling remains absent. Current
  totals are 1,020 unique markers and 111 `ported` ledger rows without a
  marker.
- `s_variant_split` maps the comma-specialized behavior of
  `BlockTextTokenSplit` (`0x00477440`). Its adjacent comment identifies the
  exact original routine and its in-place token behavior; the fixed output
  capacity is a documented port bound. Current totals are 1,021 unique markers
  and 110 `ported` ledger rows without a marker.
- `movie_key_path` and `movie_level_video_path` carry partial mappings for
  `MoviePlayNamed` (`0x00415910`) and `MoviePlayLevelVideo` (`0x00415990`). Their
  source comments and the movie API documentation identify the corresponding
  config-path stages; the frame-loop caller opens and plays the movie. Current
  totals are 1,023 unique markers and 108 `ported` ledger rows without a
  marker.
- An earlier save-menu/bitmap/FLIC review left `save_write_thumbnail`
  unmarked because the port reads a software framebuffer instead of querying
  a DirectDraw surface. Re-reviewing the implementation against the full
  `BitmapFileWrite` decompile showed that the actual file-writing operation
  matches: it writes the same 54-byte 24-bit BMP header, bottom-up BGR rows,
  four-byte row padding, and pixel payload. The input acquisition differs, so
  the marker identifies the bitmap serialization operation, not DirectDraw
  surface extraction. `SaveMenuThumbnailConfigLoad` and `MenuRunLoop` still
  have no corresponding source functions. `flic_tick` unifies the original
  RAM/stream advance paths, with its established mapping on
  `FlicRamFrameAdvance` (`0x004841d0`); do not reuse that address for the
  stream entry or mark the larger frame pump as `FlicFrameAdvanceAndBlit`.
- `OsfSnapshotTableBuild` (`0x00459360`) remains unmapped. Its original
  operation resets the live table and rebuilds it from the shadow snapshot;
  the port only implements `osf_snapshot` in the opposite direction (live to
  shadow) and invokes that after writes/reads. There is no current
  shadow-to-live source definition to annotate.
- A further sweep of the remaining ledger citations found no additional
  definitions to mark in several tempting cases. `SelectionDragAreaCommandCreate`
  (`0x00403270`) is cited only at a queue call site; the original creates one
  move command per selected entity, while the port has no batch creator.
  `MenuScrollStateUpdate` (`0x00413970`) does not match the port's
  viewport-relative edge scroll: the original uses absolute cursor bands, a
  squared velocity, and an unowned global offset. `GameConfigInit`
  (`0x0041fb50`) is a large startup/configuration routine with no corresponding
  all-in-one port function. The RNC type-1/type-2 decoder entries
  (`0x0049c8e0`, `0x0049ca60`) remain dormant and unimplemented; only shared
  table and dispatch helpers are ported.
- `input_begin_frame` is now marked as the successful-device state-rollover
  portion of `InputStatePoll` (`0x0047eef0`): it snapshots the current 256-key
  state and clears it before the port captures the next event batch. DirectInput
  device acquisition/retry remains platform-owned. Current totals are 1,024
  unique markers and 107 `ported` ledger rows without a marker.
- The next subsystem pass found no additional direct boundary in the scene,
  timed-callback, or sound-route candidates. `GlobalTimedCallbackDispatch`
  (`0x0044bf00`) walks and destroys a linked callback queue, which has no
  source owner. `SoundSubmissionRouteWrapper` (`0x00465050`) resolves an
  original scene object before calling the already-mapped positional submit
  routine (`0x00465090`); the port has no equivalent scene-object resolver.
  `SceneActionNameSet`, `DDiSceneActFrameRecordParse`, and `LwscSceneLoad`
  (`0x00473370`, `0x00475d10`, `0x004765c0`) belong to the explicitly deferred
  scene-render pipeline; the `.ae` parser stops after assembling action data.
  `FlicFrameAdvanceAndBlit` (`0x00484070`) wraps either frame decoder with
  DirectDraw surface locking and presentation; `flic_tick_loop` only advances
  and decodes, while overlay drawing is split across frontend/render code.
  Do not map those separate pieces to the wrapper entry.
- `fe_selected_level_action` is a partial mapping for
  `FrontEndSelectedAction` (`0x00416d70`). It resolves the selected campaign
  or tutorial picker row to its level key, matching the original's selected
  list-entry lookup; the port's separate save-slot-to-level path remains in
  the frame loop. The helper was extracted from `fe_activate` without changing
  the selection behavior. Current totals are 1,025 unique markers and 106
  `ported` ledger rows without a marker.
- `camera_registry_probe` maps to `MainSimFeatureFlagLatch` (`0x00435a50`).
  The full decompile and `camera.hpp` agree on the `0x08000000` state-bit
  gate, registry probe `0x40b`, and debug-bit `0x2` latch; the port threads
  the original globals through `EngineContext`. Current totals are 1,026
  unique markers and 105 `ported` ledger rows without a marker.
- `camera_state_root` maps to `CurrentLevelGet` (`0x00431560`): both return
  the active level-state root pointer, threaded through `EngineContext` in
  the port. This corrects the earlier exclusion that treated this accessor
  as a generic field read. Current totals are 1,027 unique markers and 104
  `ported` ledger rows without a marker.
- `level_rand_u16` maps to `RandomNext` (`0x0048e170`), the exact LCG
  state-advance and 15-bit extraction paired with `level_random_seed_set`
  (`0x0048e160`). Other modules' local RNG copies remain unmapped because the
  original shares one state. Current totals are 1,028 unique markers and 103
  `ported` ledger rows without a marker.
- `ai_lava_crust_activity_register` maps the tile-probe/table-insertion
  portion of `EntityLavaCrustActivityRegister` (`0x0043a960`). Its enclosing
  entity tick supplies the activity-eligibility gates; the missing stats-bit
  gate, no-grid fallback, and duplicate suppression are documented deviations.
  Current totals are 1,029 unique markers and 102 `ported` ledger rows
  without a marker.
- `fe_font_glyph_draw_and_measure` maps the glyph-grid, optional draw, and
  advance operation of `BitmapFontGlyphDrawAndMeasure` (`0x0047a470`). It
  combines existing software blit and ink-width helpers; DirectDraw surface
  handling, stored float advances, and clipped RGBA output are documented
  port differences. Current totals are 1,030 unique markers and 101 `ported`
  ledger rows without a marker.
- A review of the remaining movie and message-ticker citations found no new
  one-to-one boundary. `MoviePlayWait` (`0x00415700`) is the DirectShow sample
  update/frame-bound loop; `movie_video_playback_loop` instead pumps the
  port's FFmpeg frame-source, present, and quit callbacks and is documented as
  the replacement for `DirectShowVideoPlaybackLoop` (`0x0042efc0`).
  `MoviePlayLoop` (`0x004157c0`) is a timed bitmap-surface display, while the
  AVI path in `frameloop.cpp` opens and decodes a movie, handles audio, pacing,
  and skip input. These are related playback features, but the original
  function boundaries and responsibilities do not line up. Likewise,
  `message_update` only advances the message clock, `message_cooldown_tick`
  updates per-id cooldowns, and neither implements the distinct queue walk or
  incident-text consumption represented by `InfoMessageQueueTick`
  (`0x00419f30`) and `IncidentMessageTextConsumer` (`0x0041a080`). Keep these
  addresses unassigned absent a matching implementation boundary. Totals
  remain 1,030 unique markers and 101 `ported` ledger rows without markers.
- `fe_item_pos` now carries a partial mapping to `MenuTick` (`0x00412c00`):
  the adjacent function comment identifies the original item-position
  arithmetic, and the helper computes the menu/item anchor, optional text
  centering, and scroll-plane offset used by rendering and hit testing. The
  port's special absolute save/load rows and separate frontend data model are
  not the original routine's full behavior. Current totals are 1,031 unique
  markers and 100 `ported` ledger rows without markers.
- `frontend_build_slots` now carries a partial `SaveMenuBuild` mapping
  (`0x00415a10`) for adding runtime slot rows to the save/load menus. The
  function-specific source comment and implementation establish this boundary;
  config-key resolution, slot-summary initialization, and callback/tree setup
  remain outside the helper. Current totals are 1,032 unique markers and 99
  `ported` ledger rows without markers.
- `save_write_thumbnail` maps to the bitmap-serialization operation of
  `BitmapFileWrite` (`0x0047c8f0`). Its generated BMP header and pixel-row
  layout match the decompile; the port takes RGBA framebuffer bytes rather
  than querying and mask-converting a DirectDraw surface. Current totals are
  1,033 unique markers and 98 `ported` ledger rows without markers.
- A follow-up terrain/surface pass left `TerrainCellChangedFlagSet`
  (`0x0044efb0`), `TerrainVoxelNeighborsMutate` (`0x0044f380`),
  `TerrainSurfaceHeightInterpolate` (`0x0044f940`), and
  `TerrainCellSurfaceVectorsRebuild` (`0x004514f0`) unmarked. The source's
  `cell_refresh_seam` is an explicit no-op hook; terrain neighbor mutation
  and surface-vector rebuilding belong to the absent mesh renderer; and the
  camera's `terrain_height` approximation reads one voxel byte instead of
  interpolating the selected triangle's corner heights. The related cell
  flag helpers therefore do not establish those original function boundaries.
  `ImageSurfaceBlitTransformed` (`0x0047dbf0`) also remains unmatched: its
  decompile converts float color channels and commits them through the image
  surface interface, while the port's similarly named pixel helpers perform
  software scaling or texture copies. Totals remain 1,033 unique markers and
  98 `ported` ledger rows without markers.
- `TerrainCellProgressUpdate` (`0x00434570`) does not map to the port's
  `step_path`. The original accumulates `dt / speed` in a per-cell float at
  voxel offset `+0x14` and compares it with a global threshold. The port
  advances the entity through world-space waypoint distances using cells per
  second, consumes leftover frame time across waypoints, and keeps no
  per-cell progress accumulator. Similar arrival outcomes do not make these
  the same function boundary. Totals remain 1,033 unique markers and 98
  `ported` ledger rows without markers.
- The ledger's remaining RNC low-bit helper `WadReverseLowBits`
  (`0x0049d000`) has no port-side implementation to annotate. `resource.cpp`
  leaves the type-1/type-2 codecs dormant and returns a failure marker for
  those payloads; the raw payload path does not use bit reversal. The current
  ledger has 2,564 rows: its 1,124 `ported` rows are the only rows with
  source citations, and 98 of those still lack markers. Totals remain 1,033
  unique markers with no duplicate addresses.
- `selection_candidate_filter_default` maps to `SelectionCandidateFilterDefault`
  (`0x004526c0`). The full decompile is an exact wrapper calling
  `SelectionCandidateFilter(entity, mode, 1)` and returning; the port wrapper
  preserves that call and ignores the callee's return as the original does.
  A fresh address audit must zero-pad marker addresses before comparing them
  with the ledger. It finds 1,034 unique markers and 97 cited `ported` rows
  without markers, with no duplicate addresses. This supersedes the preceding
  stale total of 1,033 markers and 98 missing rows.
- A targeted pass over the remaining command/selection and level-map candidates
  found no additional one-to-one definitions. `FUN_004030e0` copies an optional
  variable-length payload into a type-`0x13` record after validating its
  source; `entity_command15_validated_record_queue` instead creates a type
  `0x1a` record and links it at kind `0xf`. `FUN_00403270` creates and attaches
  one type-0 command record per drag-selection entity after eligibility checks;
  `selection_add` only updates selection state. `FUN_0040ea10` writes an
  erosion phase/timer and registers the cell in the pending-stage table, while
  `level_load` only copies/scatters the map byte and has no such stage table.
  The menu scroll behavior at `FUN_00413970` is embedded approximately in the
  larger `frontend_tick`, with different band/velocity arithmetic, rather
  than represented by a distinct matching helper. Keep these addresses
  unassigned until those missing behaviors or exact boundaries are ported.
- `rng_thunk_draw15` maps to the distinct duplicate LCG entry
  `thunk_FUN_0048e170` (`0x004798a0`). Its original body updates the same
  shared state with the same LCG constants and returns the same 15-bit slice;
  the port wrapper forwards to the single shared RNG implementation. This
  is a separate address from `RandomNext` (`0x0048e170`), so it does not
  duplicate that marker. The earlier blanket note about leaving other RNG
  copies unmapped does not apply to this exact wrapper. Current totals are
  1,035 unique markers and 96 cited `ported` rows without markers, with no
  duplicate addresses.
- A config/text-pool pass did not find exact port definitions for
  `ConfigStringCopy` (`0x00479050`) or `BlockTextTokenAllocate` (`0x00479270`).
  The former returns a separately heap-allocated copy; `block_get_strptr`
  returns borrowed tree storage and `block_get_str` copies into caller-owned
  storage. The latter pops and initializes a pooled 0x20-byte linked record;
  the port parser instead constructs a tree using individually `calloc`'d
  `BlockRec`s. Existing pool grow/init/destroy mappings cover distinct
  operations, not allocation. Keep these addresses unassigned absent a
  matching lifetime/record boundary.
- The FLIC pass found no direct mappings for `FlicFrameAdvanceAndBlit`
  (`0x00484070`) or `FlicStreamFrameAdvance` (`0x00484260`). The former wraps
  timer conversion, surface lock/unlock, a RAM-or-stream step, and surface
  blit; `flic_tick_loop` only matches the RAM stepper `FlicRamFrameAdvance`
  (`0x004841d0`) already marked in source. The stream stepper reads the next
  frame from a game-file stream and seeks on loop, while this port preloads
  chunks at open and decodes them on tick. The renderer/caller responsibilities
  are split across the movie and platform layers, so assigning either wrapper
  address to `flic_tick_loop` or a presentation helper would misattribute the
  function boundary. Current totals remain 1,035 unique markers and 96 cited
  `ported` ledger entries without markers.
- `entity_stats_word_atc8` now maps directly to `FUN_00469fc0`: Finding 457
  identifies the stats-record dword at offset `+0xc8`, and the full decompile
  is exactly that nested-record read. This also corrects a stale AI comment
  that had described `0x00469fc0` as a linkability test; the original
  linkability operation is still unported. The address audit now finds 1,036
  unique markers and 95 cited `ported` ledger entries without markers.
- Two more similar-looking candidates remain unmatched after body comparison.
  `FUN_00419b00` tests whether the per-ID compiled display-template pointer is
  nonzero (Finding 441); `message_dispatch` only checks its separate catalog
  name and sound fields, and the port does not model compiled templates.
  `FUN_0045faf0` resolves the Save/Advance button blocks, loads multiple
  surfaces, parses their coordinates, and releases temporary strings;
  `mission_reward_init` currently captures only the configured positions and
  does not implement that resource-loading boundary. Neither address belongs
  on those broader source functions.
- A reset/entity/scene batch yielded no further direct mapping. `ResetLevelAndExit`
  (`0x004250a0`) only calls the watchdog-mode update and CRT exit; the engine
  shutdown source is explicitly a stub and does not implement that wrapper.
  `EntityPositionToTerrainCell` (`0x004424f0`) extracts the world position,
  calls the absent current-level height/grid-pick path, and has no matching
  source boundary; `level_cell_of` is only a scalar rounding helper.
  `EntityVisualTypeTablesInit` (`0x00472710`) initializes large renderer
  tables and a COM object, while the port's `s_kinds` is a constant name
  lookup table used by `.ae` parsing. `EntityCreateByType`, `VehicleTypeInit`,
  and `RockMonsterTypesLoad` also remain absent: the source's entity allocator
  does not do type-specific setup, and its static name-to-kind tables do not
  load the original configuration records.
- The terrain queue pair `TerrainCellUpdateQueueFlush` (`0x004321a0`) and
  `TerrainCellPreviousStateTrack` (`0x00432200`) also lack matching port
  boundaries. The former scans a terrain-owned update queue, conditionally
  calls the cell-refresh path, then clears the queue; the latter snapshots a
  separate coordinate list and clears each cell's `0x100` flag. The port's
  dirty-cell queue (`terrain_cell_dirty_queue_push_and_clear`) stores a
  different set of cells for the renderer's min-distance color pass, while
  the timestamp table is a separate erosion dedup structure. Neither is the
  original update queue or previous-state tracker.
- The predug loader's explicit `FUN_0044a4b0 (stub)` call now has a matching
  `STUB` boundary, `predug_coordinate_pool_append_stub`, at `0x0044a4b0`.
  The full decompile confirms it appends an `(x,y)` pair to a capped
  20-entry pool; that pool and its consumers remain unported, so the source
  placeholder intentionally does no work. This is a stub annotation, not a
  claim that the predug coordinate behavior has been implemented. Totals are
  1,037 unique annotated addresses with 94 cited `ported` ledger rows lacking
  markers.
- The WinAPI reference-build `movie_open_path` placeholder now carries
  `STUB: LEGORR 0x0047ec90` for `AviPlayerCreate`. The movie API identifies
  that corpus entry as the AVI-by-path constructor; its body allocates the
  player record and initializes the frame retriever. The native implementation
  of `movie_open_path` performs the corresponding file/decoder construction,
  while the WinAPI reference configuration deliberately leaves this API as a
  stub because playback is expected through AVIFIL32 elsewhere. Do not assign
  the inner `AviFrameRetrieverOpen` address (`0x0047e800`) to this same
  function. Totals are 1,038 unique annotated addresses with 93 cited
  `ported` ledger rows lacking markers.
- A cursor/audio/render pass found no new direct boundaries. `CursorPositionUpdate`
  (`0x0047f010`) polls and clamps the global Win32 cursor, then computes a
  position delta; `input_capture_events` consumes event coordinates and keeps
  no matching delta state. `DirectSoundListenerSpatialUpdate` (`0x0047abc0`)
  updates a DirectSound listener through COM calls; the port's positional
  submit routine only queues sample requests, and its playback seam is inert.
  `PositionTrackerUpdateAll` (`0x0045d6d0`) walks a separate position-offset
  table and submits reposition operations; the mapped `ManagerTick`
  (`0x0045a730`) updates a different 12-slot smoothed-target table.
  `SurfaceBlitRect` (`0x0047dcb0`) rounds source/destination rectangles and
  calls DirectDraw Blt with flags; the native software blitters copy pixels
  into a framebuffer and do not implement that surface operation. Keep these
  addresses distinct from the nearby mapped input, manager, sound, and bitmap
  helpers.
- A startup, command, and save-menu recheck found no additional defensible
  boundaries. `StartLevelOverrideIfEnabledGet` (`0x00401c00`) returns the
  override string pointer only when bit `0x200` in the original engine feature
  word is set; the port's startup parser does not parse `_startlevel` or model
  that pointer/flag pair. `CmdChainPurge` (`0x00402310`) walks command records,
  retains and adjusts active entries, prunes completed ones, applies
  per-command callbacks, and caps/relinks the resulting chain; the current
  `cmd_chain_purge` clears and frees the entire chain. `SaveMenuThumbnailConfigLoad`
  (`0x00413a60`) loads six thumbnail resources and counts completed records
  from the save contexts; the port's save-state and frontend slot-row helpers
  split or omit those operations. `LoadMenuBuild` (`0x004161f0`) builds the
  save/load menu tree, localization, config labels, and callbacks, while the
  port uses a different frontend model and separate slot-row builder. Keep
  these addresses unassigned until the corresponding state and exact operation
  boundaries are represented in source.
- `TerrainCellOrientationToggleAndRebuild` (`0x0044edf0`) does not map to
  `terrain_block_fixup`. The original toggles a cell's orientation flag,
  rotates its four corner-block records, rebuilds surface vectors, and marks
  the affected neighboring cells. `terrain_block_fixup` only initializes
  edge markers while `terrain_create` builds the terrain table; it neither
  toggles cell state nor runs the original rebuild/refresh sequence. The nearby
  corpus comment documents the inspiration for that setup repair, not an
  implementation mapping.
- `PendingLevelName` (`0x0042f0c0`) is a corpus misnaming for a level-exit
  routine: it frees the current level's associated handles, resets several
  engine and UI globals, releases level resources, and invokes teardown across
  camera, entity, scene, input, and message subsystems before clearing the
  current-level global. `level_destroy` only frees the port-owned voxel,
  terrain, layer, and next-level allocations; frame-loop restart and shutdown
  code does not implement the routine's ordered subsystem reset. Do not map the
  decompiler's misleading `PendingLevelName` label to a level-name getter or to
  `level_destroy`.
- The option/save helpers also remain unmatched. `BrightnessSet`
  (`0x00414d30`) sets Direct3D material colors and power values; the port's
  `render_apply_brightness` multiplies finished framebuffer pixels instead.
  `OptionsApplyFromSlot` (`0x00415150`) calls the speed, brightness, and volume
  runtime setters from the current save context; the port restores option data
  into its `GameOptions` store and applies effects through separate frame/menu
  code. `SlotSummary` (`0x004152c0`) sums per-record scores and completion
  counts and formats a menu summary string; the port's `save_slot_state`
  exposes filled/completed/resume fields but has no matching summary operation.
  Do not assign these addresses to the related pixel pass or persistence
  helpers.
- `SceneActivityUpdateDispatch` (`0x00447c40`) is not implemented by
  `ai_set_activity` or `entity_activity_scene_state_update`. The original
  dispatches by entity type into scene/activity runners, updates the current
  and previous activity names and render state, and may update a status-text
  scene object. `ai_set_activity` only stores activity id/parameter/timer and
  mirrors the last id; the separate scene-state helper changes a flag word and
  clears one level field. Those are ported fragments around the call sites, not
  the original dispatch boundary.
- `SceneRenderLockToggle` (`0x00473b40`) remains an unported hook rather than
  an annotatable port function. The original changes a scene object's lock
  bit and calls the renderer's lock/unlock operation; `scene_visibility_hook_call`
  only records the handle and flag for tests, and the API wrappers that invoke
  it implement different flag and record-field operations. Do not map the
  test-observation hook to the DirectDraw lock routine.
- `SoundSubmissionRouteWrapper` (`0x00465050`) has no matching port wrapper.
  The original accepts a scene/entity handle, resolves its sound position,
  then forwards to `PositionalSoundSubmit` (`0x00465090`); the port implements
  the latter boundary directly as `sound_positional_sound_submit`, without the
  scene-handle resolution wrapper. Do not assign the wrapper address to the
  already-mapped positional submit function.
- `FUN_0041f940` was incorrectly grouped with camera-step hooks in `view.cpp`.
  The full decompile shows it sets or clears bit `0x20000` in the engine state
  word; it is not a camera function. The comment now records the correct
  identity. No current source function performs this bit setter, so it remains
  an unmapped ledger entry rather than being assigned to a view helper.
- The adjacent view attribution for `FUN_00431570` / `FUN_0044fe70` was also
  wrong: the former returns the current level block's `+4` terrain handle
  (despite its `CurrentLevelHeightGet` label), and the latter interpolates a
  terrain surface height from neighboring corner samples. The port's
  `terrain_height` reads one voxel byte, and `cam_surface_height_stub` has no
  result or parameters, so neither is a matching boundary. The view comments
  now describe the omitted interpolation and camera-height offset accurately.
- A follow-up view-comment audit corrected two false attributions:
  `FUN_004369f4` is the original camera tilt-axis setter, while
  `FUN_004798b0` is `RandomFloatRange`, which draws from the shared RNG and
  scales into a float interval. `main_view_key_step` groups pan, tilt, and
  zoom under one mode argument, so it is not a one-to-one implementation of
  the tilt setter; the port has no float-range helper corresponding to
  `RandomFloatRange`. The `view_tail` decay expression is unrelated. Both
  entries remain unmarked.
- The resource/UI wrapper review found no additional exact boundary.
  `FontRecordCreate` (`0x0047e050`) allocates and initializes a pooled 0x20-byte
  font record, including its source surface, dimensions, and two keyed color
  indices; the port decodes the bitmap into a `TextureSurface` and retains its
  palette keys there, without the font-record pool or creation operation.
  `BitmapSurfaceSave` (`0x0047e440`) only unwraps a DirectDraw surface and calls
  `BitmapFileWrite` (`0x0047c8f0`), which is already represented by
  `save_write_thumbnail`; the port writes its framebuffer directly and has no
  separate surface wrapper. `MenuRunLoop` (`0x00413b80`) is not a distinct
  source function: its menu loop is embedded in `LegorrrRun`, alongside level
  loading, gameplay, save handling, and shutdown. Do not assign the wrapper or
  broad frame-loop address to those narrower original entries.
- `terrain_activity_build_record_stub` is marked `STUB` at `0x004311d0`.
  The full decompile confirms this exact per-cell `Activity_Build` scene-record
  operation: its nonzero path allocates and links a scene record and updates
  the cell pointer table, while the zero path does not create or update one.
  The source has an explicit no-op seam at the loader spawn point and at the
  queued-work transition; the scene record, linked list, and cell pointer table
  are not modeled by the port, so this records a deliberate stub boundary, not
  an implementation claim. `cmd_chain_purge` (`0x00402310`) remains unmapped:
  the original selectively removes completed records and applies registry and
  cap handling, while the port currently clears and frees the entire entity
  chain.
- The four exact pool-callback wrappers are now explicitly marked `STUB`:
  `coordinate_pair_callback_dispatch` (`0x00402a40`),
  `argument_callback_dispatch` (`0x00402aa0`),
  `coordinate_callback_dispatch` (`0x00402b30`), and
  `coordinate_callback_dispatch_by_mode` (`0x00402b60`). Their declarations
  identify the exact corpus entries, and each original wrapper only marshals
  its arguments before entering the unported entity-pool callback walker.
  The port bodies are deliberate no-op seams, so `STUB` records those function
  boundaries without claiming the callback registry or callbacks are present.
  Current totals are 1,043 unique markers and 88 cited `ported` ledger rows
  without markers, with no duplicate addresses.
- `ui_mode_resource_positions_apply` is marked `STUB` at `0x00419100`. Its
  header identifies the exact original entry, and the source retains the
  original two-entry handle walk, but the corresponding surface-reposition
  call remains an explicit no-op seam and its position table has no ported
  loader. This makes the known function boundary visible without claiming the
  reposition behavior is implemented. Current totals are 1,044 unique
  markers and 87 cited `ported` ledger rows without markers, with no duplicate
  addresses.
- The remaining undocumented candidate pass found no defensible extra
  one-to-one boundary among the outstanding addresses. `MenuInputTick`
  (`0x00411a00`) is part of the per-menu input state machine, while
  `frontend_tick` combines the frame's front-end work; the port has no
  matching menu-object input routine. `AdvisorOverlayUpdate` (`0x0041b490`)
  and the `0x0040e7f0` / `0x0040e990` manager ticks are still unported.
  `VehicleTypeInit` / `RockMonsterTypesLoad` (`0x0042cda0` / `0x0042d100`)
  populate original config-backed records; the port's hardcoded type-name
  tables only resolve names and do not initialize those records.
  `FUN_00435300`, `FUN_00454600`, `FUN_0044f090`, `FUN_00450b60`, and
  `FUN_00450600` are original world-render or terrain-mutation passes with no
  corresponding render pipeline in the port; `FUN_00450890` is an accessor
  for a float at record offset `+0x10`, not the nearby surface-height logic.
  `FUN_004369f4` is a tilt-axis setter represented only as one mode of the
  generic, currently no-op `main_view_key_step`; `EntityRename` (`0x00437880`)
  remains in the absent entity rename layer. `ObjectiveTextOverlayLoad`
  (`0x004574e0`), `FUN_0045a460` (text positioning through the original
  renderer), `FUN_0046f920` (per-effect motion), `FUN_004820d0` (resource
  release), `DDiSceneDrawEnd` (`0x00477150`), and
  `TextBufferAppendFormat` (`0x00485230`) likewise have no equivalent source
  operation: objective text is not loaded, the render calls are TODOs, the
  effect/release paths are only counters, scene runtime is absent, and
  `main_hud_text` writes diagnostics to stderr. `RandomFloatRange`
  (`0x004798b0`) has no float-range wrapper, and `simtick_manager_set_metrics`
  combines the two distinct field getters `0x004897d0` / `0x004850e0` into a
  port hook, so neither can be mapped to it. Leave these entries unmarked
  until their actual source boundaries are implemented.
