// Ghidra post-analysis script (Java): create function entries that
// auto-analysis missed and decompile documented targets from the reverse-
// engineering findings corpus to /tmp/engine-decompiles/engine-region.c
//
// Run: analyzeHeadless <projdir> <proj> -import LegoRR.exe \
//        -scriptPath tools/ghidra -postScript CreateEngineFunctions.java -deleteProject

import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.decompiler.DecompiledFunction;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.symbol.SourceType;

import java.io.File;
import java.io.PrintWriter;

public class CreateEngineFunctions extends GhidraScript {

    static final long[] TARGETS = {
        0x4232e0L,  // EngineTick — frame tick (DAT_00505930)
        0x424d00L,  // EngineShutdown variant A (DAT_00505934)
        0x424cf0L,  // EngineShutdown variant B
        0x4250c0L,  // MainSimStep — quit gate inside EngineTick
        0x45a730L,  // ManagerTick — indexed subsystem dispatch
        0x419b80L,  // MessageDispatch — HUD/event notification hub
        0x435f5cL,  // ViewContextTick — ticked 3x per frame
        0x4231f0L,  // EnginePreTick — per-frame housekeeping
        0x4589c0L,  // QuitPoll — quit request poll
        0x4525c0L,  // SelectedEntity — selection/global entity root
        0x425c30L,  // TransitionControl — fade/transition controller
        0x437880L,  // EntityRename
        0x48e380L,  // KeyToAscii
        0x411f00L,  // KeyContextCheck
        0x456240L,  // SystemStateFlags — gated by bit masks
        0x4199f0L,  // MessagePreCheck — message filter before queueing
        0x435940L,  // KeepRunning — "should the app continue?" probe
        0x456550L,  // SetSystemStateFlags — sole writer of DAT_004ff980
        0x4520d0L,  // SelectionRoot — called in EngineTick
        0x4521d0L,  // SelectionClusterA
        0x452200L,  // SelectionClusterB
        0x4521f2L,  // SelectionGateHelper
        0x429890L,  // LevelLoad — the big per-level config/map loader
        0x42f0c0L,  // PendingLevelName — supplies name to KeepRunning
        0x42cda0L,  // VehicleTypeInit — materializes type table from config
        0x42ed70L,  // EntityPointer — (kind, index) → entity pointer
        0x42b510L,  // MapLoadPipeline — loads every named map layer for a level
        0x44e0c0L,  // TerrainCreate — voxel grid created from surfacemap
        0x4515a0L,  // MapFileReadHeader — .map header (w,h)
        0x4515c0L,  // MapFileGetCell — .map cell fetch (x,y)
        0x42bd20L,  // LoadPredugLayer
        0x42c480L,  // LoadErodeCryoreLayers
        0x42bf40L,  // LoadEmergeLayer
        0x42c6a0L,  // LoadPathAiLayers
        0x42c760L,  // LoadFallinLayer
        0x42c9d0L,  // LoadBlockPointers
        0x42cc90L,  // LoadTerrainMap
        0x42c120L,  // LoadLayerL
        0x42c060L,  // LoadLayerM
        0x42f6f0L,  // CellInitPass — per-cell post-init sweep
        // --- savegame / checkpoint region (batch 8) ---
        0x417a90L,  // SavReadSlot
        0x417bd0L,  // SavWriteSlot
        0x417df0L,  // SavRefreshSlots
        0x417e50L,  // SavSlotCtx
        0x417e70L,  // SavBuildCtx
        0x417e90L,  // SavGetSlot
        0x417ea0L,  // SavSetSlot
        0x417eb0L,  // SavMarkRecord
        0x417f40L,  // SavWriteRecord
        0x417f90L,  // SavRecordPtr
        0x417fc0L,  // SavCommit
        0x4173e0L,  // SavTreeLoad
        0x4172f0L,  // SavTreeWalk2
        0x4171c0L,  // SavTreeWalk
        0x416e70L,  // SavClearVisited
        0x4179b0L,  // SavMenuList
        0x417090L,  // LevelNodeGet
        0x416f40L,  // ListHasItem
        0x417020L,  // ListGetNode
        0x416ed0L,  // ListAddName
        0x416fb0L,  // ListSetNode
        0x415a10L,  // SaveMenuBuild
        0x4161f0L,  // LoadMenuBuild
        0x416d60L,  // IsInGame
        0x416e00L,  // ActiveObjectList
        0x416e20L,  // ListFindName
        0x462270L,  // CheckpointStep
        0x4177e0L,  // SavObjSerialize
        0x417700L,  // SavObjCallbackB
        // --- checkpoint blob + OSF module (batch 9) ---
        0x45fda0L,  // CheckpointBlobBuild
        0x4601e0L,  // CheckpointBlobFree
        0x4601f0L,  // CheckpointBlobGet
        0x461790L,  // RewardDisplayTick
        0x461dd0L,  // RewardZoneCheck
        0x459190L,  // OsfAdd
        0x4592e0L,  // OsfSnapshot
        0x4593e0L,  // OsfWrite
        0x458be0L,  // ObjectiveTick
        // --- reward/task manager (batch 10) ---
        0x45f7f0L,  // RewardInit
        0x45f030L,  // TaskBlockInit
        0x45f750L,  // TaskBlockFinish
        0x45f1f0L,  // TaskSoundsLoad
        0x45f230L,  // TaskFontLoad
        0x45f290L,  // TaskBoxImages
        0x45f3e0L,  // TaskImages
        0x45f530L,  // TaskBoxRect
        0x45f5f0L,  // TaskFlicPath
        0x4600a0L,  // TaskImportanceRefresh
        0x460140L,  // TaskQuotaRefresh
        0x45f7b0L,  // TaskFlushImages
        0x45f7d0L,  // TaskFlushFont
        0x45c040L,  // CrystalQuotaSetup
        0x462800L,  // RewardScoreAdd
        // --- entity command subsystem (batch 11) ---
        0x401cf0L,  // CmdRegistryInit
        0x401cb0L,  // CmdKindByName
        0x406440L,  // CmdAlloc
        0x406480L,  // CmdFree
        0x4064c0L,  // CmdPoolGrow
        0x4035c0L,  // CmdCreateMove
        0x4036c0L,  // CmdCreateTarget
        0x402720L,  // CmdCreateTask
        0x403610L,  // CmdCreateGated
        0x404030L,  // CmdLinkHead
        0x404070L,  // CmdLinkTail
        0x404190L,  // CmdChainWalk
        0x407450L,  // IdleQueueRegister
        0x402220L,  // CmdTickGlobal
        0x402310L,  // CmdChainPurge
        0x402100L,  // CmdFlushAll
        0x402850L,  // MinionCompanionCmd
        0x403700L,  // CmdCancelByCoords
        0x402cd0L,  // CmdCancelRef
        0x402000L,  // CmdMarkMatching
        0x402060L,  // CmdSetKind
        // --- front-end / menus (batch 12, finding 23) ---
        0x40a0c0L,  // ScrollingTextDisplay — the main-menu credits/text scroller
        0x4103d0L,  // MainMenuTextItem — main-menu Credits item callback
        0x410d50L,  // MenuTextRecordMake — text/cycle item record
        0x410e20L,  // MenuButtonRecordMake — trigger/button item record
        0x410fb0L,  // MenuItemImageSet — MenuImage/MenuImageDark surface loading
        0x411260L,  // ListAppend — grow-and-append on the {ptr,count} lists
        0x412c00L,  // MenuTick — draw/step one open menu (item record switch)
        0x413b80L,  // MenuRunLoop — modal menu loop (blocks until close flag +0x28)
        0x413e20L,  // MenuShow — clear all item +0x28, run the modal loop
        0x413e60L,  // MenuItemType — item value type tag (cycle/trigger/textinput/slider/…)
        0x413f00L,  // StrReplace — character replacement (e.g. '_' -> ' ' for titles)
        0x413f90L,  // MenuItemRecordAlloc — allocate an Item%i record
        0x414010L,  // LoadItemStrings — load up to 8 item strings when type tag 0x10
        0x414070L,  // ListAlloc — allocate {ptr,count} list record
        0x4140c0L,  // MenuBuild — config-driven menu constructor (Menu::<name>)
        0x414c90L,  // MenuListFindName — strcmpi name search in a list
        0x414ce0L,  // AvisEnabled — DontPlayAvis gate (boot movies)
        0x414d30L,  // BrightnessSet — options brightness callback (param/10 RGB)
        0x414de0L,  // GameSpeedSelect — options discrete-select (FUN_0047b530)
        0x414df0L,  // VolumeSet — options volume callback (param/9 L/R)
        0x414e10L,  // OptionMusicDefault — default for DAT_00557038
        0x414f90L,  // ScrollToSliderIndexStore — float -> 0..5 index, stores DAT_00557030
        0x415030L,  // ScrollToSliderIndex — float -> 0..5 menu slider index
        0x415360L,  // ScrollSliderRefresh — DAT_00557030 = ScrollToSliderIndex()
        0x4150b0L,  // SaveMenuRefresh — rebuild save/load trees (SavTreeLoad)
        0x415150L,  // OptionsApplyFromSlot — apply saved volume/brightness/speed on load
        0x415220L,  // FrontEndStartLevel — menu "start level": fade + LevelLoad + ingame bit
        0x4152c0L,  // SlotSummary — "%i point" completed-game summary for a slot
        0x415370L,  // OverlayMenuGate — paused/options overlay open gate
        0x4154b0L,  // LevelChainBuild — StartLevel + NextLevel -> ordered level list
        0x415700L,  // MoviePlayWait — FLIC frame loop with quit-probe cancellation
        0x4157c0L,  // MoviePlayLoop — play movie by config key to completion
        0x415910L,  // MoviePlayNamed — play a named movie (config-key -> path)
        0x415990L,  // MoviePlayLevelVideo — play <level> Video movie (LevelLoad path)
        0x415cf0L,  // MovieShownReset — DAT_005574d0 = 0xffffffff
        0x415d00L,  // SaveMenuLayoutLoad — SaveText/LevelText layout keys + rects
        0x416150L,  // StreamPlayLevel — trigger Stream_LevelName_Level_%d
        0x4161a0L,  // StreamPlayTutorial — trigger Stream_LevelName_TLevel_%d
        0x416910L,  // OptionsWriteToSlot — stash DAT_00557030/34/3c into save context
        0x416940L,  // OptionsLoadOrDefault — front-end options defaults {1,8,5}
        0x4169c0L,  // FrontEndStateChange — front-end flow dispatcher (0..4)
        0x416c80L,  // MenuEnter — reset state + select item + MenuShow
        0x416d00L,  // FrontEndActive — the front-end gate (Lego.cfg "FrontEnd" && mode<3)
        0x416d50L,  // SaveSlotOpen — DAT_00557020 != -1
        0x416d70L,  // FrontEndSelectedAction — selected level name for LevelLoad
        0x416dd0L,  // QuitMenuProbe — DAT_0055701c != 0 => quit now
        0x416de0L,  // LevelStartFlag — DAT_00557048 != 0 (advance/start level)
        0x416df0L,  // MenuOptionsFlag — DAT_0055704c != 0 (options overlay)
        0x41fa80L,  // StartLevelPilots — spawn Pilot + set in-game bits from menu start
        0x460220L,  // StartLevelSelected — commit pending level + re-create Pilot
        // --- documented function starts from newly added findings ---
        0x401000L,  // FUN_00401000
        0x4018c0L,  // FUN_004018c0
        0x401930L,  // FUN_00401930
        0x401990L,  // FUN_00401990
        0x401a70L,  // FUN_00401a70
        0x401b20L,  // FUN_00401b20
        0x401b30L,  // FUN_00401b30
        0x402f30L,  // FUN_00402f30
        0x402f70L,  // FUN_00402f70
        0x403a50L,  // FUN_00403a50
        0x403d10L,  // FUN_00403d10
        0x403f30L,  // FUN_00403f30
        0x404fc0L,  // FUN_00404fc0
        0x406540L,  // FUN_00406540
        0x4065e0L,  // FUN_004065e0
        0x406e40L,  // FUN_00406e40
        0x406ec0L,  // FUN_00406ec0
        0x406f80L,  // FUN_00406f80
        0x408380L,  // FUN_00408380
        0x408570L,  // FUN_00408570
        0x4085f0L,  // FUN_004085f0
        0x408940L,  // FUN_00408940
        0x4089a0L,  // FUN_004089a0
        0x4090c0L,  // FUN_004090c0
        0x409300L,  // FUN_00409300
        0x40aa10L,  // FUN_0040aa10
        0x40aa40L,  // FUN_0040aa40
        0x40cce0L,  // FUN_0040cce0
        0x40e9c0L,  // FUN_0040e9c0
        0x40ea10L,  // FUN_0040ea10
        0x40eab0L,  // FUN_0040eab0
        0x40ee50L,  // FUN_0040ee50
        0x40f000L,  // FUN_0040f000
        0x40f2b0L,  // FUN_0040f2b0
        0x40f330L,  // FUN_0040f330
        0x4103f0L,  // FUN_004103f0
        0x410440L,  // FUN_00410440
        0x4105a0L,  // FUN_004105a0
        0x411a00L,  // FUN_00411a00
        0x4121b0L,  // FUN_004121b0
        0x418040L,  // FUN_00418040
        0x418830L,  // FUN_00418830
        0x419230L,  // FUN_00419230
        0x4193e0L,  // FUN_004193e0
        0x419420L,  // FUN_00419420
        0x4194b0L,  // FUN_004194b0
        0x419530L,  // FUN_00419530
        0x41a080L,  // FUN_0041a080
        0x41a300L,  // FUN_0041a300
        0x41b2d0L,  // FUN_0041b2d0
        0x41d040L,  // FUN_0041d040
        0x41dca0L,  // FUN_0041dca0
        0x41e750L,  // FUN_0041e750
        0x41ea50L,  // FUN_0041ea50
        0x41f900L,  // FUN_0041f900
        0x41f9e0L,  // FUN_0041f9e0
        0x41fb50L,  // FUN_0041fb50
        0x4261c0L,  // FUN_004261c0
        0x426420L,  // FUN_00426420
        0x426520L,  // MainSimInteractionAndCellUpdate
        0x4288e0L,  // FUN_004288e0
        0x429470L,  // FUN_00429470
        0x4295f0L,  // FUN_004295f0
        0x429810L,  // FUN_00429810
        0x42bb60L,  // FUN_0042bb60
        0x42c330L,  // FUN_0042c330
        0x42e020L,  // OListEntityRecordParse
        0x42e8b0L,  // ObjectListEntityTypeResolve
        0x42efc0L,  // FUN_0042efc0
        0x42f350L,  // FUN_0042f350
        0x430320L,  // FUN_00430320
        0x430470L,  // FUN_00430470
        0x430530L,  // FUN_00430530
        0x430df0L,  // FUN_00430df0
        0x430ee0L,  // FUN_00430ee0
        0x431450L,  // FUN_00431450
        0x431560L,  // CurrentLevelGet
        0x431580L,  // FUN_00431580
        0x431780L,  // FUN_00431780
        0x431da0L,  // FUN_00431da0
        0x432550L,  // FUN_00432550
        0x432600L,  // FUN_00432600
        0x432770L,  // FUN_00432770
        0x432b50L,  // FUN_00432b50
        0x432bd0L,  // FUN_00432bd0
        0x432c90L,  // FUN_00432c90
        0x433090L,  // FUN_00433090
        0x433380L,  // FUN_00433380
        0x434570L,  // FUN_00434570
        0x434a50L,  // FUN_00434a50
        0x434c10L,  // FUN_00434c10
        0x434e80L,  // FUN_00434e80
        0x435e0eL,  // FUN_00435e0e
        0x435f16L,  // FUN_00435f16
        0x436900L,  // FUN_00436900
        0x4369f4L,  // FUN_004369f4
        0x436aa5L,  // FUN_00436aa5
        0x436b87L,  // FUN_00436b87
        0x436c20L,  // FUN_00436c20
        0x4372a0L,  // FUN_004372a0
        0x4373d0L,  // FUN_004373d0
        0x437540L,  // FUN_00437540
        0x437940L,  // FUN_00437940
        0x437c20L,  // FUN_00437c20
        0x437d00L,  // FUN_00437d00
        0x4385b0L,  // FUN_004385b0
        0x438610L,  // FUN_00438610
        0x438770L,  // FUN_00438770
        0x4387f0L,  // FUN_004387f0
        0x4389e0L,  // FUN_004389e0
        0x438c60L,  // FUN_00438c60
        0x439d50L,  // FUN_00439d50
        0x439e10L,  // FUN_00439e10
        0x43a300L,  // FUN_0043a300
        0x43a5f0L,  // FUN_0043a5f0
        0x43abf0L,  // FUN_0043abf0
        0x43ad50L,  // FUN_0043ad50
        0x43af30L,  // FUN_0043af30
        0x43bb20L,  // FUN_0043bb20
        0x43bc40L,  // FUN_0043bc40
        0x43f100L,  // FUN_0043f100
        0x43f150L,  // FUN_0043f150
        0x43f190L,  // FUN_0043f190
        0x43f5b0L,  // FUN_0043f5b0
        0x43f6a0L,  // FUN_0043f6a0
        0x43f7d0L,  // FUN_0043f7d0
        0x43fb40L,  // FUN_0043fb40
        0x43fe70L,  // FUN_0043fe70
        0x43fff0L,  // FUN_0043fff0
        0x4401b0L,  // FUN_004401b0
        0x4409e0L,  // FUN_004409e0
        0x4410f0L,  // FUN_004410f0
        0x441700L,  // FUN_00441700
        0x4424f0L,  // FUN_004424f0
        0x4428a0L,  // FUN_004428a0
        0x4475c0L,  // FUN_004475c0
        0x447b30L,  // FUN_00447b30
        0x447c40L,  // FUN_00447c40
        0x447ea0L,  // FUN_00447ea0
        0x448c50L,  // FUN_00448c50
        0x44a200L,  // FUN_0044a200
        0x44a4b0L,  // FUN_0044a4b0
        0x44b350L,  // FUN_0044b350
        0x44b370L,  // FUN_0044b370
        0x44be30L,  // FUN_0044be30
        0x44c4a0L,  // FUN_0044c4a0
        0x44dec0L,  // FUN_0044dec0
        0x44e4d0L,  // FUN_0044e4d0
        0x44e6d0L,  // FUN_0044e6d0
        0x44f230L,  // FUN_0044f230
        0x44f380L,  // FUN_0044f380
        0x44f6d0L,  // FUN_0044f6d0
        0x44f700L,  // FUN_0044f700
        0x4502e0L,  // FUN_004502e0
        0x452010L,  // FUN_00452010
        0x452060L,  // FUN_00452060
        0x452690L,  // FUN_00452690
        0x4526c0L,  // FUN_004526c0
        0x4526e0L,  // FUN_004526e0
        0x452870L,  // FUN_00452870
        0x452be0L,  // FUN_00452be0
        0x452df0L,  // FUN_00452df0
        0x452e70L,  // FUN_00452e70
        0x453200L,  // FUN_00453200
        0x453320L,  // FUN_00453320
        0x456810L,  // FUN_00456810
        0x456830L,  // FUN_00456830
        0x456b80L,  // FUN_00456b80
        0x456cb0L,  // FUN_00456cb0
        0x457420L,  // SetTutorialBlockIsPath wrapper (code start missing from Ghidra)
        0x458670L,  // FUN_00458670
        0x4588e0L,  // FUN_004588e0
        0x459050L,  // FUN_00459050
        0x4592a0L,  // FUN_004592a0
        0x459470L,  // FUN_00459470
        0x459560L,  // FUN_00459560
        0x45a2e0L,  // FUN_0045a2e0
        0x45a590L,  // FUN_0045a590
        0x45c830L,  // FUN_0045c830
        0x45ca70L,  // FUN_0045ca70
        0x45cbd0L,  // FUN_0045cbd0
        0x45cc20L,  // FUN_0045cc20
        0x45cc40L,  // FUN_0045cc40
        0x45cc70L,  // FUN_0045cc70
        0x45cd90L,  // FUN_0045cd90
        0x461410L,  // FUN_00461410
        0x462030L,  // FUN_00462030
        0x462390L,  // FUN_00462390
        0x462460L,  // FUN_00462460
        0x4624a0L,  // FUN_004624a0
        0x462700L,  // FUN_00462700
        0x462780L,  // FUN_00462780
        0x462e60L,  // FUN_00462e60
        0x4641c0L,  // building-placement footprint overlay and validity check
        0x464740L,  // FUN_00464740
        0x464c70L,  // FUN_00464c70
        0x464d00L,  // FUN_00464d00
        0x464e20L,  // FUN_00464e20
        0x464ef0L,  // FUN_00464ef0
        0x464f60L,  // FUN_00464f60
        0x464fa0L,  // FUN_00464fa0
        0x465050L,  // FUN_00465050
        0x465090L,  // FUN_00465090
        0x465190L,  // FUN_00465190
        0x4651a0L,  // FUN_004651a0
        0x465260L,  // FUN_00465260
        0x4652a0L,  // FUN_004652a0
        0x4652b0L,  // FUN_004652b0
        0x4652c0L,  // FUN_004652c0
        0x4652d0L,  // FUN_004652d0
        0x465370L,  // FUN_00465370
        0x465430L,  // FUN_00465430
        0x4659b0L,  // FUN_004659b0
        0x465b00L,  // FUN_00465b00
        0x4667e0L,  // FUN_004667e0
        0x469bb0L,  // FUN_00469bb0
        0x469c10L,  // FUN_00469c10
        0x469de0L,  // FUN_00469de0
        0x469e60L,  // FUN_00469e60
        0x469e80L,  // FUN_00469e80
        0x469ea0L,  // FUN_00469ea0
        0x469f20L,  // FUN_00469f20
        0x469f40L,  // FUN_00469f40
        0x46a060L,  // FUN_0046a060
        0x46ac20L,  // FUN_0046ac20
        0x46ac60L,  // FUN_0046ac60
        0x46ad00L,  // FUN_0046ad00
        0x46c3f0L,  // FUN_0046c3f0
        0x46ce10L,  // FUN_0046ce10
        0x46d140L,  // FUN_0046d140
        0x46da50L,  // FUN_0046da50
        0x46ffb0L,  // FUN_0046ffb0
        0x4721e0L,  // FUN_004721e0
        0x4722b0L,  // FUN_004722b0
        0x472390L,  // FUN_00472390
        0x472430L,  // FUN_00472430
        0x4724a0L,  // FUN_004724a0
        0x472530L,  // FUN_00472530
        0x472560L,  // FUN_00472560
        0x4726c0L,  // FUN_004726c0
        0x4726d0L,  // FUN_004726d0
        0x4726f0L,  // FUN_004726f0
        0x472710L,  // FUN_00472710
        0x472800L,  // FUN_00472800
        0x472900L,  // FUN_00472900
        0x472920L,  // FUN_00472920
        0x472950L,  // FUN_00472950
        0x472a40L,  // FUN_00472a40
        0x472a50L,  // FUN_00472a50
        0x472cd0L,  // FUN_00472cd0
        0x473340L,  // FUN_00473340
        0x473370L,  // FUN_00473370
        0x4734c0L,  // FUN_004734c0
        0x473690L,  // FUN_00473690
        0x473b40L,  // FUN_00473b40
        0x473da0L,  // FUN_00473da0
        0x473db0L,  // FUN_00473db0
        0x4744f0L,  // FUN_004744f0
        0x474710L,  // FUN_00474710
        0x474a20L,  // FUN_00474a20
        0x474c00L,  // FUN_00474c00
        0x474d30L,  // FUN_00474d30
        0x474da0L,  // FUN_00474da0
        0x475120L,  // FUN_00475120
        0x475140L,  // FUN_00475140
        0x475390L,  // FUN_00475390
        0x4753f0L,  // FUN_004753f0
        0x475430L,  // FUN_00475430
        0x4754c0L,  // FUN_004754c0
        0x475710L,  // FUN_00475710
        0x4757a0L,  // FUN_004757a0
        0x475930L,  // FUN_00475930
        0x4759f0L,  // FUN_004759f0
        0x475c00L,  // FUN_00475c00
        0x475e40L,  // FUN_00475e40
        0x475f70L,  // FUN_00475f70
        0x476160L,  // FUN_00476160
        0x476180L,  // FUN_00476180
        0x4761a0L,  // FUN_004761a0
        0x4761e0L,  // FUN_004761e0
        0x476200L,  // FUN_00476200
        0x4765c0L,  // FUN_004765c0
        0x476770L,  // FUN_00476770
        0x4767e0L,  // FUN_004767e0
        0x476850L,  // FUN_00476850
        0x477010L,  // FUN_00477010
        0x477150L,  // FUN_00477150
        0x477240L,  // FUN_00477240
        0x477290L,  // FUN_00477290
        0x477440L,  // FUN_00477440
        0x477550L,  // FUN_00477550
        0x477670L,  // FUN_00477670
        0x4777a0L,  // FUN_004777a0
        0x477bf0L,  // FUN_00477bf0
        0x477f30L,  // FUN_00477f30
        0x478010L,  // FUN_00478010
        0x478040L,  // FUN_00478040
        0x4781d0L,  // FUN_004781d0
        0x4784c0L,  // FUN_004784c0
        0x4786c0L,  // FUN_004786c0
        0x478940L,  // FUN_00478940
        0x478980L,  // FUN_00478980
        0x4789a0L,  // FUN_004789a0
        0x4789c0L,  // FUN_004789c0
        0x478df0L,  // FUN_00478df0
        0x478e20L,  // FUN_00478e20
        0x478e60L,  // FUN_00478e60
        0x478f50L,  // FUN_00478f50
        0x479050L,  // FUN_00479050
        0x479270L,  // FUN_00479270
        0x479490L,  // FUN_00479490
        0x4798e0L,  // FUN_004798e0
        0x47a640L,  // FUN_0047a640
        0x47a850L,  // FUN_0047a850
        0x47a870L,  // FUN_0047a870
        0x47ab80L,  // FUN_0047ab80
        0x47aba0L,  // FUN_0047aba0
        0x47abc0L,  // FUN_0047abc0
        0x47ad10L,  // FUN_0047ad10
        0x47ad30L,  // FUN_0047ad30
        0x47ad70L,  // FUN_0047ad70
        0x47b020L,  // FUN_0047b020
        0x47b0f0L,  // FUN_0047b0f0
        0x47b130L,  // FUN_0047b130
        0x47b160L,  // FUN_0047b160
        0x47b1a0L,  // FUN_0047b1a0
        0x47b220L,  // FUN_0047b220
        0x47b390L,  // FUN_0047b390
        0x47b410L,  // FUN_0047b410
        0x47b4a0L,  // FUN_0047b4a0
        0x47b4d0L,  // FUN_0047b4d0
        0x47b500L,  // FUN_0047b500
        0x47b530L,  // FUN_0047b530
        0x47b580L,  // FUN_0047b580
        0x47b6c0L,  // FUN_0047b6c0
        0x47b790L,  // FUN_0047b790
        0x47b8e0L,  // FUN_0047b8e0
        0x47b970L,  // FUN_0047b970
        0x47ba20L,  // FUN_0047ba20
        0x47baa0L,  // FUN_0047baa0
        0x47bc30L,  // FUN_0047bc30
        0x47bdb0L,  // FUN_0047bdb0
        0x47c0c0L,  // FUN_0047c0c0
        0x47c100L,  // FUN_0047c100
        0x47c610L,  // FUN_0047c610
        0x47c8d0L,  // FUN_0047c8d0
        0x47cc20L,  // FUN_0047cc20
        0x47cc50L,  // FUN_0047cc50
        0x47cee0L,  // FUN_0047cee0
        0x47d9d0L,  // FUN_0047d9d0
        0x47dbf0L,  // FUN_0047dbf0
        0x47dcb0L,  // FUN_0047dcb0
        0x47e760L,  // FUN_0047e760
        0x47e7b0L,  // FUN_0047e7b0
        0x47e800L,  // FUN_0047e800
        0x47e980L,  // FUN_0047e980
        0x47e9a0L,  // FUN_0047e9a0
        0x47e9e0L,  // FUN_0047e9e0
        0x47eb20L,  // FUN_0047eb20
        0x47ebf0L,  // FUN_0047ebf0
        0x47ec90L,  // FUN_0047ec90
        0x47ecf0L,  // FUN_0047ecf0
        0x47ed00L,  // FUN_0047ed00
        0x47ed40L,  // FUN_0047ed40
        0x47ed50L,  // FUN_0047ed50
        0x47ed60L,  // FUN_0047ed60
        0x47ed80L,  // FUN_0047ed80
        0x47ed90L,  // FUN_0047ed90
        0x47eef0L,  // FUN_0047eef0
        0x47efb0L,  // FUN_0047efb0
        0x47f010L,  // FUN_0047f010
        0x47f6e0L,  // FUN_0047f6e0
        0x47f850L,  // FUN_0047f850
        0x47f980L,  // FUN_0047f980
        0x47fa50L,  // FUN_0047fa50
        0x47fac0L,  // FUN_0047fac0
        0x47fb10L,  // FUN_0047fb10
        0x47fea0L,  // FUN_0047fea0
        0x47fed0L,  // FUN_0047fed0
        0x4800a0L,  // FUN_004800a0
        0x4800c0L,  // FUN_004800c0
        0x4809e0L,  // FUN_004809e0
        0x4823e0L,  // FUN_004823e0
        0x482ac0L,  // FUN_00482ac0
        0x482d30L,  // FUN_00482d30
        0x482d60L,  // FUN_00482d60
        0x4830c0L,  // FUN_004830c0
        0x483420L,  // FUN_00483420
        0x483600L,  // FUN_00483600
        0x4838b0L,  // FUN_004838b0
        0x483940L,  // FUN_00483940
        0x4839c0L,  // FUN_004839c0
        0x483a70L,  // FUN_00483a70
        0x483a90L,  // FUN_00483a90
        0x483f00L,  // FUN_00483f00
        0x484070L,  // FUN_00484070
        0x485300L,  // FUN_00485300
        0x4876c0L,  // FUN_004876c0
        0x487ba0L,  // shared-object track updates and timed-event dispatch
        0x487de0L,  // FUN_00487de0
        0x487ed0L,  // FUN_00487ed0
        0x488c00L,  // FUN_00488c00
        0x488c70L,  // FUN_00488c70
        0x488e70L,  // FUN_00488e70
        0x488f10L,  // FUN_00488f10
        0x4890c0L,  // FUN_004890c0
        0x489100L,  // FUN_00489100
        0x489210L,  // FUN_00489210
        0x489280L,  // FUN_00489280
        0x489330L,  // FUN_00489330
        0x4897e0L,  // FUN_004897e0
        0x48b330L,  // FUN_0048b330
        0x48b360L,  // FUN_0048b360
        0x48b390L,  // FUN_0048b390
        0x48be40L,  // FUN_0048be40
        0x48d980L,  // FUN_0048d980
        0x48dad0L,  // FUN_0048dad0
        0x48db70L,  // FUN_0048db70
        0x48db80L,  // FUN_0048db80
        0x48de20L,  // FUN_0048de20 — generic array sort with comparator
        0x48e160L,  // FUN_0048e160
        0x48e170L,  // FUN_0048e170
        0x48e1e0L,  // FUN_0048e1e0
        0x48ead0L,  // FUN_0048ead0
        0x48eb90L,  // FUN_0048eb90
        0x48f1d0L,  // FUN_0048f1d0
        0x48f500L,  // FUN_0048f500
        0x4907f0L,  // FUN_004907f0
        0x493ca0L,  // FUN_00493ca0
        // --- additional procedure starts referenced by the changed findings ---
        0x439280L,  // EntityBuildPlacementCommandBatchExpand
        0x439340L,  // EntityBuildPlacementCommandPairCreate
        0x4394b0L,  // EntityBuildPlacementAlternateCommandSetup
        0x447210L,  // EntityAdjacentPlacementCellSearch
        0x452430L,  // EntityRandomValidCellCommandCreate
        0x475470L,  // DDiSceneObjectPoseMethodDispatch
        0x4790d0L,  // ConfigBooleanGetDefault2
        0x483f60L,  // FlicFileHeaderValidate
        0x4841d0L,  // FlicRamFrameAdvance
        0x484260L,  // FlicStreamFrameAdvance
        0x484320L,  // FlicChunkWalk
        0x4844b0L,  // FlicFrameRecordWalk
        0x484610L,  // FlicSubrecordOpcodeDispatch
        0x484720L,  // FlicChunkBufferAndContinueLK
        0x4847d0L,  // FlicChunkBufferAndContinueType0B
        0x484880L,  // FlicOpcode16ModeDispatch
        0x4848d0L,  // FlicDecode8BitOpcode25Mode4
        0x4849d0L,  // FlicDecode16BitKeyframe
        0x484b20L,  // FlicOpcode25DecodeDispatch
        0x484b90L,  // FlicOpcode25SuccessStub
        0x484c00L,  // FlicDecode16BitRowRecords
        0x484c90L,  // FlicOpcode27FallbackNoop
        0x484ca0L,  // FlicDecode8BitAlternateDelta
        0x484e50L,  // FlicDecode16BitDelta
        0x485030L,  // FlicOpcode27DecodeDispatch
        0x489460L,  // MapFileResourceSlotRelease
        0x47f130L,  // WadMountStartup
        0x47fe30L,  // FileOpenRouteSelect
        0x48bd30L,  // WadEntrySlotAllocate
        0x48bda0L,  // WadEntryNameLookup
        0x48bf70L,  // WadPathOpen
        0x4783f0L,  // WindowCreate
        0x4250a0L,  // ResetLevelAndExit
        0x41fb40L,  // TimeScaleGet
        0x419b20L,  // InfoMessageQueueFindId
        0x419f30L,  // InfoMessageQueueTick
        0x41a2c0L,  // InfoMessageTimerDecay
        0x451f60L,  // CommandQueueCancelOpcode
        0x452520L,  // SelectionSnapshotCreate
        0x452610L,  // SelectionContainsEntity
        0x45d870L,  // CommandEntityReferenceRemap
        0x40a400L,  // PreloadNumberedBitmaps
        0x40b310L,  // LoadTileVariants
        0x42b330L,  // LoadTileMeshes
        0x4653a0L,  // PreloadSharedIndexed
        0x474050L,  // CreateBmpSurface
        0x474410L,  // ParseIndexedSurfaceName
        0x477bd0L,  // SetLowTextureFlag
        0x47cd50L,  // QueryTextureMem
        0x480ac0L,  // MaterialPass
        0x481820L,  // BindModelTextures
        0x481c50L,  // ParseUV
        0x482e70L,  // TextureGetOrCreate
        0x483030L,  // TextureCacheLookup
        0x483080L,  // TextureCacheInsert
        0x4869f0L,  // LoadLWSCScene
        0x4885c0L,  // ModelResourceResolveOrLoad
        0x48c0c0L,  // LwoStringDuplicate
        0x48c1d0L,  // LwoReadBigEndianFloat
        0x48c210L,  // LwoPoints
        0x48c360L,  // LwoCurves
        0x48c3e0L,  // LwoPolygons
        0x48c690L,  // LwoSurfaceNames
        0x48c820L,  // LwoSurfaceSettings
        0x48d2c0L,  // LoadLWO
        0x490090L,  // MaterialResolve
        0x419470L,  // InfoMessageAlwaysPlayFlagSet
        0x419650L,  // InfoMessageSoundLoad
        0x419680L,  // InfoMessageSecondarySoundSet
        0x457d40L,  // ObjectiveConfigLoad
        0x458580L,  // ObjectiveCrystalOrOreArm
        0x4585c0L,  // ObjectiveBlockArm
        0x4585f0L,  // ObjectiveTimerArm
        0x458620L,  // ObjectiveConstructionArm
        0x479500L,  // D3DRMVectorRandom
        0x47dbc0L,  // ResourceRecordAddRef
        0x475900L,  // DDiSceneObjectArgumentsNormalize
        0x40b5f0L,  // TileModelEntityRegister
        0x4728c0L,  // SharedTextureDirectorySet
        0x4774b0L,  // WhitespaceTokenize
        0x480050L,  // ReadCrlfLine
        0x4805b0L,  // SharedTextureRootRegister
        0x480650L,  // SharedTextureRootInitialize
        0x48d7c0L,  // ModelObjectCreate
        0x48d870L,  // ModelObjectFree
        0x48dc80L,  // GrowHeapArray
        0x48eae0L,  // FileResourceHandleClose
        0x48f460L,  // FileResourceHandleSeek
        0x49d030L,  // DirectDrawDeviceInitialize
        0x47ec80L,  // AviFileInitialize
        0x47efd0L,  // AviFileShutdown
        0x47ccf0L,  // DirectDrawObjectsRelease
        0x485e80L,  // RenderSetup
        0x4893f0L,  // ResourceStateTableReset
        0x48b260L,  // StartupStreamStateReset
        0x48b290L,  // StartupStreamsClose
        0x48b2f0L,  // StartupConfigStreamLoad
        0x48da50L,  // StringFindSubstring
        0x477fa0L,  // RendererBeginFrameOnce
        0x4783d0L,  // WindowTitleSet
        // --- loader, startup, message, and simulation helpers (batch 16) ---
        0x49c7c0L,  // ExpandWadPayload
        0x49c8e0L,  // DecodeWadPayloadType1
        0x49ca60L,  // DecodeWadPayloadType2
        0x48bd50L,  // WadEntryFindAcrossMountedArchives
        0x494e30L,  // StdFileOpen
        0x495000L,  // StdFileHandleAcquire
        0x48eb60L,  // StdFileOpenIntoHandle
        0x47f4f0L,  // FindCdKeyDrive
        0x47f640L,  // MountWadFromPath
        0x49dcd0L,  // StringToUpper
        0x45d6d0L,  // PositionTrackerUpdateAll
        0x41b930L,  // TimedStatusAndEventTick
        0x424600L,  // TerrainCellFlagCleanup
        0x44eff0L,  // TerrainDirtyCellUpdate
        0x4196a0L,  // MessageSubscriberDispatch
        0x456210L,  // FlhMagicUuNoop
        0x459100L,  // ObjectiveEntityPairedFieldPredicate — callback passed by objective evaluator
        // --- WAD compression bitstream helpers ---
        0x49c840L,  // WadPayloadDecodeDispatch
        0x49cc70L,  // WadBitstreamOutputInitialize
        0x49ccc0L,  // WadReadBitsLsbFirst
        0x49cd30L,  // WadReadBitsMsbFirst
        0x49cd90L,  // WadHuffmanTableRead
        0x49ce00L,  // WadHuffmanSymbolDecode
        0x49ce70L,  // WadRunLengthDecode
        0x49ceb0L,  // WadBackreferenceOffsetDecode
        0x49cf50L,  // WadHuffmanTableClear
        0x49cf90L,  // WadHuffmanCanonicalCodesBuild
        0x49d000L,  // WadReverseLowBits
        0x403270L,  // selection-drag area command
        0x4112e0L,  // menu rectangle hit test
        0x411360L,  // menu mouse-input helper
        0x411870L,  // slider mouse handler
        0x412170L,  // overlay-list count
        0x413970L,  // menu scroll update
        0x413a60L,  // save-menu thumbnail config load
        0x41fa20L,  // game-config callback registry initialization
        0x425b40L,  // periodic callback dispatch
        0x4297b0L,  // CD-audio watchdog mode update
        0x4319b0L,  // terrain cell type read
        0x4319e0L,  // map-file cell read
        0x4321a0L,  // pending terrain-cell update flush
        0x432200L,  // terrain-cell previous-state tracking
        0x432e00L,  // predug edge-wall flag setter
        0x4334c0L,  // map-file dimensions read
        0x4377d0L,  // EntityPoolFindByCallback
        0x4383b0L,  // rover work-queue update
        0x439370L,  // build/placement command setup
        0x439990L,  // entity activity scene-state update
        0x43a960L,  // entity activity list registration
        0x43aea0L,  // random emergence spawn attempt
        0x43b270L,  // entity frame-update dispatch
        0x44a2a0L,  // entity work-object position lookup
        0x44a910L,  // building placement preview update
        0x44b480L,  // permitted-activity membership check
        0x44c530L,  // entity can-do/riding capability check
        0x44efb0L,  // terrain-cell change flag setter
        0x44bf00L,  // global timed-callback dispatch
        0x4528c0L,  // selection state tick
        0x452c50L,  // selection entity removal
        0x452cc0L,  // selected-entity state monitor
        0x452d60L,  // selection command handler
        0x459360L,  // OSF snapshot table build
        0x4593d0L,  // OSF read dirty-flag getter
        0x460200L,  // selected-level start object update
        0x469cc0L,  // terrain-type movement speed getter
        0x475d10L,  // DDiScene ActFrame record parser
        0x477710L,  // config boolean parser
        0x477fd0L,  // engine callback triplet setter
        0x478ff0L,  // config nested-path child lookup
        0x4790b0L,  // config path value getter
        0x4792e0L,  // config block path lookup
        0x479ee0L,  // font resource load
        0x47a180L,  // reward text drawing
        0x47aad0L,  // free sound-bank slot search
        0x47a550L,  // font record and surface release
        0x47c8f0L,  // bitmap file writer
        0x47d470L,  // image surface release
        0x47db90L,  // RGB channel pack
        0x47e050L,  // font record constructor
        0x47e190L,  // font device-color conversion
        0x47e2d0L,  // RGB channel unpack
        0x47e440L,  // bitmap surface save wrapper
        0x47f660L,  // sound path fallback formatter
        0x47fb60L,  // sound sample file open by configured name
        0x47fca0L,  // game-file length query
        0x483c80L,  // FLIC file open and state creation
        0x488b50L,  // CD-audio device state update
        0x489260L,  // CD-audio error text retrieval
        0x489410L,  // map-file resource slot allocation
        0x4894a0L,  // map-file resource slot lookup
        0x489c30L,  // indexed bitmap palette/data preparation
        0x48d9e0L,  // bounded formatted-string output
        0x418110L,  // save-state event context setter
        0x43c440L,  // permitted activity classifier
        0x4470d0L,  // conditional type-5 entity spawn
        0x44c110L,  // attached-entity wake helper
        0x4755e0L,  // camera teleport/offset vtable call
        0x48dbd0L,  // engine heap allocation wrapper
        0x48f2b0L,  // streamed audio file length query
        0x41c4f0L,  // advisor animation position and request update
        0x45c670L,  // advisor animation scene lookup and request
        0x460360L,  // checkpoint-blob task progress refresh and clamp
        0x43c810L,  // per-entity flag-word behaviour evaluator
        0x41b490L,  // advisor overlay update and draw dispatch
        0x45c6b0L,  // advisor animation slot-id resolver
        0x480170L,  // MapOpen2 container-backed map-image loader
        0x488070L,  // interpolate shared-object float track and apply it
        0x401c00L,  // optional command-line StartLevel override pointer
        0x475300L,  // gated DDiScene ActFrame timing updater
        0x461c90L,  // conditional Score task text draw on reward screen
        0x462600L,  // task text pass and score-display gate
        0x48e060L,  // Visual Studio x87 float-to-longlong conversion helper
        0x440c70L,  // TerrainGridPathSearch
        0x44f640L,  // MapCellToWorldCenter
        0x44f940L,  // TerrainSurfaceHeightInterpolate
        0x4574e0L,  // ObjectiveTextOverlayLoad
        0x46c6f0L,  // VehicleTypeDefinitionParse
        0x46eb80L,  // WeaponTypeConfigLoad
        0x46b7c0L,  // TimedEffectAndCueUpdate
        0x489dd0L,  // DirectXResultDiagnosticDispatch
        0x402170L,  // LevelNoGatherObjectSetup
        0x40bf70L,  // RockFallStyleNameLookup
        0x40e930L,  // ErosionTimingStateInit
        0x40ce80L,  // MapCellStateBufferRebuild
        0x4191a0L,  // MainSimStateAdmissionPredicate
        0x41c440L,  // AdvisorCallbackScheduleGate
        0x425ce0L,  // ScrollScaleSetAndClamp
        0x4262e0L,  // ObjectiveScoreTextOriginSet
        0x426320L,  // ObjectiveTaskTextOriginSet
        0x428800L,  // ActionFeedbackSoundDispatch
        0x422850L,  // MiscObjectResourcesLoad
        0x42d100L,  // RockMonsterTypesLoad
        0x42d2b0L,  // BuildingTypesLoad
        0x42d460L,  // UpgradeTypesLoad
        0x42d600L,  // ObjectNamesLoad
        0x42da20L,  // ObjectTheNamesLoad
        0x431570L,  // CurrentLevelHeightGet
        0x44f4e0L,  // TerrainCellSurfaceTypeSet
        0x44f540L,  // TerrainCellSurfaceTypeGet
        0x44f570L,  // TerrainCellSurfaceTypeReset
        0x44f5c0L,  // TerrainCellSurfaceAppearanceApply
        0x44f870L,  // TerrainWorldPointToCell
        0x44fb90L,  // TerrainSurfacePointProjectOffset
        0x4500d0L,  // TerrainCellSurfaceVerticesGet
        0x480220L,  // ResourcePathResolveAgainstBase
        0x4850f0L,  // TextBufferCreate
        0x485190L,  // TextBufferDestroy
        0x485390L,  // TextBufferDrawWrapped
        0x479170L,  // ConfigRgbVectorRead
        0x483270L,  // RenderObjectColorVectorSet
        0x477610L,  // CaseInsensitiveSubstringFind
        0x46a730L,  // CameraMoveCommandRoute
        0x435a50L,  // MainSimFeatureFlagLatch
        0x42c440L,  // LevelRecordTimerCountdown
        0x42cb70L,  // LavaErosionCellTimersAdvance
        0x42cf50L,  // MiniFigureTypeTableLoad
        0x42de40L,  // ViewportTargetPositionClamp
        0x42c5b0L,  // TerrainCellByteStateClassify
        0x428020L,  // EntityCommandStatusFeedbackDispatch
        0x428330L,  // TerrainCommandFeedbackDispatch
        0x44e880L,  // TerrainGridCellsInitialize
        0x44edf0L,  // TerrainCellOrientationToggleAndRebuild
        0x4514f0L,  // TerrainCellSurfaceVectorsRebuild
        0x46a4c0L,  // FlagMaskCallbacksAllPass
        0x431a30L,  // SelectedEntitiesAnyPredicateMatch
        0x48f650L,  // GameFileDelete
        0x48f6d0L,  // GameDirectoryCreate
        0x48f700L,  // GameFileMetadataFind
        0x48c120L,  // LwoTextureProjectionModeParse
        0x489d90L,  // DirectXErrorMessageFormat
        0x48dbf0L,  // EngineHeapAllocateWithRetry
        0x48dc40L,  // EngineHeapAllocateAligned
        0x491900L,  // EnginePoolAllocate
        0x4909d0L,  // EngineFormatStringToStream
        0x4377b0L,  // EntityPoolFindByCallbackFiltered
        0x442260L,  // EntityTransformPositionGet
        0x4291a0L,  // SceneMarkerUpdateAndDraw
        0x425d90L,  // TerrainPointerCommandUpdate
        0x4230c0L,  // DebugPanelTextDrawUpdate
        0x42b850L,  // SurfaceTextCharacterMapInitialize
        0x42f2e0L,  // TerrainCellEdgeFlagSetAndUpdate
        0x46b700L,  // TimedEffectCueRequestSet
        0x46b730L,  // TimedEffectPersistenceSet
        0x46b770L,  // TimedEffectUpdateRequestSet
        0x46b7a0L,  // TimedEffectTimestampReset
        0x46b8b0L,  // RectangularGridOverlayGeometryDraw
        0x449f50L,  // EntityTypeOperationDispatch
        0x449ff0L,  // EntityTypeOperationRelease
        0x48b4e0L,  // WwadArchiveIndexLoad
        0x483b70L,  // Direct3DDeviceStateApply
        0x480900L,  // CachedRenderResourceInstanceCreate
        0x477590L,  // FormattedTextEscapeDecodeAndEmit
        0x47d800L,  // Bitmap24BitSurfaceConvert
        0x435dc8L,  // CameraViewContextClose
        0x4248b0L,  // SelectedEntityGroundMarkerRender
        0x42cd50L,  // UpgradeNameEntryFind
        0x41f420L,  // SelectedEntitiesAnyMatch
        0x4102b0L,  // UiTextUnderscoresToSpaces
        0x428780L,  // TerrainCellCommandPredicate
        0x42b480L,  // RectangularResourceTableDestroy
        0x4798b0L,  // RandomFloatRange
        0x44f1a0L,  // InputKeyMappingAddBidirectional
        0x441940L,  // EntityActionSequenceReset
        0x44a310L,  // ObjectMovementInputAccumulate
        0x46a510L,  // SceneObjectStateDispatch
        0x48bce0L,  // LwoParserRecordAtIndex
        0x47d700L,  // PopCount32
        0x47d720L,  // FindFirstSetBit32
        0x48fb90L,  // GameFileReadBuffered
        0x479020L,  // SortedEntryNextEqualKey
        0x402ae0L,  // TerrainCellRefreshDispatch
        0x402510L,  // TerrainCellCommandCreate
        0x439e70L,  // EntityAttachedObjectListDetach
        0x441b30L,  // EntityStateReset
        0x46b520L,  // UiTextEntryLoad
        0x46f0d0L,  // ObjectTypeNameIndexFind
        0x4859b0L,  // TextBufferReset
        0x496c00L,  // Win32FileDescriptorSeek
        0x491430L,  // FormatArgumentReadDword
        0x491450L,  // FormatArgumentReadDouble
        0x491470L,  // FormatArgumentReadWord
        0x41a7d0L,  // InputKeyGroupRegister
        0x403c00L,  // EntityActivityRecordRemoveByType
        0x401c50L,  // FormattedTextPrintfWidth
        0x401c80L,  // FormattedTextPrintf
        0x47a150L,  // FormattedTextFormatDraw
        0x47a1e0L,  // FormattedTextDrawAndMeasure
        0x47a470L,  // BitmapFontGlyphDrawAndMeasure
        0x40d240L,  // TerrainCellHasOccupiedOrFlaggedNeighbor
        0x40d3f0L,  // TerrainCellSpecialStatePredicate
        0x40de40L,  // EntityFenceSparkTerrainInteraction
        0x432aa0L,  // TerrainCellOccupancyOrFlagCheck
        0x432170L,  // TerrainCellFlag100Test
        0x474cc0L,  // LwoLayerPolygonVertexDataRead
        0x474c40L,  // LwoLayerPolygonVertexCount
        0x4788d0L,  // Direct3DRenderStateSetCached
        0x47f600L,  // DebugOutputFormat
        0x485230L,  // TextBufferAppendFormat
        0x485250L,  // TextBufferAppendVFormat
        0x493c10L,  // FpuControlWordUpdateMasked
        0x494c70L,  // ScanStreamGetChar
        0x485ff0L,  // SurfaceDrawPolyline
        0x486720L,  // BresenhamLineWalkClipped
        0x47f590L,  // WadMountPathCacheStore
        0x47fc20L,  // GameFileReadByte
        0x47fce0L,  // GameFileReadLine
        0x47fd40L,  // GameFileReadLineDispatch
        0x47fdb0L,  // GameFilePrintFormat
        0x47fe20L,  // GameFileSystemTypeGet
        0x47ff50L,  // GameFileHandleRelease
        0x47ffc0L,  // MountedPathPrefixRewrite
        0x486090L,  // SurfaceFillPixelSpans
        0x486210L,  // SurfaceFillPixelSpanRecords
        0x486390L,  // SurfaceDrawCirclePixels
        0x4864d0L,  // SurfaceColorPackToPixel
        0x486550L,  // SurfaceLockConfigurePixelWriter
        0x486650L,  // SurfaceUnlockResetPixelWriter
        0x486690L,  // SurfacePixelWriterSelectByFormat
        0x4772d0L,  // CameraProjectPointToScreen
        0x479530L,  // SurfaceTriangleNormalCalculate
        0x474b30L,  // LwoLayerEnabledSet
        0x435170L,  // TerrainSurfaceTypeNameGet
        0x435f28L,  // TerrainGridBorderInsetGet
        0x4525b0L,  // SelectedEntitySetHasAny
        0x48fe50L,  // BufferedStreamReadLine
        0x48fe20L,  // BufferedStreamGetByte
        0x48f830L,  // FileFindNextRecord
        0x48f680L,  // FileSetReadOnlyAttribute
        0x48fa20L,  // DriveCurrentDirectoryGet
        0x48ffa0L,  // PathResolveFull
        0x496ee0L,  // Win32ErrorMapToErrno
        // --- reward panel, terrain view, and simulation helpers (finding 238) ---
        0x460950L,  // CheckpointBlobTaskProgressRecalculate
        0x460f00L,  // RewardStatsPanelDraw
        0x461070L,  // RewardStatsTextFormat
        0x45dbc0L,  // TerrainViewTileGeometryBuild
        0x45e9e0L,  // TerrainTileStyleResolve
        0x45e940L,  // ViewPointToScreenGeometryTransform
        0x46d380L,  // AttachedModelOrientationUpdate
        0x405c10L,  // PendingCommandEntityAssignmentTick
        0x41baa0L,  // SimulationPointerActionDispatch
        0x407d60L,  // BuildingTypeConfigRecordParse
        0x433e80L,  // TerrainProjectedCellStateRefresh
        0x470760L,  // MeshBuildEightSidedSegment
        0x493ef0L,  // CrtFormattedScanV
        0x4075a0L,  // TickEntityScreenEffects
        0x410740L,  // SaveMenuListEntryAppend
        0x411530L,  // MenuWidgetItemScreenRect
        0x411790L,  // MenuPickWidgetUnderPointer
        0x433770L,  // SceneLightsBuildFromConfig
        0x451cd0L,  // MessageStringTableInitialize
        0x459b00L,  // RenderEntityScreenRing
        0x45adb0L,  // SimCursorInteractionTick
        0x45b310L,  // SimCategoryCellHitTest
        0x4638a0L,  // SimTrackCursorHitTest
        0x46b1d0L,  // TooltipOverlayStateInitialize
        0x46bc30L,  // TimedEffectSlotRender
        0x4734f0L,  // DDiSceneLightCreate
        0x4083a0L,  // SceneEntitySlotTableRebuild
        0x4181b0L,  // HelpWindowResourcesInit
        0x41ee80L,  // CellGridPointHitTest
        0x4355c0L,  // MenuOptionPanelTextureUpdate
        0x43eea0L,  // SimulationEntityQueueFlush
        0x4403d0L,  // LevelDebugCellRetargetDispatch
        0x4515e0L,  // TerrainCellRayHitTest
        0x45b590L,  // RectContainsOffsetPoint
        0x462ad0L,  // RewardScrollTickerAdvanceAndDraw
        0x462f20L,  // TerrainCellQuadGeometryEmit
        0x46c160L,  // UpgradeLevelTableParse
        0x46c510L,  // SceneChannelSlotsRebuild
        0x46db90L,  // CellQuadRunStage
        0x46de80L,  // CellQuadRunEmit
        0x405950L,  // PendingCommandTimerFireScan
        0x407a10L,  // PendingTypeValueSlotSelect
        0x40ab70L,  // DependencyGridConfigLoad
        0x40d850L,  // TerrainCellNeighborFloodWalk
        0x442b10L,  // EntityMovementAccumulatorApply
        0x442f80L,  // EntityPoseOrientationDispatch
        0x4438b0L,  // PathStepAdvanceAndArrivalCheck
        0x444680L,  // StepMovementVectorResolve
        0x4469c0L,  // EntityNearestConnectedTerrainCellRouteSelect
        0x44ead0L,  // TerrainCellListSurfaceHeightClampMutate
        0x463230L,  // TerrainDirtyCellQuadNormalEmitPass
        0x463590L,  // TerrainCellSurfaceNormalGet
        0x406990L,  // TypeTableResourceRowLoad
        0x40e490L,  // EncyclopediaConfigFilesOpen
        0x434710L,  // PanelButtonLayoutConfigLoad
        0x446610L,  // EntityMovementTickScaleAndDeltaGet
        0x45a030L,  // UiPanelNameTablesInit
        0x45b9d0L,  // PanelRotationControlConfigLoad
        0x47d000L,  // PalettizedBitmapSurfaceDecode
        0x485a20L,  // KeyboardKeyNamesInit
        0x4877e0L,  // SceneEntrySlotTableExpand
        0x497130L,  // Win32FileDescriptorRead
        0x498350L,  // NumberStringScanParse
        0x499240L,  // Win32FileDescriptorOpen
        0x408ec0L,  // KindNineNeighborEmitForFootprintCells
        0x418450L,  // HelpWindowInfoConfigStringsLoad
        0x41ada0L,  // InterfaceImagesConfigLoad
        0x4425f0L,  // SubobjectPoseDeltaNormalizeAndDispatch
        0x44db50L,  // KeyValueEqualsFileLoad
        0x463f00L,  // SceneQuadGridEmit
        0x46e610L,  // TerrainEdgeFlagMidpointUpdate
        0x475a70L,  // SceneActionRecordActFrameScan
        0x47a520L,  // BitmapFontGlyphMeasureWidth
        0x481b80L,  // CounterFormatStringBuild
        0x485710L,  // TextWrapMeasureAndDrawLines
        0x491b40L,  // EnginePoolChunkTryAllocate
        0x496cc0L,  // Win32FileDescriptorWrite
        0x409600L,  // TerrainDescriptorTilesResetAndFree
        0x411f10L,  // PercentSplitSurfaceBlit
        0x412750L,  // MenuHoverStatusTextDraw
        0x41ab00L,  // InterfaceBuildImagesConfigLoad
        0x44c1b0L,  // EntityHeadingStepMove
        0x44c9c0L,  // LightEffectsFadeConfigLoad
        0x44cca0L,  // LightEffectsMoveConfigLoad
        0x44e8c0L,  // SurfaceVertexSpanValidate
        0x45d110L,  // RegionContainmentScanUpdate
        0x4702b0L,  // EntitySurfaceQuadSegmentIntersect
        0x476bf0L,  // SurfacePixelRowsFlipInPlace
        0x479630L,  // SurfaceTriangleAreaCalculate
        0x47d520L,  // Palette8To16BitSurfaceExpand
        0x4124f0L,  // MenuSaveImageConfigRectBlit
        0x41b680L,  // CatalogCallbackByIdDispatch
        0x433c10L,  // EntityNeighborBlockedCellStepProbe
        0x441ed0L,  // EntityFaceTargetActionStart
        0x446e40L,  // EntityNearestCellRouteCommit
        0x448a60L,  // EntityNeighborCellMoveCommandHandle
        0x44bc30L,  // EntityMotionToSceneNodeSync
        0x450960L,  // CellDirtyQueuePushAndClear
        0x465f40L,  // InstanceQuadVerticesBuild
        0x471a60L,  // EntityVelocitySteerBlendUpdate
        0x47d740L,  // SurfacePixelRowsFlipWithScratch
        0x487a00L,  // SharedObjectClone
        0x4899f0L,  // ObjectHierarchyRecursiveClone
        0x40b470L,  // ProjectedQuadScreenAreaCalculate
        0x40c990L,  // MiscObjectsEffectSlotBind
        0x41afd0L,  // InterfaceSurroundImagesConfigLoad
        0x4334f0L,  // RenderQualityConfigApply
        0x440a10L,  // AttachedEntityDirectionVectorUpdate
        0x44a5d0L,  // NearestOpenCellCoordSearch
        0x44dde0L,  // StringTableEntrySetCurrent
        0x453da0L,  // CameraFocusTweenUpdate
        0x45bcd0L,  // CameraDragFromCursorApply
        0x45d8b0L,  // ScreenCoordNormalizeInit
        0x46faf0L,  // EntitySpawnRecordSlotInit
        0x471fc0L,  // EntityVelocitySlotDispatch
        0x481ac0L,  // StringTrailingDigitsSplit
        0x4915a0L,  // EnginePoolArenaCreate
        // --- batch 9 (config bands, graphics mode, anim/route, flight-record helpers) ---
        0x401530L,  // AdvisorPositionConfigLoad
        0x4070b0L,  // BubbleActionStringsInit
        0x409d50L,  // EntityCellPathEffectsApply
        0x41aa00L,  // EntityTableMapsAllocate
        0x4345f0L,  // PanelsConfigEntryApply
        0x434af0L,  // TextMessagesConfigLoad
        0x437150L,  // ToolTipIconsConfigLoad
        0x444260L,  // EntityRandomCellStepRouteIssue
        0x447950L,  // EntityDirectionalActivityStart
        0x46a7f0L,  // UiMessageStringsInit
        0x46fc70L,  // ObjectFlightCurveRecordInit
        0x47e510L,  // DirectDrawDisplayModeSetup
        0x482500L,  // InstanceQuadVertexDataFill
        0x4882e0L,  // SceneNodeTransformMatrixApply
        // --- batch 10 (config loaders, menu/panel records, ray-scan, and layout helpers) ---
        0x40bfd0L,  // RockFallStylesConfigLoad
        0x40c3a0L,  // ResourceAvailabilitySweep
        0x40dc20L,  // CellPairEdgeEffectApply
        0x409790L,  // CellPathRecordRegisterApply
        0x4185f0L,  // HelpWindowButtonLayoutLoad
        0x411100L,  // MenuSectionRecordCreate
        0x41eca0L,  // AdvisorDialogStackLayout
        0x431b20L,  // PlacementBlockedAdjacentCellScan
        0x431c70L,  // FirstFreeNeighborCellPick
        0x401270L,  // AdvisorConfigSlotLookup
        0x40e1e0L,  // CellCornerOccupancyScan
        0x418da0L,  // HelpWindowButtonHitTest
        0x41eac0L,  // EventTimerCountdownAdvance
        0x427e00L,  // ToolTipInfoStringsLoad
        0x433180L,  // CellTypeSpeedMultiplierResolve
        0x43a650L,  // ActionKindEntityCreateAndCommandIssue
        0x442300L,  // EntityVelocityVectorDispatch
        0x4477d0L,  // PathStepLookaheadEntryWrite
        0x44b680L,  // EntitySceneObjectTreeAttach
        0x4519b0L,  // VisualModelRecordCreate
        0x45a9c0L,  // PanelTextureSlotsBlit
        0x45cdc0L,  // PriorityImagesConfigLoad
        0x46cff0L,  // SceneRecordTemplateInstantiate
        0x46dd10L,  // StagedCellCandidateSelect
        0x473c60L,  // XfTextureNameFormat
        0x4748f0L,  // MeshRecordAllocateOrAppend
        0x47d2d0L,  // SurfacePixelValueEncode
        0x482b50L,  // RenderStateFlagsApply
        0x402960L,  // EntityKindCommandIssue
        0x40a4b0L,  // QuadMeshRenderRecordCreate
        0x40c5b0L,  // ResourceSlotRegisterName
        0x41b3c0L,  // EntityQueryFlagCount
        0x438460L,  // EntityKindRecordUpdateDispatch
        0x4396c0L,  // ItemNameByKindLookup
        0x449700L,  // RouteStepCommandIssue
        0x44bb30L,  // EntityNodeScaleUpdate
        0x44d0d0L,  // DriftOffsetStepApply
        0x4512d0L,  // TerrainCellNormalSample
        0x451ab0L,  // PrimitiveModelRecordCreate
        0x466250L,  // ExpandCellEntitySpawn
        0x46e9d0L,  // StagedCellCandidateCollect
        0x4826c0L,  // MeshVertexRecordFill
        0x403750L,  // CommandKindRecordIssueAndLink
        0x407240L,  // BubbleConfigTextureLoad
        0x40a5e0L,  // RecordCountDigitsFill
        0x40b1b0L,  // DependencyGridOccupancyPass
        0x40e0c0L,  // FenceSparkLaunchRegister
        0x40f6c0L,  // SceneNodeRecordCreate
        0x412af0L,  // MenuSurfaceBlitScaled
        0x41a920L,  // LevelStateCountersClear
        0x41c1c0L,  // AdvisorOverlayHitResolve
        0x427f80L,  // LevelUpgradeNameStringsLoad
        0x4455a0L,  // EntityMoveVectorSurfaceProbe
        0x4473b0L,  // RouteToCellCommandIssue
        0x449880L,  // BlockedCellApproachWeightWrite
        0x46ab10L,  // TextSlotMessageAssign
        0x46e220L,  // CellSurfaceQuadHeightFill
        0x474f10L,  // SceneObjectBoundsUnion
        0x488170L,  // AnimationTrackKeysInterpolate
        0x4068c0L,  // PathArcLengthInterpolate
        0x40c230L,  // SceneSlotPoseScaleRegister
        0x40e350L,  // CellEntityKindCreateOrRemove
        0x40f5f0L,  // ErosionEightNeighborPropagate
        0x4105f0L,  // MenuParamRecordCreate
        0x4129d0L,  // MenuOverlaySurfaceAndTextSetup
        0x419de0L,  // RecordListEntryMoveToFront
        0x435230L,  // RandomVariantCellScatter
        0x4507d0L,  // TerrainCellEdgeVectorQuery
        0x451b30L,  // LinkedRecordListFlatten
        0x45e6d0L,  // TerrainPointPickResolve
        0x464110L,  // RotatedPairsScratchFill
        0x4780b0L,  // RendererWindowModeApply
        0x482620L,  // MeshSegmentVertexFill
        0x48c040L,  // StringCaseSubstringFind
        0x402d50L,  // KindNineCellCommandIssue
        0x404100L,  // CommandQueueSweepRelink
        0x408c80L,  // StringTableIndexLookup
        0x41c6e0L,  // PointerActionStateDispatch
        0x41cd30L,  // PositionTrackerBoxDraw
        0x41f740L,  // CellFourNeighborWalkCheck
        0x434480L,  // CellSurfaceUpdateDispatch
        0x43c570L,  // KindFourPostCreateResolve
        0x44a4f0L,  // NearestCellCoordinateWorldResolve
        0x44b140L,  // SmallSpiderCellSwarmSpawn
        0x4508a0L,  // CellNeighborDirectionCode
        0x451180L,  // CellVariantEntryMatchCheck
        0x45cf50L,  // PrioritiesConfigLoad
        0x46b660L,  // CommandSampleStatsAccumulate
        0x46f270L,  // TypeMotionScaleApplyDispatch
        0x471960L,  // SeeThroughWallsRayCheck
        0x473bc0L,  // ResourceNameRegisterOrQuery
        0x47c2e0L,  // DirectDrawDeviceCreate
        0x47c4b0L,  // DirectDrawEnumeratedDeviceCreate
        0x409470L,  // NeighborCellEntitySearchRecurse
        0x40b880L,  // CellVisualChannelRelay
        0x40ba00L,  // SceneRotationFlagsApply
        0x40bb90L,  // SceneFrameFromTripletApply
        0x40c0d0L,  // SceneActSlotLoadAndRegister
        0x40cb70L,  // SceneTimeoutEntrySweep
        0x410490L,  // StringRegistryIntern
        0x419e80L,  // RecordListEntryRemoveAndRelease
        0x44a3d0L,  // PlaceCellNearestResolve
        0x44ae50L,  // RandomKindSixScatterSpawn
        0x44c7f0L,  // LightEffectsBlinkConfigApply
        0x450a80L,  // CellListMinDistanceScaleApply
        0x476330L,  // ResourceTreeCallbackWalk
        0x476410L,  // ResourceNameMatchCallback
        0x489910L,  // ResourceTreeCallbackCount
        0x4089d0L,  // Segment2DPointDistance
        0x40f0e0L,  // KindSixRandomMessageDispatchSweep
        0x40f920L,  // KindTwoSteeringTorqueApply
        0x40fb90L,  // SteeringPhysicsStep
        0x410690L,  // MenuRecordResourcesRelease
        0x41c800L,  // CatalogSlotFlagUpdate
        0x424730L,  // SinePhaseMidpointBlendDispatch
        0x439a20L,  // GatedStepCommandIssueWithTargetRecord
        0x43a120L,  // CellStepCommandIssueWithJitter
        0x43aab0L,  // ProximityActivityRaise
        0x4420d0L,  // KindEffectSlotInitDispatch
        0x443510L,  // SceneObjectPosePositionSync
        0x4440f0L,  // ActivityTimeoutAccumulateAndRaise
        0x444460L,  // FlaggedVectorStepCommandIssue
        0x448880L,  // RecordGatedStepCommandIssue
        0x448c90L,  // CardinalStepCommandIssueAndLink
        0x448eb0L,  // ProximityStepCommandIssueWithJitter
        0x4490a0L,  // ProximityStepCommandIssueWithLink
        0x4492b0L,  // CellStepCommandIssueWithTargetMark
        0x449ac0L,  // ProximityStepCommandIssueClearingGate
        0x44b500L,  // ObjectPositionStepCommandIssueAndLink
        0x453bb0L,  // FrameTimerArrayAdvanceAndAdvisorClear
        0x463d20L,  // ViewBlitRectPositionsApply
        0x466490L,  // CellSightRayCheckAndFlag
        0x470540L,  // SegmentCenterProximityCheck
        0x471d20L,  // VelocitySlotSteerDispatchAndSound
        0x471f00L,  // PoseOffsetVelocitySlotBlend
        0x482370L,  // MeshVerticesScaleApply
        0x488a70L,  // LoadRecordInterfacesReleaseFree
        0x44afb0L,  // ConfiguredTinyMonsterSpawnAndTeardown
        0x44b250L,  // PairingActivityRaiseAndMutualLink
        0x4350a0L,  // SurfaceTypeDescriptionsConfigLoad
        0x444fb0L,  // EntityTurnToVectorActivityIssue
        0x4451e0L,  // EntityPathStepVectorNormalize
        0x44a7d0L,  // PolyVariantSectionModelBuild
        0x44d760L,  // JitterStepApply
        0x45ac30L,  // PanelTextureSlotsAllocate
        0x4629d0L,  // PanelEventEntryAppend
        0x46f3b0L,  // RayHitScanAndEffectApply
        0x482220L,  // MeshRecordAllocateAndFill
    };
    static final String[] NAMES = { "EngineTick", "EngineShutdown", "EngineShutdownAlt",
        "MainSimStep", "ManagerTick", "MessageDispatch", "ViewContextTick",
        "EnginePreTick", "QuitPoll", "SelectedEntity", "TransitionControl",
        "EntityRename", "KeyToAscii", "KeyContextCheck", "SystemStateFlags",
        "MessagePreCheck", "KeepRunning", "SetSystemStateFlags",
        "SelectionRoot", "SelectionClusterA", "SelectionClusterB",
        "SelectionGateHelper", "LevelLoad", "PendingLevelName",
        "VehicleTypeInit", "EntityPointer", "MapLoadPipeline", "TerrainCreate",
        "MapFileReadHeader", "MapFileGetCell", "LoadPredugLayer",
        "LoadErodeCryoreLayers", "LoadEmergeLayer", "LoadPathAiLayers",
        "LoadFallinLayer", "LoadBlockPointers", "LoadTerrainMap",
        "LoadLayerL", "LoadLayerM", "CellInitPass",
        "SavReadSlot", "SavWriteSlot", "SavRefreshSlots", "SavSlotCtx", "SavBuildCtx",
        "SavGetSlot", "SavSetSlot", "SavMarkRecord", "SavWriteRecord", "SavRecordPtr",
        "SavCommit", "SavTreeLoad", "SavTreeWalk2", "SavTreeWalk", "SavClearVisited",
        "SavMenuList", "LevelNodeGet", "ListHasItem", "ListGetNode", "ListAddName",
        "ListSetNode", "SaveMenuBuild", "LoadMenuBuild", "IsInGame", "ActiveObjectList",
        "ListFindName", "CheckpointStep", "SavObjSerialize", "SavObjCallbackB",
        "CheckpointBlobBuild", "CheckpointBlobFree", "CheckpointBlobGet",
        "RewardDisplayTick", "RewardZoneCheck", "OsfAdd", "OsfSnapshot", "OsfWrite",
        "ObjectiveTick", "RewardInit", "TaskBlockInit", "TaskBlockFinish", "TaskSoundsLoad",
        "TaskFontLoad", "TaskBoxImages", "TaskImages", "TaskBoxRect", "TaskFlicPath",
        "TaskImportanceRefresh", "TaskQuotaRefresh", "TaskFlushImages", "TaskFlushFont",
        "CrystalQuotaSetup", "RewardScoreAdd", "CmdRegistryInit", "CmdKindByName", "CmdAlloc",
        "CmdFree", "CmdPoolGrow", "CmdCreateMove", "CmdCreateTarget", "CmdCreateTask",
        "CmdCreateGated", "CmdLinkHead", "CmdLinkTail", "CmdChainWalk", "IdleQueueRegister",
        "CmdTickGlobal", "CmdChainPurge", "CmdFlushAll", "MinionCompanionCmd",
        "CmdCancelByCoords", "CmdCancelRef", "CmdMarkMatching", "CmdSetKind",
        // batch 12 — front-end & menus (finding 23)
        "ScrollingTextDisplay", "MainMenuTextItem", "MenuTextRecordMake",
        "MenuButtonRecordMake", "MenuItemImageSet", "ListAppend", "MenuTick",
        "MenuRunLoop", "MenuShow", "MenuItemType", "StrReplace",
        "MenuItemRecordAlloc", "LoadItemStrings", "ListAlloc", "MenuBuild",
        "MenuListFindName", "AvisEnabled", "BrightnessSet", "GameSpeedSelect",
        "VolumeSet", "OptionMusicDefault", "ScrollToSliderIndexStore",
        "ScrollToSliderIndex", "ScrollSliderRefresh", "SaveMenuRefresh",
        "OptionsApplyFromSlot", "FrontEndStartLevel", "SlotSummary",
        "OverlayMenuGate", "LevelChainBuild", "MoviePlayWait", "MoviePlayLoop",
        "MoviePlayNamed", "MoviePlayLevelVideo", "MovieShownReset",
        "SaveMenuLayoutLoad", "StreamPlayLevel", "StreamPlayTutorial",
        "OptionsWriteToSlot", "OptionsLoadOrDefault", "FrontEndStateChange",
        "MenuEnter", "FrontEndActive", "SaveSlotOpen", "FrontEndSelectedAction",
        "QuitMenuProbe", "LevelStartFlag", "MenuOptionsFlag", "StartLevelPilots",
        "StartLevelSelected" };

