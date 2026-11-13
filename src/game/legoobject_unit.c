// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-023 -- original translation unit (text 00436c20-0044c710; finding
 * 505): the original's LegoObject.c, the object manager (the object pool,
 * object creation and removal, the per-object activity and AI updates).
 *
 * Rebuilt in the original's shape (ADR 0005) in three parts by address. This
 * file holds part 1 (issue #575): the functions 00436c20-0043a960, in
 * original order, in the C-compatible subset, so the matching build compiles
 * it as C. The native port compiles it as C++. Parts 2 and 3 move the rest
 * of the unit here from legoobject_port.cpp.
 *
 * The port's own model of LegoObject.c (EntitySlot, the GameContext and the
 * hook tables) is a different contract. Its implementations stay,
 * unannotated and under the port's names, in legoobject_port.cpp, which becomes
 * the unit's port tail once parts 2 and 3 have drained it. Native callers
 * keep using them. The functions here that keep a port name are the ones
 * whose callers already use the original's contract.
 *
 * The original keeps its state in one LegoObject_Globs struct (objectGlobs,
 * 004de790-004eadd0). The port models its fields as separate globals in
 * legoobject_port.cpp, which this unit reads with C linkage
 * (legoobject_unit_imports.h). Fields the port does not model are defined
 * here, one annotated global each. */
#include "aitask_port_aliases.h"
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

/* objectGlobs.recordObjs (004e8544): the level's deferred object records,
 * 0x2c bytes each, that LegoObject_CreateAndLinkDeferred realises when
 * their block is uncovered. The port keeps its own queue. */
// GLOBAL: LEGORR 0x004e8544
LegoObjGB_DeferredRecord objectGlobs_recordObjs[200];

/* objectGlobs.recordObjsCount (004ea7a4). */
// GLOBAL: LEGORR 0x004ea7a4
unsigned int objectGlobs_recordObjsCount;


/* ---- state, group C ---- */

/* The third primary teleporter (legoGlobs, DAT_00556454); lego_unit.c
 * defines the first two (0055644c, 00556450). */
// GLOBAL: LEGORR 0x00556454
Lego_Object* legoGlobs_f_556454;

/* Stats.c's static ObjectStats records (0x150 bytes each, first word 0x30)
 * that LegoObject_Create points objects without a stats row at: upgrade
 * parts, dynamite, barriers and the small object types. Only this unit
 * reads them; they stay here until Stats.c is converted. */
// GLOBAL: LEGORR 0x004a97a0
static uint32_t s_stats_upgrade_part[0x54] = { 0x30 };

// GLOBAL: LEGORR 0x004a98f0
static uint32_t s_stats_dynamite[0x54] = { 0x30 };

// GLOBAL: LEGORR 0x004a9a40
static uint32_t s_stats_barrier[0x54] = { 0x30 };

// GLOBAL: LEGORR 0x004a9b90
static uint32_t s_stats_small_object[0x54] = { 0x30 };


/* ---- state, group D ---- */
/* LegoObject.c's pooled 0.0f in .rdata, which the unit's float compares
 * read (about a hundred references, 004385b0-0044c710). Annotated, so a
 * read of it is a named operand on both sides; the original reads a
 * literal. */
// GLOBAL: LEGORR 0x0049e3a4
static const float legoobject_zero = 0.0f;


/* ---- state, group E ---- */


/* ---- state, group F ---- */


/* ---- state, group G ---- */


/* ---- state, group H (part 2) ---- */

/* objectGlobs (004eadc8): the 125-second timer of the level's periodic
 * pass (legoobject_update_all). The port keeps its own (s_scene_timer). */
// GLOBAL: LEGORR 0x004eadc8
float objectGlobs_f_4eadc8;


/* ---- state, group I (part 2) ---- */

/* objectGlobs (004dee0c): the 25-second cycle 0043c2b0 advances and
 * 0043c2f0 reads. The port keeps it unannotated (entity.cpp's
 * s_periodic_timer25). */
// GLOBAL: LEGORR 0x004dee0c
float objectGlobs_f_4dee0c;

/* objectGlobs (004e8444): every event mask 0043f5b0 has recorded. */
// GLOBAL: LEGORR 0x004e8444
unsigned int objectGlobs_f_4e8444;

/* legoGlobs.objTeleportQueue (00556474-00556517): the object requests
 * (type, id) a teleporting building serves in order, and their count. The
 * port keeps them unannotated (interface_port.cpp's lego_object_requests,
 * which interface_unit.c reads). The count is lego_unit.c's stand-in
 * legoGlobs_f_556514. */
// GLOBAL: LEGORR 0x00556474
LegoObjI_Requests legoGlobs_objectRequests;


/* ---- state, group J (part 2) ---- */

/* The object LegoObject_Callback_Update (0043c810) is updating; only that
 * function writes it. */
// GLOBAL: LEGORR 0x00556090
Lego_Object* legoGlobs_f_556090;

/* objectGlobs (004eadcc): counts the drill frames of minifigures without a
 * drill route; every fourth plays sample 8. */
// GLOBAL: LEGORR 0x004eadcc
unsigned int objectGlobs_f_eadcc;

/* LegoObject.c's .data (004a48b0, 1.0f): the fraction left of the spark
 * effect (flags2 0x40000000) that 0043c810 runs down. */
// GLOBAL: LEGORR 0x004a48b0
static float s_legoobject_spark_fraction = 1.0f;


/* ---- state, group K (part 2) ---- */


/* ---- state, group L (part 2) ---- */


/* ---- state, group M (part 2) ---- */


/* ---- state, group N (part 2) ---- */


/* ---- state, group O (part 2) ---- */


/* ------------------------------------------------------------------ */
/* Functions, in original order.                                       */
/* ------------------------------------------------------------------ */

/* ---- functions, group A (00436c20-00437510) ---- */

/* LegoObject_Initialise: clear objectGlobs (004de790, 0xc63c bytes, the
 * port's separate globals in legoobject_port.cpp, at their original addresses in
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

    legoobject_remove_all();
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
    legoobject_remove_all();
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

    legoobject_position_to_terrain_cell((Lego_Object*)anchor, &blockPos[0], &blockPos[1]);
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
        legoobject_set_activity(obj, 0x36, 0);
        legoobject_activity_register_bookkeeping(obj);
        return;
    }
    if (on && !(obj->flags4 & 0x2000)) {
        obj->flags4 |= 0x1000;
        legoobject_set_activity(obj, 0x35, 0);
    }
    legoobject_activity_register_bookkeeping(obj);
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
        legoobject_action_sequence_reset(obj, 0);
        obj->routeToObject = NULL;
    }
    return 0;
}


/* ---- functions, group B (00437540-00437cc0) ---- */

// FUNCTION: LEGORR 0x00437540
int legoobject_remove(Lego_Object* obj)
{
    s_grid_count_first[(obj->id + obj->type * 15) * 16 + obj->objLevel]++;

    if (obj->flags4 & 0x100000) {
        sound_handle_stop_forward(obj->drillSoundHandle);
    }
    if (obj->flags4 & 0x80000) {
        sound_handle_stop_forward(obj->soundHandle);
    }

    legoobject_run_through_lists_skip_upgrade_parts(
        (Lego_ObjectCallback)legoobject_gb_weapon_target_clear_callback, obj);

    if (obj->type == 3) {
        argument_callback_dispatch(obj);
    }
    if (obj->activityCont != NULL) {
        ledger_stub_00472a40_DDiSceneObjectDestroy(obj->activityCont);
    }
    if (obj->carryingThisObject != NULL) {
        legoobject_attached_object_list_detach(obj->carryingThisObject, 0);
    }

    if (obj->flags4 & 0x40) {
        obj->flags4 &= ~0x40;
        if (obj->routeToObject != NULL) {
            obj->routeToObject->flags4 &= ~0x40;
        }
    }

    legoobject_tracked_pointer_remove(obj);

    if (obj->flags4 & 0x1) {
        legoGlobs_flags1 &= ~0x10000000;
    }
    if (obj->flags4 & 0x80) {
        if (obj->routeToObject != NULL) {
            obj->routeToObject->flags4 &= ~0x80;
        }
        obj->routeToObject = NULL;
    }

    legoobject_action_sequence_reset(obj, 0);
    legoobject_type_four_callback_dispatch(obj);
    legoobject_attached_object_list_detach(obj, 0);
    entity_drain_command_chain((struct EntitySlot*)obj);
    legoobject_pool_find_with_local_predicate(obj);
    global_value_clear_if_equal((int)(uintptr_t)obj);
    if ((entity_stats_word_get((struct EntitySlot*)obj) & 4) ||
        (entity_stats_word_get((struct EntitySlot*)obj) & 2)) {
        task_callback_dispatch(obj);
    }
    selection_object_detach(obj);
    info_remove_object_references(obj);
    entity_pointer_registries_remove((struct EntitySlot*)obj);
    legoobject_callback_dispatch_unless_type_five(obj);
    lego_object_pointer_registry_clear_match(obj);

    if (obj->type == 1) {
        entity_payload_type1_release(
            (struct ScenePairedAnimationState*)obj->vehicle);
        free(obj->vehicle);
    } else if (obj->type == 2) {
        entity_payload_release_subslots((struct EntitySlot*)obj->miniFigure);
        free(obj->miniFigure);
    } else if (obj->type == 3) {
        if (entity_stats_word_get((struct EntitySlot*)obj) & 0x200000) {
            legoobject_auxiliary_payload_release(obj);
        }
        entity_payload_release_subslots((struct EntitySlot*)obj->rockMonster);
        free(obj->rockMonster);
    } else if (obj->type == 4) {
        entity_payload_teardown((struct EntitySlot*)obj->building);
        free(obj->building);
    } else if (obj->flags3 & 0x20000) {
        ledger_stub_00472a40_DDiSceneObjectDestroy(obj->other);
    } else if (obj->type == 10) {
        render_resource_binding_release(
            (struct RenderResourceBinding*)obj->upgradePart);
        free(obj->upgradePart);
    }

    obj->nextFree = (Lego_Object*)s_pool_record_head;
    s_pool_record_head = obj;
    return 0;
}

// FUNCTION: LEGORR 0x004377b0
int legoobject_run_through_lists_skip_upgrade_parts(Lego_ObjectCallback callback, void* data)
{
    return legoobject_run_through_lists(callback, data, 1);
}

// FUNCTION: LEGORR 0x004377d0
int legoobject_run_through_lists(Lego_ObjectCallback callback, void* data, int skipUpgradeParts)
{
    unsigned int list;
    unsigned int count;
    unsigned int i;
    Lego_Object* obj;

    for (list = 0; list < s_pool_chunk_count; list++) {
        if (s_pool_chunks[list] != NULL) {
            count = 1 << list;
            for (i = 0; i < count; i++) {
                obj = &((Lego_Object*)s_pool_chunks[list])[i];
                if (obj != NULL && obj->nextFree == obj) {
                    if (!skipUpgradeParts || !(obj->flags3 & 0x80000)) {
                        if (callback(obj, data)) {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00437880
void legoobject_rename(Lego_Object* obj, const char* name)
{
    if (name != NULL && *name != '\0') {
        if (obj->customName == NULL) {
            obj->customName = (char*)malloc(11);
        }
        strncpy(obj->customName, name, 11);
        obj->customName[11] = '\0';
    } else {
        if (obj->customName != NULL) {
            free(obj->customName);
        }
        obj->customName = NULL;
    }
}

// FUNCTION: LEGORR 0x004378e0
void legoobject_grid_object_records_release(void)
{
    unsigned int i;

    for (i = 0; i < objectGlobs_recordObjsCount; i++) {
        if (objectGlobs_recordObjs[i].linkName != NULL) {
            free(objectGlobs_recordObjs[i].linkName);
        }
        if (objectGlobs_recordObjs[i].name != NULL) {
            free(objectGlobs_recordObjs[i].name);
        }
    }
    objectGlobs_recordObjsCount = 0;
    memset(objectGlobs_recordObjs, 0, 200 * sizeof(int));
    global_counter_004ff990_reset();
}

// FUNCTION: LEGORR 0x00437940
void legoobject_deferred_olist_create_and_link(int* blockPos)
{
    Lego_Object* objs[200];
    char* linkNames[200];
    char* names[200];
    unsigned int count;
    unsigned int i;
    unsigned int j;
    uint32_t shapeCount;
    unsigned int direction;
    void* shape;
    void* points;
    LegoObjGB_DeferredRecord* rec;
    Lego_Object* obj;
    Lego_Object* target;
    unsigned int mode;

    count = 0;
    for (i = 0; i < objectGlobs_recordObjsCount; i++) {
        rec = &objectGlobs_recordObjs[i];
        if (rec->blockPos[0] == blockPos[0] && rec->blockPos[1] == blockPos[1]) {
            if (rec->type == 4) {
                shape = (void*)task_field84_get_and_field88_out(
                    (char*)legoGlobs_buildingData + rec->id * 0x14c, &shapeCount);
                direction = (unsigned int)(rec->heading * 0.15915494f * 8.0f);
                direction = ((direction & 1) + direction) >> 1;
                points = rotated_pairs_scratch_fill((const int*)rec, (const int*)shape,
                                                    shapeCount, direction);
                obj = (Lego_Object*)construction_place_building(
                    rec->id, (int*)rec, direction, (int*)points, shapeCount, 0);
            } else {
                obj = legoobject_create(rec->model, rec->type, rec->id);
            }

            if (obj != NULL) {
                if (rec->type != 4) {
                    legoobject_world_position_set(obj, rec->worldPos[0],
                                              rec->worldPos[1], rec->heading, 1);
                }
                if (rec->type == 6 || rec->type == 7 || rec->type == 8 || rec->type == 9) {
                    cmd_create_task((struct EntitySlot*)obj, 0);
                }
                obj->health = rec->health;
                if (obj->type == 2) {
                    legoobject_gb_tutorial_counter_increment_adapter(obj->type);
                    info_send(0x21, NULL, obj, NULL);
                }
                obj->flags4 |= 0x200;
                legoobject_kind_record_update_dispatch(obj);
                if (obj->type == 11) {
                    electric_fence_add((struct ElectricFenceObject*)obj);
                }
                objs[count] = obj;
                names[count] = rec->name;
                linkNames[count] = rec->linkName;
                count++;
                rec->name = NULL;
            }

            objectGlobs_recordObjsCount--;
            *rec = objectGlobs_recordObjs[objectGlobs_recordObjsCount];
            i--;
        }
    }

    for (i = 0; i < count; i++) {
        if (linkNames[i] != NULL) {
            for (j = 0; j < count; j++) {
                if (i != j && _strcmpi(names[j], linkNames[i]) == 0) {
                    obj = objs[i];
                    target = objs[j];
                    obj->routeToObject = target;
                    obj->flags2 |= 0x4;
                    if (entity_capability_flags_get((struct EntitySlot*)target) & 0x100) {
                        mode = 1;
                    } else if (!(entity_stats_word_get((struct EntitySlot*)target) & 0x1000000)) {
                        mode = 4;
                    } else if (!(entity_stats_word_get((struct EntitySlot*)target) & 0x2000000)) {
                        mode = 2;
                    } else {
                        mode = 1;
                    }
                    legoobject_linked_activity_event_dispatch(obj, mode);
                    legoobject_linked_route_flag_clear(obj, target);
                }
            }
        }
    }

    for (i = 0; i < count; i++) {
        if (linkNames[i] != NULL) {
            free(linkNames[i]);
        }
        if (names[i] != NULL) {
            free(names[i]);
        }
    }
}

// FUNCTION: LEGORR 0x00437c20
void legoobject_deferred_olist_queue_add(void* model, int type, int id, float* worldPos, float heading, float health, const char* name, const char* linkName)
{
    LegoObjGB_DeferredRecord* rec;

    rec = &objectGlobs_recordObjs[objectGlobs_recordObjsCount++];
    rec->worldPos[0] = worldPos[0];
    rec->worldPos[1] = worldPos[1];
    rec->heading = heading;
    rec->model = model;
    rec->type = type;
    rec->id = id;
    rec->health = health;
    if (name != NULL) {
        rec->name = string_duplicate(name);
    }
    if (linkName != NULL) {
        rec->linkName = string_duplicate(linkName);
    }
    terrain_map_world_to_block((struct TerrainMap*)level_current_terrain_get_original(),
                               worldPos[0], worldPos[1],
                               &rec->blockPos[0], &rec->blockPos[1]);
}

// FUNCTION: LEGORR 0x00437cc0
int legoobject_flag40000_threshold_compare(Lego_Object* obj)
{
    if (entity_state_flags_get((struct EntitySlot*)obj) & 0x40000) {
        if (entity_stats_float_ate8((struct EntitySlot*)obj) < obj->health) {
            return 1;
        }
    }
    return 0;
}


/* ---- functions, group C (00437d00-00438580) ---- */

// FUNCTION: LEGORR 0x00437d00
Lego_Object* legoobject_create(void* model, int type, int id)
{
    Lego_Object* obj;
    int soundOn;

    if ((obj = legoobject_pool_allocate()) != NULL) {
        obj->type = type;
        obj->id = id;
        obj->flags3 = 0;
        obj->health = 100.0f;
        obj->energy = 100.0f;

        renderer_flag_bit6_set(0);
        soundOn = sound_subsystem_enabled();
        sound_subsystem_enable_set(0, 0);

        if (obj->type == 1) {
            obj->vehicle = malloc(0x1ec);
            scene_record_template_instantiate((struct ScenePairedAnimationState*)model, (struct ScenePairedAnimationState*)obj->vehicle);
            scene_assign_owner_from_model((struct ScenePairedAnimationState*)obj->vehicle, obj);
            obj->carryNullFrames = object_record_field11c_get(obj->vehicle);
            scene_record_indexed_entries_rebuild_original(obj->vehicle, 0);
            obj->abilityFlags = 0x20;
            obj->flags3 |= 0x441190;
            entity_building_index_set((struct EntitySlot*)obj, 0);
            if (LEGOOBJ_FIELD(model, int, 0xb4)) obj->flags3 |= 0x2;
            if (LEGOOBJ_FIELD(model, int, 0xdc)) {
                obj->flags3 |= 0x20;
                if (entity_stats_word_get((struct EntitySlot*)obj) & 0x2000000) obj->flags3 |= 0x40;
            }
        } else if (obj->type == 2) {
            obj->miniFigure = malloc(0x74);
            renderer_record_clone((struct AnimationObject*)model, (struct AnimationObject*)obj->miniFigure);
            scene_assign_entity_owner(obj->miniFigure, (uint32_t)(uintptr_t)obj);
            obj->carryNullFrames = ((uint32_t (*)(void*))flic_opcode25_success_stub)(obj->miniFigure);
            obj->flags3 |= 0x4401f5;
            if (LEGOOBJ_FIELD(model, int, 0x2c)) obj->flags3 |= 0x2;
            entity_building_index_set((struct EntitySlot*)obj, 0);
            obj->flags3 |= 0x8000;
            legoobject_activity_grant(obj, 0);
        } else if (obj->type == 3) {
            obj->rockMonster = malloc(0x74);
            renderer_record_clone((struct AnimationObject*)model, (struct AnimationObject*)obj->rockMonster);
            scene_assign_entity_owner(obj->rockMonster, (uint32_t)(uintptr_t)obj);
            obj->carryNullFrames = ((uint32_t (*)(void*))flic_opcode25_success_stub)(obj->rockMonster);
            obj->flags3 |= 0x8440200;
            if (LEGOOBJ_FIELD(model, int, 0x2c)) obj->flags3 |= 0x2;
            entity_building_index_set((struct EntitySlot*)obj, 0);
            if (entity_stats_word_get((struct EntitySlot*)obj) & 0x200000) {
                legoobject_scene_object_tree_attach(obj);
                if (!(entity_stats_word_get((struct EntitySlot*)obj) & 0x400000)) {
                    legoobject_type_operation_dispatch(obj, 1);
                    obj->flags3 &= ~0x100;
                }
            } else {
                obj->field_32c = 0;
            }
            if (legoobject_flag40000_threshold_compare(obj)) entity_command15_validated_record_queue((struct EntitySlot*)obj);
            obj->flags3 |= 0x80000000;
        } else if (obj->type == 4) {
            obj->building = malloc(0x14c);
            scene_object_record_clone_retain_handles((struct SceneObjectRecord*)model, (struct SceneObjectRecord*)obj->building);
            scene_assign_entity_owner(obj->building, (uint32_t)(uintptr_t)obj);
            obj->carryNullFrames = scene_object_flags_get(obj->building);
            obj->flags3 |= 0x40100;
            entity_payload_state_set((struct EntitySlot*)obj->building, 0);
            entity_building_index_set((struct EntitySlot*)obj, 0);
            if (entity_stats_word_get((struct EntitySlot*)obj) & 0x10) legoGlobs_f_55644c = (uint32_t)(uintptr_t)obj;
            if (entity_stats_word_get((struct EntitySlot*)obj) & 0x20) legoGlobs_f_556450 = (uint32_t)(uintptr_t)obj;
            if (entity_stats_word_get((struct EntitySlot*)obj) & 0x40) legoGlobs_f_556454 = obj;
            entity_stats_word_get((struct EntitySlot*)obj);
            if (entity_stats_word_get((struct EntitySlot*)obj) & 0x4000) task_record_kind11_timed_queue(obj);
            task_record_kind10_queue(obj);
            obj->abilityFlags = 0x20;
            legoobject_activity_scene_state_update();
        } else if (obj->type == 10) {
            obj->upgradePart = malloc(0x10);
            render_resource_binding_copy_and_convert((struct RenderResourceBinding*)model, (struct RenderResourceBinding*)obj->upgradePart);
            obj->stats = s_stats_upgrade_part;
            obj->flags3 |= 0x80000;
        } else {
            obj->other = entity_visual_resource_load(model);
            scene_object_owner_set(obj->other, (uint32_t)(uintptr_t)obj);
            obj->flags3 |= 0x20000;
            if (obj->type == 11) obj->flags3 |= 0x40100;
            if (obj->type == 7 || obj->type == 6) obj->flags3 |= 0x100100;
            else if (obj->type == 8 || obj->type == 13) obj->flags3 |= 0x40000;
            legoobject_rover_work_queue_update(obj, 1);
            switch (obj->type) {
            case 8:
                obj->stats = s_stats_dynamite;
                break;
            case 9:
                obj->stats = s_stats_barrier;
                break;
            case 12:
            case 13:
            case 14:
            case 16:
            case 17:
            case 18:
            case 19:
                obj->stats = s_stats_small_object;
                break;
            default:
                entity_building_index_set((struct EntitySlot*)obj, 0);
                break;
            }
        }

        obj->activityName2 = s_activity[0];
        obj->activityName1 = s_activity[9];
        if (legoobject_activity_register_bookkeeping(obj)) obj->flags3 |= 0x8;
        obj->activityName1 = s_activity[16];
        if (legoobject_activity_register_bookkeeping(obj)) obj->flags3 |= 0x4000000;
        obj->activityName1 = s_activity[2];
        if (legoobject_activity_register_bookkeeping(obj)) obj->flags3 |= 0x10000000;
        obj->activityName1 = s_activity[0];
        legoobject_activity_register_bookkeeping(obj);

        task_record_kind14_remove_if_head((struct EntitySlot*)obj);
        renderer_flag_bit6_set(1);
        if (soundOn) sound_subsystem_enable_set(1, 0);

        legoobject_kind_four_post_create_resolve(obj);
        s_grid_count_second[(obj->type * 15 + obj->id) * 16]++;
        if (entity_state_flags_get((struct EntitySlot*)obj) & 0x80000000) obj->flags3 &= ~0x100;

        return obj;
    }

    return NULL;
}

// FUNCTION: LEGORR 0x004382c0
Lego_Object* legoobject_pool_allocate(void)
{
    Lego_Object* newObj;

    if (s_pool_record_head == NULL) legoobject_pool_chunk_allocate();

    newObj = (Lego_Object*)s_pool_record_head;
    s_pool_record_head = newObj->nextFree;
    memset(newObj, 0, sizeof(Lego_Object));
    newObj->nextFree = newObj;
    newObj->targetBlockPos[0] = newObj->targetBlockPos[1] = -1.0f;

    return newObj;
}

/* objectGlobs.listCount, re-read after the listSet store. In the original
 * both are fields of objectGlobs, so MSVC must assume the indexed store
 * may alias the count and loads it again. The port keeps them as separate
 * globals; a volatile read restores the reloads (finding: effective match,
 * the count load is scheduled one instruction earlier). */
#define LEGOOBJ_GC_LIST_COUNT (*(volatile uint32_t*)&s_pool_chunk_count)

// FUNCTION: LEGORR 0x00438310
void legoobject_pool_chunk_allocate(void)
{
    Lego_Object* list;
    unsigned int count, loop;

    count = 1 << s_pool_chunk_count;
    s_pool_chunks[s_pool_chunk_count] = malloc(sizeof(Lego_Object) * count);
    if ((list = (Lego_Object*)s_pool_chunks[LEGOOBJ_GC_LIST_COUNT]) != NULL) {
        LEGOOBJ_GC_LIST_COUNT++;
        for (loop = 1; loop < count; loop++) list[loop - 1].nextFree = &list[loop];
        list[count - 1].nextFree = (Lego_Object*)s_pool_record_head;
        s_pool_record_head = list;
    }
}

// FUNCTION: LEGORR 0x00438390
unsigned int legoobject_global_animation_state_get(void)
{
    return s_global_animation_state;
}

// FUNCTION: LEGORR 0x004383a0
void legoobject_global_animation_state_set(unsigned int state)
{
    s_global_animation_state = state;
}

// FUNCTION: LEGORR 0x004383b0
void legoobject_rover_work_queue_update(Lego_Object* obj, int mode)
{
    float r, g, b;
    unsigned int loop;

    if (obj->type == 6) {
        if (mode) {
            r = legoGlobs_PowerCrystalRGB[0];
            g = legoGlobs_PowerCrystalRGB[1];
            b = legoGlobs_PowerCrystalRGB[2];
        } else {
            r = legoGlobs_UnpoweredCrystalRGB[0];
            g = legoGlobs_UnpoweredCrystalRGB[1];
            b = legoGlobs_UnpoweredCrystalRGB[2];
        }
        for (loop = 0; loop < (unsigned int)animated_texture_frame_count(obj->other); loop++) {
            scene_mesh_color_and_alpha_set(obj->other, loop, r, g, b, 1.0f);
            scene_mesh_color_set(obj->other, loop, r, g, b);
        }
    }
}

// FUNCTION: LEGORR 0x00438460
void legoobject_kind_record_update_dispatch(Lego_Object* obj)
{
    if (obj->type == 4) {
        legoobject_activity_register_bookkeeping(obj);
        obj->flags3 |= 0x20;
        if (entity_stats_word_get((struct EntitySlot*)obj) & 0x8) task_record_create_and_scan_grid(obj);
        else if (entity_stats_word_get((struct EntitySlot*)obj) & 0x2) timed_task_record_initialize_and_link(obj, 7, 0, 0);
        else if (entity_stats_word_get((struct EntitySlot*)obj) & 0x4) timed_task_record_initialize_and_link(obj, 6, 0, 0);
        else obj->flags3 &= ~0x20;
        legoobject_activity_scene_state_update();
        obj->flags4 |= 0x200000;
    } else if (obj->type == 1) {
        six_resource_slots_set_state(obj->vehicle, 0);
    }

    if (obj->type == 2 || obj->type == 1) legoobject_pool_find_with_local_predicate(obj);

    building_upgrade_state_changed_dispatch(obj->type, obj->id, 0, obj->flags4 & 0x200);

    if (obj->type == 4) s_global_animation_state++;
    if (obj->type == 2 || obj->type == 1) scalar_task_record_queue(obj);
    if (obj->type == 1 || obj->type == 4) obj->flags4 |= 0x40000;
}

// FUNCTION: LEGORR 0x00438580
void legoobject_flag80000000_set_or_clear(Lego_Object* obj, int on)
{
    if (on) obj->flags3 &= 0x7fffffff;
    else obj->flags3 |= 0x80000000;
    legoobject_activity_scene_state_update();
}


/* ---- functions, group D (004385b0-00438ed0) ---- */

/* LegoObject_IsActive-style check: not dead, not being removed, and (unless
 * relaxed) ready for work. */
// FUNCTION: LEGORR 0x004385b0
int legoobject_shared_eligibility_check(Lego_Object* obj, int relaxed)
{
    if (!(obj->flags1 & 0x700100) && !(obj->flags2 & 0x100000) &&
        obj->health >= legoobject_zero && !(obj->flags1 & 0x20000) &&
        (relaxed || ((obj->flags3 & 0x20000000) && !(obj->flags3 & 0x80000000))))
    {
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00438610
Lego_Object* legoobject_create_in_world(void* model, int type, int id, int last, float x, float y, float heading)
{
    Lego_Object* obj;

    if ((obj = legoobject_create(model, type, id)) != NULL) {
        legoobject_world_position_set(obj, x, y, heading, 1);
        if (obj->type == 6 || obj->type == 7) legoobject_animation_state_set(obj, last);
        return obj;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00438670
Lego_Object* legoobject_at_grid_position_find(int* cell)
{
    LegoObjGD_BlockSearch search;

    search.result = NULL;
    search.bx = cell[0];
    search.by = cell[1];
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_grid_position_match_callback, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x004386b0
int legoobject_grid_position_match_callback(Lego_Object* obj, void* data)
{
    LegoObjGD_BlockSearch* search = (LegoObjGD_BlockSearch*)data;
    int blockPos[2];

    if (obj->type == 4 && (entity_state_flags_get((struct EntitySlot*)obj) & 0x200) &&
        legoobject_shared_eligibility_check(obj, 0))
    {
        legoobject_position_to_terrain_cell(obj, &blockPos[0], &blockPos[1]);
        if (blockPos[0] == search->bx && blockPos[1] == search->by) {
            search->result = obj;
            return 1;
        }
    }
    return 0;
}

/* 0041f8e0 is empty in this build (noop_callback); the original compares
 * whatever it leaves in eax, so it is called through an int-returning
 * type. Only the matching build runs this function. */
// FUNCTION: LEGORR 0x00438720
int legoobject_available_resource_spend(Lego_Object* obj, int amount)
{
    if (LEGOOBJ_FIELD(g_active_level_retail, int, 0x1f8)) amount = 0;
    if (((int (*)(int))noop_callback)(0) >= amount) {
        global_object_field98_add(amount);
        return 1;
    }
    info_send(0xe, NULL, obj, NULL);
    return 0;
}

// FUNCTION: LEGORR 0x00438770
int legoobject_building_upgrade_resource_admission_check(Lego_Object* building, unsigned int* outCost)
{
    if (building->objLevel + 1 < building_upgrade_admission_check(building->type, building->id)) {
        if ((unsigned int)lego_upgrade_resource_check(0) >= (unsigned int)legoGlobs_BuildingUpgradeCostOre) {
            if (outCost) *outCost = 0;
            return 1;
        } else if ((unsigned int)lego_upgrade_resource_check(1) >= (unsigned int)legoGlobs_BuildingUpgradeCostStuds) {
            if (outCost) *outCost = legoGlobs_BuildingUpgradeCostStuds;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x004387f0
void legoobject_building_upgrade_commit(Lego_Object* building)
{
    unsigned int cost;
    float wPos[3];

    if (building->type == 4) {
        if (legoobject_building_upgrade_resource_admission_check(building, &cost)) {
            lego_upgrade_resource_debit(cost != 0, cost ? cost : legoGlobs_BuildingUpgradeCostOre);
            building->field_3d8 = 0.0f;
            entity_payload_state_set((struct EntitySlot*)building->building, building->objLevel + 1);
            legoobject_gd_building_index_set_adapter((struct EntitySlot*)building, building->objLevel + 1);
            ddiscene_object_position_get(legoobject_type_component_get(building), NULL, wPos);
            lava_effect_select_and_place(9, wPos, NULL);
            building_upgrade_state_changed_dispatch(building->type, building->id, building->objLevel, 0);
        }
    }
}

// FUNCTION: LEGORR 0x004388b0
int legoobject_action_kind_prerequisite_check(Lego_Object* obj, int action)
{
    unsigned int studs, ore;

    if (obj->type == 1 && record_flag_bit_test(obj->vehicle, action, 1)) {
        studs = (entity_mapped_indexed_record_word_at_fc_get(obj->type, obj->id, action) % 5 != 0);
        studs += entity_mapped_indexed_record_word_at_fc_get(obj->type, obj->id, action) / 5;
        ore = entity_mapped_indexed_record_word_at_fc_get(obj->type, obj->id, action);
        if ((unsigned int)lego_upgrade_resource_check(0) < ore &&
            (unsigned int)lego_upgrade_resource_check(1) < studs)
        {
            return 0;
        }
        return 1;
    }
    return 0;
}

/* 36%: the original reloads the mask and ORs 0x4000 into it in memory, and
 * keeps alt in ecx; MSVC 5.0 folds every C form tried here into one store
 * of 0x6000 (finding notes in the GAME-023 group D report). */
// FUNCTION: LEGORR 0x00438960
Lego_Object* legoobject_unknown_00438960(Lego_Object* obj, int alt)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = 0.0f;
    search.y = 0.0f;
    search.bestDistance = 10000.0f;
    search.mask = 0x2000;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = 0;
    if (alt) search.mask |= 0x4000;
    if (obj) legoobject_transform_position_get(obj, &search.x, &search.y);
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x004389e0
Lego_Object* legoobject_route_marker_candidate_find(Lego_Object* source, int secondary)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = 0.0f;
    search.y = 0.0f;
    search.bestDistance = 10000.0f;
    search.mask = 0x1000;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = 0;
    legoobject_transform_position_get(source, &search.x, &search.y);
    if (secondary) search.mask |= 0x400;
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438a60
Lego_Object* legoobject_query_center_mask01(int* cell, int a, int b, int c)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = 0.0f;
    search.y = 0.0f;
    search.bestDistance = 10000.0f;
    search.mask = 0x1;
    search.arg14 = a;
    search.arg18 = b;
    search.arg1c = c;
    search.arg20 = 0;
    terrain_map_block_to_world((struct TerrainMap*)level_current_terrain_get_original(), cell[0], cell[1], &search.x, &search.y);
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438ae0
Lego_Object* legoobject_query_center_mask02(Lego_Object* obj, int* cell, int a, int b)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = 0.0f;
    search.y = 0.0f;
    search.bestDistance = 10000.0f;
    search.mask = 0x2;
    search.arg14 = a;
    search.arg18 = 0;
    search.arg1c = b;
    search.arg20 = 0;
    if (obj) legoobject_transform_position_get(obj, &search.x, &search.y);
    else terrain_map_block_to_world((struct TerrainMap*)level_current_terrain_get_original(), cell[0], cell[1], &search.x, &search.y);
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438b80
Lego_Object* legoobject_query_center_mask_from_argument(Lego_Object* obj, int mask)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = 0.0f;
    search.y = 0.0f;
    search.bestDistance = 10000.0f;
    search.mask = mask;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = 0;
    legoobject_transform_position_get(obj, &search.x, &search.y);
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438bf0
Lego_Object* legoobject_query_center_mask80(Lego_Object* obj)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = 0.0f;
    search.y = 0.0f;
    search.bestDistance = 10000.0f;
    search.mask = 0x80;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = 0;
    legoobject_transform_position_get(obj, &search.x, &search.y);
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438c60
Lego_Object* legoobject_candidate_search(Lego_Object* obj)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = 0.0f;
    search.y = 0.0f;
    search.bestDistance = 10000.0f;
    search.mask = 0x4;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = 0;
    legoobject_transform_position_get(obj, &search.x, &search.y);
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438cd0
Lego_Object* legoobject_query_center_mask20(float* center)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = center[0];
    search.y = center[1];
    search.bestDistance = 10000.0f;
    search.mask = 0x20;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = 0;
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438d30
Lego_Object* legoobject_query_center_mask10(float* center)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = center[0];
    search.y = center[1];
    search.bestDistance = 10000.0f;
    search.mask = 0x10;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = 0;
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438d90
Lego_Object* legoobject_query_center_mask200(float* center)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = center[0];
    search.y = center[1];
    search.bestDistance = 10000.0f;
    search.mask = 0x200;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = 0;
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438df0
Lego_Object* legoobject_lookup_at_world_position(float* position)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = position[0];
    search.y = position[1];
    search.bestDistance = 10000.0f;
    search.mask = 0x8;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = 0;
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438e50
Lego_Object* legoobject_query_center_mask100(Lego_Object* obj, float* center, int extra)
{
    LegoObjGD_SearchData search;

    search.result = NULL;
    search.x = 0.0f;
    search.y = 0.0f;
    search.bestDistance = 10000.0f;
    search.mask = 0x100;
    search.arg14 = 0;
    search.arg18 = 0;
    search.arg1c = 0;
    search.arg20 = extra;
    if (center) {
        search.x = center[0];
        search.y = center[1];
    }
    if (obj) legoobject_transform_position_get(obj, &search.x, &search.y);
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_route_marker_candidate_visit, &search);
    return search.result;
}

// FUNCTION: LEGORR 0x00438ed0
int legoobject_flags_contain_requested_single_bit(Lego_Object* obj, int selector)
{
    if (!(entity_state_flags_get((struct EntitySlot*)obj) & 0x4) || selector != 1) {
        if (!(entity_state_flags_get((struct EntitySlot*)obj) & 0x8) || selector != 2) {
            if (!(entity_state_flags_get((struct EntitySlot*)obj) & 0x10) || selector != 4) {
                if (!(entity_state_flags_get((struct EntitySlot*)obj) & 0x20) || selector != 8) {
                    if (!(entity_state_flags_get((struct EntitySlot*)obj) & 0x40) || selector != 0x10) {
                        if (!(entity_state_flags_get((struct EntitySlot*)obj) & 0x80) || selector != 0x20) {
                            return 0;
                        }
                    }
                }
            }
        }
    }
    return 1;
}


/* ---- functions, group E (00438f60-00439930) ---- */

/* Whether obj is the object the "Docks" definition names (its type and
 * index). The spatial query callback below passes the object itself. */
// FUNCTION: LEGORR 0x00438f60
int legoobject_dock_definition_matches_cell(Lego_Object* obj)
{
    int type;
    int index;

    // STRING: LEGORR 0x004a5060
    if (entity_type_pair_lookup("Docks", &type, &index, 0) &&
        obj->type == type && obj->id == index) {
        return 1;
    }
    return 0;
}

/* LegoObject_RunThroughLists callback of the spatial queries: keeps the
 * nearest object that the search mask accepts. Always returns 0, so the
 * walk visits every object. */
// FUNCTION: LEGORR 0x00438fb0
int legoobject_route_marker_candidate_visit(Lego_Object* obj, void* data)
{
    LegoObjGE_RouteMarkerSearch* search = (LegoObjGE_RouteMarkerSearch*)data;
    unsigned int flags = search->flags;
    int found = 0;
    float pos[2];
    float distance;

    if (legoobject_shared_eligibility_check(obj, flags & 0x400)) {
        flags &= ~0x400;
        if (!(flags & 0x4000) || !(obj->flags4 & 0x10)) {
            if (flags == 0) {
                if (obj->type == 4) found = 1;
            } else {
                if (flags & 0x1) {
                    if ((entity_stats_word_get((struct EntitySlot*)obj) & 0x8) &&
                        legoobject_action_kind_supported(
                            obj, search->actionKind, search->actionArgument,
                            search->actionExtra)) {
                        found = 1;
                    }
                }
                if (flags & 0x1000) {
                    if (legoobject_dock_definition_matches_cell(obj)) found = 1;
                }
                if (flags & 0x2000) {
                    if (!(entity_state_flags_get((struct EntitySlot*)obj) & 0x400) &&
                        !(entity_state_flags_get((struct EntitySlot*)obj) & 0x200) &&
                        legoobject_shared_eligibility_check(obj, 0)) {
                        found = 1;
                    }
                }
                if (flags & 0x100) {
                    if (legoobject_flags_contain_requested_single_bit(obj, search->selector)) found = 1;
                }
                if (flags & 0x40) {
                    if (!(entity_stats_word_get((struct EntitySlot*)obj) & 0x8) && obj->type == 4) found = 1;
                }
                if (flags & 0x80) {
                    if ((entity_state_flags_get((struct EntitySlot*)obj) & 0x2) &&
                        obj->interactObject == NULL) {
                        found = 1;
                    }
                }
                if (flags & 0x800) {
                    if (entity_stats_word_get((struct EntitySlot*)obj) & 0x80000000) found = 1;
                }
                if (flags & 0x10) {
                    if (entity_stats_word_get((struct EntitySlot*)obj) & 0x10) found = 1;
                }
                if (flags & 0x20) {
                    if (entity_stats_word_get((struct EntitySlot*)obj) & 0x20) found = 1;
                }
                if (flags & 0x200) {
                    if (entity_stats_word_get((struct EntitySlot*)obj) & 0x40) found = 1;
                }
                if (flags & 0x2) {
                    if (((entity_stats_word_get((struct EntitySlot*)obj) & 0x2) && search->actionKind == 7) ||
                        ((entity_stats_word_get((struct EntitySlot*)obj) & 0x4) && search->actionKind == 6)) {
                        found = 1;
                    }
                }
                if (flags & 0x4) {
                    if (entity_stats_word_get((struct EntitySlot*)obj) & 0x4000) found = 1;
                }
                if (flags & 0x8) {
                    if (entity_stats_word_get((struct EntitySlot*)obj) & 0x100000) found = 1;
                }
            }
            if (found) {
                legoobject_transform_position_get(obj, &pos[0], &pos[1]);
                if ((distance = (float)sqrt((search->x - pos[0]) * (search->x - pos[0]) +
                                            (search->y - pos[1]) * (search->y - pos[1]))) <
                    search->bestDistance) {
                    search->bestDistance = distance;
                    search->result = obj;
                }
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00439200
int legoobject_task_kind_filtered_search(int selector)
{
    if (selector != 1 && selector != 2 && selector != 4) {
        if (legoobject_run_through_lists_skip_upgrade_parts(
                legoobject_task_flags_and_cell_task_match, &selector)) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00439240
int legoobject_task_flags_and_cell_task_match(Lego_Object* obj, void* task)
{
    if (entity_stats_word_get((struct EntitySlot*)obj) & 0x8) {
        if (legoobject_action_kind_supported(
                obj, *(int*)task, 0, 0)) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00439280
void legoobject_build_placement_command_batch_expand(int* cell)
{
    uint32_t crystals;
    uint32_t crystalsAlt;
    uint32_t ores;
    uint32_t oresAlt;
    uint32_t i;

    if (terrain_cell_byte_state_classify_current(cell, &crystals, &crystalsAlt,
                                                 &ores, &oresAlt)) {
        for (i = 0; i < crystals; i++) {
            legoobject_build_placement_command_setup(cell, 0, NULL, 1);
        }
        for (i = 0; i < crystalsAlt; i++) {
            legoobject_build_placement_command_setup(cell, 1, NULL, 1);
        }
        for (i = 0; i < ores; i++) {
            legoobject_build_placement_alternate_command_setup(cell, 0, NULL, 1);
        }
        for (i = 0; i < oresAlt; i++) {
            legoobject_build_placement_alternate_command_setup(cell, 1, NULL, 1);
        }
    }
}

// FUNCTION: LEGORR 0x00439340
void legoobject_build_placement_command_pair_create(int* cell, int id)
{
    legoobject_build_placement_command_setup(cell, id, NULL, 1);
    legoobject_build_placement_alternate_command_setup(cell, id, NULL, 1);
}

// FUNCTION: LEGORR 0x00439370
void legoobject_build_placement_command_setup(int* cell, int id, LegoObjGE_Point2F* world, int update)
{
    float inset;
    LegoObjGE_Point2F pos;
    float vertices[4][3];
    Lego_Object* obj;

    inset = terrain_context_scale_get(Lego_GetMap()) * 0.5f -
            terrain_context_scale_get(Lego_GetMap()) * 0.25f;
    if (world != NULL) {
        pos = *world;
    } else {
        terrain_map_block_vertices_get(Lego_GetMap(), cell[0], cell[1],
                                       (struct TerrainMapVector3*)vertices);
        pos.x = random_float_range(vertices[0][0] + inset, vertices[2][0] - inset);
        pos.y = random_float_range(vertices[0][1] - inset, vertices[2][1] + inset);
    }
    obj = legoobject_create_in_world(legoGlobs_contCrystal, 6, 0, id, pos.x, pos.y, 0.0f);
    cmd_produce(0x16, obj, 0, 0);
    if (update) {
        info_send(0, NULL, NULL, (struct InfoPoint2I*)cell);
        advisor_text_animation_update(0x11, 1, 0);
        LEGOOBJ_FIELD(level_current_owner_get(), LegoObjGE_Block*, 0xb8)
            [LEGOOBJ_FIELD(level_current_owner_get(), int, 0x14) * cell[1] + cell[0]].field_2c++;
    }
}

// FUNCTION: LEGORR 0x004394b0
void legoobject_build_placement_alternate_command_setup(int* cell, int id, LegoObjGE_Point2F* world, int update)
{
    float inset;
    LegoObjGE_Point2F pos;
    float vertices[4][3];
    Lego_Object* obj;

    inset = terrain_context_scale_get(Lego_GetMap()) * 0.5f -
            terrain_context_scale_get(Lego_GetMap()) * 0.25f;
    if (world != NULL) {
        pos = *world;
    } else {
        terrain_map_block_vertices_get(Lego_GetMap(), cell[0], cell[1],
                                       (struct TerrainMapVector3*)vertices);
        pos.x = random_float_range(vertices[0][0] + inset, vertices[2][0] - inset);
        pos.y = random_float_range(vertices[0][1] - inset, vertices[2][1] + inset);
    }
    obj = legoobject_create_in_world(legoGlobs_contOresTable, 7, 0, id, pos.x, pos.y, 0.0f);
    cmd_produce(0x1c, obj, 0, 0);
    advisor_text_animation_update(0x13, 1, 0);
    if (update) {
        LEGOOBJ_FIELD(level_current_owner_get(), LegoObjGE_Block*, 0xb8)
            [LEGOOBJ_FIELD(level_current_owner_get(), int, 0x14) * cell[1] + cell[0]].field_30++;
    }
}

// FUNCTION: LEGORR 0x004395e0
const char* legoobject_display_name_lookup(Lego_Object* obj)
{
    switch (obj->type) {
    case 2: return s_object_name_kind2[*(int*)obj->miniFigure];
    case 3: return s_object_name_kind3[*(int*)obj->rockMonster];
    case 1: return s_object_name_kind1[*(int*)obj->vehicle];
    case 4: return s_object_name_kind4[*(int*)obj->building];
    case 10: return s_object_name_kind10[*(int*)obj->upgradePart];
    case 6: return s_object_name_special[0];
    case 7: return s_object_name_special[obj->id + 1];
    case 8: return s_object_name_special[3];
    case 9: return s_object_name_special[4];
    case 11: return s_object_name_special[5];
    case 12: return s_object_name_special[6];
    case 13: return s_object_name_special[7];
    case 15: return s_object_name_special[8];
    }
    return NULL;
}

// FUNCTION: LEGORR 0x004396c0
const char* legoobject_item_name_by_kind(unsigned int kind, int index)
{
    switch (kind) {
    case 6: return "PowerCrystal";
    case 7: return index == 0 ? "Ore" : "ProcessedOre";
    case 1: return legoGlobs_vehicleName[index];
    case 2: return legoGlobs_miniFigureName[index];
    case 3: return legoGlobs_rockMonsterName[index];
    case 4: return legoGlobs_buildingName[index];
    case 8: return "Dynamite";
    case 9: return "Barrier";
    case 11: return "ElectricFence";
    case 12: return "SpiderWeb";
    case 13: return "OohScary";
    case 0xffffffff: return "tvcamera";
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00439790
const char* legoobject_the_name_get(unsigned int kind, int index)
{
    switch (kind) {
    case 2: return s_object_the_name_kind2[index];
    case 3: return s_object_the_name_kind3[index];
    case 1: return s_object_the_name_kind1[index];
    case 4: return s_object_the_name_kind4[index];
    case 10: return s_object_the_name_kind10[index];
    case 6: return s_object_the_name_special[0];
    case 7: return s_object_the_name_special[index + 1];
    case 8: return s_object_the_name_special[3];
    case 9: return s_object_the_name_special[4];
    case 11: return s_object_the_name_special[5];
    case 12: return s_object_the_name_special[6];
    case 13: return s_object_the_name_special[7];
    case 15: return s_object_the_name_special[8];
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00439860
const char* legoobject_name_get(unsigned int kind, int index)
{
    switch (kind) {
    case 2: return s_object_name_kind2[index];
    case 3: return s_object_name_kind3[index];
    case 1: return s_object_name_kind1[index];
    case 4: return s_object_name_kind4[index];
    case 10: return s_object_name_kind10[index];
    case 6: return s_object_name_special[0];
    case 7: return s_object_name_special[index + 1];
    case 8: return s_object_name_special[3];
    case 9: return s_object_name_special[4];
    case 11: return s_object_name_special[5];
    case 12: return s_object_name_special[6];
    case 13: return s_object_name_special[7];
    case 15: return s_object_name_special[8];
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00439930
void legoobject_copy_two_dword_pair(void* record, unsigned int* outA, unsigned int* outB)
{
    *outA = ((unsigned int*)record)[0];
    *outB = ((unsigned int*)record)[1];
}


/* ---- functions, group F (00439950-00439e40) ---- */

/* 00439950: the tools' display names from the ToolNames block, into
 * legoGlobs.langToolName (00556228), which follows legoGlobs.toolName
 * (005561fc) in the original's Lego_Globs. */
// FUNCTION: LEGORR 0x00439950
void legoobject_tool_name_strings_load(void* config, const char* gameName)
{
    unsigned int i;

    for (i = 0; i < LEGO_TOOL_TYPE_COUNT; i++) {
        legoGlobs_langToolName[i] = block_get_str_copy(
            (struct BlockRec*)config,
            // STRING: LEGORR 0x004a5068
            string_path_join(gameName, "ToolNames", legoGlobs_toolName[i], NULL));
    }
}

// FUNCTION: LEGORR 0x00439990
void legoobject_activity_scene_state_update(void)
{
    if (s_entity_state_flags & 0x10) {
        s_entity_state_flags |= 0x8;
        return;
    }
    s_entity_state_flags |= 0x4;
    global_object_field98_clear_current();
}

// FUNCTION: LEGORR 0x004399c0
int legoobject_flag400_and_health_threshold_check(Lego_Object* obj)
{
    unsigned int capacity;

    if (obj->flags1 & 0x400) {
        if (obj->type == 1) {
            capacity = (obj->carryNullFrames < stats_instance_word_at_ec(obj))
                ? obj->carryNullFrames : stats_instance_word_at_ec(obj);
            if (obj->numCarriedObjects < capacity && obj->field_3d4 < 75.0f)
                return 0;
        }
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00439a20
int legoobject_gated_step_command_issue(Lego_Object* obj, Lego_Object* target)
{
    int* routeX;
    int* routeY;
    int fromX;
    int fromY;
    unsigned int count;
    float targetPos[2];
    float pos[2];

    if (legoobject_flag400_and_health_threshold_check(obj) == 0 &&
        legoobject_accepts_target_for_task_check(obj, target, 1))
    {
        int toX;
        int toY;

        legoobject_transform_position_get(target, &targetPos[0], &targetPos[1]);
        terrain_map_world_to_block(Lego_GetMap(), targetPos[0], targetPos[1], &toX, &toY);
        legoobject_transform_position_get(obj, &pos[0], &pos[1]);
        terrain_map_world_to_block(Lego_GetMap(), pos[0], pos[1], &fromX, &fromY);

        if (fromX == toX && fromY == toY) {
            legoobject_route_setup_and_marker_associate(obj, 1,
                (unsigned int*)&toX, (unsigned int*)&toY, targetPos);
        } else {
            if (!legoobject_terrain_neighbor_path_check(obj, fromX, fromY, toX, toY,
                    (unsigned int**)&routeX, (unsigned int**)&routeY, (unsigned int*)&count, NULL, NULL))
                return 0;
            legoobject_route_setup_and_marker_associate(obj, count,
                (unsigned int*)routeX, (unsigned int*)routeY, targetPos);
            free(routeX);
            free(routeY);
        }
        ((unsigned char*)obj->routeBlocks)[obj->routeBlocksTotal * 0x14 - 3] = 3;
        obj->routeToObject = target;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00439b80
int legoobject_task_target_cell_eligibility_check(Lego_Object* obj, Lego_Object* target)
{
    if (obj->type == 1) {
        if (target->id == 0) {
            if (!(entity_capability_flags_get(LEGOOBJ_SLOT(obj)) & 0x200))
                return legoobject_query_center_mask02(obj, NULL, target->type, target->objLevel) != NULL;
        } else {
            return 0;
        }
    }
    return 1;
}

// FUNCTION: LEGORR 0x00439bd0
int legoobject_accepts_target_for_task_check(Lego_Object* obj, Lego_Object* target, int extra)
{
    if (obj->type == 3) {
        if (!(obj->flags3 & 0x8000000))
            return 0;
    } else if (!(obj->flags3 & 0x20)) {
        return 0;
    }
    if (obj->flags1 & 0x1)
        return 0;
    if (extra && legoobject_flags_mask107c33de_or_1c00203_any_test(obj))
        return 0;
    if ((entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x4) && target->type != 6)
        return 0;
    if ((entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x2) && target->type != 7)
        return 0;
    if (obj->numCarriedObjects != 0 && obj->carriedObjects[0]->type != target->type)
        return 0;
    return 1;
}

// FUNCTION: LEGORR 0x00439c80
void legoobject_indexed_flag_set_and_refresh(Lego_Object* obj)
{
    unsigned int level;

    if (obj->type == 1) {
        level = entity_type_mask_get(obj->vehicle);
        level |= 1 << obj->field_404;
        scene_record_indexed_entries_rebuild_original(obj->vehicle, level);
        entity_building_index_set(LEGOOBJ_SLOT(obj), level);
        advisor_text_animation_update(0x19, 1, 0);
    }
}

// FUNCTION: LEGORR 0x00439cd0
void legoobject_animation_state_set(Lego_Object* obj, unsigned int state)
{
    entity_building_index_set(LEGOOBJ_SLOT(obj), state);
}

// FUNCTION: LEGORR 0x00439cf0
unsigned int legoobject_stats_flag10_test(Lego_Object* obj)
{
    return entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x10;
}

// FUNCTION: LEGORR 0x00439d10
unsigned int legoobject_stats_flag20_test(Lego_Object* obj)
{
    return entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x20;
}

// FUNCTION: LEGORR 0x00439d30
unsigned int legoobject_stats_flag40_test(Lego_Object* obj)
{
    return entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x40;
}

// FUNCTION: LEGORR 0x00439d50
int legoobject_vehicle_unload_adjacent_cell_find(Lego_Object* obj, int* outCell)
{
    LegoObjF_Point2I blockPos;
    LegoObjF_Point2I offsets[4];
    unsigned int i;

    offsets[0].x = 0;
    offsets[0].y = -1;
    offsets[1].x = 1;
    offsets[1].y = 0;
    offsets[2].x = 0;
    offsets[2].y = 1;
    offsets[3].x = -1;
    offsets[3].y = 0;

    legoobject_position_to_terrain_cell(obj, &blockPos.x, &blockPos.y);
    for (i = 0; i < 4; i++) {
        offsets[i].x += blockPos.x;
        offsets[i].y += blockPos.y;
        if (terrain_cell_flag_8_bounded_current(offsets[i].x, offsets[i].y) &&
            terrain_cell_route_and_occupancy_check_current(NULL, offsets[i].x, offsets[i].y,
                offsets[i].x, offsets[i].y, 0))
        {
            if (outCell != NULL)
                *(LegoObjF_Point2I*)outCell = offsets[i];
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00439e10
int legoobject_vehicle_unload_cell_find_if_eligible(Lego_Object* obj, int* outCell)
{
    if (entity_capability_flags_get(LEGOOBJ_SLOT(obj)) & 0x10)
        return legoobject_vehicle_unload_adjacent_cell_find(obj, outCell);
    return 0;
}

// FUNCTION: LEGORR 0x00439e40
int legoobject_record_flag20_query(Lego_Object* obj, int* outCell)
{
    if (entity_capability_flags_get(LEGOOBJ_SLOT(obj)) & 0x20)
        return legoobject_vehicle_unload_adjacent_cell_find(obj, outCell);
    return 0;
}

/* ---- functions, group G (00439e70-0043a960) ---- */

// FUNCTION: LEGORR 0x00439e70
void legoobject_attached_object_list_detach(Lego_Object* obj, int resetMode)
{
    unsigned int flags;
    unsigned int noRemoveTasks;
    Lego_Object* routeTo;
    unsigned int routeToStats;
    unsigned int i;
    Lego_Object* carried;
    LegoObjF_Point2I blockPos;
    float pos[3];
    float contPos[3];
    void* cont;
    unsigned char* routeFlag;

    flags = obj->flags1;
    noRemoveTasks = flags & 0x200;
    if ((flags & 0x400) && (!(flags & 0x2000) || resetMode)) {
        routeTo = obj->routeToObject;
        routeToStats = 0;
        if (routeTo != NULL)
            routeToStats = entity_stats_word_get(LEGOOBJ_SLOT(routeTo)) & 0x8;
        if (routeToStats == 0 && resetMode) {
            entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 6, NULL, 1);
            entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 9, NULL, 1);
            entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 15, NULL, 1);
            entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 20, NULL, 1);
        }
        for (i = 0; i < obj->numCarriedObjects; i++) {
            carried = obj->carriedObjects[i];
            obj->carriedObjects[i] = NULL;
            carried->carryingThisObject = NULL;
            carried->interactObject = NULL;
            if (routeToStats && resetMode) {
                entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 5, carried, 1);
                legoobject_action_record_consume_and_cleanup(routeTo, carried);
                legoobject_activity_scene_state_update();
            } else {
                legoobject_position_to_terrain_cell(carried, &blockPos.x, &blockPos.y);
                if (!terrain_cell_flag_8_bounded_current(blockPos.x, blockPos.y) ||
                    terrain_cell_state_equals_six_current(&blockPos.x) ||
                    terrain_cell_occupied_or_flag_current(blockPos.x, blockPos.y, 0))
                {
                    cont = legoobject_type_component_get(carried);
                    legoobject_transform_position_get(obj, &pos[0], &pos[1]);
                    ddiscene_object_position_get(cont, NULL, contPos);
                    ddiscene_object_position_set(cont, NULL, pos[0], pos[1], contPos[2]);
                }
                if (resetMode) {
                    entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 7, carried,
                                                  i == obj->numCarriedObjects - 1);
                    if (carried->type != 7 && carried->type != 6)
                        carried->flags1 |= 0x80000;
                } else if (noRemoveTasks) {
                    entity_activity_record_remove_by_type(obj, 3, carried);
                } else {
                    entity_activity_record_remove_by_type(obj, 5, carried);
                    entity_activity_record_remove_by_type(obj, 6, carried);
                    entity_activity_record_remove_by_type(obj, 9, carried);
                    entity_activity_record_remove_by_type(obj, 15, carried);
                    entity_activity_record_remove_by_type(obj, 7, carried);
                    entity_activity_record_remove_by_type(obj, 4, NULL);
                    entity_activity_record_remove_by_type(obj, 20, carried);
                }
                if (carried->type == 5)
                    legoobject_type5_finalize_if_allowed(carried);
            }
        }
        obj->numCarriedObjects = 0;
        obj->flags1 &= ~0x400;
        for (i = 0; i < obj->routeBlocksTotal; i++) {
            routeFlag = &((LegoObjG_RouteBlock*)obj->routeBlocks)[i].actionFlags;
            if (*routeFlag == 7 || *routeFlag == 5)
                *routeFlag = 0;
        }
    }
}

// FUNCTION: LEGORR 0x0043a300
int legoobject_deposit_activity_gate(Lego_Object* obj, Lego_Object* target)
{
    int fromY;
    int* routeX;
    int* routeY;
    int fromX;
    unsigned int count;
    int result;
    int toX;
    int toY;
    int dir;
    void* cont;
    float targetPos[2];
    float pos[2];
    float facing[3];
    float contPos[3];

    if (obj->flags1 & 0x400) {
        if ((entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x4) ||
            (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x2))
        {
            obj->flags1 |= 0x2000;
            legoobject_set_activity(obj, 0x1d, 1);
            legoobject_activity_register_bookkeeping(obj);
            legoobject_activity_tick(obj, 1.0f);
            return 1;
        } else if (legoobject_accepts_target_for_task_check(target, obj->carriedObjects[0], 0) ||
            (entity_stats_word_get(LEGOOBJ_SLOT(target)) & 0x8))
        {
            cont = NULL;
            if (entity_stats_word_get(LEGOOBJ_SLOT(target)) & 0x8)
                cont = scene_object_indexed_secondary_handle_get_or_create(target->building, s_activity_resource_slots[10]);
            else if (target->type == 4)
                cont = scene_object_primary_handle_get_or_create(target->building);

            if (cont != NULL) {
                ddiscene_object_position_get(cont, NULL, contPos);
                ddiscene_object_facing_pair_get(cont, NULL, facing, NULL);
                targetPos[0] = contPos[0];
                targetPos[1] = contPos[1];
                if (fabs(facing[0]) > fabs(facing[1]))
                    dir = (facing[0] > 0.0f) ? 3 : 1;
                else
                    dir = (facing[1] > 0.0f) ? 2 : 0;
            } else {
                legoobject_transform_position_get(target, &targetPos[0], &targetPos[1]);
            }

            terrain_map_world_to_block(Lego_GetMap(), targetPos[0], targetPos[1], &toX, &toY);
            legoobject_transform_position_get(obj, &pos[0], &pos[1]);
            terrain_map_world_to_block(Lego_GetMap(), pos[0], pos[1], &fromX, &fromY);

            if (fromX == toX && fromY == toY) {
                result = legoobject_route_setup_and_marker_associate(obj, 1,
                (unsigned int*)&toX, (unsigned int*)&toY, targetPos);
            } else {
                if (!legoobject_terrain_neighbor_path_check(obj, fromX, fromY, toX, toY,
                        (unsigned int**)&routeX, (unsigned int**)&routeY, (unsigned int*)&count, NULL, NULL))
                    return 0;
                result = legoobject_route_setup_and_marker_associate(obj, count,
                (unsigned int*)routeX, (unsigned int*)routeY, targetPos);
                free(routeX);
                free(routeY);
            }

            if (result) {
                obj->routeToObject = target;
                if (entity_stats_word_get(LEGOOBJ_SLOT(target)) & 0x8) {
                    ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].actionFlags = 9;
                    obj->routeToObject = target;
                    return 1;
                }
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].actionFlags = 7;
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].flags |= 4;
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].flags |= dir & 3;
            }
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0043a5f0
void legoobject_visual_resource_get_or_create(Lego_Object* obj)
{
    void* handle;

    handle = NULL;
    if (obj->type == 4)
        handle = scene_object_primary_handle_get_or_create(obj->building);
    else if (obj->type == 3)
        handle = renderer_cached_handle_50_get_or_create((struct AnimationObject*)obj->rockMonster);
    else if (obj->type == 2)
        handle = renderer_cached_handle_50_get_or_create((struct AnimationObject*)obj->miniFigure);
    else if (obj->type == 1)
        handle = scene_resource_cached_slot_b8_get((struct SceneResourceAnimationRecord*)obj->vehicle);
    if (handle == NULL)
        legoobject_type_component_get(obj);
}

// FUNCTION: LEGORR 0x0043a120
int legoobject_cell_step_command_issue_with_jitter(Lego_Object* obj, int* target, float* input, int mode, int markPending)
{
    int result;
    int* routeX;
    int* routeY;
    int blockX;
    int blockY;
    unsigned int count;
    float offset[2];
    float jitter;
    int side;

    result = 0;
    if (obj->flags1 & 0x400) {
        legoobject_position_to_terrain_cell(obj, &blockX, &blockY);
        if (blockX == target[0] && blockY == target[1]) {
            result = legoobject_route_setup_and_marker_associate(obj, 1,
                (unsigned int*)&target[0], (unsigned int*)&target[1], NULL);
        } else if (legoobject_terrain_neighbor_path_check(obj, blockX, blockY,
                       target[0], target[1], (unsigned int**)&routeX, (unsigned int**)&routeY, (unsigned int*)&count, NULL, NULL)) {
            result = legoobject_route_setup_and_marker_associate(obj, count,
                (unsigned int*)routeX, (unsigned int*)routeY, NULL);
            free(routeX);
            free(routeY);
        }

        if (result) {
            if (obj->type == 1) {
                jitter = random_float_range(0.55f, 0.75f);
                side = rng_thunk_draw15() % 4;
                markPending = 1;
                switch (side) {
                case 0:
                    offset[0] = 0.5f;
                    offset[1] = jitter;
                    break;
                case 1:
                    offset[1] = 0.5f;
                    offset[0] = jitter;
                    break;
                case 2:
                    offset[0] = 0.5f;
                    offset[1] = 1.0f - jitter;
                    break;
                case 3:
                    offset[1] = 0.5f;
                    offset[0] = 1.0f - jitter;
                    break;
                }
            } else {
                offset[0] = input[0];
                offset[1] = input[1];
            }

            if (mode)
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].actionFlags = 9;
            else
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].actionFlags = 8;
            if (input != NULL) {
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].blockOffset[0] = offset[0];
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].blockOffset[1] = offset[1];
            }
            if (markPending)
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].flags |= 8;
        }
    }
    return result;
}

// FUNCTION: LEGORR 0x0043a650
Lego_Object* legoobject_action_kind_create_and_command_issue(Lego_Object* source, int kind, int argument, int last)
{
    int slot;
    void* handle;
    void* model;
    float pos[3];
    Lego_Object* obj;

    if (!legoobject_action_kind_supported(source, kind, argument, last))
        return NULL;

    switch (kind) {
    case 9:
        slot = 8;
        break;
    case 6:
    case 7:
        slot = 10;
        break;
    case 8:
        slot = 9;
        break;
    default:
        slot = 0;
        break;
    }
    handle = scene_object_indexed_secondary_handle_get_or_create(source->building, s_activity_resource_slots[slot]);
    ddiscene_object_position_get(handle, NULL, pos);

    if (kind == 6) {
        model = legoGlobs_contCrystal;
        legoobject_activity_scene_state_update();
        global_object_field8c_subtract(1);
    } else if (kind == 7) {
        model = (&legoGlobs_contOresTable)[argument];
        lego_upgrade_resource_debit(argument, 1);
    } else if (kind == 9) {
        model = legoGlobs_contBarrier;
    } else if (kind == 8) {
        model = legoGlobs_contDynamite;
    } else if (kind == 11) {
        model = legoGlobs_contElectricFence;
    } else if (kind == 13) {
        model = legoGlobs_contOohScary;
    } else {
        return NULL;
    }

    obj = legoobject_create_in_world(model, kind, argument, last, pos[0], pos[1], 0.0f);
    if (kind == 7)
        cmd_produce(0x1c, obj, 0, NULL);
    else
        cmd_produce(0x16, obj, 0, NULL);
    if (kind == 9) {
        // STRING: LEGORR 0x004a507c
        scene_action_name_set(obj->other, "Short");
        return obj;
    }
    if (kind == 8)
        // STRING: LEGORR 0x004a5074
        scene_action_name_set(obj->other, "Normal");
    return obj;
}

// FUNCTION: LEGORR 0x0043a7c0
int legoobject_action_kind_supported(Lego_Object* obj, int kind, int argument, int last)
{
    if (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x8) {
        if (kind == 6 && Lego_GetCrystalCount(1))
            return 1;
        if (kind == 7 && lego_upgrade_resource_check(argument))
            return 1;
        if (kind == 9)
            return 1;
        if (kind == 8)
            return 1;
        if (kind == 11)
            return 1;
        if (kind == 13)
            return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0043a850
void legoobject_action_record_consume_and_cleanup(Lego_Object* obj, Lego_Object* record)
{
    if (obj->type == 3 && obj->field_348 == NULL) {
        obj->field_348 = malloc(6 * sizeof(int));
        memset(obj->field_348, 0, 6 * sizeof(int));
    }
    if (record->type == 6) {
        if (obj->type == 3)
            ((int*)obj->field_348)[record->objLevel]++;
        else
            global_object_field8c_increment();
    } else if (record->type == 7 && obj->type != 3) {
        lego_level_resource_balance_add_and_sound(record->id);
    }
    record->carryingThisObject = NULL;
    legoobject_remove(record);
    if (obj->type != 3)
        terrain_cell_record_counts_prune(obj);
}

// FUNCTION: LEGORR 0x0043a8f0
void legoobject_simulation_update_counter_add(int unused, unsigned int count)
{
    while (count-- > 0)
        global_object_field8c_increment();
    legoobject_activity_scene_state_update();
}

// FUNCTION: LEGORR 0x0043a910
void legoobject_simulation_mode_counter_add(int unused, unsigned int count)
{
    while (count-- > 0)
        lego_level_resource_balance_add_and_sound(0);
}

// FUNCTION: LEGORR 0x0043a930
void legoobject_tracked_pointer_remove(void* pointer)
{
    unsigned int i;

    for (i = 0; i < LegoObjG_liveCount; i++) {
        if (pointer == LegoObjG_liveObjs[i])
            LegoObjG_liveObjs[i] = NULL;
    }
}

// FUNCTION: LEGORR 0x0043a960
void legoobject_lava_crust_activity_register(Lego_Object* obj)
{
    LegoObjF_Point2I blockPos;

    if (obj->type == 1 && !(entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x2000000)) {
        legoobject_position_to_terrain_cell(obj, &blockPos.x, &blockPos.y);
        if (LegoObjG_currLevel->blocks[LegoObjG_currLevel->width * blockPos.y + blockPos.x].terrain == 9 &&
            LegoObjG_liveCount < 100)
        {
            LegoObjG_liveObjs[LegoObjG_liveCount] = obj;
            LegoObjG_liveCount++;
        }
    }
}

/* ---- functions, group H (0043a9e0-0043bbc0; part 2, issue #588) ---- */

/* Registers a lava-crust object (LegoObject_QueueForRockfall wrapper). */
// FUNCTION: LEGORR 0x0043a9e0
void legoobject_queue_for_rockfall_processing_if_eligible(Lego_Object* obj)
{
    legoobject_lava_crust_activity_register(obj);
}

/* A free, healthy object starts activityName[0x24] (once) when the other
 * object carries a power crystal (type 6); both block positions are read
 * and unused. */
// FUNCTION: LEGORR 0x0043a9f0
void legoobject_pair_interaction_gate(Lego_Object* obj, Lego_Object* other)
{
    int blockPos[2];
    int otherPos[2];

    if (!(obj->flags1 & 0x400) && !(obj->flags2 & 1) && !(obj->flags1 & 0x1000100) &&
        obj->health > legoobject_zero && (other->flags1 & 0x400) &&
        other->carriedObjects[0]->type == 6)
    {
        legoobject_position_to_terrain_cell(obj, &blockPos[0], &blockPos[1]);
        legoobject_position_to_terrain_cell(other, &otherPos[0], &otherPos[1]);
        if (s_activity[0x24] != obj->activityName1) {
            legoobject_set_activity(obj, 0x24, 0);
            legoobject_activity_register_bookkeeping(obj);
            obj->flags2 |= 0x200000;
        }
    }
}

/* A creature that can flee and is hurt below its threshold starts
 * activityName[0x24] when it stands near the centre of a solid block. */
// FUNCTION: LEGORR 0x0043aab0
void legoobject_proximity_activity_raise(Lego_Object* obj)
{
    int blockPos[2];
    float pos[2];
    float wPos[2];
    float dist;

    if (obj->type == 3 && (entity_state_flags_get(LEGOOBJ_SLOT(obj)) & 0x4000) &&
        entity_stats_float_ate8(LEGOOBJ_SLOT(obj)) < obj->health &&
        s_activity[0x24] != obj->activityName1 && !(obj->flags1 & 0x400))
    {
        legoobject_position_to_terrain_cell(obj, &blockPos[0], &blockPos[1]);
        if (level_cell_occupied_at_current(blockPos) && !terrain_cell_flag_8000_current(blockPos)) {
            legoobject_transform_position_get(obj, &pos[0], &pos[1]);
            terrain_map_block_to_world(Lego_GetMap(), blockPos[0], blockPos[1], &wPos[0], &wPos[1]);
            dist = (float)sqrt((pos[0] - wPos[0]) * (pos[0] - wPos[0]) +
                               (pos[1] - wPos[1]) * (pos[1] - wPos[1]));
            if (dist < terrain_context_scale_get(Lego_GetMap()) * 0.33333334f) {
                legoobject_set_activity(obj, 0x24, 0);
                legoobject_activity_register_bookkeeping(obj);
                obj->flags2 |= 0x200000;
            }
        }
    }
}

/* At the object's block: a random variant scatter one time in five, and
 * on a solid block its marker, work-flag clear and cell update; then every
 * minifigure carrying a power crystal within the object's range drops
 * it. */
// FUNCTION: LEGORR 0x0043abf0
void legoobject_terrain_cell_variant_update_queue(Lego_Object* obj)
{
    int blockPos[2];

    legoobject_position_to_terrain_cell(obj, &blockPos[0], &blockPos[1]);
    if (rng_thunk_draw15() % 5 == 0)
        legoobject_gh_random_variant_cell_scatter_adapter(blockPos, 3, 1);
    if (level_cell_occupied_at_current(blockPos)) {
        scene_marker_create_at_position_or_object(obj, NULL);
        terrain_cell_work_flag_clear_current(blockPos, 0x3f);
        lego_cell_init_pass((struct Lego_Level*)g_active_level_retail, blockPos[0], blockPos[1], 0);
    }
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_terrain_cell_variant_visit_callback, obj);
}

/* 0043abf0's callback: a minifigure carrying a power crystal (type 6)
 * within the source object's range (stats +0x94) drops it. */
// FUNCTION: LEGORR 0x0043ac90
int legoobject_terrain_cell_variant_visit_callback(Lego_Object* obj, void* data)
{
    Lego_Object* source;
    float pos[2];
    float srcPos[2];

    if (obj->type == 2 && (obj->flags1 & 0x400) && obj->carriedObjects[0]->type == 6) {
        source = (Lego_Object*)data;
        legoobject_transform_position_get(source, &srcPos[0], &srcPos[1]);
        legoobject_transform_position_get(obj, &pos[0], &pos[1]);
        if ((float)sqrt((pos[0] - srcPos[0]) * (pos[0] - srcPos[0]) +
                        (pos[1] - srcPos[1]) * (pos[1] - srcPos[1])) <
            entity_stats_float_at94(LEGOOBJ_SLOT(source)))
        {
            legoobject_attached_object_list_detach(obj, 0);
            legoobject_action_sequence_reset(obj, 0);
        }
    }
    return 0;
}

/* A creature (rock monster model id) emerges 12.75 units from a random
 * objectGlobs predug block (004e844c), or from the one nearest originObj
 * (0044a4f0). */
// FUNCTION: LEGORR 0x0043ad50
Lego_Object* legoobject_predug_spawn_coordinate_select_and_create(Lego_Object* originObj, int id)
{
    Lego_Object* obj;
    float heading;
    float wPos[2];
    int blockPos[2];
    float dir[2];
    unsigned int index;

    obj = NULL;
    if (s_predug_coordinate_count != 0) {
        heading = random_float_range(0.0f, 6.2831855f);
        dir[0] = (float)sin(heading);
        dir[1] = (float)cos(heading);
        if (originObj != NULL) {
            legoobject_nearest_cell_coordinate_world_resolve(originObj, blockPos);
        } else {
            index = rng_thunk_draw15() % s_predug_coordinate_count;
            blockPos[0] = s_predug_coordinates[index][0];
            blockPos[1] = s_predug_coordinates[index][1];
        }
        terrain_map_block_to_world(Lego_GetMap(), blockPos[0], blockPos[1], &wPos[0], &wPos[1]);
        wPos[0] = (float)(dir[0] * 12.75f) + wPos[0];
        wPos[1] = (float)(dir[1] * 12.75f) + wPos[1];
        obj = legoobject_create_in_world((LegoObjH_CreatureModel*)legoGlobs_rockMonsterData + id,
                                         3, id, 0, wPos[0], wPos[1], heading);
        if (obj != NULL) {
            obj->flags1 |= 0x100;
            obj->flags3 &= 0x7fffffff;
            legoobject_set_activity(obj, 0xc, 0);
            legoobject_activity_register_bookkeeping(obj);
            aitask_ai_idle_chain_record_attach(LEGOOBJ_SLOT(obj));
            info_send(0x1f, NULL, obj, NULL);
        }
    }
    return obj;
}

/* LegoObject_TryGenerateRMonsterAtRandomBlock: up to 1000 random blocks. */
// FUNCTION: LEGORR 0x0043aea0
Lego_Object* legoobject_emergence_random_spawn_attempt(void)
{
    int id;
    int width;
    unsigned int i;
    int height;
    int by;
    int bx;
    Lego_Object* obj;

    id = active_engine_field_b0_get_current();
    width = ((LegoObjH_Map*)Lego_GetMap())->blockWidth;
    height = ((LegoObjH_Map*)Lego_GetMap())->blockHeight;
    for (i = 0; i < 1000; i++) {
        bx = rng_thunk_draw15() % width;
        by = rng_thunk_draw15() % height;
        obj = legoobject_creature_spawn_from_emergence(
            (LegoObjH_CreatureModel*)legoGlobs_rockMonsterData + id, 3, id, bx, by);
        if (obj != NULL)
            return obj;
    }
    return NULL;
}

/* LegoObject_TryGenerateRMonster: a rock monster emerges from a wall block
 * onto its first open neighbour, facing out. */
// FUNCTION: LEGORR 0x0043af30
Lego_Object* legoobject_creature_spawn_from_emergence(void* model, int type, int id, int bx, int by)
{
    float nx;
    float ny;
    float wx;
    float wy;
    unsigned int i;
    Lego_Object* obj;
    int found;
    int objType;
    int objID;
    int blockPos[2];
    float headings[4];
    int dirs[4][2];

    blockPos[0] = bx;
    blockPos[1] = by;
    if (type == 3 && terrain_cell_walkable_flag_current(bx, by) &&
        !terrain_cell_flag_20_bounded_current(bx, by) && !terrain_cell_mask_c0_current(bx, by) &&
        !terrain_cell_state_equals_one_current(blockPos) && !terrain_cell_flag_reserved_current(blockPos))
    {
        dirs[0][0] = 0;
        dirs[0][1] = -1;
        dirs[1][0] = 1;
        dirs[1][1] = 0;
        dirs[2][0] = 0;
        dirs[2][1] = 1;
        dirs[3][0] = -1;
        dirs[3][1] = 0;
        headings[0] = 0.0f;
        headings[1] = 1.5707964f;
        headings[2] = 3.1415927f;
        headings[3] = 4.712389f;
        for (i = 0; i < 4; i++) {
            dirs[i][0] += bx;
            dirs[i][1] += by;
            if (terrain_cell_flag_8_bounded_current(dirs[i][0], dirs[i][1]) &&
                !terrain_cell_mask_180000_current(dirs[i]) &&
                terrain_cell_route_and_occupancy_check_current(NULL, dirs[i][0], dirs[i][1],
                                                               dirs[i][0], dirs[i][1], 0))
                break;
        }
        if (i < 4) {
            terrain_map_block_to_world(Lego_GetMap(), bx, by, &wx, &wy);
            terrain_map_block_to_world(Lego_GetMap(), dirs[i][0], dirs[i][1], &nx, &ny);
            nx = ((float)(nx * 10.0f) + (float)(wx * 9.0f)) * (1.0f / 19.0f);
            ny = ((float)(ny * 10.0f) + (float)(wy * 9.0f)) * (1.0f / 19.0f);
            obj = legoobject_create_in_world(model, 3, id, 0, nx, ny, headings[i]);
            obj->flags1 |= 0x100;
            legoobject_dispatch_event31_for_pending_flag(obj);
            legoobject_set_activity(obj, 0xc, 0);
            legoobject_activity_register_bookkeeping(obj);
            aitask_ai_idle_chain_record_attach(LEGOOBJ_SLOT(obj));
            sound_submission_route_wrapper(legoobject_type_component_get(obj), 10, 0, 1, NULL);
            level_tile_mark_current_target(blockPos, 1);
            obj->targetBlockPos[0] = (float)blockPos[0];
            obj->targetBlockPos[1] = (float)blockPos[1];

            found = 0;
            // STRING: LEGORR 0x004a509c
            if (entity_type_pair_lookup("RockMonster", &objType, &objID, NULL) &&
                objType == obj->type && objID == obj->id)
            {
                info_send(2, NULL, obj, NULL);
                found = 1;
            }
            if (!found) {
                // STRING: LEGORR 0x004a5090
                if (entity_type_pair_lookup("Lavamonster", &objType, &objID, NULL) &&
                    objType == obj->type && objID == obj->id)
                {
                    info_send(3, NULL, obj, NULL);
                    found = 1;
                }
                if (!found) {
                    // STRING: LEGORR 0x004a5084
                    if (entity_type_pair_lookup("Icemonster", &objType, &objID, NULL) &&
                        objType == obj->type && objID == obj->id)
                    {
                        info_send(4, NULL, obj, NULL);
                        found = 1;
                    }
                }
            }
            if (found)
                info_send(0x27, NULL, obj, NULL);
            global_counter_at_00555324_increment();
            return obj;
        }
    }
    return NULL;
}

/* objectGlobs.flags bits: 4 the level's blocks changed, 8 an activity
 * changed, 0x10 updating. Every 125 seconds the level's periodic pass
 * runs. */
// FUNCTION: LEGORR 0x0043b270
void legoobject_update_all(float elapsed)
{
    if (s_entity_state_flags & 4)
        lego_terrain_cell_previous_state_track();
    s_entity_state_flags |= 0x10;
    legoobject_run_through_lists(legoobject_update_callback, &elapsed, 0);
    s_entity_state_flags &= ~0x10;
    if (s_entity_state_flags & 4)
        lego_terrain_cell_update_queue_flush();
    if (s_entity_state_flags & 8) {
        legoobject_activity_scene_state_update();
        s_entity_state_flags &= ~8;
    } else {
        s_entity_state_flags &= ~4;
    }
    legoobject_timed_callback_list_update(&elapsed);
    objectGlobs_f_4eadc8 -= elapsed;
    if (objectGlobs_f_4eadc8 <= legoobject_zero) {
        objectGlobs_f_4eadc8 = 125.0f;
        lego_periodic_callback_dispatch(1);
    }
}

// FUNCTION: LEGORR 0x0043b320
void legoobject_remove_all(void)
{
    s_entity_state_flags |= 2;
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_remove_all_callback, NULL);
    s_entity_state_flags &= ~2;
}

// FUNCTION: LEGORR 0x0043b350
int legoobject_remove_all_callback(Lego_Object* obj, void* data)
{
    return legoobject_remove(obj);
}

/* Finds the object under the screen position (x, y). */
// FUNCTION: LEGORR 0x0043b360
int legoobject_pick_at_screen_position(unsigned int x, unsigned int y, Lego_Object** outObj)
{
    LegoObjH_PickSearch search;

    search.result = outObj;
    search.x = (float)x;
    search.y = (float)y;
    *outObj = NULL;
    return legoobject_run_through_lists_skip_upgrade_parts(legoobject_pick_at_screen_position_callback, &search);
}

/* 0043b360's callback: casts the pick ray through the main view and tests
 * it against the object's collision sphere, or a building's every shape
 * block. */
// FUNCTION: LEGORR 0x0043b3b0
int legoobject_pick_at_screen_position_callback(Lego_Object* obj, void* data)
{
    unsigned int count;
    unsigned int shapeCount;
    unsigned int i;
    void* view;
    float screen[4];
    float dir[3];
    float origin[3];
    int blockPos[2];
    float ray[3];
    float far_[3];
    float points[10][3];
    LegoObjH_PickSearch* search;
    int rotation;
    LegoObjF_Point2I* shape;
    float len;

    search = (LegoObjH_PickSearch*)data;
    view = legoGlobs_viewMain;
    screen[0] = search->x;
    screen[1] = search->y;
    screen[2] = 0.0f;
    screen[3] = 1.0f;
    if (!legoobject_shared_eligibility_check(obj, 1))
        return 0;

    legoobject_position_to_terrain_cell(obj, &blockPos[0], &blockPos[1]);
    legoobject_transform_position_get(obj, &points[0][0], &points[0][1]);
    points[0][2] = terrain_map_world_z_get(Lego_GetMap(), points[0][0], points[0][1]);
    points[0][2] -= entity_stats_float_at74(LEGOOBJ_SLOT(obj)) * 0.5f;
    if (obj->type == 4) {
        ddiscene_object_facing_pair_get(legoobject_type_component_get(obj), NULL, dir, NULL);
        if (fabs(dir[0]) > fabs(dir[1])) {
            if (dir[0] > legoobject_zero)
                rotation = 1;
            else
                rotation = 3;
        } else {
            if (dir[1] > legoobject_zero)
                rotation = 0;
            else
                rotation = 2;
        }
        shape = (LegoObjF_Point2I*)task_field84_get_and_field88_out((LegoObjH_BuildingModel*)legoGlobs_buildingData + obj->id,
                                                                     &shapeCount);
        shape = (LegoObjF_Point2I*)rotated_pairs_scratch_fill(blockPos, (int*)shape, shapeCount, rotation);
        count = 0;
        for (i = 0; i < shapeCount; i++) {
            if (i == shapeCount - 1 || shape[i].x != shape[i + 1].x || shape[i].y != shape[i + 1].y) {
                terrain_map_block_to_world(Lego_GetMap(), shape[i].x, shape[i].y,
                                           &points[count][0], &points[count][1]);
                points[count][2] = terrain_map_world_z_get(Lego_GetMap(), points[count][0], points[count][1]);
                points[count][2] -= entity_stats_float_at74(LEGOOBJ_SLOT(obj)) * 0.5f;
                count++;
            } else {
                i++;
            }
        }
    } else {
        count = 1;
    }

    camera_screen_coordinate_project((struct ViewProjectionAdapter*)view, origin, screen);
    screen[2] = 1.0f;
    camera_screen_coordinate_project((struct ViewProjectionAdapter*)view, far_, screen);
    ray[0] = far_[0] - origin[0];
    ray[1] = far_[1] - origin[1];
    ray[2] = far_[2] - origin[2];
    len = 1.0f / (float)sqrt(ray[0] * ray[0] + ray[1] * ray[1] + ray[2] * ray[2]);
    ray[0] *= len;
    ray[1] *= len;
    ray[2] *= len;
    for (i = 0; i < count; i++) {
        if (sphere_ray_intersection_discriminant_positive(points[i],
                entity_stats_float_at78(LEGOOBJ_SLOT(obj)), origin, ray))
        {
            *search->result = obj;
            return 1;
        }
    }
    return 0;
}

/* Selects every object whose screen position lies in the rectangle the two
 * corners span (unless the tutorial blocks selection). */
// FUNCTION: LEGORR 0x0043b6c0
void legoobject_select_in_screen_rect(void* viewport, float* corner1, float* corner2)
{
    LegoObjH_RectSearch search;

    if (!(LegoObjH_NERPsTutorialFlags(0) & 8)) {
        search.viewport = viewport;
        search.left = (corner1[0] < corner2[0]) ? corner1[0] : corner2[0];
        search.top = (corner2[1] > corner1[1]) ? corner1[1] : corner2[1];
        search.right = (corner1[0] > corner2[0]) ? corner1[0] : corner2[0];
        search.bottom = (corner2[1] >= corner1[1]) ? corner2[1] : corner1[1];
        legoobject_run_through_lists_skip_upgrade_parts(legoobject_select_in_screen_rect_callback, &search);
        cmd_produce(0x2d, NULL, 0, NULL);
    }
}

// FUNCTION: LEGORR 0x0043b770
int legoobject_select_in_screen_rect_callback(Lego_Object* obj, void* data)
{
    LegoObjH_RectSearch* search;
    void* cont;
    float position[3];
    float screen[4];

    cont = legoobject_type_component_get(obj);
    ddiscene_object_position_get(cont, NULL, position);
    search = (LegoObjH_RectSearch*)data;
    render_interface_transform_point(search->viewport, screen, position);
    screen[0] = screen[0] / screen[3];
    screen[1] = screen[1] / screen[3];
    if (screen[0] >= search->left && screen[1] >= search->top &&
        screen[0] <= search->right && screen[1] <= search->bottom)
    {
        cmd_produce(1, obj, 1, NULL);
    }
    return 0;
}

/* Switches a minifigure's (or a driven vehicle's driver's) first-person
 * meshes. */
// FUNCTION: LEGORR 0x0043b820
void legoobject_first_person_meshes_swap(Lego_Object* obj, unsigned int camera, int swap)
{
    if (obj->type == 2 || ((obj = obj->driveObject) != NULL && obj->type == 2))
        renderer_object_update_from_record_field6c((struct AnimationObject*)obj->miniFigure, swap, camera);
}

/* Switches every other object's medium/high meshes by its distance from the
 * listener's container (or back, without one). */
// FUNCTION: LEGORR 0x0043b850
void legoobject_lod_meshes_update(Lego_Object* obj, void* cont, float mediumDist, float highDist, int usePosition)
{
    LegoObjH_LodSearch search;

    search.obj = obj;
    search.usePosition = usePosition;
    search.mediumDist = mediumDist;
    search.highDist = highDist;
    memset(search.position, 0, sizeof(search.position));
    if (obj->type != 2 && obj->driveObject != NULL && obj->driveObject->type == 2)
        search.obj = obj->driveObject;
    if (usePosition)
        ddiscene_object_position_get(cont, NULL, search.position);
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_lod_meshes_update_callback, &search);
}

// FUNCTION: LEGORR 0x0043b8d0
int legoobject_lod_meshes_update_callback(Lego_Object* obj, void* data)
{
    LegoObjH_LodSearch* search;
    void* cont;
    float position[3];
    float dist;

    search = (LegoObjH_LodSearch*)data;
    if (obj != search->obj) {
        if (search->usePosition) {
            if (obj->field_34c == 0) {
                cont = legoobject_type_component_get(obj);
                ddiscene_object_position_get(cont, NULL, position);
                dist = (float)sqrt((search->position[0] - position[0]) * (search->position[0] - position[0]) +
                                   (search->position[1] - position[1]) * (search->position[1] - position[1]) +
                                   (search->position[2] - position[2]) * (search->position[2] - position[2]));
                if (dist <= search->highDist) {
                    if (obj->type == 2)
                        renderer_object_update_from_record_field68((struct AnimationObject*)obj->miniFigure, 1);
                    else if (obj->type == 3)
                        renderer_object_update_from_record_field68((struct AnimationObject*)obj->rockMonster, 1);
                    else if (obj->type == 1)
                        scene_entity_animation_tracks_set_enabled((struct ScenePairedAnimationState*)obj->vehicle, 1);
                    obj->field_34c = 2;
                } else if (dist <= search->mediumDist) {
                    if (obj->type == 2)
                        renderer_object_update_from_record_field64((struct AnimationObject*)obj->miniFigure, 1);
                    else if (obj->type == 3)
                        renderer_object_update_from_record_field64((struct AnimationObject*)obj->rockMonster, 1);
                    else if (obj->type == 1)
                        paired_resource_state_set((struct ScenePairedAnimationState*)obj->vehicle, 1);
                    obj->field_34c = 1;
                }
            }
        } else {
            if (obj->field_34c == 1) {
                if (obj->type == 2)
                    renderer_object_update_from_record_field64((struct AnimationObject*)obj->miniFigure, 0);
                else if (obj->type == 3)
                    renderer_object_update_from_record_field64((struct AnimationObject*)obj->rockMonster, 0);
                else if (obj->type == 1)
                    paired_resource_state_set((struct ScenePairedAnimationState*)obj->vehicle, 0);
            } else if (obj->field_34c == 2) {
                if (obj->type == 2)
                    renderer_object_update_from_record_field68((struct AnimationObject*)obj->miniFigure, 0);
                else if (obj->type == 3)
                    renderer_object_update_from_record_field68((struct AnimationObject*)obj->rockMonster, 0);
                else if (obj->type == 1)
                    scene_entity_animation_tracks_set_enabled((struct ScenePairedAnimationState*)obj->vehicle, 0);
            }
            obj->field_34c = 0;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0043baf0
int legoobject_flags_mask107c33de_or_1c00203_any_test(Lego_Object* obj)
{
    if (obj->flags1 & 0x107c33de)
        return 1;
    return (obj->flags2 & 0x1c00203) != 0;
}

/* A building that can hold more (+0x324 below its capacity) clears its
 * full flag; a full deposit building starts its deposit activity. */
// FUNCTION: LEGORR 0x0043bb20
void legoobject_building_deposit_state_monitor(Lego_Object* obj)
{
    unsigned int capacity;

    if (obj->type == 4) {
        if (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 2) {
            capacity = (obj->carryNullFrames < stats_instance_word_at_ec(obj))
                ? obj->carryNullFrames : stats_instance_word_at_ec(obj);
        } else {
            capacity = obj->numCarriedObjects;
        }
        if (obj->numCarriedObjects < capacity || (obj->flags1 & 0x2000) ||
            (obj->flags3 & 0x1000000))
        {
            obj->flags3 &= ~0x1000000;
        } else if ((entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 4) ||
                   (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 2))
        {
            legoobject_deposit_activity_gate(obj, NULL);
        }
    }
}

/* A rock monster drops the power crystal it carries and is flagged for
 * removal (+0x3e8 0x800000) at health -1. */
// FUNCTION: LEGORR 0x0043bbc0
void legoobject_detach_carried_object_and_reset_state(Lego_Object* obj)
{
    Lego_Object* carried;

    if (obj->type == 3) {
        carried = obj->carriedObjects[0];
        if (carried != NULL && carried->type == 6) {
            obj->carriedObjects[0] = NULL;
            obj->numCarriedObjects = 0;
            carried->carryingThisObject = NULL;
            obj->flags1 &= ~0x400;
            carried->health = -1.0f;
        }
        if (obj->field_348 != NULL)
            level_state_field9c_add(*(int*)obj->field_348);
        obj->health = -1.0f;
        obj->flags3 |= 0x800000;
    }
}


/* ---- functions, group I1 (0043bc40-0043c6b0; part 2, issue #588) ---- */

/* The removal and teleport-out step of an object: once flagged for
 * removal (+0x3e8 0x800000) the object is detached and removed; otherwise,
 * at zero health, a teleportable object starts its teleport-up effect. */
// FUNCTION: LEGORR 0x0043bc40
void legoobject_objective_sidecar_append_gate(Lego_Object* obj)
{
    int cell[2];
    const char* activityName;
    void* cont;
    float scale;
    int blockPos[2];

    if (obj->type != 4) {
        if (obj->flags2 & 0x8000000)
            return;
        if (obj->flags1 & 0x200000)
            return;
    }
    if (obj->health > legoobject_zero)
        return;

    if ((entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x80000070) && obj->field_3cc != NULL &&
        (obj->field_3cc->flags1 & 0x200000))
    {
        obj->field_3cc->health = -1.0f;
        obj->field_3cc->flags1 &= ~0x200000;
        legoobject_kind_record_update_dispatch(obj);
        legoobject_set_activity(obj->field_3cc, 0, 1);
        legoobject_activity_register_bookkeeping(obj->field_3cc);
    }

    if (obj->flags3 & 0x800000) {
        if (obj->driveObject != NULL) {
            interface_driven_object_menu_check((struct LegoObject*)obj);
            obj->driveObject->flags2 &= ~0x8;
            obj->driveObject->driveObject = NULL;
            obj->driveObject = NULL;
        }
        if (obj->type == 2 && (obj->flags4 & 0x8))
            legoobject_gi_osf_add_adapter(obj);
        if (obj->type == 1)
            scene_record_indexed_entries_rebuild_original(obj->vehicle, 0);
        legoobject_kind_one_random_spawn_trigger(obj);
        if (obj->type == 5) {
            legoobject_type5_finalize_if_allowed(obj);
            return;
        }
        if (obj->type == 4) {
            entity_destroy_tracked_pointer_cleanup(obj);
            return;
        }
        legoobject_remove(obj);
        return;
    }

    if (obj->type == 3) {
        if (entity_state_flags_get(LEGOOBJ_SLOT(obj)) & 0x8000) {
            if (obj->flags1 & 0x100000)
                return;
            legoobject_action_route_state_initialize(obj);
            return;
        }
        if (!(entity_state_flags_get(LEGOOBJ_SLOT(obj)) & 0x100000))
            return;
        legoobject_state_reset(obj, 0, 1);
        obj->flags1 |= 0x20000;
        legoobject_set_activity(obj, 0xd, 0);
        legoobject_activity_register_bookkeeping(obj);
        return;
    }

    if (obj->type == 2 || obj->type == 1 || obj->type == 11 ||
        (obj->type == 4 && ((LEGOOBJ_FIELD(obj, unsigned char, 0x390) & 4) || (obj->flags4 & 0x8))))
    {
        if (!(obj->flags3 & 0x40000))
            return;
        if (obj->flags1 & 0x400000)
            return;
        activityName = obj->activityName1;
        cont = legoobject_type_component_get(obj);
        if (obj->flags4 & 0x8)
            legoobject_kind_one_random_spawn_trigger(obj);
        if (obj->type == 4)
            legoobject_gi_cell_path_effects_apply_adapter(obj);
        if (obj->flags4 & 0x80) {
            if (obj->routeToObject != NULL)
                obj->routeToObject->flags4 &= ~0x80;
            obj->routeToObject = NULL;
        }
        if (obj->driveObject != NULL) {
            legoobject_position_to_terrain_cell(obj, &cell[0], &cell[1]);
            if ((entity_capability_flags_get(LEGOOBJ_SLOT(obj)) & 1) && (obj->flags1 & 0x400) &&
                obj->carriedObjects[0] != NULL &&
                !terrain_cell_route_and_occupancy_check_current(LEGOOBJ_SLOT(obj->carriedObjects[0]), cell[0], cell[1], cell[0], cell[1], 1))
            {
                obj->carriedObjects[0]->health = -1.0f;
            }
            if ((entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x20000) &&
                !terrain_cell_route_and_occupancy_check_current(LEGOOBJ_SLOT(obj->driveObject), cell[0], cell[1], cell[0], cell[1], 1))
            {
                obj->driveObject->health = -1.0f;
            } else {
                interface_driven_object_menu_check((struct LegoObject*)obj);
                obj->driveObject->flags2 &= ~0x8;
                if (obj->driveObject->type == 2)
                    activity_runner_update((struct AnimationObject*)obj->driveObject->miniFigure, s_activity[0], 0.0f);
                obj->driveObject->driveObject = NULL;
                obj->driveObject = NULL;
            }
        }
        if (obj->type == 11)
            electric_fence_remove((struct ElectricFenceObject*)obj);
        legoobject_position_to_terrain_cell(obj, &blockPos[0], &blockPos[1]);
        if (obj->type == 2 || obj->type == 1 || obj->type == 4)
            info_send(0x26, NULL, NULL, (struct InfoPoint2I*)blockPos);
        if (obj->type == 2)
            info_send(10, NULL, NULL, (struct InfoPoint2I*)blockPos);
        else if (obj->type == 1)
            info_send(11, NULL, NULL, (struct InfoPoint2I*)blockPos);
        else if (obj->type == 4)
            info_send(12, NULL, NULL, (struct InfoPoint2I*)blockPos);
        if (obj->flags2 & 0x2) {
            legoobject_state_reset(obj, 1, 1);
        } else {
            legoobject_state_reset(obj, 0, 1);
            scene_action_name_set(cont, activityName);
            obj->activityName2 = obj->activityName1;
        }
        obj->flags1 |= 0x400000;
        obj->activityCont = entity_visual_resource_load(legoGlobs_contMiniTeleportUp);
        ddiscene_frame_update_and_callbacks(obj->activityCont, 0.0f);
        scene_render_lock_toggle(obj->activityCont, 0);
        ddiscene_object_position_set(obj->activityCont, legoobject_type_component_get(obj), 0.0f, 0.0f, 0.0f);
        ddiscene_object_pose_method_dispatch(obj->activityCont, NULL, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f);
        if (obj->type == 4) {
            scale = entity_stats_float_at6c(LEGOOBJ_SLOT(obj));
            if (scale <= legoobject_zero)
                scale = 4.0f;
            if (scale > legoobject_zero) {
                scale *= 0.25f;
                scene_frame_scale_set(obj->activityCont, 1, scale, 1.0f, scale);
            }
        }
    } else if (obj->type == 4) {
        if (obj->flags2 & 0x100000)
            return;
        obj->flags2 |= 0x100000;
        legoobject_gi_cell_path_effects_apply_adapter(obj);
        legoobject_set_activity(obj, 0x2a, 0);
        legoobject_activity_register_bookkeeping(obj);
        aitask_task_registry_remove_type10_entries_for_entity(LEGOOBJ_SLOT(obj));
    } else {
        if (obj->type == 6)
            obj->flags3 |= 0x800000;
        return;
    }
    selection_entity_remove_active(LEGOOBJ_SLOT(obj));
    selection_object_detach(obj);
}

/* The stats float at +0xdc of (type, id), plus every live object's, plus
 * that of (type2, id2): nonzero while the sum stays positive, or when the
 * first object's own entry is zero. */
// FUNCTION: LEGORR 0x0043c200
int legoobject_state_pair_predicate(int type, int id, int type2, int id2)
{
    float sum;

    if (stats_table_float_at_dc(type, id) != legoobject_zero) {
        sum = 0.0f;
        legoobject_run_through_lists_skip_upgrade_parts(legoobject_nested_float_add_unless_blocked, &sum);
        if (stats_table_float_at_dc(type, id) + stats_table_float_at_dc(type2, id2) + sum <= legoobject_zero)
            return 0;
    }
    return 1;
}

/* The LegoObject_RunThroughLists callback of 0043c200. */
// FUNCTION: LEGORR 0x0043c280
int legoobject_nested_float_add_unless_blocked(Lego_Object* obj, void* data)
{
    if (obj->type != 4 || legoobject_shared_eligibility_check(obj, 0))
        *(float*)data += entity_stats_float_atdc(LEGOOBJ_SLOT(obj));
    return 0;
}

// FUNCTION: LEGORR 0x0043c2b0
void legoobject_periodic_timer25_advance(float elapsed, int renderTrack)
{
    objectGlobs_f_4dee0c += elapsed;
    if (objectGlobs_f_4dee0c > 25.0f)
        objectGlobs_f_4dee0c = 0.0f;
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_periodic_timer25_callback, &renderTrack);
}

/* The LegoObject_RunThroughLists callback of 0043c2b0. */
// FUNCTION: LEGORR 0x0043c2f0
int legoobject_periodic_timer25_callback(Lego_Object* obj, void* data)
{
    float fraction;
    int radius;

    fraction = objectGlobs_f_4dee0c * 0.04f;
    if ((obj->abilityFlags & 0x20) && (radius = entity_stats_word_atac(LEGOOBJ_SLOT(obj))) != 0) {
        if (obj->type != 4 || legoobject_shared_eligibility_check(obj, 0)) {
            if (legoobject_zero == objectGlobs_f_4dee0c) {
                int blockPos[2];
                legoobject_position_to_terrain_cell(obj, &blockPos[0], &blockPos[1]);
                terrain_cell_work_flag_circle_set_current(blockPos, radius);
                return 0;
            }
            if (*(int*)data) {
                float position[2];
                legoobject_transform_position_get(obj, &position[0], &position[1]);
                textured_scene_object_draw_at_position(
                    (void*)(size_t)global_state_field84_get_current_target(), position,
                    radius * (terrain_context_scale_get(Lego_GetMap()) * fraction),
                    1.0f - fraction);
            }
        }
    }
    return 0;
}

/* A type-2 object (a minifigure) whose current task is of type 0x1a. The
 * original compares the type as `type - 2 == 0` (sub, not cmp) in both
 * functions that test this, as a switch or a shared helper would. */
#define LEGOOBJ_I_GATHERING(obj) \
    ((obj) != NULL && (obj)->type - 2 == 0 && (obj)->aiTask != NULL && \
     ((LegoObjI_AITask*)(obj)->aiTask)->taskType == 0x1a)

/* The object (a type 2) is running a task of type 0x1a whose target is
 * within reach and not behind a wall. */
// FUNCTION: LEGORR 0x0043c3e0
int legoobject_type26_command_completion_gate(Lego_Object* obj)
{
    if (LEGOOBJ_I_GATHERING(obj) &&
        entity_centers_within_configured_radius(LEGOOBJ_SLOT(obj),
            LEGOOBJ_SLOT(((LegoObjI_AITask*)obj->aiTask)->targetObject)) &&
        !object_pair_ray_hit_check(obj, ((LegoObjI_AITask*)obj->aiTask)->targetObject))
    {
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0043c440
int legoobject_can_do_activity_classify(Lego_Object* obj)
{
    if (legoobject_activity_scan(obj, 4))
        return 1;
    if (legoobject_activity_scan(obj, 5))
        return 2;
    return legoobject_activity_scan(obj, 7) ? 3 : 0;
}

/* 004385b0 in relaxed mode, and the condition. Part 1 pulled it forward
 * (finding 572); part 2 puts it back in address order. */
// FUNCTION: LEGORR 0x0043c490
int legoobject_task_type_and_condition_check(void* unused, Lego_Object* target, int condition)
{
    if (legoobject_shared_eligibility_check(target, 1) && condition)
        return 1;
    return 0;
}

// FUNCTION: LEGORR 0x0043c4c0
void legoobject_finish_state_1a(Lego_Object* obj)
{
    int kind;

    if (obj->routeToObject != NULL) {
        kind = legoobject_can_do_activity_classify(obj);
        if (legoobject_task_type_and_condition_check(obj, obj->routeToObject, kind))
            legoobject_face_target_action_start(obj, obj->routeToObject, kind);
    }
    obj->flags2 &= ~0x4000000;
    aitask_entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 0x1a, NULL, 1);
}

// FUNCTION: LEGORR 0x0043c530
void legoobject_mark_and_finish_state_1a(Lego_Object* obj)
{
    if (LEGOOBJ_I_GATHERING(obj)) {
        obj->flags2 |= 0x4000000;
        legoobject_finish_state_1a(obj);
    }
}

// FUNCTION: LEGORR 0x0043c570
void legoobject_kind_four_post_create_resolve(Lego_Object* obj)
{
    int crystals;
    int blockPos[2];

    if (obj->type == 4 && !(obj->flags3 & 0x80000000)) {
        crystals = stats_instance_word_at_bc(obj);
        legoobject_position_to_terrain_cell(obj, &blockPos[0], &blockPos[1]);
        if (!(entity_state_flags_get(LEGOOBJ_SLOT(obj)) & 0x200) &&
            !(entity_state_flags_get(LEGOOBJ_SLOT(obj)) & 0x400) &&
            !construction_find_resource_object(blockPos, crystals))
        {
            obj->flags3 &= ~0x20000000;
            type4_object_registry_insert(LEGOOBJ_SLOT(obj));
            return;
        }
        obj->flags3 |= 0x20000000;
        if ((entity_state_flags_get(LEGOOBJ_SLOT(obj)) & 0x200) &&
            legoobject_shared_eligibility_check(obj, 0) && Lego_GetCrystalCount(1))
        {
            construction_flood_path_blocks(blockPos);
        }
    }
}

// FUNCTION: LEGORR 0x0043c650
int legoobject_capacity_or_state_predicate(Lego_Object* obj)
{
    if (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x80000) {
        if (stats_instance_word_at_e0(obj) && obj->field_348 != NULL &&
            *(unsigned int*)obj->field_348 >= stats_instance_word_at_e0(obj))
        {
            return 0;
        }
        if (!(obj->flags3 & 0x80000000))
            return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0043c6b0
void legoobject_activity_counters_advance(Lego_Object* obj, float elapsed)
{
    float before3d8;
    float before3d4;

    before3d8 = obj->field_3d8;
    before3d4 = obj->field_3d4;
    obj->field_3d8 += elapsed;
    obj->field_3d4 += elapsed;
    obj->field_3dc += elapsed;
    if (obj->type == 2) {
        if (engine_float_field210_get_by_index_current(2) != legoobject_zero &&
            engine_float_field210_get_by_index_current(obj->type) <= obj->field_3d8 &&
            engine_float_field210_get_by_index_current(obj->type) > before3d8)
        {
            info_send(0x22, NULL, obj, NULL);
        }
        if (engine_float_field260_get_current_target() != legoobject_zero &&
            engine_float_field260_get_current_target() <= obj->field_3d4 &&
            engine_float_field260_get_current_target() > before3d4)
        {
            info_send(0x23, NULL, obj, NULL);
        }
    } else if ((obj->type == 6 || obj->type == 7 || obj->type == 9 ||
                (obj->type == 11 && !(obj->flags2 & 0x2000000))) &&
               obj->field_3d8 >= 750.0f && before3d8 < 750.0f)
    {
        obj->field_3d8 = 0.0f;
        aitask_entity_deferred_work_schedule(LEGOOBJ_SLOT(obj));
    }
}

/* ---- functions, group J (0043c810; part 2, issue #588) ---- */

/* LegoObject_Callback_Update: one object's frame. Ghidra merged it into
 * 0043c6b0's span; LegoObject_UpdateAll (0043b270) pushes it. The flag
 * words are flags1-flags4 (+0x3e0-+0x3ec): one handler runs per frame, the
 * first whose flag is set, most of them finishing when the activity's
 * animation wraps (animTime > 0). */
// FUNCTION: LEGORR 0x0043c810
int legoobject_update_callback(Lego_Object* obj, void* data)
{
    float elapsed;
    int stopDrillSound;
    float tickResult;
    int result;
    int transitionDone;
    Lego_Object* target;
    void* cont;

    elapsed = *(float*)data;
    stopDrillSound = 1;
    legoGlobs_f_556090 = obj;
    legoobject_activity_counters_advance(obj, elapsed);

    if (obj->flags4 & 0x200000) {
        obj->activityName1 = NULL;
        legoobject_activity_register_bookkeeping(obj);
        obj->flags4 &= ~0x200000;
    }
    if (obj->flags2 & 0x8000000) {
        if (obj->field_368 <= legoobject_zero)
            legoobject_attached_entity_wake(obj);
        obj->field_368 -= elapsed;
        return 0;
    }
    if (obj->flags4 & 0x40000) {
        cont = legoobject_type_component_get(obj);
        if (cont != NULL) {
            int sample = (int)entity_stats_word_atc8((struct EntitySlot*)obj);
            if (sample != 0) {
                obj->soundHandle = sound_submission_route_wrapper(cont, sample, 1, 1, NULL);
                obj->flags4 |= 0x80000;
            }
        }
        obj->flags4 &= ~0x40000;
    }
    if ((s_panel_highlight_flags & 1) && legoobject_ai_can_do(obj)) {
        type2_timed_registry_insert((struct EntitySlot*)obj);
        obj->flags4 |= 0x20;
    } else {
        set_type2_object_record_value((struct EntitySlot*)obj, 0);
        obj->flags4 &= ~0x20;
    }
    if (obj->type == 1 && (obj->flags4 & 0x40) && obj->routeToObject == NULL &&
        obj->driveObject == NULL) {
        obj->flags4 &= ~0x40;
        legoobject_lava_crust_activity_register(obj);
    }
    if (obj->type == 2 || obj->type == 1) {
        LegoObjJ_Level* level;
        LegoObjJ_Point2I blockPos;
        level = (LegoObjJ_Level*)level_current_owner_get();
        legoobject_position_to_terrain_cell(obj, &blockPos.x, &blockPos.y);
        if (level->blocks[level->width * blockPos.y + blockPos.x].flags1 & 0x80)
            lego_emerge_map_trigger_spawn((struct Lego_Level*)level, (struct LegoPoint2I2A*)&blockPos, NULL);
    }
    legoobject_building_deposit_state_monitor(obj);
    if (obj->type == 2 && (obj->flags2 & 8) && obj->driveObject != NULL &&
        obj->driveObject->type == 1) {
        animation_object_time_update((struct AnimationObject*)obj->miniFigure,
            ((float (*)(void*))scene_object_attached_resource_release)(obj->driveObject->vehicle));
    } else {
        tickResult = legoobject_activity_tick(obj, elapsed);
    }
    if (entity_stats_word_get((struct EntitySlot*)obj) & 0x80000070)
        legoobject_spawn_request_queue_process(obj);
    if (obj->type != 4 || legoobject_shared_eligibility_check(obj, 0))
        lego_air_supply_amount_update((struct EntitySlot*)obj, elapsed);
    if (s_entity_state_flags & 4)
        legoobject_kind_four_post_create_resolve(obj);
    else
        obj->flags3 &= ~0x40000000;

    if (obj->flags1 & 0x400000) {
        if (obj->activityCont != NULL) {
            cont = legoobject_type_component_get(obj);
            scene_object_camera_teleport_or_offset(cont, 2, 0.0f, 0.0f, -elapsed);
            if (ddiscene_advance_by_time_target(obj->activityCont, elapsed) > legoobject_zero) {
                obj->health = -1.0f;
                obj->flags3 |= 0x800000;
                obj->flags1 &= ~0x400000;
                ledger_stub_00472a40_DDiSceneObjectDestroy(obj->activityCont);
            }
            return 0;
        }
        obj->flags3 |= 0x800000;
        return 0;
    }

    if (obj->flags1 & 8) {
        float wy, wx;
        LegoObjJ_Point2I pick;
        LegoObjJ_Point2I cell;
        float speed;
        int bx, by;
        int ok;
        LegoObjG_RouteBlock* route;
        if (obj->flags1 & 0x10) {
            cont = legoobject_type_component_get(obj);
            obj->drillSoundHandle = sound_submission_route_wrapper(cont,
                (int)entity_stats_word_at_c0_or_c4((struct EntitySlot*)obj, 0), 1, 1, NULL);
            obj->flags4 |= 0x100000;
            obj->flags1 &= ~0x10;
        }
        route = (LegoObjG_RouteBlock*)obj->routeBlocks;
        if (route != NULL) {
            bx = route[obj->routeBlocksCurrent].blockPos[0];
            by = route[obj->routeBlocksCurrent].blockPos[1];
        }
        if (route == NULL ||
            !terrain_cell_walkability_query_current((struct EntitySlot*)obj,
                (int)obj->targetBlockPos[0], (int)obj->targetBlockPos[1],
                obj->flags3 & 0x2000000, 1)) {
            obj->flags1 &= ~8;
            obj->flags3 &= ~0x2000000;
            cont = legoobject_type_component_get(obj);
            sound_handle_stop_forward(obj->drillSoundHandle);
            obj->flags4 &= ~0x100000;
            sound_submission_route_wrapper(cont,
                (int)entity_stats_word_at_c0_or_c4((struct EntitySlot*)obj, 1), 0, 1, NULL);
            ((void (*)(int, int, int))flic_opcode27_fallback_noop)(0, 0, 0);
            obj->flags3 &= ~0x4000;
        } else if (legoobject_work_object_position_get(obj, &wx, &wy)) {
            if (obj->type == 2)
                ((void (*)(void*, void*, int))flic_opcode27_fallback_noop)(
                    renderer_cached_handle_40_get_or_create(obj->miniFigure),
                    lego_mode_dependent_field24_get(), 0);
            speed = entity_stats_float_at24_indexed((struct EntitySlot*)obj,
                lego_terrain_cell_material_get((int)obj->targetBlockPos[0], (int)obj->targetBlockPos[1]));
            if ((entity_stats_word_get((struct EntitySlot*)obj) & 1) && (obj->flags3 & 0x2000000))
                speed *= 0.5f;
            if (terrain_cell_drill_work_add_current((int)obj->targetBlockPos[0],
                    (int)obj->targetBlockPos[1], speed, elapsed)) {
                if (terrain_cell_walkability_query_current((struct EntitySlot*)obj,
                        (int)obj->targetBlockPos[0], (int)obj->targetBlockPos[1],
                        obj->flags3 & 0x2000000, 1)) {
                    cell.x = (int)obj->targetBlockPos[0];
                    cell.y = (int)obj->targetBlockPos[1];
                    if (obj->flags3 & 0x2000000)
                        ok = lego_terrain_cell_initialize((struct Lego_Level*)level_current_owner_get(), cell.x, cell.y);
                    else
                        ok = lego_terrain_cell_mutate((struct Lego_Level*)level_current_owner_get(), bx, by, 0);
                    if (ok) {
                        entity_command_unlink_by_kind((struct EntitySlot*)obj, 8, NULL, 1);
                        obj->flags1 &= ~8;
                        level_tile_mark_current_target((int*)&cell, 0);
                        if (((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].actionFlags == 1)
                            ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].actionFlags = 0;
                        cont = legoobject_type_component_get(obj);
                        sound_handle_stop_forward(obj->drillSoundHandle);
                        obj->flags4 &= ~0x100000;
                        sound_submission_route_wrapper(cont,
                            (int)entity_stats_word_at_c0_or_c4((struct EntitySlot*)obj, 1), 0, 1, NULL);
                        ((void (*)(int, int, int))flic_opcode27_fallback_noop)(0, 0, 0);
                        obj->flags3 &= ~0x4000;
                    }
                }
            } else if (((int (*)(float, float, int*, int*))current_level_terrain_grid_pick)(wx, wy, &pick.x, &pick.y) &&
                       pick.x == bx && pick.y == by) {
                obj->flags3 |= 0x4000;
            }
        }
    }

    if (obj->flags2 & 0x4000)
        legoobject_heading_step_move(obj, elapsed);
    if ((obj->flags2 & 0x40000000) && elapsed > legoobject_zero) {
        cont = legoobject_type_component_get(obj);
        if (cont != NULL) {
            LegoObjJ_Vector3F rnd;
            LegoObjJ_Vector3F dir;
            LegoObjJ_Vector3F up;
            ddiscene_object_facing_pair_get(cont, NULL, (float*)&dir, (float*)&up);
            legoobject_vector3_scale(&up, &up, 10.0f);
            d3drm_vector_random((float*)&rnd);
            legoobject_vector3_add(&up, &up, &rnd);
            legoobject_vector3_scale(&up, &up, s_legoobject_spark_fraction * 0.1f);
            up.z = -1.0f;
            ddiscene_object_pose_method_dispatch(cont, NULL, dir.x, dir.y, dir.z, up.x, up.y, up.z);
            s_legoobject_spark_fraction -= elapsed * 0.16f;
            if (s_legoobject_spark_fraction < legoobject_zero) {
                s_legoobject_spark_fraction = 1.0f;
                obj->flags2 &= ~0x40000000;
            }
        }
    }

    if (obj->flags1 & 0x100000) {
        legoobject_monster_spawn_behavior_tick(obj);
    } else if (obj->flags1 & 0x4000000) {
        if (obj->animTime > legoobject_zero) obj->flags1 &= ~0x4000000;
    } else if (obj->flags4 & 0x1000) {
        if (obj->animTime > legoobject_zero) obj->flags4 = (obj->flags4 & ~0x1000) | 0x2000;
    } else if (obj->flags4 & 0x4000) {
        if (obj->animTime > legoobject_zero) {
            obj->flags4 &= ~0x4000;
            legoobject_set_activity(obj, 0, 0);
        }
    } else if (obj->flags4 & 0x2000) {
        legoobject_set_activity(obj, 0x37, 0);
    } else if (obj->flags2 & 0x4000) {
    } else if (obj->flags1 & 4) {
        if (obj->flags1 & 0x80) {
            if (obj->animTime > legoobject_zero) {
                cont = legoobject_type_component_get(obj);
                render_object_rotate_axis(cont, 1, 0.0f, 1.0f, 0.0f, 1.5707964f);
                ddiscene_frame_update_and_callbacks(cont, 0.0f);
                obj->flags1 &= ~0x80;
            }
        } else if (obj->animTime > legoobject_zero) {
            cont = legoobject_type_component_get(obj);
            if (s_activity[9] == obj->activityName1 || s_activity[20] == obj->activityName1)
                render_object_rotate_axis(cont, 1, 0.0f, 1.0f, 0.0f, -1.5707964f);
            else
                render_object_rotate_axis(cont, 1, 0.0f, 1.0f, 0.0f, 1.5707964f);
            obj->flags1 = (obj->flags1 & ~4) | 1;
            if (obj->routeBlocksTotal != 0) {
                if ((obj->flags1 & 0x800000) && obj->type == 2)
                    legoobject_set_activity(obj, 3, 0);
                else
                    legoobject_set_activity(obj, 1, 0);
            } else {
                legoobject_set_activity(obj, 0, 0);
            }
        }
    } else if (obj->flags2 & 0x20) {
        legoobject_route_step_command_issue(obj);
    } else if (obj->flags1 & 0x80000000) {
        if (obj->animTime > legoobject_zero) obj->flags1 &= ~0x80000000;
    } else if (obj->flags1 & 0x10000000) {
        if (obj->animTime > legoobject_zero) {
            cont = legoobject_type_component_get(obj);
            obj->flags1 &= ~0x10000000;
            ddiscene_object_pose_method_dispatch(cont, NULL, obj->field_2c8, obj->field_2cc, 0.0f, 0.0f, 0.0f, -1.0f);
            legoobject_set_activity(obj, 0, 0);
        }
    } else if (obj->flags2 & 0x400000) {
        if (obj->flags2 & 0x10000) legoobject_kind_effect_slot_init_dispatch(obj, 1);
        if (obj->animTime > legoobject_zero) {
            obj->flags2 &= ~0x400000;
            if (!(obj->flags2 & 0x20000)) legoobject_kind_effect_slot_init_dispatch(obj, 1);
        }
    } else if (obj->flags4 & 0x8000) {
        if (obj->animTime > legoobject_zero) {
            obj->flags4 &= ~0x8000;
            if (obj->routeToObject != NULL) {
                LegoObjJ_Vector3F pos;
                LegoObjJ_Vector3F dir;
                cont = ((void* (*)(Lego_Object*))legoobject_visual_resource_get_or_create)(obj->routeToObject);
                if (cont != NULL) {
                    ddiscene_object_position_get(cont, NULL, (float*)&pos);
                    ddiscene_object_facing_pair_get(cont, NULL, (float*)&dir, NULL);
                    ddiscene_object_position_set(legoobject_type_component_get(obj), NULL, pos.x, pos.y,
                        terrain_map_world_z_get(Lego_GetMap(), pos.x, pos.y));
                    ddiscene_object_pose_method_dispatch(legoobject_type_component_get(obj), NULL,
                        dir.x, dir.y, dir.z, 0.0f, 0.0f, -1.0f);
                }
                legoobject_activity_toggle(obj->routeToObject, 0);
                legoobject_set_activity(obj, 0, 0);
            } else {
                legoobject_set_activity(obj, 0, 0);
            }
        }
    } else if (obj->flags2 & 0x80000000) {
        if (obj->routeToObject != NULL) {
            void* cont2;
            LegoObjJ_Vector3F pos;
            LegoObjJ_Vector3F dir;
            LegoObjJ_Vector3F up;
            cont = legoobject_type_component_get(obj);
            cont2 = legoobject_type_component_get(obj->routeToObject);
            target = obj->routeToObject;
            ddiscene_object_position_get(cont2, NULL, (float*)&pos);
            ddiscene_object_facing_pair_get(cont2, NULL, (float*)&dir, (float*)&up);
            ddiscene_object_position_set(cont, NULL, pos.x, pos.y, pos.z);
            ddiscene_object_pose_method_dispatch(cont, NULL, dir.x, dir.y, dir.z, up.x, up.y, up.z);
            if (obj->animTime > legoobject_zero) {
                target->carriedObjects[target->numCarriedObjects] = obj;
                target->carriedObjects[target->numCarriedObjects]->carryingThisObject = target;
                target->carriedObjects[target->numCarriedObjects]->carryingIndex = target->numCarriedObjects;
                target->flags1 |= 0x400;
                legoobject_activity_toggle(target, 0);
                target->numCarriedObjects++;
                obj->flags2 &= ~0x80000000;
                legoobject_set_activity(obj, 0, 0);
                entity_command_unlink_by_kind((struct EntitySlot*)obj, 0x1e, NULL, 1);
                target->flags4 &= ~0x10000;
            }
        } else {
            obj->flags2 &= ~0x80000000;
        }
    } else if (obj->flags2 & 0x10000000) {
        if (obj->animTime > legoobject_zero) {
            obj->flags2 &= ~0x10000000;
            if (obj->flags1 & 0x400) {
                LegoObjJ_Vector3F pos;
                obj->carriedObjects[0]->flags3 &= ~0x80000000;
                ddiscene_object_position_get(legoobject_type_component_get(obj->carriedObjects[0]), NULL, (float*)&pos);
                sound_submission_route_wrapper(NULL, 0x1a, 0, 0, (float*)&pos);
                legoobject_rover_work_queue_update(obj->carriedObjects[0], 1);
            }
        }
    } else if (obj->flags2 & 0x800000) {
        if (obj->flags2 & 0x10000) legoobject_kind_effect_slot_init_dispatch(obj, 2);
        if (obj->animTime > legoobject_zero) {
            obj->flags2 &= ~0x800000;
            if (!(obj->flags2 & 0x20000)) legoobject_kind_effect_slot_init_dispatch(obj, 2);
        }
    } else if (obj->flags2 & 0x1000000) {
        if (obj->flags2 & 0x10000) legoobject_kind_effect_slot_init_dispatch(obj, 3);
        if (obj->animTime > legoobject_zero) {
            obj->flags2 &= ~0x1000000;
            if (!(obj->flags2 & 0x20000)) legoobject_kind_effect_slot_init_dispatch(obj, 3);
        }
    } else if (obj->flags1 & 0x800) {
        if (obj->type == 3 && renderer_cached_handle_4c_is_not_one((struct AnimationObject*)obj->rockMonster))
            legoobject_clear_linked_object_state(obj);
        if (obj->animTime > legoobject_zero) obj->flags1 &= ~0x800;
    } else if (obj->flags1 & 0x4000) {
        target = obj->routeToObject;
        if (target != NULL && !(target->flags1 & 0x2000) && !(target->flags3 & 0x1000000) &&
            target->numCarriedObjects != target->carryNullFrames)
            obj->flags1 = (obj->flags1 & ~0x4000) | 0x2000;
    } else if (obj->flags1 & 0x2000) {
        if (obj->animTime > legoobject_zero) {
            obj->flags1 &= ~0x2000;
            entity_command_unlink_by_kind((struct EntitySlot*)obj, 5, obj, 1);
            legoobject_simulation_queue_flush(obj);
        } else if (obj->routeToObject != NULL) {
            obj->routeToObject->flags3 |= 0x1000000;
        }
    } else if (obj->flags1 & 0x20000000) {
        if (obj->animTime > legoobject_zero) {
            obj->flags1 &= ~0x20000000;
            obj->flags3 &= ~0x4000;
        }
    } else if (obj->flags2 & 0x200000) {
        if (obj->flags2 & 0x10000) legoobject_terrain_cell_variant_update_queue(obj);
        if (obj->animTime > legoobject_zero) {
            obj->flags2 &= ~0x200000;
            entity_command_unlink_by_kind((struct EntitySlot*)obj, 0x19, NULL, 1);
            if (!(obj->flags2 & 0x20000)) legoobject_terrain_cell_variant_update_queue(obj);
        }
    } else if (obj->flags2 & 0x40000) {
        if (obj->flags2 & 0x10000) legoobject_linked_proximity_response(obj);
        if (obj->animTime > legoobject_zero) {
            obj->flags2 &= ~0x40000;
            if (!(obj->flags2 & 0x20000)) legoobject_linked_proximity_response(obj);
        }
    } else if (obj->flags2 & 0x20000000) {
        if (obj->animTime > legoobject_zero) {
            obj->flags2 &= ~0x20000000;
            obj->flags3 &= ~0x80000000;
        }
    } else if (obj->flags1 & 0x1000000) {
        if ((entity_state_flags_get((struct EntitySlot*)obj) & 0x80000) &&
            obj->routeToObject != NULL && !(obj->routeToObject->flags4 & 0x10)) {
            if (legoobject_shared_eligibility_check(obj->routeToObject, 0)) {
                info_send(0xf, NULL, obj->routeToObject, NULL);
                ((void (*)(int))panel_selection_highlight_toggle)(1);
            }
            obj->routeToObject->flags4 |= 0x10;
            legoobject_activity_scene_state_update();
        }
        if ((obj->flags2 & 0x10000) && legoobject_progress_negative_delta_handle(obj))
            obj->flags2 &= ~0x80000;
        if (obj->animTime > legoobject_zero) {
            if (!(obj->flags2 & 0x20000) && legoobject_progress_negative_delta_handle(obj)) {
                obj->flags2 &= ~0x80000;
                entity_command_unlink_by_kind((struct EntitySlot*)obj, 10, NULL, 1);
            }
            if (obj->flags2 & 0x80000) {
                cont = legoobject_type_component_get(obj);
                ddiscene_frame_update_and_callbacks(cont, obj->animTime);
            } else {
                obj->flags1 &= ~0x1000000;
                if (obj->routeToObject != NULL) {
                    obj->routeToObject->flags4 &= ~0x10;
                    legoobject_activity_scene_state_update();
                    obj->routeToObject = NULL;
                }
                entity_command_unlink_by_kind((struct EntitySlot*)obj, 10, NULL, 1);
            }
        }
    } else if (obj->flags1 & 0x40000000) {
        if (obj->animTime > legoobject_zero) {
            if (legoobject_progress_advance_by_25(obj)) {
                obj->flags1 &= ~0x40000000;
                entity_command_unlink_by_kind((struct EntitySlot*)obj, 0x10, NULL, 1);
                entity_command_unlink_by_kind((struct EntitySlot*)obj, 0x11, NULL, 1);
            } else {
                cont = legoobject_type_component_get(obj);
                ddiscene_frame_update_and_callbacks(cont, obj->animTime);
            }
        }
    } else if (obj->flags2 & 0x40) {
        obj->flags2 &= ~0x40;
        if (obj->aiTask != NULL) {
            legoobject_activity_grant(obj, ((LegoObjJ_AITask*)obj->aiTask)->activity);
            entity_command_unlink_by_kind((struct EntitySlot*)obj, 0x13, NULL, 1);
        }
    } else if (obj->flags2 & 0x400) {
        if (obj->animTime > legoobject_zero) {
            obj->flags2 &= ~0x400;
            entity_command_unlink_by_kind((struct EntitySlot*)obj, 0x17, NULL, 1);
        }
    } else if (obj->flags2 & 0x8000) {
        if (obj->type == 2) {
            if (obj->routeToObject != NULL) {
                if (!(animation_object_time_get((struct AnimationObject*)obj->miniFigure) <
                      entity_stats_float_atf4((struct EntitySlot*)obj) *
                          entity_speed_component_get((struct EntitySlot*)obj->routeToObject))) {
                    unsigned int upgrade;
                    entity_command_unlink_by_kind((struct EntitySlot*)obj, 0x15, NULL, 1);
                    legoobject_set_activity(obj, 0, 0);
                    legoobject_activity_register_bookkeeping(obj);
                    upgrade = obj->objLevel + 1;
                    if (upgrade < building_upgrade_admission_check(obj->type, obj->id)) {
                        entity_building_index_set((struct EntitySlot*)obj, upgrade);
                        building_upgrade_state_changed_dispatch(obj->type, obj->id, obj->objLevel, 0);
                    }
                    obj->routeToObject = NULL;
                    obj->flags2 &= ~0x8000;
                }
            } else {
                obj->flags2 &= ~0x8000;
            }
        } else if (obj->animTime > legoobject_zero) {
            if (obj->routeToObject == NULL ||
                !(entity_state_flags_get((struct EntitySlot*)obj->routeToObject) & 2)) {
                obj->flags2 &= ~0x8000;
            } else if (!(obj->routeToObject->flags2 & 0x8000)) {
                entity_command_unlink_by_kind((struct EntitySlot*)obj, 0x15, NULL, 1);
                if (entity_mapped_indexed_record_word_at_fc_get(obj->type, obj->id, obj->field_404) <=
                    (unsigned int)lego_upgrade_resource_check(0)) {
                    legoobject_indexed_flag_set_and_refresh(obj);
                    lego_upgrade_resource_debit(0, entity_mapped_indexed_record_word_at_fc_get(obj->type, obj->id, obj->field_404));
                } else if (stats_table_word_at_100(obj->type, obj->id, obj->field_404) <=
                           (unsigned int)lego_upgrade_resource_check(1)) {
                    legoobject_indexed_flag_set_and_refresh(obj);
                    lego_upgrade_resource_debit(1, stats_table_word_at_100(obj->type, obj->id, obj->field_404));
                }
                obj->routeToObject->interactObject = NULL;
                obj->routeToObject = NULL;
                obj->flags2 &= ~0x8000;
            }
        }
    } else if (obj->flags1 & 0x80000) {
        ddiscene_apply_act_frame_timing(legoobject_type_component_get(obj));
        if ((obj->flags2 & 0x10000) && (obj->flags1 & 0x400) && obj->carriedObjects[0] != NULL) {
            LegoObjJ_Vector3F pos;
            ddiscene_object_position_get(legoobject_type_component_get(obj->carriedObjects[0]), NULL, (float*)&pos);
            if (obj->carriedObjects[0]->type == 7)
                sound_submission_route_wrapper(NULL, 0x24, 0, 0, (float*)&pos);
            else if (obj->carriedObjects[0]->type == 6)
                sound_submission_route_wrapper(NULL, 0x25, 0, 0, (float*)&pos);
            else
                sound_submission_route_wrapper(NULL, 0x23, 0, 0, (float*)&pos);
        }
        if (obj->animTime > legoobject_zero) {
            obj->flags1 &= ~0x80000;
            legoobject_attached_object_list_detach(obj, 1);
        }
    } else if (obj->flags1 & 0x1000) {
        if (obj->animTime > legoobject_zero) obj->flags1 &= ~0x1000;
    } else if (obj->carryingThisObject != NULL) {
    } else if ((obj->flags1 & 1) && legoobject_type26_command_completion_gate(obj)) {
        legoobject_action_sequence_reset(obj, 1);
        legoobject_mark_and_finish_state_1a(obj);
    } else if (obj->flags1 & 1) {
        ((void (*)(Lego_Object*, float))legoobject_path_step_advance_and_arrival_check)(obj, elapsed);
        legoobject_activity_timeout_accumulate_and_raise(obj, elapsed);
        legoobject_proximity_activity_raise(obj);
        if (obj->flags2 & 0x4000000) {
            if (!entity_centers_within_configured_radius((struct EntitySlot*)obj,
                    (struct EntitySlot*)((LegoObjJ_AITask*)obj->aiTask)->object) ||
                object_pair_ray_hit_check(obj, ((LegoObjJ_AITask*)obj->aiTask)->object))
                obj->flags2 &= ~0x4000000;
            else
                legoobject_finish_state_1a(obj);
        } else if (obj->flags1 & 8) {
            LegoObjJ_Point2I cell;
            cell.x = (int)obj->targetBlockPos[0];
            cell.y = (int)obj->targetBlockPos[1];
            legoobject_set_activity(obj, 4, 0);
            level_tile_mark_current_target((int*)&cell, 1);
        } else if (obj->flags1 & 2) {
            legoobject_set_activity(obj, 8, 0);
        } else if (obj->flags2 & 0x400) {
            legoobject_set_activity(obj, 0x32, 0);
        } else if (obj->flags2 & 0x10000000) {
            legoobject_set_activity(obj, 0x30, 0);
        } else if (obj->flags1 & 0x40000) {
            legoobject_set_activity(obj, 0x1e, obj->type == 2 ? 2 : 0);
        } else if (obj->flags2 & 0x80000000) {
            legoobject_set_activity(obj, 0x33, 0);
        } else if (obj->flags2 & 4) {
            if (obj->routeToObject != NULL)
                legoobject_linked_route_flag_clear(obj, obj->routeToObject);
        } else if (obj->flags1 & 0x40) {
            LegoObjJ_Point2I cell;
            cell.x = (int)obj->targetBlockPos[0];
            cell.y = (int)obj->targetBlockPos[1];
            level_tile_mark_current_target((int*)&cell, 1);
            legoobject_set_activity(obj, 7, legoGlobs_ReinforceHits);
        } else if (obj->flags1 & 0x80000) {
            legoobject_set_activity(obj, 0x1f, 0);
        } else if (obj->flags1 & 0x20000) {
            LegoObjJ_Point2I blockPos;
            blockPos.x = (int)obj->targetBlockPos[0];
            blockPos.y = (int)obj->targetBlockPos[1];
            if (!(entity_state_flags_get((struct EntitySlot*)obj) & 0x100000) &&
                !terrain_cell_walkable_flag_current(blockPos.x, blockPos.y)) {
                obj->flags1 &= ~0x20000;
                obj->flags2 |= 0x20;
            } else if (terrain_cell_flag_20_bounded_current(blockPos.x, blockPos.y)) {
                legoobject_set_activity(obj, 0xe, 0);
                if (!legoobject_activity_register_bookkeeping(obj)) {
                    legoobject_set_activity(obj, 0xd, 0);
                    legoobject_activity_register_bookkeeping(obj);
                }
                if (entity_state_flags_get((struct EntitySlot*)obj) & 0x10000000)
                    lego_terrain_cell_flag20_clear_and_refresh((int*)&blockPos);
            } else {
                legoobject_set_activity(obj, 0xd, 0);
            }
        } else if (obj->flags1 & 0x1000000) {
            legoobject_set_activity(obj, 0x20, 0);
        } else if (obj->flags2 & 0x200000) {
            legoobject_set_activity(obj, 0x24, 0);
        } else if (obj->flags2 & 0x8000) {
            if (obj->type == 2) {
                legoobject_set_activity(obj, 0x32, 100);
            } else {
                legoobject_set_activity(obj, 0x29, 0);
                legoobject_set_activity(obj->routeToObject, 0x29, 0);
                legoobject_activity_register_bookkeeping((Lego_Object*)obj->routeToObject);
                obj->routeToObject->flags2 |= 0x8000;
            }
        } else if (obj->flags1 & 0x40000000) {
            legoobject_set_activity(obj, 0x23, 0);
        } else if (obj->flags1 & 0x200) {
            if (obj->routeToObject == NULL || obj->routeToObject->carryingThisObject != NULL) {
                obj->flags1 &= ~0x200;
            } else {
                legoobject_set_activity(obj, 0xf, 0);
                if (obj->type == 3) {
                    if (obj->routeToObject->type == 5)
                        legoobject_set_activity(obj, 0x10, 0);
                    else if (obj->routeToObject->type == 6)
                        legoobject_set_activity(obj, 0x23, 0);
                }
                obj->carriedObjects[obj->numCarriedObjects] = obj->routeToObject;
                obj->routeToObject = NULL;
                obj->carriedObjects[obj->numCarriedObjects]->carryingThisObject = obj;
                obj->carriedObjects[obj->numCarriedObjects]->carryingIndex = obj->numCarriedObjects;
                obj->flags1 |= 0x400;
                entity_command_unlink_by_kind((struct EntitySlot*)obj, 3,
                    obj->carriedObjects[obj->numCarriedObjects], 1);
                ai_idle_chain_record_attach((struct EntitySlot*)obj);
                obj->numCarriedObjects++;
            }
        } else if (obj->flags1 & 0x2000) {
            legoobject_set_activity(obj, 0x1d, 0);
            target = obj->routeToObject;
            if (target != NULL &&
                ((entity_stats_word_get((struct EntitySlot*)target) & 4) ||
                 (entity_stats_word_get((struct EntitySlot*)target) & 2))) {
                target = obj->routeToObject;
                if (!(target->flags1 & 0x2000) && !(target->flags3 & 0x1000000) &&
                    target->numCarriedObjects != target->carryNullFrames)
                    target->flags3 |= 0x1000000;
                else
                    obj->flags1 = (obj->flags1 & ~0x2000) | 0x4000;
            }
        } else if (obj->flags1 & 0x800) {
            legoobject_set_activity(obj, 0x13, 0);
        } else if ((obj->flags1 & 4) || (obj->flags1 & 0x20000000)) {
        } else if (obj->flags1 & 0x400) {
            LegoObjJ_Point2I blockPos;
            legoobject_position_to_terrain_cell(obj, &blockPos.x, &blockPos.y);
            if (((unsigned int)terrain_cell_speed_class_current((int*)&blockPos) > 2 ||
                 ((LegoObjJ_Level*)g_active_level_retail)->blocks[
                     ((LegoObjJ_Level*)g_active_level_retail)->width * blockPos.y + blockPos.x].terrain == 9) &&
                (obj->flags3 & 0x10000000))
                legoobject_set_activity(obj, 0x12, 0);
            else
                legoobject_set_activity(obj, 0x11, 0);
        } else if (obj->flags1 & 0x80000000) {
            legoobject_action_sequence_reset(obj, 0);
            legoobject_set_activity(obj, 0xb, 1);
        } else if (obj->flags1 & 1) {
            LegoObjJ_Point2I blockPos;
            legoobject_position_to_terrain_cell(obj, &blockPos.x, &blockPos.y);
            if ((obj->flags1 & 0x800000) && obj->type == 2)
                legoobject_set_activity(obj, 3, 0);
            else if (((unsigned int)terrain_cell_speed_class_current((int*)&blockPos) > 2 ||
                      ((LegoObjJ_Level*)g_active_level_retail)->blocks[
                          ((LegoObjJ_Level*)g_active_level_retail)->width * blockPos.y + blockPos.x].terrain == 9) &&
                     (obj->flags3 & 0x10000000))
                legoobject_set_activity(obj, 2, 0);
            else
                legoobject_set_activity(obj, 1, 0);
        }
    } else if (obj->flags1 & 0x200000) {
        if (obj->animTime > legoobject_zero) {
            obj->flags1 &= ~0x200000;
            if (obj->type == 1) {
                legoobject_set_activity(obj, 1, 0);
                legoobject_activity_register_bookkeeping(obj);
            }
            legoobject_set_activity(obj, 0, 0);
            legoobject_kind_record_update_dispatch(obj);
        }
        legoobject_pose_orientation_dispatch(obj, elapsed);
        cont = legoobject_type_component_get(obj);
        if (cont != NULL) {
            LegoObjJ_Vector3F dir;
            ddiscene_object_facing_pair_get(cont, NULL, (float*)&dir, NULL);
            ddiscene_object_pose_method_dispatch(cont, NULL, dir.x, dir.y, dir.z, 0.0f, 0.0f, -1.0f);
        }
    } else if (legoobject_activity_terrain_eligibility_transition(obj, elapsed, &transitionDone)) {
        if (transitionDone)
            entity_command_unlink_by_kind((struct EntitySlot*)obj, 0xb, NULL, 1);
    } else if (obj->flags1 & 0x100) {
        if (obj->animTime > legoobject_zero) {
            obj->flags1 &= ~0x100;
            if (legoobject_type3_flag04000000_test(obj)) {
                LegoObjJ_Point2I cell;
                cell.x = (int)obj->targetBlockPos[0];
                cell.y = (int)obj->targetBlockPos[1];
                cmd_produce(0x20, obj, 0, NULL);
                level_tile_mark_current_target((int*)&cell, 0);
            }
        }
    } else if (obj->flags2 & 0x100000) {
        if (obj->animTime > legoobject_zero) {
            obj->flags2 &= ~0x100000;
            obj->flags3 |= 0x800000;
        }
    } else if (obj->flags2 & 4) {
        target = obj->routeToObject;
        obj->flags2 &= ~4;
        entity_command_unlink_by_kind((struct EntitySlot*)obj, 0x12, NULL, 1);
        obj->flags2 |= 8;
        target->flags4 &= ~0x80;
        if (target->routeToObject != NULL)
            target->routeToObject->flags4 &= ~0x80;
        target->routeToObject = NULL;
        target->flags2 &= ~0x10;
        obj->driveObject = target;
        target->driveObject = obj;
        obj->routeToObject = NULL;
        interface_object_menu_check((struct LegoObject*)obj->driveObject);
        legoobject_pose_orientation_dispatch(obj, elapsed);
        target->activityName2 = NULL;
        legoobject_activity_register_bookkeeping(target);
    } else if (obj->flags2 & 8) {
    } else if (obj->flags1 & 0x20000) {
        if (obj->animTime > legoobject_zero) {
            obj->flags1 &= ~0x20000;
            obj->flags2 &= ~0x20;
            legoobject_detach_carried_object_and_reset_state(obj);
        }
    } else if (obj->flags2 & 0x200) {
        if (obj->animTime > legoobject_zero) {
            void* descriptor;
            LegoObjJ_Point2I blockPos;
            legoobject_position_to_terrain_cell(obj, &blockPos.x, &blockPos.y);
            descriptor = terrain_descriptor_find_exact_current((const uint32_t*)&blockPos, NULL);
            if (descriptor != NULL && (LEGOOBJ_FIELD(descriptor, unsigned char, 0xd0) & 2)) {
                terrain_cell_variant_flag_propagate_current((int*)&blockPos);
                simtick_selection_group_release((const uint32_t*)&blockPos);
                info_send(0x20, NULL, NULL, (struct InfoPoint2I*)&blockPos);
                aitask_task_pair_record_enqueue((int*)&blockPos);
            }
            obj->flags2 &= ~0x200;
            entity_command_unlink_by_kind((struct EntitySlot*)obj, 0x16, NULL, 1);
        }
    } else if (obj->flags1 & 0x40000) {
        if (obj->type == 1) legoobject_pose_orientation_dispatch(obj, elapsed);
        if (obj->animTime > legoobject_zero) {
            obj->flags1 &= ~0x40000;
            entity_command_unlink_by_kind((struct EntitySlot*)obj, 0xc, NULL, 1);
            if (obj->type == 1) legoobject_set_activity(obj, 0, 0);
        }
    } else if (obj->flags1 & 0x80000000) {
        if (obj->animTime > legoobject_zero) obj->flags1 &= ~0x80000000;
    } else if (obj->flags2 & 0x100) {
        if (obj->animTime > legoobject_zero) obj->flags2 &= ~0x100;
    } else if (obj->flags1 & 0x10000) {
        if (rng_thunk_draw15() % 10)
            legoobject_random_cell_step_route_issue(obj);
        obj->flags1 &= ~0x10000;
    } else if (obj->flags2 & 2) {
        if (!(((Lego_Object*)obj->field_3c0)->flags2 & 1) || obj->animTime > legoobject_zero) {
            obj->flags2 &= ~2;
            legoobject_set_activity(obj, 0x27, 0);
            legoobject_activity_register_bookkeeping(obj);
            obj->flags2 |= 0x100;
        }
    } else if (obj->flags2 & 1) {
        LegoObjJ_Vector3F pos;
        if (obj->flags2 & 0x10000) {
            legoobject_transform_position_get(obj, &pos.x, &pos.y);
            ((void (*)(Lego_Object*, float, float, float, int))legoobject_world_position_set)(
                (Lego_Object*)obj->field_3c0, pos.x, pos.y,
                legoobject_vector_direction_angle_radians_get(obj), 1);
        }
        if (obj->animTime > legoobject_zero) {
            cont = ((void* (*)(Lego_Object*))legoobject_visual_resource_get_or_create)(obj);
            ddiscene_object_position_get(cont, NULL, (float*)&pos);
            legoobject_world_position_set(obj, pos.x, pos.y, 0, 0);
            obj->flags2 &= ~1;
            ((void (*)(Lego_Object*, float, int, float))legoobject_damage_apply)(
                (Lego_Object*)obj->field_3c0, 10.0f, 1, elapsed);
            legoobject_set_activity(obj, 0, 0);
        }
    } else if (entity_neighbor_blocked_cell_step_probe((struct EntitySlot*)obj, elapsed, 0)) {
    } else if ((result = legoobject_movement_accumulator_apply(obj, elapsed, &tickResult)) != 0) {
        float wx, wy;
        if (result == 1) {
            if (obj->flags1 & 0x400)
                legoobject_set_activity(obj, 0x11, 0);
            else
                legoobject_set_activity(obj, 1, 0);
        } else if (result == 4) {
            legoobject_set_activity(obj, 0, 0);
        } else if (result == 2) {
            legoobject_set_activity(obj, 6, 0);
        } else if (result == 3 &&
                   legoobject_work_object_position_get(obj, &wx, &wy)) {
            int bx, by;
            float speed;
            if (terrain_map_world_to_block(Lego_GetMap(), wx, wy, &bx, &by) &&
                terrain_cell_walkable_flag_current(bx, by)) {
                speed = entity_stats_float_at24_indexed((struct EntitySlot*)obj,
                    lego_terrain_cell_material_get(bx, by));
                if (!(obj->flags4 & 0x20000)) {
                    legoobject_type_component_get(obj);
                    obj->drillSoundHandle = sound_alias_play(
                        (int)entity_stats_word_at_c0_or_c4((struct EntitySlot*)obj, 0), 1);
                    obj->flags4 |= 0x120000;
                }
                stopDrillSound = 0;
                legoobject_set_activity(obj, 4, 0);
                if (terrain_cell_drill_work_add_current(bx, by, speed, elapsed) &&
                    terrain_cell_walkability_query_current((struct EntitySlot*)obj, bx, by, 0, 1))
                    lego_terrain_cell_mutate((struct Lego_Level*)level_current_owner_get(), bx, by, 0);
            }
        }
        if (obj->type == 2) {
            if (!(objectGlobs_f_eadcc & 3)) sound_alias_play(8, 0);
            objectGlobs_f_eadcc++;
        }
    } else if (obj->flags1 & 0x200) {
        if (obj->animTime > legoobject_zero) {
            obj->flags1 &= ~0x200;
            if (obj->flags1 & 0x400) {
                if (obj->type == 3) {
                    target = obj->carriedObjects[0];
                    if (target->type == 5) {
                        target = legoobject_query_center_mask_from_argument(obj, 0x400);
                        cmd_produce(0x22, obj, (int)(uintptr_t)target, NULL);
                    } else {
                        obj->carriedObjects[0] = NULL;
                        obj->numCarriedObjects = 0;
                        target->carryingThisObject = NULL;
                        target->interactObject = NULL;
                        entity_command_unlink_by_kind((struct EntitySlot*)obj, 3, target, 1);
                        legoobject_action_record_consume_and_cleanup(obj, target);
                        obj->flags1 &= ~0x400;
                    }
                } else if (obj->carriedObjects[obj->numCarriedObjects - 1]->type == 6) {
                    global_object_field94_increment();
                } else if (obj->carriedObjects[obj->numCarriedObjects - 1]->type == 7) {
                    global_object_fielda8_increment();
                    info_send(0x1a, NULL, obj, NULL);
                }
            }
        }
    } else if (obj->flags1 & 0x8000) {
        if (obj->animTime > legoobject_zero) obj->flags1 &= ~0x8000;
    } else if (obj->flags1 & 4) {
    } else {
        if (obj->flags1 & 0x400)
            legoobject_set_activity(obj, 0x16, 0);
        else if (obj->type == 4 && (!(obj->flags3 & 0x20000000) || (obj->flags3 & 0x80000000)))
            legoobject_set_activity(obj, 0x2b, 0);
        else if (obj->type == 3 && (obj->flags3 & 0x80000000))
            legoobject_set_activity(obj, 0x2b, 0);
        else if (obj->type == 1 && (obj->flags4 & 0x2000))
            legoobject_set_activity(obj, 0x37, 0);
        else
            legoobject_set_activity(obj, 0, 0);
        legoobject_pose_orientation_dispatch(obj, elapsed);
    }

    if (stopDrillSound && (obj->flags4 & 0x20000)) {
        legoobject_type_component_get(obj);
        sound_handle_stop_forward(obj->drillSoundHandle);
        obj->flags4 &= ~0x100000;
        sound_alias_play((int)entity_stats_word_at_c0_or_c4((struct EntitySlot*)obj, 1), 0);
        obj->flags4 &= ~0x20000;
    }
    legoobject_activity_register_bookkeeping(obj);
    if (obj->driveObject != NULL && obj->type != 2)
        legoobject_scene_object_pose_position_sync(obj);
    if (obj->flags1 & 0x400)
        legoobject_attached_direction_vector_update(obj);
    if (entity_flag100_and_shared_eligibility_check((struct EntitySlot*)obj))
        ((int (*)(void*, float))entity_velocity_slot_dispatch)(obj, elapsed);
    ((void (*)(Lego_Object*, float))legoobject_behavior_callback_tick)(obj, elapsed);
    legoobject_motion_to_scene_node_sync(obj, *(float*)data);
    legoobject_energy_low_target_route_command_tick(obj, elapsed);
    if (obj->flags2 & 0x10000)
        obj->flags2 = (obj->flags2 & ~0x10000) | 0x20000;
    obj->flags4 &= ~2;
    legoobject_objective_sidecar_append_gate(obj);
    return 0;
}

/* ---- functions, group I2 (0043eea0-0043f7d0; part 2, issue #588) ---- */

/* Hands every carried object over: a depositing object (stats 4 or 2)
 * consumes them (ore of type 7 is processed into one new object, the rest
 * credited), otherwise they move to the object it routes to, while that
 * one has room. */
// FUNCTION: LEGORR 0x0043eea0
void legoobject_simulation_queue_flush(Lego_Object* obj)
{
    Lego_Object* target;
    int deposit;
    unsigned int i;
    unsigned int j;
    Lego_Object* carried;
    Lego_Object** slot;
    void* cont;
    float position[3];

    target = obj->routeToObject;
    if ((entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 4) || (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 2))
        deposit = 1;
    else
        deposit = 0;
    for (i = 0, slot = obj->carriedObjects; i < obj->numCarriedObjects; i++, slot++) {
        carried = *slot;
        carried->routeToObject = NULL;
        if (deposit) {
            carried->carryingThisObject = NULL;
            carried->interactObject = NULL;
            if (carried->type == 7) {
                if (i == 0 || i >= stats_instance_word_at_ec(obj)) {
                    cont = scene_object_primary_handle_get_or_create(obj->building);
                    if ((i == 0) == 1) {
                        ddiscene_object_position_get(cont, NULL, position);
                        cmd_produce(0x1c, legoobject_create_in_world(legoGlobs_contProcessedOre, 7, 1, 0,
                                                                     position[0], position[1], 0.0f),
                                    0, NULL);
                        legoobject_remove(carried);
                        goto next;
                    }
                    lego_level_resource_balance_add_and_sound(0);
                }
                legoobject_remove(carried);
            } else {
                legoobject_action_record_consume_and_cleanup(obj, carried);
                if (obj->type == 4)
                    scene_object_scalar_adjust_by_direction(obj->building, 1);
                legoobject_activity_scene_state_update();
            }
        } else {
            if (target->numCarriedObjects == target->carryNullFrames) {
                for (j = i; j < obj->numCarriedObjects; j++)
                    obj->carriedObjects[j - i] = obj->carriedObjects[j];
                obj->numCarriedObjects -= i;
                return;
            }
            target->carriedObjects[target->numCarriedObjects] = carried;
            carried->carryingThisObject = target;
            carried->carryingIndex = target->numCarriedObjects;
            target->numCarriedObjects++;
            target->flags1 |= 0x400;
            cmd_produce(0x1a, NULL, 0, NULL);
            if (entity_stats_word_get(LEGOOBJ_SLOT(target)) & 4)
                info_send(0x1d, NULL, target, NULL);
        }
    next:
        *slot = NULL;
    }
    obj->numCarriedObjects = 0;
    obj->routeToObject = NULL;
    obj->flags1 &= ~0x400;
}

/* Clears the 0x40 link flag on target and on the object it routes to.
 * obj is not read. */
// FUNCTION: LEGORR 0x0043f100
void legoobject_linked_route_flag_clear(Lego_Object* obj, Lego_Object* target)
{
    target->flags4 &= ~0x40;
    if (target->routeToObject != NULL)
        target->routeToObject->flags4 &= ~0x40;
}

/* The container trigger-frame callback (registered by Lego.c's config
 * init): flags the container's owner object. */
// FUNCTION: LEGORR 0x0043f130
void legoobject_trigger_frame_callback(void* cont, void* data)
{
    Lego_Object* owner;

    owner = (Lego_Object*)(size_t)scene_object_owner_get(cont);
    if (owner != NULL)
        owner->flags2 |= 0x10000;
}

/* Queues an object request (type, id) for a teleporting building, nine at
 * most. obj is not read. The queue is read through a pointer: the
 * original's queue and count are fields of one Lego_Globs, so a store to
 * the queue reloads the count. */
// FUNCTION: LEGORR 0x0043f150
int legoobject_spawn_request_queue_add(Lego_Object* obj, int type, int id)
{
    LegoObjI_Requests* queue = &legoGlobs_objectRequests;

    if (legoGlobs_f_556514 < 9) {
        queue->type[legoGlobs_f_556514] = type;
        queue->id[legoGlobs_f_556514] = id;
        legoGlobs_f_556514++;
        return 1;
    }
    return 0;
}

/* A teleporting building (obj) serves the first queued object request it
 * can: the request's type matches one of the building's teleport kinds,
 * it is affordable, and the level has room for it. The new object is
 * placed at the building, starts its teleport-down activity, and the
 * request leaves the queue. */
// FUNCTION: LEGORR 0x0043f190
void legoobject_spawn_request_queue_process(Lego_Object* obj)
{
    int found;
    unsigned int i;
    Lego_Object* created;
    void* cont;
    int barracksType;
    int barracksId;
    void* model;
    int blockPos[2];
    float position[3];
    float direction[3];
    LegoObjI_Requests* queue = &legoGlobs_objectRequests;

    found = 0;
    if (legoobject_shared_eligibility_check(obj, 0) && legoGlobs_f_556514 > 0 &&
        (obj->field_3cc == NULL || !(obj->field_3cc->flags1 & 0x200000)) &&
        !(obj->flags4 & 0xc0))
    {
        for (i = 0; i < legoGlobs_f_556514; i++) {
            if (((stats_table_word_at_148(legoGlobs_objectRequests.type[i], legoGlobs_objectRequests.id[i]) & 0x1000) &&
                 (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x10)) ||
                ((stats_table_word_at_148(legoGlobs_objectRequests.type[i], legoGlobs_objectRequests.id[i]) & 0x800) &&
                 (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x20)) ||
                ((stats_table_word_at_148(legoGlobs_objectRequests.type[i], legoGlobs_objectRequests.id[i]) & 0x2000) &&
                 (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x40)) ||
                ((stats_table_word_at_148(legoGlobs_objectRequests.type[i], legoGlobs_objectRequests.id[i]) & 0x400000) &&
                 (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x80000000)))
            {
                if ((int)entity_indexed_record_word_at_b4_get(legoGlobs_objectRequests.type[i],
                                                              legoGlobs_objectRequests.id[i], 0) <=
                    Lego_GetCrystalCount(0))
                {
                    entity_type_pair_lookup("Barracks", &barracksType, &barracksId, NULL);
                    if (legoobject_state_pair_predicate(legoGlobs_objectRequests.type[i],
                                                        legoGlobs_objectRequests.id[i],
                                                        barracksType, barracksId))
                    {
                        found = 1;
                        break;
                    }
                }
            }
        }
        if (found) {
            lego_entity_at(legoGlobs_objectRequests.type[i], legoGlobs_objectRequests.id[i], &model);
            obj->field_3cc = legoobject_create_in_world(model, legoGlobs_objectRequests.type[i],
                                                        legoGlobs_objectRequests.id[i], 0, 0.0f, 0.0f, 0.0f);
            global_object_field8c_subtract(entity_indexed_record_word_at_b4_get(
                legoGlobs_objectRequests.type[i], legoGlobs_objectRequests.id[i], 0));
            legoobject_activity_scene_state_update();
            if (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x40) {
                obj->field_3cc->flags4 |= 0x40;
                obj->flags4 |= 0x40;
                obj->field_3cc->routeToObject = obj;
            } else if (entity_stats_word_get(LEGOOBJ_SLOT(obj->field_3cc)) & 0x20000) {
                obj->field_3cc->flags4 |= 0x80;
                obj->flags4 |= 0x80;
                obj->field_3cc->routeToObject = obj;
            }
            cont = scene_object_primary_handle_get_or_create(obj->building);
            ddiscene_object_position_get(cont, NULL, position);
            ddiscene_object_facing_pair_get(cont, NULL, direction, NULL);
            if (legoGlobs_objectRequests.type[i] == 1 &&
                (stats_table_word_at_148(1, legoGlobs_objectRequests.id[i]) & 0x1000))
            {
                terrain_map_world_to_block(Lego_GetMap(), position[0], position[1], &blockPos[0], &blockPos[1]);
                terrain_map_block_to_world(Lego_GetMap(), blockPos[0], blockPos[1], &position[0], &position[1]);
            }
            legoobject_world_position_set(obj->field_3cc, position[0], position[1], 0, 0);
            legoobject_velocity_vector_dispatch(obj->field_3cc, 0, direction);
            entity_state_override_pop(LEGOOBJ_SLOT(obj->field_3cc));
            obj->field_3cc->flags1 |= 0x200000;
            legoobject_set_activity(obj->field_3cc, 0x4e, 1);
            legoobject_activity_register_bookkeeping(obj->field_3cc);
            created = obj->field_3cc;
            if (created->type == 1)
                six_resource_slots_set_state(created->vehicle, 1);
            for (legoGlobs_f_556514--; i < legoGlobs_f_556514; i++) {
                queue->type[i] = queue->type[i + 1];
                queue->id[i] = queue->id[i + 1];
            }
        }
    }
}

// FUNCTION: LEGORR 0x0043f560
void legoobject_pool_find_with_local_predicate(Lego_Object* obj)
{
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_pool_find_local_predicate_callback, obj);
}

/* The LegoObject_RunThroughLists callback of 0043f560: clears a +0x3cc
 * reference to the object being removed. */
// FUNCTION: LEGORR 0x0043f580
int legoobject_pool_find_local_predicate_callback(Lego_Object* obj, void* data)
{
    if (obj->field_3cc == (Lego_Object*)data) {
        obj->field_3cc = NULL;
        return 1;
    }
    return 0;
}

/* Records events on a minifigure and reports the first one. */
// FUNCTION: LEGORR 0x0043f5b0
void legoobject_linked_activity_event_dispatch(Lego_Object* obj, unsigned int events)
{
    if (obj->type == 2) {
        obj->abilityFlags |= events;
        objectGlobs_f_4e8444 |= events;
        if (events & 0x1)
            info_send(0x17, NULL, obj, NULL);
        else if (events & 0x2)
            info_send(0x18, NULL, obj, NULL);
        else if (events & 0x4)
            info_send(0x14, NULL, obj, NULL);
        else if (events & 0x8)
            info_send(0x15, NULL, obj, NULL);
        else if (events & 0x10)
            info_send(0x16, NULL, obj, NULL);
        else if (events & 0x20)
            info_send(0x19, NULL, obj, NULL);
        obj->field_3d4 = 0.0f;
    }
}

// FUNCTION: LEGORR 0x0043f6a0
void legoobject_damage_apply(Lego_Object* obj, float damage, int reactToHit, float elapsed)
{
    if (legoobject_shared_eligibility_check(obj, 1) && obj->carryingThisObject == NULL) {
        if (obj->type == 4 && damage != legoobject_zero && obj->health == 100.0f)
            aitask_entity_threshold_progress_event_queue(LEGOOBJ_SLOT(obj), 0);
        if (legoobject_type3_flag04000000_test(obj) &&
            (unsigned int)(damage * 10000.0f) >
            (unsigned int)((elapsed * (entity_stats_float_at80(LEGOOBJ_SLOT(obj)) * 0.04f)) * 10010.0f))
        {
            global_float_accumulator_00555330_add(damage);
        }
        if (obj->type == 2)
            global_float_accumulate_00555120(damage);
        obj->health -= damage;
        if (reactToHit && damage > legoobject_zero) {
            obj->flags2 |= 0x3000;
            obj->field_3d0 += damage;
            flagged_object_timer_registry_insert(LEGOOBJ_SLOT(obj));
        }
        if ((entity_state_flags_get(LEGOOBJ_SLOT(obj)) & 0x800000) && damage > legoobject_zero)
            lego_panel_selection_highlight_toggle_adapter(1);
    }
}

/* The per-tick health and energy step: decay both, take the block's lava
 * and terrain damage, strike electric fences, and send a hungry
 * minifigure to eat at the nearest reachable building. */
// FUNCTION: LEGORR 0x0043f7d0
void legoobject_energy_low_target_route_command_tick(Lego_Object* obj, float elapsed)
{
    float damage;
    unsigned int i;
    Lego_Object* target;
    int* pathX;
    int* pathY;
    float pathCount;
    int blockPos[2];
    int targetPos[2];
    int ownPos[2];
    int offsets[4][2];

    damage = 0.0f;
    if (obj->type == 10 || obj->type == 5)
        return;
    if (obj->health > legoobject_zero) {
        if (!(obj->flags3 & 0x80000000))
            legoobject_damage_apply(obj, elapsed * (entity_stats_float_at80(LEGOOBJ_SLOT(obj)) * 0.04f), 0, elapsed);
    } else {
        obj->health = 0.0f;
    }
    i = 0;
    if (obj->energy > legoobject_zero) {
        if (!(obj->flags3 & 0x80000000))
            obj->energy -= elapsed * (entity_energy_decay_rate_get(LEGOOBJ_SLOT(obj)) * 0.04f);
    } else {
        obj->energy = 0.0f;
    }

    legoobject_position_to_terrain_cell(obj, &blockPos[0], &blockPos[1]);
    if (obj->carryingThisObject == NULL &&
        LegoObjG_currLevel->blocks[LegoObjG_currLevel->width * blockPos[1] + blockPos[0]].terrain == 6 &&
        (obj->type == 7 || obj->type == 6))
    {
        level_state_field9c_add(1);
        obj->health = -1.0f;
        obj->flags3 |= 0x800000;
    }
    if (entity_cell_type_speed_multiplier_resolve_current(LEGOOBJ_SLOT(obj), blockPos[0], blockPos[1], elapsed, &damage))
        legoobject_damage_apply(obj, damage, 1, elapsed);
    if ((entity_state_flags_get(LEGOOBJ_SLOT(obj)) & 0x10000) && !(obj->flags1 & 0x100100) &&
        !(obj->flags2 & 0x8000000))
    {
        electric_fence_object_strike((struct ElectricFenceObject*)obj);
    }

    if (!(s_panel_highlight_flags & 0x200000) && obj->type == 2 &&
        obj->energy < (obj->health * 0.01f) * legoGlobs_MinEnergyForEat &&
        obj->field_3dc > 125.0f && (obj->field_3dc = 0.0f, obj->driveObject == NULL) &&
        !aitask_entity_task_chain_contains_kind(LEGOOBJ_SLOT(obj), 0x11) &&
        (target = legoobject_candidate_search(obj)) != NULL)
    {
        offsets[0][0] = 0;
        offsets[0][1] = -1;
        offsets[1][0] = 1;
        offsets[1][1] = 0;
        offsets[2][0] = 0;
        offsets[2][1] = 1;
        offsets[3][0] = -1;
        offsets[3][1] = 0;
        legoobject_position_to_terrain_cell(obj, &ownPos[0], &ownPos[1]);
        legoobject_position_to_terrain_cell(target, &targetPos[0], &targetPos[1]);
        for (; i < 4; i++) {
            if (((int (*)(Lego_Object*, unsigned int, unsigned int, unsigned int, unsigned int,
                          int**, int**, float*, void*, void*))legoobject_terrain_neighbor_path_check)(
                    obj, ownPos[0], ownPos[1], targetPos[0] + offsets[i][0], offsets[i][1] + targetPos[1],
                    &pathX, &pathY, &pathCount, NULL, NULL))
            {
                free(pathX);
                free(pathY);
                aitask_entity_target_command_queue(LEGOOBJ_SLOT(obj), LEGOOBJ_SLOT(target));
                break;
            }
        }
    }
    if (obj->energy > obj->health)
        obj->energy = obj->health;
    legoobject_damage_hurt_sound_submit(obj, elapsed, damage);
    if ((obj->flags2 & 0x2000) && !(obj->flags2 & 0x1000)) {
        quad_mesh_render_record_create(obj, (int)obj->field_3d0);
        obj->field_3d0 = 0.0f;
        obj->flags2 &= ~0x2000;
    }
    obj->flags2 &= ~0x1000;
}

/* ---- functions, group K (0043fb40-00440c70; part 2, issue #588) ---- */

/* LegoObject_UpdateHurtSound?: a minifigure taking damage plays SND_Hurt
 * once, then lets the sound run out (twice its length, at 1/25 speed)
 * before the next. */
// FUNCTION: LEGORR 0x0043fb40
int legoobject_damage_hurt_sound_submit(Lego_Object* obj, float elapsed, float damage)
{
    int sfx;

    if (obj->type == 2 && damage != legoobject_zero) {
        // STRING: LEGORR 0x004a50a8
        if (sound_alias_lookup("SND_Hurt", &sfx)) {
            if (obj->field_3fc != legoobject_zero) {
                obj->field_3fc -= elapsed * -0.04f;
                if (obj->field_3fc > obj->field_400 + obj->field_400) {
                    obj->field_3fc = 0.0f;
                    obj->field_400 = 0.0f;
                    return 1;
                }
            } else {
                sound_submission_route_wrapper(legoobject_type_component_get(obj), sfx, 0, 1, NULL);
                obj->field_3fc += elapsed;
                obj->field_400 = (float)sound_object_duration_get(sfx);
            }
        }
    }
    return 1;
}

/* Get out of the object carrying this one: on a free block, take the
 * carrier's pose, release the carrier and follow it. */
// FUNCTION: LEGORR 0x0043fc20
int legoobject_record_pair_pose_copy_and_activity_raise(Lego_Object* obj)
{
    Lego_Object* carrier;
    void* cont;
    void* objCont;
    void* carrierCont;
    int blockPos[2];
    float pos[3];
    float up[3];
    float dir[3];

    carrier = obj->carryingThisObject;
    if (carrier != NULL) {
        cont = legoobject_k_carry_null_get(carrier);
        if (cont != NULL) {
            ddiscene_object_position_get(cont, NULL, pos);
            if (terrain_map_world_to_block(Lego_GetMap(), pos[0], pos[1], &blockPos[0], &blockPos[1])) {
                if (terrain_cell_route_and_occupancy_check_current(LEGOOBJ_SLOT(obj), blockPos[0], blockPos[1], blockPos[0], blockPos[1], 0)) {
                    objCont = legoobject_type_component_get(obj);
                    if (objCont != NULL) {
                        carrierCont = legoobject_type_component_get(carrier);
                        if (carrierCont != NULL) {
                            obj->carryingThisObject = NULL;
                            carrier->carriedObjects[0] = NULL;
                            carrier->flags1 &= ~0x400;
                            carrier->numCarriedObjects = 0;
                            ddiscene_object_position_get(carrierCont, NULL, pos);
                            ddiscene_object_facing_pair_get(carrierCont, NULL, dir, up);
                            ddiscene_object_position_set(objCont, NULL, pos[0], pos[1], pos[2]);
                            ddiscene_object_pose_method_dispatch(objCont, NULL, dir[0], dir[1], dir[2], up[0], up[1], up[2]);
                            legoobject_activity_toggle(carrier, 1);
                            aitask_ai_idle_chain_record_attach(LEGOOBJ_SLOT(carrier));
                            legoobject_set_activity(obj, 0x34, 0);
                            legoobject_activity_register_bookkeeping(obj);
                            aitask_ai_idle_chain_record_attach(LEGOOBJ_SLOT(obj));
                            obj->routeToObject = carrier;
                            obj->flags4 |= 0x8000;
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/* Can the vehicle this object carries be dropped on its block? */
// FUNCTION: LEGORR 0x0043fdc0
int legoobject_action_target_cell_is_valid(Lego_Object* obj)
{
    void* cont;
    int blockPos[2];
    float pos[3];

    if ((entity_capability_flags_get(LEGOOBJ_SLOT(obj)) & 1) && (obj->flags1 & 0x400) &&
        obj->carriedObjects[0] != NULL && obj->carriedObjects[0]->type == 1)
    {
        cont = legoobject_k_carry_null_get(obj);
        if (cont != NULL) {
            ddiscene_object_position_get(cont, NULL, pos);
            if (terrain_map_world_to_block(Lego_GetMap(), pos[0], pos[1], &blockPos[0], &blockPos[1])) {
                if (terrain_cell_route_and_occupancy_check_current(LEGOOBJ_SLOT(obj->carriedObjects[0]), blockPos[0], blockPos[1], blockPos[0], blockPos[1], 1))
                    return 1;
            }
        }
    }
    return 0;
}

/* Route the object to the block under the target's carry null, to pick
 * the target up (route action 0x15). */
// FUNCTION: LEGORR 0x0043fe70
int legoobject_route_to_explicit_target(Lego_Object* obj, Lego_Object* target)
{
    int result;
    void* cont;
    unsigned int* xs;
    unsigned int* ys;
    unsigned int count;
    int bx;
    int by;
    int objX;
    int objY;
    float pos[3];

    result = 0;
    if ((entity_capability_flags_get(LEGOOBJ_SLOT(obj)) & 2) &&
        (entity_capability_flags_get(LEGOOBJ_SLOT(target)) & 1) && obj->carryingThisObject == NULL)
    {
        legoobject_position_to_terrain_cell(obj, &objX, &objY);
        cont = legoobject_k_carry_null_get(target);
        if (cont != NULL) {
            ddiscene_object_position_get(cont, NULL, pos);
            if (terrain_map_world_to_block(Lego_GetMap(), pos[0], pos[1], &bx, &by)) {
                if (bx == objX && by == objY) {
                    result = legoobject_route_setup_and_marker_associate(obj, 1, (unsigned int*)&bx, (unsigned int*)&by, pos);
                } else if (legoobject_terrain_neighbor_path_check(obj, objX, objY, bx, by, &xs, &ys, &count, NULL, NULL)) {
                    result = legoobject_route_setup_and_marker_associate(obj, count, xs, ys, pos);
                    free(xs);
                    free(ys);
                } else {
                    return 0;
                }
                if (result) {
                    obj->routeToObject = target;
                    ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].actionFlags = 0x15;
                    legoobject_activity_toggle(target, 1);
                    return 1;
                }
            }
        }
    }
    return result;
}

/* Route the object to the nearest route marker candidate (route action
 * 0x13). */
// FUNCTION: LEGORR 0x0043fff0
int legoobject_route_marker_setup(Lego_Object* obj)
{
    Lego_Object* target;
    void* cont;
    int result;
    unsigned int count;
    int bx;
    int by;
    int ownX;
    int ownY;
    float pos[3];
    unsigned int* xs;
    unsigned int* ys;

    legoobject_position_to_terrain_cell(obj, &ownX, &ownY);
    target = legoobject_route_marker_candidate_find(obj, 1);
    if (target != NULL && (cont = legoobject_k_carry_null_get(target)) != NULL) {
        ddiscene_object_position_get(cont, NULL, pos);
        if (terrain_map_world_to_block(Lego_GetMap(), pos[0], pos[1], &bx, &by)) {
            if (bx == ownX && by == ownY) {
                result = legoobject_route_setup_and_marker_associate(obj, 1, (unsigned int*)&bx, (unsigned int*)&by, pos);
            } else {
                if (!legoobject_terrain_neighbor_path_check(obj, ownX, ownY, bx, by, &xs, &ys, &count, NULL, NULL))
                    return 0;
                result = legoobject_route_setup_and_marker_associate(obj, count, xs, ys, pos);
                free(xs);
                free(ys);
            }
            if (result) {
                obj->routeToObject = target;
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].actionFlags = 0x13;
                return 1;
            }
        }
    }
    return 0;
}

/* Every live object standing on the block is sent to routeTo
 * (LegoObject_FUN_004440a0) and leaves the live list. */
// FUNCTION: LEGORR 0x00440130
void legoobject_update_and_unregister_at_tile(Lego_Object* routeTo, int* blockPos)
{
    unsigned int i;
    Lego_Object* obj;
    int objBlockPos[2];

    for (i = 0; i < LegoObjG_liveCount; i++) {
        obj = LegoObjG_liveObjs[i];
        if (obj != NULL) {
            legoobject_position_to_terrain_cell(obj, &objBlockPos[0], &objBlockPos[1]);
            if (objBlockPos[0] == blockPos[0] && objBlockPos[1] == blockPos[1]) {
                obj->routeToObject = routeTo;
                legoobject_linked_flag_and_command_update(obj);
                LegoObjG_liveObjs[i] = NULL;
            }
        }
    }
}

/* Get the driver out of the vehicle: on a block the driver can use, at
 * the dock of the building the vehicle is routing to, or on the vehicle's
 * unload block. */
// FUNCTION: LEGORR 0x004401b0
int legoobject_terrain_cell_to_world_position(Lego_Object* obj, int noUnloadCell)
{
    Lego_Object* driver;
    void* cont;
    void* driverCont;
    Lego_Object* building;
    int blockPos[2];
    int unloadPos[2];
    float pos[3];
    float dir[3];

    driver = obj->driveObject;
    if (!(entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x20000))
        return 0;
    if (driver == NULL)
        return 0;
    if (driver->flags1 & 1)
        return 0;
    cont = legoobject_k_carry_null_get(obj);
    driverCont = legoobject_type_component_get(driver);
    ddiscene_object_position_get(cont, NULL, pos);
    if (!terrain_map_world_to_block(Lego_GetMap(), pos[0], pos[1], &blockPos[0], &blockPos[1]))
        return 0;
    if ((entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x2000000) &&
        terrain_cell_route_and_occupancy_check_current(LEGOOBJ_SLOT(driver), blockPos[0], blockPos[1], blockPos[0], blockPos[1], 1))
        goto getOut;
    if (obj->flags4 & 0x40) {
        building = obj->routeToObject;
        if (building == NULL || building->type != 4)
            return 0;
        cont = scene_object_secondary_handle_get_or_create(building->building);
        if (cont == NULL)
            return 0;
        ddiscene_object_position_get(cont, NULL, pos);
        ddiscene_object_facing_pair_get(cont, NULL, dir, NULL);
        goto getOut;
    }
    if (!noUnloadCell && legoobject_vehicle_unload_cell_find_if_eligible(obj, unloadPos)) {
        terrain_map_block_to_world(Lego_GetMap(), unloadPos[0], unloadPos[1], &pos[0], &pos[1]);
        goto getOut;
    }
    if (!(entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x2000000))
        aitask_object_attached_record_allocate(LEGOOBJ_SLOT(obj));
    return 0;

getOut:
    obj->driveObject = NULL;
    driver->driveObject = NULL;
    driver->flags2 &= ~8;
    ddiscene_object_position_set(driverCont, NULL, pos[0], pos[1],
        terrain_map_world_z_get(Lego_GetMap(), pos[0], pos[1]));
    ddiscene_object_pose_method_dispatch(driverCont, NULL, dir[0], dir[1], 0.0f, 0.0f, 0.0f, -1.0f);
    legoobject_set_activity(driver, 0, 0);
    driver->activityName2 = NULL;
    legoobject_activity_register_bookkeeping(driver);
    selection_entity_remove_active(LEGOOBJ_SLOT(obj));
    return 1;
}

/* Route a minifigure to the vehicle it is to drive (route action 0xc):
 * to the side of the vehicle's block the vehicle faces, or to its dock;
 * a minifigure that can enter a vehicle at the water's edge goes to the
 * nearest block it can reach next to it. */
// FUNCTION: LEGORR 0x004403d0
int legoobject_level_debug_cell_retarget_dispatch(Lego_Object* obj, Lego_Object* target)
{
    void* cont;
    int result;
    int dir;
    LegoObjI_AITask* task;
    Lego_Object* building;
    int bx;
    int by;
    unsigned int side;
    unsigned int count;
    int targetX;
    int targetY;
    int ownX;
    int ownY;
    float facing[3];
    float pos[3];
    float ownPos[2];
    unsigned int* xs;
    unsigned int* ys;

    cont = NULL;
    task = (LegoObjI_AITask*)obj->aiTask;
    if ((task == NULL || (task->taskType == 0x12 && LEGOOBJ_FIELD(task, int, 0x60) == 0)) &&
        obj->driveObject == NULL && target->driveObject == NULL && obj->type == 2)
    {
        if ((obj->abilityFlags & 5) && (entity_stats_word_get(LEGOOBJ_SLOT(target)) & 0x2000000) &&
            (entity_stats_word_get(LEGOOBJ_SLOT(target)) & 0x20000) && !(target->flags4 & 0x40))
        {
            cont = legoobject_type_component_get(target);
        } else if (obj->abilityFlags & 2) {
            if ((entity_stats_word_get(LEGOOBJ_SLOT(target)) & 0x20000) && (target->flags4 & 0x40) &&
                (building = target->routeToObject) != NULL && building->type == 4)
            {
                cont = scene_object_secondary_handle_get_or_create(building->building);
            } else if ((obj->abilityFlags & 2) && (entity_stats_word_get(LEGOOBJ_SLOT(target)) & 0x20000) &&
                       legoobject_record_flag20_query(target, NULL))
            {
                legoobject_position_to_terrain_cell(target, &bx, &by);
                if (legoobject_nearest_connected_cell_route_select(obj,
                        bx, by, 0, &side, 1))
                {
                    ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].blockOffset[1] = 0.5f;
                    ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].blockOffset[0] = 0.5f;
                    switch (side) {
                    case 0:
                        ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].blockOffset[1] = 0.1f;
                        break;
                    case 1:
                        ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].blockOffset[0] = 0.1f;
                        break;
                    case 2:
                        ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].blockOffset[1] = 0.9f;
                        break;
                    case 3:
                        ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].blockOffset[0] = 0.9f;
                        break;
                    }
                    ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].flags |= 8;
                    ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].actionFlags = 0xc;
                    obj->routeToObject = target;
                    return 1;
                }
            }
        }

        if (cont != NULL) {
            ddiscene_object_position_get(cont, NULL, pos);
            ddiscene_object_facing_pair_get(cont, NULL, facing, NULL);
            if (fabs(facing[0]) > fabs(facing[1])) {
                if (facing[0] > legoobject_zero)
                    dir = 1;
                else
                    dir = 3;
            } else {
                if (facing[1] > legoobject_zero)
                    dir = 0;
                else
                    dir = 2;
            }
            terrain_map_world_to_block(Lego_GetMap(), pos[0], pos[1], &targetX, &targetY);
            legoobject_transform_position_get(obj, &ownPos[0], &ownPos[1]);
            terrain_map_world_to_block(Lego_GetMap(), ownPos[0], ownPos[1], &ownX, &ownY);
            if (ownX == targetX && ownY == targetY) {
                result = legoobject_route_setup_and_marker_associate(obj, 1, (unsigned int*)&targetX, (unsigned int*)&targetY, pos);
            } else {
                if (!legoobject_terrain_neighbor_path_check(obj, ownX, ownY, targetX, targetY, &xs, &ys, &count, NULL, NULL))
                    return 0;
                result = legoobject_route_setup_and_marker_associate(obj, count, xs, ys, pos);
                free(xs);
                free(ys);
            }
            if (result) {
                obj->routeToObject = target;
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].actionFlags = 0xc;
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].flags |= 4;
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].flags |= (dir & 3);
                return 1;
            }
        }
    }
    return 0;
}

/* Damage every object (flags3 0x40000) within radius of obj, scaled
 * down to nothing at the edge; rock monsters within eventRadius also get
 * event 31. */
// FUNCTION: LEGORR 0x004407b0
void legoobject_spatial_callback_query(Lego_Object* obj, float radius, float damage, float eventRadius)
{
    LegoObjK_SearchData search;

    search.obj = obj;
    legoobject_transform_position_get(obj, &search.x, &search.y);
    search.radius = radius;
    search.damage = damage;
    search.eventRadius = eventRadius;
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_spatial_callback_query_visit, &search);
}

// FUNCTION: LEGORR 0x00440800
int legoobject_spatial_callback_query_visit(Lego_Object* obj, void* data)
{
    LegoObjK_SearchData* search;
    float pos[2];
    float dx;
    float dy;
    float dist;

    search = (LegoObjK_SearchData*)data;
    if (obj != search->obj && (obj->flags3 & 0x40000)) {
        legoobject_transform_position_get(obj, &pos[0], &pos[1]);
        dy = pos[1] - search->y;
        dx = pos[0] - search->x;
        dist = (float)sqrt(dy * dy + dx * dx);
        if (dist < search->radius)
            legoobject_damage_apply(obj, (1.0f - dist / search->radius) * search->damage, 1, 1.0f);
        if (obj->type == 3 && dist < search->eventRadius)
            legoobject_dispatch_event31_for_pending_flag(obj);
    }
    return 0;
}

/* Push every movable object (state flag 1) within radius of the position
 * (or of obj) away from it. */
// FUNCTION: LEGORR 0x004408c0
void legoobject_spatial_callback_query_at_optional_coordinate(Lego_Object* obj, float* position, float radius)
{
    LegoObjK_SearchData search;

    search.obj = obj;
    if (position != NULL) {
        search.x = position[0];
        search.y = position[1];
    } else {
        legoobject_transform_position_get(obj, &search.x, &search.y);
    }
    search.radius = radius;
    legoobject_run_through_lists_skip_upgrade_parts(legoobject_spatial_callback_query_at_optional_coordinate_visit, &search);
}

// FUNCTION: LEGORR 0x00440920
int legoobject_spatial_callback_query_at_optional_coordinate_visit(Lego_Object* obj, void* data)
{
    LegoObjK_SearchData* search;
    float dist;
    float vec[2];
    float pos[2];
    float scale;

    search = (LegoObjK_SearchData*)data;
    if (obj != search->obj && (entity_state_flags_get(LEGOOBJ_SLOT(obj)) & 1)) {
        legoobject_transform_position_get(obj, &pos[0], &pos[1]);
        vec[0] = pos[0] - search->x;
        vec[1] = pos[1] - search->y;
        dist = (float)sqrt(vec[0] * vec[0] + vec[1] * vec[1]);
        if (dist < search->radius) {
            if (obj->type == 3)
                legoobject_dispatch_event31_for_pending_flag(obj);
            scale = 1.0f / dist;
            vec[0] = vec[0] * scale;
            vec[1] = vec[1] * scale;
            legoobject_flagged_vector_step_command_issue(obj, vec);
        }
    }
    return 0;
}

/* LegoObject_SetActivity. */
// FUNCTION: LEGORR 0x004409e0
void legoobject_set_activity(Lego_Object* obj, int activity, int restart)
{
    obj->activityName1 = s_activity[activity];
    obj->animRepeat = restart;
    obj->animTime = 0.0f;
}

/* Recompute each carried object's direction from its carry null. */
// FUNCTION: LEGORR 0x00440a10
void legoobject_attached_direction_vector_update(Lego_Object* obj)
{
    unsigned int i;
    Lego_Object* carried;
    void* null;
    void* cont;
    float len;
    LegoObjL_Vector3F pos;

    for (i = 0; i < obj->numCarriedObjects; i++) {
        carried = obj->carriedObjects[i];
        if (!(carried->flags3 & 0x2000)) {
            if (obj->type == 3) {
                null = renderer_cached_handle_48_get_or_create(obj->rockMonster);
            } else if (obj->type == 2) {
                if (obj->driveObject != NULL && obj->driveObject->type == 1)
                    null = attached_entity_slot_object_get((struct SceneResourceAnimationRecord*)obj->driveObject->vehicle,
                        stats_instance_word_at_f0(obj->driveObject));
                else
                    null = renderer_cached_handle_48_get_or_create(obj->miniFigure);
            } else if (obj->type == 4) {
                null = scene_object_indexed_handle_get_or_create(obj->building, carried->carryingIndex);
            } else if (obj->type == 1) {
                null = attached_entity_slot_object_get((struct SceneResourceAnimationRecord*)obj->vehicle,
                    stats_instance_word_at_f0(obj) + carried->carryingIndex);
            }
            if (null != NULL) {
                cont = legoobject_type_component_get(carried);
                ddiscene_object_position_get(cont, NULL, &carried->field_2a0);
                ddiscene_object_position_set(cont, null, 0.0f, 0.0f, 0.0f);
                ddiscene_object_pose_method_dispatch(cont, null, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
                ddiscene_object_position_get(cont, NULL, &pos.x);
                legoobject_vector3d_subtract(&pos.x, &pos.x, &carried->field_2a0);
                len = legoobject_vector3d_modulus(&pos.x);
                carried->faceDirectionLength = len;
                legoobject_vector3d_scale((LegoObjL_Vector3F*)carried->faceDirection, &pos, 1.0f / len);
            }
        }
    }
}

/* Set every texture frame of a type-5 object's animated texture. */
// FUNCTION: LEGORR 0x00440bf0
void legoobject_object_list_field_assign_all(Lego_Object* obj, int value)
{
    unsigned int count;
    unsigned int i;

    if (obj->type == 5) {
        count = animated_texture_frame_count(obj->other);
        for (i = 0; i < count; i++)
            animated_texture_frame_set(obj->other, i, (const void*)value);
    }
}

/* LegoObject_Route_ScoreNoCallback without the callback. */
// FUNCTION: LEGORR 0x00440c30
int legoobject_grid_route_search_adapter(Lego_Object* obj, unsigned int startX, unsigned int startY, unsigned int endX, unsigned int endY, unsigned int** outXs, unsigned int** outYs, unsigned int* outCount)
{
    return legoobject_terrain_neighbor_path_check(obj, startX, startY, endX, endY, outXs, outYs, outCount, NULL, NULL);
}

/* LegoObject_Route_Score?: a breadth-first search over the level's blocks
 * from (startX, startY), numbering each reached block with its step in
 * objectGlobs' step map (004dee04, count 004dee08), until it reaches
 * (endX, endY) or the callback accepts a block; then walk the steps back
 * down from the end into two new coordinate lists. */
// FUNCTION: LEGORR 0x00440c70
int legoobject_terrain_grid_path_search(Lego_Object* obj, unsigned int startX, unsigned int startY, unsigned int endX, unsigned int endY, unsigned int** outXs, unsigned int** outYs, unsigned int* outCount, int (*callback)(Lego_Object* obj, int* blockPos, void* data), void* data)
{
    unsigned int* countPtr;
    int found;
    unsigned int current;
    unsigned int step;
    unsigned int j;
    unsigned int width;
    unsigned int height;
    unsigned int counts[2];
    int blockPos[2];
    int DIRECTIONS[4][2];
    LegoObjG_RouteBlock lists[2][100];
    unsigned int* map;
    int i;
    int routeOk;
    unsigned int x;
    unsigned int y;
    unsigned int* cell;
    unsigned int other;
    unsigned int* xs;
    unsigned int* ys;
    unsigned int k;
    int n;
    int dir;
    unsigned int cx;
    unsigned int cy;

    DIRECTIONS[0][0] = 0;
    DIRECTIONS[0][1] = -1;
    DIRECTIONS[1][0] = 1;
    DIRECTIONS[1][1] = 0;
    DIRECTIONS[2][0] = 0;
    DIRECTIONS[2][1] = 1;
    DIRECTIONS[3][0] = -1;
    DIRECTIONS[3][1] = 0;
    found = 0;

    map = (unsigned int*)Lego_GetMap();
    width = map[0];
    height = map[1];
    if (startX >= width || startY >= height)
        return 0;
    if (callback == NULL && (endX >= width || endY >= height ||
        !terrain_cell_route_and_occupancy_check_current(LEGOOBJ_SLOT(obj), endX, endY, endX, endY, 0)))
        return 0;

    if (s_terrain_path_depth == NULL || s_terrain_path_depth_count != width * height) {
        if (s_terrain_path_depth != NULL)
            free(s_terrain_path_depth);
        s_terrain_path_depth_count = width * height;
        s_terrain_path_depth = (unsigned int*)malloc(s_terrain_path_depth_count * sizeof(unsigned int));
    }
    if (s_terrain_path_depth != NULL) {
        memset(s_terrain_path_depth, 0, width * height * sizeof(unsigned int));

        current = 0;
        s_terrain_path_depth[width * startY + startX] = 1;
        lists[0][0].blockPos[0] = startX;
        lists[0][0].blockPos[1] = startY;
        counts[0] = 1;
        counts[1] = 0;
        countPtr = &counts[0];
        step = 1;
        do {
            step++;
            for (j = 0; j < *countPtr; j++) {
                for (i = 0; i < 4; i++) {
                    x = lists[current][j].blockPos[0];
                    y = lists[current][j].blockPos[1];
                    if (i == 0) y--;
                    else if (i == 1) x++;
                    else if (i == 2) y++;
                    else if (i == 3) x--;

                    if (x < width && y < height) {
                        routeOk = terrain_cell_route_and_occupancy_check_current(LEGOOBJ_SLOT(obj), x, y,
                            x + DIRECTIONS[i][0], y + DIRECTIONS[i][1], 0);
                        cell = &s_terrain_path_depth[y * width + x];
                        if (*cell == 0 && routeOk) {
                            other = (current == 0);
                            lists[other][counts[other]].blockPos[0] = x;
                            lists[other][counts[other]].blockPos[1] = y;
                            counts[other]++;
                            *cell = step;
                            if (callback != NULL) {
                                blockPos[0] = x;
                                blockPos[1] = y;
                                if (callback(obj, blockPos, data)) {
                                    found = 1;
                                    endX = x;
                                    endY = y;
                                }
                            } else if (x == endX && y == endY) {
                                found = 1;
                            }
                        }
                    }
                }
            }
            *countPtr = 0;
            current = (current == 0);
            countPtr = &counts[current];
            if (*countPtr == 0)
                found = -1;
        } while (found == 0);

        if (found == -1)
            return 0;

        xs = (unsigned int*)malloc(step * sizeof(unsigned int));
        if (xs != NULL && (ys = (unsigned int*)malloc(step * sizeof(unsigned int))) != NULL) {
            k = step - 1;
            cx = endX;
            cy = endY;
            xs[k] = cx;
            ys[k] = cy;
            xs[0] = startX;
            ys[0] = startY;
            dir = 0;
            for (; k != 1; k--) {
                for (n = 0; n < 4; n++) {
                    x = cx;
                    y = cy;
                    if (dir == 0) y = cy - 1;
                    else if (dir == 1) x = cx + 1;
                    else if (dir == 2) y = cy + 1;
                    else if (dir == 3) x = cx - 1;
                    if (x < width && y < height && s_terrain_path_depth[y * width + x] == k) {
                        xs[k - 1] = x;
                        ys[k - 1] = y;
                        cx = x;
                        cy = y;
                        break;
                    }
                    if (++dir >= 4)
                        dir = 0;
                }
            }
            *outXs = xs;
            *outYs = ys;
            *outCount = step;
        }
    }
    return 1;
}

/* ---- functions, group L (004410f0-00442560; part 2, issue #588) ---- */

/* LegoObject_Route_ScoreNoCallback: a breadth-first search over the
 * level's blocks from (startX, startY), scoring each reached block in
 * objectGlobs' cost map (004dee00), until it reaches (endX, endY) or the
 * callback accepts a block. Then walk the scores back down from the end
 * into two new coordinate lists. A route whose first step is not to an
 * adjacent block is searched again by legoobject_terrain_grid_path_search. */
// FUNCTION: LEGORR 0x004410f0
int legoobject_terrain_neighbor_path_check(Lego_Object* obj, unsigned int startX, unsigned int startY, unsigned int endX, unsigned int endY, unsigned int** outXs, unsigned int** outYs, unsigned int* outCount, LegoObjL_RouteCallback callback, void* data)
{
    int i;
    unsigned int* countPtr;
    unsigned int width;
    unsigned int current;
    unsigned int step;
    float stepCost;
    unsigned int height;
    int found;
    int result;
    unsigned int j;
    float cost;
    unsigned int counts[2];
    unsigned int foundStep;
    int blockPos[2];
    int nextPos[2];
    int DIRECTIONS[4][2];
    LegoObjG_RouteBlock lists[2][100];
    unsigned int x;
    unsigned int y;
    int tx;
    int ty;
    unsigned int* map;

    DIRECTIONS[0][0] = 0;
    DIRECTIONS[0][1] = -1;
    DIRECTIONS[1][0] = 1;
    DIRECTIONS[1][1] = 0;
    DIRECTIONS[2][0] = 0;
    DIRECTIONS[2][1] = 1;
    DIRECTIONS[3][0] = -1;
    DIRECTIONS[3][1] = 0;
    result = 0;
    found = 0;

    map = (unsigned int*)Lego_GetMap();
    width = map[0];
    height = map[1];
    if (startX >= width || startY >= height)
        return 0;
    if (callback == NULL) {
        if (endX >= width || endY >= height)
            return 0;
        if (!terrain_cell_route_and_occupancy_check_current(LEGOOBJ_SLOT(obj), endX, endY, endX, endY, 0))
            return 0;
    }

    if (s_terrain_neighbor_path_cost == NULL || s_terrain_path_depth_count != width * height) {
        if (s_terrain_neighbor_path_cost != NULL)
            free(s_terrain_neighbor_path_cost);
        s_terrain_path_depth_count = width * height;
        s_terrain_neighbor_path_cost = (float*)malloc(s_terrain_path_depth_count * sizeof(float));
    }
    if (s_terrain_neighbor_path_cost != NULL) {
        memset(s_terrain_neighbor_path_cost, 0, width * height * sizeof(float));

        step = 1;
        counts[0] = 1;
        current = 0;
        s_terrain_neighbor_path_cost[width * startY + startX] = 1.0f;
        lists[0][0].blockPos[0] = startX;
        lists[0][0].blockPos[1] = startY;
        counts[1] = 0;
        countPtr = &counts[0];
        do {
            step++;
            for (j = 0; j < *countPtr; j++) {
                cost = s_terrain_neighbor_path_cost[width * lists[current][j].blockPos[1] + lists[current][j].blockPos[0]];
                for (i = 0; i < 4; i++) {
                    x = lists[current][j].blockPos[0];
                    y = lists[current][j].blockPos[1];
                    if (i == 0) y--;
                    else if (i == 1) x++;
                    else if (i == 2) y++;
                    else if (i == 3) x--;

                    if (x < width && y < height) {
                        stepCost = 1.0f;
                        tx = x + DIRECTIONS[i][0];
                        ty = y + DIRECTIONS[i][1];
                        nextPos[0] = tx;
                        nextPos[1] = ty;
                        if (level_cell_occupied_at_current(nextPos))
                            stepCost = 0.5f;
                        if (terrain_cell_route_and_occupancy_check_current(LEGOOBJ_SLOT(obj), x, y, tx, ty, 0)) {
                            float* score = &s_terrain_neighbor_path_cost[width * y + x];
                            if (*score == legoobject_zero || *score > stepCost + cost) {
                                unsigned int other = (current == 0);
                                *score = stepCost + cost;
                                lists[other][counts[other]].blockPos[0] = x;
                                lists[other][counts[other]].blockPos[1] = y;
                                counts[other]++;
                                if (callback != NULL) {
                                    blockPos[0] = x;
                                    blockPos[1] = y;
                                    if (callback(obj, blockPos, data)) {
                                        result = 1;
                                        endX = x;
                                        endY = y;
                                    }
                                } else if (x == endX && y == endY) {
                                    found = 1;
                                    foundStep = step;
                                }
                            }
                        }
                    }
                }
            }
            *countPtr = 0;
            current = (current == 0);
            countPtr = &counts[current];
            if (counts[current] == 0)
                result = -1;
        } while (result == 0);

        if (found) {
            result = 1;
            step = foundStep;
        }
        if (result == -1)
            return 0;

        {
            unsigned int* xs;
            unsigned int* ys;
            unsigned int count;
            unsigned int last;
            unsigned int k;
            unsigned int cx;
            unsigned int cy;
            float best;
            float diff;
            unsigned int bestPos[2];

            count = step;
            xs = (unsigned int*)malloc(step * sizeof(unsigned int));
            if (xs != NULL && (ys = (unsigned int*)malloc(step * sizeof(unsigned int))) != NULL) {
                last = step - 1;
                xs[last] = endX;
                ys[last] = endY;
                xs[0] = startX;
                ys[0] = startY;
                cx = endX;
                cy = endY;
                for (k = last; k != 1; k--) {
                    best = 10000.0f;
                    for (i = 0; i < 4; i++) {
                        x = cx;
                        y = cy;
                        if (i == 0) y = cy - 1;
                        else if (i == 1) x = cx + 1;
                        else if (i == 2) y = cy + 1;
                        else if (i == 3) x = cx - 1;
                        blockPos[0] = x;
                        blockPos[1] = y;
                        if (x < width && y < height) {
                            diff = s_terrain_neighbor_path_cost[width * cy + cx] - s_terrain_neighbor_path_cost[width * y + x];
                            if ((diff > legoobject_zero && diff < best) ||
                                (diff == best && level_cell_occupied_at_current(blockPos))) {
                                best = diff;
                                bestPos[0] = x;
                                bestPos[1] = y;
                            }
                        }
                    }
                    xs[k - 1] = bestPos[0];
                    ys[k - 1] = bestPos[1];
                    cx = bestPos[0];
                    cy = bestPos[1];
                }
                *outXs = xs;
                *outYs = ys;
                *outCount = count;
                if ((xs[0] != xs[1] || abs((int)(ys[0] - ys[1])) != 1) &&
                    (ys[0] != ys[1] || abs((int)(xs[0] - xs[1])) != 1)) {
                    free(xs);
                    free(ys);
                    return legoobject_terrain_grid_path_search(obj, startX, startY, endX, endY,
                        outXs, outYs, outCount, callback, data);
                }
            }
        }
    }
    return 1;
}

/* LegoObject_Route_Begin: replace the object's route with the blocks of
 * the two coordinate lists. Each block gets its offset within the block:
 * the centre, a random point for objects whose stats allow it, or, for the
 * last block, wPos's. A block where the route turns, and the route check
 * from the next block to the previous one answers 2, is flagged 0x10. A
 * pair of blocks that are not neighbours fails the route. */
// FUNCTION: LEGORR 0x00441700
int legoobject_route_setup_and_marker_associate(Lego_Object* obj, unsigned int count, unsigned int* bxList, unsigned int* byList, float* wPos)
{
    unsigned int i;
    int prevDir;
    int failed;
    LegoObjG_RouteBlock* routeBlocks;
    float offset[2];
    int dir;

    if (legoobject_flags_mask107c33de_or_1c00203_any_test(obj))
        return 0;

    prevDir = -1;
    failed = 0;
    offset[0] = 0.5f;
    offset[1] = 0.5f;
    if (obj->flags4 & 0x40) {
        obj->routeToObject = NULL;
        obj->flags4 &= ~0x40;
    }

    routeBlocks = (LegoObjG_RouteBlock*)malloc(count * sizeof(LegoObjG_RouteBlock));
    if (routeBlocks != NULL) {
        for (i = 0; i < count; i++) {
            routeBlocks[i].blockPos[0] = bxList[i];
            routeBlocks[i].blockPos[1] = byList[i];
            if (wPos != NULL && i == count - 1) {
                terrain_map_world_to_block_offset(Lego_GetMap(), (struct TerrainMapPoint2*)wPos,
                    NULL, (struct TerrainMapPoint2*)offset);
                *(LegoObjGE_Point2F*)routeBlocks[i].blockOffset = *(LegoObjGE_Point2F*)offset;
            } else if (!(obj->flags3 & 0x1000) &&
                       (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x20000000)) {
                routeBlocks[i].blockOffset[0] = random_float_range(0.3f, 0.7f);
                routeBlocks[i].blockOffset[1] = random_float_range(0.3f, 0.7f);
            } else {
                routeBlocks[i].blockOffset[1] = 0.5f;
                routeBlocks[i].blockOffset[0] = 0.5f;
            }
            routeBlocks[i].flags = 0;
            routeBlocks[i].actionFlags = 0;
            if (i < count - 1) {
                dir = cell_neighbor_direction_code(bxList[i + 1], byList[i + 1], bxList[i], byList[i]);
                if (dir == -1) {
                    failed = 1;
                    break;
                }
                if (i != 0 && prevDir != dir &&
                    terrain_cell_route_and_occupancy_check_current(LEGOOBJ_SLOT(obj), bxList[i + 1], byList[i + 1],
                        bxList[i - 1], byList[i - 1], 0) == 2)
                {
                    routeBlocks[i].flags |= 0x10;
                }
                prevDir = dir;
            }
        }
    }

    if (!failed) {
        legoobject_action_sequence_reset(obj, 0);
        obj->flags1 |= 1;
        obj->flags3 &= ~0x400;
        obj->routeBlocks = routeBlocks;
        obj->routeBlocksTotal = count;
        obj->routeBlocksCurrent = 0;
        obj->field_290 = 0.0f;
        obj->field_28c = 0.0f;
        return 1;
    }
    free(routeBlocks);
    return 0;
}

/* LegoObject_Route_End: drop the object's route. An interrupted route
 * releases the object it was heading for, a rock monster's carried
 * boulder and AITask records (4 and 10) and the reserved target block; a
 * completed route of a minifigure faces it along its last direction. */
// FUNCTION: LEGORR 0x00441940
void legoobject_action_sequence_reset(Lego_Object* obj, int completed)
{
    unsigned int i;
    int blockPos[2];

    if (obj->routeBlocks != NULL) {
        if (completed) {
            if (obj->type == 2) {
                animation_object_transform_set((struct AnimationObject*)obj->miniFigure,
                    obj->field_298, obj->field_29c);
                obj->faceDirection[0] = obj->field_298;
                obj->faceDirection[1] = obj->field_29c;
            }
        } else {
            for (i = obj->routeBlocksCurrent; i < obj->routeBlocksTotal; i++) {
                unsigned char actionFlags = ((LegoObjG_RouteBlock*)obj->routeBlocks)[i].actionFlags;
                if (actionFlags == 3) {
                    if (obj->routeToObject != NULL) {
                        if (obj->routeToObject->type == 5)
                            legoobject_type5_finalize_if_allowed(obj->routeToObject);
                        else
                            obj->routeToObject->interactObject = NULL;
                        obj->routeToObject = NULL;
                    }
                    break;
                }
                if (actionFlags == 7) {
                    obj->routeToObject = NULL;
                    break;
                }
            }
            if (obj->type == 3) {
                if ((obj->flags1 & 0x400) && obj->carriedObjects[0]->type == 5)
                    legoobject_attached_object_list_detach(obj, 0);
                entity_activity_record_remove_by_type(obj, 4, NULL);
                entity_activity_record_remove_by_type(obj, 10, NULL);
            }
            if (obj->flags1 & 8) {
                blockPos[0] = (int)obj->targetBlockPos[0];
                blockPos[1] = (int)obj->targetBlockPos[1];
                level_tile_mark_current_target(blockPos, 0);
            }
            aitask_entity_task_queue_flush(LEGOOBJ_SLOT(obj));
        }
        free(obj->routeBlocks);
        obj->routeBlocks = NULL;
        aitask_entity_task_state_update_after_cleanup(LEGOOBJ_SLOT(obj), completed);
    }

    obj->routeBlocksTotal = 0;
    obj->routeBlocksCurrent = 0;
    obj->field_290 = 0.0f;
    obj->field_28c = 0.0f;
    obj->field_294 = 0.0f;
    obj->flags1 &= ~0x00800001;
    obj->flags3 &= ~0x4400;
    if (obj->routeToObject != NULL)
        obj->routeToObject->flags4 &= ~0x10;
    if (obj->flags1 & 2) {
        legoobject_vector3d_scale((LegoObjL_Vector3F*)obj->faceDirection,
            (LegoObjL_Vector3F*)obj->faceDirection, -1.0f);
        obj->flags1 &= ~2;
    }
    legoobject_set_activity(obj, 0, 0);
}

/* LegoObject_StopActivity: end the route and the activity states the
 * object is in (flags1-flags4), releasing the objects, AITask records and
 * blocks they hold. restart drops a leading kind-14 AITask record and
 * restarts activity 0; detach drops what the object carries first. */
// FUNCTION: LEGORR 0x00441b30
void legoobject_state_reset(Lego_Object* obj, int restart, int detach)
{
    int blockPos[2];

    legoobject_action_sequence_reset(obj, 0);
    if (detach)
        legoobject_attached_object_list_detach(obj, 0);

    if (obj->flags2 & 2) {
        obj->flags2 &= ~2;
        legoobject_set_activity(obj, 0x27, 0);
        legoobject_activity_register_bookkeeping(obj);
        obj->flags2 |= 0x100;
        if (((Lego_Object*)obj->field_3c0)->flags2 & 1) {
            ((Lego_Object*)obj->field_3c0)->flags2 &= ~1;
            legoobject_set_activity(obj, 0, 0);
        }
    }
    if (obj->flags2 & 1) {
        obj->flags2 &= ~1;
        legoobject_set_activity(obj, 0, 0);
        if (((Lego_Object*)obj->field_3c0)->flags2 & 2) {
            ((Lego_Object*)obj->field_3c0)->flags2 &= ~2;
            legoobject_set_activity((Lego_Object*)obj->field_3c0, 0x27, 0);
            legoobject_activity_register_bookkeeping((Lego_Object*)obj->field_3c0);
            ((Lego_Object*)obj->field_3c0)->flags2 |= 0x100;
        }
    }

    obj->flags2 &= ~0x01c04020;
    obj->flags1 &= ~0x90000800;
    if (obj->flags1 & 0x2000) {
        obj->flags1 &= ~0x2000;
        entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 5, obj, 1);
        legoobject_simulation_queue_flush(obj);
        if (obj->routeToObject != NULL)
            obj->routeToObject->flags3 &= ~0x01000000;
    }
    obj->flags1 &= ~0x20000200;
    obj->flags3 &= ~0x4000;
    if (obj->flags2 & 0x200000)
        obj->flags2 &= ~0x200000;
    obj->flags2 &= ~0x40000;
    if (obj->flags1 & 0x1000000) {
        obj->flags2 &= ~0x80000;
        entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 10, NULL, 1);
        obj->flags1 &= ~0x1000000;
    }
    if (obj->flags1 & 0x40000000)
        obj->flags1 &= ~0x40000000;
    if (obj->flags2 & 0x40)
        obj->flags2 &= ~0x40;
    if (obj->flags2 & 0x400)
        obj->flags2 &= ~0x400;
    if (obj->flags2 & 0x8000) {
        if (obj->routeToObject != NULL &&
            (entity_state_flags_get(LEGOOBJ_SLOT(obj->routeToObject)) & 2))
        {
            if (obj->routeToObject->flags2 & 0x8000)
                goto skipInteract;
            entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 0x15, NULL, 1);
            obj->routeToObject->interactObject = NULL;
            obj->routeToObject = NULL;
        }
        obj->flags2 &= ~0x8000;
    }
skipInteract:
    if (obj->flags1 & 0x80000) {
        obj->flags1 &= ~0x80000;
        legoobject_attached_object_list_detach(obj, 1);
    }
    obj->flags1 &= ~0x1000;
    if (obj->flags1 & 0x40) {
        blockPos[0] = (int)obj->targetBlockPos[0];
        blockPos[1] = (int)obj->targetBlockPos[1];
        obj->flags1 &= ~0x40;
        obj->flags1 |= 0x80000000;
        level_tile_mark_current_target(blockPos, 0);
    }
    obj->flags1 &= ~0x04000000;
    if (obj->flags2 & 4) {
        obj->routeToObject = NULL;
        obj->flags2 &= ~4;
    }
    obj->flags1 &= ~0x80058000;
    obj->flags2 &= ~0x00030300;
    if (restart) {
        task_record_kind14_remove_if_head(LEGOOBJ_SLOT(obj));
        legoobject_set_activity(obj, 0, 1);
        legoobject_activity_register_bookkeeping(obj);
    }
}

/* Remove a type-5 object (a boulder) after creating its marker effect,
 * unless objectGlobs.flags bit 2 holds removals off. */
// FUNCTION: LEGORR 0x00441ea0
void legoobject_type5_finalize_if_allowed(Lego_Object* obj)
{
    if (!(s_entity_state_flags & 2) && obj->type == 5) {
        scene_marker_create_at_object_position(obj);
        legoobject_remove(obj);
    }
}

/* An object whose weapon has recharged faces a rock monster (type 3) and
 * starts the fire activity (0x2c) of the weapon kind (1 laser, 2 pusher,
 * 3 freezer). The direction and target are kept for
 * legoobject_kind_effect_slot_init_dispatch. legoGlobs.flags2 bit 4 holds
 * fire off. */
// FUNCTION: LEGORR 0x00441ed0
int legoobject_face_target_action_start(Lego_Object* obj, Lego_Object* target, int kind)
{
    float dir[3];
    float targetPos[3];
    float objPos[3];
    float len;

    if (obj->field_360 > legoobject_zero || (s_panel_highlight_flags & 4) || target->type != 3)
        return 0;
    if (legoobject_flags_mask107c33de_or_1c00203_any_test(obj))
        return 0;

    legoobject_action_sequence_reset(obj, 0);
    ddiscene_object_position_get(legoobject_type_component_get(target), NULL, targetPos);
    ddiscene_object_position_get(legoobject_type_component_get(obj), NULL, objPos);
    objPos[2] -= entity_stats_float_at74(LEGOOBJ_SLOT(obj)) * 0.5f;
    targetPos[2] -= entity_stats_float_at74(LEGOOBJ_SLOT(target)) * 0.5f;

    dir[0] = targetPos[0] - objPos[0];
    dir[1] = targetPos[1] - objPos[1];
    dir[2] = targetPos[2] - objPos[2];
    len = 1.0f / (float)sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
    dir[0] *= len;
    dir[1] *= len;
    dir[2] *= len;
    legoobject_velocity_vector_dispatch(obj, 0.0f, dir);

    switch (kind) {
    case 1:
        legoobject_set_activity(obj, 0x2c, 1);
        legoobject_activity_register_bookkeeping(obj);
        obj->flags2 |= 0x400000;
        break;
    case 2:
        legoobject_set_activity(obj, 0x2c, 1);
        legoobject_activity_register_bookkeeping(obj);
        obj->flags2 |= 0x800000;
        break;
    case 3:
        legoobject_set_activity(obj, 0x2c, 1);
        legoobject_activity_register_bookkeeping(obj);
        obj->flags2 |= 0x1000000;
        break;
    }

    *(LegoObjL_Vector3F*)obj->weaponTarget = *(LegoObjL_Vector3F*)targetPos;
    *(LegoObjL_Vector3F*)obj->weaponDirection = *(LegoObjL_Vector3F*)dir;
    return 1;
}

/* Fire the weapon of the kind from the object's position, along its stored
 * direction (kinds 1-3) or at its stored target (kind 4), and start the
 * weapon's recharge. */
// FUNCTION: LEGORR 0x004420d0
void legoobject_kind_effect_slot_init_dispatch(Lego_Object* obj, int kind)
{
    float wPos[3];
    unsigned int weapon;

    legoobject_transform_position_get(obj, &wPos[0], &wPos[1]);
    wPos[2] = terrain_map_world_z_get(Lego_GetMap(), wPos[0], wPos[1]) -
              entity_stats_float_at74(LEGOOBJ_SLOT(obj)) * 0.5f;
    switch (kind) {
    case 1:
        weapon = object_type_name_index_find(
            // STRING: LEGORR 0x004a50b4
            "Lazer");
        entity_spawn_record_slot_init(obj, wPos, obj->weaponDirection, weapon, 1);
        break;
    case 2:
        weapon = object_type_name_index_find("Pusher");
        entity_spawn_record_slot_init(obj, wPos, obj->weaponDirection, weapon, 2);
        break;
    case 3:
        weapon = object_type_name_index_find("Freezer");
        entity_spawn_record_slot_init(obj, wPos, obj->weaponDirection, weapon, 3);
        break;
    case 4:
        weapon = object_type_name_index_find("Lazer");
        object_flight_curve_record_init(obj, wPos, obj->weaponTarget, weapon, 4);
        break;
    }
    obj->field_360 = weapon_type_recharge_time_get_target(weapon);
}

/* Stop the object (dropping what it carries), start activity 0x4d and
 * set its health to -1. */
// FUNCTION: LEGORR 0x00442210
void legoobject_action_route_state_initialize(Lego_Object* obj)
{
    legoobject_state_reset(obj, 0, 1);
    legoobject_set_activity(obj, 0x4d, 1);
    if (legoobject_activity_register_bookkeeping(obj))
        obj->flags1 |= 0x100000;
    obj->health = -1.0f;
}

/* LegoObject_GetPosition: the world x/y of the object's container. */
// FUNCTION: LEGORR 0x00442260
void legoobject_transform_position_get(Lego_Object* obj, float* outX, float* outY)
{
    void* cont;
    float wPos[3];

    cont = legoobject_type_component_get(obj);
    ddiscene_object_position_get(cont, NULL, wPos);
    *outX = wPos[0];
    *outY = wPos[1];
}

/* LegoObject_GetFaceDirection: the container's direction in the x/y plane,
 * normalised. */
// FUNCTION: LEGORR 0x004422a0
void legoobject_surface_vector_normalize(Lego_Object* obj, float* outDir)
{
    void* cont;
    float dir[3];
    float len;

    cont = legoobject_type_component_get(obj);
    ddiscene_object_facing_pair_get(cont, NULL, dir, NULL);
    len = 1.0f / (float)sqrt(dir[1] * dir[1] + dir[0] * dir[0]);
    outDir[0] = len * dir[0];
    outDir[1] = len * dir[1];
}

/* LegoObject_SetHeadingOrDirection: face the object along dir, or, with
 * no direction, the heading rotated from +y about -z. */
// FUNCTION: LEGORR 0x00442300
void legoobject_velocity_vector_dispatch(Lego_Object* obj, float heading, float* dir)
{
    LegoObjL_Vector3F* face;
    float v[3];
    float axis[3];

    if (dir != NULL) {
        v[0] = dir[0];
        v[1] = dir[1];
        v[2] = dir[2];
    } else {
        v[2] = 0.0f;
        v[0] = 0.0f;
        v[1] = 1.0f;
        axis[1] = 0.0f;
        axis[0] = 0.0f;
        axis[2] = -1.0f;
        d3drm_vector_rotate(v, v, axis, heading);
    }

    if (obj->type == 1)
        scene_resource_draw_at_origin((struct SceneResourceAnimationRecord*)obj->vehicle, v[0], v[1], v[2]);
    else if (obj->type == 2)
        animation_object_transform_set((struct AnimationObject*)obj->miniFigure, v[0], v[1]);
    else if (obj->type == 3)
        animation_object_transform_set((struct AnimationObject*)obj->rockMonster, v[0], v[1]);
    else if (obj->type == 4)
        scene_object_pair_set_render_parameters(obj->building, v[0], v[1]);
    else if (obj->flags3 & 0x20000)
        ddiscene_object_pose_method_dispatch(obj->other, NULL, v[0], v[1], v[2], 0.0f, 0.0f, -1.0f);

    obj->faceDirectionLength = 0.01f;
    face = (LegoObjL_Vector3F*)obj->faceDirection;
    *face = *(LegoObjL_Vector3F*)v;
    legoobject_vector3d_scale((LegoObjL_Vector3F*)&obj->field_2a0, face, -0.01f);
}

/* LegoObject_GetHeading: the angle of the face direction from +y,
 * clockwise, in [0, 2pi). */
// FUNCTION: LEGORR 0x00442480
float legoobject_vector_direction_angle_radians_get(Lego_Object* obj)
{
    float dir[2];
    float theta;

    legoobject_surface_vector_normalize(obj, dir);
    theta = (float)acos((0.0f * dir[0] + 1.0f * dir[1]) / ((float)sqrt(0.0 * 0.0 + 1.0 * 1.0) *
                                  (float)sqrt(dir[0] * dir[0] + dir[1] * dir[1])));
    if (dir[0] < legoobject_zero)
        theta = 6.2831855f - theta;
    return theta;
}

/* LegoObject_GetBlockPos: the block under the object. */
// FUNCTION: LEGORR 0x004424f0
void legoobject_position_to_terrain_cell(Lego_Object* obj, int* outX, int* outY)
{
    void* cont;
    float wPos[3];

    cont = legoobject_type_component_get(obj);
    ddiscene_object_position_get(cont, NULL, wPos);
    terrain_map_world_to_block(Lego_GetMap(), wPos[0], wPos[1], outX, outY);
}

/* LegoObject_GetWorldZCallback: the terrain height, for the container
 * position setters (group M). */
// FUNCTION: LEGORR 0x00442540
float legoobject_world_z_callback(float x, float y, struct TerrainMap* map)
{
    return terrain_map_world_z_get(map, x, y);
}

/* LegoObject_GetWorldZCallback_Lake: the terrain height, plus, on lake
 * blocks (terrain 9), 8 x the centre value Map3D's block lookup gives. */
// FUNCTION: LEGORR 0x00442560
float legoobject_world_z_callback_lake(float x, float y, struct TerrainMap* map)
{
    float centre;
    int blockPos[2];

    centre = 0.0f;
    if (terrain_map_world_to_block_centre(map, x, y, &blockPos[0], &blockPos[1], &centre)) {
        if (LegoObjG_currLevel->blocks[LegoObjG_currLevel->width * blockPos[1] + blockPos[0]].terrain == 9)
            centre *= 8.0f;
        else
            centre = 0.0f;
    }
    return terrain_map_world_z_get(map, x, y) + centre;
}

/* ---- functions, group M (004425f0-00443840; part 2, issue #588) ---- */

/* Move the object's model to (x, y) on the terrain, facing the way it
 * moved (flags1 0x2 turns it round): the face direction becomes the
 * normalised horizontal move from the old position. */
// FUNCTION: LEGORR 0x004425f0
void legoobject_subobject_pose_delta_normalize_and_dispatch(Lego_Object* obj, float x, float y)
{
    void* cont;
    float len;
    float wPos[3];

    if (obj->type == 1)
        cont = (void*)scene_object_resource_get(obj->vehicle);
    else if (obj->type == 2)
        cont = animation_object_handle_get(obj->miniFigure);
    else if (obj->type == 3)
        cont = animation_object_handle_get(obj->rockMonster);
    ddiscene_object_position_get(cont, NULL, wPos);
    if (x != wPos[0] || y != wPos[1]) {
        obj->faceDirection[2] = 0.0f;
        obj->faceDirection[0] = x;
        obj->faceDirection[1] = y;
        *(LegoObjL_Vector3F*)&obj->field_2a0 = *(LegoObjL_Vector3F*)wPos;
        legoobject_m_vector3d_subtract((LegoObjL_Vector3F*)obj->faceDirection, (LegoObjL_Vector3F*)obj->faceDirection, (LegoObjL_Vector3F*)&obj->field_2a0);
        obj->faceDirection[2] = 0.0f;
        len = legoobject_m_vector3d_modulus((LegoObjL_Vector3F*)obj->faceDirection);
        obj->faceDirectionLength = len;
        legoobject_vector3d_scale((LegoObjL_Vector3F*)obj->faceDirection, (LegoObjL_Vector3F*)obj->faceDirection, 1.0f / len);
    }

    if (obj->type == 1) {
        if (obj->flags1 & 0x2)
            scene_resource_draw_at_origin((struct SceneResourceAnimationRecord*)obj->vehicle,
                -obj->faceDirection[0], -obj->faceDirection[1], -obj->faceDirection[2]);
        else
            scene_resource_draw_at_origin((struct SceneResourceAnimationRecord*)obj->vehicle,
                obj->faceDirection[0], obj->faceDirection[1], obj->faceDirection[2]);
        vehicle_set_position((struct VehicleModel*)obj->vehicle, x, y, legoobject_world_z_callback, Lego_GetMap());
    } else if (obj->type == 2) {
        if (obj->flags1 & 0x2)
            animation_object_transform_set((struct AnimationObject*)obj->miniFigure,
                -obj->faceDirection[0], -obj->faceDirection[1]);
        else
            animation_object_transform_set((struct AnimationObject*)obj->miniFigure,
                obj->faceDirection[0], obj->faceDirection[1]);
        render_node_position_from_callback((struct AnimationObject*)obj->miniFigure, x, y,
                                           (LegoObjM_HeightFn)legoobject_world_z_callback_lake, Lego_GetMap());
    } else if (obj->type == 3) {
        animation_object_transform_set((struct AnimationObject*)obj->rockMonster,
            obj->faceDirection[0], obj->faceDirection[1]);
        render_node_position_from_callback((struct AnimationObject*)obj->rockMonster, x, y,
                                           (LegoObjM_HeightFn)legoobject_world_z_callback, Lego_GetMap());
    }
}

/* LegoObject_SetPositionAndHeading: place the object's model at (x, y)
 * on the terrain and, with assignHeading, face it along the heading theta
 * (+y rotated about -z). */
// FUNCTION: LEGORR 0x004428a0
void legoobject_world_position_set(Lego_Object* obj, float x, float y, float theta, int assignHeading)
{
    LegoObjL_Vector3F pos;
    float dir[3];
    float axis[3];

    if (assignHeading) {
        dir[2] = 0.0f;
        dir[0] = 0.0f;
        dir[1] = 1.0f;
        axis[1] = 0.0f;
        axis[0] = 0.0f;
        axis[2] = -1.0f;
        d3drm_vector_rotate(dir, dir, axis, theta);
    }

    if (obj->type == 1) {
        if (assignHeading)
            scene_resource_draw_at_origin((struct SceneResourceAnimationRecord*)obj->vehicle, dir[0], dir[1], dir[2]);
        vehicle_set_position((struct VehicleModel*)obj->vehicle, x, y, legoobject_world_z_callback, Lego_GetMap());
    } else if (obj->type == 2) {
        if (assignHeading)
            animation_object_transform_set((struct AnimationObject*)obj->miniFigure, dir[0], dir[1]);
        render_node_position_from_callback((struct AnimationObject*)obj->miniFigure, x, y,
                                           (LegoObjM_HeightFn)legoobject_world_z_callback_lake, Lego_GetMap());
    } else if (obj->type == 3) {
        if (assignHeading)
            animation_object_transform_set((struct AnimationObject*)obj->rockMonster, dir[0], dir[1]);
        render_node_position_from_callback((struct AnimationObject*)obj->rockMonster, x, y,
                                           (LegoObjM_HeightFn)legoobject_world_z_callback, Lego_GetMap());
    } else if (obj->type == 4) {
        if (assignHeading)
            scene_object_pair_set_render_parameters(obj->building, dir[0], dir[1]);
        scene_object_pair_set_callback_scalar(obj->building, x, y,
                                              (LegoObjM_ProbeFn)legoobject_world_z_callback, Lego_GetMap());
    } else if (obj->flags3 & 0x20000) {
        if (assignHeading)
            ddiscene_object_pose_method_dispatch(obj->other, NULL, dir[0], dir[1], dir[2], 0.0f, 0.0f, -1.0f);
        /* The z goes through a double, as the original's does: its
         * result is pushed after the call's cleanup, not stored into the
         * call's argument slot. */
        ddiscene_object_position_set(obj->other, NULL, x, y, (float)(terrain_map_world_z_get(Lego_GetMap(), x, y) + 0.0));
    }

    if (assignHeading)
        *(LegoObjL_Vector3F*)obj->faceDirection = *(LegoObjL_Vector3F*)dir;
    obj->faceDirectionLength = 0.01f;
    legoobject_vector3d_scale((LegoObjL_Vector3F*)&obj->field_2a0, (LegoObjL_Vector3F*)obj->faceDirection, -0.01f);
    pos.x = x;
    pos.y = y;
    pos.z = 0.0f;
    legoobject_vector3d_add((LegoObjL_Vector3F*)&obj->field_2a0, (LegoObjL_Vector3F*)&obj->field_2a0, &pos);
}

/* The first-person move: turn by field_2c4 and step by the field_2bc /
 * field_2c0 input at the move speed, then undo the step if it enters a
 * blocked block or runs into an object; otherwise follow the terrain.
 * Returns 0 (no move), 1 (moved), 3 (on a walkable-flagged block it may
 * enter) or 4 (reversing, which it may not); *outTransSpeed is the speed
 * of a move. The inputs are cleared. */
// FUNCTION: LEGORR 0x00442b10
int legoobject_movement_accumulator_apply(Lego_Object* obj, float elapsed, float* outTransSpeed)
{
    void* cont;
    int result;
    int collided;
    Lego_Object* hit;
    int bx;
    int by;
    float moveSpeed;
    int reversing;
    float transSpeed;
    float sign;
    float inv;
    float hitInv;
    float wPos[2];
    float oldPos[2];
    float dir[3];
    float pos[3];
    float up[3];

    cont = legoobject_type_component_get(obj);
    result = 0;
    reversing = obj->field_2c0 < 0;
    legoobject_movement_tick_scale_and_delta_get(obj, elapsed, &moveSpeed, &transSpeed);
    ddiscene_object_facing_pair_get(cont, NULL, dir, up);
    ddiscene_object_position_get(cont, NULL, pos);
    oldPos[0] = pos[0];
    oldPos[1] = pos[1];

    if (legoobject_zero != obj->field_2c4) {
        sign = 1.0f;
        if (obj->type == 1)
            sign = reversing ? -1.0f : 1.0f;
        render_object_rotate_axis(cont, 1, 0.0f, 1.0f, 0.0f, sign * elapsed * obj->field_2c4);
        result = 1;
        *(LegoObjL_Vector3F*)obj->faceDirection = *(LegoObjL_Vector3F*)dir;
    }

    if (obj->field_2bc != 0 || obj->field_2c0 != 0) {
        float trans[3];
        trans[1] = 0.0f;
        inv = 1.0f / (float)sqrt((float)obj->field_2bc * (float)obj->field_2bc +
                                 (float)obj->field_2c0 * (float)obj->field_2c0);
        trans[2] = inv * (float)obj->field_2c0 * moveSpeed;
        trans[0] = inv * (float)obj->field_2bc * moveSpeed;
        scene_object_camera_teleport_or_offset(cont, 1, trans[0], trans[1], trans[2]);
        result = 1;
    }

    if (result) {
        collided = 0;
        legoobject_position_to_terrain_cell(obj, &bx, &by);
        legoobject_transform_position_get(obj, &wPos[0], &wPos[1]);
        if ((hit = legoobject_flag_gated_callback_dispatch(
                 obj, wPos, 0, 1)) != NULL) {
            LegoObjM_Point2F hitDir;
            legoobject_transform_position_get(hit, &hitDir.x, &hitDir.y);
            legoobject_vector2d_subtract(&hitDir, (LegoObjM_Point2F*)wPos, &hitDir);
            legoobject_vector2d_subtract((LegoObjM_Point2F*)oldPos, (LegoObjM_Point2F*)wPos, (LegoObjM_Point2F*)oldPos);
            if (legoobject_vector2d_dot(legoobject_vector2d_normalise(&hitDir, &hitDir), legoobject_vector2d_normalise((LegoObjM_Point2F*)oldPos, (LegoObjM_Point2F*)oldPos)) <= legoobject_zero)
                collided = 1;
        }

        if (!lego_runtime_flag40000000_get() &&
            (!terrain_cell_route_and_occupancy_check_current(LEGOOBJ_SLOT(obj), bx, by, bx, by, 1) || collided ||
             terrain_cell_type_is_8_or_10_or_11_current(bx, by))) {
            ddiscene_object_position_set(cont, NULL, pos[0], pos[1], pos[2]);
            ddiscene_object_pose_method_dispatch(cont, NULL, dir[0], dir[1], dir[2], up[0], up[1], up[2]);
            *(LegoObjL_Vector3F*)obj->faceDirection = *(LegoObjL_Vector3F*)dir;
        } else {
            if (terrain_cell_walkable_flag_current(bx, by))
                terrain_map_world_edge_point_get(Lego_GetMap(), wPos[0], wPos[1], terrain_cell_flag_2000_current(bx, by),
                                                 0.0f, &wPos[0], &wPos[1]);
            if (obj->type == 1)
                vehicle_set_position((struct VehicleModel*)obj->vehicle, wPos[0], wPos[1], legoobject_world_z_callback, Lego_GetMap());
            else if (obj->type == 2)
                render_node_position_from_callback((struct AnimationObject*)obj->miniFigure, wPos[0], wPos[1],
                                                   (LegoObjM_HeightFn)legoobject_world_z_callback_lake, Lego_GetMap());
            else if (obj->type == 3)
                render_node_position_from_callback((struct AnimationObject*)obj->rockMonster, wPos[0], wPos[1],
                                                   (LegoObjM_HeightFn)legoobject_world_z_callback, Lego_GetMap());
        }

        if (terrain_cell_walkable_flag_current(bx, by) &&
            terrain_cell_walkability_query_current(LEGOOBJ_SLOT(obj), bx, by, 0, 1) &&
            (obj->type != 2 || legoobject_activity_scan(obj, 0)))
            result = 3;
        if (reversing && !(entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x2000000))
            result = 4;
    }

    obj->field_2bc = 0;
    obj->field_2c0 = 0;
    obj->field_2c4 = 0.0f;
    if (result)
        *outTransSpeed = transSpeed;
    return result;
}

/* Keep the object's model on the terrain: vehicles, buildings and rock
 * monsters at their position; a minifigure eased (at elapsed / 2) towards
 * the edge point of a walkable-flagged block; and, for the objects types
 * 6-13 that nothing carries, sink into a 0x800-flagged block (under the
 * effect container 0040c5b0 gives it) or rise back out, and level out. */
// FUNCTION: LEGORR 0x00442f80
void legoobject_pose_orientation_dispatch(Lego_Object* obj, float elapsed)
{
    aitask_entity_pending_task_flag_clear(LEGOOBJ_SLOT(obj));
    if (obj->type == 1) {
        float x;
        float y;
        legoobject_transform_position_get(obj, &x, &y);
        vehicle_set_position((struct VehicleModel*)obj->vehicle, x, y, legoobject_world_z_callback, Lego_GetMap());
    } else if (obj->type == 4) {
        float x;
        float y;
        legoobject_transform_position_get(obj, &x, &y);
        scene_object_pair_set_callback_scalar(obj->building, x, y,
                                              (LegoObjM_ProbeFn)legoobject_world_z_callback, Lego_GetMap());
    } else if (obj->type == 3) {
        float x;
        float y;
        legoobject_transform_position_get(obj, &x, &y);
        render_node_position_from_callback((struct AnimationObject*)obj->rockMonster, x, y,
                                           (LegoObjM_HeightFn)legoobject_world_z_callback, Lego_GetMap());
    } else if (obj->type == 2) {
        int bx;
        int by;
        float dx;
        float dy;
        float half;
        float len;
        float z;
        LegoObjM_Point2F newPos;
        LegoObjM_Point2F edge;
        float pos[3];
        ddiscene_object_position_get(legoobject_type_component_get(obj), NULL, pos);
        newPos.x = pos[0];
        newPos.y = pos[1];
        legoobject_position_to_terrain_cell(obj, &bx, &by);
        if (terrain_cell_walkable_flag_current(bx, by) && !terrain_cell_type_is_8_or_10_or_11_current(bx, by)) {
            terrain_map_world_edge_point_get(Lego_GetMap(), newPos.x, newPos.y, terrain_cell_flag_2000_current(bx, by), 0.0f,
                                             &edge.x, &edge.y);
            dx = edge.x - newPos.x;
            dy = edge.y - newPos.y;
            half = elapsed * 0.5f;
            len = (float)sqrt(dx * dx + dy * dy);
            if (len > half) {
                half = half / len;
                newPos.x = half * dx + newPos.x;
                newPos.y = half * dy + newPos.y;
            }
        }
        z = legoobject_world_z_callback(newPos.x, newPos.y, Lego_GetMap());
        if (pos[0] != newPos.x || pos[1] != newPos.y || pos[2] != z)
            render_node_position_from_callback((struct AnimationObject*)obj->miniFigure, newPos.x, newPos.y,
                                               (LegoObjM_HeightFn)legoobject_world_z_callback_lake, Lego_GetMap());
    } else if ((obj->type == 6 || obj->type == 7 || obj->type == 9 || obj->type == 8 || obj->type == 11 ||
                obj->type == 13) && obj->carryingThisObject == NULL) {
        unsigned int bx;
        unsigned int by;
        float wz;
        float dir[3];
        float up[3];
        legoobject_position_to_terrain_cell(obj, (int*)&bx, (int*)&by);
        if (obj->type != 9 && terrain_cell_flag_800_bounded_current(bx, by) && obj->field_2dc == NULL) {
            obj->field_2dc = ((void* (*)(unsigned int, unsigned int))scene_resource_slot_register_name)(bx, by);
            obj->targetBlockPos[0] = (float)bx;
            obj->targetBlockPos[1] = (float)by;
            obj->flags3 &= ~0x100000;
        }
        if (obj->field_2dc != NULL &&
            terrain_cell_flag_800_bounded_current((int)obj->targetBlockPos[0], (int)obj->targetBlockPos[1])) {
            ddiscene_object_position_get(obj->field_2dc, NULL, dir);
            wz = terrain_map_world_z_get(Lego_GetMap(), dir[0], dir[1]) - 1.2285f;
            if (dir[2] > wz)
                dir[2] = wz;
            ddiscene_object_position_set(obj->other, NULL, dir[0], dir[1], dir[2]);
            ddiscene_object_pose_method_dispatch(obj->other, obj->field_2dc, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
            return;
        }
        obj->field_2dc = NULL;
        {
            float pos[3];
            ddiscene_object_position_get(obj->other, NULL, pos);
            wz = terrain_map_world_z_get(Lego_GetMap(), pos[0], pos[1]) - 1.2285f;
            if (pos[2] < wz - 0.5f) {
                pos[2] += elapsed;
                if (pos[2] > wz)
                    pos[2] = wz;
                ddiscene_object_position_set(obj->other, NULL, pos[0], pos[1], pos[2]);
            }
            if (pos[2] > wz - -0.5f) {
                pos[2] -= elapsed;
                if (pos[2] < wz)
                    pos[2] = wz;
                ddiscene_object_position_set(obj->other, NULL, pos[0], pos[1], pos[2]);
            }
        }
        ddiscene_object_facing_pair_get(obj->other, NULL, dir, up);
        if (dir[2] < legoobject_zero) {
            dir[2] -= elapsed * -0.05f;
            if (dir[2] > legoobject_zero)
                dir[2] = 0.0f;
            ddiscene_object_pose_method_dispatch(obj->other, NULL, dir[0], dir[1], dir[2], 0.0f, 0.0f, -1.0f);
        } else if (dir[2] > legoobject_zero) {
            dir[2] -= elapsed * 0.05f;
            if (dir[2] < legoobject_zero)
                dir[2] = 0.0f;
            ddiscene_object_pose_method_dispatch(obj->other, NULL, dir[0], dir[1], dir[2], 0.0f, 0.0f, -1.0f);
        }
    }
}

/* Move the driver (driveObject) with the object it drives: copy the
 * position and orientation of the vehicle's 0xc0 null (or the object's own
 * container) to the driver's container where they differ. */
// FUNCTION: LEGORR 0x00443510
void legoobject_scene_object_pose_position_sync(Lego_Object* obj)
{
    void* driveCont;
    void* cont;
    float srcPos[3];
    float srcUp[3];
    float srcDir[3];
    float dstPos[3];
    float dstDir[3];
    float dstUp[3];

    driveCont = legoobject_type_component_get(obj->driveObject);
    if (obj->type != 1 || (cont = scene_resource_cached_slot_c0_get((struct SceneResourceAnimationRecord*)obj->vehicle)) == NULL)
        cont = legoobject_type_component_get(obj);
    ddiscene_object_position_get(driveCont, NULL, dstPos);
    ddiscene_object_position_get(cont, NULL, srcPos);
    if (dstPos[0] != srcPos[0] || dstPos[1] != srcPos[1] || dstPos[2] != srcPos[2])
        ddiscene_object_position_set(driveCont, NULL, srcPos[0], srcPos[1], srcPos[2]);
    ddiscene_object_facing_pair_get(driveCont, NULL, dstDir, dstUp);
    ddiscene_object_facing_pair_get(cont, NULL, srcDir, srcUp);
    if (dstDir[0] != srcDir[0] || dstDir[1] != srcDir[1] || dstDir[2] != srcDir[2] ||
        dstUp[0] != srcUp[0] || dstUp[1] != srcUp[1] || dstUp[2] != srcUp[2])
        ddiscene_object_pose_method_dispatch(driveCont, NULL, srcDir[0], srcDir[1], srcDir[2],
                                             srcUp[0], srcUp[1], srcUp[2]);
}

/* Try up to five random activities in the 0x38-0x4c band (the idle
 * animations); the first that starts sets flags1 0x8000. */
// FUNCTION: LEGORR 0x00443670
int legoobject_random_activity_attempt(Lego_Object* obj)
{
    unsigned int i;

    if (!(obj->flags1 & 0x400) && !legoobject_flags_mask107c33de_or_1c00203_any_test(obj)) {
        for (i = 0; i < 5; i++) {
            legoobject_set_activity(obj, rng_thunk_draw15() % 21 + 0x38, 1);
            if (legoobject_activity_register_bookkeeping(obj)) {
                obj->flags1 |= 0x8000;
                return 1;
            }
        }
    }
    return 0;
}

/* A rock monster with flags3 0x4000000. */
// FUNCTION: LEGORR 0x004436f0
int legoobject_type3_flag04000000_test(Lego_Object* obj)
{
    if (obj->type == 3 && (obj->flags3 & 0x4000000))
        return 1;
    return 0;
}

/* The nearest block a route from blockPos reaches that
 * legoobject_route_cell_candidate_accept accepts, in outPos. */
// FUNCTION: LEGORR 0x00443710
int legoobject_adjacent_position_candidate_find(Lego_Object* obj, int* blockPos, int* outPos)
{
    int useRoute;
    unsigned int* bxList;
    unsigned int* byList;
    unsigned int count;

    useRoute = 0;
    if (!legoobject_route_cell_candidate_accept(obj, blockPos, &useRoute)) {
        if (legoobject_terrain_neighbor_path_check(obj, blockPos[0], blockPos[1], 0, 0, &bxList, &byList, &count,
                                                   legoobject_route_cell_candidate_accept, &useRoute) ||
            legoobject_terrain_neighbor_path_check(obj, blockPos[0], blockPos[1], 0, 0, &bxList, &byList, &count,
                                                   legoobject_route_cell_candidate_accept, &useRoute)) {
            outPos[0] = bxList[count - 1];
            outPos[1] = byList[count - 1];
            free(bxList);
            free(byList);
            return 1;
        }
    }
    return 0;
}

/* A rock monster with flags3 0x80000000 and not yet flags2 0x20000000:
 * set it and start activity 0x31. */
// FUNCTION: LEGORR 0x004437f0
void legoobject_dispatch_event31_for_pending_flag(Lego_Object* obj)
{
    if (obj->type == 3 && (obj->flags3 & 0x80000000) && !(obj->flags2 & 0x20000000)) {
        obj->flags2 |= 0x20000000;
        legoobject_set_activity(obj, 0x31, 0);
        legoobject_activity_register_bookkeeping(obj);
    }
}

/* The route callback of legoobject_adjacent_position_candidate_find: a
 * block the object can enter, that does not hurt it, and is not
 * 0x180000-flagged; unless *data, not a walkable-flagged block. */
// FUNCTION: LEGORR 0x00443840
int legoobject_route_cell_candidate_accept(Lego_Object* obj, int* blockPos, void* data)
{
    if (*(int*)data == 0 && terrain_cell_walkable_flag_current(blockPos[0], blockPos[1]))
        return 0;
    if (terrain_cell_route_and_occupancy_check_current(LEGOOBJ_SLOT(obj), blockPos[0], blockPos[1],
                                                       blockPos[0], blockPos[1], 1) &&
        !entity_cell_type_speed_multiplier_resolve_current(LEGOOBJ_SLOT(obj), blockPos[0], blockPos[1], 0.0f, NULL) &&
        !terrain_cell_mask_180000_current(blockPos))
        return 1;
    return 0;
}

/* ---- functions, group N (004438b0-00444620; part 2, issue #588) ---- */

/* LegoObject_Route_UpdateMovement: move the object along its routing
 * curve by the elapsed time. At the end of the route run the last block's
 * action (the cases are in the original's code order); stop early near an
 * action's target. LegoObject_Callback_Update (0043c810) calls it. */
// FUNCTION: LEGORR 0x004438b0
void legoobject_path_step_advance_and_arrival_check(Lego_Object* obj, float elapsed)
{
    int skip;
    int moved;
    LegoObjG_RouteBlock* routeBlocks;
    LegoObjG_RouteBlock* block;
    unsigned int index;
    int bx;
    int by;
    float transSpeed;
    float workY;
    float workX;
    float elapsedCopy;
    float dist;
    unsigned char action;
    Lego_Object* hit;
    Point2F pos;

    skip = 0;
    routeBlocks = (LegoObjG_RouteBlock*)obj->routeBlocks;
    elapsedCopy = elapsed;
    moved = 1;
    if (routeBlocks != NULL) {
        if (obj->type == 1) obj->field_3d4 = 0.0f;
        if (obj->field_290 >= obj->field_28c) {
            if (obj->flags3 & 0x400) {
                block = &routeBlocks[obj->routeBlocksCurrent];
                obj->flags3 &= ~0x400;
                switch (block->actionFlags) {
                case 2: obj->flags1 |= 0x40; break;
                case 3: obj->flags1 |= 0x200; break;
                case 4: obj->flags1 |= 0x40000; break;
                case 5: obj->flags1 |= 0x800; break;
                case 6: obj->flags1 |= 0x1000000; break;
                case 9: obj->flags1 |= 0x80000; break;
                case 7: obj->flags1 |= 0x2000; break;
                case 10: obj->flags1 |= 0x20000; break;
                case 8: legoobject_attached_object_list_detach(obj, 1); break;
                case 11: obj->flags1 |= 0x40000000; break;
                case 13: obj->flags2 |= 0x40; break;
                case 12: obj->flags2 |= 0x4; break;
                case 14: obj->flags2 |= 0x400; break;
                case 15: obj->flags2 |= 0x8000; break;
                case 16: obj->flags2 |= 0x200000; break;
                case 18: obj->flags2 |= 0x10000000; break;
                case 17: obj->flags2 |= 0x4000000; break;
                case 19: legoobject_linked_flag_and_command_update(obj); break;
                case 21: obj->flags2 |= 0x80000000; break;
                case 20: entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 0x1d, NULL, 1); break;
                }
                if (block->flags & 0x20) obj->flags2 |= 0x80000;
                legoobject_action_sequence_reset(obj, 1);
                if (obj->flags1 & 0x8000000) {
                    if ((rng_thunk_draw15() % (unsigned int)(10.0f / elapsed)) == 0)
                        obj->flags1 &= ~0x8000000;
                    else
                        entity_neighbor_blocked_cell_step_probe(LEGOOBJ_SLOT(obj), 150.0f, 1);
                }
                return;
            }
            if (obj->routeBlocksTotal > 1) {
                obj->routeBlocksCurrent++;
                if ((routeBlocks[obj->routeBlocksCurrent].flags & 0x10) &&
                    routeBlocks[obj->routeBlocksCurrent].actionFlags != 1)
                    skip = 1;
            } else {
                obj->flags3 |= 0x400;
                routeBlocks[obj->routeBlocksCurrent].flags |= 0x8;
            }
            index = obj->routeBlocksCurrent;
            if (skip) index++;
            if (routeBlocks[index].actionFlags == 5)
                legoobject_path_step_lookahead_write(obj);
            moved = legoobject_step_movement_vector_resolve(obj, skip, obj->flags3 & 0x8,
                                                            !(obj->flags1 & 0x400));
            if (obj->routeBlocksCurrent >= obj->routeBlocksTotal - 1) obj->flags3 |= 0x400;
        }
        if (moved) {
            if (!(obj->flags1 & 0x8) &&
                routeBlocks[obj->routeBlocksCurrent].actionFlags == 1 &&
                legoobject_work_object_position_get(obj, &workX, &workY) &&
                ((int (*)(float, float, int*, int*))current_level_terrain_grid_pick)(workX, workY, &bx, &by) &&
                terrain_cell_walkable_flag_current(bx, by) &&
                bx == routeBlocks[obj->routeBlocksCurrent].blockPos[0] &&
                by == routeBlocks[obj->routeBlocksCurrent].blockPos[1]) {
                if (terrain_cell_walkability_query_current(LEGOOBJ_SLOT(obj),
                        (int)obj->targetBlockPos[0], (int)obj->targetBlockPos[1],
                        obj->flags3 & 0x2000000, 1))
                    obj->flags1 |= 0x18;
                else
                    obj->flags1 |= 0x80000000;
            }

            legoobject_movement_tick_scale_and_delta_get(obj, elapsed, &transSpeed, &elapsedCopy);
            if (!(obj->flags3 & 0x4000)) obj->field_290 += transSpeed;
            if (obj->field_290 > obj->field_28c) {
                obj->field_294 = obj->field_290 - obj->field_28c;
                obj->field_290 = obj->field_28c;
            }
            path_arc_length_interpolate((struct BezierCurve*)&obj->routeCurveCount, obj->field_290, &pos);

            action = routeBlocks[obj->routeBlocksCurrent].actionFlags;
            if (action == 3) {
                if (obj->routeToObject != NULL) {
                    Point2F target;
                    Point2F dir;
                    legoobject_transform_position_get(obj->routeToObject, &target.x, &target.y);
                    legoobject_n_vector2d_subtract(&dir.x, &target.x, &pos.x);
                    dist = legoobject_n_vector2d_modulus(&dir.x);
                    if (dist <= 6.6f) {
                        obj->flags1 |= 0x200;
                        legoobject_action_sequence_reset(obj, 1);
                        if (obj->type == 2 && legoobject_zero != dist) {
                            animation_object_transform_set((struct AnimationObject*)obj->miniFigure, dir.x, dir.y);
                            obj->faceDirection[0] = dir.x;
                            obj->faceDirection[1] = dir.y;
                        }
                        return;
                    }
                }
            } else if (action == 12) {
                if (obj->routeToObject != NULL) {
                    Point2F target;
                    legoobject_transform_position_get(obj->routeToObject, &target.x, &target.y);
                    dist = (float)sqrt((pos.x - target.x) * (pos.x - target.x) +
                                       (pos.y - target.y) * (pos.y - target.y));
                    if (dist <= entity_stats_float_at6c(LEGOOBJ_SLOT(obj->routeToObject))) {
                        obj->flags2 |= 0x4;
                        legoobject_action_sequence_reset(obj, 1);
                        return;
                    }
                }
            } else if (action == 9 && !(routeBlocks[obj->routeBlocksCurrent].flags & 0xc)) {
                Point2F target;
                target = *(Point2F*)obj->routeCurvePoints[obj->routeCurveCount - 1];
                if ((float)sqrt((pos.x - target.x) * (pos.x - target.x) +
                                (pos.y - target.y) * (pos.y - target.y)) <= 6.0f) {
                    obj->flags1 |= 0x80000;
                    legoobject_action_sequence_reset(obj, 1);
                    return;
                }
            }

            if (!(obj->flags3 & 0x800)) {
                action = routeBlocks[obj->routeBlocksCurrent].actionFlags;
                if (action != 7 && action != 6 && action != 3 && action != 5) {
                    hit = legoobject_flag_gated_callback_dispatch(obj, &pos.x, 0, 0);
                    if (hit != NULL) {
                        if ((entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x40000000) && hit->type == 1) {
                            legoobject_action_sequence_reset(obj, 0);
                            legoobject_set_activity(obj, 0x20, 0);
                            legoobject_activity_register_bookkeeping(obj);
                            obj->routeToObject = hit;
                            obj->flags2 |= 0x40000;
                            return;
                        }
                        legoobject_step_movement_vector_resolve(obj, 0, 0, 0);
                        obj->flags3 |= 0x800;
                        obj->field_290 += transSpeed;
                    }
                }
            }
            if (!(obj->flags3 & 0x4000))
                legoobject_subobject_pose_delta_normalize_and_dispatch(obj, pos.x, pos.y);
            if (routeBlocks[obj->routeBlocksTotal - 1].actionFlags == 4 && obj->type == 1) {
                LegoObjJ_Point2I blockPos;
                int endX;
                int endY;
                terrain_map_world_to_block(Lego_GetMap(), pos.x, pos.y, &blockPos.x, &blockPos.y);
                endX = routeBlocks[obj->routeBlocksTotal - 1].blockPos[0];
                endY = routeBlocks[obj->routeBlocksTotal - 1].blockPos[1];
                if (blockPos.x == endX && blockPos.y == endY) {
                    obj->flags1 |= 0x40000;
                    legoobject_action_sequence_reset(obj, 1);
                }
            }
        }
    } else {
        legoobject_action_sequence_reset(obj, 0);
    }
}

/* Route action 19 of 004438b0: when the object routes to a building, tag
 * both (flags4 0x40) and drop the object's 0x1c command. */
// FUNCTION: LEGORR 0x004440a0
void legoobject_linked_flag_and_command_update(Lego_Object* obj)
{
    Lego_Object* routeTo;

    routeTo = obj->routeToObject;
    if (routeTo != NULL && routeTo->type == 4) {
        obj->flags4 |= 0x40;
        routeTo->flags4 |= 0x40;
        entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 0x1c, NULL, 1);
    }
}

/* While the object carries (flags1 0x400; a rock monster only a boulder):
 * at or below its stats' health threshold (+0x90) it drops its load
 * (00439e70). Above it, its carry timer (+0x33c) grows by the elapsed
 * time, and once the timer outweighs its health (stats rate +0x8c) it
 * stops and rests (activity 0x22), unless it stands on a type-9 block or
 * on its route's last block. */
// FUNCTION: LEGORR 0x004440f0
void legoobject_activity_timeout_accumulate_and_raise(Lego_Object* obj, float elapsed)
{
    LegoObjJ_Point2I blockPos;
    float rate;

    if ((obj->flags1 & 0x400) && (obj->type != 3 || obj->carriedObjects[0]->type == 5)) {
        if (entity_stats_float_at90(LEGOOBJ_SLOT(obj)) < obj->health && !(obj->flags4 & 0x20)) {
            rate = entity_stats_float_at8c(LEGOOBJ_SLOT(obj)) * 0.01f;
            if (legoobject_zero != rate && !(obj->flags1 & 0x4) &&
                obj->field_33c * 0.04f > rate * obj->health) {
                legoobject_position_to_terrain_cell(obj, &blockPos.x, &blockPos.y);
                if (!terrain_cell_state_equals_six_current(&blockPos.x) &&
                    LegoObjG_currLevel->blocks[LegoObjG_currLevel->width * blockPos.y + blockPos.x].terrain != 9 &&
                    obj->routeBlocksCurrent != obj->routeBlocksTotal - 1) {
                    obj->field_33c = 0.0f;
                    obj->flags3 |= 0x4000;
                    obj->flags1 |= 0x20000000;
                    legoobject_set_activity(obj, 0x22, 0);
                    legoobject_activity_register_bookkeeping(obj);
                }
            }
            obj->field_33c += elapsed;
            return;
        }
        legoobject_attached_object_list_detach(obj, 0);
    }
}

/* Route the object to a random point of one of the eight blocks around
 * it, in at most sixteen tries; 1 when a route was set. */
// FUNCTION: LEGORR 0x00444260
int legoobject_random_cell_step_route_issue(Lego_Object* obj)
{
    int bx;
    unsigned int* xList;
    int by;
    unsigned int* yList;
    unsigned int i;
    unsigned int count;
    Point2F wPos;
    LegoObjJ_Point2I offsets[8];
    float vertices[4][3];
    int r;

    offsets[0].x = -1; offsets[0].y = 0;
    offsets[1].x = -1; offsets[1].y = 1;
    offsets[2].x = 0;  offsets[2].y = 1;
    offsets[3].x = 1;  offsets[3].y = 1;
    offsets[4].x = 1;  offsets[4].y = 0;
    offsets[5].x = 1;  offsets[5].y = -1;
    offsets[6].x = 0;  offsets[6].y = -1;
    offsets[7].x = -1; offsets[7].y = -1;
    if (legoobject_flags_mask107c33de_or_1c00203_any_test(obj)) return 0;
    legoobject_position_to_terrain_cell(obj, &bx, &by);
    for (i = 0; i < 16; i++) {
        r = rng_thunk_draw15() % 8;
        if (terrain_cell_route_and_occupancy_check_current(LEGOOBJ_SLOT(obj), bx + offsets[r].x, by + offsets[r].y, bx + offsets[r].x, by + offsets[r].y, 0)) {
            terrain_map_block_vertices_get(Lego_GetMap(), bx + offsets[r].x, by + offsets[r].y,
                                           (struct TerrainMapVector3*)vertices);
            wPos.x = random_float_range(vertices[0][0] - -1.0f, vertices[2][0] - 1.0f);
            wPos.y = random_float_range(vertices[0][1] - -1.0f, vertices[2][1] - 1.0f);
            if (legoobject_terrain_neighbor_path_check(obj, bx, by, bx + offsets[r].x, by + offsets[r].y,
                                                       &xList, &yList, &count, NULL, NULL)) {
                if (legoobject_route_setup_and_marker_associate(obj, count, xList, yList, &wPos.x)) {
                    free(xList);
                    free(yList);
                    return 1;
                }
                free(xList);
                free(yList);
            }
        }
    }
    return 0;
}

/* Run away: route the object, if it is free to, to the point
 * legoGlobs.MiniFigureRunAway along dir; its last route block gets flag
 * 0x40. */
// FUNCTION: LEGORR 0x00444460
void legoobject_flagged_vector_step_command_issue(Lego_Object* obj, float* dir)
{
    LegoObjJ_Point2I blockPos;

    legoobject_position_to_terrain_cell(obj, &blockPos.x, &blockPos.y);
    if (!(obj->flags1 & 0x800000) && legoobject_shared_eligibility_check(obj, 1) &&
        !lego_entity_matches_current_selection_or_owner(obj) && obj->driveObject == NULL &&
        !terrain_cell_walkable_flag_current(blockPos.x, blockPos.y)) {
        unsigned int* yList;
        unsigned int count;
        unsigned int* xList;
        Point2F wPos;
        LegoObjJ_Point2I toBlockPos;

        legoobject_state_reset(obj, 1, 1);
        legoobject_transform_position_get(obj, &wPos.x, &wPos.y);
        legoobject_vector2d_ray_end_point(&wPos.x, &wPos.x, dir, legoGlobs_MiniFigureRunAway);
        if (terrain_map_world_to_block(Lego_GetMap(), wPos.x, wPos.y, &toBlockPos.x, &toBlockPos.y)) {
            if (toBlockPos.x == blockPos.x && toBlockPos.y == blockPos.y) {
                /* A one-block route: the original frees these stack
                 * addresses below as well. */
                xList = (unsigned int*)&blockPos.x;
                yList = (unsigned int*)&blockPos.y;
                count = 1;
            } else if (!legoobject_terrain_neighbor_path_check(obj, blockPos.x, blockPos.y,
                           toBlockPos.x, toBlockPos.y, &xList, &yList, &count, NULL, NULL)) {
                return;
            }
            if (legoobject_route_setup_and_marker_associate(obj, count, xList, yList, &wPos.x))
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].flags |= 0x40;
            free(xList);
            free(yList);
            obj->flags1 |= 0x800000;
            idle_register(LEGOOBJ_SLOT(obj));
        }
    }
}

/* Once (flags1 0x4000000), stop the object and start activity 0x21,
 * unless it drives a vehicle. */
// FUNCTION: LEGORR 0x00444620
void legoobject_enter_event21_state_once(Lego_Object* obj)
{
    if (!(obj->flags1 & 0x4000000) && obj->driveObject == NULL) {
        legoobject_state_reset(obj, 0, 1);
        obj->flags1 |= 0x4000000;
        legoobject_set_activity(obj, 0x21, 0);
        legoobject_activity_register_bookkeeping(obj);
        ai_idle_chain_record_attach(LEGOOBJ_SLOT(obj));
    }
}


/* ---- functions, group O (00444680-00445830; part 2, issue #588) ---- */

/* LegoObject_Route_ResolveStep?: set up the Bezier curve from the
 * object's position to the next route block (nextBlock advances the route
 * first). With turn set, an object facing away from the step turns (or,
 * with reverse, backs up) first; returns 0 when the turn takes this
 * frame. */
// FUNCTION: LEGORR 0x00444680
int legoobject_step_movement_vector_resolve(Lego_Object* obj, int nextBlock, int turn, int reverse)
{
    int cby;
    int cbx;
    int diag;
    int adjusted;
    int result;
    int by;
    int bx;
    int sx;
    int sy;
    float dot1;
    float dot2;
    float len;
    unsigned char flags;
    Point2F* offset;
    float jx;
    float jy;
    Point2F move;
    Point2F to;
    Point2F dir;
    Point2F pos;
    Point2F unused;
    LegoObjJ_Vector3F toward;
    LegoObjJ_Vector3F facing;
    Point2F offsets[4];
    LegoObjG_RouteBlock* route;

    diag = 0;
    adjusted = 0;
    result = 1;
    route = (LegoObjG_RouteBlock*)obj->routeBlocks;
    sx = route[obj->routeBlocksCurrent].blockPos[0];
    sy = route[obj->routeBlocksCurrent].blockPos[1];
    legoobject_position_to_terrain_cell(obj, &bx, &by);
    legoobject_transform_position_get(obj, &pos.x, &pos.y);
    dir.x = obj->faceDirection[0];
    dir.y = obj->faceDirection[1];
    if (nextBlock) {
        terrain_map_block_to_world(Lego_GetMap(),
            ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksCurrent].blockPos[0],
            ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksCurrent].blockPos[1],
            &unused.x, &unused.y);
        obj->routeBlocksCurrent++;
        terrain_map_block_to_world(Lego_GetMap(),
            ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksCurrent].blockPos[0],
            ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksCurrent].blockPos[1],
            &to.x, &to.y);
    } else {
        terrain_map_block_to_world(Lego_GetMap(), sx, sy, &to.x, &to.y);
        sx = bx;
        sy = by;
    }
    terrain_map_block_to_world(Lego_GetMap(), sx, sy, &move.x, &move.y);

    if (((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksCurrent].actionFlags != 1) {
        terrain_map_world_to_block(Lego_GetMap(), to.x, to.y, &cbx, &cby);
        diag = terrain_cell_flag_2000_current(cbx, cby);
        if (terrain_cell_walkable_flag_current(cbx, cby) &&
            !terrain_cell_type_is_8_or_10_or_11_current(cbx, cby))
        {
            if (terrain_cell_flag_80_current(cbx, cby)) {
                if (fabs(to.x - move.x) <= fabs(to.y - move.y)) {
                    offsets[0].x = 0.1f;
                    offsets[0].y = 0.0f;
                    offsets[1].x = -0.1f;
                    offsets[1].y = 0.0f;
                    offsets[2].x = -0.1f;
                    offsets[2].y = 0.0f;
                    offsets[3].x = 0.1f;
                    offsets[3].y = 0.0f;
                    offset = &offsets[legoobject_o_block_byte3(cbx, cby)];
                } else {
                    offsets[0].x = 0.0f;
                    offsets[0].y = -0.1f;
                    offsets[1].x = 0.0f;
                    offsets[1].y = -0.1f;
                    offsets[2].x = 0.0f;
                    offsets[2].y = 0.1f;
                    offsets[3].x = 0.0f;
                    offsets[3].y = 0.1f;
                    offset = &offsets[legoobject_o_block_byte3(cbx, cby)];
                }
                legoobject_o_vector2_add(&to, offset, &to);
            }
            terrain_map_world_edge_point_get(Lego_GetMap(), to.x, to.y,
                terrain_cell_flag_2000_current(cbx, cby), 0.05f, &to.x, &to.y);
            adjusted = 1;
        }
    }

    /* jx, jy are read below (flags & 8) even when the block was adjusted
     * and they were never set, as in the original. */
    if (!adjusted) {
        jx = (((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksCurrent].blockOffset[0] - 0.5f) *
             terrain_context_scale_get(Lego_GetMap());
        jy = (((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksCurrent].blockOffset[1] - 0.5f) *
             terrain_context_scale_get(Lego_GetMap());
        to.x = jx + to.x;
        to.y = jy + to.y;
    }

    flags = ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksCurrent].flags;
    if (flags & 4) {
        switch (flags & 3) {
        case 0:
            move.x = 0.0f;
            move.y = 1.0f;
            break;
        case 1:
            move.x = 1.0f;
            move.y = 0.0f;
            break;
        case 2:
            move.x = 0.0f;
            move.y = -1.0f;
            break;
        case 3:
            move.x = -1.0f;
            move.y = 0.0f;
            break;
        }
        turn = 0;
    } else if (flags & 0x40) {
        legoobject_o_vector2_subtract(&move, &to, &pos);
    } else if ((flags & 8) && (legoobject_zero != jx || legoobject_zero != jy)) {
        move.x = jx;
        move.y = jy;
    } else {
        legoobject_o_vector2_subtract(&move, &to, &move);
        if (adjusted && !diag) {
            if (fabs(move.x) <= fabs(move.y)) move.x = 0.0f;
            else move.y = 0.0f;
        } else if (legoobject_zero == move.x && legoobject_zero == move.y) {
            legoobject_o_vector2_subtract(&move, &to, &pos);
        }
    }

    if (turn) {
        toward.x = to.x - pos.x;
        toward.y = to.y - pos.y;
        toward.z = 0.0f;
        facing.x = dir.x;
        facing.y = dir.y;
        facing.z = 0.0f;
        legoobject_o_vector3_normalise(&toward);
        legoobject_o_vector3_normalise(&facing);
        len = 1.0f / legoobject_o_vector2_modulus(&move);
        dot1 = move.x * len * facing.x + move.y * len * facing.y;
        dot2 = legoobject_o_vector3_dot(&toward, &facing);
        if (reverse && dot1 > 0.7f && dot2 < -0.7f) {
            dir.x = -dir.x;
            dir.y = -dir.y;
            move.x = -move.x;
            move.y = -move.y;
            obj->flags1 |= 2;
        } else if ((dot1 <= legoobject_zero && dot2 < 0.7f && fabs(dot2) > 0.1) ||
                   legoobject_move_vector_surface_probe(obj))
        {
            if (dot1 < -0.9f) {
                if (obj->type != 1) {
                    dir.x = obj->faceDirection[0] = -obj->faceDirection[0];
                    dir.y = obj->faceDirection[1] = -obj->faceDirection[1];
                }
                legoobject_set_activity(obj, (obj->flags1 & 0x400) ? 0x15 : 10, 0);
                if (obj->type != 1) obj->flags1 |= 0x80;
            } else {
                legoobject_o_vector3_cross(&toward, &toward, (LegoObjJ_Vector3F*)obj->faceDirection);
                d3drm_vector_rotate(obj->faceDirection, obj->faceDirection, &toward.x, -1.5707964f);
                if (obj->type != 1) {
                    dir.x = obj->faceDirection[0];
                    dir.y = obj->faceDirection[1];
                }
                if (toward.z > legoobject_zero)
                    legoobject_set_activity(obj, (obj->flags1 & 0x400) ? 0x15 : 10, 1);
                else
                    legoobject_set_activity(obj, (obj->flags1 & 0x400) ? 0x14 : 9, 1);
            }
            obj->flags1 |= 4;
            if (obj->type != 1) {
                result = 0;
                obj->flags1 &= ~1;
            }
        }
    }

    len = (float)sqrt((pos.x - to.x) * (pos.x - to.x) + (pos.y - to.y) * (pos.y - to.y)) * 0.2f;
    if (len > 0.1f) {
        if (!_finite(dir.x)) {
            dir.x = 1.0f;
            dir.y = 0.0f;
        }
        vector2_set_length(&dir, len);
        vector2_set_length(&move, len * 1.6f);
        bezier_curve_samples_build((BezierCurve*)&obj->routeCurveCount, &pos, &dir, &to, &move, 50);
        legoobject_path_step_vector_normalize(obj);
    } else {
        obj->routeCurvePoints[0][0] = pos.x;
        obj->routeCurvePoints[0][1] = pos.y;
        obj->routeCurveCount = 0;
    }
    obj->field_28c = polyline_segment_lengths_total((BezierCurve*)&obj->routeCurveCount);
    obj->field_298 = move.x;
    obj->flags3 &= ~0x800;
    obj->field_290 = obj->field_294;
    obj->field_29c = move.y;
    return result;
}

/* LegoObject_FaceTowards?: turn a mini-figure towards dir (x, y): a
 * quarter turn left or right, or about face, with the turning activity. */
// FUNCTION: LEGORR 0x00444fb0
void legoobject_turn_to_vector_activity_issue(Lego_Object* obj, float* dir)
{
    LegoObjJ_Vector3F v;
    float angle;
    int activity;

    if (obj->type == 2) {
        v.x = dir[0];
        v.y = dir[1];
        v.z = 0.0f;
        legoobject_o_vector3_normalise((LegoObjJ_Vector3F*)obj->faceDirection);
        legoobject_o_vector2_normalise((Point2F*)&v);
        angle = (float)acos(legoobject_o_vector3_dot((LegoObjJ_Vector3F*)obj->faceDirection, &v));
        legoobject_action_sequence_reset(obj, 0);
        if (angle > 2.3561945f) {
            obj->faceDirection[0] = -obj->faceDirection[0];
            obj->faceDirection[1] = -obj->faceDirection[1];
            obj->flags1 |= 0x80;
            activity = 10;
        } else if (angle > 0.7853982f) {
            legoobject_o_vector3_cross(&v, &v, (LegoObjJ_Vector3F*)obj->faceDirection);
            d3drm_vector_rotate(obj->faceDirection, obj->faceDirection, &v.x, -1.5707964f);
            if (v.z < legoobject_zero) activity = 9;
            else activity = 10;
        } else {
            return;
        }
        if (obj->flags1 & 0x400) activity = (activity == 10) + 0x14;
        obj->flags1 |= 4;
        legoobject_set_activity(obj, activity, 0);
        legoobject_activity_register_bookkeeping(obj);
    }
}

/* LegoObject_Route_CurveAvoid?: push the route curve's points out of
 * blocked blocks (0.71 blocks from the block's centre) and, for objects
 * with stats flag 0x80, around the other objects in the way
 * (legoobject_path_step_vector_callback). */
// FUNCTION: LEGORR 0x004451e0
void legoobject_path_step_vector_normalize(Lego_Object* obj)
{
    Lego_Object* self;
    unsigned int i;
    int bx;
    int by;
    float len;
    Point2F centre;
    Point2F* point;
    unsigned char action;

    if (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x20000000) {
        action = ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksCurrent].actionFlags;
        if (action != 7 && action != 3 && action != 6 && action != 5) {
            self = obj;
            if (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x80)
                legoobject_run_through_lists_skip_upgrade_parts(legoobject_path_step_vector_callback, &self);
            for (i = 0; i < obj->routeCurveCount; i++) {
                point = (Point2F*)obj->routeCurvePoints[i];
                terrain_map_world_to_block(Lego_GetMap(), point->x, point->y, &bx, &by);
                if (terrain_cell_occupied_or_flag_current(bx, by, 0)) {
                    terrain_map_block_to_world(Lego_GetMap(), bx, by, &centre.x, &centre.y);
                    legoobject_o_vector2_subtract(point, point, &centre);
                    len = legoobject_o_vector2_modulus(point);
                    legoobject_o_vector2_scale(point, point, 1.0f / len);
                    legoobject_o_vector2_scale(point, point, terrain_context_scale_get(Lego_GetMap()) * 0.71f);
                    legoobject_o_vector2_add(point, point, &centre);
                }
            }
        }
    }
}

/* The RunThroughLists callback of 004451e0: data points at the routing
 * object. Bend its curve's inner points away from this object when they
 * pass within both collision radii. */
// FUNCTION: LEGORR 0x00445340
int legoobject_path_step_vector_callback(Lego_Object* obj, void* data)
{
    Lego_Object* routing;
    float radius;
    unsigned int i;
    float dist;
    float dx;
    float dy;
    Point2F pos;
    Point2F away;
    float* points;
    float* last;

    if (obj != *(Lego_Object**)data && obj->carryingThisObject != *(Lego_Object**)data &&
        obj->interactObject != *(Lego_Object**)data && !(obj->flags3 & 0x2000) &&
        !(entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x200))
    {
        routing = *(Lego_Object**)data;
        radius = entity_stats_float_at6c(LEGOOBJ_SLOT(routing));
        legoobject_transform_position_get(obj, &pos.x, &pos.y);
        for (i = 1; i < routing->routeCurveCount - 1; i++) {
            dy = pos.y - routing->routeCurvePoints[i][1];
            dx = pos.x - routing->routeCurvePoints[i][0];
            dist = (float)sqrt(dy * dy + dx * dx);
            if (dist < entity_stats_float_at6c(LEGOOBJ_SLOT(obj)) + radius) {
                legoobject_o_vector2_subtract(&away, (Point2F*)routing->routeCurvePoints[i], &pos);
                if (legoobject_zero != legoobject_o_vector2_modulus(&away)) {
                    legoobject_o_vector2_normalise(&away);
                } else {
                    away.x = random_float_range(0.0f, 1.0f);
                    away.y = random_float_range(0.0f, 1.0f);
                    legoobject_o_vector2_normalise(&away);
                }
                legoobject_o_vector2_scale(&away, &away, entity_stats_float_at6c(LEGOOBJ_SLOT(obj)) + radius);
                legoobject_o_vector2_add(&away, &away, &pos);
                legoobject_o_vector2_add(&away, (Point2F*)routing->routeCurvePoints[i + 1], &away);
                legoobject_o_vector2_add(&away, (Point2F*)routing->routeCurvePoints[i - 1], &away);
                legoobject_o_vector2_scale(&away, &away, 0.33333334f);
                *(Point2F*)routing->routeCurvePoints[i] = away;
            }
        }
        points = routing->routeCurvePoints[routing->routeCurveCount - 2];
        last = routing->routeCurvePoints[routing->routeCurveCount - 1];
        last[0] = points[0] + (points[0] - points[-2]);
        last[1] = points[1] + (points[1] - points[-1]);
    }
    return 0;
}

/* Whether the object's block is not walkable or a third of a block ahead
 * along its facing is blocked. */
// FUNCTION: LEGORR 0x004455a0
int legoobject_move_vector_surface_probe(Lego_Object* obj)
{
    int by2;
    int bx2;
    int by;
    int bx;
    Point2F dir;
    Point2F pos;

    legoobject_position_to_terrain_cell(obj, &bx, &by);
    if (!terrain_cell_walkable_flag_current(bx, by)) {
        legoobject_transform_position_get(obj, &pos.x, &pos.y);
        legoobject_o_vector2_copy(&dir, (Point2F*)obj->faceDirection);
        legoobject_o_vector2_normalise(&dir);
        legoobject_o_vector2_scale(&dir, &dir, terrain_context_scale_get(Lego_GetMap()) * 0.33333334f);
        legoobject_o_vector2_add(&pos, &dir, &pos);
        terrain_map_world_to_block(Lego_GetMap(), pos.x, pos.y, &bx2, &by2);
        if (terrain_cell_walkable_flag_current(bx2, by2) || terrain_cell_occupied_or_flag_current(bx2, by2, 0))
            return 1;
    }
    return 0;
}

/* LegoObject_UpdateBehaviour?: a rock monster checks the objects around it
 * (00445830); lit dynamite and the scary sound wake the mini-figures near
 * it once their activity is 75 frames from its end (00445770). */
// FUNCTION: LEGORR 0x004456e0
void legoobject_behavior_callback_tick(Lego_Object* obj, float elapsed)
{
    float frames;

    if (obj->type == 3)
        legoobject_run_through_lists_skip_upgrade_parts(legoobject_behavior_callback_b, obj);
    if ((obj->type == 8 || obj->type == 13) && (obj->flags3 & 0x10000)) {
        frames = (float)ddiscene_object_frame_position_get(obj->other);
        if (ddiscene_object_length_get(obj->other) > frames - 75.0f) {
            legoobject_run_through_lists_skip_upgrade_parts(legoobject_behavior_callback_a, obj);
        }
    }
}

/* The RunThroughLists callback of 004456e0 for dynamite (data): a
 * mini-figure that can flee and is within the radius runs away. */
// FUNCTION: LEGORR 0x00445770
int legoobject_behavior_callback_a(Lego_Object* obj, void* data)
{
    Point2F pos;
    Point2F opos;
    Point2F dir;
    Point2F d;
    float dist;

    if (obj->type == 2 && (obj->abilityFlags & 8)) {
        legoobject_transform_position_get(obj, &pos.x, &pos.y);
        legoobject_transform_position_get((Lego_Object*)data, &opos.x, &opos.y);
        legoobject_o_vector2_subtract(&d, &pos, &opos);
        dist = legoobject_o_vector2_modulus(&d);
        if (dist < legoGlobs_DynamiteDamageRadius) {
            legoobject_o_vector2_scale(&dir, &d, 1.0f / dist);
            legoobject_flagged_vector_step_command_issue(obj, &dir.x);
            return 0;
        }
    }
    return 0;
}

/* The RunThroughLists callback of 004456e0 for a rock monster (data):
 * its reactions to the object, by the monster's stats flags. */
// FUNCTION: LEGORR 0x00445830
int legoobject_behavior_callback_b(Lego_Object* obj, void* data)
{
    float dist;
    Lego_Object* monster = (Lego_Object*)data;
    float radius;
    float x;
    float y;
    int type;
    int id;
    Point2F d;
    Point2F pos;
    Point2F mpos;
    int blockPos[2];
    float dir[3];

    legoobject_transform_position_get(monster, &mpos.x, &mpos.y);
    legoobject_transform_position_get(obj, &pos.x, &pos.y);
    legoobject_o_vector2_subtract(&d, &pos, &mpos);
    dist = legoobject_o_vector2_modulus(&d);
    radius = entity_stats_float_at70(LEGOOBJ_SLOT(monster));
    if (monster->flags3 & 0x80000000) {
        if (obj->type == 2 || obj->type == 1) {
            if (entity_stats_float_at9c(LEGOOBJ_SLOT(monster)) > dist) {
                legoobject_dispatch_event31_for_pending_flag(monster);
                return 1;
            }
        }
        return 0;
    }
    if (obj->flags2 & 2) return 0;
    if (obj->flags2 & 0x100) return 0;
    if ((s_panel_highlight_flags & 1) && (obj->flags4 & 4)) return 0;
    if (obj->driveObject) return 0;

    if (obj->type == 2) {
        if (entity_stats_word_get(LEGOOBJ_SLOT(monster)) & 0x8000) {
            ddiscene_object_facing_pair_get(legoobject_type_component_get(monster), NULL, dir, NULL);
            x = 1.0f / (float)sqrt(dir[0] * dir[0] + dir[1] * dir[1]);
            y = dir[1] * x * 15.0f + mpos.y;
            x = dir[0] * x * 15.0f + mpos.x;
            terrain_map_world_to_block(Lego_GetMap(), x, y, &blockPos[0], &blockPos[1]);
            if (!terrain_cell_walkable_flag_current(blockPos[0], blockPos[1]) &&
                !terrain_cell_occupied_or_flag_current(blockPos[0], blockPos[1], 1))
            {
                x = pos.x - x;
                y = pos.y - y;
                if ((float)sqrt(x * x + y * y) < entity_stats_float_at6c(LEGOOBJ_SLOT(obj))) {
                    legoobject_pairing_activity_raise_and_mutual_link(monster, obj);
                    return 0;
                }
            }
        }
        if (entity_stats_word_get(LEGOOBJ_SLOT(monster)) & 0x200) {
            if (dist < entity_stats_float_at6c(LEGOOBJ_SLOT(monster)) + entity_stats_float_at6c(LEGOOBJ_SLOT(obj))) {
                legoobject_enter_event21_state_once(obj);
                monster->health = -1.0f;
                monster->flags3 |= 0x800000;
            }
        }
        if (entity_stats_word_get(LEGOOBJ_SLOT(monster)) & 0x800) {
            if (legoobject_zero != radius && dist < entity_stats_float_at6c(LEGOOBJ_SLOT(obj)) + radius) {
                legoobject_o_vector2_scale(&d, &d, 1.0f / dist);
                legoobject_flagged_vector_step_command_issue(obj, &d.x);
            }
        }
    }

    if ((obj->type == 1 || obj->type == 2) && !(obj->flags1 & 0x200)) {
        if (entity_stats_float_ate8(LEGOOBJ_SLOT(monster)) < obj->health &&
            entity_stats_float_at94(LEGOOBJ_SLOT(obj)) > dist)
        {
            legoobject_pair_interaction_gate(monster, obj);
        }
    }

    if ((obj->type == 2 || obj->type == 1) && (entity_stats_word_get(LEGOOBJ_SLOT(monster)) & 0x2000) &&
        !(monster->flags1 & 0x8000000))
    {
        if (dist < entity_stats_float_at6c(LEGOOBJ_SLOT(obj)) + radius) {
            if (legoobject_random_cell_step_route_issue(monster)) monster->flags1 |= 0x8000000;
        }
    }

    if (obj->type == 2 && (entity_stats_word_get(LEGOOBJ_SLOT(monster)) & 0x200000)) {
        x = pos.x - LEGOOBJ_FIELD(monster, LegoObjO_Home*, 0x32c)->x;
        y = pos.y - LEGOOBJ_FIELD(monster, LegoObjO_Home*, 0x32c)->y;
        if ((float)sqrt(x * x + y * y) < entity_stats_float_at6c(LEGOOBJ_SLOT(obj)) + radius) {
            legoobject_damage_apply(obj, entity_indexed_float_lookup_with_minus_one_fallback(
                                             // STRING: LEGORR 0x004a50c8
                                             object_type_name_index_find("BatAttack"), obj),
                                    1, 1.0f);
            monster->flags2 |= 0x80;
            if (!(LEGOOBJ_FIELD(monster, LegoObjO_Home*, 0x32c)->flags & 1)) {
                aitask_entity_task_batch_append((struct EntitySlot**)&monster, 1, LEGOOBJ_SLOT(obj));
                LEGOOBJ_FIELD(monster, LegoObjO_Home*, 0x32c)->flags |= 1;
            }
        }
    }

    if (entity_stats_word_get(LEGOOBJ_SLOT(monster)) & 0x40000) {
        // STRING: LEGORR 0x004a50bc
        if (entity_type_pair_lookup("Scorpion", &type, &id, NULL) && type == obj->type && id == obj->id) {
            if (legoobject_zero != radius && dist < entity_stats_float_at6c(LEGOOBJ_SLOT(obj)) + radius) {
                legoobject_o_vector2_scale(&d, &d, 1.0f / dist);
                legoobject_flagged_vector_step_command_issue(obj, &d.x);
            }
        }
    }
    return 0;
}

/* ================================================================== */
/* Part 3 (issue #615): 00445d70-0044c710, groups P-T.                 */
/* Part 3's declarations, state and imports sit here, after parts 1    */
/* and 2, so the symbol counter at every part-1/2 function does not    */
/* move (finding 538 rule B, finding 577). Natively the file is C++,   */
/* so part 3 opens its own extern "C" block.                           */
/* ================================================================== */
#ifdef __cplusplus
extern "C" {
#endif

/* ---- part 3 prototypes, group P (00445d70-004473b0) ---- */
Lego_Object* legoobject_flag_gated_callback_dispatch(Lego_Object* obj, float* pos, float minRadius, int skipFlag8); /* 00445d70 */
int legoobject_flag_gated_callback_b(Lego_Object* obj, void* data); /* 00445df0 merged callback */
int legoobject_flag_gated_callback_a(Lego_Object* obj, void* data); /* 004460f0 merged callback */
void legoobject_movement_tick_scale_and_delta_get(Lego_Object* obj, float elapsed, float* outTransSpeed, float* outElapsed); /* 00446610 */
int legoobject_route_adjacent_step_append(Lego_Object* obj, int bx, int by); /* 004468c0 */
int legoobject_nearest_connected_cell_route_select(Lego_Object* obj, unsigned int bx, unsigned int by, int diagonals, unsigned int* outIndex, int strict); /* 004469c0 */
int legoobject_nearest_cell_route_commit(Lego_Object* obj, unsigned int bx, unsigned int by, int diagonals); /* 00446e40 */
int legoobject_conditional_type_five_spawn(Lego_Object* obj); /* 004470d0 */
int legoobject_try_create_linked_at_position(Lego_Object* obj, int bx, int by, Lego_Object* linked); /* 004471b0 */
int legoobject_adjacent_placement_cell_search(Lego_Object* obj, Lego_Object* target); /* 00447210 */
int legoobject_route_to_cell_command_issue(Lego_Object* obj, int bx, int by, Lego_Object* target); /* 004473b0 */

/* ---- part 3 prototypes, group Q (004474f0-00447ea0) ---- */
void legoobject_linked_proximity_response(Lego_Object* obj); /* 004474f0 */
int legoobject_progress_negative_delta_handle(Lego_Object* obj); /* 004475c0 */
int legoobject_progress_advance_by_25(Lego_Object* obj); /* 00447730 */
void legoobject_clear_linked_object_state(Lego_Object* obj); /* 00447780 */
void legoobject_path_step_lookahead_write(Lego_Object* obj); /* 004477d0 */
void legoobject_type_four_callback_dispatch(Lego_Object* obj); /* 00447900 */
int legoobject_type_four_callback(Lego_Object* obj, void* data); /* 00447920 merged callback */
void legoobject_directional_activity_start(Lego_Object* obj, float* dir, int mode); /* 00447950 */
void legoobject_dispatch_event5_and_set_flag(Lego_Object* obj); /* 00447b00 */
float legoobject_activity_tick(Lego_Object* obj, float elapsed); /* 00447b30 */
int legoobject_activity_register_bookkeeping(Lego_Object* obj); /* 00447c40 */
void legoobject_type_behavior_update(Lego_Object* obj, float elapsed); /* 00447ea0 */

/* ---- part 3 prototypes, group R (004487c0-00449ac0) ---- */
void legoobject_clear_nested_state_8_fields(Lego_Object* obj); /* 004487c0 */
int legoobject_move_to_cell_coordinates(Lego_Object* obj, int* blockPos); /* 00448800 */
int legoobject_record_gated_step_command_issue(Lego_Object* obj, int* blockPos); /* 00448880 */
int legoobject_kind6_tick_down_marker_spawn(Lego_Object* obj); /* 004489a0 */
int legoobject_neighbor_cell_move_command_handle(Lego_Object* obj, int* blockPos); /* 00448a60 */
int legoobject_set_activity_place(Lego_Object* obj); /* 00448c50 */
int legoobject_cardinal_step_command_issue_and_link(Lego_Object* obj, Lego_Object* target, int linkMode); /* 00448c90 */
int legoobject_proximity_step_command_issue_with_jitter(Lego_Object* obj, Lego_Object* target, int marker); /* 00448eb0 */
int legoobject_kind6_record_approach_route_issue(Lego_Object* obj); /* 00449010 */
int legoobject_proximity_step_command_issue_with_link(Lego_Object* obj, Lego_Object* target, int mode, int setGate); /* 004490a0 */
int legoobject_walkable_target_cell_route_issue(Lego_Object* obj, int* blockPos); /* 00449240 */
int legoobject_cell_step_command_issue_with_target_mark(Lego_Object* obj, int* blockPos); /* 004492b0 */
int legoobject_has_task_type_4_5_7(Lego_Object* obj); /* 004493e0 */
int legoobject_target_cell_route_and_associate(Lego_Object* obj, Lego_Object* target); /* 00449430 */
int legoobject_target_cell_route_marker14(Lego_Object* obj, Lego_Object* target); /* 00449520 */
int legoobject_coordinate_route_and_mark(Lego_Object* obj, int* blockPos); /* 00449610 */
int legoobject_route_step_command_issue(Lego_Object* obj); /* 00449700 */
int legoobject_blocked_cell_approach_weight_write(Lego_Object* obj, unsigned int bx, unsigned int by, float amount); /* 00449880 */
int legoobject_activity_terrain_eligibility_transition(Lego_Object* obj, float elapsed, int* out); /* 00449980 */
void legoobject_activity23_state_transition(Lego_Object* obj); /* 00449a70 */
int legoobject_proximity_step_command_issue_clearing_gate(Lego_Object* obj, Lego_Object* target); /* 00449ac0 */

/* ---- part 3 prototypes, group S (00449c00-0044ae50) ---- */
void legoobject_simtick_render_step(void); /* 00449c00 */
void legoobject_render_step_callback(void* group, void* item, void* data); /* 00449c20 merged callback */
void legoobject_scene_object_state_handle_apply(Lego_Object* obj, int hide); /* 00449cd0 */
int legoobject_hide_check_callback(Lego_Object* obj, void* data); /* 00449cf0 merged callback */
void legoobject_type_operation_dispatch(Lego_Object* obj, int hide); /* 00449f50 */
int legoobject_type_operation_release(Lego_Object* obj); /* 00449ff0 */
void legoobject_follow_target_pose_get(Lego_Object* obj, int cameraNo, float* pos, float* dir); /* 0044a070 */
void* legoobject_type_component_get(Lego_Object* obj); /* 0044a200 */
int legoobject_work_object_position_get(Lego_Object* obj, float* outX, float* outY); /* 0044a2a0 */
void legoobject_movement_input_accumulate(Lego_Object* obj, int forward, int turn, float elapsed); /* 0044a310 */
void legoobject_type11_cell_coordinate_append(int* blockPos); /* 0044a390 */
int legoobject_type11_cell_coordinate_nearest_resolve(Lego_Object* obj, int* outBlockPos); /* 0044a3d0 */
void legoobject_predug_coordinate_pool_append(int* blockPos); /* 0044a4b0 */
int legoobject_nearest_cell_coordinate_world_resolve(Lego_Object* obj, int* outBlockPos); /* 0044a4f0 */
int legoobject_nearest_open_cell_coord_search(Lego_Object* obj, int* outX, int* outY, int useMinDist, int allowC0, int allow20); /* 0044a5d0 */
int legoobject_open_cell_coord_search_callback(const void* a, const void* b); /* 0044a7a0 merged callback */
void* legoobject_poly_variant_section_model_build(void* config, const char* gameName, const char* dir, int polyKind, unsigned int cameraCount); /* 0044a7d0 */
int legoobject_building_placement_preview_update(int objType, int objID, int clicked, int toolTipOnly, float* mousePos, int bx, int by, int active, void* surface); /* 0044a910 */
void legoobject_sfx_grid_config_load(void* config, const char* gameName); /* 0044acc0 */
int legoobject_sfx_at_cell_get(Lego_Object* obj); /* 0044ad80 */
void legoobject_scatter_spawn_gate_set(int on); /* 0044adc0 */
void legoobject_kind_one_random_spawn_trigger(Lego_Object* obj); /* 0044ade0 */
void legoobject_random_kind_six_scatter_spawn(Lego_Object* obj, unsigned int count, int fromBuilding); /* 0044ae50 */

/* ---- part 3 prototypes, group T (0044af90-0044c5f0) ---- */
void legoobject_attached_spawn_record_process(Lego_Object* obj); /* 0044af90 */
void legoobject_tiny_monster_spawn_and_teardown(Lego_Object* obj); /* 0044afb0 */
void legoobject_small_spider_cell_swarm_spawn(int bx, int by, unsigned int count); /* 0044b140 */
void legoobject_pairing_activity_raise_and_mutual_link(Lego_Object* first, Lego_Object* second); /* 0044b250 */
int legoobject_activity_is_special(int activity); /* 0044b350 */
void legoobject_activity_grant(Lego_Object* obj, int activity); /* 0044b370 */
int legoobject_activity_scan(Lego_Object* obj, int activity); /* 0044b480 */
int legoobject_type_two_target_resolve_and_apply(Lego_Object* obj, int code); /* 0044b4c0 */
int legoobject_position_step_command_issue_and_link(Lego_Object* obj, int selector); /* 0044b500 */
void legoobject_scene_object_tree_attach(Lego_Object* obj); /* 0044b680 */
void legoobject_matrix_rotate_z_with_offset(void* cont, float* matrix, float offset); /* 0044b7a0 */
void legoobject_scene_node_sync_callback(void* flock, void* item, void* data); /* 0044b850 merged callback */
void legoobject_resource_dispose_callback(void* flock, void* item, void* data); /* 0044bab0 merged callback */
void legoobject_resource_and_callback_state_dispose(void* flock); /* 0044bae0 */
void legoobject_node_scale_update(Lego_Object* obj, int doubled); /* 0044bb30 */
void legoobject_motion_to_scene_node_sync(Lego_Object* obj, float elapsed); /* 0044bc30 */
void legoobject_auxiliary_payload_release(Lego_Object* obj); /* 0044be30 */
void legoobject_timed_callback_list_update(float* elapsed); /* 0044bf00 */
int legoobject_monster_spawn_behavior_tick(Lego_Object* obj); /* 0044bfd0 */
int legoobject_one_shot_attached_effect_start(Lego_Object* obj, float duration); /* 0044c030 */
void legoobject_attached_entity_wake(Lego_Object* obj); /* 0044c110 */
int legoobject_pair_scalar_fields_set(Lego_Object* obj, float* dir, float distance); /* 0044c150 */
void legoobject_heading_step_move(Lego_Object* obj, float elapsed); /* 0044c1b0 */
void legoobject_selection_clear_marker_and_reset_hud(Lego_Object* obj); /* 0044c4a0 */
int legoobject_selection_clear_callback(Lego_Object* obj, void* data); /* 0044c500 merged callback */
int legoobject_ai_can_do(Lego_Object* obj); /* 0044c530 */
void legoobject_scatter_spawn_enable_and_drain(void); /* 0044c550 */
int legoobject_scatter_spawn_drain_callback(Lego_Object* obj, void* data); /* 0044c5f0 merged callback */

/* ---- state and imports, group P (00445d70-004473b0; part 3, issue #615) ---- */

/* Point2F and Vector3F (Maths.h). */
typedef struct LegoObjP_Point2F {
    float x;
    float y;
} LegoObjP_Point2F;
typedef struct LegoObjP_Vector3F {
    float x;
    float y;
    float z;
} LegoObjP_Vector3F;

/* The search record of 00445d70's two RunThroughLists callbacks. */
typedef struct LegoObjP_CollisionSearch {
    Lego_Object* obj;    /* +0x00 the object that moves */
    float* pos;          /* +0x04 Point2F: where it moves to */
    float minRadius;     /* +0x08 */
    Lego_Object* result; /* +0x0c the object it hits */
    int skipFlag8;       /* +0x10 */
} LegoObjP_CollisionSearch;

/* Container.c (weapons_imports.h; container_unit.cpp). */
void scene_object_coordinate_transform_in_place(void* object,
                                                void* coordinates,
                                                void* transform); /* 00475070 */
/* Maths.c (gods98/maths.h). */
int point_in_polygon_2d(const float* point, const float* fromList,
                        const float* toList, unsigned int count); /* 00479b80 */
/* Collision.c (view.hpp, placement.cpp). */
float point_to_polyline_minimum_segment_distance(const float* point,
                                                 const float* first_point,
                                                 const float* last_point,
                                                 unsigned int segment_count); /* 00408b00 */
/* Vehicle.c (vehicle_model.h, vehicle_model.c). */
struct VehicleModel;
float vehicle_get_trans_coef(struct VehicleModel* vehicle);                /* 0046db70 */
/* Creature.c: Creature_GetTransCoef (animation_object.c). */
float animation_object_value_get(void* object);                            /* 00407090 */
/* Building.c: Building_GetTransCoef returns 0.0 (building_model.cpp's
 * constant_zero_float_get, which takes no argument in the port). */
double constant_zero_float_get(void* building);                            /* 00408930 */
/* Stats.c (entity.hpp, entity_stats.c). */
float entity_stats_float_at20(const struct EntitySlot* e);                 /* 00469cb0 */
float entity_stats_float_atd4(const struct EntitySlot* e);                 /* 00469ce0 */
float entity_stats_float_atd8(const struct EntitySlot* e);                 /* 00469d20 */
void* entity_stats_field_at_a0_corpus(const struct EntitySlot* e);         /* 00469da0 */
/* Lego.c (lego_unit.h, lego_unit.c). */
extern void* legoGlobs_contBoulder;                                        /* 00556368 */
void* lego_terrain_cell_palette_resource_at_grid_position(const int* xy); /* 00429160 */

/* ---- functions, group P (00445d70-004473b0; part 3, issue #615) ---- */

/* LegoObject_CheckCollision: the first object that obj would hit at pos.
 * Stats flag 0x100 tests obj's collision box (004460f0), 0x80 its radius
 * (00445df0). */
// FUNCTION: LEGORR 0x00445d70
Lego_Object* legoobject_flag_gated_callback_dispatch(Lego_Object* obj, float* pos, float minRadius, int skipFlag8)
{
    LegoObjP_CollisionSearch search;

    search.obj = obj;
    search.pos = pos;
    search.minRadius = minRadius;
    search.result = NULL;
    search.skipFlag8 = skipFlag8;
    if (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x100)
        legoobject_run_through_lists_skip_upgrade_parts(legoobject_flag_gated_callback_a, &search);
    if (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x80)
        legoobject_run_through_lists_skip_upgrade_parts(legoobject_flag_gated_callback_b, &search);
    return search.result;
}

/* The RunThroughLists callback of 00445d70 for a moving object with a
 * collision radius (stats flag 0x80): liveObj is hit when the radii
 * overlap, or when pos is in or near liveObj's collision box. */
// FUNCTION: LEGORR 0x00445df0
int legoobject_flag_gated_callback_b(Lego_Object* liveObj, void* data)
{
    LegoObjP_CollisionSearch* search = (LegoObjP_CollisionSearch*)data;
    Lego_Object* obj = search->obj;
    LegoObjP_Point2F livePos;
    void* cont;
    float dist;
    float dx;
    float dy;
    LegoObjP_Vector3F box[4];
    LegoObjP_Point2F poly[5];
    int i;

    if (search->skipFlag8 && (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 8)) return 0;
    if (liveObj != obj && liveObj->carryingThisObject != obj && obj->carryingThisObject != liveObj &&
        liveObj != obj->driveObject && !((obj->flags1 & 0x200000) || (obj->flags1 & 0x400000)) &&
        !((liveObj->flags1 & 0x200000) || (liveObj->flags1 & 0x400000)))
    {
        if (entity_stats_float_at74(LEGOOBJ_SLOT(liveObj)) > search->minRadius) {
            legoobject_transform_position_get(liveObj, &livePos.x, &livePos.y);
            cont = legoobject_type_component_get(liveObj);
            dx = livePos.x - search->pos[0];
            dy = livePos.y - search->pos[1];
            dist = (float)sqrt(dx * dx + dy * dy);
            if (entity_stats_word_get(LEGOOBJ_SLOT(liveObj)) & 0x80) {
                if (entity_stats_float_at6c(LEGOOBJ_SLOT(obj)) + entity_stats_float_at6c(LEGOOBJ_SLOT(liveObj)) > dist)
                    search->result = liveObj;
            } else if (entity_stats_word_get(LEGOOBJ_SLOT(liveObj)) & 0x100) {
                if (entity_stats_float_at6c(LEGOOBJ_SLOT(obj)) + entity_stats_float_at6c(LEGOOBJ_SLOT(liveObj)) > dist) {
                    box[0].x = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(liveObj)))[0] * -0.5f;
                    box[0].z = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(liveObj)))[1] * 0.5f;
                    box[1].x = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(liveObj)))[0] * 0.5f;
                    box[1].z = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(liveObj)))[1] * 0.5f;
                    box[2].x = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(liveObj)))[0] * 0.5f;
                    box[2].z = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(liveObj)))[1] * -0.5f;
                    box[3].x = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(liveObj)))[0] * -0.5f;
                    box[3].z = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(liveObj)))[1] * -0.5f;
                    box[0].y = box[1].y = box[2].y = box[3].y = 0.0f;
                    for (i = 0; i < 4; i++) {
                        scene_object_coordinate_transform_in_place(cont, &box[i], &box[i]);
                        poly[i] = *(LegoObjP_Point2F*)&box[i];
                    }
                    poly[4] = poly[0];
                    if (point_in_polygon_2d(search->pos, &poly[0].x, &poly[1].x, 4)) {
                        search->result = liveObj;
                    } else {
                        dist = point_to_polyline_minimum_segment_distance(search->pos, &poly[0].x, &poly[1].x, 4);
                        if (entity_stats_float_at6c(LEGOOBJ_SLOT(obj)) > dist) search->result = liveObj;
                    }
                }
            }
        }
        if (search->result) return 1;
    }
    return 0;
}

/* The RunThroughLists callback of 00445d70 for a moving object with a
 * collision box (stats flag 0x100): liveObj is hit when it is inside the
 * box moved to pos, or when the two boxes overlap. */
// FUNCTION: LEGORR 0x004460f0
int legoobject_flag_gated_callback_a(Lego_Object* liveObj, void* data)
{
    LegoObjP_CollisionSearch* search = (LegoObjP_CollisionSearch*)data;
    Lego_Object* obj = search->obj;
    LegoObjP_Point2F objPos;
    LegoObjP_Point2F livePos;
    void* liveCont;
    void* objCont;
    float dist;
    float dx;
    float dy;
    int i;
    int j;

    if (search->skipFlag8 && (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 8)) return 0;
    if (liveObj != obj && liveObj->carryingThisObject != obj && obj->carryingThisObject != liveObj &&
        !((obj->flags1 & 0x200000) || (obj->flags1 & 0x400000)) &&
        !((liveObj->flags1 & 0x200000) || (liveObj->flags1 & 0x400000)) &&
        entity_stats_float_at74(LEGOOBJ_SLOT(liveObj)) > search->minRadius)
    {
        legoobject_transform_position_get(liveObj, &livePos.x, &livePos.y);
        legoobject_transform_position_get(obj, &objPos.x, &objPos.y);
        objPos.x -= search->pos[0];
        objPos.y -= search->pos[1];
        liveCont = legoobject_type_component_get(liveObj);
        objCont = legoobject_type_component_get(obj);
        if (entity_stats_word_get(LEGOOBJ_SLOT(liveObj)) & 0x80) {
            dx = livePos.x - search->pos[0];
            dy = livePos.y - search->pos[1];
            dist = (float)sqrt(dx * dx + dy * dy);
            if (dist < entity_stats_float_at6c(LEGOOBJ_SLOT(obj)) + entity_stats_float_at6c(LEGOOBJ_SLOT(liveObj))) {
                LegoObjP_Vector3F box[4];
                LegoObjP_Point2F poly[5];
                box[0].x = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(obj)))[0] * -0.5f;
                box[0].z = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(obj)))[1] * 0.5f;
                box[1].x = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(obj)))[0] * 0.5f;
                box[1].z = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(obj)))[1] * 0.5f;
                box[2].x = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(obj)))[0] * 0.5f;
                box[2].z = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(obj)))[1] * -0.5f;
                box[3].x = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(obj)))[0] * -0.5f;
                box[3].z = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(obj)))[1] * -0.5f;
                box[0].y = box[1].y = box[2].y = box[3].y = 0.0f;
                for (i = 0; i < 4; i++) {
                    scene_object_coordinate_transform_in_place(objCont, &box[i], &box[i]);
                    poly[i] = *(LegoObjP_Point2F*)&box[i];
                    poly[i].x = objPos.x + poly[i].x;
                    poly[i].y = objPos.y + poly[i].y;
                }
                poly[4] = poly[0];
                if (point_in_polygon_2d(&livePos.x, &poly[0].x, &poly[1].x, 4)) {
                    search->result = liveObj;
                } else {
                    dist = point_to_polyline_minimum_segment_distance(&livePos.x, &poly[0].x, &poly[1].x, 4);
                    if (entity_stats_float_at6c(LEGOOBJ_SLOT(liveObj)) > dist) search->result = liveObj;
                }
            }
        } else if (entity_stats_word_get(LEGOOBJ_SLOT(liveObj)) & 0x100) {
            dx = livePos.x - search->pos[0];
            dy = livePos.y - search->pos[1];
            dist = (float)sqrt(dx * dx + dy * dy);
            if (dist < entity_stats_float_at6c(LEGOOBJ_SLOT(obj)) + entity_stats_float_at6c(LEGOOBJ_SLOT(liveObj))) {
                Lego_Object* objs[2];
                LegoObjP_Vector3F boxes[2][4];
                LegoObjP_Point2F polys[2][5];
                objs[0] = liveObj;
                objs[1] = obj;
                for (i = 0; i < 2; i++) {
                    boxes[i][0].x = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(objs[i])))[0] * -0.5f;
                    boxes[i][0].z = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(objs[i])))[1] * 0.5f;
                    boxes[i][1].x = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(objs[i])))[0] * 0.5f;
                    boxes[i][1].z = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(objs[i])))[1] * 0.5f;
                    boxes[i][2].x = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(objs[i])))[0] * 0.5f;
                    boxes[i][2].z = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(objs[i])))[1] * -0.5f;
                    boxes[i][3].x = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(objs[i])))[0] * -0.5f;
                    boxes[i][3].z = ((float*)entity_stats_field_at_a0_corpus(LEGOOBJ_SLOT(objs[i])))[1] * -0.5f;
                    boxes[i][0].y = boxes[i][1].y = boxes[i][2].y = boxes[i][3].y = 0.0f;
                }
                for (j = 0; j < 4; j++) {
                    scene_object_coordinate_transform_in_place(objCont, &boxes[1][j], &boxes[1][j]);
                    polys[1][j] = *(LegoObjP_Point2F*)&boxes[1][j];
                }
                for (j = 0; j < 4; j++) {
                    scene_object_coordinate_transform_in_place(liveCont, &boxes[0][j], &boxes[0][j]);
                    polys[0][j] = *(LegoObjP_Point2F*)&boxes[0][j];
                }
                for (i = 0; i < 2; i++) polys[i][4] = polys[i][0];
                for (j = 0; j < 4; j++) {
                    if (point_in_polygon_2d(&polys[0][j].x, &polys[1][0].x, &polys[1][1].x, 4))
                        search->result = liveObj;
                }
            }
        }
        if (search->result) return 1;
    }
    return 0;
}

/* LegoObject_GetSpeedAndElapsed: the object's route speed this frame
 * (outTransSpeed) and its animation time step (outElapsed), from its
 * stats, its terrain and its model's transition coefficient. */
// FUNCTION: LEGORR 0x00446610
void legoobject_movement_tick_scale_and_delta_get(Lego_Object* obj, float elapsed, float* outTransSpeed, float* outElapsed)
{
    unsigned int flag400 = obj->flags1 & 0x400;
    unsigned int flag2 = obj->flags1 & 2;
    unsigned int flag8 = obj->flags1 & 8;
    unsigned int slow = obj->flags1 & 0x2000000;
    float transCoef;
    float speed;

    if (obj->type == 1) {
        transCoef = vehicle_get_trans_coef((struct VehicleModel*)obj->vehicle);
    } else if (obj->type == 2) {
        transCoef = animation_object_value_get(obj->miniFigure);
    } else if (obj->type == 3) {
        transCoef = animation_object_value_get(obj->rockMonster);
    } else if (obj->type == 4) {
        transCoef = (float)constant_zero_float_get(obj->building);
    } else if ((obj->flags3 & 0x20000) || obj->type == 10) {
        *outElapsed = elapsed;
        return;
    }

    if (flag8) {
        *outTransSpeed = entity_stats_float_at20(LEGOOBJ_SLOT(obj)) * 0.05f;
    } else if (flag400 && obj->type != 1) {
        *outTransSpeed = entity_stats_float_at20(LEGOOBJ_SLOT(obj)) * 0.5f;
    } else {
        speed = entity_stats_float_at20(LEGOOBJ_SLOT(obj));
        *outTransSpeed = speed;
        if (flag2) *outTransSpeed = speed * 0.5f;
    }
    if (slow) *outTransSpeed *= 0.05f;

    {
        float centre;
        int blockPos[2];
        float x;
        float y;
        float coef;

        centre = 1.0f;
        legoobject_transform_position_get(obj, &x, &y);
        if (terrain_map_world_to_block_centre(Lego_GetMap(), x, y, &blockPos[0], &blockPos[1], &centre)) {
            coef = 1.0f - ((float)(unsigned int)terrain_cell_speed_class_current(blockPos) * 0.25f) *
                              (1.0f - entity_stats_float_atd4(LEGOOBJ_SLOT(obj)));
            if (level_cell_occupied_at_current(blockPos)) coef = entity_stats_float_atd8(LEGOOBJ_SLOT(obj)) * coef;
            if (obj->type == 2 &&
                ((LegoObjJ_Level*)g_active_level_retail)->blocks[((LegoObjJ_Level*)g_active_level_retail)->width * blockPos[1] + blockPos[0]].terrain == 9)
            {
                coef = (1.0f - centre * 0.8f) * coef;
            }
            if (terrain_cell_flag_8_bounded_current(blockPos[0], blockPos[1])) *outTransSpeed *= coef;
        }
    }

    if (legoobject_zero != transCoef) *outElapsed = *outTransSpeed / transCoef;
    else *outElapsed = 1.0f;
    if (obj->type == 4 && (entity_state_flags_get(LEGOOBJ_SLOT(obj)) & 2) && (obj->flags2 & 0x8000))
        *outElapsed = entity_speed_component_get(LEGOOBJ_SLOT(obj)) * *outElapsed;
    *outElapsed *= elapsed;
    *outTransSpeed *= elapsed;
}

/* LegoObject_Route_AddBlock: append the block (bx, by) to the route being
 * built when it is a cardinal neighbour of the route's last block. */
// FUNCTION: LEGORR 0x004468c0
int legoobject_route_adjacent_step_append(Lego_Object* obj, int bx, int by)
{
    int lastX;
    int lastY;
    LegoObjG_RouteBlock* newList;

    if (obj->flags1 & 1) {
        lastX = ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].blockPos[0];
        lastY = ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].blockPos[1];
        if (((bx == lastX && (by == lastY - 1 || by == lastY + 1)) ||
             (by == lastY && (bx == lastX - 1 || bx == lastX + 1))) &&
            (newList = (LegoObjG_RouteBlock*)realloc(obj->routeBlocks,
                                                      sizeof(LegoObjG_RouteBlock) * (obj->routeBlocksTotal + 1))) != NULL)
        {
            obj->routeBlocks = newList;
            ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal].blockPos[0] = bx;
            ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal].blockPos[1] = by;
            ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal].blockOffset[0] = 0.5f;
            ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal].blockOffset[1] = 0.5f;
            ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal].flags = 0;
            ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal].actionFlags = 1;
            obj->routeBlocksTotal++;
            obj->flags3 &= ~0x400;
            return 1;
        }
    }
    return 0;
}

/* Routes obj next to the block (bx, by): onto it from obj's own block
 * when that is one of its neighbours, else along the shortest path to the
 * nearest neighbour reachable. outIndex receives the neighbour's index;
 * diagonals selects the eight-neighbour table. */
// FUNCTION: LEGORR 0x004469c0
int legoobject_nearest_connected_cell_route_select(Lego_Object* obj, unsigned int bx, unsigned int by, int diagonals, unsigned int* outIndex, int strict)
{
    unsigned int j;
    unsigned int i;
    unsigned int count;
    unsigned int counts[8];
    unsigned int* xs[8];
    unsigned int* ys[8];
    unsigned int valid[8] = {0};
    LegoObjP_Point2F dirs8[8] = {{-1.0f, -2.0f}, {0.0f, -2.0f}, {1.0f, -1.0f}, {1.0f, 0.0f},
                                 {0.0f, 1.0f}, {-1.0f, 1.0f}, {-2.0f, 0.0f}, {-2.0f, -1.0f}};
    LegoObjP_Point2F dirs4[4] = {{0.0f, -1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f}};
    float py;
    float px;
    float wy;
    float wx;
    unsigned int found;
    unsigned int bestCount;
    float dist;
    unsigned int best;
    int objBy;
    int objBx;
    int result;
    LegoObjP_Point2F* dirs;
    int canReach;

    best = (unsigned int)-1;
    bestCount = (unsigned int)-1;
    found = (unsigned int)-1;
    result = 0;
    canReach = 1;
    if (diagonals) {
        dirs = dirs8;
        count = 8;
    } else {
        dirs = dirs4;
        count = 4;
    }

    legoobject_position_to_terrain_cell(obj, &objBx, &objBy);
    if (!strict) {
        terrain_map_block_to_world(Lego_GetMap(), bx, by, &wx, &wy);
        legoobject_transform_position_get(obj, &px, &py);
        dist = (float)sqrt((wx - px) * (wx - px) + (wy - py) * (wy - py));
        if (dist < ((float*)Lego_GetMap())[4]) canReach = 0;
    }

    for (i = 0; i < count; i++) {
        dirs[i].x += (float)bx;
        dirs[i].y += (float)by;
        if (dirs[i].x == (float)objBx && dirs[i].y == (float)objBy && canReach &&
            (diagonals || !terrain_cell_walkable_flag_current(objBx, objBy)))
        {
            found = i;
            break;
        }
    }

    if (found != (unsigned int)-1) {
        if (legoobject_route_setup_and_marker_associate(obj, 1, (unsigned int*)&objBx, (unsigned int*)&objBy, NULL)) {
            if (outIndex) *outIndex = found;
            result = 1;
        } else {
            result = 0;
        }
        return result;
    }

    for (i = 0; i < count; i++) {
        if ((float)objBx != dirs[i].x || (float)objBy != dirs[i].y) {
            if (legoobject_terrain_neighbor_path_check(obj, objBx, objBy, (int)dirs[i].x, (int)dirs[i].y,
                                                       &xs[i], &ys[i], &counts[i], NULL, NULL))
                valid[i] = 1;
        }
    }

    for (i = 0; i < count * 2; i++) {
        if (best != (unsigned int)-1 && i == count) break;
        j = i % count;
        if (valid[j]) {
            if (i < count && terrain_cell_walkable_flag_current((int)dirs[j].x, (int)dirs[j].y)) continue;
            if (counts[j] < bestCount) {
                best = j;
                bestCount = counts[j];
            }
        }
    }

    if (best != (unsigned int)-1) {
        if (legoobject_route_setup_and_marker_associate(obj, counts[best], xs[best], ys[best], NULL)) {
            if (outIndex) *outIndex = best;
            result = 1;
        } else {
            result = 0;
        }
    }

    for (i = 0; i < count; i++) {
        if (valid[i]) {
            free(xs[i]);
            free(ys[i]);
        }
    }
    return result;
}

/* Routes obj to the block (bx, by) through the nearest reachable
 * neighbour (004469c0) and ends the route at an offset inside the block:
 * at the side it enters from (diagonals) or at a random point. */
// FUNCTION: LEGORR 0x00446e40
int legoobject_nearest_cell_route_commit(Lego_Object* obj, unsigned int bx, unsigned int by, int diagonals)
{
    unsigned int index;

    if (terrain_cell_walkability_query_current(LEGOOBJ_SLOT(obj), bx, by, diagonals, 1) &&
        legoobject_nearest_connected_cell_route_select(obj, bx, by, diagonals, &index, 1))
    {
        if (diagonals) {
            float blockOffsets[8][2] = {{0.9f, 0.5f}, {0.1f, 0.5f}, {0.5f, 0.1f}, {0.5f, 0.9f},
                                        {0.1f, 0.5f}, {0.9f, 0.5f}, {0.5f, 0.9f}, {0.5f, 0.1f}};
            int corners[8] = {0, 1, 1, 2, 2, 3, 3, 0};
            int offsets[4][2] = {{-1, -1}, {0, -1}, {0, 0}, {-1, 0}};
            legoobject_route_adjacent_step_append(obj, offsets[corners[index]][0] + bx,
                                                  offsets[corners[index]][1] + by);
            ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].blockOffset[0] = blockOffsets[index][0];
            ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].blockOffset[1] = blockOffsets[index][1];
            obj->targetBlockPos[0] = (float)bx;
            obj->flags3 |= 0x2000000;
            obj->targetBlockPos[1] = (float)by;
            return 1;
        }
        legoobject_route_adjacent_step_append(obj, bx, by);
        ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].blockOffset[0] = random_float_range(0.3f, 0.7f);
        ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].blockOffset[1] = random_float_range(0.3f, 0.7f);
        obj->targetBlockPos[0] = (float)bx;
        obj->flags3 &= ~0x2000000;
        obj->targetBlockPos[1] = (float)by;
        return 1;
    }
    return 0;
}

/* A boulder thrower (flags3 0x4000000) that is healthy enough creates a
 * boulder, hides it, and routes to pick it up at the nearest open block. */
// FUNCTION: LEGORR 0x004470d0
int legoobject_conditional_type_five_spawn(Lego_Object* obj)
{
    int blockPos[2];
    Lego_Object* boulder;

    if ((obj->flags3 & 0x4000000) && entity_stats_float_at90(LEGOOBJ_SLOT(obj)) < obj->health &&
        legoobject_nearest_open_cell_coord_search(obj, &blockPos[0], &blockPos[1], 1, 0, 0))
    {
        boulder = (Lego_Object*)legoobject_create(legoGlobs_contBoulder, 5, 0);
        legoobject_world_position_set(boulder, 10000.0f, 10000.0f, 0.0f, 1);
        legoobject_type_operation_dispatch(boulder, 1);
        if (legoobject_try_create_linked_at_position(obj, blockPos[0], blockPos[1], boulder)) {
            legoobject_object_list_field_assign_all(boulder, (int)(uintptr_t)lego_terrain_cell_palette_resource_at_grid_position(blockPos));
            obj->field_33c = 0.0f;
            return 1;
        }
        legoobject_type5_finalize_if_allowed(boulder);
    }
    return 0;
}

/* Routes obj to pick up linked at the block (bx, by): the route's last
 * block gets action 3, and the two objects are linked. */
// FUNCTION: LEGORR 0x004471b0
int legoobject_try_create_linked_at_position(Lego_Object* obj, int bx, int by, Lego_Object* linked)
{
    if (!(obj->flags1 & 0x400) && legoobject_blocked_cell_approach_weight_write(obj, bx, by, 4.0f)) {
        ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].actionFlags = 3;
        obj->routeToObject = linked;
        linked->interactObject = obj;
        return 1;
    }
    return 0;
}

/* A rock monster (type 3) routes to a block two blocks from target from
 * which it can throw at it: twelve candidates, starting at a random one in
 * a random direction, first skipping blocks with mask 0x180000. Without a
 * target it drops what it carries. */
// FUNCTION: LEGORR 0x00447210
int legoobject_adjacent_placement_cell_search(Lego_Object* obj, Lego_Object* target)
{
    unsigned int start;
    int blockPos[2];
    int targetPos[2];
    int reverse;
    unsigned int pass;
    unsigned int i;
    unsigned int j;
    unsigned int k;

    if (obj->type == 3) {
        if (target) {
            int offsets[12][2] = {{2, 0}, {0, 2}, {-2, 0}, {0, -2}, {2, -1}, {2, 1},
                                  {1, 2}, {-1, 2}, {-2, 1}, {-2, -1}, {-1, -2}, {1, -2}};
            start = rng_thunk_draw15() % 12;
            reverse = rng_thunk_draw15() % 2;
            legoobject_position_to_terrain_cell(target, &targetPos[0], &targetPos[1]);
            for (pass = 0; pass < 2; pass++) {
                for (i = 0, j = start; i < 12; i++, j++) {
                    k = j;
                    if (reverse == 1) k = 24 - j;
                    blockPos[0] = offsets[k % 12][0] + targetPos[0];
                    blockPos[1] = offsets[k % 12][1] + targetPos[1];
                    if ((pass == 1 || !terrain_cell_mask_180000_current(blockPos)) &&
                        legoobject_route_to_cell_command_issue(obj, blockPos[0], blockPos[1], target))
                        return 1;
                }
            }
            return 0;
        }
        legoobject_attached_object_list_detach(obj, 0);
    }
    return 0;
}

/* Routes obj (carrying) to the block (bx, by), next to target, and aims
 * what it carries at target: the route's last block gets action 5. */
// FUNCTION: LEGORR 0x004473b0
int legoobject_route_to_cell_command_issue(Lego_Object* obj, int bx, int by, Lego_Object* target)
{
    int result = 0;
    int targetX;
    int targetY;

    if ((obj->flags1 & 0x400) && !legoobject_flags_mask107c33de_or_1c00203_any_test(obj)) {
        legoobject_position_to_terrain_cell(target, &targetX, &targetY);
        if (targetX != bx || targetY != by) {
            int objX;
            int objY;
            unsigned int* xs;
            unsigned int* ys;
            unsigned int count;

            legoobject_position_to_terrain_cell(obj, &objX, &objY);
            if (objX == bx && objY == by) {
                result = legoobject_route_setup_and_marker_associate(obj, 1, (unsigned int*)&objX, (unsigned int*)&objY, NULL);
            } else {
                if (!legoobject_terrain_neighbor_path_check(obj, objX, objY, bx, by, &xs, &ys, &count, NULL, NULL))
                    return 0;
                result = legoobject_route_setup_and_marker_associate(obj, count, xs, ys, NULL);
                free(xs);
                free(ys);
            }
            if (result) {
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].actionFlags = 5;
                obj->carriedObjects[0]->routeToObject = target;
                return result;
            }
        }
    }
    return 0;
}

/* ---- state and imports, group Q (004474f0-00447ea0; part 3, issue #615) ---- */

/* Lego.c (lego_unit.h). */
extern float legoGlobs_DrainTime;                                          /* 00556f04 */
/* Stats.c (entity.hpp, entity_stats.c). */
float entity_stats_float_at88(const struct EntitySlot* e);                 /* 00469f20 */
/* Vehicle.c (vehicle_model.h, vehicle_model.c). */
float vehicle_move_animation(struct VehicleModel* vehicle, float elapsed1, float elapsed2,
                             unsigned int repeats);                        /* 0046d1c0 */
/* Creature.c (animation_object.c) and Building.c (view.hpp, building_model.cpp). */
float renderer_object_timing_value_adjust_by_count(void* record, float time,
                                                   unsigned int count);    /* 00406d30 */
float animation_loop_time_adjust(void* record, float argument, unsigned int count); /* 00408860 */
/* Upgrade.c: Upgrade_Part_MoveAnimation (0046c110), upgrade_levels.cpp's
 * original-shape function (finding 597). */
float upgrade_part_move_animation(struct RenderResourceBinding* upgradeModel, float elapsed,
                                  unsigned int repeat);                    /* 0046c110 */
/* Vehicle.c (Vehicle_SetActivity 0046c3f0, Vehicle_GetCameraNull 0046da50):
 * the port's entries have other contracts, so an adapter at the end of
 * legoobject_port.cpp stands in. Building.c's Building_SetActivity
 * (00408570) and Building_GetCameraNull (004085f0) are building_model.c's
 * (finding 592). */
int legoobject_gq_vehicle_set_activity_adapter(void* vehicle, const char* activityName,
                                               float time);                /* 0046c3f0 */
void* vehicle_get_camera_null(struct VehicleModel* vehicle, unsigned int frameNo); /* 0046da50 */
unsigned int building_set_activity(void* building, const char* activityName,
                                   float time);                            /* 00408570 */
void* scene_activity_change_secondary(void* activity, int index);          /* 004085f0 */
void* activity_runner_state_read(void* object, int cameraNo);              /* 00406f80 */
/* Weapons.c (weapons.c). */
void weapons_generic_damage_object(void* obj, int weaponType, int knownWeapon,
                                   float elapsed, const float* dir);       /* 0046f270 */
/* Camera.c (camera_control.c); legoGlobs.cameraMain is view.cpp's. */
void camera_shake(void* cam, float intensity, float duration);             /* 00435f32 */
extern void* s_zoom_step_context;                                          /* 005560e4 */
/* Lego.c (lego_unit.h, lego_unit.c). */
extern float legoGlobs_DynamiteMaxDamage;                                  /* 00556468 */
extern float legoGlobs_DynamiteWakeRadius;                                 /* 0055646c */
void terrain_cell_secondary_flag_8_set_current(int bx, int by, int on);    /* 004324d0 */
void terrain_cell_flag_80000_clear_or_set_current(const int blockPos[2], int on); /* 00432710 */
/* Map3D.c (terrain_map.c). */
void terrain_map_block_highlight_clear(struct TerrainMap* map, int bx, int by); /* 0044f570 */
/* Effects.c (rock_fall_styles.c): Effect_Spawn_Explosion. */
void effect_spawn_explosion(void* obj, float* pos);                         /* 0040c750 */

/* ---- functions, group Q (004474f0-00447ea0; part 3, issue #615) ---- */

/* The object obj routes to (routeToObject) is pushed away and hurt when
 * it is within the two collision radii. */
// FUNCTION: LEGORR 0x004474f0
void legoobject_linked_proximity_response(Lego_Object* obj)
{
    Lego_Object* target = obj->routeToObject;
    LegoObjP_Point2F dir;
    LegoObjP_Point2F targetPos;
    LegoObjP_Point2F objPos;
    float dist;

    obj->routeToObject = NULL;
    if (target) {
        legoobject_transform_position_get(target, &targetPos.x, &targetPos.y);
        legoobject_transform_position_get(obj, &objPos.x, &objPos.y);
        legoobject_n_vector2d_subtract(&dir.x, &targetPos.x, &objPos.x);
        dist = legoobject_n_vector2d_modulus(&dir.x);
        if (dist < entity_stats_float_at6c(LEGOOBJ_SLOT(target)) + entity_stats_float_at6c(LEGOOBJ_SLOT(obj)) * 2.0f) {
            legoobject_directional_activity_start(target, &dir.x, 1);
            legoobject_damage_apply(target, 10.0f, 1, 1.0f);
        }
    }
}

/* Works on the object obj routes to: drains a building's power, upgrades
 * it, or repairs (or, with a negative repair value, damages) it. Returns
 * nonzero when the work is done. */
// FUNCTION: LEGORR 0x004475c0
int legoobject_progress_negative_delta_handle(Lego_Object* obj)
{
    int done = 0;
    Lego_Object* target = obj->routeToObject;
    float repair;

    if (target && !(target->flags2 & 0x100000)) {
        if (entity_state_flags_get(LEGOOBJ_SLOT(obj)) & 0x80000) {
            if (legoobject_shared_eligibility_check(target, 0)) {
                if (obj->field_3d8 >= legoGlobs_DrainTime) {
                    legoobject_process_queued_grid_animations(obj, -1, 1.0f);
                    obj->field_3d8 = 0.0f;
                }
            } else {
                return 1;
            }
        } else if (obj->flags4 & 0x400000) {
            legoobject_building_upgrade_commit(target);
            obj->flags4 &= ~0x400000;
        } else {
            if (obj->type == 3) info_send(5, NULL, target, NULL);
            repair = entity_stats_float_at88(LEGOOBJ_SLOT(obj));
            if (repair < legoobject_zero) {
                legoobject_damage_apply(target, -repair, 1, 1.0f);
                target->flags2 |= 0x40000000;
            } else {
                target->health = repair + target->health;
            }
            if (target->health >= 100.0f) {
                target->health = 100.0f;
                done = 1;
            } else {
                aitask_entity_threshold_progress_event_queue(LEGOOBJ_SLOT(target), 0);
            }
        }
        if (target->health <= legoobject_zero) done = 1;
    } else {
        done = 1;
    }
    if (done) obj->routeToObject = NULL;
    return done;
}

/* Eating: energy rises by 25 up to health. */
// FUNCTION: LEGORR 0x00447730
int legoobject_progress_advance_by_25(Lego_Object* obj)
{
    float energy = obj->energy + 25.0f;

    obj->routeToObject = NULL;
    obj->energy = energy;
    if (energy > obj->health) {
        obj->energy = obj->health;
        return 1;
    }
    return 0;
}

/* Puts down the object obj carries, without dropping it in place. */
// FUNCTION: LEGORR 0x00447780
void legoobject_clear_linked_object_state(Lego_Object* obj)
{
    Lego_Object* carried;

    if (obj->flags1 & 0x400) {
        carried = obj->carriedObjects[0];
        obj->flags1 &= ~0x400;
        obj->numCarriedObjects = 0;
        obj->carriedObjects[0] = NULL;
        carried->flags3 |= 0x2000;
        aitask_entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 4, LEGOOBJ_SLOT(carried), 1);
    }
}

/* Ends the route a tenth of a block towards the target of the object obj
 * carries, and hands the target's position to the carried object. */
// FUNCTION: LEGORR 0x004477d0
void legoobject_path_step_lookahead_write(Lego_Object* obj)
{
    Lego_Object* target = obj->carriedObjects[0]->routeToObject;
    LegoObjP_Point2F pos;
    LegoObjP_Point2F dir;
    LegoObjP_Point2F targetPos;
    LegoObjP_Point2F offset;
    float inv;

    terrain_map_block_to_world(Lego_GetMap(),
                               ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].blockPos[0],
                               ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].blockPos[1],
                               &pos.x, &pos.y);
    legoobject_transform_position_get(target, &targetPos.x, &targetPos.y);
    dir.x = targetPos.x - pos.x;
    dir.y = targetPos.y - pos.y;
    inv = 1.0f / (float)sqrt(dir.x * dir.x + dir.y * dir.y);
    dir.x = dir.x * inv;
    dir.y = inv * dir.y;
    dir.x = dir.x * 0.1f;
    dir.y = dir.y * 0.1f;
    pos.x = dir.x + pos.x;
    pos.y = dir.y + pos.y;
    terrain_map_world_to_block_offset(Lego_GetMap(), (struct TerrainMapPoint2*)&pos, NULL,
                                      (struct TerrainMapPoint2*)&offset);
    *(LegoObjP_Point2F*)((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].blockOffset = offset;
    ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].flags |= 8;
    *(LegoObjP_Point2F*)obj->carriedObjects[0]->targetBlockPos = targetPos;
    obj->carriedObjects[0]->routeToObject = NULL;
}

/* Releases every boulder (type 5) that targets obj, a building (type 4). */
// FUNCTION: LEGORR 0x00447900
void legoobject_type_four_callback_dispatch(Lego_Object* obj)
{
    if (obj->type == 4) legoobject_run_through_lists_skip_upgrade_parts(legoobject_type_four_callback, obj);
}

/* The RunThroughLists callback of 00447900: a boulder routing to the
 * building data is dropped by whoever carries it. */
// FUNCTION: LEGORR 0x00447920
int legoobject_type_four_callback(Lego_Object* obj, void* data)
{
    if (obj->type == 5 && obj->routeToObject == (Lego_Object*)data && obj->carryingThisObject)
        legoobject_action_sequence_reset(obj->carryingThisObject, 0);
    return 0;
}

/* LegoObject_StartTurn-like reaction to a push along dir: the object (or
 * the vehicle it drives) plays the hit-front/back/left/right activity
 * that matches dir against its facing, or hit-hard / thrown without a
 * direction. */
// FUNCTION: LEGORR 0x00447950
void legoobject_directional_activity_start(Lego_Object* obj, float* dir, int mode)
{
    LegoObjP_Point2F face;
    LegoObjP_Point2F d;
    float c;
    float inv;
    float dot;
    float limit;
    unsigned int activity;

    if ((obj->flags2 & 2) && obj->field_3c0) obj = (Lego_Object*)obj->field_3c0;
    if (!(obj->flags1 & 0x1000) && legoobject_shared_eligibility_check(obj, 1)) {
        if (dir) {
            face = *(LegoObjP_Point2F*)obj->faceDirection;
            d = *(LegoObjP_Point2F*)dir;
            c = (float)cos(0.78539818f);
            inv = 1.0f / (float)sqrt(face.x * face.x + face.y * face.y);
            face.x = face.x * inv;
            face.y = face.y * inv;
            dot = face.x * d.x + face.y * d.y;
            limit = 1.0f - c;
            if (dot > limit) activity = 0x1a;
            else if (dot < -limit) activity = 0x19;
            else if (face.y * d.x - face.x * d.y > legoobject_zero) activity = 0x18;
            else activity = 0x17;
        } else {
            activity = mode ? 0x1b : 0x24;
        }
        legoobject_state_reset(obj, 0, 1);
        legoobject_set_activity(obj, activity, 1);
        if (!legoobject_activity_register_bookkeeping(obj)) {
            legoobject_set_activity(obj, 0, 1);
            legoobject_activity_register_bookkeeping(obj);
        }
        legoobject_activity_tick(obj, 1.0f);
        obj->flags1 |= 0x1000;
        if (obj->type == 3) obj->flags3 &= 0x7fffffff;
    }
}

/* Starts activity 5 (teleport) and marks the object teleporting. */
// FUNCTION: LEGORR 0x00447b00
void legoobject_dispatch_event5_and_set_flag(Lego_Object* obj)
{
    legoobject_set_activity(obj, 5, 0);
    legoobject_activity_register_bookkeeping(obj);
    obj->flags1 |= 0x200000;
}

/* LegoObject_MoveAnimation: advances the object's activity animation by
 * its speed-scaled time step and returns that step. */
// FUNCTION: LEGORR 0x00447b30
float legoobject_activity_tick(Lego_Object* obj, float elapsed)
{
    float transSpeed;
    float moveElapsed;

    legoobject_movement_tick_scale_and_delta_get(obj, elapsed, &transSpeed, &moveElapsed);
    if (!(obj->flags1 & 0x4000)) {
        if (obj->type == 1) {
            obj->animTime = vehicle_move_animation((struct VehicleModel*)obj->vehicle, moveElapsed, elapsed, obj->animRepeat);
        } else if (obj->type == 2) {
            obj->animTime = renderer_object_timing_value_adjust_by_count(obj->miniFigure, moveElapsed, obj->animRepeat);
        } else if (obj->type == 3) {
            obj->animTime = renderer_object_timing_value_adjust_by_count(obj->rockMonster, moveElapsed, obj->animRepeat);
        } else if (obj->type == 4) {
            obj->animTime = animation_loop_time_adjust(obj->building, moveElapsed, obj->animRepeat);
        } else if (obj->flags3 & 0x20000) {
            legoobject_type_behavior_update(obj, elapsed);
        } else if (obj->type == 10) {
            obj->animTime = upgrade_part_move_animation((struct RenderResourceBinding*)obj->upgradePart, moveElapsed, obj->animRepeat);
        }
    }
    if (obj->animTime > legoobject_zero) task_record_kind14_remove_if_head(LEGOOBJ_SLOT(obj));
    return moveElapsed;
}

/* LegoObject_UpdateActivityChange: when the activity name changed, starts
 * it on the object's model (and the driver's "Activity_<name><vehicle>"
 * animation), and registers the object as not idle unless the new
 * activity is a resting or moving one. Returns the model's result. */
// FUNCTION: LEGORR 0x00447c40
int legoobject_activity_register_bookkeeping(Lego_Object* obj)
{
    int result = 0;
    const char* name = obj->activityName1;
    const char* activity;
    int isRoute;
    char buffer[256];

    if (name != obj->activityName2) {
        if (name != s_activity[0] && name != s_activity[1] && name != s_activity[4] && name != s_activity[6] &&
            name != s_activity[8] && name != s_activity[17] && name != s_activity[22] && name != s_activity[2])
            aitask_ai_idle_chain_record_attach(LEGOOBJ_SLOT(obj));

        switch (obj->type) {
        case 1:
            result = legoobject_gq_vehicle_set_activity_adapter(obj->vehicle, obj->activityName1, obj->animTime);
            obj->field_2dc = vehicle_get_camera_null((struct VehicleModel*)obj->vehicle, obj->field_2e0);
            break;
        case 2:
            result = activity_runner_update((struct AnimationObject*)obj->miniFigure, obj->activityName1, obj->animTime);
            obj->field_2dc = activity_runner_state_read(obj->miniFigure, obj->field_2e0);
            break;
        case 3:
            result = activity_runner_update((struct AnimationObject*)obj->rockMonster, obj->activityName1, obj->animTime);
            obj->field_2dc = activity_runner_state_read(obj->rockMonster, obj->field_2e0);
            break;
        case 4:
            result = building_set_activity(obj->building, obj->activityName1, obj->animTime);
            obj->field_2dc = scene_activity_change_secondary(obj->building, obj->field_2e0);
            break;
        }

        if ((entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x20000) && obj->driveObject && obj->driveObject->type == 2) {
            isRoute = !_strcmpi(s_activity[1], obj->activityName1);
            // STRING: LEGORR 0x004a50dc
            activity = isRoute ? "Activity_" : obj->activityName1;
            // STRING: LEGORR 0x004a50d4
            sprintf(buffer, "%s%s", activity, legoobject_item_name_by_kind(obj->type, obj->id));
            activity_runner_update((struct AnimationObject*)obj->driveObject->miniFigure, buffer, 0.0f);
            obj->driveObject->field_2dc = activity_runner_state_read(obj->driveObject->miniFigure, obj->driveObject->field_2e0);
            obj->driveObject->activityName1 = obj->driveObject->activityName2;
        }
        obj->animTime = 0.0f;
        obj->activityName2 = obj->activityName1;
        obj->flags2 &= ~0x20000;
        return result;
    }
    return 0;
}

/* LegoObject_UpdateMiscObject: the per-frame behaviour of the miscellaneous
 * object types: a thrown boulder (5) rolls to its target and hits what it
 * meets, a crystal (6) spins, dynamite (8) and a sonic blaster (13) tick
 * down and explode, an electric fence (11) is placed, and the barrier,
 * spider web and other types (9, 12, 18, 14, 16, 17, 19) run their
 * animation states. */
// FUNCTION: LEGORR 0x00447ea0
void legoobject_type_behavior_update(Lego_Object* obj, float elapsed)
{
    LegoObjP_Vector3F pos;
    LegoObjP_Vector3F target;
    LegoObjP_Vector3F dir;
    LegoObjP_Point2F pos2D;
    LegoObjP_Point2F dir2D;
    int blockPos[2];
    int blockPos2[2];
    float len;
    float speed;
    float inv;
    Lego_Object* hit;
    float wx;
    float wy;
    float angle;

    if (obj->type == 5) {
        if (obj->flags3 & 0x2000) {
            ddiscene_object_position_get(obj->other, NULL, &pos.x);
            target.x = obj->targetBlockPos[0];
            target.y = obj->targetBlockPos[1];
            target.z = 0.0f;
            target.z = terrain_map_world_z_get(Lego_GetMap(), target.x, target.y) -
                       entity_stats_float_at6c(LEGOOBJ_SLOT(obj));
            dir.x = target.x - pos.x;
            dir.y = target.y - pos.y;
            dir.z = target.z - pos.z;
            len = (float)sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            speed = elapsed * 3.5f;
            inv = 1.0f / len;
            dir.x = dir.x * inv;
            dir.y = dir.y * inv;
            dir.z = dir.z * inv;
            dir.x = dir.x * speed;
            dir.y = dir.y * speed;
            dir.z = dir.z * speed;
            pos.x = dir.x + pos.x;
            pos.y = dir.y + pos.y;
            pos.z = dir.z + pos.z;
            if (len < speed) {
                obj->carryingThisObject = NULL;
                obj->health = -1.0f;
                obj->flags3 = (obj->flags3 & ~0x2000) | 0x800000;
            } else {
                target = pos;
            }
            pos2D.x = target.x;
            pos2D.y = target.y;
            terrain_map_world_to_block(Lego_GetMap(), target.x, target.y, &blockPos[0], &blockPos[1]);
            hit = legoobject_flag_gated_callback_dispatch(
                obj, &pos2D.x, terrain_map_world_z_get(Lego_GetMap(), target.x, target.y) - target.z, 1);
            if (hit) {
                obj->carryingThisObject = NULL;
                obj->flags3 &= ~0x2000;
                inv = 1.0f / (float)sqrt(dir.x * dir.x + dir.y * dir.y);
                dir2D.x = inv * dir.x;
                dir2D.y = inv * dir.y;
                weapons_generic_damage_object(hit, object_type_name_index_find("Boulder"), 1, 0.0f, &dir2D.x);
                obj->flags3 |= 0x800000;
                obj->health = -1.0f;
                sound_submission_route_wrapper(NULL, 0x22, 0, 0, &target.x);
            }
            ddiscene_object_position_set(obj->other, NULL, target.x, target.y, target.z);
        }
    } else if (obj->type == 6) {
        if (!obj->carryingThisObject)
            render_object_rotate_axis(obj->other, 1, 0.0f, 1.0f, 0.0f, elapsed * 0.01f);
    } else if (obj->type == 8 || obj->type == 13) {
        if (obj->flags3 & 0x10000) {
            if (obj->health <= legoobject_zero || ddiscene_advance_by_time_target(obj->other, elapsed) != 0.0f) {

                blockPos[0] = (int)obj->targetBlockPos[0];
                blockPos[1] = (int)obj->targetBlockPos[1];
                if (blockPos[0] && blockPos[1])
                    lego_terrain_cell_mutate((struct Lego_Level*)level_current_owner_get(), blockPos[0], blockPos[1], 0);
                if (obj->type == 8) {
                    legoobject_position_to_terrain_cell(obj, &blockPos[0], &blockPos[1]);
                    terrain_cell_flag_80000_clear_or_set_current(blockPos, 0);
                    legoobject_spatial_callback_query(obj, legoGlobs_DynamiteDamageRadius, legoGlobs_DynamiteMaxDamage,
                                                      legoGlobs_DynamiteWakeRadius);
                    sound_alias_play(0x2b, 0);
                    effect_spawn_explosion(obj, NULL);
                    camera_shake(s_zoom_step_context, 5.0f, 50.0f);
                } else if (obj->type == 13) {
                    legoobject_transform_position_get(obj, &pos.x, &pos.y);
                    legoobject_position_to_terrain_cell(obj, &blockPos2[0], &blockPos2[1]);
                    pos.z = terrain_map_world_z_get(Lego_GetMap(), pos.x, pos.y);
                    lava_effect_select_and_place(8, &pos.x, NULL);
                    camera_shake(s_zoom_step_context, 2.0f, 100.0f);
                    terrain_cell_flag_80000_clear_or_set_current(blockPos2, 0);
                }
                obj->health = -1.0f;
                obj->flags3 |= 0x800000;
            }
        } else if (obj->flags1 & 0x80000) {

            obj->flags1 &= ~0x80000;
            obj->flags3 |= 0x10000;
            scene_action_name_set(obj->other, "TickDown");
            ddiscene_frame_update_and_callbacks(obj->other, 0.0f);
            legoobject_position_to_terrain_cell(obj, &blockPos2[0], &blockPos2[1]);
            terrain_cell_flag_80000_clear_or_set_current(blockPos2, 1);
            info_send(13, NULL, obj, NULL);
            terrain_map_block_highlight_clear(Lego_GetMap(), (int)obj->targetBlockPos[0], (int)obj->targetBlockPos[1]);
        } else {
            ddiscene_advance_by_time_target(obj->other, elapsed);
        }
    } else if (obj->type == 11) {
        if (obj->flags1 & 0x80000) {

            obj->flags1 &= ~0x80000;
            electric_fence_add((struct ElectricFenceObject*)obj);
            legoobject_position_to_terrain_cell(obj, &blockPos2[0], &blockPos2[1]);
            terrain_map_block_to_world(Lego_GetMap(), blockPos2[0], blockPos2[1], &wx, &wy);
            angle = legoobject_vector_direction_angle_radians_get(obj) + 0.78539819f;
            legoobject_world_position_set(obj, wx, wy, angle - (float)fmod(angle, 1.5707964f), 1);
            terrain_cell_secondary_flag_8_set_current(blockPos2[0], blockPos2[1], 0);
        }
    } else if (obj->type == 9) {
        if (ddiscene_advance_by_time_target(obj->other, elapsed) != 0.0f) {
            if (obj->flags1 & 0x80000) {
                obj->flags1 = (obj->flags1 & ~0x80000) | 0x100;
                // STRING: LEGORR 0x004a5120
                scene_action_name_set(obj->other, "Expand");
                ddiscene_frame_update_and_callbacks(obj->other, 0.0f);
            } else if (obj->flags1 & 0x100) {
                obj->flags1 &= ~0x100;
                obj->flags3 |= 0x10000;
                // STRING: LEGORR 0x004a5118
                scene_action_name_set(obj->other, "Long");
                ddiscene_frame_update_and_callbacks(obj->other, 0.0f);
            } else if (obj->flags3 & 0x800000) {
                obj->health = 100.0f;
                obj->flags3 &= ~0x800000;
                obj->flags1 |= 0x200000;
                // STRING: LEGORR 0x004a510c
                scene_action_name_set(obj->other, "Teleport");
                ddiscene_frame_update_and_callbacks(obj->other, 0.0f);
            } else if (obj->flags1 & 0x200000) {
                obj->flags1 &= ~0x200000;
                obj->health = -1.0f;
                obj->flags3 |= 0x800000;
            }
        }
    } else if (obj->type == 12) {
        if (ddiscene_advance_by_time_target(obj->other,
                                            ((obj->flags1 & 0x2000000) && !(obj->flags1 & 0x100)) ? 0.0f : elapsed) != 0.0f)
        {
            if (obj->flags1 & 0x100) {
                obj->flags1 &= ~0x100;
                obj->flags3 |= 0x10000;
                scene_action_name_set(obj->other, "FlapInWind");
                ddiscene_frame_update_and_callbacks(obj->other, 0.0f);
            } else if (obj->flags3 & 0x800000) {
                obj->health = 100.0f;
                obj->flags3 &= ~0x800000;
                obj->flags1 |= 0x200000;
                // STRING: LEGORR 0x004a50fc
                scene_action_name_set(obj->other, "Disintegrate");
                ddiscene_frame_update_and_callbacks(obj->other, 0.0f);
            } else if (obj->flags1 & 0x200000) {
                obj->flags1 &= ~0x200000;
                obj->health = -1.0f;
                obj->flags3 |= 0x800000;
            }
        }
    } else if (obj->type == 18) {
        if (ddiscene_advance_by_time_target(obj->other, elapsed) != 0.0f) {
            if (obj->flags1 & 0x100) {
                obj->flags1 &= ~0x100;
                obj->flags3 |= 0x10000;
                scene_action_name_set(obj->other, "Normal");
                ddiscene_frame_update_and_callbacks(obj->other, 0.0f);
            } else if (obj->flags3 & 0x800000) {
                obj->health = 100.0f;
                obj->flags3 &= ~0x800000;
                obj->flags1 |= 0x200000;
                // STRING: LEGORR 0x004a50f4
                scene_action_name_set(obj->other, "Melt");
                ddiscene_frame_update_and_callbacks(obj->other, 0.0f);
            } else if (obj->flags1 & 0x200000) {
                obj->flags1 &= ~0x200000;
                obj->health = -1.0f;
                obj->flags3 |= 0x800000;
                ((Lego_Object*)obj->field_364)->flags2 &= ~0x8000000;
            }
        }
    } else if (obj->type == 14 || obj->type == 16 || obj->type == 17 || obj->type == 19) {
        ddiscene_advance_by_time_target(obj->other, elapsed);
    }
}

/* ---- state and imports, group R (004487c0-00449ac0; part 3, issue #615) ---- */

/* Lego.c (lego_unit.c): the level's cell predicates and markers. */
int lego_entity_move_destination_cell_usable(struct EntitySlot* liveObj, unsigned int bx, unsigned int by); /* 00432950 */
int level_cell_flag8_check_target(const void* unused, int bx, int by); /* 004329d0 */
int lego_terrain_cell_allows_placement(struct EntitySlot* liveObj, unsigned int bx, unsigned int by); /* 00432a20 */
void terrain_cell_flag_80000_clear_or_set_current(const int blockPos[2], int on); /* 00432710 */
void terrain_cell_flag_10000000_set_current(const int cell_xy[2], int set); /* 00433120 */
void lego_terrain_cell_queued_work_transition(unsigned int bx, unsigned int by); /* 00431140 */
/* ElectricFence.c (terrain_cell_objects.h). */
int electric_fence_can_place_at_block(int bx, int by); /* 0040d240 */

/* The route block the unit's route buffer ends in. The original's route
 * block offset is a Point2F, copied as a struct (two dword moves); the
 * header's float[2] is read through LegoObjGE_Point2F for that copy. */
#define LEGOOBJ_R_LAST_ROUTE(o) \
    (((LegoObjG_RouteBlock*)(o)->routeBlocks)[(o)->routeBlocksTotal - 1])

/* ---- functions, group R (004487c0-00449ac0; part 3, issue #615) ---- */

// FUNCTION: LEGORR 0x004487c0
void legoobject_clear_nested_state_8_fields(Lego_Object* obj)
{
    if ((obj->flags1 & 0x400) && obj->carriedObjects[0]->type == 8) {
        obj->carriedObjects[0]->targetBlockPos[1] = 0.0f;
        obj->carriedObjects[0]->targetBlockPos[0] = 0.0f;
        legoobject_attached_object_list_detach(obj, 1);
    }
}

// FUNCTION: LEGORR 0x00448800
int legoobject_move_to_cell_coordinates(Lego_Object* obj, int* blockPos)
{
    Lego_Object* carried;

    if (obj->carriedObjects[0]->type == 8 &&
        lego_entity_move_destination_cell_usable(LEGOOBJ_SLOT(obj), blockPos[0], blockPos[1]) &&
        legoobject_blocked_cell_approach_weight_write(obj, blockPos[0], blockPos[1], 7.0f)) {
        carried = obj->carriedObjects[0];
        LEGOOBJ_R_LAST_ROUTE(obj).actionFlags = 9;
        carried->targetBlockPos[0] = (float)blockPos[0];
        carried->targetBlockPos[1] = (float)blockPos[1];
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00448880
int legoobject_record_gated_step_command_issue(Lego_Object* obj, int* blockPos)
{
    int result;
    Lego_Object* carried;
    unsigned int* routeX;
    unsigned int* routeY;
    unsigned int count;
    LegoObjF_Point2I ownPos;

    if (obj->carriedObjects[0]->type == 0xd &&
        level_cell_flag8_check_target(obj, blockPos[0], blockPos[1])) {
        legoobject_position_to_terrain_cell(obj, &ownPos.x, &ownPos.y);
        if (ownPos.x == blockPos[0] && ownPos.y == blockPos[1]) {
            legoobject_route_setup_and_marker_associate(obj, 1,
                (unsigned int*)&blockPos[0], (unsigned int*)&blockPos[1], NULL);
            return 0;
        }
        if (legoobject_terrain_neighbor_path_check(obj, ownPos.x, ownPos.y, blockPos[0], blockPos[1],
                &routeX, &routeY, &count, NULL, NULL)) {
            result = legoobject_route_setup_and_marker_associate(obj, count, routeX, routeY, NULL);
            free(routeX);
            free(routeY);
            if (result) {
                carried = obj->carriedObjects[0];
                LEGOOBJ_R_LAST_ROUTE(obj).actionFlags = 9;
                carried->targetBlockPos[0] = (float)blockPos[0];
                carried->targetBlockPos[1] = (float)blockPos[1];
                return 1;
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x004489a0
int legoobject_kind6_tick_down_marker_spawn(Lego_Object* obj)
{
    Lego_Object* marker;
    float pos[2];
    int blockPos[2];

    if (legoobject_activity_scan(obj, 6)) {
        legoobject_transform_position_get(obj, &pos[0], &pos[1]);
        legoobject_position_to_terrain_cell(obj, &blockPos[0], &blockPos[1]);
        marker = legoobject_create_in_world(legoGlobs_contOohScary, 0xd, 0, 0, pos[0], pos[1],
            legoobject_vector_direction_angle_radians_get(obj));
        terrain_cell_flag_80000_clear_or_set_current(blockPos, 1);
        // STRING: LEGORR 0x004a50e8
        scene_action_name_set(marker->other, "TickDown");
        ddiscene_frame_update_and_callbacks(marker->other, 0.0f);
        marker->flags3 |= 0x10000;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00448a60
int legoobject_neighbor_cell_move_command_handle(Lego_Object* obj, int* blockPos)
{
    int result;
    unsigned int* routeX;
    unsigned int* routeY;
    unsigned int count;
    float jitter;
    Lego_Object* carried;
    LegoObjGE_Point2F offset;
    LegoObjF_Point2I ownPos;

    if (obj->carriedObjects[0]->type == 0xb &&
        electric_fence_can_place_at_block(blockPos[0], blockPos[1])) {
        legoobject_position_to_terrain_cell(obj, &ownPos.x, &ownPos.y);
        if (ownPos.x == blockPos[0] && ownPos.y == blockPos[1]) {
            result = legoobject_route_setup_and_marker_associate(obj, 1,
                (unsigned int*)&blockPos[0], (unsigned int*)&blockPos[1], NULL);
        } else {
            if (!legoobject_terrain_neighbor_path_check(obj, ownPos.x, ownPos.y, blockPos[0], blockPos[1],
                    &routeX, &routeY, &count, NULL, NULL))
                return 0;
            result = legoobject_route_setup_and_marker_associate(obj, count, routeX, routeY, NULL);
            free(routeX);
            free(routeY);
        }
        if (result) {
            LEGOOBJ_R_LAST_ROUTE(obj).actionFlags = 9;
            if (obj->type == 1) {
                jitter = random_float_range(0.55f, 0.75f);
                switch (rng_thunk_draw15() % 4) {
                case 0:
                    offset.x = 0.5f;
                    offset.y = jitter;
                    break;
                case 1:
                    offset.y = 0.5f;
                    offset.x = jitter;
                    break;
                case 2:
                    offset.x = 0.5f;
                    offset.y = 1.0f - jitter;
                    break;
                case 3:
                    offset.y = 0.5f;
                    offset.x = 1.0f - jitter;
                    break;
                }
                *(LegoObjGE_Point2F*)LEGOOBJ_R_LAST_ROUTE(obj).blockOffset = offset;
                LEGOOBJ_R_LAST_ROUTE(obj).flags |= 8;
            }
            carried = obj->carriedObjects[0];
            carried->targetBlockPos[0] = (float)blockPos[0];
            carried->targetBlockPos[1] = (float)blockPos[1];
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00448c50
int legoobject_set_activity_place(Lego_Object* obj)
{
    legoobject_set_activity(obj, 0x1e, 0);
    legoobject_activity_register_bookkeeping(obj);
    obj->flags2 |= 0x200;
    return 1;
}

// FUNCTION: LEGORR 0x00448c90
int legoobject_cardinal_step_command_issue_and_link(Lego_Object* obj, Lego_Object* target, int linkMode)
{
    int result;
    void* cont;
    unsigned int* routeX;
    unsigned int* routeY;
    unsigned int count;
    int dir;
    LegoObjF_Point2I blockPos;
    LegoObjF_Point2I ownPos;
    LegoObjF_Point2I targetPos;
    LegoObjGE_Point2F delta;
    float pos[3];

    result = 0;
    if (target->type == 4) {
        if (obj->type == 2) {
            if (legoobject_proximity_step_command_issue_with_jitter(obj, target, 1))
                result = 1;
        } else if (obj->type == 1) {
            cont = legoobject_k_carry_null_get(target);
            ddiscene_object_position_get(cont, NULL, pos);
            terrain_map_world_to_block(Lego_GetMap(), pos[0], pos[1], &blockPos.x, &blockPos.y);
            legoobject_position_to_terrain_cell(obj, &ownPos.x, &ownPos.y);
            legoobject_position_to_terrain_cell(target, &targetPos.x, &targetPos.y);
            delta.x = (float)blockPos.x - (float)targetPos.x;
            delta.y = (float)blockPos.y - (float)targetPos.y;
            if (fabs(delta.x) > fabs(delta.y))
                dir = (delta.x > legoobject_zero) ? 1 : 3;
            else
                dir = (delta.y > legoobject_zero) ? 0 : 2;
            if (ownPos.x == blockPos.x && ownPos.y == blockPos.y) {
                result = legoobject_route_setup_and_marker_associate(obj, 1,
                    (unsigned int*)&blockPos.x, (unsigned int*)&blockPos.y, NULL);
            } else {
                if (legoobject_terrain_neighbor_path_check(obj, ownPos.x, ownPos.y, blockPos.x, blockPos.y,
                        &routeX, &routeY, &count, NULL, NULL)) {
                    result = legoobject_route_setup_and_marker_associate(obj, count, routeX, routeY, NULL);
                    free(routeX);
                    free(routeY);
                } else
                    return 0;
            }
            if (result) {
                LEGOOBJ_R_LAST_ROUTE(obj).actionFlags = 0xf;
                LEGOOBJ_R_LAST_ROUTE(obj).flags |= 4;
                LEGOOBJ_R_LAST_ROUTE(obj).flags |= dir & 3;
                obj->field_404 = linkMode;
                obj->routeToObject = target;
                target->interactObject = obj;
            }
        }
    }
    return result;
}

// FUNCTION: LEGORR 0x00448eb0
int legoobject_proximity_step_command_issue_with_jitter(Lego_Object* obj, Lego_Object* target, int marker)
{
    unsigned int dir;
    float scale;
    float jitter;
    LegoObjF_Point2I blockPos;
    LegoObjGE_Point2F offsets[4];

    if (obj->type == 2 && target->type == 4) {
        legoobject_position_to_terrain_cell(target, &blockPos.x, &blockPos.y);
        if (legoobject_nearest_connected_cell_route_select(obj, blockPos.x, blockPos.y, 0, &dir, 1)) {
            scale = 4.0f / terrain_context_scale_get(Lego_GetMap());
            jitter = random_float_range(0.1f, 0.9f);
            offsets[0].x = jitter;
            offsets[0].y = 1.0f - scale;
            offsets[1].x = 1.0f - scale;
            offsets[1].y = jitter;
            offsets[2].x = jitter;
            offsets[2].y = scale;
            offsets[3].x = scale;
            offsets[3].y = jitter;
            dir = (dir - 2) & 3;
            *(LegoObjGE_Point2F*)LEGOOBJ_R_LAST_ROUTE(obj).blockOffset = offsets[dir];
            if (marker)
                LEGOOBJ_R_LAST_ROUTE(obj).actionFlags = 0xf;
            else
                LEGOOBJ_R_LAST_ROUTE(obj).actionFlags = 0xe;
            LEGOOBJ_R_LAST_ROUTE(obj).flags |= 4;
            LEGOOBJ_R_LAST_ROUTE(obj).flags |= dir & 3;
            obj->routeToObject = target;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00449010
int legoobject_kind6_record_approach_route_issue(Lego_Object* obj)
{
    int blockPos[2];

    if (obj->type == 2 && (obj->flags1 & 0x400) && obj->carriedObjects[0]->type == 6 &&
        (obj->carriedObjects[0]->flags3 & 0x80000000) &&
        legoobject_type11_cell_coordinate_nearest_resolve(obj, blockPos) &&
        legoobject_blocked_cell_approach_weight_write(obj, blockPos[0], blockPos[1], 3.0f)) {
        LEGOOBJ_R_LAST_ROUTE(obj).actionFlags = 0x12;
        obj->targetBlockPos[0] = (float)blockPos[0];
        obj->targetBlockPos[1] = (float)blockPos[1];
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x004490a0
int legoobject_proximity_step_command_issue_with_link(Lego_Object* obj, Lego_Object* target, int mode, int setGate)
{
    int centre;
    unsigned int dir;
    float scale;
    float jitter;
    LegoObjF_Point2I blockPos;
    LegoObjGE_Point2F offsets[4];

    if ((obj->type == 2 || obj->type == 3) && target->type == 4) {
        if (obj->type == 3 && !(entity_state_flags_get(LEGOOBJ_SLOT(obj)) & 0x80000))
            centre = 1;
        else
            centre = 0;
        legoobject_position_to_terrain_cell(target, &blockPos.x, &blockPos.y);
        if (legoobject_nearest_connected_cell_route_select(obj, blockPos.x, blockPos.y, 0, &dir, obj->type != 3)) {
            scale = 4.0f / terrain_context_scale_get(Lego_GetMap());
            if (centre)
                jitter = 0.5f;
            else
                jitter = random_float_range(0.2f, 0.8f);
            offsets[0].x = jitter;
            offsets[0].y = 1.0f - scale;
            offsets[1].x = 1.0f - scale;
            offsets[1].y = jitter;
            offsets[2].x = jitter;
            offsets[2].y = scale;
            offsets[3].x = scale;
            offsets[3].y = jitter;
            dir = (dir - 2) & 3;
            *(LegoObjGE_Point2F*)LEGOOBJ_R_LAST_ROUTE(obj).blockOffset = offsets[dir];
            LEGOOBJ_R_LAST_ROUTE(obj).actionFlags = 6;
            LEGOOBJ_R_LAST_ROUTE(obj).flags |= 4;
            LEGOOBJ_R_LAST_ROUTE(obj).flags |= dir & 3;
            if (mode)
                LEGOOBJ_R_LAST_ROUTE(obj).flags |= 0x20;
            if (setGate)
                obj->flags4 |= 0x400000;
            obj->routeToObject = target;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00449240
int legoobject_walkable_target_cell_route_issue(Lego_Object* obj, int* blockPos)
{
    if (lego_terrain_cell_allows_placement(LEGOOBJ_SLOT(obj), blockPos[0], blockPos[1]) &&
        legoobject_blocked_cell_approach_weight_write(obj, blockPos[0], blockPos[1], 3.0f)) {
        LEGOOBJ_R_LAST_ROUTE(obj).actionFlags = 2;
        obj->targetBlockPos[0] = (float)blockPos[0];
        obj->targetBlockPos[1] = (float)blockPos[1];
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x004492b0
int legoobject_cell_step_command_issue_with_target_mark(Lego_Object* obj, int* blockPos)
{
    int result;
    unsigned int* routeX;
    unsigned int* routeY;
    unsigned int count;
    LegoObjF_Point2I ownPos;

    if (terrain_cell_speed_class_current(blockPos)) {
        if (!(entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x10000))
            return 1;
        legoobject_position_to_terrain_cell(obj, &ownPos.x, &ownPos.y);
        if (ownPos.x == blockPos[0] && ownPos.y == blockPos[1]) {
            result = legoobject_route_setup_and_marker_associate(obj, 1,
                (unsigned int*)&blockPos[0], (unsigned int*)&blockPos[1], NULL);
        } else {
            if (legoobject_terrain_neighbor_path_check(obj, ownPos.x, ownPos.y, blockPos[0], blockPos[1],
                    &routeX, &routeY, &count, NULL, NULL)) {
                result = legoobject_route_setup_and_marker_associate(obj, count, routeX, routeY, NULL);
                free(routeX);
                free(routeY);
            } else
                return 0;
        }
        if (result) {
            LEGOOBJ_R_LAST_ROUTE(obj).actionFlags = 4;
            if (obj->type == 1) {
                terrain_cell_flag_80000_clear_or_set_current(blockPos, 1);
                terrain_cell_flag_10000000_set_current(blockPos, 1);
                return 1;
            }
        } else
            return 0;
    }
    return 1;
}

// FUNCTION: LEGORR 0x004493e0
int legoobject_has_task_type_4_5_7(Lego_Object* obj)
{
    if (obj->type == 2) {
        if (legoobject_activity_scan(obj, 4) || legoobject_activity_scan(obj, 5) ||
            legoobject_activity_scan(obj, 7))
            return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00449430
int legoobject_target_cell_route_and_associate(Lego_Object* obj, Lego_Object* target)
{
    int result;
    unsigned int* routeX;
    unsigned int* routeY;
    unsigned int count;
    LegoObjF_Point2I targetPos;
    LegoObjF_Point2I ownPos;

    legoobject_position_to_terrain_cell(target, &targetPos.x, &targetPos.y);
    legoobject_position_to_terrain_cell(obj, &ownPos.x, &ownPos.y);
    if (ownPos.x == targetPos.x && ownPos.y == targetPos.y) {
        result = legoobject_route_setup_and_marker_associate(obj, 1,
            (unsigned int*)&targetPos.x, (unsigned int*)&targetPos.y, NULL);
    } else {
        if (legoobject_terrain_neighbor_path_check(obj, ownPos.x, ownPos.y, targetPos.x, targetPos.y,
                &routeX, &routeY, &count, NULL, NULL)) {
            result = legoobject_route_setup_and_marker_associate(obj, count, routeX, routeY, NULL);
            free(routeX);
            free(routeY);
        } else
            return 0;
    }
    if (result) {
        LEGOOBJ_R_LAST_ROUTE(obj).actionFlags = 0x11;
        obj->routeToObject = target;
    }
    return result;
}

// FUNCTION: LEGORR 0x00449520
int legoobject_target_cell_route_marker14(Lego_Object* obj, Lego_Object* target)
{
    int result;
    unsigned int* routeX;
    unsigned int* routeY;
    unsigned int count;
    LegoObjF_Point2I targetPos;
    LegoObjF_Point2I ownPos;

    legoobject_position_to_terrain_cell(obj, &ownPos.x, &ownPos.y);
    legoobject_position_to_terrain_cell(target, &targetPos.x, &targetPos.y);
    if (ownPos.x == targetPos.x && ownPos.y == targetPos.y) {
        result = legoobject_route_setup_and_marker_associate(obj, 1,
            (unsigned int*)&targetPos.x, (unsigned int*)&targetPos.y, NULL);
    } else {
        if (legoobject_terrain_neighbor_path_check(obj, ownPos.x, ownPos.y, targetPos.x, targetPos.y,
                &routeX, &routeY, &count, NULL, NULL)) {
            result = legoobject_route_setup_and_marker_associate(obj, count, routeX, routeY, NULL);
            free(routeX);
            free(routeY);
        } else
            return 0;
    }
    if (result)
        LEGOOBJ_R_LAST_ROUTE(obj).actionFlags = 0x14;
    return result;
}

// FUNCTION: LEGORR 0x00449610
int legoobject_coordinate_route_and_mark(Lego_Object* obj, int* blockPos)
{
    int result;
    unsigned int* routeX;
    unsigned int* routeY;
    unsigned int count;
    LegoObjF_Point2I ownPos;

    legoobject_position_to_terrain_cell(obj, &ownPos.x, &ownPos.y);
    if (ownPos.x == blockPos[0] && ownPos.y == blockPos[1]) {
        result = legoobject_route_setup_and_marker_associate(obj, 1,
            (unsigned int*)&blockPos[0], (unsigned int*)&blockPos[1], NULL);
    } else {
        if (legoobject_terrain_neighbor_path_check(obj, ownPos.x, ownPos.y, blockPos[0], blockPos[1],
                &routeX, &routeY, &count, NULL, NULL)) {
            result = legoobject_route_setup_and_marker_associate(obj, count, routeX, routeY, NULL);
            free(routeX);
            free(routeY);
        } else
            return 0;
    }
    if (result) {
        LEGOOBJ_R_LAST_ROUTE(obj).actionFlags = 0x10;
        obj->targetBlockPos[0] = (float)blockPos[0];
        obj->targetBlockPos[1] = (float)blockPos[1];
    }
    return result;
}

// FUNCTION: LEGORR 0x00449700
int legoobject_route_step_command_issue(Lego_Object* obj)
{
    int result;
    unsigned int* routeX;
    unsigned int* routeY;
    unsigned int count;
    LegoObjF_Point2I blockPos;
    LegoObjF_Point2I ownPos;

    result = 0;
    if (entity_state_flags_get(LEGOOBJ_SLOT(obj)) & 0x100000) {
        if (legoobject_nearest_cell_coordinate_world_resolve(obj, &blockPos.x)) {
            legoobject_position_to_terrain_cell(obj, &ownPos.x, &ownPos.y);
            if (ownPos.x == blockPos.x && ownPos.y == blockPos.y) {
                result = legoobject_route_setup_and_marker_associate(obj, 1,
                    (unsigned int*)&blockPos.x, (unsigned int*)&blockPos.y, NULL);
            } else if (legoobject_terrain_neighbor_path_check(obj, ownPos.x, ownPos.y, blockPos.x, blockPos.y,
                           &routeX, &routeY, &count, NULL, NULL)) {
                result = legoobject_route_setup_and_marker_associate(obj, count, routeX, routeY, NULL);
                free(routeX);
                free(routeY);
            } else
                result = 0;
            if (result) {
                LEGOOBJ_R_LAST_ROUTE(obj).actionFlags = 10;
                obj->targetBlockPos[0] = (float)blockPos.x;
                obj->targetBlockPos[1] = (float)blockPos.y;
            }
        }
    } else if (legoobject_nearest_open_cell_coord_search(obj, &blockPos.x, &blockPos.y, 1, 0, 1) &&
               legoobject_blocked_cell_approach_weight_write(obj, blockPos.x, blockPos.y, 3.0f)) {
        LEGOOBJ_R_LAST_ROUTE(obj).actionFlags = 10;
        obj->targetBlockPos[0] = (float)blockPos.x;
        obj->targetBlockPos[1] = (float)blockPos.y;
        result = 1;
    }
    if (result)
        argument_callback_dispatch(obj);
    return result;
}

// FUNCTION: LEGORR 0x00449880
int legoobject_blocked_cell_approach_weight_write(Lego_Object* obj, unsigned int bx, unsigned int by, float amount)
{
    unsigned int dir;
    float offset;

    if (terrain_cell_walkable_flag_current(bx, by) &&
        legoobject_nearest_connected_cell_route_select(obj, bx, by, 0, &dir, 1)) {
        offset = amount / terrain_context_scale_get(Lego_GetMap());
        LEGOOBJ_R_LAST_ROUTE(obj).blockOffset[1] = 0.5f;
        LEGOOBJ_R_LAST_ROUTE(obj).blockOffset[0] = 0.5f;
        switch (dir) {
        case 0:
            LEGOOBJ_R_LAST_ROUTE(obj).blockOffset[1] = offset;
            break;
        case 1:
            LEGOOBJ_R_LAST_ROUTE(obj).blockOffset[0] = offset;
            break;
        case 2:
            LEGOOBJ_R_LAST_ROUTE(obj).blockOffset[1] = 1.0f - offset;
            break;
        case 3:
            LEGOOBJ_R_LAST_ROUTE(obj).blockOffset[0] = 1.0f - offset;
            break;
        }
        LEGOOBJ_R_LAST_ROUTE(obj).flags |= 8;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00449980
int legoobject_activity_terrain_eligibility_transition(Lego_Object* obj, float elapsed, int* out)
{
    LegoObjF_Point2I blockPos;

    *out = 0;
    if (!(obj->flags1 & 0x40))
        return 0;
    blockPos.x = (int)obj->targetBlockPos[0];
    blockPos.y = (int)obj->targetBlockPos[1];
    if (!lego_terrain_cell_allows_placement(LEGOOBJ_SLOT(obj), blockPos.x, blockPos.y)) {
        legoobject_set_activity(obj, 0xb, 1);
        obj->flags1 = (obj->flags1 & ~0x40) | 0x80000000;
    } else {
        if (obj->animTime <= legoobject_zero)
            return 1;
        obj->flags1 &= ~0x40;
        legoobject_set_activity(obj, 0, 0);
        lego_terrain_cell_queued_work_transition(blockPos.x, blockPos.y);
        obj->targetBlockPos[1] = -1.0f;
        obj->targetBlockPos[0] = -1.0f;
        *out = 1;
    }
    level_tile_mark_current_target(&blockPos.x, 0);
    return 1;
}

// FUNCTION: LEGORR 0x00449a70
void legoobject_activity23_state_transition(Lego_Object* obj)
{
    if (obj->type == 2)
        obj->flags2 |= 0x800;
    if (!(obj->flags1 & 0x400)) {
        obj->flags1 |= 0x40000000;
        legoobject_set_activity(obj, 0x23, 0);
        legoobject_activity_register_bookkeeping(obj);
        aitask_ai_idle_chain_record_attach(LEGOOBJ_SLOT(obj));
    }
}

// FUNCTION: LEGORR 0x00449ac0
int legoobject_proximity_step_command_issue_clearing_gate(Lego_Object* obj, Lego_Object* target)
{
    unsigned int dir;
    float scale;
    LegoObjF_Point2I blockPos;
    LegoObjGE_Point2F offsets[4];

    if (obj->type == 2 && target->type == 4) {
        legoobject_position_to_terrain_cell(target, &blockPos.x, &blockPos.y);
        if (legoobject_nearest_connected_cell_route_select(obj, blockPos.x, blockPos.y, 0, &dir, 1)) {
            scale = 4.0f / terrain_context_scale_get(Lego_GetMap());
            offsets[0].x = 0.5f;
            offsets[0].y = 1.0f - scale;
            offsets[1].x = 1.0f - scale;
            offsets[1].y = 0.5f;
            offsets[2].x = 0.5f;
            offsets[2].y = scale;
            offsets[3].x = scale;
            offsets[3].y = 0.5f;
            dir = (dir - 2) & 3;
            *(LegoObjGE_Point2F*)LEGOOBJ_R_LAST_ROUTE(obj).blockOffset = offsets[dir];
            LEGOOBJ_R_LAST_ROUTE(obj).actionFlags = 0xb;
            LEGOOBJ_R_LAST_ROUTE(obj).flags |= 4;
            LEGOOBJ_R_LAST_ROUTE(obj).flags |= dir & 3;
            obj->routeToObject = target;
            return 1;
        }
        obj->flags2 &= ~0x800;
    }
    return 0;
}

/* ---- state and imports, group S (00449c00-0044ae50; part 3, issue #615) ---- */

/* Lego.c (lego_unit.c, C linkage). */
int lego_grid_cell_entity_eligibility_check(int bx, int by);           /* 00433e30 */
int terrain_cell_mask18_clear_current(unsigned int bx, unsigned int by); /* 00432f40 */
void media_action_feedback_sound_dispatch(int event);                  /* 00428800 */
extern int legoGlobs_f_556448;                    /* 00556448 placement direction */
/* Steering (steering.c, C): the follower stack of a flock. */
void ptrstack_for_each(void* group, void (*callback)(void* group, void* item, void* data),
                       void* data);                                     /* 0040fe90 */
/* Creature.c and Vehicle.c (bezier.c, animation_object.c, C). */
int scene_object_mode_is_one(const void* scene_object);                 /* 00406980 */
void animation_object_visibility_set(const void* object, unsigned int hide); /* 00406c90 */
void scene_object_pair_apply_mode(const void* record, uint32_t flag);   /* 00408670 */
void scene_resource_visibility_set(void* record, uint32_t flag);       /* 0046d2c0 */
/* PORT-ONLY adapters at the end of legoobject_port.cpp: the port's entries
 * have C++ linkage. */
void legoobject_gs_container_hidden_set_adapter(void* cont, int hide);  /* 00473b20 */
int legoobject_gs_container_is_hidden_adapter(void* cont);              /* 00473ba0 */
/* vehicle_model_link_names.h's matching-build link names of 0046d330 and
 * 0046da90. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define vehicle_is_hidden entity_type_flag_bit1_get_target
#define vehicle_get_drill_null scene_resource_cached_slot_b4_get
#endif
unsigned int vehicle_is_hidden(struct VehicleModel* vehicle);             /* 0046d330 */
int legoobject_gs_object_is_hidden_adapter(void* model);                /* 004086a0 */
int legoobject_gs_vehicle_camera_flip_adapter(void* vehicle);           /* 0046c3d0 */
void* vehicle_get_drill_null(struct VehicleModel* vehicle);          /* 0046da90 */
/* MeshLOD.c (terrain_map.c, C). */
void* model_entry_record_load(void* previous, const char* partName, const char* dir,
                              const char* meshName, unsigned int setID); /* 004519b0 */
void* model_primitive_record_create(void* previous, const char* name, unsigned int setID); /* 00451ab0 */
extern uint32_t s_engine_feature_word;            /* 00505f84 mainGlobs.flags */
/* Building placement: Pointer.c, Interface.c, Advisor.c, Stats.c (C). */
void media_pointer_cursor_select(int slot);                             /* 0045cc20 */
void selection_clear_request_and_hud_reset(void);                       /* 0041e750 */
void surface_visibility_set(const void* record, uint32_t flag);         /* 00464720 */
void cmd_operation_group_append_object(uint32_t key, struct EntitySlot* entity); /* 00408ce0 */
uint32_t entity_indexed_record_word_at_b0_get(int type, int id, int level); /* 00469ac0 */
uint32_t stats_table_word_at_b8(int type, int id, int level);           /* 00469b20 */
uint32_t stats_table_word_at_d0(int type, int id, int level);           /* 00469f60 */
/* A PORT-ONLY adapter at the end of legoobject_port.cpp: Building.c's
 * entry has C++ linkage and another contract. */
int* legoobject_gs_placement_footprint_check_adapter(void* surface, int* blockPos, int* shape,
                                                    unsigned int count, int direction,
                                                    void* map, int waterEntrances); /* 004641c0 */
/* Construction.c (construction.c, construction.h). */
unsigned int construction_zone_start_building(int objID, int* blockPos,
                                              unsigned int direction,
                                              int* shapePoints,
                                              unsigned int shapeCount); /* 00409790 */
void construction_spawn_barriers(int* blockPos, int* shapePoints,
                                 unsigned int shapeCount);          /* 00408ec0 */
void construction_zone_request_resource(int* blockPos, int objType,
                                        int objID, unsigned int objLevel,
                                        unsigned int count);        /* 00408e50 */

/* Lego_Level's block grid size. */
typedef struct LegoObjS_Level {
    unsigned char pad_00[0x14];
    unsigned int width;               /* +0x14 */
    unsigned int height;              /* +0x18 */
} LegoObjS_Level;

/* Map3D's block size (Map3D +0x10). */
typedef struct LegoObjS_Map {
    unsigned char pad_00[0x10];
    float blockSize;                  /* +0x10 */
} LegoObjS_Map;

/* The record 0044a5d0 sorts (a block and its distance). */
typedef struct LegoObjS_CellDistance {
    float x;
    float y;
    float distance;
} LegoObjS_CellDistance;

/* ---- functions, group S (00449c00-0044ae50; part 3, issue #615) ---- */

// FUNCTION: LEGORR 0x00449c00
void legoobject_simtick_render_step(void)
{
    legoobject_run_through_lists(legoobject_hide_check_callback, NULL, 0);
}

/* A flock member's container is shown on uncovered, eligible blocks only. */
// FUNCTION: LEGORR 0x00449c20
void legoobject_render_step_callback(void* group, void* item, void* data)
{
    void* cont;
    float wPos[3];
    int bx;
    int by;

    cont = LEGOOBJ_FIELD(item, void*, 0x98);
    ddiscene_object_position_get(cont, NULL, wPos);
    terrain_map_world_to_block(Lego_GetMap(), wPos[0], wPos[1], &bx, &by);
    if ((unsigned int)bx <= ((LegoObjS_Level*)level_current_owner_get())->width &&
        (unsigned int)by <= ((LegoObjS_Level*)level_current_owner_get())->height &&
        !lego_grid_cell_entity_eligibility_check(bx, by) &&
        !terrain_cell_mask18_clear_current(bx, by)) {
        scene_render_lock_toggle(cont, 0);
        return;
    }
    scene_render_lock_toggle(cont, 1);
}

// FUNCTION: LEGORR 0x00449cd0
void legoobject_scene_object_state_handle_apply(Lego_Object* obj, int hide)
{
    void* cont;

    cont = legoobject_type_component_get(obj);
    if (cont != NULL)
        legoobject_gs_container_hidden_set_adapter(cont, hide);
}

/* The RunThroughLists callback of 00449c00: hide the object unless its
 * blocks are visible. */
// FUNCTION: LEGORR 0x00449cf0
int legoobject_hide_check_callback(Lego_Object* obj, void* data)
{
    int hide;
    LegoObjF_Point2I* shape;
    unsigned int count;
    unsigned int i;
    int skip;
    unsigned int dir;
    int bx;
    int by;
    LegoObjF_Point2I shapePos;
    float faceDir[2];
    float wPos[2];

    hide = 0;
    if (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x200000) {
        ptrstack_for_each((void*)(uintptr_t)obj->field_32c, legoobject_render_step_callback, NULL);
        return 0;
    }
    /* 004424f0 returns the block lookup's result in the original; group
     * M's prototype says void, so it is called through the original's type. */
    if (!((int (*)(Lego_Object*, int*, int*))legoobject_position_to_terrain_cell)(obj, &bx, &by)) {
        legoobject_type_operation_dispatch(obj, 1);
        return 0;
    }
    if (terrain_cell_flag_8_bounded_current(bx, by))
        obj->flags3 &= ~0x100000;
    if (!terrain_cell_mask18_clear_current(bx, by) && !(obj->flags3 & 0x100000) &&
        !(obj->type == 2 && obj->driveObject != NULL &&
          (entity_state_flags_get(LEGOOBJ_SLOT(obj->driveObject)) & 0x40000000)))
        legoobject_type_operation_dispatch(obj, 0);
    else
        legoobject_type_operation_dispatch(obj, 1);

    if (lego_grid_cell_entity_eligibility_check(bx, by) ||
        terrain_cell_mask18_clear_current(bx, by) || (obj->flags3 & 0x100000)) {
        hide = 1;
        if (obj->type == 4) {
            legoobject_surface_vector_normalize(obj, faceDir);
            legoobject_position_to_terrain_cell(obj, &shapePos.x, &shapePos.y);
            legoobject_transform_position_get(obj, &wPos[0], &wPos[1]);
            if (fabs(faceDir[0]) > fabs(faceDir[1])) {
                if (faceDir[0] > legoobject_zero)
                    dir = 1;
                else
                    dir = 3;
            } else {
                if (faceDir[1] > legoobject_zero)
                    dir = 0;
                else
                    dir = 2;
            }
            shape = (LegoObjF_Point2I*)task_field84_get_and_field88_out((char*)legoGlobs_buildingData + obj->id * 0x14c, &count);
            shape = (LegoObjF_Point2I*)rotated_pairs_scratch_fill(&shapePos.x, (int*)shape, count, dir);
            for (i = 0; i < count; i++) {
                if (i != count - 1 && shape[i].x == shape[i + 1].x &&
                    shape[i].y == shape[i + 1].y) {
                    skip = 1;
                } else {
                    skip = 0;
                    if (!lego_grid_cell_entity_eligibility_check(shape[i].x, shape[i].y)) {
                        hide = 0;
                        break;
                    }
                }
                if (skip)
                    i++;
            }
        }
    }
    legoobject_scene_object_state_handle_apply(obj, hide);
    return 0;
}

// FUNCTION: LEGORR 0x00449f50
void legoobject_type_operation_dispatch(Lego_Object* obj, int hide)
{
    if (obj->type == 1) {
        scene_resource_visibility_set(obj->vehicle, hide);
        return;
    }
    if (obj->type == 2) {
        animation_object_visibility_set(obj->miniFigure, hide);
        return;
    }
    if (obj->type == 3) {
        animation_object_visibility_set(obj->rockMonster, hide);
        return;
    }
    if (obj->type == 4) {
        scene_object_pair_apply_mode(obj->building, hide);
        return;
    }
    if (obj->type == 10) {
        animation_object_visibility_set(obj->upgradePart, hide);
        return;
    }
    if (obj->flags3 & 0x20000)
        scene_render_lock_toggle(obj->other, hide);
}

// FUNCTION: LEGORR 0x00449ff0
int legoobject_type_operation_release(Lego_Object* obj)
{
    if (obj->type == 1)
        return vehicle_is_hidden((struct VehicleModel*)obj->vehicle);
    if (obj->type == 2)
        return legoobject_gs_object_is_hidden_adapter(obj->miniFigure);
    if (obj->type == 3)
        return legoobject_gs_object_is_hidden_adapter(obj->rockMonster);
    if (obj->type == 4)
        return legoobject_gs_object_is_hidden_adapter(obj->building);
    if (obj->type == 10)
        return legoobject_gs_object_is_hidden_adapter(obj->upgradePart);
    if (obj->flags3 & 0x20000)
        return legoobject_gs_container_is_hidden_adapter(obj->other);
}

// FUNCTION: LEGORR 0x0044a070
void legoobject_follow_target_pose_get(Lego_Object* obj, int cameraNo, float* pos, float* dir)
{
    int flip;
    LegoObjJ_Vector3F up;

    if (obj->field_2dc == NULL || obj->field_2e0 != cameraNo) {
        if (obj->type == 1) {
            if ((obj->field_2dc = vehicle_get_camera_null((struct VehicleModel*)obj->vehicle, cameraNo)) != NULL)
                obj->field_2e0 = cameraNo;
        } else if (obj->type == 2) {
            if ((obj->field_2dc = activity_runner_state_read(obj->miniFigure, cameraNo)) != NULL)
                obj->field_2e0 = cameraNo;
        } else if (obj->type == 3) {
            if ((obj->field_2dc = activity_runner_state_read(obj->rockMonster, cameraNo)) != NULL)
                obj->field_2e0 = cameraNo;
        }
    }
    if (obj->type == 1)
        flip = legoobject_gs_vehicle_camera_flip_adapter(obj->vehicle);
    else if (obj->type == 2)
        flip = scene_object_mode_is_one(obj->miniFigure);
    else if (obj->type == 3)
        flip = scene_object_mode_is_one(obj->rockMonster);
    if (obj->field_2dc != NULL) {
        ddiscene_object_position_get(obj->field_2dc, NULL, pos);
        ddiscene_object_facing_pair_get(obj->field_2dc, NULL, dir, &up.x);
        legoobject_o_vector3_normalise((LegoObjJ_Vector3F*)dir);
        if (flip)
            legoobject_o_vector3_scale((LegoObjJ_Vector3F*)dir, (LegoObjJ_Vector3F*)dir, -1.0f);
    }
}

// FUNCTION: LEGORR 0x0044a200
void* legoobject_type_component_get(Lego_Object* obj)
{
    switch (obj->type) {
    case 1:
        return (void*)scene_object_resource_get(obj->vehicle);
    case 2:
        return animation_object_handle_get(obj->miniFigure);
    case 3:
        return animation_object_handle_get(obj->rockMonster);
    case 4:
        return animation_object_handle_get(obj->building);
    case 10:
        return animation_object_handle_get(obj->upgradePart);
    }
    if (obj->flags3 & 0x20000)
        return obj->other;
    return obj;
}

// FUNCTION: LEGORR 0x0044a2a0
int legoobject_work_object_position_get(Lego_Object* obj, float* outX, float* outY)
{
    void* null;
    float wPos[3];

    if (obj->type == 1)
        null = vehicle_get_drill_null((struct VehicleModel*)obj->vehicle);
    else if (obj->type == 2)
        null = renderer_cached_handle_40_get_or_create(obj->miniFigure);
    else if (obj->type == 3)
        null = renderer_cached_handle_40_get_or_create(obj->rockMonster);
    else
        return 0;
    if (null != NULL) {
        ddiscene_object_position_get(null, NULL, wPos);
        *outX = wPos[0];
        *outY = wPos[1];
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0044a310
void legoobject_movement_input_accumulate(Lego_Object* obj, int forward, int turn, float elapsed)
{
    if (obj->type == 2 && obj->driveObject != NULL)
        obj = obj->driveObject;
    if (obj->flags1 & 1)
        legoobject_action_sequence_reset(obj, 0);
    if (!(entity_capability_flags_get(LEGOOBJ_SLOT(obj)) & 4))
        turn = 0;
    obj->field_2bc += turn;
    obj->field_2c0 += forward;
    obj->field_2c4 += elapsed;
}

/* Deviation: naked asm. The original reloads the count after each store
 * (objectGlobs' array and count share one struct, so MSVC assumes the
 * indexed store may alias the count); with the two separate globals the
 * port keeps, MSVC 5.0 caches the count. C forms tried: [count][0..1]
 * indexing and a Point2I cast of the array; both cache the count. */
// FUNCTION: LEGORR 0x0044a390
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void legoobject_type11_cell_coordinate_append(int* blockPos)
{
    __asm {
        mov ecx, dword ptr [s_type11_cell_coordinate_count]
        cmp ecx, 0ah
        jae type11_append_return
        mov eax, dword ptr [esp + 4]
        mov edx, dword ptr [eax]
        mov dword ptr [ecx*8 + s_type11_cell_coordinates], edx
        mov ecx, dword ptr [s_type11_cell_coordinate_count]
        mov eax, dword ptr [eax + 4]
        mov dword ptr [ecx*8 + s_type11_cell_coordinates + 4], eax
        mov eax, dword ptr [s_type11_cell_coordinate_count]
        inc eax
        mov dword ptr [s_type11_cell_coordinate_count], eax
    type11_append_return:
        ret
    }
}
#else
void legoobject_type11_cell_coordinate_append(int* blockPos)
{
    if (s_type11_cell_coordinate_count < 10) {
        s_type11_cell_coordinates[s_type11_cell_coordinate_count][0] = blockPos[0];
        s_type11_cell_coordinates[s_type11_cell_coordinate_count][1] = blockPos[1];
        s_type11_cell_coordinate_count++;
    }
}
#endif

// FUNCTION: LEGORR 0x0044a3d0
int legoobject_type11_cell_coordinate_nearest_resolve(Lego_Object* obj, int* outBlockPos)
{
    unsigned int best;
    unsigned int i;
    float bestDist;
    float dist;
    LegoObjM_Point2F pos;
    LegoObjM_Point2F wPos;

    best = (unsigned int)-1;
    bestDist = 10000.0f;
    legoobject_transform_position_get(obj, &pos.x, &pos.y);
    for (i = 0; i < s_type11_cell_coordinate_count; i++) {
        terrain_map_block_to_world(Lego_GetMap(), s_type11_cell_coordinates[i][0],
                                   s_type11_cell_coordinates[i][1], &wPos.x, &wPos.y);
        dist = (float)sqrt((pos.x - wPos.x) * (pos.x - wPos.x) + (pos.y - wPos.y) * (pos.y - wPos.y));
        if (dist < bestDist) {
            bestDist = dist;
            best = i;
        }
    }
    if (best != (unsigned int)-1) {
        if (outBlockPos != NULL) {
            outBlockPos[0] = s_type11_cell_coordinates[best][0];
            outBlockPos[1] = s_type11_cell_coordinates[best][1];
        }
        return 1;
    }
    return 0;
}

/* Deviation: naked asm, as 0044a390 (the count reloads after each store). */
// FUNCTION: LEGORR 0x0044a4b0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void legoobject_predug_coordinate_pool_append(int* blockPos)
{
    __asm {
        mov ecx, dword ptr [s_predug_coordinate_count]
        cmp ecx, 14h
        jae predug_append_return
        mov eax, dword ptr [esp + 4]
        mov edx, dword ptr [eax]
        mov dword ptr [ecx*8 + s_predug_coordinates], edx
        mov ecx, dword ptr [s_predug_coordinate_count]
        mov eax, dword ptr [eax + 4]
        mov dword ptr [ecx*8 + s_predug_coordinates + 4], eax
        mov eax, dword ptr [s_predug_coordinate_count]
        inc eax
        mov dword ptr [s_predug_coordinate_count], eax
    predug_append_return:
        ret
    }
}
#else
void legoobject_predug_coordinate_pool_append(int* blockPos)
{
    if (s_predug_coordinate_count < 20) {
        s_predug_coordinates[s_predug_coordinate_count][0] = blockPos[0];
        s_predug_coordinates[s_predug_coordinate_count][1] = blockPos[1];
        s_predug_coordinate_count++;
    }
}
#endif

// FUNCTION: LEGORR 0x0044a4f0
int legoobject_nearest_cell_coordinate_world_resolve(Lego_Object* obj, int* outBlockPos)
{
    unsigned int best;
    unsigned int i;
    float bestDist;
    float dist;
    LegoObjM_Point2F pos;
    LegoObjM_Point2F wPos;

    best = (unsigned int)-1;
    bestDist = 10000.0f;
    legoobject_transform_position_get(obj, &pos.x, &pos.y);
    for (i = 0; i < s_predug_coordinate_count; i++) {
        terrain_map_block_to_world(Lego_GetMap(), s_predug_coordinates[i][0],
                                   s_predug_coordinates[i][1], &wPos.x, &wPos.y);
        dist = (float)sqrt((pos.x - wPos.x) * (pos.x - wPos.x) + (pos.y - wPos.y) * (pos.y - wPos.y));
        if (dist < bestDist) {
            bestDist = dist;
            best = i;
        }
    }
    if (best != (unsigned int)-1) {
        if (outBlockPos != NULL) {
            outBlockPos[0] = s_predug_coordinates[best][0];
            outBlockPos[1] = s_predug_coordinates[best][1];
        }
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0044a5d0
int legoobject_nearest_open_cell_coord_search(Lego_Object* obj, int* outX, int* outY, int useMinDist, int allowC0, int allow20)
{
    struct TerrainMap* map;
    LegoObjF_Point2I cur;
    LegoObjF_Point2I blockPos;
    unsigned int count;
    float dist;
    LegoObjM_Point2F pos;
    LegoObjM_Point2F wPos;
    LegoObjS_CellDistance list[100];

    map = Lego_GetMap();
    count = 0;
    legoobject_transform_position_get(obj, &pos.x, &pos.y);
    legoobject_position_to_terrain_cell(obj, &blockPos.x, &blockPos.y);
    for (cur.y = blockPos.y - 10; cur.y < blockPos.y + 10; cur.y++) {
        for (cur.x = blockPos.x - 10; cur.x < blockPos.x + 10; cur.x++) {
            if (terrain_cell_walkable_flag_current(cur.x, cur.y) &&
                (allow20 || !terrain_cell_flag_20_bounded_current(cur.x, cur.y)) &&
                (allowC0 || !terrain_cell_mask_c0_current(cur.x, cur.y))) {
                terrain_map_block_to_world(map, cur.x, cur.y, &wPos.x, &wPos.y);
                dist = (float)sqrt((wPos.x - pos.x) * (wPos.x - pos.x) + (wPos.y - pos.y) * (wPos.y - pos.y));
                if ((!useMinDist || dist >= ((LegoObjS_Map*)map)->blockSize) && count < 100) {
                    list[count].x = (float)cur.x;
                    list[count].y = (float)cur.y;
                    list[count].distance = dist;
                    count++;
                }
            }
        }
    }
    if (count != 0) {
        qsort(list, count, sizeof(LegoObjS_CellDistance), legoobject_open_cell_coord_search_callback);
        *outX = (int)list[0].x;
        *outY = (int)list[0].y;
        return 1;
    }
    return 0;
}

/* The qsort comparator of 0044a5d0: nearest first. */
// FUNCTION: LEGORR 0x0044a7a0
int legoobject_open_cell_coord_search_callback(const void* a, const void* b)
{
    if (((LegoObjS_CellDistance*)a)->distance > ((LegoObjS_CellDistance*)b)->distance)
        return 1;
    if (((LegoObjS_CellDistance*)a)->distance < ((LegoObjS_CellDistance*)b)->distance)
        return -1;
    return 0;
}

// FUNCTION: LEGORR 0x0044a7d0
void* legoobject_poly_variant_section_model_build(void* config, const char* gameName, const char* dir, int polyKind, unsigned int cameraCount)
{
    unsigned int i;
    void* model;
    void* first;
    struct BlockRec* item;
    const char* name;
    char buffer[256];

    model = NULL;
    first = NULL;
    if (polyKind != 3)
        cameraCount = 1;
    for (i = 0; i < cameraCount; i++) {
        if (polyKind == 3)
            // STRING: LEGORR 0x004a5140
            sprintf(buffer, "FPPoly::Camera%i", i + 1);
        else if (polyKind == 2)
            // STRING: LEGORR 0x004a5134
            sprintf(buffer, "HighPoly");
        else if (polyKind == 1)
            // STRING: LEGORR 0x004a5128
            sprintf(buffer, "MediumPoly");
        for (item = block_subgroup((struct BlockRec*)config, string_path_join(gameName, buffer, NULL));
             item != NULL; item = block_next_equal_key(item)) {
            name = item->key;
            if (*name != '!' || !(s_engine_feature_word & 0x800000)) {
                if (*name == '!')
                    name++;
                if (_strcmpi("NULL", item->value) == 0)
                    model = model_primitive_record_create(model, name, i);
                else
                    model = model_entry_record_load(model, name, dir, item->value, i);
                if (first == NULL)
                    first = model;
            }
        }
    }
    return first;
}

// FUNCTION: LEGORR 0x0044a910
int legoobject_building_placement_preview_update(int objType, int objID, int clicked, int toolTipOnly, float* mousePos, int bx, int by, int active, void* surface)
{
    int* shape;
    int* placed;
    unsigned int count;
    int waterEntrances;
    unsigned int i;
    uint32_t group;
    Lego_Object* teleport;
    uint32_t cost;
    LegoObjF_Point2I blockPos;
    LegoObjM_Point2F delta;
    LegoObjM_Point2F wPos;

    blockPos.x = bx;
    blockPos.y = by;
    if (objType == 4) {
        if (active) {
            if (terrain_cell_flag_8_bounded_current(bx, by)) {
                shape = (int*)task_field84_get_and_field88_out((char*)legoGlobs_buildingData + objID * 0x14c, &count);
                waterEntrances = stats_table_word_at_d0(4, objID, 0);
                surface_visibility_set(surface, 0);
                terrain_map_block_to_world(Lego_GetMap(), bx, by, &wPos.x, &wPos.y);
                delta.x = mousePos[0] - wPos.x;
                delta.y = mousePos[1] - wPos.y;
                if (delta.x > legoobject_zero && (float)fabs(delta.y) < delta.x)
                    legoGlobs_f_556448 = 1;
                else if (delta.x < legoobject_zero && (float)fabs(delta.y) < (float)fabs(delta.x))
                    legoGlobs_f_556448 = 3;
                else if (delta.y > legoobject_zero && (float)fabs(delta.x) < delta.y)
                    legoGlobs_f_556448 = 0;
                else if (delta.y < legoobject_zero && (float)fabs(delta.x) < (float)fabs(delta.y))
                    legoGlobs_f_556448 = 2;
                for (i = 0; i < 5; i++) {
                    placed = legoobject_gs_placement_footprint_check_adapter(surface, &blockPos.x, shape, count,
                        (i + legoGlobs_f_556448) & 3, LEGOOBJ_FIELD(g_active_level_retail, void*, 0x4),
                        waterEntrances);
                    if (placed != NULL) {
                        if (!toolTipOnly) {
                            media_pointer_cursor_select(0x16);
                            if (clicked) {
                                group = construction_zone_start_building(objID, &blockPos.x,
                                    (i + legoGlobs_f_556448) & 3, placed, count);
                                teleport = legoobject_query_center_mask02(NULL, &blockPos.x, 7, 0);
                                cost = stats_table_word_at_b8(4, objID, 0);
                                if (!(stats_table_word_at_144(4, objID) & 8))
                                    construction_spawn_barriers(&blockPos.x, placed, count);
                                construction_zone_request_resource(&blockPos.x, 6, 0, 0,
                                    entity_indexed_record_word_at_b4_get(4, objID, 0));
                                if (teleport != NULL && cost != 0)
                                    construction_zone_request_resource(&blockPos.x, 7, 1, 0, cost);
                                else
                                    construction_zone_request_resource(&blockPos.x, 7, 0, 0,
                                        entity_indexed_record_word_at_b0_get(4, objID, 0));
                                cmd_operation_group_append_object(group, NULL);
                                surface_visibility_set(surface, 1);
                                selection_clear_request_and_hud_reset();
                                media_action_feedback_sound_dispatch(0);
                                return 0;
                            }
                        }
                        legoGlobs_f_556448 = (i + legoGlobs_f_556448) & 3;
                        break;
                    }
                }
                if (i != 5)
                    return 1;
                if (!toolTipOnly) {
                    media_pointer_cursor_select(0x17);
                    if (clicked) {
                        media_action_feedback_sound_dispatch(1);
                        advisor_text_animation_update(0xf, 0, 0);
                        return 1;
                    }
                }
            } else {
                if (!toolTipOnly) {
                    media_pointer_cursor_select(0x17);
                    if (clicked)
                        media_action_feedback_sound_dispatch(1);
                }
                surface_visibility_set(surface, 1);
            }
            advisor_text_animation_update(0xf, 0, 0);
            return 1;
        }
        surface_visibility_set(surface, 1);
    }
    return 1;
}

// FUNCTION: LEGORR 0x0044acc0
void legoobject_sfx_grid_config_load(void* config, const char* gameName)
{
    struct BlockRec* item;
    int sfx;
    int x;
    int y;

    memset(s_terrain_sfx_grid, 0, sizeof(s_terrain_sfx_grid));
    for (x = 0; x < 20; x++) {
        for (y = 0; y < 15; y++) {
        }
    }
    // STRING: LEGORR 0x004a5154
    for (item = block_subgroup((struct BlockRec*)config, string_path_join(gameName, "ObjTtSFXs", NULL));
         item != NULL; item = block_next_equal_key(item)) {
        if (entity_type_pair_lookup(item->key, &x, &y, NULL) &&
            sound_alias_lookup(item->value, &sfx)) {
            ((int (*)[15])s_terrain_sfx_grid)[x][y] = sfx;
        }
    }
}

// FUNCTION: LEGORR 0x0044ad80
int legoobject_sfx_at_cell_get(Lego_Object* obj)
{
    unsigned int type;
    unsigned int id;

    legoobject_copy_two_dword_pair(obj, &type, &id);
    return ((int (*)[15])s_terrain_sfx_grid)[type][id];
}

// FUNCTION: LEGORR 0x0044adc0
void legoobject_scatter_spawn_gate_set(int on)
{
    if (on)
        s_entity_state_flags |= 0x20;
    else
        s_entity_state_flags &= ~0x20;
}

// FUNCTION: LEGORR 0x0044ade0
void legoobject_kind_one_random_spawn_trigger(Lego_Object* obj)
{
    Lego_Object* holder;

    holder = NULL;
    if (!(s_entity_state_flags & 0x20) && obj->type == 1 && !(obj->flags4 & 0x800)) {
        if (obj->flags4 & 0x40)
            holder = obj->routeToObject;
        legoobject_random_kind_six_scatter_spawn(holder != NULL ? holder : obj,
            entity_indexed_record_word_at_b4_get(1, obj->id, 0), holder != NULL);
        obj->flags4 |= 0x800;
    }
}

// FUNCTION: LEGORR 0x0044ae50
void legoobject_random_kind_six_scatter_spawn(Lego_Object* obj, unsigned int count, int fromBuilding)
{
    void* null;
    Lego_Object* crystal;
    LegoObjM_Point2F pos;
    LegoObjM_Point2F offset;
    LegoObjF_Point2I blockPos;
    float wPos[3];

    for (; count > 0; count--) {
        if (fromBuilding && obj->type == 4 &&
            (null = scene_object_secondary_handle_get_or_create(obj->building)) != NULL) {
            ddiscene_object_position_get(null, NULL, wPos);
            pos.x = wPos[0];
            pos.y = wPos[1];
        } else {
            legoobject_transform_position_get(obj, &pos.x, &pos.y);
        }
        offset.x = random_float_range(-5.0f, 5.0f);
        offset.y = random_float_range(-5.0f, 5.0f);
        if (terrain_map_world_to_block(Lego_GetMap(), offset.x + pos.x, offset.y + pos.y, &blockPos.x, &blockPos.y) &&
            terrain_cell_flag_8_bounded_current(blockPos.x, blockPos.y)) {
            pos.x = offset.x + pos.x;
            pos.y = offset.y + pos.y;
        }
        crystal = legoobject_create_in_world(legoGlobs_contCrystal, 6, 0, 0, pos.x, pos.y, 0.0f);
        cmd_create_task(LEGOOBJ_SLOT(crystal), 0);
    }
}

/* ---- state and imports, group T (0044af90-0044c5f0; part 3, issue #615) ---- */

/* Stats.c (entity.hpp, ai.hpp; entity_stats.c). */
int ai_activity_cap(const struct EntitySlot* e);                           /* 0046a060 */
int entity_grouped_code_table_find_value_index(uint32_t value, int* outTool); /* 0046a120 */
/* Interface.c (interface_unit.h). */
void interface_back_to_main_if_selected(struct LegoObject* obj);           /* 0041d010 */

/* Group T, 0044b680-0044c5f0. A flock (steering.c's SteeringGroup, 0x28
 * bytes) and its items (SteeringItem, 0xa0 bytes): the fields this unit
 * reads. */
typedef struct LegoObjT_FlockItem {
    float target[3];                  /* +0x00 */
    unsigned char pad_0c[0x24 - 0x0c];
    float yaw;                        /* +0x24 */
    float pitch;                      /* +0x28 */
    unsigned char pad_2c[0x58 - 0x2c];
    float transform[16];              /* +0x58 row 3 is the position */
    void* container;                  /* +0x98 */
    void* next;                       /* +0x9c */
} LegoObjT_FlockItem;
typedef struct LegoObjT_Flock {
    LegoObjT_FlockItem* leader;       /* +0x00 */
    unsigned char pad_04[0x14 - 0x04];
    float attackTime;                 /* +0x14 */
    unsigned char pad_18[0x24 - 0x18];
    unsigned int flags;               /* +0x24 bit 0: attacking */
} LegoObjT_Flock;
/* The data 0044b850 receives: the step and the object's flocks stats. */
typedef struct LegoObjT_FlockUpdate {
    float elapsed;
    float height;                     /* stats FlocksHeight */
    int tilt;                         /* stats flag 0x800000 */
    int onGround;                     /* stats flag 0x4000000 */
    int animate;                      /* stats flag 0x10000000 */
} LegoObjT_FlockUpdate;
/* objectGlobs' released-flock list (004eadd0; entity.hpp's layout). */
struct EntityAuxiliaryReleaseRecord {
    void* scene_object;               /* the flock */
    struct EntityAuxiliaryReleaseRecord* next;
    uint32_t flags;                   /* the stats flags */
    float value;                      /* stats FlocksHeight */
};
/* Steering (steering.c, C). */
void* scene_node_record_create(const float* position, const float* target, float turnStep,
                               float speed, float wanderRadius, float wanderPeriod,
                               void* container);                        /* 0040f6c0 */
void* callback_state_record_create(const float* position, const float* target,
                                   float turnStep, float speed, float wanderRadius,
                                   void* container);                    /* 0040f7d0 */
void callback_state_list_dispose(void* group);                          /* 0040f850 */
void three_value_record_copy_and_mark_ready(void* group, const float* target); /* 0040fdb0 */
void scene_object_list_update_transforms(void* group, float elapsed);   /* 0040fe00 */
void ptrstack_push(void* group, void* item);                            /* 0040fe40 */
void scene_object_list_apply_float_callback(void* group, float spread); /* 0040ff50 */
void object_vector3_set(void* group, float turnStep, float speed, float wanderRadius); /* 0040ff70 */
void global_vector_set_and_notify(void* group, float turnStep, float speed,
                                  float wanderRadius, float wanderPeriod); /* 0040ffe0 */
void scene_node_position_set(void* group, const float* position);       /* 00410020 */
void scene_object_list_apply_position_callback(void* group, const float* position); /* 00410070 */
void scene_object_list_apply_second_position_callback(void* group, const float* target); /* 004100b0 */
void steering_item_follow_target(void* group, void* item, void* data);  /* 004100d0 */
int callback_operation_run_and_return_status(void* group);              /* 00410280 */
/* Stats.c's flocks getters (entity_stats.c; entity_stats_link_names.h). */
float entity_stats_float_at130(const struct EntitySlot* e);             /* 0046a170 */
float entity_stats_float_at134(const struct EntitySlot* e);             /* 0046a190 */
float entity_stats_float_at120(const struct EntitySlot* e);             /* 0046a1b0 */
float entity_stats_float_at128(const struct EntitySlot* e);             /* 0046a1d0 */
float entity_stats_float_at124(const struct EntitySlot* e);             /* 0046a1f0 */
unsigned int entity_stats_word_at13c(const struct EntitySlot* e);       /* 0046a210 */
float entity_stats_float_at12c(const struct EntitySlot* e);             /* 0046a230 */
float entity_stats_float_at138(const struct EntitySlot* e);             /* 0046a250 */
/* G98 Container.c (view.hpp, scene_pool.hpp: C linkage). */
void* scene_root_object_get(void);                                      /* 00472940 */
void* ddiscene_object_allocate(const void* cont);                       /* 00472950 */
float ddiscene_advance_by_time_target(void* scene_object, float delta); /* 00475120 */
float ddiscene_object_length_get(void* scene_object);                   /* 00475390 */
int ddiscene_object_facing_pair_get(void* object, void* alternate_frame,
                                    float first[3], float second[3]);   /* 00475500 */
void ddiscene_scene_object_attach_or_unlink(void* pair_record, void* source_pair_record); /* 00475710 */
/* G98 Maths.c (maths.h, C). */
float* d3drm_vector_rotate(float* r, float* v, float* axis, float theta); /* 00479510 */
void matrix4_multiply(float* r, const float* a, const float* b);        /* 00479ce0 */
void matrix4_set_rotation_x(float* m, float t);                         /* 00479d50 */
void matrix4_copy(float* r, const float* m);                            /* 00479eb0 */
/* Map3D.c (terrain_map.h, C). */
float terrain_map_world_z_get(struct TerrainMap* map, float x, float y); /* 0044f940 */
void terrain_map_world_normal_get(struct TerrainMap* map, float x, float y,
                                  struct TerrainMapVector3* normal);   /* 0044fab0 */
int terrain_map_screen_pick(struct TerrainMap* map, void* viewport, unsigned int mx,
                            unsigned int my, unsigned int* bx, unsigned int* by,
                            struct TerrainMapVector3* intersection);   /* 004502e0 */
/* Lego.c (lego_unit.h, lego_unit.c). */
void lego_viewport_target_position_clamp(struct EntitySlot* liveObj, const int* blockPos,
                                         int smooth);                   /* 0042de40 */
extern void* legoGlobs_contIceCube;               /* 005563a8 */
extern void* legoGlobs_f_556108;                  /* 00556108 */
extern int s_object_f_4eabc4;                     /* 004eabc4 objectGlobs list count */
extern int s_object_f_4eabc8;                     /* 004eabc8 its buildings */
/* Mouse position (crt_mouse.h, crt.cpp). */
extern uint32_t s_renderer_word_a;                /* 0076ab24 mouse x */
extern uint32_t s_renderer_word_b;                /* 0076ab28 mouse y */
/* Reward.c (reward.h, reward.c). */
void global_counter_at_00555328_increment(void);                        /* 00462880 */
/* objectGlobs' list of the objects 0044c5f0 has visited (004ea7c4); its
 * count is 004eabc4. Its definition belongs with the unit's other state at
 * the end of this file (not yet added). */
extern Lego_Object* objectGlobs_f_4ea7c4[256];   /* 004ea7c4 */
/* PORT-ONLY adapters, original ABI (legoobject_port.cpp). 004897d0 is a
 * one-line getter (return ((int*)p)[3]) the linker folded with
 * Flic_GetWidth; 0044bc30 calls it on the flock (SteeringGroup.hasPosition).
 * 004756b0 (Container_SetMatrix) has C++ linkage in the port. */
int legoobject_gt_flock_has_position_adapter(void* flock);              /* 004897d0 */
void legoobject_gt_container_matrix_set_adapter(void* cont, uint32_t mode,
                                                const float* matrix);   /* 004756b0 */

/* ---- functions, group T (0044af90-0044c5f0; part 3, issue #615) ---- */

/* Spawns the scatter of creatures an object's spawn record (field_348)
 * names. */
// FUNCTION: LEGORR 0x0044af90
void legoobject_attached_spawn_record_process(Lego_Object* obj)
{
    if (obj->field_348) legoobject_random_kind_six_scatter_spawn(obj, *(int*)obj->field_348, 0);
}

/* A rock, lava or ice monster that breaks up leaves up to three tiny
 * monsters (TinyRM, TinyIM) that route away. */
// FUNCTION: LEGORR 0x0044afb0
void legoobject_tiny_monster_spawn_and_teardown(Lego_Object* obj)
{
    int found = 0;
    const char* name;
    int type;
    int id;
    void* model;
    float x;
    float y;
    unsigned int count;
    Lego_Object* tiny;

    if (entity_type_pair_lookup("RockMonster", &type, &id, NULL) && type == obj->type && id == obj->id) {
        found = 1;
        // STRING: LEGORR 0x004a5168
        name = "TinyRM";
    } else {
        name = (const char*)obj; /* the original's value when no monster matches */
    }
    if (!found) {
        found = 0;
        if (entity_type_pair_lookup("Lavamonster", &type, &id, NULL) && type == obj->type && id == obj->id) {
            name = "TinyRM";
            found = 1;
        }
        if (!found && entity_type_pair_lookup("Icemonster", &type, &id, NULL) && type == obj->type &&
            id == obj->id)
        {
            // STRING: LEGORR 0x004a5160
            name = "TinyIM";
        }
    }
    if (entity_type_pair_lookup(name, &type, &id, (int*)&model)) {
        legoobject_transform_position_get(obj, &x, &y);
        for (count = 0; count < 3; count++) {
            tiny = legoobject_create_in_world(model, type, id, 0, x, y, (float)(rng_thunk_draw15() % 7));
            tiny->health = -1.0f;
            if (!legoobject_route_step_command_issue(tiny)) {
                legoobject_remove(tiny);
                return;
            }
            obj->flags2 |= 0x20;
        }
    }
}

/* Two to five small spiders appear at random points of the block
 * (bx, by). */
// FUNCTION: LEGORR 0x0044b140
void legoobject_small_spider_cell_swarm_spawn(int bx, int by, unsigned int count)
{
    void* model;
    int type;
    int id;
    LegoObjP_Vector3F verts[4];
    unsigned int i;
    Lego_Object* spider;

    // STRING: LEGORR 0x004a5170
    if (entity_type_pair_lookup("SmallSpider", &type, &id, (int*)&model)) {
        terrain_map_block_vertices_get(Lego_GetMap(), bx, by, (struct TerrainMapVector3*)verts);
        for (i = (count & 3) + 2; i > 0; i--) {
            spider = legoobject_create_in_world(model, type, id, 0,
                                                random_float_range(verts[0].x + 1.0f, verts[2].x - 1.0f),
                                                random_float_range(verts[0].y - 1.0f, verts[2].y + 1.0f),
                                                (float)(rng_thunk_draw15() % 7));
            spider->flags1 |= 0x10000;
        }
    }
}

/* Two mini-figures (or a mini-figure and a monster) start the paired
 * activities 0x25/0x26 and link to each other through field_3c0. */
// FUNCTION: LEGORR 0x0044b250
void legoobject_pairing_activity_raise_and_mutual_link(Lego_Object* first, Lego_Object* second)
{
    if (legoobject_shared_eligibility_check(second, 1) && !(second->flags2 & 0x100) && !(first->flags2 & 1) &&
        !(second->flags2 & 2) && !legoobject_flags_mask107c33de_or_1c00203_any_test(first) &&
        entity_stats_float_ate8(LEGOOBJ_SLOT(first)) < first->health)
    {
        interface_back_to_main_if_selected((struct LegoObject*)second);
        legoobject_state_reset(second, 0, 1);
        legoobject_state_reset(first, 0, 1);
        legoobject_set_activity(first, 0x25, 1);
        legoobject_activity_register_bookkeeping(first);
        legoobject_set_activity(second, 0x26, 1);
        legoobject_activity_register_bookkeeping(second);
        aitask_ai_idle_chain_record_attach(LEGOOBJ_SLOT(first));
        aitask_ai_idle_chain_record_attach(LEGOOBJ_SLOT(second));
        first->flags2 |= 1;
        second->flags2 |= 2;
        first->field_3c0 = second;
        second->field_3c0 = first;
    }
}

/* Tool types 4, 5 and 7 take a carry slot of their own. */
// FUNCTION: LEGORR 0x0044b350
int legoobject_activity_is_special(int activity)
{
    if (activity != 4 && activity != 5 && activity != 7) return 0;
    return 1;
}

/* LegoObject_AddTool-like: gives obj the tool, replacing a special tool
 * or the oldest one when the object carries its maximum. */
// FUNCTION: LEGORR 0x0044b370
void legoobject_activity_grant(Lego_Object* obj, int activity)
{
    unsigned int cap = ai_activity_cap(LEGOOBJ_SLOT(obj));
    unsigned int i;

    obj->field_3f4 = 0;
    if (cap) {
        obj->flags4 &= ~4;
        if (legoobject_can_do_activity_classify(obj) || legoobject_activity_is_special(activity)) obj->flags4 |= 4;
        if (legoobject_activity_is_special(activity)) {
            for (i = 0; i < obj->numCarriedTools; i++) {
                if (legoobject_activity_is_special(obj->carriedTools[i])) {
                    obj->carriedTools[i] = activity;
                    return;
                }
            }
        }
        if (cap > 0) {
            if (obj->numCarriedTools == cap) {
                for (i = 0; i < obj->numCarriedTools - 1; i++) obj->carriedTools[i] = obj->carriedTools[i + 1];
                obj->carriedTools[obj->numCarriedTools - 1] = activity;
                return;
            }
            obj->carriedTools[obj->numCarriedTools] = activity;
            obj->numCarriedTools++;
        }
    }
}

/* Whether obj carries the tool. */
// FUNCTION: LEGORR 0x0044b480
int legoobject_activity_scan(Lego_Object* obj, int activity)
{
    unsigned int i;

    for (i = 0; i < obj->numCarriedTools; i++) {
        if (obj->carriedTools[i] == activity) return 1;
    }
    return 0;
}

/* Whether a mini-figure carries the tool the task type needs. */
// FUNCTION: LEGORR 0x0044b4c0
int legoobject_type_two_target_resolve_and_apply(Lego_Object* obj, int code)
{
    int tool;

    if (obj->type == 2 && entity_grouped_code_table_find_value_index(code, &tool))
        return legoobject_activity_scan(obj, tool);
    return 0;
}

/* Routes a mini-figure to the building it stands at, to the camera null
 * the tool type names (s_activity_resource_slots), with action 0xd. */
// FUNCTION: LEGORR 0x0044b500
int legoobject_position_step_command_issue_and_link(Lego_Object* obj, int selector)
{
    int result = 0;
    int ok = 0;
    float pos[2];
    Lego_Object* building;
    float nullPos[3];
    float target[2];
    LegoObjF_Point2I toBlock;
    LegoObjF_Point2I fromBlock;
    unsigned int* xs;
    unsigned int* ys;
    unsigned int count;

    if (obj->type == 2) {
        legoobject_transform_position_get(obj, &pos[0], &pos[1]);
        building = legoobject_lookup_at_world_position(pos);
        if (building) {
            ddiscene_object_position_get(
                scene_object_indexed_secondary_handle_get_or_create(building->building, s_activity_resource_slots[selector]),
                NULL, nullPos);
            target[0] = nullPos[0];
            target[1] = nullPos[1];
            terrain_map_world_to_block(Lego_GetMap(), nullPos[0], nullPos[1], &toBlock.x, &toBlock.y);
            terrain_map_world_to_block(Lego_GetMap(), pos[0], pos[1], &fromBlock.x, &fromBlock.y);
            if (fromBlock.x == toBlock.x && fromBlock.y == toBlock.y) {
                result = legoobject_route_setup_and_marker_associate(obj, 1, (unsigned int*)&toBlock.x,
                                                                     (unsigned int*)&toBlock.y, target);
            } else {
                if (legoobject_terrain_neighbor_path_check(obj, fromBlock.x, fromBlock.y, toBlock.x, toBlock.y,
                                                           &xs, &ys, &count, NULL, NULL))
                {
                    ok = legoobject_route_setup_and_marker_associate(obj, count, xs, ys, target);
                    free(xs);
                    free(ys);
                    result = ok;
                }
            }
            ok = result;
            if (result) {
                obj->routeToObject = building;
                ((LegoObjG_RouteBlock*)obj->routeBlocks)[obj->routeBlocksTotal - 1].actionFlags = 0xd;
            }
        }
    }
    return ok;
}

/* LegoObject_Flocks_Initialise: a flock of FlocksSize clones of the
 * object's container, each a steering item around a leader that carries
 * a reference to the container. */
// FUNCTION: LEGORR 0x0044b680
void legoobject_scene_object_tree_attach(Lego_Object* obj)
{
    float position[3] = { 0.0f, 0.0f, 0.0f };
    float target[3] = { 0.0f, 0.0f, 0.0f };
    int count;
    void* cont;
    void* root;
    void* clone;

    count = entity_stats_word_at13c(LEGOOBJ_SLOT(obj));
    cont = legoobject_type_component_get(obj);
    root = ddiscene_object_allocate(cont);
    ddiscene_scene_object_attach_or_unlink(root, scene_root_object_get());
    obj->field_32c = (int)(uintptr_t)callback_state_record_create(NULL, NULL,
        entity_stats_float_at120(LEGOOBJ_SLOT(obj)), entity_stats_float_at124(LEGOOBJ_SLOT(obj)),
        0.0f, obj);
    ((LegoObjT_Flock*)(uintptr_t)obj->field_32c)->leader->container = root;
    for (; count > 0; count--) {
        clone = entity_visual_resource_load(cont);
        ddiscene_scene_object_attach_or_unlink(clone, root);
        ptrstack_push((void*)(uintptr_t)obj->field_32c,
            scene_node_record_create(position, target,
                entity_stats_float_at120(LEGOOBJ_SLOT(obj)), entity_stats_float_at124(LEGOOBJ_SLOT(obj)),
                entity_stats_float_at128(LEGOOBJ_SLOT(obj)), entity_stats_float_at12c(LEGOOBJ_SLOT(obj)),
                clone));
    }
}

/* Turns a flock item's transform a quarter turn about x, swaps its
 * position into world axes and sets the container's matrix. */
// FUNCTION: LEGORR 0x0044b7a0
void legoobject_matrix_rotate_z_with_offset(void* cont, float* matrix, float offset)
{
    float x;
    float y;
    float z;
    float result[16];
    float rot[16];

    z = -matrix[13] - offset;
    x = matrix[12];
    y = matrix[14];
    matrix[12] = 0.0f;
    matrix[13] = 0.0f;
    matrix[14] = 0.0f;
    matrix4_set_rotation_x(rot, 1.5707964f);
    matrix4_multiply(result, rot, matrix);
    matrix4_copy(matrix, result);
    matrix[12] = x;
    matrix[13] = y;
    matrix[14] = z;
    legoobject_gt_container_matrix_set_adapter(cont, 0, matrix);
}

/* The per-item step of a flock (ptrstack_for_each): place the item's
 * container, tilt it or stand it on the ground, and run its animation. */
// FUNCTION: LEGORR 0x0044b850
void legoobject_scene_node_sync_callback(void* flock, void* item, void* data)
{
    LegoObjT_FlockUpdate* update = (LegoObjT_FlockUpdate*)data;
    LegoObjT_FlockItem* it = (LegoObjT_FlockItem*)item;
    void* cont = it->container;
    float up[3];
    float dir[3];
    float x;
    float y;
    float z;

    if (!update->onGround) {
        legoobject_matrix_rotate_z_with_offset(cont, it->transform, update->height);
        if (update->tilt) {
            ddiscene_object_facing_pair_get(cont, NULL, dir, up);
            /* Tip the up vector halfway towards -z. The original's x and y
             * make a round trip through the FPU (fld/fstp) that only a
             * double expression gives; the z offset is a float. */
            up[0] = (float)(up[0] - 0.0);
            up[1] = (float)(up[1] - 0.0);
            up[2] = up[2] - 1.0f;
            legoobject_vector3_scale((LegoObjJ_Vector3F*)up, (LegoObjJ_Vector3F*)up, 0.5f);
            ddiscene_object_pose_method_dispatch(cont, NULL, dir[0], dir[1], dir[2], up[0], up[1], up[2]);
        }
    } else {
        float north[3] = { 0.0f, 1.0f, 0.0f };
        float axis[3] = { 0.0f, 0.0f, -1.0f };
        x = it->transform[12];
        y = it->transform[14];
        z = terrain_map_world_z_get(Lego_GetMap(), x, y);
        terrain_map_world_normal_get(Lego_GetMap(), x, y, (struct TerrainMapVector3*)up);
        d3drm_vector_rotate(dir, north, axis, -it->yaw);
        ddiscene_object_pose_method_dispatch(cont, NULL, up[0], up[1], up[2], dir[0], dir[1], dir[2]);
        ddiscene_object_facing_pair_get(cont, NULL, up, dir);
        ddiscene_object_pose_method_dispatch(cont, NULL, dir[0], dir[1], dir[2], up[0], up[1], up[2]);
        ddiscene_object_position_set(cont, NULL, x, y, z);
    }
    if (update->animate) {
        if (it->pitch < legoobject_zero) {
            if (ddiscene_object_length_get(cont) != legoobject_zero &&
                ddiscene_advance_by_time_target(cont, update->elapsed) > legoobject_zero)
                ddiscene_frame_update_and_callbacks(cont, 0.0f);
        } else ddiscene_advance_by_time_target(cont, update->elapsed);
    } else ddiscene_advance_by_time_target(cont, update->elapsed);
}

/* Unlinks and removes a flock item's container (ptrstack_for_each). */
// FUNCTION: LEGORR 0x0044bab0
void legoobject_resource_dispose_callback(void* flock, void* item, void* data)
{
    void* cont = ((LegoObjT_FlockItem*)item)->container;
    if (cont) {
        ddiscene_scene_object_attach_or_unlink(cont, NULL);
        ledger_stub_00472a40_DDiSceneObjectDestroy(cont);
    }
}

/* LegoObject_Flocks_Destroy: the items' containers, the leader's
 * reference, then the flock. */
// FUNCTION: LEGORR 0x0044bae0
void legoobject_resource_and_callback_state_dispose(void* flock)
{
    void* cont;
    if (flock) {
        ptrstack_for_each(flock, legoobject_resource_dispose_callback, NULL);
        cont = ((LegoObjT_Flock*)flock)->leader->container;
        ddiscene_scene_object_attach_or_unlink(cont, NULL);
        ledger_stub_00472a40_DDiSceneObjectDestroy(cont);
        ((LegoObjT_Flock*)flock)->leader->container = NULL;
        callback_state_list_dispose(flock);
    }
}

/* Sets the flock's steering from the object's flocks stats, doubled (and
 * more) while it attacks. */
// FUNCTION: LEGORR 0x0044bb30
void legoobject_node_scale_update(Lego_Object* obj, int doubled)
{
    void* flock = (void*)(uintptr_t)obj->field_32c;
    if (doubled) {
        object_vector3_set(flock, entity_stats_float_at120(LEGOOBJ_SLOT(obj)) * 2.0f,
                           entity_stats_float_at124(LEGOOBJ_SLOT(obj)) * 2.0f,
                           entity_stats_float_at128(LEGOOBJ_SLOT(obj)) * 2.0f);
        global_vector_set_and_notify(flock, entity_stats_float_at120(LEGOOBJ_SLOT(obj)) * 4.0f,
                                     entity_stats_float_at124(LEGOOBJ_SLOT(obj)) * 3.0f,
                                     entity_stats_float_at128(LEGOOBJ_SLOT(obj)) * 2.0f,
                                     entity_stats_float_at12c(LEGOOBJ_SLOT(obj)) * 2.0f);
        return;
    }
    object_vector3_set(flock, entity_stats_float_at120(LEGOOBJ_SLOT(obj)),
                       entity_stats_float_at124(LEGOOBJ_SLOT(obj)),
                       entity_stats_float_at128(LEGOOBJ_SLOT(obj)));
    global_vector_set_and_notify(flock, entity_stats_float_at120(LEGOOBJ_SLOT(obj)),
                                 entity_stats_float_at124(LEGOOBJ_SLOT(obj)),
                                 entity_stats_float_at128(LEGOOBJ_SLOT(obj)),
                                 entity_stats_float_at12c(LEGOOBJ_SLOT(obj)));
}

/* LegoObject_Flocks_Update: steer the flock around the object's container
 * and step every item. */
// FUNCTION: LEGORR 0x0044bc30
void legoobject_motion_to_scene_node_sync(Lego_Object* obj, float elapsed)
{
    float target[3];
    float pos[3];
    LegoObjT_FlockUpdate update;

    if (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x200000) {
        if (!(entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x400000) &&
            !legoobject_gs_container_is_hidden_adapter(legoobject_type_component_get(obj)))
            legoobject_type_operation_dispatch(obj, 1);
        if (((LegoObjT_Flock*)(uintptr_t)obj->field_32c)->flags & 1)
            ((LegoObjT_Flock*)(uintptr_t)obj->field_32c)->attackTime += elapsed;
        if (entity_stats_float_at138(LEGOOBJ_SLOT(obj)) <= elapsed && !(obj->flags2 & 0x80)) {
            aitask_entity_command_unlink_by_kind(LEGOOBJ_SLOT(obj), 1, NULL, 1);
            ((LegoObjT_Flock*)(uintptr_t)obj->field_32c)->flags &= ~1;
        }
        ddiscene_object_position_get(legoobject_type_component_get(obj), NULL, pos);
        target[0] = pos[0];
        target[1] = -pos[2];
        target[2] = pos[1];
        if (!legoobject_gt_flock_has_position_adapter((void*)(uintptr_t)obj->field_32c)) {
            scene_node_position_set((void*)(uintptr_t)obj->field_32c, target);
            scene_object_list_apply_position_callback((void*)(uintptr_t)obj->field_32c, target);
            scene_object_list_apply_second_position_callback((void*)(uintptr_t)obj->field_32c, target);
        }
        if (obj->flags2 & 0x80) {
            legoobject_node_scale_update(obj, 1);
            obj->flags2 &= ~0x80;
        } else legoobject_node_scale_update(obj, 0);
        three_value_record_copy_and_mark_ready((void*)(uintptr_t)obj->field_32c, target);
        scene_object_list_apply_float_callback((void*)(uintptr_t)obj->field_32c, entity_stats_float_at134(LEGOOBJ_SLOT(obj)));
        scene_object_list_update_transforms((void*)(uintptr_t)obj->field_32c, elapsed);
        update.elapsed = elapsed;
        update.height = entity_stats_float_at130(LEGOOBJ_SLOT(obj));
        update.tilt = (entity_stats_word_get(LEGOOBJ_SLOT(obj)) >> 23) & 1;
        update.onGround = (entity_stats_word_get(LEGOOBJ_SLOT(obj)) >> 26) & 1;
        update.animate = (entity_stats_word_get(LEGOOBJ_SLOT(obj)) >> 28) & 1;
        ptrstack_for_each((void*)(uintptr_t)obj->field_32c, legoobject_scene_node_sync_callback, &update);
    }
}

/* LegoObject_Flocks_Remove: a flock that leaves keeps flying to its
 * leader's target on objectGlobs' released list (004eadd0) until every
 * item arrives; with stats flag 0x8000000 (or no memory) it goes now. */
// FUNCTION: LEGORR 0x0044be30
void legoobject_auxiliary_payload_release(Lego_Object* obj)
{
    float target[3];
    struct EntityAuxiliaryReleaseRecord* rec;
    LegoObjT_Flock* flock;

    if (entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x8000000) {
        legoobject_resource_and_callback_state_dispose((void*)(uintptr_t)obj->field_32c);
        return;
    }
    rec = (struct EntityAuxiliaryReleaseRecord*)malloc(sizeof(struct EntityAuxiliaryReleaseRecord));
    if (rec == NULL) {
        legoobject_resource_and_callback_state_dispose((void*)(uintptr_t)obj->field_32c);
        return;
    }
    flock = (LegoObjT_Flock*)obj->field_32c;
    target[0] = flock->leader->target[0];
    target[1] = flock->leader->target[1];
    target[2] = flock->leader->target[2];
    rec->scene_object = flock;
    rec->flags = entity_stats_word_get(LEGOOBJ_SLOT(obj));
    rec->next = s_entity_auxiliary_release_records;
    rec->value = entity_stats_float_at130(LEGOOBJ_SLOT(obj));
    s_entity_auxiliary_release_records = rec;
    scene_node_position_set((void*)(uintptr_t)obj->field_32c, target);
    scene_object_list_apply_second_position_callback((void*)(uintptr_t)obj->field_32c, target);
}

/* LegoObject_Flocks_UpdateReleased: steps every released flock and frees
 * those whose items have all arrived. */
// FUNCTION: LEGORR 0x0044bf00
void legoobject_timed_callback_list_update(float* elapsed)
{
    struct EntityAuxiliaryReleaseRecord* rec;
    struct EntityAuxiliaryReleaseRecord* next;
    struct EntityAuxiliaryReleaseRecord* prev;
    LegoObjT_FlockUpdate update;

    prev = NULL;
    for (rec = s_entity_auxiliary_release_records; rec != NULL; rec = next) {
        ptrstack_for_each(rec->scene_object, steering_item_follow_target, elapsed);
        update.elapsed = *elapsed;
        update.height = rec->value;
        update.tilt = (rec->flags >> 23) & 1;
        update.onGround = (rec->flags >> 26) & 1;
        update.animate = (rec->flags >> 28) & 1;
        ptrstack_for_each(rec->scene_object, legoobject_scene_node_sync_callback, &update);
        if (callback_operation_run_and_return_status(rec->scene_object)) {
            next = rec->next;
            if (prev) prev->next = next;
            else s_entity_auxiliary_release_records = next;
            legoobject_resource_and_callback_state_dispose(rec->scene_object);
            free(rec);
        } else {
            prev = rec;
            next = rec->next;
        }
    }
}

/* A monster whose spawn timer (+0x2d4) has run: break it up, spawn its
 * scatter, and flag it. */
// FUNCTION: LEGORR 0x0044bfd0
int legoobject_monster_spawn_behavior_tick(Lego_Object* obj)
{
    if (obj->animTime > legoobject_zero) {
        obj->flags1 &= ~0x100000;
        legoobject_tiny_monster_spawn_and_teardown(obj);
        legoobject_attached_spawn_record_process(obj);
        obj->flags3 |= 0x800000;
        global_counter_at_00555328_increment();
        return 1;
    }
    return 0;
}

/* Freezes a mini-figure once: an ice cube (type 0x12) at its position,
 * linked both ways through +0x364, for duration * 25 ticks. */
// FUNCTION: LEGORR 0x0044c030
int legoobject_one_shot_attached_effect_start(Lego_Object* obj, float duration)
{
    LegoObjP_Vector3F pos;
    float heading;
    Lego_Object* cube;

    if (!(obj->flags2 & 0x8000000) && obj->type == 3) {
        obj->flags2 |= 0x8000000;
        obj->field_368 = duration * 25.0f;
        legoobject_transform_position_get(obj, &pos.x, &pos.y);
        heading = legoobject_vector_direction_angle_radians_get(obj);
        cube = legoobject_create_in_world(legoGlobs_contIceCube, 0x12, 0, 0, pos.x, pos.y, heading);
        // STRING: LEGORR 0x004a517c
        scene_action_name_set(cube->other, "Start");
        ddiscene_frame_update_and_callbacks(cube->other, 0.0f);
        cube->flags1 |= 0x100;
        obj->field_364 = cube;
        cube->field_364 = obj;
        return 1;
    }
    return 0;
}

/* Wakes the object linked at +0x364 unless it is already going. */
// FUNCTION: LEGORR 0x0044c110
void legoobject_attached_entity_wake(Lego_Object* obj)
{
    if (!(((Lego_Object*)obj->field_364)->flags1 & 0x200000)) {
        ((Lego_Object*)obj->field_364)->health = 100.0f;
        ((Lego_Object*)obj->field_364)->flags3 |= 0x800000;
    }
}

/* Starts a slide of a vehicle, mini-figure or rock monster along dir for
 * distance (flag 0x4000 of +0x3e4). */
// FUNCTION: LEGORR 0x0044c150
int legoobject_pair_scalar_fields_set(Lego_Object* obj, float* dir, float distance)
{
    if (obj->type != 2 && obj->type != 3 && obj->type != 1) return 0;
    legoobject_state_reset(obj, 0, 1);
    obj->field_3b4 = dir[0];
    obj->field_3b8 = dir[1];
    obj->field_3bc = distance;
    obj->flags2 |= 0x4000;
    return 1;
}

/* One step of that slide; a blocked step snaps the direction to the
 * nearest axis and retries, and a blocked retry ends the slide. */
// FUNCTION: LEGORR 0x0044c1b0
void legoobject_heading_step_move(Lego_Object* obj, float elapsed)
{
    LegoObjM_Point2F pos;
    LegoObjM_Point2F move;
    int block[2];
    float len;

    legoobject_transform_position_get(obj, &pos.x, &pos.y);
    legoobject_vector2d_scale(&move, (LegoObjM_Point2F*)&obj->field_3b4, elapsed);
    pos.x = move.x + pos.x;
    pos.y = move.y + pos.y;
    terrain_map_world_to_block(Lego_GetMap(), pos.x, pos.y, &block[0], &block[1]);
    if (terrain_cell_route_and_occupancy_check_current(LEGOOBJ_SLOT(obj), block[0], block[1], block[0], block[1], 1) &&
        !legoobject_flag_gated_callback_dispatch(obj, &pos.x, 0.0f, 0)) {
        legoobject_world_position_set(obj, pos.x, pos.y, 0.0f, 0);
    } else {
        len = legoobject_o_vector2_modulus((Point2F*)&obj->field_3b4);
        legoobject_o_vector2_normalise((Point2F*)&obj->field_3b4);
        if (obj->field_3b4 > 0.5f) {
            obj->field_3b4 = 1.0f;
            obj->field_3b8 = 0.0f;
        } else if (obj->field_3b4 < -0.5f) {
            obj->field_3b4 = -1.0f;
            obj->field_3b8 = 0.0f;
        } else if (obj->field_3b8 > 0.5f) {
            obj->field_3b4 = 0.0f;
            obj->field_3b8 = 1.0f;
        } else if (obj->field_3b8 < -0.5f) {
            obj->field_3b4 = 0.0f;
            obj->field_3b8 = -1.0f;
        }
        legoobject_vector2d_scale((LegoObjM_Point2F*)&obj->field_3b4, (LegoObjM_Point2F*)&obj->field_3b4, len);
        legoobject_transform_position_get(obj, &pos.x, &pos.y);
        legoobject_vector2d_scale(&move, (LegoObjM_Point2F*)&obj->field_3b4, elapsed);
        pos.x = move.x + pos.x;
        pos.y = move.y + pos.y;
        terrain_map_world_to_block(Lego_GetMap(), pos.x, pos.y, &block[0], &block[1]);
        if (terrain_cell_route_and_occupancy_check_current(LEGOOBJ_SLOT(obj), block[0], block[1], block[0], block[1], 1) &&
            !legoobject_flag_gated_callback_dispatch(obj, &pos.x, 0.0f, 0)) {
            legoobject_world_position_set(obj, pos.x, pos.y, 0.0f, 0);
        } else obj->flags2 &= ~0x4000;
    }
    if (obj->field_3bc <= legoobject_zero) obj->flags2 &= ~0x4000;
    obj->field_3bc -= legoobject_vector2d_modulus(&move);
}

/* Interface_ClearSelection's object half: flags a selected unit for the
 * selection redraw, then resets the panel. */
// FUNCTION: LEGORR 0x0044c4a0
void legoobject_selection_clear_marker_and_reset_hud(Lego_Object* obj)
{
    if (obj->flags4 & 2) {
        if (entity_state_flags_get(LEGOOBJ_SLOT(obj)) & 0x20000) {
            if (!(entity_stats_word_get(LEGOOBJ_SLOT(obj)) & 0x20000) || obj->driveObject) {
                legoGlobs_flags1 |= 0x10000000;
                obj->flags4 |= 1;
            }
        }
    }
    selection_clear_request_and_hud_reset();
}

/* Clears that flag on every object (LegoObject_RunThroughLists, pushed by
 * Message.c's 004526e0). */
// FUNCTION: LEGORR 0x0044c500
int legoobject_selection_clear_callback(Lego_Object* obj, void* data)
{
    if (obj->flags4 & 1) legoGlobs_flags1 &= ~0x10000000;
    obj->flags4 &= ~1;
    return 0;
}

/* LegoObject_CanDo: only a mini-figure that can take an activity. */
// FUNCTION: LEGORR 0x0044c530
int legoobject_ai_can_do(Lego_Object* obj)
{
    if (obj->type == 2 && legoobject_can_do_activity_classify(obj)) return 1;
    return 0;
}

/* Moves the view to the next building not yet visited (then the next
 * vehicle), restarting the round when every one has been. */
// FUNCTION: LEGORR 0x0044c550
void legoobject_scatter_spawn_enable_and_drain(void)
{
    int vehicles;

    s_entity_state_flags |= 0x40;
    if (!legoobject_run_through_lists_skip_upgrade_parts(legoobject_scatter_spawn_drain_callback, NULL)) {
        if (s_object_f_4eabc8 == 0) {
            vehicles = 1;
            if (!legoobject_run_through_lists_skip_upgrade_parts(legoobject_scatter_spawn_drain_callback, &vehicles) &&
                s_object_f_4eabc4 != 0) {
                s_object_f_4eabc4 = 0;
                s_object_f_4eabc8 = 0;
                legoobject_run_through_lists_skip_upgrade_parts(legoobject_scatter_spawn_drain_callback, &vehicles);
            }
        } else {
            s_object_f_4eabc4 = 0;
            s_object_f_4eabc8 = 0;
            legoobject_run_through_lists_skip_upgrade_parts(legoobject_scatter_spawn_drain_callback, NULL);
        }
    }
}

/* The first building (data NULL) or vehicle (*data) not in the visited
 * list: add it, move the view to it and put the pointer's container at
 * the mouse's terrain point. Not matched: the original's list and count
 * are fields of one struct (objectGlobs), so after the store to the list
 * it reloads the count; two separate globals let it reuse the register. */
// FUNCTION: LEGORR 0x0044c5f0
int legoobject_scatter_spawn_drain_callback(Lego_Object* obj, void* data)
{
    float wPos[3];
    unsigned int bx;
    unsigned int by;
    unsigned int i;
    int vehicles = 0;

    if (data) vehicles = *(int*)data;
    if (vehicles) {
        if (obj->type != 2) return 0;
    } else if (obj->type != 4) return 0;
    for (i = 0; i < (unsigned int)s_object_f_4eabc4; i++) {
        if (objectGlobs_f_4ea7c4[i] == obj) return 0;
    }
    if ((unsigned int)s_object_f_4eabc4 < 0x100) {
        objectGlobs_f_4ea7c4[s_object_f_4eabc4] = obj;
        s_object_f_4eabc4++;
        if (obj->type == 4) s_object_f_4eabc8++;
        lego_viewport_target_position_clamp(LEGOOBJ_SLOT(obj), NULL, 0);
        if (terrain_map_screen_pick((struct TerrainMap*)LEGOOBJ_FIELD(g_active_level_retail, void*, 0x4),
                                    legoGlobs_viewMain, s_renderer_word_a, s_renderer_word_b,
                                    &bx, &by, (struct TerrainMapVector3*)wPos))
            ddiscene_object_position_set(legoGlobs_f_556108, NULL, wPos[0], wPos[1], wPos[2] - 250.0f);
        return 1;
    }
    s_object_f_4eabc4 = 0;
    s_object_f_4eabc8 = 0;
    return 1;
}

#ifdef __cplusplus
}
#endif


/* ================================================================== */
/* LegoObject.c's state (objectGlobs, 004de790-004eadd0), moved from   */
/* legoobject_port.cpp when that file retired to the port tail (issue #615).*/
/* Defined after every function, so no function's symbol counter      */
/* moves (finding 538 rule B, finding 577). The port reads these with  */
/* C linkage. The activityName and abilityName tables (004decc4,      */
/* 004ead60) stay in legoobject_port.cpp: they point at the port's     */
/* name strings, one of them lego_port.cpp's C++ object.               */
/* ================================================================== */
#ifdef __cplusplus
extern "C" {
#endif
// GLOBAL: LEGORR 0x004de790
void* s_pool_chunks[32];
// GLOBAL: LEGORR 0x004de810
void* s_pool_record_head;

// GLOBAL: LEGORR 0x004de814
int s_terrain_sfx_grid[300]; /* DAT_004de814 role */



// GLOBAL: LEGORR 0x004dee00
float* s_terrain_neighbor_path_cost;
/* DAT_004dee04 / DAT_004dee08: the retail path-search functions share and
 * reuse one zeroed dword per map cell. */
// GLOBAL: LEGORR 0x004dee04
uint32_t* s_terrain_path_depth;
// GLOBAL: LEGORR 0x004dee08
uint32_t s_terrain_path_depth_count;

// GLOBAL: LEGORR 0x004dee10
uint32_t s_pool_chunk_count;

/* --- the entity activity scene-state leaf (see ai.hpp) ---------------------
 * This shared zero-init word owns the ported DAT_004dee14 bits. The corpus
 * reads its level pointer through the threaded GameContext. */
// GLOBAL: LEGORR 0x004dee14
uint32_t s_entity_state_flags;
// GLOBAL: LEGORR 0x004dee18
uint32_t s_activity_resource_slots[11] = {
    0, 0, 1, 1, 4, 4, 4, 4, 3, 2, 4
};

// GLOBAL: LEGORR 0x004dee44
uint32_t s_grid_count_second[0x12c0];

// GLOBAL: LEGORR 0x004e2684
uint32_t s_task_counter_nonzero;

// GLOBAL: LEGORR 0x004e3944
uint32_t s_grid_count_first[0x12c0];

// GLOBAL: LEGORR 0x004e7184
uint32_t s_task_counter_zero;

/* The corpus keeps a bounded twenty-entry pool of the predug coordinates
 * encountered while loading the layer. */
// GLOBAL: LEGORR 0x004e844c
int s_predug_coordinates[20][2];

// GLOBAL: LEGORR 0x004e84ec
int s_type11_cell_coordinates[10][2];

// GLOBAL: LEGORR 0x004e853c
unsigned int s_predug_coordinate_count;

// GLOBAL: LEGORR 0x004e8540
unsigned int s_type11_cell_coordinate_count;

/* The six-dword render snapshot is a separate zero-filled global block in
 * the corpus (DAT_004ea7ac). Its individual values are unresolved; keep the
 * storage opaque and expose only the exact copy/clear operations. */
// GLOBAL: LEGORR 0x004ea7ac
uint32_t s_render_config_snapshot[6];

// GLOBAL: LEGORR 0x004eabcc
struct EntitySlot* s_idle100[100];
// GLOBAL: LEGORR 0x004ead5c
int s_idle100_n;

/* GlobalAnimationStateSet (FUN_004383a0) — see entity.hpp. */
/* objectGlobs (004ea7c4): the objects 0044c5f0 has already visited when
 * cycling the camera; its count and stage are lego_unit.c's 004eabc4 and
 * 004eabc8. */
// GLOBAL: LEGORR 0x004ea7c4
Lego_Object* objectGlobs_f_4ea7c4[256];

// GLOBAL: LEGORR 0x004eadc4
uint32_t s_global_animation_state; /* DAT_004eadc4 */
// GLOBAL: LEGORR 0x004eadd0
struct EntityAuxiliaryReleaseRecord* s_entity_auxiliary_release_records;
#ifdef __cplusplus
}
#endif
