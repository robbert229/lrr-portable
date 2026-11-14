// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-040 SelectPlace.c (finding 633): the building placement marker, a
 * mesh of up to ten coloured block outlines.
 *
 * C-compatible header (ADR 0005). selectplace.c is compiled as C in the
 * matching build and as C++ in the native port, where nothing calls it:
 * the port's models (selectplace_port.cpp) stay the live ones there. Shared
 * C++ headers do not include this file (finding 506);
 * src/game/test_c_boundaries.cpp includes it. In the matching build the
 * four functions take the port's names (selectplace_link_names.h). */
#ifndef LEGORRR_GAME_SELECTPLACE_H
#define LEGORRR_GAME_SELECTPLACE_H

#ifdef __cplusplus
extern "C" {
#endif

struct Container;

/* SelectPlace (8 bytes). */
typedef struct SelectPlace {
    struct Container* contMesh; /* +0 */
    float tileDepth;            /* +4 */
} SelectPlace;

typedef struct SelectPlace_Point2I {
    int x;
    int y;
} SelectPlace_Point2I;

SelectPlace* selectplace_create(struct Container* root, float tileDepth); /* 00463f00 */
SelectPlace_Point2I* selectplace_transform_shape(const SelectPlace_Point2I* blockPos,
                                                 const SelectPlace_Point2I* shapePoints,
                                                 unsigned int shapeCount,
                                                 int rotation);          /* 00464110 */
SelectPlace_Point2I* selectplace_check_and_update(SelectPlace* selectPlace,
                                                  SelectPlace_Point2I* blockPos,
                                                  const SelectPlace_Point2I* shapePoints,
                                                  unsigned int shapeCount, int rotation,
                                                  void* map, unsigned int waterEntrances); /* 004641c0 */
void selectplace_hide(SelectPlace* selectPlace, int hide);              /* 00464720 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_SELECTPLACE_H */
