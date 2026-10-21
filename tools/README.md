# `tools/` — game-data tooling

## `rrlwo.py` — `.lwo` (LightWave 4) → glTF 2.0 converter

Reads the game's 1,132 embedded models (`\Models\…` inside `LegoRR0.wad`)
straight from the archives and exports `.glb` — geometry, materials with
colours, and textures pulled from the same WADs. All format semantics cite
[`docs/findings/11-model-and-scene-formats.md`](../docs/findings/11-model-and-scene-formats.md);
the conversion decisions worth knowing:

* **UVs (planar)** — LWOB stores none; the `CTEX` block says the mapping is
  *planar*, so we project each face along its dominant normal axis and divide
  by the `TSIZ` scale, which is the engine's own scheme, not an invented one.
* **`.uv` sidecars** — the 60 models that ship one get the engine's real
  behaviour instead of the planar guess (see
  [`docs/findings/19-uv-grammar.md`](../docs/findings/19-uv-grammar.md)):
  each surface's texture path is overridden by the sidecar's path line, and
  per-corner UVs (stored `v`, negated as the engine stores it) replace the
  projection in POLS ring order. Shared vertices needing different uv
  duplicate automatically through the keyed-vertex table — the same seam
  break the engine performs. Name lines and the trailing keyframe blocks are
  ignored, exactly like the engine's parser; `--no-uv` opts out.
* **Normals** — flat, so shared positions do not share vertices across
  faces (glTF has no face normals).

Lookups resolve by **full path first** (`list` prints full paths, and `-n`
takes them back); the engine's own path layer works the same way. Sidecars
only ever resolve exactly — `.uv` belongs to the model beside it.

```
python3 tools/rrlwo.py list                          # full paths of models in the wads
python3 tools/rrlwo.py dump "Creatures\LavaMonster\Arml.lwo"   # incl. uv summary
python3 tools/rrlwo.py gltf "Buildings\Barracks\Base.lwo" -o base.glb
python3 tools/rrlwo.py gltf PATH.lwo -o out.glb --wad-dir mod/L1   # modded trees
python3 tools/rrlwo.py gltf -n NAME.lwo -o out.glb --no-uv        # ignore sidecars
```

Conversion notes found the hard way (documented in finding 11 §3 terms):
polygon indices in `POLS` are 1-based per spec, but four exporters used
0-based — the tool auto-detects per file (a zero index is impossible
1-based). Negative indices mean the polygon is wound backwards and get the
winding flipped. `SURF` blocks use tag + u16-BE size records anchored by the
SRFS surface names; BMP payloads convert to PNG with stdlib only.

Verified: 1,129/1,132 exported, with 3 `*/Steam.lwo` skipped as point-only
particle sprites (single-vertex `POLS` polygons, not fan geometry — a data
fact of those files, flagged explicitly since 2026-10-20) and all 60 `.uv`
sidecar models exported with their real per-corner UVs and overridden
texture paths applied. Structural checks (GLB framing, buffer lengths, index
ranges, PNG inflation) on every model, and a visual check on
`Buildings\Barracks\Base.glb` in a glTF viewer — flat-shaded meshes with
textures, as the design predicts (user, 2026-10-20).

> Correction note: the original 1,132/1,132 batch (2026-10-20, finding 11
> tooling) looked entries up **by basename**, which silently fed duplicate-
> basename models (`arml.lwo` exists under five creature folders, `Steam.lwo`
> under three) the *first* same-named file instead of their own. Exact-path
> resolution landed with the `.uv` pass; per-entry payloads are verified
> correct from then on.

## `rrwad.py` — the `.wad` (WWAD) container tool

Full unpack / pack / verify for `LegoRR0.wad` and `LegoRR1.wad`, per the
format documented in [`docs/findings/01-binary-and-loader-map.md`](../docs/findings/01-binary-and-loader-map.md)
(§ "WAD container format").

```
"WWAD" | u32 count | count*NUL names | count*NUL dev-paths | count*16B TOC | data
TOC entry: flags u32 | orig_len u32 | stored_len u32 | file_offset u32
```

Commands:

| Command | Purpose |
| --- | --- |
| `list WAD` | index listing (`# index size @offset name`) |
| `unpack WAD DEST` | extract a proper directory tree + hidden `.rrwad.json` manifest (names, dev-paths, flags, sizes, offsets) |
| `pack MANIFEST -o OUT` | rebuild the archive from the manifest + edited files |
| `verify WAD` | in-memory round-trip: every archive here packs back **byte-identical** |
| `extract WAD NAME...` | stream one entry to stdout (case-insensitive name, like the game) |

The manifest is what makes mods lossless: the dev-path strings (`\\ROBW\C\
Dev\SourceSafe\Lego\Data\…`) are kept verbatim even though the game ignores
them, so repacked archives are indistinguishable from the originals — except
for the data you changed, and payload offsets recomputed for new sizes.

Round-trip proof (run 2026-10-20, both shipped archives):

```
verify: OK — source/LegoRR0.wad round-trips byte-identical (5631 entries, 78,968,800 B)
verify: OK — source/LegoRR1.wad round-trips byte-identical (214 entries, 3,754,454 B)
```

Mod workflow:

```sh
python3 tools/rrwad.py unpack source/LegoRR1.wad mod/L1
# edit mod/L1/Lego.cfg (or any entry)
python3 tools/rrwad.py pack mod/L1/.rrwad.json -o mod/LegoRR1.wad
```

Notes:

* Nothing is compressed in the shipped files (`stored == original`, flags all
  `1`); the engine's dormant compression codecs stay untested (finding 01).
* Cross-wad shadowing (`LegoRR1` beats `LegoRR0`, scan order `9→0` per
  finding 01) is decided by mount order at runtime, not by entry order —
  repacking either archive never changes which file the game opens.
* Name lookups are case-insensitive `__strcmpi` with no collisions observed
  in either archive, so extracted trees are safe on case-insensitive
  filesystems.
* `unwad.py` is the earlier extract-only script from the finding-01 work;
  `rrwad.py` supersedes it (it also converts `foo\bar.bmp` names into real
  directories instead of literal backslash files).
