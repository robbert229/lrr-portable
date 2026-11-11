# MSVC 5.0 permuter: initial bounded demonstration

Measured from matching build commit `041ab287` on 2026-11-10. That
`game-only.json` has 2,128 mapped functions, 1,634 non-exact functions and
134 with raw similarity from 75% inclusive to 90% exclusive. The issue's
1,670 non-matching count is outdated. Its 141 figure excludes the separately
classified register/order-only set; the 134 here is the whole score band,
not a direct replacement. The issue does not provide the 61 addresses or a
reproducible classification, so their current count was not recomputed.

Each search used two workers and at most the stated wall-clock budget. Scores
below are the single-function scores; no source files were changed.

| Address | Budget | Candidates scored | Before | Best | 100%? |
| --- | ---: | ---: | ---: | ---: | --- |
| `0x45d470` priorities_move_up | 30 s | 1 | 75.00% | 75.00% | no |
| `0x479500` d3drm_vector_random | 30 s | 0 | 75.00% | 75.00% | no |
| `0x479af0` line_intersection_2d_compute | 45 s | 5 | 86.67% | 86.67% | no |
| `0x49ce70` wad_run_length_decode | 25 s | 1 | 84.21% | 84.21% | no |
| `0x40cf50` | 25 s | 1 | 79.07% | 79.07% | no |
| `0x403910` | 25 s | 2 | 86.21% | 86.21% | no |

No winning diff was found, so there are no winners to apply and no
winner-specific full regression results or collateral drops to report. A full
`make reccmp-check` on the unchanged source tree passed. This is not the
issue's requested demonstration over the register/order-only cohort, and it
does not establish whether any improvement would survive the full gate.
Statement ordering, temporary variables and expression association are not yet
implemented. The rewrite set and target selection need expanding, and at least a few actual
winners need full regression checks before the issue can be closed.
