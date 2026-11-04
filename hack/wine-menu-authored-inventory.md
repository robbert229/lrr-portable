# Shipped authored menu inventory (read-only source hypotheses, 2026-11-03)
Provenance: external menu-pair-20261103-reference3/config.json points at menu-pair-20261103-production/snapshot. Its inventory.json SHA-256 aba09bd2ce7ea58bf665c8874669e52a8acaad6732d42ed002d95d2f8f174a15, verified.json says verified; verification-diff.json contains four empty differences. Snapshot reference LegoRR0.wad SHA-256 e02fa495f144d31142ef01a15221b1b7865d2a8264c43700523f2b52f91eaf68; LegoRR1.wad 5a967d3147533c61390120f923cea2bd275cc29bc7a8563de21c62bd9de6229d, read O_NOATIME|O_NOFOLLOW and checked against inventory. Existing tools/rrwad.py parsed copied bytes; LegoRR1.wad::Lego.cfg SHA-256 20cbf383bc672b0c330fe45e5fc509371de93d0b46958af82ac1b71f10fccb4f. No protected live reads or old archives. Line numbers below refer to cfg bytes. Current actual pair evidence only establishes settings OK, main quit No and Yes, and visible text spacing divergence, as documented in hack/wine-menu-parity-matrix.md. Every other listed contract is unknown, not passed.

Common acceptance: each row requires ordinary pointer target, keyboard selection/activation, disabled/locked appearance and nonactivation, Escape, visible Back where present, destination, revisit and process shutdown; record initial save/settings, persistence after revisit/restart, candidate/reference identity, and actual effects. Source hypotheses: frontend.cpp handles item navigation and input; frameloop.cpp owns launch/load/pause/restart/quit and credits, mission.cpp owns reward; frontend.cpp:3073 sends nested Escape to root, root pause Escape resumes, other root Escape emits quit. Source is not retail proof. The cfg coordinates are authored targets, not measured hit regions. All record/routing hypotheses require paired tests.

## Exact configured menu item ledger
Every line is one acceptance row: exercise the explicit item, its configured destination/value, all common acceptance requirements. PausedMenu2 Options requires non-tutorial (NotInTuto); check both tutorial and normal. Configured Image Back in SaveMenu closes reward modal, whereas other Back entries follow Next. Pause confirmation Yes triggers quit or restart according to submenu, No goes Menu1; observe application lifecycle and progress. Main quit Yes exits, No goes Main per candidate caller. Standalone OptionsMenu has no established caller; do not assume reachable. Replay Objective is authored in pause options but candidate trigger is a no-op. Item titles and layout follow their containing MenuN.

