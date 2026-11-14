// SPDX-License-Identifier: LGPL-3.0-or-later
/* Vehicle.c, original translation unit GAME-048 (finding 505), rebuilt in
 * original shape (ADR 0005; findings 590 and 617): text 0046c3d0-0046db90,
 * .rdata 0049e7e4-0049e7f0, literals 004aa9f8-004aaa2c. The unit has no
 * .bss. Its first three functions (0046c3d0-0046c510), which set a
 * vehicle's activity and hang its upgrade parts on their nulls, were in the
 * unit map's GAME-047 until finding 617.
 *
 * Each vehicle type's model is loaded once from its `.ae` config: one or
 * two activity containers, the wheel mesh and its nulls, the named nulls
 * (camera, carry, drill, deposit, driver, fire, pivots), the upgrades and
 * the medium-detail meshes. Every vehicle object clones that model, and
 * Vehicle_SetPosition places the clone on the terrain: it tilts the body to
 * the plane of the first three wheels, drops each wheel onto the ground and
 * turns it by the distance it rolled.
 *
 * The port's implementation stays in the port tail vehicle_model_port.cpp,
 * unannotated. In the matching build vehicle_model_link_names.h gives this
 * unit's functions the port's names, so every caller reaches them; natively
 * the port tail serves those names. upgrade_link_names.h does the same for
 * the Upgrade.c functions the unit calls. */
#include "vehicle_model_link_names.h"
#include "upgrade_link_names.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "vehicle_model.h"
#include "vehicle_model_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct Vehicle_Vector3F {
    float x, y, z;
} Vehicle_Vector3F;

#include "vehicle_model_maths.h"

#define VEHICLE_MAXWHEELS 6
#define VEHICLE_MAXCARRYNULLS 10
#define VEHICLE_MAXCAMERANULLS 4
#define VEHICLE_MAXWEAPONS 3
#define VEHICLE_MAXFIRES 2

#define VEHICLE_FLAG_SOURCE 0x1      /* owns the null names (Vehicle_Load) */
#define VEHICLE_FLAG_HIDDEN 0x2
#define VEHICLE_FLAG_NOACTIVITY1 0x4 /* contAct1's animation is not moved */
#define VEHICLE_FLAG_HOLDMISSING 0x8

/* Config's "" (004b8a14), which config.c defines (finding 581). */
extern const char s_config_angle_empty[4];

/* The part of a Container the unit reads: its type (+0x10; 1 is a mesh). */
typedef struct Vehicle_Container {
    unsigned char pad_00[0x10];
    int type;                                     /* +0x10 */
} Vehicle_Container;

/* One part of an upgrade level (Upgrade.c, GAME-047; 0x14 bytes). */
typedef struct Vehicle_UpgradePartInfo {
    unsigned int level;                           /* +0x00 */
    unsigned int nullFrameNo;                     /* +0x04 */
    char* nullName;                               /* +0x08 */
    char* weaponName;                             /* +0x0c */
    void* upgradeData;                            /* +0x10 */
} Vehicle_UpgradePartInfo;

/* An upgrade part's model (Upgrade.c; 0x10 bytes), each upgrade-part
 * LegoObject's clone. */
typedef struct Vehicle_UpgradePart {
    unsigned int objID;                           /* +0x00 */
    void* cont;                                   /* +0x04 */
    struct Vehicle_PartObject* nextObject;        /* +0x08 */
    Vehicle_UpgradePartInfo* partInfo;            /* +0x0c */
} Vehicle_UpgradePart;

/* The part of a LegoObject the unit reads: its upgrade part (+0x20). */
typedef struct Vehicle_PartObject {
    unsigned char pad_00[0x20];
    Vehicle_UpgradePart* upgradePart;             /* +0x20 */
} Vehicle_PartObject;

/* Upgrade.c's model (GAME-047), embedded in the vehicle model at +0x124. */
typedef struct Vehicle_Upgrades {
    Vehicle_UpgradePartInfo* parts;               /* +0x00 */
    unsigned int count;                           /* +0x04 */
    unsigned int currentLevel;                    /* +0x08 */
    unsigned int levels;                          /* +0x0c a bit per type */
    Vehicle_PartObject* objectList;               /* +0x10 the shown parts */
} Vehicle_Upgrades;

