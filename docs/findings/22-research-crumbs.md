# 22 — Research crumbs: message registry, the advisor flag word, D3DRM verdict

Closes the three "small research questions" that remained after findings 01–21: the
population site of the 40-entry message config (finding 04 §Open), the true identity of
the `DAT_004ffbc0` "input-device mode word" (finding 05, corrected here), and whether
D3DRM ever touches a hardware (HAL) device. No new formats; this is bookkeeping that
the port needs for two modules (message/HUD bus, objective advisors) and one deletion
(the whole D3DRM dependency can be stubbed or routed to whatever renderer we ship).

## 1. Message registry: hardcoded 40-name catalog + `InfoMessages` config section

Finding 04 flagged the config block as populated "somewhere (level-load path?)" and
finding 05 ruled out level data. Both halves now have positive evidence:

- **Catalog (names):** `FUN_00419230` writes 40 `s_Info_*` string pointers into
  `DAT_004dc658 … DAT_004dc6f4` (stride 4), sets `_DAT_004dcd3c = param_1`, and ORs
  `DAT_004dcd48 |= 0x18` (system enable + auto-cooldown, finding 04). Its only xref is
  `0x41fd13`, inside `FUN_0041fb50` — the message-module init — which is installed as
  the init callback by `FUN_0041fa20` (`local_c = FUN_0041fb50`, engine tick
  `LAB_004232e0`, shutdown `LAB_00424d00`/`LAB_00424cf0`, registered via
  `FUN_00477fd0`). So population is **at startup**, not per level.
- **Per-entry data (sound handles/flags):** also from `FUN_0041fb50` (xref `0x422553`),
  `FUN_00434c10` walks the config section keyed `InfoMessages`
  (`s_InfoMessages` `0x4a484c`): entry+4 is a message name mapped to the id by a catalog
  lookup (`FUN_004193e0`), the sound path is loaded (`FUN_0047d9d0`) and stored at
  config+`0x04` by `FUN_00419650` (addref via `FUN_0047dbc0`), the display text pointer
  via `FUN_004194b0`, an optional second sound via `FUN_00419680`, and the
  "play regardless of game state" flag via `FUN_00419470` — set when the flag string is
  empty/short (<4 chars) or compares equal to `AlwaysPlaySFX`
  (`s_ALWAYSPLAYSFX` `0x4a483c`). The `& 0x20000` cooldown rule (25.0 s reset) from
  finding 04 stays as documented; that flag comes from the same section.

The 40-slot catalog, in table order (slot = `(addr − 0x4dc658)/4`; ids are assigned by
matching the name, order is load order): `CrystalFound`, `OreSeamFound`, `RockMonster`,
`LavaRockMonster`, `IceRockMonster`, `UnderAttack`, `Landslide`, `CaveIn`, `Constructed`,
`CavernLocated`, `LegoManDeath`, `VehicleDeath`, `BuildingDeath`, `DynamitePlaced`,
`NoPower`, `PowerDrain`, `AirDepleting`, `AirLow`, `AirOut`, `AirRestored`,
`TrainDriver`, `TrainDynamite`, `TrainRepair`, `TrainPilot`, `TrainSailor`,
`TrainScanner`, `OreCollected`, `WallDug`, `WallReinforced`, `CrystalPower`,
`LavaErode`, `SlugEmerge`, `PathCompleted`, `FoundMinifigure`, `CanUpgradeMinifigure`,
`CanTrainMinifigure`, `CrystalSeamFound`, `GenericSeamFound`, `GenericDeath`,
`GenericMonster` (all with the `Info_` prefix as stored strings).

## 2. `DAT_004ffbc0` is not an input-device word — it is the objective/advisor flag word

Finding 05's ledger called `DAT_004ffbc0` an "input-device mode word (mouse/alt
devices), inferred from usage" — and its "1/2/4 = alternative device modes" note maps
directly onto what that word actually carries. That inference is wrong. All 57 xrefs
sit in four adjacent functions, `0x457d–0x4591`, all handling a **level objective
state word**:

- `FUN_00457d40` — per-level objective-config loader (writer, see table below)
- `FUN_00459050` — objective checker (gates on `0x10/0x20/0x100/0x200/0x800`)
- `FUN_00458670` — objective-**result announcer** (sets 1/2/4/8, shows `"Acheived"` /
  `"Failed"` / `"FailedCrystals"` overlays, stores result index 0/1/2 into view slot
  `[0x9f]` and `_DAT_004ffe30`)
- `FUN_00458be0` — per-frame objective state machine (branches on 1/2/4, dispatches
  the outcome, shows the advisor hint when bit `8` is set then clears it, honoring the
  suppression bits 10/20/80)

Finding 05's "dwell-slot `uVar2 ∈ {0,1,2,3,4}`" observation is the same bits 1/2/4/8:
those are objective-result states, not devices.

**Config-side bits** written by the loader `FUN_00457d40`, reached from the level-load
path (single caller `FUN_00429890` at `0x42ab0a`); it zeroes the word at entry, then
reads config keys:

