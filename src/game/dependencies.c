// SPDX-License-Identifier: LGPL-3.0-or-later
/* Dependencies.c, original translation unit GAME-010 (finding 505), rebuilt
 * in original shape (ADR 0005; finding 612): text 0040ab10-0040b310, the
 * literals 004a1d8c-004a1d98 and the .bss block 004b8bc8-004c717c.
 *
 * Lego.cfg's Dependencies block: what an object type (at a level) needs
 * before it can be built or trained. Each entry names the object, optionally
 * with HitOnceStay (once met, the requirements stay met) and a level, and
 * lists the required objects, each optionally with a level. The table holds,
 * for each object type and index, a requirement list per level and the
 * level's flags; Dependencies_Update re-checks them against the live
 * objects.
 *
 * The unit starts at 0040ab10, not 0040ab70: Dependencies_SetEnabled and
 * Dependencies_Reset_ClearAllLevelFlags_10c touch only this table (finding
 * 612). The port's implementation stays in the port tail
 * dependencies_port.cpp, unannotated. In the matching build
 * dependencies_link_names.h gives this unit's functions the port's names,
 * so every caller reaches them; natively the port tail serves those
 * names. */
#include "dependencies_link_names.h"

#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "dependencies.h"
#include "dependencies_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

#define DEPENDENCIES_OBJECTTYPES 20
#define DEPENDENCIES_OBJECTIDS 15
#define DEPENDENCIES_MAXLEVELS 16

/* levelFlags. */
#define DEPENDENCY_FLAG_HITONCESTAY 0x1
#define DEPENDENCY_FLAG_HITONCE 0x2     /* HitOnceStay requirements met */
#define DEPENDENCY_FLAG_UNLOCKED 0x4
#define DEPENDENCY_FLAG_LOCKED 0x8      /* unlocked once, then lost */
#define DEPENDENCY_FLAG_100 0x100

/* One required object (0x10 bytes). */
typedef struct Dependencies_Requirement {
    int objType;
    unsigned int objID;
    unsigned int objLevel;
    int hasLevel;
} Dependencies_Requirement;

typedef struct Dependencies_Object {
    unsigned int requirementsCount[DEPENDENCIES_MAXLEVELS];           /* +0x00 */
    Dependencies_Requirement* requirementsList[DEPENDENCIES_MAXLEVELS]; /* +0x40 */
    int hasLevels;                                                     /* +0x80 */
    unsigned int levelFlags[DEPENDENCIES_MAXLEVELS];                   /* +0x84 */
} Dependencies_Object; /* 0xc4 */

typedef struct Dependencies_Globs {
    Dependencies_Object objects[DEPENDENCIES_OBJECTTYPES][DEPENDENCIES_OBJECTIDS]; /* +0x0000 004b8bc8 */
    int disabled;                                                      /* +0xe5b0 004c7178 */
} Dependencies_Globs; /* 0xe5b4 */

/* What Dependencies_Object_FindDependents collects (0x204 bytes, the
 * caller's). */
typedef struct Dependencies_Dependents {
    int objTypes[32];           /* +0x000 */
    unsigned int objIDs[32];    /* +0x080 */
    unsigned int objLevels[32]; /* +0x100 */
    int hasLevels[32];          /* +0x180 */
    unsigned int count;         /* +0x200 */
} Dependencies_Dependents;

// GLOBAL: LEGORR 0x004b8bc8
static Dependencies_Globs dependencyGlobs;

// FUNCTION: LEGORR 0x0040ab10
void dependencies_set_enabled(int on)
{
    dependencyGlobs.disabled = !on;
}

// FUNCTION: LEGORR 0x0040ab30
void dependencies_reset_clear_all_level_flags_10c(void)
{
    unsigned int objType;
    unsigned int objID;
    unsigned int objLevel;

    for (objType = 0; objType < DEPENDENCIES_OBJECTTYPES; objType++) {
        for (objID = 0; objID < DEPENDENCIES_OBJECTIDS; objID++) {
            for (objLevel = 0; objLevel < DEPENDENCIES_MAXLEVELS; objLevel++) {
                dependencyGlobs.objects[objType][objID].levelFlags[objLevel] &=
                    ~(DEPENDENCY_FLAG_UNLOCKED | DEPENDENCY_FLAG_LOCKED | DEPENDENCY_FLAG_100);
            }
        }
    }
}

