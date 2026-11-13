// SPDX-License-Identifier: LGPL-3.0-or-later
/* ScrollInfo.c (GAME-039) imports: C declarations of the callees and
 * globals the unit uses from other modules (ADR 0005 step 6; finding 611).
 * Included only by scrollinfo.c and test_c_boundaries.cpp, so the other C
 * units' symbol numbering does not change (finding 568, "Unit-private
 * imports header"). Each declaration names a definition with C linkage, by
 * the name it has in the matching build.
 *
 * Images.c: Image_LoadBMPScaled (0047d9d0) and Image_DisplayScaled
 * (0047dcb0) are images.h's adapters (scrollinfo.h includes it).
 * Panel.c (panel_unit.c): panelGlobs (005000e0), read inline for the
 * panels' current positions, and Panel_IsCollapsed (0045ab30). */
#ifndef LEGORRR_GAME_SCROLLINFO_IMPORTS_H
#define LEGORRR_GAME_SCROLLINFO_IMPORTS_H

#include "panel_unit.h"

#endif /* LEGORRR_GAME_SCROLLINFO_IMPORTS_H */
