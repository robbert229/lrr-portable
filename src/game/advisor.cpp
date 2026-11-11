// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-001 Advisor.c: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-001.csv. */
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
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include "heap.hpp"
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
#include "camera.hpp"
#include "frameloop.hpp"
#include "mainsim.hpp"
#include "tickers.hpp"
#include "game/building_scene.hpp"
#include "game/actor_scene.hpp"
#include "game/scene_render.hpp"
#include "scene_pool.hpp"
#include "model.hpp"
#include "objective_for_advisor.hpp"
#include "crt/crt_for_advisor.hpp"
#include "gods98/engine_for_advisor.hpp"

/* The corpus's normalize numerator is the .rdata dword 1.0 at 0x49e008
 * (objdump-verified fdivr operand in FUN_00401750); a named const static
 * keeps the fld operand resolvable by Reccmp. */
// GLOBAL: LEGORR 0x0049e008
static const float s_normalize_unit = 1.0f;   /* DAT_0049e008, verified */

// GLOBAL: LEGORR 0x004a1030
static const char s_advisor_anim_talk_top[] = "Advisor_Anim_Talk_Top";
// GLOBAL: LEGORR 0x004a1048
static const char s_advisor_anim_point_up[] = "Advisor_Anim_Point_Up";
// GLOBAL: LEGORR 0x004a1060
static const char s_advisor_anim_point_nw[] = "Advisor_Anim_Point_NW";
// GLOBAL: LEGORR 0x004a1078
static const char s_advisor_anim_point_w[] = "Advisor_Anim_Point_W";
// GLOBAL: LEGORR 0x004a1090
static const char s_advisor_anim_point_sw[] = "Advisor_Anim_Point_SW";
// GLOBAL: LEGORR 0x004a10a8
static const char s_advisor_anim_point_s[] = "Advisor_Anim_Point_S";
// GLOBAL: LEGORR 0x004a10c0
static const char s_advisor_anim_point_se[] = "Advisor_Anim_Point_SE";
// GLOBAL: LEGORR 0x004a10d8
static const char s_advisor_anim_point_e[] = "Advisor_Anim_Point_E";
// GLOBAL: LEGORR 0x004a10f0
static const char s_advisor_anim_point_ne[] = "Advisor_Anim_Point_NE";
// GLOBAL: LEGORR 0x004a1108
static const char s_advisor_anim_point_n[] = "Advisor_Anim_Point_N";
// GLOBAL: LEGORR 0x004a1120
static const char s_advisor_anim_point_to_map[] = "Advisor_Anim_PointToMap";
// GLOBAL: LEGORR 0x004a1138
static const char s_advisor_name_talk_in_game[] = "Advisor_TalkInGame";
// GLOBAL: LEGORR 0x004a114c
static const char s_advisor_name_control_zoom_out[] = "Advisor_PanelPoint_ControlZoomOut";
// GLOBAL: LEGORR 0x004a1170
static const char s_advisor_name_control_zoom_in[] = "Advisor_PanelPoint_ControlZoomIn";
// GLOBAL: LEGORR 0x004a1194
static const char s_advisor_name_top_go_back[] = "Advisor_PanelPoint_TopPanelGoBack";
// GLOBAL: LEGORR 0x004a11b8
static const char s_advisor_name_top_call_to_arms[] = "Advisor_PanelPoint_TopPanelCallToArms";
// GLOBAL: LEGORR 0x004a11e0
static const char s_advisor_name_top_priorities[] = "Advisor_PanelPoint_TopPanelPriorities";
// GLOBAL: LEGORR 0x004a1208
static const char s_advisor_name_top_options[] = "Advisor_PanelPoint_TopPanelOptions";
// GLOBAL: LEGORR 0x004a122c
static const char s_advisor_name_top_info[] = "Advisor_PanelPoint_TopPanelInfo";
// GLOBAL: LEGORR 0x004a124c
static const char s_advisor_name_info_dock_close[] = "Advisor_PanelPoint_InfoDockClose";
// GLOBAL: LEGORR 0x004a1270
static const char s_advisor_name_info_dock_goto[] = "Advisor_PanelPoint_InfoDockGoto";
// GLOBAL: LEGORR 0x004a1290
static const char s_advisor_name_radar_map_view[] = "Advisor_PanelPoint_RadarMapView";
// GLOBAL: LEGORR 0x004a12b0
static const char s_advisor_name_radar_zoom_out[] = "Advisor_PanelPoint_RadarZoomOut";
// GLOBAL: LEGORR 0x004a12d0
static const char s_advisor_name_radar_zoom_in[] = "Advisor_PanelPoint_RadarZoomIn";
// GLOBAL: LEGORR 0x004a12f0
static const char s_advisor_name_radar_tagged[] = "Advisor_PanelPoint_RadarTaggedObjectView";
// GLOBAL: LEGORR 0x004a131c
static const char s_advisor_name_radar_toggle[] = "Advisor_PanelPoint_RadarToggle";
// GLOBAL: LEGORR 0x004a133c
static const char s_advisor_name_icon_top[] = "Advisor_IconPoint_TopButtons";
// GLOBAL: LEGORR 0x004a135c
static const char s_advisor_name_icon_back[] = "Advisor_IconPoint_BackButton";
// GLOBAL: LEGORR 0x004a137c
static const char s_advisor_name_icon_normal[] = "Advisor_IconPoint_Normal";
// GLOBAL: LEGORR 0x004a1398
static const char s_advisor_name_objective_failed[] = "Advisor_ObjectiveFailed";
// GLOBAL: LEGORR 0x004a13b0
static const char s_advisor_name_objective_achieved[] = "Advisor_ObjectiveAcheived";
// GLOBAL: LEGORR 0x004a13cc
static const char s_advisor_name_objective[] = "Advisor_Objective";

