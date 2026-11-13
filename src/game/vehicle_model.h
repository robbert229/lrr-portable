// SPDX-License-Identifier: LGPL-3.0-or-later
/* Vehicle.c (original translation unit GAME-048, finding 505): the vehicle
 * models. Each is loaded from its `.ae` config (activity containers, wheel
 * mesh and nulls, named nulls, weapon pivots, upgrades and level-of-detail
 * meshes), cloned per object, and placed on the terrain with its wheels
 * (finding 590).
 *
 * C-compatible header (ADR 0005). vehicle_model.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * includes it.
 *
 * In the matching build vehicle_model_link_names.h, which vehicle_model.c
 * includes first, gives most of these functions the port's names
 * (record.hpp, resource.hpp, lego_unit_imports.h), so every caller reaches
 * them. Natively they keep these names and only vehicle_set_position has a
 * caller (legoobject_unit.c, itself unreached natively): the port tail
 * vehicle_model_port.cpp serves the port's names. */
#ifndef LEGORRR_GAME_VEHICLE_MODEL_H
#define LEGORRR_GAME_VEHICLE_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

struct VehicleModel;
struct TerrainMap;

/* The terrain height callback Vehicle_SetPosition samples (Map3D's
 * GetWorldZ in the original; the port names Map3D TerrainMap). */
typedef float (*VehicleModel_GetWorldZCallback)(float xPos, float yPos,
                                                struct TerrainMap* map);

int vehicle_load(struct VehicleModel* vehicle, unsigned int objID,
                 void* root, const char* filename,
                 const char* gameName);                           /* 0046c6f0 */
void vehicle_remove(struct VehicleModel* vehicle);                /* 0046ce10 */
void vehicle_swap_poly_medium(struct VehicleModel* vehicle,
                              int swap);                          /* 0046ced0 */
void vehicle_swap_poly_high(struct VehicleModel* vehicle,
                            int swap);                            /* 0046cf20 */
unsigned int vehicle_can_upgrade_type(struct VehicleModel* vehicle,
                                      unsigned int upgradeType,
                                      int current);               /* 0046cf40 */
unsigned int vehicle_get_upgrade_level(struct VehicleModel* vehicle); /* 0046cf80 */
void vehicle_set_upgrade_level(struct VehicleModel* vehicle,
                               unsigned int objLevel);            /* 0046cf90 */
void vehicle_hide_wheels(struct VehicleModel* vehicle, int hide); /* 0046cfc0 */
int vehicle_clone(struct VehicleModel* srcVehicle,
                  struct VehicleModel* destVehicle);              /* 0046cff0 */
void vehicle_set_owner_object(struct VehicleModel* vehicle,
                              void* liveObj);                     /* 0046d140 */
float vehicle_get_animation_time(struct VehicleModel* vehicle);   /* 0046d1a0 */
float vehicle_move_animation(struct VehicleModel* vehicle,
                             float elapsed1, float elapsed2,
                             unsigned int repeats);               /* 0046d1c0 */
void vehicle_populate_wheel_nulls(struct VehicleModel* vehicle);  /* 0046d260 */
void vehicle_hide(struct VehicleModel* vehicle, int hide);        /* 0046d2c0 */
unsigned int vehicle_is_hidden(struct VehicleModel* vehicle);     /* 0046d330 */
void vehicle_set_orientation(struct VehicleModel* vehicle, float xDir,
                             float yDir, float zDir);             /* 0046d350 */
void vehicle_set_position(struct VehicleModel* vehicle, float xPos,
                          float yPos,
                          VehicleModel_GetWorldZCallback zCallback,
                          struct TerrainMap* surfMap);            /* 0046d380 */
void* vehicle_get_activity_container(struct VehicleModel* vehicle); /* 0046d9e0 */
void* vehicle_find_null(struct VehicleModel* vehicle, const char* name,
                        unsigned int frameNo);                    /* 0046d9f0 */
void* vehicle_get_camera_null(struct VehicleModel* vehicle,
                              unsigned int frameNo);              /* 0046da50 */
void* vehicle_get_drill_null(struct VehicleModel* vehicle);       /* 0046da90 */
void* vehicle_get_deposit_null(struct VehicleModel* vehicle);     /* 0046dac0 */
void* vehicle_get_driver_null(struct VehicleModel* vehicle);      /* 0046daf0 */
void* vehicle_get_carry_null(struct VehicleModel* vehicle,
                             unsigned int frameNo);               /* 0046db20 */
unsigned int vehicle_get_carry_null_frames(struct VehicleModel* vehicle); /* 0046db60 */
float vehicle_get_trans_coef(struct VehicleModel* vehicle);       /* 0046db70 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_VEHICLE_MODEL_H */
