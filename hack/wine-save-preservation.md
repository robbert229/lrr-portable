# Wine reference save preservation — 2026-11-02

Exact original recovery is **unavailable from the bounded local evidence**.
`reference/Data/Saves/5.sav` was **not restored or written**. Its current changed
bytes were preserved before recovery inspection. This closes the recovery search;
it does not complete gameplay validation or erase the baseline preservation loss.

- Original SHA256: `e33adbaa64b671252402f4319e1c307be5bdd79319272d128ccee73a3da889e7`.
- Current / baseline after-runtime SHA256: `41ceff64d8d7d8590c7828cf72e76d32263c2ff929c6f831e179d19ad36ea69a`.
- Both original and current size: 418,880 bytes. Size equality supplies no recovery authority.
- Original mtime_ns: `1793585878389643679`; current mtime_ns: `1793651727688030361`.

Authoritative baseline: `build-reccmp/runtime-evidence/20261102T202607Z-msvc5-baseline/manifest.json`,
including its cleanup preservation limitation; also read `initial-files.json`,
`fresh-files.json`, `cleanup.json`, `hack/wine-runtime-baseline.md`, root `AGENTS.md`
and `/var/home/johnrowl/.agents/skills/chief-of-staff/SKILL.md`.
The explicit no-delegation instruction governed this preservation task.
The initial fingerprint records the original hash, size and mtime only. The fresh
fingerprint records executable/PDB/report assets, not an original save backup.

Evidence directory: `build-reccmp/runtime-evidence/save-preservation/20261102T204335.557017Z/`
(verified ignored by git). `current-5.sav` is the first-action byte backup;
`preservation.json` records its verified hash, original inode/device, size, mode,
uid/gid, nanosecond atime/mtime/ctime and xattrs. Reads of existing save candidates
used `O_NOATIME` and `O_NOFOLLOW`; no save metadata was deliberately altered.
`baseline-provenance.json` identifies and hashes the authoritative metadata inputs.

The final target check still matches the baseline changed-state hash, the
preserved bytes and every metadata field captured at entry, including inode,
mtime and ctime. This supports no intervening change during this inspection;
byte equality cannot prove that identical bytes were never rewritten earlier.
`final-target-check.json` records that limitation explicitly. `process-check.json`
records no LegoRR/winedbg process name; only the two pre-existing winedevice.exe
PIDs 2821989 and 2822012 were found. No processes were signaled or launched.
The integrator's completed cleanup independently records no owned game/debugger.

The bounded search inventoried 730 files under `reference/`, root `Saves/`,
`save_fixture/`, and every existing `build*/runtime-evidence/` (only
`build-reccmp/runtime-evidence/` existed). Repository filename discovery also
located mission/save fixture copies under root and `build/`, and the known
`docs/evidence/legorr-rev-entity-snapshot.gzip`. No backup directories or additional
save archives were discovered. Directory symlinks were not followed; `.git/`,
`3rdparty/`, `subprojects/` and `.agents/` were excluded from backup-name discovery.
No home/system recovery search, network, git checkout/reset, build, test or game
launch was performed. Source files and the dirty miniwin submodule were not edited.

Checked 46 regular files (35 `.sav` files), including every save copy found,
`.osf` companions in the search roots, all files exactly the original length
under the reference/evidence roots, and the named snapshot. None matched the
original hash. The snapshot's bytes begin with JSON (`{"`), so its misleading
`.gzip` suffix cannot be treated as a compressed save; a single bounded gzip
read rejected its header, as recorded in `recovery-search.json`. Its complete
raw bytes were hashed and did not match. No candidate file was unreadable.
The baseline after-runtime copy matches only the changed state. No reconstruction,
conversion, same-size substitution or other-slot downgrade was attempted.

`recovery-search.json` is the complete candidate/hash record;
`search-inventory.json`, `repo-local-backup-name-inventory.json`, `search-roots.json`
and `search.py` record the bounded search and its selection rules. The new
preservation copy was excluded as an independent recovery source.

Before any subsequent serialized runtime validation:

1. Assign one integration owner, stop launches, confirm owned games/debuggers
   have exited normally, and exclude concurrent writers. Keep unrelated source
   work and shared Wine processes untouched. Carry forward this unresolved
   original-save loss; any newly designated starting state must explicitly be
   the changed state, never presented as the recovered original.
2. Before launching either binary, copy **bytes and metadata**, not just hashes,
   into a unique ignored evidence packet. Snapshot the complete `reference/`
   tree to cover every mutable save/settings path: both `Data/Saves/` and
   `Saves/`, every slot and `.osf` companion, configuration/settings files,
   logs, directory entries and previously absent paths. Record relative paths,
   SHA256, size, ownership/mode, timestamps, xattrs/ACLs and symlink targets.
   Independently hash the backup bytes and verify a stable source snapshot.
