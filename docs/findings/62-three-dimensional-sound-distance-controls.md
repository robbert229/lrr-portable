# 62 — Three-dimensional sound distance controls are live buffer settings

The shipped executable keeps a minimum and maximum distance value for its
DirectSound sound buffers, and routes the `RollOffFor3DSounds` setting to the
listener interface. A separate mode switch changes the minimum distance for
all tracked buffers. These are static configuration and interface-call
observations; no acoustic curve or audible result was measured.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-sound-instance-spatial-expansion.bundle`.
Focused IDs:

- `ev_283012591a3e636bd693bae7155afd79c42c71d389e899ba5069fb0a999260dd`
  (`FUN_0047a640`, initial sound-system values),
- `ev_9cf0ef11e49abe4974600709d27cddc78521eee9c94f6ba65c1a2b8c2731b1f4`
  (`FUN_0041fb50`, configuration reads and setting calls),
- `ev_e56a96b67810198e09b52235915ee16b7e89d1712bd8ae7c2a7432c9421593cf`
  (`FUN_004295f0`, mode-dependent minimum-distance updates),
- `ev_d9ec352e70b78c76aba1d0dbc244857ae21b37768a42ad3e73075372f9730ed3`
  (`FUN_0047b4a0`, buffer `+0x44` setter),
- `ev_10aca7ed0db89d60a8e03ebd6f64ac6cf9c1d1f93900643e2ca9a93cc7da3e8a`
  (`FUN_0047b4d0`, buffer `+0x40` setter), and
- `ev_d09cc215f59a87d3aed1e91e44f5caf83ba9b1f7c0df6ff45f6cc49f34ef2d91`
  (`FUN_0047c100`, rolloff clamp and listener call).

## Values and update paths

At sound-system initialization, `FUN_0047a640` assigns `1.0` to
`DAT_0050668c` and `200.0` to `DAT_00506690`, then initializes the sound
backend. The helper `FUN_0047b4a0(value)` stores a new `DAT_0050668c` and calls
vtable slot `+0x44` on each node in the tracked sound-buffer list.
`FUN_0047b4d0(value)` does the same with `DAT_00506690` and slot `+0x40`.
These offsets match the DirectSound 3D-buffer min-distance and max-distance
setter slots; that interface-method naming is inferred from the vtable layout.

During engine setup, `FUN_0041fb50` reads `MinDistFor3DSoundsOnTopView`,
`MaxDistFor3DSounds`, and `RollOffFor3DSounds`. The minimum-distance value is
stored in `DAT_00556ec4`, with `150.0` used when the parsed value is zero. A
nonzero maximum-distance value is sent to `FUN_0047b4d0`; a zero value leaves
the initialized value in place. The rolloff path first supplies `1.0`, then
passes a nonzero parsed value to `FUN_0047c100`, which clamps it to `[0, 10]`
before calling listener vtable slot `+0x3c`.

`FUN_004295f0` switches the minimum-distance value applied to all tracked
buffers: its `param_1 == 0` branch supplies `50.0`, while its `param_1 == 1`
branch supplies `DAT_00556ec4`. The key name includes “OnTopView”, but the
meaning of the numeric mode values is not established by this routine alone.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Sound initialization sets distance globals to `1.0` and `200.0` | observed (`FUN_0047a640`) |
| The config keys feed the stored minimum, maximum, and rolloff values | observed (`FUN_0041fb50`) |
| Minimum and maximum values are sent to every tracked buffer through `+0x44` and `+0x40` | observed; DirectSound method names inferred from vtable layout |
| Rolloff is clamped to `[0,10]` before the listener call | observed (`FUN_0047c100`) |
| Mode branches `0` and `1` apply `50.0` and the configured minimum, respectively | observed (`FUN_004295f0`) |
| The resulting audible curve, units, and exact user-facing meaning of mode `0`/`1` | unknown; static calls do not measure acoustic output |
