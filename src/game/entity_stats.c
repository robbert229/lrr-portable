// SPDX-License-Identifier: LGPL-3.0-or-later
/* Stats.c (original translation unit GAME-043, finding 505; rebuilt in
 * original shape, ADR 0005, finding 581). Text 004667e0-0046a370, .bss
 * 00502bd8-00503188, literals 004a9ce0-004aa448.
 *
 * Stats_Initialise reads the game config's Stats block: for every object
 * named there it allocates `Levels` StatsData records (0x150 bytes each),
 * fills them from the per-level and per-object keys, and points
 * statsGlobs.objectStats[type][id] at them. The getters read an object's
 * current record (LegoObject +0x334), or a record by type, ID and level.
 *
 * The port's stats loader (entity_config.cpp, entity.cpp) fills the same
 * statsGlobs through entity_stats_port_globs(); the port's own getters are
 * in the port tail entity_stats_port.cpp, for native builds only. */
/* Matching build: LegoObject.c callers call legoobject_unit.c (#588). */
#include "legoobject_port_aliases.h"
#include "entity_stats_link_names.h"
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "entity_stats.h"
#include "gods98/config.h"
#include "gods98/utils.h"
#include "gods98/sound.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Lego.c (GAME-021, lego_unit.c) */
int lego_entity_type_pair_lookup(const char* objName, int* outType,
                                 unsigned int* outID,
                                 void** outModel);          /* 0042e8b0 */
unsigned int lego_stats_level_record_count_for_stat_type(int objType); /* 0042ef40 */
/* GAME-007 (placement.cpp): Lego_GetBuildingBase... by name */
unsigned int string_table_index_lookup(const char* value);  /* 00408c80 */
/* LegoObject.c (GAME-023) */
void entity_kind_four_post_create_resolve(StatsObject* obj); /* 0043c570 */
extern unsigned int s_grid_count_second[0x12c0];            /* 004dee44 */
extern unsigned int s_grid_count_first[0x12c0];             /* 004e3944 */
/* Config.c (G98-006): the original's pooled ":" and "" resolve to these. */
extern const char s_config_rgb_separator[];                 /* 004a1cd4 ":" */
extern const char s_config_angle_empty[4];                  /* 004b8a14 "" */
#ifdef __cplusplus
}
#endif

// GLOBAL: LEGORR 0x00502bd8
static Stats_Globs statsGlobs;

/* Config.h's lookups as the original expands them: the path is built
 * again for each lookup, and a missing item reads as "". */
#define STATS_ID(name) \
    string_path_join(gameName, STATS_KEY, item->key, (name), NULL)
#define STATS_STR(name) \
    (block_get_strptr(config, STATS_ID(name)) ? \
     block_get_strptr(config, STATS_ID(name)) : s_config_angle_empty)
#define STATS_INT(name) atoi(STATS_STR(name))
#define STATS_REAL(name) atof(STATS_STR(name))
#define STATS_BOOL(name) block_get_bool_path(config, STATS_ID(name))

#define STATS_RECORD(loop) (statsGlobs.objectStats[objType][objID][(loop)])

