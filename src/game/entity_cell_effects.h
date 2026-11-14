// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-008 (original translation unit, finding 505; rebuilt in finding
 * 652): building teardown.
 *
 * C-compatible header (ADR 0005). entity_cell_effects.c is compiled as C in
 * the matching build and as C++ in the native port. Shared C++ headers do
 * not include this file: every include and declaration a header adds
 * shifts MSVC's symbol numbering (finding 506). In the matching build
 * three of the names below are the port's (entity_cell_effects_link_names.h,
 * included only by the unit). src/game/test_c_boundaries.cpp includes this
 * header. */
#ifndef LEGORRR_GAME_ENTITY_CELL_EFFECTS_H
#define LEGORRR_GAME_ENTITY_CELL_EFFECTS_H

#ifdef __cplusplus
extern "C" {
#endif

struct Lego_Object;

void building_set_cost_drop_disabled(int disabled);                    /* 00409d40 */
void building_remove_from_terrain(struct Lego_Object* buildingObj);    /* 00409d50 */
void building_drop_cost(struct Lego_Object* buildingObj);              /* 00409f20 */
void object_tracked_replace_and_remove(struct Lego_Object* liveObj);   /* 00409ff0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_ENTITY_CELL_EFFECTS_H */
