# Finding 643 — The C runtime from LIBC.LIB, LIBC statics by map, and the late members

Issue #447, stage S5 of the [#388 plan](../plans/388-original-link-order.md).
Follows findings [540](540-crt-from-libc-lib.md),
[574](574-import-directory-and-library-order.md),
[608](608-libc-crt-switch-prototype.md) (the blocked prototype) and
[641](641-main-unit-original-shape-winmain.md) (Main.c converted, which
removed the last game code that waited on the CRT).

Measured with `make reccmp-check`, `make match-progress`, `make pe-audit` and
`make opt-ref-callers OPT_REF_ARGS=--summary` on `main` `c46f8e97` and on
this change.

## Summary

1. **`src/crt/` is no longer linked into the matching build.** The C runtime
   comes from LIBC.LIB alone. `crt.cpp` and `crt_heap.cpp` are tagged
   `portable` in the link-order list: they build natively and in the MinGW
   reference build, not in the MSVC 5.0 matching build.
2. **The 57 non-runtime symbols** the matching link still took from
   `crt.cpp` (port accessors, their state, and 20 game `GLOBAL`s) move
   unchanged to the port-tail file `gods98/port_state.cpp`.
3. **The 112 `/include` directives are retired.** Every member they named is
   now pulled by a real reference; none is still needed.
4. **Finding 608's three losses are resolved**, not accepted:
   - `0x004907c0` is LIBC's `__shift` at 100%;
   - `0x004939d0` is LIBC's `__get_fname` at 100% (the port's stand-in was 92.31%);
   - `0x0049dcd0` is LIBC's `__strupr` at 100%, now in its original place.
5. **New: LIBC's statics are named from the link map** (`tools/reccmp_map_statics.py`).
   Static functions come from the map's "Static symbols"; static data from
   the LIBC member's COFF relocations. This maps 42 more LIBC functions and
   raises 21, with no function below the baseline.
6. **The late members come through `oldnames.lib`.** The callers use the old
   names (`strnicmp`, `itoa`, `strlwr`, `strupr`). So `strnicmp`, `xtoa`,
   `strlwr` and `strupr` are pulled in LINK's second pass, after the
   `RtlUnwind` thunk, in the original's order, all at 100%.
7. **The import library order** (finding 574) already gives the original's
   thunk order d3drm, ddraw, avifil32, dinput, dsound, kernel32
   (`RtlUnwind`); it is unchanged.

| Measure | `main` `c46f8e97` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 303,156 (47.18%) | **311,747 (48.52%)** |
| Accuracy, full / game-only | 96.48% / 96.20% | **96.53%** / 96.20% |
| Mapped / exact, full | 2,766 / 2,259 | **2,808 / 2,307** |
| Mapped / exact, game-only | 2,521 / 2,050 | 2,519 / 2,049 (two stand-ins became LIBC functions) |
| Functions below the baseline | | **0** (21 up, 42 newly mapped) |
| `opt_ref` removed / removed exact | 27 / 23 | 26 / 22 |
| Function-order inversions, full: adjacent / pairs | 63 / 13,511 | **43 / 11,990** |
| LIBC member order vs the CRT units: in-order run / pair inversions | 56 of 130 / 2,110 | **83 of 130 / 881** |
| Image end (gate) | `0x875000` | `0x875000` |
| `.text` / `.rdata` virtual size | 1,359,557 / 22,784 | 1,359,301 / 22,792 |
| `.data` virtual / raw size | 3,271,952 / 106,496 | 3,271,376 / 105,984 |
| `.idata` | 4,682 | 4,682 |
| Entry point (RVA) | `0x13acd0` | `0x13abc0` |
| Imports | 11 DLLs, 167 names, order equal | same |
| Lint ledger | 549 | 465 |

**CRT units** (`translation-units.csv` rows of kind `crt`, 272 functions,
sized to the next ledger function):

| Band | Before: functions / bytes | After: functions / bytes |
| --- | ---: | ---: |
| Exact | 192 / 37,203 | **236 / 46,686** |
| 95–99.99% | 25 / 12,710 | 21 / 8,382 |
| Below 95% | 12 / 1,185 | 8 / 1,448 |
| Unmapped | 43 / 6,459 | **7 / 1,041** |

The "below 95%" band grows in bytes because newly mapped functions enter it
(`CallCatchBlock` 74.11%, `_doexit` 81.97%, `_xcptlookup` 85.29%,
`_cvtdate` 89.05%, `_siglookup` 87.50%); none of them was mapped before.

## 1. What left the matching link

