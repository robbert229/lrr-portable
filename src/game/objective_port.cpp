// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-029 Objective.c's port models (the port tail, ADR 0006 decision 4).
 * Relocated verbatim by tools/relocate_unit.py (ADR 0006; body hashes:
 * docs/coverage/relocations/GAME-029.csv) as objective_unit.cpp, which
 * finding 631 renamed when it rebuilt the unit in original shape as
 * objective_unit.c. The models are unannotated: natively they are the live
 * objective code. In the matching build the unit serves the names its
 * callers use (objective_unit_link_names.h), and this file compiles its
 * own definitions of them out (LRR_OBJECTIVE_PORT_NAMES). */
/* Matching build: LegoObject.c callers call legoobject_unit.c (#575). */
/* Matching build: Lego.c callers call lego_unit.c (#587). */
#include "lego_port_aliases.h"
#include "legoobject_port_aliases.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gods98/block.hpp"
#include "gods98/engine.hpp"
#include "gods98/media.hpp"
#include "gods98/record.hpp"
#include "gods98/scene_resources.hpp"
#include "gods98/string.hpp"
#include "gods98/texture.hpp"
#include "gods98/surface.hpp"
#include "gods98/sound.hpp"
#include "gods98/view.hpp"
#include "gods98/resource.hpp"
#include "level.hpp"
#include "game.hpp"
#include "objective.hpp"
#include "entity.hpp"
#include "ai.hpp"
#include "cmd.hpp"
#include "select.hpp"
#include "spawn.hpp"
#include "terrain_grid.hpp"
#include "savegame.hpp"
#include "mission.hpp"
#include "gods98/message.hpp"
#include "simulation_ui_ports.hpp"
#include "port_loop_ports.hpp"
#include "frontend.hpp"
#include "effects.hpp"
#include "util.hpp"
#include "crt/crt.hpp"
#include "objective_for_objective_unit.hpp"
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include "heap.hpp"
#include "crt/crt_for_objective_unit.hpp"
#include <stdarg.h>
#include <time.h>
#include "frontend.h"
#include "terrain_feedback.hpp"
#include "gods98/audio.hpp"
#include "gods98/simtick.hpp"
#include "gods98/flic.hpp"
#include "gods98/input.hpp"
#include "priority_config_ports.hpp"
#include "tutorial_hud_ports.hpp"
#include "ui_action_dispatch.hpp"
#include "camera_drag.hpp"
#include "screen_pick.hpp"
#include "pointer_cursor.hpp"
#include "ui_list.hpp"
#include "frontend_for_objective_unit.hpp"
#include <stdint.h>
#include <limits.h>
#include "gods98/camera.hpp"
#include "type_config_ports.hpp"
#include "upgrades.hpp"
#include "scene_render.hpp"
#include "level_for_objective_unit.hpp"

/* The port's definitions of the names the unit takes in the matching build
 * (objective_unit_link_names.h): every build but MSVC 5.0's. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define LRR_OBJECTIVE_PORT_NAMES 0
#else
#define LRR_OBJECTIVE_PORT_NAMES 1
#endif

typedef struct ObjectiveEntityMatchContext {
    LevelState* level;
    uint32_t flags;
} ObjectiveEntityMatchContext;
/* DAT_004ffbc0 is the runtime result word. It is separate from the level's
 * configured objective mask in LevelState::objective. */
uint32_t s_objective_result_word;

static uint32_t s_value_004ffe48 = 0u;

static float s_objective_voice_stream_timeout;

