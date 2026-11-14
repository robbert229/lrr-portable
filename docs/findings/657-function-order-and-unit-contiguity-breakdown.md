# 657. Function-order inversions and unit contiguity under the original link mode, by cause

Issue #388, its second criterion: "the function-order inversion count …
drops to near zero; remaining exceptions are listed with causes". Measured
on `main` `c42dc2cf` with full matching builds under the gate's link mode:
`/DEBUG /OPT:REF /OPT:NOICF` (finding 642), with the CRT from LIBC.LIB
(finding 643). The change is link-order and metric work only. No game or
gods98 source changes.

## Summary

1. **Over half the full inversion pairs came from two units in the wrong
   place.** LATE-001 (`gods98/rnc.c`, the WAD decompressor) and LATE-002
   (`gods98/init.c`, the mode-selection dialog) follow the import thunks in
   the original. The matching build linked them as explicit objects, so they
   came before all of LIBC, giving 6,716 pairs. They are now linked from
   libraries after the import libraries, one library per unit
   (`LRR_LINK_LATE_MEMBERS` in `src/link_order.cmake`). That leaves 23 pairs.
2. **The contiguity metric split units at their own unlisted functions.**
   Some annotated functions are not in the function manifest that
   `translation_units.py` builds units from: secondary entry points like
   `__NLG_Notify1`, NERPs' table functions, and `init_dialog_handler`. The
   metric treated them as foreign, and each one broke its own unit's run.
   They now count with the unit whose range holds them (`layout_progress.py`,
   with a test). 25 units are no longer reported broken. Four CRT units are
   newly reported broken, and they are real: each range holds a second LIBC
   member.
3. **No score changed.** The match baseline's scores are identical (only
   its input hash moved), as are the exact-function bytes, mapped counts and
   accuracy.
4. **What remains is LIBC member order (5,528 of 5,568 pairs), plus 17
   pairs of in-unit order and 23 for RtlUnwind's thunk.** None of it can be
   fixed from the link-order list. The tables below give each cause and its
   owner.

## Before and after

| Measure (`binary-progress.json`) | `c42dc2cf` | This change |
| --- | ---: | ---: |
| Function-order inversions, full: pairs / adjacent | 12,261 / 39 | **5,568 / 39** |
| Function-order inversions, game-only: pairs / adjacent | 63 / 11 | **17 / 10** |
| Unit-order inversions (unit medians): pairs / adjacent | 1,274 / 28 | **1,000 / 28** |
| Unit contiguity | 195 of 225 (86.67%) | **216 of 225 (96.00%)** |
| Unit contiguity, old metric, this build | | 195 of 225 |
| Data-offset agreement | 19 of 57 units, 300 unattributed | same |
| Exact-function code bytes | 330,196 | same |
| Accuracy, full / game-only | 97.00% / 96.65% | same |
| Entry point RVA (original `0x8f000`) | `0xbe700` | `0xbd410` (4,848 bytes earlier) |
| `.text` (winapi link audit) | 841,445 bytes | 841,461 (alignment of the late objects) |

The brief quoted 12,327 / 43 pairs and 194 units. Those figures are from
before `c42dc2cf`; the table uses the committed `binary-progress.json` of
`c42dc2cf`, which this worktree's baseline build reproduced exactly.

