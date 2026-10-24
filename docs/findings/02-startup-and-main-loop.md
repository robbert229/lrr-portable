# 02 — Startup & Main Loop

**Observations from** `FUN_004777a0` (WinMain body — itself called from the CRT `entry` @ `0x48f000`), `WadMountStartup @ 0x47f130`, `FUN_0041fa20` / `FUN_00477fd0` (engine registration), and the loop in `FUN_004777a0`. Ghidra 12.1.4, 2026-10-19; re-verified from scratch 2026-10-20 (finding 20). All addresses are virtual (image base `0x400000`).

## WinMain body: `FUN_004777a0`

1. **Single-instance mutex** — `CreateMutexA(NULL, TRUE, "Lego Rock Raiders %s")` (name composed by `FUN_0048d9e0` from a format string + `"Mutex"`); a `GetLastError()` of `ERROR_ALREADY_EXISTS` (`0xB7`) skips the *entire* body and returns 0 (i.e. a second instance does nothing).
2. **Wad prefix from exe basename** — walks `GetCommandLineA()` to the last `\`, copies the **basename** (chars after the last backslash) into `DAT_00505814`, strips quotes in place, then uppercases the extension (`FUN_0049dcd0`) and NUL-terminates at `.EXE` (`FUN_0048da50` ≙ `strstr`). Result: `DAT_00505814 = "LegoRR"` — the prefix used by the mount's `"%s%i.wad"` loop (so `DAT_00505810 = &DAT_00505814`). The data *directory* is built separately in the mount: `getcwd() + "\" + "Data"` → `DAT_005342a8`. *(Corrects an earlier "install directory" reading — finding 20.)*
3. **Registry** — `SOFTWARE\LEGO Media Games Rock R…`: `StandardParameters`, `NoHALMessage` (HAL-failure popup text default `No DirectX 3D accelerator could be found…`).
4. **Arg parsing** — `FUN_00477bf0(cmdline, &local_c8c, &local_c90)` → config word pair consumed later.
5. **Init order**: `FUN_0048b260`, `FUN_004893f0`, **WAD mount** `WadMountStartup(exedir, local_c90, regkey)`, `FUN_00478df0`, `FUN_0047ed90`.
6. **Window + device**: `WindowCreate @ 0x4783f0(hInst)` (creates window `DAT_00505800`), `FUN_00488b50(local_c8c)`, then `FUN_0049d030(1, flags & 0x2000, flags & 0x40, flags & 0x100, buf)`.
7. **Engine hook-up**: `FUN_0047ec80(DAT_0076ac84)` + `FUN_00485e80(0)` (render/graphics prep), then `FUN_0041fa20(&DAT_00505814)` → registers the callback triplet (below), then the frame loop runs.

## The callback triplet (the real game module boundary)

`FUN_0041fa20(exedir)` builds `{init, tick, shutdown}` and hands it to `FUN_00477fd0`, which stores them at `DAT_0050592c/930/934` and sets `DAT_00505938 = 1` (registered flag):

| Slot | Address | Role |
| --- | --- | --- |
| `DAT_0050592c` | `FUN_0041fb50` | per-frame *init/reset* check — huge local-frame function initializing float constants (`DAT_00556f2c = 0.3e-1` etc.); cleared when it returns 0 (once) |
| `DAT_00505930` | `LAB_004232e0` | **the frame tick** `f(dt)` |
| `DAT_00505934` | `LAB_00424cf0` (stub → `FUN_004250a0`); `LAB_00424d00` (full teardown) only when `DAT_00505f88 == 10` (loader mode) | shutdown |

The whole game logic lives between `0x41fb50`–`0x424d00`, which explains why Ghidra reports no named functions there (direct-label entry, no data refs). This region is the heart of the executable.

## Frame loop (observed)

```c
while (!quit) {
  DAT_0076ab4c = 0;                // per-frame primary mouse-button down-event latch
  DAT_0076ab50 = DAT_0076ab54 = DAT_0076ab58 = 0; // adjacent input-event state
  pump_messages();                 // FUN_00478040
  FUN_0047eef0();                  // pre-tick (input state?)
  FUN_0047f010();                  // pre-tick 2
  if (tick(dt) == 0) quit = true;  // DAT_00505930
  FUN_00477fa0();
  FUN_0047c8d0();
  // dt: fixed 0.8333333 if flags&0x80, else timeGetTime delta * 0.025 (25 ms units), clamped to 3.0
  //      flags&0x10 -> 0.0 ; DAT_0050593c override wins when set
}
// teardown: (*DAT_00505934)(), FUN_0047ccf0, DestroyWindow, FUN_0047efd0, FUN_00478e20,
//           FUN_00484c90(0), FUN_0048b290, FUN_0048b2f0
```

`DAT_00505f84` is the master feature word (`&0x80` fixed-step, `&0x10` pause/skip-time, `&2` alt mode, bits `0x100/0x2000/0x40` gate init calls).

## WAD mount detail (`WadMountStartup @ 0x47f130`)

- Loops `%s%i.wad` for i=0..9 through `MountWadFromPath` (`0x47f640`; mounts into the next free context slot); success sets `bVar2`.
- CD detection via `FindCdKeyDrive` (`0x47f4f0`), with `CDMissing` / retry MessageBox loop (`cd.key`, `delme.dat` markers; `SetupError`). The helper scans drive letters C: through Z: for a CD-ROM containing `cd.key`, then saves the matching drive letter.
- On missing data: `DataMissing` + `MessageBox` + exit `FUN_0048e4b0(0)`.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| WinMain flow, mutex, registry key, window HWND slot | observed |
| `DAT_00505814` = exe basename − `.EXE` (wad prefix, **not** the install dir) | observed — re-derived 2026-10-20, finding 20 |
| Shutdown slot: `LAB_00424cf0` (stub→`FUN_004250a0`) for normal play; `LAB_00424d00` when `DAT_00505f88 == 10` | observed — corrected here, finding 20 |
| Triplet registration mechanism | observed |
| dt calculation and loop constants | observed |
| `FUN_0041fb50`'s float table = game-rule constants | **inferred** — constants like `0.3e-1`, `0.7e-1` smell like speeds/friction, not proven |
| What `LAB_004232e0` (tick) does internally | **resolved** — decompiled; see [03 — Engine Tick](03-engine-tick.md) |
| `FUN_00488b50` = Direct3D device init | inferred from HAL message adjacency |

## Next targets (priority order)

1. ~~`LAB_004232e0` tick~~ done via headless `analyzeHeadless` + `tools/ghidra/CreateEngineFunctions.java` — see [03 — Engine Tick](03-engine-tick.md).
2. The `.map` / `.blx` / `.lwo` loaders (consumer side of the resource loader).
3. Save/load: grep strings for `SAVE`/`.sv` under `Data\`.
