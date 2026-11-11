# Finding 535 — Lego.c's candidate boundary is undecidable

Issue #445, R4's first step. **Verdict: undecidable** whether GAME-021 is one
original translation unit or two at `0042f350`. The observed data steps are
compatible with both, and neither linkage nor object ownership survives in the
available executable. Keep `0041f870`–`00435b20` as one **provisional** unit
in the spec and link-order list, not a claim that it was definitely one
object. A two-object split is not supported strongly enough to change the map.

The candidate partitions the 216 manifest functions 106/110. All ranges in
this finding are half-open unless a list of actual addresses is stated. The
original executable has SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
Reproduce the instruction, operand, call, padding and marker inventories with:

```sh
python3 tools/experiments/445/analyze.py > /tmp/445-evidence.txt
```

This uses `pefile` and Capstone via `translation_units.py`'s reachable-code
extraction, the function manifest, and the original PE. The inventories count
**exact operand addresses**, not independently proven variables or objects;
an operand may point into a larger aggregate or coincidentally be a numeric
immediate. Indirect calls, unreachable code, unreferenced data, relocations and
source linkage are not exhaustively recovered. The decompile is
`docs/decompiles/full/LEGORR.full.c`.

| Evidence | Observation | Weight |
| --- | --- | --- |
| `.text` alignment and fill | `0042f2e0` ends at `0042f34d`; three `90` bytes precede aligned `0042f350`. Known inter-unit starts `004065f0` and `00406990` have ten and two `90` bytes; intra-unit `0042f2e0` has fourteen. `0041f870` and `00435b20` also follow NOP fill. | Neutral: ordinary 16-byte function alignment applies both within and between objects. |
| Calls and linkage | 53 distinct direct caller/callee pairs cross the proposed cut, including `0042f2e0` → `0042f6f0` and `00435940` → `0042f0c0`. The decompile shows calls but does not preserve C `static` or the original COFF symbols. Reconstruction linkage is not evidence of original linkage. | Weak against a split by cohesion; these calls are legal between objects if the callees have external linkage. No cross-cut callee is proven `static`. |
| Compiler-generated code | The candidate function has indexed switches at `0042f425` and `0042f545` with tables at `0042f668` and `0042f6a8` in its own `.text` body. They are function-local switch machinery, not a per-TU start or end marker. No identifying TU-specific compiler artifact was established in this range. | Neutral; lack of an identified artifact is not proof of absence. |
| `.rdata` constants | The left side uses 29 exact `.rdata` addresses, the right 14, four shared. Later-only floats at `0049e350`–`0049e370` follow left-only `0049e34c`; but the right also uses `0049e2c8` (−1.0) amidst left-side constants, and both sides use `0049e2b0` (1.0), `0049e2b4` (25.0), `0049e2b8` (0.0) and `0049e338` (−0.5). | Weak for two contributions from the late run; overlap weighs against a clean partition. Literal pooling, first-user placement and unknown ownership mean overlap is not proof of one object. |
| D3DRM header constants | The complete `.rdata` scan has the 20 copies in finding 524, none between the GAME-020 and GAME-022 anchors or inside the candidate run. | Weak against two header-including objects; neither half is required to include `d3drm.h`, so absence cannot disprove a split. |
| Strings | First-used literals rise from left-side `004a46dc` (`tvcamera`) to right-side `004a46e8` (`Activity_Build`) and continue through `004a4890`. The full scan finds 287 left and 37 right exact string addresses, three shared; earlier pooled strings also occur. | Weak for a split: the same rise is expected from first use within one object. Literal pooling prevents treating shared strings as private ownership. |
| `.data` / ordinary `.bss` | Only `004a3558` is classified as initialized non-string data. In the GAME-021 attributed `.bss` range, right-side operands `004de424`, `004de588`, `004de58c` are interspersed with left-side `004de428`–`004de584`; some could be fields of larger objects. Other operands point outside that range. | Weak against a clean two-run split, not proof: references are not definitions, and field ownership and object sizes are unknown. |
| Commons | The sweep finds 315 left and 54 right exact operands in the likely common region, 37 shared. Finding 525 predicts reverse **first COFF common definition** order, not order of uses; the original names, sizes and definition status are missing. | Neutral: these addresses cannot establish the predicted two-object allocation order. `unit_data_offsets.py` (finding 529) compares reconstructed annotated items and cannot supply missing original COFF ownership. |
| Source strings | The referenced strings inventory contains `Lego.cfg` but no attributable `.c` file path or assertion naming either side; the candidate decompile uses Ghidra-generated names and contains no preserved `__FILE__` linkage. | Neutral: no source-name boundary anchor. `Lego.cfg` is configuration, not a source filename. |

The late `.rdata` and string runs begin at first-use changes rather than an
independently identified object start. In particular, `0049e2c8` has a
right-side first user (`00433e80`) amid constants used earlier in code, so a
naive split at `0049e350` does not partition all referenced constants. Nor can
we turn the observed common addresses into two runs by sorting references:
finding 525's reverse allocation applies to original definitions in COFF
symbol-table order, not to the addresses' users.

## Consequences for R4

Relocation should treat GAME-021 as one provisional object in the current
link-order list; do not invent a new unit id, filename, or boundary from this
measurement. The remaining R4 relocation is separate work. If independent
original object files, a link map/PDB retaining static/public symbols, or
reliable source paths become available, establish the actual definitions and
per-object section contributions, then revisit the spec and link-order key.
A demonstrated private cross-cut callee would refute the split; a demonstrable
second object contribution or source-file anchor would support it. A missing
D3DRM copy or a data step by itself does neither.
