// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what frontend.cpp uses from panel_unit.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by frontend.cpp only. */
#ifndef LRR_GAME_FRONTEND_FROM_PANEL_UNIT_HPP
#define LRR_GAME_FRONTEND_FROM_PANEL_UNIT_HPP

/* GAME-032 is converted (finding 554): Panel.c's state is panelGlobs in
 * game/panel_unit.h. The port's names for its fields map onto it. */
#include "panel_unit.h"
#include "gods98/image_adapters.h"

/* Superseded: the native HUD draws a panel's buttons with
 * panel_texture_slots_render (frontend.hpp); Panel_Button_DisplayAll is
 * panel_texture_slots_blit_to(int) in game/panel_unit.h. Kept uncalled. */
void panel_texture_slots_blit_to(int panel, void* pixels, int pitch, int width, int height);
#define s_ui_panel_names (panelGlobs.panelName)
#define s_ui_panel_indexed_names (panelGlobs.buttonName)
#define s_panel_surface_handle (*(void**)&panelGlobs.sideBarMeterImage)
#define s_panel_width (panelGlobs.sideBarMeterOffset.x)
#define s_panel_height (panelGlobs.sideBarMeterOffset.y)
#define s_panel_extra (panelGlobs.sideBarMeterLength)
#define s_panel_layout_surface (*(void**)&panelGlobs.airMeterJuiceImage)
#define s_panel_layout_glyph (*(void**)&panelGlobs.airMeterNoAirImage)
#define s_panel_layout_extra (panelGlobs.airMeterJuiceLength)
#define s_panel_layout_offset (&panelGlobs.airMeterNoAirOffset.x)
#define s_overlay_anim_float (panelGlobs.sideBarMeterLevel)
#define s_panel_ui_record (*(void**)&panelGlobs.scrollTextWindow)

#endif /* LRR_GAME_FRONTEND_FROM_PANEL_UNIT_HPP */
