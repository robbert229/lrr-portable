// SPDX-License-Identifier: LGPL-3.0-or-later
/* Loader.c (GAME-025) imports: C declarations of the callees the unit uses
 * from other modules (ADR 0005 step 6). Included only by loader.c and
 * test_c_boundaries.cpp, so the other C units' symbol numbering does not
 * change (finding 568, "Unit-private imports header"). The unit is compiled
 * only where DirectDraw exists (not natively), so it names the Images.c and
 * DirectDraw.c units directly. */
#ifndef LEGORRR_GAME_LOADER_IMPORTS_H
#define LEGORRR_GAME_LOADER_IMPORTS_H

#include <stdint.h>

#include "gods98/files.h"
#include "gods98/images.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TextureSurface;

/* GAME-001 Advisor.c: Font_GetStringInfo, a Font.h inline (advisor.c). */
void formatted_text_printf(const struct TextureSurface* font,
                           uint32_t* width, uint32_t* lineCount,
                           const char* msg, ...);                 /* 00401c80 */
/* gods98 Utils.c (utils.c): Util_Tokenise and Util_StrCpy. */
int sound_variant_split(char* text, char** out, const char* separators); /* 00477440 */
char* string_duplicate(const char* text);                         /* 00477550 */
/* gods98 Main.c (frameloop.cpp): Main_LoopUpdate. */
void main_loop_frame_pump(int draw_projection_overlay);           /* 00477f30 */
/* gods98 Font.c (font.c): Font_PrintF. */
void font_print_f(const struct TextureSurface* font, int x, int y,
                  const char* msg, ...);                          /* 0047a180 */
/* gods98 DirectDraw.c (directdraw.c): DirectDraw_Flip. */
void directdraw_flip(void);                                       /* 0047c8d0 */
/* gods98 Images.c (images_unit.c). */
void Image_Remove(Image* dead);                                   /* 0047d470 */
Image* Image_LoadBMPScaled(const char* fileName, unsigned int width,
                           unsigned int height);                 /* 0047d9d0 */
void Image_DisplayScaled(Image* image, const Area2F* src,
                         const Point2F* destPos,
                         const Point2F* destSize);                /* 0047dcb0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LOADER_IMPORTS_H */
