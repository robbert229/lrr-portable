# 60 — Win32 cursor sampling stores clamped client coordinates

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence bundle:
[`legorr-cursor-routing-expansion.bundle`](../evidence/legorr-cursor-routing-expansion.bundle).
This expands the native input stage left open in findings [58](58-win32-message-pump-filter.md)
and [59](59-directinput-key-state-to-text.md); it does not reuse the keyboard table as a
mouse source.

`FUN_0047f010` calls `GetClientRect`, converts the client origin `(0,0)` to
screen coordinates with `ClientToScreen`, reads the cursor in screen coordinates
with `GetCursorPos`, then subtracts the converted origin. It clamps the resulting
coordinates to `0..DAT_00505924-1` and `0..DAT_00505928-1`, respectively, before
storing them in `DAT_0076ab24` (x) and `DAT_0076ab28` (y). The helper writes the
position and deltas only when `GetClientRect` succeeds. [Evidence:
`ev_07a445ebcdf1e8e7e9c7893d6b2d117380c886e07ae495f74ec2c87ee1aaa89e`]

The same helper computes per-sample deltas in `DAT_0076ab34/ab38` by subtracting
the previous stored x/y from the new clamped x/y. The main frame loop calls the
Win32 message pump, the DirectInput key-state poll, then this cursor sampler in
that order. This establishes code order within the loop, not an OS event-to-frame
latency. [Evidence:
`ev_07a445ebcdf1e8e7e9c7893d6b2d117380c886e07ae495f74ec2c87ee1aaa89e`,
`ev_dab0eeeb42997d0b8f6a70f83b3b33f2b0d631471c4e432d655868d720bf00c7`]

## Caveats

- The clamp limits are stored globals; this analysis does not prove they track
  the live `GetClientRect` dimensions after a resize.
- `FUN_0047f010` does not itself check focus or acquire a mouse device. The
  evidence supports a Win32 cursor-position read, not DirectInput mouse polling.
- Coordinates are client-relative after subtracting the client origin, then
  clamped. DPI scaling and display-coordinate behavior were not tested.
