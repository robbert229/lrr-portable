# Finding 677 — Weapons.c, Mesh.c and Lwt.c: source shapes for the big functions and their neighbours

Issue #808, part of #388 and ADR 0005. Base: `main` `316b70f6`. Measured
with one full `make reccmp-check` of the final tree; the single-function loop
(`tools/match_function.py`) was used for each experiment.

## Summary

Five functions become exact and one more rises to 97.65%. All six are in
Mesh.c (G98-015) and Lwt.c (G98-031, `src/gods98/lwo.c`). Every one of the
four Lwt.c functions that changed now has the original's size, so the whole
of Lwt.c is size-correct for the first time. Weapons.c is unchanged: nothing
tried there is a committable original form (below).

| Address | Function | Size | `main` | After | Size agrees |
| --- | --- | ---: | ---: | ---: | --- |
| `004807a0` | mesh_obtain_from_list | 47 | 44.44 | **100** | yes (was longer) |
| `00481ac0` | mesh_get_texture_seq_info | 184 | 31.11 | **100** | yes (was 18 bytes short) |
| `00482040` | mesh_set_texture_time2 | 131 | 85.19 | **100** | yes (was 1 byte short) |
| `0048c210` | lwo_points_read | 321 | 72.97 | **100** | yes (as before) |
| `0048c3e0` | lwo_polygons_read | 677 | 58.35 | **100** | yes (was 48 bytes short) |
| `0048c820` | lwo_surface_settings_read | 2,716 | 80.66 | **97.65** | yes (was 52 bytes short) |
| `00480ac0` | mesh_parse_lwo | 3,386 | 78.43 | 78.43 | yes (as before) |
| `00470760` | weapons_mesh_build_segment | 2,912 | 54.67 | 54.67 | no: 0x22 bytes long (as before) |

No function is lower than the baseline.

| Measure | `main` `316b70f6` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 340,131 (52.93%) | **341,491 (53.15%)** |
| Functions that rose / fell, full | | 6 / **0** |
| Reccmp accuracy, full / game-only | 97.20% / 96.88% | 97.28% / 96.97% |
| Correct-size functions, full / game-only | 2,600 / 2,309 | **2,605 / 2,314** |
| Units with every function the correct size | 166 of 227 | **167** (G98-031) |
| Identical prefix, raw / masked | 11 B / 528 B | 11 B / 528 B |
| `.text` identical at original addresses | 25,975 B (4.04%) | 25,978 B (4.04%) |
| Unit-relative identical bytes | 280,229 (50.59%) | 281,589 (50.84%) |
| Entry point (original `0048f000`) | `0048d0e0` | `0048d130` |
| Function-order inversions, full / game-only | 222 / 1 pairs | 222 / 1 pairs |
| Image end (`make pe-audit`) | `0x7f5000` | `0x7f5000` |

Every size change moves toward the original: Mesh.c and Lwt.c together grow
by 96 bytes after alignment, so the entry point is 96 bytes nearer the original's. No unit
moves away from its original size.

`make match-progress`, `make unit-data-offsets` (no tracked file changed)
and `make pe-audit` were run on the final tree and the generated files are
committed. `make tools-test` (no `--allow-growth`), `make winapi-link-check`,
`make reccmp-lint` and native ctest (37 of 37) pass, and the native build
starts.

Only `src/gods98/mesh_unit.c` and `src/gods98/lwo.c` change. No other
thread's file is touched.

## Mesh.c

### `004807a0` Mesh_ObtainFromList: the external globals struct

Finding 669 found this function to be the same case as AITask.c's pool
allocator. The original tests the free-list head, calls Mesh_AddList if it is
null, and always reloads the head afterwards. With `static Mesh_Globs
meshGlobs` MSVC 5.0 keeps the tested head in a register across the call. With
`Mesh_Globs meshGlobs = { 0 };` the function is exact, and no other function
in the unit changes (single-function scores of every annotated function in the unit compared).

As finding 669 shows, `= { 0 }` keeps the struct in the object's `.bss` (an
external symbol rather than a static one), so the storage does not move:
`_meshGlobs` stays at `005343c4`. This is the second unit where the original's
code calls for an external struct. A follow-up should still test every unit's
`static` globals struct this way, as finding 669 proposed.

### `00481ac0` Mesh_GetTextureSeqInfo: declaration order and no final return

