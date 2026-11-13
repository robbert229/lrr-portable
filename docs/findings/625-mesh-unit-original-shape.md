# Finding 625 — Mesh.c (G98-015) rebuilt in original shape, with a port tail

Issue #698, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
G98-015 is gods98's immediate-mode mesh module. OpenLRR names the object
`Mesh.c` and its functions `Mesh_*`, and the unit map now uses that name
(`inferred`). #549 relocated the unit verbatim to `gods98/mesh_unit.cpp` but
did not convert it. **The extent holds** (`004805b0`-`00483c80`): 59
functions, the 58 of the decompile manifest and the render callback
`004827f0`, which is reached only through a function pointer. See
[Unit extent](#unit-extent).

What the unit does:

- **Meshes.** A mesh is a Direct3DRM user visual on a frame. Its render
  callback (Mesh_RenderCallback, `004827f0`) draws its groups with the
  Direct3D device's DrawIndexedPrimitive. Meshes come from a pool of blocks
  of 1, 2, 4, ... records (`meshGlobs.listSet`) with a free list linked
  through the last word (`004807a0`-`004807f0`). Mesh_CreateOnFrame
  (`00480870`) and Mesh_Clone (`00480900`) make one. Mesh_Remove
  (`004820d0`) frees it when its last clone goes.
- **LightWave objects.** Mesh_Load (`004809e0`) reads an object through
  Lwo.c, and Mesh_ParseLWO (`00480ac0`) splits it into one group per
  surface. It welds the vertices, averages the face normals and maps the
  textures, from the object's `.uv` file (Mesh_UViewMesh, `00481c50`) or
  by wrapping (`00481fa0`). Mesh_GetSurfInfo (`00481820`) loads each
  surface's texture or numbered texture sequence (`00481ac0`, `00481b80`).
  Mesh_SetTextureTime2 (`00482040`) steps the sequences.
- **Groups and materials** (`004821a0`-`004827d0`, `004830c0`-`00483540`).
- **Rendering** (`00482ac0`-`00482d60`, `00483580`-`00483b70`). Meshes with
  alpha-blended groups are queued on the post-render list and drawn after
  the scene by Mesh_PostRenderAll (`00482d60`, from Viewport_Render). The
  device's texture, material and texture-stage states are saved and
  restored around them.
- **Textures** (`00482e70`-`00483080`): Mesh_LoadTexture looks a name up in
  two 2048-entry lists, one for the object's directory and one for the
  shared texture directory (Mesh_Initialise, `004805b0`).

This change rebuilds the unit as the C unit
[`src/gods98/mesh_unit.c`](../../src/gods98/mesh_unit.c), with:

- the C header [`mesh_api.h`](../../src/gods98/mesh_api.h). `mesh_unit.h` is
  taken: it is the boundary header that lws.c, animclone.c, sfx.c and
  weapons.c include;
- the unit-private imports header [`mesh_imports.h`](../../src/gods98/mesh_imports.h);
- the matching-build link names [`mesh_link_names.h`](../../src/gods98/mesh_link_names.h);
- the inline vector helpers [`mesh_maths.h`](../../src/gods98/mesh_maths.h).

The relocated `mesh_unit.cpp` becomes the port tail
[`mesh_port.cpp`](../../src/gods98/mesh_port.cpp), under `git mv`,
unannotated.

"Before" is the *match baseline* at `main` `d4c4cf5c`. "After" is this
change, from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (59 functions, weighted by span, 14,032 bytes) | 16.07% | **81.93%** |
| Exact functions in the unit | 11 | **50** (no asm) |
| Mapped functions in the unit | 52 | **59** |
| Exact-function code bytes (`binary-progress.json`) | 208,965 | **217,689** |
| Accuracy, full / game-only | 89.03 / 87.97 | **90.16 / 89.21** |
| `opt_ref` annotated (full): removed / exact removed | 398 / 220 | 393 / 231 |
| `opt_ref` annotated (game-only) | 391 / 213 | 386 / 224 |
| Gate image end (`make pe-audit`) | `0x892000` | `0x8a1000` |
| `/OPT:REF` relink `.bss` (`binary-progress.json`) | 1,304,096 | 1,353,792 |
| Reccmp lint ledger (entries) | 1,042 | 1,024 |

The weights are each function's span up to the next function, padding
included. An unmapped function counts as 0%.

The names are the unit's own, after OpenLRR's `Mesh_*` functions. In the
matching build six of them take the port's names (see
[Callers](#callers)); the table gives the unit's.

| Address | Function | Port name before | Span | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `004805b0` | `mesh_initialise` | `shared_texture_root_register` | 160 | 29.89 | **100.00** |
| `00480650` | `mesh_create_global_material` | (none) | 304 | unmapped | **100.00** |
| `00480780` | `mesh_set_material` | `lego::zero_result_is_success` | 32 | 11.43 | **100.00** |
| `004807a0` | `mesh_obtain_from_list` | `render_resource_instance_record_acquire` | 48 | 57.89 | 44.44 (flagged) |
| `004807d0` | `mesh_return_to_list` | `render_resource_instance_record_release` | 32 | 100.00 | **100.00** |
| `004807f0` | `mesh_add_list` | `render_resource_instance_pool_grow` | 128 | 30.00 | **100.00** |
| `00480870` | `mesh_create_on_frame` | `engine_task_callback_record_allocate_and_initialize` | 144 | 31.65 | **100.00** |
| `00480900` | `mesh_clone` | `lego::cached_render_resource_instance_create` | 224 | 17.78 | **100.00** |
| `004809e0` | `mesh_load` | `lwo_model_entry_load` | 224 | 17.48 | **100.00** |
| `00480ac0` | `mesh_parse_lwo` | `material_pass_execute` | 3424 | 6.78 | 38.57 |
| `00481820` | `mesh_get_surf_info` | `bind_model_textures_for_model` | 672 | 12.48 | 97.98 |
| `00481ac0` | `mesh_get_texture_seq_info` | (none) | 192 | unmapped | 31.11 |
| `00481b80` | `mesh_get_next_in_sequence` | (none) | 208 | unmapped | **100.00** |
| `00481c50` | `mesh_uview_mesh` | (none) | 848 | unmapped | 85.06 |
| `00481fa0` | `mesh_get_texture_uvs_wrap` | (none) | 160 | unmapped | **100.00** |
| `00482040` | `mesh_set_texture_time2` | `ledger_stub_00482040_…` | 144 | 15.94 | 85.19 |
| `004820d0` | `mesh_remove` | (none) | 208 | unmapped | **100.00** |
| `004821a0` | `mesh_get_group` | `indexed_record_entry_read` | 112 | 100.00 | **100.00** |
| `00482210` | `mesh_get_group_count` | `pointer_value_get` | 16 | 100.00 | **100.00** |
| `00482220` | `mesh_add_group` | `mesh_record_allocate_and_fill` | 304 | 15.55 | **100.00** |
| `00482350` | `mesh_alter_group_render_flags` | `indexed_record_field64_set_target` | 32 | 100.00 | **100.00** |
| `00482370` | `mesh_scale` | `mesh_vertices_scale_apply` | 112 | 14.18 | **100.00** |
| `004823e0` | `mesh_set_vertices` | `terrain_cell_quad_vertices_write` | 144 | 13.56 | **100.00** |
| `00482470` | `mesh_get_vertices` | `indexed_geometry_data_copy_to_output` | 144 | 11.86 | **100.00** |
| `00482500` | `mesh_set_vertices_point_normal_at` | `instance_quad_vertex_data_fill` | 288 | 9.22 | 77.17 |
| `00482620` | `mesh_set_vertices_same_normal` | `mesh_segment_vertex_fill` | 160 | 31.30 | **100.00** |
| `004826c0` | `mesh_set_vertices_vnt` | `mesh_vertex_record_fill` | 192 | 9.64 | **100.00** |
| `00482780` | `mesh_is_group_hidden` | `indexed_record_flag6c_bit0_get` | 32 | 100.00 | **100.00** |
| `004827a0` | `mesh_hide_group` | `indexed_record_flag6c_bit0_set` | 48 | 100.00 | **100.00** |
| `004827d0` | `mesh_hide` | `record_flag2c_bit0_set` | 32 | 100.00 | **100.00** |
| `004827f0` | `mesh_render_callback` | `scene_runtime_user_visual_manager` | 720 | 0.00 | 99.60 |
| `00482ac0` | `mesh_set_mesh_render_desc` | `model_render_record_state_setup` | 144 | 55.81 | **100.00** |
| `00482b50` | `mesh_set_render_desc` | `render_state_flags_apply` | 352 | 60.14 | **100.00** |
| `00482cb0` | `mesh_set_alpha_render` | `renderer_command_mode_and_pair_send` | 48 | 78.95 | **100.00** |
| `00482ce0` | `mesh_add_to_post_render_list` | `renderer_command_node_prepend` | 80 | 21.33 | **100.00** |
| `00482d30` | `mesh_clear_post_render_list` | `model_render_list_clear` | 48 | 100.00 | **100.00** |
| `00482d60` | `mesh_post_render_all` | `model_render_pass_dispatch` | 272 | 9.54 | **100.00** |
| `00482e70` | `mesh_load_texture` | (none) | 448 | unmapped | 90.26 |
| `00483030` | `mesh_search_texture_list` | `texture_cache_name_slot_find` | 80 | 100.00 | **100.00** |
| `00483080` | `mesh_add_texture_reference` | `texture_cache_name_slot_append` | 64 | 32.43 | **100.00** |
| `004830c0` | `mesh_set_group_texture` | `animated_texture_frame_surface_set` | 128 | 17.98 | **100.00** |
| `00483140` | `mesh_remove_group_texture` | `ledger_stub_00483140_…` | 48 | 24.00 | **100.00** |
| `00483170` | `mesh_create_group_material` | `indexed_record_transform_initialize` | 208 | 15.69 | **100.00** |
| `00483240` | `mesh_set_group_material` | `indexed_record_transform_copy_in` | 48 | 51.28 | **100.00** |
| `00483270` | `mesh_set_group_colour` | `render_object_color_vector_set` | 400 | 16.34 | **100.00** |
| `00483400` | `mesh_get_group_material` | `indexed_record_data_address_get_target` | 32 | 100.00 | **100.00** |
| `00483420` | `mesh_set_group_material_values` | `screen_effect_item_opacity_set_target` | 288 | 30.51 | **100.00** |
| `00483540` | `mesh_set_identity_matrix` | `matrix4_identity_initialize` | 64 | 100.00 | **100.00** |
| `00483580` | `mesh_set_current_viewport` | `render_interface_capability_check` | 128 | 18.18 | **100.00** |
| `00483600` | `mesh_set_current_gods_viewport` | `direct3d_viewport_setup` | 144 | 24.24 | **100.00** |
| `00483690` | `mesh_set_transform` | `matrix_method_pair_apply` | 384 | 9.30 | **100.00** |
| `00483810` | `mesh_set_texture_stage_state` | `ledger_stub_00483810_…` | 160 | 11.11 | **100.00** |
| `004838b0` | `mesh_store_texture_and_mat` | `d3d_render_state_sync` | 144 | 10.69 | **100.00** |
| `00483940` | `mesh_restore_texture_and_mat` | `direct3d_render_state_restore` | 128 | 18.05 | **100.00** |
| `004839c0` | `mesh_render_mesh` | `model_render_child_pass_dispatch` | 176 | 26.59 | **100.00** |
| `00483a70` | `mesh_can_render_group` | `model_child_render_eligibility_check` | 32 | 76.92 | **100.00** |
| `00483a90` | `mesh_render_group` | `model_render_child_material_pass` | 112 | 21.33 | **100.00** |
| `00483b00` | `mesh_set_group_render_desc` | `world_object_render_state_apply` | 112 | 24.00 | **100.00** |
| `00483b70` | `mesh_render_triangle_list` | `direct3d_device_state_apply` | 272 | 14.81 | **100.00** |

**Seven functions had no source** (`00480650`, `00481ac0`, `00481b80`,
`00481c50`, `00481fa0`, `004820d0`, `00482e70`). They are written and
mapped. The 11 that were exact keep their exactness. Before, seven of them
were naked asm in the port; now they are C.

Callers that rose, with no source change:

- lws.c (G98-020): Lws_Clone `00487a00`, Lws_AnimateTextures `00487d50`,
  Lws_SetDissolveLevel `004880d0`, Lws_LoadMesh `004885c0` and Lws_FreeNode
  `00488a70` become exact;
- animclone.c: AnimClone_CloneLwoMeshes `00489b30` becomes exact;
- screen_effect_quads.c (DamageFont): `0040a400` and `0040aa40` become exact;
- sfx.c (Smoke): `004653a0` and `004659b0` become exact; `00465430` 94.12%
  to 94.38% and `00465e00` 87.67% to 89.04%;
- weapons.c: `0046f550` becomes exact.

### The drop: `004807a0` (flagged, owner review)

Mesh_ObtainFromList falls from the port model's 57.89% to 44.44%, with two
instructions fewer. The rebuild computes the address of the free-list link
once (`lea esi, [edx + 0x30]`) and uses it for the load before the
`memset` and for the store after it. The original addresses `[edx + 0x30]`
twice and needs no second register. The port model wrote the link through
`memcpy` at a byte offset, which hides the common subexpression. The
original-shape source is the same as Font.c's Font_Create (`0047a580`,
61.11%) and AITask.c's `00406440` (65.22%), and none of them matches.
Tried, with no change: eight statement and declaration forms (a temporary
for the next link, a chained assignment, `newMesh[0].nextFree`, casts,
`sizeof(*newMesh)`, Font_Create's guarded form) and a 0-300 sweep of the
symbol counter before the function.

The gate baseline is refreshed for this one function. It is not
owner-reviewed.

### What is left

- **Container.c adapters.** Two calls go through unannotated adapters at the
  end of `mesh_port.cpp`, because the port has no C-linkage entry with the
  original's arguments. #697 owns `container_unit.cpp`.
  - Container_LoadTextureSurface (`00474050`) is called twice in
    `00482e70` and has no port entry at all.
  - Container_Frame_GetContainer (`00475830`, the port's
    `scene_object_first_non_kind6_child_get`, C++ linkage) is called once
    in `004827f0`.

  These calls are the whole difference in `004827f0`, and part of it in
  `00482e70`.
- **`00480ac0` Mesh_ParseLWO** (38.57%, 3,424 bytes): the structure, the
  calls and the allocation order match. The register allocation and the
  frame (0xb0 against 0xa0 bytes) differ throughout. Writing the vertex
  copies component by component, as the original does, raised it from
  32.61%.
- **`00481ac0`** (31.11%). The original keeps the accumulated number on the
  stack and the length in `ebp`. The success path falls off the end, so
  `eax` returns the `tnumlen` pointer, as in the original: no `return`
  matches it. Thirty declaration orders change nothing.
- **`00481c50`** (85.06%): two stack slots are swapped (the poly and
  poly-vertex counters).
- **`00482040`** (85.19%) and **`00482500`** (77.17%) differ in a base and
  index register order and in the x87 normalise. The original stores the
  modulus and the scale as floats (`fst`); three forms of the helpers were
  tried.
- **`00481820`** (97.98%) and **`00482e70`** (90.26%): register order and
  spill slots.

## Unit extent

Findings 599 and 603 apply: MSVC 5.0 pools float literals per object.

- **The data.** The unit's state is one `.bss` block, `005343c4`-`005405dc`
  (`meshGlobs`, 0xc218 bytes), laid out as follows:

  | Address | Field |
  | --- | --- |
  | `005343c4` | the post-render list |
  | `005343c8` | 50 texture-stage states (value and changed flag) |
  | `00534558` | the shared texture directory |
  | `0053455c` | the texture name list (2048 × 12 bytes) |
  | `0053a55c` | the shared texture name list (2048 × 12 bytes) |
  | `0054055c`, `00540560` | the two lists' counts |
  | `00540564`-`00540578` | the saved and current texture, material and render-state handles |
  | `0054057c` | the global material's handle |
  | `00540580` | the global material |
  | `00540584` | the 20 pool blocks |
  | `005405d4` | the free list |
  | `005405d8` | the block count |

  Only the unit's functions reference it (`convert_unit.py`'s data
  analysis; first reference `004805b4`, last `00483c44`).
- **The literal pool.** `0049e9e0`-`0049e9f8` holds `1.0f 0.0f 10.0f
  0.00390625f -0.5f`, a zero pad word, then the double `1.0`:
  - Mesh_ParseLWO and the normalises load `1.0f`;
  - the colour and material clamps load `0.0f` and `1.0f`;
  - Mesh_ParseLWO's power scale loads `10.0f`;
  - Mesh_GetSurfInfo's colour scale (`/ 256.0f`) loads `0.00390625f`;
  - Mesh_GetTextureUVsWrap (`- -0.5f`) loads `-0.5f`;
  - Mesh_SetTextureTime2's `fmod` loads the double `1.0`.

  `0049e9d0`-`0049e9dc` (`1 2 4 -1`) is not referenced from this range.
- **GUIDs.** Only the unit uses IID_IDirect3DDevice2 (`0049f6c8`),
  IID_IDirect3DTexture2 (`0049f6f8`) and IID_IDirect3DRMViewport2
  (`0049f958`); `mesh_unit.c` defines them. IID_IDirect3DViewport3
  (`0049f778`) is shared with Viewports.c, whose copy now has external
  linkage.
- **The calls.** The unit calls Main.c, Container.c, Utils.c, Maths.c,
  Files.c, Lwo.c and the C library. Its callers are Lego.c (Mesh_Initialise),
  Viewports.c (Mesh_PostRenderAll), Container.c, Lws.c, AnimClone.c, SFX.c,
  Weapons.c and the damage font.
- **The edges.** Files.c (G98-014) ends with `00480570` and owns its own
  pool. Flic.c (G98-016) starts at `00483c80` with its own strings.

`translation-units.spec.csv`: G98-015 becomes `Mesh.c` (`inferred`) with
this evidence. `python3 tools/translation_units.py build`: 0 conflicts.

## How the conversion is done

### State and imports

`Mesh_Globs meshGlobs` is a `static` definition, as the block is in `.bss`,
not the common region. `make unit-data-offsets`, G98-015 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| G98-015 | n/a (0, 1 unknown) | n/a (0, 0 unknown) | n/a (1, 0 unknown) | - |

The port's statics for the same state stay in `mesh_port.cpp` and lose
their five `// GLOBAL` annotations. `meshGlobs` adds 49,688 bytes to the
gate build (image end `0x892000` to `0x8a1000`) and to the relink's `.bss`.

Main.c's IDirect3DDevice3 (`0050591c`) gets a stand-in, `s_main_im_device`,
annotated at the end of `mesh_port.cpp`, like lws_port.cpp's
`s_main_d3drm`. Main_ChangeRenderState (`004788d0`) and Main_RestoreStates
(`00478940`) get `extern "C"` in place in `surface.hpp`.

### The port tail

`mesh_unit.cpp` becomes `mesh_port.cpp`, a `tail` entry (Meson too). Its
`// FUNCTION` and `// GLOBAL` annotations are removed. The frozen relocation
headers (`*_for_mesh_unit.hpp`, `*_from_mesh_unit.hpp`) are unchanged. The
lint ledger loses the file's 18 entries.

### Callers

- **Link names.** `mesh_link_names.h` gives six functions the port's names
  in the matching build: Mesh_GetGroup, Mesh_AddGroup, Mesh_GetVertices,
  Mesh_IsGroupHidden, Mesh_SetGroupTexture and Mesh_SetGroupColour.
  `mesh_port.cpp` compiles its definitions of those names out there.
  - Their callers pass the original's arguments: Container.c's model
    (`container_unit.cpp`), the C units, and the port tails `sfx_port.cpp`,
    `weapons_port.cpp` and `level.cpp`.
  - `record.hpp` gives three of the names C linkage in place.
  - Without this, seven Container.c functions lost their call targets, and
    `00474ae0` lost its exact match.
- **lws.c and animclone.c.** Mesh_Clone and Mesh_Remove are the unit's, by
  `mesh_unit.h`'s names, in every build. `lws_port.cpp`'s two adapters for
  them are deleted: natively lws.c is not reached from port code.
  `mesh_unit.h`'s matching-build macros point lws.c, animclone.c, sfx.c and
  weapons.c at the unit for the other entries.
- **`screen_effect_quads_imports.h`, `lego_unit_imports.h` and
  `viewports_imports.h`** do the same for the damage font, Lego.c and
  Viewports.c.

Natively the port keeps serving the port names, and nothing calls the
unit's other functions (#552).

### The symbol counter's phase

Finding 538's rule B decides much of this unit. A sweep of 0-600 inert
declarations at the top of the file found:

- the number of exact functions cycles between 34 and 49 with a period of
  about 128;
- with no extra declarations, the unit sat near the minimum (35 exact).

Inert declarations are not used. The unit instead uses two forms of the
original's header shape, chosen by measurement among plausible variants:

- `<windows.h>` in the matching build only, as `config.c` does. The
  original's Standard.h includes it. MSVC 5.0's `d3drm.h`, `ddraw.h` and
  `d3d.h` are DirectX 3's and do not help: they gave 34 to 41 exact.
- three enums in `mesh_api.h`, as in OpenLRR's Mesh.h: `Mesh_Colour`,
  `Mesh_Type` and `Mesh_Flags`.

Together they reach the sweep's best phase. The enums replace `#define`s
and type the API; the include is the original's. Neither is tagged
`Deviation (scoring only)`. The unit has no naked asm and no
`Deviation (scoring only)`.

### Source shapes worth noting

- The early-return forms of Mesh_CreateGlobalMaterial and the two
  Mesh_SetCurrentViewport functions match. The nested forms (OpenLRR's) do
  not.
- `0.0f == value` and `1.0f != value` in Mesh_SetGroupMaterialValues (the
  constant first) put the x87 operands in the original's order.
- `if (...) Main_ChangeRenderState(41, TRUE); else (41, FALSE)` gives the
  original's `push 1` / `push 0` branch; the `?:` form computes a bit
  shift.
- Mesh_RenderTriangleList's device-2 pointer is declared at function scope,
  so its dead `= NULL` store is kept, as in the original.
- Mesh_UViewMesh copies the vertex through two locals (the vertex index and
  the new index). Without them, it scores 53%.
- Mesh_GetNextInSequence pads with a counter tested `>= 1`. The `while
  (tnumlen)` form is a `je`; the original is `cmp eax, 1; jb`.

## Checks

- `make reccmp-check`: one drop, `004807a0`, flagged above, with the
  baseline refreshed. The C units' callers rose (listed above), and no other
  function changed score.
- `make match-progress`: baseline, listing digests and
  `binary-progress.json` regenerated. `winapi-link-check` and `check-map`:
  OK.
- `make unit-data-offsets`: the row above.
- `make tools-test` (no `--allow-growth`): pass. The lint ledger shrinks
  from 1,042 to 1,024.
- Native build and `ctest`: 37 of 37 pass.
- `gcc -std=gnu89 -Werror=declaration-after-statement` on the unit: pass.

## `/OPT:REF` (finding 579)

`make opt-ref-callers`: `mesh_unit.c.obj` loses no function. The totals move
from 398 / 220 to 393 / 231 (full) and from 391 / 213 to 386 / 224
(game-only):

- the seven newly mapped functions are kept;
- 11 more removed functions are exact: the sfx.c, weapons.c and damage-font
  callers that became exact are still unreachable from a live root
  (Lego.c's, finding 579).
