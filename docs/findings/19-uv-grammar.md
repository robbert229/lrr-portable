# 19 — `.uv` sidecar grammar: fully resolved (and mostly unread)

*Target: `LegoRR.exe` (x86 PE, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).*
*Provider: Ghidra 12.1.4 via REA. All addresses cite the shipped image.*

## TL;DR

The `.uv` files (60 shipped, all paired by basename with a `.lwo`) are a
line-based **text sidecar** supplying two things the LWO itself lacks:
per-surface **texture-path overrides** and per-corner **UV coordinates**.
The parser is `FUN_00481c50` (`0x481c50`), called exactly once per model —
from the mesh builder `FUN_00480ac0` — and *only* when a `.uv` stream exists.
Everything else in the file (track-name lines, trailing keyframe blocks) is
**structurally ignored** by the engine. Grammar validated field-for-field
against all 60 shipped files: **60/60 clean**.

This closes finding 11 §6 open item 1. It also corrects finding 11 §2: the
earlier "parses it via `FUN_0047fb60` → `FUN_0048eb90`" note conflated the
*load* helpers with the *parse* — `FUN_0047fb60`/`FUN_0048eb90`/`FUN_0048eae0`
are the generic open-and-read-everything layer, not the grammar parser.

The reconstruction (`src/gods98/model.cpp`, `model_apply_uv_text`) implements
this grammar and is corpus-validated: the shipped 60 sidecars all apply and
digest identically to the `tools/rrlwo.py` reference, path overrides and
negated-v corner pairs included (meson suite `model`). The engine's
duplicate-vertex-on-conflict remesh (§2) is a consumer-side consequence of
corner vs vertex indexing; the port keeps the per-corner table and resolves
conflicts at draw-assembly time.

## 1. Evidence chain (load → store → single consumer → teardown)

| Step | Address | Facts |
| --- | --- | --- |
| Compose path | `0x48d34f` inside `FUN_0048d2c0` | the string `%s.uv` (`0x4afc90`) is referenced **only** here (only xref); the LWO loader builds `<name>.uv` only when the caller supplied a uv slot (`param_6 != 0`) |
| Existence check + open | `FUN_0047fb60` (`0x47fb60`), `FUN_0047f6e0` | generic load-and-sanity helper; result is a **stream handle** stored into model slot `+0x1c` (`*param_6` written at two points, `0x48d2c0` body) — never read there |
| Storage | `FUN_0048d7c0` | model struct (32 B): `+4` nVerts, `+8` nPolys, `+0xc` nSurfaces, `+0x10` PNTS array, `+0x14` POLS meta, `+0x18` SURF records (0x54 stride), `+0x1c` uv stream |
| Single consumer | `FUN_00481c50(param_1=model, param_2=uvTable)` | called from `FUN_00480ac0` (mesh builder) inside `if (*(model + 0x1c) != 0)`; also `FUN_00480ac0` reserves a per-vertex uv table (`alloc(nVerts * 0x140)`) only on this branch |
| Teardown | `FUN_0048d870` | model free calls `FUN_0047fac0(model[7])` — **closes the stream, reads nothing** |

Exhaustive cross-checks that no other code reads the stream: all callers of
the stream primitives `FUN_0047f980` (read), `FUN_0047f850` (seek),
`FUN_0048f460` (mmap seek), `FUN_0047fb10` (size) were enumerated — every
site belongs to the LWO chunk handlers, FLIC player, WAV loader, savegame
layer, or scene loader, none to the uv slot (the `"%s.uv"` string has exactly
one xref, so no other code opens the file).

## 2. Parser mechanics

`FUN_00481c50` is a line-loop state machine:

- `FUN_00480050(buf, 0x400, stream)` — reads one CRLF line; returns 0 at EOF
  (clean exit).
- `FUN_004774b0(line, &tokens)` — splits on the ctype-class whitespace table
  (`PTR_DAT_004afd68` / `FUN_0048f1d0` mask 8) into a token-pointer array;
  returns the token count. **Blank lines return 0 and the entire state
  machine skips them** (`if (iVar9 != 0)` guard) — line counting never
  advances on blanks.
- Numeric helpers: `FUN_0048db70` = atoi, `FUN_0048d980` = atof,
  `FUN_00477550` = strdup.
- Counters: `local_830` = processed-line index; `local_838` phase-A
  line index; `local_840` uv-set/block index; `local_820` current polygon
  index; `local_83c` current corner index inside that polygon.

Phase gating uses the **model's own counts** (`+8` nPolys, `+0xc` nSurfaces),
never the file's header numbers — i.e. the `.uv` content must be *positionally*
consistent with the `.lwo` it sits next to; the header numbers are purely
decorative for the engine.

## 3. Grammar

1-based line numbers, with `s` = model surface count (`SRFS` length) and
`m` = model polygon count (`POLS` length):

| Lines | Content | Engine use |
| --- | --- | --- |
| 1 | version, always `"2"` | skipped (never inspected) |
| 2 | `s`, the surface count | skipped — value validated as equal to `s` in all 60 files |
| 3 … 2+`s` | one *track name* per surface, e.g. `Arm L  Y` | skipped (first `s` phase-A lines) |
| 3+`s` … 2+2`s` | one **BMP path per surface**, e.g. `Z:\Lego\data\creatures\lavamonster\LavaTextures.bmp` | **strdup'd over each SURF record's `+8` field** — the texture-name slot that `SURF/TIMG` populates (`puVar4[2] = FUN_0048c0c0(...)` in `FUN_0048c820`, `0x48c820` region). The `.uv` path therefore *overrides* the LWO's TIMG. |
| 3+2`s` | `m`, the polygon count | skipped (enters phase B) |
| then, `m` blocks | block header `"<polyIdx> <vertexCount>"` followed by exactly `vertexCount` lines `"<u> <v> <z>"` | see below |
| tail | `s` keyframe blocks: `"<k>"` + `k` lines of 6 floats | **structurally never read** (see §5) |

