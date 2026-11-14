// SPDX-License-Identifier: LGPL-3.0-or-later
/* Teleporter.c, original translation unit GAME-044 (finding 505), rebuilt
 * in original shape (ADR 0005; finding 621): text 0046a370-0046a7f0, .data
 * 004aa450-004aa460 and the .bss word 00503188.
 *
 * The teleporter services. Teleporter_Start marks every object of the
 * chosen types as teleporting. With TELEPORT_MODE_SERVICE it also adds a
 * service at the camera's position: a ring that grows from it, or a
 * one-in-N chance a frame. Teleporter_Update removes each teleporting
 * object once its service lets it go, and Teleporter_ServiceAll reports
 * whether none of the chosen types is still teleporting. Lego.c starts the
 * level's pilots this way, and Objective.c teleports them out at the end.
 *
 * The port's implementation stays in the port tail record_chains_port.cpp,
 * unannotated. In the matching build teleporter_link_names.h and Lego.c's
 * lego_unit_imports.h give this unit's functions the names their callers
 * use; natively the port tail serves those names. */
#include "teleporter_link_names.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>

#include "crt/vc5names.h"
#include "teleporter.h"
#include "teleporter_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

/* The LegoObject's teleporter fields (+0x390 to +0x398). legoobject_unit.h
 * keeps them as padding, so that its includers' symbol numbering does not
 * change (finding 538). */
#define TELEPORTER_MODE(obj) LEGOOBJ_FIELD((obj), unsigned int, 0x390)
#define TELEPORTER_FLAGS(obj) LEGOOBJ_FIELD((obj), unsigned int, 0x394)
#define TELEPORTER_SERVICE(obj) LEGOOBJ_FIELD((obj), Teleporter_Service*, 0x398)

/* LegoObject flags3 0x800000 (LIVEOBJ3_REMOVING) and flags4 8
 * (LIVEOBJ4_TELEPORTING). */
#define TELEPORTER_LIVEOBJ3_REMOVING 0x800000
#define TELEPORTER_LIVEOBJ4_TELEPORTING 0x8

/* Teleporter_Start's search, passed to Teleporter_Callback_Start. */
typedef struct Teleporter_Search {
    unsigned int objType;             /* +0x0 LegoObject_Type */
    unsigned int modeFlags;           /* +0x4 */
    unsigned int teleportFlags;       /* +0x8 */
    Teleporter_Service* service;      /* +0xc */
} Teleporter_Search;

#include "teleporter_maths.h"

// GLOBAL: LEGORR 0x004aa450
Teleporter_Globs teleporterGlobs = { 0, 40, NULL, 3.0f };

// GLOBAL: LEGORR 0x00503188
static int s_teleporter_none_teleporting;

// FUNCTION: LEGORR 0x0046a370
void teleporter_remove_all(Teleporter_Service* service)
{
    if (service) {
        teleporter_remove_all(service->next);
        free(service);
    }
}

// FUNCTION: LEGORR 0x0046a390
void teleporter_restart(void)
{
    teleporterGlobs.teleportingCount = 0;
    teleporter_remove_all(teleporterGlobs.serviceList);
    teleporterGlobs.serviceList = NULL;
}

/* The callbacks have external linkage, as in the original: MSVC 5.0 emits a
 * static function after its first user, not where it is defined
 * (finding 658). */
// FUNCTION: LEGORR 0x0046a3c0
int teleporter_callback_start(Lego_Object* liveObj, void* data)
{
    Teleporter_Search* search = (Teleporter_Search*)data;

    if ((unsigned int)liveObj->type == search->objType) {
        TELEPORTER_MODE(liveObj) = search->modeFlags;
        TELEPORTER_FLAGS(liveObj) = search->teleportFlags;
        if (search->modeFlags & TELEPORT_MODE_SERVICE) {
            TELEPORTER_SERVICE(liveObj) = search->service;
            search->service->teleportingCount++;
        }
        teleporterGlobs.teleportingCount++;
        liveObj->flags4 |= TELEPORTER_LIVEOBJ4_TELEPORTING;
    }
    return 0;
}

/* legoGlobs' fields (lego_globs.h, finding 653), included before their
 * first use so that the functions above keep their symbol counter
 * (finding 538). */
#include "lego_globs.h"

// FUNCTION: LEGORR 0x0046a420
Teleporter_Point2F teleporter_get_camera_position(void)
{
    float worldPos[3];
    Teleporter_Point2F position;
    void* camera = s_zoom_step_context;

    if (s_context_mode != 1) camera = legoGlobs_cameraFP;
    camera_topdown_position_get_adapter(camera, TELEPORTER_LEGO_GET_MAP(), worldPos);
    position.x = worldPos[0];
    position.y = worldPos[1];
    return position;
}

// FUNCTION: LEGORR 0x0046a470
void teleporter_add_service(Teleporter_Service* service)
{
    service->next = teleporterGlobs.serviceList;
    teleporterGlobs.serviceList = service;
}

