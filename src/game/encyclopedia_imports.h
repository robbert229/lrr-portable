// SPDX-License-Identifier: LGPL-3.0-or-later
/* Encyclopedia.c (GAME-014) imports: C declarations of the callees and
 * globals the unit uses from other modules (ADR 0005 step 6). Included only
 * by encyclopedia.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header"). */
#ifndef LEGORRR_GAME_ENCYCLOPEDIA_IMPORTS_H
#define LEGORRR_GAME_ENCYCLOPEDIA_IMPORTS_H

#include "gods98/files.h"
#include "panel_unit.h"

#ifdef __cplusplus
extern "C" {
#endif

struct BlockRec;
struct Lego_Object;
struct ViewProjectionAdapter;
struct EntitySlot;

#define encyclopedia_typeCounts ((unsigned int*)((char*)&legoGlobs_config + 0x1bc))

/* GAME-021 Lego.c (lego_unit.c): Lego_DrawObjectSelectionBox. */
void lego_selected_entity_ground_marker_render(
    struct EntitySlot* liveObj, struct ViewProjectionAdapter* view,
    float r, float g, float b);                               /* 004248b0 */
/* GAME-023 LegoObject.c (legoobject_unit.c): LegoObject_GetObjTypeInfo. */
void legoobject_copy_two_dword_pair(void* record, unsigned int* outA,
                                    unsigned int* outB);      /* 00439930 */
/* GAME-032 Panel.c (panel_unit.h) and gods98 Config.c (config.h). */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_ENCYCLOPEDIA_IMPORTS_H */
