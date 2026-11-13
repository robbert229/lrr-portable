// SPDX-License-Identifier: LGPL-3.0-or-later
/* RadarMap.c (GAME-036) imports: C declarations of the callees and globals
 * the unit uses from other modules (ADR 0005 step 6; finding 606). Included
 * only by radarmap.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names a definition with C linkage, by the name it has in
 * the matching build, with the parameter types the other C units' imports
 * headers give it. */
#ifndef LEGORRR_GAME_RADARMAP_IMPORTS_H
#define LEGORRR_GAME_RADARMAP_IMPORTS_H

#include <stdint.h>

#include "gods98/draw.h"
#include "gods98/maths.h"
#include "lego_unit.h"
#include "legoobject_unit.h"
#include "terrain_grid.h"
#include "terrain_map.h"

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;
struct LevelState;

/* Main.c (advisor.c): Main_appWidth and Main_appHeight. */
int display_width_get(void);                                     /* 00401c30 */
int display_height_get(void);                                    /* 00401c40 */

/* Interface.c (interface_unit.c): the radar map's block highlight is shown
 * only while this is set. */
uint32_t sim_status_banner_word_get(void);                       /* 0041b9b0 */

/* Lego.c (lego_unit.c): legoGlobs.currLevel (005560d4, terrain_grid.h),
 * legoGlobs.viewMain (005560dc, lego_unit.h), legoGlobs.viewMode (00556ec8)
 * and the block queries. */
extern int32_t s_context_mode;                                   /* 00556ec8 */
uint32_t terrain_cell_occupied_or_flag_current(int cx, int cy, int mode); /* 00432aa0 */
uint32_t terrain_cell_flag_8_bounded_current(uint32_t cx, uint32_t cy); /* 00432b50 */
int terrain_cell_walkable_flag_current(uint32_t cx, uint32_t cy);  /* 00432bd0 */
uint32_t terrain_cell_flag_wave_current(int cx, int cy);         /* 00432c20 */
uint32_t terrain_cell_flag_20_bounded_current(uint32_t cx, uint32_t cy); /* 00432d90 */
int terrain_cell_mask18_clear_current(unsigned int cx,
                                      unsigned int cy);          /* 00432f40 */
unsigned int level_cell_flag_4_current(unsigned int cx,
                                       unsigned int cy);         /* 00432f90 */
uint32_t level_cell_occupied_at_current(const int cell_xy[2]);   /* 00433090 */
int terrain_cell_type_get_at_current(uint32_t cx, uint32_t cy,
                                     uint32_t* out_type);        /* 00433330 */
/* 00431570 Lego_GetMap takes no arguments. lego_unit.c defines it with the
 * port's two ignored cell arguments, so the C matching build calls it
 * through an unprototyped function type, which pushes none, as the original
 * does; the native C++ compile goes through lego_port.cpp's adapter (as
 * fallin_imports.h). */
void* level_current_terrain_get_original(void);
#ifdef __cplusplus
#define RADARMAP_LEGO_GET_MAP() \
    ((struct TerrainMap*)level_current_terrain_get_original())
#else
#define RADARMAP_LEGO_GET_MAP() \
    ((struct TerrainMap*)((void* (*)())level_current_terrain_get)())
#endif

/* Stats.c (entity_stats.c), by its link name: StatsObject_GetStatsFlags2. */
uint32_t entity_state_flags_get(const struct EntitySlot* e);     /* 00469e80 */

/* Viewports.c (viewports.c), by its link name: Viewport_InverseTransform
 * (the screen point, a Point4F, to a world Point3F). */
struct ViewProjectionAdapter;
void camera_screen_coordinate_project(
    const struct ViewProjectionAdapter* adapter, float* output,
    const float* input);                                         /* 00477290 */

/* DirectDraw.c (directdraw.c): DirectDraw_Clear. */
void directdraw_clear(struct Area2F* window, unsigned int colour); /* 0047ce20 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_RADARMAP_IMPORTS_H */
