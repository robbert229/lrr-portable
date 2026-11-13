// SPDX-License-Identifier: LGPL-3.0-or-later
/* Boundary header for the AVI player unit (G98-012, finding 604): the
 * extern "C" entry points the rest of gods98 and the game call. The unit
 * itself is C++ in the original (thiscall classes with vtables and EH
 * frames, finding 505 rule 8), and so is avi_player.cpp. Shared C++ headers
 * do not include this file (ADR 0005 entropy policy).
 *
 * The binary attests none of the names. The port's own movie module keeps
 * its names (movie.hpp) and runs on its own code (avi_player_port.cpp). */
#ifndef LEGORRR_GODS98_AVI_PLAYER_H
#define LEGORRR_GODS98_AVI_PLAYER_H

#ifdef __cplusplus
class AviMovie;
extern "C" {
#else
typedef struct AviMovie AviMovie;
#endif

struct tagRECT;

void avi_initialise(void* directDraw);                         /* 0047ec80 */
AviMovie* avi_load(char* fName);                               /* 0047ec90 */
int avi_update(AviMovie* anim);                                /* 0047ecf0 */
void avi_blit_to_back_buffer(AviMovie* anim,
                             struct tagRECT* destRect);         /* 0047ed00 */
void avi_set_time(AviMovie* anim, unsigned long time);         /* 0047ed40 */
unsigned long avi_get_time(AviMovie* anim);                    /* 0047ed50 */
unsigned long avi_get_length(AviMovie* anim);                  /* 0047ed60 */
void avi_remove(AviMovie* anim);                               /* 0047ed70 */
int avi_is_ok(AviMovie* anim);                                 /* 0047ed80 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_AVI_PLAYER_H */
