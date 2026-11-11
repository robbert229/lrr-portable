// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-032 Panel.c: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-032.csv. */
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
#include "frontend_shared.hpp"
#include "frontend_for_helpwindow.hpp"
#include <float.h>
#include <errno.h>
#include "crt/crt_shared.hpp"
#include "mission.hpp"
#include "../gods98/frameloop.hpp"
#include "../platform/platform.h"
#include "gods98/scene_resources.hpp"
#include "game.hpp"
#include "ai.hpp"
#include "spawn.hpp"
#include "terrain_grid.hpp"
#include <stdint.h>
#include "tutorial_hud_shared.hpp"
#include "priorities.h"
#include <limits.h>
#include "record_shared.hpp"
#include "game/render.hpp"
#include "scene_pool.hpp"
#include "model.hpp"
#include "frontend_for_panel_unit.hpp"
#include "crt/crt_for_panel_unit.hpp"
#include "tutorial_hud_for_panel_unit.hpp"
#include "gods98/simtick_for_panel_unit.hpp"
#include "gods98/view_for_panel_unit.hpp"

#define MISS_CRYSTAL_QUOTA 26 /* crystal display budget (CrystalQuotaSetup) */

// GLOBAL: LEGORR 0x004a8378
static const char s_panel_button_crystal_crystals[] = "PanelButton_CrystalSideBar_Crystals";
// GLOBAL: LEGORR 0x004a839c
static const char s_panel_button_crystal_ore[] = "PanelButton_CrystalSideBar_Ore";
// GLOBAL: LEGORR 0x004a83bc
static const char s_panel_button_camera_rotate[] = "PanelButton_CameraControl_Rotate";
// GLOBAL: LEGORR 0x004a83e0
static const char s_panel_button_camera_cycle_buildings[] = "PanelButton_CameraControl_CycleBuildings";
// GLOBAL: LEGORR 0x004a840c
static const char s_panel_button_camera_zoom_out[] = "PanelButton_CameraControl_ZoomOut";
// GLOBAL: LEGORR 0x004a8430
static const char s_panel_button_camera_zoom_in[] = "PanelButton_CameraControl_ZoomIn";
// GLOBAL: LEGORR 0x004a8454
static const char s_panel_button_encyclopedia_close[] = "PanelButton_Encyclopedia_Close";
// GLOBAL: LEGORR 0x004a8474
static const char s_panel_button_priority_reset[] = "PanelButton_PriorityList_Reset";
// GLOBAL: LEGORR 0x004a8494
static const char s_panel_button_priority_close[] = "PanelButton_PriorityList_Close";
// GLOBAL: LEGORR 0x004a84b4
static const char s_panel_button_priority_up8[] = "PanelButton_PriorityList_UpOne8";
// GLOBAL: LEGORR 0x004a84d4
static const char s_panel_button_priority_up7[] = "PanelButton_PriorityList_UpOne7";
// GLOBAL: LEGORR 0x004a84f4
static const char s_panel_button_priority_up6[] = "PanelButton_PriorityList_UpOne6";
// GLOBAL: LEGORR 0x004a8514
static const char s_panel_button_priority_up5[] = "PanelButton_PriorityList_UpOne5";
// GLOBAL: LEGORR 0x004a8534
static const char s_panel_button_priority_up4[] = "PanelButton_PriorityList_UpOne4";
// GLOBAL: LEGORR 0x004a8554
static const char s_panel_button_priority_up3[] = "PanelButton_PriorityList_UpOne3";
// GLOBAL: LEGORR 0x004a8574
static const char s_panel_button_priority_up2[] = "PanelButton_PriorityList_UpOne2";
// GLOBAL: LEGORR 0x004a8594
static const char s_panel_button_priority_up1[] = "PanelButton_PriorityList_UpOne1";
// GLOBAL: LEGORR 0x004a85b4
static const char s_panel_button_priority_disable9[] = "PanelButton_PriorityList_Disable9";
// GLOBAL: LEGORR 0x004a85d8
static const char s_panel_button_priority_disable8[] = "PanelButton_PriorityList_Disable8";
// GLOBAL: LEGORR 0x004a85fc
static const char s_panel_button_priority_disable7[] = "PanelButton_PriorityList_Disable7";
// GLOBAL: LEGORR 0x004a8620
static const char s_panel_button_priority_disable6[] = "PanelButton_PriorityList_Disable6";
// GLOBAL: LEGORR 0x004a8644
static const char s_panel_button_priority_disable5[] = "PanelButton_PriorityList_Disable5";
// GLOBAL: LEGORR 0x004a8668
static const char s_panel_button_priority_disable4[] = "PanelButton_PriorityList_Disable4";
// GLOBAL: LEGORR 0x004a868c
static const char s_panel_button_priority_disable3[] = "PanelButton_PriorityList_Disable3";
// GLOBAL: LEGORR 0x004a86b0
static const char s_panel_button_priority_disable2[] = "PanelButton_PriorityList_Disable2";
// GLOBAL: LEGORR 0x004a86d4
static const char s_panel_button_priority_disable1[] = "PanelButton_PriorityList_Disable1";
// GLOBAL: LEGORR 0x004a86f8
static const char s_panel_button_information_function[] = "PanelButton_Information_Function";
// GLOBAL: LEGORR 0x004a871c
static const char s_panel_button_information_toggle[] = "PanelButton_Information_Toggle";
// GLOBAL: LEGORR 0x004a873c
static const char s_panel_button_top_call_to_arms[] = "PanelButton_TopPanel_CallToArms";
// GLOBAL: LEGORR 0x004a875c
static const char s_panel_button_top_priorities[] = "PanelButton_TopPanel_Priorities";
// GLOBAL: LEGORR 0x004a877c
static const char s_panel_button_top_options[] = "PanelButton_TopPanel_Options";
// GLOBAL: LEGORR 0x004a879c
static const char s_panel_button_info_dock_close[] = "PanelButton_InfoDock_Close";
// GLOBAL: LEGORR 0x004a87b8
static const char s_panel_button_info_dock_goto[] = "PanelButton_InfoDock_Goto";
// GLOBAL: LEGORR 0x004a87d4
static const char s_panel_button_radar_map_view[] = "PanelButton_Radar_MapView";
// GLOBAL: LEGORR 0x004a87f0
static const char s_panel_button_radar_zoom_out[] = "PanelButton_Radar_ZoomOut";
// GLOBAL: LEGORR 0x004a880c
static const char s_panel_button_radar_zoom_in[] = "PanelButton_Radar_ZoomIn";
// GLOBAL: LEGORR 0x004a8828
static const char s_panel_button_radar_tagged[] = "PanelButton_Radar_TaggedObjectView";
// GLOBAL: LEGORR 0x004a884c
static const char s_panel_button_radar_toggle[] = "PanelButton_Radar_Toggle";
// GLOBAL: LEGORR 0x004a8868
static const char s_panel_name_info_dock[] = "Panel_InfoDock";
// GLOBAL: LEGORR 0x004a8878
static const char s_panel_name_camera_control[] = "Panel_CameraControl";
// GLOBAL: LEGORR 0x004a888c
static const char s_panel_name_priority_list[] = "Panel_PriorityList";
// GLOBAL: LEGORR 0x004a88a0
static const char s_panel_name_information[] = "Panel_Information";
// GLOBAL: LEGORR 0x004a88b4
static const char s_panel_name_top_panel[] = "Panel_TopPanel";
// GLOBAL: LEGORR 0x004a88c4
static const char s_panel_name_crystal_side_bar[] = "Panel_CrystalSideBar";
// GLOBAL: LEGORR 0x004a88dc
static const char s_panel_name_messages_side[] = "Panel_MessagesSide";
// GLOBAL: LEGORR 0x004a88f0
static const char s_panel_name_messages[] = "Panel_Messages";
// GLOBAL: LEGORR 0x004a8900
static const char s_panel_name_radar_overlay[] = "Panel_RadarOverlay";
// GLOBAL: LEGORR 0x004a8914
static const char s_panel_name_radar_fill[] = "Panel_RadarFill";
/* UiPanelNameTablesInit publishes these exact pointer-table bands. The
 * button table is twelve 24-pointer rows and ends at DAT_005007d0. */