- L349 `PausedMenu::Menu1::Item1 = Trigger:60:50:Continue_Game:1`
- L350 `PausedMenu::Menu1::Item2 = Next:60:80:Options:Menu2:NotInTuto`
- L351 `PausedMenu::Menu1::Item3 = Next:60:110:Advanced_Options:Menu5`
- L352 `PausedMenu::Menu1::Item4 = Next:60:140:Restart_Mission:Menu4`
- L353 `PausedMenu::Menu1::Item5 = Next:60:170:Quit:Menu3`
- L368 `PausedMenu::Menu2::Item1 = Slider:-80:30:255:0:Game_Speed:0:5:Interface\FrontEnd\Vol_OffBar.bmp:Interface\FrontEnd\Vol_OnBar.bmp:Interface\FrontEnd\Vol_Leftcap.bmp:Interface\FrontEnd\Vol_Rightcap.bmp:Interface\FrontEnd\Vol_Plus.bmp:Interface\FrontEnd\Vol_Minus.bmp:Interface\FrontEnd\Vol_PlusHi.bmp:Interface\FrontEnd\Vol_MinusHi.bmp`
- L369 `PausedMenu::Menu2::Item2 = Slider:-80:55:255:0:SFX_Volume:0:10:Interface\FrontEnd\Vol_OffBar.bmp:Interface\FrontEnd\Vol_OnBar.bmp:Interface\FrontEnd\Vol_Leftcap.bmp:Interface\FrontEnd\Vol_Rightcap.bmp:Interface\FrontEnd\Vol_Plus.bmp:Interface\FrontEnd\Vol_Minus.bmp:Interface\FrontEnd\Vol_PlusHi.bmp:Interface\FrontEnd\Vol_MinusHi.bmp`
- L370 `PausedMenu::Menu2::Item3 = Slider:-80:80:255:0:Music_Volume:0:10:Interface\FrontEnd\Vol_OffBar.bmp:Interface\FrontEnd\Vol_OnBar.bmp:Interface\FrontEnd\Vol_Leftcap.bmp:Interface\FrontEnd\Vol_Rightcap.bmp:Interface\FrontEnd\Vol_Plus.bmp:Interface\FrontEnd\Vol_Minus.bmp:Interface\FrontEnd\Vol_PlusHi.bmp:Interface\FrontEnd\Vol_MinusHi.bmp`
- L371 `PausedMenu::Menu2::Item4 = Slider:-80:105:255:0:Brightness:0:10:Interface\FrontEnd\Vol_OffBar.bmp:Interface\FrontEnd\Vol_OnBar.bmp:Interface\FrontEnd\Vol_Leftcap.bmp:Interface\FrontEnd\Vol_Rightcap.bmp:Interface\FrontEnd\Vol_Plus.bmp:Interface\FrontEnd\Vol_Minus.bmp:Interface\FrontEnd\Vol_PlusHi.bmp:Interface\FrontEnd\Vol_MinusHi.bmp`
- L372 `PausedMenu::Menu2::Item5 = Cycle:-80:130:255:0:Help_Window:2:Off:On`
- L373 `PausedMenu::Menu2::Item6 = Trigger:-80:155:Replay_Objective:1`
- L374 `PausedMenu::Menu2::Item7 = Next:-80:180:Back:Menu1`
- L390 `PausedMenu::Menu3::Item1 = Trigger:20:70:Yes_-_Quit:1`
- L391 `PausedMenu::Menu3::Item2 = Next:20:95:No_-_Continue_with_Mission:Menu1`
- L407 `PausedMenu::Menu4::Item1 = Trigger:20:70:Yes_-_Restart_Mission:1`
- L408 `PausedMenu::Menu4::Item2 = Next:20:95:No_-_Continue_with_Mission:Menu1`
- L423 `PausedMenu::Menu5::Item1 = Cycle:-150:45:310:0:Wall_Detail:2:High:Low`
- L424 `PausedMenu::Menu5::Item2 = Cycle:-150:70:310:0:Music:2:On:Off`
- L425 `PausedMenu::Menu5::Item3 = Cycle:-150:95:310:0:Sound_Effects:2:On:Off`
- L426 `PausedMenu::Menu5::Item4 = Cycle:-150:120:310:0:Automatic_Game_Speed:2:On:Off`
- L427 `PausedMenu::Menu5::Item5 = Next:-150:145:Back:Menu1`
- L455 `SaveMenu::Menu1::Item1 = Trigger:-314:236:Interface\FrontEnd\LP_Normal.bmp:Interface\FrontEnd\LP_Glow.bmp:Interface\FrontEnd\LP_Dull.bmp:ToolTip_FrontEnd_Back:1`
- L484 `MainMenuFull::Menu1::Item1 = Next:0:0:Start_Game:Menu2`
- L485 `MainMenuFull::Menu1::Item2 = Next:0:40:Load_A_Saved_Game:Menu4`
- L486 `MainMenuFull::Menu1::Item3 = Next:0:80:Training_Missions:Menu3`
- L487 `MainMenuFull::Menu1::Item4 = Trigger:0:120:Credits:0`
- L488 `MainMenuFull::Menu1::Item5 = Next:0:160:Quit_Game:Menu5`
- L504 `MainMenuFull::Menu2::Item1 = Next:6:436:Interface\FrontEnd\LP_Normal.bmp:Interface\FrontEnd\LP_Glow.bmp:Interface\FrontEnd\LP_Dull.bmp:ToolTip_FrontEnd_Back:Menu1`
- L519 `MainMenuFull::Menu3::Item1 = Next:6:436:Interface\FrontEnd\LP_Normal.bmp:Interface\FrontEnd\LP_Glow.bmp:Interface\FrontEnd\LP_Dull.bmp:ToolTip_FrontEnd_Back:Menu1`
- L540 `MainMenuFull::Menu4::Item1 = Next:-314:236:Interface\FrontEnd\LP_Normal.bmp:Interface\FrontEnd\LP_Glow.bmp:Interface\FrontEnd\LP_Dull.bmp:ToolTip_FrontEnd_Back:Menu1`
- L565 `MainMenuFull::Menu5::Item1 = Trigger:0:60:Yes:1`
- L566 `MainMenuFull::Menu5::Item2 = Next:0:100:No:Menu1`
- L583 `OptionsMenu::Menu1::Item1 = Slider:-80:30:255:0:Game_Speed:0:5:Interface\FrontEnd\Vol_OffBar.bmp:Interface\FrontEnd\Vol_OnBar.bmp:Interface\FrontEnd\Vol_Leftcap.bmp:Interface\FrontEnd\Vol_Rightcap.bmp:Interface\FrontEnd\Vol_Plus.bmp:Interface\FrontEnd\Vol_Minus.bmp:Interface\FrontEnd\Vol_PlusHi.bmp:Interface\FrontEnd\Vol_MinusHi.bmp`
- L584 `OptionsMenu::Menu1::Item2 = Slider:-80:55:255:0:SFX_Volume:0:10:Interface\FrontEnd\Vol_OffBar.bmp:Interface\FrontEnd\Vol_OnBar.bmp:Interface\FrontEnd\Vol_Leftcap.bmp:Interface\FrontEnd\Vol_Rightcap.bmp:Interface\FrontEnd\Vol_Plus.bmp:Interface\FrontEnd\Vol_Minus.bmp:Interface\FrontEnd\Vol_PlusHi.bmp:Interface\FrontEnd\Vol_MinusHi.bmp`
- L585 `OptionsMenu::Menu1::Item3 = Slider:-80:80:255:0:Music_Volume:0:10:Interface\FrontEnd\Vol_OffBar.bmp:Interface\FrontEnd\Vol_OnBar.bmp:Interface\FrontEnd\Vol_Leftcap.bmp:Interface\FrontEnd\Vol_Rightcap.bmp:Interface\FrontEnd\Vol_Plus.bmp:Interface\FrontEnd\Vol_Minus.bmp:Interface\FrontEnd\Vol_PlusHi.bmp:Interface\FrontEnd\Vol_MinusHi.bmp`
- L586 `OptionsMenu::Menu1::Item4 = Slider:-80:105:255:0:Brightness:0:10:Interface\FrontEnd\Vol_OffBar.bmp:Interface\FrontEnd\Vol_OnBar.bmp:Interface\FrontEnd\Vol_Leftcap.bmp:Interface\FrontEnd\Vol_Rightcap.bmp:Interface\FrontEnd\Vol_Plus.bmp:Interface\FrontEnd\Vol_Minus.bmp:Interface\FrontEnd\Vol_PlusHi.bmp:Interface\FrontEnd\Vol_MinusHi.bmp`
- L587 `OptionsMenu::Menu1::Item5 = Cycle:-80:130:255:0:Help_Window:2:Off:On`
- L588 `OptionsMenu::Menu1::Item6 = Trigger:-80:155:Replay_Objective:1`
- L589 `OptionsMenu::Menu1::Item7 = Trigger:-80:180:Continue_Game:1`