/* Config key names (finding 22). */
static const char* K_DONT_ADV      = "DontShowObjectiveAdvisor";
static const char* K_DONT_ACH_ADV  = "DontShowObjectiveAcheiveAdvisor";
static const char* K_DONT_FAIL_ADV  = "DontShowObjectiveFailedAdvisor";
static const char* K_ACH_AVI       = "ObjectiveAcheivedAVI";
static const char* K_CRYSTAL_OBJ   = "CrystalObjective";
static const char* K_ORE_OBJ       = "OreObjective";
static const char* K_TIMER_OBJ     = "TimerObjective";
static const char* K_CONSTRUCT_OBJ = "ConstructionObjective";
static const char* K_BLOCK_OBJ     = "BlockObjective";

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static const char* obj_cfg_str(BlockFile* cfg, const char* level, const char* key, char* out, size_t out_sz);
static void obj_set_block(LevelState* s, const int32_t targets[2]);
static void obj_set_construction(LevelState* s, long a, long b);
static void obj_set_resources(LevelState* s, int32_t crystal, int32_t ore);
static void obj_set_timer(LevelState* s, float ticks, int hit_time_fail);
static void obj_split_two(const char* s, int32_t* a, int32_t* b);
static int obj_typed_int(BlockFile* cfg, const char* level, const char* key, int dflt);
static void objective_voice_alias_format(char* out, size_t out_size, const char* outcome, const char* family, const char* level);
static int objective_entity_pair_match(EntitySlot* entity, void* payload);

void objective_load(BlockFile* cfg, const char* level_name, LevelState* s)
{
    if (cfg == NULL || s == NULL) {
        return;
    }
    s_objective_active_level = s;
    s_objective_voice_alias[0] = '\0';
    s_objective_voice_alias_pending = 0;
    s_objective_result_word = 0;
    s_objective_voice_stream_handle = -1;
    s_objective_voice_stream_timeout = 0.0f;
    s_objective_voice_result_active = 0;
    s_objective_tick_resolved = 1;
    /* Zero the 0x54-byte objective slot block (+0xbc..+0x110). */
    for (int o = OBJ_SLOT_HANDLE; o < OBJ_SLOT_HANDLE + 0x54; o += 4) {
        level_set_int(s, o, 0);
    }
    s->objective = 0;
    s->objective_pending = 1;

    /* Advisor-suppression keys: bit set when the key is NOT 1. */
    if (obj_typed_int(cfg, level_name, K_DONT_ADV, 0) != 1) {
        s->objective |= LVL_OBJECTIVE_NO_ADVISOR;
        s_objective_result_word |= LVL_OBJECTIVE_NO_ADVISOR;
    }
    if (obj_typed_int(cfg, level_name, K_DONT_ACH_ADV, 0) != 1) {
        s->objective |= LVL_OBJECTIVE_NO_ACH_ADV;
        s_objective_result_word |= LVL_OBJECTIVE_NO_ACH_ADV;
    }
    if (obj_typed_int(cfg, level_name, K_DONT_FAIL_ADV, 0) != 1) {
        s->objective |= LVL_OBJECTIVE_NO_FAIL_ADV;
        s_objective_result_word |= LVL_OBJECTIVE_NO_FAIL_ADV;
    }

    /* Achieved-AVI slot: the corpus loads an AVI by name and stores dims at
     * level+0xe4/+0xe8, or sets +0xec=1 when the file has no dimensions.
     * Surface/movie handles are the render lane — the dims and the marker
     * are kept for tests. */
    {
        char value[256];
        if (obj_cfg_str(cfg, level_name, K_ACH_AVI, value, sizeof(value)) != NULL) {
            char fields[UTIL_QUERY_MAX_FIELDS][256];
            int st = util_query_fields(value, fields, sizeof(fields[0]));
            if (st == 3) {
            level_set_float(s, OBJ_OFF_AVI_DIM, (float)atoi(fields[1]));
            level_set_float(s, OBJ_OFF_AVI_DIM + 4, (float)atoi(fields[2]));
            } else {
                level_set_int(s, OBJ_OFF_AVI_SECOND, 1);
            }
        }
    }

    /* Resource objectives (finding 22 table). */
    {
        char value[256];
        int32_t crystal = 0, ore = 0;
        if (obj_cfg_str(cfg, level_name, K_CRYSTAL_OBJ, value, sizeof(value)) != NULL) {
            crystal = atoi(value);
        }
        if (obj_cfg_str(cfg, level_name, K_ORE_OBJ, value, sizeof(value)) != NULL) {
            ore = atoi(value);
        }
        obj_set_resources(s, crystal, ore);
    }
    {
        char value[64];
        if (obj_cfg_str(cfg, level_name, K_TIMER_OBJ, value, sizeof(value)) != NULL) {
            const char* mode = strchr(value, ':');
            float ticks = (float)atoi(value) * 25.0f;
            int htf = mode != NULL &&
                strcasecmp_lite(mode + 1, "HitTimeFailObjective") == 0;
            obj_set_timer(s, ticks, htf);
        }
    }
    {
        char value[64];
        if (obj_cfg_str(cfg, level_name, K_CONSTRUCT_OBJ, value, sizeof(value)) != NULL) {
            int32_t a = 0, b = 0;
            obj_split_two(value, &a, &b);
            obj_set_construction(s, a, b);
        }
    }
    {
        char value[64];
        if (obj_cfg_str(cfg, level_name, K_BLOCK_OBJ, value, sizeof(value)) != NULL) {
            int32_t targets[2] = { 0, 0 };
            obj_split_two(value, &targets[0], &targets[1]);
            obj_set_block(s, targets);
        }
    }
    /* ObjectiveText feeds the message registry (corpus FUN_004574e0); not
     * stored on the level block — HUD milestone. */
    (void)K_DONT_ADV;

    if (s_objective_result_word == 0u) {
        s->objective = LVL_OBJECTIVE_NO_ADVISOR;
        s_objective_result_word = LVL_OBJECTIVE_NO_ADVISOR;
    }
}
/* Corpus FUN_00458580 — crystal/ore objective record words. */
static void obj_set_resources_target(uint32_t* level_record, int32_t crystal,
                                     int32_t ore)
{
    if (crystal != 0) {
        s_objective_result_word |= LVL_OBJECTIVE_CRYSTALS;
        level_record[OBJ_OFF_CRYSTAL_TARGET / 4] = (uint32_t)crystal;
    }
    if (ore != 0) {
        s_objective_result_word |= LVL_OBJECTIVE_ORE;
        level_record[OBJ_OFF_ORE_TARGET / 4] = (uint32_t)ore;
    }
}

