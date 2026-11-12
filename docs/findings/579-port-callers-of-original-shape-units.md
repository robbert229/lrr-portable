# 579. Port callers of the original-shape units: what keeps `/OPT:REF` from keeping them

Issue #589, a follow-up of finding 576. Measured on `main` at `c191f476` (after #586). Finding 576 found that `/OPT:REF` removes about half of the annotated functions, and that their live callers are still port code. The plan was to switch live port callers onto the units' functions where the contracts match, starting with `interface_unit.c`, `container_unit.cpp`, `frontend_unit.cpp`, `terrain_map.c` and `reward.c`.

## Summary

1. **The measure.** The new `opt_ref_link.py callers` (`make opt-ref-callers`, about 1 s) lists each object's removed annotated functions and their callers in the original. It reads the callers from the original's direct calls and marks each one `live`, `removed` or `unmapped` in the rebuild. For a live caller it aligns the two call sequences and names the function the rebuilt caller calls at the original's call site, which is the substitute to switch. It also names each function's **roots**: the topmost removed callers, which a live caller has to reach first.
2. **Most removed functions hang off Lego.c and LegoObject.c.** Of 1,298 removed annotated functions (581 exact):
   - **703 (309 exact)** have their roots only in `lego_unit.c`, `lego.cpp`, `legoobject_unit.c` or `legoobject.cpp`;
   - **182 (100 exact)** have roots there and elsewhere.

   The original's call trees run from WinMain through Lego.c's config loading, level load and free, engine tick and simulation step, and through LegoObject.c's per-object updates. The rebuilt versions of those functions are themselves removed, or are port models that call the port's own systems. So interface, reward, terrain map, steering, entity stats and weapons code stays unreferenced until #587 and #588 give those roots live callers. No caller switch in another file can bring them back.
3. **The remaining 413 have different contracts.** 323 (131 exact) are reached only from live port models whose contracts differ: hook tables, the port's own state, other structures. 90 (41 exact) have no direct caller in the original. These are class (c), listed [below](#c-gaps-different-contracts).
4. **One switch has the same contract (a).** `aitask_cmd_alloc` (`0x00406440`) called `timeGetTime` directly. The original calls Main.c's clock, `main_time_get` (`0x00478000`, exact), which is a tail call to `timeGetTime`. `aitask.c` now calls `main_time_get`. Natively that is the same function, and `aitask_imports.h` swaps one declaration for another, so the symbol counter does not move (rule B, finding 538).
   - `/OPT:REF` now keeps `main_time_get`.
   - `aitask_cmd_alloc` goes from 60.87% to 65.22%.
   - The relink's import thunks go from 6 to 5: WINMM `timeGetTime` was referenced only directly.
5. **No (b) adapters.** Each near-same pair turned out to route through a port hook table or port-only state. A thin adapter could not equalise it without changing what the matching build runs.

## Before and after

| | `main` `c191f476` | This change |
| --- | ---: | ---: |
| `opt_ref.annotated.full`: removed / exact removed | 1,304 / 586 | **1,303 / 585** |
| `opt_ref.annotated.game_only`: removed / exact removed | 1,267 / 557 | 1,266 / 556 |
| Removed `.text` symbols / bytes | 3,238 / 414,664 | 3,238 / 414,654 |
| Relink import thunks | 6 | 5 |
| Exact-function code bytes | 105,485 | 105,485 |
| `make reccmp-check` | | 1 improved (`0x00406440` 60.87% → 65.22%), none fell |

`opt_ref` counts by address and `callers` counts by map symbol; the 6-function difference (1,304 vs 1,298) is aliases at shared addresses.

### Per object

Each object with at least 15 removed annotated functions, outside Lego.c and LegoObject.c, and the four Lego.c and LegoObject.c objects for reference. The counts are the same before and after this change. The one function it brings back, `main_time_get`, is in `frameloop.cpp`, which goes from 5 removed (2 exact) to 4 (1). The total row is `main`'s; after this change it is 1,297 (580). Each class gives functions (exact). "L only": all roots are in Lego.c or LegoObject.c objects. "L and other": some roots are. "Port caller": the roots elsewhere have live port-model callers or unmapped original callers. "No caller": no direct call in the original.

