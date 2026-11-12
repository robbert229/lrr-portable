# Finding 549 — Animation-object unit (GAME-004) rebuilt in original shape

Issue #513, under [ADR 0005](../adr/0005-original-shape-c-units.md). R1
(finding 527) relocated GAME-004 verbatim into `src/game/animation_object.cpp`.
This change rebuilds it as the C unit
[`src/game/animation_object.c`](../../src/game/animation_object.c), with the C
boundary header [`animation_object.h`](../../src/game/animation_object.h), and
deletes the `.cpp`. The unit is text `00406990`–`004070b0`.

An animation object is a 0x74-byte record. It holds:

- one activity container loaded from an `.ae` file;
- the names of six kinds of null frame, read from the `.ae` config;
- a cache of the nulls already looked up by name;
- three level-of-detail mesh lists (medium, high, first-person).

"Before" is the *match baseline* at `7ab9594c`: the head of #519 (GAME-001), which is stacked on #515. This change is stacked on #519.
"After" is this change. `make reccmp-check` and `make match-function` give the
same per-function scores.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, all 21 functions, 1,671 bytes) | 24.94% | **97.65%** |
| Exact functions | 2 | **15 of 21** |
| Mapped functions | 18 | 21 |
| Accuracy, full (`binary-progress.json`) | 56.66% | 57.11% |
| Accuracy, game-only | 52.01% | 52.52% |
| Exact-function code bytes | 66,839 | 67,562 |
| Order inversions, full (pairs) | 210,142 | 210,224 |

Byte weights are the original's code bytes, without the `0x90` fill. An
unmapped function counts as 0%. Pair inversions rise only because three more
functions are mapped. All 21 functions link in original order.

| Address | Function | Bytes | before | after |
| --- | --- | ---: | ---: | ---: |
| `00406990` | `entity_type_config_resource_row_load` | 620 | 13.27 | 97.60 |
| `00406c00` | `renderer_object_update_from_record_field64` | 35 | 92.86 | 92.86 |
| `00406c30` | `renderer_object_update_from_record_field68` | 35 | 92.86 | 92.86 |
| `00406c60` | `renderer_object_update_from_record_field6c` | 38 | 93.33 | 93.33 |
| `00406c90` | `animation_object_visibility_set` | 22 | 100.00 | **100.00** |
| `00406cb0` | `renderer_record_clone` | 96 | 23.53 | **100.00** |
| `00406d10` | `animation_object_time_update` | 24 | 11.76 | **100.00** |
| `00406d30` | `renderer_object_timing_value_adjust_by_count` | 108 | 42.55 | **100.00** |
| `00406da0` | `animation_object_time_get` | 17 | 11.11 | **100.00** |
| `00406dc0` | `animation_object_transform_set` | 40 | 13.95 | **100.00** |
| `00406df0` | `render_node_position_from_callback` | 56 | 37.68 | **100.00** |
| `00406e30` | `animation_object_handle_get` | 8 | 100.00 | **100.00** |
| `00406e40` | `activity_runner_update` | 127 | unmapped | **100.00** |
| `00406ec0` | `entity_payload_release_subslots` | 142 | 23.30 | 94.34 |
| `00406f50` | `animation_object_null_find` (new) | 46 | unmapped | **100.00** |
| `00406f80` | `activity_runner_state_read` | 48 | 21.54 | 81.82 |
| `00406fb0` | `renderer_cached_handle_40_get_or_create` | 40 | unmapped | **100.00** |
| `00406fe0` | `renderer_cached_handle_48_get_or_create` | 39 | 20.00 | **100.00** |
| `00407010` | `renderer_cached_handle_50_get_or_create` | 39 | 20.00 | **100.00** |
| `00407040` | `renderer_cached_handle_4c_is_not_one` | 74 | 58.33 | **100.00** |
| `00407090` | `animation_object_value_get` | 17 | 11.11 | **100.00** |

13 functions are newly exact. The three newly mapped functions had port
models with inert `/* FUNCTION */` comments, or none:

