# Finding 525 — MSVC 5.0 common-symbol allocation

Issue #449; relates to #448 and [ADR 0006](../adr/0006-original-link-order.md).
The experiments use the matching build's MSVC 5.0 RTM `cl`, `link` and
`lib` under Wine, `/TC /ML /O2 /Ob1 /DNDEBUG`, `/INCREMENTAL:NO /MAP`, and
`/NODEFAULTLIB`. They test ordinary external C tentative definitions, not
all possible COFF symbol-resolution cases. Run
`distrobox enter developer -- bash -lc 'python3 tools/experiments/449/run.py'`;
the generated sources, objects and maps stay in `build-449/suite/`.

## Measured rule

For these links, scan the **COFF symbol tables** of the included objects in
linker's input/pull order. When an external common (section 0, nonzero value)
is first encountered, remember it. Later commons of the same decorated name
increase its size to the largest value but do not change its position.
Allocate the distinct commons **in reverse of that first-common sequence**,
after ordinary object `.bss`. An `extern`-only entry (section 0, value 0)
does not establish a common's position in these tests. An archive member
contributes only when pulled; moving the archive across the entry object did
not alter these tests' result.

For size `n`, LINK aligns the next address to `min(32, next_power_of_two(n))`
bytes, then reserves `n` bytes. This is size-based, not the C type's natural
alignment: even a 9-byte packed struct is aligned to 16 in this model. Sizes
1–9, 15–17, 31–33, 64, 65 and 129 were tested. A 129-byte object starts at
`...20`, demonstrating the 32-byte cap. Padding is before each object, not
included in the common's COFF size.

This is not alphabetical or size sorting. Names matter **indirectly**:
CL's COFF symbol-table order changes on a rename, and the linker reverses
that order. We have not established CL's symbol-table ordering algorithm, nor
whether all compiler options, `/OPT:REF`, initialized strong definitions or
other COFF producers obey this model. Do not substitute source declaration
order for the COFF stream.

## Experiments

Addresses below are map RVAs plus image base. `a`'s common table is
`shared(8), alpha(8), zebra(12), bee(4)`; `b`'s is
`gamma(4), shared(28), delta(20), cat(4)`. The `shared` size in the output
is 28 regardless of which object is first.

| Change | Inputs or observations | Common order from `/MAP` |
| --- | --- | --- |
| Object order | `m a b` | cat, delta, gamma, bee, zebra, alpha, shared |
| Reverse objects | `m b a` | bee, zebra, alpha, cat, delta, shared, gamma |
| Rename `alpha` to `aardvark` | otherwise same as `m a b` | cat, delta, gamma, bee, aardvark, zebra, shared |
| Extern-only references | `x m a b` and `m a b x` | same as `m a b` |
| Archive position | `libmain ab.lib` and `ab.lib libmain` | same as `m a b` |
| Sizes 1–65 | one object, all referenced | 16 commons in reverse COFF order, not size order: `s15` precedes `s09` |
| Packing and size cap | packed size 9, natural size 16, byte array size 129, byte | `packval` at `00402000`, `natval` at `00402010`, `bigarr` at `00402020`, `tiny` at `004020a1` |

For example the first link puts `cat` at `00402000`, `delta` at
`00402020`, `gamma` at `00402034`, `bee` at `00402038`, `zebra` at
`00402040`, `alpha` at `00402050` and `shared` at `00402060`. Reversing
the objects does not simply reverse every symbol: the shared symbol remains
at its first-common insertion position while its size is the maximum.

## Original and rebuild

The original's virtual zero-fill data starts before `00549520`; finding 505
identifies `00549520` as the likely start of its common region. The original
has no surviving object symbol tables or map in this repository. Its PE and
decompile provide addresses and uses, but not the original names, sizes,
tentative-vs-strong status or COFF symbol-table order of all globals. Thus
an exact forward prediction for the whole region from the measured rule is
**not possible with the available evidence**; treating reconstructed names
and definitions as original inputs would be circular.

One independently interpretable pair is consistent with the rule. The
original `00576500` buffer is passed with length 200 to `mciSendStringA`;
the table cleared as 256 pointers starts at `005765e0`. With the reconstructed
units in the original order (`keys` before `soundsys`), reversing their
common entries predicts sound before keys and rounds `00576500 + 200` up to
`005765e0`. **2 of 2 selected globals have the predicted relative placement,
1 of 1 pair and its separation agree**. The absolute start was taken from the
original, and this does not establish what precedes the pair or prove that
both original definitions were commons. The decompile shows their uses at
`00488...` and `00485...`; annotations are in `soundsys.c` and `keys.c`.
The scoring-only extra key slot is 4 bytes beyond the original's 256 entries.

The matching map has 12 `<common>` entries: the two game/gods98 entries
`sound_mci_return`, `key_names`, then ten CRT entries. The first two appear
in the predicted order with the same `0xe0` separation (`009408c0`,
`009409a0`). The rest need the actual pulled LIBC member sequence and COFF
symbol streams for a full prediction; this experiment does not claim to
have checked those ten. The original counterparts of the CRT commons have
not been established here.

The matching report maps 145 annotated data entities in original addresses
`00549520`–`0076dfff`; their rebuilt order has 4,463 reversed pairs out of
10,440. These are **not 145 independently identified original commons**:
many annotations describe subobjects, initialized/scoring storage or C++
statics. For example `g_view_grid_edge` is placed before `g_view_pan_cap` and
`g_view_pan_scale`, though its original address follows both; and
`s_field_grid_desc` precedes `s_field_grid_buf` in the rebuild although the
original annotations order them oppositely. These are source-side storage
and grouping differences, not evidence against common allocation. Run
`python3 tools/experiments/449/compare.py` after `make reccmp-check` for the
complete 145-row inventory, addresses, and source declarations. Entity
matches and their sizes are Reccmp's reconstruction-derived mappings.

## Consequences for #448

Relocating units into the link-order list is necessary, but not sufficient to
recover common order: commons run in reverse first-definition COFF order,
and declarations, compiler symbol-table ordering, type sizes, library pulls,
and scoring-only storage matter. A future check can read actual COFF tables
and the map, model the first common and max size, then compare the predicted
sequence and offsets against `<common>` map entries. For original validation,
first recover enough original ownership, definition status, sizes and within-
object ordering from independent evidence. The 145-row inventory makes the
remaining storage differences visible but is not that proof. Keep #449 open
until that wider original validation and the full matching-map check exist.
