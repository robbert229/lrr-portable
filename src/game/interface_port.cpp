// SPDX-License-Identifier: LGPL-3.0-or-later
/* Port tail (ADR 0006 decision 4): the port's own interface model.
 *
 * GAME-020 (the original's Interface.c) is rebuilt in original shape in
 * interface_unit.c (issue #545). The port's interface is a different
 * contract: hook tables, its own state words (shared with frontend.cpp,
 * input.cpp, resource.cpp and simtick.cpp through the frozen per-unit
 * headers) and the native HUD. These are the port's implementations of the
 * unit's functions under the port's names, unannotated, so the native
 * callers and tests keep their behaviour (finding 564). The adapters and
 * stand-ins that interface_unit.c calls through game/interface_callees.h are
 * at the end. */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include "frontend.hpp"
#include "frontend.h"
#include "objective.hpp"
#include "level.hpp"
#include "entity.hpp"
#include "terrain_feedback.hpp"
#include "port_loop_ports.hpp"
#include "effects.hpp"
#include "savegame.hpp"
#include "gods98/audio.hpp"
#include "gods98/engine.hpp"
#include "gods98/simtick.hpp"
#include "gods98/message.hpp"
#include "gods98/flic.hpp"
#include "gods98/media.hpp"
#include "crt/crt.hpp"
#include "gods98/heap.hpp"
#include "gods98/record.hpp"
#include "gods98/texture.hpp"
#include "gods98/surface.hpp"
#include "gods98/sound.hpp"
#include "gods98/block.hpp"
#include "gods98/input.hpp"
#include "util.hpp"
#include "select.hpp"
#include "cmd.hpp"
#include "priority_config_ports.hpp"
#include "tutorial_hud_ports.hpp"
#include "ui_action_dispatch.hpp"
#include "simulation_ui_ports.hpp"
#include "camera_drag.hpp"
#include "screen_pick.hpp"
#include "pointer_cursor.hpp"
#include "ui_list.hpp"
#include "gods98/view.hpp"
#include "gods98/string.hpp"
#include "gods98/resource.hpp"
#include "ai.hpp"
#include "building_scene.hpp"
#include "actor_scene.hpp"
#include "terrain_grid.hpp"
#include "mission.hpp"
#include "spawn.hpp"
#include "cmd_shared.hpp"
#include "combat_ray_ports.hpp"
#include "gods98/scene_pool.hpp"
#include "gods98/scene_resources.hpp"
#include "weapon.hpp"
#include "bezier.h"
#include <stddef.h>
#include <float.h>
#include <stdint.h>
#include "type_config_ports.hpp"
#include "entity_pose_delta.hpp"
#include "entity_motion_ports.hpp"
#include "activity_timeout.hpp"
#include "entity_motion_probe.hpp"
#include "type_motion_scale.hpp"
#include "entity_quad_collision.hpp"
#include "gods98/model.hpp"
#include "upgrades.hpp"
#include "entity_for_lego.hpp"
#include <limits.h>
#include "game.hpp"
#include "gods98/camera.hpp"
#include "scene_render.hpp"
#include "objective_shared.hpp"
#include "render.hpp"
#include "frameloop.hpp"
#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "frontend_for_interface.hpp"
#include "interface_unit.h"
#include "interface_callees.h"
#include "gods98/image_adapters.h"
#include "cmd_for_interface.hpp"
#include "effects_for_interface.hpp"
#include "entity_for_interface.hpp"
#include "objective_for_interface.hpp"
#include "upgrades_for_interface.hpp"
#include "gods98/input_for_interface.hpp"
#include "gods98/message_for_interface.hpp"
#include "gods98/resource_for_interface.hpp"
#include "gods98/simtick_for_interface.hpp"

typedef struct EntityActionRequest {
    uint32_t type;
    uint32_t subtype;
    uint32_t stats_mask;
    uint32_t state_mask;
    uint32_t flags3f0_mask;
    uint32_t mode;
} EntityActionRequest;

typedef struct QueueCellRequest {
    const LevelState* level;
    int x;
    int y;
} QueueCellRequest;

#define SIMTICK_MUTE_RESET 25.0f

/* SelectionRegionParametersSet (FUN_0041e9d0): four integer words and one
 * float. Each word is zero-extended onto the x87 stack (fild of a zeroed
 * qword, an unsigned conversion) and the slots rotate: arg0 -> +08,
 * arg1 -> +0c, arg2 -> +00, arg3 -> +04; the float multiplies the .rdata
 * 0.04 constant in single precision into the +10 slot. */
static const float s_region_scale_004 = 0.04f; /* DAT_0049e298, verified */

static float    s_mute_timer   = SIMTICK_MUTE_RESET; /* _DAT_004a2b58, boot fresh */

uint32_t s_incident_state;

TextureSurface* s_back_button_highlight;
TextureSurface* s_back_button_pressed;
int s_back_button_width;

int s_back_button_height;

/* The corpus keeps the three 74-slot windows as individually-addressed
 * tables (DAT_004dcff8 / DAT_004dd120 / DAT_004dd248, contiguous zero-tail
 * band 0x4dcff8..0x4dd370); Reccmp only resolves whole-object operands, so
 * each window gets its own annotated array (the s_fe_flow_* carve-out
 * precedent). Own storage, zero-init like the corpus's tail. */
uint32_t s_indexed_window_c[INPUT_INDEXED_WINDOW_SLOTS]; /* default window */
uint32_t s_indexed_window_a[INPUT_INDEXED_WINDOW_SLOTS]; /* pressed window */
uint32_t s_indexed_window_b[INPUT_INDEXED_WINDOW_SLOTS]; /* flag8 window */
uint32_t s_input_entry_flags[INPUT_FLAG_TABLE_SLOTS];

uint32_t s_input_action_click_counts[INPUT_FLAG_TABLE_SLOTS];

struct ResourceHandleSetBandSlot s_handle_set_band[24]; /* DAT_004ddc04..DAT_004ddcc4, stride 8 */

/* --- the selection-coordinate store (2026-10-24) ---------------------------
 * Corpus band DAT_004ddcc4/cc8/cfc/dcf4/f8; the other owners of these words
 * are unported seams, so the port models them as own storage (the
 * unowned-global deviation). Each word carries its own GLOBAL annotation:
 * Reccmp only resolves whole-object operands, so accesses at an offset
 * inside a shared array never match. */
uint32_t s_map_store_pair_x;   /* DAT_004ddcc4 */
uint32_t s_map_store_pair_y;   /* DAT_004ddcc8 */
uint32_t s_obj_sel_word;          /* DAT_004ddccc */
float s_obj_track_word;     /* _DAT_004ddcd0 */

/* CursorTileSelectionUpdateByType (FUN_0041cc80) — see frontend.hpp. */
/* The corpus addresses the pair as two individual dwords; carve annotated
 * statics (whole-object resolution, the s_fe_flow_* precedent). Own
 * storage, zero-init like the corpus's tail. */
uint32_t s_cursor_tile_first;  /* DAT_004ddcd8 role */
uint32_t s_cursor_tile_second; /* DAT_004ddcdc role */
/* The corpus's parameter register quintet, DAT_004ddce0..f0 at stride 4.
 * Each float carries its own GLOBAL annotation: Reccmp only resolves
 * whole-object operands, so a shared-array base never matches the corpus's
 * absolute accesses in the FUN_0041a990 and FUN_0041e9d0 readers. */
float s_reg_e0;            /* DAT_004ddce0 */
float s_reg_e4;            /* DAT_004ddce4 */
float s_reg_e8;            /* DAT_004ddce8 */
float s_reg_ec;            /* DAT_004ddcec */
float s_reg_f0;            /* DAT_004ddcf0 */

uint32_t s_map_store_last_x;   /* DAT_004ddcf4 */
uint32_t s_map_store_last_y;   /* DAT_004ddcf8 */
uint32_t s_map_store_kind;     /* DAT_004ddcfc */
uint32_t s_pointer_action_type; /* DAT_004ddd00 */
uint32_t s_pointer_action_kind; /* DAT_004ddd04 */
uint32_t s_pointer_action_entry;/* DAT_004ddd08 */
/* --- the selection-region band (see frontend.hpp) ---------------------------
 * Zero-initialized statics per the unowned-global convention: the corpus's
 * words have no ported owner (its UI/hit-test writers are unported), so
 * the zero load image is the faithful state (deviation). */
uint32_t s_sel_c0c;
uint32_t s_sel_c10;
float    s_sel_c14;
float    s_sel_c18;

uint32_t s_status_banner_word;
float s_status_blink_timer;

uint32_t s_cell_perm[CELL_PERMISSION_ENTRIES];

uint32_t s_interface_button_surfaces[2];
float    s_inter_timer = 0.0f; /* DAT_004de1ec */

/* ---- mute timer (FUN_0041ba10) ------------------------------------------ */
uint32_t s_mute_word    = 0;       /* DAT_004de1f4 */

uint32_t s_inter_flags;    /* DAT_004de1f8 */

static uint32_t s_mute_last    = 0;       /* DAT_004de1fc */

int      s_mute_engaged = 0;       /* DAT_004de200 */

static TextureSurface* s_interface_button_surface_native[2];

namespace unported_ledger_stubs {
int ledger_stub_0041ee80_CellGridPointHitTest(
    uint32_t x, uint32_t y, float region_x, float region_y,
    uint32_t* action_out, uint32_t* cell_x_out, uint32_t* cell_y_out);

int ledger_stub_0043fc20_RecordPairPoseCopyAndActivityRaise(
    EntitySlot* entity);
} // namespace unported_ledger_stubs

/* The action catalog gate is DAT_004de1f8, owned by the frontend. It is
 * distinct from the engine placement-mode word at DAT_00556ed4. */
extern uint32_t interaction_flags_peek(void);

/* UI-owned operations used by IncidentMessageTextConsumer. The text output
 * record layout and implementations live in game/frontend.cpp; forward
 * declarations avoid pulling its broad header into the engine message module. */
extern uint32_t interaction_flags_peek(void);

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static int fe_token_copy(char* out, size_t out_size, const char* token, size_t token_size);
static int fe_token_is(const char* token, size_t length, const char* expected);
static void fe_interface_tooltip_parse(const char* token, size_t length, char* text, size_t text_size, int* sound, int* has_sound);
static int menu_kind_gated(int kind);
static void cancel_action_if_map_coordinate_matches(uint32_t mode, uint32_t required_mode, const uint32_t coordinate[2]);
static int selected_map_coordinate_matches(const uint32_t coordinate[2]);
static int tutorial_path_menu_action_precheck_raw( GameContext* ctx, const EngineContext* engine, TerrainDescriptorNode* descriptors, TerrainCellSidecarPresenceQuery sidecar_query, int (*target_cell_valid)(EntitySlot*), int (*route_marker_candidate)(EntitySlot*, int), int (*vehicle_unload_eligible)(EntitySlot*, int), int (*entity_unload_cell_available)(EntitySlot*, int), int (*selected_action_14_15_route_predicate)(EntitySlot*), int (*building_upgrade_allowed)(EntitySlot*), int (*entity_action_kind_prerequisite)(EntitySlot*, int), int (*building_upgrade_limit_allows)(EntitySlot*), int action, int x, int y);
static void ui_action_dispatch_global_low_byte_clear( UiActionDispatchState* state);
static void ui_action_dispatch_mode_request(UiActionDispatchState* state, int mode, int argument);
static void ui_action_dispatch_selection_clear(UiActionDispatchState* state);
static uint32_t* player_click_count_table(int kind);
namespace unported_ledger_stubs { static int cell_grid_point_hit_live_state_build( CellGridPointHitTestState* live_state, CellGridPointHitRecord* live_records); }
namespace unported_ledger_stubs { static int cell_grid_point_hit_record_size_get(void* context, uint32_t type, uint32_t row, float* width, float* height); }
static int entity_action_request_find(GameContext* ctx, const EntityActionRequest* request);
static int entity_action_request_matches(EntitySlot* entity, void* payload);
static int queue_walkable_cell_matches(EntitySlot* entity, void* payload);
static int queue_placement_cell_matches(EntitySlot* entity, void* payload);

/* InterfaceMenuActionLabelsInit (FUN_0041a300): load the action key table,
 * enable timed-effect entry 4, and install the cursor/selection region. The
 * name table itself uses immutable executable strings in the port. */
void interface_menu_action_labels_init(uint32_t x, uint32_t y,
                                       const void* context)
{
    timed_effect_persistence_set(4, 1);
    s_interface_menu_action_xy[0] = (float)x;
    s_interface_menu_action_xy[1] = (float)y;
    s_interface_menu_action_context = context;
    interaction_action_code_map_initialize();
    selection_region_parameters_set(x, y,
                                    (uint32_t)(display_width_get() + 10),
                                    y, 750.0f);
    objective_selection_state_reset();
}

/* InteractionActionCodeMapInitialize (FUN_0041a660): registers the fixed
 * eighteen interaction-action groups in the corpus's call order — groups
 * 0..5, 7, 8, then 0xa, 0xb, 0xc, with group 9 landing between 0xc and
 * 0xd, and group 6 left unregistered (the corpus skips it). The action
 * codes ride as the corpus's own pushed constants, one push per code. */
void interaction_action_code_map_initialize(void)
{
    resource_input_key_group_register(0, 4, 1, 2, 3, 4);
    resource_input_key_group_register(1, 3, 5, 6, 0xe);
    resource_input_key_group_register(2, 1, 7);
    resource_input_key_group_register(3, 1, 0xe);
    resource_input_key_group_register(4, 1, 0x10);
    resource_input_key_group_register(5, 2, 9, 0xe);
    resource_input_key_group_register(7, 4, 0xb, 0xc, 0xd, 0xf);
    resource_input_key_group_register(8, 2, 0x2b, 0x2f);
    resource_input_key_group_register(0xa, 7, 0x16, 0x14, 0x42, 0x19,
                                      0x2d, 0x2e, 0x29);
    resource_input_key_group_register(0xb, 6, 0x16, 0x14, 0x19, 0x2d, 0x2e,
                                      0x1a);
    resource_input_key_group_register(0xc, 2, 0x18, 0x29);
    resource_input_key_group_register(9, 10, 0x1f, 0x17, 0x15, 0x30, 0x21,
                                      0x40, 0x39, 0x2d, 0x2e, 0x20);
    resource_input_key_group_register(0xd, 4, 0x24, 0x22, 0x41, 0x28);
    resource_input_key_group_register(0xe, 1, 0x2a);
    resource_input_key_group_register(0xf, 3, 0x2c, 0x2e, 0x2d);
    resource_input_key_group_register(0x10, 8, 0x31, 0x32, 0x33, 0x34, 0x35,
                                      0x36, 0x37, 0x38);
    resource_input_key_group_register(0x11, 6, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e,
                                      0x3f);
    resource_input_key_group_register(0x12, 4, 0x43, 0x44, 0x45, 0x46);
}
/* --- the key-group action-code band (see resource.hpp) ---------------------
 * InputKeyGroupRegister (FUN_0041a7d0) stages one interaction-action group:
 * the count word rides in at the pair's +4 slot FIRST (corpus order), then
 * a count*4 heap array takes the code dwords. The codes ride the stack as
 * the corpus's own cdecl varargs and are read out with va_arg in index
 * order. Deviations: the staging copy of the variadic window into locals
 * (and the always-zero byte tail) fold into a direct dword copy, and the
 * group guard answers where the corpus strides the band blindly.
 * Registering the same group twice re-stages in place; the corpus leaks the
 * superseded array, and so does the port. */
void resource_input_key_group_register(int group, int count, ...)
{
    va_list ap;

    if (group < 0 || group >= 24) {
        return; /* deviation: the corpus strides the band blindly */
    }
    s_handle_set_band[group].word2 = (uint32_t)count;
    void* array = engine_heap_alloc((long)count * 4); /* FUN_0048dbd0 */
    s_handle_set_band[group].handle = array;
    va_start(ap, count);
    if (array != NULL) {
        uint32_t* dst = (uint32_t*)array;
        const uint32_t n = (uint32_t)count & 0x3fffffffu;
        for (uint32_t i = 0; i != n; ++i) {
            dst[i] = (uint32_t)va_arg(ap, int);
        }
    }
    va_end(ap);
}
void resource_handle_set_release(void)
{
    for (int i = 0; i != 24; ++i) {
        /* corpus: stride the band, free the non-zero dwords under cc4 */
        if (s_handle_set_band[i].handle != NULL) {
            engine_heap_free(s_handle_set_band[i].handle); /* FUN_0048db80 */
            s_handle_set_band[i].handle = NULL; /* deviation: the corpus dangles */
        }
    }
    /* corpus free order: 734 73c 744 ad4 99c 9a4 738 740 748 ad8 9a0 9a8 */
    static const size_t order[12] = { 0, 2, 4, 10, 6, 8, 1, 3, 5, 11, 7, 9 };
    for (size_t k = 0; k != 12; ++k) {
        void** p = &s_handle_set_named[order[k]];
        if (*p != NULL) {
            engine_heap_free(*p);
            *p = NULL;
        }
    }
}

