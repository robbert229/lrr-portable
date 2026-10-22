# 48 — DirectDraw and D3DRM renderer setup modes

*Target: `LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), Ghidra 12.1.4 via REA.*

Finding 22 established that the D3DRM path requests a device and viewport without requesting a HAL device. This finding fills in the preceding DirectDraw setup and the bridge into that renderer.

## Observed setup path

`FUN_0047c610` (`0x47c610`, 701 bytes) consumes a mode value and optional records, filters each optional record on its low flag bit, saves the selected dimensions/window-related values in globals, then creates DirectDraw. After querying the DirectDraw interface, it sets cooperative level through vtable slot `+0x50`. When `param_1` is nonzero it also calls slot `+0x54` with the selected width and height before creating the primary surface. When zero, it requests a second surface through the surface-creation method instead. Both paths then call `FUN_0047cee0` and `FUN_004781d0`; only the nonzero path hides the cursor on success.

The nonzero path is consistent with a fullscreen/display-mode path and the zero path with a windowed path, but those mode labels are analyst inferences from the call sequence. The argument meanings and DirectDraw vtable method identities are not fully recovered in this dossier.

`FUN_004781d0` (`0x4781d0`, 314 bytes) creates the D3DRM object, queries the DirectDraw interface for the device wrapper, then calls that wrapper's `+0x3c` method with the selected surface and D3DRM object to populate `DAT_00505918`. It then queries the device for the `IDirect3DDevice3` interface at `DAT_0050591c`. The model-render setup `FUN_00483600` (`0x483600`) obtains `IDirect3DViewport3` from its input object's `+0x10` field and passes that viewport to device vtable slot `+0x30`.

Failure branches release the interfaces acquired along the respective paths. The static bodies do not establish whether every external driver-side mode change is restored after partial initialization failure.

## Evidence

- `ev_32fb14ef2009363722ff458f2625dc5824f713e6d217837831b392e75106a05a` — DirectDraw setup body and mode branches.
- `ev_2aa52ab0de467e2970385872e396b7eeadb3a231e5fc57d670d1bb58df6fd236` — D3DRM/device initialization.
- `ev_b2e007f9ff25857488e577c08778d92b846f175a357212ed0fe63abf5a79ee80` — viewport query and device installation.

All are retained in [`legorr-engine-expansion.bundle`](../evidence/legorr-engine-expansion.bundle). This is static shipped-artifact evidence; mode labels remain inferred as noted above.
