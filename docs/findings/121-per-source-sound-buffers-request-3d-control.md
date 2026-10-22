# 121 — Per-source sound buffers request 3D control

The sound-record loader passes each record through `FUN_0047b6c0` to
`IDirectSound::CreateSoundBuffer`. The descriptor created there has flags
`0x200f2`, which includes bit `0x10` (`DSBCAPS_CTRL3D`). The other descriptor
fields observed are size `0x14`, byte length from the sound record at `+0x104`,
and format pointer from `+0x11c`. On success, the returned buffer pointer is
stored at record offset `+0x120`.

The global distance helpers `FUN_0047b4a0` and `FUN_0047b4d0` walk the linked
list rooted at `DAT_00506684`. For each node they load a buffer pointer from
node offset `+8` and call vtable slot `+0x44` or `+0x40`, passing the new value
and a trailing zero. The source setup therefore requests 3D control on created
buffers, while the configured distance changes propagate across all buffers
already represented in the list. This connects the per-source buffer creation
path to the configuration path summarized in finding 62 and the listener
interface acquired in finding 119.

## Evidence

REA/Ghidra 12.1.4 analysis of shipped `source/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
Bundle: `docs/evidence/legorr-sound3d-per-source.bundle`.

- `ev_42a48d242a5c0f775c2ebfcbf3fe6538712d8722f03b1612835635c5ed8d561f` — descriptor flags, fields, and returned buffer storage in `FUN_0047b6c0`.
- `ev_dc7c21b926f91ab25267c5b44752b55a18606e88e98c5e8c85f4e7304252d7e9` — sound-record loading calls `FUN_0047b6c0` before filling the buffer.
- `ev_d92fea5a9a70d200c84f3c4459ea55dae61215ac16bd6a87c8ffee9bdadfad41` and `ev_021a2d15b3edd7ad713e9fcddb9f492d80bf671c9dec6cadccbf8e5c8752e098` — iteration over tracked sound-buffer nodes and calls through `+0x44` / `+0x40`.

## Limits

The binary directly shows descriptor flag value `0x200f2` and setter vtable
slots. Identifying bit `0x10` as `DSBCAPS_CTRL3D`, and the slots as the
DirectSound minimum- and maximum-distance setters, uses DirectSound interface
and flag definitions. Static evidence does not show audible output, whether
all records are in the linked list at every config update, or which individual
sounds use 3D positioning.
