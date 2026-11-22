// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_pointer — case-insensitive pointer names and timed overrides,
 * including replacement, expiry at exactly zero and a tick past expiry. */
#include "game/pointer_link_names.h"
#include "game/pointer.h"
#include "test/lrr_test.h"

int main(void)
{
    pointer_initialise();
    int standard = -1;
    int drill = -1;
    int blocked = -1;
    CHECK_EQ(pointer_get_type("pointer_standard", &standard), 1);
    CHECK_EQ(standard, 0);
    CHECK_EQ(pointer_get_type("pOiNtEr_DrIlL", &drill), 1);
    CHECK_EQ(drill, 3);
    CHECK_EQ(pointer_get_type("Pointer_CantDrill", &blocked), 1);
    CHECK_EQ(blocked, 4);
    CHECK_EQ(pointer_get_type("Pointer_DoesNotExist", &blocked), 0);
    CHECK_EQ(blocked, 4);

    pointer_set_current(drill, 2.0f);
    pointer_set_current_if_timer_finished(standard);
    CHECK_EQ(pointer_get_current_type(), drill);
    pointer_update(0.5f);
    pointer_set_current_if_timer_finished(standard);
    CHECK_EQ(pointer_get_current_type(), drill);
    pointer_update(1.5f);
    CHECK_EQ(pointer_get_current_type(), drill);
    pointer_set_current_if_timer_finished(standard);
    CHECK_EQ(pointer_get_current_type(), standard);

    pointer_set_current(drill, 4.0f);
    pointer_set_current(blocked, 1.0f);
    pointer_set_current_if_timer_finished(standard);
    CHECK_EQ(pointer_get_current_type(), blocked);
    pointer_update(2.0f);
    pointer_set_current_if_timer_finished(standard);
    CHECK_EQ(pointer_get_current_type(), standard);
    return lrr_test_result();
}
