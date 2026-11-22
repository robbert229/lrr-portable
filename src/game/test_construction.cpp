// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_construction — zone lookup, resource matching, readiness on the
 * last delivery, and removal from the zone list. */
#include "game/construction_link_names.h"
#include "game/construction.h"
#include "game/legoobject_record.h"
#include "test/lrr_test.h"

int main(void)
{
    int origin[] = { 2, 4 };
    int other[] = { 4, 2 };
    Lego_ConstructionZone* zone = construction_zone_create(origin);
    Lego_ConstructionZone* neighbor = construction_zone_create(other);
    CHECK(zone != neighbor, "each construction site has its own zone");
    CHECK(zone->handle != neighbor->handle, "zone handles distinguish sites");
    CHECK_EQ(construction_zone_find_by_handle_or_at_block(nullptr, &zone->handle), zone);
    CHECK_EQ(construction_zone_find_by_handle_or_at_block(other, nullptr), neighbor);

    Lego_Object crystal = {};
    crystal.type = 6; // Power crystal.
    Lego_Object ore = {};
    ore.type = 7;
    ore.id = 1; // Stud.
    zone->requestCount = 2;
    construction_zone_place_resource(zone->handle, &crystal);
    CHECK_EQ(zone->placedCount, 1u);
    CHECK_EQ(zone->flags & CONSTRUCTION_FLAG_READY, 0u);
    construction_zone_place_resource(zone->handle, &ore);
    CHECK_EQ(zone->placedCount, 2u);
    CHECK_EQ(zone->progress, 125.0f);
    CHECK((zone->flags & CONSTRUCTION_FLAG_READY) != 0u, "the last delivery readies the site");
    CHECK_EQ(construction_zone_count_of_resource_placed(zone, crystal.type, 0), 1u);
    CHECK_EQ(construction_zone_count_of_resource_placed(zone, ore.type, 1), 1u);
    CHECK_EQ(construction_zone_count_of_resource_placed(zone, ore.type, 0), 0u);
    CHECK((ore.flags4 & 0x100u) != 0u, "the stud is marked as placed");

    construction_zone_free(zone);
    CHECK_EQ(construction_zone_exists_at_block(origin), 0);
    CHECK_EQ(construction_zone_exists_at_block(other), 1);
    construction_remove_all();
    CHECK_EQ(construction_zone_exists_at_block(other), 0);
    return lrr_test_result("construction");
}
