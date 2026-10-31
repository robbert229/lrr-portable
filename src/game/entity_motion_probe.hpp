// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GAME_ENTITY_MOTION_PROBE_HPP
#define LEGORRR_GAME_ENTITY_MOTION_PROBE_HPP

#include "entity.hpp"
#include <math.h>

typedef struct EntityMotionSurfaceProbeHooks {
    void* context;
    int (*current_cell_get)(void* context, EntitySlot* entity, int cell_xy[2]);
    int (*world_position_get)(void* context, EntitySlot* entity, float xy[2]);
    int (*movement_vector_get)(void* context, EntitySlot* entity, float xy[2]);
    float (*delta_get)(void* context);
    int (*world_to_cell)(void* context, const float xy[2], int cell_xy[2]);
    int (*cell_invalid)(void* context, int x, int y);
    int (*cell_reject)(void* context, int x, int y, int mode);
} EntityMotionSurfaceProbeHooks;

/* EntityMoveVectorSurfaceProbe (FUN_004455a0): predict one third of the
 * movement tick along the entity's normalized motion vector and report
 * whether the projected cell is invalid or blocks movement. */
static inline int entity_move_vector_surface_probe(
    EntitySlot* entity, const EntityMotionSurfaceProbeHooks* hooks)
{
    int current[2];
    int candidate_cell[2];
    float position[2];
    float vector[2];
    float length;
    float step;
    if (entity == NULL || hooks == NULL || hooks->current_cell_get == NULL ||
        hooks->world_position_get == NULL || hooks->movement_vector_get == NULL ||
        hooks->delta_get == NULL || hooks->world_to_cell == NULL ||
        hooks->cell_invalid == NULL || hooks->cell_reject == NULL ||
        !hooks->current_cell_get(hooks->context, entity, current) ||
        hooks->cell_invalid(hooks->context, current[0], current[1]) ||
        !hooks->world_position_get(hooks->context, entity, position) ||
        !hooks->movement_vector_get(hooks->context, entity, vector)) return 0;
    length = (float)sqrt(vector[0] * vector[0] + vector[1] * vector[1]);
    if (length == 0.0f) return 0;
    step = hooks->delta_get(hooks->context) * (1.0f / 3.0f) / length;
    position[0] += vector[0] * step;
    position[1] += vector[1] * step;
    if (!hooks->world_to_cell(hooks->context, position, candidate_cell)) return 1;
    if (hooks->cell_invalid(hooks->context, candidate_cell[0],
                            candidate_cell[1])) return 1;
    return hooks->cell_reject(hooks->context, candidate_cell[0],
                              candidate_cell[1], 0) != 0;
}

#endif
