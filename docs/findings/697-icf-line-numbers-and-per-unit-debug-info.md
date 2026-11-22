# 697. Identical-COMDAT folding compares line-number counts: the original's per-unit debug information and /Gy

Part of the identity work in PR #848. Follows findings 692, 695 and 696.
Measured with scratch compiles and relinks of a frozen copy of the matching
build at `f1d9f865`, and with Reccmp runs on those relinks. Nothing under
`build-reccmp/`, the sources or CMake was changed.

## Question

The original folds four LIBC copies that the rebuild keeps apart, 112 bytes
of `.text`:
- `_CallMemberFunction1`/`2` into `0` (32);
- aw_cmp.obj's static `_strncnt` into aw_map.obj's (48);
- `__matherr` (16);
- `__fpclear` (16).

Its `__umatherr` and `__87except` call NERPs.c's `00456210` where they would
call `__matherr`. `__87except` reads `__matherr_flag`, so `matherr.obj` was
linked and its `_matherr` was folded. A game definition of `_matherr` would
have clashed with it. Its `_FPmtinit` and `_FPmtterm` (`004afcac`,
`004afcb0`) hold `00484c90`, Flic.c's empty function, where they would hold
`__fpclear`. Its operator delete is RadarMap.c's `0045daf0` (finding 696). So
the original's link folded identical COMDATs (`/OPT:ICF`), LIBC into game
code included. Yet it kept five groups of byte-identical game functions
apart (finding 695):

| Group | Original addresses |
| --- | --- |
| allocate | AITask.c `004021e0`, Files.c `0047ff30` |
| free | AITask.c `00402210`, Files.c `0047ff40`, RadarMap.c `0045daf0` |
| return TRUE | Lego.c `00432550`, Flic.c `00484b90` |
| float setter | PTL.c `0045d8a0`, Viewports.c `00477240` |
| `return *p` | Container.c `00473680`, Files.c `0047fe20`, Mesh.c `00482210` |

Which compile options per unit reproduce both?

## Results

### 1. LINK 5.0 folds two COMDATs only if their COFF line-number counts agree

`/OPT:ICF` with `/VERBOSE` prints every fold: "Selected symbol" (the copy
kept) and "Replaced symbol(s)". A relink with every unit compiled with `/Zi`
folds all five game groups and the LIBC pairs, but no LIBC function into a
game function. A relink with NERPs.c (`tutorial_script.c`) compiled without
debug information folds `__matherr` into `flh_magic_uu_noop`.

`/Z7` and `/Zd` block that fold too, and the three debug kinds fold with
each other (a `/Zi` AITask.c with a `/Z7` or `/Zd` Files.c). What they share
is COFF line numbers in the `.text` COMDAT. `/Zd` writes no `.debug$S`, and
LIBC's members have no line numbers. The count is what LINK compares, as
these scratch compiles of PTL.c's `radarmap_set_zoom` show. Each variant was
relinked against Viewports.c's identical `/Zi` copy (two line entries):

| PTL.c's source form | Code | Line entries (line deltas) | Folds |
| --- | --- | --- | --- |
| as written (three lines) | same | 2 (`+2`) | yes |
| three blank lines added | same | 2 (`+5`) | yes |
| a local and a split statement | same | 2 (`+6`) | yes |
| `{ ... }` on the declaration's line | same | 1 | **no** |
| `{` on the declaration's line, `}` after the statement | same | 1 | **no** |

A `/Zi` object with its line-number tables zeroed (pointers and counts, the
`.debug$S` kept) folds into LIBC like an object without debug information.
NERPs.c's `00456210` absorbs `__matherr`, Flic.c's no-op absorbs
`__fpclear`, and RadarMap.c's free wrapper leaves AITask.c's group.

So two copies stay apart if either lacks `/Gy` (no COMDAT), or if one has
line numbers and the other has none, or if both have line numbers but a
different number of entries.

### 2. `/Zi` also changes the code of seven functions

Every unit was compiled with and without `/Zi`. Relocations and section
contents were compared. `/Z7` gives `/Zi`'s code, and `/Zd` gives the
no-debug code. Only seven C functions differ (`match_function.py`,
`/Zd` appended for the no-debug code):

| Function | Unit | `/Zi` | no debug information |
| --- | --- | ---: | ---: |
| `water_update_flow_blocks` `0046e610` | Water.c | 96.69 | **100.00** (byte-identical) |
| `weapons_ray_block_sweep` `00471630` | GAME-050 | **100.00** (byte-identical) | 98.99 |
| `surface_triangle_area_calculate` `00479630` | Maths.c | 63.47 | **75.99** |
| `kind_two_steering_torque_apply` `0040f920` | GAME-016 | **81.48** | 77.40 |
| `legoobject_movement_accumulator_apply` `00442b10` | LegoObject.c | **90.53** | 84.67 |
| `legoobject_heading_step_move` `0044c1b0` | LegoObject.c | **97.75** | 94.51 |
| `legoobject_turn_to_vector_activity_issue` `00444fb0` | LegoObject.c | 71.23, 0x230 slot | 74.05, 0x220 |

