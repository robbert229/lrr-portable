// SPDX-License-Identifier: LGPL-3.0-or-later
/* Building.c, original translation unit GAME-006 (finding 505), rebuilt in
 * original shape (ADR 0005; finding 592): text 00407d60-004089d0, .rdata
 * 0049e070-0049e084, literals 004a1cbc-004a1d4c. The unit has no .bss.
 *
 * Each building type's model is loaded once from its `.ae` config: the
 * activity container, the named nulls (carry, camera, deposit, entrance,
 * tool, fire, pivots), the upgrades, the footprint shape and the optional
 * power-level scene. Every building object clones that model. Upgrading a
 * building rebuilds its upgrade parts and finds the weapon nulls on them.
 *
 * The port's implementation stays in the port tail building_model_port.cpp,
 * unannotated. In the matching build building_model_link_names.h gives this
 * unit's functions the port's names, so every caller reaches them; natively
 * the port tail serves those names. */
#include "building_model_link_names.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "building_model.h"
#include "building_model_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
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

/* Config's "" (004b8a14) and ":" (004a1cd4), which config.c defines
 * (finding 581). */
extern const char s_config_angle_empty[4];
extern const char s_config_rgb_separator[];

typedef struct Building_Point2I {
    int x, y;
} Building_Point2I;

/* Upgrade.c's part data (GAME-047): the table entry a part was made from. */
typedef struct Building_UpgradeData {
    unsigned int level;                           /* +0x00 */
    unsigned int nullFrameNo;                     /* +0x04 */
    char* nullName;                               /* +0x08 */
    void* weapon;                                 /* +0x0c the weapon slot's name */
} Building_UpgradeData;

/* Upgrade.c's part (LegoObject +0x20). */
typedef struct Building_UpgradePart {
    unsigned int objID;                           /* +0x00 */
    void* cont;                                   /* +0x04 */
    struct Building_PartObject* nextObject;       /* +0x08 */
    Building_UpgradeData* data;                   /* +0x0c */
} Building_UpgradePart;

/* The part of a LegoObject the unit reads: its upgrade part (+0x20). */
typedef struct Building_PartObject {
    unsigned char pad_00[0x20];
    Building_UpgradePart* upgradePart;            /* +0x20 */
} Building_PartObject;

/* Upgrade.c's model (GAME-047), embedded in the building model at +0x8c. */
typedef struct Building_Upgrades {
    unsigned char pad_00[0x08];
    unsigned int currentLevel;                    /* +0x08 */
    unsigned int levels;                          /* +0x0c a bit per type */
    Building_PartObject* firstObject;             /* +0x10 */
} Building_Upgrades;

/* The original's BuildingModel, 0x14c bytes. +0xa0-+0x148 is weapons.c's
 * Weapons_Model (finding 590), of which the unit writes the nulls. */
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
    void* fireNulls[BUILDING_MAXWEAPONS][2];      /* +0x0a0 */
    void* xPivotNulls[BUILDING_MAXWEAPONS];       /* +0x0b8 */
    void* yPivotNulls[BUILDING_MAXWEAPONS];       /* +0x0c4 */
    float pivotMaxZ;                              /* +0x0d0 */
    float pivotMinZ;                              /* +0x0d4 */
    unsigned int fireNullCounts[BUILDING_MAXWEAPONS]; /* +0x0d8 */
    unsigned char weapons_e4[0x48];               /* +0x0e4 */
    void* weaponParts[BUILDING_MAXWEAPONS];       /* +0x12c the parts' data */
    unsigned char weapons_138[0x0c];              /* +0x138 */
    unsigned int weaponCount;                     /* +0x144 */
    unsigned int flags;                           /* +0x148 */
} BuildingModel;

