# 20 — Verify Pass: Fresh Re-derivation of the Critical Path

**Target:** `source/LegoRR.exe` · SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`
**Analysis provider:** Ghidra 12.1.4 (REA) · fresh session, no carried annotations · 2026-10-20

**Method:** the critical path (`PE entry → WinMain → init → frame loop → engine tick`) was
re-derived from scratch in a new REA/Ghidra session, and the shipped post-script
(`tools/ghidra/CreateEngineFunctions.java`) was rerun on a scratch `analyzeHeadless`
project and diffed byte-for-byte against `docs/decompiles/engine-region.c`.

## Verdict

- **No contradicting evidence anywhere on the critical path.** WinMain's init order,
  the callback-triplet registration, the frame-loop dt math, and the tick structure all
  reproduce exactly the claims in findings 02/03.
- **The named corpus reproduces byte-identical**: 7,653 lines, `diff` exit 0, zero
  changed lines between the shipped and fresh decompile.
- Finding 02's *"install directory at `DAT_00505814`"* wording is corrected below (it is
  actually the exe basename used as the `.wad` prefix).

## 1. Entry & WinMain chain

| Step | Address | Observation |
| --- | --- | --- |
| CRT `entry` | `0x48f000` (PE `AddressOfEntryPoint` RVA `0x8f000`) | Ghidra literal name `entry`: `GetVersion`, command-line fetch, `GetStartupInfoA`, then `uVar8 = FUN_004777a0(hModule, 0, cmdline, argc)`; exit via `FUN_0048e4b0`. |
| WinMain body | `FUN_004777a0` | Compound mutex name (`"%s Mutex"` + `CreateMutexA`), `ERROR_ALREADY_EXISTS` (`0xB7`) ⇒ skips body & returns 0; last-`\` basename copy into `DAT_00505814`; quote strip; registry `StandardParameters` / `NoHALMessage` (`FUN_0048b360`); arg parse `FUN_00477bf0`; init `FUN_0048b260 → FUN_004893f0 → FUN_0047f130(mount) → FUN_00478df0 → FUN_0047ed90`; window `FUN_004783f0`; DDraw `FUN_00488b50` + `FUN_0049d030(1, &0x2000, &0x40, &0x100, …)`; `FUN_0047ec80` / `FUN_00485e80(0)`; hookup `FUN_0041fa20`; frame loop; teardown (incl. `FUN_00484c90(0)`, `FUN_0048b290`, `FUN_0048b2f0`). |
| Frame loop | inside `FUN_004777a0` | `DAT_0076ab4c..ab58` scratch cleared; `FUN_00478040`; `&2` → `DAT_00505808=1`; `FUN_0047eef0` / `FUN_0047f010`; indirect `(*DAT_00505930)(dt)` (0 ⇒ quit); `FUN_00477fa0` / `FUN_0047c8d0`; dt = `0.8333333` (`&0x80`), `DAT_0050593c` override, else `timeGetTime` Δ×`0.025` clamp `3.0`, `0.0` (`&0x10`). |

Evidence: overview `ev_de67056a9fd3339b1e46a34d5af28fac998ae99cad72ad09760938b9abc2f9e1`;
WinMain body `ev_b5ef40ba5b18bf057d18524835589bb15d7e6501faf00a75338504b2f684cde9`;
mount `ev_27454cf22922676eb1fd259d63d0cc528593b15c25133b089b63092aef9feb11`.

## 2. Registration & the tick pointer (independent of auto-analysis)

`FUN_0041fa20(&DAT_00505814)` zeroes the 958-dword engine state block at `DAT_005560c0`,
stores the exe-dir pointer at `DAT_005560c4` and calls `FUN_004783d0`, then hands
`{FUN_0041fb50, LAB_004232e0, LAB_00424cf0}` to `FUN_00477fd0`, which fixes:

| Slot | Value | Note |
| --- | --- | --- |
| `DAT_0050592c` | `FUN_0041fb50` | per-frame init/reset once |
| `DAT_00505930` | `LAB_004232e0` — **the frame tick** | the exact link finding 03's method note predicted |
| `DAT_00505934` | `LAB_00424cf0` (`LAB_00424d00` when app-mode == 10) | shutdown |
| `DAT_00505938` | `1` | registered flag |

`resolve_containing_procedure("0x4232e0")` in the fresh session returns
`not_in_procedure` — live confirmation that auto-analysis skips the tick region
(reached only through the data-indirect call), reproducing finding 03's method note
without relying on it.

Evidence: hookup `ev_864b515a61cdc863c05f66b90df1a7a34e0e22905757a94c28c42dca66f69c36`;
registration `ev_c0e15b0f49a5c45c3e58669ec8916d8deb9515ba1a0f81e8c22e9ba9c7b3c2b2`;
tick resolve `ev_9473f98fa51d5526bcdbca10364f0e59467552b77dd1197192bba97d91200c57`.

## 3. Tick body structure (corpus lines 1–130 re-read)

Matches finding 03's reading exactly: loader gate `(DAT_00505f88==10 && KeepRunning==0 ⇒
return 0)`; `DAT_00556ecc & 0x100000` ⇒ dt = 0.0; `FUN_004231f0` housekeeping; scaled
`fVar10 = FUN_0041fb40() × dt` (raw `dt` kept in `fVar2`); camera cluster `FUN_0046a730`,
`FUN_00435a50`, `FUN_00431560`, `FUN_0042c440`, `FUN_0042cb70`, `FUN_004261c0`; the
screen→world projection solving `DAT_00556148/14c` with distance test against
`_DAT_0049e2cc`; `&4` force-scale; `MainSimStep` gate `FUN_004250c0(fVar10, dt, &3 out) ⇒ 0`
returns; then the subsystem ticker order is **identical** to the finding, including
`FUN_00435f5c ×3` (the three view contexts) and the `&0x10`-gated HUD overlay
(`"Fallin Mode : %s"` etc.).

## 4. Corrections / enrichments to finding 02

1. **`DAT_00505814` = exe basename minus `.EXE`, not the install directory.**
   WinMain copies the characters *after the last backslash* of `GetCommandLineA()`,
   uppercases the extension (`FUN_0049dcd0`, in-place when `DAT_00549180 == 0`) and NULs
   at the `.EXE` (literal at `0x4aac70`, found by `FUN_0048da50` ≙ `strstr`), leaving
   `"LegoRR"`. The mount then formats `"%s%i.wad"` with it (string `0x4aaf50`) — so the
   wads resolve **relative to the working directory** (why launchers chdir into the game
   dir). The actual data *directory* is built in `FUN_0047f130` as
   `getcwd() + "\" + "Data"` (`"%s\%s"` `0x4a1cbc`, `"Data"` at `0x4aaf5c`) → `DAT_005342a8`.
   Evidence: mount `ev_2745…`; strings at `0x4aaf5c` / `0x4aaf50` /
   `0x4a1cbc` `ev_ddf0bb5fb6d73cde3faedbe8879d32572d762140ffbc1c60cf6a083f20545131`;
   `".EXE"` at `0x4aac70` `ev_1436e5780c0c0eb4bd51e14b2a6fd42399d4e2f390cec698738232d6ed4fb21e`.