## Dynamic slots and confirmations
MainMenuFull Menu4 and SaveMenu Menu1 additionally render five dynamic slots (1–5), each an independent acceptance row in empty and occupied states. Load: test selection, selected record/level, back, empty behavior, revisit and restart; candidate frontend.cpp:2836 emits LOAD even when empty, frameloop.cpp:3067 may fall back to Level01. Save: test cancellation, each empty and each occupied slot, overwrite confirmation presence/absence, file contents and reload; frontend.cpp:2836 emits SAVE, frameloop.cpp:3807 writes checkpoint. No overwrite confirmation menu is authored in these four menu sets; do not infer retail has none. Save Back and Escape must each preserve slot state; Escape emits QUIT in candidate but reward caller discards it. There are five configured overlays per load/save menu; candidate models six rows, requiring check of any extra row.

## Level nodes
Each node row requires fresh/progressed save, enabled/disabled state and lock reason, hover/click and keyboard, destination/briefing, visible Back/Escape, scrolling/pages, return and shutdown. FrontEndOpen/LevelLinks are authored facts, not proven exact unlock algorithm. List includes nodes reachable by transitive configured LevelLinks starting at the two StartLevel roots; NextLevel is the authored progression and is separately shown. Other configured Levels outside those graphs are not asserted reachable.


