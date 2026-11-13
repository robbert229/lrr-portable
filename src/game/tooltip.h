// SPDX-License-Identifier: LGPL-3.0-or-later
/* ToolTip.c (original translation unit GAME-046, finding 505; finding 597):
 * the tool tips, by the unit's own names.
 *
 * C-compatible header (ADR 0005). tooltip.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include this
 * file: every include and declaration a header adds shifts MSVC's symbol
 * numbering (finding 506). src/game/test_c_boundaries.cpp includes it.
 *
 * In the matching build tooltip_link_names.h, which tooltip.c includes
 * first, gives seven of these functions the port's names (frontend.hpp,
 * cmd.hpp, effects.hpp), so every caller reaches them. Natively nothing
 * calls these names: the port tail tooltip_port.cpp and frontend.cpp serve
 * the port's names. */
#ifndef LEGORRR_GAME_TOOLTIP_H
#define LEGORRR_GAME_TOOLTIP_H

#include "effects.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TextureSurface;
struct ToolTip;

void tooltip_initialise(struct TextureSurface* font, unsigned int borderSize,
                        unsigned int offset, float hoverTime,
                        unsigned int appWidth, unsigned int appHeight,
                        unsigned int offsetY, float red, float green,
                        float blue);                               /* 0046b1d0 */
int tooltip_get_type(const char* toolTipName, int* toolTipType);   /* 0046b4d0 */
void tooltip_set_content(int toolTipType, const char* msg, ...);   /* 0046b520 */
void tooltip_add_icon(int toolTipType, Image* image);              /* 0046b660 */
void tooltip_set_sfx(int toolTipType, int sfxType);                /* 0046b700 */
void tooltip_set_flag10(int toolTipType, int on);                  /* 0046b730 */
void tooltip_activate(int toolTipType);                            /* 0046b770 */
void tooltip_reset_timer(int toolTipType);                         /* 0046b7a0 */
void tooltip_update(unsigned int mouseX, unsigned int mouseY,
                    float elapsedAbs);                             /* 0046b7c0 */
void tooltip_draw_box(Area2F box, ColourRGBF rgbNormal,
                      ColourRGBF rgbHighlight, ColourRGBF rgbShadow,
                      int dark);                                   /* 0046b8b0 */
void tooltip_draw(struct ToolTip* toolTip, unsigned int mouseX,
                  unsigned int mouseY);                            /* 0046bc30 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_TOOLTIP_H */