A scratch relink of the gate's objects without `crt.cpp.obj` and
`crt_heap.cpp.obj` reported 57 unresolved externals. None is a LIBC
symbol. They are:
- the port accessors: the `crt_counter_*`/`crt_float_*` peeks, `request_record_*`, `panel_draw_*`,
  `crt_surface_*`, `string_cache_*`, `engine_feature_word_*`, `renderer_*`
  and the `float_to_long` adapters;
- their state;
- the game words with `GLOBAL` markers: `s_global_float_00556f00`, `s_display_w/h`,
  the renderer words `0076ab24`–`0076ab58`, `s_render_feature_flags` and
  `s_engine_feature_word`.

They move as whole bands into `gods98/port_state.cpp`, a tail entry, which
every build compiles. `crt.cpp` keeps the native runtime models and the
LIBC markers. The bands carry the accessors that only native tests call
too, so the declarations in `crt.hpp` keep one definition each.

**What `crt.cpp` still compiled under MSVC, and its fate:**

| Item | Before | After |
| --- | --- | --- |
| 112 `/include` directives | forced members | retired; the scratch relink and the final map show every member pulled by a reference |
| `string_prefix_reserve` `// FUNCTION: 0x004907c0` | port stand-in, 100% | `PORT-ONLY` native model; `libc_symbols.csv` row `__shift` |
| `_matherr` `// FUNCTION: 0x004939d0` | port override of `_matherr`, scored as `_get_fname`, 92.31% | gone; native model `crt_math_exception_handler_get` is `PORT-ONLY`; row `__get_fname` |
| `s_math_handlers` `// GLOBAL: 0x004b2170` | port copy of `_get_fname`'s table | marker retired; row for LIBC's own table (below) |
| 12 `LIBRARY` markers naming statics with commentary on the symbol line (`_swap (private qsort.obj helper…)`) | never matched | bare map names (`_swap`, `__initterm`, `?CatchGuardHandler@@…`), commentary moved above |

The three native-only `GLOBAL`s on LIBC data copies (`0x004b2900`,
`0x004b2940`, `0x004b29a4`) are no longer compiled in the matching build,
so those data markers no longer map. They were the port's copies, not what
LIBC links. Function scores are unaffected.

### The `portable` platform

`lrr_link(... portable)` means every build but the MSVC 5.0 matching build:
- CMake adds the entry unless `LRR_CLASSIC_MSVC5`;
- in `tools/link_order.py`, `applies_to("winapi")` is false for it (the tools'
  `winapi` is the matching build), while `native` and `None` are true;
- `pe_audit.py` and `winapi_link_audit.py` now ask `applies_to` instead of
  testing the field;
- a committed-list test checks that no `src/crt/` source is in the matching
  link.

## 2. LIBC statics by map (`tools/reccmp_map_statics.py`)

**The problem (findings 540 and 608).** LIBC's objects carry no debug
information. The rebuilt PDB has a public for each external LIBC function
but nothing for a static one. A marker or data-source row that names a
static, such as `__shift` or `?FindHandler@@…`, therefore stayed unmapped,
although the function was linked.

**Functions.** LINK 5.0's map lists every static function under "Static
symbols", with its address and object (`libc:cvt.obj`). Before Reccmp
matches symbols, the module adds a rebuilt-side entity for each LIBC static
that an original-side entity asks for by symbol.
- A name that two members define (`_wcsncnt`, `_strncnt`) is skipped.
- The size runs to the next map symbol in the same section.
- A public whose PDB size covers its whole `.asm` contribution is clamped
  to where the static starts. Example: `__global_unwind2` ran over
  `__unwind_handler`.

**Aliases.** A member reached through `oldnames.lib` has two publics at one
address (`_strupr`/`__strupr`), and the PDB keeps only one. The map has
both, so the alias that a marker names is set on that address.

**Data.** Static data is not in the map. For a static data symbol that an
original-side row names, the module finds it in the toolchain's
`lib/libc.lib` member (`RECCMP_LIBC_LIB` overrides the path):
- a function of that member whose map address is known refers to it
  through an `IMAGE_REL_I386_DIR32` relocation;
- the rebuilt operand minus the object's addend gives the address, either of
  the symbol or of its section;
- the size runs to the next symbol in the object's section.

**Not a scoring deviation.** It names what LIBC linked at each address,
from the link's own map and library. `RECCMP_MAP_STATICS=0` turns it off.
`reccmp_full_diff.py` installs it; `match_function.py` does not need it,
because it never scores LIBC.

