// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what helpwindow uses from frontend.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_FRONTEND_FOR_HELPWINDOW_HPP
#define LRR_GAME_FRONTEND_FOR_HELPWINDOW_HPP

/* --- selection-state band (see frontend.hpp) -------------------------------
 * Zero-initialized module statics. The resource-position and help-window
 * button hit-test routines are ported; the help-window layout loader and
 * other state writers remain unported, so their inputs land at zero until
 * installed through the port hooks. The corpus's 0x1c-stride handle tables
 * index blindly; the explicit slot setters and diagnostics guard their
 * 16-entry windows (index * 7 dwords). */
#define SEL_HANDLE_ENTRIES 16

#define SEL_HANDLE_STRIDE 7 /* dwords per 0x1c-stride entry */

#define LAYOUT_POSITION_ENTRIES 2   /* corpus: two help-window buttons */

extern float s_help_window_button_rects[LAYOUT_POSITION_ENTRIES][4];
extern UiModeResourceBlitRectHook s_ui_resource_reposition_hook;
extern OverlayTextBufferOps s_overlay_text_buffer_ops;
extern void (*s_help_window_sound_clear)(int);
uint32_t surface_load_handle(const char* path);

#endif /* LRR_GAME_FRONTEND_FOR_HELPWINDOW_HPP */
