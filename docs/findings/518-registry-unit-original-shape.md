# Finding 518 — registry path query (G98-029) in original shape

Issue #415. The ADR 0005 conversion starts with `convert_unit.py G98-029`.
The three functions at `0048b330`–`0048b4a0` now live in
`src/gods98/g98_029.c`, in original order, with a C boundary header.

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `0048b330` | `registry_path_split` | 64.00% | 100.00% |
| `0048b360` | `registry_path_accessor` | 31.82% | 100.00% |
| `0048b390` | `registry_path_create_keys` | 10.53% | 65.56% |

## Boundary and behavior

The dossier shows the three registry functions form a call chain: the
accessor fixes the root to `HKEY_LOCAL_MACHINE`, the recursive walker calls
the splitter and itself, and no function uses data attributed to the adjacent
stream or WAD code. G98-028 manipulates startup stream commons; G98-030 begins
with the private WAD string reader, called by its archive loader. Neither
neighbor calls this chain. There is no attributed registry data to provide a
section-order anchor. The normal 16-byte alignment and NOP fill are not object
boundaries (finding 505), so the separate-unit boundary is supported by
behavior but remains **low confidence**, rather than independently proven.
The spec records that limitation. No non-leaf neighbor is required by the
registry chain.

The walker creates each component and closes it on return; at the leaf modes
0 and 1 query identically, while other modes fail after walking the path.
It does not inspect the returned type. As in the original, the splitter has
no null or length guard and the walker uses a 100-byte component buffer.
Windows uses the real ADVAPI32 APIs. The native adapter maintains process-local
case-insensitive key paths, initially empty, and reports missing values; no
native mechanism supplies Windows registry values.

## Matching and validation

Against the baseline at the conversion's base, `make reccmp-check` reports
no drops, lost mappings or new mappings. The only outside score change is
`WinMain` (30.81% → 31.83%), a direct caller whose startup-stream reset was
restored to its original call site. The three former WinMain registry hook
functions remain as unused definitions to preserve the declaration count:
removing them changed `winmain_parse_options` from 79.91% to 79.48%.
No rule 2 exception is needed. The other mapped callers do not change score.

The native CMake build and all 34 tests pass, including `c_boundaries` and a
registry traversal test. The C unit passes the GNU89 declaration check.
`translation_units.py build` reports zero data conflicts. The matching gate
was run before refreshing the baseline and whole-binary progress together
with `make match-progress`.
