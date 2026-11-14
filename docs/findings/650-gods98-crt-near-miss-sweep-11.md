# Finding 650 — Near-miss sweep 11: gods98 (Container, lwo, Lws, Sound3D, Mesh) and the CRT's LIBC members

Issue #751, part of #388 and ADR 0005. It follows the earlier sweeps
(findings 626, 629, 630, 634, 635, 636, 644 and 645) and the link-mode and
CRT switches (findings 642 and 643). The targets are the near misses of
Container.c (G98-002, `src/gods98/container.c`), lwo (G98-031, `lwo.c`),
Lws.c (G98-020, `lws.c`), Sound3D (G98-009, `sound3d.c`) and Mesh.c
(G98-015, `mesh_unit.c`), and the CRT's LIBC members that were not exact or
not mapped.

Measured on `main` `5f251885`, with one full `make reccmp-check` before the
change and one after.

| Measure | `main` `5f251885` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 319,635 (49.74%) | **325,409 (50.64%)** |
| Accuracy, full / game-only | 96.72% / 96.42% | **96.81% / 96.44%** |
| Mapped / exact, full | 2,827 / 2,348 | **2,832 / 2,382** |
| Mapped / exact, game-only | 2,536 / 2,088 | 2,537 / 2,097 |
| Functions that rose / newly mapped / fell, full | | 33 / 6 / **0** (one re-addressed, below) |
| `opt_ref` annotated removed | 0 | 0 |
| Image end (gate, `/DEBUG /OPT:REF /OPT:NOICF`) | `0x7f5000` | `0x7f5000` |
| Function-order inversions, full / game-only pairs | 12,029 / 44 | 12,329 / 67 (newly mapped LIBC functions and the `DirectSoundCreate` thunk sort after the port tail's mapped functions) |
| Lint ledger | 465 | 466 (`lws_maths_interpolate`, an inline helper, below) |

**One flagged change of address, not a drop.** `make reccmp-check` reports
`0x0048e0ad _acos` as newly unmapped. The marker was on the wrong
address: `0x0048e0ad` is an internal label of LIBC's acos.obj (where
`__CIacos` calls into `_acos` past its `__fload_withFB` prologue), and
`_acos` starts at `0x0048e0a4`, as `crt.cpp`'s comment already said. The
marker now names `0x0048e0a4`, which maps at 96.23% (`0x0048e0ad` was
88.46%), and `__CIacos` rises from 83.33% to 100%. The baseline is
refreshed with `make match-progress`'s steps.

## What closed the gaps

### 1. gods98: four unannotated pooled strings (`container.c`)

