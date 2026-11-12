// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * Fonts.c entries (surface.hpp, record.hpp, bitmap_font.cpp).
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GODS98_FONT_API_H
#define LEGORRR_GODS98_FONT_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct TextureSurface;
void* font_atlas_record_create(const char* name);           /* 00479ee0 */
void font_atlas_record_release(void* record);               /* 0047a550 */
uint32_t object_field_be4_dword_get(const void* record);    /* 0047a540 */
/* Font_PrintF (0047a180) in the original ABI; unannotated adapter. */
void font_print_f(const struct TextureSurface* font, int x, int y,
                  const char* msg, ...);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_FONT_API_H */
