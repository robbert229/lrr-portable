// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-023 -- original translation unit (text 00436c20-0044c710; finding
 * 505): the original's LegoObject.c, the object manager (the object pool,
 * object creation and removal, the per-object activity and AI updates).
 *
 * Rebuilt in the original's shape (ADR 0005) in three parts by address. This
 * file holds part 1 (issue #575): the functions 00436c20-0043a960, in
 * original order, in the C-compatible subset, so the matching build compiles
 * it as C. The native port compiles it as C++. Parts 2 and 3 move the rest
 * of the unit here from legoobject.cpp.
 *
 * The port's own model of LegoObject.c (EntitySlot, the GameContext and the
 * hook tables) is a different contract. Its implementations stay,
 * unannotated and under the port's names, in legoobject.cpp, which becomes
 * the unit's port tail once parts 2 and 3 have drained it. Native callers
 * keep using them. The functions here that keep a port name are the ones
 * whose callers already use the original's contract.
 *
 * The original keeps its state in one LegoObject_Globs struct (objectGlobs,
 * 004de790-004eadd0). The port models its fields as separate globals in
 * legoobject.cpp, which this unit reads with C linkage
 * (legoobject_unit_imports.h). Fields the port does not model are defined
 * here, one annotated global each. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "legoobject_unit.h"
#include "legoobject_unit_imports.h"

/* ------------------------------------------------------------------ */
/* State: objectGlobs fields the port does not model, and legoGlobs     */
/* stand-ins.                                                           */
/* ------------------------------------------------------------------ */

/* ---- state, group A ---- */

/* objectGlobs.queuedAnimationFraction (004ea7a8): the whole units of queued
 * grid animation work (legoobject_process_queued_grid_animations). */
// GLOBAL: LEGORR 0x004ea7a8
static float objectGlobs_queuedAnimationFraction;
/* objectGlobs.ToolTipIcon_Ore (004eadc0). The other tool tip icons
 * (004ead78-004eadbc) are lego_unit.c's. */
// GLOBAL: LEGORR 0x004eadc0
static struct Image* objectGlobs_toolTipIconOre;


/* ---- state, group B ---- */


/* ---- state, group C ---- */


/* ---- state, group D ---- */


/* ---- state, group E ---- */


/* ---- state, group F ---- */


/* ---- state, group G ---- */


/* ------------------------------------------------------------------ */
/* Functions, in original order.                                       */
/* ------------------------------------------------------------------ */

/* ---- functions, group A (00436c20-00437510) ---- */

/* LegoObject_Initialise: clear objectGlobs (004de790, 0xc63c bytes, the
 * port's separate globals in legoobject.cpp, at their original addresses in
 * the matching build), then install the activity, ability and tool names. */
