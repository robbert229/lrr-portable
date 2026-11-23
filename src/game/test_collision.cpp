// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_collision — distance to each region of a segment, the closest
 * of several segments, and projection onto an unbounded line. */
#include "game/collision_link_names.h"
#include "game/collision.h"
#include "test/lrr_test.h"

int main(void)
{
    const CollisionPoint2F start = { 0.0f, 0.0f };
    const CollisionPoint2F end = { 8.0f, 0.0f };
    const CollisionPoint2F before = { -3.0f, 4.0f };
    const CollisionPoint2F inside = { 4.0f, 3.0f };
    const CollisionPoint2F after = { 11.0f, 4.0f };
    CHECK_NEAR(collision_segment_point_distance(&before, &start, &end), 5.0f, 1e-6);
    CHECK_NEAR(collision_segment_point_distance(&inside, &start, &end), 3.0f, 1e-6);
    CHECK_NEAR(collision_segment_point_distance(&after, &start, &end), 5.0f, 1e-6);

    const CollisionPoint2F first[] = { start, { 0.0f, 4.0f } };
    const CollisionPoint2F second[] = { end, { 8.0f, 4.0f } };
    CHECK_NEAR(collision_polyline_point_distance(&inside, first, second, 2u), 1.0f, 1e-6);

    CollisionPoint2F projected = {};
    collision_project_onto_line(&start, &end, &after, &projected);
    CHECK_NEAR(projected.x, 11.0f, 1e-6);
    CHECK_NEAR(projected.y, 0.0f, 1e-6);

    const CollisionPoint2F origin = { 2.0f, 2.0f };
    const CollisionPoint2F direction = { 0.0f, 4.0f };
    const CollisionPoint2F point = { -2.0f, -3.0f };
    collision_project_onto_direction(&origin, &direction, &point, &projected);
    CHECK_NEAR(projected.x, 2.0f, 1e-6);
    CHECK_NEAR(projected.y, -3.0f, 1e-6);
    return lrr_test_result("collision");
}
