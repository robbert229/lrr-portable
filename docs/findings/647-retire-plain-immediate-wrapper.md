# Finding 647 — The plain-immediate wrapper retires; a hard image-end guard replaces it

Issue #748. Follows findings
[584](584-plain-immediates-past-the-original-image.md) (why the wrapper
existed) and [642](642-gate-takes-the-original-link-mode.md) (the gate links
in the original's mode, and the image ends at `0x7f5000`). Measured with
`make reccmp-check`, `make match-progress`, `make pe-audit` and
`make match-function` on `main` `5f251885` and on this change. Rebased onto
#752 (`f1a9cb16`, the original's resources and header stamps): every score is
the same as #752's baseline, and the image still ends at `0x7f5000`. No source
file changed. The matching build is the same.

## Summary

1. **Without the wrapper, `main` scores the same.** On `main` `5f251885`, a
   Reccmp run with `RECCMP_PLAIN_IMMEDIATES=0` gives the same score,
   `effective` and `stub` for all 2,827 full and 2,536 game-only functions
   as the default run.
2. **With the wrapper removed, the reports are the same as `main`'s.**
   `results.json` and `game-only.json` differ from `main`'s only in their
   timestamps. Every listing and diff is equal. Accuracy stays 96.72% full
   and 96.42% game-only. Exact-function code bytes stay 319,635 (49.74%).
3. **`make pe-audit` and the gate now fail when the image covers a plain
   constant.** The constants are the three that finding 584 measured:
   `0x800000`, `0xff0000` and `0x1000000`. The other two are higher, so the
   check fails as soon as the image ends past `0x800000` (an image that ends
   at exactly `0x800000` does not contain it). `make reccmp-compare` runs
   `pe_audit.py --image-end-only`, so `make reccmp-check` fails too. The
   message names finding 584 and says to shrink `.data`/`.bss`. It does not
   suggest restoring the wrapper. On this tree the image ends at `0x7f5000`,
   45,056 bytes below the limit.
4. **`match_function.py`'s span without a build was the wrapper's last
   dependency.** When no matching build exists, the single-function loop
   treats a constant as an address up to a default span end. That default
   was `0xA00000`, which covers `0x800000`. Without the wrapper, that span
   would lower 28 of the 104 functions whose full-report listings contain
   one of the three constants. The default is now `0x800000`. With a build
   (span `0x7f5000`) and without one, all 104 functions score the same as
   `main`'s `match_function.py` with the wrapper.

## What was removed

- `tools/reccmp_plain_immediates.py` and `tools/tests/test_reccmp_plain_immediates.py`.
- Its install in `reccmp_full_diff.py`, its attach in `match_function.py`
  and their `RECCMP_PLAIN_IMMEDIATES` toggle.
- Its `check` step in `make reccmp-compare`, which `pe_audit.py --image-end-only` replaces.
- Its `make tools-test` line, its entries in the `binary_progress_hash.py`
  input list and in `tools/binary_progress_inputs.txt`, and its section in
  `tools/README.md`. The pointers in ADR 0005 and ADR 0006 now say it is
  retired.

The `Deviation (scoring only)` phrase remains in the comparison tooling
only on `reccmp_vtordisp_scope.py` (finding 616).

## The guard

`tools/pe_audit.py`:

- `image_end_errors(end)` returns one error for any covered constant.
  `summary()` prints it in place of the old warning, and `main()` returns 1
  after it writes the summary and any `--json` output.
- `--image-end-only` reads `ImageBase` and `SizeOfImage` from the two PE
  headers with `struct` (no `pefile`) and runs only that check. It passes
  with a message when `reference/LegoRR.exe` is absent, as the old check
  did. A missing rebuilt image still exits 2.

`tools/tests/test_pe_audit.py` covers the boundary cases: an image that ends
at `0x800000` passes and one that ends at `0x801000` fails. It also checks
the exit status and message of `--image-end-only` and of the full audit on
fixture PEs.

## Generated files

`make match-progress` changes only the input hashes, plus the `opt_ref`
`original_annotated` / `port_unit` split, which moves by one 32-byte function
between builds of the same source. `main` committed 2,532 / 14, the first
build of this change gave 2,533 / 13, #752 committed 2,531 / 15, and this
change rebased onto #752 gives 2,532 / 14. Every Reccmp score and listing is
unchanged, and `make winapi-link-check` reports 2,533 annotated functions
(573,008 bytes) on every one of these builds. So the row is noise in the
relink's classification, not a change. (#748 first said `main`'s row was
stale. That was wrong.) `make unit-data-offsets`, `make opt-ref-callers` and
`make pe-audit` write no committed file.

| Measure | `main` `5f251885` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 319,635 (49.74%) | 319,635 (49.74%) |
| Accuracy, full / game-only | 96.72 / 96.42 | 96.72 / 96.42 |
| Mapped / exact (full) | 2,827 / 2,348 | 2,827 / 2,348 |
| Gate link | `/DEBUG /OPT:REF /OPT:NOICF` | `/DEBUG /OPT:REF /OPT:NOICF` |
| Image end | `0x7f5000` | `0x7f5000` |
| Scoring wrapper | plain-immediate (no effect) | none |
| Image past `0x800000` | `pe-audit` warns, wrapper compensates | `pe-audit` and the gate fail |

## Reproducing

```sh
# on main 5f251885 (wrapper present):
make reccmp-report
RECCMP_PLAIN_IMMEDIATES=0 python3 tools/reccmp_full_diff.py "$(command -v reccmp-reccmp)" \
  --target LEGORR --json /tmp/nopi/results.json        # and --nolib for game-only
# on this change:
make reccmp-check                                        # image end: ... 0x7f5000, margin 45,056
make pe-audit
python3 tools/pe_audit.py --image-end-only
```
