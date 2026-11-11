// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/game/savegame.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GAME_SAVEGAME_SHARED_HPP
#define LRR_GAME_SAVEGAME_SHARED_HPP

extern const char* MODE_RB;
extern const char* MODE_WB;
long save_stream_length(FILE* fp);

#endif /* LRR_GAME_SAVEGAME_SHARED_HPP */