/* The original's VehicleModel, 0x1ec bytes. */
typedef struct VehicleModel {
    unsigned int objID;                           /* +0x000 */
    char* wheelNullName;                          /* +0x004 */
    void* contAct1;                               /* +0x008 */
    void* contAct2;                               /* +0x00c */
    void* contWheels[VEHICLE_MAXWHEELS];          /* +0x010 */
    void* wheelNulls[VEHICLE_MAXWHEELS];          /* +0x028 */
    unsigned int wheelNullsCount;                 /* +0x040 */
    unsigned char pad_044[0x18];                  /* +0x044 */
    Vehicle_Vector3F lastUp;                      /* +0x05c */
    float wheelRadius;                            /* +0x068 */
    Vehicle_Vector3F wheelLastPos[VEHICLE_MAXWHEELS]; /* +0x06c */
    char* drillNullName;                          /* +0x0b4 */
    char* depositNullName;                        /* +0x0b8 */
    char* fireNullName;                           /* +0x0bc */
    char* driverNullName;                         /* +0x0c0 */
    char* yPivotNullName;                         /* +0x0c4 */
    char* xPivotNullName;                         /* +0x0c8 */
    void* drillNull;                              /* +0x0cc */
    void* depositNull;                            /* +0x0d0 */
    void* driverNull;                             /* +0x0d4 */
    int cameraFlipDir;                            /* +0x0d8 */
    char* carryNullName;                          /* +0x0dc */
    char* cameraNullName;                         /* +0x0e0 */
    void* carryNulls[VEHICLE_MAXCARRYNULLS];      /* +0x0e4 */
    void* cameraNulls[VEHICLE_MAXCAMERANULLS];    /* +0x10c */
    unsigned int carryNullFrames;                 /* +0x11c */
    unsigned int cameraNullFrames;                /* +0x120 */
    Vehicle_Upgrades upgrades;                    /* +0x124 */
    /* +0x138-+0x1e0 is weapons.c's Weapons_Model, of which the unit
     * writes the nulls. */
    void* fireNullPairs[VEHICLE_MAXWEAPONS][VEHICLE_MAXFIRES]; /* +0x138 */
    void* xPivotNulls[VEHICLE_MAXWEAPONS];        /* +0x150 */
    void* yPivotNulls[VEHICLE_MAXWEAPONS];        /* +0x15c */
    float pivotMaxZ;                              /* +0x168 */
    float pivotMinZ;                              /* +0x16c */
    unsigned int fireNullPairFrames[VEHICLE_MAXWEAPONS]; /* +0x170 */
    unsigned char weapons_17c[0x48];              /* +0x17c */
    Vehicle_UpgradePartInfo* weaponParts[VEHICLE_MAXWEAPONS]; /* +0x1c4 */
    unsigned char weapons_1d0[0x0c];              /* +0x1d0 */
    unsigned int weaponCount;                     /* +0x1dc */
    ModelEntryRecord* polyMedium1;                /* +0x1e0 */
    ModelEntryRecord* polyMedium2;                /* +0x1e4 */
    unsigned int flags;                           /* +0x1e8 */
} VehicleModel;

// FUNCTION: LEGORR 0x0046c3d0
int vehicle_is_camera_flip_dir(VehicleModel* vehicle)
{
    return vehicle->cameraFlipDir == 1;
}

// FUNCTION: LEGORR 0x0046c3f0
int vehicle_set_activity(VehicleModel* vehicle, const char* activityName,
                         float elapsed)
{
    int result;
    unsigned int loop;

    linked_record_field4_clear(vehicle->polyMedium1);

    vehicle->drillNull = NULL;
    vehicle->depositNull = NULL;
    vehicle->driverNull = NULL;
    for (loop = 0; loop < vehicle->cameraNullFrames; loop++) vehicle->cameraNulls[loop] = NULL;
    for (loop = 0; loop < vehicle->carryNullFrames; loop++) vehicle->carryNulls[loop] = NULL;

    result = scene_action_name_set(vehicle->contAct1, activityName);
    vehicle->flags &= ~VEHICLE_FLAG_NOACTIVITY1;
    if (!result && (vehicle->flags & VEHICLE_FLAG_HOLDMISSING)) {
        if (!ledger_stub_00473340_SceneActionNamePresent(vehicle->contAct1, (void*)activityName))
            vehicle->flags |= VEHICLE_FLAG_NOACTIVITY1;
    }
    if (vehicle->contAct2) result |= scene_action_name_set(vehicle->contAct2, activityName);

    if (!(vehicle->flags & VEHICLE_FLAG_NOACTIVITY1)) ddiscene_frame_update_and_callbacks(vehicle->contAct1, elapsed);
    if (vehicle->contAct2) ddiscene_frame_update_and_callbacks(vehicle->contAct2, elapsed);

    vehicle_populate_wheel_nulls(vehicle);
    vehicle_set_upgrade_activity(vehicle, activityName);

    return result;
}

#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: activityName is an objectGlobs field (004de790 +0x534,
 * legoobject_globs.h, finding 662). objectGlobs is declared here, without
 * its struct, just before the one function that reads it, so the symbol
 * counter of the functions above does not move; it costs two symbols, and
 * the header would cost three. Deviation (scoring only). */