The other units' code does not depend on `/Zi`.

### 3. What `/Gy` leaves visible

Without `/Gy`, `/OPT:REF` cannot remove a unit's unreferenced functions.
Files.c's `file_flush` (the original's File_Flush, which `/OPT:REF` removed,
finding 667) then stays, 48 bytes. Mesh.c without `/Gy` changes
`mesh_parse_lwo` (80.90% to 78.09%, and its padded size). AITask.c,
Container.c, Viewports.c and PTL.c without `/Gy` change no score and no
padded size. Lego.c without it packs five functions (finding 695 and the
coordinator's relinks).

### 4. The assignment

| Unit | Debug information | `/Gy` | Evidence | Pinned |
| --- | --- | --- | --- | --- |
| GAME-028 NERPs.c | none | yes | `__matherr` and Flic_Unpack fold into `00456210` | strong |
| GAME-036 RadarMap.c | none | yes | LIBC's operator delete folds into `0045daf0` (operator new is LIBC's too, finding 540) | strong |
| G98-016 Flic.c | none | yes | `__fpclear` folds into `00484c90`; its own no-op groups fold | strong |
| G98-025 AnimClone.c | none | yes | Flic_GetWidth folds into `004897d0` (finding 632) | strong, given Flic.c |
| GAME-049 Water.c | none | | `water_update_flow_blocks` is exact only without it; its empty callee folds into `00484c90` | strong |
| G98-007 Maths.c | none | | `surface_triangle_area_calculate` 63.47 → 75.99 | moderate |
| G98-015 Mesh.c | none | yes | `mesh_parse_lwo` needs `/Gy`; must differ from Files.c | moderate |
| GAME-021 Lego.c | `/Zi` | yes | its return-TRUE stays apart from Flic.c's; no `/Gy` packs five functions | strong |
| G98-014 Files.c | `/Zi` | yes | `/OPT:REF` removed File_Flush; without debug information its free wrapper folds with RadarMap.c's | strong |
| GAME-050 | `/Zi` | | `weapons_ray_block_sweep` is exact only with it | strong |
| GAME-023 LegoObject.c | `/Zi` | yes | two of three functions closer, and the 0x230 slot | moderate |
| GAME-016 steering.c, GAME-017 FrontEnd.c | `/Zi` | yes | `kind_two_steering_torque_apply`; FrontEnd.c's frees fold into `0040f7b0` | weak |
| GAME-002 AITask.c | `/Zi` | **no** | apart from Files.c (`/Zi`, `/Gy`) and RadarMap.c (none, `/Gy`) | inferred |
| G98-002 Container.c | none | **no** | apart from Files.c (`/Zi`) and Mesh.c (none) | inferred |
| G98-003 Viewports.c / GAME-035 PTL.c | none / `/Zi` | yes | apart | follows the split |
| the other gods98 units, rnc.c, init.c | none | yes | no fold or code evidence | unconstrained |
| the other game units | `/Zi` | yes | no fold or code evidence | unconstrained |
| GAME-022 camera_control.c | `/Zi` | | already `/Od` | unchanged |

In short: the gods98 library without debug information, except Files.c, and
the game with `/Zi`, except NERPs.c, RadarMap.c and Water.c. Every unit has
`/Gy` except AITask.c and Container.c. These two are the weakest part of the
story. The same separation follows if their identical functions had a
different number of line entries from Files.c's, for example a function body
written on one line. No measurement here tells the two apart, because
neither removing `/Gy` from them nor reformatting changes any byte.

The original has exactly one `ret`-only function (`00484c90`), one
`return 0` (`00456210`) and one `return p->field_0c` (`004897d0`). So
every unit that defined its own empty function, or one of the other two,
had no debug information. Lego.c, LegoObject.c and Files.c call
`00484c90`, so the empty functions they call were defined in other units.

### 5. Relinks (`/OPT:ICF`)

`.text` is against the gate's relink (`/OPT:NOICF`). "Gaps" are the
functions whose padded size differs from the original (`sizegaps.py`):
31 functions, net −80 bytes, in every relink below, the same as the gate.