### Tutorial01
- L3283 `Tutorial01`: FullName=1._Moving_Rock_Raiders; NextLevel=Levels::Tutorial02; LevelLinks=Levels::Tutorial02; FrontEndX=101; FrontEndY=113; FrontEndOpen=TRUE; MenuBMP=Levels\TutorialLevels\Tutorial01.bmp,Levels\TutorialLevels\GTutorial01.bmp,Levels\TutorialLevels\Tutorial01G.bmp
- L3352 `Tutorial02`: FullName=2._Rock_Types_And_Drilling; NextLevel=Levels::Tutorial03; LevelLinks=Levels::Tutorial03; FrontEndX=282; FrontEndY=121; MenuBMP=Levels\TutorialLevels\Tutorial02.bmp,Levels\TutorialLevels\GTutorial02.bmp,Levels\TutorialLevels\Tutorial02G.bmp
- L3418 `Tutorial03`: FullName=3._Building_Rock_Raider_HQ; NextLevel=Levels::Tutorial04; LevelLinks=Levels::Tutorial04; FrontEndX=450; FrontEndY=122; MenuBMP=Levels\TutorialLevels\Tutorial03.bmp,Levels\TutorialLevels\GTutorial03.bmp,Levels\TutorialLevels\Tutorial03G.bmp
- L3488 `Tutorial04`: FullName=4._Moving_Rock_Raider_Vehicles; NextLevel=Levels::Tutorial05; LevelLinks=Levels::Tutorial05; FrontEndX=28; FrontEndY=230; MenuBMP=Levels\TutorialLevels\Tutorial04.bmp,Levels\TutorialLevels\GTutorial04.bmp,Levels\TutorialLevels\Tutorial04G.bmp
- L3557 `Tutorial05`: FullName=5._Using_Explosives; NextLevel=Levels::Tutorial06; LevelLinks=Levels::Tutorial06; FrontEndX=268; FrontEndY=260; MenuBMP=Levels\TutorialLevels\Tutorial05.bmp,Levels\TutorialLevels\GTutorial05.bmp,Levels\TutorialLevels\Tutorial05G.bmp
- L3624 `Tutorial06`: FullName=6._Constructing_Advanced_Buildings; NextLevel=Levels::Tutorial07; LevelLinks=Levels::Tutorial07; FrontEndX=501; FrontEndY=238; MenuBMP=Levels\TutorialLevels\Tutorial06.bmp,Levels\TutorialLevels\GTutorial06.bmp,Levels\TutorialLevels\Tutorial06G.bmp
- L3692 `Tutorial07`: FullName=7._Drilling_With_Vehicles; NextLevel=Levels::Tutorial08; LevelLinks=Levels::Tutorial08; FrontEndX=123; FrontEndY=307; MenuBMP=Levels\TutorialLevels\Tutorial07.bmp,Levels\TutorialLevels\GTutorial07.bmp,Levels\TutorialLevels\Tutorial07G.bmp
- L3760 `Tutorial08`: FullName=8._Defending_Rock_Raider_HQ; NextLevel=Levels::Level01; LevelLinks=Levels::Level01; FrontEndX=415; FrontEndY=307; MenuBMP=Levels\TutorialLevels\Tutorial08.bmp,Levels\TutorialLevels\GTutorial08.bmp,Levels\TutorialLevels\Tutorial08G.bmp
- L3830 `Level01`: FullName=Driller_Night!; MenuBMP=Levels\GameLevels\Level01.bmp,Levels\GameLevels\GLevel01.bmp,Levels\GameLevels\Level01G.bmp; NextLevel=Levels::Level05; LevelLinks=Levels::Level05,Levels::Level03; FrontEndX=265; FrontEndY=475; FrontEndOpen=TRUE
- L4197 `Level05`: FullName=The_Path_To_Power; MenuBMP=Levels\GameLevels\Level02.bmp,Levels\GameLevels\GLevel02.bmp,Levels\GameLevels\Level02G.bmp; NextLevel=Levels::Level03; LevelLinks=Levels::Level04,Levels::Level02; FrontEndX=151; FrontEndY=555
- L4015 `Level03`: FullName=Rubble_Trouble!; MenuBMP=Levels\GameLevels\Level03.bmp,Levels\GameLevels\GLevel03.bmp,Levels\GameLevels\Level03G.bmp; NextLevel=Levels::Level04; LevelLinks=Levels::Level02,Levels::Level06; FrontEndX=392; FrontEndY=573
- L4104 `Level04`: FullName=A_Breath_Of_Fresh_Air; MenuBMP=Levels\GameLevels\Level04.bmp,Levels\GameLevels\GLevel04.bmp,Levels\GameLevels\Level04G.bmp; NextLevel=Levels::Level02; LevelLinks=Levels::Level07; FrontEndX=32; FrontEndY=659
- L3921 `Level02`: FullName=It's_A_Hold_Up; MenuBMP=Levels\GameLevels\Level05.bmp,Levels\GameLevels\GLevel05.bmp,Levels\GameLevels\Level05G.bmp; NextLevel=Levels::Level06; LevelLinks=Levels::Level07,Levels::Level08; FrontEndX=266; FrontEndY=652
- L4293 `Level06`: FullName=Explosive_Action; MenuBMP=Levels\GameLevels\Level06.bmp,Levels\GameLevels\GLevel06.bmp,Levels\GameLevels\Level06G.bmp; NextLevel=Levels::Level07; LevelLinks=Levels::Level08; FrontEndX=486; FrontEndY=656
- L4386 `Level07`: FullName=Search_'n'_Rescue; MenuBMP=Levels\GameLevels\Level07.bmp,Levels\GameLevels\GLevel07.bmp,Levels\GameLevels\Level07G.bmp; NextLevel=Levels::Level08; LevelLinks=Levels::Level09; FrontEndX=125; FrontEndY=845
- L4476 `Level08`: FullName=Breathless; MenuBMP=Levels\GameLevels\Level08.bmp,Levels\GameLevels\GLevel08.bmp,Levels\GameLevels\Level08G.bmp; NextLevel=Levels::Level09; LevelLinks=Levels::Level09; FrontEndX=339; FrontEndY=847
- L4569 `Level09`: FullName=Frozen_Frenzy; MenuBMP=Levels\GameLevels\Level09.bmp,Levels\GameLevels\GLevel09.bmp,Levels\GameLevels\Level09G.bmp; NextLevel=Levels::Level10; LevelLinks=Levels::Level10,Levels::Level11; FrontEndX=213; FrontEndY=971
- L4662 `Level10`: FullName=Erode_Works; MenuBMP=Levels\GameLevels\Level10.bmp,Levels\GameLevels\GLevel10.bmp,Levels\GameLevels\Level10G.bmp; NextLevel=Levels::Level11; LevelLinks=Levels::Level12,Levels::Level13; FrontEndX=43; FrontEndY=1105
- L4755 `Level11`: FullName=Water_Lot_Of_Fun; MenuBMP=Levels\GameLevels\Level11.bmp,Levels\GameLevels\GLevel11.bmp,Levels\GameLevels\Level11G.bmp; NextLevel=Levels::Level12; LevelLinks=Levels::Level13,Levels::Level14; FrontEndX=444; FrontEndY=1088
- L4848 `Level12`: FullName=Water_Works; MenuBMP=Levels\GameLevels\Level12.bmp,Levels\GameLevels\GLevel12.bmp,Levels\GameLevels\Level12G.bmp; NextLevel=Levels::Level13; LevelLinks=Levels::Level15; FrontEndX=46; FrontEndY=1263
- L4940 `Level13`: FullName=Rock_Hard; MenuBMP=Levels\GameLevels\Level13.bmp,Levels\GameLevels\GLevel13.bmp,Levels\GameLevels\Level13G.bmp; NextLevel=Levels::Level14; LevelLinks=Levels::Level15,Levels::Level16; FrontEndX=251; FrontEndY=1273
- L5037 `Level14`: FullName=Don't_Panic!; MenuBMP=Levels\GameLevels\Level14.bmp,Levels\GameLevels\GLevel14.bmp,Levels\GameLevels\Level14G.bmp; NextLevel=Levels::Level15; LevelLinks=Levels::Level16; FrontEndX=448; FrontEndY=1274
- L5131 `Level15`: FullName=Ice_Spy; MenuBMP=Levels\GameLevels\Level15.bmp,Levels\GameLevels\GLevel15.bmp,Levels\GameLevels\Level15G.bmp; NextLevel=Levels::Level16; LevelLinks=Levels::Level17; FrontEndX=55; FrontEndY=1407
- L5220 `Level16`: FullName=Split_Down_The_Middle; NextLevel=Levels::Level17; LevelLinks=Levels::Level17; FrontEndX=407; FrontEndY=1410; MenuBMP=Levels\GameLevels\Level16.bmp,Levels\GameLevels\GLevel16.bmp,Levels\GameLevels\Level16G.bmp
- L5306 `Level17`: FullName=Lava_Laughter; MenuBMP=Levels\GameLevels\Level17.bmp,Levels\GameLevels\GLevel17.bmp,Levels\GameLevels\Level17G.bmp; NextLevel=Levels::Level18; LevelLinks=Levels::Level18,Levels::Level19; FrontEndX=236; FrontEndY=1510
- L5396 `Level18`: FullName=Oresome!; MenuBMP=Levels\GameLevels\Level18.bmp,Levels\GameLevels\GLevel18.bmp,Levels\GameLevels\Level18G.bmp; NextLevel=Levels::Level19; LevelLinks=Levels::Level20,Levels::Level21; FrontEndX=96; FrontEndY=1704
- L5493 `Level19`: FullName=Fire_'n'_Water; MenuBMP=Levels\GameLevels\Level19.bmp,Levels\GameLevels\GLevel19.bmp,Levels\GameLevels\Level19G.bmp; NextLevel=Levels::Level20; LevelLinks=Levels::Level21,Levels::Level22; FrontEndX=376; FrontEndY=1687
- L5579 `Level20`: FullName=Run_The_Gauntlet; MenuBMP=Levels\GameLevels\Level20.bmp,Levels\GameLevels\GLevel20.bmp,Levels\GameLevels\Level20G.bmp; NextLevel=Levels::Level21; LevelLinks=Levels::Level23; FrontEndX=32; FrontEndY=1805
- L5673 `Level21`: FullName=Air_Raiders; MenuBMP=Levels\GameLevels\Level21.bmp,Levels\GameLevels\GLevel21.bmp,Levels\GameLevels\Level21G.bmp; NextLevel=Levels::Level22; LevelLinks=Levels::Level23,Levels::Level24; FrontEndX=235; FrontEndY=1795
- L5769 `Level22`: FullName=Lake_Of_Fire; MenuBMP=Levels\GameLevels\Level22.bmp,Levels\GameLevels\GLevel22.bmp,Levels\GameLevels\Level22G.bmp; NextLevel=Levels::Level23; LevelLinks=Levels::Level24; FrontEndX=454; FrontEndY=1816
- L5857 `Level23`: FullName=Back_To_Basics; MenuBMP=Levels\GameLevels\Level23.bmp,Levels\GameLevels\GLevel23.bmp,Levels\GameLevels\Level23G.bmp; NextLevel=Levels::Level24; LevelLinks=Levels::Level25; FrontEndX=110; FrontEndY=1909
- L5948 `Level24`: FullName=Hot_Stuff; MenuBMP=Levels\GameLevels\Level24.bmp,Levels\GameLevels\GLevel24.bmp,Levels\GameLevels\Level24G.bmp; NextLevel=Levels::Level25; LevelLinks=Levels::Level25; FrontEndX=359; FrontEndY=1919
- L6038 `Level25`: FullName=Rocky_Horror; MenuBMP=Levels\GameLevels\Level25.bmp,Levels\GameLevels\GLevel25.bmp,Levels\GameLevels\Level25G.bmp; FrontEndX=163; FrontEndY=2096
Nodes in graph: 33

