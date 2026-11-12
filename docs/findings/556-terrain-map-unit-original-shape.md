# Finding 556 — Terrain map unit (GAME-026) rebuilt in original shape, with a port tail

Issue #522, under [ADR 0005](../adr/0005-original-shape-c-units.md). GAME-026
(text `0044e0c0`–`00451cd0`) is the original's `Map3D.c` followed by the
level-of-detail mesh lists (`MeshLOD`). It was relocated verbatim into
`src/game/terrain_map.cpp`. This change rebuilds it as the C unit
[`src/game/terrain_map.c`](../../src/game/terrain_map.c), with the C boundary
header [`terrain_map.h`](../../src/game/terrain_map.h).

GAME-026 is the first converted unit whose functions are live native code.
The port built its terrain on its own model: `TerrainState` with native
render sidecars, `TerrainSurfaceGrid`, and the scene-render hooks. About 120
caller functions and 283 `ptr_terrain` uses depend on it. The original
functions work on the original 0x73d4-byte map and the gods98 Container mesh,
so they cannot serve those callers without a large native rewrite. The owner
chose a **unit plus a port tail**:

- every one of the 62 functions is written in original shape in
  `terrain_map.c`;
- the port's implementations move, unannotated and under the port's names, to
  the port tail [`terrain_map_port.cpp`](../../src/game/terrain_map_port.cpp)
  (ADR 0006 decision 4), so native callers and tests keep today's behaviour;
- callers are switched only where their contract already equals the
  original's (below). The rest are listed for a follow-up.

"Before" is the *match baseline* on `main` at `0a90abfa`. "After" is this
change. Scores are from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, all 62 functions, 14,623 bytes) | 19.32% | **81.42%** |
| Exact functions in the unit | 7 | **35 of 62** (38 in the single-function loop; 3 are register-only "effective" matches in the full build) |
| Mapped functions in the unit | 56 | 62 |
| Accuracy, full (`binary-progress.json`) | 59.11% | 60.47% |
| Accuracy, game-only | 54.75% | 56.27% |
| Exact-function code bytes | 72,434 | 76,390 |

