# 540. The CRT from LIBC.LIB: the matching build already links it; the misses were names

Issue #497, an experiment-first step of #447 (stage S5 of the
[#388 plan](../plans/388-original-link-order.md),
[ADR 0006](../adr/0006-original-link-order.md) decision 4). Measured on
`main` at `a0096524` (after #493 and R5, #494). This changes no game or gods98 function's
source.

## Summary

1. **The premise was mostly already true.** Of the 272 functions in CRT units
   (`translation-units.csv`, `unit_kind` `crt`), the matching build already
   linked **214 from LIBC.LIB** (99 exact, mean 96.06%). Earlier work had put
   most port reimplementations in `src/crt/` under `#if !defined(_MSC_VER)`
   and annotated their addresses with `LIBRARY ... SYMBOL` markers. Only 14
   mapped CRT functions were still port definitions, and 44 were unmapped.
2. **The near-misses were annotation names, not code.** 107 of the 115
   non-exact LIBC functions differed from the original only in operand names.
   The original's CRT data addresses carried `GLOBAL` markers on the port's
   shadow variables (`s_process_heap` at `0x0076dec4`), while LIBC's code
   references its own symbols (`__crtheap`). Other LIBC data and helpers had no
   annotation at all, so Reccmp printed the original's raw address against a
   named rebuilt symbol.
3. **What lands:**
   - `src/crt/libc_symbols.csv`, a Reccmp data source
     (`reccmp-project.yml`): 97 original addresses of LIBC public symbols
     (71 data, 7 functions, 19 string literals), matched by decorated symbol.
   - 69 port `GLOBAL` markers at LIBC data addresses are retired: 35 name a
     LIBC public that the CSV now names, and 34 name LIBC statics that have no
     public symbol. Each marker becomes a comment on the same line, so no line
     number moves.
   - Two `LIBRARY` markers had misspelled symbols: `_sopen` and `_setmbcp`
     (the publics are `__sopen` and `__setmbcp`).
   - Eight port reimplementations leave the matching build, and their addresses
     are annotated with the LIBC members' symbols. Four are in `crt/crt.cpp`:
     `__CIfmod`, `__ms_p5_test_fdiv`, `__fptrap` and
     `__seh_longjmp_unwind@4`. The six in `game/seh_runtime.cpp` are
     `trnsctrl.obj`'s functions (CRT-029); four have public symbols. The native
     builds keep all of them.
4. **Effect:** **110 functions improve, none drop; 87 become exact**, and 10
   LIBC functions are newly mapped (7 exact), which gives 94 new exact
   functions. Exact-function code bytes go from 44,636 (6.95%) to 64,777
   (10.08%). Full mean score goes from 55.18% to 55.85%. Every changed score
   is a CRT function. No link failures; the set of linked LIBC members (171)
   does not change, and neither does the import table.

## Inventory

**Method.**
- The members and public symbols of the toolchain's `LIBC.LIB` were read with
  `tools/msvc_coff.py`'s archive reader.
- For each CRT-unit function, the rebuilt address that Reccmp matched was
  looked up in `build-reccmp/LegoRR.map` to find the contributing object.
- To find each remaining port function's LIBC counterpart, it was placed at the
  same offset from a function of the same unit that LIBC already provides.
- Data operands were paired by disassembling each LIBC function at both
  addresses with capstone and pairing the absolute operands of aligned
  instructions.

**The table.**
- **Before** is `main`'s matching build at `970a1822`. The CRT region's
  scores there equal `main`'s at `a0096524`, by the committed baseline.
  **After** is this change.
- **LIBC member** is the object that supplies the unit's mapped functions
  after the change. `port:` marks a port object, and `(not linked)` a LIBC
  member that the link does not pull.
- **Port source** is `translation-units.csv`'s `current_source` before the
  change; `—` means the source had no definition.
- The LATE rows are the units after the import thunks. LATE-001 and LATE-002
  are game code (the WAD decompressor and the display-mode dialog) and are out
  of scope.

| Unit | Source file | LIBC member | Port source (before) | Before | After |
| --- | --- | --- | --- | --- | --- |
| CRT-001 | fpinit.c | fpinit.obj | crt/crt.cpp | 2/2 mapped, 0 exact, mean 74.8 | 2/2 mapped, 2 exact, mean 100.0 |
| CRT-002 | atof.c | atof.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 97.8 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-003 | sprintf.c | sprintf.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-004 | strstr.asm | strstr.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 98.5 | 1/1 mapped, 0 exact, mean 98.5 |
| CRT-005 | atox.c | atox.obj | crt/crt.cpp | 2/2 mapped, 2 exact, mean 100.0 | 2/2 mapped, 2 exact, mean 100.0 |
| CRT-006 | free.c | free.obj | crt/crt_heap.cpp | 1/1 mapped, 0 exact, mean 97.4 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-007 | malloc.c | malloc.obj | crt/crt_heap.cpp | 3/3 mapped, 1 exact, mean 95.9 | 3/3 mapped, 3 exact, mean 100.0 |
| CRT-008 | realloc.c | realloc.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 97.7 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-009 | qsort.c | qsort.obj | crt/crt.cpp | 1/3 mapped, 1 exact, mean 100.0 | 1/3 mapped, 1 exact, mean 100.0 |
| CRT-010 | ftol.asm | ftol.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-011 | ? | asin.obj | crt/crt.cpp | 2/2 mapped, 0 exact, mean 74.0 | 2/2 mapped, 0 exact, mean 76.0 |
| CRT-012 | rand.c | rand.obj | game/level.cpp | 2/2 mapped, 0 exact, mean 88.9 | 2/2 mapped, 2 exact, mean 100.0 |
| CRT-013 | ? | 87fmod.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 50.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-014 | vsprintf.c | port: string.cpp.obj | gods98/string.cpp | 1/1 mapped, 0 exact, mean 43.0 | 1/1 mapped, 0 exact, mean 43.0 |
| CRT-015 | chkstk.asm | chkstk.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-016 | floor.c | floor.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-017 | toupper.c | toupper.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 97.7 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-018 | crt0dat.c | crt0dat.obj | crt/crt.cpp | 3/5 mapped, 2 exact, mean 82.1 | 3/5 mapped, 3 exact, mean 100.0 |
| CRT-019 | fabs.c | fabs.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-020 | strncpy.asm | strncpy.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-021 | ieeemisc.c | ieeemisc.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-022 | sscanf.c | sscanf.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-023 | ceil.c | ceil.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-024 | lldiv.asm | lldiv.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-025 | llmul.asm | llmul.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-026 | new.cpp | port: crt_heap.cpp.obj | crt/crt_heap.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-027 | fclose.c | fclose.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-028 | fopen.c | fopen.obj | crt/crt.cpp | 2/2 mapped, 2 exact, mean 100.0 | 2/2 mapped, 2 exact, mean 100.0 |
| CRT-029 | trnsctrl.cpp | trnsctrl.obj | crt/crt.cpp, game/seh_runtime.cpp | 9/9 mapped, 3 exact, mean 46.0 | 7/9 mapped, 4 exact, mean 98.4 |
| CRT-030 | exsup.asm | exsup.obj | crt/crt.cpp | 5/5 mapped, 1 exact, mean 89.4 | 5/5 mapped, 3 exact, mean 97.3 |
| CRT-031 | crt0.c | wincrt0.obj | crt/crt.cpp | 2/2 mapped, 0 exact, mean 93.2 | 2/2 mapped, 1 exact, mean 99.6 |
| CRT-032 | isctype.c | isctype.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-033 | strncmp.asm | strncmp.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-034 | ftell.c | ftell.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 97.8 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-035 | fseek.c | fseek.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 98.6 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-036 | fwrite.c | fwrite.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-037 | remove.c | unlink.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-038 | chmod.c | chmod.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-039 | mkdir.c | mkdir.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-040 | findfile.c | findfile.obj | crt/crt.cpp | 4/4 mapped, 1 exact, mean 96.2 | 4/4 mapped, 4 exact, mean 100.0 |
| CRT-041 | getcwd.c | getcwd.obj | crt/crt.cpp | 3/3 mapped, 2 exact, mean 98.8 | 3/3 mapped, 3 exact, mean 100.0 |
| CRT-042 | fread.c | fread.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-043 | fflush.c | fflush.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-044 | fgetc.c | fgetc.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-045 | fgets.c | fgets.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-046 | vfprintf.c | vfprintf.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-047 | stricmp.c | stricmp.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 98.5 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-048 | fullpath.c | fullpath.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 97.8 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-049 | fscanf.c | fscanf.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-050 | splitpat.c | splitpat.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 99.4 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-051 | fp8.c | fp8.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-052 | testfdiv.c | testfdiv.obj | crt/crt.cpp | 2/2 mapped, 0 exact, mean 67.5 | 2/2 mapped, 2 exact, mean 100.0 |
| CRT-053 | cvt.c | cvt.obj, port: crt.cpp.obj | crt/crt.cpp | 5/7 mapped, 1 exact, mean 95.5 | 5/7 mapped, 3 exact, mean 99.3 |
| CRT-054 | cfin.c | cfin.obj | — | 0/1 mapped, 0 exact | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-055 | _flsbuf.c | _flsbuf.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 96.7 | 1/1 mapped, 0 exact, mean 98.3 |
| CRT-056 | output.c | output.obj | crt/crt.cpp | 1/7 mapped, 0 exact, mean 95.7 | 1/7 mapped, 0 exact, mean 96.6 |
| CRT-057 | strchr.asm | strchr.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-058 | heapinit.c | heapinit.obj | crt/crt_heap.cpp | 1/1 mapped, 0 exact, mean 92.6 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-059 | sbheap.c | sbheap.obj | crt/crt.cpp, crt/crt_heap.cpp | 8/8 mapped, 2 exact, mean 95.5 | 8/8 mapped, 8 exact, mean 100.0 |
| CRT-060 | handler.cpp | handler.obj | crt/crt_heap.cpp | 1/1 mapped, 0 exact, mean 92.3 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-061 | 87disp.asm | 87disp.obj | crt/crt.cpp | 2/2 mapped, 0 exact, mean 97.2 | 2/2 mapped, 0 exact, mean 97.2 |
| CRT-062 | ? | common.obj, genexcep.obj | crt/crt.cpp | 6/6 mapped, 6 exact, mean 100.0 | 6/6 mapped, 6 exact, mean 100.0 |
| CRT-063 | 87cdisp.asm | 87cdisp.obj | crt/crt.cpp | 5/7 mapped, 2 exact, mean 97.6 | 5/7 mapped, 5 exact, mean 100.0 |
| CRT-064 | ? | adj_fdiv.obj | crt/crt.cpp | 2/2 mapped, 1 exact, mean 99.6 | 2/2 mapped, 1 exact, mean 99.6 |
| CRT-065 | frnd.c | frnd.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-066 | fpexcept.c | fpexcept.obj, port: crt.cpp.obj | crt/crt.cpp | 8/8 mapped, 1 exact, mean 95.8 | 8/8 mapped, 5 exact, mean 98.5 |
| CRT-067 | util.c | util.obj | crt/crt.cpp | 3/3 mapped, 3 exact, mean 100.0 | 3/3 mapped, 3 exact, mean 100.0 |
| CRT-068 | fpctrl.c | fpctrl.obj | crt/crt.cpp | 4/4 mapped, 4 exact, mean 100.0 | 4/4 mapped, 4 exact, mean 100.0 |
| CRT-069 | aw_map.c | aw_map.obj | crt/crt.cpp | 1/2 mapped, 0 exact, mean 96.7 | 1/2 mapped, 1 exact, mean 100.0 |
| CRT-070 | input.c | input.obj | crt/crt.cpp | 1/5 mapped, 0 exact, mean 99.6 | 1/5 mapped, 0 exact, mean 99.8 |
| CRT-071 | close.c | close.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 94.4 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-072 | _freebuf.c | _freebuf.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-073 | _open.c | _open.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 98.1 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-074 | stream.c | stream.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 91.9 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-075 | frame.cpp | frame.obj | crt/crt.cpp | 3/10 mapped, 1 exact, mean 97.4 | 3/10 mapped, 1 exact, mean 98.9 |
| CRT-076 | lowhelpr.asm | lowhelpr.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-077 | hooks.cpp | hooks.obj | crt/crt.cpp | 2/4 mapped, 0 exact, mean 80.1 | 2/4 mapped, 0 exact, mean 86.2 |
| CRT-078 | ismbbyte.c | ismbbyte.obj | crt/crt.cpp | 1/2 mapped, 1 exact, mean 100.0 | 1/2 mapped, 1 exact, mean 100.0 |
| CRT-079 | stdenvp.c | stdenvp.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 94.7 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-080 | stdargv.c | stdargv.obj | crt/crt.cpp | 1/2 mapped, 0 exact, mean 88.9 | 1/2 mapped, 0 exact, mean 94.4 |
| CRT-081 | a_env.c | aw_env.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 98.8 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-082 | mbctype.c | mbctype.obj | crt/crt.cpp | 1/5 mapped, 0 exact, mean 88.9 | 2/5 mapped, 1 exact, mean 98.3 |
| CRT-083 | ioinit.c | ioinit.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 93.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-084 | exsup3.asm | exsup3.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-085 | crt0msg.c | crt0msg.obj | crt/crt.cpp | 2/2 mapped, 0 exact, mean 89.1 | 2/2 mapped, 1 exact, mean 98.5 |
| CRT-086 | aw_str.c | aw_str.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 94.6 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-087 | lseek.c | lseek.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 91.4 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-088 | write.c | write.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 95.6 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-089 | dosmap.c | dosmap.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 77.1 | 1/1 mapped, 0 exact, mean 91.4 |
| CRT-090 | dtoxtime.c | dtoxtime.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 95.7 | 1/1 mapped, 0 exact, mean 98.9 |
| CRT-091 | _filbuf.c | _filbuf.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 97.8 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-092 | read.c | read.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 97.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-093 | _sftbuf.c | _sftbuf.obj | crt/crt.cpp | 2/2 mapped, 1 exact, mean 92.7 | 2/2 mapped, 1 exact, mean 97.9 |
| CRT-094 | tolower.c | tolower.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 97.7 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-095 | mbsnbcpy.c | mbsnbcpy.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 97.5 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-096 | ieee87.c | ieee87.obj | crt/crt.cpp | 2/4 mapped, 2 exact, mean 100.0 | 2/4 mapped, 2 exact, mean 100.0 |
| CRT-097 | intrncvt.c | intrncvt.obj | crt/crt.cpp | 12/12 mapped, 10 exact, mean 99.0 | 12/12 mapped, 10 exact, mean 99.0 |
| CRT-098 | _fptostr.c | _fptostr.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-099 | cfout.c | cfout.obj | crt/crt.cpp | 2/2 mapped, 1 exact, mean 92.9 | 2/2 mapped, 1 exact, mean 96.4 |
| CRT-100 | memmove.asm | memmove.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-101 | crt0fp.c | crt0fp.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-102 | strgtold.c | strgtold.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 99.4 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-103 | _getbuf.c | _getbuf.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 93.3 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-104 | isatty.c | isatty.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 90.5 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-105 | wctomb.c | wctomb.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 94.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-106 | ulldiv.asm | ulldiv.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-107 | ullrem.asm | ullrem.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-108 | 87except.c | 87except.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 97.9 | 1/1 mapped, 0 exact, mean 99.0 |
| CRT-109 | mbtowc.c | mbtowc.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 94.8 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-110 | _ctype.c | _ctype.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-111 | llshl.asm | llshl.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-112 | ungetc.c | ungetc.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-113 | osfinfo.c | osfinfo.obj | crt/crt.cpp | 4/4 mapped, 0 exact, mean 90.3 | 4/4 mapped, 4 exact, mean 100.0 |
| CRT-114 | open.c | open.obj | crt/crt.cpp | 0/1 mapped, 0 exact | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-115 | validate.cpp | validate.obj | crt/crt.cpp | 3/3 mapped, 3 exact, mean 100.0 | 3/3 mapped, 3 exact, mean 100.0 |
| CRT-116 | abort.c | abort.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-117 | wcslen.c | wcslen.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-118 | crtmbox.c | crtmbox.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 81.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-119 | calloc.c | calloc.obj | crt/crt_heap.cpp | 1/1 mapped, 0 exact, mean 95.9 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-120 | tzset.c | tzset.obj | crt/crt.cpp | 3/4 mapped, 0 exact, mean 89.5 | 3/4 mapped, 2 exact, mean 99.6 |
| CRT-121 | mantold.c | mantold.obj | crt/crt.cpp | 5/5 mapped, 5 exact, mean 100.0 | 5/5 mapped, 5 exact, mean 100.0 |
| CRT-122 | x10fout.c | x10fout.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 98.7 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-123 | tenpow.c | tenpow.obj | crt/crt.cpp | 2/2 mapped, 1 exact, mean 98.2 | 2/2 mapped, 2 exact, mean 100.0 |
| CRT-124 | chsize.c | chsize.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 94.7 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-125 | winsig.c | winsig.obj | crt/crt.cpp | 1/2 mapped, 0 exact, mean 90.4 | 1/2 mapped, 0 exact, mean 96.6 |
| CRT-126 | wcstombs.c | wcstombs.obj | crt/crt.cpp | 1/2 mapped, 0 exact, mean 96.6 | 1/2 mapped, 1 exact, mean 100.0 |
| CRT-127 | getenv.c | getenv.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 95.8 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-128 | setmode.c | setmode.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 90.0 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-129 | mbsnbicl.c | mbsnbico.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 92.9 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-130 | wtombenv.c | wtombenv.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 98.6 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-131 | aw_cmp.c | aw_cmp.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 96.5 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-132 | setenv.c | setenv.obj | crt/crt.cpp | 1/3 mapped, 0 exact, mean 95.2 | 1/3 mapped, 1 exact, mean 100.0 |
| CRT-133 | mbschr.c | mbschr.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 97.4 | 1/1 mapped, 1 exact, mean 100.0 |
| CRT-134 | strdup.c | strdup.obj | crt/crt.cpp | 1/1 mapped, 1 exact, mean 100.0 | 1/1 mapped, 1 exact, mean 100.0 |
| LATE-001 | ? | port: resource.cpp.obj | gods98/resource.cpp | 14/14 mapped, 1 exact, mean 48.7 | 14/14 mapped, 1 exact, mean 48.7 |
| LATE-002 | ? | port: crt.cpp.obj, port: surface.cpp.obj | crt/crt.cpp, gods98/surface.cpp | 8/8 mapped, 0 exact, mean 28.7 | 8/8 mapped, 0 exact, mean 28.7 |
| LATE-003 | strnicmp.c | strnicmp.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 98.8 | 1/1 mapped, 1 exact, mean 100.0 |
| LATE-004 | xtoa.c | xtoa.obj | crt/crt.cpp | 1/2 mapped, 1 exact, mean 100.0 | 1/2 mapped, 1 exact, mean 100.0 |
| LATE-005 | strlwr.c | strlwr.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 97.8 | 1/1 mapped, 1 exact, mean 100.0 |
| LATE-006 | strupr.c | strupr.obj | crt/crt.cpp | 1/1 mapped, 0 exact, mean 97.8 | 1/1 mapped, 1 exact, mean 100.0 |

Unmapped functions in the units above are LIBC **statics**:
- `_swap`, `_shortsort`, `_initterm`, `_doexit`, `_parse_cmdline`, `_xtoa`;
- `output.c`'s `_write_char` group and `input.c`'s `__inc` group;
- `frame.obj`'s `FindHandler` group, `mbctype.obj`'s helpers, `_siglookup`,
  `_findenv`, `_copy_environ`, `_cvtdate`, `_strncnt` and `_wcsncnt`;
- the `$L` labels.

Their `LIBRARY` markers already name them, but the PDB has no public symbol
for them, so Reccmp cannot map them (see the blockers below).

## What the experiment changed, and why each part

### LIBC data and helpers named by symbol

Reccmp matches a `GLOBAL` marker by the variable name in the declaration that
follows it, so a marker cannot name a LIBC symbol such as `__crtheap` without
an invented declaration (which would also shift MSVC's symbol counter,
finding 538). A CSV data source sets the original entity's `symbol`, and
`match_symbols` pairs it with the rebuilt public of the same decorated name
before any other matching. The CSV:

- does not compile, so it cannot cause entropy drift;
- is read only by Reccmp; `tools/binary_progress_hash.py` now hashes it and
  `reccmp-project.yml`, so `matching-progress-freshness` tracks them.

**Rows that were tried and dropped:**
- `__unwind_handler` (`0x0048ef30`) is a label inside `__global_unwind2`. As
  a function it truncated `__global_unwind2` (96.30% to 75.56%).
- `?FrameUnwindFilter@@...` is a static and never matched.

**Strings.** The 19 string rows raise nine functions. They also turn one raw
`.rdata` operand of `__output` into a placeholder, which renumbers the
placeholders after it. That leaves `__output` at 96.59% instead of 99.88%
without the string rows, still above its baseline (95.74%).

### Port markers at LIBC addresses

A port marker at a LIBC address made Reccmp print the port's name on the
original side. Retiring the marker lets the CSV's symbol name both sides, or,
for a LIBC static, lets both sides print the same placeholder.

Retiring a marker changes no code, and the variables themselves stay:
- About half of them are native-only.
- The rest are still compiled into `crt.cpp`'s and `crt_heap.cpp`'s matching
  objects as dead port state, for example the `tzset.c` caches,
  `s_decimal_separator`, `s_fd_fallback_record`, `s_predefined_codepage_records`
  and `s_heap_alloc_retry_enabled`. Removing them belongs to the drain of
  `crt.cpp` (S5) and shifts that unit's symbol counter, so it is left out.
- One marker was in game code: `s_rand_state` (`game/level.cpp`,
  `0x004afcc8`) is `rand.obj`'s static `holdrand`. It is native-only there;
  the matching build calls LIBC's `_rand`.

### Port reimplementations removed from the matching build

| Address | Port definition (removed under MSVC) | LIBC member, symbol | Before | After |
| --- | --- | --- | ---: | ---: |
| `0x0048e1aa` | `crt_math_intrinsic_dispatch` (naked `mov edx; jmp`) | `87fmod.obj` `__CIfmod` (CRT-013 was unnamed) | 50.00 | 100.00 |
| `0x00490230` | `processor_feature_fallback_false` | `testfdiv.obj` `__ms_p5_test_fdiv` | 50.00 | 100.00 |
| `0x00496895` | `crt_local_unwind_from_frame` (no callers) | `exsup3.obj` `__seh_longjmp_unwind@4` | 100.00 | 100.00 |
| `0x00498340` | `crt_fatal_message_exit` | `crt0fp.obj` `__fptrap` | 100.00 | 100.00 |
| `0x0048ec00` | `seh_frame_unwind` | `trnsctrl.obj` `?_UnwindNestedFrames@@...` | 22.86 | 96.88 |
| `0x0048eca0` | `call_setting_frame_12` | `trnsctrl.obj` `?_CallCatchBlock2@@...` | 11.90 | 97.06 |
| `0x0048ed00` | `crt_exception_frame_filter_invoke` | `trnsctrl.obj` static `CatchGuardHandler` | 27.45 | unmapped |
| `0x0048ed30` | `crt_exception_frame_install_and_dispatch` | `trnsctrl.obj` `?_CallSETranslator@@...` | 12.41 | 94.74 |
| `0x0048ee00` | `crt_exception_scope_filter_and_unwind` | `trnsctrl.obj` static `TranslatorGuardHandler` | 13.11 | unmapped |
| `0x0048ee90` | `seh_scope_table_lookup` | `trnsctrl.obj` `?_GetRangeOfTrysToCheck@@...` | 26.67 | 100.00 |

**How the identifications were checked.**
- The `trnsctrl.obj` identifications follow from function sizes. The original's
  gaps (0x60, 0x40, 0x60, 0x30, 0xd0, 0x90) equal the toolchain member's
  function sizes, once `_CallMemberFunction1`, `_CallMemberFunction2` and
  `___CxxLongjmpUnwind@4` are left out: the original's `trnsctrl.obj` lacks
  them.
- `0x0048e1aa` was identified from its bytes: `ba d0fc4a00 e9 .. d9c9` is
  `87fmod.obj`'s `__CIfmod` entry.

**Callers and drift.**
- Every in-file caller of the four `crt.cpp` definitions is native-only.
- `seh_runtime.cpp`'s body is now `#if !defined(_MSC_VER)`, so its matching
  object is empty. The native program still compiles it.
- Removing the definitions shifts MSVC's symbol counter for the rest of
  `crt.cpp`. It was checked with
  `match_function.py --source src/crt/crt.cpp` against `main`'s file: of the
  327 annotated functions, only LIBC ones changed, all upward. None of
  `crt.cpp`'s game functions moved.

### Lint

`tools/lint_reccmp_annotations.py` placed a definition at the first
non-directive line after the previous `;`, `{` or `}`. A backslash continuation
line of a multi-line `#define` counts as such a line. Removing
`crt_local_unwind_from_frame` left `crt_asin`'s only preceding boundary before
`CRT_DTOLD`'s continuation line, so the lint missed `crt_asin`'s `LIBRARY`
marker.

The lint now skips continuation lines when it looks for the marker. It keeps
the old span for the debt key, so existing keys do not change. A fixture test
covers the case.

The baseline only shrinks, by 8 entries:
- 5 definitions this change removes or annotates;
- 3 false positives of the same continuation bug
  (`entity_stats_float_at20`, `mission_checkpoint_task_progress_refresh_bound`,
  `surface_render_setup`).

No `--allow-growth`.

## Measurements

`make reccmp-check` against `main`'s baseline: **no function falls below its
baseline**, and the map check passes.

| Measure (`binary-progress.json`) | `main` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 44,636 (6.95%) | 64,777 (10.08%) |
| Mean score, full / game-only | 55.18% / 51.02% | 55.85% / 51.09% |
| Rebuilt `.text` | 934,532 B | 933,492 B (−1,040, the removed port definitions) |
| Rebuilt `.rdata` | 458,415 B | 458,423 B |
| `.text` identical at original addresses | 17,920 B | 17,925 B |
| `.data` identical at original addresses | 11,943 B (16.31%) | 11,904 B (16.26%) |
| Unit-relative identity | 36,608 B (6.61%) | 36,955 B (6.67%) |
| Unit contiguity | 180 / 217 (82.95%) | 184 / 219 (84.02%) |
| Pair inversions, full / game-only | 319,385 / 305,459 | 320,302 / 304,840 |
| Unit-order pair inversions | 3,165 | 3,305 |
| Size-exact units | 26 | 24 |

**Effects on the layout measures.**
- **Inversions.** The newly mapped LIBC functions, and the trnsctrl functions
  that move from `seh_runtime.cpp`'s legacy position to LIBC, sit after the
  port tail. So full pair inversions and unit-order inversions rise. The
  game-only inversions fall.
- **Size-exact units.** Two units are no longer size-exact. Reccmp sizes a
  LIBC function up to the next public symbol, so `__CIfmod` (10 bytes in the
  original) measures 54 and `__fptrap` (11) measures 16 with padding. Both
  score 100%.
- **Section bytes.** The small `.data` loss at original addresses is a
  shift. In `.text`, the code after the removed definitions
  moves up by 1,040 bytes. The shrink also crosses a page boundary (`.text`
  ends at `0xe3e74` instead of `0xe4284`), so `.rdata`, `.data` and `.bss` all
  start 4 KiB lower, and so do the absolute pointers stored in `.data`. The
  image size is 4,096 bytes smaller.
- **Imports.** The PE import table is unchanged, at 150 `__imp_` symbols. The
  entry point and image size move with `.text`.

### Changed functions

Every change is in a CRT unit, and the cause of each is the naming or the
library member described above. There are no drops, so no classifier labels
apply.

**Instruction counts.** Instruction-count changes come only from the eight
port-to-LIBC replacements, and are for the owner to review:
- `0x0048e1aa`: 2 → 15;
- `0x0048ec00`: 38 → 32;
- `0x0048eca0`: 50 → 34;
- `0x0048ed30`: 88 → 57;
- `0x0048ee90`: 81 → 54;
- `0x00490230`: 5 → 25;
- `0x00498340`: 4 → 9, including padding.

**No game listing changes.** No function outside the CRT changes its
listing digest.

**Game-only scope.** The 10 removed or retired port functions leave this scope,
because `LIBRARY` entries are not counted in it. `$CRT_C_Initializer(1)`
(`0x004973f0`, 95.31%) enters it: Reccmp's start-up analysis finds it once
`___xc_a`/`___xi_a` are named.

#### Improved (110, full scope)

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `0x0048d920` | `__fpmath` | 93.33% | 100.00% |
| `0x0048d940` | `__cfltcvt_init` | 56.25% | 100.00% |
| `0x0048d980` | `_atof` | 97.83% | 100.00% |
| `0x0048db80` | `_free` | 97.37% | 100.00% |
| `0x0048dbd0` | `_malloc` | 94.74% | 100.00% |
| `0x0048dc40` | `__heap_alloc` | 93.10% | 100.00% |
| `0x0048dc80` | `_realloc` | 97.66% | 100.00% |
| `0x0048e0ad` | `_asin` | 64.71% | 68.63% |
| `0x0048e160` | `_srand` | 88.89% | 100.00% |
| `0x0048e170` | `_rand` | 88.89% | 100.00% |
| `0x0048e1aa` | `__CIfmod` | 50.00% | 100.00% |
| `0x0048e380` | `_toupper` | 97.67% | 100.00% |
| `0x0048e480` | `__cinit` | 46.15% | 100.00% |
| `0x0048ec00` | `?_UnwindNestedFrames@@YGXPAUEHRegistrationNode@@PAUEHExceptionRecord@@@Z` | 22.86% | 96.88% |
| `0x0048eca0` | `?_CallCatchBlock2@@YAPAXPAUEHRegistrationNode@@PBU_s_FuncInfo@@PAXHK@Z` | 11.90% | 97.06% |
| `0x0048ed30` | `?_CallSETranslator@@YAHPAUEHExceptionRecord@@PAUEHRegistrationNode@@PAX2PBU_s_FuncInfo@@H1@Z` | 12.41% | 94.74% |
| `0x0048ee90` | `?_GetRangeOfTrysToCheck@@YAPBU_s_TryBlockMapEntry@@PBU_s_FuncInfo@@HHPAI1@Z` | 26.67% | 100.00% |
| `0x0048efdd` | `__NLG_Notify1` | 75.00% | 100.00% |
| `0x0048efe6` | `__NLG_Notify` | 85.71% | 100.00% |
| `0x0048f000` | `_WinMainCRTStartup` | 92.20% | 99.29% |
| `0x0048f1a0` | `__amsg_exit` | 94.12% | 100.00% |
| `0x0048f2b0` | `_ftell` | 97.75% | 100.00% |
| `0x0048f460` | `_fseek` | 98.57% | 100.00% |
| `0x0048f700` | `__findfirst` | 97.48% | 100.00% |
| `0x0048f830` | `__findnext` | 97.14% | 100.00% |
| `0x0048f950` | `__findclose` | 90.00% | 100.00% |
| `0x0048fa20` | `__getdcwd` | 96.33% | 100.00% |
| `0x0048ff10` | `__strcmpi` | 98.48% | 100.00% |
| `0x0048ffa0` | `__fullpath` | 97.83% | 100.00% |
| `0x00490090` | `__splitpath` | 99.38% | 100.00% |
| `0x00490230` | `__ms_p5_test_fdiv` | 50.00% | 100.00% |
| `0x00490280` | `__ms_p5_mp_test_fdiv` | 85.00% | 100.00% |
| `0x004902b0` | `__forcdecpt` | 97.50% | 100.00% |
| `0x00490400` | `__cftoe` | 95.35% | 99.22% |
| `0x00490540` | `__cftof` | 92.52% | 97.20% |
| `0x00490640` | `__cftog` | 92.21% | 100.00% |
| `0x004908a0` | `__flsbuf` | 96.67% | 98.33% |
| `0x004909d0` | `__output` | 95.74% | 96.59% |
| `0x00491560` | `__heap_init` | 92.59% | 100.00% |
| `0x004915a0` | `___sbh_new_region` | 92.74% | 100.00% |
| `0x00491710` | `___sbh_release_region` | 88.24% | 100.00% |
| `0x00491770` | `___sbh_decommit_pages` | 97.18% | 100.00% |
| `0x00491840` | `___sbh_find_block` | 94.44% | 100.00% |
| `0x004918a0` | `___sbh_free_block` | 95.00% | 100.00% |
| `0x00491900` | `___sbh_alloc_block` | 96.41% | 100.00% |
| `0x00491d90` | `__callnewh` | 92.31% | 100.00% |
| `0x00492170` | `__cintrindisp2` | 95.00% | 100.00% |
| `0x004921ae` | `__cintrindisp1` | 93.75% | 100.00% |
| `0x004921e4` | `__ctrandisp2` | 99.12% | 100.00% |
| `0x00493170` | `__handle_qnan1` | 92.86% | 100.00% |
| `0x004931c0` | `__except1` | 98.65% | 100.00% |
| `0x004935c0` | `__handle_exc` | 96.83% | 100.00% |
| `0x004939a0` | `__set_errno` | 90.00% | 100.00% |
| `0x00493ca0` | `___crtLCMapStringA` | 96.74% | 100.00% |
| `0x00493ef0` | `__input` | 99.57% | 99.83% |
| `0x00494d10` | `__close` | 94.44% | 100.00% |
| `0x00494e30` | `__openfile` | 98.13% | 100.00% |
| `0x00495000` | `__getstream` | 91.94% | 100.00% |
| `0x004954e0` | `___FrameUnwindToState` | 95.83% | 98.61% |
| `0x004959e0` | `?_DestructExceptionObject@@YAXPAUEHExceptionRecord@@E@Z` | 96.23% | 98.11% |
| `0x00495ae0` | `?terminate@@YAXXZ` | 92.73% | 98.18% |
| `0x00495b70` | `?_inconsistency@@YAXXZ` | 67.42% | 74.16% |
| `0x00495df0` | `__setenvp` | 94.68% | 100.00% |
| `0x00495ee0` | `__setargv` | 88.89% | 94.44% |
| `0x00496190` | `___crtGetEnvironmentStringsA` | 98.77% | 100.00% |
| `0x004965c0` | `___initmbctable` | 88.89% | 100.00% |
| `0x004965d0` | `__ioinit` | 93.02% | 100.00% |
| `0x004968b0` | `__FF_MSGBANNER` | 85.71% | 100.00% |
| `0x004968f0` | `__NMSG_WRITE` | 92.40% | 97.08% |
| `0x00496ad0` | `___crtGetStringTypeA` | 94.57% | 100.00% |
| `0x00496c00` | `__lseek` | 91.43% | 100.00% |
| `0x00496cc0` | `__write` | 95.56% | 100.00% |
| `0x00496ee0` | `__dosmaperr` | 77.14% | 91.43% |
| `0x00496f50` | `___loctotime_t` | 95.65% | 98.91% |
| `0x00497040` | `__filbuf` | 97.85% | 100.00% |
| `0x00497130` | `__read` | 97.02% | 100.00% |
| `0x004974d0` | `__stbuf` | 85.42% | 95.83% |
| `0x004975d0` | `_tolower` | 97.67% | 100.00% |
| `0x004976d0` | `__mbsnbcpy` | 97.53% | 100.00% |
| `0x00497ec0` | `__fltout` | 85.71% | 92.86% |
| `0x00498350` | `___strgtold12` | 99.44% | 100.00% |
| `0x00498ae0` | `__getbuf` | 93.33% | 100.00% |
| `0x00498b40` | `__isatty` | 90.48% | 100.00% |
| `0x00498b70` | `_wctomb` | 94.00% | 100.00% |
| `0x00498ce0` | `__87except` | 97.92% | 98.96% |
| `0x00498e00` | `_mbtowc` | 94.79% | 100.00% |
| `0x00498fe0` | `__alloc_osfhnd` | 91.78% | 100.00% |
| `0x004990a0` | `__set_osfhnd` | 92.11% | 100.00% |
| `0x00499150` | `__free_osfhnd` | 90.62% | 100.00% |
| `0x004991f0` | `__get_osfhandle` | 86.67% | 100.00% |
| `0x00499730` | `___crtMessageBoxA` | 81.03% | 100.00% |
| `0x004997c0` | `_calloc` | 95.95% | 100.00% |
| `0x00499860` | `___tzset` | 88.24% | 100.00% |
| `0x00499880` | `__tzset` | 83.84% | 98.69% |
| `0x00499b30` | `__isindst` | 96.36% | 100.00% |
| `0x0049a1b0` | `_$I10_OUTPUT` | 98.74% | 100.00% |
| `0x0049a800` | `___multtenpow12` | 96.49% | 100.00% |
| `0x0049a890` | `__chsize` | 94.70% | 100.00% |
| `0x0049aa10` | `_raise` | 90.41% | 96.58% |
| `0x0049abe0` | `_wcstombs` | 96.60% | 100.00% |
| `0x0049ae10` | `_getenv` | 95.77% | 100.00% |
| `0x0049aea0` | `__setmode` | 90.00% | 100.00% |
| `0x0049af20` | `__mbsnbicoll` | 92.86% | 100.00% |
| `0x0049af60` | `___wtomb_environ` | 98.57% | 100.00% |
| `0x0049afe0` | `___crtCompareStringA` | 96.46% | 100.00% |
| `0x0049b2b0` | `___crtsetenv` | 95.22% | 100.00% |
| `0x0049b5b0` | `__mbschr` | 97.37% | 100.00% |
| `0x0049dab0` | `__strnicmp` | 98.78% | 100.00% |
| `0x0049dc10` | `__strlwr` | 97.75% | 100.00% |
| `0x0049dcd0` | `__strupr` | 97.75% | 100.00% |

#### Newly mapped (10, full scope)

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `0x00490310` | `__cropzeros` | unmapped | 100.00% |
| `0x00490380` | `__positive` | unmapped | 100.00% |
| `0x004903a0` | `__fassign` | unmapped | 100.00% |
| `0x00490750` | `__cfltcvt` | unmapped | 100.00% |
| `0x004907f0` | `__fltin` | unmapped | 100.00% |
| `0x00495c00` | `__XcptFilter` | unmapped | 99.06% |
| `0x004962f0` | `__setmbcp` | unmapped | 96.63% |
| `0x004967d8` | `__except_handler3` | unmapped | 100.00% |
| `0x004973f0` | `$CRT_C_Initializer(1)` | unmapped | 95.31% |
| `0x00499240` | `__sopen` | unmapped | 100.00% |

#### Newly unmapped (2, full scope)

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `0x0048ed00` | `crt_exception_frame_filter_invoke` | 27.45% | unmapped |
| `0x0048ee00` | `crt_exception_scope_filter_and_unwind` | 13.11% | unmapped |

## Blockers for #447

What still separates the CRT region from exact matches, each with its cause:

1. **LIBC statics have no public symbol.**
   - 43 CRT-unit functions stay unmapped, such as `_swap`, `_initterm`,
     `output.c`'s `_write_*` and `frame.obj`'s `FindHandler`, plus
     `CatchGuardHandler` and `TranslatorGuardHandler`.
   - 34 data addresses have no name: `holdrand`, the `cvt.c` decimal-format
     state, `crtmbox.c`'s cached procedures, the `tzset.c` caches and others.
   - Both sides then print matching placeholders. But Reccmp cannot score the
     functions, and a static's data cannot be checked by name.
   - The map lists these statics in its "Static symbols" section with rebuilt
     addresses. Pairing original and rebuilt addresses needs either Reccmp
     support for matching by rebuilt address, or a generated pairing from the
     map.
2. **Port definitions still in the matching build:**
   - `0x004907c0` `string_prefix_reserve` is `cvt.obj`'s static `_shift`. The
     port copy is 100% and is called only from native code. Removing it would
     unmap an exact function (blocker 1).
   - `0x0048ead0` `engine_calloc` is `new.obj`'s `operator new`
     (`??2@YAPAXI@Z`). Nothing in the rebuild calls `operator new`, so the link
     does not pull `new.obj`. The original's callers have to be converted to
     use `new` first.
   - `0x0048e1e0` `formatted_text_buffer_build` (`gods98/string.cpp`) is
     `_vsprintf`. LIBC's `_vsprintf` is already linked, through the
     `/include` list. The gods98 callers call the port name, so this changes
     with their conversion.
   - `0x004939d0`, the port's MSVC `_matherr`, is in fact `fpexcept.obj`'s
     static `_get_fname`. The original's `__umatherr` and `__87except` call
     `_matherr` at **`0x00456210`**: the game defines its own `_matherr`, which
     returns 0, and the ledger names it `flh_magic_uu_noop` (in `crt.cpp`'s
     game band). The port's `_matherr` should move to that game function. This
     waits for R5 (#494), which moves `crt.cpp`'s game functions.
3. **The toolchain's LIBC.LIB is not the original's build.**
   - The original's `trnsctrl.obj` lacks `_CallMemberFunction1/2` and
     `___CxxLongjmpUnwind@4`.
   - `_asin` differs in code: the original has no `__fload_withFB` call
     (68.63%).
   - `_strstr` and `?_inconsistency` differ only around labels and padding.

   These cannot become exact by naming.
4. **Data layout.**
   - Raw-address operands remain where the rebuilt data lies differently. The
     original's `errtable` ends at `_iob`, and `_iob+0x20`/`+0x40` show as raw
     addresses on both sides (`__flsbuf`, `__stbuf`, `__dosmaperr`).
   - LIBC's `.rdata` tables (`__lookuptable`, `rterrs`) differ the same way.

   These follow the data-layout stage (S6).
5. **Import thunks.** `RtlUnwind` is still a raw thunk address on the original
   side (`__global_unwind2`, `_UnwindNestedFrames`). It waits for the import
   order and naming in S5.
6. **S5 items not touched here:**
   - the `/include` directives in `crt.cpp`;
   - the import-library order;
   - `oldnames.lib`'s late members;
   - the LIBC member-order check against `translation-unit-ranges.csv`.

   `crt.cpp` still links: it holds game functions (R5) and the `/include`
   list. `seh_runtime.cpp` stays a legacy entry in the link-order list, but
   its matching object is now empty.

## Reproducing

    make reccmp-check && make match-progress
    python3 tools/match_function.py --source src/crt/crt.cpp --no-diff   # crt.cpp drift check

The inventory scripts (COFF archive index, operand pairing) are one-off and
not committed. `tools/msvc_coff.py`'s `_archive_members` and `parse_coff`
reproduce the member and symbol lists.
