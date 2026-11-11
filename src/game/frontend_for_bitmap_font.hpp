// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what bitmap_font.cpp sees from
 * frontend.cpp. A verbatim snapshot of frontend_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GAME_FRONTEND_FOR_BITMAP_FONT_HPP
#define LRR_GAME_FRONTEND_FOR_BITMAP_FONT_HPP

/* --- the screen-effect release (see frontend.hpp) --------------------------
 * The corpus table runs ten entries from DAT_004b8aa0 at stride 0x20: the
 * handle sits at entry +0, the flag dword twenty-eight bytes above the
 * corpus's walk pointer. Unowned: zero-init statics here. */
#define SCREEN_EFFECT_COUNT 10

extern void* s_screen_effect_handle[SCREEN_EFFECT_COUNT];
extern uint32_t s_screen_effect_flags[SCREEN_EFFECT_COUNT];
extern int s_screen_effect_seam_calls;
extern void* s_screen_effect_seam_last_handle;
extern int s_screen_effect_seam_last_flag;

extern void* s_ency_files[4];
extern void* s_ency_crystal;
extern void* s_ency_ore;

extern TextureSurface* s_interface_message_tab_info_text;
extern uint32_t s_screen_right_edge;
extern void* s_panel_ui_record;
namespace unported_ledger_stubs {
extern UiEntryHitTestState* s_ui_entry_hit_test_state;
} // namespace unported_ledger_stubs

int fe_font_cell(const TextureSurface* f, int* cw, int* ch);
void fe_font_keys(const TextureSurface* f, int cw, int ch, uint32_t keys[2]);
int fe_font_advance(const TextureSurface* f, int cw, int ch, int g, const uint32_t keys[2]);
int fe_font_glyph_draw_and_measure(void* px, int pitch, int w, int h, int x, int y, const TextureSurface* f, int cw, int ch, int g, const uint32_t keys[2], int draw);
extern FeFormattedTextTextureResolve s_formatted_text_texture_resolve;

extern AdvisorInlineTokenOps s_advisor_inline_token_ops;
extern uint32_t s_advisor_inline_last_sound;
extern uint32_t s_advisor_inline_token_generation;
extern float s_advisor_inline_duration;

namespace unported_ledger_stubs {
extern InputDeviceReleaseHooks s_input_device_release_hooks;
extern uint32_t s_input_device_release_flags;
extern uint32_t s_input_device_system_flags;
extern int s_input_device_active_track;
} // namespace unported_ledger_stubs

extern float s_bubble_offset_x;
extern float s_bubble_offset_y;
extern void* s_panel_position_slots[5];
extern float s_panel_position_offset_x;
extern float s_panel_position_offset_y;
void panel_blit_seam_call(void* res, float x, float y);
int frontend_config_fields(const char* value, char* storage, unsigned int storage_size, char** fields, unsigned int field_count, char separator);

extern PointerCursorState s_pointer_cursor_runtime;

extern float s_snorm_colors[90];
extern float s_snorm_cells[2][20];
extern float s_snorm_pairs[2][7][2];
extern uint32_t s_snorm_rows;
extern uint32_t s_snorm_sentinels[2];

extern uint32_t s_advisor_text_state_flags;
extern int s_advisor_text_current_slot;
extern float s_advisor_text_animation_timer;
extern float s_advisor_text_animation_reset_timer;
extern const char* s_advisor_text_request_source;
extern AdvisorTextAnimationOps s_advisor_text_animation_ops;
extern AdvisorTextDisplayOps s_advisor_text_display_ops;
extern void* s_message_font;
extern void* s_message_text_buffers[2];
extern float s_message_panel_y;
extern float s_message_text_y;
extern float s_message_text_height;
void advisor_message_layout_commit(void);
void advisor_message_layout_reset(void);

#define TIMED_EFFECT_RECORD_COUNT 40

extern const char* const s_tooltip_names[38];
extern TimedEffectRecord s_timed_effect_records[TIMED_EFFECT_RECORD_COUNT];
extern float s_timed_effect_tick;
extern TooltipOverlayState s_tooltip_overlay_state;
void timed_effect_cue_request_set_corpus(int index, uint32_t value);
void tooltip_overlay_state_fill( TooltipOverlayState* state, void* font, uint32_t value2, uint32_t value3, float scale, uint32_t value5, uint32_t value6, uint32_t value7, float color_r, float color_g, float color_b, uint32_t font_metric);
void timed_effect_timestamp_reset_corpus(int index);
void timed_effect_persistence_set_corpus(int index, int set);

#endif /* LRR_GAME_FRONTEND_FOR_BITMAP_FONT_HPP */