// FUNCTION: LEGORR 0x00436c20
void legoobject_initialise(void)
{
    memset(s_pool_chunks, 0, 0xc63c);
    s_activity[0] = "Activity_Stand";
    // STRING: LEGORR 0x004a5038
    s_activity[1] = "Activity_Route";
    // STRING: LEGORR 0x004a5020
    s_activity[2] = "Activity_RouteRubble";
    // STRING: LEGORR 0x004a500c
    s_activity[3] = "Activity_RunPanic";
    // STRING: LEGORR 0x004a4ffc
    s_activity[4] = "Activity_Drill";
    // STRING: LEGORR 0x004a4fe8
    s_activity[5] = "Activity_Teleport";
    // STRING: LEGORR 0x004a4fd8
    s_activity[6] = "Activity_Walk";
    // STRING: LEGORR 0x004a4fc4
    s_activity[7] = "Activity_Reinforce";
    // STRING: LEGORR 0x004a4fb0
    s_activity[8] = "Activity_Reverse";
    // STRING: LEGORR 0x004a4f9c
    s_activity[9] = "Activity_TurnLeft";
    // STRING: LEGORR 0x004a4f88
    s_activity[10] = "Activity_TurnRight";
    // STRING: LEGORR 0x004a4f78
    s_activity[11] = "Activity_CantDo";
    // STRING: LEGORR 0x004a4f68
    s_activity[12] = "Activity_Emerge";
    // STRING: LEGORR 0x004a4f58
    s_activity[13] = "Activity_Enter";
    // STRING: LEGORR 0x004a4f44
    s_activity[14] = "Activity_EnterRein";
    // STRING: LEGORR 0x004a4f30
    s_activity[15] = "Activity_Collect";
    // STRING: LEGORR 0x004a4f20
    s_activity[16] = "Activity_Gather";
    // STRING: LEGORR 0x004a4f10
    s_activity[17] = "Activity_Carry";
    // STRING: LEGORR 0x004a4ef8
    s_activity[18] = "Activity_CarryRubble";
    // STRING: LEGORR 0x004a4ee8
    s_activity[19] = "Activity_Throw";
    // STRING: LEGORR 0x004a4ed0
    s_activity[20] = "Activity_CarryTurnLeft";
    // STRING: LEGORR 0x004a4eb8
    s_activity[21] = "Activity_CarryTurnRight";
    // STRING: LEGORR 0x004a4ea4
    s_activity[22] = "Activity_CarryStand";
    // STRING: LEGORR 0x004a4e90
    s_activity[23] = "Activity_HitLeft";
    // STRING: LEGORR 0x004a4e7c
    s_activity[24] = "Activity_HitRight";
    // STRING: LEGORR 0x004a4e68
    s_activity[25] = "Activity_HitFront";
    // STRING: LEGORR 0x004a4e54
    s_activity[26] = "Activity_HitBack";
    // STRING: LEGORR 0x004a4e40
    s_activity[27] = "Activity_HitHard";
    // STRING: LEGORR 0x004a4e2c
    s_activity[28] = "Activity_Dynamite";
    // STRING: LEGORR 0x004a4e18
    s_activity[29] = "Activity_Deposit";
    // STRING: LEGORR 0x004a4e08
    s_activity[30] = "Activity_Clear";
    // STRING: LEGORR 0x004a4df8
    s_activity[31] = "Activity_Place";
    // STRING: LEGORR 0x004a4de8
    s_activity[32] = "Activity_Repair";
    // STRING: LEGORR 0x004a4dd8
    s_activity[33] = "Activity_Slip";
    // STRING: LEGORR 0x004a4dc8
    s_activity[34] = "Activity_Rest";
    // STRING: LEGORR 0x004a4db8
    s_activity[35] = "Activity_Eat";
    // STRING: LEGORR 0x004a4da8
    s_activity[36] = "Activity_Stamp";
    // STRING: LEGORR 0x004a4d94
    s_activity[37] = "Activity_ThrowMan";
    // STRING: LEGORR 0x004a4d74
    s_activity[38] = "Activity_ThrownByRockMonster";
    // STRING: LEGORR 0x004a4d64
    s_activity[39] = "Activity_GetUp";
    // STRING: LEGORR 0x004a4d50
    s_activity[40] = "Activity_BuildPath";
    // STRING: LEGORR 0x004a4d3c
    s_activity[41] = "Activity_Upgrade";
    // STRING: LEGORR 0x004a4d28
    s_activity[42] = "Activity_Explode";
    // STRING: LEGORR 0x004a4d14
    s_activity[43] = "Activity_Unpowered";
    // STRING: LEGORR 0x004a4d00
    s_activity[44] = "Activity_FireLaser";
    // STRING: LEGORR 0x004a4cec
    s_activity[45] = "Activity_Freezed";
    // STRING: LEGORR 0x004a4cd4
    s_activity[46] = "Activity_FreezeStart";
    // STRING: LEGORR 0x004a4cc0
    s_activity[47] = "Activity_FreezeMelt";
    // STRING: LEGORR 0x004a4cac
    s_activity[48] = "Activity_Recharge";
    // STRING: LEGORR 0x004a4c9c
    s_activity[49] = "Activity_WakeUp";
    // STRING: LEGORR 0x004a4c8c
    s_activity[50] = "Activity_Train";
    // STRING: LEGORR 0x004a4c78
    s_activity[51] = "Activity_FloatOn";
    // STRING: LEGORR 0x004a4c64
    s_activity[52] = "Activity_FloatOff";
    // STRING: LEGORR 0x004a4c50
    s_activity[53] = "Activity_Opening";
    // STRING: LEGORR 0x004a4c3c
    s_activity[54] = "Activity_Closing";
    // STRING: LEGORR 0x004a4c2c
    s_activity[55] = "Activity_Open";
    // STRING: LEGORR 0x004a4c18
    s_activity[56] = "Activity_Waiting1";
    // STRING: LEGORR 0x004a4c04
    s_activity[57] = "Activity_Waiting2";
    // STRING: LEGORR 0x004a4bf0
    s_activity[58] = "Activity_Waiting3";
    // STRING: LEGORR 0x004a4bdc
    s_activity[59] = "Activity_Waiting4";
    // STRING: LEGORR 0x004a4bc8
    s_activity[60] = "Activity_Waiting5";
    // STRING: LEGORR 0x004a4bb4
    s_activity[61] = "Activity_Waiting6";
    // STRING: LEGORR 0x004a4ba0
    s_activity[62] = "Activity_Waiting7";
    // STRING: LEGORR 0x004a4b8c
    s_activity[63] = "Activity_Waiting8";
    // STRING: LEGORR 0x004a4b78
    s_activity[64] = "Activity_Waiting9";
    // STRING: LEGORR 0x004a4b64
    s_activity[65] = "Activity_Waiting10";
    // STRING: LEGORR 0x004a4b50
    s_activity[66] = "Activity_Waiting11";
    // STRING: LEGORR 0x004a4b3c
    s_activity[67] = "Activity_Waiting12";
    // STRING: LEGORR 0x004a4b28
    s_activity[68] = "Activity_Waiting13";
    // STRING: LEGORR 0x004a4b14
    s_activity[69] = "Activity_Waiting14";
    // STRING: LEGORR 0x004a4b00
    s_activity[70] = "Activity_Waiting15";
    // STRING: LEGORR 0x004a4aec
    s_activity[71] = "Activity_Waiting16";
    // STRING: LEGORR 0x004a4ad8
    s_activity[72] = "Activity_Waiting17";
    // STRING: LEGORR 0x004a4ac4
    s_activity[73] = "Activity_Waiting18";
    // STRING: LEGORR 0x004a4ab0
    s_activity[74] = "Activity_Waiting19";
    // STRING: LEGORR 0x004a4a9c
    s_activity[75] = "Activity_Waiting20";
    // STRING: LEGORR 0x004a4a88
    s_activity[76] = "Activity_Waiting21";
    // STRING: LEGORR 0x004a4a74
    s_activity[77] = "Activity_Crumble";
    // STRING: LEGORR 0x004a4a60
    s_activity[78] = "Activity_TeleportIn";
    // STRING: LEGORR 0x004a4a40
    s_ability_type_names[0] = "LegoObject_AbilityType_Pilot";
    // STRING: LEGORR 0x004a4a20
    s_ability_type_names[1] = "LegoObject_AbilityType_Sailor";
    // STRING: LEGORR 0x004a4a00
    s_ability_type_names[2] = "LegoObject_AbilityType_Driver";
    // STRING: LEGORR 0x004a49e0
    s_ability_type_names[3] = "LegoObject_AbilityType_Dynamite";
    // STRING: LEGORR 0x004a49c0
    s_ability_type_names[4] = "LegoObject_AbilityType_Repair";
    // STRING: LEGORR 0x004a49a0
    s_ability_type_names[5] = "LegoObject_AbilityType_Scanner";
    memset(s_pool_chunks, 0, sizeof(s_pool_chunks));
    // STRING: LEGORR 0x004a4984
    s_tool_type_names[0] = "LegoObject_ToolType_Drill";
    // STRING: LEGORR 0x004a4968
    s_tool_type_names[1] = "LegoObject_ToolType_Spade";
    // STRING: LEGORR 0x004a494c
    s_tool_type_names[2] = "LegoObject_ToolType_Hammer";
    // STRING: LEGORR 0x004a4930
    s_tool_type_names[3] = "LegoObject_ToolType_Spanner";
    // STRING: LEGORR 0x004a4914
    s_tool_type_names[4] = "LegoObject_ToolType_Laser";
    // STRING: LEGORR 0x004a48f4
    s_tool_type_names[5] = "LegoObject_ToolType_PusherGun";
    // STRING: LEGORR 0x004a48d4
    s_tool_type_names[6] = "LegoObject_ToolType_BirdScarer";
    // STRING: LEGORR 0x004a48b4
    s_tool_type_names[7] = "LegoObject_ToolType_FreezerGun";

    s_activity_resource_slots[0] = 0;
    s_activity_resource_slots[1] = 0;
    s_activity_resource_slots[2] = 1;
    s_activity_resource_slots[3] = 1;
    s_activity_resource_slots[8] = 3;
    s_activity_resource_slots[9] = 2;
    s_activity_resource_slots[10] = 4;
    s_activity_resource_slots[7] = 4;
    s_activity_resource_slots[4] = 4;
    s_activity_resource_slots[5] = 4;
    s_activity_resource_slots[6] = 4;

    s_pool_record_head = NULL;
    s_pool_chunk_count = 0;
    s_entity_state_flags = 1;
}

