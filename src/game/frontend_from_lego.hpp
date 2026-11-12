// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what frontend.cpp uses from lego.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by frontend.cpp only. */
#ifndef LRR_GAME_FRONTEND_FROM_LEGO_HPP
#define LRR_GAME_FRONTEND_FROM_LEGO_HPP

namespace frontend_config {
void panels_config_entry_apply(BlockFile* config, int display_width, int display_height);
void panel_button_layout_config_load(BlockFile* config, int display_width, int display_height);
void text_messages_config_load(BlockFile* config);
} // namespace frontend_config

#endif /* LRR_GAME_FRONTEND_FROM_LEGO_HPP */