// GLOBAL: LEGORR 0x004b2db0
static const char* s_advisor_position_names[21];

/* The 21 records each occupy nine dwords (stride 0x24). Word 8 is the
 * initialization/position-mode flag word; the position getter reads words
 * 6 and 7 at +0x18/+0x1c. */
// GLOBAL: LEGORR 0x004b2e04
uint32_t s_adv_actions[ADV_ACTION_ROWS][9];
// GLOBAL: LEGORR 0x004b30f8
const char* s_advisor_animation_names[11];

// GLOBAL: LEGORR 0x004b3124
AdvisorAnimationSlot s_adv_animation_slots[11];
// GLOBAL: LEGORR 0x004b31a8
static int s_adv_scene_active_row;
// GLOBAL: LEGORR 0x004b31ac
static uint32_t s_adv_scene_root_handle;
// GLOBAL: LEGORR 0x004b31b0
static uint32_t s_adv_scene_transform_context;
// GLOBAL: LEGORR 0x004b31b4
float s_adv_scene_scale;
// GLOBAL: LEGORR 0x004b31b8
static void* s_adv_scene_companion_handle;

/* --- the unowned DAT word-store band (see crt.hpp) -------------------------- */
// GLOBAL: LEGORR 0x004b31bc
uint32_t s_advisor_state_word = 0u; /* DAT_004b31bc */

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static int obj_strcasecmp(const char* a, const char* b);
static int advisor_config_tokens_split(char* value, char** tokens, int capacity);
static void advisor_name_tables_initialize(void);

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
// FUNCTION: LEGORR 0x00401210
void advisor_scene_visual_state_initialize(void)
{
    lego::resource_interface_call_slot8c(
        (void*)(uintptr_t)s_adv_scene_transform_context);
    lego::resource_interface_call_slot90(
        (void*)(uintptr_t)s_adv_scene_transform_context);
    s_adv_scene_scale = 0.96f;
}

