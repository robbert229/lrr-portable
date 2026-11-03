# Wine scene-height integration checkpoint

Bounded phase stopped 2026-11-03 UTC. **The reviewed five-file height repair is
integrated; real loaded-data PE32 height red/green and the review regression
boundaries pass. Ordinary Moving tutorial briefing, sustained scene,
pause/resume and WM close exit 0 are preserved. Ordinary selection/movement
and complete NEXT progression remain OPEN.** Sole shared ownership is retained
pending explicit root transfer. No further terrain/model/parity work starts here.

Evidence packet:
`build-reccmp/runtime-evidence/20261103T001147Z-sceneheight-integration/`.
Read `checkpoint-result.json`, `final-frozen-identity.json`,
`scene-observation-and-regressions.log`, `all-runtime-attempt-results.json`,
`paired-effective-context-comparison.json`, and `final-restoration.json`.

## Retained source and production identity

Entry HEAD remains dirty `dc7f167b1a4b19efe65a35fcb5523fdf61e62fc1`.
Verified the previous shutdown packet's 176 source hashes, miniwin diff,
EXE `63cc5b0403c6f50a7cb82417d1ef8ec128ea30d65899b86ca4f7d9d7a1aa69de`,
and PDB `12679761cca0de76d4f6b0084fea3789d8eb46af1c3946573fbc74f8f8d3bc4f`.
The frozen backup's bytes and live full inventories matched before work.

Applied **only**
`/var/tmp/wine-sceneheight-root-corrections-20261103/sceneheight-contract.patch`,
SHA256 `671eb5b36da0063042bf09d0a9f19ed3a75ffa10b546ad89de694f454a372f51`.
The original four-file patch was not applied. The external handoffs, original
complete handoff, manifest and independent review were read. They remain
unchanged external artifacts. Exact result hashes match all five reviewed files:

- `src/game/level.cpp` / `level.hpp`: CPU geometry Y seeding, retained constructor
  scale, and no-CPU mutation presentation seam.
- `src/game/scene_render.cpp` / `scene_render.hpp`: geometry-Y cache, full-triple
  orientation, shared terrain/actor floor, and shared triangle projection rejection.
- `src/game/terrain_grid.cpp`: **only** the physical-cardinal getter caller hunk.

All other **171** source hashes and the dirty miniwin diff match entry.
The temporary frame-loop include/invocation is removed byte-for-byte back to
entry. FUNCTION/GLOBAL annotation lines in all five files remain unchanged.
The earlier `tutorial_hud.cpp` typed-text fix remains SHA256
`8975555b09afef4f5720ab1f4dd8793061b4bc0658673da8bb00c1d751981d26`.
No pointer/NEXT/history/callback, harness, Makefile, asset or desktop-setting
repair was integrated. No delegates, commits, pushes, PRs, merge, native64 or
SDL work occurred.

Final production freeze, unchanged during ordinary replay and subsequent probes:

| Artifact | SHA256 |
| --- | --- |
| EXE | `c7e28d4b19a193ba765af3db8ded5f72a3d557e09daadb6a91e8890ea1957788` |
| PDB | `fe41960370d2254cc4f377bc0ebbe81319366491c507170867fb86b09d6154c5` |
| CMake cache | `7473b486394bf5315db88c6da159c0bb91304bc18ee1120f45ab45221f741525` |
| Full results JSON | `573b959d064151c9f082b7f929121a6507fa4ec0c0c3cbab16e8f1233e7a83e9` |
| Game results JSON | `df0b373be50701da00c72c8115ef702713c950aaa54e26b53612e7886eaa09d6` |

`production-build/` preserves complete EXE/PDB/cache and both HTML/JSON reports.
All source and report hashes are in the final identity. Raw C++ snapshots and
probe sources are outside repository/source scans at
`/var/tmp/lrr-sceneheight-integration/` and
`/var/home/johnrowl/.cache/lrr-sceneheight-integration/`.

## Executed real loaded-data height red/green

