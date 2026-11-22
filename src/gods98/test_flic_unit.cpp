// SPDX-License-Identifier: LGPL-3.0-or-later
/* flic_unit.c — two palette packets update disjoint colors and regenerate
 * the high-color lookup while keeping palette index zero transparent. */
#include <cstdint>

#include "gods98/flic_unit.h"
#include "test/lrr_test.h"

int main()
{
    unsigned char chunk[] = {
        21, 0, 0, 0, 4, 0, // Size and Colour256 chunk type.
        2, 0,              // Two packets.
        1, 1, 255, 0, 0,   // Skip index 0, set index 1 to red.
        1, 2, 0, 255, 0, 0, 0, 255, // Skip index 2, set indices 3 and 4.
    };
    FLICSTRUCT flic = {};
    flic.fsSource = chunk;
    flic.fsDisplayMode = FLICMODE_HICOLOR;
    flic.fsPalette256[0] = 63;
    flic.fsPalette256[2 * 3 + 0] = 63;
    flic.fsPalette256[2 * 3 + 1] = 63;
    flic.fsPalette256[2 * 3 + 2] = 63;

    CHECK_EQ(Flic_Colour256(&flic), 1);
    CHECK_EQ(flic.fsPalette256[0], 63u);
    CHECK_EQ(flic.fsPalette256[3], 63u);
    CHECK_EQ(flic.fsPalette256[4], 0u);
    CHECK_EQ(flic.fsPalette256[6], 63u);
    CHECK_EQ(flic.fsPalette256[3 * 3 + 1], 63u);
    CHECK_EQ(flic.fsPalette256[4 * 3 + 2], 63u);
    CHECK_EQ(Flic_GetPaletteColour(0, &flic), 0u);
    CHECK_EQ(Flic_GetPaletteColour(1, &flic), 0xf800u);
    CHECK_EQ(Flic_GetPaletteColour(2, &flic), 0xffffu);
    CHECK_EQ(Flic_GetPaletteColour(3, &flic), 0x07e0u);
    CHECK_EQ(Flic_GetPaletteColour(4, &flic), 0x001fu);
    return lrr_test_result();
}