// FUNCTION: LEGORR 0x00401240
float vec2_dist(const float* a, const float* b) /* FUN_00401240 */
{
    float dy = a[1] - b[1];
    float dx = a[0] - b[0];
    return sqrtf(dy * dy + dx * dx);
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

// FUNCTION: LEGORR 0x004013a0
float vec_len(const float* a) /* FUN_004013a0 */
{
    return sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
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

// FUNCTION: LEGORR 0x00401420
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) float vec_dist(const float* a, const float* b)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 8]
        fld dword ptr [eax + 8]
        fld dword ptr [eax + 4]
        fsub dword ptr [ecx + 4]
        fxch st(1)
        fsub dword ptr [ecx + 8]
        fld dword ptr [eax]
        fsub dword ptr [ecx]
        fxch st(2)
        fst dword ptr [esp + 4]
        fld st(1)
        fxch st(1)
        fmul dword ptr [esp + 4]
        fxch st(1)
        fmul st, st(2)
        fxch st(3)
        fst dword ptr [esp + 4]
        fmul dword ptr [esp + 4]
        fxch st(3)
        faddp st(1), st
        fxch st(2)
        faddp st(2), st
        fxch st(1)
        fsqrt
        fstp st(1)
        ret
    }
}
#else
float vec_dist(const float* a, const float* b) /* FUN_00401420 */
{
    float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return sqrtf(dx * dx + dy * dy + dz * dz);
}
#endif
// FUNCTION: LEGORR 0x00401470
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void vec_cross(float* dst, const float* b,
                                 const float* c)
{
    __asm {
        mov edx, dword ptr [esp + 8]
        mov ecx, dword ptr [esp + 0x0c]
        mov eax, dword ptr [esp + 4]
        fld dword ptr [edx + 8]
        fld dword ptr [ecx + 8]
        fmul dword ptr [edx + 4]
        fxch st(1)
        fmul dword ptr [ecx + 4]
        fsubp st(1), st
        fstp dword ptr [eax]
        fld dword ptr [ecx + 8]
        fld dword ptr [edx + 8]
        fmul dword ptr [ecx]
        fxch st(1)
        fmul dword ptr [edx]
        fsubp st(1), st
        fstp dword ptr [eax + 4]
        fld dword ptr [edx + 4]
        fld dword ptr [ecx + 4]
        fmul dword ptr [edx]
        fxch st(1)
        fmul dword ptr [ecx]
        fsubp st(1), st
        fstp dword ptr [eax + 8]
        ret
    }
}
#else
void vec_cross(float* dst, const float* b, const float* c) /* FUN_00401470 */
{
    dst[0] = b[1] * c[2] - b[2] * c[1];
    dst[1] = b[2] * c[0] - b[0] * c[2];
    dst[2] = b[0] * c[1] - b[1] * c[0];
}
#endif

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

