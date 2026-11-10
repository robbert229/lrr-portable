// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GAME_UI_LIST_HPP
#define LEGORRR_GAME_UI_LIST_HPP

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Priority-panel button action (FUN_0045b620): buttons 0-8 toggle a
 * priority row, buttons 9-16 move rows 1-8 up one. */
void ui_list_priority_button_action_dispatch(uint32_t action);
void ui_pointer_cursor_name_registry_init(void); /* FUN_0045c830 */
const char* ui_pointer_cursor_name_get(uint32_t index);
void ui_pointer_cursor_override_set(float duration);
float ui_pointer_cursor_override_remaining(void);
void ui_pointer_cursor_override_tick(float elapsed); /* FUN_0045cd90 */
typedef struct UiOverlayInitSink {
    void* context;
    void (*panel_create)(void* context, const char* name, int group,
                         int x, int y, int width, int height, int flags);
    void (*element_add)(void* context, int group, int kind,
                        float x, float y, float width, float height,
                        const char* name);
    void* (*element_create)(void* context, int group, int kind,
                            float x, float y, float width, float height,
                            int flags);
    void (*handle_store)(void* context, int which, void* handle);
};
void ui_scroll_info_panel_initialize(const UiOverlayInitSink* sink); /* FUN_0045b740 */
void ui_overlay_create_configured_element(const UiOverlayInitSink* sink); /* FUN_0045b850 */
typedef struct UiInfoPanelStringInit {
    int text_id;
    uint32_t slot;
    uint32_t mode;
    uint32_t value;
} UiInfoPanelStringInit;
typedef struct UiInfoPanelStringEntry {
    void* text_handle;
    uint32_t flags;
    uint32_t value;
} UiInfoPanelStringEntry;
#define UI_INFO_PANEL_STRING_CAPACITY 64
typedef struct UiInfoPanelStringTable {
    UiInfoPanelStringEntry entries[UI_INFO_PANEL_STRING_CAPACITY];
} UiInfoPanelStringTable;
typedef void* (*UiInfoPanelStringResolveHook)(void* context, int text_id);
uint32_t ui_info_panel_string_entries_initialize(
    UiInfoPanelStringTable* table, const UiInfoPanelStringInit* entries,
    uint32_t count, void* context,
    UiInfoPanelStringResolveHook resolve); /* FUN_004636b0 */
typedef struct UiTrackCursorSlot {
    uint32_t index;
    uint32_t record_flags; /* retail DAT_0055215c: bit 2 enables, bit 3 selects Y */
    float button_rects[2][4]; /* screen-space left, top, right, bottom */
    uint32_t button_flags[2];
    float outer_rect[4]; /* screen-space bounds when no outer-hit adapter exists */
    uint32_t* selected_index;
    uint32_t selected_count;
    uint32_t selection_changed;
} UiTrackCursorSlot;
typedef struct UiTrackCursorHitHooks {
    void* context;
    /* Retail FUN_0045ab30 visibility gate for the indexed track record. */
    int (*track_visible)(void* context, uint32_t slot_index);
    /* Optional native-record adapter. It tests the translated outer bounds
     * and computes the projected item index from the payload record. */
    int (*outer_region_hit)(void* context, uint32_t slot_index,
                            int x, int y, int horizontal,
                            uint32_t* selected_index);
    void (*pointer_state_set)(void* context, int active);
} UiTrackCursorHitHooks;
typedef struct UiTrackCursorState {
    UiTrackCursorSlot* slots;
    uint32_t slot_count;
    uint32_t mode; /* 3 = no active side */
} UiTrackCursorState;
int ui_sim_track_cursor_hit_test(UiTrackCursorState* state, int x, int y,
                                 int mode, int allow_mode_change,
                                 const UiTrackCursorHitHooks* hooks,
                                 uint32_t* slot_index_out); /* FUN_004638a0 */

#ifdef __cplusplus
}
#endif

#endif
