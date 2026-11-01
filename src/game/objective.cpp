// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gods98/block.hpp"
#include "gods98/engine.hpp"
#include "gods98/media.hpp"
#include "gods98/scene_resources.hpp"
#include "gods98/string.hpp"
#include "gods98/texture.hpp"
#include "gods98/surface.hpp"
#include "gods98/sound.hpp"
#include "gods98/view.hpp"
#include "level.hpp"
#include "objective.hpp"
#include "util.hpp"
#include "crt/crt.hpp"

static ObjectiveDeferredCallback s_objective_deferred_callback;
static AdvisorTimerActionFn s_advisor_timer_action;
static AdvisorTimerFinalizeFn s_advisor_timer_finalize;
static void* s_advisor_timer_user;
static LevelState* s_objective_active_level;
static char s_objective_voice_alias[256];
static int s_objective_voice_alias_pending;
/* DAT_004ffbc0 is the runtime result word. It is separate from the level's
 * configured objective mask in LevelState::objective. */
static uint32_t s_objective_result_word;
static int s_objective_voice_stream_handle = -1;
static float s_objective_voice_stream_timeout;
static int s_objective_voice_result_active;
static int s_objective_tick_resolved;
// GLOBAL: LEGORR 0x004a6768
static int s_advisor_sound_slot = -1;
static CoordinateListHeadGetHook s_coordinate_list_head_get;
static void* s_coordinate_list_head_context;
static RewardTaskResourceRecord s_reward_task_resources[10];

void advisor_sound_alias_set(int slot)
{
    /* DAT_004a6768 receives the value from the configured advisor sound
     * table, whose entries are sound-bank slots (the 0x47b8e0 and 0x47ad70
     * callers consume it as a slot), not SoundAliasPlay IDs. */
    s_advisor_sound_slot = slot;
}

/* SoundAliasPlayGlobal (FUN_00456810) forwards the configured bank slot
 * directly to SoundBufferPlayWithPosition in global mode 2, with loop 0.
 * It does not run alias lookup or variant selection. */
// FUNCTION: LEGORR 0x00456810
void advisor_sound_alias_play_global(void)
{
    if (s_advisor_sound_slot != -1 && sound_bank_sound_on())
        (void)sound_bank_entry_play_mode(s_advisor_sound_slot, 0, 2);
}

void coordinate_list_visit_hooks_set(CoordinateListHeadGetHook head_get,
                                     void* context)
{
    s_coordinate_list_head_get = head_get;
    s_coordinate_list_head_context = context;
}

/* CoordinateListCallbackVisit (FUN_00456cb0): resolve the current list,
 * visit each {x,y,next} record, and stop as soon as the visitor returns 0. */
void coordinate_list_callback_visit(void* list_key,
                                    CoordinateListVisitCallback callback,
                                    void* user)
{
    struct CoordinateListNode {
        int x;
        int y;
        CoordinateListNode* next;
    };
    void* record;
    if (callback == NULL || s_coordinate_list_head_get == NULL) return;
    record = s_coordinate_list_head_get(list_key,
                                        s_coordinate_list_head_context);
    while (record != NULL) {
        CoordinateListNode* const node = (CoordinateListNode*)record;
        if (!callback(record, node->x, node->y, user)) break;
        record = node->next;
    }
}

void reward_task_resource_slot_set(unsigned int slot, uint32_t flags,
                                   void* image_surface, void* font_record)
{
    if (slot >= 10u) return;
    s_reward_task_resources[slot].flags = flags;
    s_reward_task_resources[slot].image_surface = image_surface;
    s_reward_task_resources[slot].font_record = font_record;
}

/* TaskFlushImages @ 0x0045f7b0: release the task's owned image surface. */
// FUNCTION: LEGORR 0x0045F7B0
void reward_task_flush_images(RewardTaskResourceRecord* task)
{
    if (task != NULL) {
        texture_release((TextureSurface*)task->image_surface);
    }
}

/* TaskFlushFont @ 0x0045f7d0: release the task's font-atlas record. */
// FUNCTION: LEGORR 0x0045F7D0
void reward_task_flush_font(RewardTaskResourceRecord* task)
{
    if (task != NULL) {
        font_atlas_record_release(task->font_record);
    }
}

/* TaskBlockFinish @ 0x0045f750: the ten 0x210-byte task records select
 * image release with flag bit 1 and font release with bit 0. */
// FUNCTION: LEGORR 0x0045F750
void reward_task_block_finish(void)
{
    unsigned int i;
    for (i = 0; i < 10u; ++i) {
        RewardTaskResourceRecord* task = &s_reward_task_resources[i];
        if ((task->flags & 2u) != 0u) reward_task_flush_images(task);
        if ((task->flags & 1u) != 0u) reward_task_flush_font(task);
        task->flags = 0;
        task->image_surface = NULL;
        task->font_record = NULL;
    }
}

void objective_deferred_callback_set(ObjectiveDeferredCallback callback)
{
    s_objective_deferred_callback = callback;
}

// FUNCTION: LEGORR 0x00453910
void objective_deferred_callback_invoke_if_set(void)
{
    if (s_objective_deferred_callback != NULL)
        s_objective_deferred_callback();
}