// FUNCTION: LEGORR 0x004667e0
int stats_initialise(BlockRec* config, const char* gameName)
{
    char* drillTimes[STATS_SURFACE_TYPES] = { NULL };
    char* stringParts[32];
    BlockRec* item;
    int objType;
    unsigned int objID;
    unsigned int levels;
    unsigned int loop;
    unsigned int flags1;
    unsigned int flags2;
    unsigned int flags3;
    unsigned int count;
    unsigned int surface;
    char* str;
    const char* tempStr;
    int buildingBase;
    int costCrystal;
    int costOre;
    int costRefinedOre;
    float trackDist;
    float alertRadius;
    float collHeight;
    float healthDecayRate;
    float energyDecayRate;
    float pickSphere;
    int drillSound;
    int drillFadeSound;
    int engineSound;
    float restPercent;
    float carryMinHealth;
    float stampRadius;
    float attackRadius;
    int waterEntrances;
    float rubbleCoef;
    float pathCoef;
    float oxygenCoef;
    int capacity;
    float collRadius;
    StatsPoint2F collBox;
    float flocksTurn;
    float flocksSpeed;
    float flocksTightness;
    float flocksGoalUpdate;
    float flocksHeight;
    float flocksRandomness;
    unsigned int flocksSize;
    float flocksAttackTime;
    float awarenessRange;
    float painThreshold;
    float freezerTime;
    float freezerDamage;
    float pusherDist;
    float pusherDamage;
    float laserDamage;
    float wakeRadius;
    float randomMoveTime;

    // STRING: LEGORR 0x004aa448
#define STATS_KEY "Stats"
    for (item = block_subgroup(config, string_path_join(gameName, STATS_KEY, NULL));
         item != NULL; item = block_next_equal_key(item)) {
        flags3 = 0;
        flags2 = 0;
        flags1 = 0;

        // STRING: LEGORR 0x004a2574
        levels = STATS_INT("Levels");
        if (lego_entity_type_pair_lookup(item->key, &objType, &objID, NULL)) {
            if (statsGlobs.objectStats[objType] == NULL) {
                count = lego_stats_level_record_count_for_stat_type(objType);
                statsGlobs.objectStats[objType] = (StatsData**)malloc(count * sizeof(StatsData*));
                memset(statsGlobs.objectStats[objType], 0, count * sizeof(StatsData*));
            }
            statsGlobs.objectLevels[objType][objID] = levels;
            statsGlobs.objectStats[objType][objID] = (StatsData*)malloc(levels * sizeof(StatsData));
            memset(statsGlobs.objectStats[objType][objID], 0, levels * sizeof(StatsData));

            // STRING: LEGORR 0x004aa43c
            if ((str = block_get_str_copy(config, STATS_ID("RouteSpeed"))) != NULL) {
                sound_variant_split(str, stringParts, s_config_rgb_separator);
                for (loop = 0; loop < levels; loop++) {
                    STATS_RECORD(loop).RouteSpeed = (float)atof(stringParts[loop]);
                }
                free(str);
            }

            // STRING: LEGORR 0x004aa42c
            drillTimes[5] = block_get_str_copy(config, STATS_ID("SoilDrillTime"));
            // STRING: LEGORR 0x004aa41c
            drillTimes[4] = block_get_str_copy(config, STATS_ID("LooseDrillTime"));
            // STRING: LEGORR 0x004aa40c
            drillTimes[3] = block_get_str_copy(config, STATS_ID("MedDrillTime"));
            // STRING: LEGORR 0x004aa3fc
            drillTimes[2] = block_get_str_copy(config, STATS_ID("HardDrillTime"));
            // STRING: LEGORR 0x004aa3ec
            drillTimes[8] = block_get_str_copy(config, STATS_ID("SeamDrillTime"));
            if (drillTimes[8] != NULL) drillTimes[10] = string_duplicate(drillTimes[8]);
            else drillTimes[10] = NULL;

            for (surface = 0; surface < STATS_SURFACE_TYPES; surface++) {
                if (drillTimes[surface] != NULL) {
                    sound_variant_split(drillTimes[surface], stringParts, s_config_rgb_separator);
                    for (loop = 0; loop < levels; loop++) {
                        STATS_RECORD(loop).DrillTimes[surface] = (float)atof(stringParts[loop]) * 25.0f;
                    }
                    free(drillTimes[surface]);
                }
            }

            // STRING: LEGORR 0x004aa3dc
            if ((tempStr = block_get_strptr(config, STATS_ID("BuildingBase"))) != NULL) {
                buildingBase = string_table_index_lookup(tempStr);
            } else {
                buildingBase = -1;
            }

            // STRING: LEGORR 0x004aa3d0
            costCrystal = STATS_INT("CostCrystal");
            // STRING: LEGORR 0x004aa3c8
            costOre = STATS_INT("CostOre");
            // STRING: LEGORR 0x004aa3b8
            costRefinedOre = STATS_INT("CostRefinedOre");
            // STRING: LEGORR 0x004aa3ac
            trackDist = STATS_REAL("TrackDist");
            // STRING: LEGORR 0x004aa3a0
            alertRadius = STATS_REAL("AlertRadius");
            // STRING: LEGORR 0x004aa394
            collHeight = STATS_REAL("CollHeight");
            // STRING: LEGORR 0x004aa384
            healthDecayRate = STATS_REAL("HealthDecayRate");
            // STRING: LEGORR 0x004aa374
            energyDecayRate = STATS_REAL("EnergyDecayRate");
            // STRING: LEGORR 0x004aa368
            pickSphere = STATS_REAL("PickSphere");
            // STRING: LEGORR 0x004aa35c
            if (!sound_alias_lookup(block_get_strptr(config, STATS_ID("DrillSound")), &drillSound)) drillSound = 2;
            // STRING: LEGORR 0x004aa34c
            if (!sound_alias_lookup(block_get_strptr(config, STATS_ID("DrillFadeSound")), &drillFadeSound)) drillFadeSound = 3;
            // STRING: LEGORR 0x004aa340
            if (!sound_alias_lookup(block_get_strptr(config, STATS_ID("EngineSound")), &engineSound)) engineSound = 0;
            // STRING: LEGORR 0x004aa334
            restPercent = STATS_REAL("RestPercent");
            // STRING: LEGORR 0x004aa324
            carryMinHealth = STATS_REAL("CarryMinHealth");
            // STRING: LEGORR 0x004aa318
            stampRadius = STATS_REAL("StampRadius");
            // STRING: LEGORR 0x004aa308
            attackRadius = STATS_REAL("AttackRadius");
            // STRING: LEGORR 0x004aa2f8
            waterEntrances = STATS_INT("WaterEntrances");
            // STRING: LEGORR 0x004aa2ec
            rubbleCoef = STATS_REAL("RubbleCoef");
            // STRING: LEGORR 0x004aa2e0
            pathCoef = STATS_REAL("PathCoef");
            // STRING: LEGORR 0x004aa2d4
            oxygenCoef = STATS_REAL("OxygenCoef");
            // STRING: LEGORR 0x004aa2c8
            capacity = STATS_INT("Capacity");

            if (0.0f == rubbleCoef) rubbleCoef = 1.0f;
            if (0.0f == pathCoef) pathCoef = 1.0f;

            // STRING: LEGORR 0x004aa2bc
            if (0.0f == (collRadius = STATS_REAL("CollRadius"))) {
                // STRING: LEGORR 0x004aa2b4
                if ((str = block_get_str_copy(config, STATS_ID("CollBox"))) != NULL) {
                    sound_variant_split(str, stringParts, ",");
                    collBox.x = (float)atof(stringParts[0]);
                    collBox.y = (float)atof(stringParts[1]);
                    collRadius = ((collBox.x > collBox.y) ? collBox.x : collBox.y) * 0.5f;
                    flags1 |= 0x100;
                }
            } else {
                flags1 |= 0x80;
            }

            for (loop = 0; loop < levels; loop++) {
                STATS_RECORD(loop).CollRadius = collRadius;
                STATS_RECORD(loop).AlertRadius = alertRadius;
                STATS_RECORD(loop).CollBox = collBox;
                STATS_RECORD(loop).PickSphere = pickSphere;
                STATS_RECORD(loop).TrackDist = trackDist;
                STATS_RECORD(loop).CollHeight = collHeight;
                STATS_RECORD(loop).HealthDecayRate = healthDecayRate;
                STATS_RECORD(loop).EnergyDecayRate = energyDecayRate;
                STATS_RECORD(loop).BuildingBase = buildingBase;
                STATS_RECORD(loop).DrillSound = drillSound;
                STATS_RECORD(loop).DrillFadeSound = drillFadeSound;
                STATS_RECORD(loop).EngineSound = engineSound;
                STATS_RECORD(loop).RestPercent = restPercent;
                STATS_RECORD(loop).CarryMinHealth = carryMinHealth;
                STATS_RECORD(loop).StampRadius = stampRadius;
                STATS_RECORD(loop).AttackRadius = attackRadius;
                STATS_RECORD(loop).WaterEntrances = waterEntrances;
                STATS_RECORD(loop).RubbleCoef = rubbleCoef;
                STATS_RECORD(loop).PathCoef = pathCoef;
                STATS_RECORD(loop).OxygenCoef = oxygenCoef;
                STATS_RECORD(loop).Capacity = capacity;
                STATS_RECORD(loop).CostCrystal = costCrystal;
                STATS_RECORD(loop).CostOre = costOre;
                STATS_RECORD(loop).CostRefinedOre = costRefinedOre;
            }

            {
                char* values;

                // STRING: LEGORR 0x004aa2a4
                if ((values = block_get_str_copy(config, STATS_ID("SingleWidthDig"))) != NULL) {
                    sound_variant_split(values, stringParts, s_config_rgb_separator);
                    for (loop = 0; loop < levels; loop++) {
                        if (block_get_bool(stringParts[loop]) == 1) STATS_RECORD(loop).flags1 |= 0x1;
                    }
                    free(values);
                }
            }
            {
                char* values;

                // STRING: LEGORR 0x004aa298
                if ((values = block_get_str_copy(config, STATS_ID("RepairValue"))) != NULL) {
                    sound_variant_split(values, stringParts, s_config_rgb_separator);
                    for (loop = 0; loop < levels; loop++) {
                        STATS_RECORD(loop).RepairValue = (float)atof(stringParts[loop]);
                    }
                    free(values);
                }
            }
            {
                char* values;

                // STRING: LEGORR 0x004aa288
                if ((values = block_get_str_copy(config, STATS_ID("SurveyRadius"))) != NULL) {
                    sound_variant_split(values, stringParts, s_config_rgb_separator);
                    for (loop = 0; loop < levels; loop++) {
                        STATS_RECORD(loop).SurveyRadius = atoi(stringParts[loop]);
                    }
                    free(values);
                }
            }
            {
                char* values;

                // STRING: LEGORR 0x004aa27c
                if ((values = block_get_str_copy(config, STATS_ID("MaxCarry"))) != NULL) {
                    sound_variant_split(values, stringParts, s_config_rgb_separator);
                    for (loop = 0; loop < levels; loop++) {
                        STATS_RECORD(loop).MaxCarry = atoi(stringParts[loop]);
                    }
                    free(values);
                }
            }
            {
                char* values;

                // STRING: LEGORR 0x004aa270
                if ((values = block_get_str_copy(config, STATS_ID("CarryStart"))) != NULL) {
                    sound_variant_split(values, stringParts, s_config_rgb_separator);
                    for (loop = 0; loop < levels; loop++) {
                        STATS_RECORD(loop).CarryStart = atoi(stringParts[loop]);
                    }
                    free(values);
                }
            }
            {
                char* values;

                // STRING: LEGORR 0x004aa260
                if ((values = block_get_str_copy(config, STATS_ID("CrystalDrain"))) != NULL) {
                    sound_variant_split(values, stringParts, s_config_rgb_separator);
                    for (loop = 0; loop < levels; loop++) {
                        STATS_RECORD(loop).CrystalDrain = atoi(stringParts[loop]);
                    }
                    free(values);
                }
            }
            {
                char* values;

                // STRING: LEGORR 0x004aa24c
                if ((values = block_get_str_copy(config, STATS_ID("NumOfToolsCanCarry"))) != NULL) {
                    sound_variant_split(values, stringParts, s_config_rgb_separator);
                    for (loop = 0; loop < levels; loop++) {
                        STATS_RECORD(loop).NumOfToolsCanCarry = atoi(stringParts[loop]);
                    }
                    free(values);
                }
            }
            {
                char* values;

                // STRING: LEGORR 0x004a4418
                if ((values = block_get_str_copy(config, STATS_ID("UpgradeTime"))) != NULL) {
                    sound_variant_split(values, stringParts, s_config_rgb_separator);
                    for (loop = 0; loop < levels; loop++) {
                        STATS_RECORD(loop).UpgradeTime = (float)atof(stringParts[loop]) * 25.0f;
                    }
                    free(values);
                }
            }
            {
                char* values;

                // STRING: LEGORR 0x004aa23c
                if ((values = block_get_str_copy(config, STATS_ID("FunctionCoef"))) != NULL) {
                    sound_variant_split(values, stringParts, s_config_rgb_separator);
                    for (loop = 0; loop < levels; loop++) {
                        STATS_RECORD(loop).FunctionCoef = (float)atof(stringParts[loop]);
                    }
                    free(values);
                }
            }
            {
                char* values;

                // STRING: LEGORR 0x004aa22c
                if ((values = block_get_str_copy(config, STATS_ID("UpgradeCostOre"))) != NULL) {
                    sound_variant_split(values, stringParts, s_config_rgb_separator);
                    for (loop = 0; loop < 4; loop++) {
                        STATS_RECORD(loop).UpgradeCostOre = atoi(stringParts[3 - loop]);
                    }
                    free(values);
                }
            }
            {
                char* values;

                // STRING: LEGORR 0x004aa218
                if ((values = block_get_str_copy(config, STATS_ID("UpgradeCostStuds"))) != NULL) {
                    sound_variant_split(values, stringParts, s_config_rgb_separator);
                    for (loop = 0; loop < 4; loop++) {
                        STATS_RECORD(loop).UpgradeCostStuds = atoi(stringParts[3 - loop]);
                    }
                    free(values);
                }
            }

            // STRING: LEGORR 0x004aa20c
            if (STATS_BOOL("ProcessOre") == 1) flags1 |= 0x2;
            // STRING: LEGORR 0x004aa1fc
            if (STATS_BOOL("ProcessCrystal") == 1) flags1 |= 0x4;
            // STRING: LEGORR 0x004aa1ec
            if (STATS_BOOL("StoreObjects") == 1) flags1 |= 0x8;
            // STRING: LEGORR 0x004aa1dc
            if (STATS_BOOL("ManTeleporter") == 1) flags1 |= 0x80000000;
            // STRING: LEGORR 0x004aa1cc
            if (STATS_BOOL("SmallTeleporter") == 1) flags1 |= 0x10;
            // STRING: LEGORR 0x004aa1bc
            if (STATS_BOOL("BigTeleporter") == 1) flags1 |= 0x20;
            // STRING: LEGORR 0x004aa1ac
            if (STATS_BOOL("WaterTeleporter") == 1) flags1 |= 0x40;
            // STRING: LEGORR 0x004aa1a0
            if (STATS_BOOL("SnaxULike") == 1) flags1 |= 0x4000;
            // STRING: LEGORR 0x004aa194
            if (STATS_BOOL("ToolStore") == 1) flags1 |= 0x100000;
            // STRING: LEGORR 0x004aa188
            if (STATS_BOOL("CauseSlip") == 1) flags1 |= 0x200;
            // STRING: LEGORR 0x004aa17c
            if (STATS_BOOL("RandomMove") == 1) flags1 |= 0x400;
            // STRING: LEGORR 0x004aa170
            if (STATS_BOOL("CanScare") == 1) flags1 |= 0x800;
            // STRING: LEGORR 0x004aa15c
            if (STATS_BOOL("CanScareScorpion") == 1) flags1 |= 0x40000;
            // STRING: LEGORR 0x004aa14c
            if (STATS_BOOL("RandomEnterWall") == 1) flags1 |= 0x1000;
            // STRING: LEGORR 0x004aa13c
            if (STATS_BOOL("ScaredByPlayer") == 1) flags1 |= 0x2000;
            // STRING: LEGORR 0x004aa12c
            if (STATS_BOOL("GrabMinifigure") == 1) flags1 |= 0x8000;
            // STRING: LEGORR 0x004aa120
            if (STATS_BOOL("CanBeDriven") == 1) flags1 |= 0x20000;
            // STRING: LEGORR 0x004aa114
            if (STATS_BOOL("CanSteal") == 1) flags1 |= 0x80000;
            // STRING: LEGORR 0x004aa104
            if (STATS_BOOL("CanClearRubble") == 1) flags1 |= 0x10000;
            // STRING: LEGORR 0x004aa0f4
            if (STATS_BOOL("RouteAvoidance") == 1) flags1 |= 0x20000000;
            // STRING: LEGORR 0x004aa0e8
            if (STATS_BOOL("BumpDamage") == 1) flags1 |= 0x40000000;
            // STRING: LEGORR 0x004aa0e0
            if (STATS_BOOL("Flocks") == 1) flags1 |= 0x200000;
            // STRING: LEGORR 0x004aa0d0
            if (STATS_BOOL("Flocks_Debug") == 1) flags1 |= 0x400000;
            // STRING: LEGORR 0x004aa0c0
            if (STATS_BOOL("Flocks_Smooth") == 1) flags1 |= 0x800000;
            // STRING: LEGORR 0x004aa0b4
            if (STATS_BOOL("CrossWater") == 1) flags1 |= 0x1000000;
            // STRING: LEGORR 0x004aa0a8
            if (STATS_BOOL("CrossLand") == 1) flags1 |= 0x2000000;
            // STRING: LEGORR 0x004aa098
            if (STATS_BOOL("Flocks_OnGround") == 1) flags1 |= 0x4000000;
            // STRING: LEGORR 0x004aa084
            if (STATS_BOOL("Flocks_QuickDestroy") == 1) flags1 |= 0x8000000;
            // STRING: LEGORR 0x004aa06c
            if (STATS_BOOL("Flocks_AnimateByPitch") == 1) flags1 |= 0x10000000;

            // STRING: LEGORR 0x004aa058
            if (STATS_BOOL("ScaredByBigBangs") == 1) flags2 |= 0x1;
            // STRING: LEGORR 0x004aa048
            if (STATS_BOOL("UpgradeBuilding") == 1) flags2 |= 0x2;
            // STRING: LEGORR 0x004aa03c
            if (STATS_BOOL("CanBeShotAt") == 1) flags2 |= 0x40000;
            // STRING: LEGORR 0x004aa028
            if (STATS_BOOL("UseBigTeleporter") == 1) flags2 |= 0x800;
            // STRING: LEGORR 0x004aa014
            if (STATS_BOOL("UseSmallTeleporter") == 1) flags2 |= 0x1000;
            // STRING: LEGORR 0x004aa000
            if (STATS_BOOL("UseWaterTeleporter") == 1) flags2 |= 0x2000;
            // STRING: LEGORR 0x004a9fe8
            if (STATS_BOOL("UseLegoManTeleporter") == 1) flags2 |= 0x400000;

            // STRING: LEGORR 0x004a9fdc
            if ((str = block_get_str_copy(config, STATS_ID("TrainPilot"))) != NULL) {
                sound_variant_split(str, stringParts, s_config_rgb_separator);
                for (loop = 0; loop < levels; loop++) {
                    if (block_get_bool(stringParts[loop]) == 1) STATS_RECORD(loop).flags2 |= 0x4;
                }
                free(str);
            }
            // STRING: LEGORR 0x004a9fd0
            if ((str = block_get_str_copy(config, STATS_ID("TrainSailor"))) != NULL) {
                sound_variant_split(str, stringParts, s_config_rgb_separator);
                for (loop = 0; loop < levels; loop++) {
                    if (block_get_bool(stringParts[loop]) == 1) STATS_RECORD(loop).flags2 |= 0x8;
                }
                free(str);
            }
            // STRING: LEGORR 0x004a9fc4
            if ((str = block_get_str_copy(config, STATS_ID("TrainDriver"))) != NULL) {
                sound_variant_split(str, stringParts, s_config_rgb_separator);
                for (loop = 0; loop < levels; loop++) {
                    if (block_get_bool(stringParts[loop]) == 1) STATS_RECORD(loop).flags2 |= 0x10;
                }
                free(str);
            }
            // STRING: LEGORR 0x004a9fb4
            if ((str = block_get_str_copy(config, STATS_ID("TrainDynamite"))) != NULL) {
                sound_variant_split(str, stringParts, s_config_rgb_separator);
                for (loop = 0; loop < levels; loop++) {
                    if (block_get_bool(stringParts[loop]) == 1) STATS_RECORD(loop).flags2 |= 0x20;
                }
                free(str);
            }
            // STRING: LEGORR 0x004a9fa8
            if ((str = block_get_str_copy(config, STATS_ID("TrainRepair"))) != NULL) {
                sound_variant_split(str, stringParts, s_config_rgb_separator);
                for (loop = 0; loop < levels; loop++) {
                    if (block_get_bool(stringParts[loop]) == 1) STATS_RECORD(loop).flags2 |= 0x40;
                }
                free(str);
            }
            // STRING: LEGORR 0x004a9f98
            if ((str = block_get_str_copy(config, STATS_ID("TrainScanner"))) != NULL) {
                sound_variant_split(str, stringParts, s_config_rgb_separator);
                for (loop = 0; loop < levels; loop++) {
                    if (block_get_bool(stringParts[loop]) == 1) STATS_RECORD(loop).flags2 |= 0x80;
                }
                free(str);
            }

            // STRING: LEGORR 0x004a9f90
            if (STATS_BOOL("Tracker") == 1) flags2 |= 0x100;
            // STRING: LEGORR 0x004a9f80
            if (STATS_BOOL("PowerBuilding") == 1) flags2 |= 0x600;
            // STRING: LEGORR 0x004a9f74
            if (STATS_BOOL("SelfPowered") == 1) flags2 |= 0x400;
            // STRING: LEGORR 0x004a9f68
            if (STATS_BOOL("AttackPaths") == 1) flags2 |= 0x4000;
            // STRING: LEGORR 0x004a9f54
            if (STATS_BOOL("SplitOnZeroHealth") == 1) flags2 |= 0x8000;
            // STRING: LEGORR 0x004a9f44
            if (STATS_BOOL("CanBeHitByFence") == 1) flags2 |= 0x10000;
            // STRING: LEGORR 0x004a9f34
            if (STATS_BOOL("CanDoubleSelect") == 1) flags2 |= 0x20000;
            // STRING: LEGORR 0x004a9f28
            if (STATS_BOOL("DrainPower") == 1) flags2 |= 0x80000;
            // STRING: LEGORR 0x004a9f1c
            if (STATS_BOOL("UseHoles") == 1) flags2 |= 0x100000;
            // STRING: LEGORR 0x004a9f10
            if (STATS_BOOL("CrossLava") == 1) flags2 |= 0x200000;
            // STRING: LEGORR 0x004a9ef8
            if (STATS_BOOL("DamageCausesCallToArms") == 1) flags2 |= 0x800000;
            // STRING: LEGORR 0x004a9eec
            if (STATS_BOOL("CanFreeze") == 1) flags2 |= 0x1000000;
            // STRING: LEGORR 0x004a9ee0
            if (STATS_BOOL("CanLaser") == 1) flags2 |= 0x2000000;
            // STRING: LEGORR 0x004a9ed8
            if (STATS_BOOL("CanPush") == 1) flags2 |= 0x4000000;
            // STRING: LEGORR 0x004a9ec8
            if (STATS_BOOL("DontShowDamage") == 1) flags2 |= 0x8000000;
            // STRING: LEGORR 0x004a9eb4
            if (STATS_BOOL("RemoveReinforcement") == 1) flags2 |= 0x10000000;
            // STRING: LEGORR 0x004a9ea4
            if (STATS_BOOL("DontShowOnRadar") == 1) flags2 |= 0x20000000;
            // STRING: LEGORR 0x004a9e94
            if (STATS_BOOL("InvisibleDriver") == 1) flags2 |= 0x40000000;
            // STRING: LEGORR 0x004a9e84
            if (STATS_BOOL("Unselectable") == 1) flags2 |= 0x80000000;

            // STRING: LEGORR 0x004a9e74
            if (STATS_BOOL("CarryVehicles") == 1) flags3 |= 0x1;
            // STRING: LEGORR 0x004a9e60
            if (STATS_BOOL("VehicleCanBeCarried") == 1) flags3 |= 0x2;
            // STRING: LEGORR 0x004a9e54
            if (STATS_BOOL("CanStrafe") == 1) flags3 |= 0x4;
            // STRING: LEGORR 0x004a9e44
            if (STATS_BOOL("ClassAsLarge") == 1) flags3 |= 0x8;
            // STRING: LEGORR 0x004a9e34
            if (STATS_BOOL("GetOutAtLand") == 1) flags3 |= 0x10;
            // STRING: LEGORR 0x004a9e28
            if (STATS_BOOL("GetInAtLand") == 1) flags3 |= 0x20;
            // STRING: LEGORR 0x004a9e14
            if (STATS_BOOL("TakeCarryingDrivers") == 1) flags3 |= 0x40;
            // STRING: LEGORR 0x004a9e04
            if (STATS_BOOL("ShowHealthBar") == 1) flags3 |= 0x80;
            // STRING: LEGORR 0x004a9df8
            if (STATS_BOOL("NeedsPilot") == 1) flags3 |= 0x100;
            // STRING: LEGORR 0x004a9de8
            if (STATS_BOOL("EnterToolStore") == 1) flags3 |= 0x200;

            // STRING: LEGORR 0x004a9ddc
            if ((flocksTurn = STATS_REAL("Flocks_Turn")) == 0.0f) flocksTurn = 0.06f;
            // STRING: LEGORR 0x004a9dcc
            if ((flocksSpeed = STATS_REAL("Flocks_Speed")) == 0.0f) flocksSpeed = 2.0f;
            // STRING: LEGORR 0x004a9db8
            if ((flocksTightness = STATS_REAL("Flocks_Tightness")) == 0.0f) flocksTightness = 2.0f;
            // STRING: LEGORR 0x004a9da4
            if ((flocksGoalUpdate = STATS_REAL("Flocks_GoalUpdate")) == 0.0f) flocksGoalUpdate = 2.0f;
            // STRING: LEGORR 0x004a9d94
            if ((flocksHeight = STATS_REAL("Flocks_Height")) == 0.0f) flocksHeight = 30.0f;
            // STRING: LEGORR 0x004a9d80
            if ((flocksRandomness = STATS_REAL("Flocks_Randomness")) == 0.0f) flocksRandomness = 2.0f;
            // STRING: LEGORR 0x004a9d74
            if ((flocksSize = STATS_INT("Flocks_Size")) == 0.0f) flocksSize = 5;
            // STRING: LEGORR 0x004a9d60
            if ((flocksAttackTime = STATS_REAL("Flocks_AttackTime")) == 0.0f) flocksAttackTime = 300.0f;
            // STRING: LEGORR 0x004a9d50
            if ((awarenessRange = STATS_REAL("AwarenessRange")) == 0.0f) awarenessRange = 0.0f;
            // STRING: LEGORR 0x004a9d40
            if ((painThreshold = STATS_REAL("PainThreshold")) == 0.0f) painThreshold = 0.0f;
            // STRING: LEGORR 0x004a9d34
            if ((freezerTime = STATS_REAL("FreezerTime")) == 0.0f) freezerTime = 0.0f;
            // STRING: LEGORR 0x004a9d24
            if ((freezerDamage = STATS_REAL("FreezerDamage")) == 0.0f) freezerDamage = 0.0f;
            // STRING: LEGORR 0x004a9d18
            if ((pusherDist = STATS_REAL("PusherDist")) == 0.0f) pusherDist = 0.0f;
            // STRING: LEGORR 0x004a9d08
            if ((pusherDamage = STATS_REAL("PusherDamage")) == 0.0f) pusherDamage = 0.0f;
            // STRING: LEGORR 0x004a9cfc
            if ((laserDamage = STATS_REAL("LaserDamage")) == 0.0f) laserDamage = 0.0f;
            // STRING: LEGORR 0x004a9cf0
            if ((wakeRadius = STATS_REAL("WakeRadius")) == 0.0f) wakeRadius = 20.0f;
            // STRING: LEGORR 0x004a9ce0
            if ((randomMoveTime = STATS_REAL("RandomMoveTime")) == 0.0f) randomMoveTime = 200.0f;

            for (loop = 0; loop < levels; loop++) {
                STATS_RECORD(loop).flags1 |= flags1;
                STATS_RECORD(loop).flags2 |= flags2;
                STATS_RECORD(loop).flags3 |= flags3;
                STATS_RECORD(loop).RandomMoveTime = randomMoveTime;
                STATS_RECORD(loop).FlocksTurn = flocksTurn;
                STATS_RECORD(loop).FlocksSpeed = flocksSpeed;
                STATS_RECORD(loop).FlocksTightness = flocksTightness;
                STATS_RECORD(loop).FlocksGoalUpdate = flocksGoalUpdate;
                STATS_RECORD(loop).FlocksHeight = flocksHeight;
                STATS_RECORD(loop).FlocksRandomness = flocksRandomness;
                STATS_RECORD(loop).FlocksSize = flocksSize;
                STATS_RECORD(loop).FlocksAttackTime = flocksAttackTime;
                STATS_RECORD(loop).AwarenessRange = awarenessRange;
                STATS_RECORD(loop).PainThreshold = painThreshold;
                STATS_RECORD(loop).PusherDamage = pusherDamage;
                STATS_RECORD(loop).PusherDist = pusherDist;
                STATS_RECORD(loop).LaserDamage = laserDamage;
                STATS_RECORD(loop).FreezerDamage = freezerDamage;
                STATS_RECORD(loop).FreezerTime = freezerTime;
                STATS_RECORD(loop).WakeRadius = wakeRadius;
            }
        }
    }

    stats_add_tool_task_type(0, 8);
    stats_add_tool_task_type(1, 0xc);
    stats_add_tool_task_type(2, 0xb);
    stats_add_tool_task_type(3, 10);

    return 1;
}

