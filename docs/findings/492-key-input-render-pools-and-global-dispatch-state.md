# Finding 492 — Key Input, Render Pools, and Global Dispatch State

`0x004A1F60` begins a 256-entry DirectInput scan-code-to-character table. `MenuInputTick` inserts the mapped character for a newly pressed scan code when its entry is nonzero; `KeyContextCheck` returns the entry for a selected scan-code index.

`0x004EA7AC` begins a six-dword render configuration snapshot. The paired copy helpers move all six values between this global block and a caller-provided buffer, or clear the block; the six fields' individual meanings remain unresolved.

`0x004EABC4` is a global scatter-spawn request state word. The worker checks and clears it while retrying or disabling the scatter-spawn task, and level teardown also clears it. Its exact state encoding is unresolved.

`0x004EADC4` counts updates dispatched for entity-kind-4 records. The entity update dispatcher increments it for that kind, but another setter can assign arbitrary values, so its complete lifecycle is unresolved.

`0x004EAE14` is the maximum absolute drift-offset threshold passed to the light-effects blink parameter setter. The drift update compares the candidate X offset plus its base against this threshold and normalizes the random step when reached.

`0x004FF990` is explicitly incremented and reset by separate helpers; the available callers do not establish a more specific meaning. `0x004FFE34` is initialized to one during objective configuration, passed by address to an objective dispatch helper, and cleared in the objective state path; its exact semantics remain unresolved.

`0x00503188` is a callback-all-pass result flag. The helper sets it before each selected callback, and a callback can clear it to make the aggregate check fail.

`0x00505810` points to the window class name derived from the executable name. `WindowCreate` uses it to register the class and create the main window. `0x00505938` marks that the engine callback triplet has been configured; the main loop checks this flag before invoking the callback path.

`0x005343B4` holds the CD drive letter discovered by searching for the key file. The sound-path fallback uses it when building a CD path.

`0x00540584` begins a 20-slot pointer table for render-resource pool chunks. Initialization clears the table, and pool growth stores each newly allocated chunk pointer by chunk index.

`0x00544858` holds the callback supplied to the shared-object directory scan. Scene-entry table expansion invokes it for selected directory entries to populate scene slots.

`0x00544A00` holds the current CD audio track identifier. The start helper stores and plays it; the watchdog checks that track and may restart it.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