- **`tnamelen` declared last.** The original keeps `num` on the stack (in the
  slot of the `push ecx` prologue) and `n` in `edx`. With `tnamelen` declared
  first, as before, MSVC gives `num` a register. Declaring `unsigned long
  tnamelen = strlen(tname);` after `c` and `n, k, num` gives the original's
  allocation exactly. Of seven declaration orders tried, the two with
  `tnamelen` last are exact and the other five all score 31.11%.
- **`k > 0`.** The loop test is `test; jbe`, an unsigned `> 0`.
- **No `return` on the success path.** The original's success path ends
  `mov [ecx], edx; pop ...; ret` with no `mov eax, 1`: it falls off the end
  with the `tnumlen` pointer in `eax`. Only the two failure paths `return 0`.
  The source keeps that and says so in a comment.

### `00482040` Mesh_SetTextureTime2: a group pointer

The original forms `&mesh->groupList[groupID]` and reads
`group->lightWaveSurfaceInfo` through it each time, as OpenLRR's
`Mesh_SetTextureTime2` does (`add eax, ebx; mov ecx, [eax+0x68]`). Our
`Mesh_LightWave_Surface* surf` local folded the address into one operand. One
block-scoped local is swapped for another, so the unit's symbol count is
unchanged.

### Unchanged in Mesh.c

- **`00480ac0` mesh_parse_lwo (78.43%).** Finding 661 left it at stack
  offsets, with an 85.20% form that cannot be committed until the unit's real
  declaration count is known. Nothing here changes that. One more structural
  residue is visible in the listing: the original keeps the two `malloc`s of
  `surfTextCoords` (UV and plain branches) as separate calls, where MSVC
  cross-jumps them in ours. That follows from `size` living in memory in the
  original and in `ebx` in ours, which is the same stack and register
  residue.
