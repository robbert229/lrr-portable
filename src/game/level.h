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
int terrain_cell_byte_state_classify_current(const int xy[2],
    uint32_t* out_a, uint32_t* out_b, uint32_t* out_c, uint32_t* out_d); /* 0042c5b0 */
int grid_record_count_difference_get(int row, int column, int slot,
                                     int subtract_first_table); /* 00437460 */
void render_config_six_field_copy_out(uint32_t out[6]);    /* 004370b0 */
uint32_t grid_record_count_get(int row, int column, int slot); /* 004374a0 */
uint32_t entity_type_grid_record_count_get(const char* type_name, int slot); /* 00455300 */
int level_objective_result_get(const struct LevelState* s); /* 00458650 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEVEL_H */
