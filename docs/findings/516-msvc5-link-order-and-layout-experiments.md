# Finding 516 — How MSVC 5.0's linker orders objects, and what that means for the matching build

Issue #388. These are the facts behind
[ADR 0006](../adr/0006-original-link-order.md) and the
[restructuring plan](../plans/388-original-link-order.md). There are three
parts:

- a small link experiment with the matching toolchain (MSVC 5.0 RTM `cl`,
  `link` and `lib` under Wine);
- measurements of an existing matching build that is source-identical to
  `main` at `041ab287`;
- an inventory of the unit graph and of the layout devices already in `src/`.

Nothing under `src/` or the build changed.

## Summary

1. LINK 5.0 places explicit objects in **command-line order**, in every
   section. That order is deterministic, so it is the link-order mechanism to
   use.
2. Library members are placed in **pull order**, and pull order follows the
   references. Today's matching build is laid out this way. It cannot
   reproduce the original's order unless every object references the same
   symbols in the same order.
3. `/ORDER` works only for external COMDAT functions. It cannot name `static`
   functions, and it does not order data.
4. `/O2` packages every function as a 16-byte-aligned COMDAT and every string
   literal as a `.data` COMDAT. When several objects define a literal, the
   **first object in link order** keeps it.
5. The compiler emits a `static` function **after its first caller**, not
   where the source defines it.
6. Common symbols are not allocated in object order. The order changed with
   the link inputs in a way three links could not explain.
7. Linking without incremental mode, which the binary-progress build already
   does, scores better than the gate's incremental link: 600 exact
   functions against 589, with 55 functions up and 5 down.
8. Reordering today's source files gains little: pair inversions fall from
   873,206 to about 650,000. Moving each function into its unit's file
   brings them to about zero.

## Link experiment

Three C files were compiled with the matching flags (`cl /TC /ML /O2 /Ob1
/DNDEBUG`). `link /INCREMENTAL:NO /MAP` linked them in several orders, from a
library, and with an order file. The build stayed in a scratch directory
inside the worktree and was deleted afterwards.

```c
/* a.c */
int a_common[4];
int mm_common;
static int a_bss[3];
int a_init = 5;
static int a_s(int x) { a_bss[x & 1] += x; return a_bss[0] * 3 + x; }
const char *a1(int x) { a_common[x & 3] = a_s(x); return "shared"; }
float a2(float x) { return x * 1.5f + (float)a_init; }

/* b.c */
int b_common;
int zz_common[100];
int mm_common;
static double b_bss;
const char *b1(int x) { b_common = x; zz_common[x] = 1; b_bss += x; return x ? "shared" : "b only"; }
float b2(float x) { return x * 1.5f; }

/* c.c: c_common[2], aa_common[50]; c1(); main() calls b1, a1, a2, b2, c1 */
```

| Link | Inputs | `.text` order (from the map) | Owner of `"shared"` | Common order |
| --- | --- | --- | --- | --- |
| L1 | `c.obj a.obj b.obj` | c1, main, a1, a_s, a2, b1, b2, then LIBC | `a.obj` | b_common, zz, a_common, c_common, aa |
| L1b | `b.obj a.obj c.obj` | b1, b2, a1, a_s, a2, c1, main, then LIBC | `b.obj` | c_common, aa, a_common, b_common, zz |
| L2 | `c.obj ab.lib` (a, b archived) | c1, main, **b1, b2, a1, a_s, a2** | `ab:b.obj` | c_common, aa, a_common, b_common, zz |
| L3 | `a.obj b.obj c.obj /ORDER:@{c1,b2,a_s,a1}` | **c1, b2, a1**, a_s, a2, b1, main | n/a | n/a |

What the runs show:

- **Explicit objects keep command-line order**, in `.text` and in `.data`.
  Each object's data block (its plain `.data` first, then its literals)
  follows the objects before it. L1 and L1b swap the blocks exactly as they
  swap the objects.
- **Library members are placed in the order they are pulled.** In L2 `c.obj`
  references `b1` before `a1`, so `b.obj` comes first, although `a.obj` comes
  first in the archive. Today's matching build behaves the same way (below).
- **Literal COMDATs go to the first object in link order.** `"shared"` lives
  in `a.obj`'s block in L1 and in `b.obj`'s block in L1b. This is the
  mechanism behind finding 505's rule 2, "a literal lives in the `.data`
  block of the first object in link order that uses it". With the original
  link order and the same literals per object, literal placement follows.
- **`/O2` makes every function a COMDAT.** In `a.obj`, each function is its
  own `.text` section with the `LNK_COMDAT` flag and 16-byte alignment, and
  each literal is a `.data` COMDAT. This matches finding 505's rule 5
  (16-byte-aligned functions with `0x90` fill, inside and between objects).
