# Finding 69 — AVI setup failures leave staged state for later teardown

*Target: shipped `LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.*
*Provider: Ghidra 12.1.4 via REA. Evidence:
`docs/evidence/legorr-avi-playback-expansion.bundle`.*

The AVI object is initialized in stages. The first successful stream API call
does not make it playable by itself: a frame retriever and stream-info record
must also succeed. The staged layout matters on failures because stream
acquisition can happen before the object's initialized byte is set.

## Staged initialization

`FUN_0047ec90` allocates a `0xc4`-byte object and runs its constructor
(`ev_7efc758007f3fd078ea1433c9765d912c52a92a1d46a84710faefcd6101735ec`).
The constructor zeroes state, installs its method table, and skips AVI setup
when global byte `DAT_00533998` is nonzero. Otherwise it runs this sequence:

1. `AVIStreamOpenFromFileA` for stream type `vids`; nonzero return exits the
   setup path.
2. `AVIStreamGetFrameOpen`; its returned pointer is saved in the object, and a
   null pointer exits the setup path.
3. `AVIStreamInfoA`; nonzero return exits the setup path.
4. Set the initialized byte, reset the frame index, capture the clock base,
   then request the surface-side object.

The method table's state is zeroed before those branches, so failures before
step 4 leave the initialized byte clear. A stream acquired at step 1 remains
stored even if step 2 or 3 fails. The destructor later releases that stream
when it runs while `DAT_00533998` is clear
(`ev_74608d1b910c4f9d9e8af66f68212b8342b23d6fffec0143b36fd28b356fed7d`,
`ev_a6ae79f2593370da78def4885cadaa80165242c1ac9ad745d9e65d83afd649cc`).

The surface-side allocation occurs after the initialized byte is set. If its
allocator returns null, the constructor stores a null surface pointer without
clearing that byte. The frame method checks the initialized byte before it
requests a frame; its bitmap-copy helper then dereferences the surface-side
pointer. This is a statically visible inconsistent failure state, not a claim
that a particular shipped movie reaches it
(`ev_74608d1b910c4f9d9e8af66f68212b8342b23d6fffec0143b36fd28b356fed7d`,
`ev_6e8a4593cddf3e6be5a71ae2b65a4625fcf250f71316c9b9ca1153c7b4bd7e0c`,
`ev_29f2efd95466f08648c6e8e6f639bcc75eab1b68ec23208ff9838ead7a7a87ee`).

## Teardown and limits

`FUN_0047e9a0` replaces the object's method-table pointer, then, when
`DAT_00533998` is clear, releases a non-null stream handle and invokes the
first method of a non-null surface-side object. The wrapper
`FUN_0047e980` calls this destructor path and optionally performs an adjacent
cleanup when its low flag bit is set (`ev_a6ae79f2593370da78def4885cadaa80165242c1ac9ad745d9e65d83afd649cc`,
`ev_acaf783c7550c928c0f942740e722469493211187d5453f5bbc0892c95cd8d90`).
Because stream release is behind the global-byte check, cleanup behavior
when that byte is set is not established here.

The observed destructor does not close the AVI frame retriever, and the shipped
executable imports no `AVIStreamGetFrameClose` function (finding 174). This
establishes the absence of an explicit close path in the binary. Whether releasing
the AVI stream also reclaims those decoder resources internally remains unknown.
No file was executed, and this finding makes no claim about decoder or audio
teardown.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Stream, frame-retriever, and stream-info setup must succeed before the initialized byte is set | observed in shipped code |
| Stream acquired before a later setup failure remains stored for the destructor | observed in shipped code |
| Surface allocation failure leaves the initialized byte set and surface pointer null | observed branch state; runtime reachability not measured |
| Teardown runs when the global byte blocks stream release | unknown |
| Exact frame-retriever ownership and close behavior outside this destructor | unknown |
