// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>

#include "ui_list.hpp"

static float s_pointer_cursor_override_timer;
// GLOBAL: LEGORR 0x00500e1c
static const char* s_pointer_cursor_names[56];

// GLOBAL: LEGORR 0x004a8ed4
static const char s_pointer_name_standard[] = "Pointer_Standard";
// GLOBAL: LEGORR 0x004a8ec4
static const char s_pointer_name_blank[] = "Pointer_Blank";
// GLOBAL: LEGORR 0x004a8eb0
static const char s_pointer_name_selected[] = "Pointer_Selected";
// GLOBAL: LEGORR 0x004a8ea0
static const char s_pointer_name_drill[] = "Pointer_Drill";
// GLOBAL: LEGORR 0x004a8e8c
static const char s_pointer_name_cant_drill[] = "Pointer_CantDrill";
// GLOBAL: LEGORR 0x004a8e7c
static const char s_pointer_name_clear[] = "Pointer_Clear";
// GLOBAL: LEGORR 0x004a8e70
static const char s_pointer_name_go[] = "Pointer_Go";
// GLOBAL: LEGORR 0x004a8e60
static const char s_pointer_name_cant_go[] = "Pointer_CantGo";
// GLOBAL: LEGORR 0x004a8e4c
static const char s_pointer_name_teleport[] = "Pointer_Teleport";
// GLOBAL: LEGORR 0x004a8e34
static const char s_pointer_name_cant_teleport[] = "Pointer_CantTeleport";
// GLOBAL: LEGORR 0x004a8e20
static const char s_pointer_name_reinforce[] = "Pointer_Reinforce";
// GLOBAL: LEGORR 0x004a8e08
static const char s_pointer_name_cant_reinforce[] = "Pointer_CantReinforce";
// GLOBAL: LEGORR 0x004a8df4
static const char s_pointer_name_radar_pan[] = "Pointer_RadarPan";
// GLOBAL: LEGORR 0x004a8de0
static const char s_pointer_name_track_object[] = "Pointer_TrackObject";
// GLOBAL: LEGORR 0x004a8dcc
static const char s_pointer_name_cant_zoom[] = "Pointer_CantZoom";
// GLOBAL: LEGORR 0x004a8dbc
static const char s_pointer_name_zoom[] = "Pointer_Zoom";
// GLOBAL: LEGORR 0x004a8dac
static const char s_pointer_name_help[] = "Pointer_Help";
// GLOBAL: LEGORR 0x004a8d98
static const char s_pointer_name_cant_help[] = "Pointer_CantHelp";
// GLOBAL: LEGORR 0x004a8d88
static const char s_pointer_name_put_down[] = "Pointer_PutDown";
// GLOBAL: LEGORR 0x004a8d78
static const char s_pointer_name_get_in[] = "Pointer_GetIn";
// GLOBAL: LEGORR 0x004a8d68
static const char s_pointer_name_get_out[] = "Pointer_GetOut";
// GLOBAL: LEGORR 0x004a8d4c
static const char s_pointer_name_tutorial_block_info[] = "Pointer_TutorialBlockInfo";
// GLOBAL: LEGORR 0x004a8d3c
static const char s_pointer_name_okay[] = "Pointer_Okay";
// GLOBAL: LEGORR 0x004a8d2c
static const char s_pointer_name_not_okay[] = "Pointer_NotOkay";
// GLOBAL: LEGORR 0x004a8d18
static const char s_pointer_name_can_build[] = "Pointer_CanBuild";
// GLOBAL: LEGORR 0x004a8d04
static const char s_pointer_name_cannot_build[] = "Pointer_CannotBuild";
// GLOBAL: LEGORR 0x004a8cf0
static const char s_pointer_name_dynamite[] = "Pointer_Dynamite";
// GLOBAL: LEGORR 0x004a8cd8
static const char s_pointer_name_cant_dynamite[] = "Pointer_CantDynamite";
// GLOBAL: LEGORR 0x004a8cc8
static const char s_pointer_name_pick_up[] = "Pointer_PickUp";
// GLOBAL: LEGORR 0x004a8cb4
static const char s_pointer_name_cant_pick_up[] = "Pointer_CantPickUp";
// GLOBAL: LEGORR 0x004a8ca0
static const char s_pointer_name_pick_up_ore[] = "Pointer_PickUpOre";
// GLOBAL: LEGORR 0x004a8c88
static const char s_pointer_name_lego_man_cant_dig[] = "Pointer_LegoManCantDig";
// GLOBAL: LEGORR 0x004a8c70
static const char s_pointer_name_vehicle_cant_dig[] = "Pointer_VehicleCantDig";
// GLOBAL: LEGORR 0x004a8c5c
static const char s_pointer_name_lego_man_dig[] = "Pointer_LegoManDig";
// GLOBAL: LEGORR 0x004a8c48
static const char s_pointer_name_vehicle_dig[] = "Pointer_VehicleDig";
// GLOBAL: LEGORR 0x004a8c2c
static const char s_pointer_name_lego_man_cant_pick_up[] = "Pointer_LegoManCantPickUp";
// GLOBAL: LEGORR 0x004a8c10
static const char s_pointer_name_vehicle_cant_pick_up[] = "Pointer_VehicleCantPickUp";
// GLOBAL: LEGORR 0x004a8bf8
static const char s_pointer_name_lego_man_pick_up[] = "Pointer_LegoManPickUp";
// GLOBAL: LEGORR 0x004a8be0
static const char s_pointer_name_vehicle_pick_up[] = "Pointer_VehiclePickUp";
// GLOBAL: LEGORR 0x004a8bc8
static const char s_pointer_name_lego_man_cant_go[] = "Pointer_LegoManCantGo";
// GLOBAL: LEGORR 0x004a8bb0
static const char s_pointer_name_vehicle_cant_go[] = "Pointer_VehicleCantGo";
// GLOBAL: LEGORR 0x004a8b9c
static const char s_pointer_name_lego_man_go[] = "Pointer_LegoManGo";
// GLOBAL: LEGORR 0x004a8b88
static const char s_pointer_name_vehicle_go[] = "Pointer_VehicleGo";
// GLOBAL: LEGORR 0x004a8b70
static const char s_pointer_name_lego_man_clear[] = "Pointer_LegoManClear";
// GLOBAL: LEGORR 0x004a8b58
static const char s_pointer_name_vehicle_clear[] = "Pointer_VehicleClear";
// GLOBAL: LEGORR 0x004a8b38
static const char s_pointer_name_surface_immovable[] = "Pointer_SurfaceType_Immovable";
// GLOBAL: LEGORR 0x004a8b1c
static const char s_pointer_name_surface_hard[] = "Pointer_SurfaceType_Hard";
// GLOBAL: LEGORR 0x004a8b00
static const char s_pointer_name_surface_medium[] = "Pointer_SurfaceType_Medium";
// GLOBAL: LEGORR 0x004a8ae4
static const char s_pointer_name_surface_loose[] = "Pointer_SurfaceType_Loose";
// GLOBAL: LEGORR 0x004a8ac8
static const char s_pointer_name_surface_soil[] = "Pointer_SurfaceType_Soil";
// GLOBAL: LEGORR 0x004a8aac
static const char s_pointer_name_surface_lava[] = "Pointer_SurfaceType_Lava";
// GLOBAL: LEGORR 0x004a8a90
static const char s_pointer_name_surface_water[] = "Pointer_SurfaceType_Water";
// GLOBAL: LEGORR 0x004a8a74
static const char s_pointer_name_surface_ore_seam[] = "Pointer_SurfaceType_OreSeam";
// GLOBAL: LEGORR 0x004a8a58
static const char s_pointer_name_surface_lake[] = "Pointer_SurfaceType_Lake";
// GLOBAL: LEGORR 0x004a8a38
static const char s_pointer_name_surface_crystal_seam[] = "Pointer_SurfaceType_CrystalSeam";
// GLOBAL: LEGORR 0x004a8a14
static const char s_pointer_name_surface_recharge_seam[] = "Pointer_SurfaceType_RechargeSeam";

