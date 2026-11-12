// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what panel_unit uses from frontend.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_FRONTEND_FOR_PANEL_UNIT_HPP
#define LRR_GAME_FRONTEND_FOR_PANEL_UNIT_HPP

#define PANEL_CATEGORY_WINDOWS 18

namespace unported_ledger_stubs {
// UITextOutputPositionAndDraw
typedef struct UiTextOutputPointerBinding {
    void* record;
    void* text_buffer;
    const float* offsets;
    int owns_text_buffer;
    int panel_index;
} UiTextOutputPointerBinding;
} // namespace unported_ledger_stubs

int ui_panel_string_equal_case_insensitive(const char* first, const char* second);
extern float s_panel_layout_size[2];
namespace unported_ledger_stubs {
extern UiTextOutputPointerBinding s_ui_text_output_bindings[64];
extern float s_ui_text_output_panel_offsets[18][8];
int ui_text_output_binding_install(void* record, void* text_buffer, const float* offsets, int owns, int panel_index);
} // namespace unported_ledger_stubs
extern FrontendPanelRotationControlState s_panel_rotation_control;

#endif /* LRR_GAME_FRONTEND_FOR_PANEL_UNIT_HPP */
