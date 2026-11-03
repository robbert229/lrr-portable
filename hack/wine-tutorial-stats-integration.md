# Win32 tutorial Stats integration — 2026-11-02

The captured Toolstation null-Stats fault is repaired. The final PE32/MSVC5
candidate reaches Moving Rock Raiders through normal settings/menu navigation,
shows its mission brief, and dismisses it without that fault. **Playable tutorial
acceptance remains incomplete:** the scene is very dark with no clearly visible
Raiders. Clean shutdown failed; the owned candidate required SIGTERM after normal
close requests failed. Movement, pause and input patches remain unintegrated at
this checkpoint. No commits, pushes, PRs, delegates or native64 tests.

Packet: `build-reccmp/runtime-evidence/20261102T212003Z-tutorial-stats-integration/manifest.json`.
Paths below are relative to that packet unless qualified.

## Actual actor, caller and ownership

One observational repeat of the archived failure used a temporary naked getter
probe, preserving registers, logging the original return address/32 stack words,
then performing the original unchecked null read. `diagnostic-1-stats-probe.log`
records:

- Actor `0x0249c2b8`, **Object2**, kind 4, placeholder row 4, empty `type_name`,
  task state -1, Stats index 0, null record/table, span 0.
- Actual level `MoveTuto_01`; return address **0x416cb7**. The diagnostic report
  maps this call into `entity_kind_four_post_create_resolve`, entry 0x416c20,
  reference 0x43c570. The raw fault remains read 0x148, diagnostic EIP 0x414b49.
- No bind-entry event preceded the first null getter. OList had passed its
  instance label `Object2` into a creator that resolves this argument as a type
  name. Kind-four post-create ran before OList's subsequent Toolstation/row-0
  assignments. This is not a special static-template actor.

`diagnostic-1-game-fault.txt` preserves the actual game-thread exception; Wine's
later debugger crash and service exceptions are separate. The immediate caller
mapping is authoritative. Other raw stack words are not a fully unwound stack;
the nearest annotated function before an unannotated address is not proof of its
dynamic frame identity.

The independent audit is
`/var/home/johnrowl/.t3/worktrees/lrr-portable/t3-a49214f2/hack/wine-tutorial-stats-audit.md`.
Its predicted caller was a hypothesis until this capture. Its production config
scope finding also holds: BlockFile retains the `Lego*` hierarchy, while this
loader family queried unqualified sections. `wad-config-source-identity.json`
independently hashes the real WAD/config/OList/.ae members, using O_NOATIME reads.
The config hash is `20cbf383bc672b0c330fe45e5fc509371de93d0b46958af82ac1b71f10fccb4f`;
the tutorial OList hash is
`7d24423a6cd217d1f45ea8def2d27dbca58d5789df14f937e72c7f09dcbaa7a5`.

## Production changes

Only `src/game/entity.cpp` and `src/game/entity_config.cpp` changed from the
handed-off startup snapshot:

1. OList passes `op.type_name` to `entity_create_with_position`, so the existing
   production creator binds the configured row before kind-four post-create.
   After creation it restores `op.name` as the instance label. The raw getter,
   entity layout, movement/AI/pose functions and allocation lifetime are unchanged.
2. A bounded loader-local helper qualifies section paths beneath
   `util_root_name(config)`. The type tables, building-upgrade/name tables,
   ObjectNames/ObjectTheNames and Stats use that scope; ToolNames uses the same
   root. Global BlockFile lookup semantics are unchanged. Type-name installation
   still precedes Stats allocation/binding.

The loader-owned `s_entity_stats_record_allocations[kind][row]` remains the real
heap owner. Entities borrow `base + index*0x150`; no dummy Stats, null-return
guard, actor skipping, static-template substitution or SEH recovery was added.
Existing .ae/resource rejection gates were not weakened. The pre-existing
metadata-only behavior when visual runtime operations are absent is **not** a
validated authored-visual load; see the next blocker below.

All temporary probes were removed before the final build. The eight startup
files, dirty miniwin and existing root notes remain intact. Complete final source
patch SHA256:
`82763536eccc597a7585b92302ccfd31c0330816244ed38b4b8f52b44101f91e`.
Stats-only patch: `stats-only.patch`, SHA256
`64e20316d7b89e931b70105cb031c655913135ee93de14bae722b7d1065a7334`.
Raw original/diagnostic source copies and the focused test source are outside the
repository under `/tmp/20261102T212003Z-tutorial-stats-integration/`.

## Build and focused Win32 validation

Final candidate SHA256:
`c869d3155092b78aa1180d749f7b2db429d1a447b35a56c5bcbec0b9e3ed819c`.
`repair-build/` retains EXE, PDB, complete full/game reports, source hashes and
source patch. Before assets are `before-LegoRR.exe`, `before-LegoRR.pdb` and
`before-reccmp-report/`; diagnostic assets are separate. `repair-build.log` is
the successful `distrobox enter developer -- make reccmp-report` output.
One failed compile, retained as `repair-build-1-type-error.log`, exposed the old
ToolNames API's `void*` root parameter; the borrowed root is now explicitly cast
at that existing read-only interface. `git diff --check` passed.

Final reports: full **2332/2396, 97.33% implemented, 52.87% accuracy, 51.46%
progress**; game-only **2113/2137, 98.88%, 48.43%, 47.88%**.
`repair-report-comparison.json` compares every row with the pre-phase production
report: no new/missing mappings or regressions. Only OList/reference 0x42e020
changed, 10.9665% to 11.1824%. The separate temporary diagnostic build's getter
score decrease and unedited 0x4388b0 shift are retained in its own comparison;
neither persists in the final report.