### MoveTuto_01
- L6212 `MoveTuto_01`: FullName=1._Moving_Rock_Raiders; NextLevel=Levels::DigTuto_01; LevelLinks=Levels::DigTuto_01; FrontEndX=101; FrontEndY=113; FrontEndOpen=TRUE; MenuBMP=Levels\TutorialLevels\Tutorial01.bmp,Levels\TutorialLevels\GTutorial01.bmp,Levels\TutorialLevels\Tutorial01G.bmp
- L6283 `DigTuto_01`: FullName=2._Rock_Types_And_Drilling; NextLevel=Levels::BuildTuto_01; LevelLinks=Levels::BuildTuto_01; FrontEndX=282; FrontEndY=121; FrontEndOpen=TRUE; MenuBMP=Levels\TutorialLevels\Tutorial02.bmp,Levels\TutorialLevels\GTutorial02.bmp,Levels\TutorialLevels\Tutorial02G.bmp
- L6349 `BuildTuto_01`: FullName=3._Building_Rock_Raider_HQ; NextLevel=Levels::MoveTuto_02; LevelLinks=Levels::MoveTuto_02; FrontEndX=450; FrontEndY=122; FrontEndOpen=TRUE; MenuBMP=Levels\TutorialLevels\Tutorial03.bmp,Levels\TutorialLevels\GTutorial03.bmp,Levels\TutorialLevels\Tutorial03G.bmp
- L6419 `MoveTuto_02`: FullName=4._Moving_Rock_Raider_Vehicles; NextLevel=Levels::DigTuto_02; LevelLinks=Levels::DigTuto_02; FrontEndX=28; FrontEndY=230; FrontEndOpen=TRUE; MenuBMP=Levels\TutorialLevels\Tutorial04.bmp,Levels\TutorialLevels\GTutorial04.bmp,Levels\TutorialLevels\Tutorial04G.bmp
- L6488 `DigTuto_02`: FullName=5._Using_Explosives; NextLevel=Levels::BuildTuto_02; LevelLinks=Levels::BuildTuto_02; FrontEndX=268; FrontEndY=260; FrontEndOpen=TRUE; MenuBMP=Levels\TutorialLevels\Tutorial05.bmp,Levels\TutorialLevels\GTutorial05.bmp,Levels\TutorialLevels\Tutorial05G.bmp
- L6555 `BuildTuto_02`: FullName=6._Constructing_Advanced_Buildings; NextLevel=Levels::DigTuto_03; LevelLinks=Levels::DigTuto_03; FrontEndX=501; FrontEndY=238; FrontEndOpen=TRUE; MenuBMP=Levels\TutorialLevels\Tutorial06.bmp,Levels\TutorialLevels\GTutorial06.bmp,Levels\TutorialLevels\Tutorial06G.bmp
- L6623 `DigTuto_03`: FullName=7._Drilling_With_Vehicles; NextLevel=Levels::DefenceTuto_01; LevelLinks=Levels::DefenceTuto_01; FrontEndX=123; FrontEndY=307; FrontEndOpen=TRUE; MenuBMP=Levels\TutorialLevels\Tutorial07.bmp,Levels\TutorialLevels\GTutorial07.bmp,Levels\TutorialLevels\Tutorial07G.bmp
- L6691 `DefenceTuto_01`: FullName=8._Defending_Rock_Raider_HQ; FrontEndX=415; FrontEndY=307; FrontEndOpen=TRUE; MenuBMP=Levels\TutorialLevels\Tutorial08.bmp,Levels\TutorialLevels\GTutorial08.bmp,Levels\TutorialLevels\Tutorial08G.bmp
Nodes in graph: 8

