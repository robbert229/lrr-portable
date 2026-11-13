// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-014 Encyclopedia.c (finding 633): the info dock's encyclopedia text
 * for the selected object.
 *
 * C-compatible header (ADR 0005). encyclopedia.c is compiled as C in the
 * matching build and as C++ in the native port, where nothing calls it: the
 * port's models (encyclopedia_port.cpp) stay the live ones there. Shared
 * C++ headers do not include this file (finding 506);
 * src/game/test_c_boundaries.cpp includes it. In the matching build four of
 * the names are the port's (encyclopedia_link_names.h). */
#ifndef LEGORRR_GAME_ENCYCLOPEDIA_H
#define LEGORRR_GAME_ENCYCLOPEDIA_H

#ifdef __cplusplus
extern "C" {
#endif

struct BlockRec;
struct Lego_Object;
struct ViewProjectionAdapter;

/* Open each object type's encyclopedia file named in the config. */
void encyclopedia_initialise(struct BlockRec* config, const char* gameName); /* 0040e490 */
/* Show the object's file in the info dock. */
void encyclopedia_select_object(struct Lego_Object* liveObj);        /* 0040e700 */
/* Stop showing the selected object's file. */
void encyclopedia_disable_update(void);                              /* 0040e7e0 */
/* Print the selected file into the info dock and draw it. */
void encyclopedia_update(float elapsed);                             /* 0040e7f0 */
/* Draw the selected object's ground marker while the dock is open. */
void encyclopedia_draw_select_box(struct ViewProjectionAdapter* view); /* 0040e8d0 */
/* Forget the object when it is removed. */
void encyclopedia_remove_current_reference(struct Lego_Object* dead); /* 0040e910 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_ENCYCLOPEDIA_H */
