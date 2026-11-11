# MSVC 5.0 permuter: bounded cohort demonstration

The report was generated on 2026-11-10 from matching-build source commit
`0cfa8749` (Keys.c conversion); the subsequent #430 changes the progress
measurement metadata, not source. It has 2,128 mapped game functions, 1,633
non-exact and 134 in the 75–89% band. The issue's 1,670 and 141 counts are
therefore stale or based on a different selection. The 134 is the entire band,
not the issue's band after excluding its register/order-only cohort.

## Classification

`tools/permuter_cohort.py` reads the game-only Reccmp JSON. For each non-exact
function it collects unmatched instructions, removes source-line annotations,
replaces register names and numeric constants with placeholders, and compares
the two multisets. Opcode and other operand text remain significant; order
may differ. The resulting cohort has **64 functions** (54 in the 75–89% band).
This is a reproducible approximation: it treats every numeric constant,
including offsets and jump-table entries, alike and loses register identity.
It does not prove that a difference is reachable by semantics-preserving
rewrites, and is not necessarily the issue author's original 61-function set.

## Search

Each address had an 18-second wall-clock budget and at most two workers; each
search could stop early on exhaustion. The [per-function results](msvc-permuter-cohort.csv)
include report and single-function before/after scores and candidate counts.
The search produced **0 improvements and 0 exact matches**. Nineteen functions
had candidates: 40 candidates were compiled and scored with no compilation
failures. Of the remaining 45, 43 had no supported rewrite; two could not be
parsed because preprocessor directives intervene between annotation and body.
No winning patches or winner-specific full regression results exist. The full
`make reccmp-check` passed on the unchanged source tree; it does not validate
any candidate or establish an absence of collateral drops for future winners.

The outcome is explained by the search space, not by a failed scoring loop:
an unchanged copy of `priorities.c` scored 75.00% both with and without the
candidate input. For example, 37 cohort entries are `entity_stats_*` accessors;
`0x469de0` differs in the instruction's field offset (`0x334` versus `0x114`).
The function is a macro-based float accessor, with no safe candidate from the
current passes. Replacing that offset would change the reconstructed layout,
not merely permute source syntax. `0x42ef40` has a switch whose compiled case
blocks and jump-table entries are ordered differently; eight return-temporary
variants all scored 86.79%, and the search does not reorder switch cases. The
two unsupported functions, `0x4330e0` and `0x46a330`, place conditional
compilation and naked assembly before their bodies. These examples show why
this bounded run finds no wins; they do not prove no safe rewrite exists.

The requested three winner-specific full checks remain impossible without
actual winners. Candidate search needs broader, reviewed transformations and a
new demonstration yielding at least three winners before that criterion can
be satisfied. Do not apply a candidate solely because its single-function
score rises: full linked-build checks and the ADR 0005 entropy policy still
apply.

As a further attempt to find regression-checkable winners, the same bounded
search was run on all 21 additional 75–89% functions with at least one
supported candidate. The [results](msvc-permuter-band.csv) record 46 scored
candidates, 0 improvements and 0 exact matches. This does not establish that
other rewrite strategies cannot improve those functions.
