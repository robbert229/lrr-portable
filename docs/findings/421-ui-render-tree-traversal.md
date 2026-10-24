# 421 — UI render-tree traversal builds indexed interfaces and processes entries

Finding 375 first catalogued these four functions. This focused trace connects
them to the LWSC resource-load path, records the index and parent/child flow,
and corrects the earlier claim that `UIRenderTreeLayoutAndRelease` clears or
releases the tree. Its body instead invokes the time/update routine at zero.

Evidence is the checked-in Ghidra decompile at
[`docs/decompiles/full/LEGORR.full.c`](../decompiles/full/LEGORR.full.c), plus
the cited function and caller-reference records from the evidence bundles.
Computed interface calls and the exact native interface/property roles remain
unresolved; claims below distinguish the visible call flow from interpretation.

## Function map

| Address | Function | Observed work |
| --- | --- | --- |
| `0x487990` | `UIRenderTreeLayoutAndRelease` | Allocates an index-to-interface table and makes a registration pass followed by an entry-processing pass over linked roots. It then calls `FUN_00487ba0(tree, 0)`. |
| `0x4880d0` | `UIRenderElementChildrenEnumerate` | Looks up an interface by the entry's assigned index, obtains a collection-like value and its count, and iterates its entries through `FUN_00483420`. Its decompiled prototype and an argument used from caller context are incomplete. |
| `0x488900` | `UIRenderTreeEntryProcess` | Looks up the indexed interface, optionally applies subrecord zero, resolves a resource for eligible entries, applies a resource flag, consumes an optional value, then recurses through child entries. |
| `0x4889a0` | `UIRenderEntrySubrecordApply` | Selects an entry subrecord by index and passes three pointers from it to `SceneNodeTransformMatrixApply @ 0x4882e0`. |

Direct function-body records: `ev_8da9fc512857ccb8b71544dadf2d362b31b0aa7c756a226f99ba6628869eb8e0`
for `0x487990` and `ev_a8edf38fe9f2a9846bb11da102c9bdd9e08ac7e7c4f701f7b1f6c1307154e753`
for `0x488900`, both in
`docs/evidence/legorr-ui-render-third-pass-2026-10-23.bundle`. Caller-reference
records for `0x4880d0` and `0x4889a0` are `ev_9034986384c069331ac32f8e4726ea2b025b00771f41d5e8ce45c1a2d2788b29`
and `ev_91327311e9bb50006e451f2ece29cb7ee9cbf8e531be5246b3794a5f023763d8`,
respectively, in
`docs/evidence/legorr-engine-helper-audit-text-io-expansion-9.bundle`; those
records establish incoming references, while their body details below come
from the checked-in decompile.

## Load-to-tree path

`LwscSceneLoad @ 0x4765c0` calls `LoadLWSCScene @ 0x4869f0`. When that returns
a non-null tree, the caller passes it to `UIRenderTreeLayoutAndRelease(tree,
param_2)`, calls `FUN_00487ba0(tree, 0)` again, optionally writes the tree
length from `FUN_004877d0`, and creates a resource callback context with
`FUN_004895c0`.

Inside `UIRenderTreeLayoutAndRelease`, the 16-bit count at tree offset `+0x24`
is multiplied by four and allocated as a pointer table stored at `+0x14`. The
root-entry list begins at `+0x1c`; its siblings link through entry offset
`+0x30`. For each root, the function calls
`SceneHierarchyInterfaceRegisterRecursive @ 0x4887c0`, then
`UIRenderTreeEntryProcess @ 0x488900`. The final call in this function is
`FUN_00487ba0(tree, 0)`. The LWSC caller immediately calls the same update
function at time zero a second time. Both calls are direct evidence; whether
the repeated initial update is required or repeats a visible side effect is
not established by these bodies.

## Registration and recursive entry processing

