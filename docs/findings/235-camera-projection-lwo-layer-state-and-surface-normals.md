# Finding 235 — Camera Projection, LWO Layer State, and Surface Normals

Three remaining rendering helpers now have direct decompiles and caller
context.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4772d0 | CameraProjectPointToScreen | Calls the camera object to transform a 3D point, then divides the first two returned components by the third. Callers use those results as screen coordinates and compare them with viewport bounds. |
| 0x479530 | SurfaceTriangleNormalCalculate | Forms a cross product from three 3D points and normalizes the resulting vector. Terrain vertex construction and another geometry routine use it. |
| 0x474b30 | LwoLayerEnabledSet | Sets or clears a layer's high-bit enabled state and calls the layer interface's enable/disable method; cached layers use a separate helper. Terrain effects disable a source layer while a temporary surface is visible and restore it afterward. |

**Evidence.** Direct decompiles and the selected scene/render caller audit are in
[legorr-engine-helper-audit-render-expansion-9.bundle](../evidence/legorr-engine-helper-audit-render-expansion-9.bundle).
Direct decompile evidence is `ev_d65bb223e6ded6a8dd1841c84019ff5cb0af216dd0ac711956dc5d09e1b67ced`;
the exact caller records are `ev_4b3fe135164d99208a1fdc7efce60b05a9afb2b1787e086d81b600b7b3b65da8`,
`ev_e0b25a9c90639d592dcd481b1ff11ee29e33b8508078591cd5e49383ed77ec70`, and
`ev_c1e2e51acfbd310e37f69465934027fb445b987f8afadfb5e57946b5095c3263`.
Analyses used shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
