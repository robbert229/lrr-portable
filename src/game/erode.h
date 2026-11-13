// SPDX-License-Identifier: LGPL-3.0-or-later
/* Erode.c (original translation unit GAME-015, finding 505; finding 599):
 * lava erosion, by the unit's own names.
 *
 * C-compatible header (ADR 0005). erode.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include this
 * file: every include and declaration a header adds shifts MSVC's symbol
 * numbering (finding 506). src/game/test_c_boundaries.cpp includes it.
 *
 * In the matching build erode_link_names.h, which erode.c includes first,
 * gives every one of these functions the port's name (level.hpp), so every
 * caller reaches them. Natively nothing calls these names: the port tail
 * lava_erosion_port.cpp and level.cpp serve the port's names. */
#ifndef LEGORRR_GAME_ERODE_H
#define LEGORRR_GAME_ERODE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Maths.h's Point2I and Vector3F. */
typedef struct ErodePoint2I {
    int x, y;
} ErodePoint2I;

typedef struct ErodeVector3F {
    float x, y, z;
} ErodeVector3F;

void erode_initialise(float triggerTime, float erodeTime,
                      float lockTime);                              /* 0040e930 */
int erode_get_free_activity(unsigned int* index);                   /* 0040e990 */
float erode_get_block_erode_rate(ErodePoint2I* blockPos);           /* 0040e9c0 */
void erode_set_block_erode_rate(ErodePoint2I* blockPos,
                                unsigned int erodeSpeed);           /* 0040ea10 */
void erode_update(float elapsedGame);                               /* 0040eab0 */
void erode_add_locked_block(ErodePoint2I* blockPos);                /* 0040ee00 */
void erode_set_active_block(ErodePoint2I* blockPos, int on);        /* 0040ee50 */
int erode_is_block_locked(ErodePoint2I* blockPos);                  /* 0040efb0 */
int erode_find_adjacent_block(ErodePoint2I* blockPos,
                              ErodePoint2I* adjPos);                /* 0040f000 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_ERODE_H */
