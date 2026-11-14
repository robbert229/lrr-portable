# Finding 669 — The identical prefix, external globals structs and the size breaks in `0x401000`–`0x40a000`

Issue #794, part of #388 and ADR 0005. Works the 32 non-exact functions
of Advisor.c, AITask.c, the curve helpers (`bezier.c`), AnimationObject,
Bubbles.c, Collision.c and Construction.c in address order, aiming for an
exact match and, where that stalls, the original's size.

**Result: four functions rise to exact, one of them to its original size,
and none falls.** The main lesson: **MSVC 5.0 compiles reads of a `static`
file-scope struct differently from an external one.** Advisor.c's and
AITask.c's state structs were `static`; as external definitions (with
`= { 0 }`, so they stay in the object's `.bss`), `advisor_transition_worker`
and `aitask_cmd_alloc` compile exactly as the original's.

The identical prefix does not grow. The raw prefix ends 11 bytes in at the
first data reference. The prefix of functions that are exact *and* at their
original address still ends at `0x401210`, whose original keeps a dead
store this compile removes.

| Measure | `main` `1f5d8a1e` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 338,194 (52.63%) | **338,909 (52.74%)** |
| Accuracy, full / game-only | 97.10% / 96.76% | **97.12% / 96.79%** |
| Functions that rose / fell | | **4 / 0** |
| Identical prefix, raw `.text` bytes | 11 (`0x401000`–`0x40100b`) | 11 |
| Identical prefix, exact functions at original addresses | 2 (to `0x401210`) | 2 |
| `.text` identical at original addresses | 34,304 B (5.34%) | 26,223 B (4.08%) |
| Fully identical functions at original address | 14 (631 B) | 8 (384 B) |
| Function-order inversions, full / game-only | 222 / 1 pairs | 222 / 1 pairs |
| Image end (gate) | `0x7f5000` | `0x7f5000` |

`make reccmp-check` passes with no function lower. `make match-progress`,
`make unit-data-offsets` and `make pe-audit` were run and the generated
files committed (`unit-data-offsets` changed no tracked file; data-offset
agreement stays 20 units). `make tools-test` (no `--allow-growth`), `make
winapi-link-check` and native ctest (37 of 37) pass, and the native build
starts and runs with the shipped WADs.

### Why "identical at original addresses" falls

The four exact functions are real gains, but the byte count at original
addresses falls. `aitask_cmd_alloc` was 16 bytes over its slot. That
cancelled most of the shortfall from 0x402c20 (−16) and 0x404280 (−64)
together with AITask.c's two port-only accessors (+32), 0x4065f0 (+16) and
0x406f80 (+16). So AnimationObject's tail, Bubbles.c, Building.c,
Collision.c and most of Construction.c, `0x406fb0` to `0x409940`, sat at
their original addresses only because those errors added up to zero. With
`aitask_cmd_alloc` the right size they all sit 16 bytes early.

They return to their original addresses when 0x402c20 reaches its size, or
0x404280's shortfall shrinks by 16. A count that rewards cancelling errors
is not a target. The correct-size count that #793 is adding measures this
work directly.

## Method

