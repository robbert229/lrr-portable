# 75 — `OListFile` entries require placement fields before entity creation

Level loading retrieves the active level's `OListFile` value and passes that path to a dedicated object-list parser. The parser walks the `.ol` block's sibling entries and reads `type`, `xpos`, `ypos`, and `heading` before it creates each entry. A missing required value or an unrecognized type terminates the parse with failure. Coordinates are decremented by one before integer/grid conversion, and heading is converted from degrees to radians before placement.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-olistfile-focused-expansion.bundle`.
Focused IDs: `ev_a27cc2cb66787dcd36dd8af2f078036b9a5bdd28e80efc8f2fc5b33e1e667009`
(`FUN_00429890`, level loader), `ev_55645c200e2f7eb8fc3d34b41a3eeccb0d4e41b65b3c496829a3556fd5c2ccdb`
(`FUN_0042e020`, object-list parser), `ev_acf7f85a1892fcb9d9b97be7b9e169968be3856e722e8a8acf0c78981404b0a1`
(`FUN_0042e8b0`, type resolver), and `ev_35aea9a74a4e5819e5382bc5133a8a120e031239c962a8d6d867cfc27d81cfb9`
(literal field-key bytes).

## Loader path and entry contract

`FUN_00429890` resolves the level group's `olistfile` field and later calls
`FUN_0042e020` with that value. `FUN_0042e020` opens the named block file,
walks its top-level children, and looks up `type`, `xpos`, `ypos`, and
`heading` for each entry. The literal bytes confirm those key names and also
show `health` and `driving` in the same parser's key region
(`ev_35aea9a74a4e5819e5382bc5133a8a120e031239c962a8d6d867cfc27d81cfb9`).

The parser treats the four placement/type values as required: if a lookup
fails, it leaves the entry loop and returns failure rather than creating that
entry. It parses `xpos` and `ypos` as numbers, subtracts `1.0` from both,
converts them to integers, and maps the resulting coordinates using the
current map handle and cell-size state. It converts heading using a
`1/180`-scale followed by pi, then passes the resulting placement and heading
to the entity-creation path. The exact world-space convention behind the
coordinate decrement is not established here.

`FUN_0042e8b0` resolves the type string case-insensitively against built-in
special names and dynamically loaded entity-type tables. An unresolved type
returns failure to the caller and stops the object-list parse. This finding
covers the static parser path; it does not establish how a particular shipped
level's `.ol` text exercises every supported type or malformed-entry branch.

## Evidence boundary

This is static analysis of the shipped executable. No level was launched and
no edited or malformed `.ol` file was replayed. `health` is present in the
parser's field keys, but its value/default behavior is outside this finding.
The separate deferred `driving` name-resolution pass is documented in
finding 83.