- **`/ORDER` places the external functions it names first, in the order
  given.** Everything else follows in object order. A `static` function
  cannot be named: `a_s` gave `LNK4037 "_a_s" does not exist; ignored`.
  LINK 5.0 adds the leading underscore itself, so the order file lists
  `c1`, not `_c1`. `/ORDER` has no effect on data.
- **Statics are emitted after their first caller.** `a.c` defines `a_s`
  before `a1`. The object's COMDAT order, and the link, are `a1`, `a_s`,
  `a2`. So "functions in original address order" (ADR 0005 recipe step 4)
  does not always mean "in that order in the source": a static helper that
  sits right after its first caller in the binary may be defined before it.
- **Common symbols** (`int x;` at file scope in C: storage class 2, section
  0, value = size) were allocated after all `.bss`. Within one object they
  kept their declaration order in every run. Across objects, the order was
  neither object order nor its reverse in all three links (L2 differs from
  L1's pattern), and it does not sort by size or name. `mm_common`, declared
  in both `a.c` and `b.c` and never referenced, does not appear in the map.
  The allocation rule is not settled; the plan has a ticket for it.

## Today's matching build

The measurements below use the matching build of #425's branch, which is
source-identical to `main` at `041ab287`. They read its `build-reccmp` and
`build-binary-progress` outputs; nothing was rebuilt.

### Link order is library pull order

The executable target links `main.cpp`, `entry.cpp`, `seh_runtime.cpp`,
`dependency_grid.cpp` and `screen_pick.cpp` directly, then
`game.lib gods98.lib game.lib platform_glue.lib`, then the import libraries
(`winmm.lib d3drm.lib … kernel32.lib user32.lib …`). LIBC.LIB comes in as a
default library.

Sorting the Reccmp report's functions by rebuilt address and grouping them by
the file that holds their marker gives the layout:

- **Every source file is one contiguous run**, as finding 505's rule 1
  predicts. 58 files give 68 runs. The extra runs are LIBC members
  interleaved with `crt.cpp`, whose `LIBRARY` markers name LIBC functions,
  and one `level.cpp` function placed elsewhere.
- **The order is executable objects first, then `game.lib` members, then
  `gods98.lib` members**, each in pull order. The executable objects run
  from `entry.cpp` at `0x401070` to `screen_pick.cpp`. The `game.lib`
  members start with `startup_config.cpp`, `terrain_grid.cpp`,
  `entity.cpp` and `util.cpp`, and end with `priorities.c` at `0x485500`.
  The `gods98.lib` members start with `frameloop.cpp` at `0x486080` and
  `resource.cpp`, and `crt.cpp` and LIBC are interleaved from `0x4972d0`.
- Function order inverts 908 times between neighbours (873,206 reversed
  pairs) in the full scope.

### Incremental and non-incremental links score differently

The gate (`make reccmp-check`) links with `/INCREMENTAL`. `make
binary-progress` builds the same sources with `/INCREMENTAL:NO`. Both
reports were compared, all 2,347 functions:

| | incremental (gate) | `/INCREMENTAL:NO` |
| --- | ---: | ---: |
| Mean score | 53.968% | 54.047% |
| Exact functions | 589 | 600 |
| Functions that differ | | 60: 55 up, 5 down |

The five drops:

| Address | Function | Incremental | `/INCREMENTAL:NO` |
| --- | --- | ---: | ---: |
| `0x404280` | `cmd_entity_command_chain_tick` | 11.99 | 10.71 |
| `0x4068c0` | `path_arc_length_interpolate` | 34.25 | 32.88 |
| `0x43f7d0` | `ledger_stub_0043f7d0_EntityEnergyLowTargetRouteCommandTick` | 27.40 | 26.69 |
| `0x46b8b0` | `rectangular_grid_overlay_geometry_draw` | 11.55 | 7.94 |
| `0x497ec0` | `__fltout` | 88.10 | 85.71 |

The code generation is the same, so these differences come from the link:
calls through incremental-link thunks, and names that Reccmp gives to
addresses and offsets. This is the effect finding 506's step (a) saw, when
listing a unit in the executable's sources dropped three call sites. Without
incremental linking there are no thunks, so explicit objects are safe.

### Reordering files is not enough

This simulation reuses the non-incremental report: it keeps each function's
rebuilt order within its file and changes only the order of objects.

| Layout | Adjacent inversions | Reversed pairs |
| --- | ---: | ---: |
| Today (pull order) | 908 | 873,206 |
| Today's 58 files, sorted by the median original address of their functions | 894 | 650,664 |
| Every function in its unit's object, units in original order | 17 | 25,903 |

In the last row, all 17 remaining adjacent inversions are caused by 23
report entries whose addresses are not function starts in the unit map. The
simulation could not place them, so it put them last. In a real build each
of them sits inside a unit. **The inversion count falls only as functions
move into unit-shaped files.** The link-order mechanism is a prerequisite,
not the gain.

## The source tree today

From `translation_units.py layout` on `0cfa8749`:

- 225 units: 50 game, 29 gods98 (after the G98-021 merge of finding 512),
  138 CRT, 6 import and 2 late. 70 units are spread over more than one
  source file, and 42 of the 61 marked source files mix units.
- 157 original game and gods98 functions have no reconstruction: 112 game
  and 45 gods98.
- Seven units are already in original shape and in their own files: G98-004
  `utils.c`, G98-006 `config.c`, G98-007 `maths.c`, G98-018 `keys.c`,
  G98-021 `soundsys.c`, G98-027 `dxerror.c` and GAME-034 `priorities.c`.
- File-static state in the largest mixed files. A function that moves to its
  unit file takes these with it, or they must become shared:

  | File | Lines | `static` lines | Function markers | Unit count |
  | --- | ---: | ---: | ---: | ---: |
  | `game/entity.cpp` | 15,737 | 309 | 297 | 21 |
  | `game/level.cpp` | 14,247 | 257 | 196 | 20 |
  | `game/frontend.cpp` | 12,848 | 414 | 160 | 20 |
  | `crt/crt.cpp` | 9,940 | 159 | 342 | 159 |
  | `gods98/view.cpp` | 9,483 | 197 | 202 | 28 |
  | `game/cmd.cpp` | 7,389 | 195 | 112 | 10 |
  | `gods98/record.cpp` | 3,582 | 53 | 115 | 31 |

### The unit call graph

Direct calls between game and gods98 units, from
`docs/coverage/call-graph/edges.csv` (79 units, 558 unit-to-unit edges):

- 11 units call no other unseen unit. The first wave has 9: GAME-003,
  G98-004, G98-007, G98-018, G98-019, G98-024, G98-026, G98-027 and G98-029.
  A second wave has 2: GAME-016 and G98-013.
- The other **68 units form one strongly connected component**: all 48
  remaining game units and 20 gods98 units.
- Only **6 unit pairs** are gods98 calling game: G98-001 calls GAME-002 and
  GAME-036; G98-005 calls GAME-021; G98-012 calls GAME-002 and GAME-036;
  G98-016 calls GAME-028. (The issue's figure of 8 predates the G98-021
  merge.) Without them the component splits into:
  - two gods98 cycles, `{G98-014, 016, 030}` (Files, Flic, Wad) and
    `{G98-002, 005, 009, 010, 015, 020, 021, 025}`;
  - six gods98 levels;
  - a **43-unit game cycle**. Five game units, GAME-011, 024, 026, 033 and
    038, sit below it.

This order matters for conversions, which change callee contracts (ADR
0005). It does not matter for moving code verbatim: a move changes no
contract, so it can go in any order.

### Layout devices already in `src/`

These were written to make Reccmp name addresses. Each one shifts data, so
each must go before data can land at original addresses:

- **Zero-filled padding in named `.data` sections**, which put tables at
  fixed offsets:
  - `game/frontend.cpp`: `s_menu_action_table_gap[0x3aad0]` and
    `s_reference_menu_data_pad0[0x79144]` in `.data$zzzzzz`;
  - `gods98/audio.cpp`: the 0x29c21-byte `MenuCompatData` in
    `.data$yyyyyy`;
  - `game/cmd.cpp`: the command pool in `.data$cmdpool`.

  These blocks are most of why the rebuilt `.data` is 988,160 bytes against
  the original's 73,216.
- **115 `#pragma comment(linker, "/include:…")` directives** in
  `crt/crt.cpp`. They force LIBC members into the link and fix part of their
  pull order.
- **Storage changed only for scoring.** `gods98/keys.c` declares
  `key_names[KEY_NAMES_COUNT + 1]` (257 slots against the original's 256), so
  that Reccmp names `Key_Find`'s past-the-end operand (finding 513). The
  table is a common symbol, so the spare slot moves every common allocated
  after it.

## Reproducing

The experiment needs only the matching toolchain:

    T=$REPROBIT_MSVC_5_0_RTM_ROOT/wine/x86
    for f in a b c; do $T/cl /nologo /TC /ML /O2 /Ob1 /DNDEBUG /c $f.c; done
    $T/link /nologo /subsystem:console /INCREMENTAL:NO /MAP:l1.map /out:l1.exe c.obj a.obj b.obj
    $T/lib /nologo /out:ab.lib a.obj b.obj
    $T/link /nologo /subsystem:console /INCREMENTAL:NO /MAP:l2.map /out:l2.exe c.obj ab.lib
    printf 'c1\nb2\na_s\na1\n' > order.txt
    $T/link /nologo /subsystem:console /INCREMENTAL:NO /MAP:l3.map /ORDER:@order.txt /out:l3.exe a.obj b.obj c.obj

The build measurements need a `make reccmp-report` and a `make
binary-progress` of the same tree. The file runs come from joining each
report entry's `recomp` address with the file that holds its marker. The
inversions come from `binary_progress.inversions`.