## Other surfaces and branches
- Startup device/mode/fullscreen choices, OK, Cancel, Escape, startup persistence: settings are not authored in these frontend menu blocks; current pair reached main by OK, with fullscreen selector difference. Inspect actual reference vs candidate on a controlled pair.
- Main Credits trigger: observe rendering, dismissal via pointer/key/Escape and restoration; frontend.cpp:2900, frameloop.cpp:3100.
- Briefing: each mission start and restart should exercise objective/briefing text, Next and Back (Main NextButton640x480=Interface\MessagePanel\OKButton.bmp, NextButtonPos640x480=380,447; BackButton640x480=Interface\MessagePanel\repeat.bmp, BackButtonPos640x480=380,462); record objective page count and controls. Per-level objective page counts are enumerated below.
- Reward SaveButton L242 = Interface\Reward\btn_save.bmp|Interface\Reward\btn_save_hi.bmp|Interface\Reward\btn_save_in.bmp|Interface\Reward\btn_save_dim.bmp|10|410; AdvanceButton L243 = Interface\Reward\btn_adv.bmp|Interface\Reward\btn_adv_hi.bmp|Interface\Reward\btn_adv_in.bmp|Interface\Reward\btn_adv_dim.bmp|570|410. For success and failure independently exercise both buttons before/after task reveal, disabled states, Save cancellation/slots, advance, progress, final-level termination. Candidate mission.cpp:1239 gates success Save versus non-success Advance despite both being drawn; caller phases differ (frameloop.cpp:345,4345). Paired evidence required.
- Restart No/Escape preserves mission; Yes reloads candidate application level (frameloop.cpp:3966); after advancing, check candidate stale application level-name risk. Pause Quit Yes candidate exits application, historical reference may return to training; compare same state. No/Escape must be tested.
- Nine options are four sliders (speed 0–5; SFX/music/brightness 0–10) and five cycles (Help Off/On; Wall High/Low; Music, Sound, Automatic Speed On/Off). For each limit, intermediate step, pointer drag, keyboard, reverse/wrap, effective behavior and persistence in save and restart. Pause options contains first five and replay; advanced contains last four. Config does not provide Apply/Cancel; Back and Escape must be tested for effects.

Completeness boundary: exact configured Item rows in four frontend sets and reachable LevelLinks graph are transcribed. Startup actual behavior, conditional authored controls in other data files, retail input/lock rules, actual overwrite behavior, and every unobserved paired branch remain unverified; therefore this is not a complete acceptance proof or exhaustive all-data inventory. See hack/wine-menu-caller-contract-trace.md for deeper lifecycle hypotheses.

