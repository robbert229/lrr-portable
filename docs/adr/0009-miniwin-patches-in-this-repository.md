# miniwin's local changes are patch files in this repository, on top of upstream isle-portable

**Status:** accepted (2026-11-20). **Amends**
[ADR 0007](0007-port-on-develop-units-on-miniwin.md) on where miniwin
patches awaiting upstream review are carried, and
[ADR 0004](0004-miniwin-native-port.md) on what the submodule points at.

## Context

ADR 0007 carried the miniwin changes that upstream had not yet accepted on a
fork, `robbert229/isle-portable` branch `lrr/dx6-interfaces`. The
`3rdparty/isle-portable` submodule pointed at that fork. By 2026-11-20 the
fork held 25 commits on top of upstream `8ce9570`:

- the DirectX 6 interface declarations;
- the window and input fixes;
- the level-rendering steps of [ADR 0008](0008-level-rendering-on-miniwin-renderer.md).

Every miniwin change took two repositories:

1. Commit to the fork and push it as a fast-forward.
2. Open a PR here that bumps the submodule.

Review here saw only a gitlink change, never the code. The fork also had to
be kept in step with every open PR.

## Decision

- **The submodule points at upstream** `isledecomp/isle-portable`. The
  gitlink pins the upstream commit our changes apply to.
- **Our changes are `patches/miniwin/*.patch`**, in `git format-patch` form.
  They are applied in file-name order with `git am`.
  - Each patch touches only `miniwin/`.
  - Each patch keeps the authorship and message of the change.
  - Each is meant for upstream, as ADR 0007 requires of the fork's commits.
- **`tools/setup-miniwin.sh` bootstraps miniwin.** It makes the sparse,
  blob-less clone of ADR 0004 and checks out the pin. It then runs
  `tools/miniwin-patches.sh apply`.
  - It is safe to rerun.
  - It moves a checkout of the old fork onto upstream, and makes it sparse.
  - It re-applies the series when the patches or the pin change.
  - It refuses to discard uncommitted changes, or commits that are not in
    the series.
- **Editing a patch** means:
  1. Commit, amend or fix up in the submodule, on top of the applied series.
  2. Run `tools/miniwin-patches.sh export`, which rewrites `patches/miniwin`
     from the submodule's commits since the pin.
  3. Commit the patch files here.

  The export is deterministic: re-exporting an unchanged series reproduces
  the files byte for byte.
- **Moving to a newer upstream** is `tools/miniwin-patches.sh rebase <commit>`.
  It rebases the series, repins the gitlink and exports.
- **The build checks the series.** CMake refuses to configure, and every
  build fails, unless `tools/miniwin-patches.sh check` finds the series
  applied and the checkout clean. So `git submodule update`, which checks
  out the bare pin, cannot build unpatched sources silently. `check` also
  fails when two patch files share a number, which happens when two PRs each
  add a patch; the second to merge re-exports.
- **`.gitmodules` sets `ignore = all`.** The submodule's HEAD (pin plus
  series) differs from the gitlink by design, and `git status` and
  `git add -A` must not offer that as a change. A repin is staged on purpose,
  by `rebase` or `git update-index`.
- **Upstream PRs are cut from the patches.** Apply them to a branch of an
  upstream clone and push that branch to a fork. A fork is still needed for
  GitHub PRs, but only as a place to push PR branches; the build no longer
  depends on it.
- **The patches drop out as upstream takes them.** When an upstream release
  includes a patch, `rebase` onto it and delete the patch the rebase
  emptied.

## Considered options

- **Keep the fork.** Rejected for the review and two-repository costs above.
- **Vendor `miniwin/` into this repository** (a squashed `git subtree`).
  Changes would be ordinary diffs. Rejected: what we change relative to
  upstream would no longer be one listing, and the patch series is the shape
  upstream review takes anyway.

## Consequences

- A miniwin change and the code that needs it land in one PR here, and the
  PR shows the change.
- Reviewing a change to an existing patch means reading a diff of a diff.
  Small, focused patches keep that readable. A patch can also be reviewed by
  applying the branch and running `git -C 3rdparty/isle-portable show`.
- The submodule's working tree always carries local commits. Tools that
  reset submodules leave it unpatched until `tools/setup-miniwin.sh` runs
  again, and the build check says so.
- `robbert229/isle-portable` `lrr/dx6-interfaces` is frozen at `3306e38`.
  That commit is identical to upstream `8ce9570` plus the first 25 patches.
