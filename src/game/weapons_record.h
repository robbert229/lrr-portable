// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-049 Weapons.c's weapons model, embedded in Vehicle.c's and Building.c's
 * models.
 * The +0x... comments are the Win32 offsets; the pointer fields hold real
 * pointers (ADR 0007 decision 4). */
#ifndef LEGORRR_GAME_WEAPONS_RECORD_H
#define LEGORRR_GAME_WEAPONS_RECORD_H


#ifdef __cplusplus
extern "C" {
#endif

struct Upgrade_PartInfo;

typedef struct Weapons_Vector3F {
    float x, y, z;
} Weapons_Vector3F;

/* The weapons part of a vehicle (VehicleModel +0x138) or building
 * (BuildingModel +0xa0) model: up to three weapon slots, each with two fire
 * nulls and an x and a y pivot. Vehicle.c and Building.c load the nulls;
 * Weapons.c aims and fires them. */
typedef struct Weapons_Model {
    void* fireNulls[3][2];            /* +0x00 */
    void* xPivotNulls[3];             /* +0x18 */
    void* yPivotNulls[3];             /* +0x24 */
    float pivotMaxZ;                  /* +0x30 the pitch range */
    float pivotMinZ;                  /* +0x34 */
    unsigned int fireNullCounts[3];   /* +0x38 */
    Weapons_Vector3F fireDirs[3];     /* +0x44 */
    Weapons_Vector3F targets[3];      /* +0x68 */
    struct Upgrade_PartInfo* weaponParts[3]; /* +0x8c the slots' parts */
    float timers[3];                  /* +0x98 */
    unsigned int weaponCount;         /* +0xa4 */
} Weapons_Model;

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_WEAPONS_RECORD_H */
