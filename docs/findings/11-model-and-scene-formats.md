# 11 — Model & Scene Formats: LightWave LWO + LWSC

*Target: `LegoRR.exe` (x86 PE, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).*
*Provider: Ghidra 12.1.4 via REA. All addresses cite the shipped image.*

## Summary

The game does **not** use a bespoke model format. It embeds a parser for
**LightWave 3D 4.0-era files** — the `.lwo` object format and the `.lws` scene
format — and ships its 1,132 model files and 750 scene files in those
container formats inside `LegoRR0.wad`. The 1999 game was built with
LightWave 4/LightWave 3D 5-generation tooling; the parsers match the LWO 4.0
chunk set, not the later LW0/LW1 (LW5+) format.

- `.lwo` → binary LightWave object (`FORM` / `LWOB`), read at runtime.
- `.lws` → LightWave scene **text** (`LWSC`), read at runtime; composes objects,
  parent/child bindings, and animation envelopes.
- `.uv` → companion text sidecar, loaded alongside `.lwo`; supplies surface
  texture paths and per-corner UV coordinates — fully documented in finding 19.
- `.ae` → `Lego* { … }` block-text files, read by the block-text machinery of
  finding 12 (`FUN_00478e60` scanner over WAD-read buffers) — the earlier
  "likely editor-side" guess was wrong; they are consumed at runtime.
- `.wav` / `.bmp` → standard formats: sounds load via `SoundLoad`
  (`FUN_0047a870`, `%s.wav` at `0x4aae5c`) and textures via the TIMG/TIMG-slot
  path — both fully documented, respectively, in findings 14 and 13.
- `.x` → **DirectX X (XOF) text** files (`xof 0302txt 0064` / `xof 0303txt
  0032`); 164 shipped, but the engine has *no* XOF reader (see §6, item 5) —
  these are editor-side leftovers.

Internal developer path strings corroborate the pipeline: `WAD Load: %s`,
`WAD Fail: %s`, and `C:\Dev\SourceSafe\gods98\dx6\god…` (`gods98` — the
working title, in `0x4aaf74–0x4ab004`).

The reconstruction (`src/gods98/model.cpp`) implements these semantics and
is corpus-validated: 1,132/1,132 shipped `.lwo` entries parse, all 60 `.uv`
sidecars apply, and the corpus view (36,506 vertices / 20,119 polygons /
3,396 surfaces) digests identically to the `tools/rrlwo.py` reference
(meson suite `model`). The loader follows *corpus* behaviour where the
exe's memory layout leaves slack — notably the POLS index origin is
auto-detected per file (§3: any zero index anywhere selects 0-based)
rather than assumed 1-based.

## 1. Model load chain

```
FUN_00472cd0 (entity creation)        ModelResourceResolveOrLoad @ 0x4885c0
        └──────────────┬──────────────────────┘
                  FUN_004809e0            model entry
                       └─ ModelObjectCreate    create 0x20-byte model header
                            ├─ LwoStringDuplicate @ 0x48c0c0 (chunk/surface name helper)
                            └─ LoadLWO @ 0x48d2c0
```

`ModelObjectCreate @ 0x48d7c0` (`0x48d7c0`–`0x48d86b`) zero-allocates a 32-byte model struct and
hands out its fields to the loader: `+4` vertex count, `+8` polygon count,
`+0xc` surface count, `+0x10` vertex array, `+0x14` polygon metadata, `+0x18`
surface table, `+0x1c` the `.uv` sidecar stream (see §2 and finding 19). The
loader is also the only place the format string `%s.lwo` (`0x4afc98`) is
referenced.

## 2. LWO file loader — `FUN_0048d2c0` (`0x48d2c0`–`0x48d7b9`)

File access goes through `FUN_0047f6e0(path, "rb")`, the shared file-open layer
that resolves either the filesystem or the WAD archives (evidence: `WAD Load: %s`
success path, `WAD Fail: %s` failure path; the open helper returns a stream
handle used by all chunk readers below).

Structure accepted:

| Field | Width | Notes |
| --- | --- | --- |
| `"FORM"` | 4 B ASCII | compared via `__strcmpi` against `0x4afc88` |
| size | u32 **big-endian** | byte-swapped in place (`0x48d319ff` arithmetic pattern) |
| `"LWOB"` | 4 B ASCII | `0x4afc80` |
| chunk loop | | while bytes remain: 4 B tag + 4 B BE size, then dispatch |

Chunk dispatch (case-insensitive, tag constants in `.data` `0x4afc28–0x4afc48`):

| Tag | Address | Handler | Purpose |
| --- | --- | --- | --- |
| `PNTS` | `0x4afc48` | `FUN_0048c210` | point (vertex) array |
| `SRFS` | `0x4afc40` | `FUN_0048c690` | surface **name** list |
| `SURF` | `0x4afc38` | `FUN_0048c820` | per-surface settings |
| `CRVS` | `0x4afc30` | `FUN_0048c360` | curves — read then **discarded** |
| `POLS` | `0x4afc28` | `FUN_0048c3e0` | polygons |
| other | — | skip (`FUN_0047f850`) | unknown chunks skipped safely |

Debug logging when verbose: `LW OBJECT %s %d\n` (`0x4afc50`); failure anywhere:
`Error in Lightwave file %s` (`0x4afc64`).

When the model header has a UV slot (`param_6 != 0`), the loader **also** opens
`%s.uv` (`0x4afc90`) for the same base name — the existence check goes through
the generic load helper `FUN_0047fb60`, and the resulting stream handle is
stored in model slot `+0x1c` (`*param_6` is written only, never read here).
Model teardown (`ModelObjectFree @ 0x48d870`) merely closes the stream. The only consumer is
the mesh builder: `FUN_00480ac0` calls `FUN_00481c50(model, uvTable)` iff the
stream is present — that is the actual `.uv` text parser, not `FUN_0047fb60`
or `FUN_0048eb90` (an earlier note here conflated those load helpers with the
parser). Field-level grammar, engine semantics, and 60/60 corpus validation:
**finding 19**.

## 3. Chunk layouts (from handler pseudocode)

All multi-byte fields on disk are **big-endian**; the handlers byte-swap.

- **PNTS** — `n = size / 12`; `n` × {f32 x, f32 y, f32 z}. Each coordinate is
  read through `FUN_0048c1d0` (read 4 B + byte-swap). Vertices allocated as a
  contiguous float array; count written back as `size / 0xc`.
- **POLS** — polygon stream, 2-byte elements: per polygon {u16 vertex-count,
  count × u16 vertex indices (1-based), u16 surface index − 1}. Debug strings
  `Poly %d, Vertex Count %d > ` and `Surface :- %d`. The per-polygon surface
  index form matches **LWO 4.0 `POLS`** (which carries surfaces per-polygon),
  not the LW5 `Pnts/Pols/VMap` split.
- **SRFS** — sequence of NUL-terminated ASCII surface names, each padded to even
  length; builds the ordered name table (`Surface %d > %s` debug).
- **SURF** — large keyword-driven parser (`FUN_0048c820`, `0x48c820–0x48d2a0`).
  Per-surface settings assembled as `Surface %d :- %s` entries, with keyword
  vocabulary (all in `.data` `0x4afac0–0x4afb7x`): `Additive`, `Double Sided`,
  `Sharp Terminator`, `Transparent Edge`, `Opaque Edge`, `Color Filter`,
  `Color Highlights`, `Smoothing`, `Outline`, `Luminous`, `Anti Aliasing`; the
  surface name default is `Default` (`0x4afc20`). Within it, `COLR` (`0x4afbe4`)
  and `TIMG` (`0x4afbf8`) sub-tags carry surface colour and image settings, and
  `(sequence)` (`0x4afbec`) denotes an animated image sequence (formats `%d,%d`
  and ` %s (%d)` nearby). Surface name comparison is case-insensitive
  (`__strnicmp`).
- **CRVS** — u16-BE curve data scanned only to compute its extent and then freed;
  curves are unused at runtime.

## 4. `.lws` scene parser — `FUN_004869f0` (`0x4869f0`–`0x4876b6`)