| Object | Removed | Exact | Bytes | L only | L and other | Port caller | No caller |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `interface_unit.c` | 88 | 37 | 20,608 | 33 (9) | 24 (9) | 13 (8) | 18 (11) |
| `aitask.c` | 66 | 58 | 14,912 | 25 (22) | 21 (18) | 13 (12) | 7 (6) |
| `frontend_unit.cpp` | 54 | 23 | 8,320 | 11 (6) | 0 | 38 (16) | 5 (1) |
| `container_unit.cpp` | 53 | 16 | 5,888 | 18 (5) | 14 (5) | 18 (6) | 3 (0) |
| `reward.c` | 46 | 19 | 15,456 | 45 (18) | 0 | 0 | 1 (1) |
| `terrain_map.c` | 42 | 20 | 11,568 | 28 (13) | 8 (4) | 6 (3) | 0 |
| `mesh_unit.cpp` | 40 | 8 | 6,272 | 8 (2) | 17 (1) | 12 (4) | 3 (1) |
| `panel_unit.c` | 31 | 18 | 6,112 | 29 (18) | 0 | 2 (0) | 0 |
| `weapons.cpp` | 31 | 12 | 6,896 | 27 (12) | 0 | 4 (0) | 0 |
| `steering.c` | 30 | 24 | 3,072 | 16 (15) | 5 (3) | 2 (2) | 7 (4) |
| `avi_player.cpp` | 25 | 4 | 2,672 | 0 | 0 | 21 (4) | 4 (0) |
| `entity_stats.cpp` | 21 | 21 | 672 | 20 (20) | 1 (1) | 0 | 0 |
| `info_unit.c` | 20 | 16 | 2,144 | 12 (9) | 0 | 7 (6) | 1 (1) |
| `terrain_cell_objects.c` | 20 | 15 | 4,544 | 13 (11) | 4 (3) | 0 | 3 (1) |
| `viewports.cpp` | 20 | 4 | 1,696 | 15 (3) | 3 (0) | 2 (1) | 0 |
| `lws.c` | 19 | 9 | 7,792 | 2 (1) | 0 | 17 (8) | 0 |
| `draw.c` | 18 | 17 | 2,912 | 5 (5) | 5 (5) | 2 (2) | 6 (5) |
| `animation_object.c` | 17 | 15 | 1,552 | 10 (9) | 2 (2) | 5 (4) | 0 |
| `images_unit.cpp` | 16 | 2 | 2,688 | 4 (0) | 4 (1) | 7 (1) | 1 (0) |
| `camera_control.cpp` | 15 | 6 | 1,184 | 10 (5) | 1 (0) | 0 | 4 (1) |
| *`lego_unit.c`* | 122 | 58 | 69,712 | 94 (45) | 10 (3) | 8 (6) | 10 (4) |
| *`legoobject.cpp`* | 64 | 0 | 17,584 | 53 (0) | 2 (0) | 2 (0) | 7 (0) |
| *`legoobject_unit.c`* | 55 | 41 | 7,920 | 26 (18) | 18 (16) | 8 (5) | 3 (2) |
| *`lego.cpp`* | 34 | 22 | 3,664 | 24 (12) | 9 (9) | 1 (1) | 0 |
| **All 82 objects** | **1,298** | **581** | | **703 (309)** | **182 (100)** | **323 (131)** | **90 (41)** |

For a Lego.c object, "L only" includes functions that are their own root, such as Lego.c functions with live port callers.

## Where the roots are

The largest Lego.c and LegoObject.c roots. The live caller in the second column calls the port's own code instead.