The delivered `pe32-loaded-height-probe.cpp` was used unchanged. A temporary
frame-loop invocation ran immediately after the actual `level_load` pipeline
in separate disposable MSVC5 PE32 processes. Diagnostic launches used the
existing `LEGORRR_LEVEL=MoveTuto_01` override, **not ordinary menu acceptance**.
No injected state from either process is credited as gameplay.

Both runs logged actual **MoveTuto_01**, **30×26**, BlockSize **40**, RoughLevel
**6**, UseRoof **1**, DigDepth **40**, RoofHeight **40**. The indexed CPU mesh
reader and scene getter were independent production owners, not duplicated
conversion formulas. Both found actual cell **(8,10)**, flags **0x0440000f**,
and a real `terrain_cell_mutation_progress` changed **all four CPU corners**.

| Observation | Baseline diagnostic | Patched diagnostic |
| --- | ---: | ---: |
| Before: corner comparisons | 2900 | 2900 |
| Before: failures, including center checks | 3547 | 0 |
| After real mutation: corner comparisons | 2900 | 2900 |
| After: failures, including center checks | 3547 | 0 |
| Probe verdict | 1, red | 0, green |

For example, the initial baseline cell (0,0) read CPU Y **-72**, scene Y **-48**;
the patched reads both return **-72**. More failures than corner comparisons
reflect additional per-cell center assertions, not additional corners.
`red-height.log` / `green-height.log` retain every observation. The red/green
EXEs are separately archived and identified; neither is the production EXE.

Commands were generated/executed by the packet's `direct-diagnostic-run.py`:

```sh
python3 <packet>/direct-diagnostic-run.py red-direct-env tutorial ordinary
python3 <packet>/direct-diagnostic-run.py green-direct-env tutorial ordinary
```

The exact executed version is saved in each run's `exact-script.py`. Here
`ordinary` selects WINEDEBUG=-all; these remain instrumented, direct-level
**diagnostics**, not ordinary acceptance. Parent's read-only diagnostic audit
independently confirmed the delivered probe bytes, real mutation and limits.

## Build, complete reports and fidelity disclosure

All production/diagnostic report builds were serialized commands:

```sh
distrobox enter developer -- make reccmp-report
```

Red diagnostic, green diagnostic and final production builds passed. The first
red compile attempt failed C1083 because host `/var/tmp` was not visible at the
same container path; moving only the external include/output to shared `/home`
resolved it. Full logs/results for the failed and passing attempts are retained.
Production retains **/O2 /Ob1**, /ML, /Zi and the unchanged CMake cache.
`git diff --check` passed. No score-only source or compiler-flag sweep occurred.

Final versus entry: full **2332→2332**, game **2113→2113** compared mappings;
**zero new/lost mappings**. Every row is compared, including complete changed
rows/disassembly and all changed fields. Full **1887** / game **1668** rows have
some field difference, including recompilation addresses and disassembly.
Exactly seven matching scores change in both reports:

| Address / function | Before | After |
| --- | ---: | ---: |
| 0x4089d0 segment_2d_point_distance | 36.4444% | 36.2832% |
| 0x40e9c0 lava_erosion_delay_from_cell | 84.2105% | 100% |
| 0x42b510 map_pipeline | 20.8531% | 27.0073% |
| 0x444fb0 EntityTurnToVectorActivityIssue | 25.5556% | 26.0388% |
| 0x44fb90 terrain_state_surface_point_project_offset | 22.8916% | 22.4000% |
| 0x4512d0 terrain_cell_normal_sample | 9.5745% | 8.6486% |
| 0x4882e0 scene_node_transform_matrix_apply | 16.1468% | 12.1547% |

The map-pipeline gain follows the changed real seeding loop. Other changed
mapped bodies have unchanged source. `decrease-code-diffs.txt` preserves actual
before/after candidate instruction differences: the segment helper reorders
spills/squares and adds FXCH; the surface-project helper reschedules x87
component differences and its branch offsets change; the normal helper changes
commutative sums, equivalent address operands and register scheduling.
**The matrix drop includes substantial real x87 scheduling/spill instruction
changes, not merely relocated addresses.** Its source, view.cpp hash, cache,
flags and miniwin remain unchanged. These facts do not independently prove
semantic equivalence or a runtime regression. Parent's fidelity review is
retained. No additional rebuild was performed solely for that note.

