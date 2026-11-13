# Finding 616 — AnimClone.c (G98-025) rebuilt in original shape, with a port tail

Issue #673, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
G98-025 is text `004894c0`-`00489c30`. It was relocated verbatim into
`src/gods98/com_object_tree.cpp` (#496). All 11 functions were
low-confidence, and one (`004897d0`) had no source. This change:

- confirms the extent and names the unit **AnimClone.c** (`inferred`);
- rebuilds it as the C unit [`animclone.c`](../../src/gods98/animclone.c)
  ([`animclone.h`](../../src/gods98/animclone.h),
  [`animclone_link_names.h`](../../src/gods98/animclone_link_names.h));
- moves the port's models to the port tail
  [`com_object_tree_port.cpp`](../../src/gods98/com_object_tree_port.cpp),
  unannotated.

"Before" is the *match baseline* on `main` at `8abe8b6b` (#669). "After" is
this change on the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (13 functions, 1,795 bytes, byte-weighted; `004897d0` counts 0 before) | 37.06% | **99.86%** |
| Exact functions | 2 (the two callbacks) | **12 of 13** |
| Mapped functions | 12 | **13** (`004897d0` written) |
| Exact-function code bytes (`binary-progress.json`) | 196,138 | **197,765** |
| Accuracy, full / game-only | 87.16% / 85.90% | 87.41% / 86.17% |
| `opt_ref` annotated, full: removed / exact removed | 428 / 207 | 423 / 214 |
| `opt_ref` annotated, game-only | 421 / 200 | 416 / 207 |
| Reccmp lint ledger (entries) | 1,080 | **1,075** |

The names follow OpenLRR's `Material_Create` and `AnimClone_*`, in the
repository's snake case. They are `inferred`. "Link name" is the name the
definition takes in the matching build (see [Link names](#link-names)).

| Address | Original | Function | Link name | Bytes | Before | After |
| --- | --- | --- | --- | ---: | ---: | ---: |
| `004894c0` | Material_Create | `material_create` | `scene_material_create` | 83 | 15.38 | **100.00** |
| `00489520` | AnimClone_Register | `anim_clone_register` | | 150 | 73.68 | **100.00** |
| `004895c0` | AnimClone_RegisterLws | `anim_clone_register_lws` | | 150 | 73.68 | **100.00** |
| `00489660` | AnimClone_Make | `anim_clone_make` | | 236 | 28.57 | **100.00** |
| `00489750` | AnimClone_Remove | `anim_clone_remove` | `scene_object_owner_link_release` | 115 | 49.57 | **100.00** |
| `004897d0` | AnimClone_IsLws | `anim_clone_is_lws` | | 8 | unmapped | **100.00** |
| `004897e0` | AnimClone_SetTime | `anim_clone_set_time` | `ddiscene_timed_hook_dispatch` | 253 | 17.45 | **100.00** |
| `004898e0` | AnimClone_FrameCountCallback | `anim_clone_frame_count_callback` (static) | | 12 | 100.00 | **100.00** |
| `004898f0` | AnimClone_SetupFrameArrayCallback | `anim_clone_setup_frame_array_callback` (static) | | 29 | 100.00 | **100.00** |
| `00489910` | AnimClone_WalkTree | `anim_clone_walk_tree` | | 218 | 56.50 | **100.00** |
| `004899f0` | AnimClone_CreateCopy | `anim_clone_create_copy` | | 311 | 19.37 | **100.00** |
| `00489b30` | AnimClone_CloneLwoMeshes | `anim_clone_clone_lwo_meshes` | | 131 | 16.46 | 98.15 |
| `00489bc0` | AnimClone_ReferenceVisuals | `anim_clone_reference_visuals` | | 99 | 16.98 | **100.00** |

No caller outside the unit changed score.

### What is left

`00489b30` (98.15%) differs in one operand: the call to Mesh_Clone
(`00480900`). The unit calls `mesh_clone`, `lws_port.cpp`'s unannotated
adapter with the original's two arguments, as `lws.c` does. The port's
entry at that address, `lego::cached_render_resource_instance_create`, has
C++ linkage and takes only the mesh. Converting Mesh.c (G98-015) closes it.
Every instruction matches.

No naked asm. One code change is tagged `Deviation (scoring only)`, in the
comparison tooling ([below](#the-vtordisp-false-positive-and-reccmp_vtordisp_scopepy)).
Two declaration-only deviations are tagged too ([Declarations](#declarations)).

## Unit extent and name

`translation-units.spec.csv` now names G98-025 `AnimClone.c` (`inferred`).
The boundaries do not move. `python3 tools/translation_units.py build`
reports 0 conflicts.

**Data.** The unit has no `.data`, `.rdata` or `.bss` of its own. It reads:

- Main.c's `lpD3DRM()` (`00505914`, `s_main_d3drm`);
- two IIDs from the DirectX GUID library, which the original links once:
  IID_IDirect3DRMFrame3 (`0049f988`) and IID_IDirect3DRMUserVisual
  (`0049fb48`).

`make unit-data-offsets` gives G98-025 `n/a` in every section (no
attributable items). Unit summary: full 0, gaps-only 10, partial 22, n/a 195.

**Call graph.** Ten of the eleven listed functions, with the two static
callbacks `004898e0` and `004898f0`, form a closed graph:

- They are entered only from Container.c: `00472a50`, `00473690`,
  `00475140`, `00475300`, `00475d10`, `004765c0` and `00473c60`.
- They call out only to Lws.c (`00487a00` Lws_Clone, `00487ba0` Lws_SetTime,
  `004889d0` Lws_Free), Mesh.c's `00480900`, `malloc`/`free` and Direct3DRM.
- The two callbacks are reached only as function pointers passed to
  `00489910`, from `00489520`, `004895c0` and `00489660`.

**The struct.** The callbacks confirm the record layout. AnimClone_Register
passes a local `AnimClone` to the frame-array callback, which writes
`partArray[partCount++]` at `+0x14`/`+0x18`. The relocation's
`ResourceTreeArrayWriter` (`reserved[0x14]`, then `entries`, `count`) was
the same record seen from its offsets.

**`004897d0` is AnimClone_IsLws** (`return clone->lws;`, `+0xc`). Its
callers are:

- Container.c's `00473c60` (Container_FormatPartName), with an AnimClone;
- Flic.c and Steering callers (`004121b0`, `0044bc30`, `0045a730`,
  `0045cc70`), with flics and flocks.

This one copy is what the linker kept when it folded identical one-line
`+0xc` getters (finding 588, finding 612). It sits inside this object,
between AnimClone_Remove and AnimClone_SetTime. The adapters that serve
the folded callers (`flic.h`, `legoobject_port.cpp`) are unchanged.

**`004894c0` is Material_Create.** OpenLRR keeps it in a file of its own.
It shares no data, literal or call with AnimClone or with Memory.c
(G98-024) before it. Its only caller is Map3D.c's Map3D_Create
(`0044e0c0`). With no `.data`, no `.rdata` and no float literals on either
side, and with alignment padding unable to separate C objects (finding 505),
no binary evidence places an object boundary at `00489520`. It stays in
G98-025 and in `animclone.c`, at its original address. If such evidence
appears, a split moves one function and no data, so no byte changes.

`00489c30` starts Bmp.c (G98-026), as before.

## How the conversion is done

### Step 0

`com_object_tree.cpp` was first renamed `com_object_tree_port.cpp` and made
a `tail` entry (also in Meson). Then `python3 tools/convert_unit.py G98-025
--stem animclone` scaffolded the unit. The unit goes into the link-order
list in G98-025's slot.

### The unit

The frames, frame arrays, visuals, animation sets, the material and
IDirect3DRM3 are called through COM method tables declared in `animclone.c`
with the slots the unit uses, as in `lws.c`. The unit calls `lws_clone`,
`lws_time_set` and `lws_free` directly. This resolves the gap in finding 579
(`com_object_tree.cpp` → `lws_time_set`), whose row now says so.

### Link names

In the matching build, `animclone_link_names.h` gives three functions the
port's C-linkage names. Each caller passes the original's arguments:

| Unit function | Port name | Callers |
| --- | --- | --- |
| Material_Create | `scene_material_create` (`mesh.h`, `view.hpp`) | `terrain_map.c`'s Map3D_Create, `view.cpp`'s wrapper |
| AnimClone_Remove | `scene_object_owner_link_release` | `container_unit.cpp` |
| AnimClone_SetTime | `ddiscene_timed_hook_dispatch` | `container_unit.cpp` (clone, time, `NULL`), `view.cpp`'s wrapper |

The last two had C++ linkage. Their declarations in `view.hpp` gain
`extern "C"` in place, which adds no declaration. In the matching build the
port tail compiles its three definitions out (`LRR_ANIMCLONE_PORT_NAMES`).
Natively the port tail serves them, unchanged, through `view.cpp`'s hooks.

No other caller exists. The other port models of the unit had no caller, so
they are deleted from the tail: the two resource-tree context builders, the
record clone, the walker, the hierarchy clone and the two visual helpers.

### The shared IIDs

The unit is C, so it needs both IIDs with C linkage:

- `lws.c`'s `s_iid_user_visual` (`0049fb48`) loses `static`. Under
  `__cplusplus` it gets an `extern "C"` declaration, so in the matching
  build (C) the only change is the storage class.
- `resource.cpp` defines `s_resource_tree_child_iid` (`0049f988`) with C++
  linkage, through the frozen `resource_for_com_object_tree.hpp`. The header
  is not edited. Its two includers, `resource.cpp` and
  `container_unit.cpp`, wrap the `#include` in `extern "C" { }`. That adds
  no declaration.

Neither change moved a score or a listing digest.

### Declarations

Six declarations now have no definition:

- `view.hpp`: `scene_act_frame_record_clone_or_create`,
  `com_child_items_visit`, `object_interface_array_resource_create` and
  `object_hierarchy_recursive_clone`;
- `resource.hpp`: `resource_tree_callback_count` and the two
  `resource_tree_callback_context_create_*`.

Deleting them was measured. Through rule B (finding 538) it dropped three
unrelated functions below the baseline:

| Function | Score | Instructions |
| --- | --- | --- |
| `00459470` osf_read | 20.28% → 9.71% | −11 |
| `0047d520` | 33.82% → 32.65% | |
| `00482620` | 31.30% → 26.09% | |

So the six declarations are kept, each with a comment tagged
`Deviation (scoring only)` (ADR 0005 entropy rule 4, as in `record.hpp`).
The hook tables and setters in `view.cpp` that the deleted models used stay:
`view.cpp` is not this change's.

### Source shapes that matched

Ten of the thirteen functions matched on the first compile.

- **AnimClone_Make** (`00489660`): the local `AnimClone data` is declared at
  function scope, not in the `else` block. In the block, MSVC loads the
  root before the `partCount = 0` store (97.89%).
- **AnimClone_WalkTree** (`00489910`): `finished = TRUE` goes before the
  child's `Release`, not after (89.27%).
- Argument slots are reused for locals as in the original:
  - the material in `power`'s slot;
  - the parent frame in `time`'s slot;
  - the name size in `orig`'s slot;
  - the visual count in `orig`'s slot.

  MSVC does this from the plain source. `tools/msvc_permuter.py` was not
  needed.
- AnimClone_CloneLwoMeshes never releases the user visual it queries. The
  original leaks it, and so does the rebuild.

## The vtordisp false positive and `reccmp_vtordisp_scope.py`

The first full run dropped one exact function outside the unit:

- `00489dd0` directx_result_diagnostic_dispatch (dxerror.c, source
  unchanged) went from 100.00% to 99.62%, with the same instructions.

The unit is now its original size, so it moved the rebuilt dispatcher to
`00492590`. Two adjacent entries of its jump tables, `0x00492be4` and
`0x004925e9`, then read `2B 49 00 E9 25 49 00 FB`. That is Reccmp's
vtordisp pattern (`sub ecx, [ecx+0]; jmp rel32`). The steps from there:

1. Reccmp added an 8-byte VTORDISP entity at `00493861`, inside the
   function.
2. Its address test for the four jump-table operands after it hit that
   entity, not the function.
3. Those operands stayed unnamed, so the function scored 99.62%.

`tools/reccmp_vtordisp_scope.py` limits Reccmp's vtordisp scan to the
original image. It is tagged `Deviation (scoring only)`, since it changes
Reccmp's inputs at run time.

- `tools/match_function.py` already scans only the original image.
- The original has no vtordisp thunk. The scan of it is kept, so a real one
  would still be found.
- The rebuild's functions come from its PDB. On this tree, this entity was
  the only one the rebuild scan found.
- `reccmp_full_diff.py` installs it. `RECCMP_VTORDISP_SCOPE=0` turns it off.
- `make tools-test` runs `tools/tests/test_reccmp_vtordisp_scope.py`. With
  Reccmp installed, it also checks that these four table words are read as
  a vtordisp.

With it, `00489dd0` stays at 100.00%. Nothing else changes: the listing
digests change only at the unit's 11 listed addresses.

## Entropy and drops

`make reccmp-check` against the baseline at `8abe8b6b`:

- 11 improved, 1 newly mapped (`004897d0`);
- **none below baseline**, and no exact match lost;
- the plain-immediate check, the winapi link check and the link-order map
  check pass.

Instruction counts changed only at the unit's addresses (listing digests):

| Address | before | after |
| --- | ---: | ---: |
| `004894c0` | 57 | 34 |
| `00489520` | 48 | 49 |
| `004895c0` | 48 | 49 |
| `00489660` | 80 | 95 |
| `00489750` | 54 | 50 |
| `004897d0` | unmapped | 3 |
| `004897e0` | 182 | 113 |
| `00489910` | 89 | 90 |
| `004899f0` | 210 | 132 |
| `00489b30` | 91 | 54 |
| `00489bc0` | 51 | 42 |

## `opt_ref` (finding 579, `make opt-ref-callers`)

| | `main` `8abe8b6b` | this change |
| --- | ---: | ---: |
| `opt_ref.annotated.full`: removed / exact removed | 428 / 207 | 423 / 214 |
| `opt_ref.annotated.game_only` | 421 / 200 | 416 / 207 |
| Removed `.text` functions / bytes | 2,532 / 278,217 | 2,509 / 277,353 |

The relink keeps three of the unit's 13 functions: Material_Create, AnimClone_Remove and
AnimClone_SetTime, whose callers reach them through the link names.
`make opt-ref-callers OPT_REF_ARGS="--object animclone.c.obj"` lists the
other ten (nine exact) as removed. Their roots are the unit itself.

- Container.c's live callers (`00473690`, `00475c80`, `004765c0`) are port
  models with other contracts. They free payloads with `free` and keep
  their own record pool (finding 579's (c) table), so they make no call at
  the original's sites.
- `004897d0`'s callers call the flic and steering adapters.

These switch when Container.c is converted. More functions are removed
(exact ones up 7) because the newly exact functions are among them. Fewer
are removed in all (−5), because AnimClone_Remove and AnimClone_SetTime now
have live callers.

## Native behaviour

Native gameplay and rendering are unchanged:

- the port tail serves `scene_material_create`,
  `scene_object_owner_link_release` and `ddiscene_timed_hook_dispatch`
  natively, with the same bodies;
- natively `animclone.c` compiles as C++ under its own names, and nothing
  calls them;
- the native timed hooks still call `lws_port.cpp`'s Lws_SetTime model,
  since the native port never loads scenes through `lws.c`.

There is no new native gap, so no `port` issue is filed.

**Matching-build runtime** (`make reccmp-run` only). In the matching build,
`container_unit.cpp`'s calls reach the unit's AnimClone_Remove and
AnimClone_SetTime with the port's owner-link records. Those have the
original's AnimClone layout (`kind`/`clonedFrom` at `+0`, `shared_mode`/`lws`
at `+0xc`, the handle array at `+0x14`/`+0x18`). The unit calls Direct3DRM
and Lws.c unguarded, as the original does.

## Files touched outside the unit

The unit's own files are `animclone.c`, `animclone.h`,
`animclone_link_names.h` and the port tail. Outside them, this change
touches only:

- `view.hpp`: `extern "C"` in place on two declarations, and the
  deviation comments;
- `resource.hpp`: a deviation comment;
- `resource.cpp`, `container_unit.cpp`: the `extern "C"` include wrapper;
- `lws.c`: `s_iid_user_visual`'s storage class;
- `lws_port.cpp`: a comment;
- `link_order.cmake`, `src/gods98/meson.build`, `test_c_boundaries.cpp`;
- `tools/reccmp_full_diff.py`, the new `tools/reccmp_vtordisp_scope.py` and
  its test, and the `Makefile`'s `tools-test`;
- finding 579's gap row.

No file owned by #670 (`crt.cpp`), #671 (`cell_flood_passes.cpp`,
`view.cpp`), #672 (`tile_variants.cpp`) or #674 (`upgrade_levels.cpp`) is
edited. The frozen `_for_`/`_from_` headers are unchanged.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK. None below
  baseline and no exact match lost.
- `make match-progress`, `make unit-data-offsets`: run, and the generated
  files are committed. `tools/binary_progress_inputs.txt` was rewritten
  (`binary_progress_hash.py --write-manifest`).
- `make winapi-link-check`: OK.
- `make tools-test`: passes (no `--allow-growth`). Lint ledger 1,080 →
  1,075.
- Native: `cmake --build build` and `ctest --test-dir build`: 37 of 37
  pass.
- C subset: `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only` on `animclone.c`.

## Reproducing

    git mv src/gods98/com_object_tree.cpp src/gods98/com_object_tree_port.cpp   # and make it a tail entry
    python3 tools/convert_unit.py G98-025 --stem animclone
    make match-function SOURCE=src/gods98/animclone.c
    make reccmp-check
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object animclone.c.obj"
    make match-progress
    make unit-data-offsets
