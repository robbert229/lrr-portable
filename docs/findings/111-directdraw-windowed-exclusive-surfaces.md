# 111 — Windowed and exclusive modes build different DirectDraw surfaces

`FUN_0047c610` builds distinct presentation paths based on its mode argument. In windowed mode it calls DirectDraw's cooperative-level method with flags 0x8 and does not call SetDisplayMode. It creates a primary surface using descriptor caps 0x2200, then creates a separate explicit-size surface with caps 0x2040 and descriptor flags 0x7.

In exclusive mode it calls cooperative level with flags 0x11, then requests the selected width, height, 16-bit depth, and zero refresh/flags through SetDisplayMode. The primary descriptor uses caps 0x2218, flags 0x21, and one backbuffer; the routine obtains that backbuffer with GetAttachedSurface caps 4 instead of creating the windowed second surface. The selected-mode caller probes 640×480×16 and may also probe 800×600×16 and 1024×768×16.

**Evidence.** REA/Ghidra 12.1.4, shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. Bundle: `docs/evidence/legorr-graphics-mode-surfaces.bundle`. Focused records: `ev_a03764ac5379a5a84b61133561057ec2a178f538f11d46e8bf449c07b78a7f3b` (instruction sequence and mode flags); `ev_32fb14ef2009363722ff458f2625dc5824f713e6d217837831b392e75106a05a` (`FUN_0047c610`); and `ev_8ed3113394265240fd4b4b9288721fec3f37a49d3274cd1aca42fb3a8a422fea` (selected-mode probes and setup caller).

**Limitations.** Vtable slots and surface flags are statically decoded, not runtime traced; actual driver behavior is not established. Finding 15 notes the generic mode calls and fixed 640×480 concern; this finding records the separate surface construction branches.
