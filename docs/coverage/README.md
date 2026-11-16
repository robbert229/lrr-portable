# Coverage of the original executable

Reverse-engineering data about the shipped `LegoRR.exe`, kept on `develop`
as reference:

- [`translation-units.spec.csv`](translation-units.spec.csv),
  [`translation-units.csv`](translation-units.csv) and
  [`translation-unit-ranges.csv`](translation-unit-ranges.csv): the original's
  translation units, the function-to-unit map and each unit's address ranges.
  The unit ids are the keys in `src/link_order.cmake`.
- [`function-areas.md`](function-areas.md) and
  [`function-areas.csv`](function-areas.csv): every function's area of
  responsibility.
- [`call-graph/`](call-graph/README.md): the original's call graph.
- [`blocked.md`](blocked.md): port candidates once recorded as blocked.

These files are frozen at the fork of `develop` (ADR 0007). Their generators,
the coverage ledger and the matching build's reports remain on `main`.
