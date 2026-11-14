// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-027: the message queue and the unit selection (the original's
 * Message.c), rebuilt in original shape (ADR 0005, finding 631). Text
 * 00451cd0-00452df0, .bss 004eaf20-004ff2a0.
 *
 * Events posted during a frame go to one bank of the two-bank queue
 * (Message_PostEvent); Message_Update swaps the banks and dispatches the
 * frame's events. The unit also keeps the debug key actions and the list of
 * selected units. The port's models of the same functions stay in
 * message_port.cpp, unannotated: natively they are the live ones and
 * nothing calls the unit. See message_unit.h for the records. */
#include <stdlib.h>
#include <string.h>

#include "message_unit_link_names.h"
#include "message_unit.h"
#include "message_unit_imports.h"
#include "crt/vc5names.h"
/* legoGlobs' fields (lego_globs.h, finding 653). lego_unit.h, included
 * above, pays for its symbols (finding 538). */
#include "lego_globs.h"

/* Lego_GetMap (00431570) takes no argument; the port's entry under that
 * address, level_current_terrain_get, takes a cell (lego_unit.h). */
#define MESSAGE_LEGO_GET_MAP() (((void* (*)())level_current_terrain_get)())

/* StatsFlags1: the object can be driven. */
#define MESSAGE_STATS1_CANBEDRIVEN 0x20000

/* Message.c's .bss (004eaf20-004ff2a0), 0x14380 bytes. */
typedef struct Message_Globs {
    Message_Event eventList[2][MESSAGE_MAXEVENTS];          /* +0x00000 */
    unsigned int eventCount[2];                             /* +0x14000 */
    unsigned int eventAB;                                   /* +0x14008 */
    Lego_Object* selectedUnitList[MESSAGE_MAXSELECTED];     /* +0x1400c */
    unsigned char keyList[12];                              /* +0x1419c */
    Message_Event messageAction[MESSAGE_MAXHOTKEYS];        /* +0x141a8 */
    const char* messageName[MESSAGE_TYPE_COUNT];            /* +0x14270 */
    unsigned int f_14374;                                   /* +0x14374 not read */
    unsigned int selectedUnitCount;                         /* +0x14378 */
    unsigned int keyCount;                                  /* +0x1437c */
} Message_Globs;

// GLOBAL: LEGORR 0x004eaf20
static Message_Globs messageGlobs;