/* Corpus FUN_004585c0 — block objective record words. */
static void obj_set_block_target(uint32_t* level_record,
                                 const int32_t targets[2])
{
    s_objective_result_word |= LVL_OBJECTIVE_BLOCK;
    level_record[OBJ_OFF_BLOCK_A / 4] = (uint32_t)targets[0];
    level_record[OBJ_OFF_BLOCK_B / 4] = (uint32_t)targets[1];
}

/* Corpus FUN_004585f0 — timer objective record word. */
static void obj_set_timer_target(uint32_t* level_record, float ticks,
                                 int hit_time_fail)
{
    s_objective_result_word |= LVL_OBJECTIVE_TIMER;
    ((float*)level_record)[OBJ_OFF_TIMER_TARGET / 4] = ticks;
    if (hit_time_fail) {
        s_objective_result_word |= LVL_OBJECTIVE_NO_ACH_ADV;
    }
}

/* Corpus FUN_00458620 — construction objective record words. */
static void obj_set_construction_target(uint32_t* level_record,
                                        uint32_t a, uint32_t b)
{
    s_objective_result_word |= LVL_OBJECTIVE_CONSTRUCTION;
    level_record[OBJ_OFF_CONSTRUCT_A / 4] = a;
    level_record[OBJ_OFF_CONSTRUCT_B / 4] = b;
}

/* GlobalFlagGet_004ffe30 (FUN_00458650): threaded objective-result read. */
#if LRR_OBJECTIVE_PORT_NAMES /* objective_unit.c's Objective_HasAchieved in the matching build */
int level_objective_result_get(const LevelState* s)
{
    return s != NULL ? s->objective_result : 0;
}
#endif /* LRR_OBJECTIVE_PORT_NAMES */

