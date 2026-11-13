// SPDX-License-Identifier: LGPL-3.0-or-later
/* Collision.c, original translation unit GAME-007 (finding 603), rebuilt in
 * original shape (ADR 0005): text 004089d0-00408c80, .rdata
 * 0049e088-0049e08c.
 *
 * Four 2D helpers: the distance from a point to a segment and to a
 * polyline, and the projection of a point onto a line and onto a direction.
 * LegoObject.c measures a mini-figure's distance to a building's shape with
 * the polyline distance; Weapons.c projects targets onto a laser's line.
 *
 * The port's implementation stays in the port tail placement_port.cpp,
 * unannotated. In the matching build collision_link_names.h gives three of
 * these functions the port's names, so every caller reaches them; natively
 * the port tail serves those names and nothing calls these. */
#include "collision_link_names.h"

#include "collision.h"

#include <math.h>

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

#include "collision_maths.h"

// FUNCTION: LEGORR 0x004089d0
float collision_segment_point_distance(const CollisionPoint2F* point,
                                       const CollisionPoint2F* start,
                                       const CollisionPoint2F* end)
{
    CollisionPoint2F dir;
    CollisionPoint2F toEnd;
    CollisionPoint2F toStart;
    float dist;
    float dot;

    collision_vector2d_subtract(&dir, end, start);
    collision_vector2d_subtract(&toEnd, point, end);
    dist = collision_vector2d_modulus(&toEnd);
    collision_vector2d_normalise(&dir);
    collision_vector2d_scale(&toEnd, &toEnd, 1.0f / dist);
    if (collision_vector2d_dot(&toEnd, &dir) < 0.0f) {
        collision_vector2d_subtract(&toStart, point, start);
        dist = collision_vector2d_modulus(&toStart);
        collision_vector2d_scale(&toStart, &toStart, 1.0f / dist);
        dot = collision_vector2d_dot(&toStart, &dir);
        if (dot <= 0.0f)
            return dist;
        return (float)sin(acos(dot)) * dist;
    }
    return dist;
}

// FUNCTION: LEGORR 0x00408b00
float collision_polyline_point_distance(const CollisionPoint2F* point,
                                        const CollisionPoint2F* first,
                                        const CollisionPoint2F* second,
                                        unsigned int count)
{
    float best = collision_segment_point_distance(point, first, second);
    float dist;
    unsigned int i;

    for (i = 1; i < count; i++) {
        dist = collision_segment_point_distance(point, &first[i], &second[i]);
        if (dist < best)
            best = dist;
    }
    return best;
}

// FUNCTION: LEGORR 0x00408b60
void collision_project_onto_line(const CollisionPoint2F* start,
                                 const CollisionPoint2F* end,
                                 const CollisionPoint2F* point,
                                 CollisionPoint2F* out)
{
    CollisionPoint2F dir;
    CollisionPoint2F rel;
    float t;

    collision_vector2d_subtract(&dir, end, start);
    collision_vector2d_normalise(&dir);
    collision_vector2d_subtract(&rel, point, start);
    t = collision_vector2d_dot(&rel, &dir);
    collision_ray_end_point(out, start, &dir, t);
}

// FUNCTION: LEGORR 0x00408bf0
void collision_project_onto_direction(const CollisionPoint2F* origin,
                                      const CollisionPoint2F* direction,
                                      const CollisionPoint2F* point,
                                      CollisionPoint2F* out)
{
    CollisionPoint2F dir = *direction;
    CollisionPoint2F rel;
    float t;

    collision_vector2d_normalise(&dir);
    collision_vector2d_subtract(&rel, point, origin);
    t = collision_vector2d_dot(&rel, &dir);
    out->x = dir.x * t + origin->x;
    out->y = dir.y * t + origin->y;
}

#ifdef __cplusplus
}
#endif
