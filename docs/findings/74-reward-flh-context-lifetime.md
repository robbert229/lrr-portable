# 74 — Reward `.flh` contexts are lazy per-task resources with transition cleanup

The reward display opens a task's `.flh` only when its flic display lane is
enabled and the task has no context yet. The context is retained in that task
record while the reward loop runs. Reward exit closes each loaded context and
clears its slot; reward progression also has a close-and-reopen path. This
completes the loader-lifetime seam left open by findings 09 and 21.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-flh-playback-state-machine.bundle`.
Focused IDs: `ev_5c94fcf74470a3c30367dd9866366dd1a0eb1d70948f534e6e0ccfa2c889ff33`
(`FUN_00461790`), `ev_ca8edd510a6624a6c97f371655f9e9f4ecfd3ee6ddac9aa7917e32e7adfcdbdc`
(reward loop exit), `ev_856b868923f19e539ce762d1ffd51c07fc17cb3a41560071b7e72d8acec6163e`
(`FUN_00462460` reload helper), `ev_22739f7db92a56c49d8a7728cf1e0deba3f0a1058a1ba0db258e628493733696`
(`FUN_00462390` transition caller), `ev_1aeab33b8e4b2ef2b04f85da1e5ea900cde86ec636a59802e182b4067056d673`
(`FUN_004624A0` progression cleanup), `ev_cd74c61a333939992691328bc5ac6e4cae2f6a6b411972c29967b07bdcdead85`
(`FUN_00483F00` close path), `ev_faf4a8805fd4fa76e0c81d83ba3c85dee5c4ba3274abdeeb4a198eeb59cf061e`
(`FUN_0045A2E0` pointer overlay loader), and `ev_0e62a2828d89032917417df6e92c2c8e38673051191dc9222052c89997762c21`
(`FUN_0045CC70` overlay tick caller).

## Reward display ownership

For a task whose record flag `0x04` and active display mask both select flics,
`FUN_00461790` checks the task's context slot at record `+0x1fc`. If it is zero,
the function opens the configured path at `+0x16c` with mode flags `0`, storing
the resulting context back at `+0x1fc`. It then ticks that context through
`FUN_00484070`; finding 73 records the frame-index advance gate. This is
per-task lazy loading, not a shared global `.flh` player.

At the end of the reward loop, `FUN_00461410` walks all ten task records. For
each record with flic flag `0x04`, it calls `FUN_00483f00` on the context at
`+0x1fc` and writes zero to that slot. `FUN_00462460` provides a second
path: close the same task context, then reopen the task path with mode flags
`0`. Reward progression/refresh branches reach this helper; `FUN_004624A0`
also walks the ten task records and calls it while clearing stale entries.

The close routine is mode-sensitive: it frees the preloaded data pointer for
RAM mode or closes the stream handle for streaming mode, then frees the common
scratch buffer at context `+0x6e0`. Its analyzed body does not show a release of
the outer `0x6e8` context allocation, so this finding makes no claim about when
or how that allocation is reclaimed.

## Separate overlay lane and hook boundary

The pointer overlay takes a different lifetime path. `FUN_0045a2e0` first
tries the still-image loader; if that does not produce a surface, it opens an
`.flh` context with an open-mode argument of `3` and stores a type tag for the flic lane.
`FUN_0045a730` then advances/draws that table entry. Per finding 21, those mode
bits select streaming plus looping, while reward mode `0` is associated with
RAM preload without looping. The call-site values and resulting context bits
are separate observations; the open call alone does not establish every
intermediate assignment.

The inspected `.flh` tick calls surface lock/unlock, the FLIC stepper, and a
blit; it has no DDiScene sample callback or direct audio call. The LWS/ACT
sample hook in findings 26 and 42 belongs to a separate scene renderer path.
This does not rule out unrelated sound triggered by reward/task logic.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Reward `.flh` path loads lazily into each task's `+0x1fc` slot | observed (`FUN_00461790`) |
| Reward-loop exit closes flagged contexts and clears their slots | observed (`FUN_00461410`) |
| Task refresh/transition can close and reopen its `.flh` context | observed (`FUN_00462460`, `FUN_004624A0`) |
| Reward and pointer paths pass open arguments 0 and 3 respectively; their contexts follow the RAM/non-loop and stream/loop paths summarized in finding 21 | derived by joining the call sites with finding 21's mode analysis; the intermediate assignment is not closed here |
| Lifetime release for the outer context allocation | unknown; not present in the analyzed close body |
| Any relationship between `.flh` frames and audio samples | unknown; no such hook appears in the inspected FLIC tick path |