`SceneHierarchyInterfaceRegisterRecursive` creates an interface through the
factory in `DAT_00505914` and stores it in the pointer table at
`tree+0x14 + index*4`. It writes the same sequential index into the entry's
16-bit field at `+0x06`, increments the index, builds a name from the entry
name and another field, replaces characters rejected by mask `0x157` with
underscores, and sends that name through the interface vtable at `+0x20`. It
then walks children from entry offset `+0x2c`, following each child's next
link at `+0x30`. These operations establish the table/index relationship and
recursive shape; the exact interface type and the generated name's external
meaning are unknown.

`UIRenderTreeEntryProcess` uses the stored `+0x06` index to retrieve the
interface pointer. When the 16-bit field at entry offset `+0x24` equals `1`,
it calls `UIRenderEntrySubrecordApply(entry, 0)`. That helper computes
`entry+0x18 + index*0x28` and passes pointers at subrecord offsets `0`,
`0x0c`, and `0x18` to `SceneNodeTransformMatrixApply`. The latter ultimately
calls interface vtable slot `+0x38` with a matrix-like value. The three
subrecord field roles are not proven by this call alone.

When entry flag bit `0` is clear, the processor calls
`ModelResourceResolveOrLoad @ 0x4885c0` with the tree context at `+0x08`, the
entry name, and its indexed interface. If the call returns a resource and
entry flag bit `2` is set, it ORs `0x01000000` into the returned resource's
word at `+0x2c`. If entry field `+0x1c` points to a value and the 16-bit field
at `+0x26` is zero, it passes the pointed-to dword to `UIRenderElementChildrenEnumerate`
and clears the pointer field afterward. Finally it recursively processes
children from `+0x2c`, following sibling links at `+0x30`.

## Collection enumeration and open questions

The decompiled `UIRenderElementChildrenEnumerate` resolves the interface
through the tree's `+0x14` table using the entry's `+0x06` index. It calls
interface slot `+0x80` to obtain temporary values, obtains a collection-like
pointer through slot `+0x1c`, reads its first dword as a count via
`FUN_00482210`, then loops from zero to that count and calls
`FUN_00483420(collection, index, temporary, 4)`. The latter is named
`ScreenEffectItemOpacitySet` elsewhere and its body treats its third argument
as a float, while this decompile prints the temporary as pointer-typed. The
exact argument/value relationship is therefore unresolved; this trace does
not claim what opacity or child property is ultimately applied.

The caller-reference evidence identifies `SharedObjectFloatTrackInterpolateApply
@ 0x488070` and `UIRenderTreeEntryProcess @ 0x488900` as callers of
`UIRenderElementChildrenEnumerate`. The former interpolates values from a
track before calling it; the latter dereferences the pointer stored in entry
field `+0x1c` and passes the pointed-to dword. The source does not explain how these caller values
correspond to the helper's temporary interface value.

## Focused follow-up slice

Disassemble or otherwise resolve the `0x4880d0` call ABI and interface methods
to determine how its temporary value reaches `ScreenEffectItemOpacitySet`,
then trace why the loaded-tree path runs `FUN_00487ba0(tree, 0)` twice and
whether its timed event callbacks produce duplicate effects. Existing
evidence proves the calls but not their intended relationship or runtime
consequence.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| `LwscSceneLoad` loads a tree and invokes `0x487990`, then `FUN_00487ba0(tree, 0)` again | observed (`0x4765c0`) |
| Tree index table uses the count at `+0x24`; recursive registration writes indices at entry `+0x06` and interface pointers into the table | observed (`0x487990`, `0x4887c0`) |
| Entry processing looks up the indexed interface, conditionally applies subrecord zero, resolves a resource, applies a flag, and recurses | observed (`0x488900`) |
| `0x4889a0` selects a `0x28`-byte subrecord and forwards three field pointers to `0x4882e0` | observed (`0x4889a0`) |
| `0x4880d0` enumerates a collection and calls `0x483420` with mode `4` | observed decompile; exact value/type relationship unresolved |
| The two time-zero update calls are intentional and side-effect free when repeated | unknown |
| Native interface type, generated-name meaning, and subrecord field roles | unknown |
