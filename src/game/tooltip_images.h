// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-046's copies of gods98 Images.h's inline size getters,
 * Image_GetWidth and Image_GetHeight, which the original expands inside
 * ToolTip_Draw (0046bc30). They have no address of their own. Only
 * tooltip.c includes this file, just before ToolTip_Draw, so the functions
 * above keep their symbol counter (finding 538); in a header they add no
 * annotation-lint debt, as panel_maths.h. */
#ifndef LEGORRR_GAME_TOOLTIP_IMAGES_H
#define LEGORRR_GAME_TOOLTIP_IMAGES_H

static __inline int tooltip_image_get_width(const Image* image)
{
    return image->width;
}

static __inline int tooltip_image_get_height(const Image* image)
{
    return image->height;
}

#endif /* LEGORRR_GAME_TOOLTIP_IMAGES_H */
