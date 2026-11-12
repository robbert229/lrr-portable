// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what movie.cpp uses from avi_player.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by movie.cpp only. */
#ifndef LRR_GODS98_MOVIE_FROM_AVI_PLAYER_HPP
#define LRR_GODS98_MOVIE_FROM_AVI_PLAYER_HPP

extern uint8_t s_avi_playback_disabled;

#endif /* LRR_GODS98_MOVIE_FROM_AVI_PLAYER_HPP */