#if LRR_OBJECTIVE_PORT_NAMES /* objective_unit.c's Objective_SetEndTeleportEnabled in the matching build */
void global_value_004ffe48_set(uint32_t value)
{
    s_value_004ffe48 = value;
}
#endif /* LRR_OBJECTIVE_PORT_NAMES */

/* Teleporter.c (teleporter.c, finding 621): Teleporter_Start (0046a510) by
 * its original-ABI name. */
extern "C" void scene_object_state_dispatch_original(uint32_t selector,
                                                     uint32_t mode,
                                                     uint32_t flags);

/* The retail function reads the current level from FUN_00431560 and has one
 * stack argument. `objective_load` binds that owner, while mission frame
 * updates rebind the live context before objective processing. */
#if LRR_OBJECTIVE_PORT_NAMES /* objective_unit.c's Objective_SetStatus in the matching build */
void objective_announce_current(int who)
{
    LevelState* s = s_objective_active_level;
    char alias[256];
    uint32_t* engine_state;
    int result = 0;
    const char* outcome = "";
    const char* family = "_Levels::";
    const char* level;
    uint32_t teleportFlags;
    if (s == NULL) return;
    s_objective_active_level = s;
    if ((s_objective_result_word & 7u) != 0 ||
        (s_objective_announce_target_gate != 0 &&
         (s_objective_result_word & 0x100u) != 0)) {
        return;
    }
    s_objective_voice_alias_pending = 0;
    level = s->ptr_name != NULL ? (const char*)s->ptr_name : "";
    switch (who) {
    case 0:
        s_objective_result_word |= 1u;
        s->objective |= LVL_OBJECTIVE_STILL_PENDING;
        break;
    case 1:
        s_objective_result_word |= 2u;
        s->objective_result = 1;
        s->objective |= LVL_OBJECTIVE_ACHIEVED;
        (void)mission_reward_success_snapshot_prepare(s);
        unported_ledger_stubs::ledger_stub_004263c0_GlobalSubsystemStateReset();
        outcome = "Acheived";
        result = 1;
        break;
    case 2:
        s_objective_result_word |= 4u;
        s->objective_result = 0;
        s->objective |= LVL_OBJECTIVE_FAILED;
        unported_ledger_stubs::ledger_stub_004263c0_GlobalSubsystemStateReset();
        outcome = "Failed";
        result = 2;
        break;
    case 3:
        s_objective_result_word |= 0x104u;
        s->objective_result = 0;
        s->objective |= LVL_OBJECTIVE_FAILED | LVL_OBJECTIVE_CRYSTALS;
        unported_ledger_stubs::ledger_stub_004263c0_GlobalSubsystemStateReset();
        outcome = "FailedCrystals";
        family = "";
        level = "";
        result = 2;
        break;
    default:
        s_objective_result_word |= 8u;
        s->objective |= LVL_OBJECTIVE_ANNOUNCED;
        return;
    }
    /* FUN_00458670 calls StartLevelSelected after the shared subsystem
     * reset for each terminal result. The portable implementation owns the
     * active GameContext through the entity-pool callback binding. */
    if (result != 0) {
        start_level_selected(entity_pool_callback_context_get());
    }
    level_set_int(s, OBJ_OFF_RESULT_INDEX, result);
    s_objective_result_word |= 8u;
    s->objective |= LVL_OBJECTIVE_ANNOUNCED;
    if (result != 0) {
        /* Teleporter_Start (0046a510, teleporter.c): teleport the pilots,
         * the minifigures, the buildings and the electric fences out
         * through a service, at random (flags 2) when DAT_004ffe48 is set,
         * else removed (flags 4). Natively the name is the port tail's
         * adapter (record_chains_port.cpp). */
        teleportFlags = s_value_004ffe48 ? 2u : 4u;
        scene_object_state_dispatch_original(1u, 2u, teleportFlags);
        scene_object_state_dispatch_original(2u, 2u, teleportFlags);
        scene_object_state_dispatch_original(8u, 2u, teleportFlags);
        scene_object_state_dispatch_original(0x400u, 2u, teleportFlags);
        level_state_stored_value_set(1u);
        lego::scatter_spawn_gate_set(1);
    }
    engine_state = engine_current_state_word();
    if (engine_state != NULL && (*engine_state & 0x10u) != 0u) {
        /* FUN_00458670 only resolves a result stream while bit 0x10 of the
         * engine state word (DAT_00556ecc) enables the advisor overlay. */
        objective_voice_alias_format(alias, sizeof(alias), outcome, family,
                                     level);
        strncpy(s_objective_voice_alias, alias,
                sizeof(s_objective_voice_alias) - 1u);
        s_objective_voice_alias[sizeof(s_objective_voice_alias) - 1u] = '\0';
        strncpy(s->obj_stream, s_objective_voice_alias,
                sizeof(s->obj_stream) - 1u);
        s->obj_stream[sizeof(s->obj_stream) - 1u] = '\0';
        s_objective_voice_alias_pending = 1;
    } else {
        s_objective_voice_alias[0] = '\0';
        s->obj_stream[0] = '\0';
    }
}
#endif /* LRR_OBJECTIVE_PORT_NAMES */

