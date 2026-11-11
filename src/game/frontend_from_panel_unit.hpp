// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what frontend.cpp uses from panel_unit.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by frontend.cpp only. */
#ifndef LRR_GAME_FRONTEND_FROM_PANEL_UNIT_HPP
#define LRR_GAME_FRONTEND_FROM_PANEL_UNIT_HPP

namespace unported_ledger_stubs {
void* ui_text_output_record_allocate(int panel, uintptr_t font_handle, const float rect[4], uint32_t buffer_size);
void ui_text_output_position_and_draw(void* record, uint32_t start_line, float line_scale);
} // namespace unported_ledger_stubs
extern const char* s_ui_panel_names[12];
extern const char* s_ui_panel_indexed_names[12][24];
void panel_texture_slots_blit_to(int panel, void* pixels, int pitch, int width, int height);
extern void* s_panel_surface_handle;
extern float s_panel_width;
extern float s_panel_height;
extern uint32_t s_panel_extra;
extern void* s_panel_layout_surface;
extern void* s_panel_layout_glyph;
extern uint32_t s_panel_layout_extra;
extern float s_panel_layout_offset[2];
extern uint32_t s_advisor_bar_surfaces[3];
extern float s_overlay_anim_float;

#endif /* LRR_GAME_FRONTEND_FROM_PANEL_UNIT_HPP */
