# MSVC 5.0 optimization flag sweep for Reccmp

## Question

Can changing MSVC 5.0 optimization flags improve the rebuilt `LegoRR.exe`
match against the original? This sweep also checks whether a seemingly more
specific code-generation flag reduces compiler variation enough to improve
matching.

## Method

The reference build used the installed VC5 RTM toolchain and the project's
Reccmp configuration: `/GX /ML /Zi /O2 /Ob1 /DNDEBUG`, with `/INCREMENTAL:YES`
and `/DEBUG` at link time to retain a PDB. Each tested variation was built in
its own ignored `build-reccmp-flags-*` directory; source flags and the
checked-in build configuration were left unchanged. A full Reccmp report was
run for every build. The game-only report used Reccmp's `--nolib` filter and
is the primary comparison below because it excludes recognized library code.

The game-only baseline was 98.54% implemented (2,094 of 2,125 functions),
32.69% accuracy, and 32.21% progress. The first percentage is coverage of
mapped functions, not binary similarity.

## Results

| Compiler flags | Implemented | Accuracy | Progress | Per-function comparison to baseline |
| --- | ---: | ---: | ---: | --- |
| `/O2 /Ob1` (baseline) | 98.54% | 32.69% | 32.21% | — |
| `/O2 /Ob0` | 98.54% | 32.32% | 31.85% | 39 improved, 72 regressed |
| `/O2 /Ob2` | 96.33% | 28.63% | 27.58% | 47 fewer mapped; among common functions, 201 improved and 465 regressed |
| `/O1 /Ob1` | 98.54% | 19.29% | 19.01% | 253 improved, 1,439 regressed |
| `/O2 /Ob1 /Oy-` | 98.54% | 26.40% | 26.01% | 332 improved, 1,417 regressed |
| `/O2 /Ob1 /G5` | 98.54% | 32.69% | 32.21% | Identical per-function scores |

The full-report scores also favored the baseline: `/Ob0` was 38.33% accuracy /
37.19% progress; `/Ob2` 35.12% / 33.39%; `/O1` 26.54% / 25.75%; and `/Oy-`
32.97% / 31.98%. `/G5` scored 38.66% / 37.51%, tying the full baseline, with
the complete per-function score vector verified identical.

## Conclusion

Keep `/O2 /Ob1` as the Reccmp profile. None of the tested alternatives
improved the aggregate game-only result, and `/G5` produced no measured
per-function change. The sweep provides no evidence that toggling these
optimization/code-generation switches reduces compiler entropy in a useful
way. Address mismatches function by function using the Reccmp output and
matching toolchain inputs, rather than changing global flags based on this
sample.

This is a bounded sweep of five flag variations, not proof that no other VC5
option can help. It also does not establish identical output across repeated
builds; testing repeatability would require rebuilding the same configuration
multiple times and comparing the resulting binaries or reports.

## Reproduction notes

Run each configuration through the developer distrobox with the configured
VC5 RTM toolchain, using a separate build directory and the normal Reccmp
wrapper. Generate both the full report and its game-only `--nolib` report.
The tested alternatives were `/O2 /Ob0`, `/O2 /Ob2`, `/O1 /Ob1`,
`/O2 /Ob1 /Oy-`, and `/O2 /Ob1 /G5`. Restore the baseline profile after
experimentation and ensure generated local Reccmp configuration points back
to the normal build directory.
