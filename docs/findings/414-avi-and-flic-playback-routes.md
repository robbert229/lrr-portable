# Finding 414 — AVI and FLIC Playback Routes

This map addresses all 41 function entries assigned by Issue #196. Forty are
AVI/FLIC playback routines; one entry was misclassified by the issue and is
reclassified below as DirectInput teardown. The two video families are separate:
the credits/AVI player uses AVIFIL32, while `.flh` overlays use the engine's own
FLIC-family reader and decoder.

**Target and evidence.** Static analysis of shipped `reference/LegoRR.exe`,
SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
Function bodies are in [`LEGORR.full.c`](../decompiles/full/LEGORR.full.c).
The AVI playback expansion evidence is in
[`legorr-avi-playback-expansion.bundle`](../evidence/legorr-avi-playback-expansion.bundle).
The FLIC byte-level and corpus analysis is in [finding 21](21-flh-container.md).

## Issue scope correction

Issue #196 lists `0x0047efd0` as `AviFileShutdown` in the video-playback
decoder area. The routine uses the DirectInput globals populated by
`DirectInputInitialize` at `0x0047ed90`: when the acquired flag is set it calls
the device method at slot `+0x20`, clears that flag, and releases the device and
parent interfaces through slot `+8`. It is therefore named `DirectInputShutdown`
and reclassified in the coverage map as `player-interface/input-device-and-cursor`.
It is documented here to account for the issue's full 41-entry list, but is not
one of the 40 AVI/FLIC functions in the area.

## AVI route

The AVI movie object owns an `AVIStream`, an `AVIStreamGetFrameOpen` result,
a DirectDraw-backed surface descriptor, stream metadata, and a clock-derived
frame index. The higher-level movie loop requests a frame, checks the current
index against the stream length after that request, resets the index and clock
at the boundary, and blits the movie surface to the shared render target. The
boundary ordering and resulting null-frame behavior are detailed in findings
[175](175-null-avi-frame-reblits-previous-pixels.md),
[176](176-avi-command-loop-restarts-at-stream-length.md), and
[172](172-avi-frames-blit-to-render-target.md).

| Address | Function | Role in the assigned route |
| --- | --- | --- |
| `0x0047e4c0` | `AviPlaybackStateInitialize` | Initializes the display-surface state and vtable, delegates display-mode and surface setup to `0x0047e510`, and sets its ready byte only when setup succeeds. |
| `0x0047e760` | `AviSurfaceLock` | If the surface-state lock byte is clear, clears a `DDSURFACEDESC`, invokes the surface vtable lock slot with flags `0x801`, and sets the byte on success. |
| `0x0047e7b0` | `AviFrameSurfaceUnlock` | Calls the surface unlock slot only when the lock byte is set, then clears it on success. The pixel-copy caller does not use lock success to guard the copy (finding 210). |
| `0x0047e800` | `AviFrameRetrieverOpen` | Initializes the movie object, opens stream index zero with type `vids`, opens a frame retriever using format selector `1`, reads `AVIStreamInfoA`, stores dimensions/rate/length, and creates the display surface. It does not open an embedded audio stream (finding 177). |
| `0x0047e980` | `AviPlayerDestroy` | Runs the retriever destructor and optionally releases the separately owned player allocation when flag bit 0 is set. |
| `0x0047e9a0` | `AviFrameRetrieverDestroy` | Restores the movie vtable, releases a non-null `AVIStream`, and invokes the surface object's first method if that object exists. It does not explicitly close the frame-retriever handle (finding 174). |
| `0x0047e9e0` | `AviFrameDecodeAndCopy` | Copies rows from a non-null decoded DIB into the surface buffer for the recognized 15- and 16-bit formats. Source rows advance bottom-up while destination rows advance by pitch; lock success controls only whether unlock is attempted (findings 123 and 210). |
| `0x0047eb20` | `AviFrameUpdateAndBlit` | Returns if the object is not ready or AVI is disabled; otherwise fetches the current index. A non-null frame is copied, then the clock and frame index are updated. A null frame leaves those fields unchanged (finding 175). |
| `0x0047ebe0` | `AviStreamFrameCountGet` | Returns zero when the AVI-disabled global is set; otherwise returns the stored `dwLength` field from stream info. |
| `0x0047ebf0` | `AviFrameSeekAndClockRebase` | Stores a requested frame index and rebases the stored time origin so the frame clock corresponds to that index. |
| `0x0047ec60` | `AviPlaybackFrameIndexGet` | Returns the stored frame index, or zero when AVI is disabled. |
| `0x0047ec80` | `AviFileInitialize` | Stores the supplied global interface and calls `AVIFileInit`. |
| `0x0047ec90` | `AviPlayerCreate` | Allocates a `0xc4`-byte player object and invokes its constructor when allocation succeeds; returns zero on allocation failure. The caller-side null handling is covered by finding 173. |
| `0x0047ecf0` | `AviFrameUpdate` | Thin wrapper forwarding to `AviFrameUpdateAndBlit`. |
| `0x0047ed00` | `AviPlaybackFrameLoop` | For a ready object, obtains the shared render surface and calls its blit slot with the movie surface and `DDBLT_WAIT` (`0x01000000`). The return value is ignored (finding 172). |
| `0x0047ed40` | `AviFrameIndexAndClockReset` | Thin wrapper forwarding the requested index to the seek-and-clock-rebase method. The command loop uses index zero when it restarts (finding 176). |
| `0x0047ed50` | `AviCurrentFrameIndexGet` | Thin wrapper forwarding to the guarded playback-index getter. |
| `0x0047ed60` | `AviStreamFrameCountGet` | Thin wrapper forwarding to the stream-length getter. |
| `0x0047ed80` | `AviPlayerReadyGet` | Returns the movie object's ready byte at offset `+0x99`. |

