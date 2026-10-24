# Finding 329 — Viewport and Global-Refresh Helpers

Two helpers update render viewport state and clear a global low byte before refreshing related state. Their flags and values remain raw where the underlying meanings are not established.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x426250 | UpdateRenderViewportState | Selects a coordinate pair from current or fallback state, sets renderer field `+0x14`, and applies the pair to the renderer. |
| 0x435a20 | GlobalStateLowByteClearAndRefresh | Clears the low byte of `DAT_00556ed4`, then refreshes the generated scene quad-grid render object held in `DAT_005563b4` with mode 1. |

Bodies and caller context are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Direct analysis for `0x426250` is in `legorr-engine-helper-audit-render-expansion-10.bundle`, record `ev_a7ef59f056b8f7ed9f9154272c995a8dda80e33a6b677f2a90e0d7bb8c8c9b3c`; its renderer-helper caller record is `ev_e757a97f65e371b8eb9f2fdb1d19700bc580dd9a18d62582d5ca7e5433aab332`. Direct evidence for the low-byte reset is in `legorr-engine-helper-audit-mid-expansion-6.bundle`, record `ev_274dad58b224a5be9b0984bef6a4873ee447b8bddad3001b0253233ebd3a8e98`. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