namespace unported_ledger_stubs {
void input_device_release(void)
{
    InputDeviceReleaseHooks* hooks = &s_input_device_release_hooks;
    uint32_t flags = s_input_device_release_flags;
    if ((flags & 1u) != 0u) {
        if (hooks->primary_release != NULL) {
            hooks->primary_release(hooks->context);
        }
        s_input_device_release_flags &= ~1u;
        if ((s_input_device_system_flags & 0x800u) != 0u) {
            if (hooks->secondary_reenable != NULL) {
                hooks->secondary_reenable(hooks->context, 1);
            }
            s_input_device_system_flags &= ~0x800u;
        }
    }
    else if ((flags & 2u) != 0u) {
        s_input_device_release_flags &= ~2u;
    }
    else if ((flags & 4u) != 0u) {
        s_input_device_release_flags &= ~0x104u;
    }

    if (hooks->advisor_clear != NULL) hooks->advisor_clear(hooks->context);
    if (s_input_device_active_track != -1) {
        if (hooks->stream_stop != NULL) {
            hooks->stream_stop(hooks->context, 0);
        }
        s_input_device_active_track = -1;
    }
    if (hooks->transition_control != NULL) {
        hooks->transition_control(hooks->context, 0, 0);
    }
    if (hooks->sound_cleanup != NULL) {
        hooks->sound_cleanup(hooks->context, 0);
    }
    if (hooks->system_state_get != NULL &&
        hooks->system_state_get(hooks->context, 0) == 0) {
        s_input_device_system_flags &= ~0x1000u;
    }
}
} // namespace unported_ledger_stubs

/* GlobalOptionLowBitsEnabled (FUN_004589a0): faithful gate over the
 * objective word's low three bits. */
int global_option_low_bits_enabled(const LevelState* s)
{
    return s != NULL && (s->objective & 7u) != 0u;
}

/* ObjectiveTick @ 0x00458be0 has four arguments in the retail caller:
 * (opaque global owner, level block, objective-check dt, frame dt). */