// FUNCTION: LEGORR 0x0040ab70
void dependencies_initialise(BlockRec* config, const char* gameName)
{
    BlockRec* arrayItem;
    char* stringParts[3];
    char* reqParts[3];
    char* requirements[10];
    unsigned int numParts;
    unsigned int numReqParts;
    unsigned int numRequirements;
    unsigned int objLevel;
    int hitOnceStay;
    int objType;
    unsigned int objID;
    int reqType;
    unsigned int reqID;
    unsigned int reqLevel;
    unsigned int i;

    // STRING: LEGORR 0x004a1d98
    for (arrayItem = block_subgroup(config, string_path_join(gameName, "Dependencies", 0));
         arrayItem != NULL; arrayItem = block_next_equal_key(arrayItem)) {
        numParts = sound_variant_split(arrayItem->key, stringParts, s_config_rgb_separator);
        if (lego_entity_type_pair_lookup(stringParts[1], &objType, &objID, NULL)) {
            // STRING: LEGORR 0x004a1d8c
            hitOnceStay = !_strcmpi(stringParts[0], "HitOnceStay");
            if (numParts == 3) {
                objLevel = atoi(stringParts[2]);
                building_upgrade_admission_check(objType, objID);
                if (hitOnceStay) {
                    dependencyGlobs.objects[objType][objID].levelFlags[objLevel] |= DEPENDENCY_FLAG_HITONCESTAY;
                }
                dependencyGlobs.objects[objType][objID].hasLevels = 1;
            } else {
                objLevel = 0;
                if (hitOnceStay) {
                    for (i = 0; i < DEPENDENCIES_MAXLEVELS; i++) {
                        dependencyGlobs.objects[objType][objID].levelFlags[i] |= DEPENDENCY_FLAG_HITONCESTAY;
                    }
                }
                dependencyGlobs.objects[objType][objID].hasLevels = 0;
            }

            numRequirements = sound_variant_split(arrayItem->value, requirements, ",");
            dependencyGlobs.objects[objType][objID].requirementsList[objLevel] =
                (Dependencies_Requirement*)malloc(numRequirements * sizeof(Dependencies_Requirement));
            dependencyGlobs.objects[objType][objID].requirementsCount[objLevel] = numRequirements;
            for (i = 0; i < numRequirements; i++) {
                numReqParts = sound_variant_split(requirements[i], reqParts, s_config_rgb_separator);
                if (lego_entity_type_pair_lookup(reqParts[0], &reqType, &reqID, NULL)) {
                    dependencyGlobs.objects[objType][objID].requirementsList[objLevel][numRequirements - i - 1].objType = reqType;
                    dependencyGlobs.objects[objType][objID].requirementsList[objLevel][numRequirements - i - 1].objID = reqID;
                    if (numReqParts == 2) {
                        reqLevel = atoi(reqParts[1]);
                        building_upgrade_admission_check(reqType, reqID);
                        dependencyGlobs.objects[objType][objID].requirementsList[objLevel][numRequirements - i - 1].objLevel = reqLevel;
                        dependencyGlobs.objects[objType][objID].requirementsList[objLevel][numRequirements - i - 1].hasLevel = 1;
                    } else {
                        dependencyGlobs.objects[objType][objID].requirementsList[objLevel][numRequirements - i - 1].hasLevel = 0;
                    }
                }
            }
        }
    }
    dependencies_update();
}

// FUNCTION: LEGORR 0x0040aea0
int dependencies_object_requirements_met(int objType, unsigned int objID, unsigned int objLevel)
{
    unsigned int* flags;
    Dependencies_Requirement* requirements;
    unsigned int count;
    unsigned int i;

    if (!dependencyGlobs.disabled) {
        flags = &dependencyGlobs.objects[objType][objID].levelFlags[objLevel];
        if (!(*flags & DEPENDENCY_FLAG_HITONCE)) {
            dependencies_object_get_requirements(objType, objID, objLevel, &requirements, &count);
            for (i = 0; i < count; i++) {
                if (!legoobject_run_through_lists_skip_upgrade_parts(dependencies_live_object_callback, &requirements[i])) {
                    return 0;
                }
            }
            if (*flags & DEPENDENCY_FLAG_HITONCESTAY) *flags |= DEPENDENCY_FLAG_HITONCE;
        }
    }
    return 1;
}

// FUNCTION: LEGORR 0x0040af40
int dependencies_live_object_callback(struct Lego_Object* liveObj, void* data)
{
    Dependencies_Requirement* search = (Dependencies_Requirement*)data;

    if (!legoobject_task_kind_filtered_search(search->objType)) {
        if (search->objType != *(int*)((char*)liveObj + 0x0)) return 0;
        if (search->objID != *(unsigned int*)((char*)liveObj + 0x4)) return 0;
        if (search->hasLevel && search->objLevel > *(unsigned int*)((char*)liveObj + 0x330)) return 0;
    }
    return 1;
}

