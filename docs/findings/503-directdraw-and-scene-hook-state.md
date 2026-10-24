# Finding 503 — DirectDraw and Scene-Hook State

`0x00556290` begins 18 terrain surface-type names, starting with Lego_SurfaceType_Tunnel. Weapon-type config loading scans the table to populate surface-specific weapon timing values; the table ends at `0x005562D8`.

`0x0076AB8C` and `0x0076AB8D` are keyboard-state bytes at DirectInput scan-code indices `0x0C` and `0x0D`, within the polled 256-byte buffer at `0x0076AB80`. Camera movement and reward scrolling read both as current input state.

The DirectDraw setup globals at `0x0076ACA4`, `0x0076ACAC`, `0x0076ACB0`, and `0x0076ACB4` are temporary interface, callback, and enumeration-result slots used by DirectDraw creation and device enumeration. The decompile shows where each is assigned and returned, but does not fully recover the callback ABI or interface ownership.

`0x0076ADD8` begins entity visual-kind names initialized by the renderer and searched when dispatching visual-kind names. `0x0076CD74` is a count used to free two-dword scene sample-hook resource records beginning at `0x0076AE34`; its allocation producer is unresolved. `0x0076CD78` and `0x0076CD7C` are a sample-hook callback and context, while `0x0076CD80` and `0x0076CD84` are the corresponding scene-end callback and context. The frame updater invokes each callback at its associated scene event.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
