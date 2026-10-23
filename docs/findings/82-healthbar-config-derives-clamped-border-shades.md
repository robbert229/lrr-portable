# 82 — Health-bar configuration derives clamped border shades

The `ObjInfo` health-bar loader requires position and width/height pairs plus
border, background, and fill RGB values before it enables the configuration.
For the border RGB, it derives two additional colors at plus and minus 40% per
channel, clamping each derived channel to `[0, 1]`. A recognized vertical flag
sets bit `0x8`; successful setup always sets bit `0x2`. The loader does not
establish how or when the health bar is rendered.

**Observations from** REA/Ghidra 12.1.4 over the shipped `reference/LegoRR.exe`
(SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence bundle:
[`legorr-object-state-and-health-ui-expansion.bundle`](../evidence/legorr-object-state-and-health-ui-expansion.bundle).
Focused record: `ev_492cabc96e6351c989458396e2c59eaaa0efbbe4f8ffddde6fc26c1b55057983`
(`FUN_00459560`).

## Load gates and derived values

`FUN_00459560` reads `HealthBarPosition` and `HealthBarWidthHeight` as
two-value entries; a result other than the observed two-value parse result
causes setup to fail. It then reads `HealthBarBorderSize`, passing a built-in
fallback pointer when that lookup is absent. `HealthBarBorderRGB`,
`HealthBarBackgroundRGB`, and `HealthBarRGB` are subsequently parsed through
the three-channel color helper; failure of any of those lookups returns zero.

After the border color is read, the loader derives a lighter and darker
variant by multiplying each channel by `1.4` and `0.6`, respectively, then
clamps each result to the inclusive range zero through one. It reads
`HealthBarVertical`; a parsed value of one ORs `0x8` into the health-bar flags.
Every successful path then ORs `0x2` and returns one.

## Boundary

This is a static description of configuration parsing and derived color
values. The meaning of the flag bits, the rendering path, and the resulting
in-game appearance are not established here. No configuration file was edited
or replayed.
