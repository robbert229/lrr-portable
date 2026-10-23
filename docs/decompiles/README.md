# Decompiled pseudocode

The complete, address-indexed Ghidra export is the canonical pseudocode corpus:
[`full/LEGORR.full.c`](full/LEGORR.full.c), with its
[`manifest.csv`](full/manifest.csv) and regeneration notes in
[`full/README.md`](full/README.md).

The older named corpus lives in [`legacy/engine-region.c`](legacy/engine-region.c).
It preserves selected function bodies and names from earlier investigation; many
bodies overlap with, but differ from, the full export. It remains useful as a
historical analysis source while those variants are reconciled. The 11 entries
that do not have matching entry addresses in the full manifest are copied to
[`full/LEGORR.supplement.c`](full/LEGORR.supplement.c) and excluded from coverage
counts.
