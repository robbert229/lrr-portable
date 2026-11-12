// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>

#include "ui_list.hpp"
#include "priorities.h"
#include "frontend.hpp"
#include "gods98/media.hpp"
#include "ui_list_shared.hpp"

const char* ui_pointer_cursor_name_get(uint32_t index)
{
    return index < 56 ? s_pointer_cursor_names[index] : NULL;
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
    uint32_t slot_count;
    int handled = 0;
    if (state == NULL || state->slots == NULL) return 0;
    /* SimTrackCursorHitTest walks exactly four 0x20-byte entries beginning
     * at DAT_00552140; the adapter may provide fewer initialized slots. */
    slot_count = state->slot_count < 4u ? state->slot_count : 4u;
    for (slot = 0; slot < slot_count; ++slot) {
        UiTrackCursorSlot* current = &state->slots[slot];
        uint32_t side;
        int visible = (current->record_flags & 4u) != 0u &&
            (hooks == NULL || hooks->track_visible == NULL ||
             hooks->track_visible(hooks->context, slot));
        if (!visible) continue;
        for (side = 0; side < 2u; ++side) {
            if (!handled && ui_track_point_in_rect(current->button_rects[side],
                                                   x, y)) {
                if (slot_index_out != NULL) *slot_index_out = slot;
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
            else {
                current->button_flags[side] &= ~6u;
            }
        }
        if (!handled) {
            uint32_t selected = 0u;
            const int has_selection_adapter =
                hooks != NULL && hooks->outer_region_hit != NULL;
            const int outer_hit = has_selection_adapter
                ? hooks->outer_region_hit(
                    hooks->context, slot, x, y,
                    (current->record_flags & 8u) == 0u, &selected)
                : ui_track_point_in_rect(current->outer_rect, x, y);
            if (outer_hit) {
                if (mode == 0) s_ui_track_pointer_outer_gate = 1;
                if (slot_index_out != NULL) *slot_index_out = slot;
                if (mode == 0) {
                    if (state->mode == 3u) {
                        state->mode = 2u;
                        if (hooks->pointer_state_set != NULL)
                            hooks->pointer_state_set(hooks->context, 1);
                    }
                }
                else if (has_selection_adapter && state->mode == 2u &&
                         current->selected_index != NULL) {
                    *current->selected_index = selected;
                    current->selection_changed = 1u;
                }
                handled = 1;
            }
            else if (mode == 0) {
                s_ui_track_pointer_outer_gate = 0;
            }
        }
        else if (mode == 0) {
            /* The retail pass clears DAT_005013c0 after a button hit or
             * when the active track misses its outer-region bounds. */
            s_ui_track_pointer_outer_gate = 0;
        }
    }
    if (mode == 0 && s_ui_track_pointer_outer_gate == 0)
        state->mode = 3u;
    return handled;
}


void ui_pointer_cursor_override_set(float duration)
{
    media_pointer_cursor_override_set(media_pointer_cursor_slot(), duration);
}

float ui_pointer_cursor_override_remaining(void)
{
    return media_pointer_cursor_override();
}

void ui_pointer_cursor_override_tick(float elapsed)
{
    media_pointer_cursor_override_tick(elapsed);
}


