# Finding 215 — In-Memory WAV Failures Leave the MMIO Handle Open

`FUN_00488c70` opens an MMIO handle and, on successful RIFF/WAVE format
parsing, returns that still-open handle to `FUN_0047b580`. The caller closes
it only after `FUN_00489100` successfully reads the data payload. If data-chunk
positioning fails, `GlobalAlloc` fails, or the payload read fails, control
reaches `FUN_0047b580`'s failure cleanup. That cleanup frees the allocated
payload buffer when present, but does not call `mmioClose` on the returned
handle.

The leak window ends before DirectSound setup: after a successful payload
read, the code closes the MMIO handle before creating/filling the sound
buffer. Failures in those later DirectSound steps therefore do not leave this
MMIO handle open. The condition is limited to in-memory WAV loading and to
failures after `FUN_00488c70` has returned an open handle.

**Evidence.** Static disassembly of shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
`FUN_00488c70` at `0x488c70` stores the open handle through its output
parameter on success and closes it on its own error path. In
`FUN_0047b580` at `0x47b580`, the returned handle is used by
`FUN_004890c0` and `FUN_00489100`; `mmioClose` occurs only on the success path
after the read (`0x47b646–0x47b648`). The failure branch at `0x47b69d` frees
the payload allocation, then returns without another close. The relevant
functions are in `docs/decompiles/full/LEGORR.full.c`; finding 122 documents
the MMIO data-window copy path.
