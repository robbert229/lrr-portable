# 667. Lego.c's callbacks, and how the original referenced `fflush`, `stricmp` and `__allmul`

Issue #786, part of #388. Follows finding 658, section 3. Measured with full
matching builds under the gate's link mode on `main` `b928af37` and on this
change.

## Summary

1. **Lego.c's two callbacks have external linkage**, as finding 658 did for
   Teleporter.c and Info.c. `00429110` (`lego_block_visible_callback`) and
   `004297a0` (`lego_cd_track_play_next_callback`) are emitted where they are
   defined. This removes Lego.c's 3 pairs. No score changes.
2. **`stricmp`: the original's callers used the old name.** Every game unit,
   Movie.cpp and the gods98 units before Files.c call `stricmp`, which
   OLDNAMES.LIB resolves to `__stricmp` only after LIBC's first pass. Files.c
   alone calls `_strcmpi` directly, in `0047ffc0`. That is what puts LIBC's
   `stricmp.obj` between `vfprintf` and `fullpath`. `strnicmp` (LATE-003)
   already shows the same habit (finding 643).
3. **`fflush` and `__allmul` come from functions `/OPT:REF` removed.** The
   original has no call to `_fflush` or `__allmul` from game or gods98 code,
   and no pointer to either. `_fflush` itself is not in the image. Only
   `_flush`, `_flushall` and `flsall` are there. Yet both members were pulled
   among the first references, not as another member's dependencies. LINK
   resolves symbols before `/OPT:REF` removes unreferenced COMDATs. So the
   references came from functions that nothing calls and the link then
   removed:
   - Files.c's File_Flush, between File_Close and File_Tell;
   - a Movie.cpp method with a 64-bit multiply, after GetDuration.

   Both are now in the source. The matching build removes them the same way.
4. **Function-order inversions fall from 799 to 222 pairs, and game-only from
   4 to 1.** Unit-order inversions fall from 187 to 45, and contiguity rises
   from 221 to 223 of 225 units. No function scores lower. Two LIBC functions
   reach 100%: `__dosmaperr` (97.14) and `$CRT_C_Initializer(1)` (98.44).

## Before and after

| Measure (`binary-progress.json`) | `main` `b928af37` | This change |
| --- | ---: | ---: |
| Function-order inversions, full: pairs / adjacent | 799 / 18 | **222 / 10** |
| Function-order inversions, game-only: pairs / adjacent | 4 / 3 | **1 / 1** |
| Unit-order inversions (unit medians): pairs / adjacent | 187 / 13 | **45 / 9** |
| Unit contiguity | 221 of 225 | **223 of 225 (99.11%)** |
| Data-offset agreement | 20 of 57 units | 20 of 57 |
| Exact-function code bytes | 336,831 (52.42%) | **336,934 (52.44%)** |
| Accuracy, full / game-only | 97.07% / 96.74% | same |
| Functions below the baseline | | **0** (2 up) |
| Image end (gate) | `0x7f5000` | same |

The brief quoted 801 and 6 pairs for `main`. Those are finding 658's figures.
The committed `binary-progress.json` on `b928af37` records 799 and 4, and
this table uses those figures.

| Inversions by segment | `b928af37` | Lego.c only | This change | Cause |
| --- | ---: | ---: | ---: | --- |
| CRT before CRT | 772 | 772 | **198** | LIBC member pull order |
| Import thunks before LATE | 23 | 23 | 23 | RtlUnwind's thunk (finding 657); unchanged |
| Game before game | 3 | **0** | 0 | Lego.c's two callbacks |
| gods98 before gods98 | 1 | 1 | 1 | AviSurface's destructor pair (G98-012) |
| **Total** | **799** | **796** | **222** | |

The "This change" column is counted per segment from the gate's Reccmp
results. The `main` column is the committed totals split the same way:
game-only 4 = Lego.c 3 + G98-012 1, and the thunks' 23 do not change.

## 1. Lego.c's callbacks

`static` was removed from both definitions, and one comment above the first
gives the reason (finding 658, section 1). No other file defines or declares
the names. `make reccmp-check` shows no score change in Lego.c.

