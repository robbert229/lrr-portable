# Finding 305 — Entity Velocity and Scene-Position Accessors

Five accessors expose entity velocity state, world-center coordinates, and one scene-object flag. Flag bit meaning remains unresolved.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x471a40 | VelocitySlotPrimaryObjectGet | Returns the primary object pointer from a velocity slot. |
| 0x471a50 | VelocitySlotSecondaryObjectGet | Returns the secondary object pointer from a velocity slot. |
| 0x471c70 | VelocitySlotVectorRead | Copies a three-component vector from a slot record at `+0x44 + index*0xc`. |
| 0x471ca0 | EntityWorldCenterPositionRead | Gets an entity pose and adjusts its Z coordinate by half the entity height. |
| 0x473ba0 | SceneObjectFlagBit3Get | Returns bit 3 from the low flag byte at object offset `+0x14`. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Slot-object getters are used by pose dispatch `0x471a60`; the vector accessor is used by slot update `0x471d20`; and world-center positions feed the ray/visibility helpers `0x4718d0` and `0x471960`. No matching direct procedure-analysis bundles were found for these exact five targets; nearby caller records are not direct evidence for the accessor bodies. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