// FUNCTION: LEGORR 0x0040af90
void dependencies_object_get_requirements(int objType, unsigned int objID, unsigned int objLevel,
                                          Dependencies_Requirement** requirements,
                                          unsigned int* count)
{
    if (dependencyGlobs.objects[objType][objID].hasLevels) {
        *requirements = dependencyGlobs.objects[objType][objID].requirementsList[objLevel];
        *count = dependencyGlobs.objects[objType][objID].requirementsCount[objLevel];
    } else {
        *requirements = dependencyGlobs.objects[objType][objID].requirementsList[0];
        *count = dependencyGlobs.objects[objType][objID].requirementsCount[0];
    }
}

// FUNCTION: LEGORR 0x0040b000
void dependencies_object_find_dependents(int objType, unsigned int objID, unsigned int objLevel,
                                         Dependencies_Dependents* dependents)
{
    unsigned int type;
    unsigned int id;
    unsigned int level;
    unsigned int i;
    Dependencies_Requirement* requirements;
    unsigned int count;

    dependents->count = 0;
    for (type = 0; type < DEPENDENCIES_OBJECTTYPES; type++) {
        for (id = 0; id < DEPENDENCIES_OBJECTIDS; id++) {
            if (dependencyGlobs.objects[type][id].hasLevels) {
                for (level = 0; level < DEPENDENCIES_MAXLEVELS; level++) {
                    dependencies_object_get_requirements(type, id, level, &requirements, &count);
                    for (i = 0; i < count; i++) {
                        if (requirements[i].objType == objType && requirements[i].objID == objID &&
                            (!requirements[i].hasLevel || requirements[i].objLevel == objLevel)) {
                            dependents->objTypes[dependents->count] = type;
                            dependents->objIDs[dependents->count] = id;
                            dependents->objLevels[dependents->count] = level;
                            dependents->hasLevels[dependents->count] = 1;
                            dependents->count++;
                        }
                    }
                }
            } else {
                dependencies_object_get_requirements(type, id, 0, &requirements, &count);
                for (i = 0; i < count; i++) {
                    if (requirements[i].objType == objType && requirements[i].objID == objID &&
                        (!requirements[i].hasLevel || requirements[i].objLevel == objLevel)) {
                        dependents->objTypes[dependents->count] = type;
                        dependents->objIDs[dependents->count] = id;
                        dependents->hasLevels[dependents->count] = 0;
                        dependents->count++;
                    }
                }
            }
        }
    }
}

// FUNCTION: LEGORR 0x0040b1b0
void dependencies_update(void)
{
    unsigned int objType;
    unsigned int objID;
    unsigned int objLevel;
    unsigned int* flags;

    for (objType = 0; objType < DEPENDENCIES_OBJECTTYPES; objType++) {
        for (objID = 0; objID < DEPENDENCIES_OBJECTIDS; objID++) {
            for (objLevel = 0; objLevel < DEPENDENCIES_MAXLEVELS; objLevel++) {
                if (objLevel == 1 && dependencyGlobs.objects[objType][objID].hasLevels) break;
                flags = &dependencyGlobs.objects[objType][objID].levelFlags[objLevel];
                if (!(*flags & DEPENDENCY_FLAG_LOCKED)) {
                    if (dependencies_object_requirements_met(objType, objID, objLevel)) {
                        if (!(*flags & DEPENDENCY_FLAG_UNLOCKED)) {
                            *flags |= DEPENDENCY_FLAG_UNLOCKED;
                        } else {
                            *flags = (*flags & ~DEPENDENCY_FLAG_UNLOCKED) | DEPENDENCY_FLAG_LOCKED;
                        }
                    } else if (*flags & DEPENDENCY_FLAG_UNLOCKED) {
                        *flags = (*flags & ~DEPENDENCY_FLAG_UNLOCKED) | DEPENDENCY_FLAG_LOCKED;
                    }
                }
            }
        }
    }
}

// FUNCTION: LEGORR 0x0040b250
unsigned int dependencies_object_is_unlocked(int objType, unsigned int objID, unsigned int objLevel)
{
    if (dependencyGlobs.objects[objType][objID].hasLevels) {
        return dependencyGlobs.objects[objType][objID].levelFlags[objLevel] & DEPENDENCY_FLAG_UNLOCKED;
    }
    return dependencyGlobs.objects[objType][objID].levelFlags[0] & DEPENDENCY_FLAG_UNLOCKED;
}

// FUNCTION: LEGORR 0x0040b2a0
void dependencies_object_add_level_flag_100(int objType, unsigned int objID, unsigned int objLevel)
{
    dependencyGlobs.objects[objType][objID].levelFlags[objLevel] |= DEPENDENCY_FLAG_100;
}

// FUNCTION: LEGORR 0x0040b2e0
unsigned int dependencies_object_get_level_flag_100(int objType, unsigned int objID, unsigned int objLevel)
{
    return dependencyGlobs.objects[objType][objID].levelFlags[objLevel] & DEPENDENCY_FLAG_100;
}

#ifdef __cplusplus
}
#endif
