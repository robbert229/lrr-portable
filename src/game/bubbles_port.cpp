// SPDX-License-Identifier: LGPL-3.0-or-later
/* The port tail of GAME-005 Bubbles.c (ADR 0006 decision 4; finding 598):
 * the port's bubble names, registries and screen-effect tick, unannotated.
 * Bubbles.c itself is rebuilt in original shape in bubbles.c.
 *
 * In the matching build (MSVC 5.0) bubbles.c defines most of these names
 * itself (bubbles_link_names.h), so their definitions here are compiled out
 * there (LRR_BUBBLES_PORT_NAMES). bubble_action_name_to_index and
 * pending_type_value_slot_select, whose port names have C++ linkage and the
 * port's contracts, stay in every build.
 * Relocated from the verbatim bubbles.cpp (body hashes:
 * docs/coverage/relocations/GAME-005.csv). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ai.hpp"
#include "cmd.hpp"
#include "entity.hpp"
#include "building_scene.hpp"
#include "actor_scene.hpp"
#include "level.hpp"
#include "terrain_grid.hpp"
#include "mission.hpp"
#include "select.hpp"
#include "gods98/view.hpp"
#include "gods98/sound.hpp"
#include "crt/crt.hpp"
#include "gods98/block.hpp"
#include "gods98/string.hpp"
#include "gods98/message.hpp"
#include "util.hpp"
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include "gods98/texture.hpp"
#include "heap.hpp"
#include "crt/crt_for_bubbles.hpp"
#include "effects.hpp"
#include "spawn.hpp"
#include "frontend.hpp"
#include "gods98/engine.hpp"
#include "gods98/media.hpp"
#include "port_loop_ports.hpp"
/* C linkage for AITask.c (aitask.c, ADR 0005 step 6, finding 570): the
 * frozen header below redeclares idle_register and
 * cmd_operation_group_append_object and inherits it. */
extern "C" void idle_register(EntitySlot* e);
extern "C" void cmd_operation_group_append_object(uint32_t key, EntitySlot* entity);
#include "cmd_for_bubbles.hpp"
#include "combat_ray_ports.hpp"
#include "simtick.hpp"
#include "gods98/record.hpp"
#include "gods98/scene_pool.hpp"
#include "gods98/scene_resources.hpp"
#include "gods98/surface.hpp"
#include "gods98/resource.hpp"
#include "weapon.hpp"
#include "bezier.h"
#include <stddef.h>
#include <stdint.h>
#include "type_config_ports.hpp"
#include "entity_pose_delta.hpp"
#include "entity_motion_ports.hpp"
#include "activity_timeout.hpp"
#include "entity_motion_probe.hpp"
#include "simulation_ui_ports.hpp"
#include "type_motion_scale.hpp"
#include "entity_quad_collision.hpp"
#include "gods98/model.hpp"
#include "upgrades.hpp"
#include "entity_for_bubbles.hpp"
#include <locale.h>
#include "util_for_bubbles.hpp"
#include "ai_for_bubbles.hpp"
#include "effects_for_bubbles.hpp"

#define AI_REG_FLAGGED_SEED  0x42160000u
#define AI_REG_TYPE2_SEED    0x4a095440u