- `00406e40` in `entity.cpp`;
- `00406fb0` in `view.cpp`;
- `00406f50` in `view.cpp` (`renderer_object_subobject_find_apply`).

## The unit

### Record

`AnimationObject` (0x74 bytes, Win32 offsets) in `animation_object.h`:

| Offset | Field | Notes |
| --- | --- | --- |
| `+0x00` | `objectId` | |
| `+0x04` | `container` | the activity container |
| `+0x08` | `reserved[6]` | the unit never touches them |
| `+0x20` | `cameraNullName` | |
| `+0x24` | `cameraNullFrames` | |
| `+0x28` | `cameraFlipDir` | |
| `+0x2c`–`+0x3c` | `drillNullName`, `footStepNullName`, `carryNullName`, `throwNullName`, `depositNullName` | |
| `+0x40`–`+0x50` | the five cached nulls, in the same order | |
| `+0x54` | `cameraNulls[4]` | cached per camera |
| `+0x64`, `+0x68`, `+0x6c` | `polyMedium`, `polyHigh`, `polyFP` | the mesh lists |
| `+0x70` | `flags` | bit 0: the object owns its names |

Other details:

- A clone (`00406cb0`) copies the whole record. It clears bit 0, flattens
  the three mesh lists and copies the container.
- The remover (`00406ec0`) frees five of the six names. It leaves
  `depositNullName`, as the original does.

### State

- **`.rdata`**: 0.0 at `0049e054` and 1.0 at `0049e058`. Both are annotated
  statics.
- **`.data`**: the unit's 12 string literals, `004a192c`–`004a19c8`. Each
  has a `// STRING` marker on the line before it. That needed some calls split
  across lines (see "Tooling notes").
- **`.bss`**: none of its own. The `""` that `00406990` passes to `atoi` when
  `CameraNullFrames` is missing is at `004b8a14`. That address is already
  annotated in `config.c` (`s_config_angle_empty`, G98-006). The unit uses a
  literal, so this one operand differs (below).

`make unit-data-offsets`, GAME-004 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-004 | gaps-only (12, 0 unknown) | gaps-only (2, 0 unknown) | n/a (0, 1 unknown) fewer than two attributable items | - |

Before, the relocated `.cpp` annotated no data, so there was nothing to
attribute. The one unknown `.bss` item is `004b8a14`.

### Contracts

Port code that is not converted yet calls seven of the unit's functions
through the port's C++ declarations. They keep those declarations' parameter
types, and the unit casts to the record:

| Function | Port contract kept | Caller |
| --- | --- | --- |
| `entity_type_config_resource_row_load` | `void*` object and game name; the port's extra `row_kind`, which the unit ignores | `entity_building_type_config_record_parse` (`00407d60`) and the `00406990` ledger stub in `entity_config.cpp` |
| `renderer_object_timing_value_adjust_by_count` | `void*` record | `animation_loop_time_adjust` (`00408860`) |
| `entity_payload_release_subslots` | `EntitySlot*` | `entity_payload_teardown` (`00408940`) |
| `animation_object_visibility_set`, `animation_object_handle_get` | `const void*` | `entity.cpp`, `select.cpp` |
| `renderer_cached_handle_40_get_or_create`, `_48_` | `void*` | `entity.cpp` |

How the C++ side changed:

- Each of the seven declarations gains `extern "C"` in place. `entity.hpp` is
  already all `extern "C"`.
- `animation_object_handle_get` now returns `void*` (the container). Its
  callers already cast through `uintptr_t`.
- The time argument of `renderer_object_timing_value_adjust_by_count` is
  now `float`, as the original passes it. So is `animation_loop_time_adjust`'s
  `argument` (`00408860`), which passes it through. That function's listing
  is unchanged.

The callers in `building_model.cpp` are port delegations: the original
`00407d60`, `00408860` and `00408940` do not call the unit. No port code
calls those three functions, so the native port never reaches the unit
through them. Their own conversion should drop the calls.

The other functions take `AnimationObject*`. The port's older C++
declarations of the same names stay in `view.hpp` and `record.hpp`,
uncalled and commented, as C++-linkage overloads (entropy rule 1).

