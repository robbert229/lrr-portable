// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-047 Upgrade.c's records, shared with the units that embed or read
 * them (Vehicle.c, Building.c, Weapons.c, Lego.c, LegoObject.c). The
 * +0x... comments are the Win32 offsets; the pointer fields hold real
 * pointers (ADR 0007 decision 4). */
#ifndef LEGORRR_GAME_UPGRADE_RECORD_H
#define LEGORRR_GAME_UPGRADE_RECORD_H

#ifdef __cplusplus
extern "C" {
#endif

struct Lego_Object;

/* One part of an upgrade level (0x14 bytes on Win32). */
typedef struct Upgrade_PartInfo {
    unsigned int level;                           /* +0x00 */
    unsigned int nullFrameNo;                     /* +0x04 */
    char* nullName;                               /* +0x08 */
    char* weaponName;                             /* +0x0c */
    void* upgradeData;                            /* +0x10 the part's model */
} Upgrade_PartInfo;

/* An object's upgrades (0x14 bytes on Win32): VehicleModel's and
 * BuildingModel's upgrades. */
typedef struct Upgrade_Model {
    Upgrade_PartInfo* parts;                      /* +0x00 */
    unsigned int count;                           /* +0x04 */
    unsigned int currentLevel;                    /* +0x08 */
    unsigned int levels;                          /* +0x0c a bit per type */
    struct Lego_Object* objectList;               /* +0x10 the shown parts */
} Upgrade_Model;

/* An upgrade part's model (0x10 bytes on Win32): lego_unit.c's upgrade types, and
 * each upgrade-part LegoObject's clone (+0x20). */
typedef struct Upgrade_PartModel {
    unsigned int objID;                           /* +0x00 */
    void* cont;                                   /* +0x04 */
    struct Lego_Object* nextObject;               /* +0x08 */
    Upgrade_PartInfo* partInfo;                   /* +0x0c */
} Upgrade_PartModel;

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_UPGRADE_RECORD_H */