// FUNCTION: LEGORR 0x00469a90
void stats_add_tool_task_type(int toolType, unsigned int taskType)
{
    statsGlobs.toolTaskTypes[toolType].taskTypes[statsGlobs.toolTaskTypes[toolType].count] = taskType;
    statsGlobs.toolTaskTypes[toolType].count++;
}

// FUNCTION: LEGORR 0x00469ac0
int stats_get_cost_ore(int objType, int objID, unsigned int objLevel)
{
    return statsGlobs.objectStats[objType][objID][objLevel].CostOre;
}

// FUNCTION: LEGORR 0x00469af0
int stats_get_cost_crystal(int objType, int objID, unsigned int objLevel)
{
    return statsGlobs.objectStats[objType][objID][objLevel].CostCrystal;
}

// FUNCTION: LEGORR 0x00469b20
int stats_get_cost_refined_ore(int objType, int objID, unsigned int objLevel)
{
    return statsGlobs.objectStats[objType][objID][objLevel].CostRefinedOre;
}

// FUNCTION: LEGORR 0x00469b50
int stats_object_get_crystal_drain(const StatsObject* obj)
{
    return statsGlobs.objectStats[obj->type][obj->id][obj->objLevel].CrystalDrain;
}

// FUNCTION: LEGORR 0x00469b80
int stats_object_get_capacity(const StatsObject* obj)
{
    return statsGlobs.objectStats[obj->type][obj->id][obj->objLevel].Capacity;
}

