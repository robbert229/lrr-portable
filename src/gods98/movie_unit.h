// SPDX-License-Identifier: LGPL-3.0-or-later
/* Boundary header for the movie unit (G98-001, finding 618): the
 * extern "C" entry points the game calls (Lego.c's Lego_PlayMovie_old,
 * Reward.c and FrontEnd.c). The unit itself is C++ in the original (a
 * thiscall class, finding 505 rule 8), and so is movie_unit.cpp. Shared C++
 * headers do not include this file (ADR 0005 entropy policy).
 *
 * The binary attests none of the names. The native port has no DirectShow:
 * there the entry points are video_graph_port.cpp's (no movie loads). */
#ifndef LEGORRR_GODS98_MOVIE_UNIT_H
#define LEGORRR_GODS98_MOVIE_UNIT_H

#ifdef __cplusplus
extern "C" {
#endif

struct G98CMovie;
struct tagRECT;

struct G98CMovie* movie_load(const char* fName);              /* 00472560 */
void movie_get_size(struct G98CMovie* mov, unsigned int* width,
                    unsigned int* height);                     /* 00472670 */
unsigned int movie_get_duration(struct G98CMovie* mov);        /* 004726c0 */
int movie_update(struct G98CMovie* mov, float speed,
                 struct tagRECT* destRect);                    /* 004726d0 */
void movie_free(struct G98CMovie* mov);                        /* 004726f0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_MOVIE_UNIT_H */
