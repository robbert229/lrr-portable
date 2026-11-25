// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_tutorial_script — independent NERPs registers and timers, and
 * resolving only zero-argument calls when decoding a script argument. */
#include "game/tutorial_script.h"
#include "game/level.h"
#include "test/lrr_test.h"

extern "C" {
int nerpfunc_set_r0(unsigned int* stack);
int nerpfunc_get_r0(unsigned int* stack);
int nerpfunc_get_r1(unsigned int* stack);
int nerpfunc_add_r0(unsigned int* stack);
int nerpfunc_sub_r0(unsigned int* stack);
int nerpfunc_set_timer0(unsigned int* stack);
int nerpfunc_set_timer3(unsigned int* stack);
int nerpfunc_get_timer0(unsigned int* stack);
int nerpfunc_get_timer3(unsigned int* stack);
void nerps_runtime_update_timers(float elapsed);
void nerps_runtime_resolve_argument(unsigned int* value);
}

int main(void)
{
    unsigned int arg = 12;
    CHECK_EQ(nerpfunc_set_r0(&arg), 12);
    arg = 3;
    CHECK_EQ(nerpfunc_add_r0(&arg), 15);
    arg = 7;
    CHECK_EQ(nerpfunc_sub_r0(&arg), 8);
    CHECK_EQ(nerpfunc_get_r0(nullptr), 8);
    CHECK_EQ(nerpfunc_get_r1(nullptr), 0);

    CHECK_EQ(nerpfunc_set_timer0(&arg), 1);
    arg = 32;
    CHECK_EQ(nerpfunc_set_timer3(&arg), 1);
    nerps_runtime_update_timers(0.5f);
    CHECK_EQ(nerpfunc_get_timer0(nullptr), 27);
    CHECK_EQ(nerpfunc_get_timer3(nullptr), 52);

    arg = 0x20001; // True
    nerps_runtime_resolve_argument(&arg);
    CHECK_EQ(arg, 1u);
    arg = 0x20000; // Stop is a statement, so it is not evaluated as an argument.
    nerps_runtime_resolve_argument(&arg);
    CHECK_EQ(arg, 0x20000u);
    arg = 32;
    nerps_runtime_resolve_argument(&arg);
    CHECK_EQ(arg, 32u);
    // Unknown names and non-countable catalog entries must not index the
    // fixed-size object counter table. No catalog or scene boot is needed.
    CHECK_EQ(entity_type_grid_record_count_get("MissingSyntheticType", 0), 0u);
    CHECK_EQ(entity_type_grid_record_count_get("TVcamera", 0), 0u);
    CHECK_EQ(entity_type_grid_record_count_get("Ore", -1), 0u);
    CHECK_EQ(entity_type_grid_record_count_get("Ore", 16), 0u);
    CHECK_EQ(entity_type_grid_record_count_get("Ore", 0), 0u);
    return lrr_test_result();
}
