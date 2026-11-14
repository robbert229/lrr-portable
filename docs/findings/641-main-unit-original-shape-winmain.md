# Finding 641 — Main.c in original shape: WinMain's start-up and shutdown

Issue #733, part of #388 and #562. Follows findings
[637](637-opt-ref-reevaluation.md) (`WinMain` is the largest `/OPT:REF`
caller row) and [634](634-lego-near-miss-sweep-4.md) (switching calls to
annotated originals revives the callees).

G98-005 (Main.c, `004777a0`–`00478df0`) is rebuilt as `src/gods98/main.c`
(with `main_imports.h`). `WinMain` (`004777a0`) used to be
`platform/winapi/entry.cpp`, which ran the port's own start-up
(`winmain_application_run`). It now runs the original's start-up, main loop
and shutdown, and every callee that `/OPT:REF` dropped because of the old
`WinMain` is live again.

Measured with `make reccmp-check`, `make match-progress` and `make opt-ref*`
on `main` `db760978` and on this change:

| Measure | `main` `db760978` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 289,516 (45.06%) | **291,112 (45.31%)** |
| Exact functions (full) | 2,146 | **2,155** |
| Mapped functions, full / game-only | 2,663 / 2,418 | 2,664 / 2,419 |
| Reccmp mean, full / game-only | 96.03 / 95.69 | 96.34 / 96.03 |
| Functions that rise / fall | | **17 / 0** (all in G98-005) |
| `opt_ref` removed / removed exact | 124 / 102 | **91 / 74** |
| `WinMain` row of the `opt_ref` callers table | 35 (29 exact) | **0** |
| Image end (gate) | `0x875000` | `0x875000` |

No function fell below the match baseline and no function outside the unit
changed score. `make match-progress`, `make unit-data-offsets` and
`make opt-ref-callers OPT_REF_ARGS=--summary` were run on the final tree, and
the generated files are committed.

## The unit

