// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-007 Construction.c's zone record, shared with the units that read a
 * block's zone (Lego.c). The +0x... comments are the Win32 offsets; the
 * pointer fields hold real pointers (ADR 0007 decision 4). */
#ifndef LEGORRR_GAME_CONSTRUCTION_RECORD_H
#define LEGORRR_GAME_CONSTRUCTION_RECORD_H

#ifdef __cplusplus
extern "C" {
#endif

struct Lego_Object;

/* Maths.h's Point2I. */
typedef struct ConstructionPoint2I {
    int x, y;
} ConstructionPoint2I;

/* Lego_ConstructionZone (0xd4 bytes on Win32; Lego_Block +0x10): a path or building under
 * construction. Construction_Zone_Create links each new zone at the head of
 * constructionGlobs.constructList. */
typedef struct Lego_ConstructionZone {
    int objID;                          /* +0x00 building type (buildings) */
    unsigned int direction;             /* +0x04 quarter turns */
    unsigned int handle;                /* +0x08 */
    ConstructionPoint2I origin;         /* +0x0c */
    ConstructionPoint2I* shapePoints;   /* +0x14 */
    unsigned int shapeCount;            /* +0x18 */
    unsigned int requestCount;          /* +0x1c resources requested */
    unsigned int placedCount;           /* +0x20 resources placed */
    struct Lego_ConstructionZone* next; /* +0x24 */
    float progress;                     /* +0x28 */
    struct Lego_Object* placedObjects[40]; /* +0x2c */
    unsigned int placedObjectCount;     /* +0x0cc */
    unsigned int flags;                 /* +0x0d0 CONSTRUCTION_FLAG_* */
} Lego_ConstructionZone;

/* Lego_ConstructionZone.flags. */
#define CONSTRUCTION_FLAG_READY 0x1u     /* every requested resource placed */
#define CONSTRUCTION_FLAG_LAYPATH 0x2u   /* a path, not a building */
#define CONSTRUCTION_FLAG_REQUESTED 0x4u /* the path's resources requested */
#define CONSTRUCTION_FLAG_USESTUDS 0x8u  /* refined ore, not ore */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_CONSTRUCTION_RECORD_H */
