// SPDX-License-Identifier: LGPL-3.0-or-later
/* RadarMap.c (original translation unit GAME-036, finding 505; finding 606):
 * the radar map, the top-down map in the corner of the game screen, by the
 * unit's own names.
 *
 * C-compatible header (ADR 0005). radarmap.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * includes it.
 *
 * In the matching build radarmap_link_names.h, which radarmap.c includes
 * first, gives eight of these functions the port's names, so every caller
 * reaches them. Natively nothing calls these names: the port tail
 * map_view_port.cpp serves the port's names. */
#ifndef LEGORRR_GAME_RADARMAP_H
#define LEGORRR_GAME_RADARMAP_H

#include <stdint.h>

#include "gods98/draw.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TerrainMap;
struct Lego_Object;

/* RadarMap (0x3c bytes), allocated by radarmap_create. Lego.c keeps the
 * level's radar map at Lego_Level +0x84. */
typedef struct RadarMap {
    struct TerrainMap* map;     /* +0x00 Map3D */
    Area2F screenRect;          /* +0x04 */
    float zoom;                 /* +0x14 screen pixels per block */
    float blockSize;            /* +0x18 world units per block */
    Point2F centrePos;          /* +0x1c world position at the centre */
    float viewLeft;             /* +0x24 world bounds drawn, set by radarmap_draw */
    float viewTop;              /* +0x28 */
    float viewRight;            /* +0x2c */
    float viewBottom;           /* +0x30 */
    DrawRect* drawRectList;     /* +0x34 the objects' rectangles while drawing */
    uint32_t drawRectCount;     /* +0x38 */
} RadarMap;

void radarmap_initialise(void);                                    /* 0045d8b0 */
RadarMap* radarmap_create(struct TerrainMap* map, const Area2F* screenRect,
                          float zoom);                             /* 0045da90 */
void radarmap_free(RadarMap* radarMap);                            /* 0045daf0 */
void radarmap_draw_survey_dot_circle(RadarMap* radarMap,
                                     const Point2F* wPos, float radius,
                                     float brightness);            /* 0045db00 */
void radarmap_draw(RadarMap* radarMap, const Point2F* centrePos);  /* 0045dbc0 */
int radarmap_can_show_object(struct Lego_Object* liveObj);         /* 0045e400 */
int radarmap_inside_radar(RadarMap* radarMap, uint32_t screenX,
                          uint32_t screenY);                       /* 0045e660 */
int radarmap_screen_to_block_pos(RadarMap* radarMap, uint32_t screenX,
                                 uint32_t screenY, Point2F* outWorldPos,
                                 int* outBx, int* outBy);          /* 0045e6d0 */
int radarmap_try_select_object(RadarMap* radarMap, uint32_t screenX,
                               uint32_t screenY,
                               struct Lego_Object** outObj,
                               Point2F* outObjPos);                /* 0045e820 */
void radarmap_transform_rect(RadarMap* radarMap, Area2F* rect);    /* 0045e940 */
int radarmap_get_block_colour(DrawRect* drawRect, int bx, int by); /* 0045e9e0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_RADARMAP_H */
