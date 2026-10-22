# 102 — Camera settings feed pointer-pan acceleration and follow bounds

REA/Ghidra 12.1.4 analysis of the shipped `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`). Evidence
is in [`legorr-camera-control-focused.bundle`](../evidence/legorr-camera-control-focused.bundle).

During startup, `FUN_0041fb50` reads the config entries `CameraSpeed`,
`CameraDropOff`, `CameraAcceleration`, and `MouseScrollIndent`, converts their
values, and passes them to `FUN_00435e0e`. That setter copies them respectively
to `DAT_005560a0`, `DAT_005560a4`, `DAT_005560a8`, and `DAT_005560ac`.
[Evidence: `ev_cb54c6ff3d56668d42e47739c778f397133cc9e9b007695b8a6e21653dd9b183`,
`ev_ef4750477e72aeddc4f5557aec82ff0891840f2b796eefe57992660f53f4964b`]

The pointer-pan helper `FUN_00436b87` accumulates the incoming movement amount
using `DAT_005560a8`, caps that accumulated amount at `DAT_005560a0`, scales the
movement vector by the capped amount, then adds it to the view's stored offset.
The follow-mode branch in `FUN_00435f5c` scales its current offset by
`DAT_005560a4`; its grid-boundary checks compare picked tile coordinates to
`DAT_005560ac`. Together, these uses tie the named configuration values to the
pan acceleration/speed ceiling and the follow camera's edge handling. [Evidence:
`ev_2cb4c64e593318467e712f388ead1aad45c0db28c3ae38ead855ef909c0785f5`,
`ev_5176270c3a003e4b9aae88df79ae9427d1fe692285b2200dde6eb2be7127f81b`]

The same startup routine reads `MouseScrollBorder` into `DAT_00556e98` and uses
5 when the parsed value is zero. Finding [61](61-cursor-screen-ray-camera-routing.md)
shows that global defining the cursor inset rectangle used by the gameplay
camera path. [Evidence:
`ev_c45933b1a41fb243efc504be77bdfe7d2a349a2b3b7bbdcedf1a501006262bfb`]

## Limits

The config names are observed string-backed keys, but no runtime config was
loaded or varied here. The functions do not establish the units or user-visible
scale of these values. The cursor inset path is static evidence; its actual
screen behavior was not replayed.