namespace lego {
void level_state_counters_clear(void)
{
    EventTimerState frontend_state;
    uint32_t i;
    LevelCounterState* state = s_counter_state;
    frontend_event_timer_state_get(&frontend_state);
    frontend_state.flags = 0u;
    frontend_event_timer_state_set(&frontend_state);
    input_action_level_state_clear();
    for (i = 0u; i < 300u; ++i)
        cell_action_permission_table_set((int)i, 0u);
    if (state == 0) return;
    memset(state->fixed_a, 0, sizeof(state->fixed_a));
    memset(state->fixed_b, 0, sizeof(state->fixed_b));
    memset(state->table_a, 0, sizeof(state->table_a));
    memset(state->table_b, 0, sizeof(state->table_b));
    state->state_counter = 0;
    if (state->dynamic_a_count > 256) state->dynamic_a_count = 256;
    if (state->dynamic_b_count > 256) state->dynamic_b_count = 256;
    memset(state->dynamic_a, 0, state->dynamic_a_count * sizeof(uint32_t));
    memset(state->dynamic_b, 0, state->dynamic_a_count * sizeof(uint32_t));
    memset(state->dynamic_c, 0, state->dynamic_b_count * sizeof(uint32_t));
    memset(state->dynamic_d, 0, state->dynamic_b_count * sizeof(uint32_t));
    memset(state->trailing, 0, sizeof(state->trailing));
    objective_selection_state_reset();
}
} // namespace lego

void objective_selection_state_reset(void)
{
    s_obj_sel_word = 0u;
    s_obj_track_word = s_reg_e8;
    s_inter_flags |= 0x84u;
}
/* CaseInsensitiveStringTableIndexFind (FUN_0041a9c0): the index is the
 * position in the 74-key table above, matching the corpus's zero-based
 * pointer walk. */
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) int interface_menu_action_label_find(const char* name,
                                                       int* out_index)
{
    __asm {
        push ebx
        mov ebx, dword ptr [esp + 8]
        push esi
        push edi
        xor edi, edi
        mov esi, OFFSET s_interface_menu_action_labels
    interface_menu_action_label_find_loop:
        mov eax, dword ptr [esi]
        push ebx
        push eax
        call _strcmpi
        add esp, 8
        test eax, eax
        je interface_menu_action_label_find_found
        add esi, 4
        inc edi
        cmp esi, OFFSET s_interface_menu_action_labels + 0x128
        jb interface_menu_action_label_find_loop
        xor eax, eax
        pop edi
        pop esi
        pop ebx
        ret
    interface_menu_action_label_find_found:
        mov ecx, dword ptr [esp + 0x14]
        mov eax, 1
        mov dword ptr [ecx], edi
        pop edi
        pop esi
        pop ebx
        ret
    }
}
#else
int interface_menu_action_label_find(const char* name, int* out_index)
{
    if (name == NULL || out_index == NULL) return 0;
    const size_t count = sizeof(s_interface_menu_action_labels)
                         / sizeof(s_interface_menu_action_labels[0]);
    for (size_t i = 0; i < count; ++i) {
        if (s_strcaseeq(name, s_interface_menu_action_labels[i])) {
            *out_index = (int)i;
            return 1;
        }
    }
    return 0;
}
#endif

/* EntityTableMapsAllocate (FUN_0041aa00): the retail entry has no arguments
 * and sizes its six zeroed maps from the loaded vehicle/building counts.
 * Native pointers remain in this owner; the table payloads are retail-width
 * dword/byte arrays and are never used to carry host pointers. */
void entity_table_maps_allocate(void)
{
    entity_table_maps_current_release();
    entity_table_maps_allocate_for_counts(&s_entity_table_maps,
                                          s_catalog_counts[0],
                                          s_catalog_counts[3]);
    resource_handle_set_slot_install(30, s_entity_table_maps.flags_a);
    resource_handle_set_slot_install(31, s_entity_table_maps.flags_b);
    resource_handle_set_slot_install(32, s_entity_table_maps.secondary_a);
    resource_handle_set_slot_install(33, s_entity_table_maps.secondary_b);
    resource_handle_set_slot_install(34, s_entity_table_maps.primary_a);
    resource_handle_set_slot_install(35, s_entity_table_maps.primary_b);
}

/* InterfaceBuildImagesConfigLoad (FUN_0041ab00): the two groups are keyed
 * by the VehicleTypes and BuildingTypes catalog names, in that order. */
int interface_build_images_config_load(BlockFile* cfg)
{
    if (cfg == NULL) {
        return 0;
    }
    keyboard_key_names_init();
    static const char* const type_sections[2] = {
        "VehicleTypes", "BuildingTypes"
    };
    int loaded = 0;
    for (int group = 0; group < 2; ++group) {
        if (s_interface_build_images[group] != NULL) {
            for (uint32_t i = 0; i < s_interface_build_image_counts[group];
                 ++i) {
                FeInterfaceBuildImage* old = &s_interface_build_images[group][i];
                texture_release(old->normal);
                texture_release(old->disabled);
                texture_release(old->pressed);
            }
            engine_heap_free(s_interface_build_images[group]);
        }
        char type_path[FE_PATH_MAX * 2];
        snprintf(type_path, sizeof(type_path), "%s::%s", util_root_name(cfg),
                 type_sections[group]);
        BlockRec* type_section = block_subgroup(cfg, type_path);
        uint32_t count = 0;
        for (BlockRec* row = type_section; row != NULL;
             row = block_next_sibling(row)) {
            ++count;
        }
        s_interface_build_image_counts[group] = count;
        s_interface_build_images[group] = (count != 0)
            ? (FeInterfaceBuildImage*)engine_heap_alloc(
                  (long)count * (long)sizeof(FeInterfaceBuildImage))
            : NULL;
        FeInterfaceBuildImage* images = s_interface_build_images[group];
        if (images == NULL) {
            s_interface_build_image_counts[group] = 0;
            continue;
        }
        memset(images, 0, (size_t)count * sizeof(*images));
        if (type_section == NULL) {
            continue;
        }
        BlockRec* type_row = type_section;
        for (uint32_t i = 0; i < count; ++i) {
            if (type_row == NULL) {
                break;
            }
            const char* type_name = block_name(type_row);
            type_row = block_next_sibling(type_row);
            if (type_name == NULL) {
                continue;
            }
            char key[FE_PATH_MAX * 2];
            snprintf(key, sizeof(key), "%s::InterfaceBuildImages::%s",
                     util_root_name(cfg), type_name);
            const char* value = block_get_strptr(cfg, key);
            if (value == NULL) {
                continue;
            }
            const char* fields[4];
            size_t lengths[4];
            const char* p = value;
            int valid = 1;
            for (int field = 0; field < 4; ++field) {
                fields[field] = p;
                const char* colon = field < 3 ? strchr(p, ':') : NULL;
                if (field < 3 && colon == NULL) {
                    valid = 0;
                    break;
                }
                lengths[field] = colon != NULL
                    ? (size_t)(colon - p) : strlen(p);
                if (lengths[field] == 0) {
                    valid = 0;
                    break;
                }
                p = colon != NULL ? colon + 1 : p + lengths[field];
            }
            if (!valid || *p != '\0') {
                continue;
            }
            FeInterfaceBuildImage* image = &images[i];
            if (!fe_token_copy(image->shortcut_name,
                               sizeof(image->shortcut_name),
                               fields[3], lengths[3])) {
                continue;
            }
            image->has_shortcut_code =
                named_option_index_find_case_insensitive(
                    image->shortcut_name, &image->shortcut_code);
            TextureSurface** states[3] = {
                &image->normal, &image->disabled, &image->pressed
            };
            for (int state = 0; state < 3; ++state) {
                if (fe_token_is(fields[state], lengths[state], "NULL")) {
                    continue;
                }
                char path[FE_PATH_MAX];
                if (!fe_token_copy(path, sizeof(path), fields[state],
                                   lengths[state])) {
                    valid = 0;
                    break;
                }
                /* FUN_0041ab00 uses BitmapSurfaceLoad for each state. These
                 * catalog records own independent surface instances. */
                *states[state] = image_surface_load(path, 0u, 0u);
            }
            if (!valid) {
                texture_release(image->normal);
                texture_release(image->disabled);
                texture_release(image->pressed);
                memset(image, 0, sizeof(*image));
                continue;
            }
            ++loaded;
        }
    }
    return loaded;
}
/* InterfaceImagesConfigLoad (FUN_0041ada0): load three bitmap states and
 * tooltip/sound/shortcut metadata for each of the 74 named action slots. */
int interface_images_config_load(BlockFile* cfg)
{
    BlockRec* section;
    BlockRec* entry;
    if (cfg == NULL) {
        return 0;
    }
    keyboard_key_names_init();
    char section_path[FE_PATH_MAX * 2];
    snprintf(section_path, sizeof(section_path), "%s::InterfaceImages",
             util_root_name(cfg));
    section = block_subgroup(cfg, section_path);
    entry = section;
    if (entry == NULL) {
        /* These two loaders run after the generic loop, including when
         * the section is absent (FUN_0041ada0 + 0x209). */
        (void)interface_build_images_config_load(cfg);
        (void)interface_back_button_config_load(cfg);
        return 0;
    }
    int loaded = 0;
    for (; entry != NULL; entry = block_next_sibling(entry)) {
        const char* label = block_name(entry);
        const char* value = block_value(entry);
        int image_index;
        if (label == NULL || value == NULL ||
            !interface_menu_action_label_find(label, &image_index)) {
            continue;
        }

        const char* fields[7];
        size_t lengths[7];
        int count = 0;
        const char* p = value;
        int valid = 1;
        while (count < 7) {
            fields[count] = p;
            const char* colon = strchr(p, ':');
            lengths[count] = colon != NULL
                ? (size_t)(colon - p) : strlen(p);
            if (lengths[count] == 0) {
                valid = 0;
                break;
            }
            ++count;
            if (colon == NULL) {
                break;
            }
            if (count == 7) {
                valid = 0;
                break;
            }
            p = colon + 1;
        }
        if (!valid || count < 6) {
            continue;
        }

        FeInterfaceImage* image = &s_interface_images[image_index];
        memset(image, 0, sizeof(*image));
        image->normal_sound = -1;
        image->disabled_sound = -1;
        if (!fe_token_copy(image->shortcut_name,
                           sizeof(image->shortcut_name),
                           fields[5], lengths[5])) {
            continue;
        }
        image->has_shortcut_code = named_option_index_find_case_insensitive(
            image->shortcut_name, &image->shortcut_code);
        TextureSurface** states[3] = {
            &image->normal, &image->disabled, &image->pressed
        };
        for (int state = 0; state < 3; ++state) {
            if (fe_token_is(fields[state], lengths[state], "NULL")) {
                continue;
            }
            char path[FE_PATH_MAX];
            if (!fe_token_copy(path, sizeof(path), fields[state],
                               lengths[state])) {
                valid = 0;
                break;
            }
            /* The corpus calls BitmapSurfaceLoad directly (FUN_0047d9d0),
             * not the port's name-keyed cache. This keeps the call site on
             * the annotated twin; surfaces in this table therefore follow
             * the corpus's independent-load identity rather than cache
             * sharing. */
            *states[state] = image_surface_load(path, 0u, 0u);
        }
        if (!valid) {
            for (int state = 0; state < 3; ++state) {
                texture_release(*states[state]);
            }
            memset(image, 0, sizeof(*image));
            continue;
        }

        int has_sound = 0;
        fe_interface_tooltip_parse(fields[3], lengths[3],
                                   image->normal_tooltip,
                                   sizeof(image->normal_tooltip),
                                   &image->normal_sound,
                                   &has_sound);
        fe_interface_tooltip_parse(fields[4], lengths[4],
                                   image->disabled_tooltip,
                                   sizeof(image->disabled_tooltip),
                                   &image->disabled_sound,
                                   &has_sound);

        if (count == 7) {
            if (fe_token_is(fields[6], lengths[6], "TRUE")
                || fe_token_is(fields[6], lengths[6], "YES")
                || fe_token_is(fields[6], lengths[6], "1")) {
                image->optional_flag = 1;
                image->has_optional_flag = 1;
            }
            else if (fe_token_is(fields[6], lengths[6], "FALSE")
                     || fe_token_is(fields[6], lengths[6], "NO")
                     || fe_token_is(fields[6], lengths[6], "0")) {
                image->optional_flag = 0;
                image->has_optional_flag = 1;
            }
        }
        ++loaded;
    }
    (void)interface_build_images_config_load(cfg);
    (void)interface_back_button_config_load(cfg);
    return loaded;
}

/* InterfaceSurroundImagesConfigLoad (FUN_0041afd0): load the eleven
 * numbered surround records. Each record is
 * `image:x:y:w:h:image-without-back:x:y`; either image may be `NULL`. */
int interface_surround_images_config_load(BlockFile* cfg)
{
    if (cfg == NULL) {
        return 0;
    }
    int loaded = 0;
    const char* root = util_root_name(cfg);
    for (int i = 0; i < FE_SURROUND_IMAGES_MAX; ++i) {
        char key[FE_PATH_MAX * 2];
        snprintf(key, sizeof(key), "%s::InterfaceSurroundImages::%d", root,
                 i + 1);
        const char* value = block_get_strptr(cfg, key);
        FeSurroundImage* record = &s_interface_surround_images[i];
        if (value == NULL) {
            record->image = NULL;
            continue;
        }

        const char* fields[8];
        size_t lengths[8];
        const char* p = value;
        int valid = 1;
        for (int field = 0; field < 8; ++field) {
            fields[field] = p;
            const char* colon = (field < 7) ? strchr(p, ':') : NULL;
            if (field < 7 && colon == NULL) {
                valid = 0;
                break;
            }
            lengths[field] = colon != NULL ? (size_t)(colon - p) : strlen(p);
            if (lengths[field] == 0) {
                valid = 0;
                break;
            }
            p = colon != NULL ? colon + 1 : p + lengths[field];
        }
        if (!valid || *p != '\0' || lengths[0] >= FE_PATH_MAX
            || lengths[5] >= FE_PATH_MAX) {
            continue;
        }

        int numbers[6];
        const int number_fields[6] = { 1, 2, 3, 4, 6, 7 };
        for (int n = 0; n < 6; ++n) {
            const int field = number_fields[n];
            char token[32];
            if (lengths[field] >= sizeof(token)) {
                valid = 0;
                break;
            }
            memcpy(token, fields[field], lengths[field]);
            token[lengths[field]] = '\0';
            /* The corpus uses its annotated atoi twin (FUN_0048db70):
             * numeric prefixes are accepted and the parsed value is not
             * narrowed to the port's earlier signed-16-bit safety range. */
            numbers[n] = atoi(token);
        }
        if (!valid) {
            continue;
        }

        int first_is_null = 0;
        int second_is_null = 0;
        if (lengths[0] == 4) {
            char token[5];
            memcpy(token, fields[0], 4);
            token[4] = '\0';
            first_is_null = strcasecmp_lite(token, "NULL") == 0;
        }
        if (lengths[5] == 4) {
            char token[5];
            memcpy(token, fields[5], 4);
            token[4] = '\0';
            second_is_null = strcasecmp_lite(token, "NULL") == 0;
        }
        if (!first_is_null) {
            char image_path[FE_PATH_MAX];
            memcpy(image_path, fields[0], lengths[0]);
            image_path[lengths[0]] = '\0';
            /* FUN_0041afd0 calls BitmapSurfaceLoad (FUN_0047d9d0), rather
             * than the port's name-keyed cache. */
            record->image = image_surface_load(image_path, 0u, 0u);
            record->x = numbers[0];
            record->y = numbers[1];
            record->width = numbers[2];
            record->height = numbers[3];
        }
        if (!second_is_null) {
            char image_path[FE_PATH_MAX];
            memcpy(image_path, fields[5], lengths[5]);
            image_path[lengths[5]] = '\0';
            record->image_without_back = image_surface_load(image_path,
                                                            0u, 0u);
            record->without_back_x = numbers[4];
            record->without_back_y = numbers[5];
        }
        ++loaded;
    }
    return loaded;
}
/* InterfaceBackButtonConfigLoad (FUN_0041b1b0): the Lego.cfg record is
 * `width:height:highlight-image:pressed-image:action`. The corpus also
 * initializes its legacy UI text entry 5; the port's menu model has no such
 * text-entry record, so retain the action token for the future UI consumer. */