extern struct LegoObject_Globs objectGlobs;
#undef s_activity
#define s_activity ((const char**)((char*)&objectGlobs + 0x534))
#endif
/* Hang each shown upgrade part on its null and set its activity, and record
 * the weapon parts' fire and pivot nulls. Building.c's
 * Building_SetUpgradeActivity (004083a0) is the same code. */
// FUNCTION: LEGORR 0x0046c510
void vehicle_set_upgrade_activity(VehicleModel* vehicle, const char* activityName)
{
    Vehicle_PartObject* obj = vehicle->upgrades.objectList;
    Vehicle_UpgradePart* upgradePart;
    Vehicle_UpgradePartInfo* partInfo;
    void* null;
    void* cont;
    const char* name;
    unsigned int loop, sub, weapon, frame;

    for (loop = 0; loop < VEHICLE_MAXWEAPONS; loop++) {
        vehicle->weaponParts[loop] = NULL;
        vehicle->yPivotNulls[loop] = NULL;
        vehicle->xPivotNulls[loop] = NULL;
        for (sub = 0; sub < VEHICLE_MAXFIRES; sub++) vehicle->fireNullPairs[loop][sub] = NULL;
    }
    vehicle->weaponCount = 0;

    while (obj) {
        upgradePart = obj->upgradePart;
        partInfo = upgradePart->partInfo;
        null = vehicle_find_null(vehicle, partInfo->nullName, partInfo->nullFrameNo);
        cont = animation_object_handle_get(upgradePart);
        ddiscene_scene_object_attach_or_unlink(cont, null);
        render_object_set_identity_transform(cont);
        if (activityName) {
            upgrade_part_set_activity((struct Upgrade_PartModel*)upgradePart, activityName);
        } else {
            const char* standName = s_activity[0];
            scene_action_name_set(upgradePart->cont, standName);
        }

        if (partInfo->weaponName) {
            weapon = vehicle->weaponCount;
            frame = 0;
            vehicle->fireNullPairFrames[weapon] = 0;
            for (sub = 0; sub < VEHICLE_MAXFIRES; sub++) {
                vehicle->fireNullPairs[weapon][sub] = NULL;
                name = xf_texture_name_format(vehicle->contAct1, vehicle->fireNullName, (const int*)&vehicle->fireNullPairFrames[weapon]);
                vehicle->fireNullPairs[weapon][vehicle->fireNullPairFrames[weapon]] = resource_name_register_or_query_target((void**)cont, name, 0, NULL);
                if (vehicle->fireNullPairs[weapon][vehicle->fireNullPairFrames[weapon]++] == NULL) break;
            }
            name = xf_texture_name_format(vehicle->contAct1, vehicle->xPivotNullName, (const int*)&frame);
            vehicle->xPivotNulls[weapon] = resource_name_register_or_query_target((void**)cont, name, 0, NULL);
            name = xf_texture_name_format(vehicle->contAct1, vehicle->yPivotNullName, (const int*)&frame);
            vehicle->yPivotNulls[weapon] = resource_name_register_or_query_target((void**)cont, name, 0, NULL);
            vehicle->weaponParts[weapon] = partInfo;
            vehicle->weaponCount++;
        }

        obj = upgradePart->nextObject;
    }
}

