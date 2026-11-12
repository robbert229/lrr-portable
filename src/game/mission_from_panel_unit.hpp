// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what mission.cpp uses from panel_unit.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by mission.cpp only. */
#ifndef LRR_GAME_MISSION_FROM_PANEL_UNIT_HPP
#define LRR_GAME_MISSION_FROM_PANEL_UNIT_HPP

/* GAME-032 is converted (finding 554): the quota is Panel.c's
 * panelGlobs.crystalQuota, and the setup is declared in game/panel_unit.h. */
#include "panel_unit.h"
#define s_crystal_quota ((int)panelGlobs.crystalQuota)

#endif /* LRR_GAME_MISSION_FROM_PANEL_UNIT_HPP */
