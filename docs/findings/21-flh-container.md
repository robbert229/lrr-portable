# 21 — `.flh` container: FLIC-family overlay animation, fully closed

*Target: `LegoRR.exe` (x86 PE, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).*
*Provider: Ghidra 12.1.4 via REA. All addresses cite the shipped image.*

## TL;DR

The `.flh` files (18 shipped, all in `LegoRR0.wad`) are **FLIC-family animation
containers** — the engine literally calls them "Flics" (`Flic File Invalid %s`
at `0x4ab124`) and reads them with a private FLIC-style decoder, not
AVIFIL32. Layout: 136-byte header, then a stream of 16-byte chunks starting at
`0x80`. Every shipped chunk is type `0xF1FA` (FLI8 frame). Each file is exactly
**one keyframe (opcode 25, FLI16-style byte RLE) + `nFrames` delta frames
(opcode 27, word-based diff) with zero-sized hold-chunks marking repeated
frames**. 16-bit pixels, colors stored as **RGB555**, expanded to 5-6-5 on
write. Validated byte-exact on all 18 files / 469 chunks, plus visual render
of reconstructed frames. This closes finding 14 §5 / open item 4.

## 1. Loader / playback function map

| Address | Role |
| --- | --- |
| `0x483c80` | Open: allocates `0x6e8` context, zeroes it, calls validator, copies width/height into ctx `+0xc`/`+0x10`, creates the overlay surface (vtable `+0x18` on the `DAT_0076ac84` surface object), seeds mode byte (see §2), allocates the 20 000-byte frame buffer at `+0x6e0` |
| `0x483f60` | Validate: reads 136 B header into ctx `+0x640`, checks `size == filesize` and magic ∈ {`0x1234`, `0x9119`, `0x911A`, `0xAF11`, `0xAF12`, `0xAF43`} |
| `0x484070` | Tick: lock surface, read two engine timer globals (`__ftol` pair), step one frame (RAM → `0x4841d0`, stream → `0x484260`), unlock, blit with source-color-key (`0x1000000` flag, `iStack_c` gated bit `0x8000`) |
| `0x4841d0` / `0x484260` | Frame steppers, RAM and streaming modes (§2) |
| `0x484320` | Chunk walker: per chunk read `size`/`type`; `type` `0x000B` → `0x4847d0`, `'LK'` `0x4B4C` → `0x484720` (both buffer-and-continue), `0xF1FA` → `0x4844b0` (decode frame), else skip by size; bound = file size |
| `0x4844b0` | Frame walker: `nRecords = byte[chunk+6]`, subrecords from chunk `+0x10`, each advanced by its own `size` |
| `0x484610` | Opcode switch on `u16 (record+4)` (§3) |
| `0x484b20` | Opcode 25 (`0x19`) dispatch: 16-bit + magic `AF43` → `0x4849d0`; 8-bit → `0x484b90`/`0x4848d0` by mode ctx `+0x6d4` (0 or 4; set to 4 at open) |
| `0x4849d0` | **16-bit keyframe decoder** (§4) |
| `0x485030` | Opcode 27 (`0x1b`) / 7 dispatch: 16-bit + `AF43` → `0x484e50`; magic `1234` → `0x484c90` |
| `0x484e50` | **16-bit delta decoder** (§5) |

Error strings (`Flic File Invalid %s`, `Flic File Not Found %s`,
`Flic Not Enough Memory`, `Not Enough Memory > %s`) sit at `0x4ab10c–0x4ab168`;
their xrefs land inside `0x483f85`/`0x483fdf`/`0x484043`/`0x48457b`/`0x484779`/
`0x484829`, i.e. the whole player region shares them.

Six open call sites: `0x41236d`/`0x4123d5` (overlay/reward flic lane, walks the
config-driven entry list and opens the selected `.flh` — this is the
`Reward*Flics` / `DisplayFlics` side documented in findings 09/14) and
`0x45a342`/`0x45cb4a`/`0x4618ac`/`0x46248a` (screen-overlay lane,
`FUN_0045a2e0` opens with flag word 3 into the `DAT_00500110` slot table).
Playback pacing is entirely timer-driven from the tick — **no per-frame delay
fields exist in the file** (chunk-header padding is zero in the whole corpus).

## 2. Header and modes

Header, 136 B read whole into ctx `+0x640` (so `ctx + 0x640 + n` = header byte
`n`); frame data starts at file offset `0x80`:

