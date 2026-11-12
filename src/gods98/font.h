// SPDX-License-Identifier: LGPL-3.0-or-later
/* Font.c's (G98-008) text renderer for units rebuilt as C.
 *
 * C boundary header (ADR 0005) for the declaration in game/frontend.hpp,
 * which carries extern "C"; src/game/test_c_boundaries.cpp includes both.
 * Shared C++ headers do not include this file (finding 506). */
#ifndef LEGORRR_GODS98_FONT_H
#define LEGORRR_GODS98_FONT_H

#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct TextureSurface;

/* FUN_0047a150, with the port's software-target contract rather than the
 * original's position and va_list. */
void formatted_text_format_draw(const struct TextureSurface* font,
                                uint32_t* width_out, int* line_count,
                                void* px, int pitch, int w, int h,
                                int x, int y, const char* format,
                                va_list args);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_FONT_H */
