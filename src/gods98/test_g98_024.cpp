// SPDX-License-Identifier: LGPL-3.0-or-later
/* g98_024.c — exhaust the level-map handle table, reuse a released slot,
 * and check that disjoint slots retain their own data. */
#include "gods98/g98_024.h"
#include "test/lrr_test.h"

int main()
{
    constexpr unsigned int capacity = 2000;
    constexpr unsigned int reused = 47;
    level_map_slots_reset();
    CHECK_EQ(level_map_slot_get(0), nullptr);
    CHECK_EQ(level_map_slot_get(capacity), nullptr);

    for (unsigned int i = 0; i < capacity; ++i) {
        const int slot = level_map_slot_allocate(4);
        if (!CHECK_EQ(slot, static_cast<int>(i))) break;
        auto* data = level_map_slot_get(i);
        if (CHECK(data != nullptr, "allocated slot has storage")) data[0] = i % 251;
    }
    CHECK_EQ(level_map_slot_allocate(4), -1);

    level_map_slot_release(reused);
    CHECK_EQ(level_map_slot_get(reused), nullptr);
    level_map_slot_release(reused);
    level_map_slot_release(capacity);
    CHECK_EQ(level_map_slot_allocate(8), static_cast<int>(reused));
    auto* data = level_map_slot_get(reused);
    if (CHECK(data != nullptr, "reused slot has new storage")) data[7] = 0x5a;
    for (unsigned int i = 0; i < capacity; ++i) {
        auto* current = level_map_slot_get(i);
        if (CHECK(current != nullptr, "occupied slot remains accessible")) {
            if (i == reused) CHECK_EQ(current[7], 0x5au);
            else CHECK_EQ(current[0], i % 251);
        }
        level_map_slot_release(i);
    }
    CHECK_EQ(level_map_slot_get(0), nullptr);
    CHECK_EQ(level_map_slot_get(capacity - 1), nullptr);
    return lrr_test_result();
}
