# 420 — Renderer and scene lifecycle: root, pooled objects, hooks, and teardown

This finding traces the eight functions assigned to `renderer-scene-lifecycle`.
Together, they initialize a global DDiScene object system, allocate its root and
pooled records, configure two animation callbacks, preserve a shared-texture
directory, and release the system's pooled storage. Findings 25, 26, 29, 32,
and 34 cover details of the object model, callback behavior, and shutdown edge
cases; this finding connects those pieces to their lifecycle callers.

Evidence is the checked-in full decompile at
[`docs/decompiles/full/LEGORR.full.c`](../decompiles/full/LEGORR.full.c),
including the function/address headers cited below. It establishes static
control and data flow, not runtime behavior after a failed initialization.

## Lifecycle map

| Address | Function | Observed responsibility |
| --- | --- | --- |
| `0x472710` | `EntityVisualTypeTablesInit` | Clears and initializes DDiScene globals and kind strings, resets pool and hook state, allocates the root object, and calls its vtable method at `+0xf8` with `1`. |
| `0x472800` | `DDiSceneObjectPoolShutdown` | Destroys live objects in each pool block, frees those blocks and auxiliary allocations, and resets the free-list head and sample-hook gate. |
| `0x4728c0` | `SharedTextureDirectorySet` | Duplicates its argument into `DAT_0076cd88`. |
| `0x4728e0` | `RendererFlagBit6Set` | Sets or clears bit `0x40` in `DAT_0076cd94`. |
| `0x472900` | `DDiSceneEndHookRegister` | Stores the scene-end callback and user value in `DAT_0076cd80/84`. |
| `0x472920` | `DDiSceneSampleHookRegister` | Stores the sample callback and user value in `DAT_0076cd78/7c`, and sets bit `0x40` in `DAT_0076cd94`. |
| `0x472940` | `SceneRootObjectGet` | Returns the root pointer in `DAT_0076add4`. |
| `0x472950` | `DDiSceneObjectAllocate` | Pops a `0x2c`-byte record from the DDiScene free list, initializes three interface-backed fields and their animation-hook records, or destroys the partial record and returns null on factory failure. |

## Initialization and root object

`EntityVisualTypeTablesInit @ 0x472710` first zeroes `0x806` dwords beginning
at `DAT_0076ad80`, then separately clears the first 20 pool-bank pointers. It
writes the string pointers and defaults used by the DDiScene kind table around
`DAT_0076add8`–`DAT_0076ae10` (the table's dispatch use is detailed in finding
25). It saves its argument at `DAT_0076ae20`, clears the free-list head
`DAT_0076add0`, sets the pool-block counter `DAT_0076cd90` to zero, initializes
`DAT_0076cd94` to `1`, clears the auxiliary count `DAT_0076cd74`, and nulls
`DAT_0076cd88`.

The initializer then calls `DDiSceneObjectAllocate(NULL)`, stores the returned
record in `DAT_0076add4`, and invokes the vtable method at offset `+0xf8` on
the first pointer in that record, passing `1`. It does not check the allocator
result before dereferencing it, so this path has no local recovery if root
allocation returns null. `SceneRootObjectGet @ 0x472940`
is a direct getter for `DAT_0076add4`. For example,
`EntitySceneObjectTreeAttach @ 0x44b680` allocates an object from an entity's
component, gets the root, and passes both to `FUN_00475710` before attaching
the object's handle to the entity record. These call sites establish the
root's use as a scene context; they do not identify the exact native interface
type of the root's first pointer.

## Object allocation and pool growth

`DDiSceneObjectAllocate @ 0x472950` reads two optional values from its input
record (`param[0]` and `param[2]`); a null input supplies zero for both. If the
free-list head `DAT_0076add0` is empty, it calls
`DDiSceneObjectArrayGrow @ 0x4759f0`. The grow function calculates the block
record count as `1 << DAT_0076cd90`, allocates that count times `0x2c` bytes,
stores the block in the corresponding slot beginning at `DAT_0076ad80`, and
increments the counter when allocation succeeds. It links each record's word
at `+0x28` to the next free record and joins the last record to the prior free
list.

The first block therefore contains **one 0x2c-byte record**: initialization
sets the counter to zero, so the first count is `1 << 0`. Later blocks contain
2, 4, 8, and so on, subject to successful allocation. This corrects the
earlier description in finding 25 §2, which divided a 64-byte block by the
record size; the allocation expression is a record count multiplied by
`0x2c`, not a fixed 64-byte allocation.

