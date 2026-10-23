# Finding 153 — Stream Start Ignores DirectSound Position and Play Errors

When `FUN_0047b970` successfully loads a stream buffer, it calls the buffer's vtable slot `+0x34` (`SetCurrentPosition`) with zero, then calls slot `+0x30` (`Play`) with `(0, 0, 1)`. It does not store or test either call's return value. It then sets the selected stream state's active field and returns success.

The file parse and DirectSound buffer-creation result are checked earlier, but failures from positioning or starting the created buffer do not alter this helper's success result or active marker. The code therefore cannot report those two start errors through this return path. This is a static control-flow finding; it does not establish whether either call fails for any shipped stream or what an installed DirectSound implementation does after such a failure.

| Operation | HRESULT checked? | Following state update |
| --- | --- | --- |
| `SetCurrentPosition(0)` | No | Continue to `Play` |
| `Play(0, 0, 1)` | No | Set active marker and return success |
| Parse and buffer creation | Yes, through `FUN_0047baa0` result | On failure, clear active marker and return failure |

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. The calls and active-marker write are in `FUN_0047b970` at `0x47b970` in `docs/decompiles/full/LEGORR.full.c`, also recorded in `docs/evidence/legorr-streamed-wav-format.bundle` (`ev_6486d5a9df603739f3e911a3ddbecf8de816bbeef37a6d5935882e522703faf8`). Microsoft documents both [SetCurrentPosition](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/mt708935%28v%3Dvs.85%29) and [Play](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/mt708933%28v%3Dvs.85%29) as returning `HRESULT`.