## Startup settings: individual acceptance IDs
Resource owner src/version.rc:40–58, candidate handler src/gods98/surface.cpp:4900–5098; actual paired screenshot reference3/settings.png and candidate2/settings.png. Screenshots show reference Driver DirectDraw HAL (display), Device choices RGB Emulation and Direct3D HAL (Wine D3DDevice using WineD3D and OpenGL), 640x480 (16 bit), Full Screen selected, Window, OK, Cancel. Candidate shows Primary Display Adapter, DirectDraw HAL, 640 x 480 x 16, OK and Cancel; radio controls are not visible in captured candidate. Both used OK to reach main; actual control interaction and persistence beyond that remain unknown.
- S01 Driver combo id 1000: enumerate/select every available driver with mouse/keyboard; validate dependent device/mode lists and default. Candidate hardcodes single Primary Display Adapter.
- S02 Device combo id 1002: enumerate/select each available hardware/software device, dependencies and disabled states. Candidate hardcodes DirectDraw HAL.
- S03 Screen Modes combo id 1003: select each available resolution/depth and keyboard operation; candidate enumerates DirectDraw modes, excludes below 16 bits, applies configured tuple filter and prefers 640x480, with code ordering caveat.
- S04 Full Screen radio id 1004: selection, focus, mode list, actual resulting surface/window and restart persistence. Candidate sets default selected but capture did not show it.
- S05 Window radio id 1005: same tests and switching in both directions.
- S06 OK id 1: mouse/Enter, valid/invalid/no mode, resulting display, main entry, persistence and clean shutdown; candidate rejects absent selection.
- S07 Cancel id 2: mouse, keyboard/Escape/window close, clean exit/no save or settings mutation; restart and compare. Escape/close routing is OS dialog behavior and requires observation.

## Objective/briefing acceptance IDs, per configured level
`Languages\ObjectiveText.txt` inside authenticated LegoRR1.wad has SHA-256 711dae8ae0a1b7b4d923ecb6335e8b0e32b6008e3776616c767d840e9f45fed8. For these 41 graph nodes each cfg ObjectiveText names that resource; section matches node name. Counts below are derived from nonempty `Objective:` line and literal `\a` separators, as candidate frameloop_brief_load src/gods98/frameloop.cpp:2559–2621 parses them; `\n` is a line break, not a page. Each entry is an independent level/page-count acceptance row; per-page instances Bxx.1...Bxx.N must check correct page contents/order, text wrapping, transitions, input gating, no premature world input, final acknowledgment and destination. Verify visible Next and Back and pointer/key/Escape on each page; actual retail semantics are not established by this data. Candidate frameloop.cpp:3714–3777 advances on any mouse press or Space/Enter, acknowledges only final page, does not implement back in this path; rendered prompt says Press SPACE to continue (frameloop.cpp:2660). Config L25–29 supplies NextButton640x480, NextButtonPos640x480, BackButton640x480, BackButtonPos640x480 and BackArrow; these are general assets, not proof they occur on briefing. Parameterizing the same navigation contract is appropriate, but every unique level and page content still needs individual evidence; aggregate pass does not cover all pages.

| ID | Configured level / objective section | Objective pages | Failure / Completion / CrystalFailure pages |
| --- | --- | ---: | --- |
| B01 | `Tutorial01` | 2 | 1 / 1 / 1 |
| B02 | `Tutorial02` | 1 | 1 / 1 / 1 |
| B03 | `Tutorial03` | 1 | 1 / 1 / 1 |
| B04 | `Tutorial04` | 1 | 1 / 1 / 1 |
| B05 | `Tutorial05` | 1 | 1 / 1 / 1 |
| B06 | `Tutorial06` | 1 | 1 / 1 / 1 |
| B07 | `Tutorial07` | 1 | 1 / 1 / 1 |
| B08 | `Tutorial08` | 1 | 1 / 1 / 1 |
| B09 | `Level01` | 2 | 1 / 1 / 1 |
| B10 | `Level05` | 2 | 1 / 1 / 1 |
| B11 | `Level03` | 2 | 1 / 1 / 1 |
| B12 | `Level04` | 2 | 1 / 1 / 1 |
| B13 | `Level02` | 2 | 1 / 1 / 1 |
| B14 | `Level06` | 3 | 1 / 1 / 1 |
| B15 | `Level07` | 2 | 1 / 1 / 1 |
| B16 | `Level08` | 3 | 1 / 1 / 1 |
| B17 | `Level09` | 3 | 1 / 1 / 1 |
| B18 | `Level10` | 3 | 1 / 1 / 1 |
| B19 | `Level11` | 2 | 1 / 1 / 1 |
| B20 | `Level12` | 2 | 1 / 1 / 1 |
| B21 | `Level13` | 2 | 1 / 1 / 1 |
| B22 | `Level14` | 2 | 1 / 1 / 1 |
| B23 | `Level15` | 2 | 1 / 1 / 1 |
| B24 | `Level16` | 2 | 1 / 1 / 1 |
| B25 | `Level17` | 2 | 1 / 1 / 1 |
| B26 | `Level18` | 3 | 1 / 1 / 1 |
| B27 | `Level19` | 3 | 1 / 1 / 1 |
| B28 | `Level20` | 1 | 1 / 1 / 1 |
| B29 | `Level21` | 2 | 1 / 1 / 1 |
| B30 | `Level22` | 2 | 1 / 1 / 1 |
| B31 | `Level23` | 2 | 1 / 1 / 1 |
| B32 | `Level24` | 2 | 1 / 1 / 1 |
| B33 | `Level25` | 3 | 1 / 1 / 1 |
| B34 | `MoveTuto_01` | 2 | 1 / 1 / 1 |
| B35 | `DigTuto_01` | 1 | 1 / 1 / 1 |
| B36 | `BuildTuto_01` | 1 | 1 / 1 / 1 |
| B37 | `MoveTuto_02` | 1 | 1 / 1 / 1 |
| B38 | `DigTuto_02` | 1 | 1 / 1 / 1 |
| B39 | `BuildTuto_02` | 1 | 1 / 1 / 1 |
| B40 | `DigTuto_03` | 1 | 1 / 1 / 1 |
| B41 | `DefenceTuto_01` | 1 | 1 / 1 / 1 |

