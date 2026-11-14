// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-035 PTL.c (original translation unit, finding 505; rebuilt in
 * finding 652).
 *
 * C-compatible header (ADR 0005). ptl.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include
 * this file: every include and declaration a header adds shifts MSVC's
 * symbol numbering (finding 506). The C callers declare the functions they
 * call in their imports headers; src/game/test_c_boundaries.cpp includes
 * this header. */
#ifndef LEGORRR_GAME_PTL_H
#define LEGORRR_GAME_PTL_H

#ifdef __cplusplus
extern "C" {
#endif

struct Message_Event;
struct RadarMap;

int ptl_initialise(const char* fname, const char* gameName);       /* 0045d7e0 */
void ptl_translate_event(struct Message_Event* event);             /* 0045d870 */
void radarmap_set_zoom(struct RadarMap* radarMap, float zoom);     /* 0045d8a0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_PTL_H */
