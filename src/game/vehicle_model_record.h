// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-048 Vehicle.c's model record, shared with the units that allocate
 * and read it (Lego.c, LegoObject.c, Weapons.c).
 * The +0x... comments are the Win32 offsets; the pointer fields hold real
 * pointers (ADR 0007 decision 4). */
#ifndef LEGORRR_GAME_VEHICLE_MODEL_RECORD_H
#define LEGORRR_GAME_VEHICLE_MODEL_RECORD_H

#include "gods98/model.h"
#include "legoobject_record.h"
#include "upgrade_record.h"
#include "weapons_record.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VEHICLE_MAXWHEELS 6
#define VEHICLE_MAXCARRYNULLS 10
#define VEHICLE_MAXCAMERANULLS 4
#define VEHICLE_MAXWEAPONS 3
#define VEHICLE_MAXFIRES 2

typedef struct Vehicle_Vector3F {
    float x, y, z;
} Vehicle_Vector3F;

/* Upgrade.c's records (upgrade_record.h), by the names the unit uses. */
typedef Upgrade_PartInfo Vehicle_UpgradePartInfo;
typedef Upgrade_PartModel Vehicle_UpgradePart;
typedef struct Lego_Object Vehicle_PartObject;
typedef Upgrade_Model Vehicle_Upgrades;

/* The original's VehicleModel, 0x1ec bytes on Win32: each vehicle type's
 * loaded model (legoGlobs.vehicleData) and each vehicle object's clone
 * (LegoObject +0x00c). */
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
    /* +0x138-+0x1e0 is Weapons.c's model, of which the unit writes the
     * nulls. */
    Weapons_Model weapons;                        /* +0x138 */
    ModelEntryRecord* polyMedium1;                /* +0x1e0 */
    ModelEntryRecord* polyMedium2;                /* +0x1e4 */
    unsigned int flags;                           /* +0x1e8 */
} VehicleModel;

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_VEHICLE_MODEL_RECORD_H */
