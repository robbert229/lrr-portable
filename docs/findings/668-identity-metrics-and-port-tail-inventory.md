# 668. Identity metrics, and what the port tail holds in the matching link

Issue #793, toward a byte-identical image. Measured with a full matching build
under the gate's link mode (`/DEBUG /OPT:REF /OPT:NOICF`, finding 642). The
base is `match/794-identity-prefix` `d6f98b83` (#798, finding 669, on `main`
`1f5d8a1e`); first measured on `main` `1f5d8a1e` itself, and the figures
below are the rebased ones. This change edits tools and docs only, so the
build is the same on both sides.

## Summary

1. **Three identity metrics are added to `binary-progress.json` (schema 5).**
   `tools/identity_metrics.py` computes them and `tools/readme_status.py`
   prints them. `make tools-test` covers them.
   - **Correct-size functions:** 2,596 of 2,833 mapped functions (439,362
     original bytes) in the full scope, and 2,305 of 2,538 (388,535 bytes)
     game-only. Reccmp scores 2,427 and 2,139 of them 100%.
   - **Identical prefix:** 11 bytes. The first difference is at `0040100b`
     in AdvisorSceneControllerInit (`00401000`). It is the address of
     `advisorGlobs`, because data does not yet sit at the original
     addresses. With address operands masked, the prefix is 528 bytes, up to
     `00401210` (AdvisorSceneVisualStateInitialize). #798's four new exact
     functions do not move either prefix (finding 669 says the same).
   - **Per-unit size agreement:** 0 of 227 units. 166 have all sizes
     correct, 2 start at the original address and 1 ends there. On `main`
     `1f5d8a1e` it was 2 (GAME-005 and GAME-006), with 6 and 4; #798 leaves
     AITask.c 48 bytes short instead of 32, which shifts every later unit by
     another 16 bytes.
2. **The port tail does not move the entry point.** The entry point is
   `0048d0e0` against the original's `0048f000`, 7,968 bytes early. The unit
   segment ends 7,984 bytes early (`0048b9f0` against `0048d920`), and the
   LIBC bytes before `WinMainCRTStartup` are 16 bytes longer. The tail is
   linked after the late units and does not affect either figure.
   **GAME-028 (NERPs.c) accounts for 6,528 of those bytes.** Its NERP
   function table points at 172 adapters in `tutorial_script_port.cpp`
   (6,464 bytes of tail code) instead of functions in the unit.
3. **The image is 540,672 bytes larger than the original.** `.data` accounts
   for 327,680 of them, mostly the tail's `.bss`; `.text` accounts for
   200,704 and `.rdata` for 12,288. The tail and the legacy file hold
   203,168 bytes of `.text`, 291,329 of `.bss`, 21,455 of initialised `.data`
   and 15,777 of `.rdata`. That covers the ~199 KB `.text`, ~23 KB `.data`
   and ~11 KB `.rdata` excess that the issue names.
4. **The tail is one link cluster.** LINK pulls archive members before
   `/OPT:REF` removes unreferenced COMDATs. A tail object referenced only by
   a function that `/OPT:REF` later discards is still pulled, and its
   non-COMDAT sections keep what they point at. A COFF-relocation model of
   this (`tools/port_tail_inventory.py`) reproduces the linked tail code
   exactly: 203,168 of the 715,008 bytes compiled. 18 tail objects are
   referenced from units. Each of 17 of them alone keeps at least 175 KB of
   tail code linked. **One table, `tickers.cpp`'s `_TICKERS`, keeps 86,576
   bytes (43%).** It is pulled only through functions the link discards.
5. The follow-ups are ordered by what unblocks entry-point and image-size
   agreement soonest (section 6). They are proposed, not filed.

## Before and after

No function or layout measure changes. Only the new fields are added.

