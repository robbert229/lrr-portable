# Active issue plan

## Issue

- #222 — RE: Investigate Record and resource pools
- Selected from the open `ready-for-agent` queue because it is a bounded six-function shared-primitives investigation with partial evidence already present in findings 368, 371, and 373. The 15-function geometry ticket is broader and no queue item has explicit priority labels.

## Acceptance criteria

- Describe the six assigned functions with function addresses and evidence.
- Trace callers/callees and data/state flow; separate observation from inference.
- Record unresolved questions and focused follow-ups.
- Post findings on the issue.

## Implementation tasks

- [x] Inspect existing findings and decompile excerpts for all six functions.
- [x] Add one consolidated lifecycle map with evidence identifiers and uncertainty notes.
- [ ] Adversarially review the complete branch diff against `origin/main`.
- [x] Verify the documentation and evidence references (`git diff --check`; manual cross-check against decompile excerpts).
- [x] Commit the branch work; findings are available in the branch and issue review comment.
- [ ] Push the verified feature branch and create a PR targeting `main`.

## Architectural decisions

- Keep the pools described as separate mechanisms because their globals, record strides, link offsets, and callers differ.
- Use operation-level names; do not assign semantic types to opaque pool records.
- Treat zero-allocation behavior as unresolved because the acquisition routines dereference the free-list head after growth without a visible failure guard.

## Verification

- Cross-check each function against `docs/decompiles/full/LEGORR.full.c` and named excerpts where present.
- Check named evidence bundle IDs in the existing findings.
- `git diff --check` passed.
- `meson setup build` passed.
- `make build` failed at the final link step with unresolved `Sleep(unsigned int)` references from `frameloop.cpp` and `DirectDrawCreate(GUID*, IDirectDraw**, IUnknown*)` from `surface.cpp`. This documentation-only branch does not introduce or modify these references.

## Progress and blockers

- Research and documentation are complete locally.
- The `3rdparty/isle-portable` submodule is initialized in this worktree.
- The branch base `origin/main` is current and matches the existing issue branch base.
- Issue #222 has been returned from `agent/blocked` to `agent/in-progress`.
- Repository policy and the updated `hack/ralph.md` agree: create a PR targeting `main` and leave it open for human review and merge.
