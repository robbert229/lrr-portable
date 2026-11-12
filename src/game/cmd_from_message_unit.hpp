// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what cmd.cpp uses from message_unit.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by cmd.cpp only. */
#ifndef LRR_GAME_CMD_FROM_MESSAGE_UNIT_HPP
#define LRR_GAME_CMD_FROM_MESSAGE_UNIT_HPP

extern int s_queued_count[2];
extern int s_ping;

#endif /* LRR_GAME_CMD_FROM_MESSAGE_UNIT_HPP */
