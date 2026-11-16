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

struct Lego_Level;
struct Lego_Object;

struct Lego_Level* level_current_owner_get(void); /* 00431560 Lego_GetLevel */
/* 00431570 Lego_GetMap takes no arguments. */
void* level_current_terrain_get(void);          /* 00431570 */
#define EFFECT_LEGO_GET_MAP() \
    ((TerrainMap*)level_current_terrain_get())

/* GAME-023 LegoObject.c (legoobject_unit.h, entity.hpp). */
void legoobject_spatial_callback_query_at_optional_coordinate(
    struct Lego_Object* obj, float* position, float radius);     /* 004408c0 */
void legoobject_transform_position_get(struct Lego_Object* obj, float* outX,
                                       float* outY);             /* 00442260 */
/* In the matching build 0044a200 is legoobject_unit.c's (GAME-023 part 3,
 * issue #615); one declaration either way. */
#define entity_type_component_get legoobject_type_component_get
void* entity_type_component_get(const void* entity_record);      /* 0044a200 */

/* gods98 Container.c (view.hpp). */
void ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]);             /* 004754c0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_ROCK_FALL_STYLES_IMPORTS_H */
