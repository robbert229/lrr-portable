// SPDX-License-Identifier: LGPL-3.0-or-later
/* Building.c (original translation unit GAME-006, finding 505): the
 * building models, in original shape (finding 592). C boundary header
 * (ADR 0005 step 6): only C units and src/game/test_c_boundaries.cpp
 * include it.
 *
 * The unit's functions are declared by their own names. In the matching
 * build building_model_link_names.h gives most of them the port's names
 * inside building_model.c, so callers that use the port's names reach the
 * unit there. The port name the electric fences (GAME-013, finding 566)
 * call is declared in building_model_ports.h. */
#ifndef LEGORRR_GAME_BUILDING_MODEL_H
#define LEGORRR_GAME_BUILDING_MODEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct BuildingModel;
struct TerrainMap;

/* The terrain height callback Building_SetPosition samples (Map3D's
 * world z). */
typedef float (*BuildingModel_GetWorldZCallback)(float xPos, float yPos,
                                                 struct TerrainMap* map);

int building_load(struct BuildingModel* building, unsigned int objID,
                  void* root, const char* filename,
                  const char* gameName);                          /* 00407d60 */
void building_change_power_level(struct BuildingModel* building,
                                 int increment);                  /* 004082e0 */
void* building_get_shape_points(struct BuildingModel* building,
                                unsigned int* count);             /* 00408360 */
void building_set_owner_object(struct BuildingModel* building,
                               void* liveObj);                    /* 00408380 */
void building_set_upgrade_activity(struct BuildingModel* building,
                                   const char* activityName);     /* 004083a0 */
unsigned int building_set_activity(struct BuildingModel* building,
                                   const char* activityName,
                                   float time);                   /* 00408570 */
void* building_get_camera_null(struct BuildingModel* building,
                               unsigned int frameNo);             /* 004085f0 */
void building_clone(struct BuildingModel* srcBuilding,
                    struct BuildingModel* destBuilding);          /* 00408620 */
void building_hide(struct BuildingModel* building, int hide);     /* 00408670 */
unsigned int building_is_hidden(struct BuildingModel* building);  /* 004086a0 */
void building_set_orientation(struct BuildingModel* building,
                              float xDir, float yDir);            /* 004086c0 */
void building_set_position(struct BuildingModel* building, float xPos,
                           float yPos,
                           BuildingModel_GetWorldZCallback zCallback,
                           struct TerrainMap* surfMap);           /* 00408710 */
void* building_get_carry_null(struct BuildingModel* building,
                              unsigned int frameNo);              /* 00408770 */
void* building_get_deposit_null(struct BuildingModel* building);  /* 004087b0 */
void* building_get_entrance_null(struct BuildingModel* building); /* 004087e0 */
void* building_get_tool_null(struct BuildingModel* building,
                             unsigned int frameNo);               /* 00408810 */
unsigned int building_get_carry_null_frames(struct BuildingModel* building); /* 00408850 */
float building_move_animation(struct BuildingModel* building,
                              float elapsed,
                              unsigned int repeats);              /* 00408860 */
float building_get_trans_coef(struct BuildingModel* building);    /* 00408930 */
void building_remove(struct BuildingModel* building);             /* 00408940 */
unsigned int building_can_upgrade_type(struct BuildingModel* building,
                                       unsigned int upgradeType,
                                       int current);              /* 00408970 */
void building_set_upgrade_level(struct BuildingModel* building,
                                unsigned int objLevel);           /* 004089a0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_BUILDING_MODEL_H */