    // Semantic names supported by the findings corpus. Targets without a
    // sufficiently specific documented role keep Ghidra's FUN_<address> name.
    static String documentedName(long address) {
        switch ((int) address) {
            case 0x401000: return "AdvisorSceneControllerInit";
            case 0x4018c0: return "AdvisorRequestAdmit";
            case 0x401930: return "AdvisorRequestResetAndSubmitSound";
            case 0x401990: return "AdvisorTransitionWorker";
            case 0x401a70: return "AdvisorSceneAdvanceAndCompletionCheck";
            case 0x401b20: return "AdvisorClearActiveState";
            case 0x401b30: return "AdvisorApplySlotTransform";
            case 0x403a50: return "EntityDrainCommandChain";
            case 0x403d10: return "EntityCommandUnlinkByKind";
            case 0x403f30: return "EntityActivityListRegister";
            case 0x406540: return "EntityPoolCallbackSearch";
            case 0x406e40: return "ActivityRunnerUpdate";
            case 0x406f80: return "ActivityRunnerStateRead";
            case 0x408380: return "SceneAssignEntityOwner";
            case 0x408570: return "SceneActivityChange";
            case 0x4085f0: return "SceneActivityChangeSecondary";
            case 0x408940: return "EntityPayloadTeardown";
            case 0x4089a0: return "EntityPayloadStateSet";
            case 0x409300: return "TutorialPathDescriptorCreate";
            case 0x4090c0: return "TerrainDescriptorFindExact";
            case 0x40aa10: return "ScreenEffectLaneDispatch";
            case 0x40aa40: return "ScreenEffectOpacityUpdate";
            case 0x40cce0: return "LavaEffectSelectAndPlace";
            case 0x40ea10: return "LavaErosionStageRegister";
            case 0x40eab0: return "LavaErosionPendingCellTick";
            case 0x40ee50: return "LavaErosionPendingCellAdd";
            case 0x40f000: return "LavaErosionNeighborCheck";
            case 0x40f330: return "LavaErosionNeighborMutate";
            case 0x40f2b0: return "LavaErosionNeighborEligible";
            case 0x411a00: return "MenuInputTick";
            case 0x419230: return "IncidentMessageCatalogInit";
            case 0x4193e0: return "IncidentMessageConfigLookup";
            case 0x4194b0: return "IncidentMessageTemplateReplace";
            case 0x419530: return "IncidentMessageTemplateCompile";
            case 0x41a080: return "IncidentMessageTextConsumer";
            case 0x41d040: return "TutorialPathMenuAction";
            case 0x41dca0: return "UiActionDispatch";
            case 0x41ea50: return "StoreSelectedMapCoordinate";
            case 0x41f900: return "UpgradeResourceCheck";
            case 0x41f9e0: return "UpgradeResourceDebit";
            case 0x41fb50: return "GameConfigInit";
            case 0x4261c0: return "SpawnCoordinatePoolTick";
            case 0x426420: return "CursorRayToCameraRoute";
            case 0x426520: return "MainSimInteractionAndCellUpdate";
            case 0x429470: return "CameraPointerRayRoute";
            case 0x42f350: return "TerrainNeighborCoordinateCheck";
            case 0x430530: return "TerrainCellMutation";
            case 0x430320: return "TerrainCellWorkFlagClear";
            case 0x430470: return "TerrainNeighborFlagSet";
            case 0x431450: return "TerrainNeighborFlagRefresh";
            case 0x432770: return "TerrainCellWalkabilityQuery";
            case 0x432b50: return "TerrainWalkableNeighborSearch";
            case 0x432c90: return "TerrainCellStateUpdate";
            case 0x432e00: return "TerrainCellPredugEdgeWallFlagSet";
            case 0x433090: return "TerrainCoordinateAndOccupancyCheck";
            case 0x433380: return "TerrainCellMutationProgress";
            case 0x434a50: return "SoundSamplesConfigLoad";
            case 0x434c10: return "IncidentMessageConfigLoad";
            case 0x435dc8: return "CameraViewContextClose";
            case 0x4248b0: return "SelectedEntityGroundMarkerRender";
            case 0x42cd50: return "UpgradeNameEntryFind";
            case 0x41f420: return "SelectedEntitiesAnyMatch";
            case 0x4102b0: return "UiTextUnderscoresToSpaces";
            case 0x428780: return "TerrainCellCommandPredicate";
            case 0x42b480: return "RectangularResourceTableDestroy";
            case 0x4798b0: return "RandomFloatRange";
            case 0x44f1a0: return "InputKeyMappingAddBidirectional";
            case 0x441940: return "EntityActionSequenceReset";
            case 0x44a310: return "ObjectMovementInputAccumulate";
            case 0x46a510: return "SceneObjectStateDispatch";
            case 0x48bce0: return "LwoParserRecordAtIndex";
            case 0x47d700: return "PopCount32";
            case 0x47d720: return "FindFirstSetBit32";
            case 0x48fb90: return "GameFileReadBuffered";
            case 0x479020: return "SortedEntryNextEqualKey";
            case 0x402ae0: return "TerrainCellRefreshDispatch";
            case 0x402510: return "TerrainCellCommandCreate";
            case 0x439e70: return "EntityAttachedObjectListDetach";
            case 0x441b30: return "EntityStateReset";
            case 0x46b520: return "UiTextEntryLoad";
            case 0x46f0d0: return "ObjectTypeNameIndexFind";
            case 0x4859b0: return "TextBufferReset";
            case 0x496c00: return "Win32FileDescriptorSeek";
            case 0x491430: return "FormatArgumentReadDword";
            case 0x491450: return "FormatArgumentReadDouble";
            case 0x491470: return "FormatArgumentReadWord";
            case 0x41a7d0: return "InputKeyGroupRegister";
            case 0x403c00: return "EntityActivityRecordRemoveByType";
            case 0x401c50: return "FormattedTextPrintfWidth";
            case 0x401c80: return "FormattedTextPrintf";
            case 0x47a150: return "FormattedTextFormatDraw";
            case 0x47a1e0: return "FormattedTextDrawAndMeasure";
            case 0x47a470: return "BitmapFontGlyphDrawAndMeasure";
            case 0x40d240: return "TerrainCellHasOccupiedOrFlaggedNeighbor";
            case 0x40d3f0: return "TerrainCellSpecialStatePredicate";
            case 0x40de40: return "EntityFenceSparkTerrainInteraction";
            case 0x432aa0: return "TerrainCellOccupancyOrFlagCheck";
            case 0x432170: return "TerrainCellFlag100Test";
            case 0x474cc0: return "LwoLayerPolygonVertexDataRead";
            case 0x474c40: return "LwoLayerPolygonVertexCount";
            case 0x4788d0: return "Direct3DRenderStateSetCached";
            case 0x47f600: return "DebugOutputFormat";
            case 0x485230: return "TextBufferAppendFormat";
            case 0x485250: return "TextBufferAppendVFormat";
            case 0x493c10: return "FpuControlWordUpdateMasked";
            case 0x494c70: return "ScanStreamGetChar";
            case 0x485ff0: return "SurfaceDrawPolyline";
            case 0x486720: return "BresenhamLineWalkClipped";
            case 0x47f590: return "WadMountPathCacheStore";
            case 0x47fc20: return "GameFileReadByte";
            case 0x47fce0: return "GameFileReadLine";
            case 0x47fd40: return "GameFileReadLineDispatch";
            case 0x47fdb0: return "GameFilePrintFormat";
            case 0x47fe20: return "GameFileSystemTypeGet";
            case 0x47ff50: return "GameFileHandleRelease";
            case 0x47ffc0: return "MountedPathPrefixRewrite";
            case 0x486090: return "SurfaceFillPixelSpans";
            case 0x486210: return "SurfaceFillPixelSpanRecords";
            case 0x486390: return "SurfaceDrawCirclePixels";
            case 0x4864d0: return "SurfaceColorPackToPixel";
            case 0x486550: return "SurfaceLockConfigurePixelWriter";
            case 0x486650: return "SurfaceUnlockResetPixelWriter";
            case 0x486690: return "SurfacePixelWriterSelectByFormat";
            case 0x4772d0: return "CameraProjectPointToScreen";
            case 0x479530: return "SurfaceTriangleNormalCalculate";
            case 0x474b30: return "LwoLayerEnabledSet";
            case 0x435170: return "TerrainSurfaceTypeNameGet";
            case 0x435f28: return "TerrainGridBorderInsetGet";
            case 0x4525b0: return "SelectedEntitySetHasAny";
            case 0x48fe50: return "BufferedStreamReadLine";
            case 0x48fe20: return "BufferedStreamGetByte";
            case 0x48f830: return "FileFindNextRecord";
            case 0x48f680: return "FileSetReadOnlyAttribute";
            case 0x48fa20: return "DriveCurrentDirectoryGet";
            case 0x48ffa0: return "PathResolveFull";
            case 0x496ee0: return "Win32ErrorMapToErrno";
            case 0x435e0e: return "CameraViewContextSet";
            case 0x436900: return "CameraPanAxisUpdate";
            case 0x4369f4: return "CameraTiltAxisUpdate";
            case 0x436aa5: return "CameraZoomUpdate";
            case 0x436b87: return "CameraPointerPanUpdate";
            case 0x4372a0: return "SpawnCoordinatePoolClear";
            case 0x437540: return "EntityTeardown";
            case 0x438c60: return "EntityCandidateSearch";
            case 0x43af30: return "CreatureSpawnFromEmergence";
            case 0x4401b0: return "TerrainCellToWorldPosition";
            case 0x4410f0: return "TerrainNeighborPathCheck";
            case 0x44e0c0: return "TerrainCreate";
            case 0x44e4d0: return "TerrainGridVertexBuild";
            case 0x451f60: return "CommandQueueCancelOpcode";
            case 0x452520: return "SelectionSnapshotCreate";
            case 0x452610: return "SelectionContainsEntity";
            case 0x4526e0: return "SelectionCandidateFilter";
            case 0x452be0: return "SelectionClear";
            case 0x452e70: return "ObjectiveAdvisorTokenLoad";
            case 0x453200: return "ObjectiveAdvisorTokenLookup";
            case 0x453320: return "TutorialScriptCommandDispatch";
            case 0x456b80: return "TutorialPathCoordinateIndex";
            case 0x45c830: return "PointerCursorNameRegistryInit";
            case 0x45ca70: return "PointerCursorConfigLoad";
            case 0x45cbd0: return "PointerCursorConfigParse";
            case 0x45cc20: return "PointerCursorSelect";
            case 0x45cc40: return "PointerCursorOverrideSet";
            case 0x45cc70: return "PointerCursorDraw";
            case 0x45cd90: return "PointerCursorOverrideTick";
            case 0x4667e0: return "EntityStatsConfigLoad";
            case 0x469c10: return "EntityBuildingIndexSet";
            case 0x469de0: return "EntityEnergyDecayRateGet";
            case 0x469f40: return "BuildingUpgradeAdmissionCheck";
            case 0x46a060: return "PermittedActivityCapacityCheck";
            case 0x46a730: return "CameraMoveCommandRoute";
            case 0x46ac20: return "AdvisorTextRequestSetup";
            case 0x46ac60: return "AdvisorTextAnimationUpdate";
            case 0x46ad00: return "AdvisorTextDisplayWorker";
            case 0x46b770: return "TimedEffectUpdateRequestSet";
            case 0x46b7c0: return "TimedEffectAndCueUpdate";
            case 0x46c3f0: return "SceneActivityPairedChannelUpdate";
            case 0x46d140: return "SceneAssignOwnerFromModel";
            case 0x457420: return "SetTutorialBlockIsPath";
            case 0x464c70: return "SoundAliasLookup";
            case 0x464d00: return "SoundSampleMarkerParse";
            case 0x464e20: return "SoundSampleVariantSelect";
            case 0x464f60: return "SoundRequestQueueSubmit";
            case 0x464fa0: return "SoundAliasPlay";
            case 0x465090: return "PositionalSoundSubmit";
            case 0x4651a0: return "SoundFrameTick";
            case 0x4652d0: return "SoundBatchQueueDrain";
            case 0x47ad70: return "SoundBufferPlayWithPosition";
            case 0x47b970: return "StreamedWavePlaybackStart";
            case 0x47baa0: return "StreamedWaveFormatAndBufferSetup";
            case 0x47bdb0: return "StreamedWaveRingBufferTick";
            case 0x4722b0: return "ConfiguredVideoGraphSetup";
            case 0x472390: return "ConfiguredVideoComRouteInit";
            case 0x4724a0: return "ConfiguredVideoSampleUpdate";
            case 0x47e800: return "AviFrameRetrieverOpen";
            case 0x47e9e0: return "AviFrameDecodeAndCopy";
            case 0x47eb20: return "AviFrameUpdateAndBlit";
            case 0x47ed00: return "AviPlaybackFrameLoop";
            case 0x47f6e0: return "GameFileOpenDispatch";
            case 0x47f850: return "WadFileSeek";
            case 0x47f980: return "WadFileRead";
            case 0x47fb10: return "WadFileCursorGet";
            case 0x48b330: return "RegistryPathSplit";
            case 0x48b360: return "RegistryPathAccessor";
            case 0x48b390: return "RegistryPathCreateKeys";
            case 0x48e160: return "RandomSeedSet";
            case 0x48e170: return "RandomNext";
            case 0x48e380: return "KeyToAscii";
            case 0x48f500: return "GameFileWrite";
            case 0x48de20: return "ArraySortWithComparator";
            case 0x431780: return "TerrainStampImpactHandler";
            case 0x431560: return "CurrentLevelGet";
            case 0x447ea0: return "EntityTypeBehaviorUpdate";
            case 0x447c40: return "SceneActivityUpdateDispatch";
            case 0x448c50: return "EntitySetActivityPlace";
            case 0x47b130: return "SoundBufferStopHandle";
            case 0x47b020: return "SoundBufferAssociationAdd";
            case 0x47b1a0: return "SoundInstanceTrackRegister";
            case 0x47b220: return "SoundInstanceTrackStopAndRemove";
            case 0x47b8e0: return "StreamedWaveBufferStepRate";
            case 0x4475c0: return "EntityProgressAndNegativeDeltaHandler";
            case 0x48dad0: return "ParseSignedDecimal";
            case 0x489280: return "CDAudioTrackCompletionPoll";
            case 0x488e70: return "WaveFileAverageBytesPerSecondRead";
            case 0x477440: return "BlockTextTokenSplit";
            case 0x478e60: return "BlockTextOpenAndScan";
            case 0x479490: return "BlockTextTokenPoolGrow";
            case 0x475930: return "EntityVisualKindDispatch";
            case 0x472710: return "EntityVisualTypeTablesInit";
            case 0x4800a0: return "ReadWholeFile";
            case 0x4800c0: return "ReadWholeFileWithProgress";
            case 0x478f50: return "ConfigEntryFind";
            case 0x479050: return "ConfigStringCopy";
            case 0x462e60: return "AnimatedTextureReplace";
            case 0x474710: return "TextureRelease";
            case 0x474a20: return "AnimatedTextureFrameCount";
            case 0x474da0: return "AnimatedTextureFrameSet";
            case 0x406ec0: return "EntityPayloadReleaseSubslots";
            case 0x46ce10: return "EntityPayloadType1Release";
            case 0x472800: return "DDiSceneObjectPoolShutdown";
            case 0x473340: return "SceneActionNamePresent";
            case 0x473370: return "SceneActionNameSet";
            case 0x475140: return "DDiSceneFrameUpdateAndCallbacks";
            case 0x464740: return "SoundAliasRegistryInit";
            case 0x465050: return "SoundSubmissionRouteWrapper";
            case 0x465b00: return "SoundInstanceUpdate";
            case 0x469e60: return "EntityStatsWordGet";
            case 0x472900: return "DDiSceneEndHookRegister";
            case 0x472a50: return "DDiSceneObjectDestroy";
            case 0x473b40: return "SceneRenderLockToggle";
            case 0x47a640: return "DirectSoundSystemInit";
            case 0x47a870: return "SoundBankEntryLoad";
            case 0x47b580: return "InMemoryWaveSampleLoad";
            case 0x47b6c0: return "SoundRecordCreateBuffers";
            case 0x47ba20: return "StreamPlaybackStopAndRelease";
            case 0x47d9d0: return "BitmapSurfaceLoad";
            case 0x47eef0: return "InputStatePoll";
            case 0x4777a0: return "WinMain";
            case 0x477bf0: return "CommandLineOptionParse";
            case 0x477f30: return "MainLoopFramePump";
            case 0x484070: return "FlicFrameAdvanceAndBlit";
            case 0x4876c0: return "SharedObjectDirectoryScan";
            case 0x487de0: return "SharedObjectTimedSoundEventUpdate";
            case 0x488c00: return "CDAudioWatchdogTick";
            case 0x489210: return "CDAudioTrackPlayFrom";
            case 0x48eb90: return "FileOpenAndReadWholeStream";
            case 0x4295f0: return "SoundMinimumDistanceUpdateMode";
            case 0x43bc40: return "ObjectiveSidecarAppendGate";
            case 0x437d00: return "EntityCreateByType";
            case 0x44b370: return "EntityActivityListAppend";
            case 0x452060: return "KeyStateGatedCommandEnqueue";
            case 0x462030: return "SaveMenuOpenFromReward";
            case 0x462780: return "RewardSaveButtonZoneCheck";
            case 0x472430: return "DirectShowInterfaceRelease";
            case 0x472560: return "DirectShowRenderSurfaceCreate";
            case 0x473690: return "EntityVisualResourceLoad";
            case 0x475710: return "DDiSceneObjectAttachOrUnlink";
            case 0x477150: return "DDiSceneDrawEnd";
            case 0x4781d0: return "Direct3DRMDeviceInit";
            case 0x47abc0: return "DirectSoundListenerSpatialUpdate";
            case 0x47b390: return "SoundSourceSpatialRefreshAll";
            case 0x47b410: return "SoundSourceSpatialRefresh";
            case 0x42e020: return "OListEntityRecordParse";
            case 0x4424f0: return "EntityPositionToTerrainCell";
            case 0x44f6d0: return "TerrainGridPick";
            case 0x4526c0: return "SelectionCandidateFilterDefault";
            case 0x45a2e0: return "OverlayImageRecordLoad";
            case 0x469e80: return "EntityStateFlagsGet";
            case 0x472cd0: return "EntityVisualLoadACT";
            case 0x472950: return "DDiSceneObjectAllocate";
            case 0x473da0: return "SceneObjectOwnerSet";
            case 0x4765c0: return "LwscSceneLoad";
            case 0x478040: return "Win32MessagePumpDispatch";
            case 0x478df0: return "BlockTextTokenPoolInit";
            case 0x478e20: return "BlockTextTokenPoolDestroy";
            case 0x4789c0: return "WindowsMixerControlInit";
            case 0x47dcb0: return "SurfaceBlitRect";
            case 0x47f010: return "CursorPositionUpdate";
            case 0x47fac0: return "GameFileClose";
            case 0x47ec90: return "AviPlayerCreate";
            case 0x488f10: return "WaveFileOpenReadChunks";
            case 0x48d980: return "ParseFloat";
            case 0x48db70: return "ParseIntegerWrapper";
            case 0x48db80: return "EngineHeapFree";
            case 0x459050: return "ObjectiveTimerEvaluate";
            case 0x461410: return "RewardScreenTick";
            case 0x465370: return "SoundDynamicAliasRegistrationEnabledGet";
            case 0x4721e0: return "DirectShowSampleSetup";
            case 0x4726f0: return "DirectShowWrapperDestroy";
            case 0x472920: return "DDiSceneSampleHookRegister";
            case 0x475390: return "DDiSceneObjectLengthGet";
            case 0x477010: return "DDiSceneDrawBegin";
            case 0x47c8d0: return "RenderSurfacePresent";
            case 0x47c610: return "DirectDrawRendererInit";
            case 0x47ed90: return "DirectInputInitialize";
            case 0x47ecf0: return "AviFrameUpdate";
            case 0x482d60: return "ModelRenderPassDispatch";
            case 0x483600: return "Direct3DViewportSetup";
            case 0x483f00: return "FlicPlayerClose";
            case 0x478980: return "MusicMixerVolumeSet";
            case 0x479270: return "BlockTextTokenAllocate";
            case 0x4809e0: return "LwoModelEntryLoad";
            case 0x458670: return "ObjectiveVoiceAliasPrepare";
            case 0x43fb40: return "DamageHurtSoundSubmit";
            case 0x41b2d0: return "CommandContextPostToHud";
            case 0x41b490: return "AdvisorOverlayUpdate";
            case 0x41c4f0: return "AdvisorAnimationPositionAndRequestUpdate";
            case 0x44b350: return "EntityActivityTypeSupportsCapacityRule";
            case 0x438770: return "BuildingUpgradeResourceAdmissionCheck";
            case 0x472a40: return "DDiSceneObjectDestroy";
            case 0x475120: return "DDiSceneAdvanceByTime";
            case 0x4784c0: return "GameWindowProcedure";
            case 0x47efb0: return "KeyboardDownKeyCount";
            case 0x4890c0: return "WaveDataChunkOpen";
            case 0x489100: return "WaveDataRead";
            case 0x452010: return "ActionCommandQueueAppend";
            case 0x456830: return "AdvisorInlineTokenRender";
            case 0x464ef0: return "SoundSubmitWithGlobalCooldown";
            case 0x42bb60: return "TextureSetConfigLoad";
            case 0x478010: return "WindowMessageDispatchFilter";
            case 0x47b4a0: return "SoundTrackedBuffersSetMinDistance";
            case 0x47b4d0: return "SoundTrackedBuffersSetMaxDistance";
            case 0x47cc50: return "WindowedRenderSurfaceBlit";
            case 0x47dbf0: return "ImageSurfaceBlitTransformed";
            case 0x47fa50: return "GameFileWrite";
            case 0x482d30: return "ModelRenderListClear";
            case 0x4652c0: return "SoundDeferredQueueDrain";
            case 0x430ee0: return "TerrainCellInitialize";
            case 0x434570: return "TerrainCellProgressUpdate";
            case 0x4409e0: return "EntityActivityRequestSet";
            case 0x441700: return "EntityRouteSetupAndMarkerAssociate";
            case 0x44f380: return "TerrainVoxelNeighborsMutate";
            case 0x4588e0: return "InputDeviceRelease";
            case 0x462390: return "RewardScreenTransitionTick";
            case 0x4754c0: return "DDiSceneMoveOrPose";
            case 0x4789a0: return "MusicMixerVolumeGet";
            case 0x487ba0: return "SharedObjectAnimationAndTimedEventUpdate";
            case 0x488c70: return "WaveFormatOpenAndRead";
            case 0x4897e0: return "DDiSceneTimedHookDispatch";
            case 0x459470: return "OsfRead";
            case 0x4744f0: return "TextureSurfaceCreate";
            case 0x477550: return "StringDuplicate";
            case 0x48f1d0: return "CharacterClassifyMask";
            case 0x47e760: return "AviSurfaceLock";
            case 0x42efc0: return "DirectShowVideoPlaybackLoop";
            case 0x438610: return "EntityCreateWithPosition";
            case 0x4389e0: return "EntityRouteMarkerCandidateFind";
            case 0x44a910: return "BuildingPlacementPreviewUpdate";
            case 0x4103f0: return "MenuWipeStart";
            case 0x473db0: return "SceneObjectOwnerGet";
            case 0x47ebf0: return "AviFrameSeekAndClockRebase";
            case 0x4065e0: return "RenderSurfaceSecondaryGet";
            case 0x410440: return "MenuWipeEnd";
            case 0x44a4b0: return "PredugCoordinatePoolAppend";
            case 0x44a200: return "EntityTypeComponentGet";
            case 0x44be30: return "EntityAuxiliaryPayloadRelease";
            case 0x43f6a0: return "EntityDamageApply";
            case 0x47aba0: return "DirectSoundListenerUpdateTick";
            case 0x4734c0: return "AdvisorCompanionTransformApply";
            case 0x4753f0: return "DDiSceneObjectFramePositionGet";
            case 0x475430: return "DDiSceneObjectPositionSet";
            case 0x476160: return "DDiSceneActionRecordField1CGet";
            case 0x476180: return "DDiSceneActionRecordField0CGet";
            case 0x476200: return "DDiSceneAnimationLastFrameGet";
            case 0x47e9a0: return "AviFrameRetrieverDestroy";
            case 0x483a90: return "ModelRenderChildMaterialPass";
            case 0x493ca0: return "LocaleCharacterMapString";
            case 0x47b530: return "SoundVolumeIndexSet";
            case 0x47e7b0: return "AviFrameSurfaceUnlock";
            case 0x47fed0: return "GameFileHandleAllocate";
            case 0x483420: return "ScreenEffectItemOpacitySet";
            case 0x43a300: return "EntityDepositActivityGate";
            case 0x48ead0: return "EngineCalloc";
            case 0x42e8b0: return "ObjectListEntityTypeResolve";
            case 0x47b0f0: return "SoundHandleToBuffer";
            case 0x43fe70: return "EntityRouteToExplicitTarget";
            case 0x4373d0: return "EntityActivityToggle";
            case 0x469ea0: return "EntityCapabilityFlagsGet";
            case 0x4907f0: return "ParseEntityHealthValue";
            case 0x4624a0: return "RewardFlhContextTick";
            case 0x462460: return "RewardFlhContextReload";
            case 0x418040: return "CampaignChainSnapshotCreate";
            case 0x47c100: return "DirectSoundListenerRolloffSet";
            case 0x478940: return "D3DMaterialStateFlush";
            case 0x483a70: return "ModelChildRenderEligibilityCheck";
            case 0x4838b0: return "D3DRenderStateSync";
            case 0x465260: return "SoundSubsystemEnableSet";
            case 0x4652a0: return "SoundSubsystemEnableSetAndStop";
            case 0x47b160: return "SoundSubsystemStopAll";
            case 0x44c4a0: return "SelectionClearMarkerAndResetHud";
            case 0x41e750: return "SelectionClearRequestAndHudReset";
            case 0x452690: return "SelectionCandidateAlreadySelectedOrMarked";
            case 0x4786c0: return "MainWindowActivationAndCloseHandler";
            case 0x431580: return "TerrainNeighborWorkFlagScan";
            case 0x430df0: return "TerrainNeighborRefresh";
            case 0x47cee0: return "DirectDrawClipperSetRegion";
            case 0x4830c0: return "AnimatedTextureFrameSurfaceSet";
            case 0x47a850: return "SoundVolumeValueValid";
            case 0x4105a0: return "CampaignLevelFallbackLabel";
            case 0x429810: return "AudioPlaybackToggle";
            case 0x4592a0: return "OsfReadStateReset";
            case 0x47b500: return "SoundBufferVolumeSet";
            case 0x47e980: return "AviPlayerDestroy";
            case 0x4726d0: return "DirectShowSampleUpdateAndBlit";
            case 0x474c00: return "AnimatedTextureFrameResolve";
            case 0x487ed0: return "SharedObjectSampleTimeWindowMatch";
            case 0x402f70: return "EntityTargetCommandQueue";
            case 0x402f30: return "EntityThresholdProgressEventQueue";
            case 0x404fc0: return "PlaceCommandEligibilityCheck";
            case 0x40e9c0: return "LavaErosionDelayFromCell";
            case 0x419420: return "IncidentMessageTextCatalogGet";
            case 0x41a300: return "InterfaceMenuActionLabelsInit";
            case 0x432600: return "TerrainCellVariantFlagPropagate";
            case 0x432bd0: return "TerrainCellWalkableFlagCheck";
            case 0x435f16: return "CursorScreenRayContextReset";
            case 0x436c20: return "ActivityNameTableInit";
            case 0x437c20: return "DeferredOListEntityQueueAdd";
            case 0x4385b0: return "EntitySharedEligibilityCheck";
            case 0x4387f0: return "BuildingUpgradeCommit";
            case 0x43abf0: return "TerrainCellVariantUpdateQueue";
            case 0x43ad50: return "PredugSpawnCoordinateSelectAndCreate";
            case 0x43bb20: return "BuildingDepositStateMonitor";
            case 0x43f100: return "LinkedEntityRouteFlagClear";
            case 0x43f5b0: return "EntityLinkedActivityEventDispatch";
            case 0x43fff0: return "EntityRouteMarkerSetup";
            case 0x4428a0: return "EntityWorldPositionSet";
            case 0x44e6d0: return "TerrainCellMaterialTransitionBegin";
            case 0x44f230: return "TerrainCellMaterialSet";
            case 0x456810: return "SoundAliasPlayGlobal";
            case 0x462700: return "PointerOverrideExpiryTick";
            case 0x465190: return "SoundFrameTick";
            case 0x4652b0: return "SoundPackedBatchQueueUpdate";
            case 0x465430: return "SoundStreamSelectorUpdate";
            case 0x4659b0: return "LavaEffectSoundHandleGetOrSubmit";
            case 0x469bb0: return "BuildingDepositCountGet";
            case 0x469f20: return "ScreenShakeFlagRaise";
            case 0x46da50: return "EntityActivityChangeDispatch";
            case 0x472530: return "VideoDurationFrameBoundUpdate";
            case 0x4726c0: return "VideoFrameBoundRefresh";
            case 0x474d30: return "TerrainCellQuadVertexEmit";
            case 0x4759f0: return "DDiSceneObjectArrayGrow";
            case 0x475c00: return "DDiSceneAnimationCallbacksSet";
            case 0x475f70: return "DDiSceneAnimationHookCreate";
            case 0x4761a0: return "DDiSceneAnimationDurationGet";
            case 0x4761e0: return "DDiSceneAnimationStateGet";
            case 0x476770: return "DDiSceneResourceNameResolve";
            case 0x4767e0: return "DDiSceneResourceExists";
            case 0x476850: return "DDiSceneResourceOpen";
            case 0x477290: return "CameraScreenCoordinateProject";
            case 0x4798e0: return "ViewRayIntersectionTest";
            case 0x47ab80: return "DirectSoundListenerViewSet";
            case 0x47ad10: return "SoundPositionSetFromVector";
            case 0x47ad30: return "AttachedSoundStopByEntity";
            case 0x47b790: return "WaveSampleDuplicateBufferCreate";
            case 0x47bc30: return "StreamedWavePlaybackDrainCheck";
            case 0x47c0c0: return "Vector3ExactEqual";
            case 0x47cc20: return "RenderSurfaceRestoreFromPrimary";
            case 0x47ed40: return "AviFrameIndexAndClockReset";
            case 0x47ed50: return "AviCurrentFrameIndexGet";
            case 0x47ed60: return "AviStreamFrameCountGet";
            case 0x47ed80: return "AviPlayerReadyGet";
            case 0x47fea0: return "WadEntryReadIntoStreamSlot";
            case 0x4823e0: return "TerrainCellQuadVerticesWrite";
            case 0x482ac0: return "ModelRenderRecordStateSetup";
            case 0x483940: return "Direct3DRenderStateRestore";
            case 0x4839c0: return "ModelRenderChildPassDispatch";
            case 0x485300: return "ObjectiveAdvisorTextCopy";
            case 0x489330: return "CDAudioDeviceOpen";
            case 0x48be40: return "WadEntryMemoryStreamOpen";
            case 0x48e1e0: return "FormattedTextBufferBuild";
            case 0x4121b0: return "MenuOverlayFlicPlaybackController";
            case 0x418830: return "BuildingUpgradeStateChangedEventDispatch";
            case 0x4288e0: return "LevelDebugInputCellMutatorDispatch";
            case 0x42c330: return "EmergeMapTriggerSpawn";
            case 0x431da0: return "TerrainCellRouteAndOccupancyCheck";
            case 0x432550: return "AlwaysAcceptPlaceCommandCell";
            case 0x434e80: return "InterfaceSpawnRequestEnqueue";
            case 0x437940: return "DeferredOListEntityCreateAndLink";
            case 0x439d50: return "VehicleUnloadAdjacentCellFind";
            case 0x439e10: return "VehicleUnloadCellFindIfEligible";
            case 0x43f150: return "EntitySpawnRequestQueueAdd";
            case 0x43f190: return "EntitySpawnRequestQueueProcess";
            case 0x44dec0: return "FileLoadingProgressUpdate";
            case 0x44f700: return "WorldToTerrainCellBoundsCheck";
            case 0x452870: return "SelectionCandidateEligibilityCheck";
            case 0x452df0: return "TutorialNerpScriptFileLoad";
            case 0x456cb0: return "CoordinateListCallbackVisit";
            case 0x459560: return "HealthBarConfigLoadAndShadeDerive";
            case 0x45a590: return "ScrollingAdvisorTextAppend";
            case 0x46ffb0: return "LaserSpatialSoundAndImpact";
            case 0x4757a0: return "DDiSceneContextGet";
            case 0x475e40: return "DDiSceneFrameRecordFind";
            case 0x477240: return "DDiSceneCurrentObjectSet";
            case 0x477670: return "GameStringHashCompute";
            case 0x43a5f0: return "EntityVisualResourceGetOrCreate";
            case 0x43f7d0: return "EntityEnergyLowTargetRouteCommandTick";
            case 0x447b30: return "EntityActivityTick";
            case 0x4502e0: return "ScreenRayTerrainCellPick";
            case 0x412170: return "MenuOverlayListCount";
            case 0x413970: return "MenuScrollStateUpdate";
            case 0x413a60: return "SaveMenuThumbnailConfigLoad";
            case 0x41fa20: return "GameConfigCallbackRegistryInit";
            case 0x425b40: return "PeriodicCallbackDispatch";
            case 0x4297b0: return "CDAudioWatchdogModeUpdate";
            case 0x4319b0: return "TerrainCellMaterialGet";
            case 0x4319e0: return "MapFileCellGet";
            case 0x4321a0: return "TerrainCellUpdateQueueFlush";
            case 0x432200: return "TerrainCellPreviousStateTrack";
            case 0x4334c0: return "MapFileDimensionsGet";
            case 0x4377d0: return "EntityPoolFindByCallback";
            case 0x4383b0: return "RoverWorkQueueUpdate";
            case 0x439370: return "EntityBuildPlacementCommandSetup";
            case 0x439990: return "EntityActivitySceneStateUpdate";
            case 0x43a960: return "EntityLavaCrustActivityRegister";
            case 0x43aea0: return "EmergenceRandomSpawnAttempt";
            case 0x43b270: return "EntityFrameUpdateDispatch";
            case 0x44a2a0: return "EntityWorkObjectPositionGet";
            case 0x44b480: return "EntityPermittedActivityCheck";
            case 0x44c530: return "EntityCanDoOrRideCheck";
            case 0x44efb0: return "TerrainCellChangedFlagSet";
            case 0x44bf00: return "GlobalTimedCallbackDispatch";
            case 0x4528c0: return "SelectionStateTick";
            case 0x452c50: return "SelectionEntityRemove";
            case 0x452cc0: return "SelectedEntityStateMonitor";
            case 0x452d60: return "SelectionCommandDispatch";
            case 0x459360: return "OsfSnapshotTableBuild";
            case 0x4593d0: return "OsfReadDirtyFlagGet";
            case 0x460200: return "SelectedLevelStartObjectUpdate";
            case 0x469cc0: return "TerrainTypeMovementSpeedGet";
            case 0x475d10: return "DDiSceneActFrameRecordParse";
            case 0x477710: return "ConfigBooleanParse";
            case 0x477fd0: return "EngineCallbackTripletSet";
            case 0x478ff0: return "ConfigNestedPathChildFind";
            case 0x4790b0: return "ConfigPathValueGet";
            case 0x4792e0: return "ConfigBlockPathFind";
            case 0x479ee0: return "FontResourceLoad";
            case 0x47a180: return "RewardTextDraw";
            case 0x47aad0: return "SoundBankFreeSlotFind";
            case 0x47a550: return "FontRecordAndSurfaceRelease";
            case 0x47c8f0: return "BitmapFileWrite";
            case 0x47d470: return "ImageSurfaceRelease";
            case 0x47db90: return "RGBChannelsPack";
            case 0x47e050: return "FontRecordCreate";
            case 0x47e190: return "FontDeviceColorConvert";
            case 0x47e2d0: return "RGBChannelsUnpack";
            case 0x47e440: return "BitmapSurfaceSave";
            case 0x47f660: return "SoundPathFallbackFormat";
            case 0x47fb60: return "SoundSampleFileOpenByName";
            case 0x47fca0: return "GameFileLengthGet";
            case 0x483c80: return "FlicFileOpenAndStateCreate";
            case 0x488b50: return "CDAudioDeviceStateUpdate";
            case 0x489260: return "CDAudioErrorTextGet";
            case 0x489410: return "MapFileResourceSlotAllocate";
            case 0x4894a0: return "MapFileResourceSlotGet";
            case 0x489c30: return "IndexedBitmapPaletteDataPrepare";
            case 0x48d9e0: return "FormattedStringOutput";
            case 0x403270: return "SelectionDragAreaCommandCreate";
            case 0x4112e0: return "MenuPointerRectangleHitTest";
            case 0x411360: return "MenuMouseCoordinateUpdate";
            case 0x411870: return "MenuSliderPointerHandler";
            case 0x45c670: return "AdvisorAnimationSceneLookupAndRequest";
            case 0x45c6b0: return "AdvisorAnimationSlotIdResolve";
            case 0x418110: return "SaveStateEventContextSet";
            case 0x43c440: return "EntityCanDoActivityClassify";
            case 0x43c810: return "EntityFlagWordBehaviorEvaluate";
            case 0x4470d0: return "EntityConditionalTypeFiveSpawn";
            case 0x44c110: return "AttachedEntityWake";
            case 0x4641c0: return "BuildingPlacementFootprintRenderAndValidate";
            case 0x4755e0: return "CameraTeleportOrOffset";
            case 0x48dbd0: return "EngineHeapAlloc";
            case 0x48f2b0: return "StreamedAudioFileLengthGet";
            case 0x460360: return "CheckpointBlobTaskProgressRefresh";
            case 0x460950: return "CheckpointBlobTaskProgressRecalculate";
            case 0x460f00: return "RewardStatsPanelDraw";
            case 0x461070: return "RewardStatsTextFormat";
            case 0x45dbc0: return "TerrainViewTileGeometryBuild";
            case 0x45e9e0: return "TerrainTileStyleResolve";
            case 0x45e940: return "ViewPointToScreenGeometryTransform";
            case 0x46d380: return "AttachedModelOrientationUpdate";
            case 0x405c10: return "PendingCommandEntityAssignmentTick";
            case 0x41baa0: return "SimulationPointerActionDispatch";
            case 0x407d60: return "BuildingTypeConfigRecordParse";
            case 0x433e80: return "TerrainProjectedCellStateRefresh";
            case 0x470760: return "MeshBuildEightSidedSegment";
            case 0x493ef0: return "CrtFormattedScanV";
            case 0x480170: return "MapOpen2";
            case 0x488070: return "SharedObjectFloatTrackInterpolateApply";
            case 0x401c00: return "StartLevelOverrideIfEnabledGet";
            case 0x475300: return "DDiSceneApplyActFrameTiming";
            case 0x461c90: return "RewardScoreTaskTextDraw";
            case 0x462600: return "RewardTaskTextDrawAndScoreGate";
            case 0x48e060: return "X87FloatToLongLongRound";
            case 0x439280: return "EntityBuildPlacementCommandBatchExpand";
            case 0x439340: return "EntityBuildPlacementCommandPairCreate";
            case 0x4394b0: return "EntityBuildPlacementAlternateCommandSetup";
            case 0x447210: return "EntityAdjacentPlacementCellSearch";
            case 0x452430: return "EntityRandomValidCellCommandCreate";
            case 0x475470: return "DDiSceneObjectPoseMethodDispatch";
            case 0x4790d0: return "ConfigBooleanGetDefault2";
            case 0x483f60: return "FlicFileHeaderValidate";
            case 0x4841d0: return "FlicRamFrameAdvance";
            case 0x484260: return "FlicStreamFrameAdvance";
            case 0x484320: return "FlicChunkWalk";
            case 0x4844b0: return "FlicFrameRecordWalk";
            case 0x484610: return "FlicSubrecordOpcodeDispatch";
            case 0x484720: return "FlicChunkBufferAndContinueLK";
            case 0x4847d0: return "FlicChunkBufferAndContinueType0B";
            case 0x484880: return "FlicOpcode16ModeDispatch";
            case 0x4848d0: return "FlicDecode8BitOpcode25Mode4";
            case 0x4849d0: return "FlicDecode16BitKeyframe";
            case 0x484b20: return "FlicOpcode25DecodeDispatch";
            case 0x484b90: return "FlicOpcode25SuccessStub";
            case 0x484c00: return "FlicDecode16BitRowRecords";
            case 0x484c90: return "FlicOpcode27FallbackNoop";
            case 0x484ca0: return "FlicDecode8BitAlternateDelta";
            case 0x484e50: return "FlicDecode16BitDelta";
            case 0x485030: return "FlicOpcode27DecodeDispatch";
            case 0x489460: return "MapFileResourceSlotRelease";
            case 0x47f130: return "WadMountStartup";
            case 0x47fe30: return "FileOpenRouteSelect";
            case 0x48bd30: return "WadEntrySlotAllocate";
            case 0x48bda0: return "WadEntryNameLookup";
            case 0x48bf70: return "WadPathOpen";
            case 0x4783f0: return "WindowCreate";
            case 0x4250a0: return "ResetLevelAndExit";
            case 0x41fb40: return "TimeScaleGet";
            case 0x419b20: return "InfoMessageQueueFindId";
            case 0x419f30: return "InfoMessageQueueTick";
            case 0x41a2c0: return "InfoMessageTimerDecay";
            case 0x45d870: return "CommandEntityReferenceRemap";
            case 0x40a400: return "PreloadNumberedBitmaps";
            case 0x40b310: return "LoadTileVariants";
            case 0x42b330: return "LoadTileMeshes";
            case 0x4653a0: return "PreloadSharedIndexed";
            case 0x474050: return "CreateBmpSurface";
            case 0x474410: return "ParseIndexedSurfaceName";
            case 0x477bd0: return "SetLowTextureFlag";
            case 0x47cd50: return "QueryTextureMem";
            case 0x480ac0: return "MaterialPass";
            case 0x481820: return "BindModelTextures";
            case 0x481c50: return "ParseUV";
            case 0x482e70: return "TextureGetOrCreate";
            case 0x483030: return "TextureCacheLookup";
            case 0x483080: return "TextureCacheInsert";
            case 0x4869f0: return "LoadLWSCScene";
            case 0x4885c0: return "ModelResourceResolveOrLoad";
            case 0x48c0c0: return "LwoStringDuplicate";
            case 0x48c1d0: return "LwoReadBigEndianFloat";
            case 0x48c210: return "LwoPoints";
            case 0x48c360: return "LwoCurves";
            case 0x48c3e0: return "LwoPolygons";
            case 0x48c690: return "LwoSurfaceNames";
            case 0x48c820: return "LwoSurfaceSettings";
            case 0x48d2c0: return "LoadLWO";
            case 0x490090: return "MaterialResolve";
            case 0x419470: return "InfoMessageAlwaysPlayFlagSet";
            case 0x419650: return "InfoMessageSoundLoad";
            case 0x419680: return "InfoMessageSecondarySoundSet";
            case 0x457d40: return "ObjectiveConfigLoad";
            case 0x458580: return "ObjectiveCrystalOrOreArm";
            case 0x4585c0: return "ObjectiveBlockArm";
            case 0x4585f0: return "ObjectiveTimerArm";
            case 0x458620: return "ObjectiveConstructionArm";
            case 0x479500: return "D3DRMVectorRandom";
            case 0x47dbc0: return "ResourceRecordAddRef";
            case 0x475900: return "DDiSceneObjectArgumentsNormalize";
            case 0x40b5f0: return "TileModelEntityRegister";
            case 0x4728c0: return "SharedTextureDirectorySet";
            case 0x4774b0: return "WhitespaceTokenize";
            case 0x480050: return "ReadCrlfLine";
            case 0x4805b0: return "SharedTextureRootRegister";
            case 0x480650: return "SharedTextureRootInitialize";
            case 0x48d7c0: return "ModelObjectCreate";
            case 0x48d870: return "ModelObjectFree";
            case 0x48dc80: return "GrowHeapArray";
            case 0x48eae0: return "FileResourceHandleClose";
            case 0x48f460: return "FileResourceHandleSeek";
            case 0x49d030: return "DirectDrawDeviceInitialize";
            case 0x47ec80: return "AviFileInitialize";
            case 0x47efd0: return "AviFileShutdown";
            case 0x47ccf0: return "DirectDrawObjectsRelease";
            case 0x485e80: return "RenderSetup";
            case 0x4893f0: return "ResourceStateTableReset";
            case 0x48b260: return "StartupStreamStateReset";
            case 0x48b290: return "StartupStreamsClose";
            case 0x48b2f0: return "StartupConfigStreamLoad";
            case 0x48da50: return "StringFindSubstring";
            case 0x477fa0: return "RendererBeginFrameOnce";
            case 0x4783d0: return "WindowTitleSet";
            case 0x49c7c0: return "ExpandWadPayload";
            case 0x49c8e0: return "DecodeWadPayloadType1";
            case 0x49ca60: return "DecodeWadPayloadType2";
            case 0x48bd50: return "WadEntryFindAcrossMountedArchives";
            case 0x494e30: return "StdFileOpen";
            case 0x495000: return "StdFileHandleAcquire";
            case 0x48eb60: return "StdFileOpenIntoHandle";
            case 0x47f4f0: return "FindCdKeyDrive";
            case 0x47f640: return "MountWadFromPath";
            case 0x49dcd0: return "StringToUpper";
            case 0x45d6d0: return "PositionTrackerUpdateAll";
            case 0x41b930: return "TimedStatusAndEventTick";
            case 0x424600: return "TerrainCellFlagCleanup";
            case 0x44eff0: return "TerrainDirtyCellUpdate";
            case 0x4196a0: return "MessageSubscriberDispatch";
            case 0x456210: return "FlhMagicUuNoop";
            case 0x459100: return "ObjectiveEntityPairedFieldPredicate";
            case 0x49c840: return "WadPayloadDecodeDispatch";
            case 0x49cc70: return "WadBitstreamOutputInitialize";
            case 0x49ccc0: return "WadReadBitsLsbFirst";
            case 0x49cd30: return "WadReadBitsMsbFirst";
            case 0x49cd90: return "WadHuffmanTableRead";
            case 0x49ce00: return "WadHuffmanSymbolDecode";
            case 0x49ce70: return "WadRunLengthDecode";
            case 0x49ceb0: return "WadBackreferenceOffsetDecode";
            case 0x49cf50: return "WadHuffmanTableClear";
            case 0x49cf90: return "WadHuffmanCanonicalCodesBuild";
            case 0x49d000: return "WadReverseLowBits";
            case 0x440c70: return "TerrainGridPathSearch";
            case 0x44f640: return "MapCellToWorldCenter";
            case 0x44f940: return "TerrainSurfaceHeightInterpolate";
            case 0x4574e0: return "ObjectiveTextOverlayLoad";
            case 0x46c6f0: return "VehicleTypeDefinitionParse";
            case 0x46eb80: return "WeaponTypeConfigLoad";
            case 0x489dd0: return "DirectXResultDiagnosticDispatch";
            case 0x402170: return "LevelNoGatherObjectSetup";
            case 0x40bf70: return "RockFallStyleNameLookup";
            case 0x40e930: return "ErosionTimingStateInit";
            case 0x40ce80: return "MapCellStateBufferRebuild";
            case 0x4191a0: return "MainSimStateAdmissionPredicate";
            case 0x41c440: return "AdvisorCallbackScheduleGate";
            case 0x425ce0: return "ScrollScaleSetAndClamp";
            case 0x4262e0: return "ObjectiveScoreTextOriginSet";
            case 0x426320: return "ObjectiveTaskTextOriginSet";
            case 0x428800: return "ActionFeedbackSoundDispatch";
            case 0x422850: return "MiscObjectResourcesLoad";
            case 0x42d100: return "RockMonsterTypesLoad";
            case 0x42d2b0: return "BuildingTypesLoad";
            case 0x42d460: return "UpgradeTypesLoad";
            case 0x42d600: return "ObjectNamesLoad";
            case 0x42da20: return "ObjectTheNamesLoad";
            case 0x431570: return "CurrentLevelHeightGet";
            case 0x44f4e0: return "TerrainCellSurfaceTypeSet";
            case 0x44f540: return "TerrainCellSurfaceTypeGet";
            case 0x44f570: return "TerrainCellSurfaceTypeReset";
            case 0x44f5c0: return "TerrainCellSurfaceAppearanceApply";
            case 0x44f870: return "TerrainWorldPointToCell";
            case 0x44fb90: return "TerrainSurfacePointProjectOffset";
            case 0x4500d0: return "TerrainCellSurfaceVerticesGet";
            case 0x480220: return "ResourcePathResolveAgainstBase";
            case 0x4850f0: return "TextBufferCreate";
            case 0x485190: return "TextBufferDestroy";
            case 0x485390: return "TextBufferDrawWrapped";
            case 0x479170: return "ConfigRgbVectorRead";
            case 0x483270: return "RenderObjectColorVectorSet";
            case 0x477610: return "CaseInsensitiveSubstringFind";
            case 0x435a50: return "MainSimFeatureFlagLatch";
            case 0x42c440: return "LevelRecordTimerCountdown";
            case 0x42cb70: return "LavaErosionCellTimersAdvance";
            case 0x42cf50: return "MiniFigureTypeTableLoad";
            case 0x42de40: return "ViewportTargetPositionClamp";
            case 0x42c5b0: return "TerrainCellByteStateClassify";
            case 0x428020: return "EntityCommandStatusFeedbackDispatch";
            case 0x428330: return "TerrainCommandFeedbackDispatch";
            case 0x44e880: return "TerrainGridCellsInitialize";
            case 0x44edf0: return "TerrainCellOrientationToggleAndRebuild";
            case 0x4514f0: return "TerrainCellSurfaceVectorsRebuild";
            case 0x46a4c0: return "FlagMaskCallbacksAllPass";
            case 0x431a30: return "SelectedEntitiesAnyPredicateMatch";
            case 0x48f650: return "GameFileDelete";
            case 0x48f6d0: return "GameDirectoryCreate";
            case 0x48f700: return "GameFileMetadataFind";
            case 0x48c120: return "LwoTextureProjectionModeParse";
            case 0x489d90: return "DirectXErrorMessageFormat";
            case 0x48dbf0: return "EngineHeapAllocateWithRetry";
            case 0x48dc40: return "EngineHeapAllocateAligned";
            case 0x491900: return "EnginePoolAllocate";
            case 0x4909d0: return "EngineFormatStringToStream";
            case 0x4377b0: return "EntityPoolFindByCallbackFiltered";
            case 0x442260: return "EntityTransformPositionGet";
            case 0x4291a0: return "SceneMarkerUpdateAndDraw";
            case 0x425d90: return "TerrainPointerCommandUpdate";
            case 0x4230c0: return "DebugPanelTextDrawUpdate";
            case 0x42b850: return "SurfaceTextCharacterMapInitialize";
            case 0x42f2e0: return "TerrainCellEdgeFlagSetAndUpdate";
            case 0x46b700: return "TimedEffectCueRequestSet";
            case 0x46b730: return "TimedEffectPersistenceSet";
            case 0x46b7a0: return "TimedEffectTimestampReset";
            case 0x46b8b0: return "RectangularGridOverlayGeometryDraw";
            case 0x449f50: return "EntityTypeOperationDispatch";
            case 0x449ff0: return "EntityTypeOperationRelease";
            case 0x48b4e0: return "WwadArchiveIndexLoad";
            case 0x483b70: return "Direct3DDeviceStateApply";
            case 0x480900: return "CachedRenderResourceInstanceCreate";
            case 0x477590: return "FormattedTextEscapeDecodeAndEmit";
            case 0x47d800: return "Bitmap24BitSurfaceConvert";
            case 0x4075a0: return "TickEntityScreenEffects";
            case 0x410740: return "SaveMenuListEntryAppend";
            case 0x411530: return "MenuWidgetItemScreenRect";
            case 0x411790: return "MenuPickWidgetUnderPointer";
            case 0x433770: return "SceneLightsBuildFromConfig";
            case 0x451cd0: return "MessageStringTableInitialize";
            case 0x459b00: return "RenderEntityScreenRing";
            case 0x45adb0: return "SimCursorInteractionTick";
            case 0x45b310: return "SimCategoryCellHitTest";
            case 0x4638a0: return "SimTrackCursorHitTest";
            case 0x46b1d0: return "TooltipOverlayStateInitialize";
            case 0x46bc30: return "TimedEffectSlotRender";
            case 0x4734f0: return "DDiSceneLightCreate";
            case 0x4083a0: return "SceneEntitySlotTableRebuild";
            case 0x4181b0: return "HelpWindowResourcesInit";
            case 0x41ee80: return "CellGridPointHitTest";
            case 0x4355c0: return "MenuOptionPanelTextureUpdate";
            case 0x43eea0: return "SimulationEntityQueueFlush";
            case 0x4403d0: return "LevelDebugCellRetargetDispatch";
            case 0x4515e0: return "TerrainCellRayHitTest";
            case 0x45b590: return "RectContainsOffsetPoint";
            case 0x462ad0: return "RewardScrollTickerAdvanceAndDraw";
            case 0x462f20: return "TerrainCellQuadGeometryEmit";
            case 0x46c160: return "UpgradeLevelTableParse";
            case 0x46c510: return "SceneChannelSlotsRebuild";
            case 0x46db90: return "CellQuadRunStage";
            case 0x46de80: return "CellQuadRunEmit";
            case 0x405950: return "PendingCommandTimerFireScan";
            case 0x407a10: return "PendingTypeValueSlotSelect";
            case 0x40ab70: return "DependencyGridConfigLoad";
            case 0x40d850: return "TerrainCellNeighborFloodWalk";
            case 0x442b10: return "EntityMovementAccumulatorApply";
            case 0x442f80: return "EntityPoseOrientationDispatch";
            case 0x4438b0: return "PathStepAdvanceAndArrivalCheck";
            case 0x444680: return "StepMovementVectorResolve";
            case 0x4469c0: return "EntityNearestConnectedTerrainCellRouteSelect";
            case 0x44ead0: return "TerrainCellListSurfaceHeightClampMutate";
            case 0x463230: return "TerrainDirtyCellQuadNormalEmitPass";
            case 0x463590: return "TerrainCellSurfaceNormalGet";
            case 0x406990: return "TypeTableResourceRowLoad";
            case 0x40e490: return "EncyclopediaConfigFilesOpen";
            case 0x434710: return "PanelButtonLayoutConfigLoad";
            case 0x446610: return "EntityMovementTickScaleAndDeltaGet";
            case 0x45a030: return "UiPanelNameTablesInit";
            case 0x45b9d0: return "PanelRotationControlConfigLoad";
            case 0x47d000: return "PalettizedBitmapSurfaceDecode";
            case 0x485a20: return "KeyboardKeyNamesInit";
            case 0x4877e0: return "SceneEntrySlotTableExpand";
            case 0x497130: return "Win32FileDescriptorRead";
            case 0x498350: return "NumberStringScanParse";
            case 0x499240: return "Win32FileDescriptorOpen";
            case 0x408ec0: return "KindNineNeighborEmitForFootprintCells";
            case 0x418450: return "HelpWindowInfoConfigStringsLoad";
            case 0x41ada0: return "InterfaceImagesConfigLoad";
            case 0x4425f0: return "SubobjectPoseDeltaNormalizeAndDispatch";
            case 0x44db50: return "KeyValueEqualsFileLoad";
            case 0x463f00: return "SceneQuadGridEmit";
            case 0x46e610: return "TerrainEdgeFlagMidpointUpdate";
            case 0x475a70: return "SceneActionRecordActFrameScan";
            case 0x47a520: return "BitmapFontGlyphMeasureWidth";
            case 0x481b80: return "CounterFormatStringBuild";
            case 0x485710: return "TextWrapMeasureAndDrawLines";
            case 0x491b40: return "EnginePoolChunkTryAllocate";
            case 0x496cc0: return "Win32FileDescriptorWrite";
            case 0x409600: return "TerrainDescriptorTilesResetAndFree";
            case 0x411f10: return "PercentSplitSurfaceBlit";
            case 0x412750: return "MenuHoverStatusTextDraw";
            case 0x41ab00: return "InterfaceBuildImagesConfigLoad";
            case 0x44c1b0: return "EntityHeadingStepMove";
            case 0x44c9c0: return "LightEffectsFadeConfigLoad";
            case 0x44cca0: return "LightEffectsMoveConfigLoad";
            case 0x44e8c0: return "SurfaceVertexSpanValidate";
            case 0x45d110: return "RegionContainmentScanUpdate";
            case 0x4702b0: return "EntitySurfaceQuadSegmentIntersect";
            case 0x476bf0: return "SurfacePixelRowsFlipInPlace";
            case 0x479630: return "SurfaceTriangleAreaCalculate";
            case 0x47d520: return "Palette8To16BitSurfaceExpand";
            case 0x4124f0: return "MenuSaveImageConfigRectBlit";
            case 0x41b680: return "CatalogCallbackByIdDispatch";
            case 0x433c10: return "EntityNeighborBlockedCellStepProbe";
            case 0x441ed0: return "EntityFaceTargetActionStart";
            case 0x446e40: return "EntityNearestCellRouteCommit";
            case 0x448a60: return "EntityNeighborCellMoveCommandHandle";
            case 0x44bc30: return "EntityMotionToSceneNodeSync";
            case 0x450960: return "CellDirtyQueuePushAndClear";
            case 0x465f40: return "InstanceQuadVerticesBuild";
            case 0x471a60: return "EntityVelocitySteerBlendUpdate";
            case 0x47d740: return "SurfacePixelRowsFlipWithScratch";
            case 0x487a00: return "SharedObjectClone";
            case 0x4899f0: return "ObjectHierarchyRecursiveClone";
            case 0x40b470: return "ProjectedQuadScreenAreaCalculate";
            case 0x40c990: return "MiscObjectsEffectSlotBind";
            case 0x41afd0: return "InterfaceSurroundImagesConfigLoad";
            case 0x4334f0: return "RenderQualityConfigApply";
            case 0x440a10: return "AttachedEntityDirectionVectorUpdate";
            case 0x44a5d0: return "NearestOpenCellCoordSearch";
            case 0x44dde0: return "StringTableEntrySetCurrent";
            case 0x453da0: return "CameraFocusTweenUpdate";
            case 0x45bcd0: return "CameraDragFromCursorApply";
            case 0x45d8b0: return "ScreenCoordNormalizeInit";
            case 0x46faf0: return "EntitySpawnRecordSlotInit";
            case 0x471fc0: return "EntityVelocitySlotDispatch";
            case 0x481ac0: return "StringTrailingDigitsSplit";
            case 0x4915a0: return "EnginePoolArenaCreate";
            case 0x401530: return "AdvisorPositionConfigLoad";
            case 0x4070b0: return "BubbleActionStringsInit";
            case 0x409d50: return "EntityCellPathEffectsApply";
            case 0x41aa00: return "EntityTableMapsAllocate";
            case 0x4345f0: return "PanelsConfigEntryApply";
            case 0x434af0: return "TextMessagesConfigLoad";
            case 0x437150: return "ToolTipIconsConfigLoad";
            case 0x444260: return "EntityRandomCellStepRouteIssue";
            case 0x447950: return "EntityDirectionalActivityStart";
            case 0x46a7f0: return "UiMessageStringsInit";
            case 0x46fc70: return "ObjectFlightCurveRecordInit";
            case 0x47e510: return "DirectDrawDisplayModeSetup";
            case 0x482500: return "InstanceQuadVertexDataFill";
            case 0x4882e0: return "SceneNodeTransformMatrixApply";
            case 0x40bfd0: return "RockFallStylesConfigLoad";
            case 0x40c3a0: return "ResourceAvailabilitySweep";
            case 0x40dc20: return "CellPairEdgeEffectApply";
            case 0x409790: return "CellPathRecordRegisterApply";
            case 0x4185f0: return "HelpWindowButtonLayoutLoad";
            case 0x411100: return "MenuSectionRecordCreate";
            case 0x41eca0: return "AdvisorDialogStackLayout";
            case 0x431b20: return "PlacementBlockedAdjacentCellScan";
            case 0x431c70: return "FirstFreeNeighborCellPick";
            case 0x401270: return "AdvisorConfigSlotLookup";
            case 0x40e1e0: return "CellCornerOccupancyScan";
            case 0x418da0: return "HelpWindowButtonHitTest";
            case 0x41eac0: return "EventTimerCountdownAdvance";
            case 0x427e00: return "ToolTipInfoStringsLoad";
            case 0x433180: return "CellTypeSpeedMultiplierResolve";
            case 0x43a650: return "ActionKindEntityCreateAndCommandIssue";
            case 0x442300: return "EntityVelocityVectorDispatch";
            case 0x4477d0: return "PathStepLookaheadEntryWrite";
            case 0x44b680: return "EntitySceneObjectTreeAttach";
            case 0x4519b0: return "VisualModelRecordCreate";
            case 0x45a9c0: return "PanelTextureSlotsBlit";
            case 0x45cdc0: return "PriorityImagesConfigLoad";
            case 0x46cff0: return "SceneRecordTemplateInstantiate";
            case 0x46dd10: return "StagedCellCandidateSelect";
            case 0x473c60: return "XfTextureNameFormat";
            case 0x4748f0: return "MeshRecordAllocateOrAppend";
            case 0x47d2d0: return "SurfacePixelValueEncode";
            case 0x482b50: return "RenderStateFlagsApply";
            case 0x402960: return "EntityKindCommandIssue";
            case 0x40a4b0: return "QuadMeshRenderRecordCreate";
            case 0x40c5b0: return "ResourceSlotRegisterName";
            case 0x41b3c0: return "EntityQueryFlagCount";
            case 0x438460: return "EntityKindRecordUpdateDispatch";
            case 0x4396c0: return "ItemNameByKindLookup";
            case 0x449700: return "RouteStepCommandIssue";
            case 0x44bb30: return "EntityNodeScaleUpdate";
            case 0x44d0d0: return "DriftOffsetStepApply";
            case 0x4512d0: return "TerrainCellNormalSample";
            case 0x451ab0: return "PrimitiveModelRecordCreate";
            case 0x466250: return "ExpandCellEntitySpawn";
            case 0x46e9d0: return "StagedCellCandidateCollect";
            case 0x4826c0: return "MeshVertexRecordFill";
            case 0x403750: return "CommandKindRecordIssueAndLink";
            case 0x407240: return "BubbleConfigTextureLoad";
            case 0x40a5e0: return "RecordCountDigitsFill";
            case 0x40b1b0: return "DependencyGridOccupancyPass";
            case 0x40e0c0: return "FenceSparkLaunchRegister";
            case 0x40f6c0: return "SceneNodeRecordCreate";
            case 0x412af0: return "MenuSurfaceBlitScaled";
            case 0x41a920: return "LevelStateCountersClear";
            case 0x41c1c0: return "AdvisorOverlayHitResolve";
            case 0x427f80: return "LevelUpgradeNameStringsLoad";
            case 0x4455a0: return "EntityMoveVectorSurfaceProbe";
            case 0x4473b0: return "RouteToCellCommandIssue";
            case 0x449880: return "BlockedCellApproachWeightWrite";
            case 0x46ab10: return "TextSlotMessageAssign";
            case 0x46e220: return "CellSurfaceQuadHeightFill";
            case 0x474f10: return "SceneObjectBoundsUnion";
            case 0x488170: return "AnimationTrackKeysInterpolate";
            case 0x4068c0: return "PathArcLengthInterpolate";
            case 0x40c230: return "SceneSlotPoseScaleRegister";
            case 0x40e350: return "CellEntityKindCreateOrRemove";
            case 0x40f5f0: return "ErosionEightNeighborPropagate";
            case 0x4105f0: return "MenuParamRecordCreate";
            case 0x4129d0: return "MenuOverlaySurfaceAndTextSetup";
            case 0x419de0: return "RecordListEntryMoveToFront";
            case 0x435230: return "RandomVariantCellScatter";
            case 0x4507d0: return "TerrainCellEdgeVectorQuery";
            case 0x451b30: return "LinkedRecordListFlatten";
            case 0x45e6d0: return "TerrainPointPickResolve";
            case 0x464110: return "RotatedPairsScratchFill";
            case 0x4780b0: return "RendererWindowModeApply";
            case 0x482620: return "MeshSegmentVertexFill";
            case 0x48c040: return "StringCaseSubstringFind";
            case 0x402d50: return "KindNineCellCommandIssue";
            case 0x404100: return "CommandQueueSweepRelink";
            case 0x408c80: return "StringTableIndexLookup";
            case 0x41c6e0: return "PointerActionStateDispatch";
            case 0x41cd30: return "PositionTrackerBoxDraw";
            case 0x41f740: return "CellFourNeighborWalkCheck";
            case 0x434480: return "CellSurfaceUpdateDispatch";
            case 0x43c570: return "KindFourPostCreateResolve";
            case 0x44a4f0: return "NearestCellCoordinateWorldResolve";
            case 0x44b140: return "SmallSpiderCellSwarmSpawn";
            case 0x4508a0: return "CellNeighborDirectionCode";
            case 0x451180: return "CellVariantEntryMatchCheck";
            case 0x45cf50: return "PrioritiesConfigLoad";
            case 0x46b660: return "CommandSampleStatsAccumulate";
            case 0x46f270: return "TypeMotionScaleApplyDispatch";
            case 0x471960: return "SeeThroughWallsRayCheck";
            case 0x473bc0: return "ResourceNameRegisterOrQuery";
            case 0x47c2e0: return "DirectDrawDeviceCreate";
            case 0x47c4b0: return "DirectDrawEnumeratedDeviceCreate";
            case 0x409470: return "NeighborCellEntitySearchRecurse";
            case 0x40b880: return "CellVisualChannelRelay";
            case 0x40ba00: return "SceneRotationFlagsApply";
            case 0x40bb90: return "SceneFrameFromTripletApply";
            case 0x40c0d0: return "SceneActSlotLoadAndRegister";
            case 0x40cb70: return "SceneTimeoutEntrySweep";
            case 0x410490: return "StringRegistryIntern";
            case 0x419e80: return "RecordListEntryRemoveAndRelease";
            case 0x44a3d0: return "PlaceCellNearestResolve";
            case 0x44ae50: return "RandomKindSixScatterSpawn";
            case 0x44c7f0: return "LightEffectsBlinkConfigApply";
            case 0x450a80: return "CellListMinDistanceScaleApply";
            case 0x476330: return "ResourceTreeCallbackWalk";
            case 0x476410: return "ResourceNameMatchCallback";
            case 0x489910: return "ResourceTreeCallbackCount";
            case 0x4089d0: return "Segment2DPointDistance";
            case 0x40f0e0: return "KindSixRandomMessageDispatchSweep";
            case 0x40f920: return "KindTwoSteeringTorqueApply";
            case 0x40fb90: return "SteeringPhysicsStep";
            case 0x410690: return "MenuRecordResourcesRelease";
            case 0x41c800: return "CatalogSlotFlagUpdate";
            case 0x424730: return "SinePhaseMidpointBlendDispatch";
            case 0x439a20: return "GatedStepCommandIssueWithTargetRecord";
            case 0x43a120: return "CellStepCommandIssueWithJitter";
            case 0x43aab0: return "ProximityActivityRaise";
            case 0x4420d0: return "KindEffectSlotInitDispatch";
            case 0x443510: return "SceneObjectPosePositionSync";
            case 0x4440f0: return "ActivityTimeoutAccumulateAndRaise";
            case 0x444460: return "FlaggedVectorStepCommandIssue";
            case 0x448880: return "RecordGatedStepCommandIssue";
            case 0x448c90: return "CardinalStepCommandIssueAndLink";
            case 0x448eb0: return "ProximityStepCommandIssueWithJitter";
            case 0x4490a0: return "ProximityStepCommandIssueWithLink";
            case 0x4492b0: return "CellStepCommandIssueWithTargetMark";
            case 0x449ac0: return "ProximityStepCommandIssueClearingGate";
            case 0x44b500: return "ObjectPositionStepCommandIssueAndLink";
            case 0x453bb0: return "FrameTimerArrayAdvanceAndAdvisorClear";
            case 0x463d20: return "ViewBlitRectPositionsApply";
            case 0x466490: return "CellSightRayCheckAndFlag";
            case 0x470540: return "SegmentCenterProximityCheck";
            case 0x471d20: return "VelocitySlotSteerDispatchAndSound";
            case 0x471f00: return "PoseOffsetVelocitySlotBlend";
            case 0x482370: return "MeshVerticesScaleApply";
            case 0x488a70: return "LoadRecordInterfacesReleaseFree";
            case 0x44afb0: return "ConfiguredTinyMonsterSpawnAndTeardown";
            case 0x44b250: return "PairingActivityRaiseAndMutualLink";
            case 0x4350a0: return "SurfaceTypeDescriptionsConfigLoad";
            case 0x444fb0: return "EntityTurnToVectorActivityIssue";
            case 0x4451e0: return "EntityPathStepVectorNormalize";
            case 0x44a7d0: return "PolyVariantSectionModelBuild";
            case 0x44d760: return "JitterStepApply";
            case 0x45ac30: return "PanelTextureSlotsAllocate";
            case 0x4629d0: return "PanelEventEntryAppend";
            case 0x46f3b0: return "RayHitScanAndEffectApply";
            case 0x482220: return "MeshRecordAllocateAndFill";
            default: return null;
        }
    }