Removing the frame-loop diagnostic causes **zero score changes and zero
new/lost mappings** relative to green. `diagnostic-removal-*-comparison.json`
contains the complete immediate comparison. Final totals: full **97.33%
implemented /52.82% accuracy /51.41% progress**; game **98.88% /48.37% /47.82%**.

## Ordinary production run: menu, Moving, pause/resume, close 0

```sh
python3 <packet>/moving-ordinary-run.py moving-ordinary-production tutorial ordinary
```

PID **4023895**. Underlying unchanged command:
`distrobox enter developer -- env WINEDEBUG=-all make reccmp-run`.
No direct-level override, injected calls/state, debugger, ptrace, breakpoint,
autoack, numeric selection, M movement, forced camera or depth bypass.
The physical Training click and visible first Moving label click enter the
correct tutorial. Both real authored pages are captured and accepted with
fresh Space presses; an added text-region comparison distinguishes Moving from
the earlier wrong-level attempt. Exact physical inputs/times are in events.

`scene.png` follows a **35.0106-second unpaused dwell**. Escape opens PausedMenu;
Enter activates Continue; another Escape opens the second pause; fresh Escape
resumes. WM close is logged before send. Launcher **exit 0**, game gone,
no terminating signal. The run's owned screenshots, result, identities and
stopped afterstate are retained. These are idle tutorial/UI observations,
not walking pause or measured post-resume displacement.

The scene shows a distinguishable procedural helmet/head/arms fragment with
lower-body occlusion. No ordinary visible actor click, selected-pointer
persistence, permitted route, commanded move, arrival or second destination
was performed or credited. No authored-model/full-body-parity requirement is
added. Height green alone does not close ordinary visibility/interaction.

## Bounded diagnostic capture and executed review regressions

After that ordinary replay, a **separate external diagnostic executable** was
linked from current production objects and external source copies, with tagged
observation code. No production source/EXE/PDB was overwritten. The external
source and EXE/PDB hashes are frozen in
`external-diagnostic-with-resources-freeze.json`; the actual diagnostic EXE is
`8b5c90d4d0de3d4269ed1c5671aec2fd1f07452051a72e78a1999cb26d5bc270`.

The passing `scene-direct-diagnostic` run used the same make/Proton/prefix/cwd
with diagnostic EXE overrides and `LEGORRR_LEVEL=MoveTuto_01`, plus
window-directed settings/Space events. **This is not physical-input acceptance.**
It first recorded the actual production scene with briefing cleared,
objective **0xb0**, reward **0**, before any regression-fixture mutation.
Then it explicitly marked the boundary and ran injected disposable fixtures.
WM close exited **0**. No camera override was used in the real observation.
All test camera/surface changes occur after that boundary in the fixture.

Actual OList Pilot **Object3/4/5** all reach `render_raider`. Their tested actor
cells' scene Y and indexed CPU Y match. Camera eye is approximately
**(-93.449,79.928,-308.641)**, focus **(-261.449,-128.072,-17.441)**.
Object3's helmet marker projects near logical **(274,175)**, consistent with
the ordinary visible fragment; the production projected-radius picker returns
Object3 there. This is diagnostic correspondence, **not an ordinary selection**.

Concrete first occlusion boundary:

- Object4 torso marker **(322,220)** has depth **378.699**, but terrain cell
  **(9,10), side 2** writes nearer depth **299.128**. Its actual wall triangle
  is `(-200,-116,-80), (-240,-36,-80), (-200,-36,-80)`.
- Object5 torso marker **(378,279)** has depth **323.604**, behind cell
  **(8,10), side 0** at **289.455**.
