# Shipped NERP callback gaps on the tutorial path

Static investigation only. No candidate code, build, game/prefix/reference tree,
launch wrapper, desktop permission, or active debugger was changed. The current
objective integration owner retains sole build/runtime rights.

Read the selected reference EXE from the saved immutable backup with O_NOATIME,
checked its known SHA256, and copied it outside scan roots for analysis:
`/var/tmp/wine-tutorial-gate-audit/reference-LegoRR.exe`.
SHA256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
A local PE-section parser read12-byte rows at VA0x004a5948; names, code pointers,
and argument metadata are recorded in `reference-callback-rows.json` there.
This parser result is local artifact inspection, not REA output.

REA GNU objdump inspection independently decoded the shipped x86 instructions:
Evidence `ev_a11e3f591f918cbf4b99f3834ca2bd449b36a1760ae101afbc33e52dd15e64a2`.
Export `/var/tmp/wine-tutorial-gate-audit/rea-objdump-evidence.json` retains the
complete record; focused extracted instructions are `callback-instructions.txt`.
Exit0, no signal, output_truncated=false, artifact_changed=false. The MCP response
exceeded its transport limit; the operation was not repeated, and the retained
complete record was exported instead. GNU executable version/format inventory
was not probed; symbol names and high-level meanings are not recovered from its
raw text. REA native Ghidra provider lacked Java in its server environment; no
setup, installation, or configuration change was made. No active binary session
was opened or replaced. Analysis of this copy does not observe runtime execution.

| ID | Shipped callback | Observed contract / candidate gap |
| --- | --- | --- |
|144|GetSelectedRecordObject,0x454220,kind0|Scans registry indices0..9 via0x42dff0, tests each returned entity with0x452610(entity,0), returns index+1 on success or0. No handler for this ID exists in current tutorial_builtin_command. Shipped M01 SelectRaider stage stops while this query is0.|
|254|SetRecordObjectPointer,0x454700,kind4|Loads the pointed argument and stores it at DAT_00555dec. No handler exists, although the candidate has a tutorial_record_pointer member. The tutorial's actor-introduction steps invoke it.|
|200|AdvanceMessage,0x453a50,kind3|Signed-clamps DAT_00555de4 to1 when above1, calls0x453920, returns0. Candidate calls0x453920 directly, omitting the clamp. Correct storage ownership still needs tracing: its scroll helper has detached pending/step sidecars.|
|210|SetCrystalPriority,0x454270,kind4|Calls0x4020a0 with kind0 and state(argument==0), returns0. No current handler. Later tutorial crystal work uses it; deferred behind visible/ordinary movement.|
|211|MakeSomeoneOnThisBlockPickUpSomethingOnThisBlock,0x457130,kind4|Builds a local dispatch record, invokes0x456cb0, returns its output. No current handler. Full pickup semantics are not established by this small slice; defer.|

The earlier negative claim that the object registry lacks a feeder is stale:
current entity.cpp has reset/append/removal and a real OList append call. Its
old entity.hpp paragraph is outdated. Reuse that production registry and actual
selection predicate; do not invent a new pointer table or return an arbitrary
selected index. No native64 concern is part of this investigation.

A linear structural walk of shipped M01.npl using the candidate's293-row argument
metadata identifies calls/indirect references to these IDs. The resulting word
offsets are external `npl-command-offsets.json`; it does not execute bytecode or
prove all branches reachable. Source companion M01.nrn independently describes
the SelectRaider gate. The correspondence/interpreter runtime remains unverified.
No pure-Python reimplementation was used as a candidate correctness test.

Immediate bounded follow-up is to recover and port the first three contracts
against actual candidate owners. Missing UI NEXT hit/render ownership and the
scroll-state representations must stay explicit if unresolved. A returned
adapter value, stored token, or higher score cannot pass tutorial interaction.
