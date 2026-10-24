# Finding 504 — Runtime Mapping and Camera State

`0x004AA450` gates the camera-move linked-record route at `0x004AA458`; the chain-clear helper resets both. The gate's broader lifecycle remains unresolved.

`0x004AFD54`, `0x004AFD58`, and `0x004AFD5C` are the three saved words written by the CRT non-local-goto frame helper: incoming EAX, the dword at EBP+8, and current EBP.

`0x004AFFC8` is the CRT arena static-pool sentinel. Arena creation uses the static pool while it equals -1, and freeing that static block restores the sentinel.

`0x004B24C0` begins the Win32-error-to-errno pair table, scanned through `0x004B2628` by the mapper. `0x0054919C` is an optional structured-exception callback; the CRT dispatcher calls it when present and stops if it reports the exception handled. Its setter and full ABI are not recovered.

`0x00549934` is a per-entry terrain edge animation sample count, used to bound sample loops in records with a `0x10A`-dword stride. `0x005560A4` is stored as a camera view-context parameter and forwarded into a scene transform helper; its specific domain meaning remains unresolved.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
