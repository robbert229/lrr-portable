// SPDX-License-Identifier: LGPL-3.0-or-later
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

static ObjectiveDeferredCallback s_objective_deferred_callback;
static AdvisorTimerActionFn s_advisor_timer_action;
static AdvisorTimerFinalizeFn s_advisor_timer_finalize;
static void* s_advisor_timer_user;
static LevelState* s_objective_active_level;
static char s_objective_voice_alias[256];
static int s_objective_voice_alias_pending;
/* DAT_004ffbc0 is the runtime result word. It is separate from the level's
 * configured objective mask in LevelState::objective. */
// GLOBAL: LEGORR 0x004ffbc0
static uint32_t s_objective_result_word;
static int s_objective_voice_stream_handle = -1;
static float s_objective_voice_stream_timeout;
static int s_objective_voice_result_active;
static int s_objective_tick_resolved;
static AdvisorAnimationPositionUpdateState s_advisor_position_update_state;
static AdvisorPositionUpdateValueResolveFn s_advisor_position_value_resolver;
static void* s_advisor_position_value_resolver_user;
// GLOBAL: LEGORR 0x004a6768
static int s_advisor_sound_slot = -1;
static CoordinateListHeadGetHook s_coordinate_list_head_get;
static void* s_coordinate_list_head_context;
static RewardTaskResourceRecord s_reward_task_resources[10];
static void objective_advisor_clear_active_state(void);

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
    if (callback == NULL) return;
    if (s_coordinate_list_head_get != NULL) {
        record = s_coordinate_list_head_get(
            list_key, s_coordinate_list_head_context);
    }
    else {
        const unsigned int list_index = (unsigned int)(uintptr_t)list_key;
        GameContext* const game = entity_pool_callback_context_get();
        LevelState* const level = game != NULL && game->level != NULL
            ? game->level : s_objective_active_level;
        if (level == NULL || list_index >= 0x37u)
            return;
        record = level->pool_heads[list_index];
    }
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

void reward_task_font_record_set(unsigned int slot, void* font_record)
{
    if (slot >= 10u) return;
    s_reward_task_resources[slot].font_record = font_record;
    if (font_record != NULL) s_reward_task_resources[slot].flags |= 1u;
    else s_reward_task_resources[slot].flags &= ~1u;
}

void* reward_task_font_record_get(unsigned int slot)
{
    return slot < 10u ? s_reward_task_resources[slot].font_record : NULL;
}

void* reward_task_image_surface_get(unsigned int slot)
{
    return slot < 10u ? s_reward_task_resources[slot].image_surface : NULL;
}

void reward_task_box_image_surface_set(unsigned int slot, void* surface)
{
    if (slot >= 10u) return;
    s_reward_task_resources[slot].box_image_surface = surface;
}