// FUNCTION: LEGORR 0x00469bb0
int stats_object_get_max_carry(const StatsObject* obj)
{
    return statsGlobs.objectStats[obj->type][obj->id][obj->objLevel].MaxCarry;
}

// FUNCTION: LEGORR 0x00469be0
int stats_object_get_carry_start(const StatsObject* obj)
{
    return statsGlobs.objectStats[obj->type][obj->id][obj->objLevel].CarryStart;
}

// FUNCTION: LEGORR 0x00469c10
int stats_object_set_object_level(StatsObject* obj, unsigned int newLevel)
{
    if (newLevel < statsGlobs.objectLevels[obj->type][obj->id]) {
        if (newLevel != obj->objLevel) {
            s_grid_count_first[((obj->type * STATS_OBJECT_IDS) + obj->id) * 16 + obj->objLevel]++;
            s_grid_count_second[((obj->type * STATS_OBJECT_IDS) + obj->id) * 16 + newLevel]++;
        }
        obj->objLevel = newLevel;
        obj->stats = &statsGlobs.objectStats[obj->type][obj->id][newLevel];
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00469cb0
float stats_object_get_route_speed(const StatsObject* obj)
{
    return obj->stats->RouteSpeed;
}

// FUNCTION: LEGORR 0x00469cc0
float stats_object_get_drill_time_type(const StatsObject* obj, int surface)
{
    return obj->stats->DrillTimes[surface];
}

// FUNCTION: LEGORR 0x00469ce0
float stats_object_get_rubble_coef(const StatsObject* obj)
{
    return obj->stats->RubbleCoef;
}

// FUNCTION: LEGORR 0x00469d00
float stats_object_get_wake_radius(const StatsObject* obj)
{
    return obj->stats->WakeRadius;
}

// FUNCTION: LEGORR 0x00469d20
float stats_object_get_path_coef(const StatsObject* obj)
{
    return obj->stats->PathCoef;
}

// FUNCTION: LEGORR 0x00469d40
float stats_object_get_coll_radius(const StatsObject* obj)
{
    return obj->stats->CollRadius;
}

// FUNCTION: LEGORR 0x00469d50
float stats_object_get_coll_height(const StatsObject* obj)
{
    return obj->stats->CollHeight;
}

// FUNCTION: LEGORR 0x00469d60
float stats_object_get_pick_sphere(const StatsObject* obj)
{
    return obj->stats->PickSphere;
}

// FUNCTION: LEGORR 0x00469d70
float stats_object_get_pain_threshold(const StatsObject* obj)
{
    return obj->stats->PainThreshold;
}

// FUNCTION: LEGORR 0x00469d90
float stats_object_get_alert_radius(const StatsObject* obj)
{
    return obj->stats->AlertRadius;
}

// FUNCTION: LEGORR 0x00469da0
StatsPoint2F* stats_object_get_coll_box(const StatsObject* obj)
{
    return &obj->stats->CollBox;
}

// FUNCTION: LEGORR 0x00469db0
float stats_object_get_track_dist(const StatsObject* obj)
{
    return obj->stats->TrackDist;
}

// FUNCTION: LEGORR 0x00469dc0
float stats_object_get_health_decay_rate(const StatsObject* obj)
{
    return obj->stats->HealthDecayRate;
}

// FUNCTION: LEGORR 0x00469de0
float stats_object_get_energy_decay_rate(const StatsObject* obj)
{
    return obj->stats->EnergyDecayRate;
}

// FUNCTION: LEGORR 0x00469e00
float stats_get_oxygen_coef(int objType, int objID)
{
    return statsGlobs.objectStats[objType][objID]->OxygenCoef;
}

// FUNCTION: LEGORR 0x00469e20
float stats_object_get_oxygen_coef(const StatsObject* obj)
{
    return obj->stats->OxygenCoef;
}

// FUNCTION: LEGORR 0x00469e40
int stats_object_get_survey_radius(const StatsObject* obj)
{
    return obj->stats->SurveyRadius;
}

// FUNCTION: LEGORR 0x00469e60
unsigned int stats_object_get_stats_flags1(const StatsObject* obj)
{
    return obj->stats->flags1;
}

// FUNCTION: LEGORR 0x00469e80
unsigned int stats_object_get_stats_flags2(const StatsObject* obj)
{
    return obj->stats->flags2;
}

// FUNCTION: LEGORR 0x00469ea0
unsigned int stats_object_get_stats_flags3(const StatsObject* obj)
{
    return obj->stats->flags3;
}

// FUNCTION: LEGORR 0x00469ec0
unsigned int stats_get_stats_flags1(int objType, int objID)
{
    return statsGlobs.objectStats[objType][objID]->flags1;
}

// FUNCTION: LEGORR 0x00469ee0
unsigned int stats_get_stats_flags2(int objType, int objID)
{
    return statsGlobs.objectStats[objType][objID]->flags2;
}

// FUNCTION: LEGORR 0x00469f00
unsigned int stats_get_stats_flags3(int objType, int objID)
{
    return statsGlobs.objectStats[objType][objID]->flags3;
}

// FUNCTION: LEGORR 0x00469f20
float stats_object_get_repair_value(const StatsObject* obj)
{
    return obj->stats->RepairValue;
}

// FUNCTION: LEGORR 0x00469f40
unsigned int stats_get_levels(int objType, int objID)
{
    return statsGlobs.objectLevels[objType][objID];
}

// FUNCTION: LEGORR 0x00469f60
int stats_get_water_entrances(int objType, int objID, unsigned int objLevel)
{
    return statsGlobs.objectStats[objType][objID][objLevel].WaterEntrances;
}

// FUNCTION: LEGORR 0x00469f90
int stats_object_get_drill_sound_type(const StatsObject* obj, int fade)
{
    if (fade) return obj->stats->DrillFadeSound;
    return obj->stats->DrillSound;
}

// FUNCTION: LEGORR 0x00469fc0
int stats_object_get_engine_sound(const StatsObject* obj)
{
    return obj->stats->EngineSound;
}

// FUNCTION: LEGORR 0x00469fe0
float stats_object_get_rest_percent(const StatsObject* obj)
{
    return obj->stats->RestPercent;
}

// FUNCTION: LEGORR 0x0046a000
float stats_object_get_carry_min_health(const StatsObject* obj)
{
    return obj->stats->CarryMinHealth;
}

// FUNCTION: LEGORR 0x0046a020
float stats_object_get_attack_radius(const StatsObject* obj)
{
    return obj->stats->AttackRadius;
}

// FUNCTION: LEGORR 0x0046a040
float stats_object_get_stamp_radius(const StatsObject* obj)
{
    return obj->stats->StampRadius;
}

// FUNCTION: LEGORR 0x0046a060
int stats_object_get_num_of_tools_can_carry(const StatsObject* obj)
{
    return obj->stats->NumOfToolsCanCarry;
}

// FUNCTION: LEGORR 0x0046a080
float stats_object_get_upgrade_time(const StatsObject* obj)
{
    return obj->stats->UpgradeTime;
}

// FUNCTION: LEGORR 0x0046a0a0
float stats_object_get_function_coef(const StatsObject* obj)
{
    return obj->stats->FunctionCoef;
}

// FUNCTION: LEGORR 0x0046a0c0
int stats_get_upgrade_cost_ore(int objType, int objID, unsigned int objLevel)
{
    return statsGlobs.objectStats[objType][objID][objLevel].UpgradeCostOre;
}

// FUNCTION: LEGORR 0x0046a0f0
int stats_get_upgrade_cost_studs(int objType, int objID, unsigned int objLevel)
{
    return statsGlobs.objectStats[objType][objID][objLevel].UpgradeCostStuds;
}

// FUNCTION: LEGORR 0x0046a120
int stats_find_tool_from_task_type(unsigned int taskType, int* outToolType)
{
    unsigned int toolType;
    unsigned int i;
    Stats_ToolTaskTypes* tool;

    for (toolType = 0; toolType < STATS_TOOL_TYPES; toolType++) {
        tool = &statsGlobs.toolTaskTypes[toolType];
        for (i = 0; i < tool->count; i++) {
            if (taskType == tool->taskTypes[i]) {
                *outToolType = toolType;
                return 1;
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0046a170
float stats_object_get_flocks_height(const StatsObject* obj)
{
    return obj->stats->FlocksHeight;
}

// FUNCTION: LEGORR 0x0046a190
float stats_object_get_flocks_randomness(const StatsObject* obj)
{
    return obj->stats->FlocksRandomness;
}

// FUNCTION: LEGORR 0x0046a1b0
float stats_object_get_flocks_turn(const StatsObject* obj)
{
    return obj->stats->FlocksTurn;
}

// FUNCTION: LEGORR 0x0046a1d0
float stats_object_get_flocks_tightness(const StatsObject* obj)
{
    return obj->stats->FlocksTightness;
}

// FUNCTION: LEGORR 0x0046a1f0
float stats_object_get_flocks_speed(const StatsObject* obj)
{
    return obj->stats->FlocksSpeed;
}

// FUNCTION: LEGORR 0x0046a210
unsigned int stats_object_get_flocks_size(const StatsObject* obj)
{
    return obj->stats->FlocksSize;
}

// FUNCTION: LEGORR 0x0046a230
float stats_object_get_flocks_goal_update(const StatsObject* obj)
{
    return obj->stats->FlocksGoalUpdate;
}

// FUNCTION: LEGORR 0x0046a250
float stats_object_get_flocks_attack_time(const StatsObject* obj)
{
    return obj->stats->FlocksAttackTime;
}

// FUNCTION: LEGORR 0x0046a270
float stats_object_get_awareness_range(const StatsObject* obj)
{
    return obj->stats->AwarenessRange;
}

// FUNCTION: LEGORR 0x0046a290
float stats_object_get_pusher_dist(const StatsObject* obj)
{
    return obj->stats->PusherDist;
}

// FUNCTION: LEGORR 0x0046a2b0
float stats_object_get_pusher_damage(const StatsObject* obj)
{
    return obj->stats->PusherDamage;
}

// FUNCTION: LEGORR 0x0046a2d0
float stats_object_get_laser_damage(const StatsObject* obj)
{
    return obj->stats->LaserDamage;
}

// FUNCTION: LEGORR 0x0046a2f0
float stats_object_get_freezer_damage(const StatsObject* obj)
{
    return obj->stats->FreezerDamage;
}

// FUNCTION: LEGORR 0x0046a310
float stats_object_get_freezer_time(const StatsObject* obj)
{
    return obj->stats->FreezerTime;
}

// FUNCTION: LEGORR 0x0046a330
void stats_object_debug_toggle_self_powered(StatsObject* obj)
{
    if (obj->stats->flags2 & 0x400) {
        obj->stats->flags2 &= ~0x400;
        entity_kind_four_post_create_resolve(obj);
    } else {
        obj->stats->flags2 |= 0x400;
        entity_kind_four_post_create_resolve(obj);
    }
}

// PORT-ONLY: the port tail's and the port loader's view of statsGlobs (finding 581)
Stats_Globs* entity_stats_port_globs(void)
{
    return &statsGlobs;
}
