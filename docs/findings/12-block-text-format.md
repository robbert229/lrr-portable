# 12 — Block-Text Data Files: the `Lego*` Format

*Target: `LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), Ghidra 12.1.4 via REA.*

## Summary

`Lego* { … }` is the engine's **one text-data language** — a brace-nested,
whitespace-separated block format used by `Lego.cfg` (the global config, finding
07), the entity animation/event files (`.ae`), level scripts (`.ptl`), and
family formats (`.ol`, `.npl`, `.nrn`, …). There is no bespoke parser per file
type: **one generic scanner + a path-lookup API** serve all of them.
The magic token `Lego*` is *data*, not a signature: the scanner never checks it
(no string or byte occurrence of `Lego*` exists in the image) — it simply parses
`name { … }` blocks, so the first token is just whatever the file starts with.

## 1. Load path

```
BlockOpen  FUN_00478e60(path)          (231 B)
  └─ ReadFileToMemory  FUN_004800a0 → FUN_004800c0 (169 B)
       ├─ FOpen  FUN_0047f6e0(path, "rb")     ← disk **or** WAD archive layer
       ├─ seek-end / size / alloc / rewind / read-all / close
       └─ optional tap hook via DAT_005343ac / DAT_005343b0 callback
```

`FUN_004800c0` reads the whole file into one heap buffer; mode `"rb"` at
`DAT_004a2770`. Since the open goes through the same `WAD Load/Fail` layer as
`.lwo` files (finding 11 §2), block files resolve transparently from the `.wad`
archives or from loose directories.

## 2. The scanner — flat index + depth counter (`FUN_00478e60` body)

No recursion and no syntax tree. The buffer is indexed **in place**:

1. Pass 1: walk bytes; `;` starts a line comment (cleared until `\n`); every
   ` \t\r\n` becomes `\0`, turning the file into a token stream.
2. Pass 2: chain tokens into 32-byte records (pool allocator `FUN_00479490`,
   blocks of `1<<n * 0x20`, links at `+0x1c`). A bare `{` token increments the
   block-depth field (`record+0xc`), a bare `}` decrements it and closes the
   record — i.e. nesting is tracked as a single integer per record, not a tree.

Lookup is **path-based on the token stream**: `FUN_004792e0(handle, "A B C")`
walks tokens matching each whitespace-separated key segment in order at the
current depth. Callers build these paths with `"%s%s%s%s%s"` (`0x4aab04`) by
space-wrapping tokens.

## 3. The query API

| Address | Working name | Semantics (evidence) |
| --- | --- | --- |
| `0x4792e0` | BlockFindPath | token-path walk → record or 0 |
| `0x4790b0` | BlockGetValue | `record+8` = value token string |
| `0x478ff0` | BlockSubgroup | validates `record+0xc < *(+0x14+0xc)`, returns child record |
| `0x478f50` | BlockQueryDefault | copies `param_1` into static `DAT_00506098`, returns it when no lookup hits |
| `0x479050` | (typed variant) | used alongside the two above in level-load code (finding 07) |
| `0x48d980` | StrToFloat | value → float |
| `0x477440` | StrMatch | comparison helper over config strings |

Value grammar is per-key: bare words, numbers, comma lists (`"R:x,y,z,w"`
rectangles, `"%d,%d"` pairs), `TRUE/FALSE`, and pipe-delimited
`file.bmp|x|y` triples (findings 09, 11).

A second, vtable'd view of the same data exists at `DAT_00505914` (methods used
by scene/animation assembly in `FUN_00475d10`/`FUN_00472950`): `+0x10` = find by
name (returns 0 on success, fills a record buffer), `+0x1c` = look up a slot by
handle (returns existing record or null), `+0x18` = store/add. The instance is
built lazily from block queries — the find takes a name and yields the record
handle. (The object storage reads as zeros and its vtable has no xrefs, so the
method roles are inferred from the three call sites above, not from a
constructor.)

## 4. Who reads what

| File | Consumer | Evidence |
| --- | --- | --- |
| `Lego.cfg` | `LevelLoad` config-DB (`DAT_005560c4`), reward tables, menus | finding 07/09 |
| `.ae` entity files | entity assembly `FUN_00472cd0` | ext string `"ae"` `0x4a19ce`; path format `%s\%s.%s` `0x4a19b8`; keys below |
| `.ptl` level scripts | level logic via same API | keys as config groups |
| `.ol` object lists | level load | `OListFile` key in `Lego.cfg` (e.g. `M01.ol`) |
| `.npl/.nrn` | navigation | `NERPFile` key in `Lego.cfg` |

### `.ae` key vocabulary (entity animation/events)

From `FUN_00472cd0` (body `0x472cd0–0x473334`), constants at `0x4aaabc–0x4aab1c`:
`FRAME`, `SCALE`, `SAMPLE`, `LOOPING`, `LWSFILE`, `TRIGGER`, `TRANSCOEF`, `FILE`,
`Activities`, plus `%s.%s`, `%s_%s`, `ActFrame`. Flow per object:

1. Build `<dir>\<name>.ae` (`%s\%s.%s` + ext `"ae"`), `BlockOpen`.
2. Read `Activities` subgroup; iterate activity records (`FUN_00478ff0`, chain
   via `+4`).
3. For each activity `X`: read `X … FILE` (animation name),
   `X … TRANSCOEF` (float via `FUN_0048d980`, default `DAT_004b8a14`),
   `X … LWSFILE`, `TRIGGER`, `SCALE`, `LOOPING`.
4. A leading `!` on an activity name makes it conditional on debug flag
   `DAT_00505f84 & 0x400000` (the flag masks the `!` away otherwise) —
   developer switches shipped in data files.
5. Result record `+0x10 = 4`; downstream: model load (finding 11 §1) and scene
   assembly.

### `.lws`/scene side

Scene assembly (`FUN_00475d10`, `FUN_004765c0`) resolves objects through the
vtable DB (`DAT_00505914 + 0x10`) and reads `ActFrame` entries `%s_%s` — the
linking between entity activities and `.lws` scene frames (finding 11 §4).

## 5. Notes and open items

1. **`FUN_004792e0` walker** — exact matching rules per path segment (case,
   partials, duplicates) need one focused pass.
2. `TRUE/FALSE`, float, and list parsing are per-callsite; no central typed
   getter — every consumer casts via `FUN_0048d980` etc.
3. Value token limits: fixed 512-B buffers for formatted key paths (visible as
   `local_200/400/…` buffers); keys longer than that would silently truncate.
4. `DAT_005343ac` read-tap callback suggests a file-change hook (mod/patch or
   debug); origin not yet identified.

### Function map

| Address | Name (working) |
| --- | --- |
| `0x4800c0` / `0x4800a0` | ReadFileToMemory |
| `0x478e60` | BlockOpen (comment-strip + index + depth) |
| `0x479270` / `0x479490` | record alloc / pool grow |
| `0x4792e0` | BlockFindPath |
| `0x4790b0` / `0x478ff0` / `0x478f50` | value / subgroup / default-get |
| `0x472cd0` | AssembleEntityConfig (`.ae` consumer) |
| `0x475d10` / `0x4765c0` | scene animation assembly |
