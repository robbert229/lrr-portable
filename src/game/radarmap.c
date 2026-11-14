// SPDX-License-Identifier: LGPL-3.0-or-later
/* RadarMap.c, original translation unit GAME-036 (finding 505), rebuilt in
 * original shape (ADR 0005; finding 606): text 0045d8b0-0045f030, .rdata
 * 0049e65c-0049e6ac and .data 004a8f28-004a931c.
 *
 * The radar map: the map panel in the corner of the game screen. Each
 * frame RadarMap_Draw fills one rectangle per visible block, coloured by
 * its surface, outlines the blocks with flag 0x20 (Lego.c's 00432d90),
 * and draws one rectangle per object. In view mode 1 it also projects the
 * screen's corners and a seven-line arrow from the main view onto the
 * ground and draws them, the camera's footprint. Lego.c creates one radar
 * map per level, and asks it which block or object is under the mouse.
 *
 * The port's implementation stays in the port tail map_view_port.cpp,
 * unannotated. In the matching build radarmap_link_names.h gives this
 * unit's functions the port's names, so every caller reaches them;
 * natively the port tail serves those names. */
#include "radarmap_link_names.h"

#include <math.h>
#include <stdlib.h>

#include "crt/vc5names.h"

#include "radarmap.h"
#include "radarmap_imports.h"
/* legoGlobs' fields (lego_globs.h, finding 653). lego_unit.h, included
 * above, pays for its symbols (finding 538). */
#include "lego_globs.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

/* Maths.h's ColourRGBF and Point4F. */
typedef struct RadarMap_Colour {
    float r, g, b;
} RadarMap_Colour;

typedef struct RadarMap_Point4F {
    float x, y, z, w;
} RadarMap_Point4F;

typedef struct RadarMap_Point3F {
    float x, y, z;
} RadarMap_Point3F;

#define RADARMAP_COLOURCOUNT 30
#define RADARMAP_MAXARROWLINES 20
#define RADARMAP_MAXOUTLINES 200
#define RADARMAP_MAXBLOCKRECTS 1500

/* The colour table's entries (0..255 in the image, 0..1 once
 * radarmap_initialise has run). */
#define RADARMAP_COLOUR_OUTLINE 0          /* solid buildings' outlines */
#define RADARMAP_COLOUR_UNKNOWN 1          /* a wall of no listed surface */
#define RADARMAP_COLOUR_WATER 2
#define RADARMAP_COLOUR_LAVA 3
#define RADARMAP_COLOUR_WALL 4             /* 4-8: soil to immovable walls */
#define RADARMAP_COLOUR_GROUND 9           /* 9-13: soil to immovable ground */
#define RADARMAP_COLOUR_OBJECT 14
#define RADARMAP_COLOUR_OBJECT_TYPE3 15
#define RADARMAP_COLOUR_RESOURCE 16        /* crystals and ore */
#define RADARMAP_COLOUR_REINFORCED 17      /* 17-21: soil to immovable */
#define RADARMAP_COLOUR_POWERED 22
#define RADARMAP_COLOUR_UNPOWERED 23
#define RADARMAP_COLOUR_ORESEAM 24
#define RADARMAP_COLOUR_CRYSTALSEAM 25
#define RADARMAP_COLOUR_RECHARGESEAM 26
#define RADARMAP_COLOUR_LAKE 27
#define RADARMAP_COLOUR_FLAG100000 28
#define RADARMAP_COLOUR_FLAG20000 29

/* The original's .data block 004a8f28-004a931c. */
typedef struct RadarMap_Globs {
    RadarMap_Colour colourTable[RADARMAP_COLOURCOUNT]; /* +0x000 */
    int highlightX;                                    /* +0x168 */
    int highlightY;                                    /* +0x16c */
    /* The arrow of the followed unit: [0] the lines' starts, [1] their
     * ends, in 640x480 screen units scaled to the display. */
    RadarMap_Point4F arrowLines[2][RADARMAP_MAXARROWLINES]; /* +0x170 */
    uint32_t arrowLineCount;                           /* +0x3f0 */
} RadarMap_Globs;

/* Maths.h's Point2I. */
typedef struct RadarMap_Point2I {
    int x, y;
} RadarMap_Point2I;

/* Searched by radarmap_callback_try_select_object. */
typedef struct RadarMap_SearchObject {
    struct Lego_Object* result;  /* +0x0 */
    Point2F wPos;                /* +0x4 */
    float radius;                /* +0xc */
} RadarMap_SearchObject;

