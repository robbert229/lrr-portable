# Rename the project to `lrr-portable`

The repository (formerly `OpenLRR` / "Rock Raider Reversed") is renamed to
**`lrr-portable`**, mirroring the sibling `isle-portable` project whose
miniwin adapter we adopted (ADR 0004). The name signals the actual goal — a
portable, platform-independent reconstruction of Lego Rock Raiders — rather
than the research-workspace framing the old name implied.

- Directory: `OpenLRR/` → `lrr-portable/`
- Meson `project()` name, README title, version.rc product strings updated.
- Findings, A.2 harness and docs keep `LegoRR` / Rock Raiders references
  (they describe the *original binary*); `lrr-portable` refers to *our
  reconstruction*.

Application of the ADR rubric: hard to reverse (repo identity) ✓, surprising
without context (renames that drop the research framing need a because) ✓,
real trade-off (research-workspace vs port embodiment) ✓.

**Consequences:** `docs/adr/0001`'s MinGW-first framing was superseded by
0004 as part of the same pivot; old references to `OpenLRR` in prose remain
as historical context but are not extended.