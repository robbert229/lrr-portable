# Paused Wine runtime checkpoint

Paused at the user's request on 2026-11-03 UTC to commit current work and rebase
onto main. The active goal remains paused. All three running delegated tasks
were interrupted; none retains execution/source ownership. No gameplay work
or launches may restart automatically.

Current source includes startup/scene-root, Stats, Win32 input coordinates,
movement ownership, pause/quit, objective acknowledgement, typed NERP text,
terrain-height, physical-pointer ingress, NEXT/history/timer/message UI and
minimum glyph-advance repairs. The most recent NEXT/input phase was interrupted
before its final ordinary gameplay validation and handoff. Treat it as work in
progress; do not mark selection, two commanded moves/arrivals, walking pause or
full reference parity complete. Do not reapply its original source patches blindly.

Last completed ordinary production checkpoint is documented in
wine-sceneheight-integration.md: EXEc7e28d4b19a193ba765af3db8ded5f72a3d557e09daadb6a91e8890ea1957788,
settings/menu/Moving briefing/35.0106s dwell/pause-resume/WMclose0. Actual CPU/cache
height and geometry regression probes passed, but ordinary actor interaction
remained open. Reference reached menu and required owned-PID SIGTERM after close
hung. Lost original save bytes remain unrecovered; designated current save is
preserved. See wine-save-preservation.md for that limitation.

Interrupted phase evidence is ignored under
build-reccmp/runtime-evidence/20261103T004743Z-next-pointer-integration/.
At the user's explicit request, the disposable UI/NERP diagnostic hooks remain
committed. Their exact probe code is versioned under tools/diagnostics/*.inc and
included through repository-relative paths; the checkpoint does not depend on
local cache sources. The one-shot invocation requires LRR_MESSAGE_UI_PROBE.
These fixtures deliberately change state and do not qualify as ordinary gameplay
acceptance. See tools/diagnostics/README.md. External evidence remains ignored.

prepared artifacts and their original hashes remain unchanged.

Pause-time full protected-state inventory matches the phase's baseline exactly:
484 reference and2218 prefix entries, no differences. No owned game/debugger/
builder process remained. Existing dirty miniwin changes are unrelated and must
remain uncommitted/preserved. Generated binaries, reports and runtime evidence
are not part of this source checkpoint.

For the rebase, fetched origin/main squash commitc27b6803 has the EXACT tree of
branch commit19648bbefe97c8c9f4298a1bccfa76b627514ed0. Rebase the later branch commits
and this checkpoint onto origin/main, preserving the work already absorbed by
that squash. A local backup branch preserves the original history. No push or
merge is authorized by this maintenance request.