- **`00481820` mesh_get_surf_info (97.98%).** `texNum` and `tnumlen` are in
  each other's stack slots. Swapping their declaration, moving them and
  `baseName`/`textName` into the sequence block (OpenLRR's scoping), and
  OpenLRR's `if ((... = Mesh_LoadTexture(...)) == NULL) break;` all leave the
  code unchanged.
- **`00481c50` mesh_uview_mesh (86.97%).** The same kind of slot swap, for
  `surface`/`polyVert`. `make match-search MODE=decls` (85 orders) changes
  nothing.
- **`00482500` mesh_set_vertices_point_normal_at (77.17%).** This is finding
  661's `fst` normalise. The original stores the modulus and then the factor
  to one stack slot and pops a dead copy. Not pursued further.

## Lwt.c (`src/gods98/lwo.c`)

The unit is Gods98's Lwt.c, and OpenLRR's `Lwt.cpp` rebuilds that source
(trigger-segfault/OpenLRR, `src/openlrr/engine/gfx/`). Its PNTSprc, POLSprc
and SURFprc compile to the original's code once they are translated to C. The
earlier bodies were written from the listing, and their differences were all
in shape:

### `0048c210` PNTSprc (lwo_points_read): early return, counted-down size

- `if (!verts) { free(sp); return 1; }` before the vertex allocation, where
  ours had the whole loop inside `if (verts)`;
- `while (csize > 0) { ... csize -= 12; }` with a separate `i` for the debug
  print, where ours had `for (i = 0; i < csize; i += 12)` and a `count`;
- the `sp`/`cp`/`vptr` pointers as in Lwt.c.

The instructions were already the original's. What changed is the stack
layout (the point at `[esp+0x10]` and the read pointer at `[esp+0x1c]`),
which declaration order does not move (`decls` search, 49 orders, no change)
but the early-return shape does.

### `0048c3e0` POLSprc (lwo_polygons_read)

- The grown array is assigned to `ply`, and `ply += pc` points it at the new
  entry, where ours had a separate `newPolys` and `&newPolys[count]`;
- the vertex loop is a counted-down `while (vcount) { ...; vcount--; }`;
- the surface index goes through a `vsurf` local with `vsurf--`.

This adds the 48 bytes the function was short. The swap must be
`LWO_SWAPWORD` (`+` of the shifted halves). `LWO_SWAPSHORT` (`|`) gives
56.78% and the old size.

### `0048c820` SURFprc (lwo_surface_settings_read): 80.66 → 97.65

- each chunk branch's locals in its own block: TIMG's `pos`, `len` and
  `cps[128]`, TTEX's `srfbN`, TFLG's and FLAG's `sval, i, f = 0` and their
  name tables as initialised arrays, TSIZ's and TCTR's `td` and `ccp`, and the
  value block's `sval`/`fval`;
- `if (setVal) { ... if (surf) { ... } }` as two tests, not `type && surf`;
- the TIMG sequence test truncates the name before it sets the sequence flag;
- `cp += len; csize -= len;` in that order after the name;
- Lwt.h's swap macros: `LWSVALUE` for the chunk length (one statement,
  `len = LWSVALUE(*(unsigned short*)cp)`, 95.85% in the loop against 95.13%
  for OpenLRR's two-statement form) and `LWSPVALUE` for the flag and value
  words. Both are added to `lwo.c` next to the existing swaps;
- the COLR debug print takes the bytes as `unsigned char`. The original
  zero-extends them (`xor ecx, ecx; mov cl, [ebp+3]`), so Gods98's `cp` must
  be unsigned there.

TIMG's inner `len` is a `size_t` because the port's `lwExtractString`
(`string_case_substring_find`) takes a `size_t*`. It is `unsigned int` in
MSVC 5.0, so the code is the same, and the native 64-bit C++ compile needs
it.

**What remains** in SURFprc: one register choice. The original keeps the
zero used for the `surf` tests in `esi` and swaps the chunk length in `eax`
(`xor edx, edx; mov dl, ah`). Ours uses `edi` for the zero and swaps in
`ecx` (`shr ecx, 8`). Seven declaration and type variants (`sc` and
`setVal` unsigned or long, the order of `srfb`/`sc`, `char* sp, *cp` split,
`cbuff` first) change nothing. `LWSURFACE* srfb = NULL` (OpenLRR's "dummy
init") costs 31 points.

### Unchanged in Lwt.c

`0048d2c0` LoadLWOB (lwo_load, 99.29%) has one `eax`/`edx` swap in an
argument. The other Lwt.c functions were already exact. In the loop, five of
them show 1–3 points lower because of callee names only.

## Weapons.c: nothing committable

Weapons.c has no Gods98 source, and OpenLRR's `Weapons.cpp` is rewritten
rather than rebuilt (its Weapon_Lazer_InitMesh builds the octagon from
renamed, precomputed terms). It is not evidence of the original's shape.

### `00470760` weapons_mesh_build_segment (54.67%)

Up to the octagon factor, the code is the original's apart from stack
offsets. The difference is the x87 schedule of the 24 vertex components.
Decoded from the listing, the original keeps `dir` at `[esp+0x10]`, the
factor at `[esp+0x1c]`, `root2` at `[esp+0x20]` and `perp` at `[esp+0x24]`.
It writes the radius-scaled cross product straight back into `dir`'s slots.
Tried:

| Variant | Loop score | Size |
| --- | ---: | --- |
| as committed (`dir = cross` struct copy) | 54.33 | 0xb82 (orig 0xb60) |
| use `cross` directly, no copy into `dir` | 44.03 | frame 12 bytes smaller |
| `dir.x = cross.x; ...` field by field | 52.17 | 0xb74 |
| the copy through a `*r = cross` pointer | 54.33 | 0xb82 |
| scale `perp` before `dir` | 54.12 | 0xb84 |
| `root2` computed after the cross product, or after the scaling | 54.33 | 0xb82 |
| `make match-search MODE=operands`, 1,200 s, 3,552 attempts | **59.57** | not checked |

The operand search's best is 18 scattered commutations: `a * K` in some
vertex terms and `K * a` in others, and `(-x) + y` for some sums only. No
consistent source form would produce it, so it is not committed. It is kept
in `build-match/search/470760-operands/best.patch` for whoever takes this
further.

### `0046f920` weapons_projectile_update (51.28%)

The original's two Container_SetOrientation calls share their tail (a
cross-jump: the straight branch ends `jmp` into the curved branch's last
pushes and call). It pushes every float argument through `fld`/`fstp
[esp]`, and its frame is 8 bytes larger. Forms tried, all lower: OpenLRR's
single orientation call after both branches with a `newDir` local (20.25%,
and 31.87% with our curve arithmetic), the same with the direction computed
field by field (31.62%), OpenLRR's `Maths_RayEndPoint` straight branch with
two calls (27.31%), and either half of OpenLRR's form combined with ours
(24.22% and 49.47%, the second 0x10 bytes long).

## Reproducing

    make match-function ADDR=0x0048c820
    make match-search ADDR=0x00470760 MODE=operands SECONDS=1200
    make match-search ADDR=0x00481c50 MODE=decls SECONDS=500
    make match-search ADDR=0x0048c210 MODE=decls SECONDS=400
