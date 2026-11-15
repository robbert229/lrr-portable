# 687. Identity measures on the original-mode link, Advisor.c's constant order, and the `.bss` gaps

Issue #830: finding 680's follow-ups 6, 7 and 8. Measured on `main`
`ebc07595` with full matching builds before and after.

## Summary

1. **The identical prefix and `data_identity` now read the original-mode
   link** (`opt-ref/LegoRR.exe`: the gate's objects relinked without
   `/DEBUG`, which `make match-progress` already produces). The gate's
   `.rdata` opens with the 84-byte debug directory, so none of the gate's
   `.rdata` operands or items can be at their original offset. Each record
   says which image it measured (`image`) and keeps the gate's figures under
   `gate` (section 1).
2. **Advisor.c's four constants are in the original's order**: 0.96, 25.0,
   1.0, 0.0 (section 2). The frame rate is a float literal now, and the
   symbol it used to cost is kept by a pointer typedef, so no function
   changes score. On the original-mode link the `.rdata` identical prefix
   goes from 0 to 18 bytes. The 25.0 operand at `0040135c` reads `.rdata`
   offset 4, the original's.
3. **The `.bss` gaps have no identifiable owner** (section 3). MSVC 5.0 keeps
   unreferenced statics, so dead data is a possible owner. The compiler's
   `.bss` layout rules narrow the 8 bytes at `004b31c0` to Advisor.c (an
   external defined after `advisorGlobs`) or AITask.c (a static, or an
   external defined before `aiTaskGlobs`). The 2,480 bytes at `004b8064`
   cannot be a single item: the 4-byte word at `004b8064` is either the
   last of AITask.c's items or padding before an 8-aligned item at
   `004b8068`. Nothing names these bytes, so nothing is defined (no filler,
   ADR 0005 rule 4).

## Before and after

| Measure (`binary-progress.json`) | `main` `ebc07595` | This change |
| --- | ---: | ---: |
| **Exact-function code bytes** | **348,304 (54.21%)** | **348,304 (54.21%)** |
| Accuracy, full / game-only | 97.59% / 97.34% | 97.59% / 97.34% |
| Functions that rose / fell | | 0 / 0 |
| `.text` identical at original addresses | 6.76% | 6.76% |
| `.data` / `.rdata` identical (section-relative, gate) | 24.72% / 11.67% | 24.72% / 11.66% |
| Identical prefix, raw / masked | 11 B / 7,187 B (gate) | 11 B / 7,187 B (original-mode link; gate the same) |
| Correct-size functions, full | 2,801 | 2,801 |
| Units with size agreement | 2 | 2 |
| Fully identical functions at their original address | 42 (2,307 B) | 42 (2,307 B) |
| Entry point (original `0048f000`) | `0048ebb0` | `0048ebb0` |
| SizeOfImage (original 3,608,576) | 4,030,464 | 4,030,464 |
| Data-offset agreement (`layout`) | 20 of 57 units | 20 of 57 |
| Tail roots | 3 | 3 |

`data_identity`, per region. Before is the gate (as on `main`); after is the
original-mode link. The gate's own figures after this change are the same as
before, apart from `.rdata`, which now has 94 paired items (below):