### Callees

The unit's callees get C linkage:

- **`extern "C"` added in place** where #519 had not already added it. Each
  function is declared in a C boundary header and paired in
  `test_c_boundaries.cpp`:

  | Header | Functions |
  | --- | --- |
  | `gods98/view.h` (from #519, unchanged) | four container-animation functions it already declares |
  | new `gods98/container.h` | five more `view.hpp`/`resource.hpp` container functions and the two ledger stubs |
  | new `gods98/model.h` | the four MeshLOD helpers (`model.hpp`, `record.hpp`, `resource.hpp`) |
  | new `gods98/texture.h` | `xf_texture_name_format` |

  `gods98/view.h`, `record.h` and `resource.h` are not extended because
  other C units include them. A first rebase onto #519 added six
  declarations to `view.h` and one to `record.h`. That moved `advisor.c`'s
  symbol counter and dropped two of its exact functions: `0x00401500`
  `vec_add` to 84.62% and `0x004016f0` `vec_dot` to 48.28% (finding 538,
  rule B). The two new headers, which only this unit includes, restore both.

- **Ledger stubs.** The stubs at `00472a40` and `004756d0` in `view.cpp` get
  `extern "C"` on their definitions, inside their namespace.
- **Two thin unmarked adapters** (ADR 0005 step 6), one of them from #519.
  The port's entries for `00472cd0` and `0044a7d0` have other contracts:
  - `entity_assembly_parse` parses an opened block;
  - the `0044a7d0` ledger stub drives a callback.

  The adapters take the original arguments and return NULL, because the port
  has no loader for either yet:
  - `container_load` (Container_Load) is #519's, in `gods98/entity.cpp` and
    `gods98/entity.h`. This change's identical copy in `view.cpp` was
    dropped on rebase;
  - `mesh_lod_load` (MeshLOD_Load) is at the end of `entity.cpp`, declared
    in the new `game/entity.h`.

### Shape notes

- **Literal strings and the annotation parser.** A `// STRING` marker must be
  on the line directly above its literal. With the marker above a
  multi-line statement, Reccmp dropped the whole function's annotation:
  `match-function` reported "no annotation for this address was loaded".
  The calls in `00406990` are split so that each literal sits on the line
  after its marker.
- **`00406d30`**: with `s_animation_object_zero != value`, the constant is
  loaded first, as in the original (94.74% the other way round).
- **`00406df0`**: the height goes through a local first. The original spills
  it to the dead `heightFn` slot and pushes it from there (61.90% inline).
- **`00406fe0`/`00407010`**: `if (name) { if (!cached) return cached =
  find(...); return cached; } return NULL;` is exact. Testing the name for
  NULL first is not (73.68%).

### Near misses

| Address | Score | What is left |
| --- | ---: | --- |
| `00406990` | 97.60 | The four calls to the two adapters, and `""` against `004b8a14`. The code is otherwise identical. |
| `00406c00`, `00406c30`, `00406c60` | 92.86 / 92.86 / 93.33 | One operand: the call to `00451bc0`. The port's `resource_list_update_matching_key` has an inert `/* FUNCTION */` comment, so the address is unmapped. |
| `00406ec0` | 94.34 | The three calls to `00451c50`, which is likewise unmapped (`owned_object_reference_chain_release`). |
| `00406f80` | 81.82 | The original saves `edi` in the prologue; MSVC 5.0 here saves it only on the path that needs it. |

More on `00406f80`:

- `tools/msvc_permuter.py` found 0 applicable rewrites.
- A rule-B sweep (finding 538: 0–156 inert declarations, step 4) is flat at
  81.82%, so the difference is not the counter's phase.
- Of 13 source variants, the best is `void* null = object->cameraNullName;
  if (null) {...} return null;` at 90.00%. It keeps the same prologue and
  shares the epilogue. It is not adopted: it is not a plausible original.

Making the inert comments at `00451bc0` and `00451c50` live would take four
more functions to 100%. That belongs to GAME-026's conversion.

## Entropy

The final `make reccmp-check` against `7ab9594c` reports:

- **no function below baseline**;
- **no score change outside the unit**;
- **no listing digest change outside the unit**, so no instruction-count
  change either.

How the superseded definitions were handled:

- **Deleted, no drift.** `view.cpp`'s port models of `00406f50` (two
  functions) and `00406fb0`. The gate shows no listing change.
