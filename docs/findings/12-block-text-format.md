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
`*` in data-side names is a **prefix-wildcard matched against the query side**:
every lookup path begins with the runtime *exe basename* (`"LegoRR"`), so a
record named `Lego*` answers to `LegoRR …` queries without the name being baked
into data (§4).

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
splits the query on `' '` (`FUN_00477440`, delimiter `DAT_004aab18`) and scans
the record chain (`+0x14` link). The target is a record whose depth field
(`record+0xc`, root records 0) equals `segmentCount − 1` and whose name (`+4`)
matches the **last** segment; the parent chain (`+0x18`) then verifies the
remaining segments top-down (segment *i* ↔ depth *i − 1*). All segment
comparisons are `__strcmpi` — **ASCII case-insensitive**; the first segment
additionally becomes a `__strnicmp` prefix test when the *record* name contains
`'*'` (an exact match anywhere beats the last prefix-matched fallback). Callers
build these paths with `"%s%s%s"`/`"%s%s%s%s%s"` (`0x4a4024`/`0x4aab04`),
always rooted with the exe-name token (§5.1).

## 3. The query API

| Address | Working name | Semantics (evidence) |
| --- | --- | --- |
| `0x4792e0` | BlockFindPath | token-path walk → record or 0 |
| `0x4790b0` | BlockGetValue | `record+8` = value token string |
| `0x478ff0` | BlockSubgroup | validates `record+0xc < *(+0x14+0xc)`, returns child record |
| `0x478f50` | ConfigEntryFind / path builder | joins the root and supplied path components with `::` into static `DAT_00506098`; actual record lookup is performed by `0x4792e0` |
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
| `Lego.cfg` | `LevelLoad` config keys, reward tables, menus | finding 07/09; queries via the exe-name root token (`DAT_005560c4` = `"LegoRR"`, §5.1) |
| `.ae` entity files | entity assembly `FUN_00472cd0` | ext string `"ae"` `0x4a19ce`; path format `%s\%s.%s` `0x4a19b8`; keys below |
| `.ptl` level scripts | level logic via same API | keys as config groups |
| `.ol` object lists | level load | `OListFile` key in `Lego.cfg` (e.g. `M01.ol`) |
| `.npl/.nrn` | navigation | `NERPFile` key in `Lego.cfg` |

### `.ae` key vocabulary (entity animation/events)

From `FUN_00472cd0` (body `0x472cd0–0x473334`), constants at `0x4aaabc–0x4aab1c`:
`FRAME`, `SCALE`, `SAMPLE`, `LOOPING`, `LWSFILE`, `TRIGGER`, `TRANSCOEF`, `FILE`,
`Activities`, plus `%s.%s`, `%s_%s`, `ActFrame` and the `"::"` path joiner
`DAT_004aab18` (bytes `3a 3a 00`). Flow per entity (`src/gods98/entity.cpp`):

1. Opening: `FUN_00472cd0` is the ACT branch (kind 4) of the entity-visual
   dispatch — `FUN_00475930` matches the kind word case-insensitively against
   the 9-slot table initialized by `FUN_00472710`:
   `{NULL, MESH, FRAME, ANIM, ACT, -, -, LWS, LWO}` (indices 0–8; slots 5/6
   are zero and never match); an optional `:NOTEXTURE` suffix sets the
   no-texture flag for the mesh kinds. All 13 call sites pass an extended
   kind (`"MESH"`, `"LWO"`, `"LWS"`, `"ANIM"`, `"ACT"`, `"LWO:NOTEXTURE"`).
2. The `.ae` path is `<name>\<basename(name)>.ae`: the name argument is
   copied, split on `'\'` (`FUN_00477440`, delimiter `DAT_004a19c8`), and the
   last token is re-joined with `"%s\%s.%s"` (`0x4a19b8`) + ext `"ae"`
   (`0x4a19c4`). `BlockOpen` (`FUN_00478e60`) parses it.
3. The record is allocated (`FUN_00472950`, result `+0x10 = 4`), then the
   activity list is read from the `<exe-base> Activities` subgroup (§5.1),
   iterated in document order (`FUN_00478ff0` + sibling chain).
4. Each activity record contributes a row keyed on its **value** (the target
   animation group name, `record+8`); per-row keys are read under
   `<root>::<group>::<KEY>`:
   `FILE` (animation/scene base name — without it the activity makes no
   scene and is omitted), `TRANSCOEF` (float via `FUN_0048d980`, default
   `DAT_004b8a14` = 0), `TRIGGER` (int via `FUN_0048dad0`, default 0),
   `LWSFILE` and `LOOPING` (shared bool `FUN_00477710`: `{YES,TRUE,ON}→1`,
   `{NO,FALSE,OFF}→0`, else 2 — the scene flag is `LWSFILE == TRUE` and
   `LOOPING != 0`), `SAMPLE` (string, `FUN_00479050`).
