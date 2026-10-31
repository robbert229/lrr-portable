// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GAME_ENTITY_QUAD_COLLISION_HPP
#define LEGORRR_GAME_ENTITY_QUAD_COLLISION_HPP

#include "entity.hpp"
#include "../gods98/view.hpp"
#include <math.h>

typedef struct EntityQuadCollisionHooks {
    void* context;
    int (*bounds_get)(void* context, EntitySlot* entity, float size_xz[2]);
    int (*object_to_world)(void* context, EntitySlot* entity,
                           const float local_xyz[3], float world_xyz[3]);
} EntityQuadCollisionHooks;

/* EntitySurfaceQuadSegmentIntersect (FUN_004702b0): transform the four
 * corners of the entity's XZ footprint and test each rectangle edge against
 * the infinite input line. The reference uses strict interior comparisons
 * on each edge, so corner-only touches do not count. */
static inline int entity_surface_quad_segment_intersect(
    EntitySlot* entity, const float start_xy[2], const float end_xy[2],
    float out_xy[2], int check_side, const EntityQuadCollisionHooks* hooks)
{
    float size[2];
    float local[12];
    float quad[8];
    float direction[2];
    unsigned int i;
    if (entity == NULL || start_xy == NULL || end_xy == NULL || hooks == NULL ||
        hooks->object_to_world == NULL) return 0;
    if (check_side != 0 &&
        entity_directed_line_side_check(entity, start_xy, end_xy)) return 0;
    if (hooks->bounds_get != NULL) {
        if (!hooks->bounds_get(hooks->context, entity, size)) return 0;
    }
    else {
        const float* stats_size = (const float*)entity_stats_field_at_a0(entity);
        if (stats_size == NULL) return 0;
        size[0] = stats_size[0];
        size[1] = stats_size[1];
    }
    local[0] = -size[0] * 0.5f; local[1] = 0.0f; local[2] = size[1] * 0.5f;
    local[3] =  size[0] * 0.5f; local[4] = 0.0f; local[5] = size[1] * 0.5f;
    local[6] =  size[0] * 0.5f; local[7] = 0.0f; local[8] =-size[1] * 0.5f;
    local[9] = -size[0] * 0.5f; local[10]= 0.0f; local[11]=-size[1] * 0.5f;
    for (i = 0; i < 4; ++i) {
        float world[3];
        if (!hooks->object_to_world(hooks->context, entity, local + i * 3,
                                    world)) return 0;
        quad[i * 2] = world[0];
        quad[i * 2 + 1] = world[2];
    }
    direction[0] = end_xy[0] - start_xy[0];
    direction[1] = end_xy[1] - start_xy[1];
    for (i = 0; i < 4; ++i) {
        const unsigned int next = (i + 1) & 3;
        float edge[2];
        float hit[2];
        float dx, dy;
        if (quad[i * 2] == quad[next * 2] &&
            quad[i * 2 + 1] == quad[next * 2 + 1]) continue;
        edge[0] = quad[next * 2] - quad[i * 2];
        edge[1] = quad[next * 2 + 1] - quad[i * 2 + 1];
        if (line_intersection_2d_compute(hit, start_xy, direction,
                                         quad + i * 2, edge) == NULL) continue;
        dx = quad[i * 2] - quad[next * 2];
        dy = quad[i * 2 + 1] - quad[next * 2 + 1];
        if (fabs(dx) <= fabs(dy)) {
            if (!((quad[i * 2 + 1] < hit[1] && hit[1] < quad[next * 2 + 1]) ||
                  (quad[next * 2 + 1] < hit[1] && hit[1] < quad[i * 2 + 1])))
                continue;
        }
        else if (!((quad[i * 2] < hit[0] && hit[0] < quad[next * 2]) ||
                   (quad[next * 2] < hit[0] && hit[0] < quad[i * 2]))) {
            continue;
        }
        if (out_xy != NULL) {
            out_xy[0] = hit[0];
            out_xy[1] = hit[1];
        }
        return 1;
    }
    return 0;
}

#endif
