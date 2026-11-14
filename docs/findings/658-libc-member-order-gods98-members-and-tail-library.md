# 658. LIBC member order: the entry point, gods98 as library members, the port tail as a library

Issue #767, part of #388's second criterion. Follows finding 657, which
attributed the remaining 5,568 function-order pairs to their causes. Measured
with full matching builds under the gate's link mode (`/DEBUG /OPT:REF
/OPT:NOICF`, finding 642) on `main` `a70180c8` and on this change. The
per-step figures in section 2 come from scratch relinks of the gate's objects
on `85050ccd` with section 1's change.

## Summary

1. **Teleporter.c's and Info.c's callbacks were `static`.** Their source was
   already in address order. MSVC 5.0 emits a `static` function after the
   function that first uses it, not where it is defined. The original's
   callbacks sit at their definition point, so they had external linkage.
   The six callbacks are now external, with no score change, and both units
   are contiguous. This removes 11 game pairs.
2. **The original linked gods98 from a library.** LINK 5.0 pulls a LIBC
   member when the first unresolved reference to it comes up. When no
   `/ENTRY` is named, LINK infers the entry point after the explicit objects.
   The original's `wincrt0.obj` follows Movie.cpp's LIBC references and
   precedes Container.c's. So the original's explicit objects were the game
   units and Movie.cpp, and the rest of gods98 came from a library. The
   matching build now links:
   - each gods98 unit after Movie.cpp from its own library, `unit_<stem>.lib`,
     searched in list order before the other libraries;
   - with no `/ENTRY`, which had made `wincrt0.obj` LIBC's first member.
3. **The port tail is linked from a library after LIBC** (`lrr_tail.lib`),
   along with the two CRT-range legacy files. As explicit objects, their LIBC
   references came before LIBC's own references, and that pulled members
   early. Now the members only they need are pulled in LINK's second pass,
   after the import thunks, where the original has nothing of its own.
4. **Function-order inversions fall from 5,568 to 801 pairs, and game-only
   from 17 to 6.** No function scores lower. Two LIBC functions reach 100%:
   `__output` (99.03) and `__NMSG_WRITE` (99.42). Unit contiguity rises from
   216 to 220 of 225, and LATE-002's data offsets now agree.
5. This amends ADR 0006 decisions 1 and 4. The addendum is **proposed**; the
   owner decides whether it stands.

## Before and after

| Measure (`binary-progress.json`) | `main` `a70180c8` | This change |
| --- | ---: | ---: |
| Function-order inversions, full: pairs / adjacent | 5,568 / 39 | **801 / 19** |
| Function-order inversions, game-only: pairs / adjacent | 17 / 10 | **6 / 4** |
| Unit-order inversions (unit medians): pairs / adjacent | 1,000 / 28 | **187 / 13** |
| Unit contiguity | 216 of 225 (96.00%) | **220 of 225 (97.78%)** |
| Data-offset agreement | 19 of 57 units | **20 of 57** (LATE-002) |
| Exact-function code bytes | 332,454 (51.74%) | **335,096 (52.15%)** |
| Accuracy, full / game-only | 97.02% / 96.68% | same |
| Functions below the baseline | | **0** (2 up) |
| Entry point RVA (original `0x8f000`) | `0xbd430` | **`0x8d1e0`** |
| Image end (gate) | `0x7f5000` | same |
| `.data` bytes equal at original addresses (whole-image relink) | 11,272 | 17,933 |
| `.rdata` bytes equal at original addresses | 1,397 | 1,325 |

The `.rdata` figure falls by 72 bytes. It compares bytes at absolute
addresses, which is still at chance level (finding 657), and no function's
score depends on it.

| Inversions by segment | `a70180c8` | This change | Cause |
| --- | ---: | ---: | --- |
| CRT before CRT | 5,528 | **772** | LIBC member pull order |
| Import thunks before LATE | 23 | 23 | RtlUnwind's thunk (finding 657); unchanged |
| Game before game | 16 | **5** | Teleporter.c 8 and Info.c 3 fixed |
| gods98 before gods98 | 1 | 1 | AviSurface's destructor pair (G98-012) |
| **Total** | **5,568** | **801** | |

## 1. Callbacks emitted after their users

The rebuilt map put each callback right after the function that first takes
its address:

| Callback | Defined between | Was emitted after |
| --- | --- | --- |
| `0046a3c0` `teleporter_callback_start` | `0046a390` and `0046a420` | `0046a510` (Teleporter_Start) |
| `0046a490` `teleporter_callback_check_type` | `0046a470` and `0046a4c0` | `0046a4c0` (Teleporter_ServiceAll) |
| `0046a5c0` `teleporter_callback_update` | `0046a510` and `0046a700` | `0046a730` (Teleporter_Update) |
| `00419810`, `004198c0`, `004199a0` (Info.c's find callbacks) | their users' predecessors | their users |

MSVC 5.0 defers a `static` function's code until its first use, and emits it
after the function that uses it. An external function is emitted where it is
defined. The definitions were already in address order, so no forward
declarations were needed. Dropping `static` was enough. No other file defines
these names. The comment above each unit's first callback gives the reason.

Lego.c's two callbacks with the same problem, `00429110` and `004297a0`, are
`static` too. The brief left Lego.c to #759's owner, so they are not changed
here. LegoObject.c's pair is a different cause: `0043a5f0` is defined before
`0043a120` in the source.

## 2. How LINK 5.0 orders LIBC's members

The scratch relinks all reused the gate's objects. Each step was measured on
LIBC's members in the map:
- **member pairs**: pairs of members out of the original's order;
- **weighted pairs**: the same pairs weighted by the functions of each unit.

The weighted count is a proxy for the function pairs, not the same measure.

| Step | Member pairs | Weighted pairs |
| --- | ---: | ---: |
| Before (`/ENTRY`, tail as explicit objects) | 946 | 4,384 |
| No `/ENTRY` | 873 | 3,566 |
| No `/ENTRY`, tail from a library before `kernel32.lib` (only drops unreferenced objects) | 805 | 3,233 |
| No `/ENTRY`, tail from a library after LIBC | 370 | 1,028 |
| The same, with `/ENTRY` | 481 | 1,995 |
| No `/ENTRY`, tail and legacy files from a library after LIBC | 258 | 711 |
| The same, with `/ENTRY` | 444 | 1,907 |
| **No `/ENTRY`, tail and legacy files after LIBC, gods98 after Movie.cpp as members (this change)** | **165** | **480** |

What the relinks show:
- **Pull order.** Members are pulled breadth-first. First come the members the
  explicit objects reference, in the order of their first reference. Then
  come the members those members reference, and so on. A library searched
  after LIBC adds its own references only in LINK's second pass, after the
  import libraries.
- **The entry point.** `/ENTRY:WinMainCRTStartup` puts `wincrt0.obj` first.
  Without it, LINK infers the entry after all explicit objects. Two scratch
  attempts to place the reference by hand changed nothing: a `/include`
  directive in an object after Movie.cpp, and an object whose only function
  calls `WinMainCRTStartup`. We did not find out why. The original's
  `wincrt0.obj` (CRT-031, which the unit map calls `crt0.c`) comes between
  `exsup.obj` (Movie.cpp's last first reference) and `isctype.obj`
  (Container.c's first). That is where the explicit objects end when gods98's
  other units come from libraries. In the final build, `wincrt0.obj` is in
  exactly that place.
- **The CMake comment that `/ENTRY` replaced** feared that LIBC's index would
  resolve shared startup data to the console `crt0.obj`. That no longer
  happens: the entry is `WinMainCRTStartup` in `wincrt0.obj`, and no
  `crt0.obj` is linked.
- **One library per gods98 unit**, searched before `kernel32.lib`, pulls all
  28 units in list order: each is referenced before its library is searched.
  The map check now requires list order for these members too, so a
  conversion that breaks this fails the gate.

### Why the step without gods98 members was not enough

Dropping `/ENTRY` without the member libraries lowered LIBC's `__setmbcp`
(`004962f0`) from 100% to 99.44%. One operand is `__rgcode_page_info`'s end,
which Reccmp names after the next data symbol:
- in the original that is `___badioinfo` (ioinit.obj);
- in that link it was `__XcptActTab` (winxfltr.obj).

Without the member libraries, `wincrt0.obj` came after all the explicit
objects' references. The members that the original pulls through it
(`winxfltr`, `ismbbyte`, `stdenvp`, `stdargv`, `aw_env`, `mbctype`, `ioinit`,
`exsup3`, `crt0msg`) were then pulled after other members' references, and
`ioinit` came before `mbctype`. With the gods98 members, the run from `hooks`
to `lseek` is the original's, member for member, and `__setmbcp` stays at
100%.

### What the libraries leave out

LINK links only the library members that something references. Of the tail
library's 123 objects (121 tail entries and the two legacy files), 91 are
linked. The 32 left out are 31 tail objects and `string.cpp`. In the gate
before this change, the 31 tail objects had no symbol in the map, public or
static, except one `.bss` block: `sfx_port.cpp`'s twelve variables, which
nothing references. The native and
reference builds are unchanged. `CMakeLists.txt` still compiles
`dependency_grid.cpp` and `screen_pick.cpp` into the program there, for their
hook registrations.

`string.cpp`'s `formatted_text_buffer_build` is not linked any more. Its
marker names LIBC's `_vsprintf`, which the original's callers call (finding
642). `level.cpp`'s two annotated functions are compiled out of the matching
build.

## 3. What remains

**CRT before CRT, 772 pairs.** The members furthest out of order, with the
pairs they are part of:

| Member (unit) | Pairs | Cause | Owner |
| --- | ---: | --- | --- |
| `fflush` (CRT-043) | 261 | In the original it comes between `fread` and `fgetc`, among Files.c's references. No unit or Movie.cpp references it now, so it comes in as another member's dependency | Files.c (G98-014) review |
| `tolower` (CRT-094) | 105 | Probably pulled through `stricmp`, which comes early (next row) | Follows `stricmp` |
| `stricmp` (CRT-047) | 76 | Advisor.c's `__strcmpi` is the first reference. In the original it comes among Files.c's references | Advisor.c (GAME-001) review |
| `__allmul` (CRT-025) | 78 | In the original it comes right after `__alldiv`, which Movie.cpp references first. No explicit object references it now | Movie.cpp (G98-001) review |
| `87cdisp`, `fpexcept`, `fpctrl`, `sbheap`, `read`, `cvt` and other dependencies | about 250 | Their referencers come in a different order | Follow the rows above |

The 28 members the original lacks are still pulled (finding 643). Most of
them now come in the second pass, after `strupr`: `snprintf`, `memchr`,
`fprintf`, `strtol`, `strcspn`, `strspn`, `ullshr`, `strtod`, `strrchr`,
`asin`, `pow`, `87tran` and `powhlp`.

**RtlUnwind's thunk, 23 pairs.** Unchanged (finding 657).

**In-unit order, 6 pairs:**
- Lego.c (GAME-021), 3: two `static` callbacks, the cause of section 1;
- LegoObject.c (GAME-023), 2: definition order;
- the AVI player (G98-012), 1: the generated destructor.

**Unit contiguity, 5 units broken:**
- GAME-021, GAME-023 and G98-012, as above;
- CRT-092 and CRT-120: their ranges hold a second LIBC member (finding 657).

CRT-077 (`hooks` and `winxfltr`) and CRT-083 (`ioinit` and `exsup3`) are
contiguous again, because those members are now pulled next to each other,
as in the original.

## The change

- `CMakeLists.txt`:
  - every entry keeps its object target, so it compiles the same wherever it
    is linked from;
  - gods98 units after `LRR_LINK_EXPLICIT_LAST` are archived into
    `unit_<stem>.lib`;
  - every entry from the first tail entry on (except the late units) is
    archived into `LRR_LINK_TAIL_LIBRARY`;
  - `link-members.txt` in the build directory records each archived object's
    library and path;
  - no `/ENTRY`.
- `src/link_order.cmake`: `LRR_LINK_EXPLICIT_LAST` and
  `LRR_LINK_TAIL_LIBRARY`, and a header that describes the five link steps.
- `tools/link_order.py`:
  - `placements()`, `member_library()`, `library_object()` and
    `member_objects()`;
  - `check` validates both settings;
  - `check-map` requires list order across explicit and member objects, and
    rejects an explicit tail object.
- `tools/pe_audit.py` classifies `unit_*:` and `lrr_tail:` objects by their
  list entry, so `winapi_link_audit` keeps its categories.
- `tools/unit_data_offsets.py` measures the member units from
  `link-members.txt`.
- Tests: `test_link_order.py` (placements, the settings, the members file,
  member order in the map) and `test_pe_audit.py` (classification).
- Docs: `docs/building.md`, `GLOSSARY.md` (*port tail*) and the ADR 0006
  addendum of 2026-11-13.

## Checks

- `make reccmp-check`: no function below the baseline; `__output` and
  `__NMSG_WRITE` rise to 100%. Winapi link check and link-order map check OK.
- `make match-progress`: the baseline, listing digests and
  `binary-progress.json` are regenerated.
- `make unit-data-offsets` (stdout only), against finding 657's figures:

  | Status | Finding 657 | This change |
  | --- | ---: | ---: |
  | full | 0 | 0 |
  | gaps-only | 16 | 17 |
  | partial | 15 | 14 |
  | n/a | 196 | 196 |

  The member units are measured from their libraries.
- `make pe-audit`:
  - the thunk order and DLL order are unchanged;
  - the entry point is `0x8d1e0` (original `0x8f000`);
  - the image end is `0x7f5000`.
- `make tools-test`, `make winapi-link-check` and native ctest pass.

## Reproducing

```sh
make reccmp-check && make match-progress && make unit-data-offsets && make pe-audit
python3 -c "import json; d=json.load(open('docs/coverage/binary-progress.json')); \
  print(d['function_order_inversions'], d['layout']['unit_contiguity'])"
```