// FUNCTION: LEGORR 0x00451cd0
void message_initialise(void)
{
    // STRING: LEGORR 0x004a5938
    messageGlobs.messageName[0] = "Message_Null";
    // STRING: LEGORR 0x004a5928
    messageGlobs.messageName[1] = "Message_Select";
    // STRING: LEGORR 0x004a5914
    messageGlobs.messageName[2] = "Message_Selected";
    // STRING: LEGORR 0x004a58fc
    messageGlobs.messageName[3] = "Message_ClearSelection";
    // STRING: LEGORR 0x004a58e8
    messageGlobs.messageName[4] = "Message_Deselect";
    // STRING: LEGORR 0x004a58d8
    messageGlobs.messageName[5] = "Message_Goto";
    // STRING: LEGORR 0x004a58c0
    messageGlobs.messageName[6] = "Message_RockMonsterGoto";
    // STRING: LEGORR 0x004a58a0
    messageGlobs.messageName[7] = "Message_RockMonsterGotoComplete";
    // STRING: LEGORR 0x004a588c
    messageGlobs.messageName[8] = "Message_UserGoto";
    // STRING: LEGORR 0x004a5878
    messageGlobs.messageName[9] = "Message_FirstPerson";
    // STRING: LEGORR 0x004a5864
    messageGlobs.messageName[10] = "Message_TrackObject";
    // STRING: LEGORR 0x004a5854
    messageGlobs.messageName[11] = "Message_TopView";
    // STRING: LEGORR 0x004a5840
    messageGlobs.messageName[12] = "Message_PlaySample";
    // STRING: LEGORR 0x004a5834
    messageGlobs.messageName[13] = "Message_Dig";
    // STRING: LEGORR 0x004a5820
    messageGlobs.messageName[14] = "Message_DigComplete";
    // STRING: LEGORR 0x004a5810
    messageGlobs.messageName[15] = "Message_Repair";
    // STRING: LEGORR 0x004a57f8
    messageGlobs.messageName[16] = "Message_RepairComplete";
    // STRING: LEGORR 0x004a57e4
    messageGlobs.messageName[17] = "Message_Reinforce";
    // STRING: LEGORR 0x004a57c8
    messageGlobs.messageName[18] = "Message_ReinforceComplete";
    // STRING: LEGORR 0x004a57b4
    messageGlobs.messageName[19] = "Message_RockFall";
    // STRING: LEGORR 0x004a5798
    messageGlobs.messageName[20] = "Message_RockFallComplete";
    // STRING: LEGORR 0x004a5780
    messageGlobs.messageName[21] = "Message_GenerateCrystal";
    // STRING: LEGORR 0x004a5760
    messageGlobs.messageName[22] = "Message_GenerateCrystalComplete";
    // STRING: LEGORR 0x004a5748
    messageGlobs.messageName[23] = "Message_CollectCrystal";
    // STRING: LEGORR 0x004a5728
    messageGlobs.messageName[24] = "Message_CollectCrystalComplete";
    // STRING: LEGORR 0x004a570c
    messageGlobs.messageName[25] = "Message_CrystalToRefinery";
    // STRING: LEGORR 0x004a56e8
    messageGlobs.messageName[26] = "Message_CrystalToRefineryComplete";
    // STRING: LEGORR 0x004a56d4
    messageGlobs.messageName[27] = "Message_GenerateOre";
    // STRING: LEGORR 0x004a56b8
    messageGlobs.messageName[28] = "Message_GenerateOreComplete";
    // STRING: LEGORR 0x004a56a4
    messageGlobs.messageName[29] = "Message_CollectOre";
    // STRING: LEGORR 0x004a5688
    messageGlobs.messageName[30] = "Message_CollectOreComplete";
    // STRING: LEGORR 0x004a5678
    messageGlobs.messageName[39] = "Message_Clear";
    // STRING: LEGORR 0x004a5660
    messageGlobs.messageName[40] = "Message_ClearComplete";
    // STRING: LEGORR 0x004a564c
    messageGlobs.messageName[46] = "Message_ClearFallIn";
    // STRING: LEGORR 0x004a5630
    messageGlobs.messageName[47] = "Message_ClearFallInComplete";
    // STRING: LEGORR 0x004a5614
    messageGlobs.messageName[31] = "Message_GenerateRockMonster";
    // STRING: LEGORR 0x004a55f0
    messageGlobs.messageName[32] = "Message_GenerateRockMonsterComplete";
    // STRING: LEGORR 0x004a55dc
    messageGlobs.messageName[33] = "Message_GatherRock";
    // STRING: LEGORR 0x004a55c0
    messageGlobs.messageName[34] = "Message_GatherRockComplete";
    // STRING: LEGORR 0x004a55a8
    messageGlobs.messageName[35] = "Message_PickRandomFloor";
    // STRING: LEGORR 0x004a5588
    messageGlobs.messageName[36] = "Message_PickRandomFloorComplete";
    // STRING: LEGORR 0x004a5570
    messageGlobs.messageName[37] = "Message_AttackBuilding";
    // STRING: LEGORR 0x004a5550
    messageGlobs.messageName[38] = "Message_AttackBuildingComplete";
    // STRING: LEGORR 0x004a5540
    messageGlobs.messageName[41] = "Message_GetIn";
    // STRING: LEGORR 0x004a5518
    messageGlobs.messageName[42] = "Message_ManSelectedAndMonsterClicked";
    // STRING: LEGORR 0x004a5500
    messageGlobs.messageName[43] = "Message_FollowAttack";
    // STRING: LEGORR 0x004a54ec
    messageGlobs.messageName[44] = "Message_CollectTool";
    // STRING: LEGORR 0x004a54d4
    messageGlobs.messageName[45] = "Message_ReduceSelection";
    // STRING: LEGORR 0x004a54c0
    messageGlobs.messageName[48] = "Message_BuildPath";
    // STRING: LEGORR 0x004a54a4
    messageGlobs.messageName[49] = "Message_BuildPathComplete";
    // STRING: LEGORR 0x004a5494
    messageGlobs.messageName[50] = "Message_Train";
    // STRING: LEGORR 0x004a547c
    messageGlobs.messageName[51] = "Message_TrainComplete";
    // STRING: LEGORR 0x004a545c
    messageGlobs.messageName[52] = "Message_GenerateCrystalAndOre";
    // STRING: LEGORR 0x004a5434
    messageGlobs.messageName[53] = "Message_GenerateCrystalAndOreComplete";
    // STRING: LEGORR 0x004a5418
    messageGlobs.messageName[54] = "Message_GenerateFromCryOre";
    // STRING: LEGORR 0x004a53f4
    messageGlobs.messageName[55] = "Message_GenerateFromCryOreComplete";
    // STRING: LEGORR 0x004a53e4
    messageGlobs.messageName[56] = "Message_Upgrade";
    // STRING: LEGORR 0x004a53cc
    messageGlobs.messageName[57] = "Message_UpgradeComplete";
    // STRING: LEGORR 0x004a53b4
    messageGlobs.messageName[58] = "Message_ClearBuilding";
    // STRING: LEGORR 0x004a5394
    messageGlobs.messageName[59] = "Message_ClearBuildingComplete";
    // STRING: LEGORR 0x004a537c
    messageGlobs.messageName[60] = "Message_ClearInitial";
    // STRING: LEGORR 0x004a535c
    messageGlobs.messageName[61] = "Message_ClearInitialComplete";
    // STRING: LEGORR 0x004a5344
    messageGlobs.messageName[62] = "Message_ClearRemovePath";
    // STRING: LEGORR 0x004a5324
    messageGlobs.messageName[63] = "Message_ClearRemovePathComplete";
    // STRING: LEGORR 0x004a5308
    messageGlobs.messageName[64] = "Message_Debug_DestroyAll";
}

