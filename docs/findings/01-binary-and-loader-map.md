# 01 — Resource Loading Map (`LegoRR.exe`)

**Target:** `source/LegoRR.exe` · SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`
**Analysis provider:** Ghidra 12.1.4 (REA) · 2,707 procedures · 2,381 strings

## Binary layout (observed)

| Segment | Start | End | Size |
| --- | --- | --- | --- |
| `.text` | `0x401000` | `0x49de00` | 640 KB |
| `.rdata` | `0x49e000` | `0x4a0c00` | 11 KB |
| `.data` | `0x4a1000` | `0x76ded4` | 2.9 MB |
| `.idata` | `0x76e000` | `0x76f200` | 4.5 KB |
| `.rsrc` | `0x770000` | `0x770c00` | 3 KB |

The 2.9 MB `.data` segment dominates the binary and contains the runtime wad/file tables described below. The game's data is statically linked into the image.

## Two file paths

`LegoRR.exe` resolves every game asset through a single dispatch routine. Strings at
`0x4aae7c` (`Data\%s`) and `0x4aaf50` (`%s%i.wad`) mark the two branches.

### Dispatcher `FUN_0047f6e0`

```
FUN_0047f6e0(name, mode)
  ├─ classify via FUN_0047fe30(name, mode)
  │    ├─ 0  → WAD branch      (log "WAD Load %s" / "WAD Fail %s")
  │    ├─ 1  → STD branch      (log "STD Load %s" / "STD Fail %s")
  │    └─ else → error: "Error in call to C:\Dev\SourceSafe\gods98\...\File Open"
  └─ allocate a 2-field handle via FUN_0047fed0(mode), store file handle at +4
```

*Classification (observed):* `FUN_0047fe30` returns `1` if the name's first byte is `w`/`W`,
otherwise it asks the filesystem whether the file exists (`FUN_0048bd50`, "not found" ⇒ `0`).
So loose `Data\...` files are the STD branch; the wad branch handles archive members.

### WAD branch — mount + open

* Mount (`FUN_0047f130`): tries `%s%i.wad` for `i = 0..9`, i.e. `LegoRR0.wad` … `LegoRR9.wad`.
  The shipped copy has 0 and 1. A failed CD-key check raises `CDMissing`; missing data raises
  `DataMissing` with an "install" message (`cd.key`, `delme.dat` marker files).
* Context table `DAT_00575500`: 16 slots, 36 bytes each (`&DAT_00575500 + i*0x24`).
  Slot `+0x18` = name pointer array, `+0x20` = name count, `+0x1C` = TOC pointer.
* Name lookup (`FUN_0048bda0`): linear `__strcmpi` over the slot's name array — case-insensitive.
* Open (`FUN_0048bf70(path, idx)`): if `idx != -1`, opens inside that wad; with `idx == -1`
  (the `DAT_004aaf00` sentinel `0xFFFFFFFF`) it scans wads `9 → 0`, so later wads shadow earlier ones.
* Entry read (`FUN_0048be40`):
  1. `FUN_0048bd30` allocates a runtime slot from a 16-entry pool at `DAT_005756a0` (stride 16).
  2. Entry flags: if `byte[0] & 2` the payload is expanded via `FUN_0049c7c0` (a small
     header format: 4-byte magic + type byte at [3] + byte-swapped length, then raw bytes at
     `payload+0x12`; types: 0 = memcpy, 1 = `FUN_0049c8e0`, 2 = `FUN_0049ca60`).
  3. Slot fields: `+0x0` data pointer (or expanded buffer), `+0x4` state=1, `+0x8` wad index, `+0xC` entry index.
* Writes into a wad are rejected: strings `Cannot write to a file stored in a Wad!` (`0x4ab060`)
  and `"fprintf" is unsupprted for wad files` (`0x4ab0c4`).

### STD branch

`FUN_0047fed0(1)` then `FUN_0048eb90(path, h)` → `FUN_0048eb60(…, 0x40)` → `FUN_00495000`
allocates a 32-byte handle block from a pool table (`DAT_0076cd98`, count `DAT_0076dda0`;
free-reuse check masks state byte at `+0xC` with `0x83`) and `FUN_00494e30` performs the read.

A related path builder `FUN_0047f660` composes the string `"%c:\%s\%s"` (string `0x4aaf64`)
using a one-byte drive letter stored at `DAT_005343b4` — this is how the game re-opens
files from the CD-ROM drive when the wad/`Data\` fallback triggers (a default of `A:\`
appears in `.rdata` right after the format string). So "STD" paths are
`<drive>:\Data\<name>` during CD fallback rather than relative paths.

## WAD container format (validated against the shipped archives)

Both `LegoRR0.wad` (78,968,800 B) and `LegoRR1.wad` (3,754,454 B) parse identically:

```
offset 0x00:  "WWAD"
offset 0x04:  uint32 entry_count          (L0: 5631, L1: 214)
offset 0x08:  entry_count × NUL-terminated archive-relative paths
             (e.g. "Buildings\Barracks\A000_ladder.BMP")
