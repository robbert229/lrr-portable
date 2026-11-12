// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what cmd.cpp uses from weapons.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by cmd.cpp only. */
#ifndef LRR_GAME_CMD_FROM_WEAPONS_HPP
#define LRR_GAME_CMD_FROM_WEAPONS_HPP

int cmd_place_ray_candidate_check(EntitySlot* entity, void* payload);

#endif /* LRR_GAME_CMD_FROM_WEAPONS_HPP */
