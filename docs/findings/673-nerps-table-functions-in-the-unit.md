# 673. NERPs.c's table functions rebuilt in the unit: GAME-028 reaches its original size

Issue #804, finding 668's follow-up 1, toward a byte-identical image
(#793). Base: `main` `316b70f6`, measured with one full matching build
before (`make reccmp-report` on `316b70f6`) and one after
(`make reccmp-check`, `make match-progress`), both under the gate's link
mode.

## Summary

1. **The 172 NERP table rows that named port-tail adapters now name
   NERPs.c functions.** Together with 12 helpers and object-list or
   block-pointer callbacks that only they reach, 184 original functions
   (5,424 code bytes, 6,512 bytes of span) are rebuilt in
   `src/game/tutorial_script.c` from the original's disassembly. Ghidra
   created none of them, and the original's function list has none of
   them: they are reached only through the table at `004a5948` or through
   an address push. The list is in section 1.
2. **183 of the 184 are exact.** `004552c0` (the powered-building count
   callback) is 40.82%: its size is right and its only difference is
   which of `eax` and `ecx` holds each parameter (section 3).
   `00457170` (`nerps_set_tutorial_pointer`) rose from 95.12% to 100%.
   No function is below the baseline.
3. **GAME-028 is 18,144 bytes against the original's 18,160** (was
   11,632, 6,528 short). The 16 bytes left are `00456830`
   `nerps_parse_message` (67.59%), whose code is 8 bytes shorter than the
   original's and so ends one 16-byte slot early. Every other function in
   the unit starts at the same offset from the unit's start as in the
   original.
4. **The entry point moves from `0048d0d0` to `0048ea60`, 1,440 bytes
   early instead of 7,984.** The unit segment ends 1,456 bytes early
   instead of 8,000.
5. **Linked tail code falls from 203,168 to 177,056 bytes (−26,112), as
   finding 668's "as linked" model predicted.** No unit references
   `tutorial_script_port.cpp` any more. The 172 adapters,
   `nerps_port_call` and the X-macro list
   `tutorial_script_port_functions.h` are deleted. Natively the tutorial
   still runs the port's interpreter.