The original's Container.c strings `"ActFrame"` (`0x004aab84`), `"%s_%s"`
(`0x004aab90`) and `"DDiScene"` (`0x004aab98`) had no `// STRING` marker,
so Reccmp left them as raw addresses on the original side. With the
markers (`"ActFrame"`'s above `CONTAINER_ACTIVITYFRAMEPREFIX`):

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `00475a70` | Container_GetActivities | 99.30 | **100** |
| `00475d10` | Container_AddActivity2 | 97.56 | **100** |
| `00475e40` | Container_Frame_Find | 98.18 | **100** |
| `00476770` | Container_GetAnimFileFrameCount | 97.67 | **100** |

### 2. gods98: three source shapes

- **`00475830` Container_SearchOwner** (85.71 to **100**): the loop is
  `while (owner == NULL && frame != NULL) { ... }`. The port's `do { if
  (frame == NULL) break; ... } while (owner == NULL);` was rotated by MSVC
  into a bottom test of both conditions; the original tests `frame` once at
  the top and `owner` at the bottom. `while (frame) { ...; if (owner)
  break; }`, `for (;;)` and `if (frame) do ... while` score 75–85%.
- **`00482e70` mesh_load_texture** (91.56 to **100**): one exit. The
  failure path is `free(texture); texture = NULL;` and both paths end in
  `return texture;`. The original keeps `texture` in its stack slot and
  stores `NULL` there before the shared epilogue. A `return NULL` after
  setting `texture = NULL` scores the same as before.
- **`00488070` lws_dissolve_interpolate** (75 to **100**): the
  interpolation goes through an inline helper, Gods98's
  `Maths_Interpolate(a, b, t)` (`a + ((b - a) * t)`), here the file-local
  `static __inline float lws_maths_interpolate`. The original loads the
  first key once and reuses it from the x87 stack (`fsub st(1)`,
  `fadd st(1)`). The same expression written out, with or without a local
  for the first key, scores 26–39%. The helper adds a few symbols to the
  counter before the rest of Lws.c's functions (rule B); `make
  match-function --source src/gods98/lws.c` and the full run show no other
  function moving except `004882e0` (23.88 to 26.92). Like `maths.c`'s inline vector helpers it
  has no address, so it is a lint-ledger entry (`definition
  lws_maths_interpolate`).

### 3. The CRT: LIBC data and thunks named in `libc_symbols.csv`

LIBC's objects carry no debug information, so an operand that names LIBC
data, a LIBC static or an import thunk stays `<OFFSET>` or raw on the
original side unless a data-source row names it. The new rows were found by
pairing raw operands in each diff (`capstone` on both images) and, for
statics, by the LIBC member's relocations (finding 643's method). Each
original address is the rebuilt symbol's place in the same operand:

- public data: `___xp_a`/`___xp_z`/`___xt_a`/`___xt_z` (`004a1018`–`004a102c`),
  `__ctype` (`004afd70`), `__indefinite` (`004b1fe0`), `__XcptActTab`
  (`004b22a8`) and `__XcptActTabCount` (`004b2328`), `__lpdays`
  (`004b29a8`), `__days` (`004b29e0`), `__exitflag`,
  `__C_Termination_Done`, `__C_Exit_Done` (`00549110`–`00549118`),
  `?_pCurrentException@@…`, `?_pCurrentExContext@@…`,
  `?__pSETranslator@@…` (`00549194`–`0054919c`), `___onexitend`/`___onexitbegin`
  (`0076decc`/`0076ded0`);
- statics, placed by `tools/reccmp_map_statics.py`: the seven SEH scope
  tables `$T7914` (wincrt0.obj, `0049eb20`), `$T12708`, `$T12726`,
  `$T12738`, `$T12748` (frame.obj, `0049ed50`–`0049ed88`), `$T12234`,
  `$T12251` (hooks.obj, `0049ed98`/`0049edb0`), 87disp.obj's
  `XAMtoTagTab` (`004b1ffd`) and tzset.obj's `_dststart$S7800`/`_dstend$S7801`
  (`004b2988`/`004b2998`);
- functions: `__flushall` (`0048fd90`) and `__fcloseall` (`00499f40`),
  neither in the ledger, and strchr.obj's `___from_strstr_to_strchr`
  label (`004914a6`), where `_strstr` jumps;
- the `DirectSoundCreate` import thunk (`0049c7ae`,
  `_DirectSoundCreate@12`), which Sound3D's `audio_system_init` calls
  (dsound imports by ordinal, so nothing named it).

The csv gains a `size` column, filled only for `__iob` (`0x280`, twenty
`FILE`s). `__flsbuf` and `__stbuf` compare against `&_iob[1]` and
`&_iob[2]`; with no size Reccmp printed them as raw addresses.

Four port-model `GLOBAL` markers in `crt.cpp` (native-only code, not in the
matching link since finding 643) named LIBC data at the wrong place or
under a port name: `0x004afd72` is `__ctype+2`, `0x004b22a8` and
`0x004b2328` are `__XcptActTab` and its count, and `0x004b29a4` is
`__lpdays-4`. They become comments pointing at the csv rows, as finding 643
did for three others.

### 4. The CRT: two funclet markers cut functions short

`LIBRARY` markers on `$L12725` (`0049573a`) and `$L12243` (`00495bde`)
named code labels, the `__finally` funclets inside `CallCatchBlock` and
`_inconsistency`. They never mapped (no map symbol), but the original-side
entity ended each function at the label. Without them, `CallCatchBlock`
(74.11) and `_inconsistency` (74.16) are compared whole and score **100**.
`translation-units.csv` loses their `current_source` (regenerated with
`translation_units.py build`), and `crt.cpp`'s link-order median moves
back to `00493ef0`.

### CRT results

| Address | Function | Before | After | How |
| --- | --- | ---: | ---: | --- |
| `0047a640` | audio_system_init (Sound3D) | 99.25 | **100** | thunk row |
| `0048da50` | `_strstr` | 98.48 | **100** | `___from_strstr_to_strchr` |
| `0048e090` | `__CIacos` | 83.33 | **100** | `_acos` at its real address |
| `0048e0a4` | `_acos` | (`0048e0ad` 88.46) | 96.23 | re-addressed, `__indefinite` |
| `0048e4f0` | `_doexit` | 81.97 | **100** | exit flags, onexit table, `__xp`/`__xt` bounds |
| `0048f000` | `_WinMainCRTStartup` | 99.29 | **100** | `$T7914` |
| `0048fd90` | `__flushall` | unmapped | **100** | row |
| `004908a0` | `__flsbuf` | 98.33 | **100** | `__iob` size |
| `004914a6` | `___from_strstr_to_strchr` | unmapped | **100** | row |
| `00491db0` | `__trandisp1` | 96.67 | **100** | `XAMtoTagTab` |
| `00491e17` | `__trandisp2` | 97.78 | **100** | `XAMtoTagTab` |
| `00495160` | `FindHandler` | 99.16 | **100** | `_pCurrentException`/`_pCurrentExContext` |
| `00495410` | `FindHandlerForForeignException` | 98.85 | **100** | `__pSETranslator` |
| `004954e0` | `___FrameUnwindToState` | 98.61 | **100** | `$T12708` |
| `00495650` | `CallCatchBlock` | 74.11 | **100** | funclet marker gone, `$T12726`, EH globals |
| `004957d0` | `BuildCatchObject` | 98.43 | 98.95 | `$T12738` |
| `004959e0` | `_DestructExceptionObject` | 98.11 | **100** | `$T12748` |
| `00495ae0` | `terminate` | 98.18 | **100** | `$T12234` |
| `00495b70` | `_inconsistency` | 74.16 | **100** | funclet marker gone, `$T12251` |
| `00495c00` | `__XcptFilter` | 99.06 | **100** | `__XcptActTab` |
| `00495d40` | `_xcptlookup` | 85.29 | **100** | `__XcptActTabCount` |
| `00495db0` | `_x_ismbbtype` | 96.00 | **100** | `__ctype` |
| `00496f50` | `___loctotime_t` | 98.91 | **100** | `__days` |
| `004973f0` | `$CRT_C_Initializer(1)` | 95.31 | 98.44 | `__iob` size |
| `004974b0` | `$CRT_C_Pre-Terminator` | unmapped | **100** | mapped once `__flushall`/`__fcloseall` were |
| `004974d0` | `__stbuf` | 95.83 | **100** | `__iob` size |
| `00499da0` | `_cvtdate` | 89.05 | 98.54 | `__lpdays`, `__days`, `_dststart`, `_dstend` |
| `00499f40` | `__fcloseall` | unmapped | **100** | row |
| `0049aa10` | `_raise` | 99.32 | **100** | `__XcptActTab` |
| `0049ab90` | `_siglookup` | 87.50 | **100** | `__XcptActTabCount` |
| `0049c7ae` | `_DirectSoundCreate@12` thunk | unmapped | **100** | row |

## Per-function results: gods98

Sizes are the ledger's address gaps; scores are full `make reccmp-check`
runs. Letters are the causes tallied below.

### The 95–99% band

| Address | Function | Size | `main` | After | Change, or what is left |
| --- | --- | ---: | ---: | ---: | --- |
| `0048d2c0` | lwo_load | 1280 | 99.29 | 99.29 | C: `lea edx`/`lea eax` for one argument, and `inc eax` scheduled one instruction early (effective match) |
| `0047ad70` | s_play_seam | 688 | 97.55 | 97.55 | C: `(slot * 3) + sound->voice` in `ecx`/`eax` here, `eax`/`edx` in the original (effective match) |
| `00481820` | mesh_get_surf_info | 672 | 97.98 | 97.98 | D: `texNum` and `tnumlen` swap stack slots |
| `0047a640` | audio_system_init | 448 | 99.25 | **100** | the `DirectSoundCreate` thunk (section 3) |
| `00476410` | Container_Frame_SearchCallback | 432 | 99.44 | 99.44 | C: `[eax + edx]` against `[edx + eax]` (effective match) |
| `00475a70` | Container_GetActivities | 400 | 99.30 | **100** | string marker (section 1) |
| `00473c60` | xf_texture_name_format | 320 | 98.99 | 98.99 | D: two `lea`s in the other order (effective match) |
| `00475e40` | Container_Frame_Find | 304 | 98.18 | **100** | string markers |
| `00475d10` | Container_AddActivity2 | 256 | 97.56 | **100** | string markers |
| `0048d7c0` | lwo_app_obj_load | 176 | 96.88 | 96.88 | C: `free(*ao)` loads into `ecx`, the original reuses `esi` (effective match) |
| `004876c0` | lws_initialise | 160 | 98.11 | 98.11 | C: `[esi + eax - 1]` against `[eax + esi - 1]` (effective match) |
| `00476770` | Container_GetAnimFileFrameCount | 112 | 97.67 | **100** | string marker |

### The 80–94% band

| Address | Function | Size | `main` | After | What is left |
| --- | --- | ---: | ---: | ---: | --- |
| `004869f0` | lws_parse | 3280 | 94.80 | 94.80 | D: the two `realloc`s of the node-list growth are interleaved in the rebuild; the original stores `nodeList` before computing `parentList`'s size |
| `0048c820` | lwo_surface_settings_read | 2720 | 80.66 | 80.66 | D: the local buffers sit 4 bytes higher (frame layout); one jump table is not recognised on the original side |
| `00472cd0` | container_load | 1648 | 85.36 | 85.36 | D: stack slots and `ebx`/`ebp` choice through the body |
| `00473690` | entity_visual_resource_load (Container_Clone) | 1168 | 83.22 | 83.22 | D: a 4-byte larger frame (`sub esp, 0x60` against `0x5c`); the clone-table `realloc` is scheduled differently |
| `00481c50` | mesh_uview_mesh | 848 | 85.06 | 85.06 | D: stack slots |
| `0047bdb0` | sound_stream_ring_buffer_tick | 784 | 81.98 | 81.98 | C/D: register choice and block layout |
| `00482e70` | mesh_load_texture | 448 | 91.56 | **100** | one exit (section 2) |
| `0047baa0` | sound_streamed_wave_buffer_setup | 400 | 82.51 | 82.51 | C: register choice |
| `0047bc30` | sound_stream_drain_check | 384 | 84.77 | 84.77 | C: `edi` against `edx` for the same global through a run of pushes |
| `00472710` | container_initialise_original | 240 | 80.43 | 80.43 | D: the stores of the default strings are scheduled differently |
| `00473370` | scene_action_name_set | 240 | 80.39 | 80.39 | C: `prevName` lives in `ebp` in the original, in a stack slot here |
| `00482040` | mesh_set_texture_time2 | 144 | 85.19 | 85.19 | C: `add eax, ebx` against a folded `[eax + ebx + 0x68]` |
| `004750b0` | render_object_set_packed_rgba_from_float_channels | 112 | 90.32 | 90.32 | B (rule B): the `|` operand order of the packed colour |
| `00473e10` | lego_fog_colour_set_adapter | 96 | 82.35 | 82.35 | B (rule B): the same `|` order |
| `00475830` | Container_SearchOwner | 80 | 85.71 | **100** | loop shape (section 2) |

Below 80% only `00488070` was touched (75 to **100**, section 2);
`004882e0` rose from 23.88 to 26.92 as a side effect.

## Why the others are short

### gods98 (20 functions still short in the two bands)

| Cause | Functions | Ledger bytes | What would close it |
| --- | ---: | ---: | --- |
| **B. Rule B phase** (finding 538): the commutative `|` order of a packed colour | 2 | 208 | the original's counter phase in Container.c; no inert declaration is allowed |
| **C. Integer register choice**: `0048d2c0`, `0047ad70`, `00476410`, `0048d7c0`, `004876c0`, `0047baa0`, `0047bc30`, `00473370`, `00482040`, `0047bdb0` | 10 | 4,688 | not found |
| **D. Stack slots, scheduling or frame layout**: `00481820`, `00473c60`, `004869f0`, `0048c820`, `00472cd0`, `00473690`, `00481c50`, `00472710` | 8 | 10,896 | not found |

Six are effective matches: `0048d2c0`, `0047ad70`, `00476410`,
`00473c60`, `0048d7c0` and `004876c0`.

What was tried without success, so a later sweep need not repeat it:

- `tools/msvc_permuter.py`, 150 s each with three workers, on `0048d2c0`,
  `0047ad70`, `00481820`, `00476410`, `00473c60`, `0048d7c0` and
  `004876c0`: no candidate improved any of them (0–7 attempts each).
- `004876c0`: `*(p + len - 1)`, `(p - 1)[len]`, `(len - 1)[p]`, the
  comparison reversed, the declaration split, `(int)len - 1`: unchanged;
  `(unsigned short)(len - 1)` 83%.
- `00476410`: `*(loop + s)`, `loop[s]`, `'?' ==` first, the locals
  reordered: unchanged.
- `0048d7c0`: `*ao = NULL` after the frees (93%), an inverted condition.
- `0047ad70`: the four orders of `slot * 3` and `sound->voice`: unchanged.
- `00481820`: four declaration orders of `texNum`/`tnumlen` and nine names
  for `tnumlen`: unchanged, so the slot order is not declaration order or
  the name.
- `004750b0`: `int`, `long` and `unsigned long` casts, regrouped `|`,
  reversed operand order: MSVC folds every form to the same code.
- `004869f0`: both `realloc` sizes written `count * sizeof`, and `+=`
  spelled out: unchanged.
- `00473690`: dropping the inner `loop` (the frame does not shrink) and
  the `clone` temporary (78.72%).
- `00488170` lws_interpolate (34.17%): the original passes pointers to
  `curr->hpb` and `next->hpb` (`lea ecx, [eax + 0x34]; add eax, 0xc`),
  which points at Gods98's inline `Maths_Vector3DInterpolate`. A nested
  inline Subtract/Scale/Add version scores 24%; the shape is close but not
  found.

### The CRT (LIBC members still short)

| Cause | Functions | What it is |
| --- | --- | --- |
| **L. LIBC data order**: an end-of-table pointer names the next object, which differs | `__NMSG_WRITE` (`_rterrs` end: `_errtable` against `__cfltcvt_tab`), `__dosmaperr` (`_errtable` end: `_iob` against `__HUGE`), `$CRT_C_Initializer(1)` (`_DoubleFormat` against `__half`) | the original's LIBC data placement; not an annotation |
| **R. Reccmp rendering**: a negative offset or a conditional jump into another function | `__output` (`___lookuptable - 0x20`, which shifts its jump-table placeholders), `_cvtdate` (`__lpdays - 4`, `__days - 4`), `_acos` (`jne __fast_exit` shown as a displacement), `__raise_exc` (its trailing jump table) | the operands are the same symbols; Reccmp prints them raw |
| **V. One `_CallMemberFunction` in the original** | `BuildCatchObject` | the original calls `_CallMemberFunction0` (`0048ebf0`) for all three; LIBC's `_CallMemberFunction1`/`2` are byte-identical copies that the original does not contain. That is what COMDAT folding of identical LIBC functions would leave, which the gate's `/OPT:NOICF` (finding 642) keeps apart; not pursued here |
| **G. The game's `_matherr`** | `__umatherr`, `__87except` | they call `0x00456210`, NERPs' zero-returning function, which the original links as `_matherr` (finding 643). Giving `tutorial_script.c`'s definition that name in the matching build is a game-unit change, left for a follow-up |
| **N. Not LIBC's** | `0049cc70` `wad_bitstream_output_initialize` (LATE-001, 44%) | a game function in `rnc.c`, outside this sweep |

Still unmapped: `0049221e`/`00492225` (87cdisp's `.asm` entry points, no
map symbol), `_strncnt` (`00493ec0`, static in two members), `0049573a`
and `00495bde` (the funclet labels of section 4, now inside their
functions), and `00495b4e` (`_abort`'s place in hooks.cpp; the rebuilt
`_abort` comes from abort.obj elsewhere).

## Files

- `src/gods98/container.c`: three `STRING` markers, `Container_SearchOwner`'s loop.
- `src/gods98/lws.c`: `lws_maths_interpolate` and its use.
- `src/gods98/mesh_unit.c`: `mesh_load_texture`'s single exit.
- `src/crt/libc_symbols.csv`: 32 rows and the `size` column (the existing rows gain an empty field).
- `src/crt/crt.cpp`: `_acos`'s address, the two funclet markers removed,
  four port-model `GLOBAL` markers retired.
- `src/link_order.cmake`: `crt.cpp`'s median `00493ef0`.
- `docs/coverage/translation-units.csv` (regenerated), the match baseline,
  `match-listing-digests.txt`, `binary-progress.json` and the lint ledger.

No file owned by #748, #749, #750 or #560 is touched.

## Reproducing

```sh
make reccmp-check        # fails only on 0x0048e0ad, re-addressed to 0x0048e0a4
make match-progress
make match-function ADDR=0x00475830   # and the other addresses above
```
