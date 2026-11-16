# 693. LIBC member order: references from unused header inline copies

Part of the identity work in PR #861. Follows findings 658, 667 and 692.
Measured with full matching builds under the gate's link mode.

## Summary

1. **15 of the 282 mapped LIBC functions were out of the original's order**
   before this change: the members atox, strstr, realloc, 87fmod, fwrite,
   part of sbheap, 87cdisp, adj_fdiv and write. LINK 5.0 pulls a member at
   the first unresolved reference to it (finding 658), and that model
   reproduced the rebuild's own order exactly. So the original had
   references the rebuild lacked.
2. **The original's code does not explain them.** Its member order starts
   atof, sprintf, strstr, atox, free, malloc, realloc, qsort, ftol, acos,
   rand, 87fmod, vsprintf. Yet Advisor.c (GAME-001) calls only atof and
   sprintf from LIBC. AITask.c (GAME-002) calls free, malloc and qsort and
   never strstr, atoi or realloc. The first callers of strstr and atoi are
   GAME-006 and GAME-004, rand's is FrontEnd.c and fmod's is LegoObject.c.
   The image's only fwrite call is Files.c's File_Write, yet fwrite.obj sits
   between Sound3D.c's fseek.obj and Files.c's remove (unlink.obj).
3. **MSVC 5.0 keeps the references of unused `extern __inline` functions.**
   A scratch compile (`/O2 /Ob1 /Zi`, the matching build's flags) shows:
   - an unused `static __inline` function leaves no trace;
   - a call in dead code (`if (0)`, `while (0)`) leaves no trace;
   - an unused `extern __inline` function is emitted as a pick-any COMDAT
     after the object's own functions, and its calls become undefined
     externals in definition order.

   `/OPT:REF` removes the copy's code, but LINK has resolved its references
   by then. The unit ranges already record header inlines closing Advisor.c
   and AITask.c (`translation-unit-ranges.csv`), as Main.h's and Font.h's
   accessors that the image keeps.
4. **Three small headers now give the matching build those references**,
   each included after its unit's last function:
   - `src/game/advisor_tail.h`: strstr, atoi, free, malloc, realloc;
   - `src/game/collision_tail.h`: rand, fmod;
   - `src/gods98/sound3d_tail.h`: fwrite.

   The original names of these helpers are not known. Only the order of
   their references matters, and nothing calls them.
5. **All 141 LIBC members with a known original address are now in the
   original's order**, and LIBC's `.data` lines up with the original's
   through constpow.obj's `__pow10neg`.
6. **rnc.c's unreferenced table was 4 bytes too long.** With LIBC's data in
   place, its last item fills 0x004b2cd4. The table that follows is 48
   bytes at 0x004b2cd8, and the RNC literal lands on the original's
   0x004b2d08. The earlier 52-byte table started with 4 zero bytes that
   stood in for the missing LIBC item.
7. **No function scores lower**, and `_cvtdate` (`0x00499da0`) rises from
   98.54% to 100%.

## A string literal placed by the same mechanism

String literals are pick-any COMDATs, and LINK keeps the first object's
copy. The original's `"LWO"` (`004a1d5c`) sits after Building.c's strings
and before the screen-effect unit's, but its first live reader is
RockFallStyles.c (`0040c953`), so the matching build placed it among
RockFallStyles.c's strings, and the 0x84 bytes between were 4 bytes early.
`src/game/building_model_tail.h` gives Building.c an unused `extern __inline`
copy that uses the literal; /OPT:REF removes its code, and the literal lands
at `004a1d5c`. After it, every `.data` word that differs from the original
is a pointer into `.text`.

## Method

- `coffrefs.py`-style scan: each object's undefined externals, in symbol
  table order, in link order. It gives each LIBC member's first reference.
- Original call sites: every `call rel32` in the original's `.text` that
  targets a LIBC function.
- Order check: the rebuild's LIBC members by their first function address,
  against the original's, with a longest-increasing-subsequence count of
  misplaced members.