| Off | Field | Notes |
| --- | --- | --- |
| `+0x00` | u32 file size | must equal actual length (`0x483f60`) |
| `+0x04` | u16 magic | shipped corpus: `0xAF43` everywhere; validator also accepts `0x1234`, `0x9119`, `0x911A`, `0xAF11`, `0xAF12` |
| `+0x06` | u16 `nFrames` | really *last frame index*: chunk count = `nFrames + 1` in all 18 files |
| `+0x08` | u16 width | → ctx `+0xc` |
| `+0x0A` | u16 height | → ctx `+0x10` |
| `+0x0C` | u16 depth | corpus: 16 everywhere; 8 selects the 8-bit decoder family |
| `+0x0E…` | 118 B | FLIC-header vestige, untouched by the engine |

The first ctx dword carries two mode bits, seeded by the surface-description
vtable call during open:

- **bit 0 — stream mode vs RAM preload.** Streaming: file handle at ctx
  `+0x6c8`, cursor `+0x18d` starts at `0x80`, 20 000-byte scratch buffer; each
  tick reads the 16-byte chunk header then pulls the rest lazily
  (`0x484260`). RAM: whole chunk stream preloaded to buffer at ctx `+0x14`,
  cursor indexes RAM (`0x4841d0`).
- **bit 1 — loop.** Past the last frame: stream mode seeks back to the loop
  mark (start of frame 0) and replays including the keyframe (`0x484260`);
  RAM mode restarts at index 1 — **the keyframe is not re-decoded, it is still
  in the RAM buffer** (`0x4841d0`: `idx = 1`, pos = loop mark).

## 3. Chunk stream

From `0x80`, consecutive chunks, all `size`-adjacent to EOF:

| Off | Field |
| --- | --- |
| `+0x00` | u32 chunk size (includes the 16-byte header) |
| `+0x04` | u16 type — `0xF1FA` in every shipped chunk |
| `+0x06` | u16 record count — `1` for frame chunks, `0` for holds |
| `+0x08` | 8 B — zero in all shipped files (no timing, no flags) |
| `+0x10` | `count` subrecords, each `[u32 size][u16 opcode][6 B][body]`, advanced by `size` |

**Record count 0 (`size == 0x10`, body empty) = frame hold**: the tick advances
frame index without touching pixels, so earlier frames repeat. This is what
makes the census work: e.g. `SLUG_ON` has `nFrames = 16`, 17 chunks = 1 key +
12 deltas + 3 holds.

Subrecord opcodes (`u16 @ rec+4`) handled by `0x484610`; shipped files use only
two:

| Opcode | Handler | Meaning |
| --- | --- | --- |
| `25` (`0x19`) | `0x484b20` → `0x4849d0` | keyframe, 16-bit §4 |
| `27` (`0x1b`) | `0x485030` → `0x484e50` | delta frame, 16-bit §5 |
| `11`/`12`/`13` | `0x484b90` | success-return stub; no pixel writes (unused in corpus) |
| `15`/`16`/`25` | depth/magic dispatch | 8-bit or magic-1234 variants (unused) |
| `4`, `16`, `27`, `7` | `0x484c00`, `0x484880`, `0x485030` | 16-bit row records, mode dispatch, and opcode dispatch (unused paths) |
| `'DS'` `0x5344`, `'UU'` `0x5555` | —, `FlhMagicUuNoop` (`0x456210`) | unused; both paths are silent no-ops |

The 8-bit opcode-25 handler at `0x484b90` is only a six-byte function that
returns success; it does not decode or write rows. The 8-bit magic-1234
opcode-27 fallback at `0x484c90` is a one-byte no-op. The implemented 8-bit
opcode-25 mode-4 path is `0x4848d0`; the alternate 8-bit delta decoder is
`0x484ca0`. The 16-bit row-record helper at `0x484c00` is also a separate
decoder. These paths are present in the image but are not used by the shipped
18-file corpus described above.

Chunk types `0x000B` and `'LK'` (`0x4B4C`) exist in the walker as buffer-and-
continue cases (`0x4847d0`/`0x484720`) but ship in zero files.

## 4. Keyframe — opcode 25 (`0x4849d0`)

Record header is 7 bytes (`size` + `opcode` + 1), body at `rec+7`, commands
read as bytes — classic **FLI16**:

For each of `height` rows, commands fill `width` pixels exactly:

| Command | Encoding | Emits |
| --- | --- | --- |
| `c < 0x80` | `c`, u16 color | `c × color` (3 B total) |
| `c ≥ 0x80` | `c`, `256−c` × u16 | `256−c` literal colors |

