// SPDX-License-Identifier: LGPL-3.0-or-later
/* Priorities.c (original translation unit GAME-034, text 0045cdc0-0045d7e0;
 * finding 505). Pilot step (a) of issue #384: the unit's existing
 * reconstructions gathered unchanged into one translation-unit-shaped file
 * in original address order. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "frontend.hpp"
#include "ui_list.hpp"
#include "cmd.hpp"
#include "priority_config_ports.hpp"
#include "simulation_ui_ports.hpp"
#include "gods98/block.hpp"
#include "gods98/sound.hpp"
#include "gods98/texture.hpp"
#include "gods98/string.hpp"
#include "gods98/media.hpp"
#include "crt/crt.hpp"

static uint32_t s_region_xy[2];  /* DAT_005013a8/3ac */
static float    s_region_wh[2];  /* DAT_005013b0/b4 */

/* The three priority loaders consume the two BlockFile views passed by the
 * original engine: `names` identifies the config paths and entry names,
 * while `values` supplies their strings.  Keep their loaded tables here in
 * command-kind/order form so callers can use the same records without
 * manufacturing PriorityConfigRows. */
static PriorityConfigState s_priority_config_loaded;
static int s_priority_alias_index[27];

static int frontend_config_fields(const char* value, char* storage,
                                  unsigned int storage_size,
                                  char** fields, unsigned int field_count,
                                  char separator)
{
    unsigned int i;
    unsigned int found = 1;
    size_t length;
    if (value == NULL || storage == NULL || fields == NULL ||
        storage_size == 0 || field_count == 0) return 0;
    length = strlen(value);
    if (length >= storage_size) return 0;
    memcpy(storage, value, length + 1);
    fields[0] = storage;
    for (i = 0; i < (unsigned int)length; ++i) {
        if (storage[i] == separator) {
            storage[i] = '\0';
            if (found < field_count) fields[found] = storage + i + 1;
            ++found;
        }
    }
    return found == field_count;
}

static int frontend_config_path(char* out, size_t out_size,
                                const char* prefix, const char* group,
                                const char* key)
{
    size_t length = 0;
    const char* parts[3];
    int p;
    parts[0] = prefix;
    parts[1] = group;
    parts[2] = key;
    if (out == NULL || out_size == 0) return 0;
    out[0] = '\0';
    for (p = 0; p < 3; ++p) {
        size_t part_length;
        if (parts[p] == NULL || parts[p][0] == '\0') continue;
        part_length = strlen(parts[p]);
        if (length != 0) {
            if (length + 2 >= out_size) return 0;
            out[length++] = ':';
            out[length++] = ':';
        }
        if (length + part_length >= out_size) return 0;
        memcpy(out + length, parts[p], part_length);
        length += part_length;
        out[length] = '\0';
    }
    return length != 0;
}

static int frontend_config_boolean(const char* value)
{
    if (value == NULL) return 2;
    if (strcasecmp_lite(value, "TRUE") == 0 ||
        strcasecmp_lite(value, "YES") == 0 ||
        strcasecmp_lite(value, "ON") == 0) return 1;
    if (strcasecmp_lite(value, "FALSE") == 0 ||
        strcasecmp_lite(value, "NO") == 0 ||
        strcasecmp_lite(value, "OFF") == 0) return 0;
    return 2;
}