/* The port's definitions of the names the unit takes in the matching build
 * (bubbles_link_names.h): every build but MSVC 5.0's. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define LRR_BUBBLES_PORT_NAMES 0
#else
#define LRR_BUBBLES_PORT_NAMES 1
#endif

#if LRR_BUBBLES_PORT_NAMES /* bubbles.c serves these names in the matching build */
static const char s_bubble_request[] = "Bubble_Request";
static const char s_bubble_recharge[] = "Bubble_Recharge";
static const char s_bubble_train[] = "Bubble_Train";
static const char s_bubble_build_path[] = "Bubble_BuildPath";
static const char s_bubble_upgrade[] = "Bubble_Upgrade";
static const char s_bubble_drive[] = "Bubble_Drive";
static const char s_bubble_eat[] = "Bubble_Eat";
static const char s_bubble_elec_fence[] = "Bubble_ElecFence";
static const char s_bubble_call_to_arms[] = "Bubble_CallToArms";
static const char s_bubble_power_off[] = "Bubble_PowerOff";
static const char s_bubble_flee[] = "Bubble_Flee";
static const char s_bubble_dig[] = "Bubble_Dig";
static const char s_bubble_repair[] = "Bubble_Repair";
static const char s_bubble_drill[] = "Bubble_Drill";
static const char s_bubble_reinforce[] = "Bubble_Reinforce";
static const char s_bubble_dynamite[] = "Bubble_Dynamite";
static const char s_bubble_goto[] = "Bubble_Goto";
static const char s_bubble_carry_elec_fence[] = "Bubble_CarryElecFence";
static const char s_bubble_carry_barrier[] = "Bubble_CarryBarrier";
static const char s_bubble_carry_dynamite[] = "Bubble_CarryDynamite";
static const char s_bubble_carry_stud[] = "Bubble_CarryStud";
static const char s_bubble_carry_ore[] = "Bubble_CarryOre";
static const char s_bubble_carry_crystal[] = "Bubble_CarryCrystal";
static const char s_bubble_collect_bird_scarer[] = "Bubble_CollectBirdScarer";
static const char s_bubble_collect_freezer[] = "Bubble_CollectFreezer";
static const char s_bubble_collect_pusher[] = "Bubble_CollectPusher";
static const char s_bubble_collect_laser[] = "Bubble_CollectLaser";
static const char s_bubble_collect_spanner[] = "Bubble_CollectSpanner";
static const char s_bubble_collect_hammer[] = "Bubble_CollectHammer";
static const char s_bubble_collect_spade[] = "Bubble_CollectSpade";
static const char s_bubble_collect_drill[] = "Bubble_CollectDrill";
static const char s_bubble_collect_barrier[] = "Bubble_CollectBarrier";
static const char s_bubble_collect_elec_fence[] = "Bubble_CollectElecFence";
static const char s_bubble_collect_dynamite[] = "Bubble_CollectDynamite";
static const char s_bubble_collect_stud[] = "Bubble_CollectStud";
static const char s_bubble_collect_ore[] = "Bubble_CollectOre";
static const char s_bubble_collect_crystal[] = "Bubble_CollectCrystal";
static const char s_bubble_idle[] = "Bubble_Idle";
/* The bubble names (.rdata 0x004a19cc-0x004a1ca4) are bubbles.c's now. */
static const char s_bubble_cant_do[] = "Bubble_CantDo";
#endif /* LRR_BUBBLES_PORT_NAMES */

static int s_bubble_action_names_initialized;

static float s_screen_effect_pulse_timer;

void idle_register(EntitySlot* e);

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void bubble_action_names_ensure_initialized(void);