#if LRR_OBJECTIVE_PORT_NAMES /* objective_unit.c's Objective_Update in the matching build */
void objective_tick_target(void* engine, LevelState* s,
                           float checker_dt, float frame_dt)
{
    int result_active_next = 0;
    (void)engine; /* the renderer/global owner is not represented here */
    /* Retail releases input-device/advisor/audio state before the objective
     * transition logic in loader and benchmark modes (app mode > 3). */
    if (engine_app_mode_get() > 3) {
        unported_ledger_stubs::input_device_release();
    }
    if (s == NULL) {
        s_objective_tick_resolved = 1;
        return;
    }
    s_objective_active_level = s;
    /* The result word's announcement bit is consumed only in one of the
     * three result branches.  The retail path requests advisor row zero
     * when that outcome allows an advisor message. */
    if ((s_objective_result_word & 1u) != 0u) {
        result_active_next = 1;
        if ((s_objective_result_word & 8u) != 0u) {
            if ((s_objective_result_word & LVL_OBJECTIVE_NO_ADVISOR) != 0u)
                (void)advisor_request_admit(0, 1);
            s_objective_result_word &= ~8u;
            s->objective &= ~LVL_OBJECTIVE_ANNOUNCED;
        }
    }
    else if ((s_objective_result_word & 2u) != 0u) {
        result_active_next = entity_flag_mask_callbacks_all_pass(0x40bu);
        if (result_active_next && (s_objective_result_word & 8u) != 0u) {
            if ((s_objective_result_word & LVL_OBJECTIVE_NO_ACH_ADV) != 0u)
                (void)advisor_request_admit(0, 1);
            s_objective_result_word &= ~8u;
            s->objective &= ~LVL_OBJECTIVE_ANNOUNCED;
        }
    }
    else if ((s_objective_result_word & 4u) != 0u) {
        result_active_next = entity_flag_mask_callbacks_all_pass(0x40bu);
        if (result_active_next && (s_objective_result_word & 8u) != 0u) {
            if ((s_objective_result_word & LVL_OBJECTIVE_NO_FAIL_ADV) != 0u)
                (void)advisor_request_admit(0, 1);
            s_objective_result_word &= ~8u;
            s->objective &= ~LVL_OBJECTIVE_ANNOUNCED;
        }
    }
    if ((s_objective_result_word & 7u) == 0) {
        int mark = 0;
        if (objective_check(s, &mark, checker_dt)) {
            objective_announce_current(mark != 0 ? 1 : 2);
        }
        else {
            /* ObjectiveTick adds the check interval to the reward score on
             * every unresolved frame (FUN_00462800(param_3)). This is the
             * elapsed-time component later copied into the checkpoint blob. */
            reward_score_add(checker_dt);
        }
    }

    /* DAT_004ffe44 also enables pending briefing audio. It is not a
     * terminal-result predicate. Like the reference, a newly announced
     * outcome enters its gated presentation branch on the following tick. */

    /* The target decrements an active stream timer before it considers a
     * newly prepared alias, and invalidates the stream sentinel on expiry. */
    if (s_objective_voice_stream_handle != -1) {
        s_objective_voice_stream_timeout -= frame_dt;
        if (s_objective_voice_stream_timeout <= 0.0f) {
            objective_advisor_clear_active_state();
            s_objective_voice_stream_handle = -1;
        }
    }

    /* Retail temporarily disables batching, submits selector zero, queries
     * duration, and restores batching. The portable play call has no instance
     * handle return, so zero is an active sentinel while the duration/timer
     * and alias-consumption ordering follow FFE3C/FFE40/FFE38/FFE44. */
    if (s_objective_voice_result_active && s_objective_voice_alias_pending) {
        int alias = 0;
        if (s_objective_voice_alias[0] != '\0' &&
            sound_alias_lookup(s_objective_voice_alias, &alias)) {
            const int batch_was_enabled = sound_batch_mode_get();
            sound_batch_queue_drain(0, 0);
            sound_alias_play(alias, 0);
            s_objective_voice_stream_handle = 0;
            s_objective_voice_stream_timeout =
                (float)((sound_object_duration_get(alias) - 1.5) * 25.0);
            sound_batch_queue_drain(batch_was_enabled != 0, 0);
        }
        /* Retail releases the interned name whether lookup succeeds or not. */
        s_objective_voice_alias_pending = 0;
        s_objective_voice_alias[0] = '\0';
    }
    /* FFE44 is updated after consuming FFE38 in the retail routine. */
    s_objective_voice_result_active = result_active_next;
    s_objective_tick_resolved =
        (result_active_next && (s_objective_result_word & 6u) != 0u)
            ? 0 : 1;
}
#endif /* LRR_OBJECTIVE_PORT_NAMES */
int objective_check(LevelState* s, int* mark, float dt)
{
    if (s == NULL) {
        return 0;
    }
    if ((s_objective_result_word & LVL_OBJECTIVE_NO_ADVISOR) != 0u) {
        return 0;
    }
    int m = 1;
    if ((s_objective_result_word & LVL_OBJECTIVE_TIMER) != 0u) {
        float timer = level_float(s, OBJ_OFF_TIMER_TARGET) - dt;
        level_set_float(s, OBJ_OFF_TIMER_TARGET, timer);
        if (timer <= 0.0f) {
            if ((s_objective_result_word & LVL_OBJECTIVE_NO_ACH_ADV) != 0u) {
                m = 0; /* HitTimeFail: timer expiry marks success */
            }
            *mark = m;
            return 1;
        }
    }
    if ((s_objective_result_word & LVL_OBJECTIVE_CRYSTALS) != 0u
        && (uint32_t)level_int(s, OBJ_OFF_CRYSTAL_LIVE)
           < (uint32_t)level_int(s, OBJ_OFF_CRYSTAL_TARGET)) {
        return 0;
    }
    if ((s_objective_result_word & LVL_OBJECTIVE_ORE) != 0u
        && (uint32_t)level_int(s, OBJ_OFF_ORE_LIVE)
           < (uint32_t)level_int(s, OBJ_OFF_ORE_TARGET)) {
        return 0;
    }
    *mark = m;
    {
        ObjectiveEntityMatchContext match;
        match.level = s;
        match.flags = s_objective_result_word;
        return entity_pool_find_by_callback_filtered_current(
            objective_entity_pair_match, &match) != 0;
    }
}
static void objective_voice_alias_format(char* out, size_t out_size,
                                         const char* outcome,
                                         const char* family,
                                         const char* level)
{
    const char* parts[4];
    size_t used = 0;
    int part;
    parts[0] = "Stream_Objective";
    parts[1] = outcome;
    parts[2] = family;
    parts[3] = level;
    if (out == NULL || out_size == 0) return;
    out[0] = '\0';
    for (part = 0; part < 4; ++part) {
        const char* text = parts[part] != NULL ? parts[part] : "";
        while (*text != '\0' && used + 1 < out_size) {
            out[used++] = *text++;
        }
    }
    out[used] = '\0';
}

