# 642. The gate takes the original's link mode: `/DEBUG /OPT:REF /OPT:NOICF`

Issue #562, closing it. Follows finding 637 (the plan and the switch
condition), finding 641 (the 27 drops left after #738) and findings 634 and
638–640 (switching callers to the annotated originals). Measured on `main`
`c46f8e97` with full matching builds before and after.

## Summary

1. **The last live-caller drops are gone.** Finding 641 left 13 annotated
   functions (10 exact) that `/OPT:REF` removed because their live original
   callers still called port substitutes. Each now has its original caller in
   the matching build ([per caller](#the-13-live-caller-drops)). Two of them
   turned out to be the callers tool's nearest-annotation attribution again
   (finding 640): their real callers were unrebuilt functions, now rebuilt.
2. **The gate links with `/DEBUG /OPT:REF /OPT:NOICF`** (`CMakeLists.txt`).
   `/OPT:NOICF` because the original folded no identical functions
   (finding 637); it also keeps the 11 functions LINK's implicit `/OPT:ICF`
   folds today.
3. **Two `/INCLUDE` options keep the annotated code nothing references.**
   - LegoObject.c's `00438390` (`legoobject_global_animation_state_get`,
     exact) is unreferenced in the original too: the floor. The original kept
     it because its objects had no `/Gy` (finding 576); `/INCLUDE` keeps it
     the same way.
   - crt.cpp's `004907c0` (`string_prefix_reserve`, exact) is a stand-in for
     LIBC cvt.obj's static `__shift`. The matching build links that LIBC
     member, and `__cftoe`/`__cftof` call it, but Reccmp cannot name it: a
     LIBC static has no PDB symbol, and `// LIBRARY: ... SYMBOL` annotations
     of LIBC statics (40 of them in crt.cpp) never map. Without the
     `/INCLUDE` the exact stand-in leaves the report. This one is a
     `Deviation (scoring only)`; owner #447.
4. **No score fell.** Against `main`'s baseline the full report has 19
   functions higher, 17 newly mapped (all new original-shape functions) and
   none lower or lost. On this tree, every one of the 2,783 mapped functions
   scores the same under the old link mode (`/DEBUG`, so `/OPT:NOREF`) and the
   new one. The game-only report loses `0048e1e0`, which moved from a port
   stand-in to LIBC's `_vsprintf` and so to the library scope (below).
5. **The image now ends at `0x7f5000`**, below `0x800000` (old link mode on
   this tree: `0x876000`; `main`: `0x875000`). `.text` is 842,293 bytes
   against the original's 642,549.
6. **Finding 584's plain-immediate wrapper (#604) changes no score on this
   tree.** A Reccmp run with `RECCMP_PLAIN_IMMEDIATES=0` gives the same 2,783
   and 2,537 scores, and its `check` passes: the rebuilt image no longer
   covers `0x800000`, `0xff0000` or `0x1000000`. It stays installed as the
   safety net, since a larger `.data` or `.bss` can cover them again.
7. **The gate now fails on a function that leaves the report.** Under
   `/OPT:REF` an annotated function that loses its last live caller leaves the
   image, so `match_baseline.py check` counts "newly unmapped" as a drop.
   `opt_ref` now measures what the gate's `/OPT:REF` removes against a
   `/DEBUG /OPT:NOREF` relink ([tooling](#tooling)).

| Measure | `main` `c46f8e97` | This change, old link mode | This change (gate) |
| --- | ---: | ---: | ---: |
| Gate link | `/DEBUG` (`/OPT:NOREF`) | `/DEBUG` (`/OPT:NOREF`) | **`/DEBUG /OPT:REF /OPT:NOICF`** |
| Exact-function code bytes | 303,156 (47.18%) | 306,639 (47.72%) | **306,639 (47.72%)** |
| Accuracy, full / game-only | 96.48 / 96.20 | 96.64 / 96.37 | **96.64 / 96.37** |
| Mapped / exact (full) | 2,766 / 2,259 | 2,783 / 2,288 | 2,783 / 2,288 |
| `opt_ref` annotated removed (exact), old measure | 27 (23) | 12 (11) | |
| ⤷ with `/OPT:NOICF` | 14 (11) | 2 (2) | |
| ⤷ with `/OPT:NOICF` and the two `/INCLUDE`s | | 0 | **0** (new measure) |
| `.text` removed against `/OPT:NOREF` | 519,341 bytes, 3,964 symbols | 519,645 bytes, 3,938 symbols | 519,645 bytes, 3,938 symbols |
| Image end | `0x875000` | `0x876000` | **`0x7f5000`** |
| `.text` virtual size | | 1,361,813 | **842,293** |

The old measure is the `/DEBUG /OPT:NOREF` link against a no-`/DEBUG`
relink with LINK's default `/OPT:REF` (and so `/OPT:ICF`), as `main` records
it. On this tree it removes 10 folded functions plus the two above; with
`/OPT:NOICF` only the two remain (`main`: 13 folds, 13 live-caller drops and
the floor). The new measure is the gate against its `/DEBUG /OPT:NOREF`
relink: no annotated function lost.

## Stacked on #744 (#742 → #743 → #744)

Rebased onto `cleanup/453-481-deviations-d3drm` (`a41895b6`) and measured
again with full runs. #740 (finding 644) had already renamed Lego.c's
Main_Setup3D adapter in `lego_unit.c`, so this change's copy of that alias in
`lego_unit_imports.h` is dropped; the other 12 switches are unchanged.

| Measure | #744 | This change on #744 |
| --- | ---: | ---: |
| Gate link | `/DEBUG` (`/OPT:NOREF`) | **`/DEBUG /OPT:REF /OPT:NOICF`** |
| Exact-function code bytes | 308,143 (47.96%) | **310,994 (48.40%)** |
| Accuracy, full / game-only | 96.52 / 96.24 | **96.68 / 96.42** |
| Mapped / exact (full) | 2,767 / 2,271 | 2,784 / 2,299 |
| `opt_ref` annotated removed (exact) | 24 (20), old measure | **0**, new measure |
| Image end | `0x875000` | **`0x7f5000`** (old link mode: `0x876000`) |

- Against #744's baseline the full report has 18 functions higher, 17 newly
  mapped and none lower or unmapped. The game-only report again loses only
  `0048e1e0` (now LIBC's `_vsprintf`).
- Every one of the 2,784 full and 2,538 game-only scores is the same under
  the old link mode (a Reccmp run on the `/DEBUG /OPT:NOREF` relink) and the
  switched gate.
- With `RECCMP_PLAIN_IMMEDIATES=0` every score is again the same and the
  check passes: the image ends 45,056 bytes below `0x800000`. The wrapper
  could be retired, but that touches its guard, `make pe-audit`'s check,
  `match_function.py` and their tests, so it stays for a follow-up.

## The 13 live-caller drops

Each row is one original call site whose rebuilt caller called a port
substitute. "After" is the full report on the switched gate.

| Removed (exact) | Original caller | What the rebuild called | Change | After |
| --- | --- | --- | --- | --- |
| Main.c `00478330` direct3d_render_preferences_apply (E) | Lego.c `004334f0` | `lego_main_setup3d_adapter` (lego_port.cpp, does nothing) | `lego_unit_imports.h` renames the adapter to the original in the matching build. The original pushes seven arguments and Main_Setup3D reads five; lego_unit.c's own seven-argument declaration renames one for one (rule B: the target is not declared in the unit, so the counter does not move) | `004334f0` 99.51 → **100** |
| `0045d7e0` LevelIdentifierRemapTableLoad, Message.c `00451fd0` (E) | Lego.c `00429890` lego_level_load | `level_identifier_remap_table_load_original` (lego_port.cpp's port loader) | `0045d7e0` rebuilt in original shape in `level_id_remap.cpp` (matching build; the port's ledger stub stays native-only) as `level_remap_table_load`, aliased in `lego_unit_imports.h`. util.cpp's table and count get their original addresses (`00555be0`, `00555d20`) | `0045d7e0` 18.18 → **100**, `0045d870` 70.83 → 75, `00429890` 94.80 → 94.85 |
| `004593e0` ObjectRecall_Save | FrontEnd.c `00417bd0` front_save_write_save_file | `front_object_recall_save_adapter` (frontend_port.cpp, does nothing) | `004593e0` rebuilt in original shape as `object_recall_save` (object_state_file.cpp, matching build) on the annotated live table `004ffe50`/`004ffe54` and a new `004ffe64`; `frontend_unit_link_names.h` renames the adapter, and frontend_port.cpp compiles the adapter out of the matching build. The port's context-based `osf_write` stays, now `PORT-ONLY` | `004593e0` 26.09 → **97.56** (its 004592e0 call goes through Objective.c's no-argument adapter: the port's `osf_snapshot` takes the game context), `00417bd0` 99.38 → **100** |
| Lego.c `00432360`, `004323c0` (E, E) | GAME-008 `00409d50` | `entity.cpp`/`level.cpp` port models | `00409d50` rebuilt in original shape (`entity_cell_effects.cpp`, matching build; the port's model stays native) | 16.00 → **100** |
| LegoObject.c `00438cd0`, `00438d30`, `00438d90` (E, E, E) | GAME-008 `00409ff0`, not `00409f20` | `entity.cpp`'s `entity_destroy_tracked_pointer_cleanup` | The callers tool attributed the three calls to `00409f20`, the nearest annotated function. The real caller is `00409ff0` (LegoObject_Destroy's tracked-pointer pass), unannotated until now. It is rebuilt in original shape under the port's C name, which LegoObject.c, Message.c and the port already call; entity.cpp compiles its model out of the matching build. `00409f20` is rebuilt as well | `00409ff0` new, **100**; `00409f20` 34.52 → **100**; `0043bc40` 99.11 → 99.33; `00452cc0` 97.92 → **100** |
| AITask.c `004033f0`, `00403430` (E, E) | NERPs's block-pointer callback `00456d00`, not `00456cb0` | not rebuilt: reached only through the `.data` pointer `004a676c` | Also an attribution artefact (finding 640's follow-up). `tutorial_script.c` now has the callback `00456d00` (13 actions), its two object searches `00457060` and `004570d0`, the pointer `004a676c`, NERPs_SetTutorialPointer `00457170`, and the 12 table rows that use them (`00454bb0`, `00457130`–`004574a0`); their 12 lines leave `tutorial_script_port_functions.h` | 16 new: 14 at **100**, `00456d00` 99.30, `00457170` 95.12 |
| CRT `0048e1e0` FormattedTextBufferBuild | 8 callers (`00410320`, `0046b520`, …) | LIBC's `_vsprintf`, already | The annotation moves to the LIBC member: `string.cpp`'s stand-in gets `// LIBRARY: LEGORR 0x0048e1e0 SYMBOL _vsprintf`. `_vsprintf` is public, so Reccmp maps it | `_vsprintf` 43.01 → **100**; 8 callers rise, 7 of them to 100 |
| CRT `004907c0` string_prefix_reserve (E) | LIBC `__cftoe`, `__cftof` | LIBC's static `__shift`, already | Kept by `/INCLUDE` (summary item 3) | 100, unchanged |

Rule B (findings 538 and 558) was checked for every edited unit with
`make match-function` over the whole unit, before and after:
- `tutorial_script.c`: no pre-existing function changes;
- `lego_unit.c`: the two aliases rename declarations one for one, and no
  function of the unit changes in the full report apart from the two
  callers;
- `object_state_file.cpp`: placed at `004593e0`'s address order, the new
  function moved the counter under the port's `osf_read` (20.28 → 9.71). It
  now follows `osf_read` in the file, and `osf_read` is unchanged.

Two shapes in the new code are worth recording:
- `00456d00` keeps its four object searches as block-scoped locals. One
  function-scope search lets MSVC 5.0 merge the identical "count the
  objects" tails of cases 10 and 13; sibling block-scope locals share their
  stack slot and keep the cases apart, as the original does.
- In `00457170` the registry lookup's output is a block-scope local: MSVC
  then stores it in the dead `on` parameter's slot, as the original does
  (`sub esp, 0x34` instead of `0x38`). `00457060` and `004570d0` match only
  with their block position as one `NERPs_Point2I`, not two `int`s.

## Tooling

- **`opt_ref_link.py relink --noref`** relinks the gate with `/DEBUG` kept
  and every `/OPT` option replaced by `/OPT:NOREF` (`build-reccmp/opt-noref`).
  `make opt-ref-link` runs it after the no-`/DEBUG` relink, which stays the
  whole-binary link (`binary-progress.json`'s `opt_ref.image`).
- **`measure`** detects the switched gate (`/OPT:REF` in its `link.txt`) and
  then compares the `--noref` relink with the gate. The removed annotated
  functions are the gate report's `FUNCTION` annotations that Reccmp could
  not map: `entities.json` entries with an original address and no rebuilt
  one. Reccmp keeps such an entry for an annotation whose function is not
  in the PDB (checked on a relink without the floor's `/INCLUDE`: it lists
  `0x438390`). `LIBRARY` statics and Reccmp's other original-only entities
  are left out. Their exactness is unknown, so `removed_exact` is null.
  `opt_ref.reference` names the comparison and `opt_ref.unmapped_annotations`
  lists them.
- **`callers`** takes the same removed set under the switched gate, listed
  under `(not linked)`, and still traces the original callers and causes.
- **`match_baseline.py check`** fails on newly unmapped functions as on
  drops.
- Tests: `test_opt_ref_link.py` covers the `--noref` command, the unmapped
  annotations, the row translation and the link-mode check.

## What is left

- **The floor and the stand-in.** `00438390` stays until the objects lose
  `/Gy` (#553). `004907c0` stays until #447 retires crt.cpp's stand-ins or
  Reccmp can name LIBC statics (a map-based lookup in the wrapper would also
  map the 40 `LIBRARY` statics).
- **`004593e0`** reaches 100 once Object Recall's `004592e0` is rebuilt with
  its original no-argument contract (GAME-030's conversion).
- **`00456d00`** (99.30): `lea eax, [block]` then `[eax+8]`, against the
  rebuild's `lea eax, [block+8]`, in the path-clear case. **`00457170`**
  (95.12): the original loads the level width before the block pointer's `y`
  for `imul`; four expression orders give the same rebuilt order.
- **Native.** Unchanged in behaviour: every rebuilt function is
  matching-build only or replaces a port model only there. The native build
  builds and its 37 tests pass.

## Files

- `CMakeLists.txt`: the link mode and the two `/INCLUDE`s.
- `src/game/entity_cell_effects.cpp`, `entity.cpp`: GAME-008's `00409d50`,
  `00409f20`, `00409ff0` in original shape (matching build).
- `src/game/tutorial_script.c`, `tutorial_script_imports.h`,
  `tutorial_script_port_functions.h`: NERPs's tutorial callback, its
  searches, `00457170` and 12 rows.
- `src/game/level_id_remap.cpp`, `util.cpp`, `lego_unit_imports.h`:
  `0045d7e0` and the Main_Setup3D alias.
- `src/game/object_state_file.cpp`, `frontend_unit_link_names.h`,
  `frontend_port.cpp`: `004593e0`.
- `src/gods98/string.cpp`: the `_vsprintf` annotation.
- `tools/opt_ref_link.py`, `tools/match_baseline.py`, `Makefile`, tests and
  docs (`docs/building.md`, `tools/README.md`, `GLOSSARY.md`, ADR 0006).
- `docs/coverage/`: match baseline, listing digests and binary progress.

No other thread's file was edited: `lego_unit.c`, `legoobject_unit.c`,
`src/crt/`, the import-library order and the `Deviation (scoring only)`
declarations are untouched. `lego_unit_imports.h` (two `#define`s) is
`lego_unit.c`'s imports header, not its function bodies.

## Reproducing

```sh
make reccmp-check                                   # no drop, nothing unmapped
make match-function SOURCE=src/game/tutorial_script.c
make opt-ref-link && make opt-ref                    # 0 annotated removed
make opt-ref-callers OPT_REF_ARGS=--summary          # empty
RECCMP_PLAIN_IMMEDIATES=0 python3 tools/reccmp_full_diff.py "$(command -v reccmp-reccmp)" \
  --target LEGORR --json /tmp/nopi.json              # same scores
```
