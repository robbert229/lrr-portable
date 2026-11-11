// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/game/frontend.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GAME_FRONTEND_SHARED_HPP
#define LRR_GAME_FRONTEND_SHARED_HPP

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

#endif /* LRR_GAME_FRONTEND_SHARED_HPP */
