// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/crt/crt.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_CRT_CRT_SHARED_HPP
#define LRR_CRT_CRT_SHARED_HPP

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

#endif /* LRR_CRT_CRT_SHARED_HPP */