// GLOBAL: LEGORR 0x004a8924
static const char s_panel_name_radar[] = "Panel_Radar";

// GLOBAL: LEGORR 0x004a8930
static const char s_panel_name_encyclopedia[] = "Panel_Encyclopedia";

extern const char s_task_name_crystals[];   /* fix-up: reward.cpp uses it (mission_for_reward.hpp) */
/* TaskBlockInit's stack name list and format operand are fixed image data. */
// GLOBAL: LEGORR 0x004a8a08
const char s_task_name_crystals[] = "Crystals";

// GLOBAL: LEGORR 0x005000e0
const char* s_ui_panel_names[12];

// GLOBAL: LEGORR 0x00500110
RequestRecord s_request_records[REQUEST_RECORD_COUNT];

// GLOBAL: LEGORR 0x00500350
const char* s_ui_panel_indexed_names[12][24];
/* These words are also the selected category-cell coordinates. The table
 * initializer sets the no-selection sentinel used by SimCategoryCellHitTest. */
// GLOBAL: LEGORR 0x005007d0
static int s_category_cell_selected_x;
// GLOBAL: LEGORR 0x005007d4
static int s_category_cell_selected_y;

// GLOBAL: LEGORR 0x005007d8
void*    s_panel_ui_record;   /* DAT_005007d8 role */

// GLOBAL: LEGORR 0x005007e0
uint32_t s_advisor_bar_surfaces[3];

// GLOBAL: LEGORR 0x005007ec
int s_crystal_quota;

/* --- the panel layout band (2026-10-25; frontend.hpp) ----------------------
 * DAT_005007f0/7f4 handle slots, DAT_005007f8/7fc size floats,
 * DAT_00500800 extra word and DAT_00500804/808 offset floats have no
 * ported corpus owner — zero-init own storage (the unowned-global
 * deviation). The corpus surface loads (FUN_0047d9d0) stay hook boundaries
 * answering NULL (the panel-load precedent), and the blits ride the seam. */
// GLOBAL: LEGORR 0x005007f0
void* s_panel_layout_surface;
// GLOBAL: LEGORR 0x005007f4
void* s_panel_layout_glyph;

// GLOBAL: LEGORR 0x00500800
uint32_t s_panel_layout_extra;
// GLOBAL: LEGORR 0x00500804
float s_panel_layout_offset[2];

/* --- the remaining DAT one-liner stores/reads (see crt.hpp) ----------------- */
// GLOBAL: LEGORR 0x0050080c
uint32_t s_value_0050080c = 0u;

// GLOBAL: LEGORR 0x00500810
void*    s_panel_surface_handle;
// GLOBAL: LEGORR 0x00500814
float    s_panel_width;
// GLOBAL: LEGORR 0x00500818
float    s_panel_height;
// GLOBAL: LEGORR 0x0050081c
uint32_t s_panel_extra;

/* --- overlay animation float (2026-10-25) ---------------------------------
 * The DAT_00500820 animation float is reference state with no port owner,
 * so it lands as a guarded static (the unowned-global deviation) and the
 * corpus x87 float10 lands as plain float. */
// GLOBAL: LEGORR 0x00500820
float s_overlay_anim_float;

// GLOBAL: LEGORR 0x00500860
extern "C" uint32_t simtick_timer_word = 0;

namespace unported_ledger_stubs {
/* PairCountHistogramBuild rolls the last six pairs into a two-row history
 * before producing its local histogram. The original table is owned by the
 * simulation HUD; retain the same 2x6 window here rather than accepting an
 * invented caller-provided array. */
// GLOBAL: LEGORR 0x00500868
static uint32_t s_pair_count_history[2][6];
static uint32_t s_pair_count_histogram[2][6];
static uint32_t s_pair_count_dominant[2];
} // namespace unported_ledger_stubs

static int      s_timer_handles[4] = { 0, 0, 0, 0 }; /* DAT_00500850..5c */

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static uint32_t* panel_item_word(int panel, int item);
static void frontend_panel_rotation_image_load(BlockFile* values, const char* config_root, const char* key, int index);
static void* panel_load_seam_call(void* res);

/* UiPanelNameTablesInit (FUN_0045a030) stores twelve panel-name pointers,
 * thirty-eight button-name pointers at their original 24-slot row offsets,
 * then initializes the category-cell sentinel to (12, 24). */
// FUNCTION: LEGORR 0x0045a030
void ui_panel_name_tables_init(void)
{
    s_category_cell_selected_x = 0x0c;
    s_category_cell_selected_y = 0x18;
    /* Keep each string pointer at the exact panel/category/slot offset from
     * the reference tables. The target leaves all unlisted slots untouched. */
    s_ui_panel_names[11] = s_panel_name_encyclopedia;
    s_ui_panel_names[0] = s_panel_name_radar;
    s_ui_panel_names[1] = s_panel_name_radar_fill;
    s_ui_panel_names[2] = s_panel_name_radar_overlay;
    s_ui_panel_names[3] = s_panel_name_messages;
    s_ui_panel_names[4] = s_panel_name_messages_side;
    s_ui_panel_names[5] = s_panel_name_crystal_side_bar;
    s_ui_panel_names[6] = s_panel_name_top_panel;
    s_ui_panel_names[7] = s_panel_name_information;
    s_ui_panel_names[8] = s_panel_name_priority_list;
    s_ui_panel_names[9] = s_panel_name_camera_control;
    s_ui_panel_names[10] = s_panel_name_info_dock;

    s_ui_panel_indexed_names[0][0] = s_panel_button_radar_toggle;
    s_ui_panel_indexed_names[0][1] = s_panel_button_radar_tagged;
    s_ui_panel_indexed_names[0][2] = s_panel_button_radar_zoom_in;
    s_ui_panel_indexed_names[0][3] = s_panel_button_radar_zoom_out;
    s_ui_panel_indexed_names[0][4] = s_panel_button_radar_map_view;
    s_ui_panel_indexed_names[10][0] = s_panel_button_info_dock_goto;
    s_ui_panel_indexed_names[10][1] = s_panel_button_info_dock_close;
    s_ui_panel_indexed_names[6][0] = s_panel_button_top_options;
    s_ui_panel_indexed_names[6][1] = s_panel_button_top_priorities;
    s_ui_panel_indexed_names[6][2] = s_panel_button_top_call_to_arms;
    s_ui_panel_indexed_names[7][0] = s_panel_button_information_toggle;
    s_ui_panel_indexed_names[7][1] = s_panel_button_information_function;
    s_ui_panel_indexed_names[8][0] = s_panel_button_priority_disable1;
    s_ui_panel_indexed_names[8][1] = s_panel_button_priority_disable2;
    s_ui_panel_indexed_names[8][2] = s_panel_button_priority_disable3;
    s_ui_panel_indexed_names[8][3] = s_panel_button_priority_disable4;
    s_ui_panel_indexed_names[8][4] = s_panel_button_priority_disable5;
    s_ui_panel_indexed_names[8][5] = s_panel_button_priority_disable6;
    s_ui_panel_indexed_names[8][6] = s_panel_button_priority_disable7;
    s_ui_panel_indexed_names[8][7] = s_panel_button_priority_disable8;
    s_ui_panel_indexed_names[8][8] = s_panel_button_priority_disable9;
    s_ui_panel_indexed_names[8][9] = s_panel_button_priority_up1;
    s_ui_panel_indexed_names[8][10] = s_panel_button_priority_up2;
    s_ui_panel_indexed_names[8][11] = s_panel_button_priority_up3;
    s_ui_panel_indexed_names[8][12] = s_panel_button_priority_up4;
    s_ui_panel_indexed_names[8][13] = s_panel_button_priority_up5;
    s_ui_panel_indexed_names[8][14] = s_panel_button_priority_up6;
    s_ui_panel_indexed_names[8][15] = s_panel_button_priority_up7;
    s_ui_panel_indexed_names[8][16] = s_panel_button_priority_up8;
    s_ui_panel_indexed_names[8][17] = s_panel_button_priority_close;
    s_ui_panel_indexed_names[8][18] = s_panel_button_priority_reset;
    s_ui_panel_indexed_names[11][0] = s_panel_button_encyclopedia_close;
    s_ui_panel_indexed_names[9][0] = s_panel_button_camera_zoom_in;
    s_ui_panel_indexed_names[9][1] = s_panel_button_camera_zoom_out;
    s_ui_panel_indexed_names[9][2] = s_panel_button_camera_cycle_buildings;
    s_ui_panel_indexed_names[9][3] = s_panel_button_camera_rotate;
    s_ui_panel_indexed_names[5][0] = s_panel_button_crystal_ore;
    s_ui_panel_indexed_names[5][1] = s_panel_button_crystal_crystals;
}

