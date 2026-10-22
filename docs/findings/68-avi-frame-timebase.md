# Finding 68 — AVI frames are indexed from elapsed time and stream rate

*Target: shipped `LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.*
*Provider: Ghidra 12.1.4 via REA. Evidence:
`docs/evidence/legorr-avi-playback-expansion.bundle`.*

The AVI player keeps a sample index and derives its next requested index from
the elapsed millisecond clock. This fills in the frame-selection path omitted
from the API overview in finding 14. These are static observations; they do
not describe decoder behavior or prove what any codec displays.

## Frame request and clock calculation

The stream object opens an `AVIStream` for type `vids`, opens its frame
retriever, then asks `AVIStreamInfoA` to populate a `0x8c`-byte stream-info
record. Only after those calls succeed does it mark the object initialized,
zero the current frame index, and save `timeGetTime()` converted from
milliseconds to seconds (`ev_74608d1b910c4f9d9e8af66f68212b8342b23d6fffec0143b36fd28b356fed7d`,
`ev_28cfc4b65b60dd09db081da318d1c63c37e2f6f77455006f30761db067a8b0ef`).

On an initialized object, `FUN_0047eb20` first calls
`AVIStreamGetFrame(stream, currentIndex)`. A non-null returned pointer is sent
to a bitmap-copy helper. The method then computes a frame-rate ratio from the
stream-info record's `dwRate` and `dwScale`, converts current time to seconds,
multiplies elapsed seconds from the saved base time by that ratio, converts
the result to an integer, and stores it as the index for the next request
(`ev_6e8a4593cddf3e6be5a71ae2b65a4625fcf250f71316c9b9ca1153c7b4bd7e0c`,
`ev_0e0cd17fe78fd01d2158f2da1644d3664bc2e58604709c486f81268ae0518426`).
The API call's import thunk has this method as its observed callsite
(`ev_8bd81e0d1c60b07d3270fa4b67061c46b7fed2024608dd5a259fadf787746784`).

If `AVIStreamGetFrame` returns null, the method skips both the bitmap-copy
call and the time/index update; it returns through the common success-shaped
tail. No explicit stream-length clamp appears in this method before the frame
request. The library's result for an out-of-range index, and the conversion
helper's exact rounding behavior, remain outside this observation.

`FUN_0047ebf0` is a seek-style setter: it stores a caller-supplied index,
captures the current time, and rebases the saved clock by subtracting
`index / frameRate` (`ev_898a3e56c12c81b87d1e2cb2da14aa9f8ad48bf8129ced94f93bd7047ba9eb37`,
`ev_7daa484fd18cb5a62623ba56880b99b782826bcda40b915e8f136ad024fdb2b2`).
That keeps subsequent time-derived indices relative to the selected frame.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Frame retrieval uses the stored index, then updates the next index from elapsed seconds and `dwRate / dwScale` | observed in shipped code |
| The first retrieved frame is necessarily frame zero | only the initial stored index is zero; retrieval depends on a non-null frame result |
| Null frame retrieval stops playback, retries, or marks completion | unknown; this method simply skips its update and returns |
| Exact conversion rounding, end-of-stream policy, and codec output | unknown; no runtime decoder observation was made |
