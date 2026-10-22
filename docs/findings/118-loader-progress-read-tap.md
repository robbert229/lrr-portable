# 118 — The whole-file read tap accumulates loader progress

DAT_005343ac is an optional loader-progress callback, not a file-change notification. Initialization
selects a LoaderProfile, configures the progress UI, and installs FUN_0044dec0. While a profile
category is active, the two observed whole-file loaders call the hook with the path, measured file
size, and stored user-data pointer after opening and measuring the file but before allocating and
reading its contents. The callback adds that size to the active category's accumulated count,
divides by its configured total, and updates/interpolates the progress UI.

This resolves finding 12's open callback question and identifies the use of the size argument
without claiming that it measures successfully consumed bytes.

**Evidence.** REA/Ghidra 12.1.4, shipped LegoRR.exe, SHA-256
c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f. Bundle:
docs/evidence/legorr-loader-progress-read-tap.bundle. Focused records:
ev_8a576e71604114cae565a89bc4bb0349e7a02fa9b18129e9b65f1c2aab8d6503 and
ev_c9c8fbf972822329f4d4bdf08e0110149868343455631819ce97d6e51b6aa6d2 (both loaders call the hook
before reading); ev_80a8b2ac8c14e3ecedd69b73e2736e55c1003a981a8f96076ab5b38512800d74 (callback
setter); ev_74ebabf8d6b6ffff8ae37936b8849aed545008243206151cce06c51aca94b1c7 (profile setup and
callback installation); ev_9cf0ef11e49abe4974600709d27cddc78521eee9c94f6ba65c1a2b8c2731b1f4 (profile
selection and active category); ev_0b53994ee8ba0c0b10232543999b2170676111a33b15d81564f2ad7d5cd8330a
(size accumulation and UI update).

**Limits.** The configured denominator's unit is unknown. The callback adds the opened file's
measured size before the body read; this does not prove the same number of bytes was consumed.
Loader-profile names and the callback's role are inferred from their state flow and UI updates.
