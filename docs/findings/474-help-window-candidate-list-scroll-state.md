# Finding 474 — Help Window Candidate List and Scroll State

The help-window candidate builder at `0x00418920` resets `DAT_004dc63c` and
increments it for each accepted candidate stored in parallel arrays rooted at
`DAT_004dc4d4`, `DAT_004dc54c`, and `DAT_004dc5c4`. The help-window render path
uses the count to walk and draw candidates.

The render path treats `DAT_004dc640` as a fractional row offset: it rounds the
value for the first candidate index and applies the fractional remainder to
the vertical placement. When a candidate exceeds the visible bottom boundary,
it derives a target offset from the remaining candidate count and stores that
target in `DAT_004dc644`. `HelpWindowButtonFeedbackUpdate` moves the current
offset toward zero or that target at a rate of `0.2 * param_2` per update.
This establishes the scrolling roles of the two floats; the exact UI events
that select each feedback mode are handled by callers.

**Evidence.** Static decompilation of shipped `reference/LegoRR.exe`,
SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
Relevant procedures are `CategoryCellCandidateListBuild @ 0x00418920`,
`HelpWindowButtonFeedbackUpdate @ 0x00419030`, and the help-window render path
in `0x00418a00`.
