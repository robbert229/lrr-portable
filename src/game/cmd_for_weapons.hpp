// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what weapons uses from cmd.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_CMD_FOR_WEAPONS_HPP
#define LRR_GAME_CMD_FOR_WEAPONS_HPP

typedef struct CmdRayCandidateWalk {
    GameContext* game;
    EntityRayHitSweepState* state;
} CmdRayCandidateWalk;

#endif /* LRR_GAME_CMD_FOR_WEAPONS_HPP */