Validation of block headers across the corpus shows `polyIdx` always equals
the block's ordinal and `vertexCount` always equals the model's real vertex
count for that polygon — so, functionally, each polygon contributes one block
listing its corners **in POLS ring order**; the second header token is unused
by the engine (the model's `POLS` meta is authoritative).

## 4. Engine semantics of the uv blocks

For each `"<u> <v> <z>"` line the parser writes, for corner `local_83c` of
polygon `local_820`:

```c
u = atof(tok[0]);
v = -atof(tok[1]);                       /* v is NEGATED on write */
vi = POLS[poly].vertices[corner];        /* u16 vertex index, from model */
if (uvTable[vi] unset) { uvTable[vi] = (u, v); mark vi; }
else if (uvTable[vi] != (u, v)) {         /* two faces want different uv for the SAME vertex */
    grow point array (FUN_0048dc80, realloc to (nVerts+0xb)*0xc — 11 extra points per growth);
    point[nVerts++] = point[vi];
    POLS[poly].vertices[corner] = nVerts - 1;   /* rewrite index in place (u16 store) */
    uvTable[new vi] = (u, v);
}
```

Consequences:

1. **UVs are per corner of a polygon ring**, keyed via the model's POLS
   vertex indices — not per original vertex. Vertex duplication (seam
   "welding break") extends `model+0x10` and rewrites POLS indices in place;
   `model+4` (nVerts) grows accordingly. The 0x140-byte-per-vertex reservation
   in `FUN_00480ac0` is the budget for this growth.
2. **The stored v is `-v` of the file value** — exporters must negate v to
   reproduce the engine's texture-space v.
3. The third token (`z`, always `0` in the shipped corpus) is ignored.
4. When a `.uv` is present, the mesh built in `FUN_00480ac0` samples uv pairs
   per corner from the uv table instead of the no-uv path
   (`alloc(nVerts*0x10)` branch). This is where the `.uv` sidecar actually
   changes the drawn mesh — and why the model struct's `+0x1c` slot gates the
   whole thing.

## 5. Why the trailing keyframe blocks are dead

After the last uv block, the parser's header-branch gate `if (local_840 <
m-1)` fails forever (`local_840` is stuck at `m-1`, since the increment is
inside the gated block), and `local_83c` is never reset (it is zeroed only
inside that gate). So every subsequent line takes
*header-condition-true → gated block false → no action* until EOF, where the
reader returns 0 and the parser exits cleanly. The `return` on
`if (m <= local_840)` is never reached from the tail. In other words the
tail (per-track envelope data: `k` keyframes × 6 floats — position+rotation,
evidently a bake of the LightWave animation) is **structurally unreachable**
while the stream parses. Scene animation instead comes from the inline
`ObjectMotion` envelopes in the `.lws` files (§ finding 11 §4), and LWO
`CRVS` curves are likewise parsed-and-discarded (`FUN_0048c360`). The shipped
pipeline duplicated animation into the scene files and left `.uv` as an
object-side sidecar carrying only what `.lwo` couldn't: textures + corner UV.

## 6. Corpus validation (60/60)

Scripted against both wads (`LegoRR0`/`LegoRR1`), parsing each `.uv` under the
grammar above with the paired `.lwo` counts:

- 60/60 files satisfy: `line1 == "2"`, `line2 == s`, `line_{2+2s} == m`,
  block headers `"<bi> <count>"` exactly sequential, block data line counts
  matching the model's per-polygon vertex counts, and a well-formed tail of
  `s` keyframe blocks with self-consistent `k` lines (`60/60`).
- Track-name lines equal the `SRFS` surface names in **58/60** files.
  The two exceptions are the same shipped file twice:
  `Mini-Figures\Pilot\GC_Larm.uv` and `World\Shared\GC_Larm.uv` rename the
  surface `Engineer orange` (LWO) to `PILOT_Blue Plastic_dig` (uv) — the
  shared arm mesh is re-skinned per pilot variant via its `.uv`, i.e. the
  sidecar encodes *material identity*, not just coordinates.
- Path-vs-TIMG: wherever the LWO actually carries a TIMG path (8/8 surfaces),
  the `.uv` path **differs** — the override is real, not redundant; where the
  LWO's TIMG is empty, the `.uv` is the sole texture source.
- No uv path equals its LWO TIMG in any file.

## 7. Implications for tooling

- `.uv` support landed in `rrlwo.py` (2026-10-20): surface-path overrides and
  per-corner UVs are applied automatically (v negated, keyed vertices handle
  the duplication), `--no-uv` opts out, and `dump` reports the sidecar. All
  60 models verified.
- `.uv` round-trip writer landed in `rrlwo.py` (`uv` command, 2026-10-20):
  reconstructing each of the 60 shipped sidecars from model + original yields
  **byte-identical** files; name lines and tail are kept verbatim (the tail
  needs no understanding — it is simply opaque, which is also how the engine
  treats it). Regenerating from the model alone (`--no-uv`) is available but
  lossy: it loses sidecar-only material names, and synthesizes planar uv and
  a dummy tail. Verified on `Base.lwo` that a synthesized sidecar exports the
  exact same uv set as the direct planar projection.

A `.uv` round-trip is now two commands: `uv` to rebuild, `gltf` to check.