## 2. How the original's objects reference LIBC

In the gate's objects, MSVC 5.0 lists external symbols in source order
across functions and in reverse first-use order within a function (Files.c's
first function lists `_remove __chmod __mkdir _fclose _fopen ...
__getcwd`). LINK pulls LIBC members in the order those symbols first become
unresolved (finding 658). Each member out of place below gives the reference
the original must have had.

### `stricmp`

The original's game code calls `0048ff10` from the first function on
(`004013f2` in Advisor.c). If any explicit object referenced `__strcmpi` or
`__stricmp`, `stricmp.obj` would be one of LIBC's first members. Before this
change it was fourth, after Advisor.c's first reference. In the original it
comes after `vfprintf`, which Files.c's File_PrintF references.

The old names `stricmp`/`strcmpi` give the symbols `_stricmp`/`_strcmpi`.
LIBC defines only `__stricmp` and `__strcmpi`, so these references stay
unresolved until OLDNAMES.LIB supplies the aliases. By then the direct
reference has pulled the member. The only direct reference is in Files.c,
after File_PrintF, in `0047ffc0`.

The change:
- the `_strcmpi(` calls in the 20 game units, Container.c, Utils.c and
  Config.c, and in Files.c's first function, now spell `stricmp(`;
- Files.c's `0047ffc0` and `00480570` keep `_strcmpi`;
- the gods98 units after Files.c (Keys.c, Lws.c, SoundSys.c, Mesh.c, Wad.c,
  Lwo.c) are left as they are, because their references come after Files.c's.

The code is unchanged: the call still reaches `__stricmp`/`__strcmpi`
(`0048ff10`). Natively, `vc5names.h` maps `stricmp` onto `strcasecmp_lite`,
as it already did `_strcmpi`.

### `fflush`

In the original, `fflush.obj` comes between `fread` and `fgetc`. The code
there, `0048fd20`, is `_flush`: it takes a stream and calls `_write`. It is
followed by `_flushall` (`0048fd90`) and `flsall` (`0048fda0`). There is no
`_fflush`. Its callers are LIBC members only: `fclose`, `fseek`, `fwrite`,
`fread` and `_file`. Nothing references `_commit`, which `_fflush` calls,
and `commit.obj` is not in the image. As a dependency of `fclose`,
`fflush.obj` would come after `close` and `_freebuf`, among the second-level
members. That is where it was before this change.

So some Files.c function between File_Read and File_GetC referenced
`_fflush`, and `/OPT:REF` then removed that function along with `_fflush` and
`_commit`. Two further pieces of evidence support this:
- **The original's line numbers.** The `__LINE__` arguments of Files.c's
  error calls are 368 (Seek), 395 (Read), 414 (Write), 431 (Close), 469
  (Tell) and 545 (GetC). The 38 lines between Close and Tell are about two
  functions' worth, and only one is in the binary.
- **No mapped gods98 function is unreferenced.** A scan of the original
  found no mapped gods98 function without a call, jump or 32-bit reference.
  The game code's only apparent case, LegoObject.c's `00438390`, is reached
  through `jmp` at `004565c0`. With objects compiled without `/Gy`, as
  finding 563 assumed, unused gods98 API functions would survive. So the
  original's objects were most likely compiled with `/Gy`, as `/O2` gives
  them in the gate (each function is its own COMDAT), and `/OPT:REF` removed
  what nothing called.

It is not dead code inside a live function. MSVC 5.0 drops the extern of a
call that the optimizer removes. It also drops the extern of an unreferenced
`static` function and of an unused `__inline` one. Scratch compiles with the
gate's flags confirmed each case. An unused pointer initialised with `fflush`
keeps the extern, but it also keeps its 4 bytes of data, and the original has
no such pointer.

`file_flush` is added between `file_close` and `file_tell`:
- it calls `fflush` for a standard stream;
- its body is inferred, and only the reference matters;
- no header declares it, so no filler declaration is added (rule 4);
- the gate removes it, as the original's link did.

### `__allmul`