| bit | source | meaning |
| --- | ------ | ------- |
| `0x10` | `DontShowObjectiveAdvisor` key ≠ 1, or forced when the word is still 0 after load | suppress generic advisor hints (on by default) |
| `0x20` | `DontShowObjectiveAcheiveAdvisor` key ≠ 1 | suppress "objective achieved" hint |
| `0x80` | `DontShowObjectiveFailedAdvisor` key ≠ 1 | suppress "objective failed" hint |
| `0x100` | `CrystalObjective` key present | crystal-count objective armed (`FUN_00458580`, target at +`0xf4`) |
| `0x200` | `OreObjective` key present | ore-count objective armed (target at +`0xf8`) |
| `0x400` | `BlockObjective` key present | block/build objective armed (`FUN_004585c0`, +`0xfc`/`0x100`) |
| `0x800` | `TimerObjective` key present | timer objective armed (`FUN_004585f0`, seconds×25 at +`0x104`; also sets `0x20` when mode is `HitTimeFailObjective`) |
| `0x1000` | `ConstructionObjective` key present | construct objective armed (`FUN_00458620`, +`0x108`/`0x10c`) |

**Runtime-side bits** managed by the announcer/state machine: `0x1` objective still
pending (set by the announcer's `param_1 = 0` path), `0x2` achieved, `0x4` failed,
`0x104` failed-with-crystals variant (`param_1 = 3` adds `0x100`), and `0x8`
result announced / advisor hint pending — set on every announcement path, tested and
cleared by `FUN_00458be0` once the hint has been shown (suppressing bits decide whether
`FUN_004018c0(0,1)` fires alongside).

The same loader resolves the `ObjectiveImage_ix_i` / `ObjectiveAcheivedImage_ix_i` /
`ObjectiveFailedImage_ix_i` keys (`.flic`-style HUD images, handles stored in the level
block at +`0xbc`…`0xd4`, sizes at +`0xc0`…) and `ObjectiveText`.

**Reader — `FUN_00459050` (objective checker)** uses the word as its gates:
bit `0x10` set ⇒ returns false (no advisor shown); with `0x800` it counts the timer down
and fails when it expires (bit `0x20` decides the `param_2` success marker); `0x100` /
`0x200` compare live crystal/ore counts against the stored targets. The rest is handled
by the announcer/state-machine pair above — no other code in the binary touches the
word.

For the port this is a level-script feature, not an input layer — it belongs next to
the objective/task-manager work (finding 09), and nothing needs to be done about
"input-device modes".

## 3. D3DRM: software path only — the HAL-vs-software question is moot

The binary imports exactly three functions from `D3DRM.DLL`
(`Direct3DRMCreate`, `D3DRMVectorRandom`, `D3DRMVectorRotate`):

- `Direct3DRMCreate` has a single call site (`0x478222` in `FUN_004781d0`, the render
  init): the created object is wired through `QueryInterface`s for `IID_IDirectX`
  (`{4516EC83-8F20-11D0-9B6D-0000C0781BC3}`) and `IID_IDirectDraw`
  (`{6C14DB81-A733-11CE-A521-0020AF0BE560}`), a device is produced by a
  vtable+`0x3c` call (`DAT_00505918`), and `IID_IDirect3DDevice3`
  (`{B0AB3B60-33D7-11D1-A981-00C04FD7B174}`) is QI'd from it (`DAT_0050591c`).
  Downstream, the model-render region (`FUN_00483600`, `0x4836xx` module) QIs
  `IID_IDirect3DViewport3` (`{B0AB3B61-…}`, GUID constant at `0x49f778`) on that device.
  That is the classic *device-on-surface* / DRendering setup — D3DRM rendering into the
  game's own DirectDraw surface, whose implementation is the D3DRM built-in CPU
  color-RLE renderer + DRendering executes.
- `D3DRMVectorRandom` / `D3DRMVectorRotate` are used as dumb free-function math
  helpers (single-line wrappers `FUN_00479500` and the `0x479524` site) — random
  scatter/rotation, not rendering.
- The byte pattern for `GUID_IDirect3DHALDevice` (`{34532120-3453-11CF-…}`, little-endian
  `20 21 53 34`) appears **zero times** in any section of the binary: no code ever
  requests a HAL device.

**Verdict:** D3DRM never uses a HAL device — it is a software renderer behind the
DRendering abstraction. Nothing in the port needs a D3DRM-compatible layer for
correctness: the device + viewport + execute pipeline can be replaced directly by the
reconstruction's own rasterization (findings 11/14 already implement the same
primitives in software), and dgVoodoo (finding 15) already maps the calls wherever the
runtime wants. The remaining perf note in finding 15 stays valid; the HAL question
can be dropped from the open-threads list.

## Evidence

All of the above is REA (ghidra provider) against `LegoRR.exe`
(SHA-256 `c3d8ce23…581e0f`): `xrefs` for `DAT_004ffbc0`, `Direct3DRMCreate` thunk
`0x49b6b0`, the `InfoMessages` call sites, and `0x457d40`/`0x419230` callers;
`procedure_pseudo_code` for `FUN_00419230`, `FUN_00434c10`, `FUN_0041fb50`,
`FUN_0041fa20`, `FUN_00457d40`, `FUN_00458580/5c0/5f0/620`, `FUN_00458670`,
`FUN_00458be0`, `FUN_00459050`, `FUN_004781d0`, `FUN_00483600`; `read_bytes` for the
GUID constants at `0x49fbd8`, `0x49f338`, `0x49f6d8`, `0x49f778`; `search_bytes` for
the HAL GUID pattern (no matches). GUID names from the published DirectX GUID registry
(`IID_IDirectX`, `IID_IDirectDraw`, `IID_IDirect3DDevice3`, `IID_IDirect3DViewport3`,
`GUID_IDirect3DHALDevice`).
