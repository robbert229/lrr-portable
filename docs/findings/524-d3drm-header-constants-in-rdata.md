# Finding 524 — The four `.rdata` words are Direct3DRM header constants

Issue #451, stage S6d of [the link-order plan](../plans/388-original-link-order.md).
The bytes are **four separate internal-linkage variables**, not an array:
`D3DRMMAP_WRAPU`, `D3DRMMAP_WRAPV`, `D3DRMMAP_PERSPCORRECT`, and
`D3DRMGROUP_ALLGROUPS`, declared in `d3drmdef.h`, included by `d3drm.h`.
The first three have type `D3DRMMAPPINGFLAG` (a `DWORD`, unsigned 32-bit);
ALLGROUPS has type `D3DRMGROUPINDEX` (a `LONG`, signed 32-bit). Each is
`static const`. These are SDK declarations, not a single symbol or a game
array. Their common source is evidenced by the actual SDK header **and** the
MSVC 5.0 object experiment below. This corrects finding 505's convenient
"table" description, not its observation of the 20 copies.

## Evidence

The original `reference/LegoRR.exe` SHA-256 is
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
`pefile` finds the bytes `01 00 00 00 02 00 00 00 04 00 00 00 ff ff ff ff`
at these virtual addresses in `.rdata`. A Capstone x86 linear sweep of the
whole raw `.text` with skip-data enabled, checking immediate and memory
operands against every byte of all 20 occurrences, finds **two** references,
both to `0049e884` (the third word of `0049e87c`):

| Instruction | Containing original function | Effect |
| --- | --- | --- |
| `0047401a` | `00473ec0`, G98-002 | Loads 4 when enabled and passes it to the mesh interface at vtable offset `0x48`. |
| `00474e65` | `00474e30`, G98-002 | Loads 4 when enabled and passes it to the same method. |

The method is `IDirect3DRMMesh::SetGroupMapping`; SDK `d3drmobj.h` gives its
signature. Its argument 4 is `D3DRMMAP_PERSPCORRECT`. Ghidra's
`docs/decompiles/full/LEGORR.full.c` folds the load into the constant 4;
`src/gods98/surface.cpp` annotates the second function. No other direct
operand in the linear sweep references any byte of these 20 copies. This is
not a proof against indirect references or disassembly alignment errors.

| Copy VA | Attribution from following references and unit order |
| --- | --- |
| `0049e034` | GAME-003 Bezier: `0049e048` used at `00406602`; `0049e04c` at `004068c7`. |
| `0049e87c` | G98-002: the two direct uses above; `0049e890` used at `004730da`. |
| `0049e89c` | G98-003 Viewports: `0049e8ac` used at `00477150`. |
| `0049e8c0` | G98-005 Main: `0049e8d0` used at `00477b19`. |
| `0049e8e8` | G98-006: `0049e8f8` used at `0047913f`. |
| `0049e910` | G98-007 Maths: `0049e920` used at `004795f5`. |
| `0049e94c` | G98-009: `0049e964` used at `0047b8ec`. |
| `0049e96c` | Between G98-009 and G98-011; likely G98-010, but no direct neighbor reference establishes the boundary. |
| `0049e980` | G98-011 Images: `0049e998` used at `0047dbf7`; G98-012 vtable follows. |
| `0049e9b4` | After G98-012 and before G98-015; G98-013 or G98-014, not individually resolved. |
| `0049e9d0` | G98-015: `0049e9e0` used at `00480f47`. |
| `0049ea10` | G98-019 Draw: `0049ea20` used at `00485ecf`. |
| `0049ea38` | G98-020 Lws: `0049ea48` used at `00487057`. |
| `0049ea5c`, `0049ea74`, `0049ea88`, `0049ea9c`, `0049eaac`, `0049eac4` | After Lws and before G98-031. The intervening units have no referenced `.rdata` anchors (finding 512); the individual owners cannot be established by these bytes alone. |
| `0049eadc` | G98-031: `0049eaf0` used at `0048d1d6`. |

These are attribution inferences from first-user references and the ordered
unit ranges in `docs/coverage/translation-unit-ranges.csv`, not linker map
symbols from the original. Some unreferenced floats occur between the table
and the first reference. LINK places object contributions in link order
(finding 516); this anchors the well-supported rows without assuming all
identical patterns have the same owner.

## Reproduction and alternatives

The configured MSVC 5.0 SDK's `include/d3drmdef.h` declares the four names
in that order. Compiling a C file containing `#include <windows.h>`,
`#include <d3drm.h>`, and `int probe(void) { return 1; }` with the configured
MSVC 5.0 compiler (`/c /O2`) emits exactly 16 `.rdata` bytes above. `objdump
-t -s` shows four **local** symbols at offsets 0, 4, 8 and 12, respectively.
They are emitted even when the probe does not use them. The SDK header
separates the third and fourth declarations with a vertex struct; the struct
has no storage. Thus the alignment and values do not require an array.

- **CRT/LIBC:** scanning the configured `libc.lib` for the exact 16 bytes
  finds none. More decisively, the header experiment creates them in an
  ordinary application object, and the copies interleave with application
  constants well before the CRT range.
- **Switch jump table:** these words are flags and a signed group index,
  not code addresses or switch offsets; there are no indexed table uses.
- **Compiler floating-point, RTTI, or exception metadata:** the actual
  header declarations and local COFF symbols explain every byte; the two
  runtime uses also pass a mapping flag to Direct3DRM. No compiler-generated
  structure is needed to explain this pattern.
- **gods98/game static array:** the independently compiled SDK declarations
  produce the same four bytes and the observed mapping use, so a project
  array is unnecessary. Identical bytes alone cannot rule out a coincidental
  unrelated object, but the repeated link positions support the header.

The current reconstruction's converted Bezier, Maths and Draw files explicitly
record that their original copies are missing. The separately existing
`/var/home/johnrowl/source/lrr-portable/build-reccmp/LegoRR.exe` contains no
complete copy of this 16-byte sequence; its map has no `D3DRMMAP` or
`D3DRMGROUP_ALLGROUPS` symbols. That build is from another checkout and is
not a matching build of this worktree, so it does not establish the current
branch's map addresses. No full matching build was run for this documentation
change.

## Consequences for #450 and relocation

For #450, model **four 4-byte local constants** and account for the header at
the beginning of an owning object's contribution before checking offsets;
do not require a single array symbol or infer an owner for a copy solely from
its address. An unreferenced constant can still occupy space. The current
unit-range CSV begins at referenced data, so it cannot itself supply every
contribution start.

For relocation, each original unit that included Direct3DRM needs its own
header constants at its original link position. This should be restored as
units are identified or converted, with MSVC 5.0 compatibility and the
normal entropy and matching-progress gates. The six unanchored late copies
cannot yet be assigned safely to individual units from available evidence;
the plan's claim that each copy uniquely identifies a specific object is not
enough to decide *which* object. This finding identifies the header and
several owners but does **not** implement all 20 inclusions; #451 remains
open. Resolve the remaining ownership before inserting them or treating the
unit-relative layout as complete.