5. A leading `!` on the activity **key** (`record+4`) gates the row on the
   engine feature word `DAT_00505f84`: when bit `0x400000` is **set** the
   activity is skipped; when **clear** (the shipped default) it is processed
   with the `!` stripped. The bit is the `-reduceanimation` standard
   parameter (`FUN_00477bf0`, string `-reduceanimation` at `0x4aad04`) —
   reduced-animation mode drops these animation states.
6. Root-level `SCALE` is read as a float (`<root>::Scale`); a non-zero value
   is applied through the record vtable (`+0x40`). Scene assembly
   (`FUN_00475d10`, `FUN_004765c0`, first-row registration `FUN_00473370`)
   is the downstream seam (§4.3 below) — the ported module ends at the parsed table.

Corpus health: every shipped `.ae` (56/56, all in `LegoRR0.wad`) has exactly the
grammar above, 439 rows across the 56 files under the default feature word,
and the reduce-animation variant drops to 354 — both digest-verified by
meson suite `entity_ae` (`--real`).

### `.lws`/scene side (§4.3)

Scene assembly (`FUN_00475d10`, `FUN_004765c0`) resolves objects through the
vtable DB (`DAT_00505914 + 0x10`) and reads `ActFrame` entries `%s_%s` — the
linking between entity activities and `.lws` scene frames (finding 11 §4).

## 5. Notes and open items

1. **`FUN_004792e0` walker — RESOLVED (2026-10-20).** Matching rules are as
   described in §2: depth-matched candidate scan over the `+0x14` chain, last
   segment by name, ancestors verified up the `+0x18` parent chain, ASCII case
   fold throughout, `name*` prefix wildcard gated to segment index 1. The root
   token that prefixes every caller-built path is `DAT_0076ae20` =
   `DAT_005560c4`, which `FUN_0041fa20` pins to `&DAT_00505814` — the command
   line cut after the last `'\'` with the extension stripped
   (`GetCommandLineA` at `0x4777a0`), i.e. the **exe basename `LegoRR`** — the
   same string the resource layer mounts wads under (finding 20). Data roots
   spelled `Lego*` therefore answer to `LegoRR …` queries by prefix; files may
   carry sibling roots for other query tokens, e.g. `LegoLow*` in
   `IceMonster.ae`/`LavaMonster.ae` (confirmed depth-0 sibling).
   The reconstruction (`src/gods98/block.cpp`) implements these semantics and
   is corpus-validated: 138/138 shipped block-text files, 12,876 records,
   digest-identical to an independent token model (meson suite `block`).
2. `TRUE/FALSE`, float, and list parsing are per-callsite; no central typed
   getter — every consumer casts via `FUN_0048d980` etc.
   `FUN_00477710` is the one shared boolean parser: `{YES,TRUE,ON}→1`,
   `{NO,FALSE,OFF}→0`, anything else `2`, strings at `0x4aabe0–0x4a4124`.
3. Value token limits: fixed 512-B buffers for formatted key paths (visible as
   `local_200/400/…` buffers); keys longer than that would silently truncate.
4. DAT_005343ac is the optional loader-progress callback (finding 118): the whole-file loaders pass
   the measured size before reading, and the callback accumulates it into the active LoaderProfile
   category. The denominator unit and bytes successfully consumed remain unknown.

### Function map

| Address | Name (working) |
| --- | --- |
| `0x4800c0` / `0x4800a0` | ReadFileToMemory |
| `0x478e60` | BlockOpen (comment-strip + index + depth) |
| `0x479270` / `0x479490` | record alloc / pool grow |
| `0x4792e0` | BlockFindPath |
| `0x4790b0` / `0x478ff0` / `0x478f50` | value / subgroup / `::`-delimited path builder |
| `0x472cd0` | AssembleEntityConfig (`.ae` consumer; ACT branch ported as `src/gods98/entity.cpp`) |
| `0x472710` | kind-table init (sets `DAT_0076add8–adf8`, `DAT_0076ae00/04/08/0c/20`) |
| `0x475930` | kind dispatch (`FUN_00475930`: `:`-split, NOTEXTURE flag, 9-slot table) |
| `0x477440` | token splitter (`FUN_00477440`: delimiter string, last-segment out) |
| `0x475d10` / `0x4765c0` | scene animation assembly |
