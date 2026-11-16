// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-008 Font.c's font record, shared with the units that read a font's
 * height (TextWindow.c, Text.c). The +0x... comments are the Win32 offsets;
 * the pointer fields hold real pointers (ADR 0007 decision 4). */
#ifndef LEGORRR_GODS98_FONT_RECORD_H
#define LEGORRR_GODS98_FONT_RECORD_H

#include "gods98/images.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FONT_GRIDWIDTH 10
#define FONT_GRIDHEIGHT 19

/* One font (0xbf4 bytes on Win32). posSet[x][y] is the source rectangle of glyph
 * 32 + y * 10 + x; its width is trimmed to the glyph's ink. */
typedef struct Font {
    Image* image;                                 /* +0x000 */
    Area2F posSet[FONT_GRIDWIDTH][FONT_GRIDHEIGHT]; /* +0x004 */
    unsigned int fontHeight;                      /* +0xbe4 */
    unsigned int tabWidth;                        /* +0xbe8 */
    unsigned int reserved;                        /* +0xbec: not read in the binary */
    struct Font* nextFree;                        /* +0xbf0 */
} Font;

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_FONT_RECORD_H */