void ui_list_state_clear(UiListState* state)
{
    if (state != NULL) memset(state, 0, sizeof(*state));
}

// FUNCTION: LEGORR 0x0045d3c0
void ui_list_priority_panel_row_move_dispatch(UiListState* state,
                                              uint32_t index)
{
    if (state == NULL) return;
    if ((state->priority_panel_mode & 2u) != 0) {
        ui_list_priority_order_move_to_front(state, index);
        return;
    }
    if ((state->priority_panel_mode & 4u) != 0)
        ui_list_entries_swap(state, index);
}

// FUNCTION: LEGORR 0x0045d3f0
void ui_list_priority_order_move_to_front(UiListState* state,
                                          uint32_t index)
{
    void* entry;
    uint32_t i;
    if (state != NULL && index != 0 && index < state->count &&
        state->count <= UI_LIST_CAPACITY) {
        entry = state->entries[0][index];
        for (i = index; i > 0; --i) {
            state->entries[0][i] = state->entries[0][i - 1];
            if (state->entry_position_set != NULL)
                state->entry_position_set(state->entries[0][i], 80 + 5 * (int)i);
        }
        state->entries[0][0] = entry;
        if (state->entry_position_set != NULL)
            state->entry_position_set(entry, 80);
    }
    if (state != NULL && state->state_changed != NULL) state->state_changed(3);
}