- Actual Raider raster calls make **1579 /0 /0** depth-passing pixel writes for
  Object3/4/5. These are writes, not unique pixels or visibility percentages.
- The radius picker still returns Object4/5 at tested obscured markers. It is
  not an occlusion-qualified selection service. No invisible marker was clicked.

Thus the observation localizes remaining obscuration to actual production wall
triangles/depth, while the tested height owners agree. It does not establish
that wall policy should be changed, certify reference geometry, or justify
hiding walls/forcing a view. No further terrain repair was attempted.

The same loaded application process then exercised actual production owners:

- `terrain_world_spatial_edge_midpoints_update`: all four physical cardinal
  selections, diagonal **0/1**, nonzero slot offsets **+7/-7** and a shared X
  offset **3.25**. Real affected cell flags are identical under each diagonal
  toggle; **zero failures**. Recorded cells are (9,14), (10,14), (9,14), (8,14).
- Actual shared projection and rasterizer: placing each of the three vertices
  below near depth **8** rejects the whole triangle with **0 drawn samples**;
  wholly behind-eye also rejects. A visible control draws **312 samples**.
- Actual `level_scene_mouse_cell`: behind-eye geometry and genuine miss return
  **0**. At pixel **(23,37)** a nearer rejected surface has positive raw hit
  distance **18.8646**, farther visible surface **112.4318**. The production
  picker returns the same farther cell **(19.00317,8.390889)** before/after
  adding the rejected nearer surface. Raw intersections use the production
  terrain triangle-hit owner, not a duplicate intersection formula.

`boundary failures=0`, `diagnostic-tests-complete result=0`. The fixtures alter
loaded data in a disposable process; none is ordinary gameplay evidence. Zero
and negative RoughLevel variants, allocation failure, lifecycle/reuse, broader
orientation/overlay coverage and picking performance remain **NOT EXECUTED**.
No further fixture expansion follows this checkpoint.

## Sequential bounded reference comparison

One `reference-bounded` launch used exactly
`distrobox enter developer -- env WINEDEBUG=-all make reference-run`.
Candidate and reference effective **runner/preloader, cwd and relevant
environment are identical**, recorded in the paired context comparison.
Supported settings were Full Screen640×480×16. No reposition/focus experiment
loop, display change or concurrent candidate occurred.

PID **4033476**. First visible difference: reference still shows the DDI intro
at the candidate's early menu-capture point, then reaches a recognizable real
main menu after settling. The reference menu is letterboxed; the candidate
menu fills its stretched client. One normal Space gesture was recorded at the
menu; the final capture after Space shows a logo-only state, whose transition
was not diagnosed. The bounded attempt stopped there. No reference tutorial, NEXT-permitted
entity selection or move policy is claimed; early M01 DisallowAll remains an
expected authored restriction, not a tested gesture failure here.

WM close was sent at **00:36:49.399 UTC**, but the owned process remained after
20 seconds. The supervisor reverified PID/exe/reference cwd and sent **SIGTERM
4033476** at **00:37:09.421 UTC**. Make returned2. This is a **failed normal exit**,
not an observed application exception; no new fault stack/cause was captured.
No equivalent reference retry followed. Final afterstate capture was made only
after owned writers were gone, then reviewed and restored.

## Failed/inconclusive attempts retained

`all-runtime-attempt-results.json`, each run's exact script/events/result, and
external build/result logs preserve failures separately:

- Three initial red diagnostic menu/focus attempts did not load the target:
  Load Saved Game assertion failure (close0), settings focus failure (make2,
  child3 after closing settings), and later focus assertion failure (close0).
- Two direct-diagnostic preflights refused existing restoration mismatches;
  no game launched. A diagnostic `_RUN_ARGUMENT` containing arguments was
  quoted as one filename by the existing recipe (Error53); corrected to the
  existing development environment override for the actual red/green runs.
- `ordinary-production` entered **Moving Rock Raider Vehicles**, because the
  longer held Down advanced past Moving. Its broad panel-image assertion was
  insufficient. It dwelled and paused, but the final Escape assertion failed;
  supervisor ordinary cleanup closed0. This run's script verdict stays failed;
  no Moving, correct two-page, or successful resume credit is imported from it.
