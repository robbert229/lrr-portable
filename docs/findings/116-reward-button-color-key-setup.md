# 116 — Reward button surfaces receive a zero-to-zero color key

RewardInit passes six literal zero values to FUN_0047dbf0 for the primary SaveButton and
AdvanceButton image handles and for up to two guarded companion handles under each button. The
helper converts two RGB triples for the target surface's pixel format, calls the surface vtable at
+0x74 with selector 8 and the resulting two-value color-key range, then ORs bit 1 into a wrapper
field at +0x18. With the reward caller's zeros, both endpoints are black.

The slot and selector match DirectDraw IDirectDrawSurface::SetColorKey with DDCKEY_DESTOVERLAY; that
API-level identification is inferred from the COM-style slot and call shape. The observed behavior
is the zero-valued endpoints, virtual call arguments, and wrapper-field update.

**Evidence.** REA/Ghidra 12.1.4, shipped LegoRR.exe, SHA-256
c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f. Bundle:
docs/evidence/legorr-reward-widget.bundle. Focused records:
ev_b524ed5312acf7ca675900c86ebfcdc7edd149d9e424993537c8c14fe4fef297 (button tuple load and six-zero
calls); ev_94be6ebcbae23ca10266e9bfb23ce4b2b3658b765eeb21c0721365141b2a4e02 (RewardInit call order);
ev_1bc4835fa521a5ba495495d4a35d0d52846cb22ff7b1fc553f178118059098a7 (helper);
ev_311b7d99fbe834b4d117cdec06884d9356cc965c8b7e8fcf444ef7037b3428bb (RGB packing);
ev_a70b99fd9bfc8334c54fd0dea2fc2e3dbd33a70aea170929ce464b360e3d8437 (surface pixel-format
conversion).

**Limits.** Static analysis only. The vtable identity and destination-overlay selector are inferred;
the wrapper bit's meaning is unknown. The fourth handle in each tuple is not passed to this helper
at the observed call site.