// FUNCTION: LEGORR 0x0045a240
void legacy_scroll_panel_controls_initialize(void)
{
    /* The original initialises the scroll info panel, creates the
       configured overlay element, then calls FUN_0045abb0(6, 1, 1) and
       FUN_0045abb0(6, 2, 1) — the panel-item request-state setter. */
    ui_scroll_info_panel_initialize(s_legacy_hud_ops.overlay_init_sink);
    ui_overlay_create_configured_element(s_legacy_hud_ops.overlay_init_sink);
    panel_item_flag32_set(6, 1, 1);
    panel_item_flag32_set(6, 2, 1);
}

// FUNCTION: LEGORR 0x0045a270
void legacy_panel_category_interaction_state_reset(void)
{
    int i;
    /* The original walks the twelve records between DAT_00500110 and
       0x500350 (stride 0x30), restores the two view-offset dwords from
       their initial pair, forces the record flags to (f & ~0xa) | 4, and
       clears each selection entry's value word and its 0x48 flag bits.
       It does not touch the HUD state-flag word. */
    for (i = 0; i < 12; ++i) {
        uint32_t j;
        LegacyPanelPortRecord* record = &s_legacy_panels[i];
        record->current[0] = record->initial[0];
        record->current[1] = record->initial[1];
        record->flags = (record->flags & ~0xau) | 4u;
        for (j = 0; j < record->selection_count && j < 64; ++j) {
            record->selection_values[j] = 0;
            record->selection_flags[j] &= ~0x48u;
        }
    }
    overlay_anim_float_install(0.0f);
}

// FUNCTION: LEGORR 0x0045a2e0
void legacy_overlay_image_record_load(const char* path, int panel,
                                      uint32_t state)
{
    LegacyPanelPortRecord* record;
    if (panel < 0 || panel >= 18) return;
    record = &s_legacy_panels[panel];
    if (record->image != NULL) {
        if (record->image_texture_owned) {
            texture_release((TextureSurface*)record->image);
        }
        else if (s_legacy_hud_ops.surface_release != NULL) {
            s_legacy_hud_ops.surface_release(record->image,
                                             s_legacy_hud_ops.context);
        }
    }
    record->selection_count = 0;
    record->flags = state;
    request_record_flags_set(panel, state);
    record->image_mode = 0;
    record->image_flags = 0;
    record->image = NULL;
    record->image_texture_owned = 0;
    if (path != NULL) {
        if (s_legacy_hud_ops.panel_texture_load != NULL) {
            record->image = s_legacy_hud_ops.panel_texture_load(
                path, s_legacy_hud_ops.context);
            record->image_texture_owned = record->image != NULL;
        }
        else if (s_legacy_hud_ops.surface_load != NULL) {
            record->image = s_legacy_hud_ops.surface_load(
                NULL, path, 0, 0, s_legacy_hud_ops.context);
        }
        else {
            record->image = image_surface_load(path, 0u, 0u);
            record->image_texture_owned = record->image != NULL;
        }
        if (record->image_texture_owned)
            texture_source_color_key_set((TextureSurface*)record->image,
                                         0u, 0u);
    }
    if (record->image != NULL) {
        if (s_legacy_hud_ops.surface_initialize != NULL)
            s_legacy_hud_ops.surface_initialize(record->image,
                                                s_legacy_hud_ops.context);
        record->image_mode = 0;
        record->image_flags |= 1u;
        request_record_flags_set(panel, state | 1u);
        return;
    }
    if (path != NULL && s_legacy_hud_ops.surface_alternate_load != NULL) {
        record->image = s_legacy_hud_ops.surface_alternate_load(
            NULL, path, panel, 3, s_legacy_hud_ops.context);
        if (record->image != NULL) {
            record->image_texture_owned = 0;
            record->image_mode = 1;
            record->image_flags |= 1u;
            request_record_flags_set(panel, state | 1u);
        }
    }
}

/* UiPanelNameTableLookup (FUN_0045a370): the corpus do-while-walks the twelve
 * panel-name dwords at DAT_005000e0..DAT_00500110, __strcmpi-comparing each
 * against the caller's name; the first case-insensitive match writes the
 * zero-based slot and returns 1, the exhausted scan returns 0. Deviations:
 * the port guards null arguments (the corpus dereferences blindly) and the
 * null-name check inside the comparator stands in for the corpus's direct
 * __strcmpi against the installed pointer. */
// FUNCTION: LEGORR 0x0045a370
int ui_panel_name_table_lookup(const char* name, int* out_index)
{
    const char** slot;
    int index;
    if (name == NULL || out_index == NULL) return 0;
    slot = s_ui_panel_names;
    index = 0;
    do {
        if (ui_panel_string_equal_case_insensitive(name, *slot)) {
            *out_index = index;
            return 1;
        }
        ++slot;
        ++index;
    } while (slot < s_ui_panel_names + 12);
    return 0;
}

// FUNCTION: LEGORR 0x0045a3b0
int legacy_panel_slot_coordinate_hit_test(int panel, int x, int y)
{
    LegacyPanelPortRecord* record;
    int hit;
    /* Preserve the port record-array bound; the corpus directly indexes its
       panel record and has no corresponding range branch. */
    if (panel < 0 || panel >= 18) return 1;
    record = &s_legacy_panels[panel];
    if (record->image_mode != 0 || record->image == NULL ||
        s_legacy_hud_ops.surface_hit == NULL) return 1;
    hit = s_legacy_hud_ops.surface_hit(record->image,
        x - (int)record->current[0], y - (int)record->current[1],
        s_legacy_hud_ops.context);
    return hit ? panel == 0 : 1;
}

/* UiPanelIndexedNameLookup (FUN_0045a410): the corpus strides the category's
 * 24 name dwords at DAT_00500350 + table * 0x60, skipping null slots, and
 * __strcmpi-compares the rest; the first match writes the zero-based slot and
 * returns 1. Deviations: the port bounds the table index to the 64 installed
 * tables (the corpus strides the DAT band blindly) and null-guards its
 * arguments. */
// FUNCTION: LEGORR 0x0045a410
int ui_panel_indexed_name_lookup(unsigned int table_index, const char* name,
                                 unsigned int* out_index)
{
    const char** slot;
    unsigned int index;
    if (name == NULL || out_index == NULL || table_index >= 12u) return 0;
    slot = s_ui_panel_indexed_names[table_index];
    index = 0;
    do {
        if (*slot != NULL &&
            ui_panel_string_equal_case_insensitive(name, *slot)) {
            *out_index = index;
            return 1;
        }
        ++slot;
        ++index;
    } while (index < 24u);
    return 0;
}