// FUNCTION: LEGORR 0x00401500
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void vec_add(float* dst, const float* a, const float* b)
{
    __asm {
        mov ecx, dword ptr [esp + 0x0c]
        mov edx, dword ptr [esp + 8]
        mov eax, dword ptr [esp + 4]
        fld dword ptr [ecx]
        fadd dword ptr [edx]
        fstp dword ptr [eax]
        fld dword ptr [edx + 4]
        fadd dword ptr [ecx + 4]
        fstp dword ptr [eax + 4]
        fld dword ptr [edx + 8]
        fadd dword ptr [ecx + 8]
        fstp dword ptr [eax + 8]
        ret
    }
}
#else
void vec_add(float* dst, const float* a, const float* b) /* FUN_00401500 */
{
    dst[0] = a[0] + b[0];
    dst[1] = a[1] + b[1];
    dst[2] = a[2] + b[2];
}
#endif

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
    sprintf(section_path, "%s::AdvisorPositions%ix%i", util_root_name(config),
            resolution_width, resolution_height);
    section = block_subgroup(config, section_path);
    for (entry = section; entry != NULL;
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

// FUNCTION: LEGORR 0x004016c0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void vec_sub(float* dst, const float* a, const float* b)
{
    __asm {
        mov ecx, dword ptr [esp + 8]
        mov edx, dword ptr [esp + 0x0c]
        mov eax, dword ptr [esp + 4]
        fld dword ptr [ecx]
        fsub dword ptr [edx]
        fstp dword ptr [eax]
        fld dword ptr [ecx + 4]
        fsub dword ptr [edx + 4]
        fstp dword ptr [eax + 4]
        fld dword ptr [ecx + 8]
        fsub dword ptr [edx + 8]
        fstp dword ptr [eax + 8]
        ret
    }
}
#else
void vec_sub(float* dst, const float* a, const float* b) /* FUN_004016c0 */
{
    dst[0] = a[0] - b[0];
    dst[1] = a[1] - b[1];
    dst[2] = a[2] - b[2];
}
#endif

// FUNCTION: LEGORR 0x004016f0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) float vec_dot(const float* a, const float* b)
{
    __asm {
        mov eax, dword ptr [esp + 8]
        mov ecx, dword ptr [esp + 4]
        fld dword ptr [eax + 4]
        fld dword ptr [ecx + 8]
        fxch st(1)
        fmul dword ptr [ecx + 4]
        fld dword ptr [eax]
        fxch st(2)
        fmul dword ptr [eax + 8]
        fxch st(2)
        fmul dword ptr [ecx]
        fxch st(2)
        faddp st(1), st
        fxch st(1)
        faddp st(1), st
        ret
    }
}
#else
float vec_dot(const float* a, const float* b) /* FUN_004016f0 */
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
#endif
// FUNCTION: LEGORR 0x00401720
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void vec_scale(float* dst, const float* src, float s)
{
    __asm {
        mov ecx, dword ptr [esp + 8]
        mov eax, dword ptr [esp + 4]
        fld dword ptr [esp + 0x0c]
        fmul dword ptr [ecx]
        fstp dword ptr [eax]
        fld dword ptr [ecx + 4]
        fmul dword ptr [esp + 0x0c]
        fstp dword ptr [eax + 4]
        fld dword ptr [ecx + 8]
        fmul dword ptr [esp + 0x0c]
        fstp dword ptr [eax + 8]
        ret
    }
}
#else
void vec_scale(float* dst, const float* src, float s) /* FUN_00401720 */
{
    dst[0] = s * src[0];
    dst[1] = s * src[1];
    dst[2] = s * src[2];
}
#endif

// FUNCTION: LEGORR 0x00401750
/* FUN_00401750 keeps its x87 load/accumulate/store schedule; the zero-length
 * input remains unguarded and propagates NaNs, as pinned in test_view.cpp. */
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void vec_normalize(float* v) /* FUN_00401750 */
{
    __asm {
        push ecx
        mov eax, dword ptr [esp + 8]
        mov edx, dword ptr [eax + 8]
        mov ecx, dword ptr [eax + 4]
        fld dword ptr [eax]
        mov dword ptr [esp], edx
        mov dword ptr [esp + 8], ecx
        fld dword ptr [esp]
        fld dword ptr [esp + 8]
        fmul dword ptr [esp + 8]
        fxch st(1)
        fmul dword ptr [esp]
        fld st(2)
        fmul st, st(3)
        fxch st(1)
        faddp st(2), st
        faddp st(1), st
        fsqrt
        fstp st(1)
        fdivr dword ptr s_normalize_unit
        fld st(0)
        fld st(1)
        fxch st(1)
        fmul dword ptr [eax]
        fxch st(1)
        fmul dword ptr [eax + 4]
        fxch st(2)
        fmul dword ptr [eax + 8]
        fxch st(1)
        fstp dword ptr [eax]
        fxch st(1)
        fstp dword ptr [eax + 4]
        fstp dword ptr [eax + 8]
        pop ecx
        ret
    }
}
#else
void vec_normalize(float* v) /* FUN_00401750 (zero length unguarded, as corpus) */
{
    float s = s_normalize_unit / sqrtf(v[0] * v[0] + (v[1] * v[1] + v[2] * v[2]));
    v[0] = s * v[0];
    v[1] = s * v[1];
    v[2] = s * v[2];
}
#endif

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

// FUNCTION: LEGORR 0x00401b20
void advisor_clear_active_state(void)
{
    s_advisor_state_word &= ~2u;
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

/* GlobalField505f88Get (FUN_00401bf0): app-mode global read. */
// FUNCTION: LEGORR 0x00401bf0
int engine_app_mode_get(void)
{
    return s_application_mode;
}

// FUNCTION: LEGORR 0x00401c20
uint32_t global_flag_bit0_is_set(void)
{
    return s_advisor_state_word & 1u;
}

// FUNCTION: LEGORR 0x00401c30
int display_width_get(void)  { return s_display_w; }
// FUNCTION: LEGORR 0x00401c40
int display_height_get(void) { return s_display_h; }

/* FormattedTextPrintfWidth (FUN_00401c50): format and measure without drawing.
 * The retail wrapper forwards its variadic arguments through
 * FormattedTextFormatDraw, including the engine's `%b` texture tokens. */
// FUNCTION: LEGORR 0x00401c50
uint32_t formatted_text_printf_width(const TextureSurface* font,
                                     const char* text, ...)
{
    if (font == NULL || text == NULL) return 0;
    uint32_t width = 0u;
    va_list args;
    va_start(args, text);
    formatted_text_format_draw(font, &width, NULL, NULL, 0, 0, 0,
                               0, 0, text, args);
    va_end(args);
    return width;
}
/* FormattedTextPrintf (FUN_00401c80): variadic front-end to the format
 * renderer. The original forwards its stack varargs to FormattedTextFormatDraw. */
// FUNCTION: LEGORR 0x00401C80
void formatted_text_printf(const TextureSurface* font,
                           uint32_t* width_out, int* line_count,
                           void* px, int pitch, int w, int h,
                           int x, int y, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    formatted_text_format_draw(font, width_out, line_count, px, pitch, w, h,
                               x, y, format, args);
    va_end(args);
}

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
