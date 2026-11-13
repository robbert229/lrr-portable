// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * Movie.c's entries (gods98/movie_unit.h, G98-001, finding 618) and
 * FrontEnd.c's movie wait. Only C units, frontend_unit.cpp and
 * src/game/test_c_boundaries.cpp include it. */
#ifndef LEGORRR_GAME_REWARD_MOVIE_H
#define LEGORRR_GAME_REWARD_MOVIE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct G98CMovie;
struct G98CMovie* movie_load(const char* fName);               /* 00472560 */
void movie_free(struct G98CMovie* mov);                         /* 004726f0 */
/* Front_PlayMovie (00415700) in the original ABI; unannotated adapter. */
void movie_play_wait(struct G98CMovie* movie, int skippable);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_REWARD_MOVIE_H */
