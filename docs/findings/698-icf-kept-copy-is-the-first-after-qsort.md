# 698. The copy an identical-COMDAT fold keeps is the first after LINK's unstable qsort

Part of the identity work in PR #861. Follows finding 697 §6. Measured by static disassembly of the toolchain's link.exe and msvcrt.dll, winedbg runs of LINK unchanged, and relinks of the identity build's frozen objects at 68fa3c1f. No source, CMake or build input was changed.

## The algorithm (static disassembly of the toolchain's link.exe, image base 0x400000)

- `0x445aa0` resolves each COMDAT as LINK meets its symbol in pass 1 (selection
  types 1–6). For each newly kept COMDAT whose section is `.text` and non-empty
  (filter `0x41e7f0`), `0x446073`–`0x4460f8` appends `{contribution, 0}` to an
  array (`[0x46b248]`, count `[0x46b244]`). A COMDAT that a later "largest"
  copy replaces has its entry zeroed (`0x446045`). Static COMDATs (storage
  class 3, such as aw_map.obj's and aw_cmp.obj's `_strncnt`) are entries of
  their own; only external names deduplicate.
- Pass-1 order is command-line objects in command-line order, then library
  members in the order they are loaded. LINK's Pass2 module list is exactly
  that order reversed.
- While applying relocations (`0x4023e0`), LINK ORs a per-COMDAT signature
  from relocation types, target storage classes and flags into the entry's
  second dword.
- `/OPT:ICF` (`0x41e850`) sorts the array once with MSVCRT's `qsort`
  (import `0x46e3dc`), element size 8, comparator `0x41e990`:
  null entries last, then `SizeOfRawData` ascending (`[c+8]-[c+0x14]`), then
  `NumberOfRelocations` ascending (word `[c+0x24]`, copied from the section
  header at `0x446402`).
- For each run of equal keys, `0x41ee90` takes each unfolded entry *i* in
  sorted order and folds every later unfolded entry *j* with the same
  signature and identical contents (`0x41efb0`, which also compares
  line-number counts) into *i*. It sets flag 0x4000 on *j* and its parent
  (`[j+0x10]`) to *i*. With `/OPT:REF`, both must be referenced (flag 0x2000).
  The loop repeats for the ICF iteration count (`[0x4530a8]`).
- **The copy kept is the group's first entry after the sort.** `0x41f170`
  prints the "Selected symbol" (the root) and "Replaced symbol(s)" (message
  IDs 254/255/256 in link.exe's string table).
- The sort is MSVCRT 5.00's `qsort` (disassembled from the toolchain's
  msvcrt.dll at `0x7802344a`), which both the matching build and ReproBit
  run (`cross/msvc5-native-crt-link.sh`). It is the classic CRT quicksort:
  cutoff 8, selection-style short sort, middle element swapped to `lo` as
  the pivot, `<= 0` / `>= 0` scans, the larger part pushed, unstable.

## Validation

`icfsim.py` rebuilds the array from a relink's objects, libraries and
`/VERBOSE` load order, and reimplements the qsort.
- Under winedbg, with a breakpoint at LINK's qsort call `0x41e88d` (LINK
  itself unchanged), the dumped array (3,009 entries, each contribution's
  size and relocation count) equals the rebuilt one entry for entry.
- It predicts the kept copy of all 87 fold groups in finding 697's 18
  `/VERBOSE` relinks, including the ones whose choice switched (A2fand,
  Bdm100, Bfa, Bnodel).
- On the current identity build it predicts all four groups (identical
  `.text` to ReproBit's `build/LegoRR.exe`). With 5, 30 and 100 extra dummy
  COMDATs on the command line it predicts all 12 groups, including the
  flips LINK actually made.

## Why `__fpclear` keeps fpinit.obj's copy

`__fpclear`, Flic.c's empty function, `__matherr` and NERPs.c's `return 0` are
all 16 bytes with no relocations: one run of 47 entries. The first partition's
pivot is the array's middle element. In today's array (3,009 entries) that is
index 1504, NERPs.c's `nerpfunc_set_r5` (16 bytes, 1 relocation), in the middle
of NERPs.c's run of eleven (16, 1) functions (indices 1499–1509). That
partition moves every smaller entry right of the middle into the left part
from the right end inward. That reverses LIBC's and gods98's small COMDATs,
so fpinit.obj's `__fpclear` (index 2498) lands ahead of Flic.c's copy (2359).
That one step decides the group. The `__matherr` pair is decided by four
later partitions inside the small region.

## What this rules out

1. *A source fact that sets the key.* The key is only (size, relocation
   count), the same for every copy in a group by construction. The order is
   then a function of the whole array's key sequence. No per-object property
   (selection type, symbol names, object order alone) decides it. Single
   changes away from the run rarely change it: 0 of 300 random single
   insertions, deletions or size changes flipped either choice. 30 or more
   change it often. So the result matches the original only when the array
   does: every COMDAT's size and relocation count in pass-1 order, including
   COMDATs `/OPT:REF` later removed.
2. *The weak-alias object.* fpinit.obj defines `__fpclear` as a strong external,
   its own `__FPmtinit`/`__FPmtterm` relocate against it, and LINK loads the
   member for `__fltused`. A weak external cannot override that definition.
3. *Defining the original's missing functions.* The original's Flic.c defined
   Flic_Palette64, Flic_DeltaByte, Flic_Black, FlicBRunDepack* (5 × `return 1`),
   FlicCopy*/FlicDeltaWord* (6 × empty) and Flic_Unpack (`return 0`). Their
   order in Flic.c is unknown, because folded copies leave no address. All
   15,625 placements in three clusters (5, 6, 1), with labels assigned after
   the sort, fail to give every original choice. Most move the middle out of
   NERPs.c's (16, 1) run. Adding FrontEnd.c's four frees too (which moves the
   pivot back into the run) keeps `__matherr` in all 1,500 samples and
   `__fpclear` in 33. The CallMemberFunction fold and `_strncnt` then fail in
   every sample.

## Conclusion

The copy LINK keeps is deterministic and fully modelled, but it is a property
of the whole COMDAT array. It will match the original when the rebuilt
array's sizes and relocation counts match the original's in pass-1 order,
including the functions `/OPT:REF` removed and the twelve Flic.c and four
FrontEnd.c functions that today are weak aliases. Until then no honest
single change pins `__fpclear`. A filler or a placement chosen only to steer
the sort would, and nothing else found does. `icfsim.py` can check any
candidate change offline in milliseconds, before a build.

## Reproduce

The scripts are in the helper's scratchpad (`/tmp/claude-1000/-var-home-johnrowl--t3-worktrees-lrr-portable-t3-10d8fb06-67208fc93a8ded25/8515f0ce-f3a5-4d8f-b890-72ff497d64df/scratchpad/`), run in the `developer` distrobox:
- `icfsim.py` rebuilds the array and sort from a relink's objects, libraries and `/VERBOSE` load order. It checks a candidate change in milliseconds without a build.
- `exp2.py` and `exp3.py` are the Flic.c and FrontEnd.c placement searches, and `trace.py` traces which sort step orders two entries.
- `mklink.py` and `snap/` relink the frozen identity objects, and `dbg/` is the winedbg harness.
