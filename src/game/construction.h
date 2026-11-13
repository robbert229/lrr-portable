// SPDX-License-Identifier: LGPL-3.0-or-later
/* Construction.c (original translation unit GAME-052, finding 603):
 * construction zones, by the unit's own names.
 *
 * C-compatible header (ADR 0005). construction.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * includes it.
 *
 * In the matching build construction_link_names.h, which construction.c
 * includes first, gives twelve of these functions the port's names, so
 * their callers reach them. The others keep these names in every build;
 * lego_unit.c and legoobject_unit.c call them by these names. Natively the
 * port tail placement_port.cpp serves the port's names, and nothing native
 * reaches these functions. */
#ifndef LEGORRR_GAME_CONSTRUCTION_H
#define LEGORRR_GAME_CONSTRUCTION_H

#ifdef __cplusplus
extern "C" {
#endif

struct Lego_Object;

/* Maths.h's Point2I. */
typedef struct ConstructionPoint2I {
    int x, y;
} ConstructionPoint2I;

/* Lego_ConstructionZone (0xd4 bytes): a path or building under
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

unsigned int construction_get_building_base(const char* name);      /* 00408c80 */
void construction_zone_place_resource(unsigned int handle,
                                      struct Lego_Object* placedObject); /* 00408ce0 */
int construction_zone_needs_more_of_resource(unsigned int handle,
                                             int objType,
                                             int objID);            /* 00408d70 */
unsigned int construction_zone_count_of_resource_placed(
    Lego_ConstructionZone* zone, int objType, int objID);           /* 00408e10 */
void construction_zone_request_resource(int* blockPos, int objType,
                                        int objID, unsigned int objLevel,
                                        unsigned int count);        /* 00408e50 */
void construction_spawn_barriers(int* blockPos, int* shapePoints,
                                 unsigned int shapeCount);          /* 00408ec0 */
int construction_zone_exists_at_block(int* blockPos);               /* 004090a0 */
Lego_ConstructionZone* construction_zone_find_by_handle_or_at_block(
    int* blockPos, unsigned int* handle);                           /* 004090c0 */
void construction_zone_complete_path(int* blockPos);                /* 00409110 */
void construction_zone_cancel_path(int* blockPos);                  /* 00409150 */
void construction_update_all(float elapsedGame);                    /* 004091e0 */
int construction_zone_origin_is_clear(Lego_ConstructionZone* zone); /* 00409270 */
int construction_zone_start_path(int* blockPos);                    /* 00409300 */
Lego_ConstructionZone* construction_zone_create(int* blockPos);     /* 00409350 */
void construction_flood_path_blocks(int* blockPos);                 /* 004093b0 */
int construction_find_resource_object(int* blockPos,
                                      int resourceAmount);          /* 00409450 */
int construction_find_resource_object_recurse(int* blockPos,
                                              int resourceAmount);  /* 00409470 */
void construction_zone_request_path_resources(
    Lego_ConstructionZone* zone);                                   /* 00409550 */
void construction_zone_cancel_building(int* blockPos);              /* 00409600 */
unsigned int construction_zone_start_building(int objID, int* blockPos,
                                              unsigned int direction,
                                              int* shapePoints,
                                              unsigned int shapeCount); /* 00409790 */
void construction_update_surrounding_blocks(int* blockPos);         /* 00409940 */
void construction_remove_all(void);                                 /* 004099d0 */
void construction_zone_free(Lego_ConstructionZone* zone);           /* 004099f0 */
void construction_zone_remove_barriers(Lego_ConstructionZone* zone); /* 00409a40 */
/* Returns the new building (a LegoObject); void* so that Lego.c's imports,
 * which do not name LegoObject, need no new declaration. */
void* construction_place_building(int objID, int* blockPos,
                                  unsigned int direction, int* shapePoints,
                                  unsigned int shapeCount,
                                  int teleportDown);                /* 00409a90 */
struct Lego_Object* construction_zone_complete_building(
    Lego_ConstructionZone* zone, int teleportDown);                 /* 00409b30 */
void construction_update_cardinal_blocks(int* blockPos);            /* 00409cd0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_CONSTRUCTION_H */