namespace unported_ledger_stubs {
/* PanelImageSlotEntryAllocate (FUN_0045a500): allocate the four-word wrapper
 * first, then create its text buffer with the supplied font handle, header
 * rectangle, and capacity. The target record order is text buffer, base Y,
 * base X, and category-coordinate pointer. */
// FUNCTION: LEGORR 0x0045a500
void* ui_text_output_record_allocate(int panel, uintptr_t font_handle,
                                     const float rect[4],
                                     uint32_t buffer_size)
{
    UiTextOutputRecord32* record;
    uint32_t header[4] = { 0u, 0u, 0u, 0u };
    void* text_buffer;
    uint32_t text_token = 0u;
    uint32_t offsets_token = 0u;
    uintptr_t pointer_value;
    if (rect == NULL || panel < 0 || panel >= 18) return NULL;
    memcpy(&header[2], &rect[2], sizeof(rect[2]));
    memcpy(&header[3], &rect[3], sizeof(rect[3]));

    /* The retail order allocates the 16-byte wrapper before its text buffer. */
    record = (UiTextOutputRecord32*)engine_heap_alloc(
        (long)sizeof(UiTextOutputRecord32));
    if (record == NULL) return NULL;
    memset(record, 0, sizeof(*record));
    text_buffer = text_buffer_create(font_handle, header, (long)buffer_size);
    record->base_x = crt_float_to_long(rect[0]);
    record->base_y = crt_float_to_long(rect[1]);

#if defined(_M_IX86) || defined(__i386__)
    text_token = (uint32_t)(uintptr_t)text_buffer;
    pointer_value = (uintptr_t)&s_ui_text_output_panel_offsets[panel][0];
    offsets_token = (uint32_t)pointer_value;
#else
    /* A 32-bit target record cannot carry native pointers on this host;
     * the sidecar is authoritative, so leave the ABI token words empty. */
    (void)pointer_value;
#endif
    record->text_buffer_pointer = text_token;
    record->panel_coordinates_pointer = offsets_token;
    if (text_buffer != NULL &&
        !ui_text_output_binding_install(record, text_buffer,
            s_ui_text_output_panel_offsets[panel], 1, panel)) {
        text_buffer_destroy(text_buffer);
        engine_heap_free(record);
        return NULL;
    }
    return record;
}
// ScrollingAdvisorTextAppend
// FUNCTION: LEGORR 0x0045a590
void ledger_stub_0045a590_ScrollingAdvisorTextAppend(void* record,
                                                      const char* format, ...)
{
    va_list args;
    va_start(args, format);
    text_buffer_append_vformat(*(void**)record, 0, format, args);
    va_end(args);
}

// FUNCTION: LEGORR 0x0045a5b0
void ui_text_output_position_and_draw(void* record, uint32_t start_line,
                                     float line_scale)
{
    const UiTextOutputRecord32* target_record =
        (const UiTextOutputRecord32*)record;
    void* text_buffer = NULL;
    const float* offsets = NULL;
    int panel_index = -1;
    unsigned int binding_index;
    if (record == NULL) return;
    for (binding_index = 0; binding_index < 64; ++binding_index) {
        if (s_ui_text_output_bindings[binding_index].record == record) {
            text_buffer = s_ui_text_output_bindings[binding_index].text_buffer;
            offsets = s_ui_text_output_bindings[binding_index].offsets;
            panel_index = s_ui_text_output_bindings[binding_index].panel_index;
            break;
        }
    }
    if (panel_index >= 0 && panel_index < 18) {
        uint32_t y_bits, x_bits;
        indexed_pair_get(panel_index, &x_bits, &y_bits);
        memcpy(&s_ui_text_output_panel_offsets[panel_index][6], &y_bits,
               sizeof(y_bits));
        memcpy(&s_ui_text_output_panel_offsets[panel_index][7], &x_bits,
               sizeof(x_bits));
    }
#if defined(_M_IX86) || defined(__i386__)
    if (text_buffer == NULL)
        text_buffer = (void*)(uintptr_t)target_record->text_buffer_pointer;
    if (offsets == NULL)
        offsets = (const float*)(uintptr_t)
            target_record->panel_coordinates_pointer;
#else
    if (text_buffer == NULL || offsets == NULL) return;
#endif
    {
        int x, y;
    /* The retail path converts x (+0x1c) before y (+0x18), calling the CRT
     * floor helper before __ftol so coordinates round downward. */
        x = (int)crt_round_to_integral_with_math_error_handling(
            (double)((float)target_record->base_x + offsets[7]));
        y = (int)crt_round_to_integral_with_math_error_handling(
            (double)((float)target_record->base_y + offsets[6]));
        float_pair_at_offset4_set_from_ints(text_buffer, x, y);
    }
    (void)text_buffer_draw_wrapped(text_buffer, start_line, line_scale, NULL);
}
} // namespace unported_ledger_stubs

/* The mapped release body blindly dereferences the holder and calls the
 * unchecked TextBufferReset target. */
// FUNCTION: LEGORR 0x0045a620
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void ui_text_output_object_release_target(
    void* const* holder)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [eax]
        push ecx
        call text_buffer_reset_target
        add esp, 4
        ret
    }
}
#else
void ui_text_output_object_release_target(void* const* holder)
{
    text_buffer_reset_target(*holder);
}
#endif

/* UIRecordCallbackForward (FUN_0045a630) — see record.hpp. */
// FUNCTION: LEGORR 0x0045a630
void ui_record_callback_forward(const void* const* ui_record,
                                uint32_t* out_a, uint32_t* out_b)
{
    if (ui_record == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    void* handle = rec_ptr(ui_record, 0x00);
    object_state_pair_get(handle, out_a, out_b);
}

// FUNCTION: LEGORR 0x0045a650
void panel_category_bounds_set(int panel, int x1, int y1, int x2, int y2)
{
    RequestRecord* record = s_request_records + panel;
    uint8_t flags;
    record->bounds_x1 = (float)x1;
    record->bounds_x2 = (float)x2;
    flags = (uint8_t)record->flags;
    record->bounds_y1 = (float)y1;
    record->bounds_y2 = (float)y2;
    record->category_word = 0u;
    if ((flags & 2u) != 0u) {
        *(float*)&record->pair_x = (float)x1;
        *(float*)&record->pair_y = (float)y1;
        return;
    }
    *(float*)&record->pair_x = (float)x2;
    *(float*)&record->pair_y = (float)y2;
}

// FUNCTION: LEGORR 0x0045a6e0
void indexed_pair_set(int index, uint32_t x, uint32_t y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT) {
        return; /* deviation: the corpus strides blindly */
    }
    s_request_records[index].pair_x = x;
    s_request_records[index].pair_y = y;
}

// FUNCTION: LEGORR 0x0045a700
void indexed_pair_get(int index, uint32_t* out_x, uint32_t* out_y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT
        || out_x == NULL || out_y == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    *out_x = s_request_records[index].pair_x;
    *out_y = s_request_records[index].pair_y;
}

