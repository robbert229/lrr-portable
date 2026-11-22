# 693. Unit printf formats that come from game data

Issue #954. Follows #951 (PR #953) and PR #955.

## Summary

1. **Several units pass data text as a printf format**, so a mod or custom
   level controls it (audit below). PR #955 made every unit
   `sprintf`/`vsprintf` a bounded `snprintf`/`vsnprintf`. After that,
   `vsnprintf` still passed every conversion except `%b` to the host's C
   library, and `snprintf` reached the host unchanged. A mod's `%n` could
   therefore write memory, an unknown letter could abort under the Windows
   UCRT, and `z`, `j`, `t` or `ll` could misread later arguments.
2. **The units' `snprintf` and `vsnprintf` now use a safe formatter**
   (`src/crt/safe_format.h`, through `crt/vc5names.h`):
   - It parses each conversion and reads the argument at its C type.
   - Only well-formed standard conversions reach the host's `snprintf`,
     one at a time.
   - `%n` writes nothing.
   - An unknown letter, the port's `%b` image token among them, prints as
     the letter alone and takes no argument. A `*` before it takes its
     `int`.
   - Widths and precisions are capped.
   - Integer, string, pointer and infinity output is the same on every
     host; float digits and rounding are the host's.

## Audit: unit call sites whose format can come from data

Every unit printf call reaches one of these formatting points. The callers
listed pass a non-literal format.

| Formatter | Callers with data formats | Data |
| --- | --- | --- |
| `text_window_printf`/`text_window_vprintf` (`textwindow.c`, imported as `text_buffer_append_format`) | `frontend_unit.c` save and level windows (`slotText`, `levelText`, `tutorialText`, level names); `lego_unit.c` dialog `title`, `msg`, `ok`, `cancel` | `Lego.cfg` and `.ol` text |
| `text_window_printf_overlay` (imported as `objective_advisor_text_copy`) | `text.c` (NERPs and game messages) | `Lego.cfg` text messages |
| `font_print_f`/`font_v_print_f2` (`font.c`) | `frontend_unit.c` menu banners, cycle names, select strings, version string; `reward.c` labels; `lego_unit.c` custom names | `Lego.cfg` menu text, player names |
| `front_util_print_f` (`frontend_unit.c`) | front-end level names | `Lego.cfg` |
| `tooltip_set_content` (`tooltip.c`) | `interface_unit.c`, `helpwindow.c`, `priorities.c`, `lego_unit.c` tooltips | `Lego.cfg` tooltip text |
| `formatted_text_escape_decode_and_emit` (`utils.c`) | `lego_unit.c` object names and themes, `reward.c` complete, failed and quit text, `interface_unit.c`, `helpwindow.c` | `Lego.cfg` |
| `panel_text_window_print_f` (`panel_unit.c`) | `info_unit.c` info messages (`%b` for image tokens, #904, #951) | `Lego.cfg` info text |
| `snprintf` | `frontend_unit.c` overwrite prompt (`frontGlobs_overwriteText`) | `Lego.cfg` |
| `Container_Frame_FormatName`, `file_error_file` | literal formats only | — |

These paths take literal formats only, so the change does not cover them:

- `fprintf(stderr, ...)` diagnostics;
- the `wsprintfA` call in `soundsys.c`;
- `file_printf`, which reaches `SDL_IOvprintf`, a filesystem substitution.

## Remaining risk

Format text still chooses which arguments are read. A `%s` or `%d` that
the caller supplies no argument for reads the next argument slot, as it
did in the original.

`font_v_print_f2` copies its format into a 1024-byte stack buffer, and each
`%b` grows to an image token there. The original did not bound the copy
(issue #967); the port does. It stops at 1023 characters and never starts a
`%b` that does not fit whole. A conversion the cut leaves unfinished takes no
argument (the safe formatter above, not the host's C library, reads it), so a
cut format reads only arguments the whole format would.
`src/gods98/test_font_format_bound.cpp` measures long formats and many `%b`.
The longest line in the shipped `Lego.cfg` is 334 characters, so no shipped
text is cut.