6. **Exact-function code bytes do not move (340,131, 52.93%)**, because
   `binary_progress.py` sums the original function list's spans, and
   that list has none of the 184 (finding 640 saw the same for 102 of the
   table's rows). Reccmp counts them: exact functions rise from 2,430 to
   2,615 (full) and from 2,142 to 2,327 (game-only).

## Before and after

| Measure | `main` `316b70f6` | This change |
| --- | ---: | ---: |
| Exact-function code bytes (`binary-progress.json`) | 340,131 (52.93%) | 340,131 (52.93%) |
| Reccmp mapped / exact, full | 2,833 / 2,430 | **3,018 / 2,615** |
| Reccmp mapped / exact, game-only | 2,538 / 2,142 | **2,723 / 2,327** |
| Accuracy, full / game-only | 97.20% / 96.88% | **97.35% / 97.07%** |
| `.text` identical at original addresses | 25,975 B (4.04%) | **28,832 B (4.49%)** |
| `.data` / `.rdata` identical | 24.78% / 11.58% | 24.37% / 11.47% |
| Correct-size functions, full | 2,600 (445,640 B) | **2,784 (451,034 B)** |
| Correct-size functions, game-only | 2,309 (394,813 B) | **2,493 (400,207 B)** |
| Identical prefix, raw / address operands masked | 11 / 528 B | 11 / 528 B |
| Units with size agreement (sizes, start, end) | 0 of 227 (166, 2, 1) | 0 of 227 (166, 2, 1) |
| GAME-028 span (original 18,160) | 11,632 (−6,528) | **18,144 (−16)** |
| GAME-021 Lego.c span (original 90,800) | 90,544 (−256) | 90,576 (−224) |
| Entry point (original `0048f000`) | `0048d0d0` (−7,984) | **`0048ea60` (−1,440)** |
| Unit segment end (original `0048d920`) | `0048b9e0` (−8,000) | `0048d370` (−1,456) |
| Linked tail code (`port_tail_inventory.py`) | 203,168 | **177,056** |
| Tail `.bss` / `.data` / `.rdata` | 285,817 / 21,002 / 15,265 | 285,821 / 20,943 / 15,209 |
| SizeOfImage / image end | 4,149,248 / `0x7f5000` | **4,128,768 / `0x7f0000`** |
| Function-order inversions, full / game-only | 222 / 1 pairs | 222 / 1 pairs |
| Reccmp annotation lint ledger | 466 | 466 |

The brief's `main` entry point, `0048d0e0`, is finding 668's base; the
`316b70f6` build measures `0048d0d0`.

**Identity.** The identical prefix does not shrink. No unit's size moves
away from the original's: GAME-028 gains 6,512 bytes toward its size and
Lego.c 32 (section 4). `.data` and `.rdata` identity fall by 0.41 and 0.11
points. No initialised variable of NERPs.c changes; the new functions add
seven strings and their float constants to the unit's literals, which
shifts later literals (`.rdata` grows by 16 bytes). The cause was not
traced further. Per-unit size
agreement stays at 0 because GAME-028's listed functions in the
original's function list are only 30, and `nerps_parse_message` is
still 8 bytes short.

`make reccmp-check` reports no function below the baseline; the baseline
diff is the 185 new functions and `00457170`. `make match-progress`,
`make unit-data-offsets` and `make pe-audit` were run and the generated
files are committed. `make tools-test` (no `--allow-growth`),
`make winapi-link-check`, `make reccmp-lint` and native ctest (37 of 37)
pass, and the native build starts with the shipped WADs.

## 1. The missing functions

The table's 293 rows point at 286 distinct functions: 285 in NERPs.c
(`00452df0`-`004574e0`) and Flic.c's `00484b90` ("True"). Before this
change 113 of the 285 were in `tutorial_script.c` (finding 586's 11,
finding 640's 102), and 172 were adapters. To find the helpers, every
`call`, `push` and `mov` immediate inside the unit's range was added to
the table's targets and the unit's annotations as a function start. That
gives 333 function starts (and the unit's end) and 12 functions that neither the table nor the unit
had: the Set\*Level search and its callback, the selection, driver and
building counts and their callbacks, the rock monster, monster-attack and
camera-lock callbacks, the record-object block callback, and `00456650`
(GetMessageWait, which no row names: SetMessage calls it).

"Code" is the original's bytes without trailing `nop`/`int3` fill;
"span" runs to the next function start. Rows are the table's indices; a
function with several rows lists each.

| Address | Code | Span | Function | Table rows |
| --- | ---: | ---: | --- | --- |
| `004539f0` | 17 | 32 | `nerpfunc_get_messages_are_up_to_date` | GetMessagesAreUpToDate (209) |
| `00453a10` | 55 | 64 | `nerpfunc_supress_arrow` | SupressArrow (206) |
| `00453a50` | 28 | 32 | `nerpfunc_advance_message` | AdvanceMessage (200) |
| `00453a70` | 18 | 32 | `nerpfunc_allow_camera_movement` | AllowCameraMovement (201) |
| `00453a90` | 35 | 48 | `nerpfunc_click_only_objects` | ClickOnlyObjects (202) |
| `00453ac0` | 35 | 48 | `nerpfunc_click_only_map` | ClickOnlyMap (203) |
| `00453af0` | 35 | 48 | `nerpfunc_click_only_icon` | ClickOnlyIcon (204) |
| `00453b20` | 35 | 48 | `nerpfunc_click_only_calltoarms` | ClickOnlyCalltoarms (252) |
| `00453b50` | 35 | 48 | `nerpfunc_disallow_all` | DisallowAll (205) |
| `00453b80` | 34 | 48 | `nerpfunc_flash_call_to_arms_icon` | FlashCallToArmsIcon (253) |
| `00453ca0` | 11 | 16 | `nerpfunc_get_timer0` | GetTimer0 (67) |
| `00453cb0` | 11 | 16 | `nerpfunc_get_timer1` | GetTimer1 (68) |
| `00453cc0` | 11 | 16 | `nerpfunc_get_timer2` | GetTimer2 (69) |
| `00453cd0` | 11 | 16 | `nerpfunc_get_timer3` | GetTimer3 (70) |
| `00453ce0` | 40 | 48 | `nerpfunc_set_timer0` | SetTimer0 (71) |
| `00453d10` | 40 | 48 | `nerpfunc_set_timer1` | SetTimer1 (72) |
| `00453d40` | 40 | 48 | `nerpfunc_set_timer2` | SetTimer2 (73) |
| `00453d70` | 40 | 48 | `nerpfunc_set_timer3` | SetTimer3 (74) |
| `00453fe0` | 54 | 64 | `nerpfunc_camera_lock_on_monster` | CameraLockOnMonster (250) |
| `00454020` | 39 | 48 | `nerps_camera_lock_on_monster_callback` | — (helper) |
| `00454050` | 29 | 32 | `nerpfunc_camera_lock_on_object` | CameraLockOnObject (138) |
| `00454070` | 23 | 32 | `nerpfunc_camera_unlock` | CameraUnlock (139) |
| `00454090` | 121 | 128 | `nerpfunc_camera_zoom_in` | CameraZoomIn (140) |
| `00454110` | 109 | 112 | `nerpfunc_camera_zoom_out` | CameraZoomOut (141) |
| `00454180` | 150 | 160 | `nerpfunc_camera_rotate` | CameraRotate (142) |
| `00454220` | 66 | 80 | `nerpfunc_get_selected_record_object` | GetSelectedRecordObject (144) |
| `00454270` | 26 | 32 | `nerpfunc_set_crystal_priority` | SetCrystalPriority (210) |
| `00454290` | 102 | 112 | `nerpfunc_set_monster_attack_powerstation` | SetMonsterAttackPowerstation (251) |
| `00454300` | 145 | 160 | `nerps_set_monster_attack_callback` | — (helper) |
| `004543a0` | 102 | 112 | `nerpfunc_set_monster_attack_nowt` | SetMonsterAttackNowt (262) |
| `00454410` | 100 | 112 | `nerps_set_objects_level` | — (helper) |
| `00454480` | 56 | 64 | `nerps_set_objects_level_callback` | — (helper) |
| `004544c0` | 107 | 112 | `nerps_record_object_at_tutorial_callback` | — (helper) |
| `00454530` | 102 | 112 | `nerpfunc_get_record_object_at_tutorial` | GetRecordObjectAtTutorial (178) |
| `004545a0` | 86 | 96 | `nerpfunc_get_record_object_amount_at_tutorial` | GetRecordObjectAmountAtTutorial (199) |
| `00454700` | 12 | 16 | `nerpfunc_set_record_object_pointer` | SetRecordObjectPointer (254) |
| `00454720` | 119 | 128 | `nerpfunc_get_oxygen_level` | GetOxygenLevel (181) |
| `004547a0` | 52 | 64 | `nerpfunc_generate_slug` | GenerateSlug (187) |
| `004547e0` | 18 | 32 | `nerpfunc_set_attack_defer` | SetAttackDefer (171) |
| `00454800` | 18 | 32 | `nerpfunc_set_call_to_arms` | SetCallToArms (166) |
| `00454820` | 9 | 16 | `nerpfunc_get_call_to_arms_button_clicked` | GetCallToArmsButtonClicked (160) |
| `00454830` | 55 | 64 | `nerpfunc_set_rock_monster` | SetRockMonster (95) |
| `00454870` | 6 | 16 | `nerpfunc_get_rock_monsters_destroyed` | GetRockMonstersDestroyed (161) |
| `004548a0` | 6 | 16 | `nerpfunc_get_hidden_objects_found` | GetHiddenObjectsFound (179) |
| `004548b0` | 15 | 16 | `nerpfunc_set_hidden_objects_found` | SetHiddenObjectsFound (180) |
| `00454980` | 39 | 48 | `nerpfunc_get_rock_monster_running_away` | GetRockMonsterRunningAway (165) |
| `004549b0` | 165 | 176 | `nerps_rock_monster_callback` | — (helper) |
| `00454a60` | 61 | 64 | `nerpfunc_set_rock_monster_pain_threshold` | SetRockMonsterPainThreshold (172) |
| `00454aa0` | 61 | 64 | `nerpfunc_set_rock_monster_health` | SetRockMonsterHealth (173) |
| `00454ae0` | 23 | 32 | `nerpfunc_set_pause_game` | SetPauseGame (155) |
| `00454b00` | 5 | 16 | `nerpfunc_get_any_key_pressed` | GetAnyKeyPressed (154) |
| `00454b10` | 56 | 64 | `nerpfunc_set_icon_pos` | SetIconPos (85) |
| `00454b50` | 15 | 16 | `nerpfunc_set_icon_space` | SetIconSpace (86) |
| `00454b60` | 15 | 16 | `nerpfunc_set_icon_width` | SetIconWidth (87) |
| `00454ba0` | 16 | 16 | `nerpfunc_set_game_fail` | SetGameFail (11) |
| `00454db0` | 92 | 96 | `nerps_get_selected_count` | — (helper) |
| `00454e10` | 14 | 16 | `nerpfunc_get_mini_figure_selected` | GetMiniFigureSelected (80) |
| `00455060` | 62 | 64 | `nerps_get_driven_count` | — (helper) |
| `004550a0` | 44 | 48 | `nerps_driven_count_callback` | — (helper) |
| `004550d0` | 35 | 48 | `nerpfunc_add_powered_crystals` | AddPoweredCrystals (119) |
| `00455100` | 35 | 48 | `nerpfunc_add_stored_ore` | AddStoredOre (186) |
| `004551f0` | 208 | 208 | `nerps_get_objects_built` | — (helper) |
| `004552c0` | 59 | 64 | `nerps_powered_count_callback` | — (helper) |
| `004553c0` | 14 | 16 | `nerpfunc_get_mini_figurein_granite_grinder` | GetMiniFigureinGraniteGrinder (207) |
| `004553d0` | 14 | 16 | `nerpfunc_get_mini_figurein_chrome_crusher` | GetMiniFigureinChromeCrusher (208) |
| `004553e0` | 14 | 16 | `nerpfunc_get_mini_figurein_small_digger` | GetMiniFigureinSmallDigger (82) |
| `004553f0` | 14 | 16 | `nerpfunc_get_mini_figurein_rapid_rider` | GetMiniFigureinRapidRider (147) |
| `00455400` | 14 | 16 | `nerpfunc_get_mini_figurein_small_truck` | GetMiniFigureinSmallTruck (225) |
| `00455410` | 14 | 16 | `nerpfunc_get_mini_figurein_small_helicopter` | GetMiniFigureinSmallHelicopter (226) |
| `00455420` | 23 | 32 | `nerpfunc_set_barracks_level` | SetBarracksLevel (227) |
| `00455440` | 23 | 32 | `nerpfunc_set_docks_level` | SetDocksLevel (228) |
| `00455460` | 23 | 32 | `nerpfunc_set_geo_dome_level` | SetGeoDomeLevel (229) |
| `00455480` | 23 | 32 | `nerpfunc_set_powerstation_level` | SetPowerstationLevel (230) |
| `004554a0` | 23 | 32 | `nerpfunc_set_tool_store_level` | SetToolStoreLevel (231) |
| `004554c0` | 23 | 32 | `nerpfunc_set_gunstation_level` | SetGunstationLevel (232) |
| `004554e0` | 23 | 32 | `nerpfunc_set_teleport_pad_level` | SetTeleportPadLevel (233) |
| `00455500` | 23 | 32 | `nerpfunc_set_super_teleport_level` | SetSuperTeleportLevel (234) |
| `00455520` | 23 | 32 | `nerpfunc_set_upgrade_station_level` | SetUpgradeStationLevel (235) |
| `00455540` | 14 | 16 | `nerpfunc_get_barracks_selected` | GetBarracksSelected (236) |
| `00455550` | 14 | 16 | `nerpfunc_get_docks_selected` | GetDocksSelected (237) |
| `00455560` | 14 | 16 | `nerpfunc_get_geo_dome_selected` | GetGeoDomeSelected (238) |
| `00455570` | 14 | 16 | `nerpfunc_get_powerstation_selected` | GetPowerstationSelected (239) |
| `00455580` | 14 | 16 | `nerpfunc_get_tool_store_selected` | GetToolStoreSelected (240) |
| `00455590` | 14 | 16 | `nerpfunc_get_gunstation_selected` | GetGunstationSelected (241) |
| `004555a0` | 14 | 16 | `nerpfunc_get_teleport_pad_selected` | GetTeleportPadSelected (242) |
| `004555b0` | 14 | 16 | `nerpfunc_get_super_teleport_selected` | GetSuperTeleportSelected (243) |
| `004555c0` | 14 | 16 | `nerpfunc_get_upgrade_station_selected` | GetUpgradeStationSelected (244) |
| `004555d0` | 16 | 16 | `nerpfunc_get_powered_barracks_built` | GetPoweredBarracksBuilt (284) |
| `004555e0` | 16 | 16 | `nerpfunc_get_powered_docks_built` | GetPoweredDocksBuilt (285) |
| `004555f0` | 16 | 16 | `nerpfunc_get_powered_geodome_built` | GetPoweredGeodomeBuilt (286) |
| `00455600` | 16 | 16 | `nerpfunc_get_powered_powerstations_built` | GetPoweredPowerstationsBuilt (287) |
| `00455610` | 16 | 16 | `nerpfunc_get_powered_tool_stores_built` | GetPoweredToolStoresBuilt (288) |
| `00455620` | 16 | 16 | `nerpfunc_get_powered_gunstations_built` | GetPoweredGunstationsBuilt (289) |
| `00455630` | 16 | 16 | `nerpfunc_get_powered_teleports_built` | GetPoweredTeleportsBuilt (290) |
| `00455640` | 16 | 16 | `nerpfunc_get_powered_vehicle_teleports_built` | GetPoweredVehicleTeleportsBuilt (291) |
| `00455650` | 16 | 16 | `nerpfunc_get_powered_upgrade_stations_built` | GetPoweredUpgradeStationsBuilt (292) |
| `00455660` | 16 | 16 | `nerpfunc_get_barracks_built` | GetBarracksBuilt (75) |
| `00455670` | 16 | 16 | `nerpfunc_get_docks_built` | GetDocksBuilt (195) |
| `00455680` | 16 | 16 | `nerpfunc_get_geodome_built` | GetGeodomeBuilt (185) |
| `00455690` | 16 | 16 | `nerpfunc_get_powerstations_built` | GetPowerstationsBuilt (170) |
| `004556a0` | 16 | 16 | `nerpfunc_get_tool_stores_built` | GetToolStoresBuilt (77) |
| `004556b0` | 16 | 16 | `nerpfunc_get_gunstations_built` | GetGunstationsBuilt (126) |
| `004556c0` | 16 | 16 | `nerpfunc_get_teleports_built` | GetTeleportsBuilt (79) |
| `004556d0` | 16 | 16 | `nerpfunc_get_vehicle_teleports_built` | GetVehicleTeleportsBuilt (128) |
| `004556e0` | 16 | 16 | `nerpfunc_get_upgrade_stations_built` | GetUpgradeStationsBuilt (135) |
| `004556f0` | 16 | 16 | `nerpfunc_get_level1_barracks_built` | GetLevel1BarracksBuilt (266) |
| `00455700` | 16 | 16 | `nerpfunc_get_level1_docks_built` | GetLevel1DocksBuilt (267) |
| `00455710` | 16 | 16 | `nerpfunc_get_level1_geodome_built` | GetLevel1GeodomeBuilt (268) |
| `00455720` | 16 | 16 | `nerpfunc_get_level1_powerstations_built` | GetLevel1PowerstationsBuilt (269) |
| `00455730` | 16 | 16 | `nerpfunc_get_level1_tool_stores_built` | GetLevel1ToolStoresBuilt (270) |
| `00455740` | 16 | 16 | `nerpfunc_get_level1_gunstations_built` | GetLevel1GunstationsBuilt (271) |
| `00455750` | 16 | 16 | `nerpfunc_get_level1_teleports_built` | GetLevel1TeleportsBuilt (272) |
| `00455760` | 16 | 16 | `nerpfunc_get_level1_vehicle_teleports_built` | GetLevel1VehicleTeleportsBuilt (273) |
| `00455770` | 16 | 16 | `nerpfunc_get_level1_upgrade_stations_built` | GetLevel1UpgradeStationsBuilt (274) |
| `00455780` | 16 | 16 | `nerpfunc_get_level2_barracks_built` | GetLevel2BarracksBuilt (275) |
| `00455790` | 16 | 16 | `nerpfunc_get_level2_docks_built` | GetLevel2DocksBuilt (276) |
| `004557a0` | 16 | 16 | `nerpfunc_get_level2_geodome_built` | GetLevel2GeodomeBuilt (277) |
| `004557b0` | 16 | 16 | `nerpfunc_get_level2_powerstations_built` | GetLevel2PowerstationsBuilt (278) |
| `004557c0` | 16 | 16 | `nerpfunc_get_level2_tool_stores_built` | GetLevel2ToolStoresBuilt (279) |
| `004557d0` | 16 | 16 | `nerpfunc_get_level2_gunstations_built` | GetLevel2GunstationsBuilt (280) |
| `004557e0` | 16 | 16 | `nerpfunc_get_level2_teleports_built` | GetLevel2TeleportsBuilt (281) |
| `004557f0` | 16 | 16 | `nerpfunc_get_level2_vehicle_teleports_built` | GetLevel2VehicleTeleportsBuilt (282) |
| `00455800` | 16 | 16 | `nerpfunc_get_level2_upgrade_stations_built` | GetLevel2UpgradeStationsBuilt (283) |
| `00455a90` | 16 | 16 | `nerpfunc_get_paths_built` | GetPathsBuilt (113) |
| `00455aa0` | 16 | 16 | `nerpfunc_get_stud_count` | GetStudCount (78) |
| `00455ab0` | 16 | 16 | `nerpfunc_get_small_helicopters_on_level` | GetSmallHelicoptersOnLevel (198) |
| `00455ac0` | 16 | 16 | `nerpfunc_get_granite_grinders_on_level` | GetGraniteGrindersOnLevel (193) |
| `00455ad0` | 16 | 16 | `nerpfunc_get_rapid_riders_on_level` | GetRapidRidersOnLevel (196) |
| `00455ae0` | 16 | 16 | `nerpfunc_get_small_diggers_on_level` | GetSmallDiggersOnLevel (194) |
| `00455af0` | 16 | 16 | `nerpfunc_get_slugs_on_level` | GetSlugsOnLevel (188) |
| `00455b00` | 16 | 16 | `nerpfunc_get_mini_figures_on_level` | GetMiniFiguresOnLevel (52) |
| `00455b10` | 16 | 16 | `nerpfunc_get_ore_refineries_built` | GetOreRefineriesBuilt (76) |
| `00455b20` | 16 | 16 | `nerpfunc_get_crystal_refineries_built` | GetCrystalRefineriesBuilt (100) |
| `00456190` | 11 | 16 | `nerpfunc_get_random` | GetRandom (4) |
| `004561a0` | 9 | 16 | `nerpfunc_get_random_true_false` | GetRandomTrueFalse (7) |
| `004561b0` | 19 | 32 | `nerpfunc_get_random10` | GetRandom10 (5) |
| `004561d0` | 19 | 32 | `nerpfunc_get_random100` | GetRandom100 (6) |
| `004561f0` | 12 | 16 | `nerpfunc_get_crystals_picked_up` | GetCrystalsPickedUp (12) |
| `00456200` | 11 | 16 | `nerpfunc_get_crystals_currently_stored` | GetCrystalsCurrentlyStored (13) |
| `00456220` | 12 | 16 | `nerpfunc_get_ore_picked_up` | GetOrePickedUp (96) |
| `00456230` | 12 | 16 | `nerpfunc_get_ore_currently_stored` | GetOreCurrentlyStored (97) |
| `00456250` | 6 | 16 | `nerpfunc_get_r0` | GetR0 (17) |
| `00456260` | 6 | 16 | `nerpfunc_get_r1` | GetR1 (18) |
| `00456270` | 6 | 16 | `nerpfunc_get_r2` | GetR2 (19) |
| `00456280` | 6 | 16 | `nerpfunc_get_r3` | GetR3 (20) |
| `00456290` | 6 | 16 | `nerpfunc_get_r4` | GetR4 (21) |
| `004562a0` | 6 | 16 | `nerpfunc_get_r5` | GetR5 (22) |
| `004562b0` | 6 | 16 | `nerpfunc_get_r6` | GetR6 (23) |
| `004562c0` | 6 | 16 | `nerpfunc_get_r7` | GetR7 (24) |
| `004562d0` | 17 | 32 | `nerpfunc_add_r0` | AddR0 (33) |
| `004562f0` | 17 | 32 | `nerpfunc_add_r1` | AddR1 (34) |
| `00456310` | 17 | 32 | `nerpfunc_add_r2` | AddR2 (35) |
| `00456330` | 17 | 32 | `nerpfunc_add_r3` | AddR3 (36) |
| `00456350` | 17 | 32 | `nerpfunc_add_r4` | AddR4 (37) |
| `00456370` | 17 | 32 | `nerpfunc_add_r5` | AddR5 (38) |
| `00456390` | 17 | 32 | `nerpfunc_add_r6` | AddR6 (39) |
| `004563b0` | 17 | 32 | `nerpfunc_add_r7` | AddR7 (40) |
| `004563d0` | 17 | 32 | `nerpfunc_sub_r0` | SubR0 (41) |
| `004563f0` | 17 | 32 | `nerpfunc_sub_r1` | SubR1 (42) |
| `00456410` | 17 | 32 | `nerpfunc_sub_r2` | SubR2 (43) |
| `00456430` | 17 | 32 | `nerpfunc_sub_r3` | SubR3 (44) |
| `00456450` | 17 | 32 | `nerpfunc_sub_r4` | SubR4 (45) |
| `00456470` | 17 | 32 | `nerpfunc_sub_r5` | SubR5 (46) |
| `00456490` | 17 | 32 | `nerpfunc_sub_r6` | SubR6 (47) |
| `004564b0` | 17 | 32 | `nerpfunc_sub_r7` | SubR7 (48) |
| `004564d0` | 15 | 16 | `nerpfunc_set_r0` | SetR0 (25) |
| `004564e0` | 15 | 16 | `nerpfunc_set_r1` | SetR1 (26) |
| `004564f0` | 15 | 16 | `nerpfunc_set_r2` | SetR2 (27) |
| `00456500` | 15 | 16 | `nerpfunc_set_r3` | SetR3 (28) |
| `00456510` | 15 | 16 | `nerpfunc_set_r4` | SetR4 (29) |
| `00456520` | 15 | 16 | `nerpfunc_set_r5` | SetR5 (30) |
| `00456530` | 15 | 16 | `nerpfunc_set_r6` | SetR6 (31) |
| `00456540` | 15 | 16 | `nerpfunc_set_r7` | SetR7 (32) |
| `00456560` | 6 | 16 | `nerpfunc_get_train_flags` | GetTrainFlags (83) |
| `00456570` | 12 | 16 | `nerpfunc_set_train_flags` | SetTrainFlags (84) |
| `00456580` | 54 | 64 | `nerpfunc_get_monsters_on_level` | GetMonstersOnLevel (189) |
| `004565c0` | 5 | 16 | `nerpfunc_get_buildings_teleported` | GetBuildingsTeleported (248) |
| `004565d0` | 18 | 32 | `nerpfunc_set_buildings_teleported` | SetBuildingsTeleported (249) |
| `00456650` | 6 | 16 | `nerpfunc_get_message_wait` | — (helper) |
| `00456660` | 12 | 16 | `nerpfunc_set_message_wait` | SetMessageWait (88) |
| `00456670` | 80 | 80 | `nerpfunc_set_message_timer_values` | SetMessageTimerValues (108) |
| `004566c0` | 11 | 16 | `nerpfunc_get_message_timer` | GetMessageTimer (107) |
| `004566d0` | 238 | 240 | `nerpfunc_set_message` | SetMessage (16) |
| `004567f0` | 21 | 32 | `nerpfunc_get_objective_showing` | GetObjectiveShowing (50) |
| **184** | **5424** | **6512** | | |

## 2. How they are written

Each function is written from its disassembly with the unit's existing
types and globals, in address order, and annotated. The table's
`nerps_port_<name>` entries become the functions' names
(`nerpfunc_<name>` for rows, `nerps_<purpose>` for helpers). The search
records the callbacks share are new typedefs in the unit
(`NERPs_ObjectSearch`, `NERPs_MonsterAttackSearch`,
`NERPs_ObjectLevelSearch`, `NERPs_RockMonsterSearch`,
`NERPs_RecordObjectSearch`), and `NERPs_Level` and `NERPs_Object` gain the
fields the rows read (crystals, ore, ore picked up and oxygen; health,
driver and `flags3`). `nerpsruntimeGlobs.f_994` is renamed
`messageWaitForKey` (SetMessageWait's flag). No storage changes.

The callees are declared in `tutorial_script_imports.h` under names that
link in both builds: the names the other C units already use, with the
matching-build `#define`s of `legoobject_port_aliases.h`,
`aitask_port_aliases.h` and `lego_unit_imports.h` repeated for the ones
whose unit name differs. Reward.c's `rewardGlobs` (GetRockMonstersDestroyed)
and LegoObject.c's `objectGlobs` (the train flags at `004e8444`) come from
`reward.h` and `legoobject_globs.h`, included just before their first use
as `lego_globs.h` already is, so the earlier functions keep their symbol
counter (finding 538). No existing function's score moved from these
includes.

Three source forms were needed beyond a direct transcription:

- **CameraLockOnObject (`00454050`)** stores the index before the flag. The
  original loads `stack[0]` once; storing the flag first makes MSVC reload
  it after the store, which may alias it.
- **The record-object block callback (`004544c0`)** declares the block
  position before the world position, and the world position is a
  three-float vector of which only `x` and `y` are written: the
  original's frame is 0x14 bytes.
- **The rock monster callback (`004549b0`)** writes the pain test as
  `p - 20.0f * (p * 0.01f)`. The original multiplies by 0.01 and then by
  20; written `p - p * 0.01f * 20.0f`, MSVC folds the two constants into
  one `fmul` of 0.2.

SetMessage's key wait (`004566d0`) calls `directdraw_flip`, which the
native port does not have (DirectDraw.c is winapi-only, finding 596). As
`advisor.c` does for `main_get_start_level`, the call is compiled out
under `LEGORRR_BACKEND_MINIWIN`. This is a port deviation, not a scoring
one; the matching build has the call.

## 3. The one function that is not exact

`004552c0` `nerps_powered_count_callback` has the original's 61 bytes and
instructions, with `eax` and `ecx` swapped: the original keeps the object
in `eax` and the search in `ecx`. Its sibling `004550a0`, with the same
first test, is exact with the opposite assignment. Four source forms were
tried (a typed parameter instead of a cast local, inline casts, an
`if`/`else` without the early return, and `return search->count = 0`);
none moves the assignment. It fits finding 663's register choice from the
symbol counter, so it is left, in the plain `if`/`else` form.

## 4. Edits outside the issue's files

- **`src/game/lego_unit.c`** (owned by #807): SetAttackDefer calls Lego.c's
  `Lego_SetAttackDefer` at `00435a80`, which the unit did not have (it is
  one of Lego.c's 256 missing bytes). It is added at its address, between
  `00435a50` and `00435aa0`, as `lego_set_attack_defer`: it sets or clears
  bit 2 of `legoGlobs.flags2`. It is exact, Lego.c's deficit falls to 224
  bytes, and the one Lego.c function after it (`00435aa0`) keeps its
  score. No declaration is added to a header.
- `tools/binary_progress_inputs.txt` drops the deleted X-macro header.

## 5. The tail after this change

`tutorial_script_port.cpp` is no longer referenced from any unit; it
stays linked with 16 bytes of code only through other tail objects.
GAME-028 now reads Input.c's key arrays (`s_global_current_keys`,
`s_global_previous_keys`), so it joins `input.cpp`'s referrers. The
original keeps those arrays in the commons at `0076aa00` and `0076ab80`;
they belong to finding 668's follow-up 3. With GAME-028's root gone, the
anchors keep more of the tail on their own: `tickers.cpp`'s `_TICKERS`
now keeps 94,656 bytes (was 86,576), and 78,192 bytes have no anchor
(was 112,384). The legacy `level.cpp` contributes 7,152 bytes of `.text`
(was 8,736).

## Reproduce

```
python3 tools/port_tail_inventory.py --json /tmp/tail.json
make reccmp-check && make match-progress
python3 tools/readme_status.py
make match-function SOURCE=src/game/tutorial_script.c
```