#if LRR_BUBBLES_PORT_NAMES /* bubbles.c serves these names in the matching build */
/* Port model of 0x004070b0 */
void bubble_action_strings_initialize(void)
{
    s_bubble_action_strings[0] = s_bubble_cant_do;
    s_bubble_action_strings[1] = s_bubble_idle;
    s_bubble_action_strings[2] = s_bubble_collect_crystal;
    s_bubble_action_strings[3] = s_bubble_collect_ore;
    s_bubble_action_strings[4] = s_bubble_collect_stud;
    s_bubble_action_strings[5] = s_bubble_collect_dynamite;
    s_bubble_action_strings[7] = s_bubble_collect_elec_fence;
    s_bubble_action_strings[6] = s_bubble_collect_barrier;
    s_bubble_action_strings[8] = s_bubble_collect_drill;
    s_bubble_action_strings[9] = s_bubble_collect_spade;
    s_bubble_action_strings[10] = s_bubble_collect_hammer;
    s_bubble_action_strings[11] = s_bubble_collect_spanner;
    s_bubble_action_strings[12] = s_bubble_collect_laser;
    s_bubble_action_strings[13] = s_bubble_collect_pusher;
    s_bubble_action_strings[14] = s_bubble_collect_freezer;
    s_bubble_action_strings[15] = s_bubble_collect_bird_scarer;
    s_bubble_action_strings[16] = s_bubble_carry_crystal;
    s_bubble_action_strings[17] = s_bubble_carry_ore;
    s_bubble_action_strings[18] = s_bubble_carry_stud;
    s_bubble_action_strings[19] = s_bubble_carry_dynamite;
    s_bubble_action_strings[20] = s_bubble_carry_barrier;
    s_bubble_action_strings[21] = s_bubble_carry_elec_fence;
    s_bubble_action_strings[22] = s_bubble_goto;
    s_bubble_action_strings[23] = s_bubble_dynamite;
    s_bubble_action_strings[24] = s_bubble_reinforce;
    s_bubble_action_strings[25] = s_bubble_drill;
    s_bubble_action_strings[26] = s_bubble_repair;
    s_bubble_action_strings[27] = s_bubble_dig;
    s_bubble_action_strings[28] = s_bubble_flee;
    s_bubble_action_strings[29] = s_bubble_power_off;
    s_bubble_action_strings[30] = s_bubble_call_to_arms;
    s_bubble_action_strings[31] = s_bubble_elec_fence;
    s_bubble_action_strings[32] = s_bubble_eat;
    s_bubble_action_strings[33] = s_bubble_drive;
    s_bubble_action_strings[34] = s_bubble_upgrade;
    s_bubble_action_strings[35] = s_bubble_build_path;
    s_bubble_action_strings[36] = s_bubble_train;
    s_bubble_action_strings[37] = s_bubble_recharge;
    s_bubble_action_strings[38] = s_bubble_request;
}

/* BubbleConfigTextureLoad (FUN_00407240): walk Bubbles entries, filter
 * names with the global 0x02000000 gate, map the action to its fixed slot,
 * and retain the loaded resource handle before initializing it. */
/* Port model of 0x00407240 */
void bubble_config_texture_load(BlockFile* cfg)
{
    BlockRec* group = block_subgroup(cfg,
        string_path_join(util_root_name(cfg), "Bubbles", (const char*)NULL));
    BlockRec* entry = group;
    while (entry != NULL) {
        const char* name = block_name(entry);
        const char* resource_name = block_value(entry);
        int enabled = 1;
        if (name != NULL && name[0] == '!') {
            if ((s_bubble_config_global_flags & 0x02000000u) != 0u) {
                enabled = 0;
            }
            else {
                ++name;
            }
        }
        if (enabled && name != NULL && resource_name != NULL
            && s_bubble_texture_load != NULL) {
            const int index = bubble_action_name_to_index(name);
            if (index >= 0 && index < BUBBLE_ACTION_NAME_COUNT) {
                const uint32_t handle = s_bubble_texture_load(
                    s_bubble_texture_context, resource_name, 0, 0);
                s_bubble_texture_handles[index] = handle;
                if (handle != 0u && s_bubble_texture_initialize != NULL) {
                    s_bubble_texture_initialize(s_bubble_texture_context,
                                                handle);
                }
            }
        }
        entry = block_next_sibling(entry);
    }
}
/* BubbleActionNameToIndex (FUN_00407300): the source compares the input with
 * all 39 initialized names and returns the matching slot. On a miss it leaves
 * the final _strcmpi result in EAX rather than returning a -1 sentinel. */
#endif /* LRR_BUBBLES_PORT_NAMES */
/* Kept in every build: the unit keeps its own name (bubbles_link_names.h). */
/* Port model of 0x00407300 */
int bubble_action_name_to_index(const char* name)
{
    int last_comparison = 0;
    if (name == NULL) {
        return -1;
    }
    bubble_action_names_ensure_initialized();
    for (int i = 0; i < BUBBLE_ACTION_NAME_COUNT; ++i) {
        const char* candidate = s_bubble_action_names[i];
        last_comparison = strcasecmp_lite(candidate, name);
        if (last_comparison == 0) return i;
    }
    return last_comparison;
}