Step 0 (`tools/convert_unit.py G98-005`) confirmed the extent: 24 annotated
functions, `.bss` `00505800`–`0050608c`, strings `004aabf0`–`004aae20`. The
range also holds one unannotated function, `00478880`, the window class's
procedure (Ghidra's `LAB_00478880`, pushed once by `004783f0`). It calls the
full-screen handler `004784c0` when `MAIN_FLAG_FULLSCREEN` is set and the
windowed handler `004786c0` otherwise. It is now annotated as
`main_window_procedure`, so the unit has 25 functions.

The code is Windows only. `main.c` compiles to nothing natively
(`LEGORRR_BACKEND_MINIWIN`, as `init.c` and `directdraw.c` do). The port's
native bodies of the functions that other modules call stay where they
were, marked `PORT-ONLY` and compiled only natively:

- `frameloop.cpp`: Main_LoopUpdate, Main_HandleIO, Main_GetWindowsBitDepth,
  Main_SetState, Main_GetTime, Main_DispatchMessage, Main_AdjustWindowRect,
  Main_SetTitle;
- `surface.cpp`: Main_ChangeRenderState, Main_RestoreStates,
  Main_SetRenderQuality, Main_SetupDirect3D, Main_SetupDisplay,
  Main_Finalise3D;
- `audio.cpp`: Main_SetCDVolume, Main_GetCDVolume, Main_CDVolume;
- `crt.cpp`: Main_GetCLFlags, Main_DisableTextureManagement.

The port's two window-procedure models in `frameloop.cpp` and the port's
Windows window (`platform/winapi/frameloop.cpp`'s `platform_create_window`)
lose their annotations, and the models are renamed `*_port` so the PDB has
one name per address. `platform/winapi/entry.cpp` is deleted. With no
annotated function left, `frameloop.cpp`, `surface.cpp`, `audio.cpp` and
`platform/winapi/frameloop.cpp` move from `legacy` to `tail` in
`src/link_order.cmake`, and `crt.cpp`'s median key becomes `00493ef0`.

The functions keep the names the port gave them, so no caller changes.
Where a C++ header declared one of them, or a global the unit uses, without
C linkage, `extern "C"` is added in place: `frameloop.hpp` (2 functions and
`s_window_close_requested`), `surface.hpp` (1), `audio.hpp` (1),
`input.hpp` (4), `engine.hpp` (`window_mode_flag_set`), `crt.hpp`
(`s_render_feature_flags`, `s_window_mouse_word_50/54/58`) and
`port_loop_ports.hpp` (`startup_config_stream_load`). In
`file_list_streams.cpp`, the three start-up stream functions
`0048b260`, `0048b290` and `0048b2f0` get `extern "C"` on their definitions.
None of these edits changed an instruction anywhere (the whole-report
comparison below).

### State

Main.c's `.bss` is the original's `mainGlobs` (0x890 bytes, 21 fields). 13 of its fields are
defined and read by the port under its own names, in nine files
(`g_platform_cursor_window_handle`, `g_main_instance_handle`,
`s_window_application_active`, `s_window_close_requested`, `s_main_d3drm`,
`s_main_device`, `s_main_im_device`, `s_scene_fog_mode`, `s_display_w`,
`s_display_h`, `s_engine_feature_word`, `s_application_mode`,
`s_render_feature_flags`). Turning them into one struct
would touch every reader's declaration, including converted units' import
headers that other threads own. The symbol counter of each of those units
would move too (finding 538). So `main.c` reads these fields
by their port names (`main_imports.h`), and defines at their original
addresses only the fields that Main.c alone uses:

| Address | Field (mainGlobs) | Definition |
| --- | --- | --- |
| `00505810` | className | `s_main_class_name` |
| `00505814` | programName[256] | `s_main_program_name` |
| `0050592c` | currState (3 callbacks) | `s_main_curr_state` |
| `00505938` | stateSet | `s_main_state_set` |
| `0050593c` | fixedFrameTiming | `s_frame_interval_override` (moved from `entry.cpp`) |
| `00505940` | renderStates[200] | `s_main_render_states` |
| `00505f80` | style | `s_main_style` (the annotation moves from `frameloop.cpp`) |
| `00505f8c` | startLevel[256] | `s_start_level_name` (moved from `entry.cpp`) |

`IID_IDirect3DRM3` (`0049fbd8`) and `IID_IDirect3DDevice3` (`0049f6d8`) are
the unit's own annotated copies, as in `directdraw.c`. Its copy of
`IID_IDirectDrawSurface` is unannotated, because `container.c` already
annotates `0049f338`. The unit's 35 literal uses carry `// STRING` markers. The empty
window title is Config.c's pooled `""` (`004b8a14`, finding 624).

## Per-function results

Scores are from the final `make reccmp-check`. "Before" is the baseline on
`main`.

| Address | Function (gods98 name) | Before | After | What is left |
| --- | --- | ---: | ---: | --- |
| `004777a0` | WinMain | 31.83 | **100.00** | |
| `00477bd0` | Main_DisableTextureManagement | 100.00 | 100.00 | |
| `00477bf0` | Main_ParseCommandLine | 80.35 | 99.57 (100\*) | `-startlevel` walks a second pointer (`s + 11` in a new register) |
| `00477f30` | Main_LoopUpdate | 60.47 | **100.00** | |
| `00477f70` | Main_GetCLFlags | 100.00 | 100.00 | |
| `00477f80` | Main_GetWindowsBitDepth | 85.71 (100\*) | 100\* | |
| `00477fa0` | Main_Finalise3D | 90.00 | **100.00** | |
| `00477fd0` | Main_SetState | 71.43 | **100.00** | |
| `00478000` | Main_GetTime | 100.00 | 100.00 | |
| `00478010` | Main_DispatchMessage | 92.86 | **100.00** | |
| `00478040` | Main_HandleIO | 62.96 | **100.00** | |
| `004780b0` | Main_SetupDisplay | 25.00 | 97.78 (100\*) | |
| `004781d0` | Main_SetupDirect3D | 12.54 | 98.00 | the `IID_IDirectDrawSurface` operand (container.c's annotated copy is static) |
| `00478310` | Main_AdjustWindowRect | 100.00 | 100.00 | |
| `00478330` | Main_SetRenderQuality | 15.91 | **100.00** | |
| `004783d0` | Main_SetTitle | 100.00 | 100.00 | |
| `004783f0` | Main_InitApp | 12.50 | 96.88 | |
| `004784c0` | Main_WndProc_FullScreen | 23.71 | 51.43 | see below |
| `004786c0` | Main_WndProc_Windowed | 24.31 | 45.71 | see below |
| `00478880` | Main_WndProc | (unmapped) | **100.00** | |
| `004788d0` | Main_ChangeRenderState | 10.00 | **100.00** | |
| `00478940` | Main_RestoreStates | 13.11 | 95.24 | the loop bound `&renderStates[200]` names the next global (`s_engine_feature_word` in the original) |
| `00478980` | Main_SetCDVolume | 100.00 | 100.00 | |
| `004789a0` | Main_GetCDVolume | 100.00 | 100.00 | |
| `004789c0` | Main_CDVolume | 38.20 | 91.29 | `eax`/`ecx`/`edx` rotation in the volume arithmetic |

\* effective match.

Notes on the code shape:

- **WinMain.** The product name is a pointer constant that the code tests
  for NULL (`mov eax, 0x4aac84; test eax, eax`). The loop pointers are
  assigned as `argv0 = s = GetCommandLineA()` and
  `ext = s = programName`. The frame timing compares `0.0f ==
  fixedFrameTiming` (the original loads the constant first). The time step
  is `(float)(now - last) * 0.025f`, computed from an unsigned value. The
  loop calls `timeGetTime` through the import directly, not through
  `Main_GetTime`. After the shutdown, WinMain calls the shared empty function
  `00484c90` with `FALSE`. In the matching build that name is Flic.c's
  `Flic_Nothing` (`flic_opcode27_fallback_noop`). It is called through
  an unprototyped cast, as `lego_unit_imports.h` calls it.
- **Main_SetupDirect3D.** The error code is uninitialised on the first two
  failure paths, as in the original, which keeps it in the dead
  argument slot. This is what reproduces `mov esi, [esp+0x10]`.
- **Main_SetRenderQuality.** The original keeps an `if (quality ==
  WIREFRAME)` whose store the compiler drops (`test eax, eax; jne +0`). The
  texture-quality branches nest filter, then mipmap.
- **Main_CDVolume.** The range `max - min` is one `DWORD`, so its `fild`
  stays on the FPU stack for both channels. The volumes are stored as
  `value + min`, and the mono value is `(left + right) / 2` in unsigned
  arithmetic. When the device has one channel, the right volume is
  `*rightVolume = *leftVolume`, which the compiler forwards through the
  result slot. Assigning `numDevs` and `result` as statements after the
  declarations, and filling `details` as cbStruct, dwControlID, cChannels,
  cMultipleItems, cbDetails, gains 1.4 points.
- **The window procedures.** These are the original's switch trees: dual
  mouse first, a `WM_CREATE` case that only breaks (it makes the first jump
  table cover 1 to 6), and chained stores in the original's order. They
  still miss MSVC's choice of `esi = 1`: the original loads the constant
  into `esi` before the mouse jump table and stores it through `esi`, while
  this build uses `eax` in each case. That is register allocation, so the
  symbol-counter phase (rule B) is the likely cause. A dead local
  `level` pointer in Main_ParseCommandLine moved Main_CDVolume by 2 points,
  which shows how sensitive the unit is to it. Padding declarations would
  be a filler (not allowed), so this is left.

## The `WinMain` call sites

Each call is the original's call, to the annotated original. Under the old
`WinMain`, all 35 functions below these roots were removed by `/OPT:REF`
(finding 637's row). After this change, none of them is.

| Original call | Callee now | Before | After |
| --- | --- | --- | --- |
| `sprintf`, `CreateMutexA`, `GetLastError` | LIBC, kernel32 | port model | original |
| `__strupr` (`0049dcd0`) | LIBC `_strupr` | removed (no call) | live |
| `strstr` (`0048da50`) | LIBC `_strstr` | port model | original |
| Registry_GetValue (`0048b360`) ×2 | `registry_path_accessor` | port wrapper | original |
| Main_ParseCommandLine (`00477bf0`) | `winmain_parse_options` | port model | original |
| `0048b260` (start-up stream reset) | `ledger_stub_0048b260_StartupStreamStateReset` | port | original |
| `004893f0` | `level_map_slots_reset` | removed | live |
| File_Initialise (`0047f130`) | `file_initialise` (`resource_mount_startup_target`) | port | original |
| Config_Initialise (`00478df0`) | `block_text_token_pool_init` | removed | live |
| Input_InitKeysAndDI (`0047ed90`) | `input_direct_initialize` | removed | live |
| Main_InitApp (`004783f0`) | `main_init_app` | port window | original |
| DirectDraw_Initialise (`0047c170`) | `directdraw_initialise` | port | original |
| Sound_Initialise (`00488b50`) | `media_cd_audio_device_state_update` | port | original |
| Init_Initialise (`0049d030`) | `init_initialise` | removed, with Init.c's dialog, the DirectDraw enumeration and its 3 callbacks, `directdraw_setup`, the ddraw thunks and `direct3drm_device_initialize` | live |
| Animation_Initialise (`0047ec80`) | `avi_initialise` | removed, with `_AVIFileInit@0` | live |
| Draw_Initialise (`00485e80`) | `surface_render_setup` | removed | live |
| Gods_Go (`0041fa20`) | `lego_game_config_callback_registry_init` | port | original |
| the state's three callbacks | `s_main_curr_state` | port triplet | original |
| Main_HandleIO, Input_ReadKeys, Input_ReadMouse2, Main_Finalise3D, DirectDraw_Flip | original names | port loop | original |
| DirectDraw_Shutdown (`0047ccf0`) | `directdraw_shutdown` | removed | live |
| Input_ReleaseKeysAndDI (`0047efd0`) | `input_direct_shutdown` | removed | live |
| Config_Shutdown (`00478e20`) | `block_text_token_pool_destroy` | removed | live |
| `00484c90` (shared empty function) | `flic_opcode27_fallback_noop` | port | original |
| `0048b290`, `0048b2f0` (start-up streams) | `ledger_stub_0048b290_StartupStreamsClose`, `startup_config_stream_load` | removed, with Files.c's directory scan, `file_check_file` and `_fscanf` | live |

Main.c's own functions that `/OPT:REF` removed come back as well:
`window_runtime_display_caps_query` (`00477f80`, through Init.c and the
DirectDraw enumeration), `direct3drm_device_initialize` (`004781d0`,
through `directdraw_setup`) and `window_runtime_adjust_rect` (`00478310`,
which the port's `renderer_window_mode_apply` never called). Files.c's
`fileRedundantList` (2,048,000 bytes) is live again. In the original-mode
relink, `.bss` therefore grows from 1,116,672 to 3,165,584 bytes, as
finding 637 predicted. The gate's image end does not change.

## `opt_ref` before and after

| | Removed | Exact | Bytes |
| --- | ---: | ---: | ---: |
| `main` `db760978` | 124 | 102 | 22,956 |
| This change | **91** | **74** | 15,984 |

45 functions are revived. Among them is `_AVIFileInit@0`, which finding 637
counted as part of the floor. It is not: the original's
Animation_Initialise calls it, and the rebuilt `avi_initialise` is now live.
12 functions are newly removed. None of them was reached from the original
`WinMain`; all 12 were kept only by the port's Windows start-up, which
`WinMain` no longer runs:

| Function | Unit | Cause | Owner |
| --- | --- | --- | --- |
| `renderer_word_a/b/c_get` (`00410b30`/`40`/`50`, exact) | FrontEnd.c | the only original caller is Weapons.c's `weapons_model_fire_update`, still below `legoobject_update_callback`'s port weapon calls | #730 |
| `legoobject_animation_state_set` (`00439cd0`), `lego_terrain_cell_previous_state_track` (`00432200`), `noop_callback` (`0041f8e0`), `always_accept_place_command_cell` (`00432550`), `rng_thunk_draw15` (`004798a0`), `file_get_system`, `file_malloc`, `engine_heap_free_wrapper` (Files.c) | LegoObject.c, Lego.c, maths.c, Files.c | `/OPT:ICF` folds: their rebuilt callers do call them, but LINK now keeps the byte-identical twin in another object instead (`engine_memory_allocate`/`engine_memory_free` and `0x484b90`/`0x484c90` came back the other way). The folded column is 13 (12) before and after | `/OPT:NOICF` (finding 637) |
| `formatted_text_buffer_build` (`0048e1e0`, `string.cpp`) | CRT stand-in | its callers call LIBC's `_vsprintf` | #447 |

## What is left, and who owns it

- **`direct3d_render_preferences_apply`** (`00478330`, exact) is still
  removed. Its only original caller, Lego.c's
  `lego_render_quality_config_apply` (`004334f0`), is itself removed, because
  `lego_game_config_init` calls the port's
  `render_quality_config_apply_original` there. Finding 634 left that alias
  out: it moves the counter at `00433380`. Owner: `lego_unit.c` (#731).
- **The window procedures** (51%, 46%) and **Main_CDVolume** (91%) are
  short only by register allocation (above).
- **`mainGlobs` as one struct** needs the port's 13 shared fields to read
  one `Main_Globs` (a follow-up across `frameloop.cpp`, `crt.cpp`,
  `engine.cpp`, `view.cpp`, the `*_port.cpp` files and the converted units'
  import headers).
- **The CRT link (#447, finding 608).** WinMain's C library calls (`sprintf`,
  `strstr`, `_strupr`, `atoi`, `malloc`, `free`) are LIBC.LIB members
  already, so nothing in this unit waits on #447. What does wait on it is
  unchanged: `crt.cpp`'s annotated stand-ins for LIBC statics
  (`string_prefix_reserve`) and the `_vsprintf` callers above.
- **The reference build.** In the MinGW reference build, `WinMain` is now
  the original too, so that build runs the original start-up (Init.c's
  dialog, DirectDraw.c, Gods_Go). The port's Windows runtime
  (`frameloop_application_*`, the `platform/winapi` window) is no longer
  entered. The reference build was not built or run here: no MinGW
  toolchain is installed, and CI does not build it either. The native build
  is unchanged. It builds, its 37 tests pass, and the game starts headless
  with the shipped WADs.

## Reproducing

```sh
python3 tools/convert_unit.py G98-005 --dry-run        # extent and footprint
make match-function ADDR=0x4777a0                     # WinMain, 100%
make reccmp-check && make match-progress
make opt-ref-link && make opt-ref                     # 91 / 74
make opt-ref-callers OPT_REF_ARGS=--summary
```
