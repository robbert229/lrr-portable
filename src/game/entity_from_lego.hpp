// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what entity.cpp uses from lego.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by entity.cpp only. */
#ifndef LRR_GAME_ENTITY_FROM_LEGO_HPP
#define LRR_GAME_ENTITY_FROM_LEGO_HPP

extern const char s_item_name_spider_web[];
extern const char s_item_name_electric_fence[];
extern const char s_item_name_processed_ore[];
extern const char s_item_name_barrier[];
extern const char s_item_name_ooh_scary[];
extern const char s_item_name_dynamite[];
extern const char s_item_name_tv_camera[];

#endif /* LRR_GAME_ENTITY_FROM_LEGO_HPP */