#if LRR_BUBBLES_PORT_NAMES /* bubbles.c serves these names in the matching build */
/* Port model of 0x00407340 */
void state_flag_toggle_when_zero(void)
{
    s_task_refresh_flag = (s_task_refresh_flag == 0u) ? 1u : 0u;
}

/* Port model of 0x00407360 */
uint32_t global_task_refresh_flag_get(void)
{
    return s_task_refresh_flag;
}

/* ResetObjectAuxiliaryState (FUN_00407370): the global refresh gate clears
 * the auxiliary word then refreshes its selected per-entity value slot. */
/* Port model of 0x00407370 */
void entity_reset_auxiliary_state(EntitySlot* e)
{
    if (e == NULL || global_task_refresh_flag_get() == 0u) {
        return;
    }
    e->auxiliary_state_388 = 0.0f;
    pending_type_value_slot_select(e, &e->auxiliary_slot_38c);
}

/* Port model of 0x004073a0 */
void entity_pointer_registries_remove(EntitySlot* record)
{
    int i;
    for (i = 0; i < AI_REG_FLAGGED_SLOTS; ++i) {
        if (s_reg_flagged[i].record == (void*)record) {
            s_reg_flagged[i].record = NULL;
        }
    }
    idle_queue_clear(record); /* window B rides the cmd idle table */
    for (i = 0; i < AI_REG_TYPE4_SLOTS; ++i) {
        if (s_reg_type4[i].record == (void*)record) {
            s_reg_type4[i].record = NULL;
        }
    }
    for (i = 0; i < AI_REG_TYPE2_SLOTS; ++i) {
        if (s_reg_type2[i].record == (void*)record) {
            s_reg_type2[i].record = NULL;
        }
    }
}

/* Port model of 0x00407410 */
void type4_object_registry_insert(EntitySlot* record)
{
    if (record == NULL || record->type != 4) {
        return; /* deviation: the corpus strides the type word blindly */
    }
    int i;
    for (i = 0; i < AI_REG_TYPE4_SLOTS; ++i) {
        if (s_reg_type4[i].record == (void*)record) {
            return; /* the corpus's no-duplicate rule */
        }
    }
    for (i = 0; i < AI_REG_TYPE4_SLOTS; ++i) {
        if (s_reg_type4[i].record == NULL) {
            s_reg_type4[i].record = (void*)record; /* timer word rides on */
            return;
        }
    }
    /* deviation: the corpus's bounded empty scan simply drops the record on
     * a full table; the port's scan drops it too. */
}

/* Port model of 0x00407450 */
void idle_register(EntitySlot* e)
{
    if (e == NULL || e->type != 2) {
        return;
    }
    int i = 0;
    while (i < IDLE_TABLE_MAX && s_idle[i].entity != e) {
        ++i;
    }
    if (i == IDLE_TABLE_MAX) {
        for (i = 0; i < IDLE_TABLE_MAX; ++i) {
            if (s_idle[i].entity == NULL) {
                s_idle[i].entity = e;
                break;
            }
        }
    }
    if (i < IDLE_TABLE_MAX) {
        s_idle[i].timer = 12.5f; /* corpus stores 0x41480000 at +4 */
    }
}

/* Port model of 0x004074b0 */
void type2_timed_registry_insert(EntitySlot* record)
{
    if (record == NULL || record->type != 2) {
        return; /* deviation: the corpus strides the type word blindly */
    }
    int i;
    for (i = 0; i < AI_REG_TYPE2_SLOTS; ++i) {
        if (s_reg_type2[i].record == (void*)record) {
            s_reg_type2[i].timer = AI_REG_TYPE2_SEED; /* re-seed the hit */
            return;
        }
    }
    for (i = 0; i < AI_REG_TYPE2_SLOTS; ++i) {
        if (s_reg_type2[i].record == NULL) {
            s_reg_type2[i].record = (void*)record;
            s_reg_type2[i].timer = AI_REG_TYPE2_SEED;
            return;
        }
    }
    /* the corpus drops the record on the full table */
}