Byte weights are the original's code bytes without the `0x90` fill; an
unmapped function counts as 0%.

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `0044e0c0` | `terrain_map_create` | 11.44 | 51.19 |
| `0044e4d0` | `terrain_map_vertices_build` | 10.71 | 99.17 |
| `0044e670` | `terrain_map_remove` | 36.36 | **100.00** |
| `0044e6b0` | `terrain_map_texture_no_fade_set` | 11.76 | **100.00** |
| `0044e6d0` | `terrain_map_block_texture_fade_set` | 6.43 | 93.88 |
| `0044e860` | `terrain_map_block_hidden_get` | 33.33 | **100.00** |
| `0044e880` | `terrain_map_vertex_normals_update_all` | 24.00 | **100.00** |
| `0044e8c0` | `terrain_map_shape_tolerance_check` | 17.56 | 83.33 |
| `0044ead0` | `terrain_map_shape_flatten` | 13.45 | 35.74 |
| `0044edf0` | `terrain_map_block_rotated_set` | 10.22 | 99.28 |
| `0044efb0` | `terrain_map_block_vertex_modified` | — | **100.00** |
| `0044eff0` | `terrain_map_update` | 24.82 | **100.00** |
| `0044f090` | `terrain_map_transitions_update` | 20.56 | 97.62 |
| `0044f1a0` | `terrain_map_fade_pair_add` | 83.52 | **100.00** |
| `0044f220` | `terrain_map_texture_set_set` | **100.00** | **100.00** |
| `0044f230` | `terrain_map_block_texture_set` | 11.54 | 93.40 |
| `0044f380` | `terrain_map_vertex_move` | 94.67 | **100.00** |
| `0044f490` | `terrain_map_perspective_correction_set` | 64.37 | 97.22 |
| `0044f4e0` | `terrain_map_block_highlight_set` | 31.58 | **100.00** |
| `0044f540` | `terrain_map_block_highlight_get` | 10.34 | **100.00** |
| `0044f570` | `terrain_map_block_highlight_clear` | 37.04 | **100.00** |
| `0044f5c0` | `terrain_map_block_emissive_set` | 18.33 | **100.00** |
| `0044f640` | `terrain_map_block_to_world` | 55.26 | **100.00** |
| `0044f6d0` | `terrain_map_world_to_block` | **100.00** | **100.00** |
| `0044f700` | `terrain_map_world_to_block_centre` | 60.42 | 96.55 |
| `0044f810` | `terrain_map_world_to_block_clamped` | 38.10 | **100.00** |
| `0044f870` | `terrain_map_world_to_block_offset` | 42.75 | 78.08 |
| `0044f940` | `terrain_map_world_z_get` | 9.65 | **100.00** |
| `0044fab0` | `terrain_map_world_normal_get` | 18.57 | 91.78 |
| `0044fb90` | `terrain_map_world_edge_point_get` | 23.65 | 57.35 |
| `0044fe70` | `terrain_map_world_z_unmodified_get` | 9.89 | 97.99 |
| `00450060` | `terrain_map_block_first_vertex_get` | 14.29 | 45.07 |
| `004500d0` | `terrain_map_block_vertices_get` | 10.20 | **100.00** |
| `00450220` | `terrain_map_block_vertices_raw_get` | 9.60 | **100.00** |
| `004502c0` | `level_coord_pair_within_bounds_target` | **100.00** | **100.00** |
| `004502e0` | `terrain_map_screen_pick` | 13.28 | 82.38 |
| `00450560` | `terrain_map_ray_intersection` | 55.24 | **100.00** |
| `00450600` | `terrain_map_visible_radius_add` | 27.32 | **100.00** |
| `00450700` | `terrain_map_block_hide` | 12.70 | 92.31 |
| `00450730` | `terrain_map_visible_add` | 58.18 | **100.00** |
| `00450780` | `terrain_map_visible_clear` | 50.91 | **100.00** |
| `004507d0` | `terrain_map_vertex_position_get` | 9.36 | **100.00** |
| `00450890` | `terrain_context_scale_get` | **100.00** | **100.00** |
| `004508a0` | `cell_neighbor_direction_code` | 44.58 | **100.00** |
| `00450960` | `terrain_map_block_uv_wobble_set` | 14.74 | **100.00** |
| `00450a80` | `terrain_map_wobble_emissive_set` | 36.26 | **100.00** |
| `00450b60` | `terrain_map_uv_wobble_update` | — | 32.43 |
| `00450f30` | `terrain_map_block_vertex_normals_update` | — | 98.86 |
| `00451180` | `terrain_map_block_pair_fades` | 42.86 | 67.61 |
| `00451230` | `terrain_map_block_vertex_normal_set` | 14.95 | 98.25 |
| `004512d0` | `terrain_map_block_vertex_normal_get` | 9.73 | 91.37 |
| `00451450` | `terrain_map_block_vertex_offset` | 14.16 | 98.33 |
| `004514f0` | `terrain_map_block_face_normals_update` | 13.51 | **100.00** |
| `004515a0` | `terrain_map_file_specs_get` | **100.00** | **100.00** |
| `004515c0` | `terrain_map_file_block_value` | **100.00** | **100.00** |
| `004515e0` | `terrain_map_block_ray_intersection` | 9.06 | 66.43 |
| `004519b0` | `model_entry_record_load` | — | 94.05 |
| `00451ab0` | `model_primitive_record_create` | 43.64 | **100.00** |
| `00451b30` | `model_entry_list_flatten` | 30.07 | **100.00** |
| `00451bc0` | `resource_list_update_matching_key` | — | 95.56 |
| `00451c30` | `linked_record_field4_clear` | **100.00** | **100.00** |
| `00451c50` | `owned_object_reference_chain_release` | — | 73.68 |

Unit functions are named after what they do (`terrain_map_*`); the original
names are in the header comments. Eight keep a port name (below).

## The unit

### State

`TerrainMap` (0x73d4 bytes, Win32 offsets) in `terrain_map.h`:

| Offset | Field | Notes |
| --- | --- | --- |
| `+0x00`–`+0x0c` | `gridWidth`, `gridHeight`, `blockWidth`, `blockHeight` | quads are one fewer than map-file cells |
| `+0x10`, `+0x14` | `blockSize`, `roughLevel` | |
| `+0x18`, `+0x1c` | `worldX`, `worldY` | world position of block (0, 0) |
| `+0x20` | `lowestZ` | lowest vertex, set by `0044e4d0` |
| `+0x24` | `mesh` | one group per quad |
| `+0x28` | `blocks` | 0x28-byte `TerrainMapBlock` records |
| `+0x2c`, `+0x30` | `textureSet`, `material` | |
| `+0x34` | `fadePairs[128][128]` | textures that fade into each other |
| `+0x4034` | `fadePairCount[128]` | |
| `+0x40b4` | `noFade[128]` | |
| `+0x42b4`, `+0x4a84` | `visible[500]`, `visibleCount` | blocks shown around a point |
| `+0x4a88`, `+0x7198` | `uvWobbles[2500]`, `uvWobbleCount` | |
| `+0x719c` | `transitions[10]` | 0x38-byte texture-fade records |
| `+0x73cc`, `+0x73d0` | `transMesh`, `flags` | |

