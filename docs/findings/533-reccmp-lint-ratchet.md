# 533. Reccmp annotation lint ratchet

On main `5f01c2ce`, the source heuristic reported 4,451 unannotated
recognizable definitions and three named ledger entries (4,454 occurrences).
Finding 528 recorded 4,453 definitions on its earlier tree; this count reflects
the current main snapshot. The lint printed file, line and
name for definitions and address and name for the ledger. It did not fail on
ledger entries alone.

The baseline now counts scoped names and normalized signature hashes for
source definitions, and addresses and names for ledger entries. It has no
file or line in the key, so a verbatim relocation retains its identity. Equal
keys are counted, although identical file-local signatures cannot be told
apart. The source scanner is a heuristic and scans preprocessor alternatives;
it does not prove that every C++ definition is recognized. The tool reports
fixed entries and refuses to add debt during an update without explicit
`--allow-growth`.

The three named entries remain in the baseline deliberately:

- `0x004369f4` CameraTiltAxisUpdate: the original tilt setter is only one
  mode of the generic, currently no-op `main_view_key_step`, not a matching
  standalone boundary (see `docs/reccmp-annotation-handoff.md`).
- `0x0049221e` CrtFloatingPointExceptionResultDispatch: the source documents
  this as a label embedded in VC5 LIBC's `87cdisp.obj` dispatcher; the
  non-MSVC function is only a portable seam, not that runtime entry.
- `0x00492225` CrtFloatingPointExceptionResultDispatchVariant: likewise an
  embedded runtime label; its portable seam is empty and is not an equivalent
  implementation. Neither CRT seam should claim the original label.

The stdlib-only lint and fixture tests run in `make tools-test`. No matching
build inputs or generated match-progress files change.