/* Port model of 0x00407510 */
void set_type2_object_record_value(EntitySlot* record, uint32_t value)
{
    if (record == NULL || record->type != 2) {
        return; /* deviation: the corpus strides the type word blindly */
    }
    for (int i = 0; i < AI_REG_TYPE2_SLOTS; ++i) {
        if (s_reg_type2[i].record != NULL) {
            s_reg_type2[i].timer = value;
        }
    }
}

/* Port model of 0x00407540 */
void flagged_object_timer_registry_insert(EntitySlot* record)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    if ((entity_capability_flags_get(record) & 0x80u) == 0u) {
        return;
    }
    int i;
    for (i = 0; i < AI_REG_FLAGGED_SLOTS; ++i) {
        if (s_reg_flagged[i].record == (void*)record) {
            s_reg_flagged[i].timer = AI_REG_FLAGGED_SEED; /* re-seed the hit */
            return;
        }
    }
    for (i = 0; i < AI_REG_FLAGGED_SLOTS; ++i) {
        if (s_reg_flagged[i].record == NULL) {
            s_reg_flagged[i].record = (void*)record;
            s_reg_flagged[i].timer = AI_REG_FLAGGED_SEED;
            return;
        }
    }
    /* the corpus drops the record on the full table */
}

/* Port model of 0x004075a0 */
void screen_effects_tick(float delta)
{
    static const int capacities[3] = { 20, 20, 50 };
    const int mode_zero = current_engine_mode_is_zero() ? 1 : 0;
    int paused = s_screen_effect_tick_hooks.simulation_paused != NULL
        ? s_screen_effect_tick_hooks.simulation_paused(
              s_screen_effect_tick_hooks.context) : mode_zero;
    int pool;
    int slot;
    /* The flagged-object ring window is owned by the AI registry module
     * (FUN_00407540); TickEntityScreenEffects advances its float timer by
     * reinterpreting the retained seed dword, clearing the pointer on expiry. */
    for (slot = 0; slot < 20; ++slot) {
        void* entity = NULL;
        uint32_t timer_bits = 0u;
        float timer;
        ai_object_registry_slot_peek(0, slot, &entity, &timer_bits);
        if (entity == NULL) continue;
        memcpy(&timer, &timer_bits, sizeof(timer));
        timer -= delta;
        memcpy(&timer_bits, &timer, sizeof(timer_bits));
        if (timer < 0.0f) entity = NULL;
        ai_object_registry_slot_set(0, slot, entity, timer_bits);
    }
    if (paused == 0) {
        if (s_screen_effect_pulse_timer >= 12.5f) {
            if (s_screen_effect_pulse_timer >= 25.0f) {
                s_screen_effect_pulse_timer -= 25.0f;
            } else if (s_screen_effect_tick_hooks.pulse_tick != NULL) {
                /* TickEntityScreenEffects' 20-entry pulse window is the
                 * type-4 registry (DAT_00557b24), not the independent
                 * 50-entry effect sidecar. Use the registry populated by
                 * Type4ObjectRegistryInsert (0x407410). */
                const int pulse_count = ai_object_registry_window_count(1);
                for (slot = 0; slot < pulse_count; ++slot) {
                    void* entity = NULL;
                    ai_object_registry_slot_peek(1, slot, &entity, NULL);
                    if (entity != NULL)
                        s_screen_effect_tick_hooks.pulse_tick(entity, delta,
                            s_screen_effect_tick_hooks.context);
                }
            }
        }
        s_screen_effect_pulse_timer += delta;
    }
    for (pool = 0; pool < 3; ++pool) {
        for (slot = 0; slot < capacities[pool]; ++slot) {
            struct ScreenEffectTickRecord* record =
                &s_screen_effect_tick_records[pool][slot];
            if (record->entity == NULL) continue;
            if (paused == 0 &&
                (s_screen_effect_tick_hooks.entity_eligible == NULL ||
                 s_screen_effect_tick_hooks.entity_eligible(record->entity,
                    pool, s_screen_effect_tick_hooks.context)) &&
                s_screen_effect_tick_hooks.entity_tick != NULL) {
                s_screen_effect_tick_hooks.entity_tick(record->entity, pool,
                    delta, s_screen_effect_tick_hooks.context);
            }
            if (pool < 2) {
                record->remaining -= delta;
                if (record->remaining < 0.0f) record->entity = NULL;
            } else if (record->remaining <= 0.0f) {
                record->entity = NULL;
            }
        }
    }
}

