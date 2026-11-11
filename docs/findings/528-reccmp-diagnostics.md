# 528. Matching-build diagnostics and annotation recovery

Measured on main `7d954455` in the developer distrobox with a clean MSVC 5.0
matching build. Reccmp runs twice (full and game-only); counts below are per
run, unless marked build. The earlier issue list predates explicit linking:
there are no pointer-stack, video-graph, CRT heap, or 0x506518 diagnostics in
this build.

| Diagnostic | Before | After | Reason / action |
| --- | ---: | ---: | --- |
| Missing function symbol, `0x44a2a0` | 1 | 0 | Marker had been left on a namespace closing brace. Moved to the existing `entity_work_object_position_get` definition, whose role corresponds to the original. |
| Missing function symbol, `0x459050` | 1 | 0 | Marker preceded a context typedef, not code. Moved to `objective_check`, the existing timer evaluation implementation. |
| Debug data out of sync, `0x480780` | 1 | 0 | The one-line function body gave Reccmp an unusable line record. Put its body on separate lines, preserving the code. |
| Duplicate FUNCTION `0x45ab30`, `0x45ab60`, `0x45ab90` | 3 | 0 | Each appeared on both sides of a preprocessor conditional; Reccmp scans both. Kept the MSVC matching-build definition's marker. |
| Duplicate STRING `0x4af9ac`, `0x4af9b4`, `0x4ab9fc`, `0x4aba0c` | 9 | 0 | The same pooled literal is used multiple times. Retained the first annotation for each original address. |
| Duplicate GLOBAL `0x505f88` | 1 | 0 | `engine.cpp` owns the annotated application mode; the separate `entry.cpp` programmer-mode variable was a duplicate claim. |
| C4091, `port_loop_ports.hpp` / `ui_list.hpp` | 33 / 7 (build) | 0 | Removed ignored `typedef` keywords from two C++ struct definitions. Names and layout are unchanged. |
| C4090 / C4022, `keys.c` | 1 / 1 (build) | 0 | Cast the array address to `void *` for `memset`, retaining the same storage and count. |
| C4172, `view.cpp` | 1 (build) | 1 allowed | The reconstruction documents that the original returns the address of its local result; keep this warning narrowly allowlisted pending reconstruction of the view unit. |
| Missing `cvtres.err` | 1 (build) | 1 allowed | The installed toolchain lacks the optional resource-compiler message catalog; its absence does not change the compiled resource. |

The gate performs a clean compile and checks build and both Reccmp outputs for
warnings and errors. The two exceptions above are explicit and narrowly
matched; unexpected diagnostics fail the run. The CRT link-order median was
updated after removing the duplicate markers; its object remains in place.

## Match effect and entropy review

Game-only implemented increased from 2,155/2,158 to 2,158/2,158. The mean
rounded per-function score changed from 51.03% to 50.99% because the three
newly mapped functions enter the denominator. No function fell below the
baseline. The table lists every changed score or normalized listing in the full
report, using the drift classifier; counts are rebuilt instructions. New
mappings have no prior listing to classify or compare.

| Address / function | Score before → after | Classifier | Instructions before → after |
| --- | --- | --- | --- |
| `0x4438b0` entity_path_step_advance_and_arrival_check | 26.49% → 26.66% | drift | 642 → 642 |
| `0x458be0` objective_tick_target | 14.38% → 15.03% | drift | 152 → 152 |
| `0x44a2a0` entity_work_object_position_get | unmapped → 34.41% | newly mapped | — → 49 |
| `0x459050` objective_check | unmapped → 36.13% | newly mapped | — → 76 |
| `0x480780` zero_result_is_success | unmapped → 11.43% | newly mapped | — → 15 |

Neither comparable listing changed instruction count; there are no drops or
instruction-count changes requiring owner review. Normalized listing equality
is mechanical evidence, not a proof of behavior. The two score improvements
follow Reccmp recognizing newly mapped call targets, not a source rewrite of
those callers.

`make tools-test` (including Meson parity and progress freshness) and the
native build and 35 tests pass. `make reccmp-lint` does not pass: it reports
4,453 unannotated definitions and three named ledger entries. The main tree
before the R1 relocation already reported 4,452 and the same three entries;
removing a duplicate conditional marker exposes one more alternative to this
source-level lint. Resolving the repository-wide annotation debt is outside
this diagnostic cleanup, so the lint has not been weakened to hide it.