static const char* obj_cfg_str(BlockFile* cfg, const char* level, const char* key,
                               char* out, size_t out_sz)
{
    if (cfg == NULL || level == NULL) {
        return NULL;
    }
    char path[512];
    if (util_level_cfg_path(cfg, level, key, path, sizeof(path)) == NULL) {
        return NULL;
    }
    const char* v = block_get_strptr(cfg, path);
    if (v == NULL) {
        return NULL;
    }
    snprintf(out, out_sz, "%s", v);
    return out;
}

/* Corpus FUN_004790d0 delegates to ConfigBooleanGet @ 0x477710. */
static int obj_typed_int(BlockFile* cfg, const char* level, const char* key, int dflt)
{
    char path[512];
    if (util_level_cfg_path(cfg, level, key, path, sizeof(path)) == NULL) {
        return dflt;
    }
    return block_get_bool_path(cfg, path);
}

/* BlockTextTokenSplit uses the corpus's comma delimiter for the two target
 * pairs. Parse into fixed-width words because the target record is x86. */
static void obj_split_two(const char* s, int32_t* a, int32_t* b)
{
    *a = 0;
    *b = 0;
    if (s == NULL) {
        return;
    }
    *a = atoi(s);
    const char* p = strchr(s, ',');
    if (p != NULL) {
        *b = atoi(p + 1);
    }
}

/* Keep the configured objective mask in the portable level sidecar while
 * the mapped target above preserves the retail global and record writes. */
static void obj_set_resources(LevelState* s, int32_t crystal, int32_t ore)
{
    if (crystal != 0) s->objective |= LVL_OBJECTIVE_CRYSTALS;
    if (ore != 0) s->objective |= LVL_OBJECTIVE_ORE;
    obj_set_resources_target(s->word, crystal, ore);
}

