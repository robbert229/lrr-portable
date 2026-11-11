// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what object_state_file.cpp sees from
 * savegame.cpp. A verbatim snapshot of savegame_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GAME_SAVEGAME_FOR_OBJECT_STATE_FILE_HPP
#define LRR_GAME_SAVEGAME_FOR_OBJECT_STATE_FILE_HPP

extern const char* MODE_RB;
extern const char* MODE_WB;
long save_stream_length(FILE* fp);

#endif /* LRR_GAME_SAVEGAME_FOR_OBJECT_STATE_FILE_HPP */
