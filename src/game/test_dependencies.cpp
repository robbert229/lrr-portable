// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_dependencies — parsing a synthetic dependency entry, checking the
 * required object type, ID and level, and retaining a HitOnceStay match. */
#include "game/dependencies_link_names.h"
#include "game/dependencies.h"
#include "game/legoobject_record.h"
#include "gods98/config.h"
#include "test/lrr_test.h"

static Lego_Object* available;

extern "C" BlockRec* __wrap_block_subgroup(BlockFile* root, const char*)
{
    return root;
}

extern "C" int __wrap_lego_entity_type_pair_lookup(const char* name, int* type,
                                                    unsigned int* id, void**)
{
    if (std::strcmp(name, "Target") == 0) {
        *type = 4;
        *id = 1;
    } else if (std::strcmp(name, "Prerequisite") == 0) {
        *type = 4;
        *id = 2;
    } else {
        return 0;
    }
    return 1;
}

extern "C" uint32_t __wrap_building_upgrade_admission_check(int, int)
{
    return 3;
}

extern "C" int __wrap_legoobject_run_through_lists_skip_upgrade_parts(
    int (*callback)(Lego_Object*, void*), void* data)
{
    return available != nullptr && callback(available, data);
}

extern "C" int __wrap_legoobject_task_kind_filtered_search(int)
{
    return 0;
}

int main(void)
{
    char key[] = "HitOnceStay:Target", value[] = "Prerequisite:2";
    BlockRec end = {};
    BlockRec entry = { nullptr, key, value, 1u, 0u, &end, nullptr, nullptr };
    dependencies_initialise(&entry, "Game");
    Dependencies_Requirement* requirements = nullptr;
    unsigned int count = 0;
    dependencies_object_get_requirements(4, 1u, 0u, &requirements, &count);
    CHECK_EQ(count, 1u);
    CHECK(requirements != nullptr, "the loaded requirement is accessible");
    CHECK_EQ(dependencies_object_is_unlocked(4, 1u, 0u), 0u);
    CHECK_EQ(dependencies_object_requirements_met(4, 1u, 0u), 0);

    dependencies_set_enabled(0);
    CHECK_EQ(dependencies_object_requirements_met(4, 1u, 0u), 1);
    dependencies_set_enabled(1);
    CHECK_EQ(dependencies_object_requirements_met(4, 1u, 0u), 0);

    Lego_Object candidate = {};
    available = &candidate;
    candidate.type = 4;
    candidate.id = 3;
    candidate.objLevel = 2;
    CHECK_EQ(dependencies_object_requirements_met(4, 1u, 0u), 0);
    candidate.id = 2;
    candidate.objLevel = 1;
    CHECK_EQ(dependencies_object_requirements_met(4, 1u, 0u), 0);
    candidate.objLevel = 2;
    CHECK_EQ(dependencies_object_requirements_met(4, 1u, 0u), 1);
    available = nullptr;
    CHECK_EQ(dependencies_object_requirements_met(4, 1u, 0u), 1);
    dependencies_update();
    CHECK(dependencies_object_is_unlocked(4, 1u, 0u) != 0u, "the new unlock is visible");

    dependencies_object_add_level_flag_100(4, 1u, 0u);
    CHECK_EQ(dependencies_object_get_level_flag_100(4, 1u, 0u), 0x100u);
    dependencies_reset_clear_all_level_flags_10c();
    CHECK_EQ(dependencies_object_get_level_flag_100(4, 1u, 0u), 0u);
    CHECK_EQ(dependencies_object_is_unlocked(4, 1u, 0u), 0u);
    CHECK_EQ(dependencies_object_requirements_met(4, 1u, 0u), 1);
    return lrr_test_result("dependencies");
}
