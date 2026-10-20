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
- `.uv` → companion text file with per-object UV/track data, read alongside `.lwo`.
- `.ae` → `Lego* { … }` block-text files; no magic/string references found in the
  executable (see §6 — likely consumed indirectly or editor-side).
- `.wav` / `.bmp` → standard formats, loaded via `%s.wav` (`0x4aae5c`) and the
  WAD/FS file layer (documentation of these is still open).

Internal developer path strings corroborate the pipeline: `WAD Load: %s`,
`WAD Fail: %s`, and `C:\Dev\SourceSafe\gods98\dx6\god…` (`gods98` — the
working title, in `0x4aaf74–0x4ab004`).

## 1. Model load chain

```
FUN_00472cd0 (entity creation)        FUN_004885c0
        └──────────────┬──────────────────────┘
                  FUN_004809e0            model entry
                       └─ FUN_0048d7c0    create 0x20-byte model header
                            ├─ FUN_0048c0c0   (header field +0)
                            └─ FUN_0048d2c0   LWO file loader
```

`FUN_0048d7c0` (`0x48d7c0`–`0x48d86b`) zero-allocates a 32-byte model struct and
hands out its fields to the loader: `+4` vertex array, `+0x10`/`+0x14` polygon
data, `+0x18` surface table, `+0x1c` debug/verbose flag. The loader is also the
only place the format string `%s.lwo` (`0x4afc98`) is referenced.

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
`%s.uv` (`0x4afc90`) for the same base name and parses it via `FUN_0047fb60` →
`FUN_0048eb90`. The unpacked `.uv` files are line-based text
(`2\r\n3\r\nHand R X…`), i.e. small named-channel tables; their field-level
grammar is not yet fully mapped (open item).

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

1. **`.uv` grammar** — parse rules inside `FUN_0048eb90`/`FUN_0048eae0`; the
   text layout (channel count header + named tracks) needs field-level docs.
2. **`SURF` field mapping** — the keyword parser is identified but the exact
   per-surface settings record (colour format, TIMG/BMP binding, sequence
   pairs) needs one more pass over `FUN_0048c820`.
3. **`Lego*` block reader** — shared between `.ae`, `.ptl`, and possibly
   config; find the generic reader (candidates: `FUN_004877e0`, `FUN_0048dc80`).
4. **Textures** — `TIMG` binding from surfaces to `.bmp` names, and the
   `LWO:NOTEXTURE` placeholder (`0x4a1dbc`, used from `0x40b623–0x40b656`).
5. `.wav`/`.x` loaders and the D3DRM dependency (`D3DRM.DLL`) remain open.

### Function map (this finding)

| Address | Name (working) | Role |
| --- | --- | --- |
| `0x4809e0` | LoadModel | entity model entry |
| `0x48d7c0` | CreateModelObject | model struct (0x20 B) |
| `0x48d2c0` | LoadLWO | `.lwo` container walk + `.uv` sidecar |
| `0x48c210` | LwoPoints | PNTS handler |
| `0x48c3e0` | LwoPolygons | POLS handler |
| `0x48c690` | LwoSurfaceNames | SRFS handler |
| `0x48c820` | LwoSurfaceSettings | SURF handler (COLR/TIMG/keywords) |
| `0x48c360` | LwoCurves | CRVS handler (discard) |
| `0x4869f0` | LoadLWSCScene | `.lws` text parser |
| `0x4765c0` | AssembleSceneAnimation | scene/anim worker |
| `0x47f6e0` | FOpenWadOrDisk | file open: FS or WAD, `"rb"` |
| `0x47f980` | FRead | chunk reader |
| `0x48dbd0` / `0x48db80` | MemAlloc / MemFree | heap |
