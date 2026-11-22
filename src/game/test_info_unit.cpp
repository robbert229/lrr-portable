// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_info_unit — case-insensitive names, oldest-instance eviction,
 * early exit during enumeration, and removing an instance from the middle. */
#include "game/info_unit.h"
#include "test/lrr_test.h"

struct Search {
    InfoMessageInstance* target;
    unsigned int visited;
};

static int find_instance(InfoMessageInstance* instance, void* data)
{
    auto& search = *static_cast<Search*>(data);
    ++search.visited;
    return instance == search.target;
}

int main(void)
{
    info_initialise(nullptr);
    int type = -1;
    CHECK_EQ(info_get_type("info_genericmonster", &type), 1);
    CHECK_EQ(type, 39);
    CHECK_EQ(info_get_type("missing", &type), 0);
    CHECK_EQ(type, 39);

    constexpr unsigned int message = 0;
    InfoMessageInstance instances[10] = {};
    for (auto& instance : instances)
        info_message_add_instance(message, &instance);
    CHECK_EQ(info_message_get_instance(message, -1), nullptr);
    CHECK_EQ(info_message_get_instance(message, 0), &instances[1]);
    CHECK_EQ(info_message_get_instance(message, 8), &instances[9]);
    CHECK_EQ(info_message_get_instance(message, 9), nullptr);

    Search search = { &instances[4], 0 };
    CHECK_EQ(info_message_enumerate(message, find_instance, &search), 1);
    CHECK_EQ(search.visited, 4u);
    CHECK_EQ(info_message_remove_instance(message, 3), &instances[4]);
    CHECK_EQ(info_message_get_instance(message, 3), &instances[5]);
    search.visited = 0;
    CHECK_EQ(info_message_enumerate(message, find_instance, &search), 0);
    CHECK_EQ(search.visited, 8u);
    for (int i = 0; i < 8; ++i)
        CHECK(info_message_remove_instance(message, 0) != nullptr, "the remaining entry is removed");
    CHECK_EQ(info_message_remove_instance(message, 0), nullptr);
    return lrr_test_result("info_unit");
}
