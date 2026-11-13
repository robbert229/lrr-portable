# Finding 609: GAME-009 (credits loop and damage font) rebuilt in original shape, with a port tail

This is issue #660, part of #388 under
[ADR 0005](../adr/0005-original-shape-c-units.md). GAME-009 had no name.
Its extent is text `0040a0c0`-`0040ab10`, and it holds two things, named
here as OpenLRR names them:

- **The credits loop**, Credits_Play (`0040a0c0`). It loads the credits
  text, preferring `<file>.cconv`, and splits it at `\r\n`. It then scrolls
  the lines, centred, one pixel every 33 ms over the looping AVI backdrop.
  It stops on a key press or a release of the left mouse button. It is the
  AVI player's only caller outside that unit (finding 604).
- **The damage font** (DamageFont.c, `0040a3d0`-`0040aae0`). It is a
  billboard of up to four digit quads and a minus sign that rises over a
  damaged object. Its colour depends on the damage (below 5, below 10, the
  rest), and it fades out over 15 time units.

#496 relocated the unit verbatim into `src/game/screen_effect_quads.cpp`.
This change rebuilds it as the C unit
[`src/game/screen_effect_quads.c`](../../src/game/screen_effect_quads.c),
with these headers:

- [`screen_effect_quads.h`](../../src/game/screen_effect_quads.h);
- [`screen_effect_quads_imports.h`](../../src/game/screen_effect_quads_imports.h);
- [`screen_effect_quads_link_names.h`](../../src/game/screen_effect_quads_link_names.h);
- [`screen_effect_quads_maths.h`](../../src/game/screen_effect_quads_maths.h),
  which holds the inline Maths.h vector helpers, as `sfx_maths.h` does.

The port's code moves to the port tail
[`screen_effect_quads_port.cpp`](../../src/game/screen_effect_quads_port.cpp)
by `git mv`. It becomes a `tail` entry and a Meson source.

"Before" is the *match baseline* at #667's head (`710e54c5`, the RNC
expander). That is stack #668: #664, #665, #666 and #667 over `main`
`4862ceaa`. "After" is this change, rebased onto it as the stack's top
layer, from the full matching build (`make reccmp-check`). The unit's
scores before are the same as on `main`.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (10 functions, weighted by span, 2,640 bytes) | 13.12% | **89.14%** |
| Exact functions in the unit | 1 | **4** (no asm) |
| Mapped functions in the unit | 9 | **10** |
| Exact-function code bytes (`binary-progress.json`) | 195,022 | **196,138** |
| Accuracy, full / game-only | 86.93% / 85.64% | 87.16% / 85.90% |
| Annotated exact functions (full) | 1,682 | 1,686 |
| `opt_ref` annotated (full): removed / exact removed | 477 / 246 | **428 / 207** |
| `opt_ref` annotated (game-only) | 451 / 226 | 421 / 200 |
| Reccmp lint ledger | 1,080 | 1,080 |