// FUNCTION: LEGORR 0x0046a490
int teleporter_callback_check_type(Lego_Object* liveObj, void* data)
{
    if ((unsigned int)liveObj->type ==
            (unsigned int)teleporter_get_service_object_type((unsigned int)(size_t)data) &&
        TELEPORTER_MODE(liveObj)) {
        s_teleporter_none_teleporting = 0;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0046a4c0
int teleporter_service_all(unsigned int objTypes)
{
    unsigned int i;
    unsigned int objType;

    for (i = 0; i < 31; i++) {
        objType = 1 << i;
        s_teleporter_none_teleporting = 1;
        if (objTypes & objType) {
            legoobject_run_through_lists_skip_upgrade_parts(
                teleporter_callback_check_type, (void*)(size_t)objType);
            if (!s_teleporter_none_teleporting) return 0;
        }
    }
    return 1;
}

// FUNCTION: LEGORR 0x0046a510
void teleporter_start(unsigned int objTypes, unsigned int modeFlags,
                      unsigned int teleportFlags)
{
    Teleporter_Search search;

    search.objType = teleporter_get_service_object_type(objTypes);
    search.modeFlags = modeFlags;
    search.teleportFlags = teleportFlags;
    if (modeFlags & TELEPORT_MODE_SERVICE) {
        search.service = (Teleporter_Service*)malloc(sizeof(Teleporter_Service));
        teleporter_add_service(search.service);
        search.service->teleportingCount = 0;
        if (teleportFlags & TELEPORT_FLAG_RING) {
            search.service->position = teleporter_get_camera_position();
            search.service->radius = 0.0f;
            search.service->speed = teleporterGlobs.ringSpeed;
        }
        if (teleportFlags & TELEPORT_FLAG_RANDOM) {
            search.service->randomInterval = teleporterGlobs.randomInterval;
        }
        search.service->flags = teleportFlags;
    }
    legoobject_run_through_lists_skip_upgrade_parts(teleporter_callback_start, &search);
}

// FUNCTION: LEGORR 0x0046a5c0
int teleporter_callback_update(Lego_Object* liveObj, void* data)
{
    Teleporter_Point2F position;
    Teleporter_Service* service;

    if (TELEPORTER_MODE(liveObj) & TELEPORT_MODE_INSTANT) {
        liveObj->health = -1.0f;
        TELEPORTER_FLAGS(liveObj) = 0;
        TELEPORTER_MODE(liveObj) = TELEPORT_MODE_DONE;
        teleporterGlobs.teleportingCount--;
        return 0;
    }
    if (TELEPORTER_MODE(liveObj) & TELEPORT_MODE_SERVICE) {
        if (TELEPORTER_FLAGS(liveObj) & TELEPORT_FLAG_RING) {
            service = TELEPORTER_SERVICE(liveObj);
            legoobject_transform_position_get(liveObj, &position.x, &position.y);
            if (teleporter_vector2d_distance(&position, &service->position) <
                TELEPORTER_SERVICE(liveObj)->radius) {
                liveObj->health = -1.0f;
                TELEPORTER_SERVICE(liveObj)->teleportingCount--;
                TELEPORTER_MODE(liveObj) = TELEPORT_MODE_DONE;
                TELEPORTER_SERVICE(liveObj) = NULL;
                TELEPORTER_FLAGS(liveObj) = 0;
                teleporterGlobs.teleportingCount--;
                return 0;
            }
        } else if (TELEPORTER_FLAGS(liveObj) & TELEPORT_FLAG_RANDOM) {
            service = TELEPORTER_SERVICE(liveObj);
            if ((rng_thunk_draw15() % service->randomInterval) == 0) {
                liveObj->health = -1.0f;
                service->teleportingCount--;
                TELEPORTER_MODE(liveObj) = TELEPORT_MODE_DONE;
                TELEPORTER_SERVICE(liveObj) = NULL;
                TELEPORTER_FLAGS(liveObj) = 0;
                teleporterGlobs.teleportingCount--;
                return 0;
            }
        } else if (TELEPORTER_FLAGS(liveObj) & TELEPORT_FLAG_REMOVE) {
            liveObj->health = -1.0f;
            liveObj->flags3 |= TELEPORTER_LIVEOBJ3_REMOVING;
            teleporterGlobs.teleportingCount--;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0046a700
void teleporter_update_services(Teleporter_Service* service, float elapsedGame)
{
    for (; service; service = service->next) {
        if (service->teleportingCount && service->flags == TELEPORT_FLAG_RING) {
            service->radius += service->speed * elapsedGame;
        }
    }
}

// FUNCTION: LEGORR 0x0046a730
void teleporter_update(float elapsedGame)
{
    teleporter_update_services(teleporterGlobs.serviceList, elapsedGame);
    if (teleporterGlobs.teleportingCount) {
        legoobject_run_through_lists_skip_upgrade_parts(teleporter_callback_update, NULL);
    }
}

// FUNCTION: LEGORR 0x0046a760
int teleporter_get_service_object_type(unsigned int objType)
{
    if (objType & TELEPORT_SERVICE_VEHICLE) return 1;
    if (objType & TELEPORT_SERVICE_MINIFIGURE) return 2;
    if (objType & TELEPORT_SERVICE_ROCKMONSTER) return 3;
    if (objType & TELEPORT_SERVICE_BUILDING) return 4;
    if (objType & TELEPORT_SERVICE_BOULDER) return 5;
    if (objType & TELEPORT_SERVICE_POWERCRYSTAL) return 6;
    if (objType & TELEPORT_SERVICE_ORE) return 7;
    if (objType & TELEPORT_SERVICE_DYNAMITE) return 8;
    if (objType & TELEPORT_SERVICE_BARRIER) return 9;
    if (objType & TELEPORT_SERVICE_UPGRADEPART) return 10;
    if (objType & TELEPORT_SERVICE_ELECTRICFENCE) return 11;
    if (objType & TELEPORT_SERVICE_SPIDERWEB) return 12;
    if (objType & TELEPORT_SERVICE_OOHSCARY) return 13;
    return 0;
}

#ifdef __cplusplus
}
#endif
