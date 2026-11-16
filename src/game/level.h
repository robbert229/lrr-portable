// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * Lego.c, LegoObject.c and Objective.c entries declared in level.hpp.
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_LEVEL_H
#define LEGORRR_GAME_LEVEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct LevelState;
/* The matching build calls Lego.c's original-shape entry in lego_unit.c
 * (GAME-021 part 2, finding 571); the port model is unannotated. One
 * declaration either way (finding 538, rule B). */
int lego_terrain_cell_byte_state_classify_current(const int* blockPos,
    unsigned int* crystalsLv0, unsigned int* crystalsLv1,
    unsigned int* oreLv0, unsigned int* oreLv1);                /* 0042c5b0 */
#define terrain_cell_byte_state_classify_current(xy, a, b, c, d) \
    lego_terrain_cell_byte_state_classify_current((xy), (unsigned int*)(a), \
        (unsigned int*)(b), (unsigned int*)(c), (unsigned int*)(d))
int grid_record_count_difference_get(int row, int column, int slot,
                                     int subtract_first_table); /* 00437460 */
void render_config_six_field_copy_out(uint32_t out[6]);    /* 004370b0 */
uint32_t grid_record_count_get(int row, int column, int slot); /* 004374a0 */
/* NERPs.c's 00455300 (tutorial_script.c, finding 586): the C matching
 * build's callers (reward.c) call the unit's; natively the port's level.cpp. */
#define entity_type_grid_record_count_get nerps_object_count
uint32_t entity_type_grid_record_count_get(const char* type_name, int slot); /* 00455300 */
/* Objective_HasAchieved (00458650) takes no argument; the macro drops
 * the port's in the matching build's C callers, and rewrites this
 * declaration into an old-style one. */
#define level_objective_result_get(s) level_objective_result_get()
int level_objective_result_get(const struct LevelState* s); /* 00458650 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEVEL_H */