// FUNCTION: LEGORR 0x00451f60
void message_remove_events_with_object(Lego_Object* obj)
{
    unsigned int loop;
    unsigned int count = messageGlobs.eventCount[messageGlobs.eventAB];

    for (loop = 0; loop < count; loop++) {
        if (messageGlobs.eventList[messageGlobs.eventAB][loop].argument1 == obj) {
            count--;
            messageGlobs.eventList[messageGlobs.eventAB][loop] =
                messageGlobs.eventList[messageGlobs.eventAB][count];
            messageGlobs.eventCount[messageGlobs.eventAB] = count;
        }
    }
}

// FUNCTION: LEGORR 0x00451fd0
int message_lookup_physical_name(const char* name)
{
    unsigned int i;

    for (i = 0; i < MESSAGE_TYPE_COUNT; i++) {
        if (!_strcmpi(messageGlobs.messageName[i] + 8, name)) return i;
    }
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
    /* The original falls off the end and returns the last comparison's
     * result; C++ needs a return. */
    return MESSAGE_TYPE_COUNT;
#endif
}

// FUNCTION: LEGORR 0x00452010
void message_add_message_action(unsigned char key, unsigned int type,
                                Lego_Object* argument1, unsigned int argument2,
                                const int* blockPos)
{
    Message_Event* event;

    messageGlobs.keyList[messageGlobs.keyCount] = key;
    event = &messageGlobs.messageAction[messageGlobs.keyCount];
    event->type = type;
    event->argument1 = argument1;
    event->argument2 = argument2;
    if (blockPos) {
        event->blockPos[0] = blockPos[0];
        event->blockPos[1] = blockPos[1];
    }
    messageGlobs.keyCount++;
}