// GLOBAL: LEGORR 0x004a8f28
RadarMap_Globs radarmapGlobs = {
    {
        {255.0f, 204.0f, 0.0f},
        {45.0f, 0.0f, 75.0f},
        {41.0f, 96.0f, 255.0f},
        {255.0f, 90.0f, 0.0f},
        {164.0f, 227.0f, 149.0f},
        {97.0f, 203.0f, 86.0f},
        {0.0f, 175.0f, 12.0f},
        {0.0f, 93.0f, 0.0f},
        {0.0f, 47.0f, 0.0f},
        {199.0f, 123.0f, 255.0f},
        {174.0f, 89.0f, 233.0f},
        {144.0f, 60.0f, 200.0f},
        {115.0f, 30.0f, 168.0f},
        {86.0f, 2.0f, 136.0f},
        {255.0f, 204.0f, 0.0f},
        {255.0f, 0.0f, 0.0f},
        {0.0f, 255.0f, 0.0f},
        {255.0f, 123.0f, 251.0f},
        {233.0f, 89.0f, 226.0f},
        {200.0f, 60.0f, 192.0f},
        {168.0f, 30.0f, 158.0f},
        {136.0f, 2.0f, 125.0f},
        {255.0f, 255.0f, 0.0f},
        {157.0f, 155.0f, 0.0f},
        {155.0f, 65.0f, 14.0f},
        {180.0f, 255.0f, 0.0f},
        {255.0f, 255.0f, 0.0f},
        {0.0f, 47.0f, 181.0f},
        {191.0f, 191.0f, 191.0f},
        {108.0f, 0.0f, 0.0f}
    }
};

#define RadarMap_SetArrowLine(i, x1, y1, x2, y2) \
    radarmapGlobs.arrowLines[0][i].x = (x1); \
    radarmapGlobs.arrowLines[0][i].y = (y1); \
    radarmapGlobs.arrowLines[1][i].x = (x2); \
    radarmapGlobs.arrowLines[1][i].y = (y2)

#define RadarMap_SetColour(drawRect, c) \
    (drawRect)->r = radarmapGlobs.colourTable[c].r; \
    (drawRect)->g = radarmapGlobs.colourTable[c].g; \
    (drawRect)->b = radarmapGlobs.colourTable[c].b

static int radarmap_callback_draw_object(struct Lego_Object* liveObj,
                                         void* data);
static int radarmap_callback_try_select_object(struct Lego_Object* liveObj,
                                               void* data);

// FUNCTION: LEGORR 0x0045d8b0
void radarmap_initialise(void)
{
    uint32_t i, j;

    for (i = 0; i < RADARMAP_COLOURCOUNT; i++) {
        float* c = &radarmapGlobs.colourTable[i].r;
        for (j = 0; j < 3; j++) c[j] *= (1.0f / 255.0f);
    }
    for (i = 0; i < RADARMAP_MAXARROWLINES; i++) {
        radarmapGlobs.arrowLines[1][i].w = 1.0f;
        radarmapGlobs.arrowLines[0][i].w = 1.0f;
    }

    RadarMap_SetArrowLine(0, 320.0f, 40.0f, 420.0f, 180.0f);
    RadarMap_SetArrowLine(1, 420.0f, 180.0f, 360.0f, 180.0f);
    RadarMap_SetArrowLine(2, 360.0f, 180.0f, 360.0f, 400.0f);
    RadarMap_SetArrowLine(3, 360.0f, 400.0f, 280.0f, 400.0f);
    RadarMap_SetArrowLine(4, 280.0f, 400.0f, 280.0f, 180.0f);
    RadarMap_SetArrowLine(5, 280.0f, 180.0f, 220.0f, 180.0f);
    RadarMap_SetArrowLine(6, 220.0f, 180.0f, 320.0f, 40.0f);
    radarmapGlobs.arrowLineCount = 7;

    for (i = 0; i < radarmapGlobs.arrowLineCount; i++) {
        for (j = 0; j < 2; j++) {
            radarmapGlobs.arrowLines[j][i].x = ((float)display_width_get() * (1.0f / 640.0f)) * radarmapGlobs.arrowLines[j][i].x;
            radarmapGlobs.arrowLines[j][i].y = ((float)display_height_get() * (1.0f / 480.0f)) * radarmapGlobs.arrowLines[j][i].y;
        }
    }

    radarmapGlobs.highlightY = -1;
    radarmapGlobs.highlightX = -1;
}

