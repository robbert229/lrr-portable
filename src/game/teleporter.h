// SPDX-License-Identifier: LGPL-3.0-or-later
/* Teleporter.c (original translation unit GAME-044, finding 621): the
 * teleporter services, which teleport objects of the chosen types into or
 * out of the level.
 *
 * C-compatible header (ADR 0005). teleporter.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 538). src/game/test_c_boundaries.cpp
 * checks it against the C++ declarations of the same functions.
 *
 * The names are OpenLRR's, in snake case, and are inferred: the binary has
 * no Teleporter_ string. */
#ifndef LEGORRR_GAME_TELEPORTER_H
#define LEGORRR_GAME_TELEPORTER_H

#ifdef __cplusplus
extern "C" {
#endif

/* TeleportObjectType: one bit a LegoObject_Type, from the vehicle (1) to
 * the scare-object (0x1000). Teleporter_GetServiceObjectType maps the
 * lowest bit set to its LegoObject_Type. */
#define TELEPORT_SERVICE_VEHICLE        0x1
#define TELEPORT_SERVICE_MINIFIGURE     0x2
#define TELEPORT_SERVICE_ROCKMONSTER    0x4
#define TELEPORT_SERVICE_BUILDING       0x8
#define TELEPORT_SERVICE_BOULDER        0x10
#define TELEPORT_SERVICE_POWERCRYSTAL   0x20
#define TELEPORT_SERVICE_ORE            0x40
#define TELEPORT_SERVICE_DYNAMITE       0x80
#define TELEPORT_SERVICE_BARRIER        0x100
#define TELEPORT_SERVICE_UPGRADEPART    0x200
#define TELEPORT_SERVICE_ELECTRICFENCE  0x400
#define TELEPORT_SERVICE_SPIDERWEB      0x800
#define TELEPORT_SERVICE_OOHSCARY       0x1000

/* Teleporter_Start's modeFlags (the object's +0x390). TELEPORT_MODE_DONE
 * is the state Teleporter_Update leaves a finished object in. */
#define TELEPORT_MODE_INSTANT   0x1
#define TELEPORT_MODE_SERVICE   0x2
#define TELEPORT_MODE_DONE      0x4

/* Teleporter_Start's teleportFlags (the object's +0x394 and the service's
 * flags): leave when the service's ring reaches the object, leave at
 * random (one chance in the service's interval a frame), or be removed. */
#define TELEPORT_FLAG_RANDOM    0x1
#define TELEPORT_FLAG_RING      0x2
#define TELEPORT_FLAG_REMOVE    0x4

/* Point2F, returned by value (eax:edx). */
typedef struct Teleporter_Point2F {
    float x;
    float y;
} Teleporter_Point2F;

/* TeleporterService (0x20 bytes): one teleport, in the list at
 * teleporterGlobs.serviceList. */
typedef struct Teleporter_Service {
    Teleporter_Point2F position;      /* +0x00 the camera's position at the start */
    float radius;                     /* +0x08 the ring, grows by speed a second */
    float speed;                      /* +0x0c */
    unsigned int teleportingCount;    /* +0x10 objects still teleporting */
    int randomInterval;               /* +0x14 one chance in this a frame */
    unsigned int flags;               /* +0x18 teleportFlags */
    struct Teleporter_Service* next;  /* +0x1c */
} Teleporter_Service;

/* Teleporter_Globs (0x10 bytes of .data at 004aa450). */
typedef struct Teleporter_Globs {
    unsigned int teleportingCount;    /* +0x0 objects still teleporting */
    int randomInterval;               /* +0x4 40 */
    Teleporter_Service* serviceList;  /* +0x8 */
    float ringSpeed;                  /* +0xc 3.0 */
} Teleporter_Globs;

extern Teleporter_Globs teleporterGlobs;

void teleporter_remove_all(Teleporter_Service* service);              /* 0046a370 */
void teleporter_restart(void);                                        /* 0046a390 */
Teleporter_Point2F teleporter_get_camera_position(void);              /* 0046a420 */
void teleporter_add_service(Teleporter_Service* service);             /* 0046a470 */
int teleporter_service_all(unsigned int objTypes);                    /* 0046a4c0 */
void teleporter_start(unsigned int objTypes, unsigned int modeFlags,
                      unsigned int teleportFlags);                    /* 0046a510 */
void teleporter_update_services(Teleporter_Service* service,
                                float elapsedGame);                   /* 0046a700 */
void teleporter_update(float elapsedGame);                            /* 0046a730 */
int teleporter_get_service_object_type(unsigned int objType);         /* 0046a760 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_TELEPORTER_H */
