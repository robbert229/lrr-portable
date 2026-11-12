// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what frontend.cpp uses from frontend_unit.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by frontend.cpp only. */
#ifndef LRR_GAME_FRONTEND_FROM_FRONTEND_UNIT_HPP
#define LRR_GAME_FRONTEND_FROM_FRONTEND_UNIT_HPP

int fe_build_chain(BlockFile* cfg, const char* start_key, char names[][FE_LEVEL_NAME_MAX], char labels[][FE_NAME_MAX], int max);
int fe_parse_menu(FrontEnd* fe, int mi, const char* name);
void fe_overlay_step(FrontEnd* fe);
int fe_save_menu_list_entry_append(FeSubmenu* submenu, const FeSlot* slot, int row);
void fe_item_pos(const FrontEnd* fe, const FeItem* it, int i, int w, const TextureSurface* measure, int* x, int* y);
void fe_slider_pointer_handle(FrontEnd* fe, FeSubmenu* sub, const FeInput* in, const TextureSurface* measure);
const char* fe_selected_level_action(const FrontEnd* fe);
FeEvent frontend_start_level(FrontEnd* fe, const char* level);

#endif /* LRR_GAME_FRONTEND_FROM_FRONTEND_UNIT_HPP */
