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
- [ ] Adversarially review the complete branch diff against `develop`.
- [ ] Verify the documentation and evidence references.
- [ ] Commit and publish the branch / issue findings.
- [ ] Resolve integration and issue completion under repository policy.

## Architectural decisions

- Keep the pools described as separate mechanisms because their globals, record strides, link offsets, and callers differ.
- Use operation-level names; do not assign semantic types to opaque pool records.
- Treat zero-allocation behavior as unresolved because the acquisition routines dereference the free-list head after growth without a visible failure guard.

## Verification

- Cross-check each function against `docs/decompiles/full/LEGORR.full.c` and named excerpts where present.
- Check named evidence bundle IDs in the existing findings.
- Documentation-only change; no build or test command is applicable.

## Progress and blockers

- Research and documentation are complete locally.
- Repository policy in `AGENTS.md` forbids merging and requires opening a PR. `hack/ralph.md` asks for direct merge and forbids PRs. The direct-merge completion path is therefore blocked by repository policy; leave the branch unmerged and report the conflict.
