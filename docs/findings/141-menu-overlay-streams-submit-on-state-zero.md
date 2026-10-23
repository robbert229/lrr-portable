# Finding 141 — Menu-Overlay Streams Submit on State Zero

When `FUN_004121b0` successfully opens a random front-end FLIC overlay, it queues that overlay's sound token with `FUN_00464f60(soundId, 0)`. The menu run loop (`FUN_00413b80`) calls `FUN_004651a0` every frame. That consumer submits the prior secondary-list entry through `FUN_00464fa0(alias, storedSelector)`, preserving the queued zero. `FUN_00464fa0` forwards it as the `FUN_0047ad70` stream-state selector. The shipped `FrontSFX_RockMonster`, `FrontSFX_Truck`, `FrontSFX_Sand`, and `FrontSFX_Slug` aliases are `@` streamed entries, so their FLIC-triggered sounds select stream state zero.

The queue adds a frame boundary between the FLIC-open event and sound submission: `FUN_00464f60` writes into the current list; `FUN_004651a0` drains the previous list, then rotates the current list into the next frame's pending slot. This means the overlay's sound does not enter the playback path in the same call that opens its FLIC. Finding 136 describes what happens if the selected stream state already has active playback: the new stream stops and releases it before starting the replacement.

| Stage | Function | Observed behavior |
| --- | --- | --- |
| Overlay open | `FUN_004121b0` | Queue the overlay sound token with selector `0` |
| Queue | `FUN_00464f60` | Store alias and selector in the secondary list |
| Menu tick | `FUN_00413b80` → `FUN_004651a0` | Drain previous list, then rotate current list |
| Playback | `FUN_00464fa0` → `FUN_0047ad70` | Forward selector `0`; `@` aliases use stream state zero |

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. The FLIC-open queue call and menu frame caller are in `docs/decompiles/full/LEGORR.full.c` at `FUN_004121b0` and `FUN_00413b80`; the sound queue writer, frame-list rotation, and playback submitter are at `FUN_00464f60`, `FUN_004651a0`, and `FUN_00464fa0`. The menu overlay selection/queue path and sound-name resolution are recorded in `docs/evidence/legorr-menu-background-audio.bundle` (`ev_94eb25021f731f4b8405f20d84c578b71584d7731e272f112a26882c4b1c4292`, `ev_0827c47223be9e7b1c8ac5500ab135596534fe61297d3e0c8773e143da96809b`). The writer and submitter are also recorded there (`ev_fbc624c0dcf6fb79a7fa07cd5d03d2473a3afc6434b1d81e19571d7ba5930d7c`, `ev_be8e1e65cbfba0701c22725feb605a1782556c1323859d2ca1f54a393cf230fa`). The four shipped aliases are in `Lego.cfg` under `reference/LegoRR1.wad`; their mappings are summarized in finding 23.