| Relink | Game groups | LIBC folds | `.text` | Copy kept, where the original keeps another |
| --- | --- | --- | ---: | --- |
| all `/Zi` | all five folded | CallMemberFunction, `_strncnt` | −192 | |
| all without debug information | all five folded (operator delete with AITask.c's) | all four | −240 | `__fpclear` kept in fpinit.obj |
| **the assignment (B)** | **all apart** | **all four**, operator delete by the weak alias | **−112** | `__fpclear` kept in fpinit.obj (Flic.c 16 short, fpinit.obj 16 long) |
| B, Flic.c defining its eleven no-ops beside their twins | all apart | all four | −112 | `__matherr` kept in matherr.obj (NERPs.c 16 short) |
| B, but Files.c without debug information and `/Gy`, AITask.c with `/Gy`, Flic.c's no-ops defined (A2fa) | all apart | all four | −64 | none, but Files.c keeps `file_flush` (+48) |
| B without the operator delete alias | all apart | all four, operator delete folded | −112 | operator delete kept in delete.obj (RadarMap.c 16 short), `__fpclear` in fpinit.obj, `_strncnt` in aw_cmp.obj |

B and its Flic.c variant linked without `/DEBUG` give the same folds and copies.

### 6. Which copy LINK keeps is not decided by the units

The fold groups follow the per-unit options. The copy LINK keeps does not
follow them, nor module order or symbol names:
- Moving Flic.c's object to the front of the command line keeps Flic.c's
  no-op, but then `__matherr`'s.
- Moving TextWindow.c's object to the command line switches the
  `_strncnt` copy.
- 1 to 200 extra weak externals (symbols only) change nothing.
- 1, 5, 30 or 100 extra, distinct, unreferenced-but-included COMDAT
  functions switch the choices apparently at random. With 100, both
  `__matherr` and `__fpclear` keep the original's copy, and `_strncnt`
  keeps the other.

The choice is a function of the whole COMDAT list, as an unstable sort over
it would be (compare finding 694's `qsort`). It will match the original
only when the linked COMDATs do. Until then, a weak alias is the only
deterministic way to put a fold on the original's copy.

### 7. Reccmp

Reccmp maps each annotation to the rebuilt function through the PDB's line
numbers. A unit without line numbers leaves the report:

| Relink | Implemented | Fell below the baseline | Newly unmapped |
| --- | ---: | ---: | ---: |
| the gate (all `/Zi`, `/OPT:NOICF`, weak object) | 3,020 / 3,023 | 0 | 0 |
| the assignment, no debug information | 2,081 / 3,023 | 545 | 939 |
| gods98 (Files.c included), NERPs.c, RadarMap.c, Water.c with `/Zd`, gate link | 2,997 / 3,002 | 184 | 23 |
| four units `/Zi` with line numbers zeroed | 2,632 / 3,023 | 79 | 388 |

`/Zd` gives the no-debug code and keeps line numbers, but it writes no
symbol records. Static data and static callbacks lose their names, and
`water_update_flow_blocks` reaches only 97.41%. Zeroing the line numbers
keeps the symbols but loses the mapping. No single compile option both lets
LIBC fold into a unit and keeps that unit in Reccmp. A `/Zd` gate for Water.c
and Maths.c alone also drops 15 Water.c functions.

## Conclusion

The original compiled gods98 without debug information except Files.c, and
the game with `/Zi` except NERPs.c, RadarMap.c and Water.c. All units had
`/Gy`, except that AITask.c and Container.c either lacked it or had line
counts different from Files.c's. LINK's folds then follow from one rule:
identical bytes, relocations and line-entry counts.

The split that works is gate-only. The gate (`LRR_RECCMP`) keeps `/Zi` on
every unit and `/OPT:NOICF`, so its scores stand. The identity build that
ReproBit imports takes the per-unit options and `/OPT:ICF`. Their code is
the same except for `water_update_flow_blocks` and
`surface_triangle_area_calculate`, which are closer to the original in the
identity build than the gate shows. The proposed CMake change adds an
option, `LRR_ORIGINAL_UNIT_FLAGS`, off by default and refused with
`LRR_RECCMP`. It moves `/Zi` and `/O2` from the configuration's flags into
each target's options and replaces them per unit. Its compile commands give
the same objects as the relinks above, and with the option off nothing
changes.

`weak_operator_delete.obj` cannot go. Without its operator delete alias,
LINK folds LIBC's operator delete with `ui_render_record_free` but keeps
LIBC's copy. Its Flic.c, Flic_Unpack and FrontEnd.c aliases name functions
the sources do not define. Defining Flic.c's no-ops makes LINK keep a Flic.c
copy for `__fpclear` but loses `__matherr` (§5). The 112 bytes fold under the
assignment, but `__fpclear`'s copy stays in fpinit.obj until the COMDAT list
matches the original's.

## Reproduce

Scripts are in this helper's scratchpad
(`/tmp/claude-1000/-var-home-johnrowl--t3-worktrees-lrr-portable-t3-0ce1dcca-8299a1f58562ac3f/af78ec90-7604-4171-87b9-16bbb0832844/scratchpad/w/`),
run in the `developer` distrobox from a checkout of `f1d9f865`:
1. `recompile2.py TAG FLAGS SRC...|ALL` compiles units with `match_function`'s
   configured command. `NODBG` drops `/Zi`, `NOGY` spells out `/O2` without
   `/Gy`, and `SRC=PATH` compiles a scratch copy.
2. `relink.py NAME -s sub-TAG.json ... '/OPT:NOICF=>/OPT:ICF /VERBOSE'`
   relinks the frozen build's objects with the substitutions. `-m`/`-M` move
   a library member to the end or front of the command line.
3. `check.py NAME` lists the fold groups against the original, LINK's
   selections from `link.out`, `.text` and the padded-size gaps.
4. `runreccmp.sh DIR` runs Reccmp and `match_baseline.py check` on a relink.
5. `zidiff.py` compares every unit's `/Zi` and no-debug objects with the
   original; `striplines.py` zeroes an object's line numbers.
6. `original-unit-flags.patch` is the proposed CMake change.
