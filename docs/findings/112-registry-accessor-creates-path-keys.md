# 112 — The registry read accessor creates each subkey path component

The registry accessor `FUN_0048b360` wraps `FUN_0048b390`, which recursively walks a backslash-delimited subkey path. `FUN_0048b330` splits the path; for each component `FUN_0048b390` calls `RegCreateKeyA`, recurses using the returned handle, then closes handles while unwinding. At the leaf it calls `RegQueryValueExA`, returns a boolean status, and does not inspect the queried value type.

Consequently, the accessor can create missing subkey components while trying to read a value. This describes the accessor's mechanics; it does not claim that any particular registry contents were present or that other persistence paths behave the same way.

**Evidence.** REA/Ghidra 12.1.4, shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. Bundle: `docs/evidence/legorr-registry-accessor.bundle`. Focused records: `ev_780d169ee45e2f8437d4083ac309b6703891989a86e1f1adbbd8004df9c2f5e6` (recursive query/create/close helper); `ev_c0068a244c4a35bdda0bce5fa94050e4c17b8821f3b2ec6038409346f633257b` (path splitter); `ev_4bf05cc9c1e7b6b2d98bedb427d3ee037a48e5060211b775d588e57c0a48c526` (API call slots); `ev_4eaa14b6db5760305ef42b9aa7029d3d337294180c20c51cda7d9b5c704e0522` (wrapper root and delegation).

**Limitations.** Static evidence establishes the code path, not runtime registry contents. Finding 02 already records setting names and fallbacks; this finding adds only the accessor behavior.
