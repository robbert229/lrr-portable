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
        0x415700L,  // MoviePlayWait — DirectShow sample-update loop with quit-probe cancellation
        0x4157c0L,  // MoviePlayLoop — timed bitmap-surface display by config key
        0x415910L,  // MoviePlayNamed — play a named movie (config-key -> path)
        0x415990L,  // MoviePlayLevelVideo — play <level> Video movie (LevelLoad path)
        0x415cf0L,  // SaveSlotSelectionClear — DAT_005574d0 = 0xffffffff
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
        0x4653a0L,  // TransientEffectTexturesPreload
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
        0x490090L,  // AssetPathComponentsParse
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
        0x47efd0L,  // DirectInputShutdown
        0x47ccf0L,  // DirectDrawObjectsRelease
        0x485e80L,  // RenderSetup
        0x4893f0L,  // MapFileResourceSlotsReset
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
        0x48bce0L,  // WadArchiveRecordAtIndex
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
        0x498000L,  // CrtMemoryMove
        0x4965d0L,  // CrtFileDescriptorTableInit
        0x499880L,  // CrtTimeZoneStateInit
        0x4962f0L,  // CrtLocaleCodePageTableUpdate
        0x493280L,  // CrtFloatingPointExceptionRaise
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
        0x40b000L,  // TileMatchingCellsCollect
        0x418a00L,  // OverlayTextBufferRefresh
        0x435b20L,  // SceneObjectChainRecordCreate
        0x44d280L,  // RandomWaypointTimerStepApply
        0x45c0d0L,  // PairCountHistogramBuild
        0x469cb0L,  // EntityComponentFloatAt20
        0x469ce0L,  // EntityComponentFloatAtD4
        0x469d20L,  // EntityComponentFloatAtD8
        0x46a0a0L,  // EntitySpeedComponentGet
        0x46f770L,  // EffectApplyByKindBranch
        0x471630L,  // RayHitCellDistanceMinimizeSweep
        0x473460L,  // LightInterfaceCallSlot40
        0x473480L,  // LightInterfaceCallSlot3C
        0x4734a0L,  // LightInterfaceCallSlot38
        0x474760L,  // InterfaceArrayForwardSync
        0x47e300L,  // InterfaceMethod18StructApply
        0x483690L,  // MatrixMethodPairApply
        0x485ea0L,  // InterfaceSlot58PairStoreClamp
        0x489660L,  // SceneActFrameRecordCloneOrCreate
        0x489b30L,  // ObjectInterfaceArrayResourceCreate
        0x41f6c0L,  // SelectedEntityDetachCallback
        0x43fc20L,  // RecordPairPoseCopyAndActivityRaise
        0x407340L,  // StateFlagToggleWhenZero
        0x418800L,  // ModeFlagPairApplyIfBit2
        0x432f00L,  // TerrainCellStateEquals6
        0x435d91L,  // EntityB8Bit3SetOrClear
        0x435f32L,  // EntityFieldsA0A4StoreA8Clear
        0x4538a0L,  // CameraFocusScrollScaleCommit
        0x454b70L,  // ObjectiveVoiceAliasRaiseCommit
        0x454b80L,  // ObjectiveVoiceAliasRaiseGateBit27
        0x46a9e0L,  // AdvisorTextBoxRecordCommit
        0x473560L,  // DDiSceneObjectKindModeSetup
        0x477f80L,  // DisplayDeviceCapsQueryDiscard
        0x49db60L,  // SignedDecimalToStringConvert
        0x49dc10L,  // StringToLowercase
        0x4350a0L,  // SurfaceTypeDescriptionsConfigLoad
        0x444fb0L,  // EntityTurnToVectorActivityIssue
        0x4451e0L,  // EntityPathStepVectorNormalize
        0x44a7d0L,  // PolyVariantSectionModelBuild
        0x44d760L,  // JitterStepApply
        0x45ac30L,  // PanelTextureSlotsAllocate
        0x4629d0L,  // PanelEventEntryAppend
        0x46f3b0L,  // RayHitScanAndEffectApply
        0x482220L,  // MeshRecordAllocateAndFill
        0x495f80L,  // CrtCommandLineArgumentScan
        0x4968f0L,  // CrtRuntimeErrorMessage
        0x49b2b0L,  // CrtPutEnvUpdate
        0x49afe0L,  // CrtCompareStringACompat
        0x4935c0L,  // CrtDoubleToIntegerRound
        0x45b740L,  // ScrollInfoPanelInitialize
        0x45e660L,  // RectContainsPointHalfOpen
        0x45e820L,  // ViewportPointCallbackDispatch
        0x463770L,  // ScrollRectResourceRecordCreate
        0x463810L,  // ScrollRectPayloadRecordCreate
        0x44c150L,  // EntityPairScalarFieldsSet
        0x44e670L,  // NestedPointerRecordDestroy
        0x495160L,  // CxxExceptionHandlerSearch
        0x495ee0L,  // CrtCommandLineArgumentsInit
        0x49a1b0L,  // CrtExtendedFloatToDecimal
        0x49a540L,  // Extended80Multiply
        0x49a800L,  // ExtendedFloatPowerOfTenScale
        0x4802d0L,  // FileTokenListLoadAndTreeScan
        0x480390L,  // DirectoryTreeScan
        0x480570L,  // DirectoryTokenListCompare
        0x481fa0L,  // MeshVertexCoordinateRemap
        0x44fab0L,  // TerrainCellSurfaceTriangleVerticesSelect
        0x43c3e0L,  // EntityType26CommandCompletionGate
        0x496190L,  // CrtEnvironmentBlockGet
        0x49abe0L,  // CrtWideCharToMultiByteCompat
        0x4957d0L,  // CxxCatchObjectBindOrCopy
        0x499b30L,  // CrtDaylightSavingTimeCheck
        0x4887c0L,  // SceneHierarchyInterfaceRegisterRecursive
        0x48e0adL,  // CrtAcosCore
        0x449430L,  // EntityTargetCellRouteAndAssociate
        0x449610L,  // EntityCoordinateRouteAndMark
        0x449980L,  // EntityActivityTerrainEligibilityTransition
        0x49ae10L,  // CrtEnvironmentVariableLookup
        0x49b4c0L,  // CrtEnvironmentEntryFind
        0x49b540L,  // CrtEnvironmentArrayClone
        0x49b5b0L,  // CrtMultibyteCharacterSearch
        0x449520L,  // EntityTargetCellRouteMarker14
        0x449240L,  // EntityWalkableTargetCellRouteIssue
        0x449010L,  // EntityKind6RecordApproachRouteIssue
        0x4489a0L,  // EntityKind6TickDownMarkerSpawn
        0x45d060L,  // PrioritiesImagePositionsConfigLoad
        0x45a650L,  // PanelCategoryBoundsSet
        0x45a270L,  // PanelCategoryInteractionStateReset
        0x45c3f0L,  // AdvisorProgressBarFillDraw
        0x49a890L,  // Win32FileDescriptorResize
        0x49aa10L,  // CrtSignalHandlerDispatch
        0x49ab90L,  // CrtSignalHandlerRecordFind
        0x44acc0L,  // TerrainObjectTypeSfxGridConfigLoad
        0x44ad80L,  // TerrainObjectTypeSfxAtCellGet
        0x449a70L,  // EntityActivity23StateTransition
        0x45d300L,  // PriorityImageSelectionStateUpdate
        0x45d3f0L,  // PriorityImageOrderMoveToFront
        0x47e130L,  // SceneObjectRegistryReleaseAll
        0x004017b0L,  // AdvisorPositionRecordInitialize
        0x004065f0L,  // CubicBezierPointEvaluate
        0x00410320L,  // StringQueueRecordOrFlush
        0x00410a10L,  // MenuItemChoiceStringsAllocate
        0x00410bf0L,  // MenuItemNumericRangeRecordAllocate
        0x004198f0L,  // TerrainCellLinkedEntriesRemove
        0x0041a1a0L,  // UIEntryHitTestAndDispatch
        0x0044da50L,  // ThreeChannelValueTweenStep
        0x00453920L,  // AdvisorScrollStepAdvance
        0x00459900L,  // HungerImagesPositionConfigLoad
        0x00459a50L,  // BubbleImagesPositionConfigLoad
        0x0045b620L,  // PriorityListButtonActionDispatch
        0x00475c80L,  // DDiScenePayloadRelease
        0x00477370L,  // DDiSceneRenderSurfacePoolGrow
        0x0047d430L,  // SceneObjectRegistryTeardown
        0x00403910L,  // CellRecordCountsPruneForEntity
        0x00418920L,  // CategoryCellCandidateListBuild
        0x004031e0L,  // ActiveEntityListContainsCode
        0x00409ff0L,  // EntityDestroyTrackedPointerCleanup
        0x00403810L,  // TerrainCellRecordCountUpdate
        0x0040cdc0L,  // MapCellRecordGridInitialize
        0x0040f890L,  // DirectionDrivenHeadingAngleStep
        0x0040c2f0L,  // TransientEffectRenderSlotConfigure
        0x00419830L,  // EntityLinkedListEntriesRemoveByKey
        0x00408860L,  // AnimationLoopTimeAdjust
        0x00439bd0L,  // EntityAcceptsTargetForTaskCheck
        0x00451230L,  // TerrainCellSurfaceVectorWrite
        0x00451450L,  // TerrainCellSurfaceVectorOffsetAdd
        0x00451c50L,  // OwnedObjectReferenceChainRelease
        0x00453220L,  // AdvisorDisplayStateResourcesRelease
        0x0045a500L,  // PanelImageSlotEntryAllocate
        0x0045d7e0L,  // LevelIdentifierRemapTableLoad
        0x00495df0L,  // CrtEnvironmentPointerArrayBuild
        0x00496ad0L,  // CrtGetStringTypeACompat
        0x00496f50L,  // CrtCalendarFieldsToTimeScalar
        0x00497b90L,  // CrtExtended80ToBinaryFloat
        0x00497e20L,  // CrtDecimalMantissaRoundAndNormalize
        0x00499da0L,  // CrtDaylightTransitionBoundaryCompute
        0x0049b660L,  // CrtStringDuplicate
        0x0049d630L,  // DisplayModeListPopulate
        0x0049dba0L,  // CrtUnsignedIntegerFormatRadix
        0x00401240L,  // Vector2Distance
        0x004013a0L,  // Vector3Length
        0x00401420L,  // Vector3Distance
        0x00401470L,  // Vector3CrossProduct
        0x00401500L,  // Vector3Add
        0x004016c0L,  // Vector3Subtract
        0x004016f0L,  // Vector3DotProduct
        0x00401750L,  // Vector3Normalize
        0x00434da0L,  // AdvisorTooltipAndTooltipSoundConfigLoad
        0x00434a00L,  // AdvisorTutorialIconImageLoad
        0x00435010L,  // TerrainCellWorkFlagCircleSet
        0x004332f0L,  // TerrainCellWorkFlagSet
        0x00439b80L,  // TaskTargetCellEligibilityCheck
        0x0049d780L,  // DisplayResolutionComboPopulate
        0x0049d870L,  // DisplayResolutionFilterTupleAppend
        0x0049d8d0L,  // DisplayModeMatchesFilterTuple
        0x0049d950L,  // DisplayModeNameIndexFind
        0x0049d9d0L,  // DisplayRefreshRateControlsSync
        0x004073a0L,  // EntityPointerRegistriesRemove
        0x004067b0L,  // PolylineSegmentLengthsAndTotalCalculate
        0x00406820L,  // BezierCurveSamplesBuild
        0x00410c70L,  // ResourceVariantRecordRelease
        0x00410f30L,  // ResourceRecordAndChildrenRelease
        0x0044cc10L,  // LightEffectFadeParametersStore
        0x00432950L,  // EntityMoveDestinationCellIsUsable
        0x0043fdc0L,  // EntityActionTargetCellIsValid
        0x004718d0L,  // ObjectPairRayHitCheck
        0x00470690L,  // TimedEntitySegmentVisualCreate
        0x00470260L,  // EntitySurfaceSegmentCollisionCheck
        0x00473e10L,  // RendererFogColorSet
        0x00403560L,  // TaskRegistryRemoveType10EntriesForEntity
        0x00403430L,  // TaskRegistryRemoveType0EntryByPair
        0x00402890L,  // TaskType23CreateForCategoryMask
        0x00431140L,  // TerrainCellQueuedWorkTransition
        0x00470630L,  // EntityDirectedLineSideCheck
        0x00473b20L,  // EntityFlag80Set
        0x00474a60L,  // SceneSurfaceTextureBindByMode
        0x00475020L,  // SceneMeshColorAndAlphaSet
        0x0042ef40L,  // StatsLevelRecordCountForStatType
        0x004093b0L,  // PassableAdjacentCellSearchExpand
        0x004712c0L,  // EntityVelocityStateRecordGet
        0x004712f0L,  // EntityVelocitySlotValueGet
        0x00471310L,  // EntityVelocitySlotValueSet
        0x0043a930L,  // TrackedEntityPointerArrayRemove
        0x0042dfc0L,  // ObjectPointerRegistryClearMatch
        0x0042dff0L,  // ObjectPointerRegistryGetSlot
        0x004293b0L,  // SelectionRectangleRender
        0x00424830L,  // SelectedObjectOutlinesDraw
        0x00432a20L,  // TerrainCellAllowsPlacement
        0x00431ab0L,  // AnyObjectCanOccupyTerrainCell
        0x00432b90L,  // TerrainCellTypeIs8Or10Or11
        0x00433330L,  // TerrainCellTypeGetAtCoords
        0x004329d0L,  // TerrainCellFlag8CheckIfInBounds
        0x0046e8a0L,  // WorldSpatialRecordFindAtGridCoordinate
        0x00471860L,  // SceneSegmentTraceNearestHit
        0x00472080L,  // EntityCentersWithinConfiguredRadius
        0x00472130L,  // EntityCentersWithinObjectRadius
        0x004728e0L,  // RendererFlagBit6Set
        0x00473ec0L,  // RenderObjectStateApplyToEntries
        0x00474e90L,  // RenderObjectHierarchyFloat3Set
        0x004750b0L,  // RenderObjectSetPackedRgbaFromFloatChannels
        0x00423080L,  // ModeDependentField24Get
        0x004230b0L,  // GlobalFloatField00556F00Get
        0x00425d80L,  // CurrentEngineModeIsZero
        0x00426230L,  // GlobalTripleCopyOut
        0x00426360L,  // GlobalStateFlagsBits6And9Set
        0x004263a0L,  // GlobalStateFlagsBit7Set
        0x004263c0L,  // GlobalSubsystemStateReset
        0x00429850L,  // ActiveEngineFieldB0Get
        0x00429860L,  // EngineFloatField210GetByIndex
        0x00429880L,  // EngineFloatField260Get
        0x0042b2f0L,  // EntityEffectCountersApplyFromRecord
        0x004314c0L,  // ActivityBuildRecordRelease
        0x00431530L,  // ActivityBuildRecordsReleaseAll
        0x0046a120L,  // GroupedCodeTableFindValueIndex
        0x0046a370L,  // LinkedRecordChainFree
        0x0046a390L,  // GlobalLinkedRecordChainClear
        0x0046a760L,  // FirstSetFlagOrdinal
        0x0046aad0L,  // MessageLabelIndexFind
        0x00475580L,  // RenderObjectRotateAxis
        0x00475610L,  // RenderObjectSetIdentityTransform
        0x00476270L,  // SceneObjectSetFormattedString
        0x004041b0L,  // EntityTaskChainContainsKind
        0x0040cf80L,  // GridCellObjectLinkInsert
        0x00412450L,  // RenderUiFrameCell
        0x0041a660L,  // InteractionActionCodeMapInitialize
        0x004302b0L,  // MapCellTrackingFlagsClearAndRefresh
        0x00432100L,  // TerrainCellFlag100SetAndTrack
        0x00432270L,  // TerrainCellVisitedFlagSetAndTrack
        0x00465c50L,  // ParticleSlotInitializeForEmitter
        0x00479b80L,  // PointInPolygon2D
        0x00479d50L,  // Matrix4SetRotationZ
        0x004760e0L,  // AttachedObjectDataReleaseAndClear
        0x00476d50L,  // ResourceSlotTableInitialize
        0x00476d80L,  // ResourceSlotTableReleaseAll
        0x0047ab00L,  // SoundBankEntryRelease
        0x0047b050L,  // SoundBankEntryVolumeSet
        0x0047b2a0L,  // KeyedResourceEntryRemove
        0x0048f950L,  // CrtFindHandleClose
        0x00491360L,  // CrtBufferedCharacterWrite
        0x00498b70L,  // CrtWideCharToMultibyte
        0x00498e00L,  // CrtMultibyteToWideChar
        0x004991f0L,  // CrtFileDescriptorHandleGet
        0x004997c0L,  // CrtCallocAligned
        0x00499fb0L,  // CrtUnsignedAddWithCarry
        0x00407300L,  // BubbleActionNameToIndex
        0x00410450L,  // StringKeyedCacheEntryFind
        0x004322d0L,  // TerrainCellVisitedFlagTest
        0x00432300L,  // TerrainCellVisitedFlagsClearAll
        0x00471ca0L,  // EntityWorldCenterPositionRead
        0x00471c70L,  // VelocitySlotVectorRead
        0x00471a40L,  // VelocitySlotPrimaryObjectGet
        0x00471a50L,  // VelocitySlotSecondaryObjectGet
        0x00473ba0L,  // SceneObjectFlagBit3Get
        0x00410520L,  // StringCacheLookupOrLoad
        0x00419430L,  // InterfaceMessageTabInfoTextLoad
        0x00432360L,  // TerrainCellPrimaryFlagsMaskClearAndRefresh
        0x004323c0L,  // TerrainCellSecondaryFlagBit0Clear
        0x004323f0L,  // TerrainCellFlagsSetAndRefresh
        0x00432450L,  // TerrainCellFlagsSetMask100400
        0x00432490L,  // TerrainCellFlagsSetMask20108000
        0x004324d0L,  // TerrainCellSecondaryFlag8SetOrClear
        0x00432520L,  // TerrainCellSecondaryFlag8Test
        0x00432560L,  // TerrainCellFlag40000000ClearOrSet
        0x004325d0L,  // TerrainCellSecondaryFlagBit0Set
        0x00432710L,  // TerrainCellFlag80000ClearOrSet
        0x00432d90L,  // TerrainCellFlag20TestInBounds
        0x0046fef0L,  // EffectModeSurfaceQueryTypeResolve
        0x0046ff70L,  // ActiveEffectRecordSlotFind
        0x0046ff90L,  // ActiveSegmentVisualRecordSlotFind
        0x00479ce0L,  // Matrix4Multiply
        0x00479af0L,  // LineIntersection2DCompute
        0x00478330L,  // Direct3DRenderPreferencesApply
        0x0047dfa0L,  // DirectDrawSurfacePixelRead
        0x0047de60L,  // DirectDrawSurfaceDescriptionGet
        0x0047def0L,  // DirectDrawSurfacePixelFormatGet
        0x00432b00L,  // TerrainCellFlag800TestInBounds
        0x00432c20L,  // TerrainCellFlag40000Test
        0x00432c50L,  // TerrainCellTypeSpeedClassGet
        0x00432dd0L,  // TerrainCellFlag08000000Test
        0x00432e60L,  // TerrainCellFlagsMaskC0Test
        0x00432e90L,  // TerrainCellFlag04000000Test
        0x00432ec0L,  // TerrainCellStateEquals1
        0x00432f40L,  // TerrainCellFlagsMask18ClearInBounds
        0x00432f90L,  // TerrainCellFlag4TestInBounds
        0x00432fd0L,  // TerrainCellFlag2000Test
        0x00433000L,  // TerrainCellFlag80Test
        0x00433030L,  // TerrainCellFlag8000Test
        0x00433060L,  // TerrainCellSecondaryFlagBit0Test
        0x004330e0L,  // TerrainCellFlagsMask180000Test
        0x00473dc0L,  // RendererFogEnableAndColorApply
        0x00473e70L,  // RendererFogModeSet
        0x00473ea0L,  // RendererFogParametersSet
        0x00474ff0L,  // SceneMeshColorSet
        0x00403320L,  // ObjectCommandTaskCreate
        0x00404e00L,  // ObjectPathSubmitToGridTarget
        0x00404f50L,  // ObjectFilterMatchesActionTarget
        0x00407410L,  // Type4ObjectRegistryInsert
        0x004074b0L,  // Type2TimedRegistryInsert
        0x00407540L,  // FlaggedObjectTimerRegistryInsert
        0x00408ce0L,  // OperationGroupAppendObject
        0x00429160L,  // TerrainCellPaletteResourceAtGridPosition
        0x004295c0L,  // EntityMatchesCurrentSelectionOrOwner
        0x004247d0L,  // ApplyDefaultTransformToFlaggedRenderObject
        0x00433120L,  // TerrainCellFlag10000000SetOrClear
        0x004332c0L,  // TerrainCellRecordByte3Get
        0x00434530L,  // TerrainGridEntryResourcesReleaseAll
        0x004382c0L,  // PooledRecordAllocateAndReset
        0x00438310L,  // PooledRecordChunkAllocateAndLink
        0x00438580L,  // EntityFlag80000000SetOrClear
        0x0047a5c0L,  // FixedSizeRecordPoolGrow
        0x0049d5e0L,  // DisplayModeDialogSetModeAndRefreshChoices
        0x00424560L,  // AirSupplyAmountUpdate
        0x00438f60L,  // DockDefinitionMatchesCell
        0x004399c0L,  // EntityFlag400AndHealthThresholdCheck
        0x00439cf0L,  // EntityStatsWordFlag10Test
        0x00439d10L,  // EntityStatsWordFlag20Test
        0x00439d30L,  // EntityStatsWordFlag40Test
        0x0043baf0L,  // EntityFlagsMask107C33DEOr1C00203AnyTest
        0x004436f0L,  // EntityType3Flag04000000Test
        0x00499150L,  // CrtFileDescriptorCloseSlot
        0x0049a050L,  // ExtendedFloat96BitShiftLeftOne
        0x0049a080L,  // ExtendedFloat96BitShiftRightOne
        0x0049a0b0L,  // CrtDecimalDigitSequenceToExtended96
        0x00424810L,  // ApplyDefaultTransformToFlaggedRenderObjects
        0x00435190L,  // SurfaceTypeConfiguredValueAtIndex
        0x0046c030L,  // RenderResourceBindingInitialize
        0x0046ead0L,  // AppendMapRowSpanToBuffer
        0x0046eb30L,  // StartMapRowSpanBufferRecord
        0x00479110L,  // ReadAngleSettingRadians
        0x00479240L,  // ReleaseLinkedObjectList
        0x00479a30L,  // RayPlaneIntersectionParameter
        0x00479ab0L,  // PointAlongRay
        0x00479c10L,  // SphereRayIntersectionDiscriminantPositive
        0x00479da0L,  // Matrix4SetRotationY
        0x00479df0L,  // Matrix4SetRotationZ
        0x00479e70L,  // Matrix4SetIdentity
        0x00479ea0L,  // Matrix4SetZero
        0x00479eb0L,  // Matrix4Copy
        0x00425cd0L,  // Global004de548Set
        0x00425d50L,  // GlobalRenderRecordSetFromInput
        0x004295a0L,  // ContextSelectedGlobalPointerGet
        0x0042b500L,  // GlobalStateField84Get
        0x004310f0L,  // TerrainCellFlag20ClearAndRefresh
        0x00433be0L,  // CurrentLevelTerrainGridPick
        0x00435550L,  // GlobalStateZeroTransitionDetect
        0x004370b0L,  // RenderConfigSixFieldCopyOut
        0x004370d0L,  // RenderConfigSixFieldCopyInOrClear
        0x00437100L,  // GridRecordCountDifferenceSum
        0x00437440L,  // GridRecordCountTablesClear
        0x00437460L,  // GridRecordCountDifferenceGet
        0x004374a0L,  // GridRecordCountGet
        0x00437cc0L,  // EntityFlag40000ThresholdCompare
        0x00438ed0L,  // EntityFlagsContainRequestedSingleBit
        0x0049af60L,  // CrtWideEnvironmentStringListConvert
        0x0049af20L,  // CrtCountedStringCompare
        0x0049aea0L,  // CrtFileDescriptorFlag80SetOrClear
        0x00426250L,  // UpdateRenderViewportState
        0x00436831L,  // CameraSetTiltBounds
        0x00436860L,  // CameraSetTilt
        0x00436a18L,  // CameraSetDistanceBounds
        0x00436953L,  // CameraSetBoundedRotationValue
        0x00435a20L,  // GlobalStateLowByteClearAndRefresh
        0x00471330L,  // InterpolatedZWithinBounds
        0x00471a20L,  // ReadIndexedPairSlotDword
        0x00471ce0L,  // EntityFlag100AndSharedEligibilityCheck
        0x0046f110L,  // WeaponTypeGetRechargeTime
        0x0046f140L,  // WeaponTypeGetDischargeRate
        0x0046f170L,  // WeaponTypeGetRange
        0x0046f1a0L,  // WeaponTypeGetWallDestroyTime
        0x004013e0L,  // AdvisorPositionNameToIndex
        0x004014c0L,  // AdvisorAnimationNameToIndex
        0x00406730L,  // Vector2DistanceVariant
        0x00406760L,  // Vector2SetLength
        0x00466380L,  // SetExpansionEntityAtGridCell
        0x004663f0L,  // CanSpawnExpansionEntityAtGridCell
        0x00465e00L,  // AdvanceTimedVisualSegmentAndFade
        0x004599c0L,  // LoadObjectHungerMeterImages
        0x00459240L,  // PopEntityStateOverride
        0x00475070L,  // SceneObjectCoordinateTransformInPlace
        0x00475090L,  // SceneObjectCoordinateTransformToOutput
        0x004756b0L,  // SceneObjectTransformMatrixSet
        0x00473680L,  // SceneObjectWrappedPointerGet
        0x0041f720L,  // CurrentLevelStateFlagsContainMask
        0x0041f920L,  // LevelStateField9CAdd
        0x0041f9a0L,  // LevelResourceBalanceAddAndSound
        0x00436a4aL,  // CameraSetDistance
        0x00436921L,  // CameraSetScalarBounds
        0x00436b33L,  // CameraGetPositionWithTerrainHeight
        0x00436b79L,  // CameraRotationGet
        0x004395e0L,  // EntityDisplayNameLookup
        0x00439950L,  // ToolNameStringResourcesLoad
        0x00482040L,  // AnimationTrackApplyWrappedFrameOffset
        0x00483810L,  // InputActionStateToggle
        0x00487cb0L,  // AnimationTrackFindKeyframeAndFraction
        0x00498fe0L,  // CrtFileDescriptorAllocate
        0x004990a0L,  // CrtFileDescriptorSetHandle
        0x0049add0L,  // BoundedWideStringLengthIncludingNull
        0x00401840L,  // AdvisorActionPositionAndModeSet
        0x00401890L,  // AdvisorActionPositionGet
        0x004011c0L,  // AdvisorAnimationEndCleanup
        0x00450700L,  // GridSurfaceWriteAtXY
        0x00439930L,  // CopyTwoDwordPairToOutputs
        0x00465160L,  // SoundObjectDurationGet
        0x00476e50L,  // ScreenBoundedSurfaceResourceAllocate
        0x00476fb0L,  // RendererObjectSetClipDistance
        0x00401c30L,  // DisplayWidthGet
        0x00401c40L,  // DisplayHeightGet
        0x00402090L,  // PriorityEntryOrderValueSet
        0x004020a0L,  // PriorityEntryStateSet
        0x004020b0L,  // PriorityEntryStateGet
        0x004021e0L,  // EngineMemoryAllocate
        0x00402210L,  // EngineMemoryFree
        0x0043c280L,  // EntityNestedFloatAddUnlessBlocked
        0x0043c4c0L,  // EntityFinishState1A
        0x0043c530L,  // EntityMarkAndFinishState1A
        0x0046a420L,  // SelectedGlobalPointTransformAndReturnX
        0x0046a470L,  // LinkedListPushAtGlobalHead
        0x00469a90L,  // IndexedValueAppend
        0x00469ac0L,  // IndexedRecordWordAtOffsetB0Get
        0x00469af0L,  // IndexedRecordWordAtOffsetB4Get
        0x00469b20L,  // IndexedRecordWordAtOffsetB8Get
        0x00469b50L,  // EntityIndexedRecordWordAtOffsetBCGet
        0x00469b80L,  // EntityIndexedRecordWordAtOffsetE0Get
        0x00469be0L,  // EntityIndexedRecordWordAtOffsetF0Get
        0x00469d00L,  // NestedFloatAtOffset9CGet
        0x00469d40L,  // NestedFloatAtOffset6CGet
        0x00469d50L,  // NestedFloatAtOffset74Get
        0x00469d60L,  // NestedFloatAtOffset78Get
        0x00469d70L,  // NestedFloatAtOffsetE8Get
        0x00469d90L,  // NestedFloatAtOffset70Get
        0x00469da0L,  // NestedFieldAddressAtOffsetA0Get
        0x00469db0L,  // NestedFloatAtOffset7CGet
        0x00469dc0L,  // NestedFloatAtOffset80Get
        0x00469e00L,  // IndexedRecordFloatAtOffsetDCGet
        0x00469e20L,  // NestedFloatAtOffsetDCGet
        0x00469e40L,  // NestedWordAtOffsetACGet
        0x00469ec0L,  // IndexedRecordWordAtOffset144Get
        0x00469ee0L,  // IndexedRecordWordAtOffset148Get
        0x00469f00L,  // IndexedRecordWordAtOffset14CGet
        0x00469f60L,  // IndexedRecordWordAtOffsetD0Get
        0x00469f90L,  // NestedWordAtOffsetC0OrC4Get
        0x00469fe0L,  // NestedFloatAtOffset8CGet
        0x00472940L,  // SceneRootObjectGet
        0x00476120L,  // ScenePropertyPrimaryValueGet
        0x00476140L,  // ScenePropertySecondaryValueGet
        0x004761c0L,  // ScenePropertyFloatAt14Get
        0x004762f0L,  // ScenePropertyOwnedBufferRelease
        0x00476d10L,  // RenderResourcePairReleaseAndClear
        0x00476f10L,  // RenderSurfaceGetValuePair80And84
        0x00477220L,  // RenderResourceTrackAndRetain
        0x004773e0L,  // RenderResourceTrackedReferencesRelease,
        0x0041a9c0L,  // CaseInsensitiveStringTableIndexFind
        0x0041f870L,  // GlobalObjectField8CIncrement
        0x0041f880L,  // GlobalObjectField8CSubtract
        0x0041f8a0L,  // GlobalObjectField98Add
        0x0041f8c0L,  // GlobalObjectField98Clear
        0x0041f980L,  // GlobalObjectField94Increment
        0x0041f990L,  // GlobalObjectFieldA8Increment
        0x0045a5b0L,  // UITextOutputPositionAndDraw
        0x0045a620L,  // UITextOutputObjectRelease
        0x00462960L,  // TaskTableContentsRelease
        0x00465010L,  // ConfiguredVariantPropertySet
        0x00465030L,  // ConfiguredVariantPropertyRead
        0x0046db20L,  // AttachedEntitySlotObjectGet
        0x00463ea0L,  // IndexedTextMetricsPairGet
        0x00463ed0L,  // IndexedTextMetricsPairSet
        0x00466210L,  // EntityFieldGridBufferRelease
        0x00476220L,  // RenderInterfacePairUpdate
        0x00476310L,  // ScenePropertyValueAt08Get
        0x004026c0L,  // EntityDeferredWorkSchedule
        0x00402e40L,  // CellCommandType13Issue
        0x00402fb0L,  // CellCommandType11Issue
        0x00403030L,  // CellCommandType3Issue
        0x00403b40L,  // EntityPendingTaskFlagClear
        0x00403b60L,  // EntityActivityRecordsRefresh
        0x00435ef4L,  // RecordFields4And14Set
        0x00435f0bL,  // RecordField14Get
        0x004422a0L,  // SurfaceVectorNormalize
        0x0044d250L,  // FloatNegateAbsoluteValue
        0x0044e6b0L,  // IndexedFlagValueSetOne
        0x00451c30L,  // LinkedRecordField4Clear
        0x004532e0L,  // IndirectHandleResolveInPlace
        0x00453900L,  // DeferredCallbackSet
        0x00453910L,  // DeferredCallbackInvokeIfSet
        0x0045a6e0L,  // IndexedPairSet
        0x0045a700L,  // IndexedPairGet
        0x0045b900L,  // IntegerPairDivideToFloatPair
        0x00462ab0L,  // RecordField18Set
        0x00462ac0L,  // RecordFlags24Or
        0x00463730L,  // IndexedFloatPairSet
        0x00465ac0L,  // TransientEffectEnableSet
        0x0046c0f0L,  // RecordResourceRelease
        0x0046ced0L,  // PairedResourceStateSet
        0x0046cfc0L,  // SixResourceSlotsSetState
        0x0047d3f0L,  // BitCount32
        0x00499690L,  // PointerRangeIsReadable
        0x004996b0L,  // PointerRangeIsWritable
        0x004996d0L,  // FunctionPointerIsExecutable
        0x0047cdd0L,  // SurfaceDescriptionGet
        0x0047c1c0L,  // DirectDrawEnumerateAndStoreCallbackResult
        0x00479510L,  // D3DRMVectorRotate
        0x00478310L,  // WindowRectAdjustForWindowedMode
        0x00406c90L,  // AnimationObjectVisibilitySet
        0x004086a0L,  // AnimationObjectVisibilityGet
        0x00406d10L,  // AnimationObjectTimeUpdate
        0x00406da0L,  // AnimationObjectTimeGet
        0x00406dc0L,  // AnimationObjectTransformSet
        0x00406e30L,  // AnimationObjectHandleGet
        0x00407090L,  // AnimationObjectValueGet
        0x00407360L,  // GlobalTaskRefreshFlagGet
        0x004090a0L,  // TerrainCellPredicateCheck
        0x0040e460L,  // GridCellRecordPresentCheck
        0x0040f7b0L,  // OptionalResourceDestroy
        0x00419e60L,  // PendingRecordReleaseIfPresent
        0x00419f10L,  // PendingRecordReleaseAll
        0x0041b470L,  // CategoryValueIsNeitherZeroNorFifteen
        0x00409d40L,  // LevelStateStoredValueSet
        0x0040ab10L,  // GlobalModeZeroInputSet
        0x0040e7e0L,  // GlobalFlagBit0Clear
        0x0040e910L,  // GlobalValueClearIfEqual
        0x0046a270L,  // EntityCollisionRadiusGet
        0x0046cf80L,  // EntityTypeMaskGet
        0x0046d330L,  // EntityTypeFlagGetBit1
        0x0046d9e0L,  // SceneObjectResourceGet
        0x00462830L,  // GlobalFloatAt00554adcAddOne
        0x00462850L,  // GlobalFloatAt00554efcAddOne
        0x00462870L,  // GlobalCounterAt00555324Increment
        0x00462880L,  // GlobalCounterAt00555328Increment
        0x00462890L,  // GlobalCounterAt0055532cIncrement
        0x00479e40L,  // MatrixIdentityAndTranslationSet
        0x0047b330L,  // NestedRecordListRemoveMatchingKey
        0x0047c170L,  // WindowStateInitialize
        0x0047d490L,  // SurfacePixelFormatQuery
        0x004792c0L,  // GlobalRecordListPushHead
        0x0047b0d0L,  // IndexedRecordStride134WordGet
        0x0047d410L,  // RendererRecordTableInitialize
        0x0047ff30L,  // EngineHeapAllocate
        0x0047ff40L,  // EngineHeapFree
        0x00402800L,  // EntityCommand14BatchIssue
        0x00402670L,  // EntityCommand15ValidatedRecordQueue
        0x00409cd0L,  // CellCardinalNeighborApply
        0x00402600L,  // EntityRecordArrayApplyZeroState
        0x00433e30L,  // GridCellEntityEligibilityCheck
        0x00437050L,  // EntityPoolResourcesRelease
        0x00437300L,  // ProcessQueuedGridAnimations
        0x004378e0L,  // GridObjectRecordsRelease
        0x004383a0L,  // GlobalAnimationStateSet
        0x0047b550L,  // SoundBufferVolumeSetMinus300
        0x0047a800L,  // DirectSoundObjectsShutdown
        0x0047a540L,  // ObjectFieldBE4DwordGet
        0x004772b0L,  // RenderInterfaceTransformPoint
        0x00475e10L,  // CallbackRecordSetFlag10AndRefresh
        0x00476f50L,  // ResourceInterfaceCallSlot48
        0x00476fd0L,  // ResourceInterfaceCallSlot8C
        0x00476ff0L,  // ResourceInterfaceCallSlot90
        0x00477250L,  // ResourceInterfaceCallSlot40AndClearField14
        0x00477270L,  // ResourceInterfaceCallSlot88
        0x00401c20L,  // GlobalFlagBit0IsSet
        0x004020c0L,  // HandleArrayReleaseAndReset
        0x00402a40L,  // CoordinatePairCallbackDispatch
        0x00402aa0L,  // ArgumentCallbackDispatch
        0x00402b30L,  // CoordinateCallbackDispatch
        0x00402b60L,  // CoordinateCallbackDispatchByMode
        0x00402ec0L,  // EntityConditionalTaskSchedule
        0x004030b0L,  // CoordinatePairEventQueue
        0x00403190L,  // ObjectEventType2Queue
        0x004033c0L,  // ObjectAttachedRecordAllocate
        0x0046b4d0L,  // CaseInsensitiveNameTableLookup
        0x00463c00L,  // RectContainsPointByConvertedCoords
        0x00463c90L,  // RectContainsPointFromObjectBounds
        0x0046c0b0L,  // CopyFourFieldsAndConvertSecond
        0x0043c200L,  // EntityStatePairPredicate
        0x00442480L,  // VectorDirectionAngleRadiansGet
        0x00443670L,  // TryRandomEntityActionFromCodeRange
        0x00443710L,  // AdjacentPositionCandidateFind
        0x004440a0L,  // LinkedEntityFlagAndCommandUpdate
        0x004471b0L,  // TryCreateLinkedEntityAtPosition
        0x004502c0L,  // CoordinatePairWithinBounds
        0x0045a370L,  // CaseInsensitiveStringTableLookup
        0x0045a410L,  // CaseInsensitiveIndexedStringLookup
        0x0045b8a0L,  // PointDistanceWithinRadius
        0x0045ab30L,  // RequestRecordSetFlag4ClearFlag8
        0x0045ab60L,  // RequestRecordSetFlag2ClearFlag8
        0x0045ab90L,  // RequestRecordFlag8IsSet
        0x00451fd0L,  // CaseInsensitiveNameTableLookupAtOffset8
        0x0047e7e0L,  // BitCount32ByPosition
        0x0047f0d0L,  // CursorSetClientPosition
        0x0047f6a0L,  // SaveDirectoryEnsureExists
        0x00406360L,  // EntityRecordAttachAndRelink
        0x004065d0L,  // GlobalDwordGet
        0x00406c00L,  // RendererObjectUpdateFromRecordField64
        0x00406c30L,  // RendererObjectUpdateFromRecordField68
        0x00406c60L,  // RendererObjectUpdateFromRecordField6c
        0x00406cb0L,  // RendererRecordClone
        0x00406d30L,  // RendererObjectTimingValueAdjustByCount
        0x00406f50L,  // RendererObjectSubobjectDispatch
        0x00406fb0L,  // RendererCachedHandle40GetOrCreate
        0x00406fe0L,  // RendererCachedHandle48GetOrCreate
        0x00407010L,  // RendererCachedHandle50GetOrCreate
        0x00407040L,  // RendererCachedHandle4cIsNotOne
        0x00401bf0L,  // GlobalField505f88Get
        0x00401210L,  // AdvisorSceneVisualStateInitialize
        0x004033f0L,  // CoordinatePairTaskRecordQueue
        0x004034a0L,  // ScalarTaskRecordQueue
        0x00403520L,  // TaskRecordKind10Queue
        0x004038c0L,  // TimedTaskRecordInitializeAndLink
        0x004039a0L,  // TaskRecordCreateAndScanGrid
        0x004039d0L,  // TaskRecordKind11TimedQueue
        0x00403ef0L,  // PendingRecordCallbackDispatch
        0x00403f60L,  // TaskRecordKind14RemoveIfHead
        0x0040fe40L,  // PointerStackPush
        0x0040fe60L,  // PointerStackPop
        0x0040fe90L,  // PointerStackForEach
        0x0040fcc0L,  // VectorScaleComponentsInPlace
        0x00408360L,  // TaskField84GetAndField88Out
        0x00408970L,  // TaskFlagBitQuery
        0x0040f5e0L,  // GlobalConfigValueSet
        0x0046a330L,  // EntityRecordFlag400ToggleAndRefresh
        0x0046c3d0L,  // RecordFieldD8EqualsOne
        0x0046c340L,  // IndexedRecordListRebuildForValue
        0x0046cf40L,  // RecordFlagBitTest
        0x0046cf90L,  // RecordIndexedEntriesRebuildAndRefresh
        0x0046d260L,  // SceneResourceSlotsRebuild
        0x0046d2c0L,  // SceneResourceVisibilitySet
        0x0046d9f0L,  // SceneResourceNamedEntryResolveWithFallback
        0x0046da90L,  // SceneResourceCachedSlotB4Get
        0x0046dac0L,  // SceneResourceCachedSlotB8Get
        0x0046daf0L,  // SceneResourceCachedSlotC0Get
        0x0046d350L,  // SceneResourceDrawAtOrigin
        0x0046e330L,  // GridSurfaceEntryReset
        0x0043c650L,  // EntityCapacityOrStatePredicate
        0x0043b6c0L,  // ClippedRectangleCallbackDispatch
        0x0043a9f0L,  // EntityPairInteractionGate
        0x0046e1c0L,  // GlobalEntityRenderParameterModeSet
        0x0046a000L,  // Entity334FloatField90Get
        0x0046a020L,  // Entity334FloatField98Get
        0x0046a040L,  // Entity334FloatField94Get
        0x0046a080L,  // Entity334FloatFieldF4Get
        0x0046a170L,  // Entity334FloatField130Get
        0x0046a190L,  // Entity334FloatField134Get
        0x0046a1b0L,  // Entity334FloatField120Get
        0x0046a1d0L,  // Entity334FloatField128Get
        0x0046a1f0L,  // Entity334FloatField124Get
        0x0046a210L,  // Entity334WordField13CGet
        0x0046a230L,  // Entity334FloatField12CGet
        0x0046a250L,  // Entity334FloatField138Get
        0x0046a290L,  // Entity334FloatField114Get
        0x0046a2b0L,  // Entity334FloatField118Get
        0x0046a2d0L,  // Entity334FloatField11CGet
        0x0046a2f0L,  // Entity334FloatField110Get
        0x0046a310L,  // Entity334FloatField10CGet
        0x0046a0c0L,  // MappedIndexedRecordWordAtFCGet
        0x0046a0f0L,  // MappedIndexedRecordWordAt100Get
        0x0047a580L,  // FixedSizeRecordPoolAcquire
        0x0047b2e0L,  // NestedRecordListRemoveByKeyRecursive
        0x0047e480L,  // TwoInterfaceFieldsReleaseAndReset
        0x004034e0L,  // TaskPairRecordEnqueue
        0x00403650L,  // EntityTaskBatchAppend
        0x004040a0L,  // EntityTaskQueueFlush
        0x00403af0L,  // EntityTaskStateUpdateAfterCleanup
        0x00435ebbL,  // ScreenEffectRecordSet
        0x0045bf70L,  // UIEntryBoundsGet
        0x0045d470L,  // UIListEntriesSwap
        0x0045e400L,  // EntityTypeAndStateFlagPredicate
        0x00462da0L,  // IndexedSurfaceResourceAllocate
        0x004634f0L,  // IndexedSurfaceEntryHideAndRecord
        0x00463540L,  // IndexedSurfaceEntriesRestore
        0x004636b0L,  // InfoPanelStringEntriesInitialize
        0x00464720L,  // SurfaceVisibilitySet
        0x00464e80L,  // DeferredSoundPlayExpire
        0x00464ec0L,  // DeferredSoundTimerSetIfIdle
        0x0046d1c0L,  // AnimationDurationForLoopCount
        0x0046f1d0L,  // EntityIndexedFloatLookupWithMinusOneFallback
        0x0047e0c0L,  // FixedSizeRecordPoolGrow
        0x004807f0L,  // RenderResourceInstancePoolGrow
        0x00488730L,  // CaseInsensitivePairTableFind
        0x00488790L,  // CaseInsensitivePairTableAppend
        0x0048b4a0L,  // BufferedCharStreamReadToString
            0x00403a10L,  // TaskCallbackDispatch
        0x0040c1b0L,  // IndexedResourceHandleSetCreate
        0x0040c720L,  // GlobalResourceHandleLoadAndEnable
        0x0040c920L,  // ResourceHandleLoadWithFallback
        0x00412190L,  // MenuAudioTickIntervalReached
        0x00413f30L,  // FilenameExtensionClassify
        0x00417f20L,  // OptionsContextSixFieldSet
        0x004180c0L,  // OptionsContextCopyAndReplaceResource
        0x004196f0L,  // IncidentMessageQueueAppend
        0x004197b0L,  // IncidentMessageQueueRemoveAt
        0x00419a80L,  // IncidentMessageNodeCreate
        0x00419ae0L,  // IncidentMessageFlagSet
        0x00419da0L,  // IncidentMessagePanelRefreshIfActive
        0x0041a250L,  // ScrollOffsetFromCursorUpdate
        0x0041a290L,  // ScreenRightEdgeCacheRefresh
        0x0041a2f0L,  // IncidentMessageStateGet
        0x0041ba90L,  // SimulationGlobalWordSet
        0x00436ac9L,  // RendererNodeRelativePositionSet
        0x00435e35L,  // ModelTransformCopyToSceneNode
        0x0045d4f0L,  // ConfigEntryEnabledToggleAtIndex
        0x00462ed0L,  // GraphicsOverlayResourcesRelease
        0x004634b0L,  // GraphicsOverlayVisibilitySet
        0x00465970L,  // TransientEffectRegistryReleaseAll
        0x004661c0L,  // GridCellRecordBufferAllocate
        0x0046c130L,  // SceneObjectSetAnimationWithFallback
        0x0046d1a0L,  // SceneObjectAttachedResourceRelease
        0x0046db60L,  // ObjectRecordField11CGet
        0x00480870L,  // TaskCallbackRecordAllocateAndInitialize
        0x00482470L,  // IndexedGeometryDataCopyToOutput
        0x0047e4c0L,  // AviPlaybackStateInitialize
        0x0047ed70L,  // InterfaceInvokeOneIfNonNull
        0x00483140L,  // IndexedRecordInterfaceRelease
        0x00483170L,  // IndexedRecordTransformInitialize
        0x00483240L,  // IndexedRecordTransformCopyIn
        0x00483400L,  // IndexedRecordDataAddressGet
        0x00483540L,  // Matrix4x4IdentityInitialize
        0x00483580L,  // RenderInterfaceCapabilityCheck
        0x004078c0L,  // EntityCursorTargetStepResolveAndTimerAdvance
        0x0040e8d0L,  // ScreenSpaceEffectDispatchIfEnabled
        0x0040b850L,  // PackedGridCellValueGet
        0x00406400L,  // PendingRecordCopyToEntityAndFlag
        0x004063e0L,  // PendingRecordEntityReferenceClear
        0x00408620L,  // SceneObjectRecordCloneRetainHandles
        0x00408670L,  // SceneObjectPairApplyMode
        0x004086c0L,  // SceneObjectPairSetRenderParameters
        0x00408710L,  // SceneObjectPairSetCallbackScalar
        0x00408770L,  // SceneObjectIndexedHandleGetOrCreate
        0x004087b0L,  // SceneObjectPrimaryHandleGetOrCreate
        0x004087e0L,  // SceneObjectSecondaryHandleGetOrCreate
        0x00408810L,  // SceneObjectIndexedSecondaryHandleGetOrCreate
        0x00408850L,  // SceneObjectFlagsGet
        0x00408b60L,  // PointProjectOntoLine2D
        0x00406980L,  // SceneObjectModeIsOne
        0x004082e0L,  // SceneObjectScalarAdjustByDirection
        0x00439cd0L,  // EntityAnimationStateSet
        0x0043a8f0L,  // SimulationUpdateCounterAdd
        0x0043a910L,  // SimulationModeCounterAdd
        0x0043a9e0L,  // EntityQueueForRockfallProcessingIfEligible
        0x0044f220L,  // SceneObjectResourceHandleSet
        0x004525e0L,  // SelectionRecordArrayGet
        0x004525f0L,  // SelectionRecordCountGet
        0x00452600L,  // SelectionRecordCountReset
        0x0046c110L,  // AnimationResourceAdvanceTime
        0x0046cf20L,  // SceneEntityAnimationTracksSetEnabled
        0x00466230L,  // GridCellRecordBufferReplace
        0x00480780L,  // ZeroResultIsSuccess
        0x004807a0L,  // FixedSizePoolRecordAcquire
        0x004807d0L,  // FixedSizePoolRecordRelease
        0x004821a0L,  // IndexedRecordEntryRead
        0x00482350L,  // IndexedRecordField64Set
        0x00482780L,  // IndexedRecordFlag6CBit0Get
        0x004827a0L,  // IndexedRecordFlag6CBit0Set
        0x004827d0L,  // RecordFlag2CBit0Set
        0x00482cb0L,  // RendererCommandModeAndPairSend
        0x00482ce0L,  // RendererCommandNodePrepend
        0x004850c0L,  // IndexedWordTableLookup
        0x004851b0L,  // FloatPairAtOffset4SetFromInts
        0x004851d0L,  // FloatPairAtOffsetCSetFromUInts
        0x004891d0L,  // MmioHandleAndGlobalBufferRelease
        0x00489d70L,  // ObjectField38Release
        0x0048f970L,  // FileTimeToLocalDateTimeStamp
        0x0048fb50L,  // DriveLetterRootTypeAvailable
        0x0048e030L,  // ByteRangeSwap
        0x004976d0L,  // DbcsAwareBoundedStringCopy
        0x0048be00L,  // FixedRecordSlotAllocate
        0x0048bfc0L,  // FixedRecordSlotResourceRelease
        0x0048bff0L,  // FixedRecordSlotResourceGet
        0x00409350L,  // PathRecordAllocateAndLink
        0x00408bf0L,  // Point2DProjectOntoDirection
        0x0040a5c0L,  // QuadMeshRenderRecordPoolSlotAllocate
        0x0040aae0L,  // EntityQuadMeshRenderRecordAllowed
        0x00409f20L,  // EntityPlacementCommandsBuildOnce
        0x0040b410L,  // TerrainCellResourceGridDispose
        0x0040b810L,  // CellVisualChannelTableResourcesClear
        0x0040b7d0L,  // CellVisualChannelTableDispose
        0x0040be10L,  // SceneLightCreateAtPosition
        0x0040c4d0L,  // SceneLightPoolExpireSweep
        0x0040c520L,  // SceneLightSecondaryPoolExpireSweep
        0x0040c6c0L,  // RockFallStyleSceneResourcesRelease
        0x0040c690L,  // SceneResourceSingletonPoolRelease
        0x0040d0a0L,  // TerrainEntityCellRecordRemove
        0x0040d100L,  // TerrainCellEntityRecordUnlink
        0x0040ee00L,  // CellCoordinateTimestampRemember
        0x0040efb0L,  // CellCoordinateTimestampExists
        0x0040fb00L,  // MatrixRotationComposeXYZ
        0x00410560L,  // ResourceNameCacheGetOrLoad
        0x00410aa0L,  // ResourceHandleListAppend
        0x00410ae0L,  // ResourceHandleListDispose
        0x00411320L,  // TextLayoutOffsetForCentering
        0x00411840L,  // FrontendProgressValueChanged
        0x00413b50L,  // InterfaceResourceHandlesRelease
        0x00418fc0L,  // UiModeTransitionRequest
        0x00419100L,  // UiModeResourcePositionsApply
        0x00419200L,  // UiModeResourceBit40SetClear
        0x004191f0L,  // UiModeResourceBit40Get
        0x0041b1b0L,  // InterfaceBackButtonConfigLoad
        0x0041b270L,  // InterfaceButtonImagesLoad
        0x0044f810L,  // GridCoordinateClampAndStore
        0x00450730L,  // TerrainCellChangeQueueAppend
        0x00450780L,  // TerrainCellChangeQueueFlush
        0x00452650L,  // ArrayFindValueIndex
        0x00456c30L,  // ObjectPoolListHeadGet
        0x00456c60L,  // ObjectPoolListsFreeAll
        0x004589a0L,  // GlobalOptionLowBitsEnabled
        0x0045b930L,  // FloatPairToIntegerPair
        0x00465a90L,  // TransientEffectRegistryEnableSet
        0x0046db70L,  // SceneObjectResourceRelease
        0x0044e860L,  // GridCellSetPayloadIndex
        0x004755b0L,  // SceneFrameScaleSet
        0x0047e3e0L,  // InterfaceMethodResultRecordInitialize
        0x004893a0L,  // CdAudioStopAndClose
        0x00487fc0L,  // KeyframeTimeSegmentAndFractionGet
        0x00485e30L,  // NamedOptionIndexFindCaseInsensitive
        0x00488b90L,  // CdAudioPlaybackAvailable
        0x0048c180L,  // OwnedResourceTreeRelease
        0x00484ba0L,  // PackedTripletTableToWordBuild
        0x00410020L,  // SceneNodePositionSet
        0x00419030L,  // HelpWindowButtonFeedbackUpdate
        0x00419780L,  // IndexedLinkedListEntryGet
        0x00440130L,  // EntityUpdateAndUnregisterAtTile
        0x00440bf0L,  // ObjectListFieldAssignAll
        0x00447780L,  // EntityClearLinkedObjectState
        0x00448800L,  // EntityMoveToCellCoordinates
        0x00450060L,  // GridCellReadSurfaceVector
        0x00450220L,  // GridCellReadSurfaceValues
        0x004524b0L,  // SelectionObjectDetach
        0x0045bf20L,  // CameraZoomStepAdjust
        0x0045b850L,  // UiOverlayCreateConfiguredElement
        0x0045c580L,  // UiOverlayAnimationAdvance
        0x0045c5f0L,  // UiOverlayAnimationApply
        0x0045da90L,  // UiRenderRecordCreate
        0x0045daf0L,  // UiRenderRecordFree
        0x00460290L,  // EntityType4IndexListBuild
        0x00460310L,  // EntityType4IndexListAppend
        0x0046f380L,  // EntityApplyImpulseAndDirection
        0x0043c2b0L,  // PeriodicTimer25Advance
        0x00439c80L,  // EntityIndexedFlagSetAndRefresh
        0x004565f0L,  // EngineModeSet
        0x00474ae0L,  // TerrainDetailEntryGetAtIndex
        0x00474e30L,  // TerrainSurfacePropertySet
        0x00475880L,  // SceneObjectOwnerLinkEnsure
        0x00476dc0L,  // RenderTargetViewportScaleSet
        0x00476f70L,  // SceneObjectQueryValueIfReady
        0x0047ce20L,  // ProjectionOverlayDrawSubmit
        0x00483b00L,  // WorldObjectRenderStateApply
        0x00487990L,  // UIRenderTreeLayoutAndRelease
        0x004880d0L,  // UIRenderElementChildrenEnumerate
        0x00488900L,  // UIRenderTreeEntryProcess
        0x004889a0L,  // UIRenderEntrySubrecordApply
        0x00489750L,  // SceneObjectOwnerLinkRelease
        0x00408e10L,  // CountMatchingCoordinatesInList
        0x00408e50L,  // CreateRepeatedResourceInstances
        0x00409110L,  // ReleaseSelectionGroupResources
        0x00409150L,  // ClearSelectionGroup
        0x00409940L,  // RefreshEightNeighborCells
        0x004099d0L,  // ReleaseAllGroupRecords
        0x0040aea0L,  // EnsureCellEntriesLoaded
        0x0040af40L,  // CellResourceEntryMatches
        0x0040b250L,  // CellFlag4IsSet
        0x0040b2a0L,  // CellFlag100Set
        0x0040b2e0L,  // CellFlag100IsSet
        0x0040bd60L,  // BuildPerspectiveProjectionMatrix
        0x0040c570L,  // ReadCellTablePair
        0x0040c8a0L,  // GetResourceSlotAddressByIndex
        0x0040d170L,  // CreateCellObjectIfAvailable
        0x0040d1f0L,  // DeleteCellObject
        0x0040dd90L,  // ComputeWorldPositionDelta
        0x004114f0L,  // HasType6ItemAtIndex
        0x004119d0L,  // HasActiveResource
        0x00417fe0L,  // CurrentSelectionContextGet
        0x00417ff0L,  // CurrentSelectionContextSet
        0x00418120L,  // IsAverageValueAbove99
        0x00418f80L,  // SelectionModeSetByIndex
        0x00419450L,  // SelectionFlag10SetClear
        0x004194f0L,  // IndexedSelectionFlag20000SetClear
        0x0041c9f0L,  // InputEntryFlag8SetClear
        0x0041ca20L,  // PlayerInputFlag8SetClear
        0x0041ca60L,  // InputEntryFlag8Get
        0x0041ca70L,  // PlayerInputFlag8Get
        0x0041cfb0L,  // CancelActionForMatchingCoordinates
        0x0041e990L,  // CellActionPermissionGet
        0x0041f230L,  // QueueActionRequestType2
        0x0041f270L,  // QueueActionRequestValue
        0x0041f2b0L,  // QueueActionRequestFlag100000
        0x0041f2f0L,  // QueueActionRequestPair
        0x0041f340L,  // QueueActionRequestIndex
        0x0041f3c0L,  // QueueCallbackRequest390
        0x0041f400L,  // QueueCallbackRequest3E0
        0x0041f970L,  // RuntimeFlag40000000Get
        0x0045d370L,  // RegionBoundsSet
        0x0044a390L,  // Type11CellCoordinateAppend
        0x0044c710L,  // SceneObjectTransformStateInit
        0x0044c790L,  // ConfiguredSceneTransformEnable
        0x0046aa90L,  // MessageOverlayStateInit
        0x0047e710L,  // DirectDrawSurfaceDescriptorInitialize
        0x00485fb0L,  // ViewportRectangleGet
        0x00488bb0L,  // CdAudioTrackStart
        0x00408b00L,  // PointToPolylineMinimumSegmentDistance
        0x00408d70L,  // TerrainDescriptorCoordinateCountLimitCheck
        0x0040a3d0L,  // ScreenEffectFlagsClearAndRelease
        0x0040ab30L,  // GridRecordFlagMaskClear
        0x0040f7d0L,  // CallbackStateRecordCreate
        0x0040f850L,  // CallbackStateListDispose
        0x0040fdb0L,  // ThreeValueRecordCopyAndMarkReady
        0x00410280L,  // CallbackOperationRunAndReturnStatus
        0x00410b60L,  // ThreeWordRecordAllocateAndInitialize
        0x00410b90L,  // BoundedFloatRecordCreate
        0x00417280L,  // RegistryValueLookupWithLock
        0x004172d0L,  // RegistryValueIndexLookupOrNotFound
        0x004189d0L,  // RenderTransitionCleanupIfActive
        0x00438720L,  // EntityAvailableResourceSpend
        0x0043bbc0L,  // EntityDetachCarriedObjectAndResetState
        0x004437f0L,  // EntityDispatchEvent31ForPendingFlag
        0x00444620L,  // EntityEnterEvent21StateOnce
        0x00447730L,  // EntityProgressAdvanceBy25
        0x00447b00L,  // EntityDispatchEvent5AndSetFlag
        0x004487c0L,  // EntityClearNestedState8Fields
        0x0044d9a0L,  // VectorWithinAxisTolerance
        0x00451bc0L,  // ResourceListUpdateMatchingKey
        0x00459530L,  // RenderThreePassUpdate
        0x0045abb0L,  // InputDeviceSetFlag20
        0x00463e40L,  // FloatRoundToNearestInt
        0x00466750L,  // GridCellInvalidateAndClearFlags
        0x00496530L,  // CrtCodePageLocaleIdLookup
        0x00496590L,  // CrtCtypeTablesReset
        0x00497040L,  // CrtStreamReadByte
        0x00497570L,  // CrtStreamBufferFlushAndRelease
        0x00499710L,  // WideStringLength
        0x00499730L,  // CrtRuntimeErrorMessageBoxDisplay
        0x00498f50L,  // CrtStreamUngetByte
        0x004965c0L,  // CrtAnsiCodePageTablesInit
        0x004975d0L,  // CrtCharacterToLowerLocale
        0x00498ae0L,  // CrtStreamBufferAllocate
        0x0040ce30L,  // SelectionMarkerResourcesRelease
        0x0041c7b0L,  // InputActionStateUpdate
        0x0041c8f0L,  // InputActionPressedGet
        0x0041c950L,  // InputBindingPressedGet
        0x0041caf0L,  // InputBindingValueGet
        0x0041cb90L,  // InputBindingDefaultValueGet
        0x0041f100L,  // ObjectInteractionFlagsAllow
        0x0041cea0L,  // SelectionObjectCommandUpdate
        0x0041cf20L,  // SelectionTargetCommandUpdate
        0x0041cf80L,  // SelectionMode7CancelAtPoint
        0x0041cfe0L,  // SelectionMode5CancelAtPoint
        0x0043a7c0L,  // EntityActionKindSupported
        0x0043a850L,  // EntityActionRecordConsumeAndCleanup
        0x004351a0L,  // ActionCodeToSoundAlias
        0x00436ae9L,  // SelectionRecordPointGet
        0x00436b0bL,  // SelectionRecordPointSet
        0x004388b0L,  // EntityActionKindPrerequisiteCheck
        0x00439e40L,  // EntityRecordFlag20Query
        0x0043b320L,  // EntityIterationCallbackGuard
        0x00491770L,  // CRTHeapReleaseUnusedPages
        0x00491840L,  // CRTHeapLocateAllocationPage
        0x004850e0L,  // ObjectField10Get
        0x00485160L,  // TextOutputField82CBit2SetClear
        0x0040d4f0L,  // TerrainCellActivationTry
        0x0040cea0L,  // TerrainCellAndNeighborsUpdate
        0x0040c750L,  // TerrainPointObjectCreate
        0x0040c830L,  // TerrainPointObjectsRemoveMatching
        0x0040cc40L,  // TerrainMarkerSetsClear
        0x0040d030L,  // TerrainRecordPoolChunkAllocate
        0x0040e700L,  // TerrainChoiceResourceSelect
        0x0041a850L,  // ResourceHandleSetRelease
        0x0041b9c0L,  // SelectionEntityCodeResolve
        0x0041cab0L,  // IndexedEntityValueGet
        0x0041cce0L,  // SelectionCoordinatesSet
        0x0041e9d0L,  // SelectionRegionParametersSet
        0x0041ec30L,  // EntityPairCountRefresh
        0x0044cff0L,  // UiElementRgbColorSetClamped
        0x0044dd40L,  // EngineLoaderProfileEntriesRelease
        0x0044e0a0L,  // GlobalResourceHandleRelease
        0x0045d550L,  // UiCursorMoveToChoiceEntry
        0x0045d640L,  // UiCursorMoveToElementCenter
        0x00494d10L,  // CrtFileDescriptorClose
        0x00494df0L,  // CrtOwnedStreamBufferRelease
        0x00498f00L,  // CrtCharacterClassFlag8Check
        0x00497f40L,  // DoubleToExtended80
        0x0048e870L,  // DoubleIsFinite
        0x00475830L,  // SceneObjectFirstNonKind6ChildGet
        0x00498b40L,  // CrtFileDescriptorFlag40Get
        0x00495db0L,  // CrtCharacterClassFlagTest
        0x00493150L,  // X87RoundToIntegral
        0x00406df0L,  // RenderNodeSetPositionFromCallback
        0x00408930L,  // ConstantZeroFloatGet
        0x00409a90L,  // WorldObjectCreateWithTransform
        0x0040af90L,  // TerrainCellRecordGet
        0x0040bde0L,  // SceneMarkerCreateAtObjectPosition
        0x0040beb0L,  // SceneMarkerCreateAtPositionOrObject
        0x0040fe00L,  // SceneObjectListUpdateTransforms
        0x0040ff50L,  // SceneObjectListApplyFloatCallback
        0x00410070L,  // SceneObjectListApplyPositionCallback
        0x004100b0L,  // SceneObjectListApplySecondPositionCallback
        0x00418190L,  // SelectionModeStateSet
        0x004181a0L,  // SelectionStateFlagClearBit0
        0x00419150L,  // SelectionStateHandleGet
        0x0041a990L,  // ObjectiveSelectionStateReset
        0x0041b300L,  // MenuSelectionResourceUpdate
        0x0041cc80L,  // CursorTileSelectionUpdateByType
        0x0041d010L,  // SelectionCancelIfCursorHitsObject
        0x004493e0L,  // SceneObjectHasTaskType4_5_7
        0x00449cd0L,  // SceneObjectStateHandleApplyValue
        0x0044b7a0L,  // MatrixApplyRotationAboutZWithOffset
        0x00441ea0L,  // EntityType5FinalizeIfAllowed
        0x00490400L,  // CrtScientificFloatFormatAppend
        0x00490540L,  // CrtFixedFloatFractionFormat
        0x00490640L,  // CrtFloatDecimalFormatPrepare
        0x00495090L,  // CxxExceptionHandlerDispatch
        0x004889d0L,  // ResourceTreeCallbackReferenceChainRelease
        0x00489520L,  // ResourceTreeCallbackContextCreateForEnumerator
        0x004895c0L,  // ResourceTreeCallbackContextCreateForResource
        0x00404ed0L,  // CheckObjectCandidate
        0x00404f10L,  // FindMatchingObjectRecord
        0x00407370L,  // ResetObjectAuxiliaryState
        0x00407510L,  // SetType2ObjectRecordValue
        0x00409450L,  // FindNearbyPlacementCandidate
        0x0040bdc0L,  // SetTimeoutTo10000
        0x0040bfc0L,  // RockFallStyleTableSet
        0x0040cf50L,  // RegisterObjectAtGridPosition
        0x0040fde0L,  // SetVectorAndNotifyListeners
        0x0040fec0L,  // SetGlobalScalarToOne
        0x0040ff70L,  // ObjectVector3Set
        0x0040ffe0L,  // GlobalVectorSetAndNotifyListeners
        0x00410b30L,  // GlobalPointerGet_004c76ab24
        0x00410b40L,  // GlobalPointerGet_004c76ab28
        0x00410b50L,  // GlobalPointerGet_004c76ab3c
        0x00417430L,  // FloatToLong
        0x00418000L,  // ShowLevelOverlayForCurrentSelection
        0x00419b50L,  // RemainingTimerDurationGet
        0x0041b9b0L,  // GlobalStateGet_004ddd1c
        0x0041ee60L,  // InteractionTimerResetAndFlagClear
        0x0041f840L,  // RemoveObjectIfClass6
        0x0041f8e0L,  // NoOpCallback
        0x00438670L,  // EntityAtGridPositionLookup
        0x00439790L,  // LocalizedObjectTheNameGet
        0x00439860L,  // LocalizedObjectNameGet
        0x00443840L,  // EntityRouteCellCandidateAccept
        0x00477f70L,  // RenderFeatureFlagsGet
        0x0047ec60L,  // AviPlaybackFrameIndexGet
        0x004802b0L,  // LoaderProgressCallbackSet
        0x00482210L,  // PointerValueGet
        0x00487760L,  // ResourceStringTablesFree
        0x004877d0L,  // ResourceEntryIdGet
        0x004894c0L,  // ComInterfaceQueryOrFallback
        0x0048bcf0L,  // WadEntryLengthGet
        0x0048bd10L,  // WadEntryField4Get
        0x0048dfd0L,  // QsortSmallPartitionSort
        0x0048e250L,  // StackProbeAllocate
        0x0048e4f0L,  // CrtTerminateProcessWithHandlers
        0x0048e5a0L,  // CrtRunCallbackArray
        0x0048e680L,  // CrtDoubleAbsoluteValue
        0x00490070L,  // VsscanfParseInput
        0x00494ca0L,  // StreamPushbackChar
        0x00494cc0L,  // StreamSkipClassifiedChars
        0x004977d0L,  // CrtControlFlagsTranslate
        0x00499fe0L,  // UInt96Add
        0x0047a1b0L,  // FormattedTextDraw
        0x0048d940L,  // CrtMathDispatchTableInitialize
        0x0048e090L,  // CrtAsin
        0x0048e480L,  // CrtFloatingPointRuntimeInitialize
        0x0048e4b0L,  // CrtExit
        0x0048ec00L,  // SehFrameUnwind
        0x0048eca0L,  // CallSettingFrame12
        0x0048ee90L,  // SehScopeTableLookup
        0x0048fa00L,  // CurrentDirectoryGet
        0x0048fd20L,  // CrtFlushStreamBuffer
        0x00493ec0L,  // BoundedStringLength
        0x00493ae0L,  // DoubleFrexpDecompose
        0x00493be0L,  // FpuStatusWordGetVariantA
        0x00493bf0L,  // FpuStatusWordGetVariantB
        0x00497aa0L,  // UInt96Zero
        0x00497ab0L,  // UInt96IsZero
        0x00497a80L,  // UInt96Copy
        0x00497ad0L,  // UInt96ShiftRight
        0x00497900L,  // UInt96HasNoBitsAtOrAbove
        0x00497970L,  // UInt96IncrementAtBit
        0x004979e0L,  // UInt96ClearFromBit
        0x00438df0L,  // EntityLookupAtWorldPosition
        0x00442210L,  // EntityActionRouteStateInitialize
        0x0044c970L,  // LightEffectsBlinkParametersApply
        0x00490280L,  // CrtFloatingPointFeatureProbe
        0x004902b0L,  // CrtNormalizeDecimalPoint
        0x004907c0L,  // StringPrefixReserve
        0x004913b0L,  // CrtWriteRepeatedChar
        0x004913f0L,  // CrtWriteStringBounded
        0x004914a0L,  // StrChr
        0x00491560L,  // CrtHeapInitialize
        0x00491710L,  // CrtHeapBlockFree
        0x00492dccL,  // CrtFmodExtendedPrecision
        0x004939d0L,  // CrtMathExceptionHandlerGet
        0x00493a00L,  // CrtMathExceptionCodeFromStatus
        0x00493a40L,  // DoubleBuildWithExponent
        0x00493a80L,  // DoubleClassify
        0x00495d90L,  // IsDbcsLeadByte
        0x004964e0L,  // CrtCodePageGet
        0x004974d0L,  // CrtStandardStreamBufferInstall
        0x00497ec0L,  // CrtLocaleDataGetOrInitialize
        0x00498340L,  // CrtFatalMessageExit
        0x00499860L,  // CrtTimeZoneInitializeOnce
        0x0048e890L,  // MarkupIntegerTokenParse
        0x0043f560L,  // EntityPoolFindWithLocalPredicate
        0x00456640L,  // GlobalFlagGet_004ff978
        0x00458650L,  // GlobalFlagGet_004ffe30
        0x00458660L,  // GlobalValueSet_004ffe48
        0x0045c4a0L,  // GlobalValueSet_0050080c
        0x0045cc10L,  // IndexedGlobalValueGet_00500a9c
        0x0045cc60L,  // GlobalValueGet_00500a98
        0x004628c0L,  // GlobalFloatAccumulate_00555120
        0x0047c160L,  // ConstantNegative10000Get
        0x0048c030L,  // WadMemoryStreamBufferGet
        0x0048e1aaL,  // CrtMathIntrinsicDispatch
        0x00491ff5L,  // CrtNoOpStub
        0x00495bdeL,  // CrtAbortAfterCallback
        0x00497870L,  // CrtNoOpStubVariant
        0x0044c7c0L,  // LightEffectsConfigurationLoad
        0x0044cef0L,  // LightEffectsMoveParametersStore
        0x0044d710L,  // WaypointVectorRandomScaleApply
        0x0046abb0L,  // TextSlotMessageWithImageAssign
        0x0048e280L,  // CrtRoundToIntegralWithMathErrorHandling
        0x0048e8e0L,  // CrtRoundToIntegralWithMathErrorHandlingVariant
        0x004906f0L,  // CrtScientificFloatFormatGuarded
        0x00490720L,  // CrtFixedDecimalFormatGuarded
        0x004908a0L,  // CrtFileStreamClose
        0x00491d90L,  // CrtNewHandlerInvokeForAllocationRetry
        0x00494c30L,  // CrtScanFormatCharNormalize
        0x0047e460L,  // ComObjectDestroyAndOptionallyFree
        0x00485210L,  // FormattedTextAppendToRingBuffer
        0x0048fed0L,  // CrtInputWithStreamBufferSetup
        0x0048b280L,  // WindowModeFlagSet
        0x0047df50L,  // PixelChannelMaskGet
        0x0044ade0L,  // EntityKindOneRandomSpawnTrigger
        0x0044bfd0L,  // ConfiguredMonsterSpawnBehaviorTick
        0x0045a240L,  // ScrollPanelControlsInitialize
        0x0047ded0L,  // DirectDrawSurfaceUnlock
        0x0048c010L,  // WadStreamLengthGet
        0x0047ebe0L,  // AviStreamFrameCountGet
        0x00495a60L,  // CrtUnwindAddressResolve
        0x0044cf70L,  // ScenePositionResetAndEffectMotionCancel
        0x0044da20L,  // RoutePositionBlockedStateSet
        0x00435580L,  // GlobalHandlePositionApplyIfPresent
        0x00435aa0L,  // PanelSelectionHighlightToggle
        0x004374d0L,  // TaskCounterIncrementByBranch
        0x004374f0L,  // EntityCallbackDispatchUnlessTypeFive
        0x00438a60L,  // SpatialObjectQueryCenterMask01
        0x00438ae0L,  // SpatialObjectQueryCenterMask02
        0x00438b80L,  // SpatialObjectQueryCenterMaskFromArgument
        0x00438bf0L,  // SpatialObjectQueryCenterMask80
        0x00438cd0L,  // SpatialObjectQueryCenterMask20
        0x00438d30L,  // SpatialObjectQueryCenterMask10
        0x00438d90L,  // SpatialObjectQueryCenterMask200
        0x00438e50L,  // SpatialObjectQueryCenterMask100
        0x00439200L,  // TaskKindFilteredSearch
        0x00439240L,  // TaskEntityFlagsAndCellTaskMatch
        0x0043b360L,  // TaskCountAtGridCoordinateQuery
        0x0043b820L,  // TypeTwoEntityEventDispatch
        0x0043b850L,  // TypeTwoEntityPositionedCallbackDispatch
        0x0043c490L,  // TaskTypeAndConditionCheck
        0x004407b0L,  // SpatialEntityCallbackQuery
        0x004408c0L,  // SpatialEntityCallbackQueryAtOptionalCoordinate
        0x00440c30L,  // GridRouteSearchAdapter
        0x00442540L,  // GridOperationArgumentAdapter
        0x004456e0L,  // EntityBehaviorCallbackTick
        0x00445d70L,  // EntityFlagGatedCallbackDispatch
        0x0044702dL,  // RegisterRelativeStateUpdateFragment
        0x004474f0L,  // LinkedEntityProximityResponse
        0x00447900L,  // TypeFourEntityCallbackDispatch
        0x0044adc0L,  // ScatterSpawnGateSet
        0x0044af90L,  // EntityAttachedSpawnRecordProcess
        0x0044b4c0L,  // TypeTwoEntityTargetResolveAndApply
        0x0044bae0L,  // SceneObjectResourceAndCallbackStateDispose
        0x0044c550L,  // GlobalScatterSpawnEnableAndDrain
        0x0044f490L,  // GridCellOperationApplyAll
        0x00450560L,  // RandomReachableGridPointSelect
        0x00452580L,  // ActiveContextRecordGet
        0x00454880L,  // GlobalCounter_004ff990Increment
        0x00454890L,  // GlobalCounter_004ff990Reset
        0x004567c0L,  // GlobalValue_004ff9bcStoreAndReturn
        0x00459f50L,  // IndexedPanelResourcePositionApply
        0x00459fd0L,  // PanelResourcePositionApplyIfEnabled
        0x0045a3b0L,  // PanelSlotCoordinateHitTest
        0x0045a630L,  // UIRecordCallbackForward
        0x0045aac0L,  // PanelItemFlag10SetClear
        0x0045ab00L,  // PanelStateFlagsToggle
        0x0045abf0L,  // PanelItemFlag8SetIfEnabled
        0x0045b610L,  // GlobalUIFlags_00500860ClearBits_1_2
        0x0045bfb0L,  // ThreeSurfaceResourcesLoadAndInitialize
        0x0045c340L,  // PanelResourceLayoutStore
        0x0045c4b0L,  // PanelResourcePositionUpdateIfEnabled
        0x0045c520L,  // PanelResourceHandleAndDimensionsStore
        0x0045cf00L,  // PriorityLabelLayoutApply
        0x0045d3c0L,  // PriorityPanelRowMoveDispatch
        0x0045d8a0L,  // ObjectField14Set
        0x0045db00L,  // TexturedSceneObjectDrawAtPosition
        0x00460910L,  // ResolvedEntityOperationDispatch
        0x004628a0L,  // GlobalFloatAccumulator_00555330Add
        0x004628e0L,  // CallbackRecordAllocateInitialize
        0x00464c50L,  // SoundDynamicAliasRegistrationSet
        0x00465380L,  // SceneEffectRegistryReset
        0x0046a950L,  // TextLayoutGeometryStoreAndApply
        0x00472670L,  // DirectDrawSurfaceDescriptionGet
        0x004756d0L,  // VideoObjectAspectRatioGet
        0x004757f0L,  // TypedChildResourceScalarGet
        0x00477310L,  // ComPropertyResultGet
        0x00485a00L,  // ObjectStatePairGet
        0x00487d50L,  // ComInterfaceQueryAndContextDispatch
        0x00488bf0L,  // CdAudioStopAndCloseIfActive
        0x00489bc0L,  // ComChildItemsVisit
        0x0048ebb0L,  // CrtExceptionHandlerTransferAfterUnlink
        0x0048ebf0L,  // CrtExceptionHandlerTransferWithAtomicPrefix
        0x0048ec60L,  // CrtExceptionDispatchForwarder
        0x0048ed00L,  // CrtExceptionFrameFilterInvoke
        0x0048ed30L,  // CrtExceptionFrameInstallAndDispatch
        0x0048ee00L,  // CrtExceptionScopeFilterAndUnwind
        0x0048efe6L,  // CrtNlgFrameStateRecord
        0x00490230L,  // ProcessorFeatureFallbackFalse
        0x004918a0L,  // SmallHeapPageFreeSlotUpdate
        0x00491cc0L,  // SmallHeapBlockResizeInPlace
        0x00491f80L,  // CrtOneArgumentMathErrorHandlingStart
        0x0049200cL,  // CrtFpuPrecisionModeCodeGet
        0x00492068L,  // DoubleExponentBitsGetOrSpecialValue
        0x0049221eL,  // CrtFloatingPointExceptionResultDispatch
        0x00492225L,  // CrtFloatingPointExceptionResultDispatchVariant
        0x00492bc6L,  // CrtDoubleRemainderReduction
        0x00493170L,  // CrtMathExceptionHandleSingleResult
        0x004931c0L,  // CrtMathExceptionHandleResultWithStatus
        0x004938f0L,  // CrtMathExceptionDispatch
        0x004939a0L,  // CrtMathErrorCodeSet
        0x00493c40L,  // CrtNoOpStubVariant2
        0x00495410L,  // CrtSehExceptionDispatcher
        0x004954e0L,  // CrtSehLocalUnwindToScope
        0x004955c0L,  // CrtSehUnwindAndInvokeHandler
        0x00495650L,  // CrtSehHandlerCallWithFrame
        0x0049573aL,  // CrtSehAbnormalTerminationCleanup
        0x004959e0L,  // CrtSehDestructorInvoke
        0x00495ae0L,  // CrtTerminateHandlerInvokeAndAbort
        0x00495b70L,  // CrtUnexpectedHandlerInvokeAndTerminate
        0x00496895L,  // CrtLocalUnwindFromFrame
        0x004968b0L,  // CrtRuntimeErrorMessageEnterExit
        0x00497770L,  // CrtFpuControlWordUpdate
        0x004977b0L,  // CrtFpuControlWordUpdateMask
        0x00497d60L,  // CrtExtendedFloatConvertFormatA
        0x00497d80L,  // CrtExtendedFloatConvertFormatB
        0x00497da0L,  // CrtDoubleToExtendedFloatConvertA
        0x00497de0L,  // CrtDoubleToExtendedFloatConvertB
        0x00498ce0L,  // CrtScanfNumericConversionAssign
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
        "MoviePlayNamed", "MoviePlayLevelVideo", "SaveSlotSelectionClear",
        "SaveMenuLayoutLoad", "StreamPlayLevel", "StreamPlayTutorial",
        "OptionsWriteToSlot", "OptionsLoadOrDefault", "FrontEndStateChange",
        "MenuEnter", "FrontEndActive", "SaveSlotOpen", "FrontEndSelectedAction",
        "QuitMenuProbe", "LevelStartFlag", "MenuOptionsFlag", "StartLevelPilots",
        "StartLevelSelected" };

        0x00403a10L,  // TaskCallbackDispatch
        0x0040c1b0L,  // IndexedResourceHandleSetCreate
        0x0040c720L,  // GlobalResourceHandleLoadAndEnable
        0x0040c920L,  // ResourceHandleLoadWithFallback
        0x00412190L,  // MenuAudioTickIntervalReached
        0x00413f30L,  // FilenameExtensionClassify
        0x00417f20L,  // OptionsContextSixFieldSet
        0x004180c0L,  // OptionsContextCopyAndReplaceResource
        0x004196f0L,  // IncidentMessageQueueAppend
        0x004197b0L,  // IncidentMessageQueueRemoveAt
        0x00419a80L,  // IncidentMessageNodeCreate
        0x00419ae0L,  // IncidentMessageFlagSet
        0x00419da0L,  // IncidentMessagePanelRefreshIfActive
        0x0041a250L,  // ScrollOffsetFromCursorUpdate
        0x0041a290L,  // ScreenRightEdgeCacheRefresh
        0x0041a2f0L,  // IncidentMessageStateGet
        0x0041ba90L,  // SimulationGlobalWordSet
        0x00436ac9L,  // RendererNodeRelativePositionSet
        0x00435e35L,  // ModelTransformCopyToSceneNode
        0x0045d4f0L,  // ConfigEntryEnabledToggleAtIndex
        0x00462ed0L,  // GraphicsOverlayResourcesRelease
        0x004634b0L,  // GraphicsOverlayVisibilitySet
        0x00465970L,  // TransientEffectRegistryReleaseAll
        0x004661c0L,  // GridCellRecordBufferAllocate
        0x0046c130L,  // SceneObjectSetAnimationWithFallback
        0x0046d1a0L,  // SceneObjectAttachedResourceRelease
        0x0046db60L,  // ObjectRecordField11CGet
        0x00480870L,  // TaskCallbackRecordAllocateAndInitialize
        0x00482470L,  // IndexedGeometryDataCopyToOutput
        0x0047e4c0L,  // AviPlaybackStateInitialize
        0x0047ed70L,  // InterfaceInvokeOneIfNonNull
        0x00483140L,  // IndexedRecordInterfaceRelease
        0x00483170L,  // IndexedRecordTransformInitialize
        0x00483240L,  // IndexedRecordTransformCopyIn
        0x00483400L,  // IndexedRecordDataAddressGet
        0x00483540L,  // Matrix4x4IdentityInitialize
        0x00483580L,  // RenderInterfaceCapabilityCheck
        0x004078c0L,  // EntityCursorTargetStepResolveAndTimerAdvance
        0x0040e8d0L,  // ScreenSpaceEffectDispatchIfEnabled
        0x0040b850L,  // PackedGridCellValueGet
        0x00406400L,  // PendingRecordCopyToEntityAndFlag
        0x004063e0L,  // PendingRecordEntityReferenceClear
        0x00408620L,  // SceneObjectRecordCloneRetainHandles
        0x00408670L,  // SceneObjectPairApplyMode
        0x004086c0L,  // SceneObjectPairSetRenderParameters
        0x00408710L,  // SceneObjectPairSetCallbackScalar
        0x00408770L,  // SceneObjectIndexedHandleGetOrCreate
        0x004087b0L,  // SceneObjectPrimaryHandleGetOrCreate
        0x004087e0L,  // SceneObjectSecondaryHandleGetOrCreate
        0x00408810L,  // SceneObjectIndexedSecondaryHandleGetOrCreate
        0x00408850L,  // SceneObjectFlagsGet
        0x00408b60L,  // PointProjectOntoLine2D
        0x00406980L,  // SceneObjectModeIsOne
        0x004082e0L,  // SceneObjectScalarAdjustByDirection
        0x00439cd0L,  // EntityAnimationStateSet
        0x0043a8f0L,  // SimulationUpdateCounterAdd
        0x0043a910L,  // SimulationModeCounterAdd
        0x0043a9e0L,  // EntityQueueForRockfallProcessingIfEligible
        0x0044f220L,  // SceneObjectResourceHandleSet
        0x004525e0L,  // SelectionRecordArrayGet
        0x004525f0L,  // SelectionRecordCountGet
        0x00452600L,  // SelectionRecordCountReset
        0x0046c110L,  // AnimationResourceAdvanceTime
        0x0046cf20L,  // SceneEntityAnimationTracksSetEnabled
        0x00466230L,  // GridCellRecordBufferReplace
        0x00480780L,  // ZeroResultIsSuccess
        0x004807a0L,  // FixedSizePoolRecordAcquire
        0x004807d0L,  // FixedSizePoolRecordRelease
        0x004821a0L,  // IndexedRecordEntryRead
        0x00482350L,  // IndexedRecordField64Set
        0x00482780L,  // IndexedRecordFlag6CBit0Get
        0x004827a0L,  // IndexedRecordFlag6CBit0Set
        0x004827d0L,  // RecordFlag2CBit0Set
        0x00482cb0L,  // RendererCommandModeAndPairSend
        0x00482ce0L,  // RendererCommandNodePrepend
        0x004850c0L,  // IndexedWordTableLookup
        0x004851b0L,  // FloatPairAtOffset4SetFromInts
        0x004851d0L,  // FloatPairAtOffsetCSetFromUInts
        0x004891d0L,  // MmioHandleAndGlobalBufferRelease
        0x00489d70L,  // ObjectField38Release
        0x0048f970L,  // FileTimeToLocalDateTimeStamp
        0x0048fb50L,  // DriveLetterRootTypeAvailable
        0x0048e030L,  // ByteRangeSwap
        0x004976d0L,  // DbcsAwareBoundedStringCopy
        0x0048be00L,  // FixedRecordSlotAllocate
        0x0048bfc0L,  // FixedRecordSlotResourceRelease
        0x0048bff0L,  // FixedRecordSlotResourceGet
        0x00409350L,  // PathRecordAllocateAndLink
        0x00408bf0L,  // Point2DProjectOntoDirection
        0x0040a5c0L,  // QuadMeshRenderRecordPoolSlotAllocate
        0x0040aae0L,  // EntityQuadMeshRenderRecordAllowed
        0x00409f20L,  // EntityPlacementCommandsBuildOnce
        0x0040b410L,  // TerrainCellResourceGridDispose
        0x0040b810L,  // CellVisualChannelTableResourcesClear
        0x0040b7d0L,  // CellVisualChannelTableDispose
        0x0040be10L,  // SceneLightCreateAtPosition
        0x0040c4d0L,  // SceneLightPoolExpireSweep
        0x0040c520L,  // SceneLightSecondaryPoolExpireSweep
        0x0040c6c0L,  // RockFallStyleSceneResourcesRelease
        0x0040c690L,  // SceneResourceSingletonPoolRelease
        0x0040d0a0L,  // TerrainEntityCellRecordRemove
        0x0040d100L,  // TerrainCellEntityRecordUnlink
        0x0040ee00L,  // CellCoordinateTimestampRemember
        0x0040efb0L,  // CellCoordinateTimestampExists
        0x0040fb00L,  // MatrixRotationComposeXYZ
        0x00410560L,  // ResourceNameCacheGetOrLoad
        0x00410aa0L,  // ResourceHandleListAppend
        0x00410ae0L,  // ResourceHandleListDispose
        0x00411320L,  // TextLayoutOffsetForCentering
        0x00411840L,  // FrontendProgressValueChanged
        0x00413b50L,  // InterfaceResourceHandlesRelease
        0x00418fc0L,  // UiModeTransitionRequest
        0x00419100L,  // UiModeResourcePositionsApply
        0x00419200L,  // UiModeResourceBit40SetClear
        0x004191f0L,  // UiModeResourceBit40Get
        0x0041b1b0L,  // InterfaceBackButtonConfigLoad
        0x0041b270L,  // InterfaceButtonImagesLoad
        0x0044f810L,  // GridCoordinateClampAndStore
        0x00450730L,  // TerrainCellChangeQueueAppend
        0x00450780L,  // TerrainCellChangeQueueFlush
        0x00452650L,  // ArrayFindValueIndex
        0x00456c30L,  // ObjectPoolListHeadGet
        0x00456c60L,  // ObjectPoolListsFreeAll
        0x004589a0L,  // GlobalOptionLowBitsEnabled
        0x0045b930L,  // FloatPairToIntegerPair
        0x00465a90L,  // TransientEffectRegistryEnableSet
        0x0046db70L,  // SceneObjectResourceRelease
        0x0044e860L,  // GridCellSetPayloadIndex
        0x004755b0L,  // SceneFrameScaleSet
        0x0047e3e0L,  // InterfaceMethodResultRecordInitialize
        0x004893a0L,  // CdAudioStopAndClose
        0x00487fc0L,  // KeyframeTimeSegmentAndFractionGet
        0x00485e30L,  // NamedOptionIndexFindCaseInsensitive
        0x00488b90L,  // CdAudioPlaybackAvailable
        0x0048c180L,  // OwnedResourceTreeRelease
        0x00484ba0L,  // PackedTripletTableToWordBuild
        0x00410020L,  // SceneNodePositionSet
        0x00419030L,  // HelpWindowButtonFeedbackUpdate
        0x00419780L,  // IndexedLinkedListEntryGet
        0x00440130L,  // EntityUpdateAndUnregisterAtTile
        0x00440bf0L,  // ObjectListFieldAssignAll
        0x00447780L,  // EntityClearLinkedObjectState
        0x00448800L,  // EntityMoveToCellCoordinates
        0x00450060L,  // GridCellReadSurfaceVector
        0x00450220L,  // GridCellReadSurfaceValues
        0x004524b0L,  // SelectionObjectDetach
        0x0045bf20L,  // CameraZoomStepAdjust
        0x0045b850L,  // UiOverlayCreateConfiguredElement
        0x0045c580L,  // UiOverlayAnimationAdvance
        0x0045c5f0L,  // UiOverlayAnimationApply
        0x0045da90L,  // UiRenderRecordCreate
        0x0045daf0L,  // UiRenderRecordFree
        0x00460290L,  // EntityType4IndexListBuild
        0x00460310L,  // EntityType4IndexListAppend
        0x0046f380L,  // EntityApplyImpulseAndDirection
        0x0043c2b0L,  // PeriodicTimer25Advance
        0x00439c80L,  // EntityIndexedFlagSetAndRefresh
        0x004565f0L,  // EngineModeSet
        0x00474ae0L,  // TerrainDetailEntryGetAtIndex
        0x00474e30L,  // TerrainSurfacePropertySet
        0x00475880L,  // SceneObjectOwnerLinkEnsure
        0x00476dc0L,  // RenderTargetViewportScaleSet
        0x00476f70L,  // SceneObjectQueryValueIfReady
        0x0047ce20L,  // ProjectionOverlayDrawSubmit
        0x00483b00L,  // WorldObjectRenderStateApply
        0x00487990L,  // UIRenderTreeLayoutAndRelease
        0x004880d0L,  // UIRenderElementChildrenEnumerate
        0x00488900L,  // UIRenderTreeEntryProcess
        0x004889a0L,  // UIRenderEntrySubrecordApply
        0x00489750L,  // SceneObjectOwnerLinkRelease
        0x00408e10L,  // CountMatchingCoordinatesInList
        0x00408e50L,  // CreateRepeatedResourceInstances
        0x00409110L,  // ReleaseSelectionGroupResources
        0x00409150L,  // ClearSelectionGroup
        0x00409940L,  // RefreshEightNeighborCells
        0x004099d0L,  // ReleaseAllGroupRecords
        0x0040aea0L,  // EnsureCellEntriesLoaded
        0x0040af40L,  // CellResourceEntryMatches
        0x0040b250L,  // CellFlag4IsSet
        0x0040b2a0L,  // CellFlag100Set
        0x0040b2e0L,  // CellFlag100IsSet
        0x0040bd60L,  // BuildPerspectiveProjectionMatrix
        0x0040c570L,  // ReadCellTablePair
        0x0040c8a0L,  // GetResourceSlotAddressByIndex
        0x0040d170L,  // CreateCellObjectIfAvailable
        0x0040d1f0L,  // DeleteCellObject
        0x0040dd90L,  // ComputeWorldPositionDelta
        0x004114f0L,  // HasType6ItemAtIndex
        0x004119d0L,  // HasActiveResource
        0x00417fe0L,  // CurrentSelectionContextGet
        0x00417ff0L,  // CurrentSelectionContextSet
        0x00418120L,  // IsAverageValueAbove99
        0x00418f80L,  // SelectionModeSetByIndex
        0x00419450L,  // SelectionFlag10SetClear
        0x004194f0L,  // IndexedSelectionFlag20000SetClear
        0x0041c9f0L,  // InputEntryFlag8SetClear
        0x0041ca20L,  // PlayerInputFlag8SetClear
        0x0041ca60L,  // InputEntryFlag8Get
        0x0041ca70L,  // PlayerInputFlag8Get
        0x0041cfb0L,  // CancelActionForMatchingCoordinates
        0x0041e990L,  // CellActionPermissionGet
        0x0041f230L,  // QueueActionRequestType2
        0x0041f270L,  // QueueActionRequestValue
        0x0041f2b0L,  // QueueActionRequestFlag100000
        0x0041f2f0L,  // QueueActionRequestPair
        0x0041f340L,  // QueueActionRequestIndex
        0x0041f3c0L,  // QueueCallbackRequest390
        0x0041f400L,  // QueueCallbackRequest3E0
        0x0041f970L,  // RuntimeFlag40000000Get
        0x0045d370L,  // RegionBoundsSet
        0x0044a390L,  // Type11CellCoordinateAppend
        0x0044c710L,  // SceneObjectTransformStateInit
        0x0044c790L,  // ConfiguredSceneTransformEnable
        0x0046aa90L,  // MessageOverlayStateInit
        0x0047e710L,  // DirectDrawSurfaceDescriptorInitialize
        0x00485fb0L,  // ViewportRectangleGet
        0x00488bb0L,  // CdAudioTrackStart
        0x00408b00L,  // PointToPolylineMinimumSegmentDistance
        0x00408d70L,  // TerrainDescriptorCoordinateCountLimitCheck
        0x0040a3d0L,  // ScreenEffectFlagsClearAndRelease
        0x0040ab30L,  // GridRecordFlagMaskClear
        0x0040f7d0L,  // CallbackStateRecordCreate
        0x0040f850L,  // CallbackStateListDispose
        0x0040fdb0L,  // ThreeValueRecordCopyAndMarkReady
        0x00410280L,  // CallbackOperationRunAndReturnStatus
        0x00410b60L,  // ThreeWordRecordAllocateAndInitialize
        0x00410b90L,  // BoundedFloatRecordCreate
        0x00417280L,  // RegistryValueLookupWithLock
        0x004172d0L,  // RegistryValueIndexLookupOrNotFound
        0x004189d0L,  // RenderTransitionCleanupIfActive
        0x00438720L,  // EntityAvailableResourceSpend
        0x0043bbc0L,  // EntityDetachCarriedObjectAndResetState
        0x004437f0L,  // EntityDispatchEvent31ForPendingFlag
        0x00444620L,  // EntityEnterEvent21StateOnce
        0x00447730L,  // EntityProgressAdvanceBy25
        0x00447b00L,  // EntityDispatchEvent5AndSetFlag
        0x004487c0L,  // EntityClearNestedState8Fields
        0x0044d9a0L,  // VectorWithinAxisTolerance
        0x00451bc0L,  // ResourceListUpdateMatchingKey
        0x00459530L,  // RenderThreePassUpdate
        0x0045abb0L,  // InputDeviceSetFlag20
        0x00463e40L,  // FloatRoundToNearestInt
        0x00466750L,  // GridCellInvalidateAndClearFlags
        0x00496530L,  // CrtCodePageLocaleIdLookup
        0x00496590L,  // CrtCtypeTablesReset
        0x00497040L,  // CrtStreamReadByte
        0x00497570L,  // CrtStreamBufferFlushAndRelease
        0x00499710L,  // WideStringLength
        0x00499730L,  // CrtRuntimeErrorMessageBoxDisplay
        0x00498f50L,  // CrtStreamUngetByte
        0x004965c0L,  // CrtAnsiCodePageTablesInit
        0x004975d0L,  // CrtCharacterToLowerLocale
        0x00498ae0L,  // CrtStreamBufferAllocate
        0x0040ce30L,  // SelectionMarkerResourcesRelease
        0x0041c7b0L,  // InputActionStateUpdate
        0x0041c8f0L,  // InputActionPressedGet
        0x0041c950L,  // InputBindingPressedGet
        0x0041caf0L,  // InputBindingValueGet
        0x0041cb90L,  // InputBindingDefaultValueGet
        0x0041f100L,  // ObjectInteractionFlagsAllow
        0x0041cea0L,  // SelectionObjectCommandUpdate
        0x0041cf20L,  // SelectionTargetCommandUpdate
        0x0041cf80L,  // SelectionMode7CancelAtPoint
        0x0041cfe0L,  // SelectionMode5CancelAtPoint
        0x0043a7c0L,  // EntityActionKindSupported
        0x0043a850L,  // EntityActionRecordConsumeAndCleanup
        0x004351a0L,  // ActionCodeToSoundAlias
        0x00436ae9L,  // SelectionRecordPointGet
        0x00436b0bL,  // SelectionRecordPointSet
        0x004388b0L,  // EntityActionKindPrerequisiteCheck
        0x00439e40L,  // EntityRecordFlag20Query
        0x0043b320L,  // EntityIterationCallbackGuard
        0x00491770L,  // CRTHeapReleaseUnusedPages
        0x00491840L,  // CRTHeapLocateAllocationPage
        0x004850e0L,  // ObjectField10Get
        0x00485160L,  // TextOutputField82CBit2SetClear
        0x0040d4f0L,  // TerrainCellActivationTry
        0x0040cea0L,  // TerrainCellAndNeighborsUpdate
        0x0040c750L,  // TerrainPointObjectCreate
        0x0040c830L,  // TerrainPointObjectsRemoveMatching
        0x0040cc40L,  // TerrainMarkerSetsClear
        0x0040d030L,  // TerrainRecordPoolChunkAllocate
        0x0040e700L,  // TerrainChoiceResourceSelect
        0x0041a850L,  // ResourceHandleSetRelease
        0x0041b9c0L,  // SelectionEntityCodeResolve
        0x0041cab0L,  // IndexedEntityValueGet
        0x0041cce0L,  // SelectionCoordinatesSet
        0x0041e9d0L,  // SelectionRegionParametersSet
        0x0041ec30L,  // EntityPairCountRefresh
        0x0044cff0L,  // UiElementRgbColorSetClamped
        0x0044dd40L,  // EngineLoaderProfileEntriesRelease
        0x0044e0a0L,  // GlobalResourceHandleRelease
        0x0045d550L,  // UiCursorMoveToChoiceEntry
        0x0045d640L,  // UiCursorMoveToElementCenter
        0x00494d10L,  // CrtFileDescriptorClose
        0x00494df0L,  // CrtOwnedStreamBufferRelease
        0x00498f00L,  // CrtCharacterClassFlag8Check
        0x00497f40L,  // DoubleToExtended80
        0x0048e870L,  // DoubleIsFinite
        0x00475830L,  // SceneObjectFirstNonKind6ChildGet
        0x00498b40L,  // CrtFileDescriptorFlag40Get
        0x00495db0L,  // CrtCharacterClassFlagTest
        0x00493150L,  // X87RoundToIntegral
        0x00406df0L,  // RenderNodeSetPositionFromCallback
        0x00408930L,  // ConstantZeroFloatGet
        0x00409a90L,  // WorldObjectCreateWithTransform
        0x0040af90L,  // TerrainCellRecordGet
        0x0040bde0L,  // SceneMarkerCreateAtObjectPosition
        0x0040beb0L,  // SceneMarkerCreateAtPositionOrObject
        0x0040fe00L,  // SceneObjectListUpdateTransforms
        0x0040ff50L,  // SceneObjectListApplyFloatCallback
        0x00410070L,  // SceneObjectListApplyPositionCallback
        0x004100b0L,  // SceneObjectListApplySecondPositionCallback
        0x00418190L,  // SelectionModeStateSet
        0x004181a0L,  // SelectionStateFlagClearBit0
        0x00419150L,  // SelectionStateHandleGet
        0x0041a990L,  // ObjectiveSelectionStateReset
        0x0041b300L,  // MenuSelectionResourceUpdate
        0x0041cc80L,  // CursorTileSelectionUpdateByType
        0x0041d010L,  // SelectionCancelIfCursorHitsObject
        0x004493e0L,  // SceneObjectHasTaskType4_5_7
        0x00449cd0L,  // SceneObjectStateHandleApplyValue
        0x0044b7a0L,  // MatrixApplyRotationAboutZWithOffset
        0x00441ea0L,  // EntityType5FinalizeIfAllowed
        0x00490400L,  // CrtScientificFloatFormatAppend
        0x00490540L,  // CrtFixedFloatFractionFormat
        0x00490640L,  // CrtFloatDecimalFormatPrepare
        0x00495090L,  // CxxExceptionHandlerDispatch
        0x004889d0L,  // ResourceTreeCallbackReferenceChainRelease
        0x00489520L,  // ResourceTreeCallbackContextCreateForEnumerator
        0x004895c0L,  // ResourceTreeCallbackContextCreateForResource
    // Semantic names supported by the findings corpus. Targets without a
    // sufficiently specific documented role keep Ghidra's FUN_<address> name.
    static String documentedName(long address) {
            case 0x00490400: return "CrtScientificFloatFormatAppend";
            case 0x00490540: return "CrtFixedFloatFractionFormat";
            case 0x00490640: return "CrtFloatDecimalFormatPrepare";
            case 0x00495090: return "CxxExceptionHandlerDispatch";
            case 0x004889d0: return "ResourceTreeCallbackReferenceChainRelease";
            case 0x00489520: return "ResourceTreeCallbackContextCreateForEnumerator";
            case 0x004895c0: return "ResourceTreeCallbackContextCreateForResource";
            case 0x00406df0: return "RenderNodeSetPositionFromCallback";
            case 0x00408930: return "ConstantZeroFloatGet";
            case 0x00409a90: return "WorldObjectCreateWithTransform";
            case 0x0040af90: return "TerrainCellRecordGet";
            case 0x0040bde0: return "SceneMarkerCreateAtObjectPosition";
            case 0x0040beb0: return "SceneMarkerCreateAtPositionOrObject";
            case 0x0040fe00: return "SceneObjectListUpdateTransforms";
            case 0x0040ff50: return "SceneObjectListApplyFloatCallback";
            case 0x00410070: return "SceneObjectListApplyPositionCallback";
            case 0x004100b0: return "SceneObjectListApplySecondPositionCallback";
            case 0x00418190: return "SelectionModeStateSet";
            case 0x004181a0: return "SelectionStateFlagClearBit0";
            case 0x00419150: return "SelectionStateHandleGet";
            case 0x0041a990: return "ObjectiveSelectionStateReset";
            case 0x0041b300: return "MenuSelectionResourceUpdate";
            case 0x0041cc80: return "CursorTileSelectionUpdateByType";
            case 0x0041d010: return "SelectionCancelIfCursorHitsObject";
            case 0x004493e0: return "SceneObjectHasTaskType4_5_7";
            case 0x00449cd0: return "SceneObjectStateHandleApplyValue";
            case 0x0044b7a0: return "MatrixApplyRotationAboutZWithOffset";
            case 0x00441ea0: return "EntityType5FinalizeIfAllowed";
            case 0x0040d4f0: return "TerrainCellActivationTry";
            case 0x0040cea0: return "TerrainCellAndNeighborsUpdate";
            case 0x0040c750: return "TerrainPointObjectCreate";
            case 0x0040c830: return "TerrainPointObjectsRemoveMatching";
            case 0x0040cc40: return "TerrainMarkerSetsClear";
            case 0x0040d030: return "TerrainRecordPoolChunkAllocate";
            case 0x0040e700: return "TerrainChoiceResourceSelect";
            case 0x0041a850: return "ResourceHandleSetRelease";
            case 0x0041b9c0: return "SelectionEntityCodeResolve";
            case 0x0041cab0: return "IndexedEntityValueGet";
            case 0x0041cce0: return "SelectionCoordinatesSet";
            case 0x0041e9d0: return "SelectionRegionParametersSet";
            case 0x0041ec30: return "EntityPairCountRefresh";
            case 0x0044cff0: return "UiElementRgbColorSetClamped";
            case 0x0044dd40: return "EngineLoaderProfileEntriesRelease";
            case 0x0044e0a0: return "GlobalResourceHandleRelease";
            case 0x0045d550: return "UiCursorMoveToChoiceEntry";
            case 0x0045d640: return "UiCursorMoveToElementCenter";
            case 0x00494d10: return "CrtFileDescriptorClose";
            case 0x00494df0: return "CrtOwnedStreamBufferRelease";
            case 0x00498f00: return "CrtCharacterClassFlag8Check";
            case 0x00497f40: return "DoubleToExtended80";
            case 0x0048e870: return "DoubleIsFinite";
            case 0x00475830: return "SceneObjectFirstNonKind6ChildGet";
            case 0x00498b40: return "CrtFileDescriptorFlag40Get";
            case 0x00495db0: return "CrtCharacterClassFlagTest";
            case 0x00493150: return "X87RoundToIntegral";
            case 0x00491770: return "CRTHeapReleaseUnusedPages";
            case 0x00491840: return "CRTHeapLocateAllocationPage";
            case 0x004850e0: return "ObjectField10Get";
            case 0x00485160: return "TextOutputField82CBit2SetClear";
            case 0x0040ce30: return "SelectionMarkerResourcesRelease";
            case 0x0041c7b0: return "InputActionStateUpdate";
            case 0x0041c8f0: return "InputActionPressedGet";
            case 0x0041c950: return "InputBindingPressedGet";
            case 0x0041caf0: return "InputBindingValueGet";
            case 0x0041cb90: return "InputBindingDefaultValueGet";
            case 0x0041f100: return "ObjectInteractionFlagsAllow";
            case 0x0041cea0: return "SelectionObjectCommandUpdate";
            case 0x0041cf20: return "SelectionTargetCommandUpdate";
            case 0x0041cf80: return "SelectionMode7CancelAtPoint";
            case 0x0041cfe0: return "SelectionMode5CancelAtPoint";
            case 0x0043a7c0: return "EntityActionKindSupported";
            case 0x0043a850: return "EntityActionRecordConsumeAndCleanup";
            case 0x004351a0: return "ActionCodeToSoundAlias";
            case 0x00436ae9: return "SelectionRecordPointGet";
            case 0x00436b0b: return "SelectionRecordPointSet";
            case 0x004388b0: return "EntityActionKindPrerequisiteCheck";
            case 0x00439e40: return "EntityRecordFlag20Query";
            case 0x0043b320: return "EntityIterationCallbackGuard";
            case 0x004965c0: return "CrtAnsiCodePageTablesInit";
            case 0x004975d0: return "CrtCharacterToLowerLocale";
            case 0x00498ae0: return "CrtStreamBufferAllocate";
            case 0x00498f50: return "CrtStreamUngetByte";
            case 0x00408b00: return "PointToPolylineMinimumSegmentDistance";
            case 0x00408d70: return "TerrainDescriptorCoordinateCountLimitCheck";
            case 0x0040a3d0: return "ScreenEffectFlagsClearAndRelease";
            case 0x0040ab30: return "GridRecordFlagMaskClear";
            case 0x0040f7d0: return "CallbackStateRecordCreate";
            case 0x0040f850: return "CallbackStateListDispose";
            case 0x0040fdb0: return "ThreeValueRecordCopyAndMarkReady";
            case 0x00410280: return "CallbackOperationRunAndReturnStatus";
            case 0x00410b60: return "ThreeWordRecordAllocateAndInitialize";
            case 0x00410b90: return "BoundedFloatRecordCreate";
            case 0x00417280: return "RegistryValueLookupWithLock";
            case 0x004172d0: return "RegistryValueIndexLookupOrNotFound";
            case 0x004189d0: return "RenderTransitionCleanupIfActive";
            case 0x00438720: return "EntityAvailableResourceSpend";
            case 0x0043bbc0: return "EntityDetachCarriedObjectAndResetState";
            case 0x004437f0: return "EntityDispatchEvent31ForPendingFlag";
            case 0x00444620: return "EntityEnterEvent21StateOnce";
            case 0x00447730: return "EntityProgressAdvanceBy25";
            case 0x00447b00: return "EntityDispatchEvent5AndSetFlag";
            case 0x004487c0: return "EntityClearNestedState8Fields";
            case 0x0044d9a0: return "VectorWithinAxisTolerance";
            case 0x00451bc0: return "ResourceListUpdateMatchingKey";
            case 0x00459530: return "RenderThreePassUpdate";
            case 0x0045abb0: return "InputDeviceSetFlag20";
            case 0x00463e40: return "FloatRoundToNearestInt";
            case 0x00466750: return "GridCellInvalidateAndClearFlags";
            case 0x00496530: return "CrtCodePageLocaleIdLookup";
            case 0x00496590: return "CrtCtypeTablesReset";
            case 0x00497040: return "CrtStreamReadByte";
            case 0x00497570: return "CrtStreamBufferFlushAndRelease";
            case 0x00499710: return "WideStringLength";
            case 0x00499730: return "CrtRuntimeErrorMessageBoxDisplay";
            case 0x00408e10: return "CountMatchingCoordinatesInList";
            case 0x00408e50: return "CreateRepeatedResourceInstances";
            case 0x00409110: return "ReleaseSelectionGroupResources";
            case 0x00409150: return "ClearSelectionGroup";
            case 0x00409940: return "RefreshEightNeighborCells";
            case 0x004099d0: return "ReleaseAllGroupRecords";
            case 0x0040aea0: return "EnsureCellEntriesLoaded";
            case 0x0040af40: return "CellResourceEntryMatches";
            case 0x0040b250: return "CellFlag4IsSet";
            case 0x0040b2a0: return "CellFlag100Set";
            case 0x0040b2e0: return "CellFlag100IsSet";
            case 0x0040bd60: return "BuildPerspectiveProjectionMatrix";
            case 0x0040c570: return "ReadCellTablePair";
            case 0x0040c8a0: return "GetResourceSlotAddressByIndex";
            case 0x0040d170: return "CreateCellObjectIfAvailable";
            case 0x0040d1f0: return "DeleteCellObject";
            case 0x0040dd90: return "ComputeWorldPositionDelta";
            case 0x004114f0: return "HasType6ItemAtIndex";
            case 0x004119d0: return "HasActiveResource";
            case 0x00417fe0: return "CurrentSelectionContextGet";
            case 0x00417ff0: return "CurrentSelectionContextSet";
            case 0x00418120: return "IsAverageValueAbove99";
            case 0x00418f80: return "SelectionModeSetByIndex";
            case 0x00419450: return "SelectionFlag10SetClear";
            case 0x004194f0: return "IndexedSelectionFlag20000SetClear";
            case 0x0041c9f0: return "InputEntryFlag8SetClear";
            case 0x0041ca20: return "PlayerInputFlag8SetClear";
            case 0x0041ca60: return "InputEntryFlag8Get";
            case 0x0041ca70: return "PlayerInputFlag8Get";
            case 0x0041cfb0: return "CancelActionForMatchingCoordinates";
            case 0x0041e990: return "CellActionPermissionGet";
            case 0x0041f230: return "QueueActionRequestType2";
            case 0x0041f270: return "QueueActionRequestValue";
            case 0x0041f2b0: return "QueueActionRequestFlag100000";
            case 0x0041f2f0: return "QueueActionRequestPair";
            case 0x0041f340: return "QueueActionRequestIndex";
            case 0x0041f3c0: return "QueueCallbackRequest390";
            case 0x0041f400: return "QueueCallbackRequest3E0";
            case 0x0041f970: return "RuntimeFlag40000000Get";
            case 0x0045d370: return "RegionBoundsSet";
            case 0x0044a390: return "Type11CellCoordinateAppend";
            case 0x0044c710: return "SceneObjectTransformStateInit";
            case 0x0044c790: return "ConfiguredSceneTransformEnable";
            case 0x0046aa90: return "MessageOverlayStateInit";
            case 0x0047e710: return "DirectDrawSurfaceDescriptorInitialize";
            case 0x00485fb0: return "ViewportRectangleGet";
            case 0x00488bb0: return "CdAudioTrackStart";
            case 0x00410020: return "SceneNodePositionSet";
            case 0x00419030: return "HelpWindowButtonFeedbackUpdate";
            case 0x00419780: return "IndexedLinkedListEntryGet";
            case 0x00440130: return "EntityUpdateAndUnregisterAtTile";
            case 0x00440bf0: return "ObjectListFieldAssignAll";
            case 0x00447780: return "EntityClearLinkedObjectState";
            case 0x00448800: return "EntityMoveToCellCoordinates";
            case 0x00450060: return "GridCellReadSurfaceVector";
            case 0x00450220: return "GridCellReadSurfaceValues";
            case 0x004524b0: return "SelectionObjectDetach";
            case 0x0045bf20: return "CameraZoomStepAdjust";
            case 0x0045b850: return "UiOverlayCreateConfiguredElement";
            case 0x0045c580: return "UiOverlayAnimationAdvance";
            case 0x0045c5f0: return "UiOverlayAnimationApply";
            case 0x0045da90: return "UiRenderRecordCreate";
            case 0x0045daf0: return "UiRenderRecordFree";
            case 0x00460290: return "EntityType4IndexListBuild";
            case 0x00460310: return "EntityType4IndexListAppend";
            case 0x0046f380: return "EntityApplyImpulseAndDirection";
            case 0x0043c2b0: return "PeriodicTimer25Advance";
            case 0x00439c80: return "EntityIndexedFlagSetAndRefresh";
            case 0x004565f0: return "EngineModeSet";
            case 0x00474ae0: return "TerrainDetailEntryGetAtIndex";
            case 0x00474e30: return "TerrainSurfacePropertySet";
            case 0x00475880: return "SceneObjectOwnerLinkEnsure";
            case 0x00476dc0: return "RenderTargetViewportScaleSet";
            case 0x00476f70: return "SceneObjectQueryValueIfReady";
            case 0x0047ce20: return "ProjectionOverlayDrawSubmit";
            case 0x00483b00: return "WorldObjectRenderStateApply";
            case 0x00487990: return "UIRenderTreeLayoutAndRelease";
            case 0x004880d0: return "UIRenderElementChildrenEnumerate";
            case 0x00488900: return "UIRenderTreeEntryProcess";
            case 0x004889a0: return "UIRenderEntrySubrecordApply";
            case 0x00489750: return "SceneObjectOwnerLinkRelease";
            case 0x00409350: return "PathRecordAllocateAndLink";
            case 0x00408bf0: return "Point2DProjectOntoDirection";
            case 0x0040a5c0: return "QuadMeshRenderRecordPoolSlotAllocate";
            case 0x0040aae0: return "EntityQuadMeshRenderRecordAllowed";
            case 0x00409f20: return "EntityPlacementCommandsBuildOnce";
            case 0x0040b410: return "TerrainCellResourceGridDispose";
            case 0x0040b810: return "CellVisualChannelTableResourcesClear";
            case 0x0040b7d0: return "CellVisualChannelTableDispose";
            case 0x0040be10: return "SceneLightCreateAtPosition";
            case 0x0040c4d0: return "SceneLightPoolExpireSweep";
            case 0x0040c520: return "SceneLightSecondaryPoolExpireSweep";
            case 0x0040c6c0: return "RockFallStyleSceneResourcesRelease";
            case 0x0040c690: return "SceneResourceSingletonPoolRelease";
            case 0x0040d0a0: return "TerrainEntityCellRecordRemove";
            case 0x0040d100: return "TerrainCellEntityRecordUnlink";
            case 0x0040ee00: return "CellCoordinateTimestampRemember";
            case 0x0040efb0: return "CellCoordinateTimestampExists";
            case 0x0040fb00: return "MatrixRotationComposeXYZ";
            case 0x00410560: return "ResourceNameCacheGetOrLoad";
            case 0x00410aa0: return "ResourceHandleListAppend";
            case 0x00410ae0: return "ResourceHandleListDispose";
            case 0x00411320: return "TextLayoutOffsetForCentering";
            case 0x00411840: return "FrontendProgressValueChanged";
            case 0x00413b50: return "InterfaceResourceHandlesRelease";
            case 0x00418fc0: return "UiModeTransitionRequest";
            case 0x00419100: return "UiModeResourcePositionsApply";
            case 0x00419200: return "UiModeResourceBit40SetClear";
            case 0x004191f0: return "UiModeResourceBit40Get";
            case 0x0041b1b0: return "InterfaceBackButtonConfigLoad";
            case 0x0041b270: return "InterfaceButtonImagesLoad";
            case 0x0044f810: return "GridCoordinateClampAndStore";
            case 0x00450730: return "TerrainCellChangeQueueAppend";
            case 0x00450780: return "TerrainCellChangeQueueFlush";
            case 0x00452650: return "ArrayFindValueIndex";
            case 0x00456c30: return "ObjectPoolListHeadGet";
            case 0x00456c60: return "ObjectPoolListsFreeAll";
            case 0x004589a0: return "GlobalOptionLowBitsEnabled";
            case 0x0045b930: return "FloatPairToIntegerPair";
            case 0x00465a90: return "TransientEffectRegistryEnableSet";
            case 0x0046db70: return "SceneObjectResourceRelease";
            case 0x0044e860: return "GridCellSetPayloadIndex";
            case 0x004755b0: return "SceneFrameScaleSet";
            case 0x0047e3e0: return "InterfaceMethodResultRecordInitialize";
            case 0x004893a0: return "CdAudioStopAndClose";
            case 0x00487fc0: return "KeyframeTimeSegmentAndFractionGet";
            case 0x00485e30: return "NamedOptionIndexFindCaseInsensitive";
            case 0x00488b90: return "CdAudioPlaybackAvailable";
            case 0x0048c180: return "OwnedResourceTreeRelease";
            case 0x00484ba0: return "PackedTripletTableToWordBuild";
            case 0x004078c0: return "EntityCursorTargetStepResolveAndTimerAdvance";
            case 0x0040e8d0: return "ScreenSpaceEffectDispatchIfEnabled";
            case 0x0040b850: return "PackedGridCellValueGet";
            case 0x00406400: return "PendingRecordCopyToEntityAndFlag";
            case 0x004063e0: return "PendingRecordEntityReferenceClear";
            case 0x00408620: return "SceneObjectRecordCloneRetainHandles";
            case 0x00408670: return "SceneObjectPairApplyMode";
            case 0x004086c0: return "SceneObjectPairSetRenderParameters";
            case 0x00408710: return "SceneObjectPairSetCallbackScalar";
            case 0x00408770: return "SceneObjectIndexedHandleGetOrCreate";
            case 0x004087b0: return "SceneObjectPrimaryHandleGetOrCreate";
            case 0x004087e0: return "SceneObjectSecondaryHandleGetOrCreate";
            case 0x00408810: return "SceneObjectIndexedSecondaryHandleGetOrCreate";
            case 0x00408850: return "SceneObjectFlagsGet";
            case 0x00408b60: return "PointProjectOntoLine2D";
            case 0x00406980: return "SceneObjectModeIsOne";
            case 0x004082e0: return "SceneObjectScalarAdjustByDirection";
            case 0x00439cd0: return "EntityAnimationStateSet";
            case 0x0043a8f0: return "SimulationUpdateCounterAdd";
            case 0x0043a910: return "SimulationModeCounterAdd";
            case 0x0043a9e0: return "EntityQueueForRockfallProcessingIfEligible";
            case 0x0044f220: return "SceneObjectResourceHandleSet";
            case 0x004525e0: return "SelectionRecordArrayGet";
            case 0x004525f0: return "SelectionRecordCountGet";
            case 0x00452600: return "SelectionRecordCountReset";
            case 0x0046c110: return "AnimationResourceAdvanceTime";
            case 0x0046cf20: return "SceneEntityAnimationTracksSetEnabled";
            case 0x00466230: return "GridCellRecordBufferReplace";
            case 0x00480780: return "ZeroResultIsSuccess";
            case 0x004807a0: return "FixedSizePoolRecordAcquire";
            case 0x004807d0: return "FixedSizePoolRecordRelease";
            case 0x004821a0: return "IndexedRecordEntryRead";
            case 0x00482350: return "IndexedRecordField64Set";
            case 0x00482780: return "IndexedRecordFlag6CBit0Get";
            case 0x004827a0: return "IndexedRecordFlag6CBit0Set";
            case 0x004827d0: return "RecordFlag2CBit0Set";
            case 0x00482cb0: return "RendererCommandModeAndPairSend";
            case 0x00482ce0: return "RendererCommandNodePrepend";
            case 0x004850c0: return "IndexedWordTableLookup";
            case 0x004851b0: return "FloatPairAtOffset4SetFromInts";
            case 0x004851d0: return "FloatPairAtOffsetCSetFromUInts";
            case 0x004891d0: return "MmioHandleAndGlobalBufferRelease";
            case 0x00489d70: return "ObjectField38Release";
            case 0x0048f970: return "FileTimeToLocalDateTimeStamp";
            case 0x0048fb50: return "DriveLetterRootTypeAvailable";
            case 0x0048e030: return "ByteRangeSwap";
            case 0x004976d0: return "DbcsAwareBoundedStringCopy";
            case 0x0048be00: return "FixedRecordSlotAllocate";
            case 0x0048bfc0: return "FixedRecordSlotResourceRelease";
            case 0x0048bff0: return "FixedRecordSlotResourceGet";
            case 0x00403a10: return "TaskCallbackDispatch";
            case 0x0040c1b0: return "IndexedResourceHandleSetCreate";
            case 0x0040c720: return "GlobalResourceHandleLoadAndEnable";
            case 0x0040c920: return "ResourceHandleLoadWithFallback";
            case 0x00412190: return "MenuAudioTickIntervalReached";
            case 0x00413f30: return "FilenameExtensionClassify";
            case 0x00417f20: return "OptionsContextSixFieldSet";
            case 0x004180c0: return "OptionsContextCopyAndReplaceResource";
            case 0x004196f0: return "IncidentMessageQueueAppend";
            case 0x004197b0: return "IncidentMessageQueueRemoveAt";
            case 0x00419a80: return "IncidentMessageNodeCreate";
            case 0x00419ae0: return "IncidentMessageFlagSet";
            case 0x00419da0: return "IncidentMessagePanelRefreshIfActive";
            case 0x0041a250: return "ScrollOffsetFromCursorUpdate";
            case 0x0041a290: return "ScreenRightEdgeCacheRefresh";
            case 0x0041a2f0: return "IncidentMessageStateGet";
            case 0x0041ba90: return "SimulationGlobalWordSet";
            case 0x00436ac9: return "RendererNodeRelativePositionSet";
            case 0x00435e35: return "ModelTransformCopyToSceneNode";
            case 0x0045d4f0: return "ConfigEntryEnabledToggleAtIndex";
            case 0x00462ed0: return "GraphicsOverlayResourcesRelease";
            case 0x004634b0: return "GraphicsOverlayVisibilitySet";
            case 0x00465970: return "TransientEffectRegistryReleaseAll";
            case 0x004661c0: return "GridCellRecordBufferAllocate";
            case 0x0046c130: return "SceneObjectSetAnimationWithFallback";
            case 0x0046d1a0: return "SceneObjectAttachedResourceRelease";
            case 0x0046db60: return "ObjectRecordField11CGet";
            case 0x00480870: return "TaskCallbackRecordAllocateAndInitialize";
            case 0x00482470: return "IndexedGeometryDataCopyToOutput";
            case 0x0047e4c0: return "AviPlaybackStateInitialize";
            case 0x0047ed70: return "InterfaceInvokeOneIfNonNull";
            case 0x00483140: return "IndexedRecordInterfaceRelease";
            case 0x00483170: return "IndexedRecordTransformInitialize";
            case 0x00483240: return "IndexedRecordTransformCopyIn";
            case 0x00483400: return "IndexedRecordDataAddressGet";
            case 0x00483540: return "Matrix4x4IdentityInitialize";
            case 0x00483580: return "RenderInterfaceCapabilityCheck";
            case 0x004017b0: return "AdvisorPositionRecordInitialize";
            case 0x004065f0: return "CubicBezierPointEvaluate";
            case 0x00410320: return "StringQueueRecordOrFlush";
            case 0x00410a10: return "MenuItemChoiceStringsAllocate";
            case 0x00410bf0: return "MenuItemNumericRangeRecordAllocate";
            case 0x004198f0: return "TerrainCellLinkedEntriesRemove";
            case 0x0041a1a0: return "UIEntryHitTestAndDispatch";
            case 0x0044da50: return "ThreeChannelValueTweenStep";
            case 0x00453920: return "AdvisorScrollStepAdvance";
            case 0x00459900: return "HungerImagesPositionConfigLoad";
            case 0x00459a50: return "BubbleImagesPositionConfigLoad";
            case 0x0045b620: return "PriorityListButtonActionDispatch";
            case 0x00475c80: return "DDiScenePayloadRelease";
            case 0x00477370: return "DDiSceneRenderSurfacePoolGrow";
            case 0x0047d430: return "SceneObjectRegistryTeardown";

        switch ((int) address) {
            case 0x004034e0: return "TaskPairRecordEnqueue";
            case 0x00403650: return "EntityTaskBatchAppend";
            case 0x004040a0: return "EntityTaskQueueFlush";
            case 0x00403af0: return "EntityTaskStateUpdateAfterCleanup";
            case 0x00435ebb: return "ScreenEffectRecordSet";
            case 0x0045bf70: return "UIEntryBoundsGet";
            case 0x0045d470: return "UIListEntriesSwap";
            case 0x0045e400: return "EntityTypeAndStateFlagPredicate";
            case 0x00462da0: return "IndexedSurfaceResourceAllocate";
            case 0x004634f0: return "IndexedSurfaceEntryHideAndRecord";
            case 0x00463540: return "IndexedSurfaceEntriesRestore";
            case 0x004636b0: return "InfoPanelStringEntriesInitialize";
            case 0x00464720: return "SurfaceVisibilitySet";
            case 0x00464e80: return "DeferredSoundPlayExpire";
            case 0x00464ec0: return "DeferredSoundTimerSetIfIdle";
            case 0x0046d1c0: return "AnimationDurationForLoopCount";
            case 0x0046f1d0: return "EntityIndexedFloatLookupWithMinusOneFallback";
            case 0x0047e0c0: return "FixedSizeRecordPoolGrow";
            case 0x004807f0: return "RenderResourceInstancePoolGrow";
            case 0x00488730: return "CaseInsensitivePairTableFind";
            case 0x00488790: return "CaseInsensitivePairTableAppend";
            case 0x0048b4a0: return "BufferedCharStreamReadToString";
            case 0x00401bf0: return "GlobalField505f88Get";
            case 0x00401210: return "AdvisorSceneVisualStateInitialize";
            case 0x004033f0: return "CoordinatePairTaskRecordQueue";
            case 0x004034a0: return "ScalarTaskRecordQueue";
            case 0x00403520: return "TaskRecordKind10Queue";
            case 0x004038c0: return "TimedTaskRecordInitializeAndLink";
            case 0x004039a0: return "TaskRecordCreateAndScanGrid";
            case 0x004039d0: return "TaskRecordKind11TimedQueue";
            case 0x00403ef0: return "PendingRecordCallbackDispatch";
            case 0x00403f60: return "TaskRecordKind14RemoveIfHead";
            case 0x0040fe40: return "PointerStackPush";
            case 0x0040fe60: return "PointerStackPop";
            case 0x0040fe90: return "PointerStackForEach";
            case 0x0040fcc0: return "VectorScaleComponentsInPlace";
            case 0x00408360: return "TaskField84GetAndField88Out";
            case 0x00408970: return "TaskFlagBitQuery";
            case 0x0040f5e0: return "GlobalConfigValueSet";
            case 0x0046a330: return "EntityRecordFlag400ToggleAndRefresh";
            case 0x0046c3d0: return "RecordFieldD8EqualsOne";
            case 0x0046c340: return "IndexedRecordListRebuildForValue";
            case 0x0046cf40: return "RecordFlagBitTest";
            case 0x0046cf90: return "RecordIndexedEntriesRebuildAndRefresh";
            case 0x0046d260: return "SceneResourceSlotsRebuild";
            case 0x0046d2c0: return "SceneResourceVisibilitySet";
            case 0x0046d9f0: return "SceneResourceNamedEntryResolveWithFallback";
            case 0x0046da90: return "SceneResourceCachedSlotB4Get";
            case 0x0046dac0: return "SceneResourceCachedSlotB8Get";
            case 0x0046daf0: return "SceneResourceCachedSlotC0Get";
            case 0x0046d350: return "SceneResourceDrawAtOrigin";
            case 0x0046e330: return "GridSurfaceEntryReset";
            case 0x0043c650: return "EntityCapacityOrStatePredicate";
            case 0x0043b6c0: return "ClippedRectangleCallbackDispatch";
            case 0x0043a9f0: return "EntityPairInteractionGate";
            case 0x0046e1c0: return "GlobalEntityRenderParameterModeSet";
            case 0x0046a000: return "Entity334FloatField90Get";
            case 0x0046a020: return "Entity334FloatField98Get";
            case 0x0046a040: return "Entity334FloatField94Get";
            case 0x0046a080: return "Entity334FloatFieldF4Get";
            case 0x0046a170: return "Entity334FloatField130Get";
            case 0x0046a190: return "Entity334FloatField134Get";
            case 0x0046a1b0: return "Entity334FloatField120Get";
            case 0x0046a1d0: return "Entity334FloatField128Get";
            case 0x0046a1f0: return "Entity334FloatField124Get";
            case 0x0046a210: return "Entity334WordField13CGet";
            case 0x0046a230: return "Entity334FloatField12CGet";
            case 0x0046a250: return "Entity334FloatField138Get";
            case 0x0046a290: return "Entity334FloatField114Get";
            case 0x0046a2b0: return "Entity334FloatField118Get";
            case 0x0046a2d0: return "Entity334FloatField11CGet";
            case 0x0046a2f0: return "Entity334FloatField110Get";
            case 0x0046a310: return "Entity334FloatField10CGet";
            case 0x0046a0c0: return "MappedIndexedRecordWordAtFCGet";
            case 0x0046a0f0: return "MappedIndexedRecordWordAt100Get";
            case 0x0047a580: return "FixedSizeRecordPoolAcquire";
            case 0x0047b2e0: return "NestedRecordListRemoveByKeyRecursive";
            case 0x0047e480: return "TwoInterfaceFieldsReleaseAndReset";
            case 0x00406360: return "EntityRecordAttachAndRelink";
            case 0x004065d0: return "GlobalDwordGet";
            case 0x00406c00: return "RendererObjectUpdateFromRecordField64";
            case 0x00406c30: return "RendererObjectUpdateFromRecordField68";
            case 0x00406c60: return "RendererObjectUpdateFromRecordField6c";
            case 0x00406cb0: return "RendererRecordClone";
            case 0x00406d30: return "RendererObjectTimingValueAdjustByCount";
            case 0x00406f50: return "RendererObjectSubobjectDispatch";
            case 0x00406fb0: return "RendererCachedHandle40GetOrCreate";
            case 0x00406fe0: return "RendererCachedHandle48GetOrCreate";
            case 0x00407010: return "RendererCachedHandle50GetOrCreate";
            case 0x00407040: return "RendererCachedHandle4cIsNotOne";
            case 0x00401c20: return "GlobalFlagBit0IsSet";
            case 0x004020c0: return "HandleArrayReleaseAndReset";
            case 0x00402a40: return "CoordinatePairCallbackDispatch";
            case 0x00402aa0: return "ArgumentCallbackDispatch";
            case 0x00402b30: return "CoordinateCallbackDispatch";
            case 0x00402b60: return "CoordinateCallbackDispatchByMode";
            case 0x00402ec0: return "EntityConditionalTaskSchedule";
            case 0x004030b0: return "CoordinatePairEventQueue";
            case 0x00403190: return "ObjectEventType2Queue";
            case 0x004033c0: return "ObjectAttachedRecordAllocate";
            case 0x0046b4d0: return "CaseInsensitiveNameTableLookup";
            case 0x00463c00: return "RectContainsPointByConvertedCoords";
            case 0x00463c90: return "RectContainsPointFromObjectBounds";
            case 0x0046c0b0: return "CopyFourFieldsAndConvertSecond";
            case 0x0043c200: return "EntityStatePairPredicate";
            case 0x00442480: return "VectorDirectionAngleRadiansGet";
            case 0x00443670: return "TryRandomEntityActionFromCodeRange";
            case 0x00443710: return "AdjacentPositionCandidateFind";
            case 0x004440a0: return "LinkedEntityFlagAndCommandUpdate";
            case 0x004471b0: return "TryCreateLinkedEntityAtPosition";
            case 0x004502c0: return "CoordinatePairWithinBounds";
            case 0x0045a370: return "CaseInsensitiveStringTableLookup";
            case 0x0045a410: return "CaseInsensitiveIndexedStringLookup";
            case 0x0045b8a0: return "PointDistanceWithinRadius";
            case 0x0045ab30: return "RequestRecordSetFlag4ClearFlag8";
            case 0x0045ab60: return "RequestRecordSetFlag2ClearFlag8";
            case 0x0045ab90: return "RequestRecordFlag8IsSet";
            case 0x00451fd0: return "CaseInsensitiveNameTableLookupAtOffset8";
            case 0x0047e7e0: return "BitCount32ByPosition";
            case 0x0047f0d0: return "CursorSetClientPosition";
            case 0x0047f6a0: return "SaveDirectoryEnsureExists";
            case 0x00402800: return "EntityCommand14BatchIssue";
            case 0x00402670: return "EntityCommand15ValidatedRecordQueue";
            case 0x00409cd0: return "CellCardinalNeighborApply";
            case 0x00402600: return "EntityRecordArrayApplyZeroState";
            case 0x00433e30: return "GridCellEntityEligibilityCheck";
            case 0x00437050: return "EntityPoolResourcesRelease";
            case 0x00437300: return "ProcessQueuedGridAnimations";
            case 0x004378e0: return "GridObjectRecordsRelease";
            case 0x004383a0: return "GlobalAnimationStateSet";
            case 0x0047b550: return "SoundBufferVolumeSetMinus300";
            case 0x0047a800: return "DirectSoundObjectsShutdown";
            case 0x0047a540: return "ObjectFieldBE4DwordGet";
            case 0x004772b0: return "RenderInterfaceTransformPoint";
            case 0x00475e10: return "CallbackRecordSetFlag10AndRefresh";
            case 0x00476f50: return "ResourceInterfaceCallSlot48";
            case 0x00476fd0: return "ResourceInterfaceCallSlot8C";
            case 0x00476ff0: return "ResourceInterfaceCallSlot90";
            case 0x00477250: return "ResourceInterfaceCallSlot40AndClearField14";
            case 0x00477270: return "ResourceInterfaceCallSlot88";
            case 0x00406c90: return "AnimationObjectVisibilitySet";
            case 0x004086a0: return "AnimationObjectVisibilityGet";
            case 0x00406d10: return "AnimationObjectTimeUpdate";
            case 0x00406da0: return "AnimationObjectTimeGet";
            case 0x00406dc0: return "AnimationObjectTransformSet";
            case 0x00406e30: return "AnimationObjectHandleGet";
            case 0x00407090: return "AnimationObjectValueGet";
            case 0x00407360: return "GlobalTaskRefreshFlagGet";
            case 0x004090a0: return "TerrainCellPredicateCheck";
            case 0x0040e460: return "GridCellRecordPresentCheck";
            case 0x0040f7b0: return "OptionalResourceDestroy";
            case 0x00419e60: return "PendingRecordReleaseIfPresent";
            case 0x00419f10: return "PendingRecordReleaseAll";
            case 0x0041b470: return "CategoryValueIsNeitherZeroNorFifteen";
            case 0x00409d40: return "LevelStateStoredValueSet";
            case 0x0040ab10: return "GlobalModeZeroInputSet";
            case 0x0040e7e0: return "GlobalFlagBit0Clear";
            case 0x0040e910: return "GlobalValueClearIfEqual";
            case 0x0046a270: return "EntityCollisionRadiusGet";
            case 0x0046cf80: return "EntityTypeMaskGet";
            case 0x0046d330: return "EntityTypeFlagGetBit1";
            case 0x0046d9e0: return "SceneObjectResourceGet";
            case 0x00462830: return "GlobalFloatAt00554adcAddOne";
            case 0x00462850: return "GlobalFloatAt00554efcAddOne";
            case 0x00462870: return "GlobalCounterAt00555324Increment";
            case 0x00462880: return "GlobalCounterAt00555328Increment";
            case 0x00462890: return "GlobalCounterAt0055532cIncrement";
            case 0x00479e40: return "MatrixIdentityAndTranslationSet";
            case 0x0047b330: return "NestedRecordListRemoveMatchingKey";
            case 0x0047c170: return "WindowStateInitialize";
            case 0x0047d490: return "SurfacePixelFormatQuery";
            case 0x004792c0: return "GlobalRecordListPushHead";
            case 0x0047b0d0: return "IndexedRecordStride134WordGet";
            case 0x0047d410: return "RendererRecordTableInitialize";
            case 0x0047ff30: return "EngineHeapAllocate";
            case 0x0047ff40: return "EngineHeapFree";
            case 0x004026c0: return "EntityDeferredWorkSchedule";
            case 0x00402e40: return "CellCommandType13Issue";
            case 0x00402fb0: return "CellCommandType11Issue";
            case 0x00403030: return "CellCommandType3Issue";
            case 0x00403b40: return "EntityPendingTaskFlagClear";
            case 0x00403b60: return "EntityActivityRecordsRefresh";
            case 0x00435ef4: return "RecordFields4And14Set";
            case 0x00435f0b: return "RecordField14Get";
            case 0x004422a0: return "SurfaceVectorNormalize";
            case 0x0044d250: return "FloatNegateAbsoluteValue";
            case 0x0044e6b0: return "IndexedFlagValueSetOne";
            case 0x00451c30: return "LinkedRecordField4Clear";
            case 0x004532e0: return "IndirectHandleResolveInPlace";
            case 0x00453900: return "DeferredCallbackSet";
            case 0x00453910: return "DeferredCallbackInvokeIfSet";
            case 0x0045a6e0: return "IndexedPairSet";
            case 0x0045a700: return "IndexedPairGet";
            case 0x0045b900: return "IntegerPairDivideToFloatPair";
            case 0x00462ab0: return "RecordField18Set";
            case 0x00462ac0: return "RecordFlags24Or";
            case 0x00463730: return "IndexedFloatPairSet";
            case 0x00465ac0: return "TransientEffectEnableSet";
            case 0x0046c0f0: return "RecordResourceRelease";
            case 0x0046ced0: return "PairedResourceStateSet";
            case 0x0046cfc0: return "SixResourceSlotsSetState";
            case 0x0047d3f0: return "BitCount32";
            case 0x00499690: return "PointerRangeIsReadable";
            case 0x004996b0: return "PointerRangeIsWritable";
            case 0x004996d0: return "FunctionPointerIsExecutable";
            case 0x0047cdd0: return "SurfaceDescriptionGet";
            case 0x0047c1c0: return "DirectDrawEnumerateAndStoreCallbackResult";
            case 0x00479510: return "D3DRMVectorRotate";
            case 0x00478310: return "WindowRectAdjustForWindowedMode";
            case 0x0041a9c0: return "CaseInsensitiveStringTableIndexFind";
            case 0x0041f870: return "GlobalObjectField8CIncrement";
            case 0x0041f880: return "GlobalObjectField8CSubtract";
            case 0x0041f8a0: return "GlobalObjectField98Add";
            case 0x0041f8c0: return "GlobalObjectField98Clear";
            case 0x0041f980: return "GlobalObjectField94Increment";
            case 0x0041f990: return "GlobalObjectFieldA8Increment";
            case 0x0045a5b0: return "UITextOutputPositionAndDraw";
            case 0x0045a620: return "UITextOutputObjectRelease";
            case 0x00462960: return "TaskTableContentsRelease";
            case 0x00465010: return "ConfiguredVariantPropertySet";
            case 0x00465030: return "ConfiguredVariantPropertyRead";
            case 0x0046db20: return "AttachedEntitySlotObjectGet";
            case 0x00463ea0: return "IndexedTextMetricsPairGet";
            case 0x00463ed0: return "IndexedTextMetricsPairSet";
            case 0x00466210: return "EntityFieldGridBufferRelease";
            case 0x00476220: return "RenderInterfacePairUpdate";
            case 0x00476310: return "ScenePropertyValueAt08Get";
            case 0x00401c30: return "DisplayWidthGet";
            case 0x00401c40: return "DisplayHeightGet";
            case 0x00402090: return "PriorityEntryOrderValueSet";
            case 0x004020a0: return "PriorityEntryStateSet";
            case 0x004020b0: return "PriorityEntryStateGet";
            case 0x004021e0: return "EngineMemoryAllocate";
            case 0x00402210: return "EngineMemoryFree";
            case 0x0043c280: return "EntityNestedFloatAddUnlessBlocked";
            case 0x0043c4c0: return "EntityFinishState1A";
            case 0x0043c530: return "EntityMarkAndFinishState1A";
            case 0x0046a420: return "SelectedGlobalPointTransformAndReturnX";
            case 0x0046a470: return "LinkedListPushAtGlobalHead";
            case 0x00469a90: return "IndexedValueAppend";
            case 0x00469ac0: return "IndexedRecordWordAtOffsetB0Get";
            case 0x00469af0: return "IndexedRecordWordAtOffsetB4Get";
            case 0x00469b20: return "IndexedRecordWordAtOffsetB8Get";
            case 0x00469b50: return "EntityIndexedRecordWordAtOffsetBCGet";
            case 0x00469b80: return "EntityIndexedRecordWordAtOffsetE0Get";
            case 0x00469be0: return "EntityIndexedRecordWordAtOffsetF0Get";
            case 0x00469d00: return "NestedFloatAtOffset9CGet";
            case 0x00469d40: return "NestedFloatAtOffset6CGet";
            case 0x00469d50: return "NestedFloatAtOffset74Get";
            case 0x00469d60: return "NestedFloatAtOffset78Get";
            case 0x00469d70: return "NestedFloatAtOffsetE8Get";
            case 0x00469d90: return "NestedFloatAtOffset70Get";
            case 0x00469da0: return "NestedFieldAddressAtOffsetA0Get";
            case 0x00469db0: return "NestedFloatAtOffset7CGet";
            case 0x00469dc0: return "NestedFloatAtOffset80Get";
            case 0x00469e00: return "IndexedRecordFloatAtOffsetDCGet";
            case 0x00469e20: return "NestedFloatAtOffsetDCGet";
            case 0x00469e40: return "NestedWordAtOffsetACGet";
            case 0x00469ec0: return "IndexedRecordWordAtOffset144Get";
            case 0x00469ee0: return "IndexedRecordWordAtOffset148Get";
            case 0x00469f00: return "IndexedRecordWordAtOffset14CGet";
            case 0x00469f60: return "IndexedRecordWordAtOffsetD0Get";
            case 0x00469f90: return "NestedWordAtOffsetC0OrC4Get";
            case 0x00469fe0: return "NestedFloatAtOffset8CGet";
            case 0x00472940: return "SceneRootObjectGet";
            case 0x00476120: return "ScenePropertyPrimaryValueGet";
            case 0x00476140: return "ScenePropertySecondaryValueGet";
            case 0x004761c0: return "ScenePropertyFloatAt14Get";
            case 0x004762f0: return "ScenePropertyOwnedBufferRelease";
            case 0x00476d10: return "RenderResourcePairReleaseAndClear";
            case 0x00476f10: return "RenderSurfaceGetValuePair80And84";
            case 0x00477220: return "RenderResourceTrackAndRetain";
            case 0x004773e0: return "RenderResourceTrackedReferencesRelease";
            case 0x00401840: return "AdvisorActionPositionAndModeSet";
            case 0x00401890: return "AdvisorActionPositionGet";
            case 0x004011c0: return "AdvisorAnimationEndCleanup";
            case 0x00450700: return "GridSurfaceWriteAtXY";
            case 0x00439930: return "CopyTwoDwordPairToOutputs";
            case 0x00465160: return "SoundObjectDurationGet";
            case 0x00476e50: return "ScreenBoundedSurfaceResourceAllocate";
            case 0x00476fb0: return "RendererObjectSetClipDistance";
            case 0x0041f720: return "CurrentLevelStateFlagsContainMask";
            case 0x0041f920: return "LevelStateField9CAdd";
            case 0x0041f9a0: return "LevelResourceBalanceAddAndSound";
            case 0x00436a4a: return "CameraSetDistance";
            case 0x00436921: return "CameraSetScalarBounds";
            case 0x00436b33: return "CameraGetPositionWithTerrainHeight";
            case 0x00436b79: return "CameraRotationGet";
            case 0x004395e0: return "EntityDisplayNameLookup";
            case 0x00439950: return "ToolNameStringResourcesLoad";
            case 0x00482040: return "AnimationTrackApplyWrappedFrameOffset";
            case 0x00483810: return "InputActionStateToggle";
            case 0x00487cb0: return "AnimationTrackFindKeyframeAndFraction";
            case 0x00498fe0: return "CrtFileDescriptorAllocate";
            case 0x004990a0: return "CrtFileDescriptorSetHandle";
            case 0x0049add0: return "BoundedWideStringLengthIncludingNull";
            case 0x004013e0: return "AdvisorPositionNameToIndex";
            case 0x004014c0: return "AdvisorAnimationNameToIndex";
            case 0x00406730: return "Vector2DistanceVariant";
            case 0x00406760: return "Vector2SetLength";
            case 0x00436831: return "CameraSetTiltBounds";
            case 0x00436860: return "CameraSetTilt";
            case 0x00436a18: return "CameraSetDistanceBounds";
            case 0x00436953: return "CameraSetBoundedRotationValue";
            case 0x00435a20: return "GlobalStateLowByteClearAndRefresh";
            case 0x00466380: return "SetExpansionEntityAtGridCell";
            case 0x004663f0: return "CanSpawnExpansionEntityAtGridCell";
            case 0x00465e00: return "AdvanceTimedVisualSegmentAndFade";
            case 0x004599c0: return "LoadObjectHungerMeterImages";
            case 0x00459240: return "PopEntityStateOverride";
            case 0x00475070: return "SceneObjectCoordinateTransformInPlace";
            case 0x00475090: return "SceneObjectCoordinateTransformToOutput";
            case 0x004756b0: return "SceneObjectTransformMatrixSet";
            case 0x00473680: return "SceneObjectWrappedPointerGet";
            case 0x00426250: return "UpdateRenderViewportState";
            case 0x00436831: return "CameraSetTiltBounds";
            case 0x00436860: return "CameraSetTilt";
            case 0x00436a18: return "CameraSetDistanceBounds";
            case 0x00436953: return "CameraSetBoundedRotationValue";
            case 0x00435a20: return "GlobalStateLowByteClearAndRefresh";
            case 0x00471330: return "InterpolatedZWithinBounds";
            case 0x00471a20: return "ReadIndexedPairSlotDword";
            case 0x00471ce0: return "EntityFlag100AndSharedEligibilityCheck";
            case 0x0046f110: return "WeaponTypeGetRechargeTime";
            case 0x0046f140: return "WeaponTypeGetDischargeRate";
            case 0x0046f170: return "WeaponTypeGetRange";
            case 0x0046f1a0: return "WeaponTypeGetWallDestroyTime";
            case 0x00425cd0: return "Global004de548Set";
            case 0x00425d50: return "GlobalRenderRecordSetFromInput";
            case 0x004295a0: return "ContextSelectedGlobalPointerGet";
            case 0x0042b500: return "GlobalStateField84Get";
            case 0x004310f0: return "TerrainCellFlag20ClearAndRefresh";
            case 0x00433be0: return "CurrentLevelTerrainGridPick";
            case 0x00435550: return "GlobalStateZeroTransitionDetect";
            case 0x004370b0: return "RenderConfigSixFieldCopyOut";
            case 0x004370d0: return "RenderConfigSixFieldCopyInOrClear";
            case 0x00437100: return "GridRecordCountDifferenceSum";
            case 0x00437440: return "GridRecordCountTablesClear";
            case 0x00437460: return "GridRecordCountDifferenceGet";
            case 0x004374a0: return "GridRecordCountGet";
            case 0x00437cc0: return "EntityFlag40000ThresholdCompare";
            case 0x00438ed0: return "EntityFlagsContainRequestedSingleBit";
            case 0x0049af60: return "CrtWideEnvironmentStringListConvert";
            case 0x0049af20: return "CrtCountedStringCompare";
            case 0x0049aea0: return "CrtFileDescriptorFlag80SetOrClear";
            case 0x00424810: return "ApplyDefaultTransformToFlaggedRenderObjects";
            case 0x00435190: return "SurfaceTypeConfiguredValueAtIndex";
            case 0x0046c030: return "RenderResourceBindingInitialize";
            case 0x0046ead0: return "AppendMapRowSpanToBuffer";
            case 0x0046eb30: return "StartMapRowSpanBufferRecord";
            case 0x00479110: return "ReadAngleSettingRadians";
            case 0x00479240: return "ReleaseLinkedObjectList";
            case 0x00479a30: return "RayPlaneIntersectionParameter";
            case 0x00479ab0: return "PointAlongRay";
            case 0x00479c10: return "SphereRayIntersectionDiscriminantPositive";
            case 0x00479da0: return "Matrix4SetRotationY";
            case 0x00479df0: return "Matrix4SetRotationZ";
            case 0x00479e70: return "Matrix4SetIdentity";
            case 0x00479ea0: return "Matrix4SetZero";
            case 0x00479eb0: return "Matrix4Copy";
            case 0x00424560: return "AirSupplyAmountUpdate";
            case 0x00438f60: return "DockDefinitionMatchesCell";
            case 0x004399c0: return "EntityFlag400AndHealthThresholdCheck";
            case 0x00439cf0: return "EntityStatsWordFlag10Test";
            case 0x00439d10: return "EntityStatsWordFlag20Test";
            case 0x00439d30: return "EntityStatsWordFlag40Test";
            case 0x0043baf0: return "EntityFlagsMask107C33DEOr1C00203AnyTest";
            case 0x004436f0: return "EntityType3Flag04000000Test";
            case 0x00499150: return "CrtFileDescriptorCloseSlot";
            case 0x0049a050: return "ExtendedFloat96BitShiftLeftOne";
            case 0x0049a080: return "ExtendedFloat96BitShiftRightOne";
            case 0x0049a0b0: return "CrtDecimalDigitSequenceToExtended96";
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
            case 0x48bce0: return "WadArchiveRecordAtIndex";
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
            case 0x4651a0: return "SoundSubsystemFrameTick";
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
            case 0x465b00: return "TransientEffectInstanceUpdate";
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
            case 0x465370: return "SoundSubsystemEnabledGet";
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
            case 0x465190: return "SoundHandleStopForward";
            case 0x4652b0: return "SoundBatchModeGet";
            case 0x465430: return "TransientEffectEmitterCreate";
            case 0x4659b0: return "TransientEffectInstanceRelease";
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
            case 0x4653a0: return "TransientEffectTexturesPreload";
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
            case 0x490090: return "AssetPathComponentsParse";
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
            case 0x47efd0: return "DirectInputShutdown";
            case 0x47ccf0: return "DirectDrawObjectsRelease";
            case 0x485e80: return "RenderSetup";
            case 0x4893f0: return "MapFileResourceSlotsReset";
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
            case 0x40b000: return "TileMatchingCellsCollect";
            case 0x418a00: return "OverlayTextBufferRefresh";
            case 0x435b20: return "SceneObjectChainRecordCreate";
            case 0x44d280: return "RandomWaypointTimerStepApply";
            case 0x45c0d0: return "PairCountHistogramBuild";
            case 0x469cb0: return "EntityComponentFloatAt20";
            case 0x469ce0: return "EntityComponentFloatAtD4";
            case 0x469d20: return "EntityComponentFloatAtD8";
            case 0x46a0a0: return "EntitySpeedComponentGet";
            case 0x46f770: return "EffectApplyByKindBranch";
            case 0x471630: return "RayHitCellDistanceMinimizeSweep";
            case 0x473460: return "LightInterfaceCallSlot40";
            case 0x473480: return "LightInterfaceCallSlot3C";
            case 0x4734a0: return "LightInterfaceCallSlot38";
            case 0x474760: return "InterfaceArrayForwardSync";
            case 0x47e300: return "InterfaceMethod18StructApply";
            case 0x483690: return "MatrixMethodPairApply";
            case 0x485ea0: return "InterfaceSlot58PairStoreClamp";
            case 0x489660: return "SceneActFrameRecordCloneOrCreate";
            case 0x489b30: return "ObjectInterfaceArrayResourceCreate";
            case 0x41f6c0: return "SelectedEntityDetachCallback";
            case 0x43fc20: return "RecordPairPoseCopyAndActivityRaise";
            case 0x407340: return "StateFlagToggleWhenZero";
            case 0x418800: return "ModeFlagPairApplyIfBit2";
            case 0x432f00: return "TerrainCellStateEquals6";
            case 0x435d91: return "EntityB8Bit3SetOrClear";
            case 0x435f32: return "EntityFieldsA0A4StoreA8Clear";
            case 0x4538a0: return "CameraFocusScrollScaleCommit";
            case 0x454b70: return "ObjectiveVoiceAliasRaiseCommit";
            case 0x454b80: return "ObjectiveVoiceAliasRaiseGateBit27";
            case 0x46a9e0: return "AdvisorTextBoxRecordCommit";
            case 0x473560: return "DDiSceneObjectKindModeSetup";
            case 0x477f80: return "DisplayDeviceCapsQueryDiscard";
            case 0x49db60: return "SignedDecimalToStringConvert";
            case 0x49dc10: return "StringToLowercase";
            case 0x498000: return "CrtMemoryMove";
            case 0x4965d0: return "CrtFileDescriptorTableInit";
            case 0x499880: return "CrtTimeZoneStateInit";
            case 0x4962f0: return "CrtLocaleCodePageTableUpdate";
            case 0x493280: return "CrtFloatingPointExceptionRaise";
            case 0x495f80: return "CrtCommandLineArgumentScan";
            case 0x4968f0: return "CrtRuntimeErrorMessage";
            case 0x49b2b0: return "CrtPutEnvUpdate";
            case 0x49afe0: return "CrtCompareStringACompat";
            case 0x4935c0: return "CrtDoubleToIntegerRound";
            case 0x45b740: return "ScrollInfoPanelInitialize";
            case 0x45e660: return "RectContainsPointHalfOpen";
            case 0x45e820: return "ViewportPointCallbackDispatch";
            case 0x463770: return "ScrollRectResourceRecordCreate";
            case 0x463810: return "ScrollRectPayloadRecordCreate";
            case 0x44c150: return "EntityPairScalarFieldsSet";
            case 0x44e670: return "NestedPointerRecordDestroy";
            case 0x495160: return "CxxExceptionHandlerSearch";
            case 0x495ee0: return "CrtCommandLineArgumentsInit";
            case 0x49a1b0: return "CrtExtendedFloatToDecimal";
            case 0x49a540: return "Extended80Multiply";
            case 0x49a800: return "ExtendedFloatPowerOfTenScale";
            case 0x4802d0: return "FileTokenListLoadAndTreeScan";
            case 0x480390: return "DirectoryTreeScan";
            case 0x480570: return "DirectoryTokenListCompare";
            case 0x481fa0: return "MeshVertexCoordinateRemap";
            case 0x44fab0: return "TerrainCellSurfaceTriangleVerticesSelect";
            case 0x43c3e0: return "EntityType26CommandCompletionGate";
            case 0x496190: return "CrtEnvironmentBlockGet";
            case 0x49abe0: return "CrtWideCharToMultiByteCompat";
            case 0x4957d0: return "CxxCatchObjectBindOrCopy";
            case 0x499b30: return "CrtDaylightSavingTimeCheck";
            case 0x4887c0: return "SceneHierarchyInterfaceRegisterRecursive";
            case 0x48e0ad: return "CrtAcosCore";
            case 0x449430: return "EntityTargetCellRouteAndAssociate";
            case 0x449610: return "EntityCoordinateRouteAndMark";
            case 0x449980: return "EntityActivityTerrainEligibilityTransition";
            case 0x49ae10: return "CrtEnvironmentVariableLookup";
            case 0x49b4c0: return "CrtEnvironmentEntryFind";
            case 0x49b540: return "CrtEnvironmentArrayClone";
            case 0x49b5b0: return "CrtMultibyteCharacterSearch";
            case 0x449520: return "EntityTargetCellRouteMarker14";
            case 0x449240: return "EntityWalkableTargetCellRouteIssue";
            case 0x449010: return "EntityKind6RecordApproachRouteIssue";
            case 0x4489a0: return "EntityKind6TickDownMarkerSpawn";
            case 0x45d060: return "PrioritiesImagePositionsConfigLoad";
            case 0x45a650: return "PanelCategoryBoundsSet";
            case 0x45a270: return "PanelCategoryInteractionStateReset";
            case 0x45c3f0: return "AdvisorProgressBarFillDraw";
            case 0x49a890: return "Win32FileDescriptorResize";
            case 0x49aa10: return "CrtSignalHandlerDispatch";
            case 0x49ab90: return "CrtSignalHandlerRecordFind";
            case 0x44acc0: return "TerrainObjectTypeSfxGridConfigLoad";
            case 0x44ad80: return "TerrainObjectTypeSfxAtCellGet";
            case 0x449a70: return "EntityActivity23StateTransition";
            case 0x45d300: return "PriorityImageSelectionStateUpdate";
            case 0x45d3f0: return "PriorityImageOrderMoveToFront";
            case 0x47e130: return "SceneObjectRegistryReleaseAll";
            case 0x4350a0: return "SurfaceTypeDescriptionsConfigLoad";
            case 0x444fb0: return "EntityTurnToVectorActivityIssue";
            case 0x4451e0: return "EntityPathStepVectorNormalize";
            case 0x44a7d0: return "PolyVariantSectionModelBuild";
            case 0x44d760: return "JitterStepApply";
            case 0x45ac30: return "PanelTextureSlotsAllocate";
            case 0x4629d0: return "PanelEventEntryAppend";
            case 0x46f3b0: return "RayHitScanAndEffectApply";
            case 0x482220: return "MeshRecordAllocateAndFill";
            case 0x00401240: return "Vector2Distance";
            case 0x004013a0: return "Vector3Length";
            case 0x00401420: return "Vector3Distance";
            case 0x00401470: return "Vector3CrossProduct";
            case 0x00401500: return "Vector3Add";
            case 0x004016c0: return "Vector3Subtract";
            case 0x004016f0: return "Vector3DotProduct";
            case 0x00401750: return "Vector3Normalize";
            case 0x00434da0: return "AdvisorTooltipAndTooltipSoundConfigLoad";
            case 0x00434a00: return "AdvisorTutorialIconImageLoad";
            case 0x00435010: return "TerrainCellWorkFlagCircleSet";
            case 0x004332f0: return "TerrainCellWorkFlagSet";
            case 0x00439b80: return "TaskTargetCellEligibilityCheck";
            case 0x0049d780: return "DisplayResolutionComboPopulate";
            case 0x0049d870: return "DisplayResolutionFilterTupleAppend";
            case 0x0049d8d0: return "DisplayModeMatchesFilterTuple";
            case 0x0049d950: return "DisplayModeNameIndexFind";
            case 0x0049d9d0: return "DisplayRefreshRateControlsSync";
            case 0x004073a0: return "EntityPointerRegistriesRemove";
            case 0x004067b0: return "PolylineSegmentLengthsAndTotalCalculate";
            case 0x00406820: return "BezierCurveSamplesBuild";
            case 0x00410c70: return "ResourceVariantRecordRelease";
            case 0x00410f30: return "ResourceRecordAndChildrenRelease";
            case 0x0044cc10: return "LightEffectFadeParametersStore";
            case 0x00432950: return "EntityMoveDestinationCellIsUsable";
            case 0x0043fdc0: return "EntityActionTargetCellIsValid";
            case 0x004718d0: return "ObjectPairRayHitCheck";
            case 0x00470690: return "TimedEntitySegmentVisualCreate";
            case 0x00470260: return "EntitySurfaceSegmentCollisionCheck";
            case 0x00473e10: return "RendererFogColorSet";
            case 0x00403560: return "TaskRegistryRemoveType10EntriesForEntity";
            case 0x00403430: return "TaskRegistryRemoveType0EntryByPair";
            case 0x00402890: return "TaskType23CreateForCategoryMask";
            case 0x00431140: return "TerrainCellQueuedWorkTransition";
            case 0x00470630: return "EntityDirectedLineSideCheck";
            case 0x00473b20: return "EntityFlag80Set";
            case 0x00474a60: return "SceneSurfaceTextureBindByMode";
            case 0x00475020: return "SceneMeshColorAndAlphaSet";
            case 0x0042ef40: return "StatsLevelRecordCountForStatType";
            case 0x004093b0: return "PassableAdjacentCellSearchExpand";
            case 0x004712c0: return "EntityVelocityStateRecordGet";
            case 0x004712f0: return "EntityVelocitySlotValueGet";
            case 0x00471310: return "EntityVelocitySlotValueSet";
            case 0x0043a930: return "TrackedEntityPointerArrayRemove";
            case 0x0042dfc0: return "ObjectPointerRegistryClearMatch";
            case 0x0042dff0: return "ObjectPointerRegistryGetSlot";
            case 0x004293b0: return "SelectionRectangleRender";
            case 0x00424830: return "SelectedObjectOutlinesDraw";
            case 0x00432a20: return "TerrainCellAllowsPlacement";
            case 0x00431ab0: return "AnyObjectCanOccupyTerrainCell";
            case 0x00432b90: return "TerrainCellTypeIs8Or10Or11";
            case 0x00433330: return "TerrainCellTypeGetAtCoords";
            case 0x004329d0: return "TerrainCellFlag8CheckIfInBounds";
            case 0x0046e8a0: return "WorldSpatialRecordFindAtGridCoordinate";
            case 0x00471860: return "SceneSegmentTraceNearestHit";
            case 0x00472080: return "EntityCentersWithinConfiguredRadius";
            case 0x00472130: return "EntityCentersWithinObjectRadius";
            case 0x004728e0: return "RendererFlagBit6Set";
            case 0x00473ec0: return "RenderObjectStateApplyToEntries";
            case 0x00474e90: return "RenderObjectHierarchyFloat3Set";
            case 0x004750b0: return "RenderObjectSetPackedRgbaFromFloatChannels";
            case 0x00423080: return "ModeDependentField24Get";
            case 0x004230b0: return "GlobalFloatField00556F00Get";
            case 0x00425d80: return "CurrentEngineModeIsZero";
            case 0x00426230: return "GlobalTripleCopyOut";
            case 0x00426360: return "GlobalStateFlagsBits6And9Set";
            case 0x004263a0: return "GlobalStateFlagsBit7Set";
            case 0x004263c0: return "GlobalSubsystemStateReset";
            case 0x00429850: return "ActiveEngineFieldB0Get";
            case 0x00429860: return "EngineFloatField210GetByIndex";
            case 0x00429880: return "EngineFloatField260Get";
            case 0x0042b2f0: return "EntityEffectCountersApplyFromRecord";
            case 0x004314c0: return "ActivityBuildRecordRelease";
            case 0x00431530: return "ActivityBuildRecordsReleaseAll";
            case 0x0046a120: return "GroupedCodeTableFindValueIndex";
            case 0x0046a370: return "LinkedRecordChainFree";
            case 0x0046a390: return "GlobalLinkedRecordChainClear";
            case 0x0046a760: return "FirstSetFlagOrdinal";
            case 0x0046aad0: return "MessageLabelIndexFind";
            case 0x00475580: return "RenderObjectRotateAxis";
            case 0x00475610: return "RenderObjectSetIdentityTransform";
            case 0x00476270: return "SceneObjectSetFormattedString";
            case 0x004041b0: return "EntityTaskChainContainsKind";
            case 0x0040cf80: return "GridCellObjectLinkInsert";
            case 0x00412450: return "RenderUiFrameCell";
            case 0x0041a660: return "InteractionActionCodeMapInitialize";
            case 0x004302b0: return "MapCellTrackingFlagsClearAndRefresh";
            case 0x00432100: return "TerrainCellFlag100SetAndTrack";
            case 0x00432270: return "TerrainCellVisitedFlagSetAndTrack";
            case 0x00465c50: return "ParticleSlotInitializeForEmitter";
            case 0x00479b80: return "PointInPolygon2D";
            case 0x00479d50: return "Matrix4SetRotationZ";
            case 0x004760e0: return "AttachedObjectDataReleaseAndClear";
            case 0x00476d50: return "ResourceSlotTableInitialize";
            case 0x00476d80: return "ResourceSlotTableReleaseAll";
            case 0x0047ab00: return "SoundBankEntryRelease";
            case 0x0047b050: return "SoundBankEntryVolumeSet";
            case 0x0047b2a0: return "KeyedResourceEntryRemove";
            case 0x0048f950: return "CrtFindHandleClose";
            case 0x00491360: return "CrtBufferedCharacterWrite";
            case 0x00498b70: return "CrtWideCharToMultibyte";
            case 0x00498e00: return "CrtMultibyteToWideChar";
            case 0x004991f0: return "CrtFileDescriptorHandleGet";
            case 0x004997c0: return "CrtCallocAligned";
            case 0x00499fb0: return "CrtUnsignedAddWithCarry";
            case 0x00407300: return "BubbleActionNameToIndex";
            case 0x00410450: return "StringKeyedCacheEntryFind";
            case 0x004322d0: return "TerrainCellVisitedFlagTest";
            case 0x00432300: return "TerrainCellVisitedFlagsClearAll";
            case 0x00471ca0: return "EntityWorldCenterPositionRead";
            case 0x00471c70: return "VelocitySlotVectorRead";
            case 0x00471a40: return "VelocitySlotPrimaryObjectGet";
            case 0x00471a50: return "VelocitySlotSecondaryObjectGet";
            case 0x00473ba0: return "SceneObjectFlagBit3Get";
            case 0x00410520: return "StringCacheLookupOrLoad";
            case 0x00419430: return "InterfaceMessageTabInfoTextLoad";
            case 0x00432360: return "TerrainCellPrimaryFlagsMaskClearAndRefresh";
            case 0x004323c0: return "TerrainCellSecondaryFlagBit0Clear";
            case 0x004323f0: return "TerrainCellFlagsSetAndRefresh";
            case 0x00432450: return "TerrainCellFlagsSetMask100400";
            case 0x00432490: return "TerrainCellFlagsSetMask20108000";
            case 0x004324d0: return "TerrainCellSecondaryFlag8SetOrClear";
            case 0x00432520: return "TerrainCellSecondaryFlag8Test";
            case 0x00432560: return "TerrainCellFlag40000000ClearOrSet";
            case 0x004325d0: return "TerrainCellSecondaryFlagBit0Set";
            case 0x00432710: return "TerrainCellFlag80000ClearOrSet";
            case 0x00432d90: return "TerrainCellFlag20TestInBounds";
            case 0x0046fef0: return "EffectModeSurfaceQueryTypeResolve";
            case 0x0046ff70: return "ActiveEffectRecordSlotFind";
            case 0x0046ff90: return "ActiveSegmentVisualRecordSlotFind";
            case 0x00479ce0: return "Matrix4Multiply";
            case 0x00479af0: return "LineIntersection2DCompute";
            case 0x00478330: return "Direct3DRenderPreferencesApply";
            case 0x0047dfa0: return "DirectDrawSurfacePixelRead";
            case 0x0047de60: return "DirectDrawSurfaceDescriptionGet";
            case 0x0047def0: return "DirectDrawSurfacePixelFormatGet";
            case 0x00432b00: return "TerrainCellFlag800TestInBounds";
            case 0x00432c20: return "TerrainCellFlag40000Test";
            case 0x00432c50: return "TerrainCellTypeSpeedClassGet";
            case 0x00432dd0: return "TerrainCellFlag08000000Test";
            case 0x00432e60: return "TerrainCellFlagsMaskC0Test";
            case 0x00432e90: return "TerrainCellFlag04000000Test";
            case 0x00432ec0: return "TerrainCellStateEquals1";
            case 0x00432f40: return "TerrainCellFlagsMask18ClearInBounds";
            case 0x00432f90: return "TerrainCellFlag4TestInBounds";
            case 0x00432fd0: return "TerrainCellFlag2000Test";
            case 0x00433000: return "TerrainCellFlag80Test";
            case 0x00433030: return "TerrainCellFlag8000Test";
            case 0x00433060: return "TerrainCellSecondaryFlagBit0Test";
            case 0x004330e0: return "TerrainCellFlagsMask180000Test";
            case 0x00473dc0: return "RendererFogEnableAndColorApply";
            case 0x00473e70: return "RendererFogModeSet";
            case 0x00473ea0: return "RendererFogParametersSet";
            case 0x00474ff0: return "SceneMeshColorSet";
            case 0x00403320: return "ObjectCommandTaskCreate";
            case 0x00404e00: return "ObjectPathSubmitToGridTarget";
            case 0x00404f50: return "ObjectFilterMatchesActionTarget";
            case 0x00407410: return "Type4ObjectRegistryInsert";
            case 0x004074b0: return "Type2TimedRegistryInsert";
            case 0x00407540: return "FlaggedObjectTimerRegistryInsert";
            case 0x00408ce0: return "OperationGroupAppendObject";
            case 0x00429160: return "TerrainCellPaletteResourceAtGridPosition";
            case 0x004295c0: return "EntityMatchesCurrentSelectionOrOwner";
            case 0x004247d0: return "ApplyDefaultTransformToFlaggedRenderObject";
            case 0x00433120: return "TerrainCellFlag10000000SetOrClear";
            case 0x004332c0: return "TerrainCellRecordByte3Get";
            case 0x00434530: return "TerrainGridEntryResourcesReleaseAll";
            case 0x004382c0: return "PooledRecordAllocateAndReset";
            case 0x00438310: return "PooledRecordChunkAllocateAndLink";
            case 0x00438580: return "EntityFlag80000000SetOrClear";
            case 0x0047a5c0: return "FixedSizeRecordPoolGrow";
            case 0x0049d5e0: return "DisplayModeDialogSetModeAndRefreshChoices";
            case 0x00404ed0: return "CheckObjectCandidate";
            case 0x00404f10: return "FindMatchingObjectRecord";
            case 0x00407370: return "ResetObjectAuxiliaryState";
            case 0x00407510: return "SetType2ObjectRecordValue";
            case 0x00409450: return "FindNearbyPlacementCandidate";
            case 0x0040bdc0: return "SetTimeoutTo10000";
            case 0x0040bfc0: return "RockFallStyleTableSet";
            case 0x0040cf50: return "RegisterObjectAtGridPosition";
            case 0x0040fde0: return "SetVectorAndNotifyListeners";
            case 0x0040fec0: return "SetGlobalScalarToOne";
            case 0x0040ff70: return "ObjectVector3Set";
            case 0x0040ffe0: return "GlobalVectorSetAndNotifyListeners";
            case 0x00410b30: return "GlobalPointerGet_004c76ab24";
            case 0x00410b40: return "GlobalPointerGet_004c76ab28";
            case 0x00410b50: return "GlobalPointerGet_004c76ab3c";
            case 0x00417430: return "FloatToLong";
            case 0x00418000: return "ShowLevelOverlayForCurrentSelection";
            case 0x00419b50: return "RemainingTimerDurationGet";
            case 0x0041b9b0: return "GlobalStateGet_004ddd1c";
            case 0x0041ee60: return "InteractionTimerResetAndFlagClear";
            case 0x0041f840: return "RemoveObjectIfClass6";
            case 0x0041f8e0: return "NoOpCallback";
            case 0x00438670: return "EntityAtGridPositionLookup";
            case 0x00439790: return "LocalizedObjectTheNameGet";
            case 0x00439860: return "LocalizedObjectNameGet";
            case 0x00443840: return "EntityRouteCellCandidateAccept";
            case 0x00477f70: return "RenderFeatureFlagsGet";
            case 0x0047ec60: return "AviPlaybackFrameIndexGet";
            case 0x004802b0: return "LoaderProgressCallbackSet";
            case 0x00482210: return "PointerValueGet";
            case 0x00487760: return "ResourceStringTablesFree";
            case 0x004877d0: return "ResourceEntryIdGet";
            case 0x004894c0: return "ComInterfaceQueryOrFallback";
            case 0x0048bcf0: return "WadEntryLengthGet";
            case 0x0048bd10: return "WadEntryField4Get";
            case 0x0048dfd0: return "QsortSmallPartitionSort";
            case 0x0048e250: return "StackProbeAllocate";
            case 0x0048e4f0: return "CrtTerminateProcessWithHandlers";
            case 0x0048e5a0: return "CrtRunCallbackArray";
            case 0x0048e680: return "CrtDoubleAbsoluteValue";
            case 0x00490070: return "VsscanfParseInput";
            case 0x00494ca0: return "StreamPushbackChar";
            case 0x00494cc0: return "StreamSkipClassifiedChars";
            case 0x004977d0: return "CrtControlFlagsTranslate";
            case 0x00499fe0: return "UInt96Add";
            case 0x0047a1b0: return "FormattedTextDraw";
            case 0x0048d940: return "CrtMathDispatchTableInitialize";
            case 0x0048e090: return "CrtAsin";
            case 0x0048e480: return "CrtFloatingPointRuntimeInitialize";
            case 0x0048e4b0: return "CrtExit";
            case 0x0048ec00: return "SehFrameUnwind";
            case 0x0048eca0: return "CallSettingFrame12";
            case 0x0048ee90: return "SehScopeTableLookup";
            case 0x0048fa00: return "CurrentDirectoryGet";
            case 0x0048fd20: return "CrtFlushStreamBuffer";
            case 0x00493ec0: return "BoundedStringLength";
            case 0x00493ae0: return "DoubleFrexpDecompose";
            case 0x00493be0: return "FpuStatusWordGetVariantA";
            case 0x00493bf0: return "FpuStatusWordGetVariantB";
            case 0x00497aa0: return "UInt96Zero";
            case 0x00497ab0: return "UInt96IsZero";
            case 0x00497a80: return "UInt96Copy";
            case 0x00497ad0: return "UInt96ShiftRight";
            case 0x00497900: return "UInt96HasNoBitsAtOrAbove";
            case 0x00497970: return "UInt96IncrementAtBit";
            case 0x004979e0: return "UInt96ClearFromBit";
            case 0x00438df0: return "EntityLookupAtWorldPosition";
            case 0x00442210: return "EntityActionRouteStateInitialize";
            case 0x0044c970: return "LightEffectsBlinkParametersApply";
            case 0x00490280: return "CrtFloatingPointFeatureProbe";
            case 0x004902b0: return "CrtNormalizeDecimalPoint";
            case 0x004907c0: return "StringPrefixReserve";
            case 0x004913b0: return "CrtWriteRepeatedChar";
            case 0x004913f0: return "CrtWriteStringBounded";
            case 0x004914a0: return "StrChr";
            case 0x00491560: return "CrtHeapInitialize";
            case 0x00491710: return "CrtHeapBlockFree";
            case 0x00492dcc: return "CrtFmodExtendedPrecision";
            case 0x004939d0: return "CrtMathExceptionHandlerGet";
            case 0x00493a00: return "CrtMathExceptionCodeFromStatus";
            case 0x00493a40: return "DoubleBuildWithExponent";
            case 0x00493a80: return "DoubleClassify";
            case 0x00495d90: return "IsDbcsLeadByte";
            case 0x004964e0: return "CrtCodePageGet";
            case 0x004974d0: return "CrtStandardStreamBufferInstall";
            case 0x00497ec0: return "CrtLocaleDataGetOrInitialize";
            case 0x00498340: return "CrtFatalMessageExit";
            case 0x00499860: return "CrtTimeZoneInitializeOnce";
            case 0x0048e890: return "MarkupIntegerTokenParse";
            case 0x0043f560: return "EntityPoolFindWithLocalPredicate";
            case 0x00456640: return "GlobalFlagGet_004ff978";
            case 0x00458650: return "GlobalFlagGet_004ffe30";
            case 0x00458660: return "GlobalValueSet_004ffe48";
            case 0x0045c4a0: return "GlobalValueSet_0050080c";
            case 0x0045cc10: return "IndexedGlobalValueGet_00500a9c";
            case 0x0045cc60: return "GlobalValueGet_00500a98";
            case 0x004628c0: return "GlobalFloatAccumulate_00555120";
            case 0x0047c160: return "ConstantNegative10000Get";
            case 0x0048c030: return "WadMemoryStreamBufferGet";
            case 0x0048e1aa: return "CrtMathIntrinsicDispatch";
            case 0x00491ff5: return "CrtNoOpStub";
            case 0x00495bde: return "CrtAbortAfterCallback";
            case 0x00497870: return "CrtNoOpStubVariant";
            case 0x0044c7c0: return "LightEffectsConfigurationLoad";
            case 0x0044cef0: return "LightEffectsMoveParametersStore";
            case 0x0044d710: return "WaypointVectorRandomScaleApply";
            case 0x0046abb0: return "TextSlotMessageWithImageAssign";
            case 0x0048e280: return "CrtRoundToIntegralWithMathErrorHandling";
            case 0x0048e8e0: return "CrtRoundToIntegralWithMathErrorHandlingVariant";
            case 0x004906f0: return "CrtScientificFloatFormatGuarded";
            case 0x00490720: return "CrtFixedDecimalFormatGuarded";
            case 0x004908a0: return "CrtFileStreamClose";
            case 0x00491d90: return "CrtNewHandlerInvokeForAllocationRetry";
            case 0x00494c30: return "CrtScanFormatCharNormalize";
            case 0x0047e460: return "ComObjectDestroyAndOptionallyFree";
            case 0x00485210: return "FormattedTextAppendToRingBuffer";
            case 0x0048fed0: return "CrtInputWithStreamBufferSetup";
            case 0x0048b280: return "WindowModeFlagSet";
            case 0x0047df50: return "PixelChannelMaskGet";
            case 0x0044ade0: return "EntityKindOneRandomSpawnTrigger";
            case 0x0044bfd0: return "ConfiguredMonsterSpawnBehaviorTick";
            case 0x0045a240: return "ScrollPanelControlsInitialize";
            case 0x0047ded0: return "DirectDrawSurfaceUnlock";
            case 0x0048c010: return "WadStreamLengthGet";
            case 0x0047ebe0: return "AviStreamFrameCountGet";
            case 0x00495a60: return "CrtUnwindAddressResolve";
            case 0x0044cf70: return "ScenePositionResetAndEffectMotionCancel";
            case 0x0044da20: return "RoutePositionBlockedStateSet";
            case 0x00435580: return "GlobalHandlePositionApplyIfPresent";
            case 0x00435aa0: return "PanelSelectionHighlightToggle";
            case 0x004374d0: return "TaskCounterIncrementByBranch";
            case 0x004374f0: return "EntityCallbackDispatchUnlessTypeFive";
            case 0x00438a60: return "SpatialObjectQueryCenterMask01";
            case 0x00438ae0: return "SpatialObjectQueryCenterMask02";
            case 0x00438b80: return "SpatialObjectQueryCenterMaskFromArgument";
            case 0x00438bf0: return "SpatialObjectQueryCenterMask80";
            case 0x00438cd0: return "SpatialObjectQueryCenterMask20";
            case 0x00438d30: return "SpatialObjectQueryCenterMask10";
            case 0x00438d90: return "SpatialObjectQueryCenterMask200";
            case 0x00438e50: return "SpatialObjectQueryCenterMask100";
            case 0x00439200: return "TaskKindFilteredSearch";
            case 0x00439240: return "TaskEntityFlagsAndCellTaskMatch";
            case 0x0043b360: return "TaskCountAtGridCoordinateQuery";
            case 0x0043b820: return "TypeTwoEntityEventDispatch";
            case 0x0043b850: return "TypeTwoEntityPositionedCallbackDispatch";
            case 0x0043c490: return "TaskTypeAndConditionCheck";
            case 0x004407b0: return "SpatialEntityCallbackQuery";
            case 0x004408c0: return "SpatialEntityCallbackQueryAtOptionalCoordinate";
            case 0x00440c30: return "GridRouteSearchAdapter";
            case 0x00442540: return "GridOperationArgumentAdapter";
            case 0x004456e0: return "EntityBehaviorCallbackTick";
            case 0x00445d70: return "EntityFlagGatedCallbackDispatch";
            case 0x0044702d: return "RegisterRelativeStateUpdateFragment";
            case 0x004474f0: return "LinkedEntityProximityResponse";
            case 0x00447900: return "TypeFourEntityCallbackDispatch";
            case 0x0044adc0: return "ScatterSpawnGateSet";
            case 0x0044af90: return "EntityAttachedSpawnRecordProcess";
            case 0x0044b4c0: return "TypeTwoEntityTargetResolveAndApply";
            case 0x0044bae0: return "SceneObjectResourceAndCallbackStateDispose";
            case 0x0044c550: return "GlobalScatterSpawnEnableAndDrain";
            case 0x0044f490: return "GridCellOperationApplyAll";
            case 0x00450560: return "RandomReachableGridPointSelect";
            case 0x00452580: return "ActiveContextRecordGet";
            case 0x00454880: return "GlobalCounter_004ff990Increment";
            case 0x00454890: return "GlobalCounter_004ff990Reset";
            case 0x004567c0: return "GlobalValue_004ff9bcStoreAndReturn";
            case 0x00459f50: return "IndexedPanelResourcePositionApply";
            case 0x00459fd0: return "PanelResourcePositionApplyIfEnabled";
            case 0x0045a3b0: return "PanelSlotCoordinateHitTest";
            case 0x0045a630: return "UIRecordCallbackForward";
            case 0x0045aac0: return "PanelItemFlag10SetClear";
            case 0x0045ab00: return "PanelStateFlagsToggle";
            case 0x0045abf0: return "PanelItemFlag8SetIfEnabled";
            case 0x0045b610: return "GlobalUIFlags_00500860ClearBits_1_2";
            case 0x0045bfb0: return "ThreeSurfaceResourcesLoadAndInitialize";
            case 0x0045c340: return "PanelResourceLayoutStore";
            case 0x0045c4b0: return "PanelResourcePositionUpdateIfEnabled";
            case 0x0045c520: return "PanelResourceHandleAndDimensionsStore";
            case 0x0045cf00: return "PriorityLabelLayoutApply";
            case 0x0045d3c0: return "PriorityPanelRowMoveDispatch";
            case 0x0045d8a0: return "ObjectField14Set";
            case 0x0045db00: return "TexturedSceneObjectDrawAtPosition";
            case 0x00460910: return "ResolvedEntityOperationDispatch";
            case 0x004628a0: return "GlobalFloatAccumulator_00555330Add";
            case 0x004628e0: return "CallbackRecordAllocateInitialize";
            case 0x00464c50: return "SoundDynamicAliasRegistrationSet";
            case 0x00465380: return "SceneEffectRegistryReset";
            case 0x0046a950: return "TextLayoutGeometryStoreAndApply";
            case 0x00472670: return "DirectDrawSurfaceDescriptionGet";
            case 0x004756d0: return "VideoObjectAspectRatioGet";
            case 0x004757f0: return "TypedChildResourceScalarGet";
            case 0x00477310: return "ComPropertyResultGet";
            case 0x00485a00: return "ObjectStatePairGet";
            case 0x00487d50: return "ComInterfaceQueryAndContextDispatch";
            case 0x00488bf0: return "CdAudioStopAndCloseIfActive";
            case 0x00489bc0: return "ComChildItemsVisit";
            case 0x0048ebb0: return "CrtExceptionHandlerTransferAfterUnlink";
            case 0x0048ebf0: return "CrtExceptionHandlerTransferWithAtomicPrefix";
            case 0x0048ec60: return "CrtExceptionDispatchForwarder";
            case 0x0048ed00: return "CrtExceptionFrameFilterInvoke";
            case 0x0048ed30: return "CrtExceptionFrameInstallAndDispatch";
            case 0x0048ee00: return "CrtExceptionScopeFilterAndUnwind";
            case 0x0048efe6: return "CrtNlgFrameStateRecord";
            case 0x00490230: return "ProcessorFeatureFallbackFalse";
            case 0x004918a0: return "SmallHeapPageFreeSlotUpdate";
            case 0x00491cc0: return "SmallHeapBlockResizeInPlace";
            case 0x00491f80: return "CrtOneArgumentMathErrorHandlingStart";
            case 0x0049200c: return "CrtFpuPrecisionModeCodeGet";
            case 0x00492068: return "DoubleExponentBitsGetOrSpecialValue";
            case 0x0049221e: return "CrtFloatingPointExceptionResultDispatch";
            case 0x00492225: return "CrtFloatingPointExceptionResultDispatchVariant";
            case 0x00492bc6: return "CrtDoubleRemainderReduction";
            case 0x00493170: return "CrtMathExceptionHandleSingleResult";
            case 0x004931c0: return "CrtMathExceptionHandleResultWithStatus";
            case 0x004938f0: return "CrtMathExceptionDispatch";
            case 0x004939a0: return "CrtMathErrorCodeSet";
            case 0x00493c40: return "CrtNoOpStubVariant2";
            case 0x00495410: return "CrtSehExceptionDispatcher";
            case 0x004954e0: return "CrtSehLocalUnwindToScope";
            case 0x004955c0: return "CrtSehUnwindAndInvokeHandler";
            case 0x00495650: return "CrtSehHandlerCallWithFrame";
            case 0x0049573a: return "CrtSehAbnormalTerminationCleanup";
            case 0x004959e0: return "CrtSehDestructorInvoke";
            case 0x00495ae0: return "CrtTerminateHandlerInvokeAndAbort";
            case 0x00495b70: return "CrtUnexpectedHandlerInvokeAndTerminate";
            case 0x00496895: return "CrtLocalUnwindFromFrame";
            case 0x004968b0: return "CrtRuntimeErrorMessageEnterExit";
            case 0x00497770: return "CrtFpuControlWordUpdate";
            case 0x004977b0: return "CrtFpuControlWordUpdateMask";
            case 0x00497d60: return "CrtExtendedFloatConvertFormatA";
            case 0x00497d80: return "CrtExtendedFloatConvertFormatB";
            case 0x00497da0: return "CrtDoubleToExtendedFloatConvertA";
            case 0x00497de0: return "CrtDoubleToExtendedFloatConvertB";
            case 0x00498ce0: return "CrtScanfNumericConversionAssign";
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