int interface_back_button_config_load(BlockFile* cfg)
{
    if (cfg == NULL) {
        return 0;
    }
    char key[FE_PATH_MAX * 2];
    snprintf(key, sizeof(key), "%s::InterfaceBackButton", util_root_name(cfg));
    const char* value = block_get_strptr(cfg, key);
    if (value == NULL) {
        return 0;
    }

    int width = 0, height = 0;
    const char* first_separator = strchr(value, ':');
    if (first_separator == NULL) {
        return 0;
    }
    width = atoi(value);
    if (width <= 0 || width > 32767) {
        return 0;
    }
    const char* p = first_separator + 1;
    const char* second_separator = strchr(p, ':');
    if (second_separator == NULL) {
        return 0;
    }
    height = atoi(p);
    if (height <= 0 || height > 32767) {
        return 0;
    }
    p = second_separator + 1;

    const char* highlight_end = strchr(p, ':');
    if (highlight_end == NULL || highlight_end == p) {
        return 0;
    }
    const char* pressed = highlight_end + 1;
    const char* pressed_end = strchr(pressed, ':');
    if (pressed_end == NULL || pressed_end == pressed) {
        return 0;
    }
    const char* action = pressed_end + 1;
    if (*action == '\0' || strchr(action, ':') != NULL) {
        return 0;
    }

    char highlight_path[FE_PATH_MAX];
    char pressed_path[FE_PATH_MAX];
    size_t highlight_len = (size_t)(highlight_end - p);
    size_t pressed_len = (size_t)(pressed_end - pressed);
    size_t action_len = strlen(action);
    if (highlight_len >= sizeof(highlight_path)
        || pressed_len >= sizeof(pressed_path)
        || action_len >= sizeof(s_back_button_action)) {
        return 0;
    }
    memcpy(highlight_path, p, highlight_len);
    highlight_path[highlight_len] = '\0';
    memcpy(pressed_path, pressed, pressed_len);
    pressed_path[pressed_len] = '\0';

    s_back_button_width = width;
    s_back_button_height = height;
    /* InterfaceBackButtonConfigLoad calls BitmapSurfaceLoad for both
     * resources. Keep these as independent loads instead of sharing the
     * name-keyed texture cache used by ordinary menu assets. */
    s_back_button_highlight = image_surface_load(highlight_path, 0u, 0u);
    s_back_button_pressed = image_surface_load(pressed_path, 0u, 0u);
    strncpy(s_back_button_action, action, action_len + 1);
    return 1;
}

void interface_button_images_load(const char* left_path,
                                  const char* right_path)
{
    const char* paths[2] = { left_path, right_path };
    for (int i = 0; i != 2; ++i) {
        TextureSurface* surface = (TextureSurface*)
            string_cache_lookup_or_load(paths[i]);
        const uint32_t handle = (uint32_t)(uintptr_t)surface;
        s_interface_button_surface_native[i] = surface;
        s_interface_button_surfaces[i] = handle;
        if (handle != 0u) {
            crt_surface_blit_record(handle, 0.0f, 0.0f); /* transform */
        }
    }
}

void command_context_post_to_hud_port(int kind, const uint32_t coord[2])
{
    /* FUN_0041b2d0 */
    global_state_low_byte_clear_and_refresh(); /* FUN_00435a20 */
    menu_selection_resource_update(kind, coord);
    store_selected_map_coordinate(kind, coord); /* FUN_0041ea50 */
}
void menu_selection_resource_update(int kind, const uint32_t coord[2])
{
    /* FUN_0041b300 */
    if (s_ctx == NULL || s_ctx->level == NULL) {
        return; /* deviation: no current level, no corpus chain */
    }
    void* terrain = s_ctx->level->ptr_terrain;
    if (menu_kind_gated((int)map_store_kind_peek())) {
        const int x = (int)map_store_last_word(0);
        const int y = (int)map_store_last_word(1);
        if (terrain_cell_surface_type_get(terrain, x, y) == 0) {
            s_menu_surface_track = 0u;
        } else {
            terrain_cell_surface_type_set(terrain, x, y,
                                          (int)s_menu_surface_track);
        }
    }
    if (menu_kind_gated(kind) && coord != NULL) {
        const uint32_t prior = (uint32_t)terrain_cell_surface_type_set(
            terrain, (int)coord[0], (int)coord[1], 4);
        if (prior != 4u) {
            s_menu_surface_track = prior;
        }
    }
}

int entity_query_flag_count(int query)
{
    if (query == 0x13) {
        return (int)s_catalog_counts[1];
    }
    if (query == 0x14 || query == 0x15) {
        int count = 0;
        for (uint32_t row = 0; row < s_catalog_counts[0]; ++row) {
            const uint32_t w148 = stats_table_word_at_148(1, (int)row);
            const uint32_t w14c = stats_table_word_at_14c(1, (int)row);
            if (query == 0x14) {
                if (((w148 & 0x1000u) != 0u || (w148 & 0x2000u) != 0u)
                    && (w14c & 8u) == 0u) {
                    count = count + 1;
                }
            } else if ((w148 & 0x800u) != 0u || (w14c & 8u) != 0u) {
                count = count + 1;
            }
        }
        return count;
    }
    if (query < 0 || (unsigned)query >= CATALOG_ID_PAIRS) {
        return 0; /* deviation: the corpus strides blindly */
    }
    /* The pair word is the stride-8 band's count word (the corpus reads
     * one and the same window for both roles); the merged model lives
     * behind the resource band accessor. */
    return (int)resource_handle_set_band_word_peek(query);
}

/* CategoryValueIsNeitherZeroNorFifteen (FUN_0041b470). */
int category_value_is_neither_zero_nor_fifteen(int value)
{
    if (value != 0 && value != 0xf) {
        return 1;
    }
    return 0;
}

/* CatalogCallbackByIdDispatch (FUN_0041b680): dispatch over the shared
 * stride-8 band. Query 0x13 walks the second catalog count, handing the
 * callback the row lane; 0x14 walks the first catalog's rows and calls
 * the callback on the rows whose +148 word raises bit 0x1000 or 0x2000
 * while the +14c bit 8 stays clear (the first callback that answers
 * nonzero wins); 0x15 hands over rows raising bit 0x800 or the +14c bit
 * 8; any other query needs a nonzero pair count and walks the group's
 * code array through the callback — 0x22/0x23 codes ride through the
 * selection-code resolve, which rewrites the array element in place
 * (the corpus's cached pick). Deviations: the query lanes thread the
 * context block (the resolve's state window is not a corpus argument),
 * NULL callback/pair arrays answer zero where the corpus strides
 * blindly, and the guarded reads fold the corpus's blind strides. */
int catalog_callback_by_id_dispatch(const GameContext* ctx, int query,
                                    int (*callback)(int, int, int, void*),
                                    void* payload)
{
    if (callback == NULL) {
        return 0; /* deviation: the corpus calls the pointer blindly */
    }
    if (query == 0x13) {
        for (uint32_t row = 0; row < s_catalog_counts[1]; ++row) {
            if (callback(0x49, 4, (int)row, payload) != 0) {
                return 1;
            }
        }
        return 0;
    }
    if (query == 0x14 || query == 0x15) {
        for (uint32_t row = 0; row < s_catalog_counts[0]; ++row) {
            int answer = 0;
            if (query == 0x14) {
                if ((stats_table_word_at_148(1, (int)row) & 0x1000u) != 0u
                    || (stats_table_word_at_148(1, (int)row) & 0x2000u) != 0u) {
                    if ((stats_table_word_at_14c(1, (int)row) & 8u) == 0u) {
                        answer = callback(0x49, 1, (int)row, payload);
                    }
                }
            } else {
                if ((stats_table_word_at_148(1, (int)row) & 0x800u) != 0u
                    || (stats_table_word_at_14c(1, (int)row) & 8u) != 0u) {
                    answer = callback(0x49, 1, (int)row, payload);
                }
            }
            if (answer != 0) {
                return 1;
            }
        }
        return 0;
    }
    if (resource_handle_set_band_word_peek(query) == 0u) {
        return 0;
    }
    int* codes = (int*)resource_handle_set_slot_peek(query);
    if (codes == NULL) {
        return 0; /* deviation: the corpus strides the pair blindly */
    }
    for (uint32_t i = 0;;) {
        if (codes[i] == 0x22 || codes[i] == 0x23) {
            /* the corpus rewrites the cached element through the resolve */
            selection_entity_code_resolve(ctx, &codes[i]);
        }
        if (callback(codes[i], 0, 0, payload) != 0) {
            return 1;
        }
        /* corpus order: the index increments and the band word re-reads
         * every hop (a callback that re-registers the group retargets the
         * walk mid-sweep), and the walk stops before the first index the
         * count no longer covers */
        if (++i == 0u
                || resource_handle_set_band_word_peek(query) <= i) {
            return 0;
        }
    }
}

void timed_status_and_event_tick(float delta)
{
    frontend_status_flash_tick(delta);
    event_timer_countdown_advance(delta);
    if (s_inter_timer > 0.0f) {
        s_inter_timer -= delta;
        return;
    }
    s_inter_flags |= 0x800u;
}

uint32_t sim_status_banner_word_get_port(void)
{
    return s_status_banner_word;
}

/* SelectionEntityCodeResolve (FUN_0041b9c0) — see frontend.hpp. The corpus
 * reads the first selection first and branches afterwards; the port's
 * order is equivalent since the read is side-effect-free. */
int selection_entity_code_resolve(const GameContext* ctx, int* code_out)
{
    const EntitySlot* first = selection_first(ctx);
    if ((s_inter_flags & 0x80u) == 0u && (s_inter_flags & 0x100u) != 0u) {
        return (code_out != NULL) ? *code_out : 0;
    }
    const int code = ((first != NULL
                       && (first->flags_3e8 & 0x80000000u) != 0u)
                          ? 0x22 : 0x23);
    if (code_out != NULL) {
        *code_out = code;
    }
    return code;
}

void simtick_mute_timer(EngineContext* c, float dt)
{
    (void)c;
    if (s_mute_word != s_mute_last) {
        s_mute_last = s_mute_word;
        s_mute_timer = SIMTICK_MUTE_RESET;
        s_mute_engaged = 0;
        s_mute_word = 0;
        return;
    }
    s_mute_timer -= dt;
    if ((s_mute_timer <= 0.0f) && (s_mute_engaged == 0)) {
        if (s_mute_hook != NULL) {
            s_mute_engaged = s_mute_hook(s_mute_word, 0);
        } else {
            s_mute_engaged = 1;
        }
    }
    s_mute_word = 0;
}

/* SimulationGlobalWordSet (FUN_0041ba90): the faithful blind store — the
 * same word the mute timer reads (simtick_set_mute_word is the port-side
 * hook with the same effect; this is the corpus's own entry point). */
void simulation_global_word_set_port(uint32_t value)
{
    s_mute_word = value;
}

namespace unported_ledger_stubs {
/* SimulationPointerActionDispatch (FUN_0041baa0): keep the retail gate,
 * clear-on-entry interaction bits, cell-action branch, overlay result flags,
 * and pointer-state transition here. Config text, action-word mapping,
 * renderer feedback, and the later action-specific statistics are supplied
 * by the frontend owner because their tables are not yet owned in this
 * module. A terrain ray test covers the available 3D miss path. */
int simulation_pointer_action_dispatch_impl(int x, int y, int active,
                                            int suppress_initial,
                                            int count_action)
{
    SimulationPointerDispatchOps* ops =
        &s_simulation_pointer_dispatch_ops;
    uint32_t action = 0u;
    uint32_t cell_x = 0u;
    uint32_t cell_y = 0u;
    uint32_t global_word;
    int hit;
    const char* label;

    if ((s_inter_flags & 0x84u) != 0x84u) {
        s_inter_flags &= ~0x7600u;
        return 0;
    }
    s_inter_flags &= ~0x7600u;

    hit = ledger_stub_0041ee80_CellGridPointHitTest(
        (uint32_t)x, (uint32_t)y, ops->cell_region_x,
        ops->cell_region_y, &action, &cell_x, &cell_y);
    if (hit != 0) {
        s_inter_flags |= 0x1000u;
        label = (ops->action_label_get != NULL) ?
            ops->action_label_get(ops->context, (int)action,
                                  (int)cell_x, (int)cell_y) : NULL;
        if (active == 0 && count_action == 0 && action != 0x14u &&
            ops->feedback != NULL) {
            ops->feedback(ops->context, 4, (int)action, (int)cell_x,
                          (int)cell_y, label, 1u);
        }
        if ((ops->feature_word & 0x100000u) == 0u &&
            ops->global_word_resolve != NULL &&
            ops->global_word_resolve(ops->context, (int)action,
                                     (int)cell_x, (int)cell_y,
                                     &global_word)) {
            /* DAT_004de1f4 is updated by SimulationGlobalWordSet. */
            simtick_set_mute_word(global_word);
        }
        (void)pointer_action_state_dispatch(0, 0, 0, active,
                                            suppress_initial);
        return 1;
    }

    hit = advisor_overlay_hit_resolve(
        (float)x, (float)y,
        ops->focused_rect.enabled != 0 ? &ops->focused_rect : NULL,
        ops->overlay_offset_x, ops->overlay_offset_y,
        ops->overlay_fallback, ops->context, &action, &cell_x, &cell_y);
    if (hit == 0) {
        if (ops->terrain_grid != NULL && ops->ray_hooks != NULL) {
            int terrain_x;
            int terrain_y;
            float hit_xyz[3];
            if (screen_ray_terrain_cell_pick(
                    ops->terrain_grid, ops->ray_hooks, (float)x, (float)y,
                    0u, &terrain_x, &terrain_y, hit_xyz)) {
                if (count_action != 0 && ops->world_hit_apply != NULL) {

                    ops->world_hit_apply(ops->context, terrain_x,
                                         terrain_y, hit_xyz, count_action);
                }
                return 1;
            }
        }
        return 0;
    }

    if (action == 0u) {
        s_inter_flags |= 0x4000u;
        if (active == 0 && count_action == 0 && ops->feedback != NULL)
            ops->feedback(ops->context, 5, 0, (int)cell_x, (int)cell_y,
                          NULL, 1u);
    }
    else {
        if (action == 0x49u) s_inter_flags |= 0x2600u;
        else if (action == 5u || action == 0xeu)
            s_inter_flags |= 0x2200u;
        else s_inter_flags |= 0x0200u;

        label = (ops->action_label_get != NULL) ?
            ops->action_label_get(ops->context, (int)action,
                                  (int)cell_x, (int)cell_y) : NULL;
        if (active == 0 && count_action == 0 && ops->feedback != NULL)
            ops->feedback(ops->context, 4, (int)action, (int)cell_x,
                          (int)cell_y, label, 1u);
        if ((ops->feature_word & 0x100000u) == 0u &&
            ops->global_word_resolve != NULL &&
            ops->global_word_resolve(ops->context, (int)action,
                                     (int)cell_x, (int)cell_y,
                                     &global_word)) {
            simtick_set_mute_word(global_word);
        }
    }

    hit = pointer_action_state_dispatch((int)action, (int)cell_x,
                                        (int)cell_y, active,
                                        suppress_initial);
    if (hit != 0 && count_action != 0) {
        if (ops->action_count != NULL)
            ops->action_count(ops->context, (int)action, (int)cell_y);
        if (ops->action_complete != NULL)
            ops->action_complete(ops->context, (int)action, (int)cell_x,
                                 (int)cell_y);
    }
    return 1;
}
} // namespace unported_ledger_stubs

/* AdvisorOverlayHitResolve (FUN_0041c1c0): first hit-test the active overlay
 * rectangle; if it misses, defer to the configurable control-tree search. */
int advisor_overlay_hit_resolve(
    float x, float y, const AdvisorOverlayRect* focused_rect,
    float offset_x, float offset_y, AdvisorOverlayFallbackHitFn fallback,
    void* context, uint32_t* result0, uint32_t* result1,
    uint32_t* result2)
{
    uint32_t hit0 = 0u;
    uint32_t hit1 = 0u;
    uint32_t hit2 = 0u;
    if (focused_rect != NULL && focused_rect->enabled != 0 &&
        x >= focused_rect->x + offset_x &&
        x <= focused_rect->x + focused_rect->width + offset_x &&
        y >= focused_rect->y + offset_y &&
        y <= focused_rect->y + focused_rect->height + offset_y) {
        if (result0 != NULL) *result0 = 0u;
        return 1;
    }
    if (fallback == NULL ||
        fallback(context, x, y, offset_x, offset_y,
                 &hit0, &hit1, &hit2) == 0) {
        return 0;
    }
    if (result0 != NULL) *result0 = hit0;
    if (result1 != NULL) *result1 = hit1;
    if (result2 != NULL) *result2 = hit2;
    return 1;
}

/* AdvisorCallbackScheduleGate @ 0x0041c440 is a no-argument global-state
 * entry. The catalog walk is available; LAB_0041c470 remains an explicit
 * callback binding because its body is unresolved. */
void advisor_callback_schedule_gate_port(void)
{
    if ((s_inter_flags & 0x04u) == 0u || (s_inter_flags & 0x80u) == 0u ||
        !input_keyboard_key_held(0x3c) ||
        s_advisor_callback_callback == NULL) {
        return;
    }
    (void)catalog_callback_by_id_dispatch(save_current_context_get(),
        (int)s_obj_sel_word, s_advisor_callback_callback, NULL);
}

/* AdvisorAnimationPositionAndRequestUpdate (FUN_0041c4f0). The live mode,
 * interaction word, scroll/lower offsets, and back-button extents come from
 * their existing owners. The point table and row/trigger tuple are the four
 * unowned DAT inputs represented by the explicit bounded state above. */
void advisor_animation_position_and_request_update_internal(
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
    float old_x, old_y;

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
    advisor_action_position_get(state->action_row, &old_x, &old_y);
    old_x += new_x;
    old_y += new_y;
    advisor_action_position_and_mode_set(state->action_row, 0x0c,
                                          old_x, old_y);
    (void)advisor_request_admit(state->action_row,
                                (frontend_state.flags & 0x40u) != 0u);
    frontend_state.flags &= ~0x20u;
    frontend_event_timer_state_set(&frontend_state);
}

/* PointerActionStateDispatch (FUN_0041c6e0): update the current pointer
 * action latch and its input flags. The interaction lock at +0x1f8 bit
 * 0x1000 suppresses state changes while preserving the prior-state result. */
