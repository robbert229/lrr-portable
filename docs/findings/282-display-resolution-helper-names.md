# Finding 282 — Display Resolution Helper Names

Five anonymous routines support display-mode selection and refresh-rate controls. Their names describe visible control flow and tuple operations without assigning undocumented UI field meanings.

| Address | Name | Observed behavior and uncertainty |
| --- | --- | --- |
| 0x49d780 | DisplayResolutionComboPopulate | Clears the resolution combo, enumerates display modes, adds modes with flag `0x2000`, and selects the marked entry. |
| 0x49d870 | DisplayResolutionFilterTupleAppend | Appends three values to a global list of `0x44`-byte filter tuples. |
| 0x49d8d0 | DisplayModeMatchesFilterTuple | Matches width, height, and bit depth against the tuple list; zero tuple fields act as wildcards. |
| 0x49d950 | DisplayModeNameIndexFind | Byte-compares a supplied name against enumerated mode names and writes the matching index. No static caller was resolved. |
| 0x49d9d0 | DisplayRefreshRateControlsSync | Reads a mode capability bit and synchronizes refresh-rate control enablement and selection, including a mode-list refresh. |

Bodies are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). `0x49d030` initializes filters and uses the tuple-match and tuple-append helpers; `0x49d630` also uses the tuple matcher. No static caller was resolved for the combo population, name-index lookup, or refresh-rate synchronizer; the synchronizer itself calls `0x49d5e0` to refresh the mode list. Direct display-mode evidence for `0x49d780` is retained in [legorr-graphics-mode-surfaces.bundle](../evidence/legorr-graphics-mode-surfaces.bundle), and caller queries are in [legorr-engine-helper-audit-text-io-expansion-9.bundle](../evidence/legorr-engine-helper-audit-text-io-expansion-9.bundle). Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