| Measure (`binary-progress.json`) | base `d6f98b83` (#798) | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 338,909 (52.74%) | 338,909 (52.74%) |
| Accuracy, full / game-only | 97.12% / 96.79% | same |
| `.text` identical at original addresses | 4.08% | same |
| Image end (gate) | `0x7f5000` | same |
| Function-order inversions, pairs: full / game-only | 222 / 1 | same |
| Correct-size functions, full: count / original bytes | — | **2,596 / 439,362** |
| Correct-size functions, game-only: count / original bytes | — | **2,305 / 388,535** |
| Identical prefix (raw / address operands masked) | — | **11 / 528 bytes** |
| Units with size agreement (correct sizes, start, end) | — | **0 of 227** (166, 2, 1) |

On `main` `1f5d8a1e` the same metrics read 2,594 / 439,088 and 2,303 /
388,261 correct-size functions, the same prefixes, and 2 of 227 units (166,
6, 4).

`make reccmp-check` reports no function lower. The match baseline and the
listing digests change only in their input fingerprint, because the metric
code is a hashed input. One existing field moved with the same source:
`opt_ref.by_category` counts two functions (64 bytes) as `port_unit` instead
of `original_annotated` (2,534 to 2,532 annotated functions against
`d6f98b83`; one function, 32 bytes, against `main` `1f5d8a1e`). The gate map's
categories are unchanged. Neither the cause nor the function was traced.
The relink's measure has shown layout noise of this size before (finding
591).

## 1. The metrics

### Correct size

Both images are measured with the same rule, `code_extent`. It counts the
instructions reachable from the function's start, stopping at `ret`, `jmp`,
`int3`, `hlt` and the next known function start. It adds in-function data
after the code that the code addresses (switch jump and index tables), and
leaves out trailing `nop` and `int3` padding. Original starts are the listed
functions plus every Reccmp-mapped address. Rebuilt starts are the map's
`.text` symbols plus the matched entities.

Two other size sources were tried on `main` `1f5d8a1e` and rejected:

- Reccmp's PDB size against `translation_units.py`'s reachable extent
  disagrees on 248 of the exact functions. The PDB size includes switch
  tables, and the reachable extent does not (`aitask_task_type23_create_for_category_mask`,
  `00402890`: 140 against 200).