int pointer_action_state_dispatch(int action, int kind, int entry,
                                  int active, int suppress_initial)
{
    const uint32_t previous_action = s_pointer_action_type;
    if (active != 0) {
        if ((s_inter_flags & 0x1000u) == 0u) {
            if (s_pointer_action_type == 0x4au) {
                if (suppress_initial != 0) {
                    goto finish;
                }
                s_pointer_action_type = (uint32_t)action;
                s_pointer_action_kind = (uint32_t)kind;
                s_pointer_action_entry = (uint32_t)entry;
                if (action == 0x49) {
                    player_input_flag8_set_clear(kind, entry, 1);
                    goto finish;
                }
            }
            else {
                if (s_pointer_action_type != (uint32_t)action) {
                    goto finish;
                }
                if (action == 0x49) {
                    if (s_pointer_action_kind == (uint32_t)kind
                        && s_pointer_action_entry == (uint32_t)entry) {
                        player_input_flag8_set_clear(kind, entry, 1);
                    }
                    goto finish;
                }
            }
            input_entry_flag8_set_clear(action, 1);
        }
    } else {
        s_pointer_action_type = 0x4au;
    }

finish:
    if (previous_action != (uint32_t)action) {
        return 0;
    }
    /* FUN_0041c6e0 has distinct true exits for the non-0x49 and pair arms. */
    if (action == 0x49) {
        if (s_pointer_action_kind == (uint32_t)kind
            && s_pointer_action_entry == (uint32_t)entry) {
            return 1;
        }
    }
    else {
        return 1;
    }
    return 0;
}

/* InputActionStateUpdate: while mode bit 7 is set and the action entry lacks
 * bit 1, ask the action helper. The helper may mutate the same word, so the
 * word is deliberately re-read before clearing bit 0. */
void input_action_state_update(int action)
{
    uint32_t flags = input_action_state_word_get(action);
    if ((flags & 2u) == 0u &&
        (interaction_flags_peek() & 0x80u) != 0u) {
        int handled = 0;
        if (s_input_action_state_query != NULL) {
            handled = s_input_action_state_query(
                s_input_action_state_query_context, action);
        }
        flags = input_action_state_word_get(action);
        if (handled != 0) {
            input_action_state_word_set(action, flags & ~1u);
            return;
        }
    }
    input_action_state_word_set(action, flags | 1u);
}

void catalog_slot_flag_update(int category, int slot)
{
    uint32_t flags;
    int should_clear = 0;
    if (catalog_slot_table_index(category) < 0 || slot < 0 ||
        slot >= CATALOG_SLOT_CAPACITY) return;
    flags = input_binding_state_word_get(category, slot);
    if ((flags & 2u) == 0 &&
        (interaction_flags_peek() & 0x80u) != 0u) {
        if (s_catalog_slot_eligible != NULL
            && s_catalog_slot_available != NULL
            && s_catalog_slot_eligible(category, slot,
                                        s_catalog_slot_context) != 0
            && s_catalog_slot_available(category, slot,
                                        s_catalog_slot_context) != 0) {
            if (category == 4) {
                should_clear = 1;
            }
            else {
                /* The retail path calls FUN_00469af0, then the empty
                 * FUN_0041f8e0. Its EAX is therefore still the rank, so the
                 * caller's `rank <= result` test is rank <= rank. Preserve
                 * the rank query's side effects when a live owner supplies
                 * one, but don't replace that no-op with an invented limit. */
                if (s_catalog_slot_rank != NULL)
                    (void)s_catalog_slot_rank(category, slot,
                                              s_catalog_slot_context);
                should_clear = 1;
            }
        }
        if (should_clear) flags &= ~1u;
        else flags |= 1u;
    }
    else {
        flags |= 1u;
    }
    input_binding_state_word_set(category, slot, flags);
}

void input_entry_flag8_set_clear(int entry, int set)
{
    if (set != 0) {
        s_input_entry_flags[entry] = s_input_entry_flags[entry] | 8u;
    } else {
        s_input_entry_flags[entry] = s_input_entry_flags[entry] & ~8u;
    }
}

void player_input_flag8_set_clear(int kind, int action, int set)
{
    /* MEASURED: the inlined kind 1/kind 4 ladder scored 24.0% vs the
     * player_flags_table helper CALL at 45.0% — MSVC5 re-canonicalizes
     * the inline form (b27 helper-route class). Keep the helper. */
    uint32_t* table = player_flags_table(kind);
    if (table == NULL || action < 0 || action >= INPUT_FLAG_TABLE_SLOTS) {
        return; /* deviation: the corpus reads any kind through the table */
    }
    if (set != 0) {
        table[action] = table[action] | 8u;
    } else {
        table[action] = table[action] & ~8u;
    }
}
uint32_t input_entry_flag8_get(int entry)
{
    return s_input_entry_flags[entry] & 8u;
}

uint32_t player_input_flag8_get(int kind, int action)
{
    const uint32_t* table = player_flags_table(kind);
    if (table == NULL || action < 0 || action >= INPUT_FLAG_TABLE_SLOTS) {
        return 0u;
    }
    return table[action] & 8u;
}

uint32_t indexed_entity_value_get(const EngineContext* ctx, int index)
{
    if (index < 0 || index >= INPUT_INDEXED_WINDOW_SLOTS) {
        return 0u; /* deviation: the corpus strides its window blindly */
    }
    if (input_action_pressed_get(ctx, index, 0) != 0) {
        return s_indexed_window_a[index]; /* DAT_004dd120 window */
    }
    if (input_entry_flag8_get(index) != 0u) {
        return s_indexed_window_b[index]; /* DAT_004dd248 window */
    }
    return s_indexed_window_c[index];     /* DAT_004dcff8 window */
}

/* InputBindingValueGet (FUN_0041caf0): resolve a catalog image from the
 * input binding's pressed state, then its flag-8 state, else the normal
 * image. Corpus kind values 1 and 4 select the two image catalogs. */
TextureSurface* interface_build_image_binding_value_get(
    const EngineContext* engine, int kind, int index)
{
    const int group = (kind == 1) ? 0 : (kind == 4) ? 1 : -1;
    const FeInterfaceBuildImage* const image =
        interface_build_image_peek(group, index);
    if (image == NULL) {
        return NULL;
    }
    if (input_binding_pressed_get(engine, kind, index, 0) != 0) {
        return image->disabled;
    }
    if (player_input_flag8_get(kind, index) != 0u) {
        return image->pressed;
    }
    return image->normal;
}

/* InputBindingDefaultValueGet (FUN_0041cb90): return the catalog's default
 * image field for kinds 1 and 4. Kinds 2, 11, and 15 read zero-initialized
 * scalar globals in the corpus image; no writer is present in the ledger. */
void* input_binding_default_value_get_port(int kind, int index, int mode)
{
    const int group = (kind == 1) ? 0 : (kind == 4) ? 1 : -1;
    if (group >= 0) {
        const FeInterfaceBuildImage* const image =
            interface_build_image_peek(group, index);
        if (image == NULL) {
            return 0u;
        }
        const TextureSurface* const value = (mode == 0)
            ? image->disabled : image->normal;
        return (void*)value;
    }
    if ((kind == 2 || kind == 11 || kind == 15)) {
        return 0u;
    }
    return 0u;
}

