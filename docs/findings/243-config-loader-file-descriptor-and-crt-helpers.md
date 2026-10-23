# Finding 243 — Config-Loader, File-Descriptor, and CRT Number Helpers

Twelve anonymous procedures now carry names grounded in their own instruction
sequences and in the named or ported procedures that call them — every one of
them has a ledger-named static caller. The `GameConfigInit` (ported) fan-out
alone names five per-section config loaders, and the CRT file layer inherits
the established `Win32FileDescriptor*`/`StdFile*` family naming.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x45a030 | UiPanelNameTablesInit | Zero-branch static initializer: writes the panel-count (12) and button-count (24) constants to `DAT_005007d0/7d4`, twelve `Panel_*` name pointers into `DAT_005000e0`–`DAT_0050010c`, and forty `PanelButton_*` name pointers into `DAT_00500350`–`DAT_00500770`; the 12/24 counts match the scan bounds of the table-search consumers `FUN_0045a370`/`FUN_0045a410` (both anonymous). Called by `GameConfigInit`. |
| 0x434710 | PanelButtonLayoutConfigLoad | Builds the panel-button layout: formats the config path `"Buttons%ix%i"`, walks its children through `ConfigEntryFind`/`ConfigNestedPathChildFind`/`SortedEntryNextEqualKey`, resolves each child key to a panel index via `FUN_0045a370` and each button token via `FUN_0045a410` (the tables initialized by `UiPanelNameTablesInit`), splits comma value tokens with `BlockTextTokenSplit`, maps `NULL` to zero, converts four values with `ParseIntegerWrapper` into stride-0x18 records, and calls the anonymous record builder `FUN_0045ac30` once per panel. Called by `GameConfigInit`. |
| 0x40e490 | EncyclopediaConfigFilesOpen | Four unrolled passes allocating handle arrays (`EngineHeapAlloc` counts `DAT_0055627c`–`DAT_00556288`), resolving each name in `DAT_005561e8`–`DAT_005561f4` under the `"Encyclopedia"` config section (`ConfigEntryFind`), and opening the resulting paths read-mode via `GameFileOpenDispatch`; the tail does the same for the literal items `PowerCrystal` and `Ore`. Called by `GameConfigInit`. |
| 0x45b9d0 | PanelRotationControlConfigLoad | Seven straight blocks reading the `"PanelRotationControl"` section keys `CenterPositionX`/`CenterPositionY`/`Radius` (default 1 when absent) into `DAT_00500824`–`DAT_0050082c`, and `LeftImage`/`RightImage`/`UpImage`/`DownImage` through comma-split → `BitmapSurfaceLoad` → `ResourceRecordAddRef` plus size floats into `DAT_00500850`–`DAT_0050085c`. Callees all ledger-named. Called by `GameConfigInit`. |
| 0x485a20 | KeyboardKeyNamesInit | Clears a 0x100-entry table, then stores ~100 `KEY_*` name strings (`KEY_ESCAPE`, `KEY_ONE`, …, `KEYPAD_ENTER`, `KEY_PRINTSCREEN`) into `DAT_005765e0`–`DAT_0057692c`, the scancode-to-name lookup table consumed by the input layer. Called by `GameConfigInit`. |
| 0x446610 | EntityMovementTickScaleAndDeltaGet | Pure compute helper taking (entity, dt, &scaleOut, &tickOut): selects a base scalar through per-type accessors and `FUN_00469cb0` (`**entity+0x334` fields), applies flag-selected ×0.05/×0.5/×0.25 scales, queries `CurrentLevelHeightGet`/`WorldToTerrainCellBoundsCheck`/`TerrainCoordinateAndOccupancyCheck`/`TerrainWalkableNeighborSearch` to blend the terrain-aware factor (including a step-kind-byte `'\t'` case for type 2), and returns scale/tick outputs — `EntityMovementAccumulatorApply` multiplies its accumulated vector by the scale, `PathStepAdvanceAndArrivalCheck` adds it to step progress, `EntityActivityTick` feeds the tick float to the animation dispatch. |
| 0x406990 | TypeTableResourceRowLoad | Per-entry body of `MiniFigureTypeTableLoad`/`RockMonsterTypesLoad`: joins the entry path to `dir\name.ae`, loads the `"ACT"` visual via `EntityVisualLoadACT`, opens the `.ae` block-text and reads `CameraNullName`/`CameraNullFrames`/`CameraFlipDir`/`DrillNullName`/`FootStepNullName`/`CarryNullName`/`ThrowNullName`/`DepositNullName` into the 0x74-stride type row, then builds medium/high/FP-camera poly-variant lists via the anonymous builder `FUN_0044a7d0` from the `"MediumPoly"`/`"HighPoly"`/`"FPPoly::Camera%i"` keys. |
| 0x499240 | Win32FileDescriptorOpen | Win32-backed open used by `StdFileOpen`: builds `SECURITY_ATTRIBUTES`, maps access bits (`GENERIC_READ/WRITE/ALL`) with `EINVAL` for invalid masks, translates share and `O_CREAT`/`O_TRUNC`/`O_EXCL`/`O_APPEND` dispositions to `CreateFileA`, allocates a slot in the 32-entry descriptor table (`DAT_0076ddc0`/`DAT_0076dec0`, shared with the already-named `Win32FileDescriptorSeek`), flags file type via `GetFileType`, probes text-mode leading Ctrl-Z through the sibling reader, and maps errors with `Win32ErrorMapToErrno`. |
| 0x497130 | Win32FileDescriptorRead | Descriptor read for `GameFileReadBuffered` and the stream fill path: validates the fd against the same descriptor table, honors EOF/in-use flag bits, serves a one-byte text-mode peek cache, runs `ReadFile`, collapses `'\r'`→`'\n'` with lookahead and stops on Ctrl-Z setting EOF bit, and converts `ERROR_ACCESS_DENIED`/EOF through `Win32ErrorMapToErrno`. |
| 0x47d000 | PalettizedBitmapSurfaceDecode | Called only by `CreateBmpSurface`: reads two DIB-style headers through object vtable `+100` calls, validates size fields and matching width/height, derives per-channel shift counts from the three RGB masks with popcount loops, then runs the decode loop taking each input byte through a 4-byte-entry palette LUT, combining three channels, packing two 16-bit pixels per dword, and writing the rows through vtable `+0x80` closes on exit. |
| 0x4877e0 | SceneEntrySlotTableExpand | Called only by `LoadLWSCScene`: when the shared-object resolve callback `DAT_00544858` (registered during `SharedObjectDirectoryScan`) is present, allocates a 0xD0-stride slot table, copies and comma-tokenizes the flagged 0x34-stride record names, invokes the callback per slot, and parses post-hyphen token integers into per-slot fields. |
| 0x498350 | NumberStringScanParse | State-machine number scanner for `ParseEntityHealthValue` and two neighbors: a `do-switch` walk over the input with state per case — sign (`+`/`-`), leading-zero skip, digit accumulation with overflow clamp 0x1451, `'e'`/`'E'` exponent entry — using the character-class table behind `FUN_0048f1d0` and the digit-buffer assembler `FUN_0049a0b0` / exponent scaler `FUN_0049a800`; writes the parsed value, start, end, and flag words into the result struct and returns a status code (in-range/underflow/overflow/invalid). |