**New `libc_symbols.csv` rows.** Original addresses come from paired raw
operands in the diffs, or from a mapped neighbour in the same LIBC object,
checked byte for byte:
- functions:
  - `__shift` `004907c0` and `__get_fname` `004939d0`;
  - `__unwind_handler` `0048ef30` (34/34 bytes);
  - `_flsall` `0048fda0` (119/128, relocations);
  - `FrameUnwindFilter` `004955a0` (32/32);
  - `ExFilterRethrow` `004957a0` (48/48);
  - `_xcptlookup` `00495d40` (65/80);
- the `RtlUnwind` thunk `0049c7b4` (`_RtlUnwind@16`);
- 18 static data objects (function-local statics carry the toolchain
  LIBC's `$S` counter):
  - `_get_fname`'s `_names` table `004b2170`;
  - `_setargv`'s `_pgmname`;
  - `_DoubleFormat` and `_FloatFormat`;
  - `__rgctypeflag` and `__rgcode_page_info`;
  - `_raise`'s four action words;
  - `_cbrackset` and `_sbrackset`;
  - `_errtable`, `_fos`, `_ret`, `_rterrs`, `_tzinfo` and `fprem_risc_table`.

**Rejected candidates.** The neighbour method gives no original address
for some statics, so they get no row:
- `__abstract_sw`, 3–11 of 64 bytes equal;
- `_ctrlevent_capture@4`, 12/80;
- `_x64toa@20`, which the original's `xtoa.obj` does not place there;
- `_strtoxl`, whose member is not in the original's CRT.

**Score changes (all up):**

| Address | Function | Before | After | Why |
| --- | --- | ---: | ---: | --- |
| `0048ec00` | `_UnwindNestedFrames` | 96.88 | 100 | `RtlUnwind` thunk named |
| `0048eca0` | `_CallCatchBlock2` | 97.06 | 100 | `CatchGuardHandler` named |
| `0048ed30` | `_CallSETranslator` | 94.74 | 100 | `TranslatorGuardHandler` named |
| `0048ef10` | `__global_unwind2` | 96.30 | 100 | `RtlUnwind` named, extent clamped |
| `0048efba` | `__abnormal_termination` | 90.00 | 100 | `__unwind_handler` named |
| `00490400` | `__cftoe` | 99.22 | 100 | `__cftoe_g`/`__shift` |
| `00490540` | `__cftof` | 97.20 | 100 | same |
| `004909d0` | `__output` | 96.59 | 99.03 | `_write_*`, `_get_*_arg` |
| `00492bc6` | `__fprem_common` | 99.30 | 100 | `fprem_risc_table` |
| `004938f0` | `__umatherr` | 96.43 | 98.21 | `__get_fname` named; the `_matherr` call target differs (below) |
| `004939d0` | `__get_fname` | 92.31 | 100 | the real LIBC function and its table |
| `00493ef0` | `__input` | 99.83 | 100 | `_cbrackset`/`_sbrackset` |
| `00495ee0` | `__setargv` | 94.44 | 100 | `_pgmname`, `_parse_cmdline` |
| `004962f0` | `__setmbcp` | 96.63 | 100 | `__rgctypeflag`, `__rgcode_page_info`, mbctype helpers |
| `004968f0` | `__NMSG_WRITE` | 97.08 | 99.42 | `_rterrs` |
| `00496ee0` | `__dosmaperr` | 91.43 | 97.14 | `_errtable` |
| `00497d60` | `__ld12tod` | 93.75 | 100 | `_DoubleFormat` |
| `00497d80` | `__ld12tof` | 93.75 | 100 | `_FloatFormat` |
| `00497ec0` | `__fltout` | 92.86 | 100 | `_fos`, `_ret` |
| `00499880` | `__tzset` | 98.69 | 100 | `_tzinfo` |
| `0049aa10` | `_raise` | 96.58 | 99.32 | action words, `_siglookup` |

**42 newly mapped.**
- 32 at 100%. Among them:
  - `CatchGuardHandler` and `TranslatorGuardHandler` (finding 540's blocker 1);
  - `FindHandler`'s neighbours `CatchIt` and `AdjustPointer`;
  - the `output.obj` and `input.obj` helpers;
  - `_swap`, `_shortsort`, `__initterm`, `_xtoa` and the `RtlUnwind` thunk.
- The rest below 100%, most of them LIBC-version differences:
  - `CallCatchBlock` 74.11, `_doexit` 81.97, `_siglookup` 87.50,
    `_cvtdate` 89.05, `_xcptlookup` 85.29;
  - `_x_ismbbtype` 96.00, `_flsall` 96.88, `BuildCatchObject` 98.43,
    `FindHandlerForForeignException` 98.85, `FindHandler` 99.16.

**Still unmapped CRT functions (7, 1,041 bytes):**
- the 87cdisp `.asm` entry points `0049221e`/`00492225`;
- `frame.cpp`'s `$L12725` (`0049573a`) and `hooks.cpp`'s `00495b4e` and
  `00495bde`, not resolved here;
- `_strncnt` (`00493ec0`) and `_wcsncnt` (`0049add0`), static in several
  members.

## 3. The late members through `oldnames.lib`

**The original.** LATE-003 to LATE-006 (`strnicmp`, `xtoa`, `strlwr`,
`strupr`) sit after the import thunks. That happens only if every reference
used the old name:
- `strnicmp` resolves to oldnames' alias member for `_strnicmp`;
- that member makes `__strnicmp` unresolved after LIBC's first search;
- LINK pulls it in its second pass, after `kernel32.lib` has given the
  `RtlUnwind` thunk.

Finding 608 guessed that LATE-001/002 were library members. They are not
needed for this. The callers' spelling is enough.

**The change.** Every matching-build caller spells the old name:
- the 40 `_strnicmp`/`_itoa`/`_strlwr`/`_strupr` calls in 11 files:
  - `frontend_unit.c`, `interface_unit.c`, `lego_unit.c` (one token),
    `objective_unit.c` and `priorities.c`;
  - `container.c`, `utils.c`, `main.c`, `config.c`, `lws.c` and `lwo.c`;
- `frontend_port.cpp`'s inline `call strnicmp`;
- the MSVC macros in `gods98/string.hpp` (`strnicmp_lite`,
  `string_folded_compare`) and `crt/crt.hpp` (`crt_signed_to_string`,
  `crt_string_to_upper`, `crt_string_to_lower`).

Natively, `crt/vc5names.h` maps `strnicmp`, `itoa` and `strlwr` onto the
port's models, as it did the underscore names. The bytes do not change.
The first try, with the units only, left `strnicmp.obj` early:
`string.hpp`'s macro still named `_strnicmp`.

| | Before | After |
| --- | --- | --- |
| Map order | `strnicmp`, `xtoa`, `strlwr`, `strupr` at member positions 15, 18, 36, 39 of 158, before the thunks | positions 155–158 of 159, after `_RtlUnwind@16` (`0054c7fa`), in the original's order |
| Scores | 100 (4 functions) | 100 (4) |

**Callers.** In the first run, 15 callers fell 0.2–2.3 points because the
alias kept the PDB name (`_strnicmp`), not the marker's `__strnicmp`. The
alias handling above restores them. No caller's score changed in the final
run.

## 4. LIBC member order

The map's first-appearance order of `libc:`/`oldnames:` members is
compared with `translation-unit-ranges.csv`'s `crt` and `late` units, by
stem, using finding 608's method.

| | Before | After |
| --- | --- | --- |
| Members | 158 | 159 (`matherr`, below) |
| Shared with the original's units | 130 | 130 |
| Longest in-order run | 56 | **83** |
| Pair inversions | 2,110 | **881** |

Retiring the `/include` list lets the original references decide pull
order, and the late members leave the middle of LIBC.

**29 members the original's CRT lacks.** The port tail still pulls them:
`acos`, `asin`, `pow`, `strtod`, `strtol`, `snprintf`, `fprintf`,
`strcspn`, `strspn`, `memchr`, `strrchr`, `ldexp` and their helpers. They
leave as the port tail shrinks.

**`matherr.obj`.** It is new: LIBC's no-op `_matherr`, pulled because the
port's `_matherr` override is gone. The original's `__umatherr` calls the
game's `_matherr`, which shares `00456210` with NERPs' zero-returning
functions (`flh_magic_uu_noop`, `tutorial_script.c`). Giving that
definition the `_matherr` name in the matching build would drop the member
and raise `__umatherr` to 100. That is a follow-up in a game unit, not
runtime work.

## 5. Other files

- **The README entry**, below.
- **`translation-units.csv`:** `004907c0` and `004939d0` lose
  `current_source`, because no source file annotates them now.
- **`src/link_order.cmake`:** `crt.cpp`'s median key `00493ef0` becomes
  `00494c30` (its markers without the two stand-ins); `port_state.cpp` is a
  new tail entry.
- **Lint ledger:** the 84 accessor definitions that moved to the tail
  leave it.
- **`binary_progress_hash.py`:** hashes the new module.
- **`make tools-test`:** runs `test_reccmp_map_statics.py`.
- **`docs/repository-layout.md`:** says what `crt/` is now.

## Reproducing

```sh
make reccmp-check && make match-progress && make pe-audit
RECCMP_MAP_STATICS=0 make reccmp-report   # the statics off, for comparison
```