LightWave scene files **are** read at runtime (one byte-level occurrence of the
magic `LWSC` exists, at `0x4ab6bc`, xref'd from the parser at `0x486a80`/
`0x486a90`). Adjacent context strings: `FirstFrame`, `LastFrame`, `1`, `lws`,
`-`. Scene grammar is matched keyword-by-keyword with `__strnicmp`:

`First` / `Last` (`FirstFrame` / `LastFrame` equivalents), `ParentObject`,
`PivotPoint`, `UnseenByCamera`, `ShadowOptions`, `ObjDissolve`, `ObjectMotion`,
`EndBehavior`, `(envelope)` (`0x4ab5f8–0x4ab667`).

Keywords `.ae` (animation-event, `Lego* { … }` block text) is read through a
generic block-text reader rather than a dedicated loader: no `Lego*`/`.ae` magic
string or byte pattern occurs anywhere in the image. The scene parser and the
block readers meet in `FUN_004765c0` (`0x4765c0`, the animation-assembly
worker called from both the entity-creation path and `FUN_00475d10`).
The `Lego*` block grammar is shared with the `.ptl` particle files and is a
follow-up target.

## 5. File-format inventory reconciliation

From `unpacked/LegoRR0` (5,631 files): 2,523 `.bmp`, 1,132 `.lwo`, 750 `.lws`,
240 `.map`, 220 `.blx`, 209 `.wav`, 164 `.x`, 60 `.uv`, 56 `.ae`, 44 `.ol`,
41 `.txt`, 37 `.ptl`, 34 each `.nrn/.npl/.nrm`, 21 `.ifo`, 18 `.flh`, 9 `.blk`,
4 `.epb`, 3 `.lev`, and one-offs. All `.lwo` files verified as
`FORM…LWOB`; all sampled `.lws` as `LWSC\r\n1`. `.ae`/`.ptl` verified as
`\r\nLego* {`.

## 6. Open items / next work

1. ~~**`.uv` grammar**~~ resolved by **finding 19**: line-stream grammar in
   `FUN_00481c50` (single consumer, called from `FUN_00480ac0` when
   `model+0x1c` is set); per-surface texture-path overrides + per-corner UV
   with v-negation and vertex duplication; names and trailing keyframe blocks
   never read. Validated 60/60 against the shipped corpus.
2. ~~**`SURF` field mapping**~~ resolved by **§5b** (record layout and the
   full sub-block vocabulary, pinned by parsing all 1,132 models) and
   **finding 19** (the `+8` TIMG-slot override semantics); what that left
   unresolved is only the unused-in-practice fields' bit meanings.
3. ~~`Lego*` block reader~~ resolved by finding 12 (`FUN_00478e60` scanner +
   `FUN_004792e0`-style path lookups over WAD-read buffers).
4. ~~Textures~~ resolved by finding 13: `TIMG` names are bound at model-open in
   the material pass `FUN_00480ac0` → `FUN_00481820` → cache `FUN_00482e70`;
   `LWO:NOTEXTURE` (`0x4a1dbc`) is the stub passed by entity registration
   `FUN_0040b5f0` (`0x40b623–0x40b656`).
5. ~~`.wav`/`.x` loaders and the D3DRM dependency~~ resolved 2026-10-20:
   * `.wav` — `SoundLoad` (`FUN_0047a870`) + the `mmio` region `0x488c00–
     0x489330`, closed by finding 14.
   * `.x` — all 164 shipped `.x` files are **XOF text** (20× `xof 0302txt
     0064` in `MiscAnims\*`/`Geo-Dome`, 144× `xof 0303txt 0032` for vehicle
     wheels and `World\ProMeshes\*PM*`), but the engine never loads them:
     no `xof`/`FileAsObject`/`DXFile` string anywhere in the image, no
     `"%s.x"` path format, no `dxfile.dll`/`D3D*.dll` import (finding 15
     shows only `DDRAW`, `DINPUT`, `DSOUND`, `WINMM`, `AVIFIL32` and three
     named `d3drm.dll` entries), and no text-format data file references an
     `.x` path. The lone `MiscAnims` string reference (`0x4a3778`, used at
     `0x421ec5` in `FUN_0041fb50`) is *not* an XOF load: `FUN_004653a0`
     loops building `"%s_%s_%d.bmp"` candidates for smoke textures — which
     is exactly `MiscAnims\Smoke\SMOKE0-2.BMP` (all three present in the
     wad). Verdict: editor-side leftovers shipped by accident; there is
     nothing to implement.
   * D3DRM — used only through `Direct3DRMCreate` + two vector helpers via
     `FUN_004781d0` (finding 15); no runtime question left beyond compat
     behaviour, covered by findings 15/16.