static void obj_set_block(LevelState* s, const int32_t targets[2])
{
    s->objective |= LVL_OBJECTIVE_BLOCK;
    obj_set_block_target(s->word, targets);
}

static void obj_set_timer(LevelState* s, float ticks, int hit_time_fail)
{
    s->objective |= LVL_OBJECTIVE_TIMER;
    if (hit_time_fail) s->objective |= LVL_OBJECTIVE_NO_ACH_ADV;
    obj_set_timer_target(s->word, ticks, hit_time_fail);
}

static void obj_set_construction(LevelState* s, long a, long b)
{
    s->objective |= LVL_OBJECTIVE_CONSTRUCTION;
    obj_set_construction_target(s->word, (uint32_t)a, (uint32_t)b);
}

/* LAB_00459100 is passed to the filtered entity walk by FUN_00459050.
 * Finding 110 records its type-1/type-2 cell-pair path and its separate
 * construction-pair path.  EntitySlot currently exposes the scene position
 * needed by the first path; the raw +0x4c/+0x50 construction pair has no
 * portable field mapping, so that second path remains unavailable here. */
static int objective_entity_pair_match(EntitySlot* entity, void* payload)
{
    ObjectiveEntityMatchContext* match =
        (ObjectiveEntityMatchContext*)payload;
    if (entity == NULL || match == NULL || match->level == NULL) return 0;
    if ((match->flags & LVL_OBJECTIVE_BLOCK) != 0u &&
        (entity->type == 1 || entity->type == 2)) {
        int cell_x = -1;
        int cell_y = -1;
        entity_position_to_terrain_cell(entity, match->level,
                                        &cell_x, &cell_y);
        if (level_coord_pair_within_bounds(match->level, cell_x, cell_y) &&
            cell_x == level_int(match->level, OBJ_OFF_BLOCK_A) &&
            cell_y == level_int(match->level, OBJ_OFF_BLOCK_B)) return 1;
    }
    return 0;
}

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: the
 * original FUN_00457d40 takes the config, game and level names, the level
 * and the screen size; the port's objective_load above takes the config,
 * the level name and the level. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (objective_load, 0x00457d40)
#if LRR_OBJECTIVE_PORT_NAMES /* objective_unit.c's Objective_LoadLevel in the matching build */
extern "C" void objective_load_original(BlockFile* config,
                                        const char* gameName,
                                        const char* levelName,
                                        struct Lego_Level* level,
                                        int screenWidth, int screenHeight)
{
    (void)gameName;
    (void)screenWidth;
    (void)screenHeight;
    objective_load(config, levelName, (LevelState*)level);
}
#endif /* LRR_OBJECTIVE_PORT_NAMES */

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: the
 * original FUN_004589a0 takes no argument and reads the current level; the
 * port's entry takes the level. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (global_option_low_bits_enabled, 0x004589a0)
extern "C" int lego_level_low_bits_enabled_adapter(void)
{
    return global_option_low_bits_enabled(g_active_level_retail);
}

/* Original-ABI adapters (ADR 0005 step 6) for GAME-029's objective_unit.c,
 * where the callee has no port entry with the original's contract. */
// PORT-ONLY: original-ABI adapter for GAME-029 objective_unit.c (osf_snapshot, 0x004592e0)
extern "C" void objective_osf_store_adapter(void)
{
    osf_snapshot(entity_pool_callback_context_get());
}

// PORT-ONLY: original-ABI adapter for GAME-029 objective_unit.c (0x00459360, no port entry)
extern "C" void objective_osf_restore_adapter(void)
{
    /* No port model of the original's restore of the live table from its
     * snapshot: nothing to do. */
}

// PORT-ONLY: original-ABI adapter for GAME-029 objective_unit.c (Image_GetScreenshot 0x0047e300, no port entry)
extern "C" int objective_image_get_screenshot_adapter(void* image,
                                                      unsigned int xsize,
                                                      unsigned int ysize)
{
    /* The port takes no screenshot for the reward screen. */
    (void)image;
    (void)xsize;
    (void)ysize;
    return 0;
}
