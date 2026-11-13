// SPDX-License-Identifier: LGPL-3.0-or-later
/* Port tail for Init.c (gods98/init.c, LATE-002, finding 613): the
 * valid-mode filter helpers that the port's start-up model
 * (directdraw_device_initialize, surface.cpp) and test_crt call, declared
 * in crt/crt.hpp. They used to keep their own 200-row table in crt.cpp;
 * they now read and write the unit's initGlobs.validModes and
 * validModeCount, the original's table (00568060, 005754e0). In the
 * Windows builds an append is the unit's Init_AddValidMode itself. */
#include "crt/crt.hpp"
#include "gods98/init.h"

void display_resolution_filter_tuples_reset(void)
{
    initGlobs.validModeCount = 0;
}

void display_resolution_filter_tuple_append(int width, int height, int bpp)
{
#if !defined(LEGORRR_BACKEND_MINIWIN)
    init_add_valid_mode((unsigned int)width, (unsigned int)height,
                        (unsigned int)bpp);
#else
    /* Init_AddValidMode's body: the native port does not build the unit's
     * code. Like the original, it does not check the table's bound. */
    DirectDraw_Mode* mode = &initGlobs.validModes[initGlobs.validModeCount];
    mode->width = (unsigned int)width;
    mode->height = (unsigned int)height;
    mode->bitDepth = (unsigned int)bpp;
    initGlobs.validModeCount++;
#endif
}

int display_resolution_filter_tuple_count(void)
{
    return (int)initGlobs.validModeCount;
}

void display_resolution_filter_tuple_get(int index, int* w, int* h, int* bpp)
{
    const DirectDraw_Mode* mode =
        (index >= 0 && (unsigned int)index < initGlobs.validModeCount)
            ? &initGlobs.validModes[index] : NULL;
    if (w != NULL) {
        *w = mode != NULL ? (int)mode->width : 0;
    }
    if (h != NULL) {
        *h = mode != NULL ? (int)mode->height : 0;
    }
    if (bpp != NULL) {
        *bpp = mode != NULL ? (int)mode->bitDepth : 0;
    }
}
