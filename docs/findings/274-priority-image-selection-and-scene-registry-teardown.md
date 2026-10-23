# Finding 274 — Priority Image Selection and Scene Registry Teardown

Two priority-image helpers update selection/order state, and two scene-registry
helpers release registered objects and tear down registry storage. Their direct
mechanics are grounded; the exact rendering effect of ordering callbacks and
the registry's object class remain open.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x45d300 | PriorityImageSelectionStateUpdate | After rectangle hit testing, updates the selected category/sentinel in `DAT_005013b8` and sets bit 1 in `DAT_005013bc` when the selection matches. Two input parameters' precise meanings remain uncertain. |
| 0x45d3f0 | PriorityImageOrderMoveToFront | Shifts preceding entries in `DAT_0050111c` when the selection path allows reordering, then refreshes positions through 0x402090 and 0x428800. The rendering effects of those callees are not established. |
| 0x47e130 | SceneObjectRegistryReleaseAll | Walks registry bucket slots, checks each candidate's `+0x1c` self marker, and passes valid entries to 0x47d470 for vtable `+8` release and free-list chaining. Called by 0x47d430 before bucket arrays and registry globals are cleared. The exact object class remains unknown. |
| 0x47d430 | SceneObjectRegistryTeardown | Calls `SceneObjectRegistryReleaseAll`, frees the registry bucket arrays, and resets the registry end marker and global state. No callers were resolved in the available reference record. |

**Evidence.** Direct decompiles are in
[legorr-ui-render-fifth-pass-2026-10-23.bundle](../evidence/legorr-ui-render-fifth-pass-2026-10-23.bundle):
`ev_18fc68c3c975666bd511522fb7eaf14900e952eedbab5bed28d3856d48ae69f3`
(selection update),
`ev_ad16393179d9edf1d47572d561a5ceb5e324ebbbea7bcde1183d8cf84cc40ced`
(order update),
`ev_18225d870e994d453ff2a0d43101c03bb6a6b50c733b51f7dde5b6869d52c8c8`
(priority action dispatch),
`ev_161deea06a21d3bd33b57709cdaa0c07e41b3451df8fcf97fc9ceb9128d23b6e`
(registry teardown),
`ev_1d8c19373e821cb8cd008fbab85d3b999bfa147e04419958c9a1ede7ea578531`
(registry release). The incoming-reference query in
[legorr-engine-helper-audit-text-io-expansion-9.bundle](../evidence/legorr-engine-helper-audit-text-io-expansion-9.bundle),
`ev_f0cf47f3aa37075053c1394bf0f7351750a6ba0fbe7f385042be356fd7db3d6a`,
returned no callers for `0x47d430`. All observations use static analysis of
shipped
All observations use static analysis of shipped
`reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).