then:        entry_count × NUL-terminated UNC build paths
             (e.g. "\\ROBW\C\Dev\SourceSafe\Lego\Data\...") — dev-tree leftovers, not used at runtime
then:        entry_count × 16-byte TOC entries:
             +0x00 u32 flags   (observed all = 1)
             +0x04 u32 original (uncompressed) length
             +0x08 u32 stored length on disk
             +0x0C u32 absolute file offset
then:        data area, entries packed back-to-back, ascending by offset
```

Validation performed: `max(offset + stored_len) == filesize` for both archives; all entries
have `stored == original` (nothing shipped compressed); payload tags at the offsets are
`FORM`, `LWSC`, `BM6…/BM8…`, `MAP `, `BLK `, `RIFF`, `xof `, `Lego…`.

**A parser can be written from this table alone — no compression, no per-entry header, no
alignment padding.** Case-insensitive name match over the flat path table is the entire lookup.

## Confidence

| Claim | Status |
| --- | --- |
| Binary segments, procedure/string counts | observed (Ghidra) |
| Dispatcher control flow, strings, xrefs | observed (Ghidra) |
| WAD file layout, offsets, uncompressed payloads | observed (validated by parsing both archives) |
| Runtime slot/context table semantics | observed (Ghidra pseudocode) |
| Compression codecs `FUN_0049c8e0` / `FUN_0049ca60` | **dormant** — flag=1, stored=original everywhere; codecs never exercised |
| STD branch = plain `Data\` files | inferred from branch structure + `Data\%s` string |
| `gods98` / `ROBW` = original dev tree | inferred from string fragments |

## Next steps

- Trace `FUN_0048eb90` (STD/open) — done (see STD branch above); unpacking both archives
  with `tools/unwad.py` — done.
- Payload inventory after unpacking all 5,845 entries:

  | Ext | Count | Source |
  | --- | --- | --- |
  | `.bmp` | 2,523 | textures/sprites (also the `BM6/BM8` bitmap tags) |
  | `.lwo` | 1,132 | **LightWave 3D 5.x objects** (`FORM`) |
  | `.lws` | 750 | **LightWave scenes** (`LWSC`) |
  | `.wav` | 387 | sounds (`RIFF`), incl. streamed voices |
  | `.map` | 240 | terrain/level maps (custom) |
  | `.blx`/`.blk` | 229 | custom block assets |
  | `.x` | 164 | DirectX `xof` frame hierarchies |
  | `.txt` | 76 | localization/help (in `LegoRR1.wad` mostly) |
  | other | ~350 | `.uv` `.ae` `.ol` `.ptl` `.nrn/npl/nrm` `.ifo` `.flh` `.lev` … |

  The engine's 3D pipeline is evidently built around imported LightWave assets plus a
  custom in-game scene representation — a strong lead for the renderer trace next.
- Document the dormant codecs `FUN_0049c8e0` / `FUN_0049ca60` (never used by shipped archives).
- `LegoRR.icd` is 0 bytes on this copy and produces no string hits — likely a dead install marker.
