# 685. `.data` and `.rdata` definition forms: FrontEnd.c, Config.c's literals, GAME-004's `""` and Smoke.c

Issue #828: finding 680's follow-ups 1 and 3, without Stats.c's records
(those are in `legoobject_unit.c`, #829's file). Stacked on #831
(`identity/827-engine-root` `fbbe53a7`, finding 684), and measured against
it with one full matching build on each side, under the gate's link mode.
The per-item offset table below was measured on the same change on
`main` `ebc07595`, before the rebase.

## Summary

1. **Config.c's three pooled names are literals again.** The units read
   `":"`, `"::"` and `""` through `s_config_rgb_separator`,
   `s_config_separator` and `s_config_angle_empty`. Config.c defined all
   three as `const` arrays, so they sat in `.rdata`. The original writes
   them as literals. MSVC 5.0 emits every literal as a `/Gf` COMDAT in
   `_DATA`, and LINK keeps the first object's copy (finding 569). Every use
   is now a literal and Config.c defines none of them, so each lands in its
   first user:
   - `":"` in GAME-006 (`building_model.c`), at its original offset
     (`004a1cd4`);
   - `"::"` in G98-002 (`container.c`), 8 bytes early (it was 1,036);
   - `""` in GAME-004 (`animation_object.c`). An all-zero literal goes to
     the object's `.bss`, which is why the original has it at `004b8a14`,
     in GAME-004's `.bss`. The rebuilt `""` was already there, as GAME-004's
     own literal. It is now the image's only `""`.

   Each literal gets one `// STRING:` marker, in the unit that owns its
   address (finding 569's rule). The extern declarations in 14 files and
   their `extern "C"` wrappers are removed.
2. **FrontEnd.c's key table is `.data`.** `s_keyCharacters[256]` loses its
   `const`, so the 1,024 bytes follow the `-1` words in `.data`, as in the
   original. `"Null"` becomes a literal in its three users. It lands in
   FrontEnd.c's `.data` (`004a23f4`, 56 bytes early).
3. **Smoke.c's four commons are one struct.** `smokeGlobs` holds `textures[10]`,
   `listHead`, `listTail` and `textureCount`, in the original's order. It is
   a single 52-byte common at `00552100`. The rebuild had four commons in
   another order (`count, head, tail, textures`).
4. **Exact-function code bytes go from 349,089 to 350,987** (54.33% to
   54.62%). Eight functions rise, four of them to 100%. One function falls
   by 1.21 points, by register drift only. Its source is unchanged (ADR 0005
   rule 2, below).
5. **`.data` items at their original offset go from 158 to 192** (3,037 to
   3,401 bytes). The first `.data` break is now GAME-012's item order
   (`004a1d5c`, +132). Finding 680 modelled 384 items for three fixes, but
   that model includes Stats.c's records, which are not in this change.
   Its table shows the GAME-009/GAME-012 break, which still stops the run
   here.

## Before and after

| Measure (`binary-progress.json`) | #831 `fbbe53a7` | This change |
| --- | ---: | ---: |
| **Exact-function code bytes** | **349,089 (54.33%)** | **350,987 (54.62%)** |
| Exact functions, full / game-only | 2,640 / 2,352 | 2,644 / 2,356 |
| Functions that rose / fell | | 8 / 1 (drift) |
| Accuracy, full / game-only | 97.63% / 97.37% | 97.63% / 97.38% |
| `.text` identical at original addresses | 6.83% | 6.83% |
| `.data` / `.rdata` identical (section-relative) | 24.43% / 11.52% | **26.20% / 12.48%** |
| Identical prefix, raw / masked | 11 B / 7,187 B (to `00402c13`) | same |
| Correct-size functions, full | 2,804 | 2,804 |
| Units with size agreement | 2 | 2 |
| Data-offset agreement (`layout`) | 20 of 57 units | **22 of 57** |
| Entry point (original `0048f000`) | `0048eb90` | `0048eb90` |
| SizeOfImage (original 3,608,576) | 4,030,464 | 4,030,464 |
| Tail roots | 2 | 2 |

`data_identity`, region by region (items at original offset / paired; none
is at its original address yet, because no region starts in place):

| Region | Original start | Rebuilt start, before → after | At original offset, before → after | First difference, after |
| --- | --- | --- | --- | --- |
| `.rdata` | `0049e000` | `004b2000` → `004b2000` | 0 of 95 → 0 of 95 | (unchanged) |
| `.data` | `004a1000` | `004b8000` → `004b8000` | **158 (3,037 B) → 192 (3,401 B)** of 2,455; objects 5 → 7 | `004a1d5c` GAME-012, +132 (was `004a1cd4`, Config.c's `":"` in `.rdata`) |
| `.bss` | `004b2db0` | `004cf190` → `004cf590` | 1 of 149 → 1 of 149 | (unchanged: `aiTaskGlobs`, 8 early) |
| commons | `00549520` | `005a35e0` → `005a39e0` | 2 of 61 → 2 of 58 | (unchanged: `005520e8`, +1,272) |

- `.data`'s raw prefix stays at 12 bytes (the CRT's code pointer).
- The commons' paired count falls by three because Smoke.c's four items
  are now one.
- `.rdata` loses 1,040 bytes (22,344 to 21,304). `.data` gains 1,024 (94,608
  to 95,632).

**Identity cost.** The gain costs no identity measure: the prefix, the
correct-size functions, the unit sizes, the entry point and SizeOfImage are
unchanged. But while the tail is still linked, the rebuilt `.data` grows
further from the original's 73,136 bytes, and the `.bss` and the commons
start 1,024 bytes further from their original addresses. Without the tail,
`.data` would be 1,060 bytes short (finding 680, section 4), and these
1,024 bytes are most of that.

Where the moved items land (offset from their region's start, rebuilt
minus original):

| Item | Original | Before | After |
| --- | --- | --- | --- |
| `":"` (GAME-006) | `004a1cd4` | in `.rdata` | **0** |
| FrontEnd.c's `-1` words | `004a1f48` | −1,048 after them | −72 |
| `s_keyCharacters` | `004a1f60` | in `.rdata` | −72 |
| `"Null"` | `004a23f4` | in `.rdata` | −56 |
| `"::"` (G98-002) | `004aab18` | in `.rdata` | −8 |
| `""` (GAME-004 `.bss`) | `004b8a14` | Config.c's array in `.rdata` | −2,488: the `.bss` gaps of finding 680 follow-up 8 (2,480 + 8) |
| `smokeGlobs` | `00552100` | four commons, another order | +1,280: `textGlobs` and the other commons that belong in `.bss` (follow-up 2) |

FrontEnd.c's −72 and `"Null"`'s −56 are the item-order breaks in GAME-009
and GAME-012, and smaller ones up to FrontEnd.c.

## Scores

| Function | Before | After |
| --- | ---: | ---: |
| `00406990` entity_type_config_resource_row_load | 99.52 | **100.00** |
| `00459560` objinfo_load_health_bar | 99.27 | **100.00** |
| `00459900` objinfo_load_hunger | 98.41 | **100.00** |
| `00459a50` objinfo_load_bubble | 98.21 | **100.00** |
| `0045c040` mission_crystal_quota_setup | 93.75 | 95.83 |
| `0045cdc0` priorities_load_images | 97.96 | 98.98 |
| `0047f980` wad_file_read | 97.59 | 98.80 |
| `00457d40` objective_load_original | 61.34 | 61.49 |
| `00411360` front_menu_item_select_test_string_collision | 92.45 | 91.24 (drift) |

The rises come from operands that now point where the original's do.

### The one drop: the counter phase (finding 550)

`00411360` has the same instruction count and differs only in register
choice and the order of two loads. Its source did not change. The cause is
the symbol counter (findings 538, 550):

- Before, `frontend_unit_imports.h` declared the two externs at the top of
  the unit.
- Now the unit creates `":"` at its first use (`00410fb0`), and `""` later.
  So `00411360` sees one symbol fewer.

`make match-function` on the three variants:

| Variant | Counter at `00411360` | `00411360` | `00411a00` |
| --- | ---: | ---: | ---: |
| `main` | 0 | 92.45 | 84.59 |
| this change | −1 | 90.63 (91.24 in the full build) | 84.38 (84.59 in the full build) |
| this change, plus the old unused externs | +1 | 91.84 | 84.38 |

Neither the `"Null"` literal nor the non-`const` table moves either
function. With them reverted one at a time, the scores stayed at 90.63 and
84.38. `00411360` is best at the old phase, and the only way back to that
phase is to add a declaration the original does not have. The baseline is
refreshed under ADR 0005 rule 2 instead: the source is unchanged, the
listing differs only in registers, and the net effect is +1,898
exact-function bytes. `00411a00` holds its baseline in the full build.

## Files outside this issue's map

These edits are minimal and line-level:

- Literal for name, and the extern declaration deleted:
  - `lego_unit.c`, `lego_unit.h`, `lego_unit_imports.h`, `entity_stats.c`
    and `main.c`/`main_imports.h` (#829's files);
  - `interface_unit.c`, `weapons.c`, `dependencies.c`/`_imports.h`,
    `lighteffects.c`, `objective_unit.c`/`_imports.h`, `objinfo.c`/`_imports.h`,
    `reward.c` and `vehicle_model.c`.
- `tools/test_convert_unit.py` (#830's directory): G98-006's
  hand-conversion test expected Config.c to define the three shared
  literals. It now expects none.

## Checks

- `make reccmp-check` passes after `make reccmp-baseline-update`. Eight
  functions rose and one fell by drift, under ADR 0005 rule 2.
  `make match-progress` rewrote the baseline, the listing digests and
  `binary-progress.json`.
- `make unit-data-offsets` and `make pe-audit` pass. Neither wrote a
  tracked file.
- `make tools-test` passes without `--allow-growth`, and so do
  `make winapi-link-check` and `make reccmp-lint` (466 violations, 0
  removable).
- The native build compiles and its 39 ctest tests pass. `LegoRR` starts
  with the offscreen driver and the shipped WADs, and runs until the
  timeout.

## Reproduce

```
make reccmp-check && make match-progress
python3 tools/data_identity.py
make match-function ADDR=0x00411360
make unit-data-offsets && make pe-audit
```