// FUNCTION: LEGORR 0x0045cdc0
void frontend_priority_images_config_load(BlockFile* values,
                                          BlockFile* names)
{
    BlockFile* name_cfg = names != NULL ? names : values;
    BlockFile* value_cfg = values != NULL ? values : names;
    BlockRec* section;
    BlockRec* entry;
    if (name_cfg == NULL || value_cfg == NULL) return;
    s_priority_config_loaded.image_count = 0;
    section = block_subgroup(name_cfg, "PriorityImages");
    for (entry = block_first_child(section); entry != NULL;
         entry = block_next_sibling(entry)) {
        char path[512];
        char row_storage[512];
        char name_storage[256];
        char* fields[4];
        char* name_fields[2];
        const char* entry_name = block_name(entry);
        const char* value;
        int kind;
        int alias = -1;
        int name_count;
        if (entry_name == NULL ||
            !cmd_kind_by_name(entry_name, &kind) || kind < 0 || kind >= 27 ||
            !frontend_config_path(path, sizeof(path), NULL,
                                  "PriorityImages", entry_name)) continue;
        value = block_get_strptr(value_cfg, path);
        if (value == NULL) value = block_value(entry);
        if (!frontend_config_fields(value, row_storage,
                (unsigned int)sizeof(row_storage), fields, 4, ':')) continue;
        name_count = frontend_config_fields(fields[0], name_storage,
                (unsigned int)sizeof(name_storage), name_fields, 2, '|') ? 2 : 1;
        s_priority_config_loaded.labels[kind] = string_duplicate(name_fields[0]);
        s_priority_config_loaded.images[kind][0] =
            texture_get_or_load(NULL, fields[1]);
        s_priority_config_loaded.images[kind][1] =
            texture_get_or_load(NULL, fields[2]);
        s_priority_config_loaded.images[kind][2] =
            texture_get_or_load(NULL, fields[3]);
        if (name_count == 2 && sound_alias_lookup(name_fields[1], &alias)) {
            s_priority_alias_index[kind] = alias;
            s_priority_config_loaded.sound_aliases[kind] =
                string_duplicate(name_fields[1]);
        }
        ++s_priority_config_loaded.image_count;
    }
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

// FUNCTION: LEGORR 0x0045cf50
int frontend_priorities_config_load(BlockFile* values, BlockFile* names,
                                    const char* section_name)
{
    BlockFile* name_cfg = names != NULL ? names : values;
    BlockFile* value_cfg = values != NULL ? values : names;
    char section_path[512];
    BlockRec* section;
    BlockRec* entry;
    unsigned int count = 0;
    s_priority_config_loaded.priority_count = 0;
    if (name_cfg == NULL || value_cfg == NULL || section_name == NULL ||
        !frontend_config_path(section_path, sizeof(section_path),
                              section_name, "Priorities", NULL)) return 1;
    section = block_subgroup(name_cfg, section_path);
    for (entry = block_first_child(section); entry != NULL;
         entry = block_next_sibling(entry)) {
        char path[512];
        const char* entry_name = block_name(entry);
        const char* value;
        int kind;
        int enabled;
        if (entry_name == NULL || !cmd_kind_by_name(entry_name, &kind)) continue;
        if (!frontend_config_path(path, sizeof(path), section_name,
                                  "Priorities", entry_name)) continue;
        value = block_get_strptr(value_cfg, path);
        if (value == NULL) value = block_value(entry);
        enabled = frontend_config_boolean(value) == 1 ? 0 : 1;
        s_priority_config_loaded.kinds[count] = (uint32_t)kind;
        s_priority_config_loaded.weights[count] = 0x50 - (int)count * 5;
        s_priority_config_loaded.enabled[count] = enabled;
        priority_entry_order_value_set(kind,
                                       s_priority_config_loaded.weights[count]);
        priority_entry_state_set(kind, enabled);
        ++count;
        if (count >= 64) break;
    }
    s_priority_config_loaded.priority_count = count;
    return 1;
}

// FUNCTION: LEGORR 0x0045d060
void frontend_priorities_image_positions_config_load(BlockFile* values,
                                                      BlockFile* names)
{
    BlockFile* name_cfg = names != NULL ? names : values;
    BlockFile* value_cfg = values != NULL ? values : names;
    int i;
    if (name_cfg == NULL || value_cfg == NULL) return;
    for (i = 1; i <= 27; ++i) {
        char index[16];
        char path[512];
        char pair_storage[128];
        char* fields[2];
        const char* value;
        sprintf(index, "%d", i);
        if (!frontend_config_path(path, sizeof(path), NULL,
                                 "PrioritiesImagePositions", index)) continue;
        if (block_find(name_cfg, path) == NULL) continue;
        value = block_get_strptr(value_cfg, path);
        if (value == NULL) continue;
        if (!frontend_config_fields(value, pair_storage,
                (unsigned int)sizeof(pair_storage), fields, 2, ':')) continue;
        s_priority_config_loaded.positions[i - 1].x = atoi(fields[0]);
        s_priority_config_loaded.positions[i - 1].y = atoi(fields[1]);
        s_priority_config_loaded.positions[i - 1].visible = 1;
    }
    s_priority_config_loaded.position_count = 27;
}

namespace unported_ledger_stubs {

// RegionContainmentScanUpdate
// FUNCTION: LEGORR 0x0045d110
int ledger_stub_0045d110_RegionContainmentScanUpdate(
    RegionContainmentEntry* entries, uint32_t count, int x, int y,
    void (*changed)(void*, int, int), void* context)
{
    return region_containment_scan_update(entries, count, x, y, changed,
                                          context);
}

} // namespace unported_ledger_stubs

/* PriorityImageSelectionStateUpdate (FUN_0045d300): the corpus's
 * priority-image selection state pair (DAT_005013b8/13bc — no ported
 * owner, own storage). A cleared gate parks the sentinel 0x1b; while the
 * sentinel rides, a cleared third argument installs the candidate and
 * raises flag bit 2; otherwise an equal state just raises the flag. The
 * answer is the pre-call state against the candidate (the corpus threads
 * the stale word through the fall-through). */
// GLOBAL: LEGORR 0x005013b8
static int32_t  s_priority_image_state;
// GLOBAL: LEGORR 0x005013bc
static uint32_t s_priority_image_flags;

// FUNCTION: LEGORR 0x0045d300
int priority_image_selection_state_update(int candidate, int gate, int skip)
{
    const int32_t prior = s_priority_image_state;
    if (gate == 0) {
        s_priority_image_state = 0x1b;
    } else if (s_priority_image_state == 0x1b) {
        if (skip == 0) {
            s_priority_image_state = candidate;
            s_priority_image_flags |= 2u;
            return candidate == 0x1b ? 1 : 0;
        }
    } else if (s_priority_image_state == candidate) {
        s_priority_image_flags |= 2u;
        return 1;
    }
    return prior == candidate ? 1 : 0;
}

void priority_image_state_set(int32_t state, uint32_t flags)
{
    s_priority_image_state = state;
    s_priority_image_flags = flags;
}

void priority_image_state_peek(int32_t* state, uint32_t* flags)
{
    if (state != NULL) {
        *state = s_priority_image_state;
    }
    if (flags != NULL) {
        *flags = s_priority_image_flags;
    }
}

// FUNCTION: LEGORR 0x0045d370
void region_bounds_set(const uint32_t* xy_pair, uint32_t w, uint32_t h)
{
    if (xy_pair == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    s_region_xy[0] = xy_pair[0];
    s_region_xy[1] = xy_pair[1];
    s_region_wh[0] = (float)w; /* the corpus's uint->float casts */
    s_region_wh[1] = (float)h;
}

void region_bounds_peek(uint32_t* out_xy, float* out_wh)
{
    if (out_xy != NULL) {
        out_xy[0] = s_region_xy[0];
        out_xy[1] = s_region_xy[1];
    }
    if (out_wh != NULL) {
        out_wh[0] = s_region_wh[0];
        out_wh[1] = s_region_wh[1];
    }
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

void frontend_priority_config_state_get(PriorityConfigState* state)
{
    if (state != NULL) *state = s_priority_config_loaded;
}

int frontend_priority_sound_alias_get(int kind)
{
    return (kind >= 0 && kind < 27) ? s_priority_alias_index[kind] : -1;
}