    @Override
    public void run() throws Exception {
        File outDir = new File("/tmp/engine-decompiles");
        outDir.mkdirs();
        File out = new File(outDir, "engine-region.c");
        int created = 0;
        int renamed = 0;

        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        FunctionManager fm = currentProgram.getFunctionManager();
        PrintWriter w = new PrintWriter(out);

        try {
            for (int i = 0; i < TARGETS.length; i++) {
                Address addr = toAddr(TARGETS[i]);
                String name = i < NAMES.length ? NAMES[i] : documentedName(TARGETS[i]);
                if (name == null) {
                    name = String.format("FUN_%08x", TARGETS[i]);
                }
                Function f = fm.getFunctionAt(addr);
                if (f == null) {
                    Function inner = fm.getFunctionContaining(addr);
                    if (inner != null) {
                        // address is mid-body of an existing function; decompile that one
                        inner.setName(name, SourceType.USER_DEFINED);
                        f = inner;
                        renamed++;
                    } else {
                        // This objective callback is not discovered as a procedure by
                        // auto-analysis. Its verified control-flow body includes two
                        // flag-gated branches and the shared epilogue through 0x459186.
                        Address bodyEnd = TARGETS[i] == 0x459100L ? toAddr(0x459186L) : addr;
                        f = fm.createFunction(name, addr, new AddressSet(addr, bodyEnd),
                                SourceType.USER_DEFINED);
                        created++;
                        println("Created " + name + " @ " + addr);
                    }
                } else {
                    f.setName(name, SourceType.USER_DEFINED);
                    renamed++;
                }
                w.println("// ==== " + name + " @ " + addr + " ====");
                DecompileResults r = decompiler.decompileFunction(f, 120, monitor);
                if (r.decompileCompleted()) {
                    DecompiledFunction df = r.getDecompiledFunction();
                    w.println(df.getC());
                } else {
                    w.println("// decompile failed: " + r.getErrorMessage());
                }
                monitor.checkCanceled();
            }
        } finally {
            w.close();
            decompiler.dispose();
        }
        println("Done: created=" + created + " renamed=" + renamed + " out=" + out);
    }
}