void advisor_timer_hooks_set(AdvisorTimerActionFn action,
                             AdvisorTimerFinalizeFn finalize, void* user)
{
    s_advisor_timer_action = action;
    s_advisor_timer_finalize = finalize;
    s_advisor_timer_user = user;
}

// FUNCTION: LEGORR 0x00453BB0
void advisor_timer_array_advance_and_advisor_clear(AdvisorTimerState* state,
                                                   float delta)
{
    float advance;
    int i;
    if (state == NULL) return;
    advance = delta * 1000.0f * 0.04f;
    for (i = 0; i < 4; ++i) state->timers[i] += advance;
    if (state->countdown == 0.0f) return;
    if (state->countdown >= 0.0f) {
        if (state->advisor_active == 0 || state->hold_countdown != 0) {
            state->countdown -= advance;
            if (state->countdown > 0.0f) goto finalize;
        }
        else {
            if (state->stored_toggle != 0 &&
                ((state->advisor_scroll > 0 && state->final_timer != 0.0f) ||
                 state->final_timer == 0.0f)) {
                if (s_advisor_timer_action != NULL)
                    s_advisor_timer_action(state->update_pending, 1,
                                           s_advisor_timer_user);
            }
            state->stored_toggle = state->update_pending;
            if (state->update_pending != 0) goto finalize;
        }
    }
    state->countdown = 0.0f;
finalize:
    if (state->finalize_pending != 0 && state->countdown < 1000.0f &&
        s_advisor_timer_finalize != NULL)
        s_advisor_timer_finalize(s_advisor_timer_user);
}

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

