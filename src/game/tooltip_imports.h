// SPDX-License-Identifier: LGPL-3.0-or-later
/* ToolTip.c (GAME-046) imports: C declarations of the callees the unit uses
 * from other modules (ADR 0005 step 6; finding 597). Included only by
 * tooltip.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names a definition with C linkage. */
#ifndef LEGORRR_GAME_TOOLTIP_IMPORTS_H
#define LEGORRR_GAME_TOOLTIP_IMPORTS_H

#include "gods98/draw.h"
#include "gods98/font_api.h"
#include "lego.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Font_GetStringInfo (the Font.h inline at 00401c80), by the port's name:
 * advisor.c's formatted_text_printf, over Font_VGetStringInfo. */
void formatted_text_printf(const struct TextureSurface* font,
                           uint32_t* width, uint32_t* lineCount,
                           const char* msg, ...);

/* SFX.c's SFX_Random_SetAndPlayGlobalSample (00464ef0), by the port's name,
 * which sfx.c's definition takes in the matching build. */
int sound_submit_with_global_cooldown(int alias, int* out_handle);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_TOOLTIP_IMPORTS_H */