void* reward_task_box_image_surface_get(unsigned int slot)
{
    return slot < 10u ? s_reward_task_resources[slot].box_image_surface : NULL;
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
        if (task->box_image_surface != NULL)
            texture_release((TextureSurface*)task->box_image_surface);
        task->flags = 0;
        task->image_surface = NULL;
        task->font_record = NULL;
        task->box_image_surface = NULL;
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

static void advisor_name_tables_initialize(void)
{
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
}

// FUNCTION: LEGORR 0x004013e0
int advisor_position_name_to_index(const char* name, int* index)
{
    int i;
    for (i = 0; i < 21; ++i) {
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
    int i;
    for (i = 0; i < 11; ++i) {
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

/* Corpus FUN_00458580 — crystal/ore objective record words. */
// FUNCTION: LEGORR 0x00458580
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

/* Keep the configured objective mask in the portable level sidecar while
 * the mapped target above preserves the retail global and record writes. */
static void obj_set_resources(LevelState* s, int32_t crystal, int32_t ore)
{
    if (crystal != 0) s->objective |= LVL_OBJECTIVE_CRYSTALS;
    if (ore != 0) s->objective |= LVL_OBJECTIVE_ORE;
    obj_set_resources_target(s->word, crystal, ore);
}

/* Corpus FUN_004585c0 — block objective record words. */
// FUNCTION: LEGORR 0x004585c0
static void obj_set_block_target(uint32_t* level_record,
                                 const int32_t targets[2])
{
    s_objective_result_word |= LVL_OBJECTIVE_BLOCK;
    level_record[OBJ_OFF_BLOCK_A / 4] = (uint32_t)targets[0];
    level_record[OBJ_OFF_BLOCK_B / 4] = (uint32_t)targets[1];
}

static void obj_set_block(LevelState* s, const int32_t targets[2])
{
    s->objective |= LVL_OBJECTIVE_BLOCK;
    obj_set_block_target(s->word, targets);
}

/* Corpus FUN_004585f0 — timer objective record word. */
// FUNCTION: LEGORR 0x004585f0
static void obj_set_timer_target(uint32_t* level_record, float ticks,
                                 int hit_time_fail)
{
    s_objective_result_word |= LVL_OBJECTIVE_TIMER;
    ((float*)level_record)[OBJ_OFF_TIMER_TARGET / 4] = ticks;
    if (hit_time_fail) {
        s_objective_result_word |= LVL_OBJECTIVE_NO_ACH_ADV;
    }
}

static void obj_set_timer(LevelState* s, float ticks, int hit_time_fail)
{
    s->objective |= LVL_OBJECTIVE_TIMER;
    if (hit_time_fail) s->objective |= LVL_OBJECTIVE_NO_ACH_ADV;
    obj_set_timer_target(s->word, ticks, hit_time_fail);
}

/* Corpus FUN_00458620 — construction objective record words. */
// FUNCTION: LEGORR 0x00458620
static void obj_set_construction_target(uint32_t* level_record,
                                        uint32_t a, uint32_t b)
{
    s_objective_result_word |= LVL_OBJECTIVE_CONSTRUCTION;
    level_record[OBJ_OFF_CONSTRUCT_A / 4] = a;
    level_record[OBJ_OFF_CONSTRUCT_B / 4] = b;
}

static void obj_set_construction(LevelState* s, long a, long b)
{
    s->objective |= LVL_OBJECTIVE_CONSTRUCTION;
    obj_set_construction_target(s->word, (uint32_t)a, (uint32_t)b);
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

// FUNCTION: LEGORR 0x00459050
typedef struct ObjectiveEntityMatchContext {
    LevelState* level;
    uint32_t flags;
} ObjectiveEntityMatchContext;

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

static int s_objective_announce_target_gate = 1;

/* The retail function reads the current level from FUN_00431560 and has one
 * stack argument. `objective_load` binds that owner, while mission frame
 * updates rebind the live context before objective processing. */
// FUNCTION: LEGORR 0x00458670
void objective_announce_current(int who)
{
    LevelState* s = s_objective_active_level;
    char alias[256];
    uint32_t* engine_state;
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
        GameContext* ctx = entity_pool_callback_context_get();
        /* The original creates four linked scene-state records and raises
         * the level/scatter gates for terminal objectives. DAT_004ffe48 is
         * initialized to zero in the portable CRT window and currently has
         * no production writer, so its observed false branch supplies the
         * record flags value 4 here. The per-entity LAB_0046a3c0 callback
         * remains renderer-owned and is not represented by this dispatch. */
        scene_object_state_dispatch(ctx, 1u, 2u, 4u, NULL, NULL);
        scene_object_state_dispatch(ctx, 2u, 2u, 4u, NULL, NULL);
        scene_object_state_dispatch(ctx, 8u, 2u, 4u, NULL, NULL);
        scene_object_state_dispatch(ctx, 0x400u, 2u, 4u, NULL, NULL);
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

int objective_presentation_kind(const LevelState* s)
{
    if (s == NULL || s != s_objective_active_level)
        return OBJECTIVE_PRESENTATION_NONE;
    if ((s_objective_result_word & 1u) != 0u)
        return OBJECTIVE_PRESENTATION_BRIEFING;
    if ((s_objective_result_word & 6u) == 0u)
        return OBJECTIVE_PRESENTATION_NONE;
    if ((s_objective_result_word & 2u) != 0u)
        return OBJECTIVE_PRESENTATION_ACHIEVED;
    return (s_objective_result_word & 0x100u) != 0u
        ? OBJECTIVE_PRESENTATION_CRYSTAL_FAILED : OBJECTIVE_PRESENTATION_FAILED;
}

int objective_briefing_active(const LevelState* s)
{
    return objective_presentation_kind(s) == OBJECTIVE_PRESENTATION_BRIEFING;
}

static void objective_presentation_cleanup(LevelState* s)
{
    objective_advisor_clear_active_state();
    if (s_objective_voice_stream_handle != -1) {
        (void)sound_stream_stop_and_release(0);
        s_objective_voice_stream_handle = -1;
    }
    unported_ledger_stubs::ledger_stub_00425c30_TransitionControl(0, 0u);
    sound_deferred_queue_drain(0);
    if (system_state_flags_get() == 0u)
        engine_debug_word_bit_set(0x1000u, 0);
    s_objective_voice_alias_pending = 0;
    s_objective_voice_alias[0] = '\0';
    s_objective_voice_result_active = 0;
    s_objective_tick_resolved = 1;
    s->obj_stream[0] = '\0';
}

int objective_briefing_acknowledge(LevelState* s)
{
    if (!objective_briefing_active(s)) return 0;
    (void)global_value_004ff9bc_store_and_return(&s->objective_pending);
    s_objective_result_word &= ~1u;
    s->objective &= ~LVL_OBJECTIVE_STILL_PENDING;
    s->objective_pending = 0;
    if ((engine_debug_word_peek() & 0x800u) != 0u) {
        media_cd_watchdog_update(1);
        engine_debug_word_bit_set(0x800u, 0);
    }
    objective_presentation_cleanup(s);
    return 1;
}

int objective_presentation_acknowledge(LevelState* s)
{
    int kind = objective_presentation_kind(s);
    if (kind == OBJECTIVE_PRESENTATION_NONE) return 0;
    if (kind == OBJECTIVE_PRESENTATION_BRIEFING)
        return objective_briefing_acknowledge(s);
    /* ObjectivePoll selects the terminal page independently, but rechecks
     * the entity gate immediately before releasing the final page. */
    if (!entity_flag_mask_callbacks_all_pass(0x40bu)) return 0;
    if (kind == OBJECTIVE_PRESENTATION_ACHIEVED) {
        s_objective_result_word &= ~2u;
        s->objective &= ~LVL_OBJECTIVE_ACHIEVED;
    } else {
        s_objective_result_word &= ~0x104u;
        s->objective &= ~(LVL_OBJECTIVE_FAILED | LVL_OBJECTIVE_CRYSTALS);
    }
    objective_presentation_cleanup(s);
    return 1;
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

/* The 21 records each occupy nine dwords (stride 0x24). Word 8 is the
 * initialization/position-mode flag word; the position getter reads words
 * 6 and 7 at +0x18/+0x1c. */
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
static void* s_adv_scene_companion_handle;
/* Controller representation of DAT_004b31bc. The CRT adapter also keeps
 * a copy; objective presentation release synchronizes their active bit. */
static uint32_t s_adv_scene_state_word;
/* Both represented words model DAT_004b31bc. A presentation release must
 * clear bit 1 in the live animation controller as well as the CRT leaf;
 * bit 0 remains owned by the normal animation-end cleanup. */
static void objective_advisor_clear_active_state(void)
{
    advisor_clear_active_state();
    s_adv_scene_state_word &= ~2u;
}

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
static AdvisorAnimationTargetConfigLookupHook s_advisor_config_target_lookup;
static void* s_advisor_config_target_lookup_user;
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

void advisor_animation_target_config_lookup_set(
    AdvisorAnimationTargetConfigLookupHook lookup, void* user)
{
    s_advisor_config_target_lookup = lookup;
    s_advisor_config_target_lookup_user = user;
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
    if (handle != 0u) {
        if (s_adv_animation_hooks.update != NULL) {
            progress = s_adv_animation_hooks.update(
                handle, delta, s_adv_animation_hooks.user);
        } else if (s_adv_animation_hooks.time_get == NULL &&
                   s_adv_animation_hooks.seek == NULL) {
            progress = ddiscene_advance_by_time(
                (void*)(uintptr_t)handle, delta);
        }
    }
    if ((s_adv_scene_state_word & 2u) == 0u) {
        return progress > 0.0f ? 0 : 1;
    }
    if (progress > 0.0f) {
        if (handle != 0u && s_adv_animation_hooks.seek != NULL) {
            const float target = s_adv_animation_slots[animation_slot].start_time
                + progress;
            s_adv_animation_hooks.seek(handle, target,
                                       s_adv_animation_hooks.user);
        } else if (handle != 0u && s_adv_animation_hooks.update == NULL &&
                   s_adv_animation_hooks.time_get == NULL) {
            const float target = s_adv_animation_slots[animation_slot].start_time
                + progress;
            (void)ddiscene_frame_update_and_callbacks(
                (void*)(uintptr_t)handle, target);
        }
        return 1;
    }
    if (handle == 0u) current = 0.0f;
    else if (s_adv_animation_hooks.time_get != NULL)
        current = s_adv_animation_hooks.time_get(
            handle, s_adv_animation_hooks.user);
    else if (s_adv_animation_hooks.update == NULL &&
             s_adv_animation_hooks.seek == NULL)
        current = ddiscene_object_length_get((void*)(uintptr_t)handle);
    else
        current = 0.0f;
    if (s_adv_animation_slots[animation_slot].end_time <= current) {
        if (handle != 0u && s_adv_animation_hooks.seek != NULL) {
            const float target =
                (current - s_adv_animation_slots[animation_slot].end_time) +
                s_adv_animation_slots[animation_slot].start_time;
            s_adv_animation_hooks.seek(handle, target,
                                       s_adv_animation_hooks.user);
        } else if (handle != 0u && s_adv_animation_hooks.update == NULL &&
                   s_adv_animation_hooks.time_get == NULL) {
            const float target =
                (current - s_adv_animation_slots[animation_slot].end_time) +
                s_adv_animation_slots[animation_slot].start_time;
            (void)ddiscene_frame_update_and_callbacks(
                (void*)(uintptr_t)handle, target);
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
    void* companion;
    /* Keep these writes in the target body: the retail initializer fills the
     * two global name tables here before it uses the scene resources. */
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
    (void)config_context;
    base_scene = scene_context_words[9]; /* param_2 + 0x24 */
    s_adv_scene_root_handle = base_scene;
    s_adv_scene_transform_context = transform_context_handle;
    companion = ddiscene_light_create(
        (void*)(uintptr_t)base_scene, 3u, 0x3f800000u,
        0x3f800000u, 0x3f800000u);
    s_adv_scene_companion_handle = companion;
    (void)ddiscene_object_pose_method_dispatch(
        companion, (void*)(uintptr_t)base_scene,
        0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);
    scene_render_lock_toggle(companion, 1);
    s_adv_scene_state_word = 0u;
}

// FUNCTION: LEGORR 0x00401990
void advisor_transition_worker(float delta)
{
    int row = s_adv_scene_active_row;
    uint32_t animation_slot;
    uint32_t animation;
    animation_slot = s_adv_actions[row][0];
    animation = s_adv_animation_slots[animation_slot].handle;
    if (animation != 0u && (s_adv_scene_state_word & 1u) != 0u &&
        !current_engine_mode_is_zero()) {
        if ((s_adv_actions[row][8] & 0x40000u) != 0u &&
            s_adv_scene_action_hook != NULL) {
            s_adv_scene_action_hook(ADVISOR_SCENE_ACTION_NOTIFY, row,
                                    animation, s_adv_scene_action_user);
        }
        scene_render_lock_toggle((void*)(uintptr_t)animation, 0);
        scene_render_lock_toggle(s_adv_scene_companion_handle, 0);
        if (s_adv_scene_action_hook != NULL) {
            s_adv_scene_action_hook(ADVISOR_SCENE_ACTION_ATTACH, row,
                                    animation, s_adv_scene_action_user);
        }
        advisor_apply_slot_transform(row);
        if (advisor_scene_advance_completion_check((int)animation_slot,
                                                   delta) != 0) {
            return;
        }
        s_adv_scene_state_word &= ~1u;
    }
    if (animation != 0u)
        scene_render_lock_toggle((void*)(uintptr_t)animation, 1);
    scene_render_lock_toggle(s_adv_scene_companion_handle, 1);
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
    memcpy(&x, &s_adv_actions[row][4], sizeof(x));
    memcpy(&y, &s_adv_actions[row][5], sizeof(y));
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
        /* FUN_00401930 unlocks the selected DDiScene, evaluates it at time
         * zero, plays the configured sound, then queues SFX 0x2a. */
        scene_render_lock_toggle((void*)(uintptr_t)animation, 0);
        ddiscene_frame_update_and_callbacks((void*)(uintptr_t)animation,
                                            0.0f);
        sound_alias_play((int)s_adv_actions[row][2], 0);
        sound_request_queue_submit(0x2a, 0);
    }
}

// FUNCTION: LEGORR 0x004018c0
int advisor_request_admit(int row, int immediate)
{
    if ((s_adv_actions[row][8] & 0x20000u) != 0u &&
        !current_engine_mode_is_zero() &&
        (s_adv_scene_state_word & 1u) == 0u) {
        uint32_t state;
        s_adv_scene_active_row = row;
        advisor_request_reset_and_submit_sound(row);
        state = s_adv_scene_state_word;
        if (immediate != 0) {
            state |= 3u;
        } else {
            state = (state & ~2u) | 1u;
        }
        s_adv_scene_state_word = state;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x004011c0
void advisor_animation_end_cleanup(void)
{
    uint32_t animation_slot = s_adv_actions[s_adv_scene_active_row][0];
    uint32_t animation = s_adv_animation_slots[animation_slot].handle;
    if (animation != 0u)
        scene_render_lock_toggle((void*)(uintptr_t)animation, 1);
    s_adv_scene_state_word &= ~1u;
    scene_render_lock_toggle(s_adv_scene_companion_handle, 1);
}

// FUNCTION: LEGORR 0x004017b0
void advisor_position_record_initialize(int row, uint32_t animation_index,
                                        int position_type,
                                        uint32_t position_mode,
                                        int action_mode, uint32_t x,
                                        uint32_t y)
{
    uint32_t* rec = &s_adv_actions[0][0] + row * 9;
    rec[8] = 0x20000u;
    rec[0] = animation_index;
    rec[1] = (uint32_t)position_type;
    rec[2] = position_mode;
    rec[4] = x;
    rec[5] = y;
    rec[6] = x;
    rec[7] = y;
    rec[3] = (uint32_t)action_mode;
    if (action_mode == 0x0c) {
        rec[8] |= 0x10000u;
    }
    if (position_type != 0x1a) {
        rec[8] |= 0x40000u;
    }
}

/* BlockTextTokenSplit (FUN_00477440): split a mutable copy in place, keeping
 * empty fields just as the engine's token splitter does. */
static int advisor_config_tokens_split(char* value, char** tokens, int capacity)
{
    int count = 0;
    char* cursor = value;
    if (value == NULL || tokens == NULL || capacity <= 0 || *value == '\0')
        return 0;
    tokens[count++] = cursor;
    while (*cursor != '\0') {
        if (*cursor == ',') {
            *cursor = '\0';
            if (count == capacity) return count + 1;
            tokens[count++] = cursor + 1;
        }
        ++cursor;
    }
    return count;
}

/* AdvisorPositionConfigLoad (FUN_00401530): find the resolution-specific
 * section, walk its entries, and resolve the name tables used by each row.
 * The data is comma-delimited: animation, message label, sound alias, x, y,
 * and optional panel name. NULL leaves the decompile's defaults (0x1a and
 * 0x0c) intact. */
// FUNCTION: LEGORR 0x00401530
void advisor_position_config_load(BlockFile* config, int resolution_width,
                                  int resolution_height)
{
    char section_path[128];
    BlockRec* section;
    BlockRec* entry;
    if (config == NULL) return;
    advisor_name_tables_initialize();
    sprintf(section_path, "%s AdvisorPositions%ix%i", util_root_name(config),
            resolution_width, resolution_height);
    section = block_subgroup(config, section_path);
    for (entry = block_first_child(section); entry != NULL;
         entry = block_next_sibling(entry)) {
        char value[512];
        char* tokens[8];
        const char* source_value = block_value(entry);
        int token_count;
        int position_index;
        int animation_index;
        int position_type = 0x1a;
        int sound_index;
        int action_mode = 0x0c;
        float x;
        float y;
        uint32_t x_bits;
        uint32_t y_bits;
        size_t value_length;
        if (source_value == NULL || block_name(entry) == NULL ||
            !advisor_position_name_to_index(block_name(entry),
                                            &position_index)) {
            continue;
        }
        value_length = strlen(source_value);
        if (value_length >= sizeof(value)) continue;
        memcpy(value, source_value, value_length + 1u);
        token_count = advisor_config_tokens_split(value, tokens, 8);
        if (token_count < 5 || token_count > 8 ||
            !advisor_animation_name_to_index(tokens[0], &animation_index) ||
            !sound_alias_lookup(tokens[2], &sound_index)) {
            continue;
        }
        if (obj_strcasecmp(tokens[1], "NULL") != 0 &&
            !ui_message_label_index_find(tokens[1], &position_type)) {
            continue;
        }
        if (token_count > 5 && obj_strcasecmp(tokens[5], "NULL") != 0 &&
            !ui_panel_name_table_lookup(tokens[5], &action_mode)) {
            continue;
        }
        x = (float)atof(tokens[3]);
        y = (float)atof(tokens[4]);
        memcpy(&x_bits, &x, sizeof(x_bits));
        memcpy(&y_bits, &y, sizeof(y_bits));
        advisor_position_record_initialize(position_index,
            (uint32_t)animation_index, position_type,
            (uint32_t)sound_index, action_mode, x_bits, y_bits);
    }
}

/* Compatibility entry point for callers that already decoded the records. */
int advisor_position_config_entries_load(
    const AdvisorPositionConfigEntry* entries, uint32_t count)
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

/* Adapter used by portable callers with an already-resolved resource loader.
 * It intentionally has no target annotation: the retail entry takes the
 * scene and config contexts, not a function pointer and user-data pair. */
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
            int start_frame = (int)(start_time * 25.0f);
            int end_frame = (int)(end_time * 25.0f);
            advisor_animation_slot_install(i, handle, (float)start_frame,
                                           (float)end_frame);
            ++loaded;
        }
    }
    return loaded;
}

/* AdvisorConfigSlotLookup @ 0x00401270 walks the eleven configured animation
 * names. The scene/config operations are renderer-owned, so the installed
 * resolver performs the observed direct-key lookup, optional `!%s` fallback,
 * LWS resource creation and scene setup. It returns true when a config entry
 * exists even if resource creation yields handle zero; the timing words are
 * still written in that case. */
// FUNCTION: LEGORR 0x00401270
void advisor_config_slots_load_target(void* scene_context,
                                      void* config_context)
{
    int i;
    const int allow_fallback =
        (engine_feature_word_peek() & 0x400000u) == 0u;
    AdvisorAnimationTargetConfigLookupHook lookup =
        s_advisor_config_target_lookup;
    if (lookup == NULL) return;
    for (i = 0; i < 11; ++i) {
        uint32_t handle = 0u;
        float start_time = 0.0f;
        float end_time = 0.0f;
        int found = lookup(scene_context, config_context,
                           s_advisor_animation_names[i], &handle,
                           &start_time, &end_time,
                           s_advisor_config_target_lookup_user);
        if (!found && allow_fallback) {
            char fallback_key[64];
            fallback_key[0] = '!';
            strcpy(fallback_key + 1, s_advisor_animation_names[i]);
            handle = 0u;
            start_time = 0.0f;
            end_time = 0.0f;
            found = lookup(scene_context, config_context, fallback_key,
                           &handle, &start_time, &end_time,
                           s_advisor_config_target_lookup_user);
        }
        if (found) {
            const int start_frame = (int)(start_time * 25.0f);
            const int end_frame = (int)(end_time * 25.0f);
            advisor_animation_slot_install(i, handle, (float)start_frame,
                                           (float)end_frame);
        }
    }
}

// FUNCTION: LEGORR 0x00401840
void advisor_action_position_and_mode_set(int row, int mode,
                                          uint32_t dword_a, uint32_t dword_b)
{
    uint32_t* rec;
    if (row < 0 || row >= ADV_ACTION_ROWS) return;
    rec = s_adv_actions[row];
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
    if (row < 0 || row >= ADV_ACTION_ROWS || dword_c == NULL ||
        dword_d == NULL) return;
    *dword_c = s_adv_actions[row][6]; /* DAT base + 0x18 */
    *dword_d = s_adv_actions[row][7]; /* DAT base + 0x1c */
}

void advisor_animation_position_update_state_set(
    const AdvisorAnimationPositionUpdateState* state)
{
    if (state == NULL) {
        memset(&s_advisor_position_update_state, 0,
               sizeof(s_advisor_position_update_state));
        return;
    }
    if (state->action_row < 0 || state->action_row >= ADV_ACTION_ROWS ||
        state->point_count > ADVISOR_POSITION_UPDATE_MAX_POINTS) return;
    s_advisor_position_update_state = *state;
}

void advisor_animation_position_update_value_resolver_set(
    AdvisorPositionUpdateValueResolveFn resolver, void* user)
{
    s_advisor_position_value_resolver = resolver;
    s_advisor_position_value_resolver_user = user;
}

/* AdvisorActionIndicatorRequest (FUN_0041e890).  The reference jump table
 * has one category for action 0, a second for these action IDs, and the
 * default category for every other value, including out-of-range values. */
int advisor_action_indicator_request(int action, int request_mode)
{
    static const unsigned char category5[69] = {
        0,1,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,0,0,0,1,0,1,
        1,1,0,0,0,0,0,0,1,0,0,1,0,1,0,0,0,0,1,1,1,1,0,
        1,1,0,1,1,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,1,1
    };
    EventTimerState frontend_state;
    frontend_event_timer_state_get(&frontend_state);
    frontend_state.flags |= 0x20u;
    if (request_mode != 0) frontend_state.flags |= 0x40u;
    frontend_event_timer_state_set(&frontend_state);
    s_advisor_position_update_state.action_row = action == 0 ? 4 :
        (action > 0 && action < 69 && category5[action] ? 5 : 3);
    s_advisor_position_update_state.trigger_selector = action;
    return 1;
}

/* AdvisorAnimationPositionAndRequestUpdate (FUN_0041c4f0). The live mode,
 * interaction word, scroll/lower offsets, and back-button extents come from
 * their existing owners. The point table and row/trigger tuple are the four
 * unowned DAT inputs represented by the explicit bounded state above. */
// FUNCTION: LEGORR 0x0041c4f0
static void advisor_animation_position_and_request_update_internal(
    int position_mode, int input_a, int input_b, const float* position,
    const TextureSurface* image, int typed)
{
    EventTimerState frontend_state;
    const AdvisorAnimationPositionUpdateState* state =
        &s_advisor_position_update_state;
    int matched = 0;
    int use_catalog_point = 0;
    int resolved_value = 0;
    float offset_x, offset_y;
    int width = 0, height = 0;
    float new_x, new_y;
    uint32_t old_x_bits, old_y_bits;
    float old_x, old_y;
    uint32_t new_x_bits, new_y_bits;

    frontend_event_timer_state_get(&frontend_state);
    if ((frontend_state.flags & 0x20u) == 0u ||
        global_flag_bit0_is_set() != 0u) return;

    if (state->trigger_selector == 0x49) {
        if (state->cached_input_a == input_a &&
            state->cached_input_b == input_b &&
            (typed || (s_advisor_position_value_resolver != NULL &&
            s_advisor_position_value_resolver(
                0x49, input_a, input_b, &resolved_value,
                s_advisor_position_value_resolver_user)))) {
            matched = 1;
        }
    }
    else if (position_mode == state->trigger_selector) {
        matched = 1;
        if (position_mode == 0) {
            use_catalog_point = 1;
        }
        else if (!typed && (s_advisor_position_value_resolver == NULL ||
            !s_advisor_position_value_resolver(
                position_mode, input_a, input_b, &resolved_value,
                s_advisor_position_value_resolver_user))) {
            matched = 0;
        }
    }
    if (!matched) return;

    if (use_catalog_point) {
        const int mode = (int)s_obj_sel_word;
        uint32_t point_index;
        if (mode < 0 || mode > 0x12) return;
        point_index = typed ? (uint32_t)entity_query_flag_count(mode) :
                              resource_handle_set_band_word_peek(mode);
        if (typed) {
            const FeSurroundImage* surround;
            float origin[2];
            /* Count zero aliases pointer storage in the executable. */
            if (point_index == 0u || point_index > 11u) return;
            surround = interface_surround_image_peek((int)point_index - 1);
            if (surround == NULL) return;
            interface_menu_action_layout_peek(origin);
            (void)interface_back_button_dimensions(&width, &height);
            offset_x = (frontend_state.flags & 0x80u) != 0u
                ? frontend_state.timer : frontend_state.lower;
            new_x = (float)surround->width + offset_x + (float)width * 0.5f;
            new_y = (float)surround->height + origin[1] + (float)height * 0.5f;
        }
        else {
            if (point_index >= state->point_count ||
                point_index >= ADVISOR_POSITION_UPDATE_MAX_POINTS) return;
            (void)interface_back_button_dimensions(&width, &height);
            offset_x = (frontend_state.flags & 0x80u) != 0u
                ? frontend_state.timer : frontend_state.lower;
            offset_y = state->vertical_origin;
            new_x = state->points[point_index].x + offset_x +
                    (float)width * 0.5f;
            new_y = state->points[point_index].y + offset_y +
                    (float)height * 0.5f;
        }
    }
    else {
        /* The decompile's zero mapped-value arm uses a fixed 40-pixel
         * height. A nonzero value is a pointer-like UI record whose +8
         * height has no live owner in the portable frontend. */
        if ((!typed && resolved_value != 0) || position == NULL) return;
        new_x = position[0];
        if ((frontend_state.flags & 0x80u) == 0u)
            new_x = frontend_state.lower;
        new_y = (typed && image != NULL ? (float)image->height * 0.5f
                                         : 20.0f) + position[1];
    }

    if (state->action_row < 0 || state->action_row >= ADV_ACTION_ROWS) return;
    advisor_action_position_get(state->action_row, &old_x_bits, &old_y_bits);
    memcpy(&old_x, &old_x_bits, sizeof(old_x));
    memcpy(&old_y, &old_y_bits, sizeof(old_y));
    old_x += new_x;
    old_y += new_y;
    memcpy(&new_x_bits, &old_x, sizeof(new_x_bits));
    memcpy(&new_y_bits, &old_y, sizeof(new_y_bits));
    advisor_action_position_and_mode_set(state->action_row, 0x0c,
                                          new_x_bits, new_y_bits);
    (void)advisor_request_admit(state->action_row,
                                (frontend_state.flags & 0x40u) != 0u);
    frontend_state.flags &= ~0x20u;
    frontend_event_timer_state_set(&frontend_state);
}

void advisor_animation_position_and_request_update_image(
    int position_mode, int input_a, int input_b, const float* position,
    const TextureSurface* image)
{
    advisor_animation_position_and_request_update_internal(
        position_mode, input_a, input_b, position, image, 1);
}

void advisor_animation_position_and_request_update(
    int position_mode, int input_a, int input_b, const float* position)
{
    advisor_animation_position_and_request_update_internal(
        position_mode, input_a, input_b, position, NULL, 0);
}

uint32_t advisor_action_flags_peek(int row)
{
    return (row >= 0 && row < ADV_ACTION_ROWS) ? s_adv_actions[row][8] : 0u;
}

typedef struct ObjectiveAdvisorResourceToken {
    const char* name;
    uintptr_t handle;
} ObjectiveAdvisorResourceToken;

/* The reference's DAT_00555da0 buffer owns every text/name pointer below.
 * Keep one copied buffer and grow the three tables as their entries appear. */
static char* s_adv_token_source;
static const char** s_adv_text_tokens;
static uint32_t s_adv_text_token_count;
static ObjectiveAdvisorResourceToken* s_adv_image_tokens;
static uint32_t s_adv_image_token_count;
static ObjectiveAdvisorResourceToken* s_adv_sound_tokens;
static uint32_t s_adv_sound_token_count;
static ObjectiveAdvisorTokenReleaseFn s_adv_token_release;
static void* s_adv_token_release_user;
static uint32_t s_adv_token_generation;

/* Compatibility table for the old 32-bit pointer-return interface and its
 * test installer. Parsed callers should use objective_advisor_token_text_get
 * on 64-bit hosts, where a native pointer cannot fit in the target dword. */
// GLOBAL: LEGORR 0x00555da8
static const uint32_t* s_adv_tokens;
// GLOBAL: LEGORR 0x00555da4
static uint32_t s_adv_token_count;

void objective_advisor_tokens_release(void)
{
    uint32_t i;
    ++s_adv_token_generation;
    if (s_adv_token_generation == 0u) ++s_adv_token_generation;
    if (s_adv_token_release != NULL) {
        for (i = 0; i < s_adv_image_token_count; ++i) {
            if (s_adv_image_tokens[i].handle != (uintptr_t)0)
                s_adv_token_release(1, s_adv_image_tokens[i].handle,
                                    s_adv_token_release_user);
        }
        for (i = 0; i < s_adv_sound_token_count; ++i) {
            if (s_adv_sound_tokens[i].handle != (uintptr_t)0)
                s_adv_token_release(2, s_adv_sound_tokens[i].handle,
                                    s_adv_token_release_user);
        }
    }
    free(s_adv_token_source);
    free(s_adv_text_tokens);
    free(s_adv_image_tokens);
    free(s_adv_sound_tokens);
    s_adv_token_source = NULL;
    s_adv_text_tokens = NULL;
    s_adv_text_token_count = 0u;
    s_adv_image_tokens = NULL;
    s_adv_image_token_count = 0u;
    s_adv_sound_tokens = NULL;
    s_adv_sound_token_count = 0u;
    s_adv_token_release = NULL;
    s_adv_token_release_user = NULL;
    s_adv_tokens = NULL;
    s_adv_token_count = 0u;
}

static char* objective_advisor_next_line(char* source, uint32_t length,
                                         uint32_t* cursor)
{
    uint32_t start;
    while (*cursor < length && (unsigned char)source[*cursor] < 0x20u) {
        source[*cursor] = '\0';
        ++*cursor;
    }
    if (*cursor >= length) return NULL;
    start = *cursor;
    while (*cursor < length && (unsigned char)source[*cursor] >= 0x20u) {
        if (source[*cursor] == '_') source[*cursor] = ' ';
        ++*cursor;
    }
    if (*cursor < length) {
        source[*cursor] = '\0';
        ++*cursor;
    }
    return source + start;
}

static int objective_advisor_text_append(const char* text)
{
    const char** grown;
    if (s_adv_text_token_count == 0xffffffffu / sizeof(*grown)) return 0;
    grown = (const char**)realloc((void*)s_adv_text_tokens,
        (size_t)(s_adv_text_token_count + 1u) * sizeof(*grown));
    if (grown == NULL) return 0;
    s_adv_text_tokens = grown;
    s_adv_text_tokens[s_adv_text_token_count++] = text;
    return 1;
}

static int objective_advisor_resource_append(
    ObjectiveAdvisorResourceToken** table, uint32_t* count,
    const char* name, uintptr_t handle)
{
    ObjectiveAdvisorResourceToken* grown;
    if (*count == 0xffffffffu / sizeof(*grown)) return 0;
    grown = (ObjectiveAdvisorResourceToken*)realloc(*table,
        (size_t)(*count + 1u) * sizeof(*grown));
    if (grown == NULL) return 0;
    *table = grown;
    grown[*count].name = name;
    grown[*count].handle = handle;
    ++*count;
    return 1;
}

int objective_advisor_tokens_parse(const char* source, uint32_t source_length,
                                   ObjectiveAdvisorTokenResolveFn resolve,
                                   ObjectiveAdvisorTokenReleaseFn release,
                                   void* user)
{
    uint32_t cursor = 0u;
    if (source == NULL && source_length != 0u) return 0;
    objective_advisor_tokens_release();
    s_adv_token_release = release;
    s_adv_token_release_user = user;
    if (source_length == 0u) return 1;
    s_adv_token_source = (char*)malloc((size_t)source_length + 1u);
    if (s_adv_token_source == NULL) {
        objective_advisor_tokens_release();
        return 0;
    }
    memcpy(s_adv_token_source, source, source_length);
    s_adv_token_source[source_length] = '\0';

    while (cursor < source_length) {
        char* line = objective_advisor_next_line(s_adv_token_source,
                                                  source_length, &cursor);
        int kind = 0;
        if (line == NULL) break;
        if (*line == ':' || *line == '$') {
            const char* name = line + 1;
            char* value_line;
            uintptr_t handle;
            kind = (*line == ':') ? 1 : 2;
            value_line = objective_advisor_next_line(s_adv_token_source,
                                                      source_length, &cursor);
            if (value_line == NULL || *name == '\0' || *value_line == '\0' ||
                resolve == NULL) continue;
            handle = resolve(kind, value_line, user);
            if (handle == 0u) continue;
            if (kind == 1) {
                if (!objective_advisor_resource_append(&s_adv_image_tokens,
                        &s_adv_image_token_count, name, handle)) {
                    if (release != NULL) release(kind, handle, user);
                    continue;
                }
            } else {
                if (!objective_advisor_resource_append(&s_adv_sound_tokens,
                        &s_adv_sound_token_count, name, handle)) {
                    if (release != NULL) release(kind, handle, user);
                    continue;
                }
            }
        } else if (*line != '\0') {
            if (!objective_advisor_text_append(line)) {
                objective_advisor_tokens_release();
                return 0;
            }
        }
    }
    s_adv_token_count = s_adv_text_token_count;
    return 1;
}

static uintptr_t objective_advisor_live_token_resolve(int kind,
                                                       const char* value,
                                                       void* user)
{
    (void)user;
    if (value == NULL || value[0] == '\0') return (uintptr_t)0;
    if (kind == 1) {
        TextureSurface* image = image_surface_load(value, 0u, 0u);
        return (uintptr_t)image;
    }
    if (kind == 2) {
        int slot = sound_bank_entry_load(value, 0, 0, 0);
        /* Zero is a valid bank slot; reserve encoded zero for a load miss. */
        return slot >= 0 ? (uintptr_t)(uint32_t)(slot + 1) : (uintptr_t)0;
    }
    return (uintptr_t)0;
}

static void objective_advisor_live_token_release(int kind, uintptr_t handle,
                                                  void* user)
{
    (void)user;
    if (handle == (uintptr_t)0) return;
    if (kind == 1) {
        texture_release((TextureSurface*)handle);
    }
    else if (kind == 2) {
        int slot = (int)(uint32_t)handle - 1;
        if (slot >= 0) (void)sound_bank_entry_release(slot);
    }
}

// FUNCTION: LEGORR 0x00452e70
int objective_advisor_token_file_load(const char* path)
{
    Resource* resource;
    int32_t size;
    int result;
    objective_advisor_tokens_release();
    if (path == NULL || path[0] == '\0') return 0;
    resource = resource_open(path, RESOURCE_AUTO);
    if (resource == NULL) return 0;
    size = resource_read(resource);
    if (size < 0) {
        resource_free(resource);
        return 0;
    }
    result = objective_advisor_tokens_parse(
        (const char*)resource->data, (uint32_t)size,
        objective_advisor_live_token_resolve,
        objective_advisor_live_token_release, NULL);
    resource_free(resource);
    return result;
}

void objective_advisor_tokens_install(const uint32_t* table, uint32_t count)
{
    objective_advisor_tokens_release();
    s_adv_tokens = table;
    s_adv_token_count = count;
}

// FUNCTION: LEGORR 0x00453200
int objective_advisor_token_lookup(uint32_t index)
{
    if (index < s_adv_token_count && s_adv_tokens != NULL) {
        return (int)s_adv_tokens[index];
    }
    if (index < s_adv_text_token_count && s_adv_text_tokens != NULL)
        return (int)(uint32_t)(uintptr_t)s_adv_text_tokens[index];
    return 0;
}

const char* objective_advisor_token_text_get(uint32_t index)
{
    return (index < s_adv_text_token_count && s_adv_text_tokens != NULL)
        ? s_adv_text_tokens[index] : NULL;
}

uint32_t objective_advisor_image_token_count(void)
{
    return s_adv_image_token_count;
}

const char* objective_advisor_image_token_name_get(uint32_t index)
{
    return (index < s_adv_image_token_count && s_adv_image_tokens != NULL)
        ? s_adv_image_tokens[index].name : NULL;
}

uintptr_t objective_advisor_image_token_handle_get(uint32_t index)
{
    return (index < s_adv_image_token_count && s_adv_image_tokens != NULL)
        ? s_adv_image_tokens[index].handle : 0u;
}

void* objective_advisor_image_token_resource_get(uint32_t index)
{
    return (void*)objective_advisor_image_token_handle_get(index);
}

uint32_t objective_advisor_sound_token_count(void)
{
    return s_adv_sound_token_count;
}

const char* objective_advisor_sound_token_name_get(uint32_t index)
{
    return (index < s_adv_sound_token_count && s_adv_sound_tokens != NULL)
        ? s_adv_sound_tokens[index].name : NULL;
}

uintptr_t objective_advisor_sound_token_handle_get(uint32_t index)
{
    return (index < s_adv_sound_token_count && s_adv_sound_tokens != NULL)
        ? s_adv_sound_tokens[index].handle : 0u;
}

uint32_t objective_advisor_token_generation_get(void)
{
    return s_adv_token_generation;
}

/* ObjectiveSidecarAppendGate (FUN_0043bc40). The entity's stats accessors,
 * activity, owner, carried-object, selection, task, and OSF state are live in
 * the portable model. Scene-object creation/labels remain renderer-owned. */
static void objective_sidecar_activity_set(EntitySlot* entity, int activity)
{
    ai_set_activity(entity, activity, 0);
    ai_idle_chain_record_attach(entity);
    entity->activity_last = entity->activity;
    entity->activity_timer = 0.0f;
}

static void objective_sidecar_owner_detach(GameContext* ctx,
                                           EntitySlot* entity)
{
    EntitySlot* owner = (EntitySlot*)entity->owner_36c;
    if (owner == NULL) return;
    selection_target_command_update(ctx, entity);
    owner->flags_3e4 &= ~8u;
    owner->owner_36c = NULL;
    entity->owner_36c = NULL;
}

// FUNCTION: LEGORR 0x0043bc40
int objective_sidecar_append_gate(EntitySlot* entity)
{
    GameContext* ctx;
    LevelState* level;
    uint32_t stats_flags;
    int cell_x = 0;
    int cell_y = 0;
    int cell_xy[2];
    int cell_valid = 0;
    EntitySlot* owner;

    if (entity == NULL) return 0;
    ctx = entity->owner_ctx;
    if (ctx == NULL) ctx = entity_pool_callback_context_get();
    level = ctx != NULL ? ctx->level : NULL;

    if (entity->type != 4
        && ((entity->flags_3e4 & 0x08000000u) != 0u
            || (entity->flags_3e0 & 0x00200000u) != 0u)) return 0;
    if (entity->field_340 > 0.0f) return 0;

    stats_flags = entity_stats_word_get(entity);

    /* FUN_0043bc40 first completes a linked +0x3cc child when the source
     * stats word has one of the three documented state groups. EntitySlot's
     * spawn_child_3cc mirror owns that exact link. */
    if ((stats_flags & 0x80000070u) != 0u &&
        entity->spawn_child_3cc != NULL &&
        (entity->spawn_child_3cc->flags_3e0 & 0x00200000u) != 0u) {
        EntitySlot* child = entity->spawn_child_3cc;
        child->field_340 = -1.0f;
        child->flags_3e0 &= ~0x00200000u;
        entity_kind_record_update_dispatch(entity);
        ai_set_activity(child, 0, 1);
        activity_register_bookkeeping(child);
    }

    /* The first linked-activity path reads the pointer at +0x3cc. That field
     * is represented by spawn_child_3cc; it is distinct from carried-object
     * (+0x308) and reciprocal pair (+0x3c0) links. */

    if ((entity->flags_3e8 & 0x00800000u) != 0u) {
        objective_sidecar_owner_detach(ctx, entity);
        if (entity->type == 2 && (entity->flags_3ec & 8u) != 0u
            && ctx != NULL) {
            osf_add(ctx, (int32_t)entity->flags_3f0,
                    (int32_t)entity->indexed_record_slot_330,
                    entity->name);
        }
        /* FUN_0046cf90 is a resource/display callback with no game-side
         * equivalent. The remaining spawn/finalize/teardown operations are
         * represented by existing module APIs. */
        if (entity->type == 1) {
            /* Deliberately no guessed replacement for FUN_0046cf90. */
        }
        lego::entity_kind_one_random_spawn_trigger(entity);
        if (entity->type != 5) {
            if (entity->type == 4)
                entity_destroy_tracked_pointer_cleanup(entity);
            else
                (void)entity_teardown(entity);
            return 1;
        }
        entity_type5_finalize_if_allowed(entity);
        return 1;
    }

    if (entity->type == 3) {
        if ((entity_state_flags_get(entity) & 0x8000u) == 0u) {
            if ((entity_state_flags_get(entity) & 0x00100000u) == 0u)
                return 0;
            entity_state_reset(entity, 0, 1);
            entity->flags_3e0 |= 0x00020000u;
            objective_sidecar_activity_set(entity, 0x0d);
            return 1;
        }
        if ((entity->flags_3e0 & 0x00100000u) != 0u) return 0;
        entity_action_route_state_initialize(entity);
        return 1;
    }

    /* The type-4 path branch reads the dword at +0x390 and the byte at
     * +0x3ec. Retail applies the BuildingTypes cell-line effects only when
     * both veto bits are clear, then marks the one-shot bit before calling
     * 0x409d50. */
    if (entity->type == 4 && (entity->flags_390 & 4u) == 0u &&
        (entity->flags_3ec & 8u) == 0u) {
        if ((entity->flags_3e4 & 0x00100000u) != 0u) return 0;
        entity->flags_3e4 |= 0x00100000u;
        unported_ledger_stubs::entity_cell_path_effects_apply_current(entity);
        objective_sidecar_activity_set(entity, 0x2a);
        task_registry_remove_type10_entries_for_entity(entity);
        return 1;
    }

    if (entity->type != 1 && entity->type != 2 && entity->type != 0xb
        && entity->type != 4 && entity->type != 6) return 0;
    if (entity->type == 6) {
        entity->flags_3e8 |= 0x00800000u;
        return 1;
    }

    if ((entity->flags_3e8 & 0x00040000u) == 0u
        || (entity->flags_3e0 & 0x00400000u) != 0u) return 0;

    entity_position_to_terrain_cell(entity, level, &cell_x, &cell_y);
    cell_valid = level != NULL;
    if ((entity->flags_3ec & 8u) != 0u)
        lego::entity_kind_one_random_spawn_trigger(entity);

    if ((entity->flags_3ec & 0x80u) != 0u) {
        owner = (EntitySlot*)entity->owner_36c;
        if (owner != NULL) owner->flags_3ec &= ~0x80u;
        entity->owner_36c = NULL;
    }

    if (entity->owner_36c != NULL && cell_valid) {
        EntitySlot* carried = (EntitySlot*)entity->carry_link_308;
        owner = (EntitySlot*)entity->owner_36c;
        if ((stats_flags & 1u) != 0u
            && (entity->flags_3e0 & 0x400u) != 0u
            && carried != NULL
            && terrain_cell_route_and_occupancy_check(
                   carried, level, cell_x, cell_y, cell_x, cell_y, 1) == 0) {
            carried->field_340 = -1.0f;
        }
        if ((stats_flags & 0x20000u) == 0u
            || terrain_cell_route_and_occupancy_check(
                   owner, level, cell_x, cell_y, cell_x, cell_y, 1) == 0) {
            objective_sidecar_owner_detach(ctx, entity);
        }
        else {
            owner->field_340 = -1.0f;
        }
    }

    if (entity->type == 0xb)
        terrain_entity_cell_record_remove(entity);

    if (cell_valid) {
        cell_xy[0] = cell_x;
        cell_xy[1] = cell_y;
        if (entity->type == 2 || entity->type == 1 || entity->type == 4)
            message_dispatch(0x26, 0, 0, cell_xy);
        if (entity->type == 2)
            message_dispatch(10, 0, 0, cell_xy);
        else if (entity->type == 1)
            message_dispatch(0xb, 0, 0, cell_xy);
        else if (entity->type == 4)
            message_dispatch(0xc, 0, 0, cell_xy);
    }

    if ((entity->flags_3e4 & 2u) == 0u) {
        entity_state_reset(entity, 0, 1);
        entity->activity_last = entity->activity;
    }
    else {
        entity_state_reset(entity, 1, 1);
    }
    entity->flags_3e0 |= 0x00400000u;
    /* Renderer sidecar allocation, transform copy, and labels are absent
     * from GameContext. The final selection detach is directly represented. */
    (void)selection_entity_remove(entity);
    selection_object_detach(entity);
    return 1;
}

namespace unported_ledger_stubs {

// CoordinateListCallbackVisit
// FUNCTION: LEGORR 0x00456cb0
void ledger_stub_00456cb0_CoordinateListCallbackVisit(
    void* list_key, CoordinateListVisitCallback callback, void* user)
{
    coordinate_list_callback_visit(list_key, callback, user);
}


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// AdvisorAnimationSceneLookupAndRequest
// FUNCTION: LEGORR 0x0045c670
int ledger_stub_0045c670_AdvisorAnimationSceneLookupAndRequest(
    int scene, int animation, int request_mode)
{
    uint32_t slot;
    if (!advisor_animation_slot_id_resolve((uint32_t)scene,
                                           (uint32_t)animation, &slot))
        return 0;
    (void)advisor_request_admit((int)slot, request_mode);
    return 1;
}



} // namespace unported_ledger_stubs
