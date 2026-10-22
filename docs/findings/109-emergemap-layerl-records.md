# 109 — EmergeMap values populate six-slot LayerL records

The level loader reads the `EmergeMap` layer and passes it as argument 15 through the map-load pipeline to `FUN_0042c120` (`LoadLayerL`). For each nonzero cell value `c`, that routine computes a zero-based record index `(c-1)/6` and slot `(c-1)%6`, growing and zeroing a 0x4c-byte record as needed.

Slot 0 stores the cell's x/y coordinates at record offsets +4/+8 and ORs voxel flag 0x80. Slots 1–5 store x/y at `record + slot*0x0c + 4/+8`, write 1 at `record + (slot+1)*0x0c`, and OR voxel flag 0x40. Values 1–6 therefore address one six-slot record, values 7–12 the next, and so on.

An absent or unopenable layer returns success without allocating records; a dimension mismatch returns failure. These are loader return paths only.

**Evidence.** REA/Ghidra 12.1.4, shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. Bundle: `docs/evidence/legorr-emergemap-layerl.bundle`. Focused records: `ev_22cae969db6b45e81e1404b6739355a517a1413e3123007025500e7da2e3c359` (LevelLoad supplies `EmergeMap`); `ev_f891b378d94b16107401ad03b0ea7ed55a7187fd8c99c3528021f5dca1f7698b` (pipeline forwards argument 15); and `ev_04c0cf439e4040f29e78465553258fe832e39ffd8c8b7df74436bce00682e145` (record and voxel writes).

**Limitations.** The consumers of the appended record array are not traced here, so the records' gameplay meaning remains unknown. This closes the `LoadLayerL` open item in finding 07 without assigning a domain name to the records.