3. Include game settings persisted outside the reference tree in the explicitly
   owned Wine prefix (registry and game configuration/save locations). Establish
   a consistent snapshot with no concurrent prefix writers; if shared Wine
   state cannot be isolated safely, defer the launch. Do not terminate shared
   Wine services or blindly restore a shared registry. This task did not inspect
   or copy private prefix data.
4. Run reference and candidate one at a time with the same recorded starting
   state. After each exit, preserve the complete changed state and logs first,
   then compare every path/hash and metadata field with the prelaunch inventory.
   Record created/deleted files too. Recheck the current hashes immediately
   before restoring; if any unowned/intervening edit is possible, stop restoration.
5. Restore only verified snapshot bytes to the intended paths, using a flushed
   temporary file in the target directory and atomic replacement; restore the
   recorded metadata and fsync the directory. Handle creations/deletions only
   when attributable to the owned run and already preserved. Verify the entire
   restored inventory, hashes and metadata, record provenance and before/after
   evidence, and release the next launch only after that verification passes.

This is a required preservation protocol, not a launch-wrapper/application fix.
No commits, pushes or PRs were made.

Candidate byte fingerprints:

| Path | Bytes | SHA256 |
| --- | ---: | --- |
| `Saves/5.osf` | 8 | `2b98e54cf7d42ec26d499f6f3c4185b0d0f91925f838efd567de6d7cf376a171` |
| `Saves/5.sav` | 63616 | `336136708b196865ea8f412c83377df1f5b99bb8dbd06bda069d042a918df602` |
| `build/mission_fixture/Saves/5.sav` | 63616 | `50c8997ff42b25f2df7b2e58f4213120ac3cb15617076a14c7147f092f38fa38` |
| `build/save_fixture/Saves/0.sav` | 63616 | `a25f9c84b05c80965c0153099d8b4f5386f16c71a9e4664ead4c0e22ddab1068` |
| `build/save_fixture/Saves/1.sav` | 63616 | `a25f9c84b05c80965c0153099d8b4f5386f16c71a9e4664ead4c0e22ddab1068` |
| `build/save_fixture/Saves/2.sav` | 63616 | `a25f9c84b05c80965c0153099d8b4f5386f16c71a9e4664ead4c0e22ddab1068` |
| `build/save_fixture/Saves/3.sav` | 63616 | `a25f9c84b05c80965c0153099d8b4f5386f16c71a9e4664ead4c0e22ddab1068` |
| `build/save_fixture/Saves/4.sav` | 63616 | `a25f9c84b05c80965c0153099d8b4f5386f16c71a9e4664ead4c0e22ddab1068` |
| `build/save_fixture/Saves/5.sav` | 63616 | `a25f9c84b05c80965c0153099d8b4f5386f16c71a9e4664ead4c0e22ddab1068` |
| `build/src/game/mission_fixture/Saves/5.sav` | 63616 | `50c8997ff42b25f2df7b2e58f4213120ac3cb15617076a14c7147f092f38fa38` |
| `build/src/game/save_fixture/Saves/0.sav` | 63616 | `a25f9c84b05c80965c0153099d8b4f5386f16c71a9e4664ead4c0e22ddab1068` |
| `build/src/game/save_fixture/Saves/1.sav` | 63616 | `a25f9c84b05c80965c0153099d8b4f5386f16c71a9e4664ead4c0e22ddab1068` |
| `build/src/game/save_fixture/Saves/2.sav` | 63616 | `a25f9c84b05c80965c0153099d8b4f5386f16c71a9e4664ead4c0e22ddab1068` |
| `build/src/game/save_fixture/Saves/3.sav` | 63616 | `a25f9c84b05c80965c0153099d8b4f5386f16c71a9e4664ead4c0e22ddab1068` |
| `build/src/game/save_fixture/Saves/4.sav` | 63616 | `a25f9c84b05c80965c0153099d8b4f5386f16c71a9e4664ead4c0e22ddab1068` |
| `build/src/game/save_fixture/Saves/5.sav` | 63616 | `a25f9c84b05c80965c0153099d8b4f5386f16c71a9e4664ead4c0e22ddab1068` |
| `build-reccmp/runtime-evidence/20261102T202607Z-msvc5-baseline/reference-Data-Saves-5-after-runtime.sav` | 418880 | `41ceff64d8d7d8590c7828cf72e76d32263c2ff929c6f831e179d19ad36ea69a` |
| `docs/evidence/legorr-rev-entity-snapshot.gzip` | 3106232 | `2bd897d71526409745194f71bc13de1f71e7a215e02c6af42745b7d31b388e95` |
| `mission_fixture/Saves/5.sav` | 63616 | `50c8997ff42b25f2df7b2e58f4213120ac3cb15617076a14c7147f092f38fa38` |
| `reference/Data/Saves/0.sav` | 418884 | `67a48afe6b9e84328a3656d5e170d7e83a61ba57876cd0b9ae9a3c6d0f5a7627` |
| `reference/Data/Saves/1.sav` | 418884 | `67a48afe6b9e84328a3656d5e170d7e83a61ba57876cd0b9ae9a3c6d0f5a7627` |
| `reference/Data/Saves/2.sav` | 418884 | `67a48afe6b9e84328a3656d5e170d7e83a61ba57876cd0b9ae9a3c6d0f5a7627` |
| `reference/Data/Saves/3.sav` | 418884 | `67a48afe6b9e84328a3656d5e170d7e83a61ba57876cd0b9ae9a3c6d0f5a7627` |
| `reference/Data/Saves/4.sav` | 418884 | `67a48afe6b9e84328a3656d5e170d7e83a61ba57876cd0b9ae9a3c6d0f5a7627` |
| `reference/Data/Saves/5.osf` | 8 | `2b98e54cf7d42ec26d499f6f3c4185b0d0f91925f838efd567de6d7cf376a171` |
| `reference/Data/Saves/5.sav` | 418880 | `41ceff64d8d7d8590c7828cf72e76d32263c2ff929c6f831e179d19ad36ea69a` |
| `reference/Saves/0.sav` | 12864 | `1f7212c7cd4861c746efb564085d6d72a56142bdb04e19f369145d7a492d0355` |
| `reference/Saves/1.sav` | 12864 | `1f7212c7cd4861c746efb564085d6d72a56142bdb04e19f369145d7a492d0355` |
| `reference/Saves/2.sav` | 12864 | `1f7212c7cd4861c746efb564085d6d72a56142bdb04e19f369145d7a492d0355` |
| `reference/Saves/3.sav` | 12864 | `1f7212c7cd4861c746efb564085d6d72a56142bdb04e19f369145d7a492d0355` |
| `reference/Saves/4.sav` | 12864 | `1f7212c7cd4861c746efb564085d6d72a56142bdb04e19f369145d7a492d0355` |
| `reference/Saves/5.osf` | 8 | `2b98e54cf7d42ec26d499f6f3c4185b0d0f91925f838efd567de6d7cf376a171` |
| `reference/Saves/5.sav` | 12864 | `1f7212c7cd4861c746efb564085d6d72a56142bdb04e19f369145d7a492d0355` |
| `save_fixture/Saves/0.osf` | 8 | `2b98e54cf7d42ec26d499f6f3c4185b0d0f91925f838efd567de6d7cf376a171` |
| `save_fixture/Saves/0.sav` | 63616 | `a25f9c84b05c80965c0153099d8b4f5386f16c71a9e4664ead4c0e22ddab1068` |
| `save_fixture/Saves/1.osf` | 28 | `83fe77ade92e6ca79ad9929a2e154eee9b6925b958ff563564b5b737f6dbcf0f` |
| `save_fixture/Saves/1.sav` | 63616 | `163c73eb3431901283c6df5ddd6cf9d1ef22143654281fadd21a851535e27a83` |
| `save_fixture/Saves/2.osf` | 8 | `2b98e54cf7d42ec26d499f6f3c4185b0d0f91925f838efd567de6d7cf376a171` |
| `save_fixture/Saves/2.sav` | 253936 | `868098195ca0c18701c4cae876dac41f71b83d793208f8e5a16aa4f911dbb71d` |
| `save_fixture/Saves/3.osf` | 48 | `8669212adb90035d73fcc9e58ece1652d4b169ae2167150e87c3f6247c1c7dc3` |
| `save_fixture/Saves/3.sav` | 63616 | `fbfde3a2e49939ab3bd114131fbd5810d38cb61a32711f38ebef98214e287270` |
| `save_fixture/Saves/4.osf` | 48 | `8669212adb90035d73fcc9e58ece1652d4b169ae2167150e87c3f6247c1c7dc3` |
| `save_fixture/Saves/4.sav` | 63616 | `163c73eb3431901283c6df5ddd6cf9d1ef22143654281fadd21a851535e27a83` |
| `save_fixture/Saves/5.osf` | 48 | `8669212adb90035d73fcc9e58ece1652d4b169ae2167150e87c3f6247c1c7dc3` |
| `save_fixture/Saves/5.sav` | 63616 | `92066a10ce03ca960bf535bb45fe4a92998c7d3f0623187f63714472093d9035` |
| `save_fixture/Saves/6.osf` | 2 | `0049aa51a630b7912728071542a064ffa7d385e0d14429bd59310a8347505815` |
