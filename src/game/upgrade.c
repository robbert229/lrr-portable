// SPDX-License-Identifier: LGPL-3.0-or-later
/* Upgrade.c, original translation unit GAME-047 (finding 505), rebuilt in
 * original shape (ADR 0005; finding 617): text 0046c030-0046c3d0, .rdata
 * 0049e7dc, literals 004aa9dc-004aa9f6. The unit has no .data or .bss.
 *
 * An upgrade part is one model (a mesh or an activity container) that an
 * upgrade adds to a vehicle or building. An upgrades model (a vehicle's
 * +0x124, a building's +0x8c) lists the parts each upgrade level shows:
 * Upgrade_Load reads them from the object's `Upgrades::LevelWXYZ` arrays,
 * one `null,frame[,weapon]` item a part. Upgrade_SetUpgradeLevel replaces
 * the shown parts with one upgrade-part LegoObject per part of the new
 * level. Vehicle.c and Building.c then hang each part on its null.
 *
 * The functions that follow in the old unit map (0046c3d0-0046c510) are
 * Vehicle.c's (GAME-048, vehicle_model.c; finding 617).
 *
 * The port's implementation stays in the port tails upgrade_levels_port.cpp
 * and tooltip_port.cpp, unannotated. In the matching build
 * upgrade_link_names.h gives five of this unit's functions the port's
 * names, so every caller reaches them; natively the port tails serve those
 * names. */
#include "upgrade_link_names.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "upgrade.h"
#include "upgrade_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

#define UPGRADE_MAXPARTS 200
#define UPGRADE_MAXLEVELS 16

#define LEGOOBJECT_UPGRADEPART 10

/* One part of an upgrade level (0x14 bytes). */
typedef struct Upgrade_PartInfo {
    unsigned int level;                           /* +0x00 */
    unsigned int nullFrameNo;                     /* +0x04 */
    char* nullName;                               /* +0x08 */
    char* weaponName;                             /* +0x0c */
    void* upgradeData;                            /* +0x10 the part's model */
} Upgrade_PartInfo;

/* An object's upgrades (0x14 bytes). */
typedef struct Upgrade_Model {
    Upgrade_PartInfo* parts;                      /* +0x00 */
    unsigned int count;                           /* +0x04 */
    unsigned int currentLevel;                    /* +0x08 */
    unsigned int levels;                          /* +0x0c a bit per type */
    struct Lego_Object* objectList;               /* +0x10 the shown parts */
} Upgrade_Model;

/* An upgrade part's model (0x10 bytes): lego_unit.c's upgrade types, and
 * each upgrade-part LegoObject's clone (+0x20). */
typedef struct Upgrade_PartModel {
    unsigned int objID;                           /* +0x00 */
    void* cont;                                   /* +0x04 */
    struct Lego_Object* nextObject;               /* +0x08 */
    Upgrade_PartInfo* partInfo;                   /* +0x0c */
} Upgrade_PartModel;

/* The part of a LegoObject the unit reads: its upgrade part (+0x20). */
typedef struct Upgrade_Object {
    unsigned char pad_00[0x20];
    Upgrade_PartModel* upgradePart;               /* +0x20 */
} Upgrade_Object;

// FUNCTION: LEGORR 0x0046c030
void upgrade_part_load(Upgrade_PartModel* upgradeModel, unsigned int objID,
                       void* root, const char* filename)
{
    memset(upgradeModel, 0, sizeof(*upgradeModel));

    upgradeModel->cont = container_load(root, filename, "LWO", 1);
    if (upgradeModel->cont == NULL)
        upgradeModel->cont = container_load(root, filename, "ACT", 1);
    if (upgradeModel->cont == NULL)
        upgradeModel->cont = container_load(root, filename, "MESH", 1);

    upgradeModel->nextObject = NULL;
    upgradeModel->objID = objID;
}

// FUNCTION: LEGORR 0x0046c0b0
void upgrade_part_clone(Upgrade_PartModel* srcUpgradeModel,
                        Upgrade_PartModel* destUpgradeModel)
{
    *destUpgradeModel = *srcUpgradeModel;
    destUpgradeModel->cont = entity_visual_resource_load(srcUpgradeModel->cont);
}