/* LegoObject_Shutdown. */
// FUNCTION: LEGORR 0x00437050
void legoobject_shutdown(void)
{
    unsigned int i;

    entity_iteration_callback_guard();
    for (i = 0; i < 32; i++) {
        if (s_pool_chunks[i] != NULL) free(s_pool_chunks[i]);
    }
    s_pool_record_head = NULL;
    s_entity_state_flags = 0;
    if (s_terrain_neighbor_path_cost != NULL) free(s_terrain_neighbor_path_cost);
    s_entity_state_flags &= ~1;
}

// FUNCTION: LEGORR 0x004370b0
void legoobject_render_config_copy_out(unsigned int* out)
{
    memcpy(out, s_render_config_snapshot, sizeof(s_render_config_snapshot));
}

// FUNCTION: LEGORR 0x004370d0
void legoobject_render_config_copy_in_or_clear(unsigned int* in)
{
    if (in != NULL) memcpy(s_render_config_snapshot, in, sizeof(s_render_config_snapshot));
    else memset(s_render_config_snapshot, 0, sizeof(s_render_config_snapshot));
}

// FUNCTION: LEGORR 0x00437100
int legoobject_grid_record_count_difference_sum(int type, int skip)
{
    int total = 0;
    unsigned int row, slot;

    for (row = 0; row < 15; row++) {
        for (slot = 0; slot < 16; slot++) {
            if (!skip || !(stats_table_word_at_144(type, row) & 8)) {
                total += legoobject_grid_record_count_difference_get(type, row, slot, 1);
            }
        }
    }
    return total;
}