// FUNCTION: LEGORR 0x0045d470
void ui_list_entries_swap(UiListState* state, uint32_t index)
{
    void* entry;
    if (state != NULL && index != 0 && index < state->count &&
        state->count <= UI_LIST_CAPACITY) {
        entry = state->entries[0][index];
        state->entries[0][index] = state->entries[1][index];
        state->entries[1][index] = entry;
        if (state->entry_position_set != NULL) {
            state->entry_position_set(state->entries[0][index],
                                      80 - 5 * (int)index);
            state->entry_position_set(state->entries[1][index],
                                      85 - 5 * (int)index);
        }
        if (state->cursor_move != NULL) state->cursor_move(index - 1);
    }
    if (state != NULL && state->state_changed != NULL) state->state_changed(0);
}

// FUNCTION: LEGORR 0x0045d4f0
void ui_list_config_entry_enabled_toggle(UiListState* state,
                                         uint32_t index)
{
    void* entry;
    int was_enabled;
    if (state == NULL || index >= state->count ||
        index >= UI_LIST_CAPACITY || state->entries[0][index] == NULL ||
        state->enabled_get == NULL || state->enabled_set == NULL) return;
    entry = state->entries[0][index];
    was_enabled = state->enabled_get(entry);
    state->enabled_set(entry, was_enabled == 0);
    if (was_enabled == 0 && (entry == state->entries[0][0] ||
                             entry == state->entries[0][1]) &&
        state->play_feedback != NULL) {
        state->play_feedback(0x10);
    }
}

// FUNCTION: LEGORR 0x0045b620
void ui_list_priority_button_action_dispatch(UiListState* state,
                                             uint32_t action)
{
    if (state == NULL) return;
    switch (action) {
    case 0: ui_list_config_entry_enabled_toggle(state, 0); return;
    case 1: ui_list_config_entry_enabled_toggle(state, 1); return;
    case 2: ui_list_config_entry_enabled_toggle(state, 2); return;
    case 3: ui_list_config_entry_enabled_toggle(state, 3); return;
    case 4: ui_list_config_entry_enabled_toggle(state, 4); return;
    case 5: ui_list_config_entry_enabled_toggle(state, 5); return;
    case 6: ui_list_config_entry_enabled_toggle(state, 6); return;
    case 7: ui_list_config_entry_enabled_toggle(state, 7); return;
    case 8: ui_list_config_entry_enabled_toggle(state, 8); return;
    case 9: ui_list_entries_swap(state, 1); return;
    case 10: ui_list_entries_swap(state, 2); return;
    case 11: ui_list_entries_swap(state, 3); return;
    case 12: ui_list_entries_swap(state, 4); return;
    case 13: ui_list_entries_swap(state, 5); return;
    case 14: ui_list_entries_swap(state, 6); return;
    case 15: ui_list_entries_swap(state, 7); return;
    case 16: ui_list_entries_swap(state, 8); return;
    default: return;
    }
}