void cursor_tile_selection_update_by_type(int kind, uint32_t first,
                                          uint32_t second)
{
    if (kind == 0x49) {
        s_cursor_tile_first = first;
        s_cursor_tile_second = second;
        return;
    }
    if (kind == 5) {
        s_cursor_tile_first = 0xFu;
        s_cursor_tile_second = 0u;
        return;
    }
    if (kind == 0xe) {
        s_cursor_tile_first = 0xBu;
        s_cursor_tile_second = 0u;
    }
    /* the corpus's else branch rides */
}
void selection_coordinates_set(const uint32_t* pair, uint32_t x, uint32_t y)
{
    if (pair == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    s_sel_c0c = pair[0];
    s_sel_c10 = pair[1];
    s_sel_c14 = (float)x; /* the corpus's int->float cast */
    s_sel_c18 = (float)y;
}
/* PositionTrackerBoxDraw (FUN_0041cd30): draw the two nested outline quads
 * in green unless the interface's transition flag is active. Each closed
 * four-point contour is one Draw_LineListEx call (draw.c) whose to-list is
 * the from-list shifted by one point. */
void position_tracker_box_draw_port(const float bounds[4])
{
    PositionTrackerBoxDrawOps* ops = &s_position_tracker_box_draw_ops;
    uint32_t flags;
    float points[10];
    int inset;
    if (bounds == NULL) return;
    flags = ops->interface_flags_get != NULL ?
        ops->interface_flags_get(ops->context) : 0u;
    if ((flags & 0x100000u) != 0u) return;
    for (inset = 1; inset <= 2; ++inset) {
        const float left = bounds[0] - (float)inset;
        const float top = bounds[1] - (float)inset;
        const float right = bounds[0] + bounds[2] + (float)inset;
        const float bottom = bounds[1] + bounds[3] + (float)inset;
        points[0] = left;
        points[1] = top;
        points[2] = right;
        points[3] = top;
        points[4] = right;
        points[5] = bottom;
        points[6] = left;
        points[7] = bottom;
        points[8] = left;
        points[9] = top;
        surface_draw_polyline((struct Point2F*)points,
                              (struct Point2F*)(points + 2), 4, 0.0f, 1.0f,
                              0.0f, 0);
    }
}

/* SelectionObjectCommandUpdate (FUN_0041cea0): in mode 12, find the
 * argument in the current selection. A singleton selection updates the HUD
 * action to 10 or 11 from the entity's action flags; a multi-selection
 * clears the pending command context. */
void selection_object_command_update(GameContext* ctx, EntitySlot* entity)
{
    if (objective_selection_word_peek() != 12u || ctx == NULL
        || entity == NULL || ctx->selection_count <= 0) {
        return;
    }
    int found = 0;
    for (int i = 0; i < ctx->selection_count; ++i) {
        if (ctx->selection[i] == entity) {
            found = 1;
            break;
        }
    }
    if (found == 0) {
        return;
    }
    if (ctx->selection_count != 1) {
        selection_clear_request_and_hud_reset();
        return;
    }
    const uint32_t capabilities = entity_capability_flags_get(entity);
    const uint32_t action_flags = entity->flags_3ec;
    const int action = ((capabilities & 0x02000000u) != 0u
                        || (action_flags & 0x40u) != 0u) ? 10 : 11;
    command_context_post_to_hud(action, NULL);
}

/* SelectionTargetCommandUpdate (FUN_0041cf20): in modes 10, 18, and 11,
 * read the selection through the count/array helpers (a NULL context
 * reads as empty) and match the passed entity's owner against every
 * entry. A matching singleton posts HUD action 12; a matching
 * multi-selection clears it. Deviation: a NULL entity faults at the
 * corpus's blind [entity+0x36c] read. */
void selection_target_command_update(GameContext* ctx, EntitySlot* entity)
{
    const uint32_t mode = s_obj_sel_word;
    if (mode == 10u || mode == 18u || mode == 11u) {
        const int count = selection_count(ctx);
        EntitySlot* const* records = selection_record_array(ctx);
        if (count > 0) {
            EntitySlot* const* p = records;
            int n = count;
            do {
                /* the corpus re-reads [entity+0x36c] on every pass */
                if (entity->owner_36c == (const void*)*p) {
                    if (count == 1) {
                        command_context_post_to_hud(12, NULL);
                    }
                    else {
                        selection_clear_request_and_hud_reset();
                    }
                }
                ++p;
                --n;
            } while (n != 0);
        }
    }
}
/* SelectionMode7CancelAtPoint (FUN_0041cf80): cancel on a matching map
 * coordinate while objective selection mode 7 is active. */
void selection_mode7_cancel_at_point_port(const uint32_t coordinate[2])
{
    cancel_action_if_map_coordinate_matches(
        objective_selection_word_peek(), 7u, coordinate);
}

/* CancelActionForMatchingCoordinates (FUN_0041cfb0): modes 1 and 4 cancel
 * when the clicked coordinate matches the stored map target. */
void cancel_action_for_matching_coordinates_port(const uint32_t coordinate[2])
{
    const uint32_t mode = objective_selection_word_peek();
    if ((mode == 1u || mode == 4u)
        && selected_map_coordinate_matches(coordinate)) {
        selection_clear_request_and_hud_reset();
    }
}

/* SelectionMode5CancelAtPoint (FUN_0041cfe0): the same coordinate match
 * cancels only while objective selection mode 5 is active. */
void selection_mode5_cancel_at_point_port(const uint32_t coordinate[2])
{
    if (objective_selection_word_peek() == 5u
        && selected_map_coordinate_matches(coordinate)) {
        selection_clear_request_and_hud_reset();
    }
}

/* SelectionCancelIfCursorHitsObject (FUN_0041d010): only active in game
 * context, and only when the entity is selected or has the cursor-hit flag.
 * The corpus then queues command 3 and clears the HUD action context. */
void selection_cancel_if_cursor_hits_object(EngineContext* engine,
                                             GameContext* ctx,
                                             EntitySlot* entity)
{
    if (engine == NULL || engine->context_mode == 0 || entity == NULL) {
        return;
    }
    if (selection_contains_entity_target(ctx, entity, NULL) != 0) {
        selection_clear_request_and_hud_reset();
    }
}
int tutorial_path_menu_action(
    GameContext* ctx, const EngineContext* engine,
    TerrainDescriptorNode* descriptors,
    TerrainCellSidecarPresenceQuery sidecar_query,
    int (*target_cell_valid)(EntitySlot*),
    int (*route_marker_candidate)(EntitySlot*, int),
    int (*vehicle_unload_eligible)(EntitySlot*, int),
    int (*entity_unload_cell_available)(EntitySlot*, int),
    int (*selected_action_14_15_route_predicate)(EntitySlot*),
    int (*building_upgrade_allowed)(EntitySlot*),
    int (*entity_action_kind_prerequisite)(EntitySlot*, int),
    int (*building_upgrade_limit_allows)(EntitySlot*),
    int action, int x, int y)
{
    const int action_result = tutorial_path_menu_action_precheck_raw(
        ctx, engine, descriptors, sidecar_query, target_cell_valid,
        route_marker_candidate, vehicle_unload_eligible,
        entity_unload_cell_available, selected_action_14_15_route_predicate,
        building_upgrade_allowed, entity_action_kind_prerequisite,
        building_upgrade_limit_allows, action, x, y);
    if (action_result <= 0) {
        return action_result;
    }

    const uint32_t global_flags = global_state_low_word_get();
    if ((global_flags & 0x6u) != 0u) {
        return action == 0 || (action >= 0x1b && action <= 0x1c);
    }
    if ((global_flags & 0x18u) != 0u) {
        return action == 0 || (action >= 0x14 && action <= 0x15);
    }
    if ((global_flags & 0x60u) != 0u) {
        return action == 0 || (action >= 0x1d && action <= 0x1e);
    }
    return action_result;
}

int ui_action_dispatch(int action)
{
    UiActionDispatchState* state = s_ui_action_dispatch_state;
    const UiActionDispatchOps* ops;
    void* entity = NULL;
    uint32_t flags;
    int type;
    if (state == NULL) return 0;
    ops = state->ops;
    if (ops != NULL && ops->current_selection_get != NULL)
        entity = ops->current_selection_get(ops->context);
    if (ops != NULL && ops->action_already_held != NULL &&
        ops->action_already_held(ops->context, action, 1)) return 0;

    switch (action) {
    case 0:
        if (state->current_mode > 0xf && state->current_mode < 0x12) {
            ui_action_dispatch_mode_request(state, 9, 0);
            return 1;
        }
        if (state->current_mode == 0x12) {
            flags = ops != NULL && ops->entity_flags_get != NULL
                ? ops->entity_flags_get(ops->context, entity, 0x3e8) : 0u;
            if ((flags & 0x02000000u) == 0u &&
                (ops == NULL || ops->entity_byte_flags_get == NULL ||
                 (ops->entity_byte_flags_get(ops->context, entity, 0x3ec)
                  & 0x40u) == 0u))
                ui_action_dispatch_mode_request(state, 0xb, 0);
            else
                ui_action_dispatch_mode_request(state, 10, 0);
            return 1;
        }
        break;
    case 1:
        if (ops != NULL && ops->cursor_action_pair_request != NULL)
            ops->cursor_action_pair_request(ops->context, 2, 0);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 2: ui_action_dispatch_mode_request(state, 0x13, 0); return 1;
    case 3: ui_action_dispatch_mode_request(state, 0x14, 0); return 1;
    case 4: ui_action_dispatch_mode_request(state, 0x15, 0); return 1;
    case 5:
    case 7:
        if (ops != NULL && ops->tutorial_path_descriptor_create != NULL)
            ops->tutorial_path_descriptor_create(
                ops->context, state->selected_cell_x, state->selected_cell_y);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 6:
        {
            int cell_xy[2];
            cell_xy[0] = state->selected_cell_x;
            cell_xy[1] = state->selected_cell_y;
            /* FUN_00430320 runs before the UI's explicit 0x20000000 clear
             * and CellInitPass. It also owns the work gates and queued
             * coordinate events, so preserve the retail call order here. */
            terrain_cell_work_flag_clear_current(cell_xy, 0x3fu);
        }
        if (ops != NULL && ops->selected_cell_reveal_mask_clear != NULL)
            ops->selected_cell_reveal_mask_clear(
                ops->context, state->selected_cell_x, state->selected_cell_y,
                0x20000000u);
        if (ops != NULL && ops->terrain_cell_update != NULL)
            ops->terrain_cell_update(ops->context, state->selected_cell_x,
                                     state->selected_cell_y);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 9:
        if (ops != NULL && ops->selected_map_action_enqueue != NULL)
            ops->selected_map_action_enqueue(ops->context, 0xc, 0,
                state->selected_cell_x, state->selected_cell_y);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0xb:
        if (ops != NULL && ops->terrain_cell_command_02510 != NULL)
            ops->terrain_cell_command_02510(ops->context,
                state->selected_cell_x, state->selected_cell_y);
        goto cell_action_finalize;
    case 0xc:
        if (ops != NULL && ops->terrain_cell_command_03030 != NULL)
            ops->terrain_cell_command_03030(ops->context,
                state->selected_cell_x, state->selected_cell_y);
        goto cell_action_finalize;
    case 0xd:
        if (ops != NULL && ops->terrain_cell_command_02d50 != NULL)
            ops->terrain_cell_command_02d50(ops->context,
                state->selected_cell_x, state->selected_cell_y);
cell_action_finalize:
        if (ops != NULL && ops->terrain_cell_action_mode_refresh != NULL)
            ops->terrain_cell_action_mode_refresh(
                ops->context, state->selected_cell_x, state->selected_cell_y);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0xe:
        if (ops != NULL && ops->terrain_cell_command_02fb0 != NULL)
            ops->terrain_cell_command_02fb0(ops->context,
                state->selected_cell_x, state->selected_cell_y);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0xf:
        if (ops != NULL && ops->terrain_cell_command_02ae0 != NULL)
            ops->terrain_cell_command_02ae0(ops->context,
                state->selected_cell_x, state->selected_cell_y);
        if (ops != NULL && ops->terrain_cell_action_mode_refresh != NULL)
            ops->terrain_cell_action_mode_refresh(
                ops->context, state->selected_cell_x, state->selected_cell_y);
        if (ops != NULL && ops->terrain_cell_command_complete != NULL)
            ops->terrain_cell_command_complete(ops->context,
                state->selected_cell_x, state->selected_cell_y);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x10:
        flags = ops != NULL && ops->selected_cell_flags_get != NULL
            ? ops->selected_cell_flags_get(ops->context,
                state->selected_cell_x, state->selected_cell_y) : 0u;
        if ((flags & 0x40000000u) == 0u) {
            if (ops != NULL &&
                ops->terrain_descriptor_tiles_reset_and_free != NULL)
                ops->terrain_descriptor_tiles_reset_and_free(ops->context,
                    state->selected_cell_x, state->selected_cell_y);
        }
        else {
            /* ClearSelectionGroup keys the operation by the selected cell. */
            if (ops != NULL && ops->selection_group_clear != NULL)
                ops->selection_group_clear(ops->context,
                    state->selected_cell_x, state->selected_cell_y);
        }
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x11:
        if (ops != NULL && ops->input_request_enqueue != NULL) {
            ops->input_request_enqueue(ops->context, 3, NULL, 0, 0);
            ops->input_request_enqueue(ops->context, 1, entity, 0, 0);
        }
        ui_action_dispatch_mode_request(state, 9, 0);
        return 1;
    case 0x12:
        if (ops != NULL && ops->input_request_enqueue != NULL) {
            ops->input_request_enqueue(ops->context, 3, NULL, 0, 0);
            ops->input_request_enqueue(ops->context, 1, entity, 0, 0);
        }
        flags = ops != NULL && ops->entity_flags_get != NULL
            ? ops->entity_flags_get(ops->context, entity, 0x3e8) : 0u;
        if ((flags & 0x02000000u) == 0u &&
            (ops == NULL || ops->entity_byte_flags_get == NULL ||
             (ops->entity_byte_flags_get(ops->context, entity, 0x3ec)
              & 0x40u) == 0u))
            ui_action_dispatch_mode_request(state, 0xb, 0);
        else
            ui_action_dispatch_mode_request(state, 10, 0);
        return 1;
    case 0x13:
        if (ops != NULL && ops->input_request_enqueue != NULL) {
            ops->input_request_enqueue(ops->context, 3, NULL, 0, 0);
            ops->input_request_enqueue(ops->context, 1, entity, 0, 0);
        }
        ui_action_dispatch_mode_request(state, 0xd, 0);
        return 1;
    case 0x14:
        flags = entity != NULL
            ? entity_capability_flags_get((const EntitySlot*)entity) : 0u;
        if ((flags & 1u) != 0u) {
            if (ops != NULL && ops->selected_entity_action_02ec0 != NULL)
                ops->selected_entity_action_02ec0(ops->context, entity);
            return 1;
        }
        if ((state->action_flags & 0x10u) != 0u) {
            ui_action_dispatch_global_low_byte_clear(state);
            return 1;
        }
        goto action_flag_set;
    case 0x15:
        if ((state->action_flags & 8u) != 0u) {
            ui_action_dispatch_global_low_byte_clear(state);
            return 1;
        }
        goto action_flag_set;
    case 0x16:
    case 0x17:
        if (ops != NULL && ops->selected_entities_detach_or_pose != NULL)
            ops->selected_entities_detach_or_pose(ops->context);
        return 1;
    case 0x18:
        if (ops != NULL && ops->ui_action_helper_00452be0 != NULL)
            ops->ui_action_helper_00452be0(ops->context);
        if (ops != NULL && ops->entity_kind_command_issue != NULL)
            ops->entity_kind_command_issue(ops->context, entity);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x19:
    case 0x1a:
        if (ops != NULL && ops->terrain_cell_to_world_position != NULL)
            ops->terrain_cell_to_world_position(ops->context, entity,
                                               action == 0x19 ? 0 : 1);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x1b:
        if ((state->action_flags & 2u) != 0u) {
            ui_action_dispatch_global_low_byte_clear(state);
            return 1;
        }
        goto action_flag_set;
    case 0x1c:
        if ((state->action_flags & 4u) != 0u) {
            ui_action_dispatch_global_low_byte_clear(state);
            return 1;
        }
        goto action_flag_set;
    case 0x1d:
        flags = 0x40u;
        goto action_flag_toggle;
    case 0x1e:
        flags = 0x20u;
        goto action_flag_toggle;
    case 0x1f:
        if (ops != NULL && ops->selected_entity_action_1f != NULL)
            ops->selected_entity_action_1f(ops->context);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x20:
    case 0x28:
    case 0x29:
    case 0x2a:
        if (ops != NULL && ops->selected_entity_action_20_group != NULL)
            ops->selected_entity_action_20_group(ops->context);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x21:
        if (ops != NULL && ops->selected_entity_action_21 != NULL)
            ops->selected_entity_action_21(ops->context);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x22:
        if (ops != NULL && ops->entity_flag_80000000_set_clear != NULL)
            ops->entity_flag_80000000_set_clear(ops->context, entity, 1);
        break;
    case 0x23:
        if (ops != NULL && ops->entity_flag_80000000_set_clear != NULL)
            ops->entity_flag_80000000_set_clear(ops->context, entity, 0);
        break;
    case 0x24:
        if (ops != NULL && ops->selected_entity_action_24 != NULL)
            ops->selected_entity_action_24(ops->context);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x25:
        if (ops != NULL && ops->selected_entity_predicate != NULL) {
            if (ops->selected_entity_predicate(ops->context, entity, 0x39cf0)) {
                state->selected_entity_slots[0] = entity;
                if (ops->selected_entity_slot_set != NULL)
                    ops->selected_entity_slot_set(ops->context, 0, entity);
                ui_action_dispatch_selection_clear(state);
                return 1;
            }
            if (ops->selected_entity_predicate(ops->context, entity, 0x39d10)) {
                state->selected_entity_slots[1] = entity;
                if (ops->selected_entity_slot_set != NULL)
                    ops->selected_entity_slot_set(ops->context, 1, entity);
                ui_action_dispatch_selection_clear(state);
                return 1;
            }
            if (ops->selected_entity_predicate(ops->context, entity, 0x39d30)) {
                state->selected_entity_slots[2] = entity;
                if (ops->selected_entity_slot_set != NULL)
                    ops->selected_entity_slot_set(ops->context, 2, entity);
                ui_action_dispatch_selection_clear(state);
                return 1;
            }
        }
        break;
    case 0x26:
    case 0x27:
        if (ops != NULL && ops->entity_action_kind_create != NULL)
            ops->entity_action_kind_create(ops->context, entity,
                                           action == 0x26 ? 6 : 7);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x2c:
        if (ops != NULL && ops->input_request_enqueue != NULL)
            ops->input_request_enqueue(ops->context, 0xb, NULL, 0, 0);
        type = ops != NULL && ops->entity_type_get != NULL
            ? ops->entity_type_get(ops->context, entity) : 0;
        if (type == 1) {
            flags = ops != NULL && ops->entity_flags_get != NULL
                ? ops->entity_flags_get(ops->context, entity, 0x3e8) : 0u;
            if ((flags & 0x02000000u) == 0u &&
                (ops == NULL || ops->entity_byte_flags_get == NULL ||
                 (ops->entity_byte_flags_get(ops->context, entity, 0x3ec)
                  & 0x40u) == 0u))
                ui_action_dispatch_mode_request(state, 0xb, 0);
            else
                ui_action_dispatch_mode_request(state, 10, 0);
            return 1;
        }
        if (type == 2) {
            ui_action_dispatch_mode_request(state, 9, 0);
            return 1;
        }
        if (type == 4) {
            ui_action_dispatch_mode_request(state, 0xd, 0);
            return 1;
        }
        break;
    case 0x2d:
    case 0x2e:
        if (ops != NULL && ops->input_request_enqueue != NULL)
            ops->input_request_enqueue(ops->context, 9, NULL,
                                       action == 0x2d ? 0 : 1, 0);
        ui_action_dispatch_mode_request(state, 0xf, 0);
        return 1;
    case 0x2f:
        if (ops != NULL && ops->selected_entity_action_2f != NULL)
            ops->selected_entity_action_2f(ops->context, entity);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x30:
        ui_action_dispatch_mode_request(state, 0x10, 0);
        return 1;
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
        if (ops != NULL && ops->selected_entity_action_31_group != NULL) {
            int callback_argument = action == 0x31 ? 0 :
                action == 0x32 ? 1 : action == 0x33 ? 2 :
                action == 0x34 ? 3 : action == 0x35 ? 4 :
                action == 0x36 ? 5 : action == 0x38 ? 6 : 7;
            ops->selected_entity_action_31_group(ops->context,
                                                 callback_argument);
        }
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x39:
        ui_action_dispatch_mode_request(state, 0x11, 0);
        return 1;
    case 0x3a:
    case 0x3b:
    case 0x3c:
    case 0x3d:
    case 0x3e:
    case 0x3f:
        flags = action == 0x3a ? 4u : action == 0x3b ? 0x10u :
                action == 0x3c ? 0x20u : action == 0x3d ? 1u :
                action == 0x3e ? 2u : 8u;
        if (ops == NULL || ops->entity_action_record_find == NULL)
            return 0;
        {
            void* action_record = ops->entity_action_record_find(
                ops->context, entity, 0, (int)flags);
            if (action_record == NULL) return 0;
            if (ops->entity_action_record_apply != NULL)
                ops->entity_action_record_apply(ops->context, action_record,
                                                (int)flags, 1);
        }
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x40:
        if (ops != NULL && ops->object_command_task_create != NULL)
            ops->object_command_task_create(ops->context, entity, 0);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x41:
        if (ops != NULL && ops->building_upgrade_commit != NULL)
            ops->building_upgrade_commit(ops->context, entity);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x42:
        ui_action_dispatch_mode_request(state, 0x12, 0);
        return 1;
    case 0x43:
    case 0x44:
    case 0x45:
    case 0x46:
        if (ops != NULL && ops->object_command_task_create != NULL)
            ops->object_command_task_create(ops->context, entity,
                action == 0x43 ? 1 : action == 0x44 ? 0 :
                action == 0x45 ? 2 : 3);
        if (ops != NULL && ops->selected_entity_field_f6_clear != NULL)
            ops->selected_entity_field_f6_clear(ops->context, entity);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x47:
        if ((state->action_flags & 1u) == 0u) {
            ui_action_dispatch_global_low_byte_clear(state);
            state->action_flags |= 1u;
            global_flag_bit0_clear();
            state->cursor_flags &= ~1u;
            ui_action_dispatch_selection_clear(state);
            return 1;
        }
        ui_action_dispatch_global_low_byte_clear(state);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x48:
        if (ops != NULL && ops->input_request_enqueue != NULL)
            ops->input_request_enqueue(ops->context, 3, NULL, 0, 0);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x49:
        if (ops != NULL && ops->cursor_action_rejected != NULL &&
            ops->cursor_action_rejected(ops->context, state->cursor_action_x,
                state->cursor_action_y, 1)) return 0;
        if (ops != NULL && ops->cursor_action_pair_request != NULL)
            ops->cursor_action_pair_request(ops->context,
                state->cursor_action_x, state->cursor_action_y);
        if (state->cursor_action_x == 4) return 1;
        break;
    default:
        break;
    }
    ui_action_dispatch_selection_clear(state);
    return 1;

action_flag_set:
    if (action == 0x14) flags = 0x10u;
    else if (action == 0x15) flags = 8u;
    else if (action == 0x1b) flags = 2u;
    else flags = 4u;
action_flag_toggle:
    ui_action_dispatch_global_low_byte_clear(state);
    state->action_flags |= flags;
    return 1;
}

void selection_clear_request_and_hud_reset_port(void)
{
    /* FUN_0041e750 enqueues command 3 with a null entity and pair, then posts
     * the HUD context with kind 0 / no coordinate. */
    cmd_produce(3, NULL, 0, NULL); /* FUN_00452060 */
    command_context_post_to_hud(0, NULL);
}

uint32_t input_action_click_count_swap(int action, uint32_t value)
{
    uint32_t old = input_action_click_count_get(action);
    if (action >= 0 && action < INPUT_FLAG_TABLE_SLOTS)
        s_input_action_click_counts[action] = value;
    return old;
}
uint32_t input_action_click_count_get(int action)
{
    if (action < 0 || action >= INPUT_FLAG_TABLE_SLOTS) return 0u;
    return s_input_action_click_counts[action];
}

uint32_t input_binding_click_count_swap(int kind, int entry, uint32_t value)
{
    uint32_t* table = player_click_count_table(kind);
    uint32_t old;
    if (table == NULL || entry < 0 || entry >= INPUT_FLAG_TABLE_SLOTS)
        return 0u;
    old = table[entry];
    table[entry] = value;
    return old;
}
uint32_t input_binding_click_count_get(int kind, int entry)
{
    const uint32_t* table = player_click_count_table(kind);
    if (table == NULL || entry < 0 || entry >= INPUT_FLAG_TABLE_SLOTS)
        return 0u;
    return table[entry];
}

int cell_action_permission_get_port(int column, int row)
{
    if ((system_state_flags_get() & 4u) == 0u) {
        return 1;
    }
    const int index = row + column * 15; /* the corpus's 0xf stride */
    if (index < 0 || index >= CELL_PERMISSION_ENTRIES) {
        return 0; /* deviation: the corpus strides blindly */
    }
    /* The retail helper returns the table dword itself, not a normalized
     * boolean. Keep callers that use its value semantics intact. */
    return (int)s_cell_perm[index];
}
void selection_region_parameters_set(uint32_t a, uint32_t b, uint32_t c,
                                     uint32_t d, float e)
{
    s_reg_e0 = (float)c;
    s_reg_e4 = (float)d;
    s_reg_e8 = (float)a;
    s_reg_ec = (float)b;
    s_reg_f0 = e * s_region_scale_004;
}

void store_selected_map_coordinate(int kind, const uint32_t coord[2])
{
    if ((s_inter_flags & 0x80u) == 0u
        || s_obj_sel_word != (uint32_t)kind) {
        s_inter_flags = (s_inter_flags & ~0x80u) | 0x100u;
        sound_alias_play(0x16, 0);
    } else {
        s_obj_sel_word = (uint32_t)kind;
        if (coord == NULL) {
            s_map_store_kind = (uint32_t)kind;
            return;
        }
        s_map_store_pair_x = coord[0];
        s_map_store_pair_y = coord[1];
    }
    if (coord != NULL) {
        s_map_store_last_x = coord[0];
        s_map_store_last_y = coord[1];
    }
    s_map_store_kind = (uint32_t)kind;
}

void event_timer_countdown_advance(float delta)
{
    EventTimerState state;
    frontend_event_timer_state_get(&state);
    if ((state.flags & 0x80u) != 0u) return;
    if ((state.flags & 0x100u) == 0u) {
        if ((state.flags & 4u) != 0u) {
            state.timer -= delta * state.rate;
            if (state.timer <= state.lower) {
                state.flags |= 0x80u;
                state.timer = state.lower;
            }
        }
        frontend_event_timer_state_set(&state);
        return;
    }
    state.timer += delta * state.rate;
    if (state.current_event == 0x16 &&
        (state.external_flags & 2u) != 0u) {
        if (s_event_timer_action_hook != NULL) {
            s_event_timer_action_hook(s_event_timer_action_context, 8, 0);
            s_event_timer_action_hook(s_event_timer_action_context, 6, 1);
        }
        state.timer = state.upper;
    }
    if (state.timer < state.upper) {
        frontend_event_timer_state_set(&state);
        return;
    }
    state.flags &= ~0x100u;
    state.timer = state.upper;
    sound_alias_play(0x15, 0);
    if (state.next_event == 0x16) {
        state.flags = (state.flags & ~4u) | 0x80u;
        if (s_event_timer_action_hook != NULL)
            s_event_timer_action_hook(s_event_timer_action_context, 8, 0);
        simtick_global_ui_flags_clear_bits12();
        simtick_global_ui_flags_set_bit2();
    }
    else if (state.next_event == 0x17) {
        state.flags = (state.flags & ~4u) | 0x80u;
        if (s_event_timer_action_hook != NULL)
            s_event_timer_action_hook(s_event_timer_action_context, 0x0b, 0);
        global_state_low_word_set(global_state_low_word_get() | 1u);
        global_flag_bit0_clear();
    }
    else {
        state.flags |= 4u;
    }
    state.current_event = state.next_event;
    state.current_x = state.next_x;
    state.current_y = state.next_y;
    frontend_event_timer_state_set(&state);
}

/* AdvisorDialogStackLayout (FUN_0041eca0): refresh geometry when dimensions
 * change, then lay out icons and queued entries upward from the top point. */
void advisor_dialog_stack_layout(float y, uint32_t parameter,
                                 uint32_t width, uint32_t height)
{
    AdvisorDialogStackEntry* entries = NULL;
    uint32_t count = 0;
    uint32_t index;
    (void)parameter;
    if (width != s_advisor_dialog_stack_width ||
        height != s_advisor_dialog_stack_height) {
        if (s_advisor_dialog_stack_ops.layout_reset != NULL) {
            s_advisor_dialog_stack_ops.layout_reset(
                s_advisor_dialog_stack_ops.context, width, height);
        }
        s_advisor_dialog_stack_width = width;
        s_advisor_dialog_stack_height = height;
    }
    if (s_advisor_dialog_stack_ops.active == NULL ||
        !s_advisor_dialog_stack_ops.active(
            s_advisor_dialog_stack_ops.context) ||
        s_advisor_dialog_stack_ops.entries_get == NULL ||
        !s_advisor_dialog_stack_ops.entries_get(
            s_advisor_dialog_stack_ops.context, width, height,
            &entries, &count) || entries == NULL) return;

    y -= 15.0f;
    for (index = 0; index < count; ++index) {
        void* icon = NULL;
        void* item = NULL;
        int resource_height = 0;
        if (s_advisor_dialog_stack_ops.icon_get != NULL) {
            icon = s_advisor_dialog_stack_ops.icon_get(
                s_advisor_dialog_stack_ops.context, index == 0 ? 0u : 1u);
        }
        if (icon == NULL) y -= 20.0f;
        else {
            if (s_advisor_dialog_stack_ops.icon_height != NULL) {
                resource_height = s_advisor_dialog_stack_ops.icon_height(
                    s_advisor_dialog_stack_ops.context, icon);
            }
            y -= (float)resource_height;
            if (s_advisor_dialog_stack_ops.icon_place != NULL) {
                s_advisor_dialog_stack_ops.icon_place(
                    s_advisor_dialog_stack_ops.context, icon, y);
            }
        }
        if (s_advisor_dialog_stack_ops.item_resolve != NULL) {
            int eligible = 0;
            if (s_advisor_dialog_stack_ops.entry_eligible != NULL) {
                eligible = s_advisor_dialog_stack_ops.entry_eligible(
                    s_advisor_dialog_stack_ops.context, entries[index].record);
            }
            item = s_advisor_dialog_stack_ops.item_resolve(
                s_advisor_dialog_stack_ops.context, entries[index].record,
                eligible);
        }
        if (item == NULL) y -= 40.0f;
        else {
            resource_height = 0;
            if (s_advisor_dialog_stack_ops.item_height != NULL) {
                resource_height = s_advisor_dialog_stack_ops.item_height(
                    s_advisor_dialog_stack_ops.context, item);
            }
            y -= (float)resource_height;
            if (s_advisor_dialog_stack_ops.item_place != NULL) {
                s_advisor_dialog_stack_ops.item_place(
                    s_advisor_dialog_stack_ops.context, item, y);
            }
            if (entries[index].has_special && entries[index].item_id != 0u &&
                s_advisor_dialog_stack_ops.special_item_apply != NULL) {
                s_advisor_dialog_stack_ops.special_item_apply(
                    s_advisor_dialog_stack_ops.context,
                    entries[index].item_id);
            }
        }
    }
}

void interaction_timer_reset_and_flag_clear(void)
{
    s_inter_timer = 25.0f; /* corpus 0x41c80000 */
    s_inter_flags &= ~0x800u;
}

namespace unported_ledger_stubs {
int ledger_stub_0041ee80_CellGridPointHitTest(
    uint32_t x, uint32_t y, float region_x, float region_y,
    uint32_t* action_out, uint32_t* cell_x_out, uint32_t* cell_y_out)
{
    CellGridPointHitTestState* state = s_cell_grid_point_hit_test_state;
    CellGridPointHitTestState live_state;
    CellGridPointHitRecord live_records[DEPENDENCY_GRID_MATCH_CAPACITY];
    float point_x = (float)x;
    const float point_y = (float)y;
    uint32_t index;

    if (action_out == NULL || (s_inter_flags & 0x800u) == 0u) return 0;
    /* The retail caller obtains its 16-byte {type,row,selector,wildcard}
     * list directly from TerrainCellRecordGet using the current cursor pair.
     * LevelState now owns that same list, so build the target's three-word
     * hit-test view from the native-width records for this synchronous call. */
    if (state == NULL) {
        if (!cell_grid_point_hit_live_state_build(&live_state, live_records))
            return 0;
        state = &live_state;
    }
    if (state == NULL || state->record_count == 0u ||
        state->records == NULL) return 0;

    region_x -= 15.0f;
    if (region_x <= point_x && point_x <= region_x + 15.0f &&
        region_y <= point_y && point_y <= region_y + 40.0f) {
        *action_out = 0x14u;
        return 1;
    }

    for (index = 0u; index < state->record_count; ++index) {
        float width;
        float height;
        float left;
        /* The reference selects DAT_004de1e8 for record zero, then
         * DAT_004de1e4 for every later record.  The adapter keeps those two
         * source slots in rectangle_width/height order. */
        const int rectangle = index == 0u ? 1 : 0;
        const CellGridPointHitRecord* record = &state->records[index];

        if (state->rectangles_present[rectangle]) {
            width = state->rectangle_width[rectangle];
            height = state->rectangle_height[rectangle];
        }
        else if (s_interface_button_surface_native[rectangle == 0 ? 1 : 0]
                 != NULL) {
            TextureSurface* image =
                s_interface_button_surface_native[rectangle == 0 ? 1 : 0];
            width = (float)image->width;
            height = (float)image->height;
        }
        else {
            width = 20.0f;
            height = 40.0f;
        }
        left = region_x - width;
        if (left <= point_x && point_x <= left + width &&
            region_y <= point_y && point_y <= region_y + height) {
            *action_out = 0x14u;
            return 1;
        }

        width = 40.0f;
        height = 40.0f;
        /* FUN_0041cb90 receives the first two dwords of the 16-byte
         * record; the third dword is only copied to the caller on a hit. */
        if (!(state->record_size_get != NULL
                  ? state->record_size_get(state->context, record->action,
                                           record->x, &width, &height)
                  : cell_grid_point_hit_record_size_get(
                        NULL, record->action, record->x, &width, &height))) {
            width = 40.0f;
            height = 40.0f;
        }
        region_x = left - width;
        if (region_x <= point_x && point_x <= region_x + width &&
            region_y <= point_y && point_y <= region_y + height) {
            *action_out = record->action;
            if (cell_x_out != NULL) *cell_x_out = record->x;
            if (cell_y_out != NULL) *cell_y_out = record->y;
            return 1;
        }
    }
    return 0;
}
} // namespace unported_ledger_stubs

/* ObjectInteractionFlagsAllow (FUN_0041f100): flags are checked in the
 * corpus's priority order. Only the first present capability bit selects
 * a state mask; no capability bits means the action is allowed directly. */
int object_interaction_flags_allow(GameContext* ctx, int type, int row)
{
    const uint32_t capabilities = stats_table_word_at_148(type, row);
    uint32_t required_stats;
    if ((capabilities & 0x1000u) != 0u) {
        required_stats = 0x10u;
    }
    else if ((capabilities & 0x800u) != 0u) {
        required_stats = 0x20u;
    }
    else if ((capabilities & 0x2000u) != 0u) {
        required_stats = 0x40u;
    }
    else if ((capabilities & 0x400000u) != 0u) {
        required_stats = 0x80000000u;
    }
    else {
        return 1;
    }
    const EntityActionRequest request = { 0, 0, required_stats, 0, 0, 0 };
    return entity_action_request_find(ctx, &request);
}
/* QueueActionRequestType2 (FUN_0041f230): the callback tests state +0x148
 * bit 2 across all live entities, after the pool's +0x3e8 filter. */
int queue_action_request_type2(GameContext* ctx)
{
    const EntityActionRequest request = { 0, 0, 0, 2u, 0, 0 };
    return entity_action_request_find(ctx, &request);
}

/* QueueActionRequestValue (FUN_0041f270): search state +0x148 for the
 * supplied mask. */
int queue_action_request_value(GameContext* ctx, uint32_t state_mask)
{
    const EntityActionRequest request = { 0, 0, 0, state_mask, 0, 0 };
    return entity_action_request_find(ctx, &request);
}

/* QueueActionRequestFlag100000 (FUN_0041f2b0): search stats word +0x144
 * for bit 0x100000. */
int queue_action_request_flag100000(GameContext* ctx)
{
    const EntityActionRequest request = { 0, 0, 0x100000u, 0, 0, 0 };
    return entity_action_request_find(ctx, &request);
}

/* QueueActionRequestPair (FUN_0041f2f0): mode 1 compares both leading
 * entity words with the two request values. The corpus at 0x41f2f0 clears
 * the inactive masks, ORs mode 1 into a zero word, then supplies the pair. */
int queue_action_request_pair(GameContext* ctx, int entity_type,
                              uint32_t entity_subtype)
{
    EntityActionRequest request;
    request.subtype = 0u;
    request.stats_mask = 0u;
    request.state_mask = 0u;
    request.flags3f0_mask = 0u;
    uint32_t mode = 0u;
    mode |= 1u;
    request.mode = mode;
    request.type = (uint32_t)entity_type;
    request.subtype = entity_subtype;
    return entity_pool_find_by_callback_filtered(
        ctx, entity_action_request_matches, &request);
}

/* QueueActionRequestIndex (FUN_0041f340): mode 2 asks whether any entity
 * has the requested bit in its +0x3f0 flags word. Keep the 0x41f340
 * zero-mask / zero-word-then-OR setup shared with QueueActionRequestPair. */
int queue_action_request_index(GameContext* ctx, uint32_t flags_mask)
{
    EntityActionRequest request;
    request.subtype = 0u;
    request.stats_mask = 0u;
    request.state_mask = 0u;
    request.flags3f0_mask = 0u;
    uint32_t mode = 0u;
    mode |= 2u;
    request.mode = mode;
    request.flags3f0_mask = flags_mask;
    return entity_pool_find_by_callback_filtered(
        ctx, entity_action_request_matches, &request);
}

/* QueueCallbackRequest390 (FUN_0041f3c0): LAB_0041f390 invokes
 * TerrainCellWalkabilityQuery(entity, x, y, 0, 0). This preserves that
 * call's entity flag/owner and per-terrain speed gates before the simple
 * cell walkability predicate. */
int queue_callback_request_390(GameContext* ctx, int x, int y)
{
    if (ctx == NULL || ctx->level == NULL) {
        return 0;
    }
    QueueCellRequest cell = { ctx->level, x, y };
    return entity_pool_find_by_callback_filtered(
        ctx, queue_walkable_cell_matches, &cell);
}
/* QueueCallbackRequest3E0 (FUN_0041f400): its callback forwards each live
 * entity and the requested cell to TerrainCellAllowsPlacement (FUN_00432a20). */
int queue_callback_request_3e0(GameContext* ctx, int x, int y)
{
    if (ctx == NULL || ctx->level == NULL) {
        return 0;
    }
    QueueCellRequest cell = { ctx->level, x, y };
    return entity_pool_find_by_callback_filtered(
        ctx, queue_placement_cell_matches, &cell);
}
/* SelectedEntitiesAnyMatch (FUN_0041f420): the corpus fetches count and
 * array through 0x4525f0/0x4525e0, then returns on the first callback hit.
 * The port routes through selection_count/selection_record_array, whose own
 * NULL guards cover ctx, and hands each entry to the predicate blind — the
 * predicates NULL-check it themselves. */
int selected_entities_any_match(const GameContext* ctx,
                                int (*predicate)(EntitySlot*, uint32_t),
                                uint32_t argument)
{
    int count = selection_count(ctx);
    EntitySlot* const* entries = selection_record_array(ctx);
    unsigned i = 0;
    EntitySlot* const* p = entries;
    while (i < (unsigned)count) {
        if (predicate(*p, argument) != 0) {
            return 1;
        }
        ++i;
        ++p;
    }
    return 0;
}

/* SelectedEntityDetachCallback (FUN_0041f6c0): capability bit 0 chooses
 * between the attachment detach path and RecordPairPoseCopyAndActivityRaise
 * for the carried object. */
int selected_entity_detach_callback(EntitySlot* entity)
{
    EntitySlot* attached;
    uint32_t capabilities;
    capabilities = entity_capability_flags_get(entity);
    attached = (EntitySlot*)entity->carry_link_308;
    if ((capabilities & 1u) != 0u) {
        if ((entity->flags_3e0 & 0x400u) != 0u && attached != NULL) {
            unported_ledger_stubs::
                ledger_stub_0043fc20_RecordPairPoseCopyAndActivityRaise(
                    attached);
        }
    }
    else if ((entity->flags_3e0 & 0x400u) == 0u || attached == NULL
             || attached->type != 9) {
        unported_ledger_stubs::entity_attached_object_list_detach(entity, 0);
    }
    return 0;
}

/* CurrentLevelStateFlagsContainMask (FUN_0041f720) — see select.hpp. */
int current_level_state_flags_contain_mask(const GameContext* ctx,
                                           uint32_t mask)
{
    const EntitySlot* e = selection_first(ctx);
    if (e == NULL) {
        return 0; /* deviation: the corpus reads *(NULL + 0x3f0) blindly */
    }
    return (e->flags_3f0 & mask) != 0u;
}

/* CellFourNeighborWalkCheck (FUN_0041f740): the tutorial path gate uses
 * this asymmetric cardinal-neighbor rule before submitting a cell action.
 * Neighbor order follows the corpus's local table: east, south, west,
 * north. */
int cell_four_neighbor_walk_check(const LevelState* s, int cx, int cy)
{
    static const int8_t offsets[4][2] = {
        { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 },
    };
    const int marked = level_cell_mask_c0(s, cx, cy) != 0;
    int occupied_neighbors = 0;
    for (int i = 0; i < 4; ++i) {
        const int neighbor = level_cell_occupied_or_flag(
            s, cx + offsets[i][0], cy + offsets[i][1], 1) != 0;
        if (neighbor != 0) {
            if (marked == 0) {
                return 0;
            }
            ++occupied_neighbors;
        }
    }
    return marked == 0 || occupied_neighbors <= 1;
}

/* RemoveObjectIfClass6 (FUN_0041f840) performs the activity-6 scan and
 * raises the tick-down marker when available; the corpus returns zero
 * regardless of whether the marker was created. */
int entity_remove_object_if_class6(GameContext* ctx, EntitySlot* e)
{
    if (e != NULL && ai_activity_scan(e, 6) != 0) {
        entity_kind6_tick_down_marker_spawn(ctx, e);
    }
    return 0;
}

static int fe_token_is(const char* token, size_t length, const char* expected)
{
    if (strlen(expected) != length) {
        return 0;
    }
    return strnicmp_lite(token, expected, (int)length) == 0;
}

static int fe_token_copy(char* out, size_t out_size,
                         const char* token, size_t token_size)
{
    if (out == NULL || out_size == 0 || token_size >= out_size) {
        return 0;
    }
    memcpy(out, token, token_size);
    out[token_size] = '\0';
    return 1;
}

/* A tooltip token is either text or text|sound-alias. */
static void fe_interface_tooltip_parse(const char* token, size_t length,
                                       char* text, size_t text_size,
                                       int* sound, int* has_sound)
{
    const char* pipe = (const char*)memchr(token, '|', length);
    const size_t text_length = pipe != NULL ? (size_t)(pipe - token) : length;
    (void)fe_token_copy(text, text_size, token, text_length);
    *sound = -1;
    *has_sound = 0;
    if (pipe != NULL && (size_t)(token + length - pipe - 1) > 0) {
        char alias[FE_NAME_MAX];
        const size_t alias_length = (size_t)(token + length - pipe - 1);
        if (fe_token_copy(alias, sizeof(alias), pipe + 1, alias_length)
            && sound_alias_lookup(alias, sound)) {
            *has_sound = 1;
        }
    }
}

namespace unported_ledger_stubs {
static int cell_grid_point_hit_record_size_get(void* context,
                                              uint32_t type,
                                              uint32_t row,
                                              float* width, float* height)
{
    TextureSurface* image;
    (void)context;
    image = (TextureSurface*)input_binding_default_value_get(
        (int)type, (int)row, 0);
    if (image == NULL) return 0;
    if (width != NULL) *width = (float)image->width;
    if (height != NULL) *height = (float)image->height;
    return 1;
}

static int cell_grid_point_hit_live_state_build(
    CellGridPointHitTestState* live_state,
    CellGridPointHitRecord* live_records)
{
    GameContext* game;
    const unported_ledger_stubs::DependencyGridTuple* tuples = NULL;
    uint32_t tuple_count = 0u;
    uint32_t index;
    uint32_t row = cursor_tile_pair_peek(0);
    uint32_t column = cursor_tile_pair_peek(1);
    if (live_state == NULL || live_records == NULL) return 0;
    game = entity_pool_callback_context_get();
    if (game == NULL || game->level == NULL ||
        !unported_ledger_stubs::dependency_grid_cell_record_get_native(
            game->level, (int)row, (int)column, 0, &tuples,
            &tuple_count)) return 0;
    if (tuple_count > DEPENDENCY_GRID_MATCH_CAPACITY)
        tuple_count = DEPENDENCY_GRID_MATCH_CAPACITY;
    for (index = 0u; index < tuple_count; ++index) {
        live_records[index].action = tuples[index].first;
        live_records[index].x = tuples[index].second;
        live_records[index].y = tuples[index].selector;
        live_records[index].reserved = tuples[index].wildcard;
    }
    memset(live_state, 0, sizeof(*live_state));
    live_state->flags = s_inter_flags;
    live_state->records = live_records;
    live_state->record_count = tuple_count;
    live_state->context = NULL;
    live_state->record_size_get = cell_grid_point_hit_record_size_get;
    return 1;
}
} // namespace unported_ledger_stubs

static void ui_action_dispatch_mode_request(UiActionDispatchState* state,
                                            int mode, int argument)
{
    const UiActionDispatchOps* ops = state->ops;
    if (ops != NULL && ops->ui_mode_request != NULL)
        ops->ui_mode_request(ops->context, mode, argument);
}

static void ui_action_dispatch_selection_clear(UiActionDispatchState* state)
{
    const UiActionDispatchOps* ops = state->ops;
    if (ops != NULL && ops->selection_clear_request_and_hud_reset != NULL)
        ops->selection_clear_request_and_hud_reset(ops->context);
}

static void ui_action_dispatch_global_low_byte_clear(
    UiActionDispatchState* state)
{
    const UiActionDispatchOps* ops = state->ops;
    if (ops != NULL && ops->global_state_low_byte_clear_and_refresh != NULL)
        ops->global_state_low_byte_clear_and_refresh(ops->context);
    state->action_flags &= ~0xffu;
}

static int menu_kind_gated(int kind)
{
    return kind == 1 || kind == 2 || kind == 3 || kind == 4
        || kind == 5 || kind == 7;
}

static int entity_action_request_matches(EntitySlot* entity, void* payload)
{
    const EntityActionRequest* request = (const EntityActionRequest*)payload;
    if (entity == NULL || request == NULL) {
        return 0;
    }
    if (entity->type == 4 && entity_shared_eligibility_check(entity, 0) == 0) {
        return 0;
    }
    if ((request->mode & 1u) != 0u
        && ((uint32_t)entity->type != request->type
            || (uint32_t)entity->etype != request->subtype)) {
        return 0;
    }
    if ((request->mode & 2u) != 0u
        && (entity->flags_3f0 & request->flags3f0_mask) == 0u) {
        return 0;
    }
    if (request->stats_mask != 0u
        && (entity_stats_word_get(entity) & request->stats_mask) == 0u) {
        return 0;
    }
    if (request->state_mask != 0u
        && (entity_state_flags_get(entity) & request->state_mask) == 0u) {
        return 0;
    }
    return 1;
}

static int entity_action_request_find(GameContext* ctx,
                                     const EntityActionRequest* request)
{
    return entity_pool_find_by_callback_filtered(
        ctx, entity_action_request_matches, (void*)request);
}

static int selected_entity_activity_present(EntitySlot* entity,
                                            uint32_t activity)
{
    return ai_activity_scan(entity, (int)activity);
}

static int selected_entity_activity_missing(EntitySlot* entity,
                                            uint32_t activity)
{
    return entity != NULL && ai_activity_scan(entity, (int)activity) == 0;
}

static int selected_entity_action_state_matches(EntitySlot* entity,
                                                uint32_t unused)
{
    (void)unused;
    return entity != NULL && (entity->flags_3e0 & 0x400u) != 0u
        && (entity->flags_3e0 & 0x2000u) == 0u;
}

static int selected_entity_progress_below_100(EntitySlot* entity,
                                              uint32_t unused)
{
    (void)unused;
    return entity != NULL && entity->progress_344 < 100.0f;
}

static int selected_entity_field340_below_100(EntitySlot* entity,
                                              uint32_t unused)
{
    (void)unused;
    return entity != NULL && entity->field_340 < 100.0f;
}

static int selected_action_14_15_nonroute_matches(EntitySlot* entity)
{
    if (entity == NULL || (entity_capability_flags_get(entity) & 1u) != 0u) {
        return 0;
    }
    if (entity->type == 2) {
        return (entity->flags_3e0 & 0x400u) == 0u
            && (entity->flags_3e8 & 0x60u) == 0x60u;
    }
    if ((entity->flags_3e8 & 0x60u) != 0x60u) {
        return 0;
    }
    uint32_t limit = entity->field_328;
    const uint32_t deposit_count = entity_stats_word_at_ec(entity);
    if (deposit_count < limit) {
        limit = deposit_count;
    }
    return (uint32_t)entity->carry_state_324 < limit;
}

/* These menu predicates have direct ports in the entity module. The
 * callback parameters remain overrides for hosts with additional runtime
 * state, while the normal game path can use the live EntitySlot/config APIs. */
static int selection_target_cell_valid_default(GameContext* ctx,
                                                EntitySlot* entity)
{
    EntitySlot* target;
    void* resource;
    float position[3];
    int cell_x, cell_y;
    if (ctx == NULL || ctx->level == NULL || entity == NULL ||
        (entity_capability_flags_get(entity) & 1u) == 0u ||
        (entity->flags_3e0 & 0x400u) == 0u || entity->carry_link_308 == NULL)
        return 0;
    target = (EntitySlot*)entity->carry_link_308;
    if (target->type != 1) return 0;

    /* EntityActionTargetCellIsValid @ 0x43fdc0 resolves the source's visual
     * scene, asks DDiScene for its position, converts that position to a map
     * cell, and tests that same cell through the linked type-1 target. */
    resource = entity->visual_resource_handles[0];
    if (resource == NULL && entity_type1_scene_resource_bind(entity))
        resource = entity->scene_resource_type1.scene_resource;
    if (resource == NULL) {
        entity_visual_resource_get_or_create(entity);
        resource = entity->visual_resource_handles[0];
        if (resource == NULL && entity_type1_scene_resource_bind(entity))
            resource = entity->scene_resource_type1.scene_resource;
    }
    if (resource == NULL ||
        !ddiscene_object_position_get(resource, NULL, position) ||
        !terrain_world_xy_to_cell(ctx->level->ptr_terrain, position[0],
                                  position[1], &cell_x, &cell_y)) return 0;
    return terrain_cell_route_and_occupancy_check(
        target, ctx->level, cell_x, cell_y, cell_x, cell_y, 1);
}

static int selection_route_marker_candidate_default(EntitySlot* entity,
                                                       int include_secondary)
{
    return entity_route_marker_candidate_find(entity, include_secondary)
        != NULL;
}

static int selection_vehicle_unload_eligible_default(EntitySlot* entity,
                                                       int unused)
{
    (void)unused;
    return entity_vehicle_unload_cell_find_if_eligible(entity, NULL);
}

static int selection_unload_cell_available_default(EntitySlot* entity,
                                                     int unused)
{
    (void)unused;
    return entity_record_flag20_query(entity, NULL);
}

static int selection_building_upgrade_allowed_default(EntitySlot* entity)
{
    return building_upgrade_resource_admission_check(entity, NULL);
}

static int selection_entity_action_prerequisite_default(EntitySlot* entity,
                                                         int action)
{
    return entity_action_kind_prerequisite_check(entity, action);
}

static int selection_building_upgrade_limit_default(EntitySlot* entity)
{
    uint32_t maximum;
    if (entity == NULL) return 0;
    maximum = building_upgrade_admission_check(entity->type, entity->etype);
    return maximum > (uint32_t)entity->indexed_record_slot_330 + 1u;
}

static int queue_placement_cell_matches(EntitySlot* entity, void* payload)
{
    const QueueCellRequest* cell = (const QueueCellRequest*)payload;
    return cell != NULL
        && terrain_cell_allows_placement(entity, cell->level, cell->x, cell->y);
}

static int queue_walkable_cell_matches(EntitySlot* entity, void* payload)
{
    const QueueCellRequest* cell = (const QueueCellRequest*)payload;
    if (entity == NULL || cell == NULL || cell->level == NULL) {
        return 0;
    }
    if ((entity_stats_word_get(entity) & 0x20000u) != 0u
        && entity->owner_36c == NULL) {
        return 0;
    }
    if ((entity->flags_3e8 & 2u) == 0u) {
        return 0;
    }
    const uint8_t* voxel = level_voxel(cell->level, cell->x, cell->y);
    if (voxel == NULL) {
        return 0;
    }
    const int terrain_type = (int)(signed char)voxel[2];
    if (terrain_type < 0 || terrain_type >= AI_STATS_TERRAIN_TYPES
        || entity_stats_float_at24_indexed(entity, terrain_type) == 0.0f) {
        return 0;
    }
    return level_walkable(cell->level, cell->x, cell->y);
}

/* TutorialPathMenuAction switch cases before the final runtime gate. */
static int tutorial_path_menu_action_precheck_raw(
    GameContext* ctx, const EngineContext* engine,
    TerrainDescriptorNode* descriptors,
    TerrainCellSidecarPresenceQuery sidecar_query,
    int (*target_cell_valid)(EntitySlot*),
    int (*route_marker_candidate)(EntitySlot*, int),
    int (*vehicle_unload_eligible)(EntitySlot*, int),
    int (*entity_unload_cell_available)(EntitySlot*, int),
    int (*selected_action_14_15_route_predicate)(EntitySlot*),
    int (*building_upgrade_allowed)(EntitySlot*),
    int (*entity_action_kind_prerequisite)(EntitySlot*, int),
    int (*building_upgrade_limit_allows)(EntitySlot*),
    int action, int x, int y)
{
    if (action != 0 && action != 1 && action != 5 && action != 6 && action != 7
        && action != 0xb
        && action != 0xc && action != 0xd && action != 0xe && action != 0x10
        && action != 0x9
        && action != 0x1d && action != 0x1e && action != 0x1f
        && action != 0x21 && action != 0x22 && action != 0x23
        && action != 0x24 && action != 0x25
        && action != 0x26 && action != 0x27
        && action != 0x16 && action != 0x17
        && action != 0x18
        && action != 0x19
        && action != 0x1a
        && action != 0x1b && action != 0x1c
        && action != 0x14 && action != 0x15
        && action != 0x20
        && action != 0x29 && action != 0x30 && action != 0x31
        && action != 0x32 && action != 0x33 && action != 0x34
        && action != 0x35 && action != 0x36 && action != 0x37
        && action != 0x38 && action != 0x39 && action != 0x3a
        && action != 0x3b && action != 0x3c && action != 0x3d
        && action != 0x3e && action != 0x3f && action != 0x2d
        && action != 0x2e && action != 0x40 && action != 0x41 && action != 0x42
        && action != 0x43 && action != 0x44 && action != 0x45
        && action != 0x46) {
        return 1; /* the reference switch's default branch is a no-op */
    }
    if (action == 0x1b || action == 0x1c || action == 0x20) {
        return 1;
    }
    if (action == 0x14 || action == 0x15) {
        if (ctx == NULL) {
            return 0;
        }
        for (int i = 0; i < ctx->selection_count; ++i) {
            EntitySlot* const entity = ctx->selection[i];
            if (entity == NULL) {
                continue;
            }
            if ((entity_capability_flags_get(entity) & 1u) != 0u) {
                if ((entity->flags_3e0 & 0x400u) == 0u
                    && (entity->flags_3ec & 0x10000u) == 0u
                    && selected_action_14_15_route_predicate != NULL
                    && selected_action_14_15_route_predicate(entity) != 0) {
                    return 1;
                }
            }
            else if (selected_action_14_15_nonroute_matches(entity) != 0) {
                return 1;
            }
        }
        return 0;
    }
    if (action == 0) {
        return map_store_kind_peek() != 0xfu;
    }
    if (action >= 0x41 && action <= 0x46) {
        EntitySlot* const entity = selection_first(ctx);
        if (entity == NULL) {
            return 0;
        }
        if (action == 0x41) {
            return (building_upgrade_allowed != NULL
                ? building_upgrade_allowed(entity)
                : selection_building_upgrade_allowed_default(entity)) != 0;
        }
        if (queue_action_request_type2(ctx) == 0) {
            return 0;
        }
        int (*action_ready)(EntitySlot*, int) =
            entity_action_kind_prerequisite != NULL
                ? entity_action_kind_prerequisite
                : selection_entity_action_prerequisite_default;
        if (action == 0x42) {
            return action_ready(entity, 1) != 0
                || action_ready(entity, 0) != 0
                || action_ready(entity, 2) != 0
                || action_ready(entity, 3) != 0;
        }
        return action_ready(
            entity, action == 0x43 ? 1 : action == 0x44 ? 0
                : action == 0x45 ? 2 : 3) != 0;
    }
    if (action == 0x40) {
        EntitySlot* const entity = selection_first(ctx);
        return entity != NULL && (entity->flags_3e0 & 0x400u) == 0u
            && (building_upgrade_limit_allows != NULL
                ? building_upgrade_limit_allows(entity)
                : selection_building_upgrade_limit_default(entity)) != 0
            && queue_action_request_flag100000(ctx) != 0;
    }
    if (ctx == NULL) {
        return 0;
    }

    if (action == 1) {
        int barracks_type = 0;
        int barracks_row = 0;
        (void)entity_type_row_for_name(
            "Barracks", &barracks_type, &barracks_row);
        return object_interaction_flags_allow(ctx, 2, 0) != 0
            && ensure_cell_entries_loaded(2u, 0, 0) != 0
            && entity_state_pair_predicate(
                   ctx, 2, 0, barracks_type, barracks_row) != 0;
    }

    if (action == 0x2d || action == 0x2e) {
        if (engine == NULL || engine->views[2] == NULL) {
            return 0;
        }
        const int required_view_mode = (action == 0x2d) ? 0 : 1;
        return selection_first(ctx) != current_selection_context_get()
            || engine->views[2]->a5 != required_view_mode;
    }

    if (action == 0x9) {
        return selection_has_any(ctx) == 0
            || active_entity_list_contains_code(ctx, 1) != 0
            || queue_action_request_flag100000(ctx) != 0;
    }
    if (action == 0x1e) {
        return active_entity_list_contains_code(ctx, 0) != 0
            || queue_action_request_flag100000(ctx) != 0;
    }
    if (action == 0x30) {
        return queue_action_request_flag100000(ctx);
    }
    if (action == 0x1f) {
        return selected_entities_any_match(
            ctx, selected_entity_progress_below_100, 0u);
    }
    if (action == 0x1a) {
        EntitySlot* const entity = selection_first(ctx);
        return entity != NULL
            && (entity->flags_3e0 & 2u) == 0u
            && (entity_stats_word_get(entity) & 0x20000u) != 0u
            && (entity->flags_3ec & 0x40u) == 0u
            && (route_marker_candidate != NULL
                ? route_marker_candidate(entity, 1)
                : selection_route_marker_candidate_default(entity, 1)) != 0;
    }
    if (action == 0x19) {
        EntitySlot* const entity = selection_first(ctx);
        if (entity == NULL || (entity->flags_3e0 & 1u) != 0u
            || (entity_stats_word_get(entity) & 0x20000u) == 0u) {
            return 0;
        }
        const uint32_t stats_flags = entity_stats_word_get(entity);
        return (stats_flags & 0x2000000u) != 0u
            || (vehicle_unload_eligible != NULL
                ? vehicle_unload_eligible(entity, 0)
                : selection_vehicle_unload_eligible_default(entity, 0)) != 0
            || (entity->flags_3ec & 0x40u) != 0u;
    }
    if (action == 0x18) {
        EntitySlot* const entity = selection_first(ctx);
        if (entity == NULL) {
            return 0;
        }

        int request_index = 2;
        uint32_t request_state_mask = 8u;
        const uint32_t capabilities = entity_capability_flags_get(entity);
        if ((capabilities & 0x100u) != 0u) {
            request_index = 1;
            request_state_mask = 4u;
        }
        else {
            const uint32_t stats_flags = entity_stats_word_get(entity);
            if ((stats_flags & 0x2000000u) != 0u) {
                if ((stats_flags & 0x1000000u) != 0u) {
                    request_index = 1;
                    request_state_mask = 4u;
                }
                else {
                    request_index = 4;
                    request_state_mask = 0x10u;
                }
            }
        }

        const int has_unload_capability = (capabilities & 0x20u) != 0u;
        const int has_unload_override = (entity->flags_3ec & 0x40u) != 0u;
        if ((!has_unload_capability || has_unload_override)
            && request_index == 2 && !has_unload_override) {
            return 0;
        }
        if (has_unload_capability && !has_unload_override
            && (entity_unload_cell_available != NULL
                ? entity_unload_cell_available(entity, 0)
                : selection_unload_cell_available_default(entity, 0)) == 0) {
            return 0;
        }
        if ((entity->flags_3e4 & 0x10u) != 0u) {
            return 0;
        }
        return queue_action_request_index(ctx, (uint32_t)request_index) != 0
            || (queue_action_request_pair(ctx, 2, 0u) != 0
                && queue_action_request_value(ctx, request_state_mask) != 0);
    }
    if (action == 0x21) {
        return selected_entities_any_match(
            ctx, selected_entity_activity_present, 6u);
    }
    if (action == 0x22 || action == 0x23) {
        EntitySlot* const entity = selection_first(ctx);
        return entity != NULL
            && (entity_state_flags_get(entity) & 0x400u) == 0u
            && (entity->flags_3e8 & 0x20000000u) != 0u;
    }
    if (action == 0x24) {
        if (selected_entities_any_match(
                ctx, selected_entity_field340_below_100, 0u) == 0) {
            return 0;
        }
        if (queue_action_request_index(ctx, 0x10u) != 0) {
            return 1;
        }
        return queue_action_request_pair(ctx, 2, 0u) != 0
            && queue_action_request_value(ctx, 0x40u) != 0;
    }
    if (action == 0x25) {
        EntitySlot* const entity = selection_first(ctx);
        return entity != NULL
            && (entity_stats_word_get(entity) & 0x70u) != 0u;
    }
    if (action == 0x16 || action == 0x17) {
        EntitySlot* const entity = selection_first(ctx);
        if (entity == NULL) {
            return 0;
        }
        if ((entity->flags_3e0 & 0x400u) != 0u
            && entity->carry_link_308 != NULL
            && ((const EntitySlot*)entity->carry_link_308)->type == 9) {
            return 0;
        }
        if ((entity_capability_flags_get(entity) & 1u) != 0u
            && (target_cell_valid != NULL
                ? target_cell_valid(entity)
                : selection_target_cell_valid_default(ctx, entity)) == 0) {
            return 0;
        }
        return selected_entities_any_match(
            ctx, selected_entity_action_state_matches, 0u);
    }
    if (action == 0x26 || action == 0x27) {
        EntitySlot* const entity = selection_first(ctx);
        if (entity == NULL) {
            return 0;
        }
        if (action == 0x26
            && entity_action_kind_supported(entity, 6, 0) == 0) {
            return 0;
        }
        return entity_action_kind_supported(entity, 7, 0);
    }
    if (action >= 0x31 && action <= 0x38) {
        static const uint8_t activities[8] = { 0, 1, 2, 3, 4, 5, 7, 6 };
        const uint32_t activity = activities[action - 0x31];
        return selected_entities_any_match(
                   ctx, selected_entity_activity_missing, activity) != 0
            && queue_action_request_flag100000(ctx) != 0;
    }
    if (action == 0x39) {
        if (queue_action_request_value(ctx, 0xfcu) == 0) {
            return 0;
        }
        const EntitySlot* entity = selection_first(ctx);
        if (entity == NULL) {
            return 0;
        }
        const uint32_t flags = entity->flags_3f0;
        return (flags & 0x3fu) != 0x3fu;
    }
    if (action >= 0x3a && action <= 0x3f) {
        static const uint32_t current_masks[6] = {
            4u, 0x10u, 0x20u, 1u, 2u, 8u
        };
        static const uint32_t request_masks[6] = {
            0x10u, 0x40u, 0x80u, 4u, 8u, 0x20u
        };
        EntitySlot* const entity = selection_first(ctx);
        const int index = action - 0x3a;
        return entity != NULL && ctx->level != NULL
            && current_level_state_flags_contain_mask(
                   ctx, current_masks[index]) == 0
            && queue_action_request_value(ctx, request_masks[index]) != 0
            && entity_stats_float_atf4(entity)
                   >= engine_float_field260_get(ctx->level);
    }

    if (action == 0x1d) {
        EntitySlot* const entity = selection_first(ctx);
        if (entity == NULL) {
            return 0;
        }
        return (entity->flags_3e8 & 2u) != 0u
            && (entity->flags_3e0 & 0x400u) == 0u;
    }

    if (ctx->level == NULL) {
        return 0;
    }

    if (action == 6) {
        return level_cell_occupied_at(ctx->level,
                                      (int)map_store_pair_word(0),
                                      (int)map_store_pair_word(1));
    }

    if (action == 5) {
        const uint32_t target[2] = {
            map_store_pair_word(0), map_store_pair_word(1)
        };
        return (descriptors != NULL
                   ? terrain_cell_predicate_check(descriptors, target)
                   : terrain_cell_predicate_check_current(target)) == 0
            && level_cell_occupied_at(ctx->level, (int)target[0],
                                      (int)target[1]) == 0
            && ensure_cell_entries_loaded(0xfu, 0, 0) != 0;
    }

    if (action == 7) {
        const uint32_t target[2] = {
            map_store_pair_word(0), map_store_pair_word(1)
        };
        const uint8_t* voxel = level_voxel(
            ctx->level, (int)target[0], (int)target[1]);
        return (descriptors != NULL
                   ? terrain_cell_predicate_check(descriptors, target)
                   : terrain_cell_predicate_check_current(target)) == 0
            && voxel != NULL && voxel[2] != 6u;
    }

    if (action == 0xe) {
        return level_cell_secondary_flag_8(ctx->level, x, y) == 0u
            && terrain_cell_has_occupied_or_flagged_neighbor(
                   ctx->level, x, y, sidecar_query) != 0
            && ensure_cell_entries_loaded(0xbu, 0, 0) != 0;
    }

    if (action == 0x10) {
        const uint8_t* voxel = level_voxel(ctx->level, x, y);
        if (voxel == NULL) {
            return 0;
        }
        uint32_t cell_flags;
        __builtin_memcpy(&cell_flags, voxel + 0x08, sizeof(cell_flags));
        return (cell_flags & 0x40100000u) != 0u
            && level_cell_flag_8000(ctx->level, x, y) == 0u
            && level_cell_occupied_at(ctx->level, x, y) == 0
            && level_cell_occupied_or_flag(ctx->level, x, y, 1) == 0u;
    }

    if (action == 0x29) {
        EntitySlot* const entity = selection_first(ctx);
        if (entity == NULL) {
            return 0;
        }
        int cell_x = -1;
        int cell_y = -1;
        entity_position_to_terrain_cell(entity, ctx->level, &cell_x, &cell_y);
        if (entity->owner_36c != NULL
            && terrain_cell_route_and_occupancy_check(
                   (const EntitySlot*)entity->owner_36c, ctx->level,
                   cell_x, cell_y, cell_x, cell_y, 1) == 0) {
            return 0;
        }
        if ((entity_capability_flags_get(entity) & 1u) != 0u
            && (entity->flags_3e0 & 0x400u) != 0u
            && entity->carry_link_308 != NULL
            && terrain_cell_route_and_occupancy_check(
                   (const EntitySlot*)entity->carry_link_308, ctx->level,
                   cell_x, cell_y, cell_x, cell_y, 1) == 0) {
            return 0;
        }
        return (entity_state_flags_get(entity) & 0x2000000u) != 0u
            || (entity->flags_3ec & 0x40u) != 0u;
    }

    if (cell_four_neighbor_walk_check(ctx->level, x, y) == 0) {
        return 0;
    }

    if (action == 0xb) {
        if (selection_has_any(ctx) == 0) {
            return queue_callback_request_390(ctx, x, y);
        }
        if (active_entity_list_contains_code(ctx, 0) == 0
            && queue_action_request_flag100000(ctx) == 0) {
            return 0;
        }
        for (int i = 0; i < ctx->selection_count; ++i) {
            if (ctx->selection[i] != NULL
                && level_walkable(ctx->level, x, y) != 0) {
                return 1;
            }
        }
        return 0;
    }

    if (action == 0xd) {
        const uint8_t* voxel = level_voxel(ctx->level, x, y);
        if (voxel == NULL || voxel[2] == 1 || voxel[2] == 11
            || level_cell_walkable_flag(ctx->level, x, y) == 0
            || level_cell_flag_reserved(ctx->level, x, y) != 0u
            || queue_action_request_flag100000(ctx) == 0) {
            return 0;
        }
        if (queue_action_request_index(ctx, 8u) != 0u) {
            return 1;
        }
        return queue_action_request_pair(ctx, 2, 0u) != 0
            && queue_action_request_value(ctx, 0x20u) != 0;
    }

    if (level_cell_flag_20_bounded(ctx->level, x, y) != 0u
        || level_cell_mask_c0(ctx->level, x, y) != 0u
        || terrain_cell_allows_placement(NULL, ctx->level, x, y) == 0) {
        return 0;
    }
    if (selection_has_any(ctx) == 0) {
        return queue_callback_request_3e0(ctx, x, y);
    }
    if (active_entity_list_contains_code(ctx, 2) == 0
        && queue_action_request_flag100000(ctx) == 0) {
        return 0;
    }
    for (int i = 0; i < ctx->selection_count; ++i) {
        if (ctx->selection[i] != NULL
            && terrain_cell_allows_placement(ctx->selection[i], ctx->level,
                                             x, y) != 0) {
            return 1;
        }
    }
    return 0;
}

static int selected_map_coordinate_matches(const uint32_t coordinate[2])
{
    return coordinate != NULL
        && map_store_pair_word(0) == coordinate[0]
        && map_store_pair_word(1) == coordinate[1];
}

static void cancel_action_if_map_coordinate_matches(uint32_t mode,
                                                     uint32_t required_mode,
                                                     const uint32_t coordinate[2])
{
    if (mode == required_mode && selected_map_coordinate_matches(coordinate)) {
        selection_clear_request_and_hud_reset();
    }
}

static uint32_t* player_click_count_table(int kind)
{
    if (kind == 1) return s_player_kind1_click_counts;
    if (kind == 4) return s_player_kind4_click_counts;
    return NULL;
}

/* ---- Interface.c (interface_unit.c): adapters and stand-ins --------------
 * The thin unannotated adapters, with the original signatures, that
 * game/interface_callees.h declares (ADR 0005 step 6), and the stand-ins for
 * the legoGlobs fields and Input.c key maps that have no C-visible owner.
 * The native port does not reach interface_unit.c's functions (its callers
 * use the implementations above), so these only have to link and to forward
 * where a port entry exists. */

/* Declared only in frozen per-unit headers. */
int entity_type_pair_for_name(const char* name, int* type_out, int* row_out,
                              int* zero_out);
float engine_float_field260_get_current_target(void);
void timed_effect_persistence_set_corpus(int index, int set);
void timed_effect_timestamp_reset_corpus(int index);
namespace unported_ledger_stubs {
int ledger_stub_004401b0_TerrainCellToWorldPosition(EntitySlot* entity,
                                                    int force);
} // namespace unported_ledger_stubs

extern "C" {

LegoCamera* lego_camera_fp;
LegoObject* lego_object_fp;
char** lego_vehicle_names;
char** lego_building_names;
unsigned int lego_vehicle_type_count;
unsigned int lego_building_type_count;
LegoObject* lego_teleport_primary_small;
LegoObject* lego_teleport_primary_big;
LegoObject* lego_teleport_primary_laser;
LegoObjectRequests lego_object_requests;
unsigned int lego_object_request_count;
const char* lego_upgrade_cost_text;
unsigned int lego_upgrade_cost_ore;
unsigned int lego_upgrade_cost_studs;
const char* lego_custom_name_separator;
unsigned int lego_flags1;
unsigned int s_global_low_word_c;
char* lego_upgrade_level_names[16];
int object_tooltip_sfx[20 * 15];
void* lego_tooltip_ore_icon;
unsigned char input_key_map[256];
unsigned char input_prev_key_map[256];

void lego_ai_create_dig(const Point2I* blockPos, int arg2, int arg3)
{
    terrain_cell_command_create(blockPos->x, blockPos->y, arg2, arg3);
}

void lego_ai_create_dynamite(const Point2I* blockPos, int arg2)
{
    kind_nine_cell_command_issue(blockPos->x, blockPos->y, arg2);
}

void lego_ai_create_reinforce(const Point2I* blockPos)
{
    cell_command_type3_issue(blockPos->x, blockPos->y);
}

void lego_ai_create_dam(const Point2I* blockPos)
{
    cell_command_type11_issue(blockPos->x, blockPos->y);
}

int aitask_list_contains(int code)
{
    return active_entity_list_contains_code(entity_pool_callback_context_get(),
                                            code);
}

void lego_path_remove(const Point2I* blockPos)
{
    unported_ledger_stubs::ledger_stub_00409600_TerrainDescriptorTilesResetAndFree(
        &blockPos->x);
}

void dependencies_get_requirements(int objType, int objIndex, int level,
                                   InterfaceDependency** list,
                                   unsigned int* count)
{
    /* The port's Dependencies_GetRequirements returns the list through a
     * 32-bit word; the native port has no original-layout list. */
    (void)objType;
    (void)objIndex;
    (void)level;
    *list = NULL;
    *count = 0;
}

int lego_block_has_lava_neighbour(int x, int y)
{
    return terrain_cell_has_occupied_or_flagged_neighbor(
        level_current_owner_get(), x, y, NULL);
}

int lego_get_resource_count(int type)
{
    /* FUN_0041f8e0's port entry is a no-op ledger callback. */
    (void)type;
    return 0;
}

unsigned int lego_get_resource(int type)
{
    return upgrade_resource_check_current(type);
}

void lego_set_tracked_object(LegoObject* obj)
{
    (void)obj;
}

float lego_get_min_health(void)
{
    return engine_float_field260_get_current_target();
}

int lego_object_type_for_name(const char* name, int* objType, int* objIndex,
                              int* level)
{
    return entity_type_pair_for_name(name, objType, objIndex, level);
}

void lego_level_block_update(LegoLevel* level, int x, int y, int arg)
{
    /* FUN_0042f6f0's port entry has another parameter list. */
    (void)level;
    (void)x;
    (void)y;
    (void)arg;
}

struct TerrainMap* lego_get_map(void)
{
    /* The port's terrain is not the original map (finding 556). */
    return NULL;
}

int lego_object_can_enter_block(LegoObject* obj, int x, int y)
{
    return terrain_cell_allows_placement((const EntitySlot*)obj,
                                         level_current_owner_get(), x, y);
}

void lego_object_set_primary_teleporter(LegoObject* obj, int on)
{
    entity_flag80000000_set_or_clear(entity_pool_callback_context_get(),
                                     (EntitySlot*)obj, on);
}

int lego_object_can_eject(LegoObject* obj, int kind, int arg3, int arg4)
{
    (void)arg4;
    return entity_action_kind_supported((EntitySlot*)obj, kind, arg3);
}

int lego_object_pair_requirement_met(int objType, int objIndex,
                                     int otherType, int otherIndex)
{
    return entity_state_pair_predicate(entity_pool_callback_context_get(),
                                       objType, objIndex, otherType,
                                       otherIndex);
}

void* lego_object_get_container(LegoObject* obj)
{
    (void)obj;
    return NULL;
}

void lego_object_drop_carried(LegoObject* obj)
{
    unported_ledger_stubs::ledger_stub_0043fc20_RecordPairPoseCopyAndActivityRaise(
        (EntitySlot*)obj);
}

void lego_object_go_to_block(LegoObject* obj, int dig)
{
    unported_ledger_stubs::ledger_stub_004401b0_TerrainCellToWorldPosition(
        (EntitySlot*)obj, dig);
}

void lego_object_get_block(LegoObject* obj, int* x, int* y)
{
    entity_position_to_terrain_cell((const EntitySlot*)obj,
                                    level_current_owner_get(), x, y);
}

LegoObject* lego_object_find_trainer(LegoObject* obj, const Point2F* center,
                                     unsigned int mask)
{
    return (LegoObject*)unported_ledger_stubs::
        ledger_stub_00438e50_SpatialObjectQueryCenterMask100(
            (const EntitySlot*)obj, (const float*)center, mask);
}

void lego_object_detach_all(LegoObject* obj, int resetMode)
{
    unported_ledger_stubs::entity_attached_object_list_detach(
        (EntitySlot*)obj, resetMode);
}

void entity_kind6_tick_down_marker_spawn_obj(LegoObject* obj)
{
    entity_kind6_tick_down_marker_spawn(entity_pool_callback_context_get(),
                                        (EntitySlot*)obj);
}

void container_get_position(void* container, void* reference,
                            Point2F* position)
{
    (void)container;
    (void)reference;
    position->x = 0.0f;
    position->y = 0.0f;
}

void tooltip_set_persistent(int index, int set)
{
    timed_effect_persistence_set_corpus(index, set);
}

void tooltip_reset_timer(int index)
{
    timed_effect_timestamp_reset_corpus(index);
}

} /* extern "C" */