// FUNCTION: LEGORR 0x00452060
void message_post_event(unsigned int type, Lego_Object* argument1,
                        unsigned int argument2, const int* blockPos)
{
    Message_Event* event;

    if (messageGlobs.eventCount[messageGlobs.eventAB] < MESSAGE_MAXEVENTS) {
        event = &messageGlobs.eventList[messageGlobs.eventAB]
                                       [messageGlobs.eventCount[messageGlobs.eventAB]++];
        memset(event, 0, sizeof(Message_Event));
        event->type = type;
        event->argument1 = argument1;
        event->argument2 = argument2;
        if (blockPos) {
            event->blockPos[0] = blockPos[0];
            event->blockPos[1] = blockPos[1];
        }
    }
}

// FUNCTION: LEGORR 0x004520d0
void message_update(void)
{
    unsigned int loop;
    unsigned int count;
    Message_Event* list;

    if (!(legoGlobs_flags1 & 0x100000)) {
        for (loop = 0; loop < messageGlobs.keyCount; loop++) {
            if (s_global_current_keys[messageGlobs.keyList[loop]]) {
                message_post_event(messageGlobs.messageAction[loop].type,
                                   messageGlobs.messageAction[loop].argument1,
                                   messageGlobs.messageAction[loop].argument2,
                                   messageGlobs.messageAction[loop].blockPos);
            }
        }
    }

    for (loop = 0; loop < messageGlobs.eventCount[messageGlobs.eventAB]; loop++) {
        command_entity_reference_remap(
            (uint32_t*)&messageGlobs.eventList[messageGlobs.eventAB][loop]);
    }

    count = messageGlobs.eventCount[messageGlobs.eventAB];
    list = messageGlobs.eventList[messageGlobs.eventAB];
    messageGlobs.eventCount[messageGlobs.eventAB] = 0;
    messageGlobs.eventAB = !messageGlobs.eventAB;
    messageGlobs.eventCount[messageGlobs.eventAB] = 0;

    for (loop = 0; loop < count; loop++) {
        switch (list[loop].type) {
        case MESSAGE_CLEARSELECTION:
            message_clear_selected_units();
            break;
        case MESSAGE_SELECT:
            message_select_object(list[loop].argument1, list[loop].argument2);
            break;
        case MESSAGE_DESELECT:
            message_deselect_object(list[loop].argument1);
            break;
        case MESSAGE_USERGOTO:
            aitask_selection_drag_area_command_create(
                (struct EntitySlot**)messageGlobs.selectedUnitList,
                messageGlobs.selectedUnitCount, list[loop].blockPos,
                list[loop].argument2);
            break;
        case MESSAGE_GOTO:
            aitask_cmd_create_move((struct EntitySlot*)list[loop].argument1,
                                   list[loop].blockPos, list[loop].argument2);
            break;
        case MESSAGE_ROCKMONSTERGOTO:
            aitask_cmd_create_move((struct EntitySlot*)list[loop].argument1,
                                   list[loop].blockPos, list[loop].argument2);
            break;
        case MESSAGE_FIRSTPERSON:
            message_enter_first_person_view(list[loop].argument2);
            break;
        case MESSAGE_TRACKOBJECT:
            if (messageGlobs.selectedUnitCount)
                lego_render_record_set_from_input(
                    (struct EntitySlot*)messageGlobs.selectedUnitList[0]);
            break;
        case MESSAGE_TOPVIEW:
            lego_sound_minimum_distance_update_mode(1, NULL, 0);
            break;
        case MESSAGE_PLAYSAMPLE:
            sfx_random_play_sound_normal(list[loop].argument2, 0);
            break;
        case MESSAGE_ROCKFALL:
            lego_terrain_stamp_impact_handle(list[loop].blockPos[0], list[loop].blockPos[1],
                                             list[loop].argument2 & 0xffff,
                                             list[loop].argument2 >> 16);
            break;
        case MESSAGE_COLLECTCRYSTAL:
            aitask_cmd_create_task((struct EntitySlot*)list[loop].argument1, 0.0f);
            break;
        case MESSAGE_COLLECTORE:
            aitask_cmd_create_task((struct EntitySlot*)list[loop].argument1, 0.0f);
            break;
        case MESSAGE_GENERATECRYSTAL:
            legoobject_build_placement_command_setup(list[loop].blockPos, 0, NULL, 1);
            break;
        case MESSAGE_GENERATEORE:
            legoobject_build_placement_alternate_command_setup(list[loop].blockPos, 0,
                                                               NULL, 1);
            break;
        case MESSAGE_GENERATECRYSTALANDORE:
            legoobject_build_placement_command_pair_create(list[loop].blockPos, 0);
            break;
        case MESSAGE_GENERATEFROMCRYORE:
            legoobject_build_placement_command_batch_expand(list[loop].blockPos);
            break;
        case MESSAGE_CRYSTALTOREFINERY:
            aitask_cmd_create_gated((struct EntitySlot*)list[loop].argument1,
                                    (struct EntitySlot*)(uintptr_t)list[loop].argument2);
            break;
        case MESSAGE_GENERATEROCKMONSTER:
            legoobject_emergence_random_spawn_attempt();
            break;
        case MESSAGE_GATHERROCK:
            legoobject_conditional_type_five_spawn(list[loop].argument1);
            break;
        case MESSAGE_PICKRANDOMFLOOR:
            message_pick_random_floor(list[loop].argument1);
            break;
        case MESSAGE_ATTACKBUILDING:
            legoobject_adjacent_placement_cell_search(list[loop].argument1,
                                                      (Lego_Object*)(uintptr_t)list[loop].argument2);
            break;
        case MESSAGE_FOLLOWATTACK:
            aitask_cmd_create_target((struct EntitySlot*)list[loop].argument1,
                                     (struct EntitySlot*)(uintptr_t)list[loop].argument2);
            break;
        case MESSAGE_REDUCESELECTION:
            message_reduce_selected_units();
            break;
        case MESSAGE_DEBUG_DESTROYALL:
            message_kill_all_selected_units();
            break;
        }
    }
}

