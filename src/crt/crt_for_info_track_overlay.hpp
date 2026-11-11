// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what info_track_overlay.cpp sees from
 * crt.cpp. A verbatim snapshot of crt_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_CRT_CRT_FOR_INFO_TRACK_OVERLAY_HPP
#define LRR_CRT_CRT_FOR_INFO_TRACK_OVERLAY_HPP

extern uint32_t s_advisor_state_word;
/* Pointer.c's current cursor type (DAT_00500a98), defined in
 * gods98/media.cpp. */
extern int s_cursor_slot;

extern uint32_t s_mode_zero_flag;

extern uint32_t s_config_word;

extern uint32_t s_ui_flag_word;
extern int s_global_value_word;

extern uint32_t s_level_stored_word;

extern uint32_t s_rockfall_base;

extern void* s_marker_table[32];
extern uint32_t s_marker_count;
extern void* s_marker_descriptor;

extern uint32_t s_task_refresh_flag;

extern uint32_t s_counter_004ff990;
extern uint32_t s_flag_004ff978;

extern uint32_t s_font_words[16];

/* DAT_00500110 record block: 0x30-byte panel-category records. Bounds, pair,
 * category word, and flags all share this storage. */
#define REQUEST_RECORD_COUNT 18

typedef struct RequestRecord {
    uint8_t  prefix[0x08];
    float    bounds_x1; /* +0x08 */
    float    bounds_y1; /* +0x0c */
    float    bounds_x2; /* +0x10 */
    float    bounds_y2; /* +0x14 */
    uint32_t pair_x;  /* +0x18: DAT_00500128, 0x30-byte category stride */
    uint32_t pair_y;  /* +0x1c: DAT_0050012c, 0x30-byte category stride */
    uint32_t category_word; /* +0x20 */
    uint8_t  reserved[0x08];
    uint32_t flags;   /* +0x2c */
} RequestRecord;

/* --- the DAT window band (see crt.hpp) ---------------------------------------
 * Auxiliary tables are zero-initialized statics. The panel position pair
 * lives in the corresponding 0x30-byte RequestRecord, preserving the
 * corpus's indexed record stride; public port hooks remain guarded. */
#define DAT_WINDOW_COUNT 18

extern RequestRecord s_request_records[REQUEST_RECORD_COUNT];
extern float s_fpair_a[DAT_WINDOW_COUNT];
extern float s_fpair_b[DAT_WINDOW_COUNT];
extern void* s_metric_records[DAT_WINDOW_COUNT];
extern uint32_t s_view_handle[DAT_WINDOW_COUNT];
extern void* s_view_rect_a[DAT_WINDOW_COUNT];
extern void* s_view_rect_b[DAT_WINDOW_COUNT];
extern int s_view_region[DAT_WINDOW_COUNT];

extern uint32_t s_scene_effect_head;
extern uint32_t s_scene_effect_tail;
extern uint32_t s_scene_effect_shared;

extern void* s_linked_list_head;
extern void* s_linked_list_aux;

#endif /* LRR_CRT_CRT_FOR_INFO_TRACK_OVERLAY_HPP */
