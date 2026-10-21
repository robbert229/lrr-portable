# 23 — Front-End & Menus (boot → main menu)

**Observations from** a fresh REA/Ghidra 12.1.4 session over `source/LegoRR.exe`
(SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21, on the menu-core region `0x4103d0`–`0x418110` plus its callbacks.
Complementary data reads from the shipped `Lego.cfg` (unpacked from `LegoRR0.wad`,
identical to the compat-staged copy). All addresses are virtual (image base
`0x400000`). Evidence: `docs/evidence/legorr-frontend.bundle` (73 records).

This finding gives the reconstruction a named corpus for the game's front-end so
the port can **boot into the main menu**: the config-driven menu builder
(`MenuBuild @ 0x4140c0`), the modal menu run-loop (`MenuRunLoop @ 0x413b80` /
`MenuShow @ 0x413e20` / `MenuTick @ 0x412c00`), the front-end gate
(`FrontEndActive @ 0x416d00`), the menu-enter / state dispatcher
(`MenuEnter @ 0x416c80`, `FrontEndStateChange @ 0x4169c0`), the selected-action
feeder into `LevelLoad` (`FrontEndSelectedAction @ 0x416d70`), the level chain
builder (`LevelChainBuild @ 0x4154b0`), the start-level entries
(`FrontEndStartLevel @ 0x415220`, `StartLevelPilots @ 0x41fa80`,
`StartLevelSelected @ 0x460220`), and the options/brightness/volume/game-speed
item callbacks. It also pins the **two config hard requirements** the port does
not yet meet: the block-path `::` separator and the shipped level-name
vocabulary (`Levels::<name>`), which together explain the open `Lego.cfg`
integration blocker (README ⚠️).

## How the game boots to the main menu

The frame slot `DAT_0050592c` (`FUN_0041fb50` — the "per-frame init/reset" from
finding 02) is really the **boot/init hub**: its callees include every
front-end entry point. Observed call edges (each verified as an incoming
reference in REA):

```
FUN_0041fb50 (boot/init)
 ├─ AvisEnabled       0x414ce0   ; gate the intro AVIs by config
 ├─ MoviePlayLoop     0x4157c0   ; play "Loading"-class movies to completion
 ├─ MoviePlayNamed    0x415910   ; play a named movie (config-key → path)
 ├─ MovieShownReset   0x415cf0   ; DAT_005574d0 = 0xffffffff (movie-shown reset)
 ├─ FrontEndActive    0x416d00   ; gate: Lego.cfg "FrontEnd TRUE" && !loader-mode
 ├─ OptionsLoadOrDefault 0x416940 ; the front-end options defaults
 ├─ MenuEnter         0x416c80   ; enter the main menu
 └─ FrontEndSelectedAction 0x416d70 ; (via later KeepRunning path)
```

The front-end is **modal**. `MenuEnter(menu-item-index)` →

1. `FrontEndStateChange @ 0x4169c0(i)` rebuilds the save/load lists for the
   chosen flow (`SaveMenuRefresh`/`SavTreeLoad`/`SavBuildCtx`, redraw via
   `FUN_00418110(1)`); case `0` resets to the main-menu root
   (`OptionsLoadOrDefault(1,1)` after clearing the current-menu globals).
2. `FUN_00413e20 @ 0x413e20` (`MenuShow`) clears every item's `+0x28` flag and
   runs the modal loop `MenuRunLoop @ 0x413b80`.
3. `MenuRunLoop` draws and steps the **currently selected menu** once per
   `timeGetTime()*0.025` dt — calling `MenuTick @ 0x412c00` each frame, handling
   the wipe/fade transitions, and **returns only when the menu's close flag
   (`+0x28`) turns 1** (a `Trigger` "Yes" / `Next` target chosen).

The gate cluster then decides what the modal loop does next:

| addr | name | role (observed) |
| --- | --- | --- |
| `0x416d00` | `FrontEndActive` | `Lego` cfg `FrontEnd` flag **and** `DAT_00505f88 < 3` → shows front-end |
| `0x416c80` | `MenuEnter` | reset state + play `0x2c` SFX + select item *i* in the main/save menu |
| `0x4169c0` | `FrontEndStateChange` | rebuild save/load lists per flow id (0=main,1/2=new-game/load,3=options,4=quit) |
| `0x416d70` | `FrontEndSelectedAction` | resolve the selected list entry → a *level name* for `LevelLoad`/`KeepRunning` |
| `0x416dd0` | `QuitMenuProbe` | nonzero ⇒ quit now (`DAT_0055701c`) |
| `0x416d50` | `SaveSlotOpen` | a save slot is open on the save menu (`DAT_00557020 != -1`) |
| `0x416de0` | `LevelStartFlag` | `DAT_00557048 != 0` — "advance/start level" requested |
| `0x416df0` | `MenuOptionsFlag` | `DAT_0055704c != 0` — options overlay toggle |

`KeepRunning @ 0x435940` (already ported) drives the front-end from the frame
loop: `FrontEndSelectedAction()` returns the level name selected on the main
menu, which `FUN_00416940(1,0)` (`OptionsLoadOrDefault`) + `LevelLoad @ 0x429890`
then load; the quit path is `MenuEnter(2)` → `QuitMenuProbe()`.

## The menu builder — `MenuBuild @ 0x4140c0`

Only caller: `LoadMenuBuild @ 0x4161f0`. Config-driven constructor:
`MenuBuild(exe-name, "MainMenuFull"|"SaveMenu"|"PausedMenu"|"OptionsMenu", …item-varargs)`.
Builds the 4 menu records into `DAT_00556fc4/fcc/fc0/fc8`.

Config grammar it reads (per-`Menu%i` group, addressed `Menu::<name>::Menu%i`):

```
MenuCount <n>                  — number of sub-menus
Menu%i {
  FullName / Title / Position (x:y) / MenuFont / LoFont / HiFont
  ItemCount <n>
  MenuImage <bmp[:0:0:1]>  MenuImageDark <bmp>
  AutoCenter TRUE|FALSE  DisplayTitle TRUE|FALSE  Anchored x:y  CanScroll
  PlayRandom TRUE|FALSE
  !Overlay%i  <avi\X.flh>:<SFX>:x:y        ; random-play overlay (FLIC + SFX + pos)
  Item%i      <Type>:…(see below)
}
```

Item value grammar (deduced from shipped values + `MenuItemType @ 0x413e60` and
the record constructors `MenuTextRecordMake @ 0x410d50` / `MenuButtonRecordMake
@ 0x410e20`):

| form | type token | example (shipped) |
| --- | --- | --- |
| `Next:x:y:label:Menu%i[:gate]` | 5 | `Next:0:0:Start_Game:Menu2` |
| `Trigger:x:y:label:1` | 1 | `Trigger:0:120:Credits:0` |
| `Trigger:x:y:imgN:imgG:imgD:tooltip:1` | 1 | `Trigger:-314:236:LP_Normal.bmp:LP_Glow.bmp:LP_Dull.bmp:ToolTip_FrontEnd_Back:1` |
| `Slider:x1:y1:x2:y2:banner:lo:hi:8×bar-bmps` | 3 | options volume/brightness/game-speed sliders |
| `Cycle:x1:y1:x2:y2:banner:n:opt1:…` | 0 | `Cycle:…:Help_Window:2:Off:On` |
| `RealSlider:…` | 4 | (commented header, finding-23 §config) |
| `TextInput:…` | 2 | (commented header) |

## The `::` config separator — why the blocker fires

The game's block-text path language is **`::`-separated**, confirmed by reading
the delimiter used by the path tokenizer `FUN_00477440`:

- `DAT_004aab18` = **`"::"`** (observed bytes `3a 3a 00`).
- `BlockFindPath @ 0x4792e0` splits the query on that delimiter before walking
  the record tree.

So the exe queries `LegoRR Main Menu::MainMenuFull MenuCount` **and**
`LegoRR Main Levels::Tutorial01 SurfaceMap` — where the first token `LegoRR`
matches the config's `Lego*` wildcard root record, and `Main` is the *section*
record (`DAT_004a24e4 = "Main"`, observed). The port's `block._` currently
splits on **space only**, so `Lego* Levels::Tutorial01 SurfaceMap` fails even
though the record exists as `Levels { Tutorial01 { … } }`.

Verified against the shipped `Lego.cfg` with the current parser:

| query | result |
| --- | --- |
| `Lego* Main FrontEnd` | `TRUE` |
| `Lego* Main DontPlayAvis` | `FALSE` |
| `Lego* Main StartLevel` | `Levels::Tutorial01` |
| `Lego* Menu MainMenuFull MenuCount` | `5` |
| `Lego* Menu MainMenuFull Menu1 Item1` | `Next:0:0:Start_Game:Menu2` (…`Menu2`–`Menu5` chain, `Quit_Game`) |
| `Lego* Menu MainMenuFull Menu1 MenuImage` | `Interface\FrontEnd\MenuBGpic.bmp` |
| `Lego* Menu OptionsMenu Menu1 Item1` | `Slider:…:Game_Speed:0:5:…8 bitmaps` |
| `Lego* Levels Tutorial01 SurfaceMap` | `Levels\TutorialLevels\MoveTuto_01\High_M01.map` (and `CryOreMap`/`PredugMap`/`OListFile`/`PTLFile`/…) |

Shipped `Lego.cfg` shape that matters to the parser: root record `Lego*`,
containing **siblings** `Main`, `Dialog`, `Reward`, `Menu`, `HelpWindow`, `Levels`,
… (`Main {` closes after the flat config — do **not** hardcode `Main Levels`).
Levels live under `Levels { Tutorial01 { … } }` with `FullName`, the ten map
layers, `NextLevel Levels::Tutorial02`, and front-end picker placement
(`FrontEndX/Y/Open`, finding 08).

## Levels: the address form

- `StartLevel = Levels::Tutorial01`, `TutorialStartLevel = Levels::MoveTuto_01`
  (observed). The level *name* the engine passes around is the `Levels::X` token —
  the `::` address — **not** `Level01`.
- `LevelChainBuild @ 0x4154b0` turns a start-level config key + `NextLevel`
  links into the ordered `{name, FullName}` list stored at `DAT_00556fe4`
  (campaign) / `DAT_00556fd0` (tutorials) with a count.
- The port demo's `LEGORRR_LEVEL=Level01` default and its `level_test --real`
  probe therefore resolve **0 real level groups**: the shipped vocabulary is
  `Levels::Tutorial01…`, and even that spelling only resolves after `::`
  tokenization. Fix = `::`-aware `block_find` (and/or a `::`→space shim in the
  level loader) + resolving the level *name* via `StartLevel`/`NextLevel` instead
  of an arbitrary "Level01".

## Start-level path (menu → in-game)

```
FrontEndSelectedAction 0x416d70   → selected level-name (from DAT_00556fe4/…d0)
  LevelLoad            0x429890   (already ported, src/game/level.*)
FrontEndStartLevel     0x415220   ; fade (TransitionControl 0,0), pending name,
                                  ; LevelLoad, clear paused-menu flags,
                                  ; set in-game bit 0x2000000, 0x5704c=0
StartLevelPilots       0x41fa80   ; create Pilot entity, set 0x8000000 bit,
                                  ; enable AI systems (1/2/8/0x400), fade, mission
StartLevelSelected     0x460220   ; commit pending level record → DAT_005529c4,
                                  ; re-create the Pilot
```

## Options / movies (the callbacks the menus invoke)

| addr | name | behavior (observed) |
| --- | --- | --- |
| `0x414ce0` | `AvisEnabled` | `Lego` cfg `DontPlayAvis` gate + `DAT_00505f88 < 3` |
| `0x4157c0` | `MoviePlayLoop` | open movie by config key (`FUN_0047d9d0`), step until done/ESC |
| `0x415910` | `MoviePlayNamed` | movie by key name (boot path) |
| `0x415990` | `MoviePlayLevelVideo` | `<level> Video` key movie (LevelLoad path) |
| `0x415700` | `MoviePlayWait` | FLIC frame loop with quit-probe cancellation |
| `0x414d30` | `BrightnessSet` | param/10 → RGB scalar on `DAT_005560f0/f8` |
| `0x414df0` | `VolumeSet` | param/9 → L/R volume (`FUN_00478980` → audio) |
| `0x414de0` | `GameSpeedSelect` | param → `FUN_0047b530` discrete option table |
| `0x414e10` | `OptionMusicDefault` | default for `DAT_00557038` (getter via `FUN_004789a0`) |
| `0x415030`/`0x414f90`/`0x415360` | `ScrollToSliderIndex`* | `_DAT_0055615c` float → 0..5 slider index into `DAT_00557030` (thresholds 2/3,1,4/3,5/3,2) |
| `0x415150` | `OptionsApplyFromSlot` | on `LevelLoad`: apply slot volume/brightness/speed callbacks |
| `0x416910` | `OptionsWriteToSlot` | stash `DAT_00557030/34/3c` into the current save context |
| `0x416940` | `OptionsLoadOrDefault` | reset front-end globals; load or default `{1,8,5}` |
| `0x4150b0` | `SaveMenuRefresh` | rebuild save/load tree from `DAT_00556fc4` level nodes |
| `0x4152c0` | `SlotSummary` | "%i point" completed-game summary string for slot items |
| `0x415d00` | `SaveMenuLayoutLoad` | `Menu::SaveText/LevelText` Layout keys (`Window`/`Panel`/`SaveSel`/`Level`/`Tutorial`/…) + `Window`/`Panel` rects |
| `0x416150`/`0x4161a0` | `StreamPlayLevel`/`StreamPlayTutorial` | `Stream_LevelName_Level_%d` / `_TLevel_%d` stream triggers |
| `0x4103d0` | `MainMenuTextItem` | the main menu's Credits/text item → `ScrollingTextDisplay @ 0x40a0c0` |

## Port impact (what a `front-end` pass needs)

1. **`block.*` must tokenize `::`** (or the level/menu loaders must translate it).
   This unlocks *every* menu/level query on the shipped `Lego.cfg` and closes the
   README ⚠️ blocker (verified: parsing is fine; the query form was not).
2. Root records: keep the `Lego*` wildcard root; do not hardcode `Main` as parent
   of `Menu`/`Levels`.
3. Level names: drive from `StartLevel`/`TutorialStartLevel`/`NextLevel`
   (`Levels::X`), not a synthetic `Level01`.
4. The front-end is **modal** (`MenuRunLoop` blocks until the menu closes) —
   the frame loop must not run the level sim while a menu is open.
5. The four menus' data model + item grammar (table above) is what
   `src/game/frontend.*` should materialise; the callbacks map onto the existing
   audio/texture/level/savegame modules.

## Menu scrolling — the `CanScroll` flag (`FUN_00413970` @ 0x413970)

`MenuBuild` reads each sub-menu's `CanScroll` key and stores it as bit `0x2` of
the menu record's flag byte (`+ 0x34`). The gate is checked every frame:
`param_1 != 0` and `(*byte *)(param_1 + 0x34) & 2`, with a companion
`DAT_005574dc` zeroed on menu enter.

`FUN_005574e0` (the plane y-offset) accumulates while the pointer is inside the
`y < 480.0` gate (`0x49e19c`) and no wheel is active (`DAT_0076ab3c/40 == 0`),
from the pointer y scaled `* 0.2126` minus `50.0` (constants `0x49e1bc` /
`0x49e1c0`), squared, with a `× 1.5` gain (`0x49e1cc`) and a `× 50.0 / × 0.08`
re-scale per an x87 comparison against `240.0` (`0x49e1c8`) — and clamps:

```c
scroll = clamp(scroll, plane_h - 480 /* viewport height */, 0)
```

(the `* 0.2126 - 50` term crosses zero at y ≈ 235, inside the 200/280 deadzone
comparison constants `0x49e1b4/b8`, so the plane decelerates into the pointer's
band and accelerates away from it; the exact flag-tested ordering of the
×50/×0.08 branch was not fully reconstructed — the port linearises to
`±min(50, distance-from-band)` which reproduces the clamp + deadzone feel).

**Every consumer adds this to the item y** — `FUN_00412c00` (the per-frame menu
tick) does `iVar16 + DAT_005574e0` for both text glyphs (`FUN_0047a180`) and the
pointer hit-test (`FUN_0047dcb0`), so drawing and picking stay aligned as the
plane pans, and `FUN_00411360` (mouse) reads the same global.

The campaign picker plane is one tall strip (`FrontEndY` scatters 113…2096), so
this is the *only* mechanism by which the 24th campaign level becomes
reachable/hittable — no per-item paging.

| Claim | State |
| --- | --- |
| `CanScroll` = menu `+0x34` bit `0x2`, gated in `FUN_00413970` | observed |
| Plane offset = `DAT_005574e0`, clamped `[h-plane_h, 0]`, added to draw + pick | observed |
| Band gate `y < 480`, deadzone comparisons 200.0/280.0, zero-crossing ~235 | observed (constants read) |
| Scale 0.2126 / offset 50.0 / gain 1.5 / rescale 50.0 & 0.08 | observed (constants read) |
| Exact sign/branch ordering of the velocity law | inferred (x87 flags not traced) |
| Shipped `CanScroll` only on `MainMenuFull Menu1` | observed (data) |

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| `MenuBuild`/`MenuRunLoop`/`MenuShow`/`MenuTick` roles + call graph | observed |
| `FrontEndActive` gate = `Lego.cfg FrontEnd` ∧ `mode<3` | observed (both sides) |
| `::` is the config path separator (`DAT_004aab18`) | observed (byte read + BlockFindPath) |
| `DAT_004a24e4` = `"Main"` section root | observed (byte read) |
| `MenuEnter`→`FrontEndStateChange`→`MenuShow`→modal loop order | observed (call edges) |
| Shipped `Lego.cfg` shape (sibling `Menu`/`Levels` under `Lego*`) | observed (data) |
| `StartLevel`/`NextLevel` = `Levels::X` address; `Level01` absent | observed (data) |
| `ScrollToSliderIndex*` / `OptionMusicDefault` exact option binding | inferred (structure; names provisional) |
| `SlotSummary` exact string format `Game %i …` | inferred (format arg — verified signature only) |
| Boot init `FUN_0041fb50` internal order (movies vs menu) | inferred (callee set; not fully decompiled) |