## 5b. On-disk SURF layout — confirmed during rrlwo.py development (2026-10-20)

The converter (`tools/rrlwo.py`) parsed all 1,132 shipped models through this
layout with zero failures, pinning §3's "keyword parser" down to concrete
bytes:

* `SURF` records arrive **one per `SURF` chunk** (not one chunk of many),
  each = even-padded surface name + sub-blocks.
* Sub-blocks = 4-char tag + **u16 big-endian** size + payload (padded to
  even), not the 1-byte size a naive LWO-5 reading would assume.
* Vocabulary observed: `COLR` (u8 r,g,b,a), `FLAG`, `DIFF`, `VDIF`, `CTEX`
  (text, e.g. `Planar Image Map`), `TIMG` (path ending in a wad basename),
  `TWRP`, `TFLG`, `TSIZ` (3×f32), `TCTR` (3×f32), `TAAS`, `TCLR`.
* Polygon-index base: `POLS` is 1-based as spec'd, **but four shipped files
  index from zero** (`MiscAnims/BirdScarer/1.lwo`, `MiscAnims/Effects/
  BEAM.LWO`, `Creatures/Spider/M_SFang2.lwo`,
  `Buildings/Powerstation/Connecting_Pipe.lwo`) — detectable since 1-based
  forbids index 0; the converter auto-detects per file.


### Function map (this finding)

| Address | Name (working) | Role |
| --- | --- | --- |
| `0x4809e0` | LoadModel | entity model entry |
| `0x48d7c0` | CreateModelObject | model struct (0x20 B) |
| `0x48d2c0` | LoadLWO | `.lwo` container walk + `.uv` sidecar |
| `0x48c0c0` | LwoStringDuplicate | allocates and copies a NUL-terminated chunk/surface string |
| `0x48c1d0` | LwoReadBigEndianFloat | reads a four-byte value and reverses byte order |
| `0x48c210` | LwoPoints | PNTS handler |
| `0x48c3e0` | LwoPolygons | POLS handler |
| `0x48c690` | LwoSurfaceNames | SRFS handler |
| `0x48c820` | LwoSurfaceSettings | SURF handler (COLR/TIMG/keywords) |
| `0x48c360` | LwoCurves | CRVS handler (discard) |
| `0x4869f0` | LoadLWSCScene | `.lws` text parser |
| `0x4765c0` | AssembleSceneAnimation | scene/anim worker |
| `0x47f6e0` | FOpenWadOrDisk | file open: FS or WAD, `"rb"` |
| `0x47f980` | FRead | chunk reader |
| `0x47fb60` | FLoadCheck | existence check / load-into-memory helper (used for `.uv` open) |
| `0x480ac0` | MaterialPass | mesh/material pass — sole consumer of the `.uv` stream |
| `0x481c50` | ParseUV | `.uv` line-stream parser (finding 19) |
| `0x4885c0` | ModelResourceResolveOrLoad | cache-aware scene/model resource resolver |
| `0x480050` | ReadCrlfLine | CRLF line reader (`buf, size, stream`) |
| `0x4774b0` | WhitespaceTokenize | whitespace tokenizer, returns token count |
| `0x48d7c0` / `0x48d870` | ModelObjectCreate / ModelObjectFree | allocate/load and release the model plus optional `.uv` stream |
| `0x48dc80` | GrowHeapArray | UV seam vertex-array growth |
| `0x48eae0` / `0x48f460` | FileResourceHandleClose / FileResourceHandleSeek | buffered/mapped resource handle operations |
| `0x48dbd0` / `0x48db80` | MemAlloc / MemFree | heap |
