# Finding 624 — Container.c (G98-002) rebuilt in original shape, with a port tail

Issue #697, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
G98-002 is gods98's scene-container module. The unit map called it a
"D3DRM scene-object module", gave it no name and listed 107 functions with a
low-confidence boundary. #549 relocated it verbatim into
`container_unit.cpp`, and it had not been converted. OpenLRR names the
object `Container.c`, and the unit map now uses that name (`inferred`).
**The extent holds** (`00472710`-`00476d50`), and the unit has **110**
functions: the map's 107 and three that are only referenced by address. See
[Unit extent](#unit-extent).

What the unit does:

- A **Container** (0x2c bytes) wraps three Direct3DRM frames:
  - the master frame, which places it;
  - the activity frame under it, which carries its visual;
  - a hidden frame, where Container_Hide parks the activity frame.
- Its type says what the visual is: a mesh made in code or loaded from an
  `.x` file, a frame hierarchy, an animation set, a set of activities read
  from an `.ae` file, a light, a reference to a frame made elsewhere, a
  LightWave scene or a LightWave object. Each activity is an animation set
  or a LightWave scene under its own "ActFrame" frame.
- Containers come from a pool of blocks that double in size
  (Container_AddList).
- Every frame the unit makes carries a Container_AppData record as its app
  data: the owner, the animation clone, the activity's file, frame count,
  current time, transition coefficient, sample and trigger frame.
- The unit also:
  - clones containers, reusing a free clone when there is one;
  - loads textures (Container_LoadTexture2, and Container_LoadTextureSurface
    for 8-bit BMPs);
  - caches the textures the mesh loader names (Container_TextureLoadCallback);
  - sets the scene's fog.

This change rebuilds the unit as the C unit
[`src/gods98/container.c`](../../src/gods98/container.c), with:

- the C header [`container_unit.h`](../../src/gods98/container_unit.h);
- the unit-private imports header [`container_imports.h`](../../src/gods98/container_imports.h);
- the link-name header [`container_link_names.h`](../../src/gods98/container_link_names.h).

The relocated `container_unit.cpp` becomes the port tail
[`container_port.cpp`](../../src/gods98/container_port.cpp) (moved with
`git mv`), with its annotations removed. The functions keep the gods98
names (`Container_Load`, `Container_Mesh_HideGroup` and so on): the port
already uses several snake-case spellings of those names
(`container_load`, `container_make_mesh2`) for adapters with the original
contracts.

"Before" is the *match baseline* of #703 (`a30f398f`, Mesh.c, over #702
and #701), on which this change is stacked. "After" is this change, from the
full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (110 functions, weighted by span, 17,984 bytes) | 25.32% | **96.67%** |
| Exact functions in the unit | 33 | **96** (one naked, `00473ba0`) |
| Mapped functions in the unit | 102 | **110** |
| Exact functions, full | 1,833 | **1,927** |
| Exact-function code bytes (`binary-progress.json`) | 225,345 | **242,555** |
| `opt_ref` annotated (full): removed / exact removed | 363 / 215 | **305 / 182** |
| `opt_ref` annotated (game-only) | 356 / 208 | 299 / 176 |
| Gate image end (`make pe-audit`) | `0x85f000` | `0x864000` |
| `/OPT:REF` relink `.bss` (`binary-progress.json`) | 1,087,648 | 1,095,872 |
| Reccmp lint ledger (entries) | 1,024 | **1,002** |

Each weight is the function's span up to the next function, padding
included. An unmapped function counts as 0%. Outside the unit, **47
functions rose and none fell**; 31 of them became exact (see
[Callers](#callers)).

| Address | gods98 name | Name in the matching build | Bytes | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `00472710` | Container_Initialise | `container_initialise_original` | 240 | unmapped | 80.43 |
| `00472800` | Container_Shutdown | `ddiscene_object_pool_shutdown` | 192 | 30.06 | **100.00** |
| `004728c0` | Container_SetSharedTextureDirectory | `shared_texture_directory_set` | 32 | 100.00 | **100.00** |
| `004728e0` | Container_EnableSoftwareRenderer | `renderer_flag_bit6_set` | 32 | 100.00 | **100.00** |
| `00472900` | Container_SetTriggerFrameCallback | `scene_end_hook_register` | 32 | 100.00 | **100.00** |
| `00472920` | Container_SetSoundTriggerCallback | `scene_sample_hook_register` | 32 | 100.00 | **100.00** |
| `00472940` | Container_GetRoot | `scene_root_object_get` | 16 | 100.00 | **100.00** |
| `00472950` | Container_Create | `ddiscene_object_allocate` | 240 | 28.28 | **100.00** |
| `00472a40` | Container_Remove | `ledger_stub_00472a40_DDiSceneObjectDestroy` | 16 | 100.00 | **100.00** |
| `00472a50` | Container_Remove2 | (same) | 640 | 24.54 | **100.00** |
| `00472cd0` | Container_Load | `container_load` | 1648 | 10.10 | 85.36 |
| `00473340` | Container_IsCurrentActivity | `ledger_stub_00473340_SceneActionNamePresent` | 48 | 14.29 | **100.00** |
| `00473370` | Container_SetActivity | `scene_action_name_set` | 240 | 52.17 | 80.39 |
| `00473460` | Container_Light_SetSpotPenumbra | `light_interface_call_slot_40` | 32 | 48.28 | **100.00** |
| `00473480` | Container_Light_SetSpotUmbra | `light_interface_call_slot_3c` | 32 | 48.28 | **100.00** |
| `004734a0` | Container_Light_SetSpotRange | `light_interface_call_slot_38` | 32 | 48.28 | **100.00** |
| `004734c0` | Container_Light_SetEnableContainer | `advisor_companion_transform_apply` | 48 | 16.33 | **100.00** |
| `004734f0` | Container_MakeLight | `ddiscene_light_create` | 112 | 16.67 | **100.00** |
| `00473560` | Container_MakeMesh2 | `container_make_mesh2` | 288 | 13.33 | **100.00** |
| `00473680` | Container_GetMasterFrame | `scene_object_wrapped_pointer_get` | 16 | 100.00 | **100.00** |
| `00473690` | Container_Clone | `entity_visual_resource_load` | 1168 | 7.82 | 83.22 |
| `00473b20` | Container_Hide2 | `legoobject_gs_container_hidden_set_adapter` | 32 | 81.82 | **100.00** |
| `00473b40` | Container_Hide | `scene_render_lock_toggle` | 96 | 100.00 | **100.00** |
| `00473ba0` | Container_IsHidden | `scene_object_flag_bit3_get_target` | 32 | 100.00 | **100.00** |
| `00473bc0` | Container_SearchTree | `resource_name_register_or_query_target` | 160 | 35.71 | **100.00** |
| `00473c60` | Container_FormatPartName | `xf_texture_name_format` | 320 | 11.07 | 98.99 |
| `00473da0` | Container_SetUserData | `scene_object_owner_set` | 16 | 100.00 | **100.00** |
| `00473db0` | Container_GetUserData | `scene_object_owner_get` | 16 | 100.00 | **100.00** |
| `00473dc0` | Container_EnableFog | `lego_fog_enable_adapter` | 80 | 15.79 | **100.00** |
| `00473e10` | Container_SetFogColour | `lego_fog_colour_set_adapter` | 96 | 29.85 | 82.35 |
| `00473e70` | Container_SetFogMode | `renderer_fog_mode_set_original` | 48 | 13.79 | **100.00** |
| `00473ea0` | Container_SetFogParams | `renderer_fog_parameters_set_original` | 32 | 27.27 | **100.00** |
| `00473ec0` | Container_SetPerspectiveCorrection | `vehicle_container_enable_sound_triggers_adapter` | 176 | 19.23 | **100.00** |
| `00473f70` | Container_SetPerspectiveCorrectionCallback | (same) | 224 | unmapped | **100.00** |
| `00474050` | Container_LoadTextureSurface | (same) | 960 | unmapped | **100.00** |
| `00474410` | Container_GetDecalColour | (same) | 224 | 32.97 | **100.00** |
| `004744f0` | Container_LoadTexture2 | `detail_container_load_texture2_adapter` | 544 | 4.08 | **100.00** |
| `00474710` | Container_FreeTexture | `detail_container_free_texture_adapter` | 80 | 78.57 | **100.00** |
| `00474760` | Container_Mesh_Swap | `interface_array_forward_sync` | 400 | 5.58 | **100.00** |
| `004748f0` | Container_Mesh_AddGroup | `mesh_record_allocate_or_append` | 304 | 17.02 | **100.00** |
| `00474a20` | Container_Mesh_GetGroupCount | `animated_texture_frame_count` | 64 | 30.77 | **100.00** |
| `00474a60` | Container_Mesh_SetQuality | `animated_texture_frame_bind` | 128 | 13.08 | **100.00** |
| `00474ae0` | Container_Mesh_IsGroupHidden | `terrain_detail_entry_get_at_index` | 80 | 100.00 | **100.00** |
| `00474b30` | Container_Mesh_HideGroup | `container_mesh_hide_group` | 208 | 18.80 | **100.00** |
| `00474c00` | Container_Mesh_HandleSeparateMeshGroups | (same) | 64 | 28.95 | **100.00** |
| `00474c40` | Container_Mesh_GetGroup | `detail_container_mesh_get_group_adapter` | 128 | 57.69 | **100.00** |
| `00474cc0` | Container_Mesh_GetVertices | `lwo_layer_polygon_vertex_data_read` | 112 | 51.69 | **100.00** |
| `00474d30` | Container_Mesh_SetVertices | `container_mesh_set_vertices` | 112 | 10.08 | **100.00** |
| `00474da0` | Container_Mesh_SetTexture | `animated_texture_frame_set` | 144 | 23.70 | **100.00** |
| `00474e30` | Container_Mesh_SetPerspectiveCorrection | `container_mesh_perspective_correction_set` | 96 | 14.29 | **100.00** |
| `00474e90` | Container_Mesh_Scale | `detail_container_mesh_scale_adapter` | 128 | 20.51 | **100.00** |
| `00474f10` | Container_Mesh_GetBox | `scene_object_bounds_union` | 224 | 17.02 | **100.00** |
| `00474ff0` | Container_Mesh_SetEmissive | `scene_mesh_color_set` | 48 | 85.71 | **100.00** |
| `00475020` | Container_Mesh_SetColourAlpha | `scene_mesh_color_and_alpha_set` | 80 | 72.97 | **100.00** |
| `00475070` | Container_Transform | `scene_object_coordinate_transform_in_place` | 32 | 100.00 | **100.00** |
| `00475090` | Container_InverseTransform | `detail_container_inverse_transform_adapter` | 32 | 100.00 | **100.00** |
| `004750b0` | Container_SetColourAlpha | `render_object_set_packed_rgba_from_float_channels` | 112 | 87.10 | 90.32 |
| `00475120` | Container_MoveAnimation | `ddiscene_advance_by_time_target` | 32 | 100.00 | **100.00** |
| `00475140` | Container_SetAnimationTime | `ddiscene_frame_update_and_callbacks` | 448 | 31.23 | **100.00** |
| `00475300` | Container_ForceAnimationUpdate | `ddiscene_apply_act_frame_timing` | 144 | 38.10 | **100.00** |
| `00475390` | Container_GetAnimationTime | `ddiscene_object_length_get` | 96 | 59.26 | **100.00** |
| `004753f0` | Container_GetAnimationFrames | `ddiscene_object_frame_position_get` | 64 | 84.62 | **100.00** |
| `00475430` | Container_SetPosition | `ddiscene_object_position_set` | 64 | 57.14 | **100.00** |
| `00475470` | Container_SetOrientation | `ddiscene_object_pose_method_dispatch` | 80 | 55.07 | **100.00** |
| `004754c0` | Container_GetPosition | `ddiscene_object_position_get` | 64 | 45.90 | **100.00** |
| `00475500` | Container_GetOrientation | `ddiscene_object_facing_pair_get` | 128 | 20.59 | **100.00** |
| `00475580` | Container_AddRotation | `render_object_rotate_axis` | 48 | 100.00 | **100.00** |
| `004755b0` | Container_AddScale | `scene_frame_scale_set` | 48 | 100.00 | **100.00** |
| `004755e0` | Container_AddTranslation | `scene_object_camera_teleport_or_offset` | 48 | 100.00 | **100.00** |
| `00475610` | Container_ClearTransform | `render_object_set_identity_transform` | 160 | 100.00 | **100.00** |
| `004756b0` | Container_AddTransform | `legoobject_gt_container_matrix_set_adapter` | 32 | 100.00 | **100.00** |
| `004756d0` | Container_GetZXRatio | `ledger_stub_004756d0_VideoObjectAspectRatioGet` | 64 | 20.00 | **100.00** |
| `00475710` | Container_SetParent | `ddiscene_scene_object_attach_or_unlink` | 144 | 12.77 | **100.00** |
| `004757a0` | Container_GetParent | (same) | 80 | 25.64 | **100.00** |
| `004757f0` | Container_GetTransCoef | `typed_child_resource_scalar_get` | 64 | 100.00 | **100.00** |
| `00475830` | Container_SearchOwner | (same) | 80 | 54.32 | 85.71 |
| `00475880` | Container_Frame_GetContainer | (same) | 128 | unmapped | **100.00** |
| `00475900` | Container_GetFrames | (same) | 48 | 100.00 | **100.00** |
| `00475930` | Container_ParseTypeString | (same) | 192 | 40.54 | **100.00** |
| `004759f0` | Container_AddList | (same) | 128 | 10.75 | **100.00** |
| `00475a70` | Container_GetActivities | (same) | 400 | 12.17 | 99.30 |
| `00475c00` | Container_SetTypeData | (same) | 128 | 17.98 | **100.00** |
| `00475c80` | Container_FreeTypeData | (same) | 144 | 16.19 | **100.00** |
| `00475d10` | Container_AddActivity2 | (same) | 256 | unmapped | 97.56 |
| `00475e10` | Container_Frame_ReferenceDestroyCallback | (same) | 48 | 83.33 | **100.00** |
| `00475e40` | Container_Frame_Find | (same) | 304 | 15.38 | 98.18 |
| `00475f70` | Container_Frame_SetAppData | `ddiscene_animation_hook_create` | 368 | 20.95 | **100.00** |
| `004760e0` | Container_Frame_RemoveAppData | (same) | 64 | 66.67 | **100.00** |
| `00476120` | Container_Frame_GetOwner | (same) | 32 | 100.00 | **100.00** |
| `00476140` | Container_Frame_GetAnimSetFileName | (same) | 32 | 100.00 | **100.00** |
| `00476160` | Container_Frame_GetAnimClone | (same) | 32 | 100.00 | **100.00** |
| `00476180` | Container_Frame_GetFrameCount | (same) | 32 | 100.00 | **100.00** |
| `004761a0` | Container_Frame_GetCurrTime | (same) | 32 | 100.00 | **100.00** |
| `004761c0` | Container_Frame_GetTransCo | (same) | 32 | 100.00 | **100.00** |
| `004761e0` | Container_Frame_GetSample | (same) | 32 | 100.00 | **100.00** |
| `00476200` | Container_Frame_GetTrigger | (same) | 32 | 100.00 | **100.00** |
| `00476220` | Container_Frame_SafeAddChild | (same) | 80 | 100.00 | **100.00** |
| `00476270` | Container_Frame_FormatName | (same) | 128 | 24.19 | 95.56 |
| `004762f0` | Container_Frame_FreeName | (same) | 32 | 100.00 | **100.00** |
| `00476310` | Container_Frame_GetName | (same) | 32 | 100.00 | **100.00** |
| `00476330` | Container_Frame_WalkTree | (same) | 224 | 42.79 | **100.00** |
| `00476410` | Container_Frame_SearchCallback | (same) | 432 | 16.41 | 99.44 |
| `004765c0` | Container_LoadAnimSet | (same) | 432 | unmapped | **100.00** |
| `00476770` | Container_GetAnimFileFrameCount | (same) | 112 | 47.89 | 97.67 |
| `004767e0` | Container_FrameLoad | (same) | 112 | 16.67 | **100.00** |
| `00476850` | Container_MeshLoad | (same) | 176 | 11.76 | **100.00** |
| `00476900` | Container_TextureLoadCallback | (same) | 752 | unmapped | **100.00** |
| `00476bf0` | Container_YFlipTexture | (same) | 240 | 73.03 | **100.00** |
| `00476ce0` | Container_TextureSetSort | (same) | 48 | unmapped | **100.00** |
| `00476d10` | Container_TextureDestroyCallback | (same) | 64 | 40.00 | **100.00** |

The second column is the unit's name. The third column is the name the
function links under in the matching build (`container_link_names.h`),
when that name is different.

### What is left

- **Call names the original listing gives differently.** No instruction
  differs in these functions:
  - `00473c60`;
  - `00475a70`, `00475d10`, `00475e40`;
  - `00476270` (vsprintf);
  - `00476410` and `00476770` (string and `_pctype` operands the original
    listing leaves unlabelled).
- **Rule B (finding 538).** `00473e10` and `004750b0` match exactly when
  compiled alone, before blocks B and D were merged into the unit. In the
  full unit they evaluate their first two colour channels in the other
  order.
  - Seven expression shapes of `00473e10` all give the same code,
    including explicit `0xff` shifts and reordered terms.
  - Moving the unit's five static forward declarations changes nothing
    here.
  - Both still differ on #703's base.

  The cause is the symbol counter's phase.
- **`00472710` Container_Initialise** (80.43%). The original loads `""`
  into `edx` before the first `rep stosd` and stores the extension names
  in source order. The rebuild loads it after the `rep stosd` and stores
  both empty names early. The `""` is Config.c's pooled
  `s_config_angle_empty`.
- **`00473370` Container_SetActivity** (80.39%). The original keeps the
  old activity name in the parameter slot of `cont` and the name to free
  in `ebp`. The rebuild gives the second a stack slot.
  - All 120 orders of the five local declarations score the same.
  - One variable for both names scores 76.84%.
- **`00472cd0` Container_Load** (85.36%) and **`00473690`
  Container_Clone** (83.22%): register and stack-slot assignment.
  - Block-scoped locals reproduce the original's slot sharing; they raised
    Load from 70.33% to 82.27%.
  - What remains is that the original keeps 0 in `ebx`.
  - `tools/msvc_permuter.py` made no progress in 300 s on Load.
- **`00475830` Container_SearchOwner** (85.71%): the loop's exit test.
- **`00473ba0` Container_IsHidden** is naked asm under
  `#if defined(_MSC_VER) && defined(_M_IX86)`, with C in `#else`.
  - No C form produces the original's `and eax, 0ffh` before the shift.
    Eight forms were tried; each scores 25% or 50%.
  - The port's body was the same naked asm.

## Unit extent

- **Call graph.** A scan of `.text` for `E8`/`E9` calls and immediates
  finds 110 functions in `00472710`-`00476d50`, each called or referenced
  from inside or outside the unit.
  - Three are missing from the map because Ghidra merged each one into the
    function before it:
    - `00473f70`: Container_SetPerspectiveCorrection's walk callback,
      referenced at `00473f11`;
    - `00476900`: Container_TextureLoadCallback, the load callback that
      `004765c0`, `004767e0` and `00476850` pass;
    - `00476ce0`: Container_TextureSetSort, the `qsort` comparator passed
      at `00476b37`.
  - The map's note that `00476bf0` and `00476d10` are uncalled is wrong.
    `00476850` calls `00476bf0` (Container_YFlipTexture) and passes
    `00476d10` (Container_TextureDestroyCallback) as a destroy callback.
- **Data.**
  - The unit's state is one common block, `0076ad80`-`0076cd98`
    (`containerGlobs`, 0x2018 bytes). Container_Initialise clears it with a
    `rep stosd` of 0x806 dwords. Only `00472710`-`00476bcf` reference it.
  - Viewports.c's commons end at `0076ad6c`, and `0076cd98` is LIBC's.
  - The unit's `.bss` is Container_FormatPartName's 1,024-byte result at
    `00505400`. Only `00473c60` references it.
  - Weapons.c's `.bss` ends at `00505400` (finding 582), and Main.c's
    starts at `00505800`.
- **Literal pool.** `.rdata` `0049e87c`-`0049e89c` holds:
  - d3drmdef.h's four `static const` values: 1, 2, 4 and -1
    (D3DRMMAP_WRAPU, D3DRMMAP_WRAPV, D3DRMMAP_PERSPCORRECT and
    D3DRMGROUP_ALLGROUPS). `00473f70` and `00474e30` read the third from
    memory.
  - then `1.0f 0.0f 255.0f 1/255.0f`.

  Viewports.c's copy of the four values starts at `0049e89c`. Weapons.c's
  `10000.0f` is at `0049e878`.
- **Strings.** They run from `004aaabc` ("FRAME") to `004aab98`
  ("DDiScene"), and Main.c's `__FILE__` follows. The "::" at `004aab18` is
  the string Config.c binds as `s_config_separator`.
- **Edges.** `00472710` follows Movie.cpp's `movie_free` (finding 618), and
  `00476d50` starts Viewports.c (finding 593).

`translation-units.spec.csv` records this evidence in G98-002's note, with
the name `Container.c` (`inferred`). `python3 tools/translation_units.py
build`: 0 conflicts.

## How the conversion is done

### State

`Container_Globs containerGlobs` is a plain file-scope definition, so C
places it in the common region at `0076ad80`:

| Offset | Field |
| ---: | --- |
| `+0x0000` | 20 pool blocks |
| `+0x0050` | the free list |
| `+0x0054` | the root container |
| `+0x0058` | the nine type names ("NULL", "MESH", "FRAME", "ANIM", "ACT", -, -, "LWS", "LWO") |
| `+0x007c` | the nine file extensions |
| `+0x00a0` | the game name |
| `+0x00a4` | four visuals (Container_Mesh_Swap's buffer) |
| `+0x00b4` | 1,000 cached textures, 8 bytes each |
| `+0x1ff4` | the texture count |
| `+0x1ff8` | the sound trigger callback and its data |
| `+0x2000` | the trigger frame callback and its data |
| `+0x2008` | the shared texture directory |
| `+0x200c` | the fog colour |
| `+0x2010` | the pool's block count |
| `+0x2014` | the flags |

`make unit-data-offsets`, G98-002 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| G98-002 | partial (17, 1 unknown) gap 6 first 004aab1c: 60/5c | full (2, 0 unknown) | n/a (0, 0 unknown) fewer than two attributable items | 0076ad80:_containerGlobs(size=8216) |

The `.data` gap is in the strings after "::". The rebuild binds "::" to
Config.c's `s_config_separator`, which now has C linkage. So container.c's
own strings after it sit 4 bytes earlier than in the original.

The port's words for the same state lose their `// GLOBAL` annotations but
stay where they are, because native code uses them:

- `crt.cpp`'s flag word, trigger hooks and shared directory
  (`0076cd78`-`0076cd94`);
- `view.cpp`'s root (`0076add4`) and fog colour (`0076cd8c`).

### Callees

- The unit keeps its own annotated `static const` copies of the IIDs it
  uses, as `movie_unit.cpp` does: IID_IDirectDrawSurface,
  IID_IDirect3DRMFrame, IID_IDirect3DRMMesh, IID_IDirect3DRMMeshBuilder3 and
  IID_IDirect3DRMTexture. IID_IDirect3DRMFrame3 is `resource.cpp`'s.
- Config.c's "::" (`s_config_separator`) gets C linkage, like its ":" and
  `""` (finding 581).
- Main.c's fog method word (`00505920`, defined in `view.cpp`) gets C
  linkage. `view.cpp` and `container_port.cpp` declare it `extern "C"`
  before they include the frozen `view_for_container_unit.hpp`, and the
  header's later declaration keeps that linkage.
- Mesh.c is mesh_unit.c (#703, finding 625). In the matching build
  container.c calls it by the names it has there: `mesh_unit.h`'s macros
  and `mesh_link_names.h`'s port names for most entries, and `mesh_scale`
  and `mesh_set_vertices` for Mesh_Scale and Mesh_SetVertices
  (`container_imports.h`, C only). Five port entries (`004821a0`,
  `00482370`, `004823e0`, `00482470`, `00482780`) get `extern "C"` in place
  in `record.hpp`, for the PORT-ONLY adapters at the end of
  `container_port.cpp` that other builds call instead.
- In the other direction, Mesh.c's `mesh_imports.h` now names
  Container_LoadTextureSurface (`00474050`) and Container_SearchOwner
  (`00475830`) directly in the matching build, in place of
  `mesh_port.cpp`'s adapters, which stay for other builds.
- AnimClone.c's `anim_clone_remove` and `anim_clone_set_time` are called by
  their matching-build names (`container_link_names.h`).
- Native builds map `_strlwr` to `crt_string_to_lower` (`crt/vc5names.h`).

### Callers

In the matching build, `container_link_names.h` gives 70 of the unit's
functions the C names their callers already use. Every file that defines
one of those names compiles it out of the matching build
(`LRR_CONTAINER_PORT_NAMES`):

- `container_port.cpp`: 51 names;
- `entity.cpp`: `container_load`;
- `surface.cpp`: `container_make_mesh2`, `container_mesh_hide_group`,
  `container_mesh_set_vertices` and
  `container_mesh_perspective_correction_set`;
- `view.cpp`: `ddiscene_light_create`;
- `lego_port.cpp`: Initialise and the fog functions;
- `legoobject_port.cpp`: Hide2 and AddTransform;
- `vehicle_model_port.cpp`: SetPerspectiveCorrection;
- `tile_variants_port.cpp`: the two texture functions, GetGroup, Scale and
  InverseTransform.

Every C caller passes the original's arguments, so no caller's source
changes. Natively the port serves every name and nothing calls the unit.

Container_Frame_SetAppData takes the port name
`ddiscene_animation_hook_create`. That name's port contract has ten
arguments; the original has eleven, the last one 0.

- In the matching build, `container_frame.h` declares it with an eleventh,
  unnamed parameter and adds a macro that passes 0. So Sound3D.c pushes the
  original's eleven arguments, and `0047b1a0` rises to 100%.
- Natively nothing changes.
- The port's C++ callers of that name (`view.cpp`'s wrapper) are dead code
  in the matching build.

Four original-ABI adapters keep calling the port, because the unit
function each one stands for already takes another name:

- `lego_light_penumbra_set_adapter` and `lego_light_umbra_set_adapter`: the
  unit serves `light_interface_call_slot_40` and `_3c`;
- `legoobject_gs_container_is_hidden_adapter`: the unit serves
  `scene_object_flag_bit3_get_target`;
- `renderer_fog_color_set_original`: the unit serves
  `lego_fog_colour_set_adapter`.

Interface.c's `container_get_position` adapter returns a 2D point, not the
original's vector, so it is not switched.

Callers outside the unit that rose (`make reccmp-check` against #703's
baseline):

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `00401000` | `advisor_scene_controller_initialize` | 98.48 | **100.00** |
| `00401270` | `advisor_config_slots_load_target` | 96.97 | 97.98 |
| `00406990` | `entity_type_config_resource_row_load` | 97.60 | 98.08 |
| `00407d60` | `entity_building_type_config_record_parse` | 99.57 | **100.00** |
| `0040b310` | `terrain_cell_resource_grid_load` | 98.81 | **100.00** |
| `0040b410` | `terrain_cell_resource_grid_dispose` | 97.44 | **100.00** |
| `0040b5f0` | `tile_model_entity_register_original` | 89.29 | 93.45 |
| `0040bb90` | `detail_set_mesh_frame` | 41.98 | 43.15 |
| `0040c0d0` | `effect_load_rock_fall_style` | 97.62 | **100.00** |
| `0040c1b0` | `indexed_resource_handle_set_create` | 98.04 | **100.00** |
| `0040c720` | `global_resource_handle_load_and_enable_original` | 92.86 | **100.00** |
| `0040c920` | `effect_load_misc_effect` | 95.56 | **100.00** |
| `0041fb50` | `lego_game_config_init` | 94.81 | 94.84 |
| `00422850` | `lego_misc_object_resources_load` | 96.19 | 99.84 |
| `00424730` | `lego_sine_phase_midpoint_blend_dispatch` | 90.91 | 95.45 |
| `00429890` | `lego_level_load` | 92.42 | 92.51 |
| `00433770` | `lego_scene_lights_build_from_config` | 96.35 | 98.31 |
| `00449cd0` | `legoobject_scene_object_state_handle_apply` | 91.67 | **100.00** |
| `0044b7a0` | `legoobject_matrix_rotate_z_with_offset` | 97.83 | **100.00** |
| `0044e0c0` | `terrain_map_create` | 51.49 | 52.38 |
| `0044e4d0` | `terrain_map_vertices_build` | 99.17 | **100.00** |
| `0044e6d0` | `terrain_map_block_texture_fade_set` | 97.52 | 98.35 |
| `0044edf0` | `terrain_map_block_rotated_set` | 99.28 | **100.00** |
| `0044f090` | `terrain_map_transitions_update` | 97.62 | **100.00** |
| `0044f230` | `terrain_map_block_texture_set` | 99.03 | **100.00** |
| `0044f490` | `terrain_map_perspective_correction_set` | 97.22 | **100.00** |
| `00450700` | `terrain_map_block_hide` | 92.31 | **100.00** |
| `00451230` | `terrain_map_block_vertex_normal_set` | 98.25 | **100.00** |
| `00451450` | `terrain_map_block_vertex_offset` | 98.33 | **100.00** |
| `004519b0` | `model_entry_record_load` | 97.62 | **100.00** |
| `00462da0` | `roof_initialise` | 98.15 | **100.00** |
| `00462e60` | `roof_set_texture` | 95.45 | **100.00** |
| `00462ed0` | `roof_shutdown` | 94.74 | **100.00** |
| `00462f20` | `roof_set_block_roof_vertices` | 98.51 | **100.00** |
| `00463230` | `roof_update` | 72.43 | 72.97 |
| `004634f0` | `roof_hide_block` | 95.24 | **100.00** |
| `00463540` | `roof_show_hidden_blocks` | 95.83 | **100.00** |
| `0046c030` | `render_resource_binding_initialize` | 93.33 | **100.00** |
| `0046c6f0` | `lego_vehicle_load_adapter` | 98.97 | 99.48 |
| `0046cff0` | `scene_record_template_instantiate` | 97.50 | **100.00** |
| `0046de80` | `water_init_vertices` | 99.08 | **100.00** |
| `0046e390` | `water_update` | 98.99 | 99.50 |
| `0046ff70` | `weapons_projectile_find_free` | 90.00 | **100.00** |
| `0047b1a0` | `sound_instance_track_register` | 94.23 | **100.00** |
| `004827f0` | `mesh_render_callback` | 99.60 | **100.00** |
| `00482e70` | `mesh_load_texture` | 90.26 | 91.56 |
| `004869f0` | `lws_parse` | 94.43 | 94.80 |

### Source shapes worth noting

- Block-scoped locals reproduce MSVC's stack-slot sharing in
  Container_Load, where `transCo`, the file size and the frame count share
  one slot.
- Locals that MSVC places in dead parameter slots need no special form:
  Container_MakeLight's light, and the frame pointers of
  Container_SetPosition and its siblings.
- Container_ClearTransform writes the identity matrix column by column.
- Branch order:
  - Container_Mesh_Swap tests `!restore` first;
  - Container_SetParent and Container_GetFrames test for NULL first;
  - Container_Mesh_GetVertices and Container_Mesh_SetVertices assign the
    result, then replace it with -1.
- Container_Mesh_SetQuality leaves its quality local unset for an unknown
  quality, as the original does; there it shares a register with the group.
- Container_Mesh_IsGroupHidden returns the group when the mesh has no group
  records, as the original does.
- Container_MoveAnimation adds into two named locals.
- A helper agent wrote blocks B and D (`00473ec0`-`00474760` and
  `00475a70`-`00476d50`). The shapes it found:
  - `00473f70` and `00475f70` need if/else, not ternaries;
  - in `00474050` the surface check tests `image.rgb`;
  - `004744f0` assigns the texture before the surface;
  - `00476410` depends on the order of its local declarations;
  - `00476900`'s trim loop needs `while (1)`.

No `Deviation (scoring only)`.

## Checks

- `make reccmp-check`: OK. No function fell below the baseline.
- The plain-immediate check, `winapi-link-check` and `check-map`: OK.
- `make match-progress`: baseline, listing digests and `binary-progress.json`
  regenerated. `binary_progress_inputs.txt` lists the new files.
- `make unit-data-offsets`: the row above.
- These pass:
  - `make tools-test` (no `--allow-growth`; the lint ledger shrinks from
    1,024 to 1,002 entries);
  - the native build and `ctest` (37 of 37);
  - `gcc -std=gnu89 -Werror=declaration-after-statement` on the unit.

## `/OPT:REF` (finding 579)

`make opt-ref-callers` for `container.c.obj`: 3 functions removed (2
exact). Each is reached only from removed Lego.c, Detail.c or Mesh.c
functions (`00433770`, `0040bb90`, `004827f0`). Before this change,
`container_unit.cpp` had 53 removed (finding 579). The totals move from
363 / 215 to 305 / 182 (full).

## Stacked on #703

First measured on `main` `d4c4cf5c`: unit 25.32% to 96.61%, 33 to 93
exact, exact-function bytes 208,965 to 225,348, `opt_ref` 398 / 220 to
359 / 205, relink `.bss` 1,304,096 to 1,312,320, image end `0x892000` to
`0x897000`. Rebased onto #703 (`a30f398f`, Mesh.c), three more unit
functions became exact (`00472a50`, `00475020` and `00475c80`), whose calls
now reach mesh_unit.c by its annotated names. #699 marks the strings
`004a4014` and `004a4024` in lego_unit.c, so container.c no longer marks
them. `containerGlobs` is unchanged at `0076ad80`: #700 retired port
`.bss` only, and Mesh.c's `meshGlobs` is in its own range.

## Reproduce

    python3 tools/convert_unit.py G98-002 --stem container --out <scratch>   # with the G98-002 link-order entry removed: the unit was already wired
    make match-function SOURCE=src/gods98/container.c
    make reccmp-check
    make opt-ref-callers OPT_REF_ARGS="--object container.c.obj --min 1"
