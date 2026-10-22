# 83 — `.ol` `driving` names resolve after object creation

The `.ol` parser records each entry's optional `driving` string while creating
objects, then makes a second pass after the entry loop to resolve those names
against the complete list. Resolution uses case-insensitive object-name
comparison and selects the first match. On a match, the parser stores the
referenced entity pointer in the new entity's `+0x2fc` field, sets bit `0x04`
in its `+0x3e4` flags, derives a mode from the referenced entity's flags, and
calls follow-up state/relationship helpers.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-olistfile-focused-expansion.bundle`.
Focused IDs: `ev_55645c200e2f7eb8fc3d34b41a3eeccb0d4e41b65b3c496829a3556fd5c2ccdb`
(`FUN_0042e020`, both object-list passes), `ev_cef11fa5ae1bfc24ad3949129e4369eaaa9377df88bc0716da75bcdfd9a32cdc`
(`FUN_0043f5b0`, state update helper), and
`ev_aad51dd44ddf50374f41fabf788bedab14263def85cadaf2a7818553e112422e`
(`FUN_0043f100`, linked-state helper).

## Deferred resolution

During the first pass, `FUN_0042e020` stores created-entity pointers and their
associated names in parallel temporary arrays. It also reads `driving` for
each record without treating a missing value as a required-field parse error.
After all entries have been processed, a second loop visits entries with a
non-null `driving` string and scans the stored names using `__strcmpi`. Thus,
a reference can point to an entry appearing later in the `.ol` file. When a
match is found, the parser writes the matched entity pointer at `+0x2fc`, sets
bit `0x04` in the current entity's `+0x3e4` flags, inspects flags on the
referenced entity, then calls `FUN_0043f5b0` and `FUN_0043f100` with the two
entities/derived state.

The helper bodies show downstream flag and state changes: `FUN_0043f5b0`
updates state only for one entity kind and emits an event based on the mode
bits; `FUN_0043f100` clears bit `0x40` on the referenced entity and, when its
own link field is populated, on that linked entity as well. These writes
support that `driving` is a deferred entity relationship, but the precise
gameplay meaning of the relationship is not identified by this trace.

## Evidence boundary

This is static analysis of the shipped executable; it does not demonstrate
runtime behavior for a specific level or every unmatched/cyclic reference.
The comparison is case-insensitive, but the trace does not establish a
user-facing error for a name that has no match. Finding 75 covers the separate
first-pass placement and required-field contract.