- Three standalone external console hosts compiled, but respectively failed
  the absolute-path mount, then full level load, then full load despite added
  game context. No regression assertion executed in them. The remaining manual
  boot dependency was not isolated. Rather than more equivalent host attempts,
  regression execution moved into the known-working full application startup.
  Transient console-host EXE/PDB versions were overwritten during retries;
  their individual binary hashes are **not available**, not inferred afterward.
- First external scene diagnostic omitted `version.rc.res`; Mode Selection was
  absent and startup returned child3. Relinking the exact existing production
  dialog resources fixed that dependency without a source change. Two subsequent
  ordinary-input diagnostic attempts left settings open and timed out; ordinary
  settings-close cleanup exited child3. Guarded supplemental attempts found the
  dialog gone/not focused and dispatched no input. The passing diagnostic used
  explicitly disclosed window-directed input instead.
- A setup-script key lookup failed before preservation setup completed; the
  corrected setup verifies the prior production-build identity. An offline
  registry-review helper first rejected escaped key names, then reference
  monitor keys; both stopped before prefix restoration until the exact changes
  were reviewed. These are tooling failures, not passing runtime evidence.

## Preservation, restoration and closure

Checked free space before archival and reused the already frozen shutdown
baseline only after full byte verification. New stopped afterstate archives
hardlink **unchanged read-only evidence bytes** to that frozen backup; changed
bytes are O_NOATIME-read, independently hashed, fsynced copies. No live
reference/prefix file was hardlinked. Complete live metadata inventories remain
per archive. No unrelated deletion or archive deduplication was performed here.
Final capacity is approximately3.8GiB on the workspace and216MiB on `/var`;
check capacity before any future external snapshot/build.

Every run's stopped afterstate was archived before reviewed restoration. Two
partial restore attempts are explicitly retained: a transient Wine server
blocked one prefix restore; another restore overlapped the newly started green
builder, which was my sequencing mistake. Guards refused prefix restoration.
Reference metadata restoration had already completed. Recovery verified the
prefix still exactly matched its archived afterstate, waited for all writers,
archived the partial/combined state, reviewed it and restored. The restore
helper now also checks writers before starting either root. No live prefix was
blindly restored; no unowned Wine process was killed.

Offline registry review: candidate/probe launches changed generated HID/WINEBUS
ContainerIds and display-driver date metadata (UTC date rolled to Nov3).
Reference additionally changed two monitor EDID values to BAD_EDID, display
memory-size and SymbolicLinkValue. Full key/value diffs are retained. No game
or user registry values changed. Exact current afterstate was rechecked before
restoring. Only the owned failed reference received a terminating signal.

`final-restoration.json` independently verifies **zero differences across all
484 reference /2218 prefix entries**: bytes, types, symlink targets, ownership,
mode, atime/mtime and xattrs. Ctime/inode are nonrestorable and never claimed.
Reference EXE/DLL/WAD bytes remain unchanged. Designated current
`Data/Saves/5.sav` remains
`41ceff64d8d7d8590c7828cf72e76d32263c2ff929c6f831e179d19ad36ea69a`.
The lost original `e33adbaa…` remains unrecovered; no original-restoration claim.

Protected PIDs **2821989/2822012** remain alive and untouched. **No owned game,
runner, debugger or Wine server remains.** KDE XwaylandEisNoPrompt remains
observed true; no permission/configuration change was made.

Next highest-value integration is the separately reviewed **NEXT/message and
pointer ownership** prerequisite, retaining the typed-text fix and this exact
five-file terrain result. Neither queued artifact was integrated here.
Ordinary permitted selection, actual commanded movement/arrival, walking
pause/resume, full reference flow and lifecycle parity remain unknown/open.
Stop this bounded phase; retain exclusive ownership until root explicitly
transfers it. The overall user goal is **not complete**.