/* LegoObject_LoadToolTipIcons. */
// FUNCTION: LEGORR 0x00437150
void legoobject_tool_tip_icons_load(void* config)
{
    struct BlockRec* item;
    unsigned int i;

    // STRING: LEGORR 0x004a5050
    for (item = block_subgroup((struct BlockRec*)config, string_path_join(s_engine_config_root, "ToolTipIcons", NULL));
         item != NULL; item = block_next_equal_key(item)) {
        for (i = 0; i < 11; i++) {
            if (s_tool_type_names[i] != NULL && _strcmpi(item->key, s_tool_type_names[i]) == 0) {
                s_object_toolTipIcons[i] = image_load_bmp_scaled(item->value, 0, 0);
                break;
            }
        }
        if (i == 11) {
            for (i = 0; i < 6; i++) {
                if (s_ability_type_names[i] != NULL && _strcmpi(item->key, s_ability_type_names[i]) == 0) {
                    s_object_toolTipIconAbilities[i] = image_load_bmp_scaled(item->value, 0, 0);
                    break;
                }
            }
        }
        if (i == 6) {
            // STRING: LEGORR 0x004a5048
            if (_strcmpi(item->key, "Blank") == 0) {
                s_object_toolTipIconBlank = image_load_bmp_scaled(item->value, 0, 0);
            } else if (_strcmpi(item->key, "Ore") == 0) {
                objectGlobs_toolTipIconOre = image_load_bmp_scaled(item->value, 0, 0);
            }
        }
    }

    if (objectGlobs_toolTipIconOre != NULL) {
        image_surface_blit_transformed((struct TextureSurface*)objectGlobs_toolTipIconOre,
                                       0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    }
}

// FUNCTION: LEGORR 0x004372a0
void legoobject_spawn_coordinate_pool_clear(void)
{
    entity_iteration_callback_guard();
    legoobject_grid_record_count_tables_clear();
    legoobject_grid_object_records_release();
    memset(s_predug_coordinates, 0, sizeof(s_predug_coordinates));
    memset(s_type11_cell_coordinates, 0, sizeof(s_type11_cell_coordinates));
    s_predug_coordinate_count = 0;
    s_type11_cell_coordinate_count = 0;
    objectGlobs_queuedAnimationFraction = 0.0f;
    memset(s_idle100, 0, sizeof(s_idle100));
    s_idle100_n = 0;
    legoobject_global_animation_state_set(0);
}

// FUNCTION: LEGORR 0x00437300
void legoobject_process_queued_grid_animations(void* anchor, int mode, float delta)
{
    int blockPos[2];
    Lego_Object* target;
    Lego_Object* obj;

    if (mode == -1) objectGlobs_queuedAnimationFraction += delta;
    else objectGlobs_queuedAnimationFraction += weapon_type_discharge_rate_get_target(mode) * delta;

    legoobject_block_position_get((struct EntitySlot*)anchor, &blockPos[0], &blockPos[1]);
    while (objectGlobs_queuedAnimationFraction >= 1.0f) {
        if ((target = legoobject_query_center_mask01(blockPos, 6, 0, 0)) == NULL) break;
        if ((obj = legoobject_action_kind_create_and_command_issue(target, 6, 0, 0)) == NULL) break;
        obj->flags3 |= 0x80000000;
        legoobject_rover_work_queue_update(obj, 0);
        objectGlobs_queuedAnimationFraction -= 1.0f;
    }
}

// FUNCTION: LEGORR 0x004373d0
void legoobject_activity_toggle(Lego_Object* obj, int on)
{
    if (!on && (obj->flags4 & 0x2000)) {
        obj->flags4 &= ~0x2000;
        obj->flags4 |= 0x4000;
        ai_set_activity((struct EntitySlot*)obj, 0x36, 0);
        activity_register_bookkeeping((struct EntitySlot*)obj);
        return;
    }
    if (on && !(obj->flags4 & 0x2000)) {
        obj->flags4 |= 0x1000;
        ai_set_activity((struct EntitySlot*)obj, 0x35, 0);
    }
    activity_register_bookkeeping((struct EntitySlot*)obj);
}

// FUNCTION: LEGORR 0x00437440
void legoobject_grid_record_count_tables_clear(void)
{
    memset(s_grid_count_first, 0, sizeof(s_grid_count_first));
    memset(s_grid_count_second, 0, sizeof(s_grid_count_second));
}

// FUNCTION: LEGORR 0x00437460
int legoobject_grid_record_count_difference_get(int row, int column, int slot, int subtract)
{
    int count = ((unsigned int (*)[15][16])s_grid_count_second)[row][column][slot];

    if (subtract) count -= ((unsigned int (*)[15][16])s_grid_count_first)[row][column][slot];
    return count;
}

// FUNCTION: LEGORR 0x004374a0
unsigned int legoobject_grid_record_count_get(int row, int column, int slot)
{
    return s_grid_count_first[(row * 15 + column) * 16 + slot];
}

// FUNCTION: LEGORR 0x004374d0
void legoobject_task_counter_increment_by_branch(int nonzero)
{
    if (nonzero) s_task_counter_nonzero++;
    else s_task_counter_zero++;
}

// FUNCTION: LEGORR 0x004374f0
void legoobject_callback_dispatch_unless_type_five(Lego_Object* obj)
{
    if (obj->type != 5) {
        legoobject_run_through_lists_skip_upgrade_parts(legoobject_linked_route_clear_callback, obj);
    }
}

// FUNCTION: LEGORR 0x00437510
int legoobject_linked_route_clear_callback(Lego_Object* obj, void* data)
{
    if (obj->routeToObject == (Lego_Object*)data) {
        entity_action_sequence_reset((struct EntitySlot*)obj, 0);
        obj->routeToObject = NULL;
    }
    return 0;
}


/* ---- functions, group B (00437540-00437cc0) ---- */


/* ---- functions, group C (00437d00-00438580) ---- */


/* ---- functions, group D (004385b0-00438ed0) ---- */


/* ---- functions, group E (00438f60-00439930) ---- */


/* ---- functions, group F (00439950-00439e40) ---- */


/* ---- functions, group G (00439e70-0043a960) ---- */

