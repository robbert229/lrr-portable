// SPDX-License-Identifier: LGPL-3.0-or-later
/* Port-only helpers of the GAME-043 port tail (entity_stats_port.cpp,
 * finding 581), for the port's stats loader in entity.cpp. */
#ifndef LEGORRR_GAME_ENTITY_STATS_PORT_HPP
#define LEGORRR_GAME_ENTITY_STATS_PORT_HPP

#include "entity_stats.h"

/* statsGlobs.objectStats[type], pointed at the port tail's fixed row
 * storage on first use. */
StatsData** entity_stats_port_row_table(int type);

#endif /* LEGORRR_GAME_ENTITY_STATS_PORT_HPP */