// FUNCTION: LEGORR 0x0045a730
void simtick_manager_tick(int index, float dt, int ingame)
{
    SimtickManagerSlot local;
    SimtickManagerSlot* slot;
    int authoritative = (s_mgr == NULL);
    uint32_t xbits, ybits;
    if (authoritative) {
        if (index < 0 || index >= 18) return;
        memset(&local, 0, sizeof(local));
        request_record_panel_bounds_peek(index, &local.target_a[0], &local.target_a[1], &local.target_b[0], &local.target_b[1]);
        indexed_pair_get(index, &xbits, &ybits);
        memcpy(&local.smoothed[0], &xbits, sizeof(xbits));
        memcpy(&local.smoothed[1], &ybits, sizeof(ybits));
        local.flags = request_record_flags_peek(index);
        slot = &local;
    } else {
        if (index < 0 || index >= s_mgr_count) return;
        slot = &s_mgr[index];
    }

    /* Both configured targets are float pairs; bit 4 selects the second. */
    float tx, ty;
    if ((slot->flags & 4u) != 0) {
        tx = slot->target_b[0];
        ty = slot->target_b[1];
    } else {
        tx = slot->target_a[0];
        ty = slot->target_a[1];
    }

    /* exponential smoothing (skip when dt equals the stored constant). */
    if (dt != g_manager_dt_const) {
        float denom = g_manager_smooth_a / dt - g_manager_smooth_b;
        if (fabsf(slot->smoothed[0] - tx) <= g_manager_snap) {
            slot->smoothed[0] = tx;
        } else {
            slot->smoothed[0] =
                ((g_manager_smooth_a / dt) * slot->smoothed[0] + tx) / denom;
        }
        if (fabsf(slot->smoothed[1] - ty) <= g_manager_snap) {
            slot->smoothed[1] = ty;
        } else {
            slot->smoothed[1] =
                ((g_manager_smooth_a / dt) * slot->smoothed[1] + ty) / denom;
        }
    }

    /* The corpus floors each coordinate before the integer conversion. */
    int ix = (int)floor((double)slot->smoothed[0]);
    int iy = (int)floor((double)slot->smoothed[1]);

    if (authoritative) {
        /* The fixture routing fields are not the panel owner. */
    } else if (slot->mode == 0) {
        ++s_repos_n;
        if (s_repos != NULL) {
            s_repos(slot->handle, ix, iy);
        }
    } else if (slot->mode == 1) {
        ++s_attach_n;
        if (s_attach != NULL) {
            s_attach(slot->handle, s_metrics[0], s_metrics[1], ingame);
        }
    }

    /* index 9: stop the four shared timers (DAT_00500860 bits 0x10..0x80). */
    if (index == 9) {
        for (int k = 0; k < 4; ++k) {
            if ((simtick_timer_word & s_timer_bits[k]) != 0) {
                simtick_timer_word &= ~s_timer_bits[k];
                /* FUN_0047dcb0(timer_handle, 0, &coord, 0) — see hook:
                 * a stop state is recorded through the reposition hook. */
                if (s_repos != NULL) {
                    s_repos(s_timer_handles[k], 0, 0);
                }
            }
        }
    }

    /* Retail truncates each difference from the selected target before
     * testing absolute distance <= 1, including nonzero screen positions. */
    if ((int)(slot->smoothed[0] - tx) < 2 && (int)(slot->smoothed[0] - tx) > -2
        && (int)(slot->smoothed[1] - ty) < 2 && (int)(slot->smoothed[1] - ty) > -2) {
        slot->flags &= ~8u;
    }
    if (authoritative) {
        memcpy(&xbits, &slot->smoothed[0], sizeof(xbits));
        memcpy(&ybits, &slot->smoothed[1], sizeof(ybits));
        indexed_pair_set(index, xbits, ybits);
        request_record_flags_set(index, slot->flags);
    }
}

/* --- the panel texture-slot blit (see frontend.hpp) --------------------------
 * The record's flag word is the float view's +0x28 dword; the texture
 * handles are the +0x10/+0x14/+0x18 float views carried bit-for-bit. A
 * missing draw-record block answers nothing (deviation: the corpus walks
 * the window unconditionally once its count word reads non-null). */
// FUNCTION: LEGORR 0x0045a9c0
void panel_texture_slots_blit_to(int panel, void* pixels, int pitch,
                                        int width, int height)
{
    const int count = panel_draw_count_peek(panel);
    if (count <= 0) {
        return; /* the corpus's count gate */
    }
    const char* block = (const char*)panel_draw_records_peek(panel);
    if (block == NULL) {
        return; /* deviation: the corpus strides a null block */
    }
    uint32_t oa = 0u, ob = 0u;
    indexed_pair_get(panel, &oa, &ob);
    const float ox = crt_float_bits(oa);
    const float oy = crt_float_bits(ob);

    for (int i = 0; i < count; ++i) {
        const float* rec = (const float*)(block + (size_t)i * 0x2c);
        uint32_t flags = 0u;
        __builtin_memcpy(&flags, rec + 10, sizeof(flags));
        if ((flags & 0x10u) != 0u) {
            continue; /* the corpus hides bit-0x10 items */
        }
        const int has40 = (flags & 0x40u) != 0u;
        int banner = 0;
        int field = 0;
        if ((flags & 8u) == 0u && (flags & 2u) != 0u) {
            if (!has40) {
                field = 4;
            } else {
                banner = (int)sim_status_banner_word_get();
                if (banner == 0) {
                    continue; /* the corpus skips the blit */
                }
                field = 4;
            }
        } else if ((flags & 4u) == 0u && (flags & 8u) == 0u) {
            if (!has40) {
                field = 6;
            } else {
                banner = (int)sim_status_banner_word_get();
                field = banner == 0 ? 6 : 4;
            }
        } else {
            if (has40) {
                banner = (int)sim_status_banner_word_get();
                if (banner != 0) {
                    field = 4;
                } else {
                    field = 5;
                }
            } else {
                field = 5;
            }
        }
        uint32_t handle = 0u;
        __builtin_memcpy(&handle, rec + field, sizeof(handle));
        if (handle != 0u) {
            float x = rec[0] + ox;
            float y = rec[1] + oy;
            TextureSurface* surface = panel_named_resource_get(
                panel, block, i, field, handle);
            crt_surface_blit_record(handle, x, y);
            if (surface != NULL && pixels != NULL)
                surface_blit_texture(pixels, pitch, width, height, surface,
                                     (int)crt_float_to_long(x),
                                     (int)crt_float_to_long(y));
        }
    }
}

// FUNCTION: LEGORR 0x0045aac0
void panel_item_flag16_set(int panel, int item, int value)
{
    uint32_t* w = panel_item_word(panel, item);
    if (w == NULL) {
        return;
    }
    *w = value != 0 ? *w | 0x10u : *w & ~0x10u;
}

// FUNCTION: LEGORR 0x0045ab00
void panel_state_flags_toggle(int index)
{
    if ((unsigned)index >= DAT_WINDOW_COUNT) {
        return;
    }
    const uint32_t f = s_request_records[index].flags;
    if ((f & 2u) != 0u) {
        s_request_records[index].flags = f & ~2u | 0xcu;
    } else {
        s_request_records[index].flags = f & ~4u | 0xau;
    }
}
#if defined(_MSC_VER) && defined(_M_IX86)
// FUNCTION: LEGORR 0x0045ab30
__declspec(naked) int request_record_ready_flag4_target(int index)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        lea eax, dword ptr [eax + eax*2]
        shl eax, 4
        add eax, offset s_request_records
        mov eax, dword ptr [eax + 0x2c]
        test al, 4
        je request_record_ready_flag4_false
        test al, 8
        jne request_record_ready_flag4_false
        mov eax, 1
        ret
    request_record_ready_flag4_false:
        xor eax, eax
        ret
        nop
        nop
        nop
        nop
    }
}
#else
int request_record_ready_flag4_target(int index)
{
    const RequestRecord* record = (const RequestRecord*)((const uint8_t*)
        s_request_records + (uint32_t)index * 0x30u);
    const uint32_t flags = record->flags;
    return (flags & 4u) != 0u && (flags & 8u) == 0u ? 1 : 0;
}
#endif

/* The corpus's 0x45ab6a/6f sequence adds the unchecked 0x30-byte record
 * address to DAT_00500110, then loads +0x2c; the 0x45ab72/78 tests keep bit
 * 2 then bit 8 in this short-circuit order. Keep the pointer step explicit
 * rather than folding the field offset into the global's displacement. */
