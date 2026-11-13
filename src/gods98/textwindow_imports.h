// SPDX-License-Identifier: LGPL-3.0-or-later
/* TextWindow.c (G98-017) imports: C declarations of the Font.c callees the
 * unit uses (ADR 0005 step 6; finding 600). Included only by textwindow.c
 * and src/game/test_c_boundaries.cpp, so the other C units' symbol numbering
 * does not change (finding 568, "Unit-private imports header"). Each
 * declaration names a definition with C linkage. */
#ifndef LEGORRR_GODS98_TEXTWINDOW_IMPORTS_H
#define LEGORRR_GODS98_TEXTWINDOW_IMPORTS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Font_GetCharWidth (0047a520): font.c in the matching build,
 * bitmap_font_port.cpp natively; game/frontend.hpp declares it too. */
int bitmap_font_glyph_measure_width(void* font, unsigned char glyph);

/* Font_OutputChar (0047a470) in the original ABI: font.c in the matching
 * build (finding 605). Natively the port's entry at that address takes a
 * render target, so this is the PORT-ONLY adapter at the end of
 * gods98/bitmap_font_port.cpp. */
int font_output_char(void* font, int x, int y, unsigned char c, int render);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_TEXTWINDOW_IMPORTS_H */