// FUNCTION: LEGORR 0x0045c830
void ui_pointer_cursor_name_registry_init(void)
{
    s_pointer_cursor_names[0] = s_pointer_name_standard;
    s_pointer_cursor_names[1] = s_pointer_name_blank;
    s_pointer_cursor_names[2] = s_pointer_name_selected;
    s_pointer_cursor_names[3] = s_pointer_name_drill;
    s_pointer_cursor_names[4] = s_pointer_name_cant_drill;
    s_pointer_cursor_names[5] = s_pointer_name_clear;
    s_pointer_cursor_names[6] = s_pointer_name_go;
    s_pointer_cursor_names[7] = s_pointer_name_cant_go;
    s_pointer_cursor_names[8] = s_pointer_name_teleport;
    s_pointer_cursor_names[9] = s_pointer_name_cant_teleport;
    s_pointer_cursor_names[10] = s_pointer_name_reinforce;
    s_pointer_cursor_names[11] = s_pointer_name_cant_reinforce;
    s_pointer_cursor_names[12] = s_pointer_name_radar_pan;
    s_pointer_cursor_names[13] = s_pointer_name_track_object;
    s_pointer_cursor_names[54] = s_pointer_name_cant_zoom;
    s_pointer_cursor_names[55] = s_pointer_name_zoom;
    s_pointer_cursor_names[14] = s_pointer_name_help;
    s_pointer_cursor_names[15] = s_pointer_name_cant_help;
    s_pointer_cursor_names[16] = s_pointer_name_put_down;
    s_pointer_cursor_names[17] = s_pointer_name_get_in;
    s_pointer_cursor_names[18] = s_pointer_name_get_out;
    s_pointer_cursor_names[19] = s_pointer_name_tutorial_block_info;
    s_pointer_cursor_names[20] = s_pointer_name_okay;
    s_pointer_cursor_names[21] = s_pointer_name_not_okay;
    s_pointer_cursor_names[22] = s_pointer_name_can_build;
    s_pointer_cursor_names[23] = s_pointer_name_cannot_build;
    s_pointer_cursor_names[24] = s_pointer_name_dynamite;
    s_pointer_cursor_names[25] = s_pointer_name_cant_dynamite;
    s_pointer_cursor_names[26] = s_pointer_name_pick_up;
    s_pointer_cursor_names[27] = s_pointer_name_cant_pick_up;
    s_pointer_cursor_names[28] = s_pointer_name_pick_up_ore;
    s_pointer_cursor_names[29] = s_pointer_name_lego_man_cant_dig;
    s_pointer_cursor_names[30] = s_pointer_name_vehicle_cant_dig;
    s_pointer_cursor_names[31] = s_pointer_name_lego_man_dig;
    s_pointer_cursor_names[32] = s_pointer_name_vehicle_dig;
    s_pointer_cursor_names[33] = s_pointer_name_lego_man_cant_pick_up;
    s_pointer_cursor_names[34] = s_pointer_name_vehicle_cant_pick_up;
    s_pointer_cursor_names[35] = s_pointer_name_lego_man_pick_up;
    s_pointer_cursor_names[36] = s_pointer_name_vehicle_pick_up;
    s_pointer_cursor_names[37] = s_pointer_name_lego_man_cant_go;
    s_pointer_cursor_names[38] = s_pointer_name_vehicle_cant_go;
    s_pointer_cursor_names[39] = s_pointer_name_lego_man_go;
    s_pointer_cursor_names[40] = s_pointer_name_vehicle_go;
    s_pointer_cursor_names[41] = s_pointer_name_lego_man_clear;
    s_pointer_cursor_names[42] = s_pointer_name_vehicle_clear;
    s_pointer_cursor_names[43] = s_pointer_name_surface_immovable;
    s_pointer_cursor_names[44] = s_pointer_name_surface_hard;
    s_pointer_cursor_names[45] = s_pointer_name_surface_medium;
    s_pointer_cursor_names[46] = s_pointer_name_surface_loose;
    s_pointer_cursor_names[47] = s_pointer_name_surface_soil;
    s_pointer_cursor_names[48] = s_pointer_name_surface_lava;
    s_pointer_cursor_names[49] = s_pointer_name_surface_water;
    s_pointer_cursor_names[50] = s_pointer_name_surface_ore_seam;
    s_pointer_cursor_names[51] = s_pointer_name_surface_lake;
    s_pointer_cursor_names[52] = s_pointer_name_surface_crystal_seam;
    s_pointer_cursor_names[53] = s_pointer_name_surface_recharge_seam;
}