#if defined(_MSC_VER) && defined(_M_IX86)
// FUNCTION: LEGORR 0x0045ab60
__declspec(naked) int request_record_ready_flag2_target(int index)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        lea eax, dword ptr [eax + eax*2]
        shl eax, 4
        add eax, offset s_request_records
        mov eax, dword ptr [eax + 0x2c]
        test al, 2
        je request_record_ready_flag2_false
        test al, 8
        jne request_record_ready_flag2_false
        mov eax, 1
        ret
    request_record_ready_flag2_false:
        xor eax, eax
        ret
        nop
        nop
        nop
        nop
    }
}
#else
int request_record_ready_flag2_target(int index)
{
    RequestRecord* record = (RequestRecord*)((uint8_t*)s_request_records +
        (uint32_t)index * 0x30u);
    const uint32_t f = record->flags;
    if ((f & 2u) != 0u && (f & 8u) == 0u) {
        return 1;
    }
    return 0;
}
#endif

#if defined(_MSC_VER) && defined(_M_IX86)
// FUNCTION: LEGORR 0x0045ab90
__declspec(naked) int request_record_flag8_is_set_target(int index)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        lea eax, dword ptr [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [eax + s_request_records + 0x2c]
        and eax, 8
        ret
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
    }
}
#else
int request_record_flag8_is_set_target(int index)
{
    RequestRecord* record = (RequestRecord*)((uint8_t*)s_request_records +
        (uint32_t)index * 0x30u);
    return (int)(record->flags & 8u);
}
#endif

// FUNCTION: LEGORR 0x0045abb0
void panel_item_flag32_set(int panel, int item, int value)
{
    uint32_t* w = panel_item_word(panel, item);
    if (w == NULL) {
        return;
    }
    *w = value != 0 ? *w | 0x20u : *w & ~0x20u;
}

/* PanelItemFlag8SetIfEnabled (FUN_0045abf0) — see crt.hpp. Bit 8 of the
 * stride-0x2c item's word at +0x28 only follows the flag while bit 32
 * (the enable gate) is set; otherwise the write is skipped. */
// FUNCTION: LEGORR 0x0045abf0
void panel_item_flag8_set_if_enabled(int panel, int item, int value)
{
    uint32_t* w = panel_item_word(panel, item);
    if (w == NULL) {
        return;
    }
    if ((*w & 0x20u) == 0u) {
        return;
    }
    *w = value != 0 ? *w | 8u : *w & ~8u;
}

/* PanelTextureSlotsAllocate (FUN_0045ac30) — see crt.hpp. Draw-record
 * fields: +0x00/+0x04 position, +0x08/+0x0c the size sums, +0x10/+0x14/
 * +0x18 the texture-slot handles, +0x20 the id word, +0x28 the flag word.
 * The corpus stores the new block over the panel's previous one without
 * freeing it (faithful); a null allocator answer skips the fill loop
 * (deviation: the corpus strides on through the failure). */
// FUNCTION: LEGORR 0x0045ac30
void panel_texture_slots_allocate(int panel, int count, const int* slot_index,
                                  const float* stream, const int* tex1,
                                  const int* tex2, const int* tex3,
                                  const int* ids)
{
    if (panel < 0 || panel >= DAT_WINDOW_COUNT || count <= 0) {
        return;
    }
    float* block = (float*)engine_heap_alloc((long)count * 0x2c);
    panel_draw_records_set(panel, block);
    panel_draw_count_set(panel, count);
    if (block == NULL) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        const int slot = (slot_index != NULL) ? slot_index[i] : i;
        if (slot < 0 || slot >= count) {
            continue; /* deviation: the corpus strides blindly */
        }
        float* rec = block + (size_t)slot * 11;
        const float* s = stream + (size_t)i * 4;
        rec[0] = s[0];
        rec[1] = s[1];
        rec[2] = s[2] + s[0];
        rec[3] = s[3] + s[1];
        rec[4] = 0.0f;
        rec[5] = 0.0f;
        rec[6] = 0.0f;
        rec[10] = 0.0f;
        if (ids != NULL) {
            rec[8] = crt_float_bits((uint32_t)ids[i]);
        }
        const int* tex[3] = { tex1, tex2, tex3 };
        const int field[3] = { 6, 4, 5 }; /* corpus slot order: +0x18,
                                            +0x10, +0x14 */
        for (int t = 0; t < 3; ++t) {
            if (tex[t] != NULL && tex[t][i] != 0) {
                const uint32_t h = crt_surface_handle_next();
                rec[field[t]] = crt_float_bits(h); /* load seam: a handle */
                crt_surface_blit_record(h, 0.0f, 0.0f); /* transform call */
            }
        }
    }
}

namespace unported_ledger_stubs {
// SimCursorInteractionTick
// FUNCTION: LEGORR 0x0045adb0
int ledger_stub_0045adb0_SimCursorInteractionTick(
    const SimCursorInteractionHooks* hooks, int x, int y,
    unsigned int action)
{
    return sim_cursor_interaction_tick(hooks, x, y, action);
}

// SimCategoryCellHitTest
// FUNCTION: LEGORR 0x0045b310
int ledger_stub_0045b310_SimCategoryCellHitTest(
    const SimCategoryHitEntry* entries, uint32_t count, int x, int y,
    int* category_out, int* item_out)
{
    return sim_category_cell_hit_test(entries, count, x, y,
                                      category_out, item_out);
}
} // namespace unported_ledger_stubs

/* GlobalUIFlagsClearBits12 (FUN_0045b610) — see simtick.hpp. */
// FUNCTION: LEGORR 0x0045b610
void simtick_global_ui_flags_clear_bits12(void)
{
    simtick_timer_word &= 0xfffffff9u;
}

// FUNCTION: LEGORR 0x0045b620
void ui_list_priority_button_action_dispatch(uint32_t action)
{
    switch (action) {
    case 0: priorities_toggle(0); return;
    case 1: priorities_toggle(1); return;
    case 2: priorities_toggle(2); return;
    case 3: priorities_toggle(3); return;
    case 4: priorities_toggle(4); return;
    case 5: priorities_toggle(5); return;
    case 6: priorities_toggle(6); return;
    case 7: priorities_toggle(7); return;
    case 8: priorities_toggle(8); return;
    case 9: priorities_move_up(1); return;
    case 10: priorities_move_up(2); return;
    case 11: priorities_move_up(3); return;
    case 12: priorities_move_up(4); return;
    case 13: priorities_move_up(5); return;
    case 14: priorities_move_up(6); return;
    case 15: priorities_move_up(7); return;
    case 16: priorities_move_up(8); return;
    default: return;
    }
}

// FUNCTION: LEGORR 0x0045b740
void ui_scroll_info_panel_initialize(const UiOverlayInitSink* sink)
{
    void* handle;
    if (sink == NULL) return;
    if (sink->panel_create != NULL)
        sink->panel_create(sink->context, "Interface_InfoPanel_scrollInfo_b", 0,
                           0, 0, 7, 0, 0);
    if (sink->element_add != NULL) {
        sink->element_add(sink->context, 0, 8, 0.0f, 0.0f, 18.0f, 11.0f, NULL);
        sink->element_add(sink->context, 0, 0, 0.0f, 11.0f, 18.0f, 11.0f, NULL);
        sink->element_add(sink->context, 0, 1, 0.0f, 0.0f, 0.0f, 0.0f,
                          "Interface_Buttons_scrollInfoSld_");
    }
    if (sink->element_create != NULL) {
        handle = sink->element_create(sink->context, 7, 0, 40.0f, 12.0f,
                                      180.0f, 43.0f, 0x100);
        if (sink->handle_store != NULL)
            sink->handle_store(sink->context, 0, handle);
        /* DAT_005007d8 is the text-output wrapper consumed by the panel
         * callback path; keep its portable owner in sync with the sink slot. */
        panel_ui_record_set(handle);
    }
}