`binary-progress.json`'s `unattributed_mapped_functions` now counts only
mapped annotations outside every unit's range (2). The new
`unlisted_mapped_functions` counts all mapped annotations that start no
manifest function (281, the old field's value).

## The inversions by segment and cause

Pairs are counted between functions ordered by original address. Each pair
is listed under the segments of its earlier and later original function.

| Segments | `c42dc2cf` | This change | Cause | Fix and owner |
| --- | ---: | ---: | --- | --- |
| CRT before LATE | 6,486 | 0 | LATE-001/002 linked as explicit objects, before LIBC | **Fixed here**: linked as library members after the import libraries |
| Import thunks before LATE | 230 | 23 | The same; the 23 left are RtlUnwind's thunk, which the original has before LATE-001 and this build has after LATE-002 | Open; see [RtlUnwind's thunk](#rtlunwinds-thunk) |
| CRT before CRT | 5,528 | 5,528 | LIBC member pull order | Not link-order work; see [LIBC member order](#libc-member-order) |
| Game before game | 16 | 16 | Function order inside four game units | Their conversions; see [in-unit order](#in-unit-order) |
| gods98 before gods98 | 1 | 1 | AviSurface's destructor pair in G98-012 | Its conversion |
| **Total** | **12,261** | **5,568** | | |

Adjacent inversions stay at 39: CRT 28, game 9, gods98 1, import/late 1.
Before, the late block's one adjacent inversion sat at RtlUnwind's thunk too.

### The late units

Finding 608 guessed that LATE-001 and LATE-002 were library members, pulled
after the import libraries. Linked from a library after `dxguid.lib`, their
code lands after the DirectX thunks. Their data does not reach the
original's place, after LIBC's (`004b2d08` 'RNC', then init.c's strings):
- init.c's strings sit among LIBC's `.data` contributions, at the point
  where LINK pulled its library: object 113 of 202 in the map's `.data`;
- rnc.c's 'RNC' literal is a COMDAT that the port tail's `resource.cpp`
  defines first, so it stays there;
- rnc.c's state is commons.

Data-offset agreement is therefore unchanged.

| Step | Pairs | Order after `_DirectSoundCreate@12` |
| --- | ---: | --- |
| `c42dc2cf` (explicit objects) | 12,261 | LATE units before LIBC |
| One library, `lrr_late.lib` | 5,694 | init.c, rnc.c, RtlUnwind thunk (126 late/late pairs) |
| **One library per unit, in list order** | **5,568** | rnc.c, init.c, RtlUnwind thunk |

In one library, LINK pulled init.c first, even though the library lists
rnc.c first. Its references come first: WinMain, in Main.c, comes before
Wad.c. With one library per member, LINK searches them in command-line
order. The CMake wiring is in `CMakeLists.txt`, with a check that every late
member is in the list. `tools/link_order.py` has `late_members`,
`late_library` and `late_object`, and its check requires each late member to
be a `unit` entry of a `late` unit. Three tools learned that the map names
these objects `late_<stem>:<file>.obj`:
- `unit_data_offsets.py` finds them in their library's object directory.
  Without that it reports "object absent from map". With it, LATE-002's
  `.rdata` is partial: 4 items, first gap `004b2d50`;
- `pe_audit.classify_object` keeps them as `unit` objects;
- `winapi_link_audit`'s categories therefore match `c42dc2cf`'s.

### RtlUnwind's thunk

The original's order is DirectX thunks, RtlUnwind's thunk, rnc.c, init.c,
`strnicmp`. Here RtlUnwind's thunk comes after init.c. LIBC's `exsup.obj`
asks for `RtlUnwind` only after `kernel32.lib` was searched, so LINK pulls
the thunk in its second pass (finding 574). The late libraries are searched
in the first pass. Two tries changed nothing:
- `kernel32.lib` listed a second time, before the late libraries;
- the late libraries named by `/DEFAULTLIB` instead of on the command line.

The original's late units were therefore searched after a pass that resolved
`RtlUnwind`. One reading: their library was searched only in LINK's second
pass, which would also explain `strnicmp`'s pull after them. This build
cannot reproduce that, because WinMain and Wad.c reference both units from
the start. It costs 23 pairs and one adjacent inversion. Owner: open (toolchain behaviour;
the original's link command is unknown).

### LIBC member order

Both images take these members from the same LIBC.LIB, and LINK pulls them
in the order their symbols become unresolved (finding 574). The original's
order is therefore its referencers' order, and the measure is sound: the
inversions are real differences in what references LIBC, and when. The
table attributes each member that is out of the original's order (42 of the
130 members the original and the build share; 88 are in order) to its first
referencer. The referencers come from the objects' COFF symbol tables in link
order, followed through LIBC's own references. That simulation approximates
LINK's order but does not reproduce the map exactly, so treat the split as
indicative.

| First referencer | Members | Pairs | Members (unit) | Owner |
| --- | ---: | ---: | --- | --- |
| Port tail | 12 | 2,085 | `_allmul` (CRT-025), `fflush` (043), `strchr` (057), `tolower` (094), `intrncvt` (097), `memmove` (100), `strgtold` (102), `ulldiv` (106), `_ctype` (110), `calloc` (119), `getenv` (127), `strdup` (134) | Conversions that retire the referencing tail code (e.g. `util.cpp`'s `__ld12tod`/`___strgtold12` path, `actor_scene.cpp`, `frontend_port.cpp`) |
| LIBC member, rooted in a unit | 11 | 1,409 | `heapinit` (058), `87cdisp` (063), `mbctype` (082), `ioinit` (083), `exsup3` (084), `write` (088), `ieee87` (096), `_fptostr` (098), `cfout` (099), `crt0fp` (101), `setmode` (128) | Follow from the rows above and below; no separate fix |
| LIBC member, rooted in the tail | 10 | 790 | `stdenvp` (079), `stdargv` (080), `crt0msg` (085), `wcslen` (117), `crtmbox` (118), `mantold` (121), `tenpow` (123), `wtombenv` (130), `aw_cmp` (131), `setenv` (132) | As the port tail row |
| Second members of multi-member units | | 605 | CRT-062 (`genexcep` + `common`), CRT-077 (`hooks` + `winxfltr`), CRT-092 (`read` + `_file`), CRT-120 (`tzset` + `closeall`) and others | Not a cause of their own; the members follow their own referencers |
| Legacy file `gods98/string.cpp` | 2 | 312 | `_flsbuf` (055), `output` (056) | Its relocation (G98-004 string helpers, plan R5) |
| A unit's own reference | 6 | 125 | `strstr` (004, `building_model.c`), `atox` (005, `animation_object.c`), `realloc` (008, `screen_effect_quads.c`), `rand` (012, `frontend_unit.c`), `fwrite` (036, `files.c`), `stricmp` (047, `advisor.c`) | Each unit's review: a CRT call the original made later, or by another name |
| Not found | 1 | 202 | `ismbbyte` (078) | Unknown |

28 members that the original's CRT lacks are still pulled, by the port tail
and by the units' port-shaped calls (finding 643's list, plus `wincrt0`, which
the unit map names `crt0`). They add no pairs, because no original function
maps to them, but they move every later member.

**A structural option, not taken.** All the tail-rooted rows (2,875 pairs) come
from the port tail being linked as explicit objects before LIBC. Linking the
tail from a library after LIBC would delay its references, but it would:
- move the tail's code and data after LIBC;
- drop tail objects that nothing references (the hook-registration files
  that `CMakeLists.txt` compiles into the program for that reason);
- contradict ADR 0006 decision 4.

That needs an ADR amendment and an owner decision, and it was not measured
here.

### In-unit order

| Unit | Pairs | Functions out of order (original address) | Owner |
| --- | ---: | --- | --- |
| GAME-044 Teleporter.c (`teleporter.c`) | 8 | the callbacks `0046a3c0`, `0046a490`, `0046a5c0` come after their users | Its conversion: definition order |
| GAME-019 Info.c (`info_unit.c`) | 3 | the find callbacks `00419810`, `004198c0`, `004199a0` come after their users | Its conversion: definition order |
| GAME-021 Lego.c (`lego_unit.c`) | 3 | `00429110` (block-visible callback), `004297a0` (CD track callback) | Its conversion |
| GAME-023 LegoObject.c (`legoobject_unit.c`) | 2 | `0043a120` after `0043a5f0` | Its conversion |
| G98-012 the AVI player (`avi_player.cpp`) | 1 | `~AviSurface` before its scalar deleting destructor | Its conversion: the compiler places the generated destructor by first use |

Game-only inversions are these 17 pairs (16 game, 1 gods98), plus nothing
else. With the late units fixed, the game-only report has no other cause.

## Unit contiguity

The metric asks whether each unit's mapped functions form one run in
original order. A mapped annotation at an address that starts no manifest
function used to count as a foreign symbol and break the run around it. The
annotation could be a secondary entry point (`__NLG_Notify1`,
`__startOneArgErrorHandling`, `__fload_withFB`) or a function the manifest
misses: NERPs' 119 in GAME-028, and `init_dialog_handler`, reached only
through its address. Such a break is an artefact: the unit is laid out
correctly. Now an annotation inside a unit's range counts with that unit,
for both the run and the order check, and only an annotation outside every
range is foreign (2). `tools/tests/test_binary_progress.py`'s
`test_unlisted_annotation_counts_with_its_unit` covers three cases: inside
the run, placed after another unit, and reversed.

| | Units |
| --- | ---: |
| Contiguous before (`c42dc2cf`) | 195 |
| Broken only by their own unlisted functions, now contiguous | +25: GAME-002, 005, 052, 009, 013, 016, 017, 020, 024, 028, 036, 041, 042, 049, 050; G98-002, 005, 010, 015, 025; CRT-030, 053, 062, 075; LATE-002 |
| Newly reported broken: a second LIBC member inside the range | −4: CRT-077, CRT-083, CRT-092, CRT-120 |
| **Contiguous after** | **216 of 225** |

Still broken, nine units:
- **In-unit order (5):** GAME-019, GAME-021, GAME-023, GAME-044 and G98-012,
  as in the in-unit table above.
- **The unit map gives two LIBC members one range (4):**

  | Unit | Range holds | Second member | Its first manifest function |
  | --- | --- | --- | --- |
  | CRT-077 `hooks.cpp` | `00495ae0`-`00495d90` | `winxfltr.obj` (`__XcptFilter` `00495c00`, `_xcptlookup` `00495d40`) | none |
  | CRT-083 `ioinit.c` | `004965d0`-`00496895` | `exsup3.obj` from `004967d0` (`__except_handler3` `004967d8`) | `00496895` |
  | CRT-092 `read.c` | `00497130`-`004974d0` | `_file.obj` (`$CRT_C_Initializer` `004973f0`) | none |
  | CRT-120 `tzset.c` | `00499860`-`00499fb0` | `closeall.obj` (`__fcloseall` `00499f40`) | none |

  Splitting them in `translation-units.spec.csv` was tried, and
  `translation_units.py build` refuses it: a unit must start at a manifest
  function, and none of these members has one. The fix is to add their
  functions to the decompile manifest and ledger, then split the four units.
  That is a unit-map change outside this thread.

## Checks

- `make reccmp-check`: no function below the baseline; winapi link check
  and link-order map check OK.
- `make match-progress`: the baseline and listing digests change only in
  their input hash.
- `make unit-data-offsets` (stdout only): full 0, gaps-only 16, partial 15,
  n/a 196. The late objects are measured from their libraries.
- `make pe-audit`: thunks in the original's order and DLL counts
  unchanged; the late objects are attributed to units.
- `make tools-test`, `make winapi-link-check` and native ctest pass.

## Rebased onto `main` `a1bc9039` (#765)

The generated files were taken from `main` and regenerated, not merged. On
`a1bc9039` the inversion and contiguity figures are the same as on
`c42dc2cf`:
- full pairs 12,261 to 5,568 (adjacent 39);
- game-only 63 to 17;
- unit-order 1,274 to 1,000;
- contiguity 195 to 216 of 225.

Exact-function bytes (330,196), accuracy (97.01% / 96.67%) and data-offset
agreement (19 of 57) are unchanged. The entry point RVA moves from `0xbe710`
to `0xbd420`. `make reccmp-check`: no function below the baseline; only the
input hash of the baseline and the listing digests changed.

## Reproducing

```sh
make reccmp-check && make match-progress && make unit-data-offsets && make pe-audit
python3 -c "import json; d=json.load(open('docs/coverage/binary-progress.json')); \
  print(d['function_order_inversions'], d['layout']['unit_contiguity'])"
```