// FUNCTION: LEGORR 0x00407d60
int building_load(BuildingModel* building, unsigned int objID, void* root,
                  const char* filename, const char* gameName)
{
    char* argv[100];
    char baseDir[1024];
    char aeFile[1024];
    unsigned int count, loop;
    BlockFile* config;
    char* shape;
    const char* scene;
    Building_Point2I* point;
    unsigned int numPoints;
    char* str;

    memset(building, 0, sizeof(BuildingModel));
    if ((building->contAct = container_load(root, filename, "ACT", 1))) {
        strcpy(baseDir, filename);
        count = sound_variant_split(baseDir, argv, "\\");
        if (count > 1) {
            for (loop = 1; loop < count; loop++) argv[loop][-1] = '\\';
        }
        sprintf(aeFile, "%s\\%s.%s", baseDir, argv[count - 1], "ae");

        if ((config = block_open(aeFile))) {
            if ((building->carryNullName = block_get_str_copy(config,
                    string_path_join(gameName, "CarryNullName", NULL)))) {
                building->carryNullFrames = atoi(block_get_strptr(config,
                        string_path_join(gameName, "CarryNullFrames", NULL))
                    ? block_get_strptr(config,
                          string_path_join(gameName, "CarryNullFrames", NULL))
                    : s_config_angle_empty);
            } else {
                building->carryNullFrames = 0;
            }

            if ((building->cameraNullName = block_get_str_copy(config,
                    string_path_join(gameName, "CameraNullName", NULL)))) {
                building->cameraNullFrames = atoi(block_get_strptr(config,
                        string_path_join(gameName, "CameraNullFrames", NULL))
                    ? block_get_strptr(config,
                          string_path_join(gameName, "CameraNullFrames", NULL))
                    : s_config_angle_empty);
            } else {
                building->cameraNullFrames = 0;
            }

            if ((building->toolNullName = block_get_str_copy(config,
                    string_path_join(gameName,
                                     // STRING: LEGORR 0x004a1d3c
                                     "ToolNullName",
                                     NULL)))) {
                building->toolNullFrames = atoi(block_get_strptr(config,
                        string_path_join(gameName,
                                         // STRING: LEGORR 0x004a1d2c
                                         "ToolNullFrames",
                                         NULL))
                    ? block_get_strptr(config,
                          string_path_join(gameName, "ToolNullFrames", NULL))
                    : s_config_angle_empty);
            } else {
                building->toolNullFrames = 0;
            }

            building->depositNullName = block_get_str_copy(config,
                string_path_join(gameName, "DepositNullName", NULL));
            building->fireNullName = block_get_str_copy(config,
                string_path_join(gameName, "FireNullName", NULL));
            building->entranceNullName = block_get_str_copy(config,
                string_path_join(gameName,
                                 // STRING: LEGORR 0x004a1d08
                                 "EntranceNullName",
                                 NULL));
            building->yPivotNullName = block_get_str_copy(config,
                string_path_join(gameName, "yPivot", NULL));
            building->xPivotNullName = block_get_str_copy(config,
                string_path_join(gameName, "xPivot", NULL));

            if (block_get_strptr(config, string_path_join(gameName, "PivotMaxZ", NULL))) {
                building->pivotMaxZ = (float)atof(block_get_strptr(config,
                        string_path_join(gameName, "PivotMaxZ", NULL))
                    ? block_get_strptr(config,
                          string_path_join(gameName, "PivotMaxZ", NULL))
                    : s_config_angle_empty);
            } else {
                building->pivotMaxZ = 1.0f;
            }
            if (block_get_strptr(config, string_path_join(gameName, "PivotMinZ", NULL))) {
                building->pivotMinZ = (float)atof(block_get_strptr(config,
                        string_path_join(gameName, "PivotMinZ", NULL))
                    ? block_get_strptr(config,
                          string_path_join(gameName, "PivotMinZ", NULL))
                    : s_config_angle_empty);
            } else {
                building->pivotMinZ = -1.0f;
            }

            upgrade_load((struct Upgrade_Model*)&building->upgrades, config, gameName);

            building->shapePoints = (Building_Point2I*)malloc(sizeof(Building_Point2I) * BUILDING_MAXSHAPEPOINTS);
            building->shapeCount = 1;
            building->shapePoints[0].x = building->shapePoints[0].y = 0;
            if ((shape = block_get_str_copy(config,
                    string_path_join(gameName,
                                     // STRING: LEGORR 0x004a1cd8
                                     "Shape",
                                     NULL)))) {
                numPoints = sound_variant_split(shape, argv, s_config_rgb_separator);
                for (loop = 0; loop < numPoints; loop++) {
                    str = argv[loop];
                    point = &building->shapePoints[building->shapeCount++];
                    point->x = atoi(str);
                    point->y = atoi(strstr(str, ",") + 1);
                }
                free(shape);
            }

            if ((scene = block_get_strptr(config, string_path_join(gameName,
                    // STRING: LEGORR 0x004a1cc4
                    "PowerLevelScene", NULL)))) {
                sprintf(aeFile, "%s\\%s", baseDir, scene);
                building->contPowerLevel = container_load(root, aeFile, "LWS", 1);
            }

            block_close(config);
        }

        scene_render_lock_toggle(building->contAct, 1);
        building->objID = objID;
        building->flags = 0;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x004082e0
void building_change_power_level(BuildingModel* building, int increment)
{
    if (increment) {
        building->powerLevel = building->powerLevel - (1.0f - building->powerLevel) * -0.1f;
    } else {
        /* (level - 0.1) * 10 / 9. Written this way round, MSVC keeps the two
         * multiplies; the other order folds the constants (finding 554). */
        building->powerLevel = 0.111111112f * ((building->powerLevel - 0.1f) * 10.0f);
    }
    if (building->powerLevel < 0.0f) building->powerLevel = 0.0f;
    building->flags |= BUILDING_FLAG_POWERLEVELCHANGED;
}

// FUNCTION: LEGORR 0x00408360
void* building_get_shape_points(BuildingModel* building, unsigned int* count)
{
    if (count) *count = building->shapeCount;
    return building->shapePoints;
}

// FUNCTION: LEGORR 0x00408380
void building_set_owner_object(BuildingModel* building, void* liveObj)
{
    scene_object_owner_set(building->contAct, (uint32_t)(size_t)liveObj);
}

// FUNCTION: LEGORR 0x004083a0
void building_set_upgrade_activity(BuildingModel* building, const char* activityName)
{
    Building_PartObject* object;
    Building_UpgradePart* part;
    Building_UpgradeData* data;
    void* parent;
    void* cont;
    const char* name;
    unsigned int loop, index;
    int instance;

    object = building->upgrades.firstObject;
    /* The first object is read before the weapon slots are cleared, and the
     * part names are formatted into a temporary before each search, as the
     * original orders its pushes. */
    for (loop = 0; loop < BUILDING_MAXWEAPONS; loop++) {
        building->weaponParts[loop] = NULL;
        building->yPivotNulls[loop] = NULL;
        building->xPivotNulls[loop] = NULL;
        memset(building->fireNulls[loop], 0, sizeof(building->fireNulls[loop]));
    }
    building->weaponCount = 0;

    while (object) {
        part = object->upgradePart;
        data = part->data;
        parent = animation_object_null_find((AnimationObject*)building, data->nullName, data->nullFrameNo);
        cont = animation_object_handle_get(part);
        ddiscene_scene_object_attach_or_unlink(cont, parent);
        render_object_set_identity_transform(cont);
        if (activityName) {
            scene_object_set_animation_with_fallback((struct RenderResourceBinding*)part, (void*)activityName);
        } else {
            const char* standName = s_activity[0];
            scene_action_name_set(part->cont, standName);
        }

        if (data->weapon) {
            index = building->weaponCount;
            instance = 0;
            building->fireNullCounts[index] = 0;
            for (loop = 0; loop < 2; loop++) {
                building->fireNulls[index][loop] = NULL;
                name = xf_texture_name_format(building->contAct, building->fireNullName,
                                              (const int*)&building->fireNullCounts[index]);
                building->fireNulls[index][building->fireNullCounts[index]] =
                    resource_name_register_or_query_target((void**)cont, name, 0, NULL);
                if (!building->fireNulls[index][building->fireNullCounts[index]++]) break;
            }
            name = xf_texture_name_format(building->contAct, building->xPivotNullName, &instance);
            building->xPivotNulls[index] = resource_name_register_or_query_target((void**)cont, name, 0, NULL);
            name = xf_texture_name_format(building->contAct, building->yPivotNullName, &instance);
            building->yPivotNulls[index] = resource_name_register_or_query_target((void**)cont, name, 0, NULL);
            building->weaponParts[index] = data;
            building->weaponCount++;
        }
        object = part->nextObject;
    }
}

// FUNCTION: LEGORR 0x00408570
unsigned int building_set_activity(BuildingModel* building, const char* activityName,
                                   float time)
{
    unsigned int loop;
    unsigned int result;

    for (loop = 0; loop < building->carryNullFrames; loop++) building->carryNulls[loop] = NULL;
    for (loop = 0; loop < building->cameraNullFrames; loop++) building->cameraNulls[loop] = NULL;
    building->depositNull = NULL;
    building->entranceNull = NULL;
    result = scene_action_name_set(building->contAct, activityName);
    ddiscene_frame_update_and_callbacks(building->contAct, time);
    building_set_upgrade_activity(building, activityName);
    return result;
}

// FUNCTION: LEGORR 0x004085f0
void* building_get_camera_null(BuildingModel* building, unsigned int frameNo)
{
    /* The result shares the name's register, as in the original, where a
     * NULL name is returned as is (finding 592: 90%; the original saves edi
     * before the test, this compile after it, as in Creature.c's 0x406f80). */
    void* null;

    if ((null = building->cameraNullName)) {
        if (!building->cameraNulls[frameNo]) {
            building->cameraNulls[frameNo] = animation_object_null_find((AnimationObject*)building,
                                                                         building->cameraNullName, frameNo);
        }
        null = building->cameraNulls[frameNo];
    }
    return null;
}

// FUNCTION: LEGORR 0x00408620
void building_clone(BuildingModel* srcBuilding, BuildingModel* destBuilding)
{
    *destBuilding = *srcBuilding;
    destBuilding->flags &= ~BUILDING_FLAG_SOURCE;
    destBuilding->contAct = entity_visual_resource_load(srcBuilding->contAct);
    if (srcBuilding->contPowerLevel) {
        destBuilding->contPowerLevel = entity_visual_resource_load(srcBuilding->contPowerLevel);
    }
}

// FUNCTION: LEGORR 0x00408670
void building_hide(BuildingModel* building, int hide)
{
    scene_render_lock_toggle(building->contAct, hide);
    if (building->contPowerLevel) scene_render_lock_toggle(building->contPowerLevel, hide);
}

// FUNCTION: LEGORR 0x004086a0
unsigned int building_is_hidden(BuildingModel* building)
{
    return scene_object_flag_bit3_get_target(building->contAct);
}

// FUNCTION: LEGORR 0x004086c0
void building_set_orientation(BuildingModel* building, float xDir, float yDir)
{
    ddiscene_object_pose_method_dispatch(building->contAct, NULL, xDir, yDir, 0.0f, 0.0f, 0.0f, -1.0f);
    if (building->contPowerLevel) {
        ddiscene_object_pose_method_dispatch(building->contPowerLevel, NULL, xDir, yDir, 0.0f, 0.0f, 0.0f, -1.0f);
    }
}

// FUNCTION: LEGORR 0x00408710
void building_set_position(BuildingModel* building, float xPos, float yPos,
                           BuildingModel_GetWorldZCallback zCallback,
                           struct TerrainMap* surfMap)
{
    float zPos = zCallback(xPos, yPos, surfMap);

    ddiscene_object_position_set(building->contAct, NULL, xPos, yPos, zPos);
    if (building->contPowerLevel) {
        ddiscene_object_position_set(building->contPowerLevel, NULL, xPos, yPos, zPos);
    }
}

// FUNCTION: LEGORR 0x00408770
void* building_get_carry_null(BuildingModel* building, unsigned int frameNo)
{
    if (building->carryNullName) {
        if (building->carryNulls[frameNo]) return building->carryNulls[frameNo];
        return building->carryNulls[frameNo] = animation_object_null_find((AnimationObject*)building,
                                                                          building->carryNullName, frameNo);
    }
    return NULL;
}

// FUNCTION: LEGORR 0x004087b0
void* building_get_deposit_null(BuildingModel* building)
{
    if (building->depositNullName) {
        if (building->depositNull) return building->depositNull;
        return building->depositNull = animation_object_null_find((AnimationObject*)building,
                                                                  building->depositNullName, 0);
    }
    return NULL;
}

// FUNCTION: LEGORR 0x004087e0
void* building_get_entrance_null(BuildingModel* building)
{
    if (building->entranceNullName) {
        if (building->entranceNull) return building->entranceNull;
        return building->entranceNull = animation_object_null_find((AnimationObject*)building,
                                                                   building->entranceNullName, 0);
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00408810
void* building_get_tool_null(BuildingModel* building, unsigned int frameNo)
{
    if (building->toolNullName) {
        if (building->toolNulls[frameNo]) return building->toolNulls[frameNo];
        return building->toolNulls[frameNo] = animation_object_null_find((AnimationObject*)building,
                                                                         building->toolNullName, frameNo);
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00408850
unsigned int building_get_carry_null_frames(BuildingModel* building)
{
    return building->carryNullFrames;
}

// FUNCTION: LEGORR 0x00408860
float building_move_animation(BuildingModel* building, float elapsed, unsigned int repeats)
{
    float time = ddiscene_advance_by_time_target(building->contAct, elapsed);
    float overrun;

    if (repeats > 1 && 0.0f != time) {
        overrun = time - (float)(ddiscene_object_frame_position_get(building->contAct) * (repeats - 1));
    } else {
        overrun = time;
    }
    if (building->flags & BUILDING_FLAG_POWERLEVELCHANGED) {
        if (building->contPowerLevel) {
            ddiscene_frame_update_and_callbacks(building->contPowerLevel,
                (float)ddiscene_object_frame_position_get(building->contPowerLevel) * building->powerLevel);
        }
        building->flags &= ~BUILDING_FLAG_POWERLEVELCHANGED;
    }
    return overrun;
}

// FUNCTION: LEGORR 0x00408930
float building_get_trans_coef(BuildingModel* building)
{
    return 0.0f;
}

// FUNCTION: LEGORR 0x00408940
void building_remove(BuildingModel* building)
{
    building_set_upgrade_level(building, (unsigned int)-1);
    if (building->contPowerLevel) ledger_stub_00472a40_DDiSceneObjectDestroy(building->contPowerLevel);
    ledger_stub_00472a40_DDiSceneObjectDestroy(building->contAct);
}

// FUNCTION: LEGORR 0x00408970
unsigned int building_can_upgrade_type(BuildingModel* building,
                                       unsigned int upgradeType, int current)
{
    unsigned int mask = 1 << upgradeType;

    if (current && (building->upgrades.levels & mask)) return 0;
    return building->upgrades.levels & mask;
}

// FUNCTION: LEGORR 0x004089a0
void building_set_upgrade_level(BuildingModel* building, unsigned int objLevel)
{
    scene_indexed_record_list_rebuild_for_value(
        (struct SceneIndexedRecordList*)&building->upgrades, (int)objLevel);
    building_set_upgrade_activity(building, NULL);
}

#ifdef __cplusplus
}
#endif
