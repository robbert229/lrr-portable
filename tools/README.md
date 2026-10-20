# `tools/` — game-data tooling

## `rrlwo.py` — `.lwo` (LightWave 4) → glTF 2.0 converter

Reads the game's 1,132 embedded models (`\Models\…` inside `LegoRR0.wad`)
straight from the archives and exports `.glb` — geometry, materials with
colours, and planar-mapped textures pulled from the same WADs. All format
semantics cite [`docs/findings/11-model-and-scene-formats.md`](../docs/findings/11-model-and-scene-formats.md);
the two conversion decisions worth knowing:

* **UVs** — LWOB stores none; the `CTEX` block says the mapping is *planar*,
  so we project each face along its dominant normal axis and divide by the
  `TSIZ` scale, which is the engine's own scheme, not an invented one.
  Exception: the 60 models that ship a `.uv` sidecar get real per-corner UVs
  from it — see [`docs/findings/19-uv-grammar.md`](../docs/findings/19-uv-grammar.md)
  (the converter currently exports those models without sidecar data; the
  sidecar's surface-path overrides would change their textures, and the
  per-corner UVs replace the planar projection).
* **Normals** — flat, so shared positions do not share vertices across
  faces (glTF has no face normals).

```
python3 tools/rrlwo.py list                          # models in the shipped wads
python3 tools/rrlwo.py dump "Creatures\Spider.lwo"    # chunk/surface structure
python3 tools/rrlwo.py gltf "Buildings\Barracks\Base.lwo" -o base.glb
python3 tools/rrlwo.py gltf PATH.lwo -o out.glb --wad-dir mod/L1   # modded trees
```

Conversion notes found the hard way (documented in finding 11 §3 terms):
polygon indices in `POLS` are 1-based per spec, but four exporters used
0-based — the tool auto-detects per file (a zero index is impossible
1-based). Negative indices mean the polygon is wound backwards and get the
winding flipped. `SURF` blocks use tag + u16-BE size records anchored by the
SRFS surface names; BMP payloads convert to PNG with stdlib only.

Verified: full library 1,132/1,132 exported without errors (2026-10-20),
structural checks (GLB framing, buffer lengths, index ranges, PNG inflation)
on all of them, and a visual check on `Buildings\Barracks\Base.glb` in a
glTF viewer — flat-shaded meshes with textures, as the design predicts
(user, 2026-10-20).

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