Failure, Completion and CrystalFailure lines are all present and one page each in these sections; their actual display conditions, page interaction and callers must be established in ordinary runs. Include them separately for each reachable result, including crystal failure where applicable; do not assume all result types are reachable for every mission.

## Runtime slot and reward acceptance IDs
Config `Menu::Save_Game=Save_Game`, `Load_Game=Save_Game` L291–292; `Menu::SaveImage` L303–313: Path=Saves, BigWidth=80, BigHeight=60, Width=60, Height=45, Pos1–5 = 40|95, 40|155, 40|215, 40|275, 40|335. `Menu::SaveText` L316–322: Window=54|435|344|45, Load=Click_on_the_saved_game_slot_that_you_want_to_load., Save=Click_on_a_save_game_slot_to_save_your_current_game., Slot=%d%%_of_game_completed., SaveSel=Save_Game_%d_selected._Saving..., LoadSel=Saved_Game_%d_selected._Loading... Candidate frameloop.cpp:3524 supplies `Save Game N`; frontend.cpp:2195 grafts dynamic rows at x=48, y=96+60*row and uses optional `Save_Game_N` stems if present (none in cfg). `frontend.cpp:5049` renders prompt and calculated completed percentage. Test actual labels, image, progress, highlight and tooltips separately.
- L01–L05: load slots 1–5 individually, each empty and occupied; selection, Back/Escape, expected record/level, no unintended write, revisit/restart. 
- V01–V05: save slots 1–5 individually, empty/occupied, cancellation, overwrite confirmation presence and Yes/No if present, committed bytes and reload, revisit/restart. Candidate immediately emits SAVE; no authored overwrite menu found; do not infer retail behavior.
- R01: success, configured Reward::CompleteText=Mission_Completed! L244, page/result text, reveal/animation, Save enabled and advance controls, all input methods and exit; verify checkpoint and progress.
- R02: ordinary failure, Reward::FailedText=Mission_Failed L245, corresponding Failure text and button states, no spurious completion/save; return and shutdown.
- R03: crystal failure, CrystalFailure text, score, save/advance eligibility and destination, distinguish from ordinary failure.
- R04: success Save button L242, click and keyboard, save modal, cancellation/each slot, continuation and persistence; caller frameloop.cpp:3790–3822.
- R05: success Advance L243 during and after reveal; compare with failure Advance, next level or terminal result; candidate hit gate mission.cpp:1239 excludes success, despite drawn button.
- R06: failure Save L242 and Advance L243 including disabled behavior; result destination and process lifetime.
- R07: end-of-campaign advance, progress persistence and main/credits/shutdown path.

Boundary update: menu config and 41 level graph objective text sections now enumerated. This is a parameterized acceptance plan, not evidence of retail navigation, unlock rules, actual available device lists or successful reachability; auxiliary level scripts and real result triggers have not been exhaustively traced.

## Advisor message hover acceptance IDs

Authenticated [Lego.cfg](/home/johnrowl/.local/share/lrr-devbox/menu-advisor-tooltip-audit-20261104/Lego.cfg:1819) SHA256 `20cbf383bc672b0c330fe45e5fc509371de93d0b46958af82ac1b71f10fccb4f`, extracted from retained `LegoRR1.wad` SHA256 `5a967d3147533c61390120f923cea2bd275cc29bc7a8563de21c62bd9de6229d`, provides these authored strings. The [static owner trace](/home/johnrowl/.local/share/lrr-devbox/menu-advisor-tooltip-audit-20261104/tooltip-owner-trace.md) establishes the reference name-to-slot mapping and documents candidate integration gaps; it does not establish the runtime cause of a particular frame.

| ID | Authored key and value | Slot / displayed name | Acceptance status |
| --- | --- | --- | --- |
| AH01 | `ToolTip_PreviousMessage=Replay_Message`, cfg L1819 | `0x1f` / Replay Message | Earlier paired candidate 1120 run displayed Replay Message on hover; playback and broader control semantics remain unverified. |
| AH02 | `ToolTip_NextMessage=Next_Message`, cfg L1820 | `0x20` / Next Message | Candidate 1120 displays the label on hover, clears it in subsequent stationary leave captures, and displays it again on reentry. See [timed paired evidence](/home/johnrowl/.local/share/lrr-devbox/menu-tooltip-timed-leave-reference-20261104/README.md); exact timing and broader semantics remain open. |

The earlier `14d597ac2fcb19bae44006d37221a2a13daf2945b00dcdea1ce7950fc089a4cf` candidate has bounded reference-backed evidence that the basic Moving Intro NEXT ordinary click advances without pause; see the [paired runtime record](/home/johnrowl/.local/share/lrr-devbox/menu-next-timed-reference-20261104/README.md). It does not establish all advisor states, tooltip behaviors, or full menu parity.
