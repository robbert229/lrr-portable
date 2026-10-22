# 115 — PTLFile builds an ordered message-name remap table

During LevelLoad, the level group's PTLFile value is passed to a loader that opens the file through
the generic block-text parser. The loader finds the executable-root group, iterates its direct
children in file order, translates each child's name and value through the case-insensitive
Message-name lookup table, and stores the resulting numeric pair in a per-level array. The enum
includes names such as Message_Null, Message_Select, Message_Dig, Message_DigComplete, Message_Goto,
and Message_RockFall.

This establishes PTL as an ordered list of message-name pairs materialized as numeric IDs. It closes
the PTL consumer portion of finding 07 and adds detail to finding 12's statement that PTL uses block
text.

**Evidence.** REA/Ghidra 12.1.4, shipped LegoRR.exe, SHA-256
c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f. Bundle:
docs/evidence/legorr-ptl-consumer.bundle. Focused records:
ev_a27cc2cb66787dcd36dd8af2f078036b9a5bdd28e80efc8f2fc5b33e1e667009 (LevelLoad reads PTLFile and
calls the loader); ev_91134c6afb65a0295af1ee3f174fff9d8c5d6a9e765e405923a555c539f2c704 (resolved
loader caller); ev_fd7311232aa686d4a6eac53ded4137d5ea52f292683b00529707938d73531452 (ordered
iteration, name/value translation, and pair storage);
ev_59b8e5e56f8d36c72052d2061ad227522899aea19d7e2db960c331eb901a1afa (Message enum names);
ev_e2e23eda5833c5e180a58b0795c110c22f5368fec504e818a796ffeb65c30cc1 (case-insensitive enum lookup).

**Limits.** The evidence proves table construction and pair storage, not a later consumer or
dispatch behavior. No runtime semantics are assigned to individual pairs.