// FUNCTION: LEGORR 0x0045b850
void ui_overlay_create_configured_element(const UiOverlayInitSink* sink)
{
    void* handle;
    if (sink == NULL || sink->element_create == NULL ||
        sink->handle_store == NULL) return;
    handle = sink->element_create(sink->context, 11, 0, 21.0f, 11.0f,
                                  221.0f, 227.0f, 0x400);
    sink->handle_store(sink->context, 1, handle);
}

/* PointDistanceWithinRadius (FUN_0045b8a0): true when the point
 * (x1,y1)-(x2,y2) distance is under `radius`. */
// FUNCTION: LEGORR 0x0045b8a0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int point_distance_within_radius(int x1, int y1, int x2,
                                                   int y2, int radius)
{
    __asm {
        sub esp, 8
        mov eax, dword ptr [esp + 0x14]
        mov edx, dword ptr [esp + 0x0c]
        mov ecx, dword ptr [esp + 0x18]
        sub eax, edx
        mov edx, dword ptr [esp + 0x10]
        mov dword ptr [esp + 0x14], eax
        fild dword ptr [esp + 0x14]
        fild dword ptr [esp + 0x1c]
        sub ecx, edx
        mov dword ptr [esp + 0x18], ecx
        fild dword ptr [esp + 0x18]
        fld st(2)
        fmul st, st(3)
        fxch st(1)
        fst dword ptr [esp + 4]
        fmul dword ptr [esp + 4]
        faddp st(1), st
        fsqrt
        fst dword ptr [esp + 0x14]
        fxch st(1)
        fcompp
        fnstsw ax
        test ah, 0x41
        fstp st(0)
        jne point_distance_outside_radius
        mov eax, 1
        add esp, 8
        ret

    point_distance_outside_radius:
        xor eax, eax
        add esp, 8
        ret
    }
}
#else
int point_distance_within_radius(int x1, int y1, int x2, int y2, int radius)
{
    const float dx = (float)(x2 - x1);
    const float dy = (float)(y2 - y1);
    return sqrtf(dx * dx + dy * dy) < (float)radius;
}
#endif

/* FUN_0045b900: the corpus casts the int pair and the divisor to float
 * before both divides (the results land in a caller float pair). */
// FUNCTION: LEGORR 0x0045b900
void integer_pair_divide_to_float_pair(int x, int y, int divisor,
                                       float* out_x, float* out_y)
{
    *out_x = (float)x / (float)divisor;
    *out_y = (float)y / (float)divisor;
}

/* FUN_0045b930's caller has the two source values in ST(0)/ST(1); its
 * arguments are only the output pointers. Keep that x87 ABI on the x86
 * reference backend, where each _ftol consumes the next stack value. */
#if defined(_MSC_VER) && defined(_M_IX86)
// FUNCTION: LEGORR 0x0045b930
__declspec(naked) void float_pair_to_integer_pair_x87(int* out_x, int* out_y)
{
    __asm {
        push esi
        push edi
        mov esi, dword ptr [esp + 0x0c]
        mov edi, dword ptr [esp + 0x10]
        call _ftol
        mov dword ptr [esi], eax
        call _ftol
        mov dword ptr [edi], eax
        pop edi
        pop esi
        ret
    }
}
#else
void float_pair_to_integer_pair_x87(int* out_x, int* out_y)
{
    /* Portable callers cannot supply the legacy x87 stack as an ABI input.
     * Callers on these targets use the explicit four-argument adapter below. */
    if (out_x != NULL) *out_x = 0;
    if (out_y != NULL) *out_y = 0;
}
#endif

// FUNCTION: LEGORR 0x0045b9d0
void frontend_panel_rotation_control_config_load(BlockFile* values,
                                                   const char* config_root)
{
    static const char* const image_keys[4] = {
        "LeftImage", "RightImage", "UpImage", "DownImage"
    };
    static const char* const numeric_keys[3] = {
        "CenterPositionX", "CenterPositionY", "Radius"
    };
    int* numeric_values[3];
    int i;
    numeric_values[0] = &s_panel_rotation_control.center_x;
    numeric_values[1] = &s_panel_rotation_control.center_y;
    numeric_values[2] = &s_panel_rotation_control.radius;
    for (i = 0; i < 3; ++i) {
        const char* const path = string_path_join(config_root,
            "PanelRotationControl", numeric_keys[i], (char*)NULL);
        const char* const value = values != NULL
            ? block_get_strptr(values, path) : NULL;
        *numeric_values[i] = value != NULL ? atoi(value) : 1;
    }
    for (i = 0; i < 4; ++i)
        frontend_panel_rotation_image_load(values, config_root,
                                            image_keys[i], i);
}

