# 119 — Sound initialization queries DirectSound's 3D listener interface

The sound initializer creates a DirectSound device, sets its cooperative level, and creates a
primary buffer. It then calls QueryInterface on that buffer using the 16-byte GUID at 0x4a04f8 and
stores the returned interface pointer in DAT_005065c0. The bytes decode to
IID_IDirectSound3DListener. If the query fails, the initializer releases the primary buffer and
device and clears the device pointer; on success it continues initialization and eventually marks
the sound subsystem active.

**Evidence.** REA/Ghidra 12.1.4, shipped LegoRR.exe, SHA-256
c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f. Bundle:
docs/evidence/legorr-directsound3d-init.bundle. Focused records:
ev_af0b509efd1bb602bc8fc21640802665ec8e16c0af37747179565cb9e2bd4079 (initializer, QueryInterface,
success/failure cleanup); ev_1405afed6a0ddc7d2cac3fa31390655728534eed2f8ff1ee0a7672da9cdc8666 (GUID
bytes); ev_6750342b5cf7c41c0795e52a15626e61e347af983c84d635257fa225b82cc6c1 (only observed xref);
ev_de604d85165ce4ca813038047efc607fab87b2375341168a40abc2d6f3a15e58 (subsequent primary-buffer
initialization).

**Limits.** Static analysis only. GUID identity is established by decoding the observed bytes
against the DirectSound interface identifier. Later calls through the returned interface remain
unnamed vtable calls here; this does not trace per-source 3D buffer setup or config-driven distance
behavior.