// FUNCTION: LEGORR 0x0046c6f0
int vehicle_load(VehicleModel* vehicle, unsigned int objID, void* root,
                 const char* filename, const char* gameName)
{
    char* stringParts[10];
    char* argv[100];
    char baseDir[1024];
    char aeFile[1024];
    char fname[1024];
    char wheelFile[1024];
    float box[6];
    unsigned int numParts, count, loop;
    BlockFile* config;
    BlockFile* config2;
    char* wheelMesh;

    memset(vehicle, 0, sizeof(VehicleModel));
    strcpy(fname, filename);
    numParts = sound_variant_split(fname, stringParts, ",");
    if ((vehicle->contAct1 = container_load(root, stringParts[0], "ACT", 1))) {
        if (numParts == 2) {
            vehicle->contAct2 = container_load(vehicle->contAct1, stringParts[1], "ACT", 1);
        } else {
            vehicle->contAct2 = NULL;
        }

        strcpy(baseDir, stringParts[0]);
        count = sound_variant_split(baseDir, argv, "\\");
        if (count > 1) {
            for (loop = 1; loop < count; loop++) argv[loop][-1] = '\\';
        }
        sprintf(aeFile, "%s\\%s.%s", baseDir, argv[count - 1], "ae");

        if ((config = block_open(aeFile))) {
            if ((vehicle->cameraNullName = block_get_str_copy(config,
                    string_path_join(gameName, "CameraNullName", NULL)))) {
                vehicle->cameraNullFrames = atoi(block_get_strptr(config,
                        string_path_join(gameName, "CameraNullFrames", NULL))
                    ? block_get_strptr(config,
                          string_path_join(gameName, "CameraNullFrames", NULL))
                    : s_config_angle_empty);
                vehicle->cameraFlipDir = block_get_bool_path(config,
                    string_path_join(gameName, "CameraFlipDir", NULL));
            } else {
                vehicle->cameraNullFrames = 0;
            }

            if ((vehicle->carryNullName = block_get_str_copy(config,
                    string_path_join(gameName, "CarryNullName", NULL)))) {
                vehicle->carryNullFrames = atoi(block_get_strptr(config,
                        string_path_join(gameName,
                            // STRING: LEGORR 0x004a1d4c
                            "CarryNullFrames", NULL))
                    ? block_get_strptr(config,
                          string_path_join(gameName, "CarryNullFrames", NULL))
                    : s_config_angle_empty);
            } else {
                vehicle->carryNullFrames = 0;
            }

            vehicle->drillNullName = block_get_str_copy(config,
                string_path_join(gameName, "DrillNullName", NULL));
            vehicle->depositNullName = block_get_str_copy(config,
                string_path_join(gameName, "DepositNullName", NULL));
            vehicle->driverNullName = block_get_str_copy(config,
                string_path_join(gameName,
                                 // STRING: LEGORR 0x004aaa2c
                                 "DriverNullName",
                                 NULL));
            vehicle->fireNullName = block_get_str_copy(config,
                string_path_join(gameName,
                    // STRING: LEGORR 0x004a1d1c
                    "FireNullName", NULL));
            vehicle->yPivotNullName = block_get_str_copy(config,
                string_path_join(gameName,
                    // STRING: LEGORR 0x004a1d00
                    "yPivot", NULL));
            vehicle->xPivotNullName = block_get_str_copy(config,
                string_path_join(gameName,
                    // STRING: LEGORR 0x004a1cf8
                    "xPivot", NULL));

            if (block_get_strptr(config, string_path_join(gameName,
                // STRING: LEGORR 0x004a1cec
                "PivotMaxZ", NULL))) {
                vehicle->pivotMaxZ = (float)atof(block_get_strptr(config,
                        string_path_join(gameName, "PivotMaxZ", NULL))
                    ? block_get_strptr(config,
                          string_path_join(gameName, "PivotMaxZ", NULL))
                    : s_config_angle_empty);
            } else {
                vehicle->pivotMaxZ = 1.0f;
            }
            if (block_get_strptr(config, string_path_join(gameName,
                // STRING: LEGORR 0x004a1ce0
                "PivotMinZ", NULL))) {
                vehicle->pivotMinZ = (float)atof(block_get_strptr(config,
                        string_path_join(gameName, "PivotMinZ", NULL))
                    ? block_get_strptr(config,
                          string_path_join(gameName, "PivotMinZ", NULL))
                    : s_config_angle_empty);
            } else {
                vehicle->pivotMinZ = -1.0f;
            }

            upgrade_load((struct Upgrade_Model*)&vehicle->upgrades, config, gameName);

            if ((wheelMesh = block_get_str_copy(config,
                    string_path_join(gameName,
                                     // STRING: LEGORR 0x004aaa20
                                     "WheelMesh",
                                     NULL)))) {
                sprintf(wheelFile, "%s\\%s", baseDir, wheelMesh);
                free(wheelMesh);

                if ((vehicle->wheelNullName = block_get_str_copy(config,
                        string_path_join(gameName,
                                         // STRING: LEGORR 0x004aaa10
                                         "WheelNullName",
                                         NULL)))) {
                    memset(vehicle->contWheels, 0, sizeof(vehicle->contWheels));
                    vehicle->contWheels[0] = container_load(vehicle->contAct1, wheelFile, "LWO", 0);
                    vehicle->wheelNullsCount = 1;
                    memset(vehicle->wheelNulls, 0, sizeof(vehicle->wheelNulls));
                    if (vehicle->contWheels[0]) {
                        if (0.0f == (vehicle->wheelRadius = (float)atof(block_get_strptr(config,
                                string_path_join(gameName,
                                                 // STRING: LEGORR 0x004aaa04
                                                 "WheelRadius",
                                                 NULL))
                            ? block_get_strptr(config,
                                  string_path_join(gameName, "WheelRadius", NULL))
                            : s_config_angle_empty))) {
                            if (((Vehicle_Container*)vehicle->contWheels[0])->type == 1) {
                                scene_object_bounds_union(vehicle->contWheels[0], box);
                                vehicle->wheelRadius = (box[4] - box[1]) * 0.5f;
                                animated_texture_frame_bind(vehicle->contWheels[0], 0, 3);
                            } else {
                                vehicle->wheelRadius = 3.0f;
                            }
                        }
                    } else {
                        vehicle->wheelRadius = 0.0f;
                        for (loop = 1; loop < VEHICLE_MAXWHEELS; loop++) vehicle->contWheels[loop] = NULL;
                    }

                    vehicle->polyMedium1 = mesh_lod_load(config, gameName, stringParts[0], 1, 1);
                    if (vehicle->contAct2) {
                        strcpy(baseDir, stringParts[1]);
                        count = sound_variant_split(baseDir, argv, "\\");
                        if (count > 1) {
                            for (loop = 1; loop < count; loop++) argv[loop][-1] = '\\';
                        }
                        sprintf(aeFile, "%s\\%s.%s", baseDir, argv[count - 1], "ae");
                        if ((config2 = block_open(aeFile))) {
                            vehicle->polyMedium2 = mesh_lod_load(config2, gameName, stringParts[1], 1, 1);
                        }
                    }

                    vehicle->flags = VEHICLE_FLAG_SOURCE;
                    if (block_get_bool_path(config, string_path_join(gameName,
                                                                     // STRING: LEGORR 0x004aa9f8
                                                                     "HoldMissing",
                                                                     NULL)) == 1) {
                        vehicle->flags |= VEHICLE_FLAG_HOLDMISSING;
                    }
                    vehicle->lastUp.y = 0.0f;
                    vehicle->lastUp.x = 0.0f;
                    vehicle->lastUp.z = -1.0f;
                    vehicle->objID = objID;
                    block_close(config);
                    return 1;
                }
            }
            block_close(config);
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0046ce10
void vehicle_remove(VehicleModel* vehicle)
{
    unsigned int loop;

    vehicle_set_upgrade_level(vehicle, (unsigned int)-1);
    if (vehicle->flags & VEHICLE_FLAG_SOURCE) {
        if (vehicle->cameraNullName) free(vehicle->cameraNullName);
        if (vehicle->wheelNullName) free(vehicle->wheelNullName);
        if (vehicle->drillNullName) free(vehicle->drillNullName);
        if (vehicle->fireNullName) free(vehicle->fireNullName);
    }
    for (loop = 0; loop < VEHICLE_MAXWHEELS; loop++) {
        if (vehicle->contWheels[loop]) ledger_stub_00472a40_DDiSceneObjectDestroy(vehicle->contWheels[loop]);
        if (vehicle->wheelNulls[loop]) ledger_stub_00472a40_DDiSceneObjectDestroy(vehicle->wheelNulls[loop]);
    }
    ledger_stub_00472a40_DDiSceneObjectDestroy(vehicle->contAct1);
    if (vehicle->contAct2) ledger_stub_00472a40_DDiSceneObjectDestroy(vehicle->contAct2);
}

// FUNCTION: LEGORR 0x0046ced0
void vehicle_swap_poly_medium(VehicleModel* vehicle, int swap)
{
    resource_list_update_matching_key(vehicle->polyMedium1, vehicle->contAct1,
                                      (void*)(size_t)(swap == 0), 0);
    if (vehicle->contAct2 && vehicle->polyMedium2) {
        resource_list_update_matching_key(vehicle->polyMedium2, vehicle->contAct2,
                                          (void*)(size_t)(swap == 0), 0);
    }
}

// FUNCTION: LEGORR 0x0046cf20
void vehicle_swap_poly_high(VehicleModel* vehicle, int swap)
{
    vehicle_swap_poly_medium(vehicle, swap);
}

// FUNCTION: LEGORR 0x0046cf40
unsigned int vehicle_can_upgrade_type(VehicleModel* vehicle,
                                      unsigned int upgradeType, int current)
{
    unsigned int mask = 1 << upgradeType;

    if (current && (vehicle_get_upgrade_level(vehicle) & mask)) return 0;
    return vehicle->upgrades.levels & mask;
}

// FUNCTION: LEGORR 0x0046cf80
unsigned int vehicle_get_upgrade_level(VehicleModel* vehicle)
{
    return vehicle->upgrades.currentLevel;
}

// FUNCTION: LEGORR 0x0046cf90
void vehicle_set_upgrade_level(VehicleModel* vehicle, unsigned int objLevel)
{
    upgrade_set_upgrade_level((struct Upgrade_Model*)&vehicle->upgrades, objLevel);
    vehicle_set_upgrade_activity(vehicle, NULL);
}

// FUNCTION: LEGORR 0x0046cfc0
void vehicle_hide_wheels(VehicleModel* vehicle, int hide)
{
    unsigned int loop;

    for (loop = 0; loop < VEHICLE_MAXWHEELS; loop++) {
        if (vehicle->contWheels[loop]) scene_render_lock_toggle(vehicle->contWheels[loop], hide);
    }
}

// FUNCTION: LEGORR 0x0046cff0
int vehicle_clone(VehicleModel* srcVehicle, VehicleModel* destVehicle)
{
    unsigned int loop;

    *destVehicle = *srcVehicle;
    destVehicle->flags &= ~VEHICLE_FLAG_SOURCE;

    if ((destVehicle->contAct1 = entity_visual_resource_load(srcVehicle->contAct1))) {
        if (srcVehicle->contAct2) {
            destVehicle->contAct2 = entity_visual_resource_load(srcVehicle->contAct2);
            ddiscene_scene_object_attach_or_unlink(destVehicle->contAct2, destVehicle->contAct1);
            ddiscene_object_position_set(destVehicle->contAct2, destVehicle->contAct1, 0.0f, 0.0f, 0.0f);
            ddiscene_object_pose_method_dispatch(destVehicle->contAct2, destVehicle->contAct1,
                                                 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
            vehicle_container_enable_sound_triggers_adapter(destVehicle->contAct2, 1);
        }
        destVehicle->polyMedium1 = model_entry_list_flatten(srcVehicle->polyMedium1);
        if (srcVehicle->polyMedium2) {
            destVehicle->polyMedium2 = model_entry_list_flatten(srcVehicle->polyMedium2);
        }
        vehicle_container_enable_sound_triggers_adapter(destVehicle->contAct1, 1);
        vehicle_populate_wheel_nulls(destVehicle);
        if (srcVehicle->contWheels[0]) {
            for (loop = 0; loop < destVehicle->wheelNullsCount; loop++) {
                if ((destVehicle->contWheels[loop] = entity_visual_resource_load(srcVehicle->contWheels[0]))) {
                    ddiscene_scene_object_attach_or_unlink(destVehicle->contWheels[loop], destVehicle->contAct1);
                    vehicle_container_enable_sound_triggers_adapter(destVehicle->contWheels[loop], 1);
                }
            }
        }
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0046d140
void vehicle_set_owner_object(VehicleModel* vehicle, void* liveObj)
{
    unsigned int loop;

    scene_object_owner_set(vehicle->contAct1, (uint32_t)(size_t)liveObj);
    if (vehicle->contAct2) scene_object_owner_set(vehicle->contAct2, (uint32_t)(size_t)liveObj);
    for (loop = 0; loop < vehicle->wheelNullsCount; loop++) {
        if (vehicle->contWheels[loop]) scene_object_owner_set(vehicle->contWheels[loop], (uint32_t)(size_t)liveObj);
    }
}

// FUNCTION: LEGORR 0x0046d1a0
float vehicle_get_animation_time(VehicleModel* vehicle)
{
    return ddiscene_object_length_get(vehicle->contAct1);
}

// FUNCTION: LEGORR 0x0046d1c0
float vehicle_move_animation(VehicleModel* vehicle, float elapsed1,
                             float elapsed2, unsigned int repeats)
{
    /* The animation results stay on the x87 stack until the float store
     * after the second call: a double, as Ghidra's float10 (finding 590). */
    double result;
    float overrun;

    result = !(vehicle->flags & VEHICLE_FLAG_NOACTIVITY1)
        ? ddiscene_advance_by_time_target(vehicle->contAct1, elapsed1) : 0.0f;
    if (vehicle->contAct2) result = ddiscene_advance_by_time_target(vehicle->contAct2, elapsed2);
    overrun = (float)result;

    if (repeats > 1 && 0.0f != overrun) {
        return overrun - (float)(ddiscene_object_frame_position_get(vehicle->contAct1) * (repeats - 1));
    }
    return overrun;
}

// FUNCTION: LEGORR 0x0046d260
void vehicle_populate_wheel_nulls(VehicleModel* vehicle)
{
    unsigned int loop;
    const char* name;

    vehicle->wheelNullsCount = 0;
    for (loop = 0; loop < VEHICLE_MAXWHEELS; loop++) {
        name = xf_texture_name_format(vehicle->contAct1, vehicle->wheelNullName, (const int*)&loop);
        vehicle->wheelNulls[loop] = resource_name_register_or_query_target((void**)vehicle->contAct1, name, 0, NULL);
        if (vehicle->wheelNulls[loop]) vehicle->wheelNullsCount++;
    }
}

// FUNCTION: LEGORR 0x0046d2c0
void vehicle_hide(VehicleModel* vehicle, int hide)
{
    unsigned int loop;

    if (hide) vehicle->flags |= VEHICLE_FLAG_HIDDEN;
    else vehicle->flags &= ~VEHICLE_FLAG_HIDDEN;

    scene_render_lock_toggle(vehicle->contAct1, hide);
    if (vehicle->contAct2) scene_render_lock_toggle(vehicle->contAct2, hide);
    for (loop = 0; loop < vehicle->wheelNullsCount; loop++) {
        if (vehicle->contWheels[loop]) scene_render_lock_toggle(vehicle->contWheels[loop], hide);
    }
}

/* The original reads the flags dword, masks it to its low byte and then
 * takes bit 1, keeping the redundant `and eax, 0xff`. No C form found keeps
 * it: (flags & 0xff) >> 1 & 1, its local and division forms, a signed mask,
 * `& VEHICLE_FLAG_HIDDEN` with `>> 1`, `!= 0`, `== 2` or a conditional, an
 * 8-bit bitfield view, and (unsigned char) casts (a byte load) all differ
 * (finding 590). Container.c's 00473ba0 (`>> 3`) has the same shape and is
 * naked asm too. */
// FUNCTION: LEGORR 0x0046d330
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) unsigned int vehicle_is_hidden(VehicleModel* vehicle)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov eax, dword ptr [eax + 1e8h]
        and eax, 0ffh
        shr eax, 1
        and eax, 1
        ret
    }
}
#else
unsigned int vehicle_is_hidden(VehicleModel* vehicle)
{
    return (vehicle->flags & 0xff) >> 1 & 1;
}
#endif

// FUNCTION: LEGORR 0x0046d350
void vehicle_set_orientation(VehicleModel* vehicle, float xDir, float yDir,
                             float zDir)
{
    ddiscene_object_pose_method_dispatch(vehicle->contAct1, NULL, xDir, yDir, zDir,
                                         0.0f, 0.0f, -1.0f);
}

// FUNCTION: LEGORR 0x0046d380
void vehicle_set_position(VehicleModel* vehicle, float xPos, float yPos,
                          VehicleModel_GetWorldZCallback zCallback,
                          struct TerrainMap* surfMap)
{
    /* Declared in this order for the original's frame layout, which /O2
     * builds from the locals' use weights and declarations (finding 590).
     * `angle` is the one scratch float: the dot product, then each wheel's
     * turn. */
    Vehicle_Vector3F newUp;
    Vehicle_Vector3F wheelPos[VEHICLE_MAXWHEELS];
    unsigned int loop;
    Vehicle_Vector3F vecB;
    Vehicle_Vector3F normB;
    Vehicle_Vector3F dir;
    Vehicle_Vector3F normA;
    Vehicle_Vector3F vecA;
    Vehicle_Vector3F up;
    float angle;

    ddiscene_object_position_set(vehicle->contAct1, NULL, xPos, yPos,
                                 zCallback(xPos, yPos, surfMap));
    ddiscene_object_facing_pair_get(vehicle->contAct1, NULL, (float*)&dir, (float*)&up);
    ddiscene_object_pose_method_dispatch(vehicle->contAct1, NULL, dir.x, dir.y, dir.z,
                                         0.0f, 0.0f, -1.0f);

    if (vehicle->wheelNullsCount) {
        for (loop = 0; loop < vehicle->wheelNullsCount; loop++) {
            ddiscene_object_position_get(vehicle->wheelNulls[loop], NULL, (float*)&wheelPos[loop]);
            wheelPos[loop].z = zCallback(wheelPos[loop].x, wheelPos[loop].y, surfMap);
        }

        /* The body's up is the mean of the normals of the triangles that
         * wheels 0, 1, 2 and 0, 2, 3 make (the original assumes four). */
        vehicle_vector3d_cross_product(&normA,
            vehicle_vector3d_subtract(&vecA, &wheelPos[1], &wheelPos[2]),
            vehicle_vector3d_subtract(&vecB, &wheelPos[1], &wheelPos[0]));
        vehicle_vector3d_normalise(&normA);
        vehicle_vector3d_cross_product(&normB,
            vehicle_vector3d_subtract(&vecA, &wheelPos[3], &wheelPos[0]),
            vehicle_vector3d_subtract(&vecB, &wheelPos[3], &wheelPos[2]));
        vehicle_vector3d_normalise(&normB);
        vehicle_vector3d_add(&newUp, &normA, &normB);
        vehicle_vector3d_normalise(&newUp);

        /* The original tests lastUp.y twice and never lastUp.z. */
        if (!_finite(vehicle->lastUp.x) || !_finite(vehicle->lastUp.y) ||
            !_finite(vehicle->lastUp.y)) {
            vehicle->lastUp = newUp;
        }
        vehicle_ray_end_point(&newUp, &newUp, &vehicle->lastUp, 0.4f);
        vehicle_vector3d_scale(&newUp, &newUp, 1.0f / vehicle_vector3d_modulus(&newUp));
        angle = vehicle_vector3d_dot(&dir, &newUp);
        vehicle->lastUp = newUp;
        dir.x -= angle * newUp.x;
        dir.y -= angle * newUp.y;
        dir.z -= angle * newUp.z;
        ddiscene_object_pose_method_dispatch(vehicle->contAct1, NULL, dir.x, dir.y, dir.z,
                                             -newUp.x, -newUp.y, -newUp.z);

        for (loop = 0; loop < vehicle->wheelNullsCount; loop++) {
            ddiscene_object_position_get(vehicle->wheelNulls[loop], NULL, (float*)&wheelPos[loop]);
            scene_object_camera_teleport_or_offset(vehicle->wheelNulls[loop], 2, 0.0f,
                -(zCallback(wheelPos[loop].x, wheelPos[loop].y, surfMap) - wheelPos[loop].z - vehicle->wheelRadius),
                0.0f);
        }

        for (loop = 0; loop < vehicle->wheelNullsCount; loop++) {
            if (vehicle->contWheels[loop]) {
                ddiscene_object_position_set(vehicle->contWheels[loop], vehicle->wheelNulls[loop],
                                             0.0f, 0.0f, 0.0f);
            }
        }

        /* Turn each wheel by the distance it moved since the last call. */
        for (loop = 0; loop < vehicle->wheelNullsCount; loop++) {
            if (vehicle->contWheels[loop]) {
                ddiscene_object_position_get(vehicle->contWheels[loop], NULL, (float*)&wheelPos[loop]);
                vehicle_vector3d_subtract(&vecA, &wheelPos[loop], &vehicle->wheelLastPos[loop]);
                angle = vehicle_vector3d_modulus(&vecA) / vehicle->wheelRadius;
                if (vehicle_vector3d_dot(&vecA, &dir) < 0.0f) angle = -angle;
                render_object_rotate_axis(vehicle->contWheels[loop], 1, 1.0f, 0.0f, 0.0f, angle);
            }
        }

        for (loop = 0; loop < vehicle->wheelNullsCount; loop++) {
            if (vehicle->contWheels[loop]) {
                ddiscene_object_position_get(vehicle->contWheels[loop], NULL,
                                             (float*)&vehicle->wheelLastPos[loop]);
            }
        }
    }
}

// FUNCTION: LEGORR 0x0046d9e0
void* vehicle_get_activity_container(VehicleModel* vehicle)
{
    return vehicle->contAct1;
}

// FUNCTION: LEGORR 0x0046d9f0
void* vehicle_find_null(VehicleModel* vehicle, const char* name,
                        unsigned int frameNo)
{
    void* cont = NULL;
    const char* partName;

    if (vehicle->contAct2) {
        partName = xf_texture_name_format(vehicle->contAct2, name, (const int*)&frameNo);
        cont = resource_name_register_or_query_target((void**)vehicle->contAct2, partName, 0, NULL);
    }
    if (cont == NULL) {
        partName = xf_texture_name_format(vehicle->contAct1, name, (const int*)&frameNo);
        cont = resource_name_register_or_query_target((void**)vehicle->contAct1, partName, 0, NULL);
    }
    return cont;
}

// FUNCTION: LEGORR 0x0046da50
void* vehicle_get_camera_null(VehicleModel* vehicle, unsigned int frameNo)
{
    if (vehicle->cameraNullName) {
        if (vehicle->cameraNulls[frameNo]) return vehicle->cameraNulls[frameNo];
        return vehicle->cameraNulls[frameNo] = vehicle_find_null(vehicle, vehicle->cameraNullName, frameNo);
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0046da90
void* vehicle_get_drill_null(VehicleModel* vehicle)
{
    if (vehicle->drillNullName) {
        if (vehicle->drillNull) return vehicle->drillNull;
        return vehicle->drillNull = vehicle_find_null(vehicle, vehicle->drillNullName, 0);
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0046dac0
void* vehicle_get_deposit_null(VehicleModel* vehicle)
{
    if (vehicle->depositNullName) {
        if (vehicle->depositNull) return vehicle->depositNull;
        return vehicle->depositNull = vehicle_find_null(vehicle, vehicle->depositNullName, 0);
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0046daf0
void* vehicle_get_driver_null(VehicleModel* vehicle)
{
    if (vehicle->driverNullName) {
        if (vehicle->driverNull) return vehicle->driverNull;
        return vehicle->driverNull = vehicle_find_null(vehicle, vehicle->driverNullName, 0);
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0046db20
void* vehicle_get_carry_null(VehicleModel* vehicle, unsigned int frameNo)
{
    if (vehicle->carryNullName) {
        if (vehicle->carryNulls[frameNo]) return vehicle->carryNulls[frameNo];
        return vehicle->carryNulls[frameNo] = vehicle_find_null(vehicle, vehicle->carryNullName, frameNo);
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0046db60
unsigned int vehicle_get_carry_null_frames(VehicleModel* vehicle)
{
    return vehicle->carryNullFrames;
}

// FUNCTION: LEGORR 0x0046db70
float vehicle_get_trans_coef(VehicleModel* vehicle)
{
    return typed_child_resource_scalar_get(vehicle->contAct1);
}

#ifdef __cplusplus
}
#endif
