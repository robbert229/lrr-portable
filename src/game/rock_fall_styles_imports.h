// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-012 (the effects unit, rock_fall_styles.c) imports: C declarations
 * of the callees and globals the unit uses from other modules (ADR 0005
 * step 6; finding 589). Included only by rock_fall_styles.c and
 * test_c_boundaries.cpp, so the other C units' symbol numbering does not
 * change (finding 568, "Unit-private imports header"). Each declaration
 * names a definition with C linkage, with the port's parameter types;
 * rock_fall_styles.c casts its original-layout records to them. */
#ifndef LEGORRR_GAME_ROCK_FALL_STYLES_IMPORTS_H
#define LEGORRR_GAME_ROCK_FALL_STYLES_IMPORTS_H

#include "gods98/config.h"
#include "gods98/container.h"
#include "gods98/entity.h"
#include "gods98/record.h"
#include "gods98/texture.h"
#include "gods98/utils.h"
#include "gods98/view.h"
#include "terrain_map.h"

#ifdef __cplusplus
extern "C" {
#endif

struct LevelState;
struct Lego_Object;

/* GAME-021 Lego.c (lego_unit.h, lego.h). The three containers and the
 * radius are legoGlobs fields, commons in the original. */
extern void* legoGlobs_contBoulderExplode;        /* 0055636c */
extern void* legoGlobs_contBoulderExplodeIce;     /* 00556370 */
extern void* legoGlobs_contSmashPath;             /* 005563ac */
extern float legoGlobs_BirdScarerRadius;          /* 00556470 */
struct LevelState* level_current_owner_get(void); /* 00431560 Lego_GetLevel */
/* 00431570 Lego_GetMap takes no arguments. lego_unit.c defines it with the
 * port's two ignored cell arguments, so the C matching build calls it
 * through an unprototyped function type, which pushes none, as the original
 * does; the native C++ compile goes through lego_port.cpp's adapter (as
 * weapons_imports.h). */
void* level_current_terrain_get(int cell_x, int cell_y);          /* 00431570 */
void* level_current_terrain_get_original(void);
#ifdef __cplusplus
#define EFFECT_LEGO_GET_MAP() \
    ((TerrainMap*)level_current_terrain_get_original())
#else
#define EFFECT_LEGO_GET_MAP() \
    ((TerrainMap*)((void* (*)())level_current_terrain_get)())
#endif

/* GAME-023 LegoObject.c (legoobject_unit.h, entity.hpp). */
void legoobject_spatial_callback_query_at_optional_coordinate(
    struct Lego_Object* obj, float* position, float radius);     /* 004408c0 */
void legoobject_transform_position_get(struct Lego_Object* obj, float* outX,
                                       float* outY);             /* 00442260 */
void* entity_type_component_get(const void* entity_record);      /* 0044a200 */

/* gods98 Container.c (view.hpp). */
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]);             /* 004754c0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_ROCK_FALL_STYLES_IMPORTS_H */
