// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-006 Building.c's model record, shared with the units that allocate
 * and read it (Lego.c, LegoObject.c, Weapons.c).
 * The +0x... comments are the Win32 offsets; the pointer fields hold real
 * pointers (ADR 0007 decision 4). */
#ifndef LEGORRR_GAME_BUILDING_MODEL_RECORD_H
#define LEGORRR_GAME_BUILDING_MODEL_RECORD_H

#include "legoobject_record.h"
#include "upgrade_record.h"
#include "weapons_record.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BUILDING_MAXCARRYNULLS 6
#define BUILDING_MAXCAMERANULLS 4
#define BUILDING_MAXTOOLNULLS 6
#define BUILDING_MAXSHAPEPOINTS 10
#define BUILDING_MAXWEAPONS 3

#define BUILDING_FLAG_SOURCE 0x1        /* the loaded model, not a clone */
#define BUILDING_FLAG_POWERLEVELCHANGED 0x2

typedef struct Building_Point2I {
    int x, y;
} Building_Point2I;

/* Upgrade.c's records (upgrade_record.h), by the names the unit uses. */
typedef Upgrade_PartInfo Building_UpgradeData;
typedef Upgrade_PartModel Building_UpgradePart;
typedef struct Lego_Object Building_PartObject;
typedef Upgrade_Model Building_Upgrades;

/* The original's BuildingModel, 0x14c bytes on Win32: each building type's
 * loaded model (legoGlobs.buildingData) and each building object's clone
 * (LegoObject +0x018). +0xa0-+0x148 is Weapons.c's model (finding 590), of
 * which the unit writes the nulls. */
typedef struct BuildingModel {
    unsigned int objID;                           /* +0x000 */
    void* contAct;                                /* +0x004 */
    char* carryNullName;                          /* +0x008 */
    char* cameraNullName;                         /* +0x00c */
    char* depositNullName;                        /* +0x010 */
    char* entranceNullName;                       /* +0x014 */
    char* toolNullName;                           /* +0x018 */
    char* fireNullName;                           /* +0x01c */
    char* yPivotNullName;                         /* +0x020 */
    char* xPivotNullName;                         /* +0x024 */
    void* carryNulls[BUILDING_MAXCARRYNULLS];     /* +0x028 */
    void* cameraNulls[BUILDING_MAXCAMERANULLS];   /* +0x040 */
    void* toolNulls[BUILDING_MAXTOOLNULLS];       /* +0x050 */
    void* depositNull;                            /* +0x068 */
    void* entranceNull;                           /* +0x06c */
    unsigned int carryNullFrames;                 /* +0x070 */
    unsigned int cameraNullFrames;                /* +0x074 */
    unsigned int toolNullFrames;                  /* +0x078 */
    void* contPowerLevel;                         /* +0x07c */
    float powerLevel;                             /* +0x080 */
    Building_Point2I* shapePoints;                /* +0x084 */
    unsigned int shapeCount;                      /* +0x088 */
    Building_Upgrades upgrades;                   /* +0x08c */
    Weapons_Model weapons;                        /* +0x0a0 */
    unsigned int flags;                           /* +0x148 */
} BuildingModel;

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_BUILDING_MODEL_RECORD_H */