// FUNCTION: LEGORR 0x0046c0f0
void upgrade_part_remove(Upgrade_PartModel* upgradeModel)
{
    ledger_stub_00472a40_DDiSceneObjectDestroy(upgradeModel->cont);
}

// FUNCTION: LEGORR 0x0046c110
float upgrade_part_move_animation(Upgrade_PartModel* upgradeModel,
                                  float elapsed, unsigned int repeat)
{
    ddiscene_advance_by_time_target(upgradeModel->cont, elapsed);
    return 0.0f;
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
// FUNCTION: LEGORR 0x0046c130
void upgrade_part_set_activity(Upgrade_PartModel* upgradeModel,
                               const char* activityName)
{
    if (!scene_action_name_set(upgradeModel->cont, activityName)) {
        const char* standName = s_activity[0];
        scene_action_name_set(upgradeModel->cont, standName);
    }
}

// FUNCTION: LEGORR 0x0046c160
void upgrade_load(Upgrade_Model* upgrades, BlockFile* config,
                  const char* gameName)
{
    unsigned int loop;
    char* stringParts[10];
    char levelName[256];
    BlockRec* arrayItem;
    Upgrade_PartInfo* partInfo;

    upgrades->count = 0;
    upgrades->parts = (Upgrade_PartInfo*)malloc(sizeof(Upgrade_PartInfo) * UPGRADE_MAXPARTS);
    memset(upgrades->parts, 0, sizeof(Upgrade_PartInfo) * UPGRADE_MAXPARTS);

    for (loop = 0; loop < UPGRADE_MAXLEVELS; loop++) {
        // STRING: LEGORR 0x004aa9e8
        sprintf(levelName, "Level%i%i%i%i", (loop >> 3) & 1, (loop >> 2) & 1,
                (loop >> 1) & 1, loop & 1);
        // STRING: LEGORR 0x004aa9dc
        arrayItem = block_subgroup(config, string_path_join(gameName, "Upgrades", levelName, NULL));
        if (arrayItem) upgrades->levels |= loop;
        for (; arrayItem; arrayItem = block_next_equal_key(arrayItem)) {
            partInfo = &upgrades->parts[upgrades->count];
            strcpy(levelName, arrayItem->value);
            if (sound_variant_split(levelName, stringParts, ",") == 3) {
                partInfo->weaponName = (char*)malloc(strlen(stringParts[2]) + 1);
                strcpy(partInfo->weaponName, stringParts[2]);
            }
            partInfo->nullName = string_duplicate(stringParts[0]);
            partInfo->level = loop;
            partInfo->nullFrameNo = atoi(stringParts[1]) - 1;
            partInfo->upgradeData = lego_upgrade_name_entry_find(arrayItem->key);
            upgrades->count++;
        }
    }

    if (upgrades->count == 0) {
        free(upgrades->parts);
        upgrades->parts = NULL;
    }
}

// FUNCTION: LEGORR 0x0046c340
void upgrade_set_upgrade_level(Upgrade_Model* upgrades, unsigned int objLevel)
{
    struct Lego_Object* obj;
    struct Lego_Object* next;
    Upgrade_PartModel* upgradePart;
    unsigned int loop;

    for (obj = upgrades->objectList; obj; obj = next) {
        upgradePart = ((Upgrade_Object*)obj)->upgradePart;
        next = upgradePart->nextObject;
        legoobject_remove(obj);
    }

    upgrades->currentLevel = objLevel;
    upgrades->objectList = NULL;

    for (loop = 0; loop < upgrades->count; loop++) {
        if (upgrades->parts[loop].level == objLevel) {
            obj = legoobject_create(upgrades->parts[loop].upgradeData, LEGOOBJECT_UPGRADEPART, loop);
            if (upgrades->objectList == NULL) upgrades->objectList = obj;
            else upgradePart->nextObject = obj;
            upgradePart = ((Upgrade_Object*)obj)->upgradePart;
            upgradePart->partInfo = &upgrades->parts[loop];
        }
    }
}

#ifdef __cplusplus
}
#endif