const char* ui_pointer_cursor_name_get(uint32_t index)
{
    return index < 56 ? s_pointer_cursor_names[index] : NULL;
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
    if (sink->element_create != NULL && sink->handle_store != NULL) {
        handle = sink->element_create(sink->context, 7, 0, 40.0f, 12.0f,
                                      180.0f, 43.0f, 0x100);
        sink->handle_store(sink->context, 0, handle);
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

uint32_t ui_info_panel_string_entries_initialize(
    UiInfoPanelStringTable* table, const UiInfoPanelStringInit* entries,
    uint32_t count, void* context, UiInfoPanelStringResolveHook resolve)
{
    uint32_t i;
    uint32_t initialized = 0;
    if (table == NULL || (count != 0u && entries == NULL)) return 0u;
    if (count > UI_INFO_PANEL_STRING_CAPACITY)
        count = UI_INFO_PANEL_STRING_CAPACITY;
    for (i = 0; i < count; ++i) {
        UiInfoPanelStringEntry* destination;
        if (entries[i].slot >= UI_INFO_PANEL_STRING_CAPACITY) continue;
        destination = &table->entries[entries[i].slot];
        destination->flags = (entries[i].mode != 0u) ? 1u : 9u;
        destination->text_handle = resolve == NULL ? NULL
            : resolve(context, entries[i].text_id);
        if (destination->text_handle != NULL) destination->flags |= 4u;
        destination->value = entries[i].value;
        ++initialized;
    }
    return initialized;
}

static int ui_track_point_in_rect(const float rect[4], int x, int y)
{
    return (float)x >= rect[0] && (float)y >= rect[1] &&
           (float)x < rect[2] && (float)y < rect[3];
}

int ui_sim_track_cursor_hit_test(UiTrackCursorState* state, int x, int y,
                                 int mode, int allow_mode_change,
                                 const UiTrackCursorHitHooks* hooks,
                                 uint32_t* slot_index_out)
{
    uint32_t slot;
    int handled = 0;
    if (state == NULL || state->slots == NULL) return 0;
    for (slot = 0; slot < state->slot_count; ++slot) {
        UiTrackCursorSlot* current = &state->slots[slot];
        uint32_t side;
        int visible = current->enabled != 0u &&
            (hooks == NULL || hooks->track_visible == NULL ||
             hooks->track_visible(hooks->context, slot));
        if (!visible) continue;
        for (side = 0; side < 2u; ++side) {
            if (!handled && ui_track_point_in_rect(current->button_rects[side],
                                                   x, y)) {
                if (slot_index_out != NULL) *slot_index_out = current->index;
                if (mode == 0) {
                    if (state->mode == 3u || side == state->mode) {
                        if (state->mode != 3u && current->selected_index != NULL) {
                            if (side == 0u) {
                                if (*current->selected_index < current->selected_count)
                                    ++*current->selected_index;
                            }
                            else if (*current->selected_index != 0u) {
                                --*current->selected_index;
                            }
                            current->selection_changed = 1u;
                        }
                        current->button_flags[side] =
                            (current->button_flags[side] & ~4u) | 2u;
                    }
                }
                else if (state->mode == 3u) {
                    if (allow_mode_change) {
                        state->mode = side;
                        current->button_flags[side] =
                            (current->button_flags[side] & ~2u) | 4u;
                    }
                }
                else if (side == state->mode) {
                    current->button_flags[side] =
                        (current->button_flags[side] & ~2u) | 4u;
                }
                handled = 1;
            }
            else if (!handled) {
                current->button_flags[side] &= ~6u;
            }
        }
        if (!handled && hooks != NULL && hooks->outer_region_hit != NULL) {
            uint32_t selected = 0u;
            if (hooks->outer_region_hit(hooks->context, slot, x, y,
                                        (int)state->outer_horizontal,
                                        &selected)) {
                if (slot_index_out != NULL) *slot_index_out = current->index;
                if (mode == 0) {
                    if (state->mode == 3u) {
                        state->mode = 2u;
                        if (hooks->pointer_state_set != NULL)
                            hooks->pointer_state_set(hooks->context, 1);
                    }
                }
                else if (state->mode == 2u && current->selected_index != NULL) {
                    *current->selected_index = selected;
                    current->selection_changed = 1u;
                }
                handled = 1;
            }
        }
    }
    return handled;
}

// FUNCTION: LEGORR 0x0045cf00
void ui_priority_label_layout_apply(UiPriorityLabelRow* rows, uint32_t count,
                                    void (*position_set)(void*, void*),
                                    void (*visibility_set)(void*, void*))
{
    uint32_t i;
    if (rows == NULL) return;
    for (i = 0; i < count; ++i) {
        rows[i].replacement_label = rows[i].source_label;
        if (position_set != NULL)
            position_set(rows[i].source_label, rows[i].source_widget);
        if (visibility_set != NULL)
            visibility_set(rows[i].replacement_label, rows[i].replacement_widget);
    }
}

void ui_pointer_cursor_override_set(float duration)
{
    s_pointer_cursor_override_timer = duration;
}

float ui_pointer_cursor_override_remaining(void)
{
    return s_pointer_cursor_override_timer;
}

// FUNCTION: LEGORR 0x0045cd90
void ui_pointer_cursor_override_tick(float elapsed)
{
    if (s_pointer_cursor_override_timer > 0.0f)
        s_pointer_cursor_override_timer -= elapsed;
}

// FUNCTION: LEGORR 0x0045d640
void ui_cursor_move_to_element_center(const UiCursorNavigation* nav,
                                      uint32_t element)
{
    int left, top, right, bottom;
    if (nav == NULL || nav->element_bounds_get == NULL || nav->cursor_move == NULL)
        return;
    if (nav->element_bounds_get(nav->context, element, &left, &top,
                                &right, &bottom))
        nav->cursor_move(nav->context, left + (right - left) / 2,
                         top + (bottom - top) / 2);
}

// FUNCTION: LEGORR 0x0045d550
void ui_cursor_move_to_choice_entry(const UiCursorNavigation* nav,
                                    uint32_t choice)
{
    int left, top, right, bottom;
    if (choice == 0) {
        ui_cursor_move_to_element_center(nav, 0);
        return;
    }
    if (choice > 8 || nav == NULL || nav->choice_bounds_get == NULL ||
        nav->cursor_move == NULL) return;
    if (nav->choice_bounds_get(nav->context, 8, choice + 8,
                               &left, &top, &right, &bottom))
        nav->cursor_move(nav->context, left + (right - left) / 2,
                         top + (bottom - top) / 2);
}
