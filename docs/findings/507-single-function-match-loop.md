# Finding 507 — Single-function match loop: parity with full Reccmp and timings

Issue #381. Tool: [`tools/match_function.py`](../../tools/README.md#match_functionpy--single-function-match-loop).

## Question

Can one function of the *matching build* be scored in seconds, by compiling
only its translation unit and skipping the link, while still reporting the
score a full `make reccmp-report` would give?

## Method

`match_function.py` compiles the translation unit with the command line CMake
generates for the matching build (`/GX /ML /Zi /O2 /Ob1 /DNDEBUG` plus the
target's defines and include paths), reads the COFF object, and lays it out
as a synthetic image: sections placed, relocations applied, undefined symbols
given slots and names as the linker PDB would name them, Win32 import and
incremental-link (ILT) thunks emulated. It then runs the same sequence of
Reccmp steps as `Compare.run` (marker ingest, the `match_*` passes, import,
SEH, float and string analysis) and scores with Reccmp's `FunctionComparator`.
Function sizes come from the object's CodeView `S_GPROC32` records and line
numbers from its COFF line table, which take the place of the linker PDB.
`LIBRARY` functions that the link takes from VC5's `LIBC.LIB` are scored from
the library member instead (`wincrt0.obj` for startup symbols that every CRT
start-up object defines).

Parity was measured with `tools/match_function_parity.py`, which scores every
annotated function file by file and compares each with the `matching` value of
the same address in a full report. Three trees were checked, each against its
own full `make reccmp-report` run:

* `d3ccdcb8` (source identical to `653caf92`), used while developing the tool;
* `9b31f55b`, two commits older (30 source files, about 2,400 lines
  different; 38 functions score differently between the two full reports),
  checked once, after development, as an out-of-sample test;
* `6f3fe74d` (#392), which moved the Priorities.c functions into
  `src/game/priorities.c`, the first unit the matching build compiles as C
  (`/TC`), also checked once after development.

Ground truth for individual differences came from loading the full
`Compare` object and inspecting its entity database.

## Results

`d3ccdcb8` and `9b31f55b`: **2,334 of 2,335** functions in the full report
score identically (exact float equality of `matching`); `6f3fe74d`: **2,335
of 2,336**. The one difference is the same function in all three.

| Area (`6f3fe74d`) | Identical / compared |
| --- | ---: |
| `src/crt` | 314 / 315 |
| `src/game` | 1150 / 1150 |
| `src/gods98` | 868 / 868 |
| `src/platform` | 3 / 3 |

A further 19 functions are scored by the tool but are absent from the full
report: seven in `gods98/ptrstack.cpp`, eleven in `game/video_graph.cpp` and
`engine_calloc`, whose objects the link never pulls in (or, for
`engine_calloc`, replaces with LIBC's).

Spot checks (identical on `d3ccdcb8` and `6f3fe74d`):

| Area | Address | Function | Built from | Full Reccmp | Single function |
| --- | --- | --- | --- | ---: | ---: |
| gods98 | `0x478e60` | `block_open` | compiled | 21.62% | 21.62% |
| gods98 | `0x4798b0` | `random_float_range` | compiled | 76.19% | 76.19% |
| gods98 | `0x40ff50` | `scene_object_list_apply_float_callback` | compiled | 87.50% | 87.50% |
| gods98 | `0x47f850` | `wad_file_seek` | compiled | 28.57% | 28.57% |
| gods98 | `0x482b50` | `render_state_flags_apply` | compiled | 59.44% | 59.44% |
| gods98 | `0x47efd0` | `input_direct_shutdown` | compiled | 21.43% | 21.43% |
| gods98 | `0x472950` | `lego::ddiscene_object_allocate` | compiled | 28.28% | 28.28% |
| gods98 | `0x43b270` | `simtick_scene_pass` | compiled | 21.13% | 21.13% |
| game | `0x401000` | `advisor_scene_controller_initialize` | compiled | 75.97% | 75.97% |
| game | `0x403f60` | `task_record_kind14_remove_if_head` | compiled | 27.78% | 27.78% |
| game | `0x48b260` | `ledger_stub_0048b260_StartupStreamStateReset` | compiled | 19.05% | 19.05% |
| game | `0x430530` | `terrain_cell_mutate` | compiled | 12.41% | 12.41% |
| game | `0x43bbc0` | `entity_detach_carried_object_and_reset_state` | compiled | 36.11% | 36.11% |
| game | `0x44c970` | `light_effect_blink_parameters_store` | compiled | 50.00% | 50.00% |
| game | `0x429890` | `level_load` | compiled | 7.77% | 7.77% |
| game | `0x40ab70` | `dependency_grid_config_load` | compiled | 11.76% | 11.76% |
| platform | `0x477bf0` | `winmain_parse_options` | compiled | 79.91% | 79.91% |
| crt | `0x498000` | `_memmove` | LIBC.LIB(memmove.obj) | 100.00% | 100.00% |
| crt | `0x48f1a0` | `__amsg_exit` | LIBC.LIB(wincrt0.obj) | 94.12% | 94.12% |
| crt | `0x48f000` | `_WinMainCRTStartup` | LIBC.LIB(wincrt0.obj) | 91.49% | 91.49% |
| crt | `0x48ef10` | `__global_unwind2` | LIBC.LIB(exsup.obj) | 96.30% | 96.30% |
| crt | `0x48ff10` | `__strcmpi` | LIBC.LIB(stricmp.obj) | 98.48% | 98.48% |
| crt | `0x490400` | `__cftoe` | LIBC.LIB(cvt.obj) | 95.35% | 95.35% |
| crt | `0x48e1aa` | `crt_math_intrinsic_dispatch` | compiled | 50.00% | 50.00% |
| crt | `0x48dc80` | `_realloc` | LIBC.LIB(realloc.obj) | 97.66% | 97.66% |
| crt | `0x499b30` | `__isindst` | LIBC.LIB(tzset.obj) | 96.36% | 98.64% |

The one difference, `__isindst`, reads `s_daylight_transition_cache` and
`s_timezone_information`, both `GLOBAL` annotations in `crt.cpp`. The full
link matches the first to a variable whose type size makes offsets into it
render as names; LIBC's `tzset.obj` never references it by name, so here it
has no size and those lines read as matching placeholders. The second is not
matched by the full link. Sizing every unreferenced `GLOBAL` annotation from
the original's layout cannot tell the two apart: it made `__isindst` and about
20 other CRT functions disagree, so the tool leaves them unsized.

Differences found and fixed while developing the tool, each confirmed against
the full entity database, are the behaviour now listed in the tools README:
names for calls into other translation units, ILT thunks for the `program`
target, `jmp [IAT]` thunks for Win32 calls without `dllimport`, LIBC absolute
symbols (`__except_list`), aliases (`__strcmpi`/`__stricmp`), and constants
inside the full link's address span (`0x800000` is inside a large `.bss`
array in that link, so full Reccmp renders it as an address). The span's end
comes from an existing matching-build `LegoRR.exe` when one is present,
otherwise from a default of `0xA00000`.

## Timings

Measured 2026-11-10 on the developer workstation (16 cores, Wine 11.0 in the
`developer` distrobox), `block_open` unless noted:

| State | Wall time |
| --- | ---: |
| Warm wineserver, flags cached | 1.2–1.4 s (compile 0.35 s) |
| Largest file (`game/entity.cpp`, 15k lines), warm | 1.8 s |
| Cold wineserver, flags cached | 1.7 s |
| Fully cold (no wineserver, no flags cache) | 3.0 s |
| 8 concurrent invocations, two cold-configuring separate build dirs | 2.9 s for all |
| 20 functions across 16 files in one call | 4.5 s |
| Every annotated function in the tree (`--jobs 6`) | 18 s |

A full `make reccmp-report` of the same tree takes several minutes. The cold
case used a copy of the Wine prefix so the shared wineserver was not
disturbed. The first configure attempt hung because Wine services started
during CMake's compiler checks inherited its output pipe under a persistent
wineserver; the tool now starts those services itself with detached stdio.

## Reproduce

```sh
make reccmp-report                 # full report for the current tree
make match-function-parity         # compares against build-reccmp/reccmp-report/results.json
make match-function ADDR=0x00478e60 MATCH_ARGS=--timings
```