| Root (removed) | Live original caller, or none | Reaches |
| --- | --- | --- |
| `0x0041fa20` / `0x0041fb50` Lego config init (`lego_unit.c`) | `WinMain` (`0x004777a0`, `entry.cpp`) runs `winmain_application_run` | `interface_initialise` and the interface loaders, `aitask_cmd_registry_init_target`, reward init, about 700 functions in all |
| `0x004232e0` engine tick, `0x004250c0` sim step, `0x00426520` interaction update (`lego_unit.c`) | port frame loop | `interface_draw`/`update`/`do_mouse`, panel and priorities draw |
| `0x00429890` level load, `0x0042f0c0` level free (`lego_unit.c`) | `frontend_start_level` (`0x00415220`) calls the port's level code | `aitask_level_no_gather_object_setup`, `interface_reset`, `aitask_cmd_flush_all`, terrain map create/remove |
| `0x0044b680`, `0x0044bb30`, `0x0044bae0` (`legoobject.cpp` port models of LegoObject.c) | removed themselves | `steering.c` (29), `entity_stats.cpp` (21), `weapons.cpp` (27), `animation_object.c` |
| `0x0046cf90` vehicle upgrade level (`vehicle_model.cpp`) | `legoobject_create` (`0x00437d00`) calls the no-op adapter `scene_record_indexed_entries_rebuild_original` | 74 functions in vehicle, container and mesh code |

## (c) gaps: different contracts

These are the live callers outside Lego.c and LegoObject.c whose original calls a removed unit function. In each case the rebuilt caller is a port model with another contract, so switching it would change native behaviour, or would change what the matching build runs on in a way the original never did. They should switch when their own unit is converted (ADR 0005). File them with the `port` label where native work is needed.

| Caller (live) | Original callee (removed) | What the rebuild calls instead, and the gap |
| --- | --- | --- |
| `building_model.cpp` `0x004087e0`, `0x00408810` | `animation_object_null_find` `0x00406f50` | `view.cpp`'s `scene_object_subobject_resolve`: a resolver hook keyed by index, not the AnimObj frame name. It would revive 64 functions (18 exact). |
| `building_model.cpp` `0x004083a0`; `vehicle_model.cpp` `0x0046d260`, `0x0046d9f0` | `resource_name_register_or_query_target` `0x00473bc0`, `xf_texture_name_format` `0x00473c60` (`container_unit.cpp`) | port scene-resource hooks; no call at the site |
| `container_unit.cpp` `0x00473690`, `0x00475c80` † | `0x00489660`, `0x00480900` | the port frees the payload with `free` and keeps its own record pool |
| `mesh_unit.cpp` `0x004809e0`; `sfx.cpp` `0x00465430` | `engine_task_callback_record_allocate_and_initialize` `0x00480870` | `calloc` of the port's parsed-model records; `0x00480870` itself works through port hooks |
| `com_object_tree.cpp` `0x004897e0` | `lws_time_set` `0x00487ba0` | `lws_port.cpp`'s ledger stub: hook tables, guarded pointers, and repeat mode without the `fmod` |
| `object_overlays.cpp` `0x00459b00`; `tooltip.cpp` `0x0046b8b0` | `surface_draw_polyline` `0x00485ff0`, `surface_fill_pixel_spans` `0x00486090` (`draw.c`) | `LegacyHudOps` hooks with a packed colour, used by the native HUD. It would revive 12 functions (11 exact). |
| `panel_unit.c` `0x0045a730` | `object_field10_get` `0x004850e0` (Flic_GetWidth) | `flic_width`/`flic_height` on the port's opaque `Flic`, whose width is not the original's `+0x10` |
| `message_unit.cpp` `0x00452cc0` | `electric_fence_remove` `0x0040d0a0` | `terrain_entity_cell_record_remove`: the port tail's own grid state, not `electricFenceGlobs` |
| `message_unit.cpp` `0x004520d0`, `0x004526e0`; `placement.cpp` `0x004091e0` | `aitask_cmd_create_*`, `aitask_entity_activity_records_refresh`, `aitask_bobbin_clear_delay` | `aitask_port.cpp`'s models on the port's `CmdRec` and its hooks (finding 570) |
| `lava_erosion.cpp` `0x0040eab0`; `camera_control.cpp` `0x00436b33` | `terrain_map_world_z_get` `0x0044f940` | `terrain_surface_height_interpolate` on `TerrainState`, or `terrain_height(EngineContext*)` (finding 556's follow-up table) |
| `weapons.cpp` `0x00471630`; `placement.cpp` `0x00409a90`; `lego.cpp` `0x00433e30` † | `terrain_map_block_ray_intersection`, `terrain_map_shape_flatten`, `terrain_map_block_hidden_get` | port terrain models; no call at the site |
| `camera_control.cpp` `0x00435dc8` | `engine_memory_free` `0x00402210` | the view-context free hook |
| `tutorial_script.cpp` `0x00453da0`, `0x00454890`, `0x00454b80` † | `interface_set_item_flash`, `interface_get_primary_selected`, `interface_set_advisor_item` and five more | the port's tutorial script drives the port interface's hook tables; no call at the site |
| `objective_unit.cpp` `0x00458670` † | `reward_level_start` `0x00460220` | `mission_reward_success_snapshot_prepare` on the port's mission state (finding 560's list) |
| `frontend_unit.cpp` `0x004140c0`, `0x00412c00`, `0x00415910`, … † | 38 menu record, draw and movie helpers | the port's `FrontEnd`/`FeMenu` builders, which parse into port structures |
| `screen_effect_quads.cpp` `0x0040a0c0` † | `avi_player_create` and the AVI frame helpers (`avi_player.cpp`, 21) | the port's movie path |

