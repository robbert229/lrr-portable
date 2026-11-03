# Win32 input integration — active checkpoint, 2026-11-02

The authorized two-file input patch builds with MSVC5 and passes a focused PE32
production-API probe. Normal game UI validation is **pending**: the existing
reference run raised a new KDE “Remote Control / Control input devices” prompt.
Input stopped when that prompt became visible; the earlier session's approval
was not reused as approval of the new prompt. The current reference process is
retained, with no restart or further input while awaiting explicit confirmation.

Packet: `build-reccmp/runtime-evidence/20261102T215407Z-input-integration/manifest.json`.
Its captured patch SHA256 is
`951a3108f86c2afc07e9e4498a8de5658ede3100dd528627b366818852761a8d`.
The Stats baseline EXE/PDB/reports/source patch are preserved as `before-*`;
baseline candidate SHA256 is
`c869d3155092b78aa1180d749f7b2db429d1a447b35a56c5bcbec0b9e3ed819c`.
The completed Stats phase is documented in
[wine-tutorial-stats-integration.md](wine-tutorial-stats-integration.md).

The input candidate SHA256 is
`3d9aca4a30e35efe24b0b35c782c21db820b97827c51f7be756834a7793a6ca9`.
Only the prepared hunks in `src/platform/winapi/frameloop.cpp` and
`src/gods98/input.cpp` were applied. D3DRM/startup and Stats changes remain intact;
movement and pause remain queued. No commits, pushes, PRs or delegates.

The platform retains framebuffer dimensions separately from client geometry,
uses one client-to-frame transform for event and cursor polling, and applies its
inverse for cursor warps. The engine sampler uses that existing platform getter.
Navigation VKs are decoded before overlapping printable ASCII on both press and
release; letters use the existing lowercase consumer vocabulary. No new hooks,
fake actors, harness changes or initialization bypasses were added.

`distrobox enter developer -- make reccmp-report` and `git diff --check` passed.
`build/` archives binary/PDB/reports and the frozen source identity. Full report:
2332/2396, 97.33% implemented, 52.84% accuracy, 51.43% progress. Game-only:
2113/2137, 98.88%, 48.39%, 47.84%. Every row was compared; no mappings were
added/lost and no unedited function's score changed. Three edited scores fell:

| Reference | Function | Before | After |
| --- | --- | ---: | ---: |
| 0x4783f0 | platform_create_window | 12.6214% | 12.5000% |
| 0x47f010 | input_cursor_position_update | 98.0769% | 19.7183% |
| 0x47f0d0 | input_cursor_client_position_set | 16.8675% | 12.7273% |

These are disclosed code-shape/contract regressions: the old mapped sampler
directly read unscaled client pixels, whereas this backend stretches a 640×480
frame across a larger client. The platform calls, retained geometry and inverse
warp change those bodies. Match scores do not establish input correctness.

The focused probe source lives outside repository/Reccmp scan roots at
`/tmp/20261102T215407Z-input-integration/test-build/input_boundary_probe.cpp`.
It links the frozen production objects/libraries and calls actual Win32 window,
cursor, presentation, event decoder and InputState APIs. It is not a native64
test or a reimplementation of the conversion formula.

- First probe: exit 3. Actual cursor round trips, polling, retained dimensions
  and canonical arrow press/release passed. Three final-held mouse assertions
  failed when a synthetic PostMessage press was followed by motion with the
  physical button still up. This failed probe and source identity are retained.
- Second probe: only the test producer changed to real Win32 `mouse_event`
  press/release. **Exit 0, failures 0**. At client sizes 640×480, 1280×960 and
  320×240, logical/display dimensions stayed 640×480. Sampling, inverse warp,
  clamping, post-poll synchronization, zero setter deltas, actual button
  press/release, all four canonical arrows and lowercase M press/release passed.
  The smaller client quantized unrepresentable edge positions by one logical
  pixel, as expected. Production sources were unchanged between these probes.

Both tests' complete post-exit reference/prefix state was archived, registry
value differences reviewed (Wine HID/WINEBUS ContainerIds plus timestamps only),
and restoration verified before the next launch. This phase owns its own full
byte-verified backup and 484/2218-entry baseline inventory. Reccmp's directory
access-time effects were recorded and only verified atime differences normalized
before the first probe. Original 5.sav remains unrecoverable; the current changed
baseline is preserved, never represented as the original.

