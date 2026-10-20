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
        "CmdCancelByCoords", "CmdCancelRef", "CmdMarkMatching", "CmdSetKind" };

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
