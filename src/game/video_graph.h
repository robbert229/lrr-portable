// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * Movie.c entries (video_graph.hpp) and FrontEnd.c's movie wait.
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_VIDEO_GRAPH_H
#define LEGORRR_GAME_VIDEO_GRAPH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct DirectShowGraphState;
struct DirectShowGraphState* directshow_render_surface_create(const char* path); /* 00472560 */
void directshow_wrapper_destroy(struct DirectShowGraphState* state); /* 004726f0 */
/* Front_PlayMovie (00415700) in the original ABI; unannotated adapter. */
void movie_play_wait(struct DirectShowGraphState* movie, int skippable);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_VIDEO_GRAPH_H */
