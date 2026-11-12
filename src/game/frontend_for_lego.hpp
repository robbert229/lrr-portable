// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lego uses from frontend.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_FRONTEND_FOR_LEGO_HPP
#define LRR_GAME_FRONTEND_FOR_LEGO_HPP

int fe_font_text_width(const char* s, const TextureSurface* f);
extern const TextureSurface* s_advisor_tooltip_font;
extern char s_tooltip_info_strings[9][256];
extern AdvisorTutorialIconLoadFn s_tutorial_icon_load;
extern AdvisorTutorialIconInitializeFn s_tutorial_icon_initialize;
extern void* s_tutorial_icon_context;
extern void* s_tutorial_icon;
extern SelectedGroundMarkerOps s_selected_ground_marker_ops;
extern DebugPanelTextOps s_debug_panel_text_ops;
extern InterfaceSpawnRequestOps s_interface_spawn_request_ops;
extern InterfaceSpawnRequestState s_interface_spawn_request_state;

#endif /* LRR_GAME_FRONTEND_FOR_LEGO_HPP */