**Evidence.** Direct decompiles are in
[legorr-engine-helpers-expansion-15.bundle](../evidence/legorr-engine-helpers-expansion-15.bundle):
`ev_9e88871063c8f52df8d74d8f496401b6f30377ca3f6a11372715c62e97ae6931`
(batch decompile of all twelve addresses). Caller observations are the
matching `procedure_callers` records, including
`ev_d67755f44b1b885b421fa3741a39243f28a7585773f31f1158317868d29e5b91`
(0x434710), `ev_ae7a6f4c7c08c73ab97f12507565a98e7f99161527a3d20a317f3751f5654038`
(0x45b9d0), `ev_66615c86406700055e7f5...` (0x40e490, all `GameConfigInit`),
`ev_63dbb3f98d3c91c8cb289759d674298785f47196165e9661c91229109f9c3167`
(0x446610 from `EntityMovementAccumulatorApply`/`PathStepAdvanceAndArrivalCheck`/`EntityActivityTick`),
`ev_80c9f459b5c470152846f...` (0x406990 from `MiniFigureTypeTableLoad`/`RockMonsterTypesLoad`),
`ev_0febe8d5ce29cf4504ba5...` (0x499240 from `StdFileOpen`),
`ev_68518e1d04425f35cfa2a...` (0x497130), `ev_bd6584949902aae0cb652...`
(0x47d000), and `ev_90ef1f25ac6b5c95ad086...` (0x4877e0).
All investigations used shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.

**Still unresolved.** The config/panel machinery leans on four anonymous
helpers kept out of the ledger names this round: `FUN_0045a370`/`FUN_0045a410`
(panel/button strcmpi index tables), `FUN_0045ac30` (per-panel button-record
builder), and `FUN_0044a7d0` (poly-variant list builder). The entity scalar
accessors `FUN_00469cb0`/`0x469ce0`/`0x469d20`/`0x46a0a0` return
`**(entity+0x334)`-relative floats whose field meanings are not pinned, and
`FUN_00432c50` is a cell-record type getter only. Five CRT helpers in the
`0x493xxxx`–`0x49bxxxx` region are now named from direct decompiles and caller
evidence: `CrtMemoryMove` (0x498000), `CrtFileDescriptorTableInit`
(0x4965d0), `CrtTimeZoneStateInit` (0x499880),
`CrtLocaleCodePageTableUpdate` (0x4962f0), and
`CrtFloatingPointExceptionRaise` (0x493280); see finding 260. Five more
runtime helpers are now named in finding 261: `CrtCommandLineArgumentScan`
(0x495f80), `CrtRuntimeErrorMessage` (0x4968f0), `CrtPutEnvUpdate`
(0x49b2b0), `CrtCompareStringACompat` (0x49afe0), and
`CrtDoubleToIntegerRound` (0x4935c0). Finding 264 adds the command-line argv
initializer, MSVC exception-handler search, and extended-float decimal/multiply/
power-of-ten helpers (0x495ee0, 0x495160, 0x49a1b0, 0x49a540, 0x49a800).
Other helpers in that region remain anonymous. The callback global
`DAT_00544858` and the resolve semantics it hides need the
`SharedObjectDirectoryScan` side traced further. The unresolved set carried
from findings 240–242 (`0x418a00`/`0x45c0d0`, `0x44d280`,
`0x473460`/`0x473480`/`0x4734a0`) keeps its status.