Per-function scores come from `tools/match_function.py` (MSVC 5.0 RTM,
matching-build flags) with variants written over the real source path,
scored and restored (finding 654's in-place rule). No variant ran during a
matching build. Each function also got some of:

- `make phase-probe` (N = 0–127 for 0x4065f0, 0–63 for 13 others);
- the `/Oa` diagnostic (finding 665; never committed);
- `make match-search` (`decls`, `operands`);
- `tools/msvc_permuter.py` (120–180 s).

Rebuilt function extents come from `LegoRR.map` (the next symbol's
address), not from Reccmp's size estimate. Reccmp undercounts the original
when a switch table follows the code (0x404280) and stops at a call for
0x402c20.

## The external-struct rule

`aitask_cmd_alloc` is AITask.c's pool allocator:

```c
if (aiTaskGlobs.freeList == NULL)
    aitask_cmd_pool_grow();
task = aiTaskGlobs.freeList;
aiTaskGlobs.freeList = task->nextFree;
memset(task, 0, sizeof(AITask));
task->nextFree = task;
```

The original tests the head in `eax`, calls the grow function if it is
null, and then always reloads the head into `esi` (0x40 bytes). With
`static AITask_Globs aiTaskGlobs;` MSVC 5.0 keeps the tested head in `esi`,
jumps past the reload on the non-null path, and takes `&task->nextFree`
into `edx` (0x42 bytes, so the next function moves 16 bytes). Making the
struct external changes only this function in the unit (single-function
scores of all 107 annotated functions compared), and it becomes exact.
`/Ox`, `/Os`, `/Ob0`, `/Op`, `/Oa`, `/Ow`, `/G4`–`/G6`, `/GX`, `/Gy` and
eight source forms do not. Advisor.c is the same case:
`advisor_transition_worker` gets the original's register assignment
(current type in `edi`, container in `ebx`) only with an external
`advisorGlobs`. Nothing else in Advisor.c changes.

The data layout is unchanged. A test object compiled with the matching
`cl` shows how MSVC 5.0 places each form:

| Definition | Section | Symbol |
| --- | --- | --- |
| `G g = { 0 };` | `.bss` of the object | external |
| `G g;` | none (common, section 0) | external |
| `static G g;` | `.bss` of the object | static |
| `G g = { 1 };` | `.data` | external |

So `= { 0 }` keeps each struct in its object's `.bss`, as finding 554 does
for Panel.c and Interface.c. The rebuilt map has `_advisorGlobs` at
`004ec6e0` and `_aiTaskGlobs` at `004ecaf0`, the addresses the code already
used. Construction.c's `constructionGlobs` was tried the same way and
changed no function, so it stays `static`.

The same allocator shape is a near miss elsewhere, outside this issue's
units: Mesh.c's `0x004807a0` mesh_obtain_from_list (44.44%, `static
Mesh_Globs meshGlobs`) and `0x0040cf80` electric_fence_create (70.48%).
ADR 0005 step 3 says to use `static` for per-object `.bss`. These results
suggest that where the original's code shows a reload across a call, the
struct should be an external `= { 0 }` definition. A follow-up should test
every unit's `static` globals struct this way.

## Per-function results

"Size" compares the rebuilt extent with the original's, to the next
function. Where a size is wrong, every later function in the image moves by
the difference.

| Address | Function | Start | End | Size | Cause / what was tried |
| --- | --- | ---: | ---: | --- | --- |
| `00401210` | advisor_scene_visual_state_initialize | 78.26 | 78.26 | ok | The original stores the front clip to a stack local (`push ecx`, `fstp [esp+4]`) and never reads it. MSVC removes the store for every form tried (12): two locals, one local twice, an array, `(void)&local`, an assignment to the parameter, a later dead use, `/Op`, external `advisorGlobs`. Single-code under the phase probe |
| `00401270` | advisor_config_slots_load_target | 97.98 | **100** | ok | `atof(argv[1]) * fps` without the `(float)` casts: the original stores each time after the next argument push |
| `00401990` | advisor_transition_worker | 86.36 | **100** | ok | External `advisorGlobs` (above). Declaration order, unsigned type, no local, a position pointer: no change |
| `00402c20` | aitask_callback_00402c20 | 31.37 | 31.37 | **−16** (0x46 in a 0x50 slot; rebuilt 0x40) | MSVC keeps the task's block coordinates from the comparison in `esi`/`edi` for the call. The original reloads both after the flags store and saves no `edi`. Nested `if`s, operand order, casts, `(&x)[0]`, an inline compare, a direct prototyped map call instead of the cast pointer, `/Oa`, `/Ow`, permuter: no change. Passing `search->blockPos` reaches the slot (0x49 bytes) but is not the original's operands, so it was not taken |
| `00403810` | aitask_terrain_cell_record_count_update | 76.83 | 76.83 | ok | `blockPos` and `b` swap `ebp`/`edi`, and the original reloads `b` in the count block. An `else if` without the `goto`, and a second object local: 61.04 |
| `00403d10` | aitask_entity_command_unlink_by_kind | 99.39 | 99.39 | ok | Call target only (a port adapter; finding 665) |
| `00404280` | aitask_cmd_entity_command_chain_tick | 83.07 | 83.07 | **−64** (code 60 bytes short) | 312 differing lines in 2.9 KB: stack-slot choices from the first local on, block order. Not attempted beyond the survey |
| `00404e00` | aitask_object_path_submit_to_grid_target | 98.65 | 98.65 | ok | Call target only (port adapter) |
| `00404fc0` | aitask_place_command_eligibility_check | 99.43 | 99.43 | ok | Depends on the count of locals before it (finding 665) |
| `00405950` | aitask_pending_command_timer_fire_scan | 98.62 | 98.62 | ok | Port adapter's float return (finding 665) |
| `00405c10` | aitask_pending_command_entity_assignment_tick | 62.29 | 62.29 | ok | 1.9 KB; not attempted |
| `00406440` | aitask_cmd_alloc | 65.22 | **100** | **+16 → ok** | External `aiTaskGlobs` (above) |
| `004065f0` | cubic_bezier_point_evaluate | 56.88 | 56.88 | **+16** (0x148 against 0x13e) | x87 scheduling. Phase probe 0–127: 10 codes, the current one best (finding 646) |
| `004067b0` | polyline_segment_lengths_total | 97.22 | 97.22 | ok | One `fadd` before `add esp, 8`. 7 forms (no `dist`, chained assignment, statement and declaration order) give 97.22 or 84.62. Single-code |
| `00406820` | bezier_curve_samples_build | 98.08 | 98.08 | ok | The original's `fdivr` reads `s_bezier_one` (`0049e050`), ours a pooled literal. The source note explains the literal |
| `004068c0` | path_arc_length_interpolate | 75.76 | 75.76 | ok | Single-code; not attempted further |
| `00406990` | entity_type_config_resource_row_load | 99.52 | 99.52 | ok | `""` pooled at another address (data) |
| `00406f80` | activity_runner_state_read | 81.82 | 81.82 | **+16** (0x33 against 0x2f) | The original saves `edi` before the null test (finding 592). An early `return NULL` gives 83.72 at 0x31 bytes, still over the 0x30 slot. 7 forms, single-code |
| `004073a0`, `004074b0`, `00407510` | Bubbles.c registry helpers | 96.67, 94.29, 92.31 | same | ok | 100% in the single-function compile. The immediates compare against a LegoObject.c `.bss` address (`0x557d54` original, `0x5dbb94` rebuilt), data layout owned by #795 |
| `004075a0`, `00407960` | screen_effects_tick, bubble_callback_draw_obj_info | 98.32, 98.15 | same | ok | The same address, and the `0x800000` flag constant that Reccmp symbolises as a data address in both images (finding 647) |
| `004085f0` | scene_activity_change_secondary | 90.00 | 90.00 | ok | `edi` save placement (finding 665, 10 forms); not repeated |
| `004089d0` | collision_segment_point_distance | 92.75 | 92.75 | ok | x87 order in the inlined Maths helpers; single-code |
| `00408b60`, `00408bf0` | point_project_onto_line2d, point2d_project_onto_direction | 62.26, 74.51 | same | ok | Phase-sensitive (best 73.58 and 78.43 at shifts 5 and 2), never 100%. Header-sized gaps as in finding 664, so no pad. x87 in the inlined helpers |
| `00408ec0` | construction_spawn_barriers | 98.68 | 98.68 | ok | Base register of the `shape[i + 1]` compare; not attempted |
| `00409600` | lego_path_remove | 67.48 | 67.48 | ok | Single-code; not attempted |
| `00409790` | construction_zone_start_building | 69.50 | 69.50 | **−16** | The original lays the non-entrance arm (`xor edi, edi; jmp`) out of line after the `ret`. `entrance = 0` first, an inverted condition, nested `if`s, statement order (4 forms) and the permuter keep it inline. Single-code |
| `00409a90` | construction_place_building | 90.00 | **100** | ok | Statement order `objID`, `origin`, `direction` |
| `00409b30` | construction_zone_complete_building | 90.28 | 90.28 | **−16** | `[esi + eax]` against `[eax + esi]` operand order, and the original keeps the compared `y` for the call. 2 operand forms; single-code |

AITask.c's two port-only accessors (`aitask_port_priority_values`/`_states`,
finding 570) add 32 bytes to the unit's end. With `aiTaskGlobs` external
they could move to the port tail, but the tail would need the struct type.
That means a `legoobject_globs.h`-style header with its symbol-counter cost
(finding 538). While 0x402c20 and 0x404280 stay short, moving them puts no
function at its original address, so it is left for the change that fixes
those two.

## Reproduce

```sh
make reccmp-report
make match-function ADDR=0x00406440
make phase-probe ADDR="0x004067b0 0x004068c0 0x00406820" PROBE_ARGS="--range 0 64 1"
```

Identical prefix and bytes at original addresses, from the two `.text`
sections (#793 adds the supported metric):

```python
import struct, sys
def text(path):
    d = open(path, 'rb').read()
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    nsec = struct.unpack_from('<H', d, pe + 6)[0]
    optsz = struct.unpack_from('<H', d, pe + 20)[0]
    sh = pe + 24 + optsz
    for i in range(nsec):
        name, vsize, va, rsize, rptr = struct.unpack_from('<8sIIII', d, sh + 40 * i)
        if name.rstrip(b'\0') == b'.text':
            return d[rptr:rptr + min(vsize, rsize)]
o = text('reference/LegoRR.exe'); r = text('build-reccmp/LegoRR.exe')
n = next(i for i in range(min(len(o), len(r))) if o[i] != r[i])
print('identical prefix: %d bytes, first difference at %#x' % (n, 0x401000 + n))
print('identical at original addresses:', sum(a == b for a, b in zip(o, r)))
```

The exact-and-placed prefix walks `build-reccmp/reccmp-report/results.json`
in original address order and stops at the first function whose `matching`
is below 1 or whose `recomp` address differs from its `address`.

## Files

- `src/game/advisor.c`: `advisorGlobs` external (`= { 0 }`), `0x401270`'s
  products without casts.
- `src/game/aitask.c`: `aiTaskGlobs` external (`= { 0 }`).
- `src/game/construction.c`: `0x409a90`'s statement order.
- `docs/coverage/`: regenerated.

No file owned by #793, #795, #796 or #797 is touched. No `#pragma optimize`
is committed, and no pad or filler was added.