namespace unported_ledger_stubs {
// CameraDragFromCursorApply
// FUNCTION: LEGORR 0x0045bcd0
int ledger_stub_0045bcd0_CameraDragFromCursorApply(
    CameraDragState* state, const CameraDragHooks* hooks, int x, int y,
    float delta)
{
    return camera_drag_from_cursor_apply(state, hooks, x, y, delta);
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x0045bf20
void camera_zoom_step_adjust(int direction, float step)
{
    if (direction == 0) {
        camera_zoom_update(s_zoom_step_context, step * -3.0f);
        return; /* layout: the corpus's arm0 ends at its own ret (0x45bf44) */
    }
    if (direction == 1) {
        camera_zoom_update(s_zoom_step_context, step * 3.0f);
    }
    /* other directions: the corpus's early exit — inert */
}

/* UiEntryBoundsGet (FUN_0045bf70): read the entry's record out of the
 * panel's installed draw-record block (stride 0x2c) and answer the
 * position plus the derived size — the position dwords ride as-is, the
 * size is the far-corner minus the near one. Deviations: the panel window
 * and the null block are guarded (the corpus strides both). */
// FUNCTION: LEGORR 0x0045bf70
void ui_entry_bounds_get(int panel, int item, float* out_bounds)
{
    if (panel < 0 || panel >= PANEL_CATEGORY_WINDOWS || out_bounds == NULL
        || item < 0) {
        return; /* deviation: the corpus strides blindly */
    }
    const char* block = (const char*)panel_draw_records_peek(panel);
    if (block == NULL) {
        return; /* deviation: the corpus strides a null block */
    }
    const float* rec = (const float*)(block + (size_t)item * 0x2c);
    out_bounds[0] = rec[0];
    out_bounds[1] = rec[1];
    out_bounds[2] = rec[2] - rec[0];
    out_bounds[3] = rec[3] - rec[1];
}
// FUNCTION: LEGORR 0x0045bfb0
void three_surface_resources_load(const char* first, const char* second,
                                  const char* third)
{
    const char* paths[3] = { first, second, third };
    /* corpus store order: slot 0 (DAT_005007e0), slot 1 lands at the
     * THIRD peek index (DAT_005007e8), slot 2 at the second (DAT_005007e4)
     */
    uint32_t* const slots[3] = { &s_advisor_bar_surfaces[0],
                                 &s_advisor_bar_surfaces[2],
                                 &s_advisor_bar_surfaces[1] };
    for (int i = 0; i != 3; ++i) {
        const uint32_t handle = surface_load_handle(paths[i]);
        *slots[i] = handle;
        if (handle != 0u) {
            crt_surface_blit_record(handle, 0.0f, 0.0f); /* transform */
        }
    }
}

/* CrystalQuotaSetup @ 0x45c040: build Reward::Quota::Crystals, parse it,
 * and clamp zero or values above the display budget to 26. */
// FUNCTION: LEGORR 0x0045c040
void mission_crystal_quota_setup(BlockFile* cfg, const char* root,
                                        const char* level)
{
    const char* path = string_path_join(root, level, "Reward", "Quota",
                                        "Crystals", (const char*)0);
    const char* value = block_get_strptr(cfg, path);
    int quota;
    if (value == NULL) {
        value = "";
    } else {
        value = block_get_strptr(cfg, path);
    }
    quota = atoi(value != NULL ? value : "");
    s_crystal_quota = quota;
    if (s_crystal_quota > MISS_CRYSTAL_QUOTA || s_crystal_quota == 0) {
        s_crystal_quota = MISS_CRYSTAL_QUOTA;
    }
}

namespace unported_ledger_stubs {
// PairCountHistogramBuild
// FUNCTION: LEGORR 0x0045c0d0
void ledger_stub_0045c0d0_PairCountHistogramBuild(uint32_t first,
                                                   uint32_t second)
{
    int group;
    int column;
    for (group = 0; group != 2; ++group) {
        for (column = 0; column != 5; ++column)
            s_pair_count_history[group][column] =
                s_pair_count_history[group][column + 1];
    }
    s_pair_count_history[0][5] = first;
    s_pair_count_history[1][5] = second;
    pair_count_histogram_build(&s_pair_count_history[0][0],
                               s_pair_count_histogram,
                               s_pair_count_dominant);
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x0045c340
void panel_resource_layout_store(void* face_resource, uint32_t width,
                                 uint32_t height, uint32_t extra,
                                 void* glyph_resource, uint32_t position_x,
                                 uint32_t position_y)
{
    s_panel_layout_surface = panel_load_seam_call(face_resource);
    s_panel_layout_size[0] = (float)width;
    s_panel_layout_size[1] = (float)height;
    s_panel_layout_extra = extra;
    s_panel_layout_glyph = panel_load_seam_call(glyph_resource);
    if (s_panel_layout_glyph != NULL) {
        panel_blit_seam_call(s_panel_layout_glyph, 0.0f, 0.0f);
    }
    s_panel_layout_offset[0] = (float)position_x;
    s_panel_layout_offset[1] = (float)position_y;
}

// FUNCTION: LEGORR 0x0045c3f0
void advisor_progress_bar_fill_draw(int row, float percent)
{
    if (s_panel_layout_surface == NULL || !(0.0f < percent)) {
        return;
    }
    float pct = percent;
    if (100.0f < pct) {
        pct = 100.0f; /* the corpus constant */
    }
    int field8 = 0;
    __builtin_memcpy(&field8, (const char*)s_panel_layout_surface + 8,
                     sizeof field8);
    uint32_t pair_x = 0u, pair_y = 0u;
    indexed_pair_get(row, &pair_x, &pair_y);
    const float dest_x
        = crt_float_bits(pair_x) + panel_layout_size_peek(0);
    const float dest_y
        = crt_float_bits(pair_y) + panel_layout_size_peek(1);
    const float size[2] = {
        (float)((double)(float)panel_layout_extra_peek()
                * (double)(float)(1.0 / 100.0) * (double)pct),
        (float)field8
    };
    crt_surface_blit_record_rect(
        (uint32_t)(uintptr_t)s_panel_layout_surface, dest_x, dest_y, NULL,
        size);
}

// FUNCTION: LEGORR 0x0045c4a0
void global_value_0050080c_set(uint32_t value)
{
    s_value_0050080c = value;
}

// FUNCTION: LEGORR 0x0045c4b0
void panel_resource_position_update(int index)
{
    if (s_panel_layout_glyph == NULL) {
        return;
    }
    uint32_t x = 0, y = 0;
    indexed_pair_get(index, &x, &y);
    const float px = (float)(int32_t)x + s_panel_layout_offset[0];
    const float py = (float)(int32_t)y + s_panel_layout_offset[1];
    if (global_value_0050080c_peek() == 0u) {
        return;
    }
    if (sim_status_banner_word_get() != 0u) {
        panel_blit_seam_call(s_panel_layout_glyph, px, py);
    }
}
// FUNCTION: LEGORR 0x0045c520
void panel_resource_handle_and_dimensions_store(void* resource, uint32_t w,
                                                uint32_t h, uint32_t extra)
{
    /* corpus order: load, +0x14 width, +0x1c extra, +0x18 height */
    s_panel_surface_handle = panel_load_seam_call(resource);
    s_panel_width = (float)w;
    s_panel_extra = extra;
    s_panel_height = (float)h;
}

// FUNCTION: LEGORR 0x0045c580
void ui_overlay_animation_advance(int mode, int steps)
{
    /* Deviation: the corpus loops `param_2 != 0` down by one, so a
     * negative count runs away; the port treats it as no steps. */
    for (int i = steps > 0 ? steps : 0; i != 0; --i) {
        if (mode == 0) {
            s_overlay_anim_float =
                (s_overlay_anim_float - 0.05f) * 20.0f * 0.05263158f;
        } else {
            s_overlay_anim_float =
                s_overlay_anim_float - (1.0f - s_overlay_anim_float) * -0.05f;
        }
    }
    if (s_overlay_anim_float < 0.0f) {
        s_overlay_anim_float = 0.0f; /* the after-loop negative guard */
    }
}

namespace unported_ledger_stubs {
// UiOverlayAnimationApply
// FUNCTION: LEGORR 0x0045c5f0
void ledger_stub_0045c5f0_UiOverlayAnimationApply(
    const UiOverlayAnimationState* state,
    void (*draw)(void*, void*, int, const float*, const float*),
    void* context)
{
    ui_overlay_animation_apply(state, draw, context);
}

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

/* AdvisorAnimationSlotIdResolve (FUN_0045c6b0): only kinds 0, 6, 9, and 10
 * have cases; each accepts its own position-index range and maps to a fixed
 * advisor animation slot. */
// FUNCTION: LEGORR 0x0045C6B0
int advisor_animation_slot_id_resolve(uint32_t animation_kind,
                                      uint32_t position_index,
                                      uint32_t* slot_id)
{
    if (slot_id == NULL) return 0;
    *slot_id = 0x15u;
    switch (animation_kind) {
    case 0:
        if (position_index <= 4u) *slot_id = 6u + position_index;
        break;
    case 6:
        if (position_index <= 2u) *slot_id = 0x0eu + position_index;
        break;
    case 9:
        if (position_index <= 1u) *slot_id = 0x12u + position_index;
        break;
    case 10:
        if (position_index <= 1u) *slot_id = 0x0bu + position_index;
        break;
    }
    return *slot_id != 0x15u;
}

static void* panel_load_seam_call(void* res)
{
    panel_blit_seam_call(res, 0.0f, 0.0f);
    return NULL; /* the port has no surface layer */
}

static void frontend_panel_rotation_image_load(BlockFile* values,
                                                const char* config_root,
                                                const char* key, int index)
{
    const char* const path = string_path_join(config_root,
        "PanelRotationControl", key, (char*)NULL);
    const char* value = values != NULL ? block_get_strptr(values, path) : NULL;
    TextureSurface* old_surface = s_panel_rotation_control.images[index];
    if (value == NULL) {
        s_panel_rotation_control.images[index] = NULL;
        if (old_surface != NULL) texture_release(old_surface);
        return;
    }

    char storage[FE_PATH_MAX * 2];
    char* fields[3];
    TextureSurface* surface = NULL;
    if (frontend_config_fields(value, storage, sizeof(storage), fields, 3,
                               ',')) {
        surface = image_surface_load(fields[0], 0u, 0u);
        if (surface != NULL) {
            s_panel_rotation_control.image_sizes[index][0] =
                (float)atof(fields[1]);
            s_panel_rotation_control.image_sizes[index][1] =
                (float)atof(fields[2]);
        }
    }
    s_panel_rotation_control.images[index] = surface;
    if (old_surface != NULL) texture_release(old_surface);
}

static uint32_t* panel_item_word(int panel, int item)
{
    if (panel < 0 || panel >= DAT_WINDOW_COUNT) {
        return NULL;
    }
    char* block = (char*)s_panel_records[panel];
    if (block == NULL || item < 0) {
        return NULL; /* deviation: the corpus strides blindly */
    }
    return (uint32_t*)(block + 0x28 + (size_t)item * 0x2c);
}