| Region | Start, original / rebuilt | Prefix before → after | Items at original offset before → after | At original address | First item not at its offset (after) |
| --- | --- | ---: | ---: | ---: | --- |
| `.rdata` | `0049e000` / `004b2000` | 0 B → **18 B** | 0 of 95 → 0 of 94 | 0 | `0049e014` (AITask.c), −4 |
| `.data` | `004a1000` / `004b8000` | 12 B → 12 B | 158 of 2,455 (3,037 B) → same | 0 | `004a1cd4` (Config.c's `":"`, in `.rdata`) |
| `.bss` | `004b2db0` / `004cf190` | — | 1 of 149 (1,040 B) → same | 0 | `004b31c8` `aiTaskGlobs`, −8 |
| commons | `00549520` / `005a3600` | — | 2 of 62 (35,760 B) → same | 0 | `005520e8`, +1,272 |

`.rdata` has one paired item fewer because the 25.0 is no longer an
annotated static, so Reccmp does not pair it any more. The `.rdata` prefix
stops at `0049e012`, inside the 1.0 at `0049e010`, which the rebuild does not
have (section 2). The section-relative `.rdata` comparison of the gate falls
by 0.01 points (11.67% to 11.66%). That comparison aligns the gate's
`.rdata`, debug directory included, with the original's.

## 1. Measuring on the original-mode link

The identical prefix (`tools/identity_metrics.py`) walks the rebuilt image
at the original addresses, raw and with address operands masked. With the
relink given (`binary_progress.py` passes `opt-ref/LegoRR.exe`), it walks
that image. `identical_prefix.image` says `original-mode link`, and
`identical_prefix.gate` holds the gate's raw and masked figures. Without the
relink, `image` is `gate`. The correct sizes and the unit agreement are still
measured on the gate: both links hold the same code at the same addresses
(`.text` is 724,437 bytes in both).

`data_identity` measures every region on the relink, with its map, when both
exist. Reccmp pairs items at the gate's addresses, so
`data_identity.translator` moves each item to the relink:

- **by the nearest anchor at or below it in its section.** An anchor is a
  map symbol whose name and object occur once in each map; both links lay
  out the same objects in the same order.
- **before a section's first anchor, by the lead.** The lead is the gate's
  extra leading zero bytes in that section: the debug directory, which
  `tools/reccmp.sh` zeroes in place (`strip_pe_debug.py`). In `.rdata` the
  first anchor is at `+0xd08` (`_s_resource_tree_child_iid`), and the items
  before it move by 84 bytes. From the first anchor on they move by 80,
  because a later object's alignment absorbs 4 of the 84 bytes. A single
  shift would put every leading item 4 bytes off.
- **checked against the bytes.** Each candidate shift is taken only if the
  item's bytes are equal in both links; otherwise the next candidate is
  tried, and the first is used when none matches.

`.data`, `.bss` and the commons start at the same addresses in both links
(`004b8000`), so they move by zero. `gate` keeps the gate's per-region start,
prefix and item counts. `readme_status.py` prints the image measured.
`tools/tests/test_data_identity.py` and `test_identity_metrics.py` cover the
translation, the lead, the relink measure and the gate figures.

The raw prefix is 11 bytes on both links. It stops at `0040100b`, the
operand `advisorGlobs` (`004b2db0`), because `.bss` does not start in place
(finding 680, section 4). The relink matters for what comes next: in it,
the 25.0 operand at `0040135c` is `[004b2004]`, offset 4 in `.rdata` like the
original's `[0049e004]`. In the gate it is `[004b2058]`, behind the debug
directory. Once `.rdata` starts at `0049e000`, the raw prefix can pass
`0040135e` on the original-mode link, and never on the gate.

## 2. Advisor.c's constants

The original's GAME-001 `.rdata` is 0.96 (`0049e000`), 25.0, 1.0, 0.0. The
rebuild had 25.0 first. Each form tried, scored with `make match-function
SOURCE=src/game/advisor.c`, against the unchanged unit:

| Form | `.rdata` order | Score changes |
| --- | --- | --- |
| named `static const float` 25.0f at file scope (`main`) | 25, 0.96, 1, 0 | — |
| the same, defined after `0x00401210` | 25, 0.96, 1, 0 | none |
| the same, a function-scope static in `0x00401270` | 25, 0.96, 1, 0 | `0x00401500` 100 → 84.62, `0x004016f0` 100 → 48.28 |
| literal, `atof(argv[1]) * 25.0f` | 0.96, 0 (qword 25.0), 1, 0 | also `0x00401270` 96.97 → 94.95 |
| literal, `(float)atof(argv[1]) * 25.0f` | **0.96, 25, 1, 0** | `0x00401500`, `0x004016f0` as above |
| that, with a named `static const float` 0.96f | 0.96, 25, 1, 0 | `0x00401210` 85.71 → 68.97 (the original stores an immediate) |
| that, with an `animData` local in `0x00401270` | 0.96, 25, 1, 0 | `0x00401270` 96.97 → 93.94 |
| that, with an `animName` local in `0x00401270` | 0.96, 25, 1, 0 | `0x00401270` 96.97 → 79.19 |
| **that, with `*lpAdvisorAnimData` on the struct typedef** | **0.96, 25, 1, 0** | **none** |

(`make match-function` scores without the link, so its absolute values
differ from the gate's; the changes are what counts.)

- **MSVC 5.0 emits named constants before literals.** A named constant
  comes first wherever it is defined, even inside a function. Literals
  follow in first-use order. The original has only literals here.
- **The frame rate multiplies a float.** `atof()` returns a double, so
  `atof(...) * 25.0f` pools 25.0 as a qword. `(float)atof(...) * 25.0f`
  pools the dword and compiles `0x00401270` to the same code as the named
  constant. The original's `fmul dword ptr [0049e004]` is this form.
- **Removing the named constant takes one symbol off the counter.**
  `vec_add` (period 4) and `vec_dot` (period 16) depend on its phase
  (finding 550, rule B). `phase_probe.py pads-at` shows that one symbol
  anywhere before `vec_add` restores both, and so do 17.
- **The symbol is the struct's pointer typedef.** `typedef struct
  AdvisorAnimData {...} AdvisorAnimData, *lpAdvisorAnimData;` is the
  original sources' house style (`*lpX`). Nothing uses the pointer name, so
  it is marked `Deviation (scoring only)`. It changes no storage.

The 25.0 loses its `GLOBAL` annotation, as the 1.0 and 0.0 never had one.
`make unit-data-offsets` now shows GAME-001's `.rdata` with no attributable
item.

**The next `.rdata` stop is AITask.c's.** The original has an unreferenced
1.0 at `0049e010`, between Advisor.c's 0.0 and AITask.c's first annotated
constant (0.0 at `0049e014`). No code reads it. A 1.0 in Advisor.c would be
pooled with its 1.0 at `0049e008`, and an unreferenced named constant there
would come before 0.96. So this 1.0 is AITask.c's: an unused named
constant, emitted first. This change does not add it, because AITask.c is
in its scope for `.bss` only (follow-up 1).

## 3. The `.bss` gaps

No code or data in the original holds an address inside `004b31c0`–`004b31c8`
or `004b8064`–`004b8a14` (a scan of every aligned and unaligned dword in the
image). The bytes are dead. Compiling probes into `aitask.c` (not committed)
gives MSVC 5.0's rules for one object's `.bss`:

- **Unreferenced statics are kept.** `static int a[2]; static char b[2480];`
  add 2,488 bytes to the object's `.bss`.
- **Uninitialised statics come first.** Externals with `= { 0 }` follow, in
  definition order. A static defined after `aiTaskGlobs` still lands before
  it.
- **Every item of 8 bytes or more is 8-aligned:** arrays of `char` or `int`,
  and structs of `int`s alike. A 4-byte item is 4-aligned. A `.bss` section
  takes the alignment of its largest item (Advisor.c's and AITask.c's are
  8-aligned; Bubbles.c's 4-byte one is 4-aligned).

The original's initialised `.data` ends at `004b2dad` (its last non-zero
byte), so `.bss` starts at `004b2db0`, with `advisorGlobs`.

**The 8 bytes at `004b31c0`** sit between `advisorGlobs` (which ends at
`004b31c0`) and `aiTaskGlobs` (`004b31c8`). Both are 8-aligned, so this is
not padding. The owner is one of:

- Advisor.c: an external `= { 0 }` defined after `advisorGlobs`. An
  uninitialised static of Advisor.c would come before `advisorGlobs`.
- AITask.c: an uninitialised static, or an external `= { 0 }` defined
  before `aiTaskGlobs`.

**The 2,480 bytes at `004b8064`** follow `aiTaskGlobs` (0x4e9c bytes, which
ends at `004b8064`). They end at GAME-004's 4-byte word (`004b8a14`).
`004b8064` is not 8-aligned, so no item of 8 bytes or more starts there, and
neither does an 8-aligned object. The word at `004b8064` is either:

- AITask.c's last item: an external of 4 bytes or less, defined after
  `aiTaskGlobs`, then perhaps more items from `004b8068`;
- or alignment padding before an 8-aligned item at `004b8068`. That item
  belongs to AITask.c, GAME-003 (`bezier.c`) or GAME-004, before its word at
  `004b8a14`. GAME-004's `.bss` would then begin with an uninitialised
  static, ahead of the external word.

The layout allows each of these. No evidence picks one or gives the items'
sizes. OpenLRR defines nothing there either. It places `advisorGlobs` at
`004b3db0` and `aiGlobs` (0x4e9c bytes) at `004b41c8`, in a build of the
game whose addresses are `0x1000` higher, so it has the same 8-byte gap.
Since nothing
names the bytes, this change defines nothing. A guessed definition would be
filler (ADR 0005 rule 4). GAME-004's file is #828's.

## Follow-ups (not filed)

1. **AITask.c's unreferenced 1.0 at `0049e010`**, as a named constant before
   its others, the first in its `.rdata`. Check that it costs no counter
   phase.
2. **The `.bss` gaps**, when a source names them. A debug or unused
   variable of AITask.c is the likeliest owner of both gaps: AITask.c is the
   one candidate on both sides of `aiTaskGlobs`.

## Checks

- `make reccmp-check` passes: no function rises or falls. `make
  match-progress` rewrote the baseline header, the listing digests and
  `binary-progress.json`.
- `make unit-data-offsets` and `make pe-audit` pass. Neither writes a
  committed file. The image end is unchanged at `0x7d8000`.
- `make tools-test` passes without `--allow-growth`, and so does `make
  winapi-link-check`.
- The native build compiles, and its 39 ctest tests pass with the real
  WADs. `LegoRR` starts with the offscreen driver and runs until the
  timeout.

## Reproduce

```
make reccmp-check && make match-progress
python3 tools/data_identity.py            # data_identity, on the original-mode link
make match-function SOURCE=src/game/advisor.c
python3 tools/phase_probe.py pads-at src/game/advisor.c 110:1 110:17
```
