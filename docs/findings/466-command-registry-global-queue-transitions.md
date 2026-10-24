# Finding 466 — Command Registry and Global Queue Transitions

Finding 10 maps the entity command record format, kind names, and creation paths. This pass follows the 21 functions assigned to `command-registry`, with emphasis on the global queue's initialization, tick, purge, and transition back into entity chains. Numeric kinds and masks remain literal when the recovered code does not establish their semantic names.

| Address | Function | Observed behavior and state flow |
| --- | --- | --- |
| `0x00401cb0` | `CmdKindByName` | Case-insensitively scans 27 names beginning at `DAT_004b327c`; on a match writes the zero-based index to the output and returns 1, otherwise returns 0 without writing the output. |
| `0x00401cf0` | `CmdRegistryInit` | Clears the 12-entry pool-block table, installs 31 AI task-type names and 27 AI priority names, fills 27 order values with `0x31` before applying explicit per-entry defaults (including value 55 for `AI_Priority_Crystal` at index 0), resets pool-growth state/free-list head, and sets `DAT_004b8060 = 1`. Called during engine initialization. |
| `0x00402000` | `CmdMarkMatching` | Walks global head list `DAT_004b3354`; for matching record kind, sets flag `0x10000` when either the optional payload at `+0x10` matches or the optional pair at `+8/+0xc` matches. |
| `0x00402060` | `CmdSetKind` | Stores the kind index at `+0x4c`; stores `DAT_004b32e8[kind]` at `+0x1c` unless record flags `+0x5c & 0x30000` are present, in which case it stores 99. |
| `0x00402090` | `PriorityEntryOrderValueSet` | Writes the supplied value into the 27-entry order table at `DAT_004b32e8`. Config loaders use this for named priority weights. |
| `0x004020a0` | `PriorityEntryStateSet` | Writes a state value into the 27-entry array at `DAT_004b7ff4[kind]`; priority config parsing is a caller. |
| `0x004020b0` | `PriorityEntryStateGet` | Returns `DAT_004b7ff4[kind]`, which is consulted by `CmdChainPurge` when deciding whether to refresh an entry's computed order. |
| `0x00402100` | `CmdFlushAll` | Nonzero argument clears bit 1 of `DAT_004b8060` and returns. Zero runs `LAB_004021f0` over entities, sets that bit while clearing both global list heads, zeroes the `0x12c0` cell-record counters and 27 priority-state entries. The callback and exact meaning of this state bit remain partly unresolved. |
| `0x00402220` | `CmdTickGlobal` | Computes a global order threshold from the priority values at indices 0, 1, and 4, then increments it; unless `DAT_004b8060` has its high bit set, runs `LAB_00404250`, marks eligible kind-10 records completed when their referenced entity's `+0x340` value is outside `(0,100)`, purges both global lists, resets global counters, then dispatches entity and pending-record scans (`0x004377b0`, `0x00405c10`, `0x00405950`). |
| `0x00402310` | `CmdChainPurge` | Removes records marked completed (`+0x5c & 0x20`) through `CmdFree`; for up to 300 remaining records it refreshes or suppresses priority values from the two tables and special-cases kind 8 cell commands. It sorts that gathered prefix using `LAB_00403f80`, relinks it, and leaves any remainder after the sorted prefix. Comparator details and the 300-record cap's gameplay impact remain unresolved. |
| `0x00402cd0` | `CmdCancelRef` | For a kind-`0x13` record, dispatches a callback scan when its referenced record is non-null. For other kinds, walks the global head list and sets completion/cancel flag `0x20` on matching kind-`0x13` records. |
| `0x00403700` | `CmdCancelByCoords` | Walks global head list and sets `+0x5c & 0x20` on kind-7 records matching the supplied selector; when coordinate matching is enabled, both record coordinates at `+0x20/+0x24` must match. |
| `0x00403750` | `CommandKindRecordIssueAndLink` | Allocates kind 7, copies supplied mode/parameters and a coordinate pair, chooses a random pair of float values when the optional vector is null (otherwise copies it), optionally sets `0x800`, links globally at the head as command kind 5, sets flag `0x400`, then calls `TerrainCellRecordCountUpdate` (`0x00403810`). See Finding 254 for its wider caller context. |
| `0x00403a50` | `EntityDrainCommandChain` | Unless `DAT_004b8060 & 2` blocks it, drains the entity's `+0x2f0` chain: completed/non-pending records are freed, records with flag `0x2000` move to the global tail list, and pending records move to the global head list. It clears the entity head and dispatches `LAB_00403ad0`. |
| `0x00404030` | `CmdLinkHead` | Pushes a record onto `DAT_004b3354`, stores the old head at `+0x60`, writes `0x44bb8000` at `+0x14`, clears completion bit 0 and sets flag `0x10`, stores `DAT_004b32e8[kind]` at `+0x1c`, and records the kind at `+0x4c`. |
| `0x00404070` | `CmdLinkTail` | Pushes a record onto `DAT_004b3358`, storing the prior tail-list head at `+0x60`, writes `0x44bb8000` at `+0x14`, and clears bits 0 and 4 of `+0x5c`. Despite its name this is a head insertion into the separately designated tail list. |
| `0x00404190` | `CmdChainWalk` | Recovered body walks links from entity `+0x2f0` through record `+0x60`; it has a recovered `void` signature and no visible return, though several callers consume it as a pointer to the chain's append position. Treat that apparent return contract as unresolved. |
| `0x004041e0` | `FUN_004041e0` | Removes the entity chain head; if its flags indicate pending global-head or tail routing and it is not completed, links it into that global list. Otherwise frees it. Returns the new entity-chain head. |
| `0x00406440` | `CmdAlloc` | If the free-list head is null, grows the pool; pops a 0x68-byte record through offset `+0x64`, zeroes 26 dwords, restores the self-link, stamps `timeGetTime()` at `+0x50`, stores the requested kind at offset 0, and returns the record. |
| `0x00406480` | `CmdFree` | Unless its second argument suppresses it, runs `CmdCancelRef`; frees owned data at `+0x40` unless flag `0x200` marks it externally owned; pushes the record onto the `DAT_004b31f8` free list through `+0x64`. |
| `0x004064c0` | `CmdPoolGrow` | Allocates `2^DAT_004b31fc` records of `0x68` bytes, stores the block base in a 12-slot table, increments the exponent on success, links the block records through `+0x64`, and prepends the final record to the current free list. |