Allocation pops the free-list head through its `+0x28` link, clears eleven
dwords, and writes the record's own address back to `+0x28` as a live-record
marker. It then calls the factory stored in `DAT_00505914` three times through
vtable offset `+0x10`, passing the two input values and previously returned
pointer as factory arguments. On success, it calls
`DDiSceneAnimationHookCreate @ 0x475f70` once for each of the three resulting
fields. The decompile shows three interface pointers and three hook-creation
calls, but this trace does not prove those interfaces' native types or that the
fields should be described as transform channels; see finding 25 for the
broader object-layout evidence. If one of the factory calls fails, the
allocator calls `DDiSceneObjectDestroy @ 0x472a40` on the partial record and
returns null.

## Hook slots and renderer gate

The two registration functions are simple global-pair setters:

- `DDiSceneEndHookRegister @ 0x472900` writes callback/user values to
  `DAT_0076cd80/84`.
- `DDiSceneSampleHookRegister @ 0x472920` writes them to
  `DAT_0076cd78/7c` and ORs `0x40` into `DAT_0076cd94`.
- `RendererFlagBit6Set @ 0x4728e0` sets or clears only bit `0x40` of that
  gate word.

`GameConfigInit @ 0x41fb50` initializes the scene root through
`EntityVisualTypeTablesInit`, then registers `LAB_0043f130` as the scene-end
callback. Its later failure path reaches `DDiSceneObjectPoolShutdown`; the
registrar's pair is not cleared there. Finding 34 documents the resulting
static fact and the unresolved question of whether a scene-end callback can
actually run after that failed-init path.

The SFX-bank initializer `FUN_00464740 @ 0x464740` registers
`LAB_00464c20` through `DDiSceneSampleHookRegister`. The sample dispatch and
the distinct scene-end dispatch are traced in finding 26. `EntityCreateByType
@ 0x437d00` calls `RendererFlagBit6Set(0)` after obtaining an entity record,
and calls `RendererFlagBit6Set(1)` on its common completion path. This proves
the bit is cleared during that creation path and set again before its normal
return; interpreting this as suppression of sample callbacks during entity
construction is plausible from finding 26's gate check, but remains an
inference about intent.

## Shared texture directory

`GameConfigInit @ 0x41fb50` reads `SharedTextures` from `Lego.cfg`. When the
value is non-null, it calls `SharedTextureDirectorySet @ 0x4728c0`, which
duplicates the string into `DAT_0076cd88`; the initializer then passes the
config value to `SharedTextureRootRegister @ 0x4805b0`. Finding 13 §6 traces
the broader preload path and subsequent use of the shared-texture root. The
setter's body establishes ownership of a duplicate in `DAT_0076cd88`; it does
not by itself establish every downstream consumer of that duplicate.

## Shutdown and remaining state

`DDiSceneObjectPoolShutdown @ 0x472800` walks the 20 pool-bank slots. For each
allocated block at slot `i`, it visits `1 << i` records at stride `0x2c` and
calls `FUN_00472a50(record, 1)` only when the record's `+0x28` word equals its
own address. It then frees the block. Afterward it sets `DAT_0076add0` and
`DAT_0076cd94` to zero, frees `DAT_0076cd74` auxiliary entries from every
other dword beginning at `DAT_0076ae34`, and frees the duplicated directory at
`DAT_0076cd88` when non-null.

The shutdown body does not reset `DAT_0076cd90`, `DAT_0076cd74`,
`DAT_0076cd88`, or either callback/user pair (`DAT_0076cd78/7c` and
`DAT_0076cd80/84`) after freeing their associated storage. Findings 32 and 34
already document the callback implications. The static decompile does not
establish whether these globals are read before the next initialization
overwrites them, nor whether any external object reference survives pool
teardown.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Initialization clears the DDiScene arena, establishes kind strings and defaults, allocates a root, and invokes its `+0xf8` method without a null check | observed (`0x472710`) |
| The root getter returns `DAT_0076add4`, and `EntitySceneObjectTreeAttach` uses it as a scene context | observed (`0x472940`, `0x44b680`) |
| Object records are `0x2c` bytes; the pool grow count is `1 << DAT_0076cd90` records | observed (`0x472950`, `0x4759f0`) |
| First pool block has one record, then successful blocks double in count | observed (initializer counter and grow expression) |
| Hook registrars store separate callback/user pairs; sample registrar also sets gate bit `0x40` | observed (`0x472900`, `0x472920`) |
| Entity creation clears bit `0x40` and sets it on the common completion path | observed (`0x437d00`); intended reason is inferred |
| Shutdown frees live objects, pool blocks, auxiliary entries, and the duplicated shared-texture directory | observed (`0x472800`) |
| Native types/semantics of the three interface-backed record fields | not established by this slice; see finding 25 |
| Whether callbacks or stale references can be used after failed initialization or shutdown | unknown; see findings 32 and 34 |