// FUNCTION: LEGORR 0x00452430
void message_pick_random_floor(Lego_Object* obj)
{
    int blockPos[2];
    unsigned int width = ((unsigned int*)MESSAGE_LEGO_GET_MAP())[0];
    unsigned int height = ((unsigned int*)MESSAGE_LEGO_GET_MAP())[1];
    int found = 0;
    unsigned int tries;

    for (tries = 0; tries < 1000; tries++) {
        blockPos[0] = (int)rng_thunk_draw15() % (int)width;
        blockPos[1] = (int)rng_thunk_draw15() % (int)height;
        if (terrain_cell_flag_8_bounded_current(blockPos[0], blockPos[1])) {
            found = 1;
            break;
        }
    }
    if (found) message_post_event(MESSAGE_PICKRANDOMFLOORCOMPLETE, obj, 0, blockPos);
}

// FUNCTION: LEGORR 0x004524b0
void message_remove_object_reference(Lego_Object* deadObj)
{
    message_remove_events_with_object(deadObj);
    message_deselect_object(deadObj);
    if ((Lego_Object*)legoGlobs_selectedObject == deadObj) {
        lego_sound_minimum_distance_update_mode(1, NULL, 0);
        legoGlobs_selectedObject = NULL;
    }
    if (LEGO_FIELD(legoGlobs_cameraTrack, Lego_Object*, 0x04) == deadObj) {
        LEGO_FIELD(legoGlobs_cameraTrack, Lego_Object*, 0x04) = NULL;
        legoobject_transform_position_get(deadObj, &legoGlobs_f_5563f8.x,
                                          &legoGlobs_f_5563f8.y);
        legoGlobs_flags1 |= 0x4000;
    }
}

