// Ghidra post-analysis script (Java): create function entries in the
// engine's indirect-call region that auto-analysis missed, then decompile
// each and write pseudocode to /tmp/engine-decompiles/engine-region.c
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
                Function f = fm.getFunctionAt(addr);
                if (f == null) {
                    Function inner = fm.getFunctionContaining(addr);
                    if (inner != null) {
                        // address is mid-body of an existing function; decompile that one
                        inner.setName(NAMES[i], SourceType.USER_DEFINED);
                        f = inner;
                        renamed++;
                    } else {
                        f = fm.createFunction(NAMES[i], addr, new AddressSet(addr),
                                SourceType.USER_DEFINED);
                        created++;
                        println("Created " + NAMES[i] + " @ " + addr);
                    }
                } else {
                    f.setName(NAMES[i], SourceType.USER_DEFINED);
                    renamed++;
                }
                w.println("// ==== " + NAMES[i] + " @ " + addr + " ====");
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
