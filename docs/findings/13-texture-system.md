# 13 — Textures: `.bmp` Loading, the Name Cache, and World Tile Sets

*Target: `LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), Ghidra 12.1.4 via REA.*

## Summary

Textures are plain Windows `.bmp` files. Every pixel surface the game draws
goes through **one** create path (`FUN_00474050`, "BMP → DDraw/D3D surface") and
**one** name-keyed reuse path (`FUN_00482e70`, "get-or-load"). Files are fetched
by the same disk-or-WAD layer that serves `.lwo`/`.cfg` (finding 11 §2, finding
12 §1) — textures inside the `.wad` archives load transparently. Four feeders
populate the system: LWO material binding at model open, the per-level
`Textures { … }` world-tile sets, shared preload, and UI/numbered bitmaps.
Animated content is expressed as *filename sequences*, not formats:
`%s%0.1x%0.1x.bmp` (`0x4a1da8`), `%s%0.1x%0.1xa/b.bmp` (`0x4a1d88`/`0x4a1d98`
region), `%s%i.bmp` / `%i.bmp` (`0x4a1d7c` / `0x4a1d74`).

## 1. Create path — `FUN_00474050` (BMP → surface, 951 B)

```
FUN_00474050(path, flags…)                    @ 0x474050
  ├─ ReadFileToMemory  FUN_004800a0            ← disk or WAD (finding 12 §1)
  ├─ FUN_00489c30        header/format parse into record (263 B)
  ├─ FUN_00474410        numeric/ctype helpers
  ├─ DAT_00505918 vtable + 0x9c   device texture-capacity query (see §4)
  ├─ DAT_0076ac84 vtable + 0x18   CreateSurface/CreateTexture call
  └─ upload via surface object methods
```

Two **device wrapper objects** own all texture creation: `DAT_00505918`
(capacity/queries, also used at `FUN_0047cd50`) and `DAT_0076ac84`
(surface factory). The loader builds `DDSURFACEDESC`-shaped structures
(size `0x7c`, required flags `0x1007`, plus usage `0x10` or `0x5000` — the
choice is gated by debug bit `DAT_00505f84 & 0x20`: with the bit set, `local_8c[0] = 0x10`,
otherwise `local_90 = 0x5000`). `FUN_004744f0` (542 B) is the named lookup
variant of the same pipeline: it consults the vtable config DB
(`DAT_00505914 + 0x5c / + 0x60`) before creating — used by the tile manager (§3).

## 2. Cache — `FUN_00482e70` (get-or-load, 435 B)

Called by exactly three sites (§3, §4, §5). Logic, from pseudocode:

1. Build name: `FUN_0048d9e0(buf, "%s%s" @ DAT_004a50d4, base, file)` (or
   plain copy when `param_1 == 0`).
2. `FUN_00483030(&DAT_0053455c, DAT_0054055c, name)` — **linear `__strcmpi`
   scan of a 12-byte-entry table** (`+0x4` name ptr, record at `+0x0`, size at
   `+0x8`); a hit `(**(record)+4)(record)` increments the refcount and returns.
3. Miss → `FUN_00474050`. On failure, retry through the **secondary table**
   `&DAT_0053a55c / DAT_00540560` (shared textures), then give up.
4. Success → `FUN_00483080` registers `{strdup(name), record, size}` into the
   table (xrefs to both tables exist only inside `FUN_00482e70` — it owns them).

## 3. LWO material binding (resolves finding 11 open item)

The model-open orchestrator `FUN_004809e0` does parsing (`ModelObjectCreate @ 0x48d7c0`) and
then the **material pass** `FUN_00480ac0` (3.4 KB). Its helper `FUN_00481820`
(669 B) is the binder:

* For each material/vertex entry it calls `FUN_00490090` (material lookup,
  369 B — also called after LWO parse in `ModelObjectCreate` and from
  `FUN_00483c80`), then joins `"%s%s"` (`DAT_004a50d4`) into a path and calls
  **`FUN_00482e70`** — so LWO surface texture names become `.bmp` loads/caches
  at model-open time, per material.
* UV/blend bytes decode as `byte * 0.00390625` (÷255) into floats; the `*1.0−x`
  flip for `uVar8 != 0` in `FUN_0040b5f0` mirrors textures on the second slot.
* Untextured geometry falls back to the stub name: `"NOTEXTURE"` (`0x4aab78`)
  / `"LWO:NOTEXTURE"` (`0x4a1dbc`). Entity registration `FUN_0040b5f0` passes
  `LWO:NOTEXTURE` as the texture override for every instance it builds.

## 4. World tile sets — `Textures { Rock/Lava/Ice }`

Config side (from `Lego.cfg`, block-text API of finding 12):

From `Lego.cfg` (~line 1884, tab-indented; `Lava` and `Ice` follow the same
keys):
```
	Textures {

		Rock {
			rooftexture				World\WorldTextures\rockroof.bmp
			surftextwidth			8
			surftextheight			8
			meshbasename			World\ProMeshes\RockPM
			texturebasename			World\WorldTextures\RockSplit\Rock
		}
		…Lava / Ice follow the same keys…
	}