Current run: `reference-input-pair`, owned PID 3880824, launcher session 9799.
`reference-input-training-hover.png` records the new permission prompt;
`reference-input-effective-context.json` retains the live runner/cwd/environment.
The source/binary are frozen. A candidate GUI run and the normal UI pair have
not yet been completed. No current-run restoration is attempted while its Wine
writers remain live. Protected PIDs 2821989/2822012 remain untouched. The sole
integration/build/desktop owner retains ownership pending current approval.

Remaining acceptance: verify ordinary pointer hover/click at rendered targets
without offsets; arrow press/release with the pointer on background; normal UI
exit; repeat after a nonintegral/enlarged client and resize, retaining logical
dimensions. Tutorial actors/playability remain partial from the Stats phase;
this input patch does not repair the queued world/grid or authored-visual work.

## Closed continuation checkpoint — 2026-11-02 22:32 UTC

The earlier approval wait above is historical. The user's newer observation and
root's fresh full-desktop capture showed no dialog. During the T3/harness restart,
the retained reference PID and launcher disappeared before this owner's first
raise request (`BadWindow`); exact exit status/mechanism is unknown, so that run
gets no normal-quit credit. Its stopped state was archived, reviewed offline and
fully restored before a sequential reference relaunch.

The frozen input EXE remains `3d9aca4a30e35efe24b0b35c782c21db820b97827c51f7be756834a7793a6ca9`.
`final-frozen-identity.json` matches all 176 source hashes plus EXE/PDB/cache and
full/game report JSON to `build/`. No rebuild or production edit was needed.
`continuation-result.json`, `continuation-handoff.md` and the appended
`events.jsonl` are the authoritative current result.

- Reference PID 3907207: settings -> Training -> Moving Rock Raiders -> two-page
  brief -> visible Raiders -> mission Quit/Yes -> Training -> main menu ->
  Quit Game/Yes -> **exit 0**. Absolute mouse positioning stopped controlling
  the gameplay/returned-menu cursor; ordinary relative motion visibly reached
  targets and worked. One failed absolute Back attempt accidentally entered
  Rock Types; all screenshots/actions and its normal return are retained.
- Candidate PID 3913771: settings accepted; actual rendered main-menu Training
  and Moving text targets highlighted and clicked without compensating offsets.
  Fresh Down/Up press-release with the pointer off rows changed the highlight;
  post-release settled capture retained. Moving loads its brief and two Space
  actions dismiss it. The post-brief scene remains dark/coarse without clearly
  recognizable Raiders: playable tutorial remains **PARTIAL**.
- Candidate Escape displayed Training; actual Back click did not reach the main
  menu, and WM close did not terminate. `candidate-input-forced-cleanup.json`
  records the exact verified owned-PID SIGTERM. Make exit 2/child 241 is cleanup,
  not a new observed fault or a clean quit. **Q FAIL**.
- Relevant environment, Proton preloader, prefix and cwd match exactly in the
  two effective-context files. Both selected FullScreen 640x480x16; candidate
  presentation uses a 2552x1406 client with nonintegral scale. Main-menu hover
  and click worked there. Both ordinary 1001x751 resize requests left geometry
  unchanged; arbitrary resized GUI remains unverified. The prior focused PE32
  dimension/warp/input checks at three sizes remain distinct evidence.
- Current `kwinrc/Xwayland/XwaylandEisNoPrompt=true` was read and recorded; no
  agent changed it and prefix restoration does not touch desktop configuration.
  Observed settings/menu response proves working current control. No permanent
  per-agent grant is inferred; no new approval prompt appeared.
- Exact reference selection/move/cancel gesture and configured mouse policy are
  **UNKNOWN**. No selection/movement or pause simulation pass is claimed.
  Catalog 0x49 release-clear and combined left/right ingress concerns are
  source-audit inputs; this phase did not change or validate their behavior.

After each stopped run, changed reference/prefix bytes were archived before
offline registry-value review and restoration. `final-restoration.json` verifies
**zero differences across all 484 reference and 2218 prefix entries**, including
hashes and restorable metadata; ctime/inode remain recorded/nonrestorable.
Protected PIDs 2821989/2822012 are live and untouched; no owned game/debugger or
wineserver remains. Current designated 5.sav hash is still `41ceff64...`; the
original `e33adbaa...` loss is unresolved. Further analysis uses archived logs.

The original three edited Reccmp decreases remain disclosed above; no score
optimization was attempted. This closes the bounded input checkpoint, with its
limits. The same owner retains exclusive execution ownership for root's newly
authorized, separately recorded corrected movement integration. Pause/pointer
patches remain queued. No commits, pushes, PRs, delegates or native64 tests.