// FUNCTION: LEGORR 0x0045da90
RadarMap* radarmap_create(struct TerrainMap* map, const Area2F* screenRect,
                          float zoom)
{
    RadarMap* radarMap;

    if (radarMap = (RadarMap*)malloc(sizeof(RadarMap))) {
        radarMap->screenRect = *screenRect;
        radarMap->zoom = zoom;
        radarMap->blockSize = terrain_context_scale_get(map);
        radarMap->map = map;
        return radarMap;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0045daf0
void radarmap_free(RadarMap* radarMap)
{
    free(radarMap);
}

// FUNCTION: LEGORR 0x0045db00
void radarmap_draw_survey_dot_circle(RadarMap* radarMap, const Point2F* wPos,
                                     float radius, float brightness)
{
    Area2F rect;
    Area2F prevWindow;
    float intensity;
    int r;

    rect.x = wPos->x;
    rect.y = wPos->y;
    rect.width = radius;
    rect.height = 0.0f;
    viewport_rectangle_get(&prevWindow);
    viewport_subrectangle_set(&radarMap->screenRect);
    radarmap_transform_rect(radarMap, &rect);
    intensity = brightness * 0.7f - -0.3f;
    r = (int)rect.width;
    surface_draw_circle_pixels((Point2F*)&rect, r, r * 2, intensity * 0.7f,
                               intensity * 0.9f, intensity, DRAW_EFFECT_NONE);
    viewport_subrectangle_set(&prevWindow);
}

// FUNCTION: LEGORR 0x0045dbc0
void radarmap_draw(RadarMap* radarMap, const Point2F* centrePos)
{
    Area2F prevWindow;
    Area2F rect;
    RadarMap_Point4F screenPts[8];
    RadarMap_Point3F worldPts[8];
    Point2F frustum[5];
    Point2F arrow[2][RADARMAP_MAXARROWLINES];
    RadarMap_Point3F arrowWorld[2][2][RADARMAP_MAXARROWLINES];
    RadarMap_Point3F planePoint, planeNormal, vec, endPoint;
    Point2F toList[RADARMAP_MAXOUTLINES];
    Point2F fromList[RADARMAP_MAXOUTLINES];
    DrawRect rectList[RADARMAP_MAXBLOCKRECTS];
    int startX, startY, endX, endY;
    int bx, by;
    uint32_t rectCount, lineCount;
    uint32_t i, j;
    DrawRect* drawRect;
    struct TerrainMap* map;

    /* vec holds the half extents in world units here, and each ray's
     * direction in the camera view below: the original reuses one
     * Vector3F (its .x and .y share the slots, finding 606). */
    radarMap->centrePos = *centrePos;
    rectCount = 0;
    lineCount = 0;
    startY = 0;
    startX = 0;
    vec.x = ((radarMap->screenRect.width * 0.5f) / radarMap->zoom) * radarMap->blockSize;
    vec.y = ((radarMap->screenRect.height * 0.5f) / radarMap->zoom) * radarMap->blockSize;
    terrain_map_world_to_block_clamped(radarMap->map, centrePos->x - vec.x,
                                       centrePos->y + vec.y, &startX, &startY);
    terrain_map_world_to_block_clamped(radarMap->map, centrePos->x + vec.x,
                                       centrePos->y - vec.y, &endX, &endY);

    viewport_rectangle_get(&prevWindow);
    viewport_subrectangle_set(&radarMap->screenRect);

    for (by = startY; by <= endY; by++) {
        for (bx = startX; bx <= endX; bx++) {
            drawRect = &rectList[rectCount];
            if (radarmap_get_block_colour(drawRect, bx, by)) {
                if (sim_status_banner_word_get() &&
                    bx == radarmapGlobs.highlightX && by == radarmapGlobs.highlightY) {
                    drawRect->r = 1.0f;
                    drawRect->g = 0.0f;
                    drawRect->b = 0.0f;
                }
                map = radarMap->map;
                drawRect->rect.x = (float)bx * radarMap->blockSize + map->worldX;
                drawRect->rect.y = -((float)by * radarMap->blockSize) + map->worldY;
                drawRect->rect.width = drawRect->rect.height = radarMap->blockSize;
                radarmap_transform_rect(radarMap, &drawRect->rect);
                drawRect->rect.width -= 1.0f;
                drawRect->rect.height -= 1.0f;

                if (terrain_cell_flag_20_bounded_current(bx, by) &&
                    lineCount < RADARMAP_MAXOUTLINES) {
                    fromList[lineCount].x = drawRect->rect.x;
                    fromList[lineCount].y = drawRect->rect.y;
                    toList[lineCount].x = (drawRect->rect.width - 1.0f) + drawRect->rect.x;
                    toList[lineCount].y = drawRect->rect.y;
                    lineCount++;
                    fromList[lineCount].x = (drawRect->rect.width - 1.0f) + drawRect->rect.x;
                    fromList[lineCount].y = drawRect->rect.y;
                    toList[lineCount].x = (drawRect->rect.width - 1.0f) + drawRect->rect.x;
                    toList[lineCount].y = (drawRect->rect.height - 1.0f) + drawRect->rect.y;
                    lineCount++;
                    fromList[lineCount].x = (drawRect->rect.width - 1.0f) + drawRect->rect.x;
                    fromList[lineCount].y = (drawRect->rect.height - 1.0f) + drawRect->rect.y;
                    toList[lineCount].x = drawRect->rect.x;
                    toList[lineCount].y = (drawRect->rect.height - 1.0f) + drawRect->rect.y;
                    lineCount++;
                    fromList[lineCount].x = drawRect->rect.x;
                    fromList[lineCount].y = (drawRect->rect.height - 1.0f) + drawRect->rect.y;
                    toList[lineCount].x = drawRect->rect.x;
                    toList[lineCount].y = drawRect->rect.y;
                    lineCount++;
                }
                rectCount++;
            }
        }
    }

    directdraw_clear(&radarMap->screenRect, 0);
    surface_fill_pixel_span_records(rectList, rectCount, DRAW_EFFECT_NONE);
    surface_draw_polyline(fromList, toList, lineCount,
                          radarmapGlobs.colourTable[RADARMAP_COLOUR_OUTLINE].r,
                          radarmapGlobs.colourTable[RADARMAP_COLOUR_OUTLINE].g,
                          radarmapGlobs.colourTable[RADARMAP_COLOUR_OUTLINE].b,
                          DRAW_EFFECT_NONE);

    radarMap->viewLeft = centrePos->x - vec.x;
    radarMap->viewTop = centrePos->y - vec.y;
    radarMap->viewRight = centrePos->x + vec.x;
    radarMap->viewBottom = centrePos->y + vec.y;
    radarMap->drawRectList = rectList;
    radarMap->drawRectCount = 0;
    legoobject_run_through_lists_skip_upgrade_parts(radarmap_callback_draw_object, radarMap);
    surface_fill_pixel_span_records(radarMap->drawRectList, radarMap->drawRectCount,
                                    DRAW_EFFECT_NONE);

    if (s_context_mode == 1) {
        screenPts[0].x = 0.0f;
        screenPts[0].y = 0.0f;
        screenPts[0].z = 0.0f;
        screenPts[0].w = 1.0f;
        screenPts[1].x = (float)display_width_get();
        screenPts[1].y = 0.0f;
        screenPts[1].z = 0.0f;
        screenPts[1].w = 1.0f;
        screenPts[2].x = (float)display_width_get();
        screenPts[2].y = (float)display_height_get();
        screenPts[2].z = 0.0f;
        screenPts[2].w = 1.0f;
        screenPts[3].x = 0.0f;
        screenPts[3].y = (float)display_height_get();
        screenPts[3].z = 0.0f;
        screenPts[3].w = 1.0f;
        screenPts[4].x = 0.0f;
        screenPts[4].y = 0.0f;
        screenPts[4].z = 1.0f;
        screenPts[4].w = 1.0f;
        screenPts[5].x = (float)display_width_get();
        screenPts[5].y = 0.0f;
        screenPts[5].z = 1.0f;
        screenPts[5].w = 1.0f;
        screenPts[6].x = (float)display_width_get();
        screenPts[6].y = (float)display_height_get();
        screenPts[6].z = 1.0f;
        screenPts[6].w = 1.0f;
        screenPts[7].x = 0.0f;
        screenPts[7].y = (float)display_height_get();
        screenPts[7].z = 1.0f;
        screenPts[7].w = 1.0f;

        planePoint.x = 0.0f;
        planePoint.y = 0.0f;
        planePoint.z = RADARMAP_LEGO_GET_MAP()->lowestZ * 0.5f;
        planeNormal.x = 0.0f;
        planeNormal.y = 0.0f;
        planeNormal.z = -1.0f;

        for (i = 0; i < 8; i++) {
            camera_screen_coordinate_project(
                (const struct ViewProjectionAdapter*)legoGlobs_viewMain,
                (float*)&worldPts[i], (const float*)&screenPts[i]);
        }

        for (j = 0; j < 2; j++) {
            for (i = 0; i < radarmapGlobs.arrowLineCount; i++) {
                if (j == 0) {
                    radarmapGlobs.arrowLines[1][i].z = 0.0f;
                    radarmapGlobs.arrowLines[0][i].z = 0.0f;
                } else {
                    radarmapGlobs.arrowLines[1][i].z = 1.0f;
                    radarmapGlobs.arrowLines[0][i].z = 1.0f;
                }
                camera_screen_coordinate_project(
                    (const struct ViewProjectionAdapter*)legoGlobs_viewMain,
                    (float*)&arrowWorld[j][0][i],
                    (const float*)&radarmapGlobs.arrowLines[0][i]);
                camera_screen_coordinate_project(
                    (const struct ViewProjectionAdapter*)legoGlobs_viewMain,
                    (float*)&arrowWorld[j][1][i],
                    (const float*)&radarmapGlobs.arrowLines[1][i]);
            }
        }

        for (j = 0; j < 2; j++) {
            for (i = 0; i < radarmapGlobs.arrowLineCount; i++) {
                vec.x = arrowWorld[1][j][i].x - arrowWorld[0][j][i].x;
                vec.y = arrowWorld[1][j][i].y - arrowWorld[0][j][i].y;
                vec.z = arrowWorld[1][j][i].z - arrowWorld[0][j][i].z;
                view_ray_intersection_test(&endPoint.x, &arrowWorld[0][j][i].x, &vec.x,
                                           &planePoint.x, &planeNormal.x);
                rect.x = endPoint.x;
                rect.y = endPoint.y;
                radarmap_transform_rect(radarMap, &rect);
                arrow[j][i].x = rect.x;
                arrow[j][i].y = rect.y;
            }
        }
        surface_draw_polyline(arrow[0], arrow[1], radarmapGlobs.arrowLineCount,
                              1.0f, 1.0f, 1.0f, DRAW_EFFECT_HALF_TRANS);

        for (i = 0; i < 4; i++) {
            vec.x = worldPts[i + 4].x - worldPts[i].x;
            vec.y = worldPts[i + 4].y - worldPts[i].y;
            vec.z = worldPts[i + 4].z - worldPts[i].z;
            view_ray_intersection_test(&endPoint.x, &worldPts[i].x, &vec.x,
                                       &planePoint.x, &planeNormal.x);
            rect.x = endPoint.x;
            rect.y = endPoint.y;
            radarmap_transform_rect(radarMap, &rect);
            frustum[i].x = rect.x;
            frustum[i].y = rect.y;
        }
        frustum[4] = frustum[0];
        surface_draw_polyline(&frustum[0], &frustum[1], 4, 0.7f, 0.7f, 0.7f,
                              DRAW_EFFECT_HALF_TRANS);
    }

    viewport_subrectangle_set(&prevWindow);
}

// FUNCTION: LEGORR 0x0045e400
int radarmap_can_show_object(struct Lego_Object* liveObj)
{
    int type = liveObj->type;

    if (type != 14 && type != 5 && type != 8 && type != 13 && type != 16 &&
        type != 17 && type != 19 && type != 9 && type != 18 && type != 4 &&
        type != 12) {
        return (~entity_state_flags_get((const struct EntitySlot*)liveObj) >> 29) & 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0045e460
static int radarmap_callback_draw_object(struct Lego_Object* liveObj, void* data)
{
    RadarMap* radarMap = (RadarMap*)data;
    Point2F wPos;
    DrawRect* drawRect;
    float size;

    if (!radarmap_can_show_object(liveObj)) return 0;

    legoobject_transform_position_get(liveObj, &wPos.x, &wPos.y);
    if (wPos.x >= radarMap->viewLeft && wPos.x <= radarMap->viewRight &&
        wPos.y >= radarMap->viewTop && wPos.y <= radarMap->viewBottom) {
        if (liveObj->type == 3) {
            RadarMap_SetColour(&radarMap->drawRectList[radarMap->drawRectCount],
                               RADARMAP_COLOUR_OBJECT_TYPE3);
            size = 1.0f / 3.0f;
        } else if (liveObj->type == 6 || liveObj->type == 7) {
            RadarMap_SetColour(&radarMap->drawRectList[radarMap->drawRectCount],
                               RADARMAP_COLOUR_RESOURCE);
            size = 0.25f;
        } else {
            RadarMap_SetColour(&radarMap->drawRectList[radarMap->drawRectCount],
                               RADARMAP_COLOUR_OBJECT);
            size = 1.0f / 3.0f;
        }
        radarMap->drawRectList[radarMap->drawRectCount].rect.x =
            wPos.x - (size * 0.5f) * radarMap->blockSize;
        radarMap->drawRectList[radarMap->drawRectCount].rect.y =
            wPos.y + (size * 0.5f) * radarMap->blockSize;
        radarMap->drawRectList[radarMap->drawRectCount].rect.width =
            size * radarMap->blockSize;
        radarMap->drawRectList[radarMap->drawRectCount].rect.height =
            size * radarMap->blockSize;
        drawRect = &radarMap->drawRectList[radarMap->drawRectCount];
        radarmap_transform_rect(radarMap, &drawRect->rect);
        radarMap->drawRectCount++;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0045e660
int radarmap_inside_radar(RadarMap* radarMap, uint32_t screenX, uint32_t screenY)
{
    if ((float)screenX >= radarMap->screenRect.x &&
        (float)screenX < radarMap->screenRect.x + radarMap->screenRect.width &&
        (float)screenY >= radarMap->screenRect.y &&
        (float)screenY < radarMap->screenRect.y + radarMap->screenRect.height) {
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0045e6d0
int radarmap_screen_to_block_pos(RadarMap* radarMap, uint32_t screenX,
                                 uint32_t screenY, Point2F* outWorldPos,
                                 int* outBx, int* outBy)
{
    float x = (float)screenX;
    float y;
    Point2F wPos;
    float scale;

    if (x >= radarMap->screenRect.x &&
        x < radarMap->screenRect.x + radarMap->screenRect.width) {
        y = (float)screenY;
        if (y >= radarMap->screenRect.y &&
            y < radarMap->screenRect.y + radarMap->screenRect.height) {
            scale = 1.0f / radarMap->zoom;
            wPos.y = radarMap->blockSize *
                     ((radarMap->screenRect.y + radarMap->screenRect.width * 0.5f) - y) *
                     scale + radarMap->centrePos.y;
            wPos.x = radarMap->centrePos.x -
                     radarMap->blockSize *
                     ((radarMap->screenRect.x + radarMap->screenRect.width * 0.5f) - x) *
                     scale;
            if (terrain_map_world_to_block(radarMap->map, wPos.x, wPos.y, outBx, outBy)) {
                if (outWorldPos) *outWorldPos = wPos;
                return 1;
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0045e820
int radarmap_try_select_object(RadarMap* radarMap, uint32_t screenX,
                               uint32_t screenY, struct Lego_Object** outObj,
                               Point2F* outObjPos)
{
    RadarMap_SearchObject search;
    RadarMap_Point2I blockPos;

    search.wPos.x = 0.0f;
    search.wPos.y = 0.0f;
    search.radius = 0.0f;
    search.result = NULL;
    if (radarmap_screen_to_block_pos(radarMap, screenX, screenY, &search.wPos,
                                     &blockPos.x, &blockPos.y)) {
        if (terrain_cell_occupied_or_flag_current(blockPos.x, blockPos.y, 1)) {
            search.radius = radarMap->blockSize;
        } else {
            search.radius = radarMap->blockSize * (1.0f / 6.0f);
        }
        if (legoobject_run_through_lists_skip_upgrade_parts(
                radarmap_callback_try_select_object, &search)) {
            *outObj = search.result;
            if (outObjPos) {
                legoobject_transform_position_get(search.result, &outObjPos->x,
                                                  &outObjPos->y);
            }
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0045e8e0
static int radarmap_callback_try_select_object(struct Lego_Object* liveObj,
                                               void* data)
{
    RadarMap_SearchObject* search = (RadarMap_SearchObject*)data;
    Point2F wPos;

    legoobject_transform_position_get(liveObj, &wPos.x, &wPos.y);
    if (fabs(wPos.x - search->wPos.x) <= search->radius &&
        fabs(wPos.y - search->wPos.y) <= search->radius) {
        search->result = liveObj;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0045e940
void radarmap_transform_rect(RadarMap* radarMap, Area2F* rect)
{
    Point2F centre;

    centre.x = (radarMap->screenRect.width / 2) + radarMap->screenRect.x;
    centre.y = (radarMap->screenRect.height / 2) + radarMap->screenRect.y;
    rect->x -= radarMap->centrePos.x;
    rect->y -= radarMap->centrePos.y;
    rect->x *= radarMap->zoom / radarMap->blockSize;
    rect->y *= radarMap->zoom / radarMap->blockSize;
    rect->width *= radarMap->zoom / radarMap->blockSize;
    rect->x += centre.x;
    rect->height *= radarMap->zoom / radarMap->blockSize;
    rect->y = centre.y - rect->y;
}

// FUNCTION: LEGORR 0x0045e9e0
int radarmap_get_block_colour(DrawRect* drawRect, int bx, int by)
{
    int blockPos[2];
    uint32_t surface;
    Lego_Block* block;

    blockPos[0] = bx;
    blockPos[1] = by;
    terrain_cell_type_get_at_current(bx, by, &surface);

    if (terrain_cell_walkable_flag_current(bx, by)) {
        if (terrain_cell_flag_wave_current(bx, by)) {
            switch (surface) {
            case 1: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_REINFORCED + 4); return 1;
            case 2: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_REINFORCED + 3); return 1;
            case 3: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_REINFORCED + 2); return 1;
            case 4: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_REINFORCED + 1); return 1;
            case 5: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_REINFORCED); return 1;
            }
        } else {
            switch (surface) {
            case 1: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_GROUND + 4); return 1;
            case 2: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_GROUND + 3); return 1;
            case 3: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_GROUND + 2); return 1;
            case 4: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_GROUND + 1); return 1;
            case 5: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_GROUND); return 1;
            case 8: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_ORESEAM); return 1;
            case 10: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_CRYSTALSEAM); return 1;
            case 11: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_RECHARGESEAM); return 1;
            }
        }
    } else if (terrain_cell_mask18_clear_current(bx, by) &&
               level_cell_flag_4_current(bx, by)) {
        block = &LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
                    [legoGlobs_currLevel->width * by + bx];
        if (block->flags1 & 0x20000) {
            RadarMap_SetColour(drawRect, RADARMAP_COLOUR_FLAG20000);
            return 1;
        }
        switch (surface) {
        case 1: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_WALL + 4); return 1;
        case 2: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_WALL + 3); return 1;
        case 3: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_WALL + 2); return 1;
        case 4: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_WALL + 1); return 1;
        case 5: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_WALL); return 1;
        case 7: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_WATER); return 1;
        case 9: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_LAKE); return 1;
        case 8: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_ORESEAM); return 1;
        case 10: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_CRYSTALSEAM); return 1;
        case 11: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_RECHARGESEAM); return 1;
        case 6: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_LAVA); return 1;
        }
    } else if (terrain_cell_flag_8_bounded_current(bx, by)) {
        switch (surface) {
        case 6: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_LAVA); return 1;
        case 7: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_WATER); return 1;
        case 9: RadarMap_SetColour(drawRect, RADARMAP_COLOUR_LAKE); return 1;
        default:
            block = &LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
                        [legoGlobs_currLevel->width * by + bx];
            if (block->flags1 & 0x100000) {
                RadarMap_SetColour(drawRect, RADARMAP_COLOUR_FLAG100000);
                return 1;
            }
            if (level_cell_occupied_at_current(blockPos)) {
                if (lego_terrain_cell_secondary_flag_100(blockPos)) {
                    RadarMap_SetColour(drawRect, RADARMAP_COLOUR_POWERED);
                    return 1;
                }
                RadarMap_SetColour(drawRect, RADARMAP_COLOUR_UNPOWERED);
                return 1;
            }
            RadarMap_SetColour(drawRect, RADARMAP_COLOUR_UNKNOWN);
            return 1;
        }
    } else {
        return 0;
    }
    return 1;
}

#ifdef __cplusplus
}
#endif