- **Deleted, then restored.** Deleting `entity.cpp`'s port model of
  `00406e40` (`activity_runner_update`) changed three listings in
  `entity.cpp`. No function dropped:

  | Address | Function | Score | Instructions |
  | --- | --- | --- | --- |
  | `0x00404f50` | `object_filter_matches_action_target` | 25.00 = | 84 = (reordered) |
  | `0x00444fb0` | `ledger_stub_00444fb0_EntityTurnToVectorActivityIssue` | 25.56 → 26.04 | 178 → 179 |
  | `0x00447950` | `entity_directional_activity_start` | 33.87 = | 127 = |

  Following #509's lesson, the definition is restored verbatim but uncalled.
  It is renamed `activity_runner_update_superseded`, and its `entity.hpp`
  declaration too, because `entity.hpp` is `extern "C"` and the unit now
  defines that name. All three listings then equal `main`'s.

## Reccmp lint ratchet

The ratchet baseline goes from 4,126 to 4,123 entries.

Removed (5):

- `renderer_object_update_from_record`, the old file's static helper;
- the deleted `view.cpp` definitions: `renderer_object_subobject_find_apply`,
  `renderer_object_subobject_dispatch` and
  `renderer_cached_handle_40_get_or_create`;
- `activity_runner_update`, which is now renamed.

Added (2):

- `activity_runner_update_superseded`, the rename above;
- the adapter `mesh_lod_load`.

ADR 0005 step 6 prescribes unmarked adapters, so the update needed
`--allow-growth` for these two keys even though the total shrinks. **For
the owner:** please confirm that this is acceptable.

## Callers and tests

- No caller's source changes, apart from the `float` time argument of
  `animation_loop_time_adjust` above.
- The new [`test_animation_object.cpp`](../../src/game/test_animation_object.cpp)
  (`animation_object`) checks:
  - the Win32 layout, on 32-bit builds;
  - the cached null lookups and their name checks;
  - the container accessor;
  - a load whose `.ae` file is missing.
- `src/gods98/test_record.cpp`, which no build compiles, still uses the old
  `animation_object_handle_get` return type.

## Tooling notes

- **`convert_unit.py` refuses a relocated unit**, as #509 found. The scaffold
  ran after the `.cpp` entry was removed from `src/link_order.cmake` by hand.
- **The scaffold's boundary-test includes** added `game/entity.hpp`, whose
  steering declarations conflict with `steering.h` in one C++ translation unit.
  It was dropped; `type_config_ports.hpp` covers the row-load pair.
- **`match_function.py --candidate`** takes one address per run.
- **`make match-progress`** needs `pefile`; a worktree-local
  `build-venv` (`--system-site-packages`) provides it.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox) passes against
  `7ab9594c`:
  - 13 improved, 3 newly mapped, none below baseline;
  - link-order map OK.
- `make match-progress` regenerated the three files.
  `pr_match_progress.py check --base 7ab9594c --head .` reports them
  fresh.
- No exact match is lost: 801 exact addresses become 814.
- `make unit-data-offsets` gives the GAME-004 row above.
- `python3 tools/translation_units.py build` reports 0 conflicts.
- `make tools-test` passes: Meson parity, link order, and the lint (above).
- `animation_object.c` passes `gcc -std=gnu89
  -Werror=declaration-after-statement -fsyntax-only -Wall`.
- `cmake -S . -B build && cmake --build build && ctest --test-dir build
  --output-on-failure`: 37/37 pass, including `c_boundaries` and the new
  `animation_object`.

## Reproducing

    python3 tools/convert_unit.py GAME-004 --stem animation_object --dry-run   # after removing the .cpp entry
    make match-function SOURCE=src/game/animation_object.c
    make reccmp-check
    make match-progress
    make unit-data-offsets
