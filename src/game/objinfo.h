// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-031 ObjInfo.c (finding 633): the overlays drawn over objects: the
 * health bar, the hunger image and the thought-bubble image.
 *
 * C-compatible header (ADR 0005). objinfo.c is compiled as C in the
 * matching build and as C++ in the native port, where nothing calls it:
 * the port's models (objinfo_port.cpp) stay the live ones there. Shared C++
 * headers do not include this file (finding 506);
 * src/game/test_c_boundaries.cpp includes it. In the matching build four
 * of the names are the port's (objinfo_link_names.h). */
#ifndef LEGORRR_GAME_OBJINFO_H
#define LEGORRR_GAME_OBJINFO_H

#ifdef __cplusplus
extern "C" {
#endif

struct BlockRec;
struct Lego_Object;
struct Image;

void objinfo_initialise(struct BlockRec* config, const char* gameName);    /* 00459530 */
int objinfo_load_health_bar(struct BlockRec* config, const char* gameName); /* 00459560 */
int objinfo_load_hunger(struct BlockRec* config, const char* gameName);     /* 00459900 */
void objinfo_load_hunger_images(struct BlockRec* config, const char* gameName); /* 004599c0 */
int objinfo_load_bubble(struct BlockRec* config, const char* gameName);     /* 00459a50 */
void objinfo_draw_health_bar(struct Lego_Object* liveObj, int screenX, int screenY); /* 00459b00 */
void objinfo_draw_hunger(struct Lego_Object* liveObj, int screenX, int screenY);     /* 00459f50 */
void objinfo_draw_bubble(struct Image* image, int screenX, int screenY);            /* 00459fd0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_OBJINFO_H */
