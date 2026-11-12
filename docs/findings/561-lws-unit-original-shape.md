# Finding 561 — LightWave scene unit (G98-020) rebuilt in original shape

Issue #536, under [ADR 0005](../adr/0005-original-shape-c-units.md); refs
#534 (finding 558's G98-020 follow-ups). #516 (finding 547) relocated G98-020
verbatim into `src/gods98/lws.cpp`. This change rebuilds it as the C unit
[`src/gods98/lws.c`](../../src/gods98/lws.c), with the C boundary header
[`lws.h`](../../src/gods98/lws.h). The port's earlier models of the unit's
functions move, unannotated, to the port tail
[`src/gods98/lws_port.cpp`](../../src/gods98/lws_port.cpp) (the approach of
#537, finding 556). The unit is text `004869f0`–`00488b50`: gods98's Lws.c,
the LightWave scene loader.

- **Lws_Parse** reads an `LWSC` version 1 `.lws` file into a node list. Each
  `LoadObject` or `AddNullObject` line starts a node; the lines after it give
  its motion keys, dissolve keys, parent, pivot and flags. Null objects named
  `SFX,<sample>,<frames>...` become sound triggers.
- **Lws_LoadMeshes** and **Lws_Clone** build a Direct3DRM frame per node and
  attach its mesh, cached by path with a shared directory as the fallback.
- **Lws_SetTime** interpolates the keys and dissolve levels, sets the mesh
  texture time and fires the triggers.

"Before" is the *match baseline* on `main` at `c0d131de` (#539). "After" is
this change.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, all 25 functions, 8,300 bytes) | 16.09% | **87.78%** |
| Exact functions in the unit | 1 | **15 of 25** |
| Mapped functions in the unit | 21 | 25 |
| Exact functions, full scope | 893 | 907 |
| Accuracy, full (`binary-progress.json`) | 60.47% | 61.10% |
| Accuracy, game-only | 56.27% | 56.97% |
| Exact-function code bytes | 76,390 | 78,560 |
| Unmapped original functions | 192 | 188 |
| Order inversions, full (pairs) | 32,339 | 32,392 |

Byte weights are the original's code bytes without the `0x90` fill. An
unmapped function counts as 0%. Pair inversions rise because four more
functions are mapped; all 25 link in original order.

| Address | gods98 name | Function | Bytes | before | after |
| --- | --- | --- | ---: | ---: | ---: |
| `004869f0` | Lws_Parse | `lws_parse` | 3269 | 7.69 | 94.25 |
| `004876c0` | Lws_Initialise | `lws_initialise` | 151 | 0.00 | 98.11 |
| `00487760` | Lws_Shutdown | `lws_shutdown` | 97 | 15.69 | **100.00** |
| `004877d0` | Lws_GetFrameCount | `resource_entry_id_get` | 11 | 100.00 | **100.00** (C, was asm) |
| `004877e0` | Lws_SetupSoundTriggers | `lws_sound_triggers_setup` | 429 | 20.64 | 98.47 |
| `00487990` | Lws_LoadMeshes | `lws_meshes_load` | 97 | unmapped | **100.00** |
| `00487a00` | Lws_Clone | `lws_clone` | 413 | 21.28 | 99.33 |
| `00487ba0` | Lws_SetTime | `lws_time_set` | 259 | 57.29 | **100.00** |
| `00487cb0` | Lws_FindPrevKey | `animation_track_find_keyframe_and_fraction` | 157 | 27.03 | **100.00** |
| `00487d50` | Lws_AnimateTextures | `lws_textures_animate` | 132 | 14.04 | 98.11 |
| `00487de0` | Lws_HandleTrigger | `lws_trigger_handle` | 226 | 21.88 | **100.00** |
| `00487ed0` | Lws_KeyPassed | `shared_object_sample_time_matches` | 229 | 40.58 | **100.00** |
| `00487fc0` | Lws_FindPrevDissolve | `keyframe_time_segment_fraction` | 165 | 42.34 | **100.00** |
| `00488070` | Lws_InterpolateDissolve | `lws_dissolve_interpolate` | 70 | 10.00 | 75.00 |
| `004880d0` | Lws_SetDissolveLevel | `lws_dissolve_level_set` | 156 | unmapped | **100.00** |
| `00488170` | Lws_Interpolate | `lws_interpolate` | 368 | 19.35 | 34.17 |
| `004882e0` | Lws_SetAbsoluteKey | `lws_absolute_key_set` | 708 | 14.00 | 23.88 |
| `004885c0` | Lws_LoadMesh | `lws_mesh_load` | 368 | 41.33 | 97.12 |
| `00488730` | Lws_SearchMeshPathList | `pair_table_find` | 82 | 45.16 | **100.00** |
| `00488790` | Lws_AddMeshPathEntry | `pair_table_append` | 44 | 31.82 | **100.00** |
| `004887c0` | Lws_CreateFrames | `lws_frames_create` | 318 | 11.11 | **100.00** |
| `00488900` | Lws_LoadNodes | `lws_nodes_load` | 145 | unmapped | **100.00** |
| `004889a0` | Lws_SetupNodeTransform | `lws_node_transform_setup` | 46 | unmapped | **100.00** |
| `004889d0` | Lws_Free | `lws_free` | 149 | 17.24 | **100.00** |
| `00488a70` | Lws_FreeNode | `lws_node_free` | 211 | 8.57 | 98.78 |

14 functions are newly exact; 4 are newly mapped (the port had no models of
them). The gods98 names are descriptive; the binary does not attest them.
`make match-function` scores `0x004876c0` as an effective match (100%\*) and
`0x004887c0` at 98.08%; the full build gives the table's figures. The
difference is the library data (`__mb_cur_max`, `_pctype`) and the operand
numbering, which the full report maps.

## The unit

### State

- **`.bss`**: one `static LwsGlobs lwsGlobs` at `00540838`, 0x402c bytes:
  the two 1,000-entry mesh path lists (`{path, mesh}`), the shared
  directory, their counts, the 100 static dissolve levels and their count,
  and the three callbacks Lws_Initialise installs.
- **`.rdata`**: `0049ea48` (1/180) and `0049ea4c` (π) are annotated
  `static const float`s. As literals MSVC folds `x * (1.0f / 180.0f) * pi`
  into one constant; the original multiplies by each in turn. They are
  declared π first: by rule A (finding 538) the later-declared operand is
  emitted first, and the original multiplies by 1/180 first. Their data
  order is therefore swapped (`make unit-data-offsets`, below). `0.0f`, `1.0f`
  and `0.5f` (`0049ea50`–`0049ea58`) are literals.
- **`0049fb48`** (IID_IDirect3DRMUserVisual, shared with G98-025) is an
  annotated static in the unit.
- **Strings**: each owned literal has a `// STRING` marker directly above it.
  A marker above a line whose literal is a macro (`LWS_HEADER`) made Reccmp
  drop the whole function (`NO_SUITABLE_NAME`), so the literals are written
  out. The pooled `"%s.%s"`, `"r"`, `","`, `"\\"`, `"%s%s"` and `"%s_%0.2i"`
  have no marker.

`make unit-data-offsets`, G98-020 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| G98-020 | partial (18, 1 unknown) gap 9 first 004ab670: 78/80 | partial (2, 0 unknown) gap 1 first 0049ea4c: 4/-4 | n/a (1, 0 unknown) fewer than two attributable items | - |

Unit summary: full 0, gaps-only 2, partial 28, n/a 195.

### COM and Mesh.c

The unit declares the method-table slots it calls: IDirect3DRMFrame3
(`Release`, `SetName`, `AddTransform`, `GetParent`, `GetTransform`,
`GetVisuals`), IUnknown, IDirect3DRMUserVisual (`GetAppData`) and
IDirect3DRM3 (`CreateFrame`). A mesh is the user visual's app data.

Mesh.c (G98-015) is still spread over port files. New C boundary header
[`gods98/mesh_unit.h`](../../src/gods98/mesh_unit.h) (`gods98/mesh.h` is
Container.c's, from #537):

- `lwo_model_entry_load` (Mesh_Load, `004809e0`), `pointer_value_get`
  (Mesh_GetGroupCount, `00482210`): `extern "C"` in place.
- `screen_effect_item_opacity_set_target` (Mesh_SetGroupMaterialValues,
  `00483420`): was `static` in `record.cpp`; now `extern "C"` at its
  definition, no other change.
- Three unannotated adapters in `lws_port.cpp` (ADR 0005 step 6):
  `mesh_clone` (Mesh_Clone, `00480900`: the port's entry takes the mesh only,
  and its type is in namespace `lego`), `mesh_texture_time_set`
  (Mesh_SetTextureTime2, `00482040`: the port's entry, a view.cpp ledger stub
  with C++ linkage, takes an integer frame offset) and `mesh_remove`
  (Mesh_Remove, `004820d0`, not modelled). Each costs the call operand
  (`0x00487a00`, `0x004885c0`, `0x00487d50`, `0x00488a70`).

Other callees: [`gods98/files_open.h`](../../src/gods98/files_open.h)
(File_Open, File_Close, File_GetLine, `extern "C"` in `resource.hpp`);
[`gods98/main.h`](../../src/gods98/main.h) for Main.c's `lpD3DRM`
(`00505914`), whose stand-in `s_main_d3drm` is an annotated global in
`lws_port.cpp` (the port does not own Main.c's renderer); `utils.h`,
`sound3d.h` and `crt/crt.h` unchanged. `realloc` maps to the port's
`_realloc` model for non-MSVC builds in `crt/vc5names.h`, in that header's
non-MSVC branch only.

### Contracts: six original functions keep port names

Port code outside the unit calls six of the unit's functions whose contracts
match the original's. They keep the port's names; their C++ declarations
gain `extern "C"` in place:

| Function | Declared in | Callers |
| --- | --- | --- |
| `resource_entry_id_get` | `record.hpp` | `test_record.cpp` |
| `animation_track_find_keyframe_and_fraction` | `view.hpp` | `view.cpp` wrapper, `lws_port.cpp` |
| `shared_object_sample_time_matches`, `keyframe_time_segment_fraction` | `scene_resources.hpp` (`namespace lego`) | `lws_port.cpp` |
| `pair_table_find`, `pair_table_append` | `crt.hpp` | `lws_port.cpp`, `test_crt.cpp` |

`lws.h` declares them with the port's `const void*`/`uint16_t*` types (the
boundary test checks them against the C++ headers). `lws.c` hides those
declarations (`LEGORRR_LWS_UNIT`) and defines them with the original's
types, under its own prototypes; the calls link by name. The port's NULL
guards are gone, so the tests' NULL checks are removed. The mesh path
count is `unsigned int`, not `unsigned long`: the port passes an `int*`,
and `unsigned long` is 64-bit on the native host (`test_crt` crashed).

### Port tail

`lws_port.cpp` keeps, unannotated, the port's models of the functions whose
port callers need the port's contract:

- `shared_object_directory_scan_configure`,
  `shared_object_scene_entry_slot_table_expand`, `shared_object_clone`,
  `animation_track_keys_interpolate`: called only from `view.cpp`'s wrappers,
  which this issue may not edit;
- `ledger_stub_00487ba0_*` (Lws_SetTime), `ledger_stub_00487d50_*`,
  `shared_object_timed_sound_event_update`,
  `lego::shared_object_float_track_interpolate_apply`,
  `scene_node_transform_matrix_apply`: `com_object_tree.cpp`'s timed hooks
  and `scene_resources.cpp`. They guard every interface slot and forward
  dissolve levels to a hook; the original calls Direct3DRM unguarded;
- `model_resource_resolve_or_load` and `lego::scene_resource_string_tables_free`:
  the port's parsed-Model cache (`model.cpp`, `entity.cpp`, `engine.cpp`).

The four unused ledger stubs (`004869f0`, `004887c0`, `004889d0`,
`00488a70`) are deleted. **For follow-up (needs `view.cpp`):** the
`view.cpp` wrappers are never called; deleting them would let the first
four port-tail functions go.

The native port never reaches `lws.c`: native creature scenes load through
`lwsc_scene.cpp` and `lwsc_miniwin.cpp` (#532), which this change does not
touch.

## Shape notes

- **`0x004869f0`**: the header's second line is read at `&line[len]` with
  `len = strlen(line) + 1`, past the terminator. `Lws_Info.flags` is a byte
  (`or byte ptr [x+0x34], 1`). The static `ObjDissolve` level is stored,
  pointed to and then the pointer is cleared, as the original does.
  `strcmp` operand order follows the original per keyword.
- **`0x00487fc0`**: exact only with the frame kept in a local and the
  previous frame read back through `node->dissolveFrame[*prev]` after
  `*prev` is stored, and with `(float)frame - prev` (not an integer
  subtraction).
- **`0x00487cb0`**: `lower` declared before `upper`, `(lower + upper) / 2`
  and a `(float)` on the next key's frame; other orders are effective
  matches only.
- **`0x00487ed0`**: the frame count is converted to `float` right after the
  call, `lastTime > time` picks the maximum, and the `< 0.5f` arm comes
  first. This fixes finding 558's two `compare` swaps.
- **`0x00487a00`**: the parent frame is a function-scope local; in the loop
  block its stack slot swaps with the visual's (95.30% → 99.33%).
- **`0x004889d0`**: Lws_Free ends with a tail call on `clonedFrom`, which
  MSVC turns into the loop.
- **`0x004877e0`**: `argc` is an `unsigned long` (`add eax, -2`).
- **`0x004887c0`**: the face-camera matrix is zeroed, then its translation
  row set to zero again, then the diagonal set to one.

## Near misses

| Address | Score | What is left |
| --- | ---: | --- |
| `0x004869f0` | 94.25 | the pooled `"%s.%s"`/`"r"` operands; register choice around the node-list `realloc` and the dissolve clamps |
| `0x004877e0`, `0x00487d50`, `0x00487a00`, `0x004885c0`, `0x00488a70` | 97–99 | a pooled `","`, the adapter call targets above, and Reccmp's per-side operand numbering after them |
| `0x00488070` | 75.00 | the original loads the current level onto the x87 stack before the branch and keeps it for both calls; 13 shapes (locals, `double`, one call with a `level` local: 75%) either store it or reload it |
| `0x00488170` | 34.17 | the expression trees match the original (checked with a symbolic x87 trace of both listings); the original addresses the scale and rotation keys through the rotation-key pointers (`add eax, 0xc`) and keeps `&hpb` in `edi`. Inline Maths.h helpers give four pointer registers (24%). A rule B sweep shows two codes, period 256 |
| `0x004882e0` | 23.88 | the trees almost match: same six `fsin`/`fcos`, the same products. The original keeps negations as `fchs` + `fadd` (MSVC folds `-a*b + c` into `fsub`), has three more stack temporaries, loads the pivot into locals, and in the face-camera arm stores each scaled diagonal element before loading the next scale component. 16 variants, `msvc_permuter.py` (no candidates) |

**Finding 558's follow-ups:** `0x00487ed0`'s two compare swaps are gone
(100%). `x87_operand_swaps.py --loose` still reports `0x004882e0`: two
`fld [scale+4/8]; fmul [mat]` against the reverse, in the face-camera arm.
Writing the products in either order leaves the code unchanged: the recomp
loads all three scale components before the first store, so the operand
order is scheduling, not declaration order. Not fixed.

Removing the four Maths.h vector helpers, which no function uses any more,
moved `0x00488170` from 34.31 to 34.17 and `0x004882e0` from 27.66 to 23.88
(declaration count, rule B). They were removed anyway: unused declarations
kept for their count are pads (ADR 0005 entropy rule 3).

## Entropy

`make reccmp-check` against `c0d131de`: **OK**, no function below the
baseline; link-order map OK. 21 improved and 4 newly mapped in the unit.

Outside the unit, one score moved and no listing digest changed:

| Function | File | Score | Label |
| --- | --- | --- | --- |
| `0x004897e0` ddiscene_timed_hook_dispatch | `gods98/com_object_tree.cpp` | 10.07 → **17.45** | improved; source unchanged. Its calls to `0x00487ba0` now name an unannotated port-tail function; the digest normalisation hides call targets |

No instruction-count change outside the unit. All header edits are
`extern "C"` in place (`crt.hpp`, `model.hpp`, `record.hpp`,
`resource.hpp` ×3, `scene_resources.hpp` ×2, `view.hpp`) or confined to
`vc5names.h`'s non-MSVC branch; none adds or removes a declaration in a
matching-build header. `record.cpp` loses one `static`.

## Reccmp lint ratchet

4,117 → 4,128 entries (+12, −1). **For the owner:**

- Removed (1): `animation_track_find_keyframe_and_fraction`, now annotated
  in `lws.c`.
- Added (12):
  - the three Mesh.c adapters (`mesh_clone`, `mesh_texture_time_set`,
    `mesh_remove`), which ADR 0005 step 6 requires unannotated;
  - the port tail's nine models (listed above), previously annotated at the
    addresses `lws.c` now holds. Four of them are reachable only from
    `view.cpp`'s uncalled wrappers.

The update needed `--allow-growth`. A first version also added a
`engine_heap_realloc` definition; it was replaced by a `realloc` macro in
`vc5names.h`, which adds no key.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK; link-order map
  OK.
- `make reccmp-baseline-update` and `make binary-progress-compare` (host,
  with `pefile` in a worktree venv) regenerated the match baseline, listing
  digests and `binary-progress.json`.
- `make unit-data-offsets`: the G98-020 row above.
- `python3 tools/translation_units.py build`: 0 conflicts;
  `tools/binary_progress_inputs.txt` regenerated.
- `make tools-test`: lint, Meson parity, link order, x87 swap tool tests OK.
- `lws.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only -Wall` (one `-Wformat` note on `"%0.2i"`, the original's
  format).
- `cmake -S . -B build && cmake --build build && ctest --test-dir build
  --output-on-failure`: 37/37, including `c_boundaries` (now pairing
  `lws.h`, `mesh_unit.h`, `files_open.h` and `main.h`) and `crt`.

## Tooling notes

- `convert_unit.py` still refuses a relocated unit; it was run in a
  scratch export with the `.cpp` entry removed from `link_order.cmake`
  (findings 553, 554).
- A `// STRING` marker must sit above a line with a string literal. Above a
  macro it drops the whole function from Reccmp.
- `sweep.py` cannot judge `0x004869f0` or `0x004882e0`: their fingerprints
  change at every step (the annotated `static const` names carry the counter).

## Reproducing

    python3 tools/convert_unit.py G98-020 --stem lws   # in a copy without the .cpp entry
    make match-function SOURCE=src/gods98/lws.c
    make reccmp-check
    make match-progress BINARY_PROGRESS_PYTHON=build-venv/bin/python3
    make unit-data-offsets
