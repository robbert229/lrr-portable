# `tools/` — game-data tooling

## `match_function.py` — single-function match loop

Scores one function of the *matching build* against the shipped
`LegoRR.exe` in about a second, without a link or a full Reccmp run. It finds
the `// FUNCTION|LIBRARY: LEGORR 0x...` annotation for the address, compiles
only that translation unit with the exact matching-build command line (from a
cached CMake configure), and runs Reccmp's own entity database, matchers and
`FunctionComparator` over a synthetic image built from the object file. The
score is Reccmp's `matching` value.

```sh
make match-function ADDR=0x00478e60                 # score + aligned diff
python3 tools/match_function.py 0x478e60 --json     # machine-readable
python3 tools/match_function.py 0x478e60 0x4798b0   # several, compiled in parallel
python3 tools/match_function.py --source src/gods98/block.cpp   # every annotated function in a file
python3 tools/match_function.py 0x478e60 --side-by-side         # orig | recomp columns
python3 tools/match_function.py --warm              # pre-start Wine and cache the flags
```

`make match-function` accepts `ADDR=` (one or more), `SOURCE=`, `JSON=1` and
`MATCH_ARGS=` (any other option). Other options: `--no-diff` (scores only),
`--keep` (keep the object and compiler log), `--timings`, `--orig FILE`.

**Output.** Text mode prints the similarity, the bytes compared, the committed
score from the *match baseline* with the delta, and a Reccmp-style unified
diff. Original addresses are real; recomp addresses are offsets from the
function start (`+0x001f`), with `(file.cpp:line)` source references. `--json`
prints one document:

```json
{"timings": {"compile": 0.36, "total": 1.2, ...},
 "functions": [{"address": "0x478e60", "name": "block_open",
   "source": "src/gods98/block.cpp", "line": 218, "origin": "compiled",
   "matching": 0.2162, "percent": "21.62%", "effective": false,
   "baseline": "21.62", "below_baseline": false,
   "orig_size": 89, "recomp_size": 89, "diff": [...], "error": "..."}]}
```

`diff` has the same structure as the `diff` entries in Reccmp's
`results.json`. `origin` is `compiled`, or `LIBC.LIB(<member>)` for a
`LIBRARY` function that the link takes from the VC5 runtime rather than from
the reconstruction. `error` appears instead of a score when the function was
not found in the object (inlined, excluded by `#if`, renamed) or the file did
not compile (the compiler output is included). Exit status: 0 when every
function was scored, 1 when any was not, 2 for setup errors.

**Environment.** Wine runs directly when `wine` is on `PATH`, otherwise
through `distrobox enter developer` (`MATCH_FN_DISTROBOX=<name>`; empty
disables). `REPROBIT_MSVC_5_0_RTM_ROOT` selects the toolchain. The flags cache
lives in `build-match/cmake` (`--build-dir` or `MATCH_FN_BUILD_DIR`). Its
configure options are read from `tools/reccmp.sh`, and it is re-configured
when those options, `CMakeLists.txt` or the toolchain file change. Each
invocation compiles in its own `build-match/runs/run-*` directory, removed on
exit, and configuring takes a file lock, so concurrent runs in one worktree
or across worktrees share no build state. They do share Wine: the tool keeps
one wineserver for the `WINEPREFIX` alive for 15 minutes after its last use
(`MATCH_FN_WINESERVER_PERSIST=<seconds>`, `0` disables) and starts Wine's
services with detached stdio. The MSVC wrapper pipes Wine through `sed`, and
services started during a compile would otherwise hold that pipe open.

**Speed.** About 1.2–1.4 s per call with a warm wineserver, 1.8 s for the
largest file, 3 s fully cold; several addresses in different files compile
in parallel. Measurements are in
[finding 507](../docs/findings/507-single-function-match-loop.md).

**Parity.** On three source trees the score equals full Reccmp's for all but
one function (`__isindst`); finding 507 has the per-area counts and a
26-function spot-check table. `make match-function-parity`
(`match_function_parity.py`) repeats the comparison against a full
`results.json` from the same tree; run it after changing these tools.
`synthetic_image.py` builds the stand-in for the linked program:

* Relocations are applied in a synthetic layout at `0x0E401000`; undefined
  symbols get slots named the way the linker PDB would name them (CodeView
  names for project code, public symbols for LIBC), Win32 calls without
  `dllimport` go through `jmp [IAT]` thunks, and functions of the `program`
  target are reached through incremental-link (ILT) thunks, as in the
  `/INCREMENTAL` matching-build link.
* Calls into other translation units are matched to their `FUNCTION`
  annotation by name; other files' line-referenced annotations are named on
  the original side, as the full run would have matched them.
* A constant inside the linked program's address span is treated as an
  address, because the full run finds some entity there (e.g. the flag
  `0x800000` lands in a large `.bss` array). The span ends where an existing
  `build-reccmp/LegoRR.exe` (or `$RECCMP_BUILD_DIR`) ends, else at
  `0xA00000`; `MATCH_FN_LINK_END` overrides.

Known differences: variables have no CodeView type sizes, so offsets render
as `var+4` rather than `var[1]` (text only), and a `GLOBAL` annotation that the
full link matches but this translation unit never references has no size,
which changes one CRT score (`__isindst`, 98.64% here, 96.36% in full).
Functions in objects the link never pulls in (`ptrstack.cpp`,
`video_graph.cpp`) are scored here but absent from the full report. Before
handing work back, `make reccmp-check` remains the gate.

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
* **Round-trip** (`rrlwo.py uv`) — writes a `.uv` sidecar for a model. With
  the sidecar present it is kept verbatim where it matters (name lines,
  tail, corner values — a rebuild is byte-exact for all 60 shipped files);
  geometry edits get planar-synthesized uv blocks for changed rings, and the
  keyframe tail is preserved verbatim. `--no-uv` rebuilds purely from the
  model instead, which drops sidecar-only material names and regenerates
  the tail (documented lossy trade); a LWO TIMG edit is only reported when
  a stale sidecar path shadows it, so adopting it stays explicit.

Lookups resolve by **full path first** (`list` prints full paths, and `-n`
takes them back); the engine's own path layer works the same way. Sidecars
only ever resolve exactly — `.uv` belongs to the model beside it.

```
python3 tools/rrlwo.py list                          # full paths of models in the wads
python3 tools/rrlwo.py dump "Creatures\LavaMonster\Arml.lwo"   # incl. uv summary
python3 tools/rrlwo.py gltf "Buildings\Barracks\Base.lwo" -o base.glb
python3 tools/rrlwo.py gltf PATH.lwo -o out.glb --wad-dir mod/L1   # modded trees
python3 tools/rrlwo.py gltf -n NAME.lwo -o out.glb --no-uv        # ignore sidecars
python3 tools/rrlwo.py uv -n PATH.lwo -o out.uv                    # write/rebuild sidecar
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
verify: OK — reference/LegoRR0.wad round-trips byte-identical (5631 entries, 78,968,800 B)
verify: OK — reference/LegoRR1.wad round-trips byte-identical (214 entries, 3,754,454 B)
```

Mod workflow:

```sh
python3 tools/rrwad.py unpack reference/LegoRR1.wad mod/L1
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