In the original, `llmul.obj` comes right after `lldiv.obj`, whose `__alldiv`
GetDuration references (`00472551`). It also comes before `new`, `fclose` and
`fopen`, which `movie_load` references in reverse order. The only caller of
`__allmul` (`0048ea90`) is LIBC's `004945e2`, so Movie.cpp must have had a
removed function with a 64-bit multiply between GetDuration and `movie_load`.

`G98CMovie::SetPosition(unsigned int ms)` is added after GetDuration. It
seeks the stream to `(LONGLONG)ms * 10000`. A scratch compile confirmed that
this multiply calls `__allmul`. MSVC 5.0 inlines a `(LL)a * b` of two signed
`int`s without the helper. As with `file_flush`, the body is inferred.

Both functions carry `// PORT-ONLY:` with the reason. That is the lint's only
exemption for a definition without an address. They are not port code: they
are the original's functions that have no address. The owner may prefer a
marker of their own.

## 3. What remains

**CRT before CRT, 198 pairs.** The members furthest out of order, with the
pairs they are part of:

| Member (unit) | Pairs | Likely cause |
| --- | ---: | --- |
| `87cdisp` (CRT-063), `CRT-064`, `fpexcept` (066), `fpctrl` (068), `util` (067) | 85, 34, 56, 28, 21 | The FP helpers come in a different order. Their referencers (`87disp`, `genexcep`, `common`, `frnd`) come after `strchr` here, before it in the original |
| `strchr` (CRT-057) | 20 | Referenced earlier in the original, by a game unit or `output` |
| `atox` (005), `strstr` (004), `qsort` (009), `malloc` (007), `realloc` (008) | 16, 10, 12, 9, 6 | The order of the game units' first references. The original has `strstr atox free malloc realloc qsort`, the gate `free malloc qsort atox ftol strstr acos realloc` |
| `fwrite` (CRT-036) | 11 | In the original a unit before Files.c references it, before `remove`. Here Files.c's File_Write is its first reference |

**RtlUnwind's thunk, 23 pairs.** Unchanged (finding 657).

**In-unit order, 1 pair:** the AVI player (G98-012), its generated
destructor.

**Unit contiguity, 2 units broken:** CRT-092 and CRT-120, whose ranges hold
a second LIBC member (finding 657).

## The change

- `src/game/lego_unit.c`: the two callbacks lose `static`.
- `src/gods98/files.c`:
  - `file_flush`, a removed function, after `file_close`;
  - the first function calls `stricmp`.
- `src/gods98/movie_unit.cpp`: `G98CMovie::SetPosition`, a removed method,
  declared in the class and defined after GetDuration.
- `stricmp(` in place of `_strcmpi(` in:
  - `advisor.c`, `aitask.c`, `bubbles.c`, `dependencies.c`, `frontend_unit.c`,
    `info_unit.c`, `interface_unit.c`, `lego_unit.c`, `legoobject_unit.c`,
    `loader.c`, `message_unit.c`, `objective_unit.c`, `panel_unit.c`,
    `pointer.c`, `reward.c`, `rock_fall_styles.c`, `text.c`, `tooltip.c`,
    `tutorial_script.c` and `weapons.c`;
  - `container.c`, `utils.c` and `config.c`.
- `src/crt/vc5names.h`: native `stricmp` maps onto `strcasecmp_lite`.

## Checks

- `make reccmp-check`: no function below the baseline. `__dosmaperr` and
  `$CRT_C_Initializer(1)` rise to 100%. Winapi link check and link-order map
  check OK.
- `make match-progress`: the baseline, listing digests and
  `binary-progress.json` are regenerated.
- `make unit-data-offsets`: full 0, gaps-only 17, partial 13, n/a 197.
- `make pe-audit`:
  - the import DLL order and common name order are unchanged;
  - the entry point is `0x8d0f0` (original `0x8f000`);
  - the image end is `0x7f5000`.
- `make tools-test` (lint 466, unchanged) and `make winapi-link-check` pass.
- Native ctest passes 37 of 37, and the native build starts.

## Reproducing

```sh
make reccmp-check && make match-progress && make unit-data-offsets && make pe-audit
awk '/ 0001:/ && /libc:/{print $NF}' build-reccmp/LegoRR.map | awk '!s[$0]++' | head -50
```
