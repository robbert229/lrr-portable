# 114 — TerrainCreate projects map height into its block table

TerrainCreate builds a 0x28-byte record per map cell. The record byte at +0x19 is an attribute byte
initialized to 0x60; +0x1A receives the cell value returned by MapFileGetCell, narrowed from the
16-bit map value to one byte. The terrain mesh builder later uses +0x1A as the cell height, scaling
it into the vertex Y coordinate as negative height times the object scale.

TerrainCreate also compares the discontinuities across the two candidate cell diagonals. It sets
orientation bit 0 in +0x19 when diagonal A–D has the smaller height difference. The orientation
fix-up rotates the four stored corner positions, recalculates normals, and marks neighboring cells
for normal/mesh refresh. Bit 2 in the attribute byte is the dirty/refresh flag. The prior finding 07
summary incorrectly assigned the 0x60 initializer to +0x1A; it belongs to +0x19.

**Evidence.** REA/Ghidra 12.1.4, shipped LegoRR.exe, SHA-256
c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f. Bundle:
docs/evidence/legorr-terraincreate-fields.bundle. Focused records:
ev_15e789ce5adc6c0da67c030ea033e152a559b78e1d07cf03af24b7bebc0acd2b (TerrainCreate writes +0x19 and
+0x1A); ev_0e1f13b0f708dc0eeab5124bebd8e384c2347d622e281795b7addc6efa1de0aa (map-cell read);
ev_3835b6d17e0635789e68651e57739ba43c177833ecacfc28572407f0d43184fb (mesh vertex height);
ev_fd007a6bdbc204c4bf24c8e70e8662c369dab34776d17c7cb8319f49b25d29e7 (diagonal selector and fix-up);
ev_3b28829a7f1c0c839e9f324f2a08f413f45c1d84d2c784f65597c838e56bc8f7 (normal refresh);
ev_3376bc5d18ffee9b0644764c4248a3e266f3a919164b8e028fed0204e0952f58 (dirty-flag use).

**Limits.** This is static evidence of construction and consumers. The meanings of +0x18 and
attribute bits 5/6 remain unknown. The finding identifies the compared height differences and
orientation choice; it does not assign a broader terrain-generation purpose.
