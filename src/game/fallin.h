// SPDX-License-Identifier: LGPL-3.0-or-later
/* Fallin.c (original translation unit GAME-051, finding 599): landslides
 * and cave-ins, by the unit's own names.
 *
 * C-compatible header (ADR 0005). fallin.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include this
 * file: every include and declaration a header adds shifts MSVC's symbol
 * numbering (finding 506). src/game/test_c_boundaries.cpp includes it.
 *
 * In the matching build fallin_link_names.h, which fallin.c includes first,
 * gives every one of these functions the port's name (level.hpp, cmd.hpp,
 * crt.hpp), so every caller reaches them. Natively nothing calls these
 * names: the port tail lava_erosion_port.cpp serves the port's names. */
#ifndef LEGORRR_GAME_FALLIN_H
#define LEGORRR_GAME_FALLIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Maths.h's Point2I, Point2F and Vector3F. */
typedef struct FallinPoint2I {
    int x, y;
} FallinPoint2I;

typedef struct FallinPoint2F {
    float x, y;
} FallinPoint2F;

typedef struct FallinVector3F {
    float x, y, z;
} FallinVector3F;

void fallin_update(float elapsedGame);                              /* 0040f0e0 */
int fallin_try_generate_land_slide(FallinPoint2I* blockPos,
                                   int allowCaveIn);                /* 0040f190 */
int fallin_can_land_slide_at_block(FallinPoint2I* blockPos);        /* 0040f2b0 */
void fallin_generate_land_slide(FallinPoint2I* blockPos,
                                unsigned int fallinDirection,
                                int allowCaveIn);                   /* 0040f330 */
void fallin_initialise(unsigned int caveInLimit);                   /* 0040f5e0 */
void fallin_generate_cave_in(FallinPoint2I* blockPos,
                             unsigned int fallinDirection);         /* 0040f5f0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_FALLIN_H */
