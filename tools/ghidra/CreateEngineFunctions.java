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
    };
    static final String[] NAMES = { "EngineTick", "EngineShutdown", "EngineShutdownAlt" };

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
                    f = fm.createFunction(NAMES[i], addr, new AddressSet(addr),
                            SourceType.USER_DEFINED);
                    created++;
                    println("Created " + NAMES[i] + " @ " + addr);
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