```

Keys: `rooftexture`, `surftextwidth`, `surftextheight`, `meshbasename`,
`texturebasename` (`Lego.cfg` `Textures` block, ~line 1884; key literals also
at `0x4a44c4–0x4a4758`).
Levels select one via key `TextureSet` (`Textures::Rock` style, `::` = block
path). Flow, inside the (very large) init/level routine containing
`FUN_0041fb50…` — `LevelLoad (FUN_00429890) → FUN_0042b510 → FUN_0042bb60`
(442 B, reads the four keys above through `FUN_00478f50`/`FUN_004790b0`):

* `FUN_0040b310` (245 B) builds each tile name **`%s%0.1x%0.1x.bmp`** and
  creates the surface via `FUN_004744f0` — with `surftextwidth/height = 8`,
  that is 64 `Rock00.bmp … Rock77.bmp` tiles per set.
* `FUN_0042b330` (329 B) builds **`%s%0.1x%0.1xa.bmp` / `…b.bmp`** — a *second
  pair of variants per tile*, then instantiates via the entity path
  `FUN_0040b5f0` (which overrides with `LWO:NOTEXTURE`, i.e. the tile geometry
  carries only UVs/colors; the BMP comes from §3 binding).
* `FUN_00462e60` (111 B) updates an animated tile/texture entry:
  `FUN_004744f0` (get), `FUN_00474710`/`FUN_00474a20`/`FUN_00474da0`
  (lock/update/unlock-style calls), driven per frame from `FUN_0042bb60`'s
  caller `FUN_0042b510`.
* `meshbasename` points at progressive meshes (the `textureset`/`rooftexture`/
  `dither` string cluster at `0x4a44c4–0x4a4758` belongs to this module); the
  dither option itself is `Dither TRUE` in `Lego.cfg Main` (`0x4a3820` area).
* The `rooftexture` single bitmap + `Lego_SurfaceType_* → <material>,SurfaceSFX_*`
  table (`Lego.cfg` `SurfaceTypeDescriptions`, `0x4a3d20+`) map gameplay
  surface types onto this visual/material vocabulary.

## 5. VRAM budget — `TextureUsage`

In the big init function (`FUN_0041fb50`, ~36 k pseudocode units), the config
key `TextureUsage` (`0x4a3bb8`; shipped value `3145728` = 3 MiB) is read as an
int. `FUN_0047cd50` (127 B) queries device `DAT_00505918 + 0x9c` for texture
memory; if the device's number satisfies the comparison, `FUN_00477bd0` (20 B)
sets runtime flag `DAT_00505f84 |= 0x20` (unless debug bit `0x40000` is on).
Bit `0x20` then feeds back into the create path's usage flags (§1) — the
budget automatically downgrades texture placement mode on small cards.

## 6. Shared preload — `SharedTextures` / `SharedObjects`

`Main` keys `SharedTextures World\Shared` and `SharedObjects World\Shared`
(`0x4a3a18` / `0x4a3a28`). At init: `SharedTextures` value is copied by
`SharedTextureDirectorySet @ 0x4728c0` and registered via
`SharedTextureRootRegister @ 0x4805b0` (which initializes it through
`SharedTextureRootInitialize @ 0x480650` and releases the previous entry through
`FUN_00482d30`).
`SharedObjects` drives `FUN_004876c0(dir, FUN_00464c70, FUN_00465090,
FUN_00465370)` — three directory-scan callbacks; the texture-relevant one
reaches `FUN_004653a0` (136 B), which loops index `i` loading
**`<base>%i.bmp`** into a global slot table `DAT_00552100` (count
`DAT_00552130`). Numbered UI bitmaps `FUN_0040a400` (`%i.bmp` / `%s%i.bmp` →
`FUN_00482e70`) use the same naming trick from the init function.

### Function map

| Address | Working name | Evidence |
| --- | --- | --- |
| `0x474050` | CreateBmpSurface | `DAT_0076ac84+0x18` create, 0x7c desc |
| `0x474410` | ParseIndexedSurfaceName | parses the `a` + three-hex-digit indexed surface suffix |
| `0x4744f0` | NamedSurfaceGet | config-DB vtable +0x5c/+0x60 |
| `0x482e70` | TextureGetOrCreate | owns caches `0x53455c/0x53a55c` |
| `0x483030` / `0x483080` | TextureCacheLookup / TextureCacheInsert | `__strcmpi`, `strdup` |
| `0x481820` | BindModelTextures | inside `FUN_00480ac0`; `%s%s`→`FUN_00482e70` |
| `0x480ac0` | MaterialPass | `ModelOpen` helper (3.4 KB) |
| `0x490090` | AssetPathComponentsParse | Splits path prefix, directory, basename, and extension; called by model creation, model texture binding, and FLIC open. |
| `0x42bb60` | TextureSetManager | keys `rooftexture`… |
| `0x40b310` | LoadTileVariants | `%s%0.1x%0.1x.bmp` |
| `0x42b330` | LoadTileMeshes | `%s%0.1x%0.1xa/b.bmp` → entity reg |
| `0x40b5f0` | TileModelEntityRegister | registers a tile mesh entity and sets the `LWO:NOTEXTURE` override |
| `0x462e60` | TexAnimUpdate | lock/update/unlock |
| `0x47cd50` | QueryTextureMem | `DAT_00505918+0x9c` |
| `0x477bd0` | SetLowTextureFlag | `DAT_00505f84 \|= 0x20` |
| `0x4653a0` | PreloadSharedIndexed | `%s%i.bmp` → `DAT_00552100` |
| `0x40a400` | PreloadNumberedBitmaps | loads numbered UI/indexed BMP names through the shared cache |
| `0x48c820` | LwoSurfaceSettings | reads per-surface settings and texture-image tags |
| `0x48d2c0` | LoadLWO | reads the LWO chunks and optional UV sidecar |
| `0x481c50` | ParseUV | applies the model's UV sidecar stream |
| `0x490090` | AssetPathComponentsParse | splits path components for model/material asset names; it does not resolve a material |

## Notes and open items

1. Exact `DAT_00505918`/`DAT_0076ac84` vtables are not materialized as objects
   (runtime-initialized, no xrefs); their roles come from the call sites
   quoted above, and the `+0x9c` semantics (available vs used texture memory)
   from the `TextureUsage` comparison — both are **inferences**, flagged.
2. `FUN_0047cd50`'s writes land in locals (`*local_20[1] = (ptr>>3) * count`)
   — its parameter appears to be an out/out-in for the probe; not fully
   reconstructed.
3. Whether tile variants `a`/`b` are blend-on-seam or day/night variants is
   not answerable from code alone; the file layout would settle it, but the
   binary asset trees were not unpacked in this workspace.
4. Palette/dither handling: the `Dither` key exists, and `FUN_00474050` picks
   usage flags, but the actual palette-conversion routine was not separated
   out (`FUN_00489c30` parse + upload is the whole pre-upload chain observed).
5. `.uv` sidecars (finding 11) bind by *mesh* name; nothing in this pass
   contradicts that. The `%s%0.1x%0.1x` scheme is world-tile-only.