The focused console test is PE32, compiled with the same MSVC5 flags and linked
against the frozen production objects/libraries. It mounts the actual WADs,
calls `entity_names_config_load`, creates Toolstation/Pilot through the production
creator (including the kind-four callback), checks the selected real records,
SelfPowered/RouteSpeed, and verifies span boundaries with the production index
setter. `probe-runtime.log`/`probe-result.json`: **exit 0, failures 0**, Toolstation
row 0/span 3, flags 0x800400; Pilot row 0/span 4, RouteSpeed 1.1. This is a Stats
boundary test, not gameplay/visual acceptance. There was no useful isolated test
of the complete UI bug before the fix; the actual archived red runtime plus the
new diagnostic capture supply that baseline.

## Equivalent runtime pair and next blocker

`reference-pair` and `candidate-pair` used normal Make targets, identical Proton
runner/preloader, compatdata, effective relevant environment (including
WINEDEBUG=-all), reference cwd and restored starting bytes/metadata. Both selected
fullscreen 640x480x16. `*-effective-context.json`, `events.jsonl`, screenshots and
launch/result JSON retain the identities/actions.

Reference: Training -> Moving Rock Raiders -> two-page mission brief -> visible
scene/actors -> mission Quit/Yes -> Training Back -> Quit Game/Yes -> exit 0.
This round did not validate commanded movement or pause/resume behavior.

Candidate: real menu selection -> mission brief -> two Space actions -> running
dark/coarse scene. `candidate-pair-tutorial-entry.png` and
`candidate-pair-after-brief.png` show the new boundary. Read-only GDB snapshots
afterward locate the PDB-owned GameContext, validate pool self-pointers/owner,
and capture the actual actors and installed table/span data:

| Actor | Kind/row | Position x,y | Selected record |
| --- | --- | --- | --- |
| Object2 / Toolstation | 4/0 | 240, 0 | base 0x2243150, index 0, span 3, flags 0x800400 |
| Object3 / Pilot | 2/0 | -241.5825, -83.2865 | base 0x223b2d0, index 0, span 4, RouteSpeed 1.1 |
| Object4 / Pilot | 2/0 | -242.7190, -1.5857 | same Pilot base/index/span |
| Object5 / Pilot | 2/0 | -240.0446, 77.1946 | same Pilot base/index/span |

Counts are [0,3,0,1,0]. See `candidate-live-actors.json`,
`candidate-live-stats-bindings.json`, raw memory snapshots and PDB-derived layout
JSON. Capture timing is explicit: after brief dismissal and subsequent attempted
navigation, with application phase 3/gameplay_prepared still set. No further
source instrumentation or recovery occurred.

**Next blocker:** recognizable playable actors are absent from the image.
`s_entity_type_runtime_ops` is entirely zero in live memory; authored visual
ownership is not installed. Independently, `scene_render.cpp:946` treats entity
positions as grid coordinates and multiplies by block size, whereas the captured
positions are world coordinates. The queued movement patch addresses that known
contract; neither observation establishes the sole visual cause. No renderer,
objective, movement or pause repair was attempted. Repeated objective word
0xb1 is retained as an observation in the archived stub log.

Escape returned the display to the Training picker; later back/Escape input,
Alt-F4 and window-manager close did not end the process. This is **not** a proven
pause/resume or menu-quit result. `candidate-forced-cleanup.json` records the exact
verified-PID **SIGTERM to 3862364**, after evidence capture, to release the owned
run for offline restoration. Make exit 2/child Error 241 follows that signal; it
is not attributed to a new application exception. No shared Wine process was
signaled. GDB sampling found live execution (including candidate PC 0x451d39 on
one attach and Wine delay on others), not a captured new fault EIP. Clean shutdown
gate **FAIL**; playable T **PARTIAL**; E/P **UNVERIFIED**.

## Preservation, cleanup and ownership checkpoint

Before launches, full independently verified byte backups and stable inventories
cover **484 reference and 2218 compatdata entries**, ownership/modes/timestamps,
xattrs/ACLs, targets and absent paths. `immutable-backup/` and
`baseline-inventory.json` retain the designated current baseline. Earlier original
5.sav remains unrecoverable; the preserved current baseline hash is
`41ceff64d8d7d8590c7828cf72e76d32263c2ff929c6f831e179d19ad36ea69a`.

Every completed run's changed state was archived before restoration. Registry
value diffs were reviewed offline: Wine device ContainerIds/display state and
runtime timestamps, with no game-key changes. Restoration rechecked archived
post-run state, performed verified atomic/fsynced replacement when required,
and checked all restorable metadata. No live/shared-registry restore occurred.
`final-restoration.json`: **zero differences**. ctime/inode are recorded but not
restorable by these filesystem operations. Protected PIDs 2821989/2822012 remain
untouched; no owned game/debugger remains.

Preservation/procedure limitations are retained explicitly:

- The inherited scanner initially changed symlink atimes through readlink; only
  those scanner effects were restored, then the scanner was corrected and stable
  full backup readback verified before the first game launch.
- The focused-test precheck rejected a directory atime difference, but its shell
  wrapper continued. Bytes/prefix content were unaffected; the completed test was
  archived/restored. Subsequent game launches use fail-closed `launch_guard.py`.
- `reference-pair-after` was an intermediate live archive taken before the final
  Yes confirmation; it is **not restoration authority**. The post-exit
  `reference-pair-final-after` archive was used for restoration.
- Reading the live stub log after restoration changed only its atime. That
  metadata delta was recorded, content/other metadata reverified, and only the
  captured atime restored. Use packet log copies for further inspection.

The queued replacement movement patch still passes read-only `git apply --check`
after this repair; it was not applied. Stats phase source/report ownership returns
to root at this checkpoint. The same sole integration/build/desktop owner retains
the seat only for the explicitly authorized, separate conditional input phase;
that phase has not started and this checkpoint claims no input runtime pass.