after each row: **1 terminator byte** (value unvalidated by the engine; corpus
values are effectively don't-care). Row output pointer
`surfbase + pitch/2 · row · 2` (pitch ctx `+0x638`, surface base `+0x6cc`);
overlong command runs just keep writing linearly into the surface.

**Color transform** on write: `((c & 0xffE0) << 1) | (c & 0x1f)` — pure
**RGB555 → RGB565** expansion (verified: `0x7C00` → `0xF800`, `0x03E0` →
`0x07C0`, blue kept in bits 0–4). If the surface-format flag at ctx `+0x6e4`
is set, the engine instead writes `c & 0x7FE0 | c & 0x1F` — 555→555 identity.
So *stored* colors are always RGB555.

## 5. Delta frame — opcode 27 (`0x484e50`)

Record header 8 bytes: `[u32 size][u16 opcode][u16 nChangedLines]`, word stream
at `rec+8`. A row cursor starts at 0; commands:

| Word test | Meaning |
| --- | --- |
| `(w & 0xC000) >> 12 == 0xC` | **row jump**: `row -= (short)w` — i.e. a negative word (`0xFFFF` → +1) skips lines; the changed-region is positioned by these |
| `== 0x4…` | end-of-record sentinel (not used by shipped files) |
| `== 0x8…` | color-embedded variant of the count word (not exercised at 16-bit in shipped files) |
| else | `w` = command count for the next changed line |

Per line record: `w` commands, each a u16 `[skip:u8 | control:u8]` followed by:

| Control | Emits | Words consumed |
| --- | --- | --- |
| `< 0x80` | `control` literal colors | `2·control` after the command word |
| `≥ 0x80` | `(256−control)` repeats of the next u16 color | 1 |

`skip` advances the pixel pointer (row start = `row·pitch` in surface
memory); commands are **not** row-length-bound — they just position, and only
the touched pixels change. After each line record: `row += 1`, changed-line
count decrements; `0` consumed = end. Colors get the same 555→565 transform as
keyframes (and, in the 0x8-path, an extra `>>1 & 0x7ff0 | …` correction when
ctx `+0x6e4` is set).

Reconstruction model that reproduces the engine: keyframe onto an `w×h` RGB555
buffer, then apply delta records in order (row jumps signed, pixel pointers
absolute within `row·w`); hold chunks are no-ops.

## 6. Validation (18/18 files, 469/469 chunks)

Every shipped `.flh` walked and decoded by a scripted faithful translation of
`0x4849d0`/`0x484e50`:

- 18 files: magic `AF43`, depth 16, `nFrames+1 == ` chunk count, type `F1FA`
  only, exactly one opcode-25 keyframe, rest opcode-27 deltas or size-16 holds,
  chunk padding zero.
- Keyframes consume exactly `size` bytes (±1 over-read on the final row
  terminator) and emit exactly `w·h` pixels with row-local command
  termination; deltas terminate precisely at their record end. Zero failures
  across 469 chunks.
- Visual proof: reconstructed HAMMER frames 0/1 render as a coherent animated
  tool cursor (no skew ⇒ row/terminator/row-jump model correct; small local
  diffs ⇒ delta model correct); `HOLO` (158×226, 81 frames) and
  `CAPTAIN`/`CAPTY` (235/234×327, 35 frames) reconstruct end-to-end.

Inventory: 11 pointer-icon flics 24×24–25×25 (`Interface\Pointers\*`,
9–80 frames), `Interface\InfoPanel\HOLO.FLH` 158×226×81f,
`Interface\Reward_CAPTAIN/CAPTY.FLH` ≈235×327×35f. `LegoRR1.wad` ships none.

## 7. Notes / theoretical coverage

- 8-bit files (depth 8) route to `0x484b90`/`0x4848d0`/`0x484c90`/`0x484ca0`
  families selected by mode ctx `+0x6d4`; magic `1234` selects `0x484c90` for
  both opcodes. **No shipped file exercises any of these** — an
  implementation that handles `AF43`/16-bit/opcodes {25,27} covers the whole
  corpus.
- RAM preload mode (§2) is also unexercised: with the context dword zeroed at
  open and nothing proven to set bit 0, streaming is what the shipped players
  demonstrably take (RAM branch in `0x4841d0` walks a RAM cursor instead).
- `'LK'`/`0x000B` chunks: loaded into buffers, never read — the loaders
  (`0x484720`/`0x4847d0`) just copy `size` bytes from the stream.
- The 6-byte remainder of subrecord headers (after size+opcode) is written but
  never read for the two shipped opcodes (keyframe: body at `+7`; delta: line
  count is the u16 at `+6`, body at `+8`).