2. **Pre-hookup / teardown extras** not itemized before:
   `FUN_0047ec80(DAT_0076ac84)` + `FUN_00485e80(0)` run before `FUN_0041fa20`;
   after the loop, teardown is `(*DAT_00505934)()`, `FUN_0047ccf0`, `DestroyWindow`,
   `FUN_0047efd0`, `FUN_00478e20`, then `FUN_00484c90(0)`, `FUN_0048b290`, `FUN_0048b2f0`.
3. **Mutex semantics wording**: a second instance does not "abort" — it skips the whole
   body and returns 0.
4. **Shutdown selection by app mode, not feature word.** `FUN_0041fa20` registers
   `LAB_00424cf0` (one-line stub → `FUN_004250a0`) as the shutdown for normal play
   (`DAT_00505f88 != 10`); the full teardown `LAB_00424d00` (the "EngineShutdown" finding 03
   describes) runs only in loader mode (`DAT_00505f88 == 10`). Finding 02's earlier
   "alt `LAB_00424cf0` when `DAT_00505f84 == 10`" row was inverted on both counts
   (wrong global, wrong direction).

## 5. Reproducibility evidence (fresh pipeline)

```sh
analyzeHeadless /tmp/opencode/ghidra-proj FreshVerify \
  -import source/LegoRR.exe -scriptPath tools/ghidra \
  -postScript CreateEngineFunctions.java -deleteProject
```

Result: `Created 5 renamed 109` (the 5 creations are `EngineTick`, `EngineShutdown`,
`EngineShutdownAlt` and two more reachable only indirectly); output
`/tmp/engine-decompiles/engine-region.c` = **7,653 lines**.

`diff docs/decompiles/engine-region.c` (fresh) → **exit 0, zero changed lines**.

Preserved artifacts (scratch project self-deleted): `/tmp/opencode/engine-region.shipped.c`,
`/tmp/opencode/engine-region.fresh.c`, `/tmp/opencode/engine-region.diff` (0 bytes).

## Confidence

| Claim | Status |
| --- | --- |
| Entry chain, WinMain flow, triplet registration, frame-loop dt math | observed (fresh REA/Ghidra pseudocode) |
| Corpus byte-identical under identical toolchain | observed (`diff` exit 0) |
| `DAT_00505814` = wad prefix (basename − `.EXE`); mount uses it in `"%s%i.wad"` | observed (`FUN_004777a0` + `FUN_0047f130`) |
| `DAT_005342a8` = cwd-based data directory | inferred — built from `getcwd() + "\" + "Data"`; consumer sites not enumerated this pass |
| Identity of `FUN_0049dcd0`'s alternate (DB-backed) branch | unknown — `DAT_00549180 != 0` path not exercised |

## Next steps (unchanged open threads)

- `.flh` container layout (finding 14 §5).
- The 40-entry config population site (finding 04 §Open); input-device writer region (finding 05).
- Consumer-site trace for `DAT_005342a8` to confirm the data-dir label above.