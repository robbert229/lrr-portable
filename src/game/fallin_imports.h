// SPDX-License-Identifier: LGPL-3.0-or-later
/* Fallin.c (GAME-051) imports: C declarations of the callees and globals
 * the unit uses from other modules (ADR 0005 step 6; finding 599). Included
 * only by fallin.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names a definition with C linkage, with the parameter
 * types the other C units' imports headers give it. */
#ifndef LEGORRR_GAME_FALLIN_IMPORTS_H
#define LEGORRR_GAME_FALLIN_IMPORTS_H

#include "gods98/maths.h"
#include "lego_unit.h"
#include "terrain_grid.h"
#include "terrain_map.h"

#ifdef __cplusplus
extern "C" {
#endif

struct InfoPoint2I;

/* GAME-012 the effects unit: Effect_Spawn_RockFall (0040c230), by the
 * port's name, which rock_fall_styles.c's definition takes in the matching
 * build. */
int scene_slot_pose_scale_register(int group, uint32_t first, uint32_t second,
                                   float x, float y, float z,
                                   float heading, float pitch);    /* 0040c230 */
/* GAME-019 Info.c (info_unit.h). */
void info_send(int type, char* text, void* object,
               struct InfoPoint2I* blockPos);                      /* 00419b80 */
/* GAME-021 Lego.c (lego_unit.c; lego.cpp's models natively). */
uint32_t terrain_cell_flag_8_bounded_current(uint32_t cx, uint32_t cy); /* 00432b50 */
int terrain_cell_walkable_flag_current(uint32_t cx, uint32_t cy);  /* 00432bd0 */
int terrain_cell_state_update_current(const int cell_xy[2]);       /* 00432c90 */
uint32_t terrain_cell_flag_20_bounded_current(uint32_t cx, uint32_t cy); /* 00432d90 */
uint32_t terrain_cell_flag_reserved_current(const int cell_xy[2]); /* 00432dd0 */
void level_tile_mark_current_target(const int cell_xy[2], int set); /* 00432e00 */
uint32_t terrain_cell_mask_c0_current(int cx, int cy);             /* 00432e60 */
uint32_t terrain_cell_edge_wall_current(const int cell_xy[2]);     /* 00432e90 */
int terrain_cell_state_equals_one_current(const int cell_xy[2]);   /* 00432ec0 */
/* 00431570 Lego_GetMap takes no arguments. lego_unit.c defines it with the
 * port's two ignored cell arguments, so the C matching build calls it
 * through an unprototyped function type, which pushes none, as the original
 * does; the native C++ compile goes through lego_port.cpp's adapter (as
 * rock_fall_styles_imports.h). */
void* level_current_terrain_get_original(void);
#ifdef __cplusplus
#define FALLIN_LEGO_GET_MAP() \
    ((struct TerrainMap*)level_current_terrain_get_original())
#else
#define FALLIN_LEGO_GET_MAP() \
    ((struct TerrainMap*)((void* (*)())level_current_terrain_get)())
#endif
/* GAME-040 SFX.c: SFX_Random_PlaySound3DOnContainer (00465050), by the
 * port's name, which sfx.c's definition takes in the matching build. */
int sound_submission_route_wrapper(const void* wrapped_position, int alias,
                                   int loop, int positional,
                                   const float* xyz);              /* 00465050 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_FALLIN_IMPORTS_H */