The movie frame index is derived from elapsed time and the stored rate rather
than incremented by one: after a successful frame fetch, `0x0047eb20` computes
`dwRate / dwScale`, stores the current time, and converts elapsed time to the
next requested index. `0x0047ed40` resets both index and time base. The outer
loop at `0x0040a0c0` owns the repeat policy, making the index reset an external
caller decision rather than a clamp inside the frame-fetch method (finding
176). The player also blits after a null frame fetch, leaving the previous
surface pixels available for that draw (finding 175).

## FLIC route

The FLIC player reads `.flh` data itself. Its open path allocates and zeroes a
context, validates the header, creates a DirectDraw overlay surface, chooses
stream or RAM mode from the context flags, and allocates scratch space. Each
tick locks the surface, advances through the selected input mode, unlocks, and
blits. The shipped corpus and exact header, chunk, opcode, palette, and decoder
behavior are fully detailed in [finding 21](21-flh-container.md); the rows
below map all 21 functions assigned by Issue #196 to that path.

| Address | Function | Role in the assigned route |
| --- | --- | --- |
| `0x00483c80` | `FlicFileOpenAndStateCreate` | Allocates/zeros the FLIC context, validates the header, creates a surface from its width and height, seeds mode fields, opens the file, and allocates its working buffer (finding 21 §§1–2). |
| `0x00483f00` | `FlicPlayerClose` | Releases the RAM-preload buffer or closes the file handle according to mode, then frees the optional enlarged chunk buffer (finding 21 §1). |
| `0x00483f60` | `FlicFileHeaderValidate` | Reads the 136-byte header, checks declared size against file length and validates the supported magic values; copies width, height, depth, and frame-count metadata into the context (finding 21 §2). |
| `0x00484070` | `FlicFrameAdvanceAndBlit` | Tick entry: locks the overlay surface, samples timer state, advances one frame in RAM or stream mode, unlocks, then blits with the observed color-key flags (finding 21 §§1–2). |
| `0x004841d0` | `FlicRamFrameAdvance` | Walks the preloaded chunk buffer and updates frame/cursor state; on loop mode it restarts at the saved loop position with index 1, retaining the already-decoded keyframe (finding 21 §2). |
| `0x00484260` | `FlicStreamFrameAdvance` | Reads the next stream chunk, invokes the chunk walker, increments counters, and either reports end-of-playback or seeks to the loop mark (finding 21 §§1–2). |
| `0x00484320` | `FlicChunkWalk` | Reads adjacent chunk headers until it decodes a frame or reaches the file bound; dispatches frame chunks and buffers the `0x000B` and `LK` cases (finding 21 §3). |
| `0x004844b0` | `FlicFrameRecordWalk` | Iterates the frame's declared subrecords and sends each record to the opcode dispatcher; streaming mode first ensures the record body is buffered (finding 21 §§1, 3). |
| `0x00484610` | `FlicSubrecordOpcodeDispatch` | Dispatches opcodes 25 and 27 to their decoders, opcode 16 through its mode helper, row records to their helper, and selected opcodes to success/no-op handlers (finding 21 §3). |
| `0x00484720` | `FlicChunkBufferAndContinueLK` | In streaming mode grows the scratch chunk buffer when required and copies the current chunk from the input stream; RAM mode does nothing (finding 21 §3). |
| `0x004847d0` | `FlicChunkBufferAndContinueType0B` | The corresponding stream-buffer path for chunk type `0x000B`; RAM mode does nothing (finding 21 §3). |
| `0x00484880` | `FlicOpcode16ModeDispatch` | Routes the observed depth/mode combinations to the opcode-27 fallback no-op; no decoded pixels are written on these branches (finding 21 §§3, 7). |
| `0x004848d0` | `FlicDecode8BitOpcode25Mode4` | Decodes 8-bit opcode-25 run/literal commands row by row through the palette lookup helper (finding 21 §§3, 7). |
| `0x004849d0` | `FlicDecode16BitKeyframe` | Decodes opcode-25 16-bit keyframe rows and applies the RGB555-to-surface-format transform (finding 21 §4). |
| `0x00484b20` | `FlicOpcode25DecodeDispatch` | Selects the opcode-25 success stub, 8-bit mode-4 decoder, or 16-bit keyframe decoder from header depth, magic, and mode (finding 21 §§3, 7). |
| `0x00484b90` | `FlicOpcode25SuccessStub` | Returns success without writing pixels; the shipped corpus does not use this handler (finding 21 §3). |
| `0x00484c00` | `FlicDecode16BitRowRecords` | Applies palette-row records to the local color table and builds the derived word table for mode 4 (finding 21 §§3, 7). |
| `0x00484c90` | `FlicOpcode27FallbackNoop` | Returns without decoding. It is selected for the documented unsupported 8-bit/magic combinations (finding 21 §§3, 7). |
| `0x00484ca0` | `FlicDecode8BitAlternateDelta` | Handles the alternate 8-bit delta command family, including row jumps, skips, literals, repeats, and palette mapping (finding 21 §7). |
| `0x00484e50` | `FlicDecode16BitDelta` | Applies 16-bit opcode-27 row deltas, including row jumps, pixel skips, literal/repeat runs, and format-dependent color transforms (finding 21 §5). |
| `0x00485030` | `FlicOpcode27DecodeDispatch` | Selects the 8-bit alternate-delta path, the 16-bit AF43 delta decoder, or the documented fallback no-op (finding 21 §§3, 7). |

For the shipped 18-file `.flh` corpus, finding 21 reports `AF43`/16-bit files,
with opcode-25 keyframes, opcode-27 deltas, and zero-sized hold chunks. Thus
several assigned 8-bit, alternate-magic, RAM-preload, and buffer-extension
branches are present in the executable but are not exercised by those shipped
files. The per-function decoder descriptions above state code paths, not claims
that the corpus uses every branch.

## Evidence limits and follow-ups

This map is static. It does not measure AVIFIL32 frame-retriever lifetime,
DirectDraw lock/blit success, which display format a particular runtime picks,
or whether malformed FLIC chunks cause an observable failure. Those limits
remain as noted in findings 123, 172, 174, 175, 210, 216, and 220. Finding 21's
scripted corpus validation establishes decoding of the shipped `.flh` samples,
not safety for arbitrary malformed files. Focused runtime follow-ups should
measure AVI retriever cleanup and exercise non-corpus FLIC modes before making
claims about leaks or decoder robustness.
