// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-023 LegoObject.c's object record, shared by every unit that reads
 * an object's fields. The port's name for it was EntitySlot. */
#ifndef LEGORRR_GAME_LEGOOBJECT_RECORD_H
#define LEGORRR_GAME_LEGOOBJECT_RECORD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Teleporter_Service;
struct Upgrade_PartModel;
struct VehicleModel;
struct BuildingModel;
struct AnimationObject;
struct SteeringGroup;
struct AITask;
struct StatsData;

/* LegoObject (0x40c bytes on Win32): the original's object record. The
 * pool (legoobject_pool_chunk_allocate) hands them out from chunks;
 * nextFree links free records. The +0x... comments are the Win32 offsets;
 * the pointer fields hold real pointers (ADR 0007 decision 4). Named by role where the port or the
 * pseudocode knows it, otherwise by offset. Lego_Object, not LegoObject:
 * interface_callees.h (GAME-020) defines a partial LegoObject view, and the
 * boundary test includes both headers. */
typedef struct Lego_Object {
    int type;                         /* +0x000 LegoObject_Type */
    int id;                           /* +0x004 object index within the type */
    char* customName;                 /* +0x008 */
    struct VehicleModel* vehicle;     /* +0x00c model, by type */
    struct AnimationObject* miniFigure;  /* +0x010 CreatureModel */
    struct AnimationObject* rockMonster; /* +0x014 CreatureModel */
    struct BuildingModel* building;   /* +0x018 */
    void* other;                      /* +0x01c */
    struct Upgrade_PartModel* upgradePart; /* +0x020 */
    void* routeBlocks;                /* +0x024 route buffer */
    unsigned int routeBlocksTotal;    /* +0x028 */
    unsigned int routeBlocksCurrent;  /* +0x02c */
    unsigned int routeCurveCount;     /* +0x030 BezierCurve routeCurve: count */
    float routeCurvePoints[50][2];    /* +0x034 routeCurve.points */
    float routeCurveDistances[50];    /* +0x1c4 routeCurve.distances */
    float field_28c;                  /* +0x28c */
    float field_290;                  /* +0x290 */
    float field_294;                  /* +0x294 */
    float field_298;                  /* +0x298 last route direction x */
    float field_29c;                  /* +0x29c last route direction y */
    float field_2a0;                  /* +0x2a0 */
    float field_2a4;                  /* +0x2a4 */
    float field_2a8;                  /* +0x2a8 */
    float faceDirection[3];           /* +0x2ac */
    float faceDirectionLength;        /* +0x2b8 */
    int field_2bc;                    /* +0x2bc */
    int field_2c0;                    /* +0x2c0 */
    float field_2c4;                  /* +0x2c4 */
    float field_2c8;                  /* +0x2c8 */
    float field_2cc;                  /* +0x2cc */
    unsigned int field_2d0;           /* +0x2d0 */
    float animTime;                   /* +0x2d4 */
    unsigned int animRepeat;          /* +0x2d8 */
    void* field_2dc;                  /* +0x2dc sink effect container */
    int field_2e0;                    /* +0x2e0 */
    void* activityCont;               /* +0x2e4 container the activity runs */
    const char* activityName1;        /* +0x2e8 activityName entries */
    const char* activityName2;        /* +0x2ec */
    struct AITask* aiTask;            /* +0x2f0 command chain head */
    float targetBlockPos[2];          /* +0x2f4 */
    struct Lego_Object* routeToObject;  /* +0x2fc */
    struct Lego_Object* interactObject; /* +0x300 reciprocal of +0x2fc */
    struct Lego_Object* carryingThisObject; /* +0x304 */
    struct Lego_Object* carriedObjects[6];  /* +0x308 */
    unsigned int carryingIndex;       /* +0x320 */
    unsigned int numCarriedObjects;   /* +0x324 */
    unsigned int carryNullFrames;     /* +0x328 */
    struct SteeringGroup* field_32c;  /* +0x32c the flock */
    unsigned int objLevel;            /* +0x330 */
    struct StatsData* stats;          /* +0x334 ObjectStats */
    float aiTimer;                    /* +0x338 */
    float field_33c;                  /* +0x33c */
    float health;                     /* +0x340 */
    float energy;                     /* +0x344 */
    void* field_348;                  /* +0x348 */
    int field_34c;                    /* +0x34c */
    int drillSoundHandle;             /* +0x350 */
    int soundHandle;                  /* +0x354 */
    float slowDeathTimer;             /* +0x358 */
    int slowDeathWeapon;              /* +0x35c */
    float weaponCoolDown;             /* +0x360 */
    void* field_364;                  /* +0x364 */
    float field_368;                  /* +0x368 */
    struct Lego_Object* driveObject;  /* +0x36c */
    int carriedTools[5];              /* +0x370 */
    unsigned int numCarriedTools;     /* +0x384 */
    float bubbleTimer;                /* +0x388 Bubbles.c's image timer */
    void* bubbleImage;                /* +0x38c Image*: the current bubble */
    unsigned int teleporterMode;      /* +0x390 Teleporter_Start's modeFlags */
    unsigned int teleporterFlags;     /* +0x394 */
    struct Teleporter_Service* teleporterService; /* +0x398 */
    float weaponDirection[3];         /* +0x39c fire direction */
    float weaponTarget[3];            /* +0x3a8 fire target position */
    float field_3b4;                  /* +0x3b4 */
    float field_3b8;                  /* +0x3b8 */
    float field_3bc;                  /* +0x3bc */
    void* field_3c0;                  /* +0x3c0 */
    struct Lego_Object* lazerHitObject; /* +0x3c4 */
    uint32_t field_3c8;               /* +0x3c8 */
    struct Lego_Object* field_3cc;    /* +0x3cc */
    float field_3d0;                  /* +0x3d0 */
    float field_3d4;                  /* +0x3d4 */
    float field_3d8;                  /* +0x3d8 */
    float field_3dc;                  /* +0x3dc */
    unsigned int flags1;              /* +0x3e0 */
    unsigned int flags2;              /* +0x3e4 */
    unsigned int flags3;              /* +0x3e8 */
    unsigned int flags4;              /* +0x3ec */
    unsigned int abilityFlags;        /* +0x3f0 */
    int field_3f4;                    /* +0x3f4 */
    int field_3f8;                    /* +0x3f8 NERPs' tutorial icon */
    float field_3fc;                  /* +0x3fc */
    float field_400;                  /* +0x400 */
    int field_404;                    /* +0x404 */
    struct Lego_Object* nextFree;     /* +0x408 */
} Lego_Object;

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGOOBJECT_RECORD_H */
