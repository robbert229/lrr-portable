// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-042 SpiderWeb.c (original translation unit, finding 505; finding
 * 620): the spider webs.
 *
 * A spider web is a type-0xc LegoObject (the level's SpiderWeb container)
 * that spans one ground block between two solid walls. The unit keeps one
 * 0xc-byte record per map block: the web object, its health and a flag
 * word. A unit that walks into a web is caught: it stays flagged until it
 * has torn the web down with the SpiderWeb weapon's damage, and the web is
 * removed when its health falls below zero or a wall beside it goes.
 *
 * C-compatible header (ADR 0005). spiderweb.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file, so its declarations move no other unit's symbol
 * numbering (finding 538); src/game/test_c_boundaries.cpp includes it.
 *
 * The names are inferred (finding 620). The record layouts are the
 * original's Win32 ones. */
#ifndef LEGORRR_GAME_SPIDERWEB_H
#define LEGORRR_GAME_SPIDERWEB_H

#include "lego_unit.h"
#include "legoobject_unit.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SpiderWeb_Block flags (+0x08). */
#define SPIDERWEB_FLAG_WEB 0x100 /* the block holds a web */

/* One record per map block (0xc bytes). */
typedef struct SpiderWeb_Block {
    Lego_Object* webObj; /* +0x00 the web, a type-0xc LegoObject */
    float health;        /* +0x04 100.0 when spun */
    unsigned int flags;  /* +0x08 SPIDERWEB_FLAG_* */
} SpiderWeb_Block;

typedef struct SpiderWeb_Globs {
    SpiderWeb_Block* webBlocks; /* 005520e8 width * height records */
    Lego_Level* level;          /* 005520ec */
} SpiderWeb_Globs;

extern SpiderWeb_Globs spiderWebGlobs; /* 005520e8 */

void spiderweb_initialise(Lego_Level* level);                  /* 004661c0 */
void spiderweb_shutdown(void);                                 /* 00466210 */
void spiderweb_restart(Lego_Level* level);                     /* 00466230 */
int spiderweb_spawn_at(int bx, int by);                        /* 00466250 */
void spiderweb_add(int bx, int by, struct Lego_Object* webObj); /* 00466380 */
int spiderweb_get_angle(int bx, int by, float* theta);         /* 004663f0 */
int spiderweb_check_collision(Lego_Object* liveObj);           /* 00466490 */
void spiderweb_update(float elapsedGame);                      /* 004665c0 */
int spiderweb_callback_update(Lego_Object* liveObj,
                              float* elapsedGame);             /* 004665e0 */
void spiderweb_remove_at(int bx, int by);                      /* 00466750 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_SPIDERWEB_H */