- The PDB size against the original's span to the next known start, without
  padding, disagrees on 249 of the 2,421. LIBC members' PDB sizes include
  their 16-byte alignment (`_atoi`: 14 against 16). Original functions that
  the list misses stretch the span (NERPs.c's table entries).

With one rule on both sides, 2,422 of the 2,425 compared exact functions
agree (2,418 of 2,421 on `main`). The
other three are MASM routines with secondary entry points: `__local_unwind2`,
`__NLG_Notify` and `__CallSettingFrame@12`. Two mapped entries have no
matched function entity and are not compared.

### Identical prefix

The prefix walks the original `.text` from `00401000` and compares raw bytes
with the rebuilt image at the same addresses. It stops 11 bytes in, at the
first `mov [advisorGlobs], "Advisor_Objective"`. Both operands are absolute
addresses of data that has not reached its original address. That limit
holds until `.data` and the string pool agree. The masked prefix, a
supplementary figure, ignores image-address operands and external branch
targets inside each listed function. It runs through AdvisorSceneControllerInit,
AdvisorAnimationEndCleanup and the padding, and stops at
`00401210`, whose first byte already differs (Reccmp 78.26%; its rebuilt
code is 4 bytes shorter; the range is #794's).

### Per-unit size agreement

A unit's functions are the listed original functions in its `text_start` to
`text_end` range. The unit agrees when all three of these hold:

- every function is mapped with the correct size;
- the first function sits at its original address;
- the last function's rebuilt code ends at its original end.

Padding after the last function is not compared. GAME-001 starts and ends in
place, but some of its functions differ in size. A unit starts in place only
while the size differences before it cancel out. GAME-002 is 48 bytes short,
so GAME-003 starts at `004065c0` instead of `004065f0`. GAME-003 and
GAME-004 are each 16 bytes long, which leaves GAME-005 to GAME-007 16 bytes
early. On `main`, where GAME-002 was 32 short, the same two units brought
GAME-005 to GAME-052 back in place, and GAME-005 and GAME-006 agreed on all
three. So start and end agreement follow
from the 166 units with all sizes correct, not the other way round.

## 2. The entry point

| | Original | Rebuilt | Delta |
| --- | ---: | ---: | ---: |
| Unit segment end (first CRT address) | `0048d920` | `0048b9f0` | −7,984 |
| LIBC before `WinMainCRTStartup` | 5,856 | 5,872 | +16 |
| Entry point | `0048f000` | `0048d0e0` | **−7,968** |

The rebuilt `.text` runs in this order:

1. units, 567,792 bytes;
2. LIBC's first pass, 56,848;
3. import thunks and dinput's data formats, 4,368;
4. the late units, 4,848;
5. the tail, 128,768;
6. the legacy file `level.cpp`, 8,736;
7. the tail, 65,664;
8. one thunk;
9. LIBC's second pass, 4,293 bytes in 29 functions.

The second pass holds the members only the tail needs (finding 658):
`87tran`, `asin`, `fprintf`, `memchr`, `pow`, `powhlp`, `snprintf`,
`strcspn`, `strlwr`, `strnicmp`, `strrchr`, `strspn`, `strtod`, `strtol`,
`strupr`, `ullshr` and `xtoa`.

The unit segment's deficit is the sum of 83 unit objects' span deltas: 27
units are 8,448 bytes short, 16 are 464 bytes long, and 40 are exact.

| Unit | Original bytes | Rebuilt | Delta |
| --- | ---: | ---: | ---: |
| GAME-028 NERPs.c | 18,160 | 11,632 | **−6,528** |
| GAME-026 Map3D.c | 15,376 | 15,088 | −288 |
| GAME-021 Lego.c | 90,800 | 90,528 | −272 |
| GAME-023 LegoObject.c | 88,816 | 88,640 | −176 |
| GAME-031 ObjInfo.c | 2,816 | 2,656 | −160 |
| GAME-017 FrontEnd.c, GAME-020 Interface.c | | | −144 each |
| G98-031 Lwo.c | 6,368 | 6,272 | −96 |
| G98-016 Flic.c | 5,232 | 5,152 | −80 |
| GAME-002 AITask.c | 18,752 | 18,704 | −48 |
| 17 more units | | | −16 to −64 each |
| G98-013 Input.c | 928 | 1,056 | +128 |
| 15 more units | | | +16 to +48 each |

GAME-028's NERP function table (`.data` `004a5948` onward) names 172
`nerps_port_*` adapters in `tutorial_script_port.cpp`. The original has
those functions in NERPs.c itself (`00453920` onward; the function list
misses most of them, see section 1). Rebuilding them in `tutorial_script.c`
would close 82% of the entry-point gap (6,528 of 7,968) and remove the largest root of the
tail.

## 3. The image size

| Section (virtual, page-aligned) | Original | Rebuilt | Delta |
| --- | ---: | ---: | ---: |
| `.text` | 643,072 | 843,776 | +200,704 |
| `.rdata` | 12,288 | 24,576 | +12,288 |
| `.data` with `.bss` | 2,936,832 | 3,264,512 | +327,680 |
| `.idata`, `.rsrc`, headers | 16,384 | 16,384 | 0 |
| **SizeOfImage** | **3,608,576** | **4,149,248** | **+540,672** |

`.data`'s virtual size grows by 324,880 bytes. The tail's and the legacy
file's `.bss` (291,329) and initialised `.data` (21,455) make up 312,784 of
that. The `.bss` belongs to a few objects:

| Object | `.bss` | `.text` |
| --- | ---: | ---: |
| `game/frontend.cpp` | 58,356 | 592 |
| `gods98/resource.cpp` | 52,708 | 2,480 |
| `game/aitask_port.cpp` | 20,120 | 25,104 |
| `game/frontend_port.cpp` | 16,068 | 16 |
| `gods98/model.cpp` | 16,008 | 2,160 |
| `game/effects.cpp` | 15,316 | 1,328 |
| `gods98/surface.cpp` | 11,476 | 4,752 |
| `game/weapon.cpp` | 9,748 | 160 |
| `game/helpwindow_port.cpp` | 8,064 | 0 |

These are the port's own copies of state that the original keeps in its
units' globals. They are pulled with their objects, so they leave the image
when the objects stop being pulled.

The image-end margin below the flag constant `0x800000` is 45,056 bytes
(findings 584 and 647). Any growth of the tail eats into it.

## 4. Inventory

The method is in `tools/port_tail_inventory.py` (tools/README.md). `.text`
bytes come from the map. Data bytes come from each object's COFF sections:
non-COMDAT sections always, and string-literal COMDATs only when the map
attributes them to the object. Two pairs of tail sources share a basename:
`gods98/frameloop.cpp` with `platform/winapi/frameloop.cpp`, and
`game/entity.cpp` with `gods98/entity.cpp`. The map cannot tell them apart,
so each pair is one row.

In the table, "Referenced from" means references from units' code and data
into the object. "Target" is where the object's role belongs: an original
unit or its globals, or native-only (dropped from the matching link). It
comes from the file's own header and from the positional pairs: a tail
reference in a mapped function, paired with the original's reference at the
same position.

| Object | `.text` | `.rdata` | `.data` | `.bss` | Referenced from | Target |
| --- | ---: | ---: | ---: | ---: | --- | --- |
| `game/legoobject_port.cpp` | 40,560 | 3,144 | 292 | 900 | tail only | native-only (GAME-023's relocated port; the unit is `legoobject_unit.c`) |
| `game/aitask_port.cpp` | 25,104 | 120 | 302 | 20,120 | GAME-002: 4 adapters | GAME-023 `0043a5f0`, `00441700`, `00438e50`; GAME-021 `00432550`; then native-only |
| `gods98/lwsc_scene.cpp` | 15,360 | 48 | 2,235 | 0 | tail only | native-only |
| `game/lego_port.cpp` | 10,608 | 1,724 | 1,280 | 24 | GAME-021, 023, 032, 041 | G98-002 `00473460`, `00473480`, `00472950`; GAME-021 `00435aa0`; then native-only |
| `game/tutorial_hud.cpp` | 9,776 | 1,016 | 91 | 820 | tail only | native-only |
| `entity.cpp` (game, gods98) | 8,880 | 0 | 0 | 0 | GAME-002 | GAME-023 `0043f5b0` |
| `game/level.cpp` (legacy) | 8,736 | 512 | 453 | 5,512 | tail only | native-only (none of its 95 linked functions is annotated) |
| `game/scene_render.cpp` | 7,040 | 504 | 0 | 108 | tail only | native-only |
| `game/terrain_map_port.cpp` | 6,784 | 300 | 36 | 0 | tail only | native-only (GAME-026 is `terrain_map.c`) |
| `gods98/lwo_port.cpp` | 6,576 | 52 | 4 | 0 | tail only | native-only (G98-031 is `lwo.c`) |
| `game/tutorial_script_port.cpp` | 6,464 | 368 | 280 | 68 | GAME-028's NERP table: 172 | **GAME-028** (NERPs.c functions) |
| `gods98/surface.cpp` | 4,752 | 136 | 20 | 11,476 | tail only | native-only |
| `gods98/texture.cpp` | 3,552 | 0 | 0 | 20 | tail only | native-only |
| `game/cmd.cpp` | 3,360 | 80 | 0 | 916 | tail only | native-only |
| `game/ai.cpp` | 3,248 | 152 | 224 | 3,408 | tail only | native-only |
| `game/weapons_port.cpp` | 3,072 | 348 | 56 | 260 | tail only | native-only (GAME-050 is `weapons.c`) |
| `gods98/resource.cpp` | 2,480 | 32 | 8,227 | 52,708 | G98-002, G98-025: `s_resource_tree_child_iid` | IID constant `0049f988` (finding 616) |
| `game/actor_scene.cpp`, `game/building_scene.cpp` | 4,752 | 56 | 682 | 132 | tail only | native-only |
| `game/terrain_grid.cpp`, `game/select.cpp` | 4,784 | 228 | 12 | 244 | tail only | native-only |
| `game/lighteffects_port.cpp` | 2,400 | 56 | 0 | 216 | tail only | native-only |
| `game/terrain_cell_objects_port.cpp` | 2,352 | 316 | 56 | 148 | GAME-013: 2 | GAME-023 `004424f0`, GAME-021 `0042f6f0` |
| `game/info_port.cpp` | 2,272 | 60 | 240 | 2,584 | tail only | native-only |
| `gods98/model.cpp` | 2,160 | 4 | 7 | 16,008 | tail only | native-only |
| `gods98/mesh_port.cpp` | 2,032 | 12 | 116 | 256 | G98-005, G98-015: `s_main_im_device` | Main.c global `0050591c` |
| `game/effects.cpp` | 1,328 | 72 | 4 | 15,316 | tail only | native-only |
| `gods98/view.cpp` | 1,136 | 36 | 4 | 1,524 | G98-002, G98-005: `s_scene_fog_mode` | Main.c global `00505920` |
| `frameloop.cpp` (gods98, winapi) | 640 | 152 | 0 | 260 | G98-005, 013, 021, GAME-021, LATE-002 | Main.c globals `00505800`-`0050580c` |
| `gods98/input.cpp` | 480 | 0 | 0 | 5,472 | GAME-017, 020, 021, 027, 037, G98-013 | the original's key arrays (`0076aa39`-`0076ac4d`, commons) |
| `gods98/port_state.cpp` | 32 | 0 | 0 | 4,656 | 15 units | Main.c `00505924`, `00505928`, `00505f84`; renderer words `0076ab24`-`0076ab58` |
| `gods98/engine.cpp` | 384 | 0 | 0 | 92 | GAME-001, 017, 019, 021, G98-005, 028 | Main.c `00505f88`; GAME-021 `0042de40` |
| `game/interface_port.cpp` | 432 | 232 | 287 | 5,044 | GAME-020: 2 | GAME-023 `0043a5f0`; G98-002 `004754c0` |
| `game/frontend_port.cpp` | 16 | 156 | 276 | 16,068 | GAME-037: 2 | GAME-017 `00415700`; save buffer `00557418` |
| `gods98/flic.cpp` | 32 | 0 | 0 | 196 | GAME-032, 033 | G98-025 `004897d0`, G98-016 `004850e0` |
| `gods98/lws_port.cpp`, `gods98/viewports_port.cpp` | 0 | 4 | 0 | 20 | G98-002, 003, 005, 010, 015, 020, 025 | Main.c `00505914`, `00505918` |
| `game/startup_config.cpp` | 48 | 0 | 0 | 564 | G98-028 | G98-028's streams `00575ce0`-`00575ce8` |
| `gods98/tickers.cpp` | 560 | 264 | 286 | 32 | none (pulled by discarded functions) | native-only; **anchor of 86,576 bytes** |
| 27 more objects with code | 10,976 | 4,284 | 2,929 | 96,844 | tail only | native-only |
| 23 objects with data only | 0 | 797 | 2,603 | 23,801 | tail only | their units' globals, or native-only |
| **Total** | **203,168** | **15,777** | **21,455** | **291,329** | | |

("27 more" are the objects with 16 to 1,216 bytes of code: `spiderweb_port`,
`lwsc_winapi`, `rock_fall_styles_port`, `container_port`,
`entity_motion_owner`, `util`, `placement_port`, `building_model_port`,
`cell_flood_passes_port`, `frontend`, `simtick`, `objective`, `entity_config`,
`wad_port`, `screen_effect_quads_port`, `block`, `weapon`, `scene_pool`,
`entity_stats_port`, `camera_control_port`, `files_port`, `media`, `message`,
`spawn`, `objective_port`, `sound` and `scene_resources`. Among them,
`game/frontend.cpp` has 592 bytes of code but the largest `.bss`, 58,356
bytes, and `weapon.cpp` 160 bytes of code with 9,748 of `.bss`. The full
per-object list is `port_tail_inventory.py`'s output.)

## 5. How the tail stays linked

`tools/port_tail_inventory.py` builds a model from the objects' COFF
relocations:

- The objects outside `lrr_tail.lib` are roots.
- A tail object is pulled when any section of a root or of a pulled object
  references one of its externals, whether or not `/OPT:REF` later keeps
  that section.
- A pulled object keeps its non-COMDAT sections.
- A `/Gy` function or string literal stays only when a kept section
  references it.

The model keeps 203,168 bytes of tail code, the linked figure exactly.
Without the archive-resolution rule, it keeps 116,592. The `_TICKERS`
table shows why. `tickers.cpp` is pulled only through `lego_port.cpp`'s
`EngineTick` and `gods98/frameloop.cpp`'s `frameloop_application_init`, and
neither is in the map. Its table is non-COMDAT `.rdata` of `ticker_*` stubs,
which call into `aitask_port.cpp`, `legoobject_port.cpp` and the rest.

| Measure | Code bytes |
| --- | ---: |
| Linked tail code | 203,168 |
| Kept only through `tickers.cpp` `_TICKERS` (`.rdata`) | 86,576 |
| Kept only through `mesh_port.cpp`'s render-dispatch ops and name table (`.data`) | 4,208 |
| Left with no anchor section | 112,384 |

Because of archive resolution, a single reference into the tail pulls the
rest. These are the root objects that units reference, with the tail code
each one keeps on its own:

| Root object | Referenced from | Keeps alone |
| --- | --- | ---: |
| `tutorial_script_port.cpp` | GAME-028 (172) | 201,504 |
| `frameloop.cpp` | G98-005, 013, 021, GAME-021, LATE-002 | 175,840 |
| `lego_port.cpp` | GAME-021, 023, 032, 041 | 175,664 |
| `input.cpp` | 6 units | 175,488 |
| `aitask_port.cpp` | GAME-002 | 175,456 |
| `terrain_cell_objects_port.cpp`, `engine.cpp`, `interface_port.cpp`, `flic.cpp`, `frontend_port.cpp` | | 175,232 to 175,280 |
| `entity.cpp`, `lws_port.cpp`, `mesh_port.cpp`, `port_state.cpp`, `resource.cpp`, `view.cpp`, `viewports_port.cpp` | | 175,216 each |
| `startup_config.cpp` | G98-028 | 48 |

So retargeting one root at a time frees little until the last one goes. The
greedy order "as linked" frees 26,112 bytes with GAME-028's table. The next
16 roots free between 0 and 624 bytes each. The last root, whichever it is,
frees 175,216.

With the anchors gone, the roots have independent effects. In greedy order:

| Step | Retarget | Frees | Left |
| ---: | --- | ---: | ---: |
| 1 | GAME-028's NERP table → `tutorial_script.c` | 34,192 | 78,192 |
| 2 | GAME-002's 4 `aitask_*_adapter` calls | 2,272 | 75,920 |
| 3 | GAME-013's `legoobject_block_position_get`, `level_block_surface_update` | 74,176 | 1,744 |
| 4–12 | `frameloop`, `lego_port`, `input`, `entity`, `engine`, `interface_port`, `startup_config`, `flic`, `frontend_port` | 1,744 | 0 |

The data-only roots (Main.c's globals in `port_state`, `view`, `mesh_port`,
`lws_port` and `viewports_port`; `resource.cpp`'s IID) hold no code here once
the anchors are gone. They still pull their objects, and with them those
objects' `.bss` and `.data`.

## 6. Proposed follow-ups

These are ordered by what unblocks entry-point and image-size agreement
soonest. They are proposed here and not filed.

1. **Rebuild NERPs.c's table functions in `tutorial_script.c` (GAME-028).**
   The NERP table should name the unit's own functions. This closes 6,528 of
   the 7,968-byte entry-point gap, removes the 172-entry root, and frees
   26–34 KB of tail code. It needs the missing functions listed first:
   `00453920` onward is mostly absent from the function list.
2. **Keep `tickers.cpp` out of the matching link.** Mark it native-only, and
   compile out the two discarded functions that pull it (`lego_port.cpp`
   `EngineTick`, `gods98/frameloop.cpp` `frameloop_application_init`) in the
   winapi build. LINK rejects an unresolved external even in a discarded
   function, so those references must go too. This frees 86,576 bytes of
   `.text` without touching an original unit, and lowers the image end by
   about 21 pages.
3. **Retarget the Main.c globals and the key arrays.** Units read `s_main_d3drm`,
   `s_main_device`, `s_main_im_device`, `s_scene_fog_mode`, the display
   words, `s_application_mode`, `s_engine_feature_word`, the window words and
   the key arrays from tail objects (`port_state`, `lws_port`,
   `viewports_port`, `mesh_port`, `view`, `engine`, `frameloop`, `input`).
   The original keeps them in Main.c (`00505800`-`00505f88`) and in the
   commons at `0076aa39`-`0076ac4d` (the key arrays and renderer words; the
   unit map does not attribute them). Defining them there in the original's shape
   removes eight roots and their `.bss`, and is needed for `.data` agreement
   anyway.
4. **Retarget the remaining code roots to the original functions they stand
   in for.** These are GAME-002's four `aitask_*_adapter` calls, GAME-013's
   two calls, `lego_port.cpp`'s five, `interface_port.cpp`'s two,
   `entity.cpp`'s one, `flic.cpp`'s two and `frontend_port.cpp`'s
   `movie_play_wait`. The targets are in section 4's table. Together with 1
   to 3, nothing references the tail, so `lrr_tail.lib` contributes nothing.
   That removes 203 KB of `.text`, 291 KB of `.bss`, 21 KB of `.data`, 16 KB
   of `.rdata`, and LIBC's 29 second-pass functions.
5. **Close the remaining unit-size deficits for the entry point.** After 1,
   the units are 1,456 bytes short net: 1,920 bytes short across 26 units and
   464 bytes long across 16. GAME-026 (−288), GAME-021 (−272) and GAME-023
   (−176) are the largest. #794 to #797 already own the first range and
   Lego.c, LegoObject.c, FrontEnd.c and Interface.c. The per-unit figures are
   in `unit_size_agreement.per_unit`, and `port_tail_inventory.py` prints the
   spans.
6. **`resource.cpp`'s IID** (`0049f988`, finding 616) is the last data-only
   root that is not a Main.c global. It belongs in `.rdata` with the
   original's GUIDs.

## Reproduce

```
make reccmp-check && make match-progress
python3 tools/readme_status.py
python3 tools/port_tail_inventory.py --json /tmp/tail.json
```
