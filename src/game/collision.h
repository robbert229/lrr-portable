// SPDX-License-Identifier: LGPL-3.0-or-later
/* Collision.c (original translation unit GAME-007, finding 603): 2D
 * distance and projection helpers, by the unit's own names.
 *
 * C-compatible header (ADR 0005). collision.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * includes it.
 *
 * In the matching build collision_link_names.h, which collision.c includes
 * first, gives every one of these functions the port's name (view.hpp,
 * C linkage), so LegoObject.c and Weapons.c reach them. Natively the port
 * tail placement_port.cpp serves the port's names and nothing calls these. */
#ifndef LEGORRR_GAME_COLLISION_H
#define LEGORRR_GAME_COLLISION_H

#ifdef __cplusplus
extern "C" {
#endif

/* Maths.h's Point2F. */
typedef struct CollisionPoint2F {
    float x, y;
} CollisionPoint2F;

float collision_segment_point_distance(const CollisionPoint2F* point,
                                       const CollisionPoint2F* start,
                                       const CollisionPoint2F* end);  /* 004089d0 */
float collision_polyline_point_distance(const CollisionPoint2F* point,
                                        const CollisionPoint2F* first,
                                        const CollisionPoint2F* second,
                                        unsigned int count);         /* 00408b00 */
void collision_project_onto_line(const CollisionPoint2F* start,
                                 const CollisionPoint2F* end,
                                 const CollisionPoint2F* point,
                                 CollisionPoint2F* out);             /* 00408b60 */
void collision_project_onto_direction(const CollisionPoint2F* origin,
                                      const CollisionPoint2F* direction,
                                      const CollisionPoint2F* point,
                                      CollisionPoint2F* out);        /* 00408bf0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_COLLISION_H */