The weights are each function's span up to the next one, padding included.
An unmapped function counts as 0%. "Link name" is the name the definition
takes in the matching build (see [Link names](#link-names)).

| Address | Function (OpenLRR) | Link name | Span | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `0040a0c0` | `credits_play` (Credits_Play) | `scrolling_text_display` | 784 | 11.76 | **100.00** |
| `0040a3d0` | `damage_font_cleanup` | `screen_effect_flags_clear_and_release` | 48 | 9.76 | **100.00** |
| `0040a400` | `damage_font_load_frames` | `preload_numbered_bitmaps` | 176 | 19.18 | 96.36 |
| `0040a4b0` | `damage_font_display_damage_over_live_object` | `quad_mesh_render_record_create` | 272 | 15.32 | **100.00** |
| `0040a5c0` | `damage_font_get_next_free` | (its own) | 32 | 29.63 | 90.91 |
| `0040a5e0` | `damage_font_set_damage_number` | (its own) | 352 | 21.89 | 99.25 |
| `0040a740` | `damage_font_mesh_render_callback` | (its own, `static`) | 720 | unmapped | 62.56 |
| `0040aa10` | `damage_font_update_all` | `simtick_table_dispatch_original` | 48 | 33.33 | 94.74 |
| `0040aa40` | `damage_font_update` | (its own) | 160 | 14.71 | 98.33 |
| `0040aae0` | `damage_font_live_object_check_can_show_damage` | `entity_quad_mesh_render_record_allowed` | 48 | 100.00 | **100.00** |

These functions outside the unit rose with no change to their code:

- `004103d0` `main_menu_text_item`, from 88.89% to **100.00%**. It calls
  the credits loop, which is now mapped.
- `004232e0` `lego_engine_tick`, from 97.51% to 97.59%. It calls
  DamageFont_UpdateAll.

## Unit extent

The map gave three functions low confidence. One belongs to the unit and
two do not.

- **`0040aae0`** (DamageFont_LiveObject_CheckCanShowDamage) belongs to the
  unit. Only `0040a4b0` and Lego.c call it. It reads the object's type and
  its StatsFlags2.
- **`0040ab10` and `0040ab30`** are Dependencies.c's (GAME-010). They are
  OpenLRR's Dependencies_SetEnabled and
  Dependencies_Reset_ClearAllLevelFlags_10c. They touch only GAME-010's
  `.bss`:
  - the dependency table: 20 object types × 15 ids × 0xc4-byte records,
    from `004b8bc8`;
  - its `disabled` word at `004c7178`, the table's end. `0040ab30`'s loop
    bound, `004c71fc`, is that end plus the record's `+0x84`.

  #663 (PR #665, finding 612, below this change in the stack) came to the
  same conclusion independently. It moves GAME-010's start to `0040ab10`
  and owns the table as `dependencyGlobs` (`004b8bc8`), and `dependencies.c`
  converts both functions. The port's native models of the two functions
  and `s_grid_rec` stay unannotated in one `#if`-guarded block at the end
  of this unit's port tail.
- **The render callback, `0040a740`**, is newly mapped. Ghidra left it
  unlabelled, so the manifest folded it into `0040a5e0`'s span. Its only
  reference is the `push 0x40a740` in `0040a4b0`, which passes it as the
  mesh's render callback. It is `static` in the unit.
- **Data.**
  - The unit's `.bss` is one block, `004b8a58`-`004b8bc4`: `damageFontGlobs`,
    0x16c bytes. It holds ten digit textures, the minus texture, and ten
    0x20-byte instances (container, mesh, digit count, position, timer,
    flags). Only these functions refer to it. The word `004b8bc4` has no
    reference; the next object's 8-aligned `.bss` starts at `004b8bc8`.
  - The unit's `.rdata` is `0049e0a8`-`0049e0c4`: 1.0, 0.0, 1/15, 2.0,
    -1.0, 3.0, 0.5 and 15.0. The literals are `004a1d64`-`004a1d88`.
    `004a1d60` ("%s") is pooled with Files.c. `004a1d7c` and `004a1d88`
    are marked in `sfx.c`, so the unit marks only `004a1d64`, `004a1d68`
    and `004a1d74`.
- **Two parts or one?** Credits_Play uses no `.bss` or `.rdata`, and
  padding cannot separate objects (finding 505). It may be an object of its
  own (Credits.c) linked just before DamageFont.c. Nothing in the image
  shows a boundary, so GAME-009 stays one unit.

The GAME-009 note in `translation-units.spec.csv` records this. `python3
tools/translation_units.py build` reports 0 conflicts.

`make unit-data-offsets`, GAME-009 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-009 | gaps-only (6, 0 unknown) | n/a (0, 0 unknown) fewer than two attributable items | n/a (1, 1 unknown) fewer than two attributable items | - |

## How the conversion is done

### State

`static DamageFont_Globs damageFontGlobs`, annotated at `004b8a58`. The
port's `s_quad_mesh_render_*` arrays and its screen-effect words stay in
the tail, unannotated.

### Link names

`screen_effect_quads_link_names.h` applies in the matching build only
(`_MSC_VER < 1200`). It gives six unit functions the port's names, and
every caller of those names passes the original's arguments:

- `scrolling_text_display` (Credits_Play). See [Callers](#callers).
- `screen_effect_flags_clear_and_release`, `preload_numbered_bitmaps` and
  `entity_quad_mesh_render_record_allowed`. `lego_unit.c` calls these, and
  so does the port's C++ (`frontend.hpp`, `resource.hpp` and `entity.hpp`
  give them C linkage).
- `quad_mesh_render_record_create`, which `legoobject_unit.c` calls.
- `simtick_table_dispatch_original`, which `lego_unit.c` calls as
  DamageFont_UpdateAll. `lego_port.cpp`'s `PORT-ONLY` adapter of that name
  is compiled out of the matching build.

The tail compiles its own definitions of the four port names out of the
matching build (`LRR_SEQ_PORT_NAMES`, an explicit `#if` as in finding 590).

Natively the unit keeps its own names and nothing calls it. The tail
serves every port name, so native behaviour is unchanged.

### Callers

**The credits loop calls the converted AVI player.** It calls `avi_load`,
`avi_update`, `avi_get_length`, `avi_get_time`, `avi_set_time`,
`avi_blit_to_back_buffer`, `avi_is_ok` and `avi_remove` through
`avi_player.h`'s `extern "C"` entry points, where the original does.

MainMenuTextItem (`004103d0`, `frontend_unit.cpp`, naked asm) calls
`scrolling_text_display`. That name was a `static` port stub, which only
raised a pending flag. In the matching build its forward declaration now
reads `extern "C"`, and the stub is compiled out. The asm's `call` then
reaches Credits_Play, and `004103d0` becomes exact. Natively the static
stub and `frameloop_credits` (in the tail) still run the port's credits.

### Callees

| Original | Called as | Note |
| --- | --- | --- |
| Font_GetHeight `0047a540`, Font_PrintF `0047a180` | `object_field_be4_dword_get`, `font_print_f` | font.c |
| Font.h's Font_GetStringWidth `00401c50` | `formatted_text_printf_width` | advisor.c |
| Input_AnyKeyPressed `0047efb0` | `input_keyboard_down_key_count` | `input.hpp` declaration made `extern "C"` |
| File_LoadBinary `004800a0`, Util_Tokenise `00477440` | `file_load_binary`, `sound_variant_split` | |
| SFX_Update `004651a0` | `sound_subsystem_frame_tick` | `sfx_link_names.h` |
| Main_LoopUpdate `00477f30` | `main_loop_frame_pump` | |
| Mesh_SetGroupTexture `004830c0` | `animated_texture_frame_surface_set` | `scene_resources.hpp` declaration made `extern "C"` (namespace `lego`) |
| Mesh_LoadTexture `00482e70` | `sfx_mesh_load_texture_adapter` | not annotated; sfx.c's adapter |
| Mesh_SetGroupMaterialValues `00483420` | `mesh_group_material_values_set` | C++ linkage only (frozen `record_from_mesh_unit.hpp`); lws.c's adapter |
| Container, Mesh, Stats, LegoObject, Viewport entries | the names `sfx_imports.h` and `legoobject_unit_imports.h` use | |
| `timeGetTime`, `Sleep` | the imports | `gods98/winmm.h` |

Making the two declarations `extern "C"` (`input.hpp`, `scene_resources.hpp`)
changes linkage only. It adds no declaration, and no listing outside the
unit changed.

### Source shapes that matched

- **Credits_Play** (11.76% to 100%):
  - The `\r\n` count runs into `lineCount` itself, which then takes
    `lineCount++`. A separate counter put it in the wrong register (89.72%).
  - The loop is `for (loop = 1; loop < fileSize; loop++)` over `buffer[loop
    - 1]` and `buffer[loop]`, with an `int` size. This gives the original's
    `cmp 1; jle` and its pointer walk.
  - The declaration order comes from a random search over 18 declarations
    (a scratch script, about 1,150 compiles). It gave 89.72% to 98.81%. The
    rest were link names, which are exact in the full build.
- **DamageFont_Update** (98.33%): the position update is
  `vector3d_scale(&dir, &dir, elapsed); vector3d_add(&pos, &pos, &dir)`
  over a local `dir = {0, 0, -1}`. MSVC folds the zero components, as the
  original's `fld 0.0; fadd` shows. Written by component, it scored at most
  74.60%. The expired branch comes first, with `if (timer >=
  DAMAGEFONT_MAXTIME)`.
- **DamageFont_SetDamageNumber** (99.25%): `digitCount++;` after the minus
  sign, then `for (i = digitCount; ...)`. One `inc` is still scheduled
  after the call instead of before it. The single-function loop calls it an
  effective match. Five placements of the increment were tried.
- **DamageFont_GetNextFree**: a pointer loop over `instanceTable`.
- **The render callback** (unmapped to 62.56%):
  - `scale = (timer * (1.0f / 15)) * 2.0f - -1.0f` gives the original's
    `fadd st,st; fsub [-1.0]`. `+ 1.0f` gives `fadd [1.0]`.
  - Viewport_GetCamera goes into a local before Container_GetOrientation.
  - The statement order is cross product, size, scale side, scale up,
    offset. Of the 20 orders tried, this one gives the original's 0x88-byte
    frame.

### What is left

| Address | Score | What is left |
| --- | ---: | --- |
| `0040a740` render callback | 62.56 | x87 scheduling of the cross product and the two scalings. The original stores the cross product to `side` and rescales `up` and `side` from memory, with `size` spilled to the offset's slot. The rebuild scales `side` in registers. Tried: 20 statement orders; inline helpers against explicit components; `size * v` against `v * size`; three ways of sharing `scale`, `size` and `offset`; four `verts` operand orders; about 750 random declaration orders. `msvc_permuter.py` found nothing in 900 s: its passes cover unsigned arithmetic, not x87 |
| `0040a5c0`, `0040aa10` | 90.91, 94.74 | Only the loop bound `004b8bc4`, one past `damageFontGlobs`: the unreferenced word before GAME-010's `.bss`, which Reccmp leaves unnamed. In the rebuild it falls in the following object. `0040a3d0`'s bound `004b8be0` (`dependencyGlobs+0x18`) now matches, since dependencies.c's `.bss` follows the unit's; on `main` it scored 94.12% |
| `0040a400` | 96.36 | The two calls to Mesh_LoadTexture `00482e70`, which has no annotated definition |
| `0040aa40` | 98.33 | The Mesh_SetGroupMaterialValues adapter operand (`00483420` has C++ linkage only) |
| `0040a5e0` | 99.25 | The `inc` placement above |

There is no naked asm and no `Deviation (scoring only)`.

## `s_map_row_span_records` (finding 602)

Finding 602 listed `view.cpp`'s `s_map_row_span_records` (51,456 bytes) as
"read by `screen_effect_quads.cpp`'s unit". That was not so. The relocated
file only declared it, through the frozen `view_for_screen_effect_quads.hpp`.
The buffer's one writer is StartMapRowSpanBufferRecord (`0046eb30`,
`cell_flood_passes.cpp`), and its readers are `view.cpp`'s
`map_row_span_record_count` and `map_row_span_record_get`. GAME-009 never
touches it, so this conversion cannot retire or shrink it, and it is
unchanged. The new unit does not include the frozen header. The buffer
goes with the conversion of `0046eb30`'s unit.

## `.bss`

- **The gate build** gains `damageFontGlobs`, 364 bytes, beside the
  tail's port state. `s_grid_rec` only moves, with the tail.
- **The `/OPT:REF` relink**: `.bss` grows from 1,668,608 to 1,668,960
  bytes (+352). On `main` it was 1,608,544 to 1,608,896.
- `s_map_row_span_records` is unchanged; see the previous section.
- The plain-immediate check (finding 584) passes.

## `/OPT:REF` (finding 579)

| | before (#667 `710e54c5`) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full`: removed / exact removed | 477 / 246 | **428 / 207** |
| `opt_ref.annotated.game_only` | 451 / 226 | 421 / 200 |

`make opt-ref-callers`:

- **`screen_effect_quads.c.obj`: none removed.** On `main`,
  `screen_effect_quads.cpp.obj` lost `0040a5e0`.
- **`avi_player.cpp.obj`: 1 removed, from 27 (24 exact) before.** The
  leaner link now keeps the AVI player's methods, getters and vtables. The
  only one it drops is `avi_initialise` (`0047ec80`), because the port's
  WinMain calls `winmain_application_run` instead.
- **AVIFIL32 is imported by the relink.** Its DLLs go from 8 to 9, and its
  import thunks from 5 to 10.

So the relink drops 49 fewer annotated functions (39 fewer exact). That
covers the AVI player's 26 and the unit, and the rest comes through the
credits loop's other callees.

## Entropy and drops

`make reccmp-check` against the baseline at #667's head (`710e54c5`):

- 10 functions improved and 1 is newly mapped (`0040a740`);
- with #664's LIBC `operator new`, the credits loop's calls into the AVI
  player link and score as before (`0040a0c0` exact). The unit touches
  neither `resource.cpp` nor `rnc.c`;
- **none is below the baseline**, and no exact match was lost;
- the plain-immediate check, the winapi link check and the link-order map
  check pass.

Listing digests changed only at the unit's addresses, so the instruction
counts outside the unit are unchanged:

| Address | before | after |
| --- | ---: | ---: |
| `0040a0c0` | 418 | 253 |
| `0040a3d0` | 19 | 17 |
| `0040a400` | 91 | 55 |
| `0040a4b0` | 144 | 88 |
| `0040a5c0` | 16 | 11 |
| `0040a5e0` | 133 | 134 |
| `0040a740` | — | 196 |
| `0040aa10` | 31 | 19 |
| `0040aa40` | 76 | 60 |

## Files outside the unit

- `frontend_unit.cpp`: `scrolling_text_display`'s declaration and the
  stub's `#if` (matching build only).
- `lego_port.cpp`: the `#if` around `simtick_table_dispatch_original`.
- `gods98/input.hpp`, `gods98/scene_resources.hpp`: one `extern "C"` each.
- `test_c_boundaries.cpp`: includes the unit's header and imports header.
- `link_order.cmake`, `meson.build`, `tools/binary_progress_inputs.txt`.

None of these files belong to #447 (CRT and link), #661
(`gods98/resource.cpp`), #662 (`info_track_overlay.cpp`) or #663
(`dependencies.cpp`, `pointer.cpp`). On the rebase, the GAME-010 block of
the port tail takes #665's form. No frozen `_for_`/`_from_` header is
edited.

## Reccmp lint ratchet

The ledger stays at 1,080. It had no entry for this unit, the port's
definitions sit in a tail file, and nothing new needs an annotation.

## Native behaviour

Unchanged. The tail serves every port name. The unit compiles natively as
C++ under its own names, and nothing calls it. Natively the credits are
still `frameloop_credits` over FFmpeg (`movie.cpp`). There is no new native
gap.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK, as above.
- `make match-progress` and `make unit-data-offsets`: run, with the
  generated files committed.
- `make tools-test`: passes without `--allow-growth`. That covers the lint,
  link order, the winapi list, Meson parity and the tool tests.
- `make winapi-link-check`: OK.
- Native: `cmake -S . -B build -DTEST_WADS_DIR=<shipped WADs>`, `cmake
  --build build` and `ctest --test-dir build`: 39/39 pass.
- C subset: `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only`. This needs a stub for `RECT` and `Sleep`, because the
  native `winmm.h` gives them to C++ only.