// FUNCTION: LEGORR 0x00452520
int message_copy_selected_units(Lego_Object*** outList, unsigned int* outCount)
{
    if (messageGlobs.selectedUnitCount) {
        *outCount = messageGlobs.selectedUnitCount;
        *outList = (Lego_Object**)malloc(messageGlobs.selectedUnitCount *
                                         sizeof(Lego_Object*));
        memcpy(*outList, messageGlobs.selectedUnitList,
               *outCount * sizeof(Lego_Object*));
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00452580
Lego_Object** message_get_selected_units2(unsigned int* outCount)
{
    if (messageGlobs.selectedUnitCount) {
        if (outCount) *outCount = messageGlobs.selectedUnitCount;
        return messageGlobs.selectedUnitList;
    }
    if (outCount) *outCount = 0;
    return NULL;
}

// FUNCTION: LEGORR 0x004525b0
int message_any_unit_selected(void)
{
    return messageGlobs.selectedUnitCount != 0;
}

// FUNCTION: LEGORR 0x004525c0
Lego_Object* message_get_primary_selected_unit(void)
{
    if (messageGlobs.selectedUnitCount) return messageGlobs.selectedUnitList[0];
    return NULL;
}

// FUNCTION: LEGORR 0x004525e0
Lego_Object** message_get_selected_units(void)
{
    return messageGlobs.selectedUnitList;
}

// FUNCTION: LEGORR 0x004525f0
unsigned int message_get_num_selected_units(void)
{
    return messageGlobs.selectedUnitCount;
}

// FUNCTION: LEGORR 0x00452600
void message_cleanup_selected_units_count(void)
{
    messageGlobs.selectedUnitCount = 0;
}

// FUNCTION: LEGORR 0x00452610
unsigned int message_is_unit_selected(Lego_Object* obj, unsigned int* outIndex)
{
    if (outIndex) {
        return message_find_index_of_object(messageGlobs.selectedUnitList,
                                            messageGlobs.selectedUnitCount,
                                            obj, outIndex);
    }
    return obj->flags3 & 0x200000;
}

// FUNCTION: LEGORR 0x00452650
int message_find_index_of_object(Lego_Object** list, unsigned int count,
                                 Lego_Object* obj, unsigned int* outIndex)
{
    unsigned int loop;

    for (loop = 0; loop < count; loop++) {
        if (list[loop] == obj) {
            if (outIndex) *outIndex = loop;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00452690
int message_is_selected_or_flags4_2(Lego_Object* obj)
{
    if (message_is_unit_selected(obj, NULL) || (obj->flags4 & 0x2)) return 1;
    return 0;
}

// FUNCTION: LEGORR 0x004526c0
void message_select_object(Lego_Object* obj, int noDoubleSelect)
{
    message_select_object2(obj, noDoubleSelect, 1);
}

// FUNCTION: LEGORR 0x004526e0
int message_select_object2(Lego_Object* obj, int noDoubleSelect,
                           int faceCamera)
{
    float faceDir[2];
    float dir[3];

    if (obj->carryingThisObject) obj = obj->carryingThisObject;

    if (message_can_select(obj)) {
        if (!noDoubleSelect && message_is_selected_or_flags4_2(obj)) {
            legoobject_selection_clear_marker_and_reset_hud(obj);
            return 0;
        }
        if (!message_is_unit_selected(obj, NULL) &&
            messageGlobs.selectedUnitCount < MESSAGE_MAXSELECTED) {
            legoobject_run_through_lists_skip_upgrade_parts(
                legoobject_selection_clear_callback, NULL);
            messageGlobs.selectedUnitList[messageGlobs.selectedUnitCount++] = obj;
            obj->flags3 |= 0x200000;
            if (obj->type != 4 && !(obj->flags1 & 0x24002000) &&
                !(obj->flags2 & 0x100)) {
                legoobject_action_sequence_reset(obj, 0);
                aitask_entity_activity_records_refresh((struct EntitySlot*)obj);
                if (obj->type == 2 && faceCamera) {
                    Container_GetOrientation(
                        LEGO_FIELD(s_zoom_step_context, struct Container*, 0x30),
                        NULL, dir, NULL);
                    faceDir[0] = -dir[0];
                    faceDir[1] = -dir[1];
                    legoobject_state_reset(obj, 0, 0);
                    legoobject_turn_to_vector_activity_issue(obj, faceDir);
                }
            }
            if (obj->flags3 & 0x80) {
                if (!(stats_object_get_stats_flags1((struct StatsObject*)obj) &
                      MESSAGE_STATS1_CANBEDRIVEN) || obj->driveObject) {
                    sfx_random_play_sound3d_on_container(
                        legoobject_type_component_get(obj), 0x11, 0, 1, NULL);
                    return 1;
                }
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00452870
int message_can_select(Lego_Object* obj)
{
    if ((obj->flags3 & 0x100) && legoobject_shared_eligibility_check(obj, 1) &&
        !(obj->flags2 & 0xa) && !(obj->flags1 & 0x24000000) &&
        (obj->type != 1 || !(obj->flags1 & 0x40000))) {
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x004528c0
void message_reduce_selected_units(void)
{
    unsigned int loop;
    Lego_Object* obj;
    int deselect;
    int vehicleNoDriver = 0;
    int driverWithFlag = 0;
    int drivableEmpty = 0;
    int drivenNoFlag = 0;
    int building = 0;
    int fence = 0;
    int deselectAll = 0;

    for (loop = 0; loop < messageGlobs.selectedUnitCount; loop++) {
        obj = messageGlobs.selectedUnitList[loop];
        if (obj->type == 2 && obj->driveObject == NULL) {
            vehicleNoDriver = 1;
            break;
        }
        if (stats_object_get_stats_flags1((struct StatsObject*)obj) &
            MESSAGE_STATS1_CANBEDRIVEN) {
            if (obj->driveObject) {
                if (!(stats_object_get_stats_flags1((struct StatsObject*)obj) &
                      0x2000000) &&
                    !(obj->flags4 & 0x40)) {
                    drivenNoFlag = 1;
                } else {
                    driverWithFlag = 1;
                }
            } else {
                drivableEmpty = 1;
            }
        } else if (obj->type == 1) {
            if (!(stats_object_get_stats_flags1((struct StatsObject*)obj) &
                  0x2000000) &&
                !(obj->flags4 & 0x40)) {
                drivenNoFlag = 1;
            } else {
                driverWithFlag = 1;
            }
        } else if (obj->type == 4) {
            building = 1;
        } else if (obj->type == 0xb && (obj->flags2 & 0x2000000)) {
            fence = 1;
        }
    }

    if (vehicleNoDriver) {
        command_context_post_to_hud(9, NULL);
        drivenNoFlag = 0;
        driverWithFlag = 0;
        drivableEmpty = 0;
        building = 0;
        fence = 0;
    } else if (driverWithFlag) {
        command_context_post_to_hud(10, NULL);
        drivenNoFlag = 0;
        drivableEmpty = 0;
        building = 0;
        fence = 0;
    } else if (drivenNoFlag) {
        command_context_post_to_hud(11, NULL);
        drivableEmpty = 0;
        building = 0;
        fence = 0;
    } else if (drivableEmpty) {
        command_context_post_to_hud(12, NULL);
        building = 0;
        fence = 0;
    } else if (building) {
        command_context_post_to_hud(13, NULL);
        fence = 0;
    } else if (fence) {
        command_context_post_to_hud(14, NULL);
    } else {
        deselectAll = 1;
        command_context_post_to_hud(0, NULL);
    }

    for (loop = 0; loop < messageGlobs.selectedUnitCount; loop++) {
        obj = messageGlobs.selectedUnitList[loop];
        deselect = 0;
        if (vehicleNoDriver && (obj->type != 2 || obj->driveObject != NULL)) {
            deselect = 1;
        } else if (driverWithFlag) {
            if (stats_object_get_stats_flags1((struct StatsObject*)obj) &
                MESSAGE_STATS1_CANBEDRIVEN) {
                if (!(stats_object_get_stats_flags1((struct StatsObject*)obj) &
                      0x2000000) &&
                    !(obj->flags4 & 0x40))
                    deselect = 1;
                if (obj->driveObject == NULL) deselect = 1;
            } else if (obj->type != 1) {
                deselect = 1;
            }
        } else if (drivenNoFlag) {
            if (stats_object_get_stats_flags1((struct StatsObject*)obj) &
                MESSAGE_STATS1_CANBEDRIVEN) {
                if ((stats_object_get_stats_flags1((struct StatsObject*)obj) &
                     0x2000000) ||
                    (obj->flags4 & 0x40))
                    deselect = 1;
                if (obj->driveObject == NULL) deselect = 1;
            } else if (obj->type != 1) {
                deselect = 1;
            }
        } else if (drivableEmpty) {
            if (!(stats_object_get_stats_flags1((struct StatsObject*)obj) &
                  MESSAGE_STATS1_CANBEDRIVEN) ||
                obj->driveObject != NULL)
                deselect = 1;
        } else if (building && obj->type != 4) {
            deselect = 1;
        } else if (fence && (obj->type != 0xb || !(obj->flags2 & 0x2000000))) {
            deselect = 1;
        }

        if (deselect || deselectAll) {
            messageGlobs.selectedUnitCount--;
            messageGlobs.selectedUnitList[loop] =
                messageGlobs.selectedUnitList[messageGlobs.selectedUnitCount];
            obj->flags3 &= ~0x200000;
            loop--;
        }
    }
}

// FUNCTION: LEGORR 0x00452be0
void message_clear_selected_units(void)
{
    unsigned int loop;

    for (loop = 0; loop < messageGlobs.selectedUnitCount; loop++) {
        if (messageGlobs.selectedUnitList[loop]->flags3 & 0x200000)
            messageGlobs.selectedUnitList[loop]->flags4 |= 0x2;
        messageGlobs.selectedUnitList[loop]->flags3 &= ~0x200000;
        messageGlobs.selectedUnitList[loop]->flags4 &= ~0x1;
    }
    messageGlobs.selectedUnitCount = 0;
}

// FUNCTION: LEGORR 0x00452c50
int message_deselect_object(Lego_Object* obj)
{
    unsigned int index;

    if (messageGlobs.selectedUnitCount > 0) {
        if (message_is_unit_selected(obj, &index)) {
            if (index == 0) command_context_post_to_hud(0, NULL);
            messageGlobs.selectedUnitList[index] =
                messageGlobs.selectedUnitList[--messageGlobs.selectedUnitCount];
            obj->flags3 &= ~0x200000;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00452cc0
unsigned int message_kill_all_selected_units(void)
{
    unsigned int loop;

    for (loop = messageGlobs.selectedUnitCount; loop > 0; loop--) {
        if ((stats_object_get_stats_flags1(
                 (struct StatsObject*)messageGlobs.selectedUnitList[0]) &
             MESSAGE_STATS1_CANBEDRIVEN) &&
            messageGlobs.selectedUnitList[0]->driveObject) {
            messageGlobs.selectedUnitList[0]->driveObject->flags2 &= ~0x8;
            messageGlobs.selectedUnitList[0]->driveObject->driveObject = NULL;
        }
        if (messageGlobs.selectedUnitList[0]->type == 4) {
            entity_destroy_tracked_pointer_cleanup(messageGlobs.selectedUnitList[0]);
        } else {
            if (messageGlobs.selectedUnitList[0]->type == 0xb)
                electric_fence_remove(
                    (struct ElectricFenceObject*)messageGlobs.selectedUnitList[0]);
            legoobject_remove(messageGlobs.selectedUnitList[0]);
        }
    }
    return messageGlobs.selectedUnitCount;
}

// FUNCTION: LEGORR 0x00452d60
int message_enter_first_person_view(unsigned int cameraFrame)
{
    Lego_Object* obj = (Lego_Object*)legoGlobs_selectedObject;
    unsigned int loop;

    if (obj && (obj->flags3 & 0x10) && (obj->type == 2 || obj->driveObject)) {
        lego_sound_minimum_distance_update_mode(0, obj, cameraFrame);
        return 1;
    }
    for (loop = 0; loop < messageGlobs.selectedUnitCount; loop++) {
        obj = messageGlobs.selectedUnitList[loop];
        if (obj->type == 2 && obj->driveObject) obj = obj->driveObject;
        if ((obj->flags3 & 0x10) && (obj->type == 2 || obj->driveObject)) {
            lego_sound_minimum_distance_update_mode(0, obj, cameraFrame);
            return 1;
        }
    }
    return 0;
}