### Callers in files other threads own

| Caller | File (owner) | Gap |
| --- | --- | --- |
| `legoobject_create` `0x00437d00` | `legoobject_unit.c` (#588) | calls the no-op adapter `scene_record_indexed_entries_rebuild_original`. The original calls `0x0046cf90`, whose port model takes a record list and channels, not the vehicle and level (74 functions). |
| `legoobject_remove` `0x00437540` | `legoobject_unit.c` (#588) | calls `lego.cpp`'s `object_pointer_registry_clear_match`, not `lego_object_pointer_registry_clear_matching` `0x0042dfc0` |
| `entity_route_cell_candidate_accept` `0x00443840`, `entity_conditional_type_five_spawn` `0x004470d0` | `legoobject.cpp` (#588) | call `level.cpp`/`lego.cpp` port cell queries instead of `lego_unit.c`'s |
| `WinMain` → Lego config init, engine tick, level load | `lego_unit.c`, `lego.cpp` (#587) | the roots of most of the table above |

## Method notes

- **Callers in the original.** Function starts are the Reccmp-mapped addresses plus every direct `call` target in the original's `.text`. Each function's extent runs to the next start. Calls through tables or registers are not seen. A scan of the original's other sections for pointers to the no-caller functions of 14 of the objects above found 2, both `avi_player.cpp` destructors referenced from `.rdata` vtables.
- **The substitute.** The tool aligns the original's call sequence (mapped to rebuilt addresses) with the rebuild's using `difflib`. Inside a differing run of equal length, calls pair by position; otherwise the whole run is reported. "No call" means the rebuild has none there. Port models are often too different for a clean pairing. Rows marked † in the (c) table are the tool's reading only. The others were checked against both sources.
- **Classification.** (a) means the same arguments and effect natively and in the matching build. (b) would need a thin `// PORT-ONLY` adapter. (c) is everything that reads the port's state, goes through a hook, or takes another structure. Matching-build-only aliases (finding 572) were used only for (a), where the caller already uses the original contract.

## Reproducing

```sh
make reccmp-check
make opt-ref-link
make opt-ref                                   # totals
make opt-ref-callers                           # objects with >= 15 removed
make opt-ref-callers OPT_REF_ARGS="--object reward.c.obj --json out.json"
```
