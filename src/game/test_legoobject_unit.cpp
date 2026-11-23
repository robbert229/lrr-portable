// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_legoobject_unit — relaxed versus normal work eligibility,
 * and ordered teleport requests up to the queue limit. */
#include "game/lego_globs.h"
#include "game/legoobject_unit.h"
#include "test/lrr_test.h"

int main(void)
{
    Lego_Object raider = {};
    raider.health = 40.0f;
    CHECK_EQ(legoobject_shared_eligibility_check(&raider, 0), 0);
    CHECK_EQ(legoobject_shared_eligibility_check(&raider, 1), 1);
    raider.flags3 = 0x20000000u; // Ready for work.
    CHECK_EQ(legoobject_shared_eligibility_check(&raider, 0), 1);
    raider.flags3 |= 0x80000000u; // Activity suspended.
    CHECK_EQ(legoobject_shared_eligibility_check(&raider, 0), 0);
    CHECK_EQ(legoobject_shared_eligibility_check(&raider, 1), 1);
    raider.flags2 = 0x100000u;
    CHECK_EQ(legoobject_shared_eligibility_check(&raider, 1), 0);
    raider.flags2 = 0;
    raider.flags3 = 0x20000000u;
    raider.health = -1.0f;
    CHECK_EQ(legoobject_shared_eligibility_check(&raider, 0), 0);
    CHECK_EQ(legoobject_shared_eligibility_check(&raider, 1), 0);

    for (int i = 0; i < 9; ++i)
        CHECK_EQ(legoobject_spawn_request_queue_add(nullptr, i % 2 + 1, i), 1);
    CHECK_EQ(legoobject_spawn_request_queue_add(nullptr, 4, 12), 0);
    CHECK_EQ(legoGlobs.objectRequestCount, 9u);
    const LegoObjI_Requests& requests = legoGlobs_objectRequests;
    for (int i = 0; i < 9; ++i) {
        CHECK_EQ(requests.type[i], i % 2 + 1);
        CHECK_EQ(requests.id[i], i);
    }
    return lrr_test_result("legoobject_unit");
}