/* PendingTypeValueSlotSelect (FUN_00407a10): the config-fed DAT words are
 * zero-init model slots; this selection retains the exact flag precedence
 * and the command/activity families that choose among them. */
#endif /* LRR_BUBBLES_PORT_NAMES */
/* Kept in every build: the unit keeps its own name (bubbles_link_names.h). */
/* Port model of 0x00407a10 */
void pending_type_value_slot_select(const EntitySlot* e, uint32_t* out)
{
#if defined(_WIN32) && defined(_M_IX86)
    int* record = *(int**)((uint8_t*)e + 0x2f0);
    int type;
    if ((*(uint8_t*)((uint8_t*)e + 0x3ec) & 0x20u) != 0u) {
        *out = s_bubble_texture_handles[30];
        return;
    }
    if ((*(uint32_t*)((uint8_t*)e + 0x3e0) & 0x800000u) != 0u) {
        *out = s_bubble_texture_handles[28];
        return;
    }
    if ((*(uint32_t*)((uint8_t*)e + 0x3e0) & 0x80000000u) != 0u) {
        *out = s_bubble_texture_handles[0];
        return;
    }
    if (record == NULL) return;
    type = record[0];
    while (type == 0xe &&
           (record = *(int**)((uint8_t*)record + 0x60)) != NULL) {
        type = record[0];
    }
    switch (type) {
    case 0:
        *out = s_bubble_texture_handles[22];
        return;
    case 3: {
        int* payload = *(int**)((uint8_t*)record + 0x10);
        if (payload[0] == 6) *out = s_bubble_texture_handles[2];
        else if (payload[0] == 7) {
            if (payload[1] != 0) *out = s_bubble_texture_handles[4];
            else *out = s_bubble_texture_handles[3];
            return;
        }
        else if (payload[0] == 8) *out = s_bubble_texture_handles[5];
        else if (payload[0] == 9) *out = s_bubble_texture_handles[6];
        else if (payload[0] == 0xb) *out = s_bubble_texture_handles[7];
        else break;
        return;
    }
    case 5: {
        int* linked = *(int**)((uint8_t*)e + 0x308);
        if (linked == NULL) break;
        type = linked[0];
        if (type == 6) *out = s_bubble_texture_handles[16];
        else if (type == 7) {
            if (linked[1] != 0) *out = s_bubble_texture_handles[18];
            else *out = s_bubble_texture_handles[17];
            return;
        }
        else if (type == 8) *out = s_bubble_texture_handles[19];
        else if (type == 9) *out = s_bubble_texture_handles[20];
        else if (type == 0xb) *out = s_bubble_texture_handles[21];
        else break;
        return;
    }
    case 7: *out = s_bubble_texture_handles[38]; return;
    case 8: *out = s_bubble_texture_handles[25]; return;
    case 9: *out = s_bubble_texture_handles[23]; return;
    case 10: *out = s_bubble_texture_handles[26]; return;
    case 0xb: *out = s_bubble_texture_handles[24]; return;
    case 0xc: *out = s_bubble_texture_handles[27]; return;
    case 0xf: *out = s_bubble_texture_handles[31]; return;
    case 0x10:
    case 0x11: *out = s_bubble_texture_handles[32]; return;
    case 0x12: *out = s_bubble_texture_handles[33]; return;
    case 0x13:
        type = record[0xe];
        if (type == 0) *out = s_bubble_texture_handles[8];
        else if (type == 1) *out = s_bubble_texture_handles[9];
        else if (type == 2) *out = s_bubble_texture_handles[10];
        else if (type == 3) *out = s_bubble_texture_handles[11];
        else if (type == 4) *out = s_bubble_texture_handles[12];
        else if (type == 5) *out = s_bubble_texture_handles[13];
        else if (type == 7) *out = s_bubble_texture_handles[14];
        else if (type == 6) *out = s_bubble_texture_handles[15];
        else break;
        return;
    case 0x15: *out = s_bubble_texture_handles[34]; return;
    case 0x16: *out = s_bubble_texture_handles[35]; return;
    case 0x17: *out = s_bubble_texture_handles[36]; return;
    case 0x1b: *out = s_bubble_texture_handles[37]; return;
    }
#else
    if (e == NULL || out == NULL) {
        return;
    }
    uint32_t address = 0u;
    if ((e->flags_3ec & 0x20u) != 0u) {
        address = 0x005578dcu;
    }
    else if ((e->flags_3e0 & 0x800000u) != 0u) {
        address = 0x005578d4u;
    }
    else if ((e->flags_3e0 & 0x80000000u) != 0u) {
        address = 0x00557864u;
    }
    else {
        const void* record = e->cmd_head;
        while (record != NULL && cmd_rec_type(record) == 0xe) {
            record = cmd_rec_next(record);
        }
        if (record == NULL) {
            return;
        }
        const int type = cmd_rec_type(record);
        switch (type) {
        case 0: address = 0x005578bcu; break;
        case 3: {
            const uint32_t* payload = (const uint32_t*)cmd_rec_payload(record);
            if (payload == NULL) break;
            switch (payload[0]) {
            case 6: address = 0x0055786cu; break;
            case 7: address = payload[1] != 0u
                ? 0x00557874u : 0x00557870u; break;
            case 8: address = 0x00557878u; break;
            case 9: address = 0x0055787cu; break;
            case 11: address = 0x00557880u; break;
            default: break;
            }
            break;
        }
        case 5: {
            const EntitySlot* linked = (const EntitySlot*)e->carry_link_308;
            if (linked == NULL) break;
            switch (linked->type) {
            case 6: address = 0x005578a4u; break;
            case 7: address = linked->etype != 0
                ? 0x005578acu : 0x005578a8u; break;
            case 8: address = 0x005578b0u; break;
            case 9: address = 0x005578b4u; break;
            case 11: address = 0x005578b8u; break;
            default: break;
            }
            break;
        }
        case 7: address = 0x005578fcu; break;
        case 8: address = 0x005578c8u; break;
        case 9: address = 0x005578c0u; break;
        case 10: address = 0x005578ccu; break;
        case 11: address = 0x005578c4u; break;
        case 12: address = 0x005578d0u; break;
        case 15: address = 0x005578e0u; break;
        case 16: case 17: address = 0x005578e4u; break;
        case 18: address = 0x005578e8u; break;
        case 19: {
            const int activity = (int)cmd_rec_value_38(record);
            if (activity >= 0 && activity <= 7) {
                address = 0x00557884u + (uint32_t)activity * 4u;
                if (activity == 6) address = 0x005578a0u;
                if (activity == 7) address = 0x0055789cu;
            }
            break;
        }
        case 21: address = 0x005578ecu; break;
        case 22: address = 0x005578f0u; break;
        case 23: address = 0x005578f4u; break;
        case 27: address = 0x005578f8u; break;
        default: break;
        }
    }
    if (address != 0u) {
        *out = pending_type_value_get(address);
    }
#endif
}

static void bubble_action_names_ensure_initialized(void)
{
    int i;
    if (s_bubble_action_names_initialized) return;
    for (i = 0; i < BUBBLE_ACTION_NAME_COUNT; ++i) {
        const char* name = bubble_action_string_get((unsigned int)i);
        if (name != NULL) bubble_action_name_register(i, name);
    }
    s_bubble_action_names_initialized = 1;
}