static int obj_strcasecmp(const char* a, const char* b)
{
    for (;;) {
        int ca = (unsigned char)*a++;
        int cb = (unsigned char)*b++;
        if (ca >= 'a' && ca <= 'z') ca -= 0x20;
        if (cb >= 'a' && cb <= 'z') cb -= 0x20;
        if (ca != cb) return ca - cb;
        if (ca == 0) return 0;
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

// GLOBAL: LEGORR 0x004a13cc
static const char s_advisor_name_objective[] = "Advisor_Objective";
// GLOBAL: LEGORR 0x004a13b0
static const char s_advisor_name_objective_achieved[] = "Advisor_ObjectiveAcheived";
// GLOBAL: LEGORR 0x004a1398
static const char s_advisor_name_objective_failed[] = "Advisor_ObjectiveFailed";
// GLOBAL: LEGORR 0x004a137c
static const char s_advisor_name_icon_normal[] = "Advisor_IconPoint_Normal";
// GLOBAL: LEGORR 0x004a135c
static const char s_advisor_name_icon_back[] = "Advisor_IconPoint_BackButton";
// GLOBAL: LEGORR 0x004a133c
static const char s_advisor_name_icon_top[] = "Advisor_IconPoint_TopButtons";
// GLOBAL: LEGORR 0x004a131c
static const char s_advisor_name_radar_toggle[] = "Advisor_PanelPoint_RadarToggle";
// GLOBAL: LEGORR 0x004a12f0
static const char s_advisor_name_radar_tagged[] = "Advisor_PanelPoint_RadarTaggedObjectView";
// GLOBAL: LEGORR 0x004a12d0
static const char s_advisor_name_radar_zoom_in[] = "Advisor_PanelPoint_RadarZoomIn";
// GLOBAL: LEGORR 0x004a12b0
static const char s_advisor_name_radar_zoom_out[] = "Advisor_PanelPoint_RadarZoomOut";
// GLOBAL: LEGORR 0x004a1290
static const char s_advisor_name_radar_map_view[] = "Advisor_PanelPoint_RadarMapView";
// GLOBAL: LEGORR 0x004a1270
static const char s_advisor_name_info_dock_goto[] = "Advisor_PanelPoint_InfoDockGoto";
// GLOBAL: LEGORR 0x004a124c
static const char s_advisor_name_info_dock_close[] = "Advisor_PanelPoint_InfoDockClose";
// GLOBAL: LEGORR 0x004a122c
static const char s_advisor_name_top_info[] = "Advisor_PanelPoint_TopPanelInfo";
// GLOBAL: LEGORR 0x004a1208
static const char s_advisor_name_top_options[] = "Advisor_PanelPoint_TopPanelOptions";
// GLOBAL: LEGORR 0x004a11e0
static const char s_advisor_name_top_priorities[] = "Advisor_PanelPoint_TopPanelPriorities";
// GLOBAL: LEGORR 0x004a11b8
static const char s_advisor_name_top_call_to_arms[] = "Advisor_PanelPoint_TopPanelCallToArms";
// GLOBAL: LEGORR 0x004a1194
static const char s_advisor_name_top_go_back[] = "Advisor_PanelPoint_TopPanelGoBack";
// GLOBAL: LEGORR 0x004a1170
static const char s_advisor_name_control_zoom_in[] = "Advisor_PanelPoint_ControlZoomIn";
// GLOBAL: LEGORR 0x004a114c
static const char s_advisor_name_control_zoom_out[] = "Advisor_PanelPoint_ControlZoomOut";
// GLOBAL: LEGORR 0x004a1138
static const char s_advisor_name_talk_in_game[] = "Advisor_TalkInGame";
// GLOBAL: LEGORR 0x004a1120
static const char s_advisor_anim_point_to_map[] = "Advisor_Anim_PointToMap";
// GLOBAL: LEGORR 0x004a1108
static const char s_advisor_anim_point_n[] = "Advisor_Anim_Point_N";
// GLOBAL: LEGORR 0x004a10f0
static const char s_advisor_anim_point_ne[] = "Advisor_Anim_Point_NE";
// GLOBAL: LEGORR 0x004a10d8
static const char s_advisor_anim_point_e[] = "Advisor_Anim_Point_E";
// GLOBAL: LEGORR 0x004a10c0
static const char s_advisor_anim_point_se[] = "Advisor_Anim_Point_SE";
// GLOBAL: LEGORR 0x004a10a8
static const char s_advisor_anim_point_s[] = "Advisor_Anim_Point_S";
// GLOBAL: LEGORR 0x004a1090
static const char s_advisor_anim_point_sw[] = "Advisor_Anim_Point_SW";
// GLOBAL: LEGORR 0x004a1078
static const char s_advisor_anim_point_w[] = "Advisor_Anim_Point_W";
// GLOBAL: LEGORR 0x004a1060
static const char s_advisor_anim_point_nw[] = "Advisor_Anim_Point_NW";
// GLOBAL: LEGORR 0x004a1048
static const char s_advisor_anim_point_up[] = "Advisor_Anim_Point_Up";
// GLOBAL: LEGORR 0x004a1030
static const char s_advisor_anim_talk_top[] = "Advisor_Anim_Talk_Top";
// GLOBAL: LEGORR 0x004b2db0
static const char* s_advisor_position_names[21];
// GLOBAL: LEGORR 0x004b30f8
static const char* s_advisor_animation_names[11];

// FUNCTION: LEGORR 0x004013e0
int advisor_position_name_to_index(const char* name, int* index)
{
    for (int i = 0; i < 21; ++i) {
        if (obj_strcasecmp(s_advisor_position_names[i], name) == 0) {
            *index = i;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x004014c0
int advisor_animation_name_to_index(const char* name, int* index)
{
    for (int i = 0; i < 11; ++i) {
        if (obj_strcasecmp(s_advisor_animation_names[i], name) == 0) {
            *index = i;
            return 1;
        }
    }
    return 0;
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
    return block_get_bool(cfg, path);
}

/* Split "a b" / "a,b" value into two longs (block/construction targets). */
static void obj_split_two(const char* s, long* a, long* b)
{
    *a = 0;
    *b = 0;
    if (s == NULL) {
        return;
    }
    *a = atoi(s);
    const char* p = strpbrk(s, " ,|");
    if (p != NULL) {
        *b = atoi(p + 1);
    }
}

/* Corpus FUN_00458580 — crystal/ore objectives. */
// FUNCTION: LEGORR 0x00458580
static void obj_set_resources(LevelState* s, long crystal, long ore)
{
    if (crystal != 0) {
        s->objective |= LVL_OBJECTIVE_CRYSTALS;
        level_set_int(s, OBJ_OFF_CRYSTAL_TARGET, (int32_t)crystal);
    }
    if (ore != 0) {
        s->objective |= LVL_OBJECTIVE_ORE;
        level_set_int(s, OBJ_OFF_ORE_TARGET, (int32_t)ore);
    }
}

/* Corpus FUN_004585c0 — block objective (two targets). */
// FUNCTION: LEGORR 0x004585c0
static void obj_set_block(LevelState* s, long a, long b)
{
    s->objective |= LVL_OBJECTIVE_BLOCK;
    level_set_int(s, OBJ_OFF_BLOCK_A, (int32_t)a);
    level_set_int(s, OBJ_OFF_BLOCK_B, (int32_t)b);
}

/* Corpus FUN_004585f0 — timer objective (ticks = seconds*25). */
// FUNCTION: LEGORR 0x004585f0
static void obj_set_timer(LevelState* s, float ticks, int hit_time_fail)
{
    s->objective |= LVL_OBJECTIVE_TIMER;
    level_set_float(s, OBJ_OFF_TIMER_TARGET, ticks);
    if (hit_time_fail) {
        s->objective |= LVL_OBJECTIVE_NO_ACH_ADV;
    }
}

/* Corpus FUN_00458620 — construction objective. */
// FUNCTION: LEGORR 0x00458620
static void obj_set_construction(LevelState* s, long a, long b)
{
    s->objective |= LVL_OBJECTIVE_CONSTRUCTION;
    level_set_int(s, OBJ_OFF_CONSTRUCT_A, (int32_t)a);
    level_set_int(s, OBJ_OFF_CONSTRUCT_B, (int32_t)b);
}

// FUNCTION: LEGORR 0x00457d40
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
    }
    if (obj_typed_int(cfg, level_name, K_DONT_ACH_ADV, 0) != 1) {
        s->objective |= LVL_OBJECTIVE_NO_ACH_ADV;
    }
    if (obj_typed_int(cfg, level_name, K_DONT_FAIL_ADV, 0) != 1) {
        s->objective |= LVL_OBJECTIVE_NO_FAIL_ADV;
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
        long crystal = 0, ore = 0;
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
            float ticks = (float)atoi(value) * 25.0f;
            int htf = strcasecmp_lite(value, "HitTimeFailObjective") == 0;
            obj_set_timer(s, ticks, htf);
        }
    }
    {
        char value[64];
        if (obj_cfg_str(cfg, level_name, K_CONSTRUCT_OBJ, value, sizeof(value)) != NULL) {
            long a = 0, b = 0;
            obj_split_two(value, &a, &b);
            obj_set_construction(s, a, b);
        }
    }
    {
        char value[64];
        if (obj_cfg_str(cfg, level_name, K_BLOCK_OBJ, value, sizeof(value)) != NULL) {
            long a = 0, b = 0;
            obj_split_two(value, &a, &b);
            obj_set_block(s, a, b);
        }
    }
    /* ObjectiveText feeds the message registry (corpus FUN_004574e0); not
     * stored on the level block — HUD milestone. */
    (void)K_DONT_ADV;

    if (s->objective == 0) {
        s->objective = LVL_OBJECTIVE_NO_ADVISOR;
    }
}

// FUNCTION: LEGORR 0x00459050
int objective_check(LevelState* s, int* mark, float dt)
{
    if (s == NULL) {
        return 0;
    }
    if ((s->objective & LVL_OBJECTIVE_NO_ADVISOR) != 0) {
        return 0;
    }
    int m = 1;
    if ((s->objective & LVL_OBJECTIVE_TIMER) != 0) {
        float timer = level_float(s, OBJ_OFF_TIMER_TARGET) - dt;
        level_set_float(s, OBJ_OFF_TIMER_TARGET, timer);
        if (timer <= 0.0f) {
            if ((s->objective & LVL_OBJECTIVE_NO_ACH_ADV) != 0) {
                m = 0; /* HitTimeFail: timer expiry marks success */
            }
            *mark = m;
            return 1;
        }
    }
    if ((s->objective & LVL_OBJECTIVE_CRYSTALS) != 0
        && (uint32_t)level_int(s, OBJ_OFF_CRYSTAL_LIVE)
           < (uint32_t)level_int(s, OBJ_OFF_CRYSTAL_TARGET)) {
        return 0;
    }
    if ((s->objective & LVL_OBJECTIVE_ORE) != 0
        && (uint32_t)level_int(s, OBJ_OFF_ORE_LIVE)
           < (uint32_t)level_int(s, OBJ_OFF_ORE_TARGET)) {
        return 0;
    }
    *mark = m;
    return 1;
}

static int s_objective_announce_target_gate = 1;

/* The retail function reads the current level from FUN_00431560 and has one
 * stack argument. `objective_load` binds that owner, while mission frame
 * updates rebind the live context before objective processing. */
// FUNCTION: LEGORR 0x00458670
void objective_announce_current(int who)
{
    LevelState* s = s_objective_active_level;
    char alias[256];
    int result = 0;
    const char* outcome = "";
    const char* family = "_Levels::";
    const char* level;
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
        outcome = "Acheived";
        result = 1;
        break;
    case 2:
        s_objective_result_word |= 4u;
        s->objective_result = 0;
        s->objective |= LVL_OBJECTIVE_FAILED;
        outcome = "Failed";
        result = 2;
        break;
    case 3:
        s_objective_result_word |= 0x104u;
        s->objective_result = 0;
        s->objective |= LVL_OBJECTIVE_FAILED | LVL_OBJECTIVE_CRYSTALS;
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
    level_set_int(s, OBJ_OFF_RESULT_INDEX, result);
    objective_voice_alias_format(alias, sizeof(alias), outcome, family, level);
    strncpy(s_objective_voice_alias, alias, sizeof(s_objective_voice_alias) - 1);
    s_objective_voice_alias[sizeof(s_objective_voice_alias) - 1] = '\0';
    strncpy(s->obj_stream, s_objective_voice_alias, sizeof(s->obj_stream) - 1);
    s->obj_stream[sizeof(s->obj_stream) - 1] = '\0';
    s_objective_voice_alias_pending = 1;
    s_objective_result_word |= 8u;
    s->objective |= LVL_OBJECTIVE_ANNOUNCED;
}

/* Explicit-level adapter used by port call sites that already own the level
 * pointer. It shares state transitions but does not claim the target ABI. */
void objective_announce(LevelState* s, int who)
{
    int previous_gate = s_objective_announce_target_gate;
    if (s == NULL) return;
    s_objective_active_level = s;
    s_objective_announce_target_gate = 0;
    objective_announce_current(who);
    s_objective_announce_target_gate = previous_gate;
}

void objective_active_level_bind(LevelState* s)
{
    s_objective_active_level = s;
}

void objective_active_level_clear(LevelState* s)
{
    if (s_objective_active_level == s) s_objective_active_level = NULL;
}

const char* objective_voice_alias_get(void)
{
    return s_objective_voice_alias;
}

int objective_voice_alias_pending_take(void)
{
    const int pending = s_objective_voice_alias_pending;
    s_objective_voice_alias_pending = 0;
    return pending;
}

const char* objective_stream(const LevelState* s)
{
    return s != NULL ? s->obj_stream : "";
}

int objective_advisor_shown(const LevelState* s)
{
    if (s == NULL || s->obj_stream[0] == '\0') {
        return 0;
    }
    /* The corpus honours the outcome's suppression bit (0x10 generic /
     * 0x20 achieved / 0x80 failed) before showing the hint. */
    uint32_t suppress = LVL_OBJECTIVE_NO_ADVISOR;
    if (s->objective & LVL_OBJECTIVE_ACHIEVED) {
        suppress |= LVL_OBJECTIVE_NO_ACH_ADV;
    }
    if (s->objective & LVL_OBJECTIVE_FAILED) {
        suppress |= LVL_OBJECTIVE_NO_FAIL_ADV;
    }
    return (s->objective & suppress) == 0 ? 1 : 0;
}

/* ObjectiveTick @ 0x00458be0 has four arguments in the retail caller:
 * (opaque global owner, level block, objective-check dt, frame dt). */
// FUNCTION: LEGORR 0x00458be0
void objective_tick_target(void* engine, LevelState* s,
                           float checker_dt, float frame_dt)
{
    int resolved_now = 0;
    (void)engine; /* the renderer/global owner is not represented here */
    if (s == NULL) {
        s_objective_tick_resolved = 1;
        return;
    }
    s_objective_active_level = s;
    /* Consume an announced hint: the state machine clears bit 8 once the
     * advisor overlay has been shown (the show call is a stub). */
    if ((s_objective_result_word & 8u) != 0) {
        s_objective_result_word &= ~8u;
        s->objective &= ~LVL_OBJECTIVE_ANNOUNCED;
    }
    if ((s_objective_result_word & 7u) == 0) {
        int mark = 0;
        if (objective_check(s, &mark, checker_dt)) {
            objective_announce_current(mark != 0 ? 1 : 2);
            resolved_now =
                (s->objective & (LVL_OBJECTIVE_ACHIEVED |
                                 LVL_OBJECTIVE_FAILED)) != 0;
        }
    }

    /* DAT_004ffe44 is the result-screen stream gate. The pending branch is
     * represented directly. Terminal branches also test
     * SceneObjectStateDispatch(0x40b); without that renderer-owned result,
     * leave their gate clear rather than assuming the scene is ready. */
    int result_active_next =
        (s_objective_result_word & 1u) != 0 ? 1 : 0;

    /* The target decrements an active stream timer before it considers a
     * newly prepared alias, and invalidates the stream sentinel on expiry. */
    if (s_objective_voice_stream_handle != -1) {
        s_objective_voice_stream_timeout -= frame_dt;
        if (s_objective_voice_stream_timeout <= 0.0f) {
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
        (resolved_now ||
         (s_objective_result_word & 7u) != 0) ? 0 : 1;
}

/* Explicit-level adapter used by the portable frame loop. */
int objective_tick(LevelState* s, float dt)
{
    objective_tick_target(NULL, s, dt, dt);
    return s_objective_tick_resolved;
}

// FUNCTION: LEGORR 0x00454b70
int objective_voice_alias_raise_commit(LevelState* s)
{
    if (s != NULL) s_objective_active_level = s;
    objective_announce_current(1);
    return 1;
}

/* ObjectiveVoiceAliasRaiseGateBit27 (FUN_00454b80) — see objective.hpp. */
// FUNCTION: LEGORR 0x00454b80
int objective_voice_alias_raise_gate_bit27(LevelState* s)
{
    if ((media_cd_flags_get() & 0x8000000u) == 0u) {
        if (s != NULL) s_objective_active_level = s;
        objective_announce_current(2);
    }
    return 1;
}

/* --- the advisor action window band (see objective.hpp) ------------------- */

#define ADV_ACTION_ROWS 21

/* The 21 records each occupy nine dwords (stride 0x24); the flags overlap
 * the getter's second read-out word at +0x20 in the recovered layout. */
// GLOBAL: LEGORR 0x004b2e04
static uint32_t s_adv_actions[ADV_ACTION_ROWS][9];
// GLOBAL: LEGORR 0x004b31a8
static int s_adv_scene_active_row;
// GLOBAL: LEGORR 0x004b31ac
static uint32_t s_adv_scene_root_handle;
// GLOBAL: LEGORR 0x004b31b0
static uint32_t s_adv_scene_transform_context;
// GLOBAL: LEGORR 0x004b31b4
static float s_adv_scene_scale;
// GLOBAL: LEGORR 0x004b31b8
static uint32_t s_adv_scene_companion_handle;
static AdvisorSceneActionHook s_adv_scene_action_hook;
static void* s_adv_scene_action_user;
static AdvisorCompanionSceneCreateHook s_adv_companion_scene_create_hook;
static void* s_adv_companion_scene_create_user;
typedef struct AdvisorAnimationSlot {
    uint32_t handle;
    float start_time;
    float end_time;
} AdvisorAnimationSlot;
// GLOBAL: LEGORR 0x004b3124
static AdvisorAnimationSlot s_adv_animation_slots[11];
static AdvisorAnimationHooks s_adv_animation_hooks;
static AdvisorTransformApplyHook s_adv_transform_apply_hook;
static void* s_adv_transform_apply_user;

void advisor_animation_hooks_set(const AdvisorAnimationHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_adv_animation_hooks, 0, sizeof(s_adv_animation_hooks));
    } else {
        s_adv_animation_hooks = *hooks;
    }
}

void advisor_animation_slot_install(int slot, uint32_t handle,
                                    float start_time, float end_time)
{
    if (slot < 0 || slot >= 11) return;
    s_adv_animation_slots[slot].handle = handle;
    s_adv_animation_slots[slot].start_time = start_time;
    s_adv_animation_slots[slot].end_time = end_time;
}

// FUNCTION: LEGORR 0x00401a70
int advisor_scene_advance_completion_check(int animation_slot, float delta)
{
    uint32_t handle;
    float progress = 0.0f;
    float current;
    if (animation_slot < 0 || animation_slot >= 11) return 1;
    handle = s_adv_animation_slots[animation_slot].handle;
    if (handle != 0u && s_adv_animation_hooks.update != NULL) {
        progress = s_adv_animation_hooks.update(handle, delta,
                                                s_adv_animation_hooks.user);
    }
    if ((advisor_state_word_peek() & 2u) == 0u) {
        return progress > 0.0f ? 0 : 1;
    }
    if (progress > 0.0f) {
        if (handle != 0u && s_adv_animation_hooks.seek != NULL) {
            s_adv_animation_hooks.seek(handle,
                s_adv_animation_slots[animation_slot].start_time + progress,
                s_adv_animation_hooks.user);
        }
        return 1;
    }
    current = handle != 0u && s_adv_animation_hooks.time_get != NULL
        ? s_adv_animation_hooks.time_get(handle, s_adv_animation_hooks.user)
        : 0.0f;
    if (s_adv_animation_slots[animation_slot].end_time <= current) {
        if (handle != 0u && s_adv_animation_hooks.seek != NULL) {
            s_adv_animation_hooks.seek(handle,
                (current - s_adv_animation_slots[animation_slot].end_time) +
                    s_adv_animation_slots[animation_slot].start_time,
                s_adv_animation_hooks.user);
        }
    }
    return 1;
}

void advisor_scene_ready_set(int ready)
{
    engine_context_mode_global_set(ready);
}

void advisor_scene_action_hook_set(AdvisorSceneActionHook hook, void* user)
{
    s_adv_scene_action_hook = hook;
    s_adv_scene_action_user = user;
}

void advisor_companion_scene_create_hook_set(
    AdvisorCompanionSceneCreateHook hook, void* user)
{
    s_adv_companion_scene_create_hook = hook;
    s_adv_companion_scene_create_user = user;
}

// FUNCTION: LEGORR 0x00401000
void advisor_scene_controller_initialize(void* config_context,
                                         const uint32_t* scene_context_words,
                                         uint32_t transform_context_handle)
{
    uint32_t base_scene;
    (void)config_context;
    if (scene_context_words == NULL) return;
    s_advisor_position_names[0] = s_advisor_name_objective;
    s_advisor_position_names[1] = s_advisor_name_objective_achieved;
    s_advisor_position_names[2] = s_advisor_name_objective_failed;
    s_advisor_position_names[3] = s_advisor_name_icon_normal;
    s_advisor_position_names[4] = s_advisor_name_icon_back;
    s_advisor_position_names[5] = s_advisor_name_icon_top;
    s_advisor_position_names[6] = s_advisor_name_radar_toggle;
    s_advisor_position_names[7] = s_advisor_name_radar_tagged;
    s_advisor_position_names[8] = s_advisor_name_radar_zoom_in;
    s_advisor_position_names[9] = s_advisor_name_radar_zoom_out;
    s_advisor_position_names[10] = s_advisor_name_radar_map_view;
    s_advisor_position_names[11] = s_advisor_name_info_dock_goto;
    s_advisor_position_names[12] = s_advisor_name_info_dock_close;
    s_advisor_position_names[13] = s_advisor_name_top_info;
    s_advisor_position_names[14] = s_advisor_name_top_options;
    s_advisor_position_names[15] = s_advisor_name_top_priorities;
    s_advisor_position_names[16] = s_advisor_name_top_call_to_arms;
    s_advisor_position_names[17] = s_advisor_name_top_go_back;
    s_advisor_position_names[18] = s_advisor_name_control_zoom_in;
    s_advisor_position_names[19] = s_advisor_name_control_zoom_out;
    s_advisor_position_names[20] = s_advisor_name_talk_in_game;
    s_advisor_animation_names[0] = s_advisor_anim_point_to_map;
    s_advisor_animation_names[1] = s_advisor_anim_point_n;
    s_advisor_animation_names[2] = s_advisor_anim_point_ne;
    s_advisor_animation_names[3] = s_advisor_anim_point_e;
    s_advisor_animation_names[4] = s_advisor_anim_point_se;
    s_advisor_animation_names[5] = s_advisor_anim_point_s;
    s_advisor_animation_names[6] = s_advisor_anim_point_sw;
    s_advisor_animation_names[7] = s_advisor_anim_point_w;
    s_advisor_animation_names[8] = s_advisor_anim_point_nw;
    s_advisor_animation_names[9] = s_advisor_anim_point_up;
    s_advisor_animation_names[10] = s_advisor_anim_talk_top;
    base_scene = scene_context_words[9]; /* param_2 + 0x24 */
    s_adv_scene_root_handle = base_scene;
    s_adv_scene_transform_context = transform_context_handle;
    s_adv_scene_companion_handle =
        s_adv_companion_scene_create_hook != NULL
            ? s_adv_companion_scene_create_hook(
                  base_scene, s_adv_companion_scene_create_user)
            : 0u;
    advisor_state_word_set(0u);
    if (s_adv_scene_action_hook != NULL) {
        s_adv_scene_action_hook(ADVISOR_SCENE_ACTION_CONTROLLER_INIT, -1,
                                transform_context_handle,
                                s_adv_scene_action_user);
    }
}

// FUNCTION: LEGORR 0x00401990
void advisor_transition_worker(float delta)
{
    int row = s_adv_scene_active_row;
    uint32_t animation_slot;
    uint32_t animation;
    animation_slot = s_adv_actions[row][0];
    animation = s_adv_animation_slots[animation_slot].handle;
    if (animation != 0u && (advisor_state_word_peek() & 1u) != 0u &&
        !current_engine_mode_is_zero()) {
        if ((s_adv_actions[row][8] & 0x40000u) != 0u &&
            s_adv_scene_action_hook != NULL) {
            s_adv_scene_action_hook(ADVISOR_SCENE_ACTION_NOTIFY, row,
                                    animation, s_adv_scene_action_user);
        }
        if (s_adv_scene_action_hook != NULL) {
            s_adv_scene_action_hook(ADVISOR_SCENE_ACTION_HIDE, row,
                                    animation, s_adv_scene_action_user);
            s_adv_scene_action_hook(ADVISOR_SCENE_ACTION_HIDE, -1,
                                    s_adv_scene_companion_handle,
                                    s_adv_scene_action_user);
            s_adv_scene_action_hook(ADVISOR_SCENE_ACTION_ATTACH, row,
                                    animation, s_adv_scene_action_user);
        }
        advisor_apply_slot_transform(row);
        if (advisor_scene_advance_completion_check((int)animation_slot,
                                                   delta) != 0) {
            return;
        }
        advisor_state_word_set(advisor_state_word_peek() & ~1u);
    }
    if (s_adv_scene_action_hook != NULL) {
        if (animation != 0u)
            s_adv_scene_action_hook(ADVISOR_SCENE_ACTION_HIDE, row,
                                    animation, s_adv_scene_action_user);
        s_adv_scene_action_hook(ADVISOR_SCENE_ACTION_HIDE, -1,
                                    s_adv_scene_companion_handle,
                                s_adv_scene_action_user);
    }
}

// FUNCTION: LEGORR 0x00401210
void advisor_scene_visual_state_initialize(void)
{
    lego::resource_interface_call_slot8c(
        (void*)(uintptr_t)s_adv_scene_transform_context);
    lego::resource_interface_call_slot90(
        (void*)(uintptr_t)s_adv_scene_transform_context);
    s_adv_scene_scale = 0.96f;
}

float advisor_scene_scale_get(void)
{
    return s_adv_scene_scale;
}

void advisor_transform_apply_hook_set(AdvisorTransformApplyHook hook,
                                      void* user)
{
    s_adv_transform_apply_hook = hook;
    s_adv_transform_apply_user = user;
}

// FUNCTION: LEGORR 0x00401b30
void advisor_apply_slot_transform(int row)
{
    uint32_t animation_slot;
    uint32_t handle;
    float x;
    float y;
    animation_slot = s_adv_actions[row][0];
    handle = s_adv_animation_slots[animation_slot].handle;
    memcpy(&x, &s_adv_actions[row][5], sizeof(x));
    memcpy(&y, &s_adv_actions[row][6], sizeof(y));
    if (s_adv_transform_apply_hook != NULL)
        s_adv_transform_apply_hook(row, handle, x, y, s_adv_scene_scale,
                                   s_adv_transform_apply_user);
}

// FUNCTION: LEGORR 0x00401930
void advisor_request_reset_and_submit_sound(int row)
{
    uint32_t animation_slot;
    uint32_t animation;
    if (row < 0 || row >= ADV_ACTION_ROWS) return;
    animation_slot = s_adv_actions[row][0];
    if (animation_slot >= 11u) return;
    animation = s_adv_animation_slots[animation_slot].handle;
    if (animation != 0u) {
        /* FUN_00401930 resets the selected DDiScene, plays the configured
         * sound alias from the action record, then queues SFX 0x2a. */
        ddiscene_frame_update_and_callbacks((void*)(uintptr_t)animation,
                                            0.0f);
        sound_alias_play((int)s_adv_actions[row][3], 0);
        sound_request_queue_submit(0x2a, 0);
    }
}

// FUNCTION: LEGORR 0x004018c0
int advisor_request_admit(int row, int immediate)
{
    if ((s_adv_actions[row][8] & 0x20000u) != 0u &&
        !current_engine_mode_is_zero() &&
        (advisor_state_word_peek() & 1u) == 0u) {
        uint32_t state;
        s_adv_scene_active_row = row;
        advisor_request_reset_and_submit_sound(row);
        state = advisor_state_word_peek();
        if (immediate != 0) {
            state |= 3u;
        } else {
            state = (state & ~2u) | 1u;
        }
        advisor_state_word_set(state);
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x004011c0
void advisor_animation_end_cleanup(void)
{
    if (s_adv_scene_active_row >= 0 &&
        s_adv_scene_active_row < ADV_ACTION_ROWS) {
        uint32_t animation_slot = s_adv_actions[s_adv_scene_active_row][0];
        uint32_t animation = animation_slot < 11u
            ? s_adv_animation_slots[animation_slot].handle : 0u;
        if (animation != 0u && s_adv_scene_action_hook != NULL)
            s_adv_scene_action_hook(ADVISOR_SCENE_ACTION_HIDE,
                                    s_adv_scene_active_row, animation,
                                    s_adv_scene_action_user);
    }
    advisor_state_word_set(advisor_state_word_peek() & ~1u);
    if (s_adv_scene_action_hook != NULL)
        s_adv_scene_action_hook(ADVISOR_SCENE_ACTION_HIDE, -1,
                                s_adv_scene_companion_handle,
                                s_adv_scene_action_user);
}

// FUNCTION: LEGORR 0x004017b0
void advisor_position_record_initialize(int row, uint32_t animation_index,
                                        int position_index,
                                        uint32_t position_mode,
                                        int action_mode, uint32_t x,
                                        uint32_t y)
{
    uint32_t* rec = s_adv_actions[row];
    rec[8] = 0x20000u;
    rec[0] = animation_index;
    rec[1] = (uint32_t)position_index;
    rec[2] = position_mode;
    rec[4] = (uint32_t)action_mode;
    rec[5] = x;
    rec[6] = y;
    rec[7] = x;
    rec[8] = y;
    if (action_mode == 0x0c) {
        rec[8] |= 0x10000u;
    }
    if (position_index != 0x1a) {
        rec[8] |= 0x40000u;
    }
}

/* AdvisorPositionConfigLoad (FUN_00401530): translate the two configured
 * names to their table slots, then initialize each valid action row. The
 * caller supplies parsed records because the engine's block iterator and
 * localized action-name resolver are separate subsystems. */
// FUNCTION: LEGORR 0x00401530
int advisor_position_config_load(const AdvisorPositionConfigEntry* entries,
                                 uint32_t count)
{
    uint32_t i;
    int loaded = 0;
    if (entries == NULL) return 0;
    for (i = 0; i < count; ++i) {
        int position_index;
        int animation_index;
        uint32_t x_bits;
        uint32_t y_bits;
        if (entries[i].position_name == NULL ||
            entries[i].animation_name == NULL ||
            !advisor_position_name_to_index(entries[i].position_name,
                                            &position_index) ||
            !advisor_animation_name_to_index(entries[i].animation_name,
                                             &animation_index)) {
            continue;
        }
        memcpy(&x_bits, &entries[i].x, sizeof(x_bits));
        memcpy(&y_bits, &entries[i].y, sizeof(y_bits));
        advisor_position_record_initialize(position_index,
            (uint32_t)animation_index, entries[i].position_type,
            entries[i].position_mode, entries[i].action_mode, x_bits, y_bits);
        ++loaded;
    }
    return loaded;
}

/* AdvisorConfigSlotLookup (FUN_00401270): resolve the configured animation
 * names into the bounded animation handle table. */
// FUNCTION: LEGORR 0x00401270
int advisor_config_slots_load(AdvisorAnimationConfigLookupHook lookup,
                              void* user)
{
    int i;
    int loaded = 0;
    if (lookup == NULL) return 0;
    for (i = 0; i < 11; ++i) {
        uint32_t handle = 0u;
        float start_time = 0.0f;
        float end_time = 0.0f;
        if (lookup(s_advisor_animation_names[i], &handle, &start_time,
                   &end_time, user)) {
            start_time *= 25.0f;
            end_time *= 25.0f;
            advisor_animation_slot_install(i, handle, start_time, end_time);
            ++loaded;
        }
    }
    return loaded;
}

// FUNCTION: LEGORR 0x00401840
void advisor_action_position_and_mode_set(int row, int mode,
                                          uint32_t dword_a, uint32_t dword_b)
{
    if (row < 0 || row >= ADV_ACTION_ROWS) {
        return; /* deviation: bound the table access */
    }
    uint32_t* rec = s_adv_actions[row];
    rec[4] = (uint32_t)mode;   /* DAT base + 0x10 */
    rec[5] = dword_a;          /* DAT base + 0x14 */
    rec[6] = dword_b;          /* DAT base + 0x18 */
    if ((uint32_t)mode == 0xcu) {
        rec[8] |= 0x10000u;    /* DAT base + 0x20 */
    } else {
        rec[8] &= 0xfffeffffu;
    }
}

// FUNCTION: LEGORR 0x00401890
void advisor_action_position_get(int row, uint32_t* dword_c, uint32_t* dword_d)
{
    *dword_c = s_adv_actions[row][7]; /* DAT base + 0x1c */
    *dword_d = s_adv_actions[row][8]; /* DAT base + 0x20 */
}

uint32_t advisor_action_flags_peek(int row)
{
    return (row >= 0 && row < ADV_ACTION_ROWS) ? s_adv_actions[row][8] : 0u;
}

/* ObjectiveAdvisorTokenLookup (FUN_00453200). The corpus's table pair
 * (count word + table pointer) has no ported writers, so the model
 * starts empty (documented deviation) and an install hook lets the
 * faithful read shape be exercised. */
// GLOBAL: LEGORR 0x00555da8
static const uint32_t* s_adv_tokens;
// GLOBAL: LEGORR 0x00555da4
static uint32_t        s_adv_token_count;

void objective_advisor_tokens_install(const uint32_t* table, uint32_t count)
{
    s_adv_tokens = table;
    s_adv_token_count = count;
}

// FUNCTION: LEGORR 0x00453200
int objective_advisor_token_lookup(uint32_t index)
{
    if (index < s_adv_token_count && s_adv_tokens != NULL) {
        return (int)s_adv_tokens[index];
    }
    return 0;
}
