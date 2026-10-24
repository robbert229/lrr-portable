# Finding 493 — Effect Timers and Message Dispatch Globals

`0x004DE548` gates scroll-scale changes: the setter stores a value here, and the scroll-scale helper allows an update when this value is zero. The value domain and owner are not recovered.

`0x004EAE08` and `0x004EAE0C` hold the lower and upper random delay bounds used by the drift-effect update. When its countdown expires, the update chooses a new delay from this range.

`0x004EAE40` and `0x004EAE44` hold the lower and upper random waypoint hold-duration bounds. `RandomWaypointTimerStepApply` selects a countdown from this range before moving to another waypoint.

`0x004EAE54` is the lower bound for random waypoint travel duration. The paired upper bound is `0x004EAE58`; `WaypointVectorRandomScaleApply` selects a duration from the pair and derives per-axis travel rates from it.

`0x004FF190` begins a fixed message-name string-pointer table populated by `MessageStringTableInitialize` and searched by name lookup. `0x004FF980` stores global system-state flags exposed through a getter and replaced through a setter.

`0x00544A08` stores the completion callback supplied when starting a CD audio track. The watchdog invokes it after detecting that playback stopped and attempting any configured restart.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