## Lifecycle and boundaries

- The registry initializer and configuration loaders divide the names, order values, and per-priority state across separate tables: name strings at `DAT_004b327c`, order values at `DAT_004b32e8`, and state values at `DAT_004b7ff4`. The numbers are used by sort/purge and command creation, but are not themselves function pointers.
- Producers link records onto one of two global list heads. `CmdTickGlobal` processes both lists each simulation tick; `CmdChainPurge` first reclaims completed records and then reorders up to 300 survivors. Records may be created directly on a global list or promoted from an entity's `+0x2f0` chain by `EntityDrainCommandChain` or `FUN_004041e0`.
- `CmdFlushAll(0)` is a stateful reset, not simply a free-every-record loop: it invokes a callback scan, sets a guard bit, drops both global heads, and clears associated tables. `CmdFlushAll(nonzero)` only clears the guard bit. Exact ordering with entity-chain drains is not established here.
- Observed `DAT_004b8060` bits: bit 0 is set by `CmdRegistryInit` and the whole word is cleared by `HandleArrayReleaseAndReset`; bit 1 is set during `CmdFlushAll(0)`, cleared by `CmdFlushAll(nonzero)`, and blocks entity-chain drains and record removals; bit 2 is only observed as an additional `EntityTaskQueueFlush` guard; bit 31 suppresses the body of `CmdTickGlobal`. No writer for bits 2 or 31 appears in the decompiled references, so their broader lifecycle and names remain unresolved.
- Finding 10 remains the broader source for record fields, kind/name tables, and creator behavior. Findings 254, 366, and 369 describe adjacent terrain issue and scheduler paths. This pass does not infer the semantic names of record kinds or flag bits from these call paths alone.

## Unresolved slices

- Recover `LAB_00403f80`'s sort ordering and establish how entries beyond the 300-record prefix are ordered over subsequent ticks.
- Recover the entity callback bodies `LAB_004021f0`, `LAB_00404250`, and `LAB_00403ad0`; determine how each interacts with the flush guard and entity chains.
- Establish why `CmdChainWalk` callers use a value that the decompiled signature/body do not return, using disassembly or runtime evidence.
- Trace external or indirect writers of `DAT_004b8060` bits 2 and 31 and establish whether bit 0 is an initialization marker or a broader lifecycle state.
- Determine whether `CmdFlushAll(0)` intentionally discards record allocations after the callback scan or whether callbacks are required to rehome/free all entries first.

## Related findings

- [Finding 10](10-entity-command-primitives.md) documents the record layout, command names, and creator/dispatch paths.
- [Finding 254](254-command-sweeps-cell-issuing-config-priorities-statistics-and-directdraw-create-helpers.md) traces callers of priority setters and terrain command issue.

Direct evidence is in [the full decompile](../decompiles/full/LEGORR.full.c). Where caller usage conflicts with the recovered callee signature, this finding records the conflict rather than assigning a new contract.