- **`.data`**: the six highlight colours at `004a52c0` (one annotated static
  array; the unit range listed only its first entry).
- **`.bss`**: `004eaed0` is `Map3D_FlattenShapeVertices`' list of the
  shape's distinct blocks: ten `{x, y}` pairs, up to `004eaf20`, where
  Message.c's `.bss` starts.
- **`.rdata`**: the 15 floats at `0049e4e4`–`0049e51c` are literals. MSVC
  pools them itself.
- **Strings**: `"%s\\%s"`, `"LWO"` and `"MESH"` (`004519b0`) are pooled with
  other units, so they carry no `STRING` marker (#518/#519's rule).

`make unit-data-offsets`, GAME-026 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-026 | n/a (1, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | n/a (1, 0 unknown) fewer than two attributable items | - |

The one `.data` item is the colour table and the one `.bss` item the flatten
list. The `.rdata` floats are literals, so none is attributed.

### Shared functions

Eight functions keep the port's name and signature, because their callers
already use the original's contract, and `004519b0`, which had no port
definition, joins them. The port's definitions are deleted:

- `cell_neighbor_direction_code` (`004508a0`, pure) and
  `terrain_context_scale_get` (`00450890`, reads `+0x10`, which is
  `blockSize` in both layouts). Both were already declared with C linkage.
- `level_coord_pair_within_bounds_target` (`004502c0`). The callers pass
  `terrain->dim`, whose first two words are the grid size. It is declared
  only in the frozen `level_*` headers, with C++ linkage. `lego.cpp` and
  `level.cpp` declare it `extern "C"` before including them, so the frozen
  redeclarations inherit C linkage. The frozen headers are not edited.
  This restores `0x004329d0` to 100%.
- The six MeshLOD functions (`004519b0`–`00451c50`; `004519b0` is the new
  `model_entry_record_load`), whose caller is the
  converted `animation_object.c`. `ModelEntryRecord` already had the
  original layout. `resource.cpp`'s hook-driven models of `00451bc0` and
  `00451c50` are uncalled; they are kept, renamed `*_superseded` with their
  `resource.hpp` declarations, so that no symbol count changes (#518's
  lesson). `004519b0` and `00451bc0` were inert `/* FUNCTION */` comments
  before; they are mapped now, which takes four `animation_object.c`
  functions to 100% (finding 549's note).

### Callers that keep the port's functions (follow-up)

These port functions have a different contract (the port's `TerrainState`
or `TerrainSurfaceGrid`, hooks, NULL guards that the tests pin, or a
different argument order). Native callers keep calling them in the tail. A
caller can switch once it is converted and the native terrain uses the
original map.

| Address | Port function (port tail) | Unit function |
| --- | --- | --- |
| `0044e0c0` | `terrain_create_retail` | `terrain_map_create` |
| `0044e4d0` | `terrain_grid_vertex_build` | `terrain_map_vertices_build` |
| `0044e670` | `nested_pointer_record_destroy` | `terrain_map_remove` |
| `0044e6b0` | `indexed_flag_value_set_one` | `terrain_map_texture_no_fade_set` |
| `0044e6d0` | `terrain_cell_material_transition_begin` | `terrain_map_block_texture_fade_set` |
| `0044e860` | `grid_cell_set_payload_index` | `terrain_map_block_hidden_get` |
| `0044e880` | `terrain_grid_cells_initialize` | `terrain_map_vertex_normals_update_all` |
| `0044e8c0` | `terrain_surface_vertex_span_validate` | `terrain_map_shape_tolerance_check` |
| `0044ead0` | `terrain_cell_surface_height_clamp` | `terrain_map_shape_flatten` |
| `0044edf0` | `terrain_cell_surface_orientation_set` | `terrain_map_block_rotated_set` |
| `0044eff0` | `simtick_marker_sweep` | `terrain_map_update` |
| `0044f090` | `terrain_material_transition_tick` | `terrain_map_transitions_update` |
| `0044f1a0` | `input_key_mapping_add_bidirectional` | `terrain_map_fade_pair_add` |
| `0044f220` | `scene_object_resource_handle_set` | `terrain_map_texture_set_set` |
| `0044f230` | `terrain_cell_material_set` | `terrain_map_block_texture_set` |
| `0044f380` | `terrain_cell_surface_voxel_neighbors_mutate` | `terrain_map_vertex_move` |
| `0044f490` | `terrain_grid_operation_apply_all` | `terrain_map_perspective_correction_set` |
| `0044f4e0` | `terrain_cell_surface_type_set` | `terrain_map_block_highlight_set` |
| `0044f540` | `terrain_cell_surface_type_get` | `terrain_map_block_highlight_get` |
| `0044f570` | `terrain_cell_surface_type_reset` | `terrain_map_block_highlight_clear` |
| `0044f5c0` | `terrain_cell_surface_appearance_apply` | `terrain_map_block_emissive_set` |
| `0044f640` | `map_cell_to_world_center` | `terrain_map_block_to_world` |
| `0044f6d0` | `terrain_grid_pick` | `terrain_map_world_to_block` |
| `0044f700` | `world_to_terrain_cell_bounds_check` | `terrain_map_world_to_block_centre` |
| `0044f810` | `grid_coordinate_clamp_and_store` | `terrain_map_world_to_block_clamped` |
| `0044f870` | `terrain_world_point_to_cell` | `terrain_map_world_to_block_offset` |
| `0044f940` | `terrain_surface_height_interpolate` | `terrain_map_world_z_get` |
| `0044fab0` | `terrain_cell_surface_triangle_vertices_select` | `terrain_map_world_normal_get` |
| `0044fb90` | `terrain_state_surface_point_project_offset` | `terrain_map_world_edge_point_get` |
| `0044fe70` | `terrain_authored_surface_height_interpolate` | `terrain_map_world_z_unmodified_get` |
| `00450060` | `grid_cell_read_surface_vector` | `terrain_map_block_first_vertex_get` |
| `004500d0` | `terrain_cell_surface_vertices_get` | `terrain_map_block_vertices_get` |
| `00450220` | `grid_cell_read_surface_values` | `terrain_map_block_vertices_raw_get` |
| `004502e0` | `screen_ray_terrain_cell_pick_target` | `terrain_map_screen_pick` |
| `00450560` | `random_reachable_grid_point_select` | `terrain_map_ray_intersection` |
| `00450600` | `terrain_surface_clear_circle` | `terrain_map_visible_radius_add` |
| `00450700` | `grid_surface_write_at_xy` | `terrain_map_block_hide` |
| `00450730` | `terrain_cell_change_queue_append` | `terrain_map_visible_add` |
| `00450780` | `terrain_cell_change_queue_flush` | `terrain_map_visible_clear` |
| `004507d0` | `terrain_cell_edge_vector_query` | `terrain_map_vertex_position_get` |
| `00450960` | `terrain_cell_dirty_queue_push_and_clear` | `terrain_map_block_uv_wobble_set` |
| `00450a80` | `cell_list_min_distance_scale_apply` | `terrain_map_wobble_emissive_set` |
| `00451180` | `cell_variant_entry_match_check` | `terrain_map_block_pair_fades` |
| `00451230` | `terrain_cell_surface_vector_write` | `terrain_map_block_vertex_normal_set` |
| `004512d0` | `terrain_cell_normal_sample` | `terrain_map_block_vertex_normal_get` |
| `00451450` | `terrain_cell_surface_vector_offset_add` | `terrain_map_block_vertex_offset` |
| `004514f0` | `terrain_cell_surface_vectors_rebuild` | `terrain_map_block_face_normals_update` |
| `004515a0` | `level_map_image_header` | `terrain_map_file_specs_get` |
| `004515c0` | `level_map_image_cell` | `terrain_map_file_block_value` |
| `004515e0` | `terrain_cell_ray_hit_test` | `terrain_map_block_ray_intersection` |

### Callees

- **`extern "C"` added in place** (no symbol-count change): in
  `gods98/record.hpp` (`packed_grid_cell_value_get`), `gods98/surface.hpp`
  (three Container mesh functions), `gods98/scene_resources.hpp` (three),
  `gods98/view.hpp` (`scene_mesh_color_set`,
  `scene_mesh_color_and_alpha_set`, `scene_material_create`,
  `render_interface_transform_point`) and `gods98/model.hpp`
  (`model_primitive_record_create`). `view.cpp` is not edited; two of its
  definitions get C linkage from `view.hpp`.
- **New C boundary headers**, included only by the unit and
  `test_c_boundaries.cpp`: [`gods98/mesh.h`](../../src/gods98/mesh.h) and
  [`game/tile_variants.h`](../../src/game/tile_variants.h). The other C
  units' headers are not extended, so their symbol numbering is unchanged.
- **Four thin unannotated adapters** with the original signatures, at the
  end of `gods98/surface.cpp` (ADR 0005 step 6):
  - `container_make_mesh2` (`00473560`): the port's entry is a C++ ledger
    stub inside a namespace in `view.cpp`;
  - `container_mesh_hide_group`, `container_mesh_set_vertices` and
    `container_mesh_perspective_correction_set` (`00474b30`, `00474d30`,
    `00474e30`): the port's entries take an extra table of host operations.
    The adapters pass none, so they change nothing natively.

  The call operand to an adapter is the only difference left in
  `0044e4d0`, `0044f090`, `0044f490`, `00450700`, `00451230` and
  `00451450`, and one of the differences in `0044e0c0`, `0044e6d0`,
  `0044edf0` and `0044f230`.
- **`Detail_GetTexture` (`0040b850`)** takes a byte in the original. The port
  declares `uint32_t`, so two calls zero-extend the texture first
  (`0044e6d0`, `0044f230`). Changing the port declaration would change
  GAME-011's own code, so it is left for that unit.

### Shape notes

- **`0x004500d0` has no return on its out-of-range path.** The original
  falls off the end with `gridWidth` in `eax`. The C source does the same
  (C allows it; g++ warns, `-Wreturn-type`). Adding `return 0` costs the
  function its match.
- **Local declaration order (finding 538, rule B).** `0x0044f940` went from
  95.12% to 100% by declaring `z` before `bx, by`. A declaration-order
  sweep found nothing more elsewhere.
- **Maths.h helpers.** The vector add, normalise and distance helpers are
  `static __inline` in
  [`terrain_map_maths.h`](../../src/game/terrain_map_maths.h), after #518 and
  #519. With the include at the top of the file, `0x0044f5c0` and
  `0x00450700` flipped `imul` operand order (rule B). The include sits just
  before its first user, `0x00450f30`, which restores both.
- **Inverted branches and early locals.** `0x0044edf0` clears the rotated bit
  in the fall-through branch (99.28%). `0x0044e6d0` computes the group index
  before anything else (93.88%). `0x00450600` keeps `radius * radius` and
  `y * y` in locals (100%).
- **Parameter slots.** `0x0044f6d0` passes the address of a dead parameter
  slot as its unused `centre`. A local gives that exactly. `0x0044f870`
  reuses its `map` and `pos` slots for the block coordinates, and no source
  shape reproduced that (78.08%).
- **MSVC 5.0 internal compiler error.** In `0x00450b60` (the UV wobble),
  copying the vertex UVs and the block wobble as `{x, y}` structs, which
  the original's integer moves suggest, makes the compiler fail (`C1001`,
  `P2\color.c`). Scalars compile but score 32.43%.

### Near misses

| Address | Score | What is left |
| --- | ---: | --- |
| `0044e0c0` | 51.19 | register assignment through the block loops; two adapter calls |
| `0044ead0` | 35.74 | the two-pass flatten loop's induction variables |
| `00450b60` | 32.43 | the UV copies (ICE, above) |
| `00450060` | 45.07 | `ebx`/`esi` choice for the flags byte; five source forms tried |
| `0044fb90` | 57.35 | 2D struct copies of the edge points; diagonal selection is exact |
| `004515e0` | 66.43 | MSVC merges the stores common to both branches of each triangle list; the original does not |
| `00451c50` | 73.68 | the original keeps the constant 2 in `bl` |
| `004502e0` | 82.38 | the screen-ray set-up order |
| `0044e8c0` | 83.33 | the duplicate-pair test reads the next pair through a second pointer |

## Entropy and drops outside the unit

`make reccmp-check` against `0a90abfa`: 53 functions improved and 15 fell
below the baseline. All 15 have the same instruction count, and the
classifier labels them `drift`. Their listing digests are unchanged. The
only change in each is a call operand: the caller now calls the
unannotated port copy in the tail instead of the function at the original
address. That follows directly from the port-tail decision. **For the
owner:**

| Address | Function | File | Score | Instructions |
| --- | --- | --- | --- | --- |
| `0x00402510` | `terrain_cell_command_create` | `cmd.cpp` | 45.35 → 44.19 | = |
| `0x00402ae0` | `terrain_cell_refresh_dispatch_current` | `level.cpp` | 92.31 → 88.46 | = |
| `0x00402d50` | `kind_nine_cell_command_issue` | `cmd.cpp` | 27.10 → 26.17 | = |
| `0x00403030` | `cell_command_type3_issue` | `cmd.cpp` | 32.20 → 30.51 | = |
| `0x00403c00` | `entity_activity_record_remove_by_type` | `entity.cpp` | **100.00 → 99.05** | = |
| `0x00409790` | `ledger_stub_00409790_CellPathRecordRegisterApply` | `placement.cpp` | 26.01 → 22.91 | = |
| `0x0040eab0` | `lava_erosion_pending_cell_tick` | `lava_erosion.cpp` | 41.42 → 41.00 | = |
| `0x0041b300` | `menu_selection_resource_update` | `interface.cpp` | 14.52 → 11.29 | = |
| `0x0042b510` | `map_pipeline` | `lego.cpp` | 28.08 → 27.74 | = |
| `0x0042f6f0` | `cell_init_pass` | `lego.cpp` | 11.09 → 9.28 | = |
| `0x00431780` | `terrain_stamp_impact_handle_current` | `lego.cpp` | 12.50 → 11.76 | = |
| `0x00444260` | `ledger_stub_00444260_EntityRandomCellStepRouteIssue` | `legoobject.cpp` | 34.84 → 34.37 | = |
| `0x0044a4f0` | `ledger_stub_0044a4f0_NearestCellCoordinateWorldResolve` | `legoobject.cpp` | 44.81 → 43.72 | = |
| `0x0044b140` | `ledger_stub_0044b140_SmallSpiderCellSwarmSpawn` | `legoobject.cpp` | 32.12 → 31.09 | = |
| `0x0046e220` | `terrain_cell_surface_quad_height_fill_current` | `cell_flood_passes.cpp` | 12.26 → 11.32 | = |

- **One exact match is lost: `0x00403c00`**, accepted by the owner. It calls
  `terrain_cell_surface_type_reset` (`0044f570`), which `level.hpp` declares
  with C linkage. Retargeting it means editing `entity.cpp`, which #521
  owns. **Follow-up for #521:** when `entity.cpp` is converted, call the
  unit's `terrain_map_block_highlight_clear` there. The unit's `0044f570`
  is 100%, so the exact count is unchanged.
- **Improved outside the unit:** `0x00406c00`, `0x00406c30`, `0x00406c60`
  and `0x00406ec0` (`animation_object.c`) to 100%, because their MeshLOD
  callees are mapped now.
- **Listing digests:** one function outside the unit changed:
  `0x0042c5b0` (`lego.cpp`), 74 instructions both sides, reordered, score
  unchanged. No instruction count changed outside the unit.
- **Symbol counts:** the `extern "C"` edits are in place. The `*_superseded`
  renames keep `resource.cpp`'s declaration count. The only new declarations
  in shared files are the two `extern "C"` lines in `lego.cpp` and
  `level.cpp`. Four forms of them (named, unnamed, typedef, mixed
  parameters) gave the same scores, and in the full build the only
  `lego.cpp`/`level.cpp` drops are the call-target ones above.

## Reccmp lint ratchet

The ratchet baseline goes from 4,061 to 4,117 entries. The 50 port models
in the tail are ledger-named definitions without an annotation, and the
lint does not exempt port-tail files that the matching build still
compiles. Two entries can be removed: the renamed `resource.cpp` models.
`--allow-growth` was needed, as in #518. **For the owner:** an exemption
for `tail` entries in the link-order list would remove these keys; that is
a tooling change, not made here.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): the 15 drops above,
  53 improved, link-order map OK.
- `make match-progress` regenerated the baseline, the listing digests and
  `binary-progress.json` (the binary-progress step ran with the host Python,
  which has `pefile`).
- `make unit-data-offsets`: the GAME-026 row above.
- `python3 tools/translation_units.py build`: 0 conflicts.
  `tools/binary_progress_inputs.txt` regenerated.
- `make tools-test` passes (Meson parity, lint with the growth above).
- `terrain_map.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only`.
- `cmake -S . -B build && cmake --build build && ctest --test-dir build
  --output-on-failure`: 37/37 pass, including `c_boundaries`, `level` and
  `record`.

## Reproducing

    python3 tools/convert_unit.py GAME-026 --stem terrain_map   # after removing the .cpp entry
    make match-function SOURCE=src/game/terrain_map.c
    make reccmp-check
    make match-progress
    make unit-data-offsets
