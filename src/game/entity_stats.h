// SPDX-License-Identifier: LGPL-3.0-or-later
/* Stats.c (original translation unit GAME-043, finding 505, finding 581):
 * the per-object stats table that Stats_Initialise loads from the game
 * config's Stats block, and its getters.
 *
 * C-compatible header (ADR 0005). entity_stats.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * includes it with the C++ declarations of the port's functions.
 *
 * In the matching build the unit's functions take the port's link names
 * (entity_stats_link_names.h, included only by entity_stats.c), so every
 * caller reaches them; natively the port tail entity_stats_port.cpp serves
 * the port's callers under those names (finding 581). */
#ifndef LEGORRR_GAME_ENTITY_STATS_H
#define LEGORRR_GAME_ENTITY_STATS_H

#ifdef __cplusplus
extern "C" {
#endif

struct BlockRec;

#define STATS_OBJECT_TYPES 20   /* LegoObject_Type_Count */
#define STATS_OBJECT_IDS 15     /* the objectLevels row width */
#define STATS_TOOL_TYPES 11     /* LegoObject_ToolType_Count */
#define STATS_TOOL_TASKS 3      /* task types per tool */
#define STATS_SURFACE_TYPES 18  /* Lego_SurfaceType_Count */

typedef struct StatsPoint2F {
    float x;
    float y;
} StatsPoint2F;

/* One object level's stats (0x150 bytes). Stats_Initialise allocates
 * `Levels` of them per object type and ID. */
typedef struct StatsData {
    unsigned int f_000[8];                  /* +0x000: not loaded */
    float RouteSpeed;                       /* +0x020 */
    float DrillTimes[STATS_SURFACE_TYPES];  /* +0x024: frames, by surface */
    float CollRadius;                       /* +0x06c */
    float AlertRadius;                      /* +0x070 */
    float CollHeight;                       /* +0x074 */
    float PickSphere;                       /* +0x078 */
    float TrackDist;                        /* +0x07c */
    float HealthDecayRate;                  /* +0x080 */
    float EnergyDecayRate;                  /* +0x084 */
    float RepairValue;                      /* +0x088 */
    float RestPercent;                      /* +0x08c */
    float CarryMinHealth;                   /* +0x090 */
    float StampRadius;                      /* +0x094 */
    float AttackRadius;                     /* +0x098 */
    float WakeRadius;                       /* +0x09c */
    StatsPoint2F CollBox;                   /* +0x0a0 */
    int BuildingBase;                       /* +0x0a8 */
    int SurveyRadius;                       /* +0x0ac */
    int CostOre;                            /* +0x0b0 */
    int CostCrystal;                        /* +0x0b4 */
    int CostRefinedOre;                     /* +0x0b8 */
    int CrystalDrain;                       /* +0x0bc */
    int DrillSound;                         /* +0x0c0 */
    int DrillFadeSound;                     /* +0x0c4 */
    int EngineSound;                        /* +0x0c8 */
    int NumOfToolsCanCarry;                 /* +0x0cc */
    int WaterEntrances;                     /* +0x0d0 */
    float RubbleCoef;                       /* +0x0d4 */
    float PathCoef;                         /* +0x0d8 */
    float OxygenCoef;                       /* +0x0dc */
    int Capacity;                           /* +0x0e0 */
    float AwarenessRange;                   /* +0x0e4 */
    float PainThreshold;                    /* +0x0e8 */
    int MaxCarry;                           /* +0x0ec */
    int CarryStart;                         /* +0x0f0 */
    float UpgradeTime;                      /* +0x0f4: frames */
    float FunctionCoef;                     /* +0x0f8 */
    int UpgradeCostOre;                     /* +0x0fc */
    int UpgradeCostStuds;                   /* +0x100 */
    unsigned int f_104[2];                  /* +0x104: not loaded */
    float FreezerTime;                      /* +0x10c */
    float FreezerDamage;                    /* +0x110 */
    float PusherDist;                       /* +0x114 */
    float PusherDamage;                     /* +0x118 */
    float LaserDamage;                      /* +0x11c */
    float FlocksTurn;                       /* +0x120 */
    float FlocksSpeed;                      /* +0x124 */
    float FlocksTightness;                  /* +0x128 */
    float FlocksGoalUpdate;                 /* +0x12c */
    float FlocksHeight;                     /* +0x130 */
    float FlocksRandomness;                 /* +0x134 */
    float FlocksAttackTime;                 /* +0x138 */
    unsigned int FlocksSize;                /* +0x13c */
    float RandomMoveTime;                   /* +0x140 */
    unsigned int flags1;                    /* +0x144 */
    unsigned int flags2;                    /* +0x148 */
    unsigned int flags3;                    /* +0x14c */
} StatsData;

/* The three task types a tool serves, and their count (Stats_AddToolTaskType,
 * Stats_FindToolFromTaskType). */
typedef struct Stats_ToolTaskTypes {
    unsigned int taskTypes[STATS_TOOL_TASKS];
    unsigned int count;
} Stats_ToolTaskTypes;

/* Stats.c's .bss (00502bd8-00503188). */
typedef struct Stats_Globs {
    StatsData** objectStats[STATS_OBJECT_TYPES];                  /* 00502bd8 */
    unsigned int objectLevels[STATS_OBJECT_TYPES][STATS_OBJECT_IDS]; /* 00502c28 */
    Stats_ToolTaskTypes toolTaskTypes[STATS_TOOL_TYPES];           /* 005030d8 */
} Stats_Globs;

/* The original object (LegoObject) as far as Stats.c reads it, at its Win32
 * offsets. In the matching build the port's EntitySlot has these fields at
 * the same offsets (asserted in entity.hpp). */
typedef struct StatsObject {
    int type;                   /* +0x000 */
    int id;                     /* +0x004 */
    unsigned char f_008[0x328];
    unsigned int objLevel;      /* +0x330 */
    StatsData* stats;           /* +0x334 */
} StatsObject;

/* PORT-ONLY: the port tail's view of statsGlobs (finding 581). */
Stats_Globs* entity_stats_port_globs(void);

/* 004667e0 Stats_Initialise */
int stats_initialise(struct BlockRec* config, const char* gameName);
/* 00469a90 Stats_AddToolTaskType */
void stats_add_tool_task_type(int toolType, unsigned int taskType);
/* 00469ac0-00469b20: by type, ID and level */
int stats_get_cost_ore(int objType, int objID, unsigned int objLevel);
int stats_get_cost_crystal(int objType, int objID, unsigned int objLevel);
int stats_get_cost_refined_ore(int objType, int objID, unsigned int objLevel);
/* 00469b50-00469be0: by the object's type, ID and level */
int stats_object_get_crystal_drain(const StatsObject* obj);
int stats_object_get_capacity(const StatsObject* obj);
int stats_object_get_max_carry(const StatsObject* obj);
int stats_object_get_carry_start(const StatsObject* obj);
/* 00469c10 StatsObject_SetObjectLevel */
int stats_object_set_object_level(StatsObject* obj, unsigned int newLevel);
/* 00469cb0-0046a330: the object's current stats record */
float stats_object_get_route_speed(const StatsObject* obj);
float stats_object_get_drill_time_type(const StatsObject* obj, int surface);
float stats_object_get_rubble_coef(const StatsObject* obj);
float stats_object_get_wake_radius(const StatsObject* obj);
float stats_object_get_path_coef(const StatsObject* obj);
float stats_object_get_coll_radius(const StatsObject* obj);
float stats_object_get_coll_height(const StatsObject* obj);
float stats_object_get_pick_sphere(const StatsObject* obj);
float stats_object_get_pain_threshold(const StatsObject* obj);
float stats_object_get_alert_radius(const StatsObject* obj);
StatsPoint2F* stats_object_get_coll_box(const StatsObject* obj);
float stats_object_get_track_dist(const StatsObject* obj);
float stats_object_get_health_decay_rate(const StatsObject* obj);
float stats_object_get_energy_decay_rate(const StatsObject* obj);
float stats_get_oxygen_coef(int objType, int objID);
float stats_object_get_oxygen_coef(const StatsObject* obj);
int stats_object_get_survey_radius(const StatsObject* obj);
unsigned int stats_object_get_stats_flags1(const StatsObject* obj);
unsigned int stats_object_get_stats_flags2(const StatsObject* obj);
unsigned int stats_object_get_stats_flags3(const StatsObject* obj);
unsigned int stats_get_stats_flags1(int objType, int objID);
unsigned int stats_get_stats_flags2(int objType, int objID);
unsigned int stats_get_stats_flags3(int objType, int objID);
float stats_object_get_repair_value(const StatsObject* obj);
unsigned int stats_get_levels(int objType, int objID);
int stats_get_water_entrances(int objType, int objID, unsigned int objLevel);
int stats_object_get_drill_sound_type(const StatsObject* obj, int fade);
int stats_object_get_engine_sound(const StatsObject* obj);
float stats_object_get_rest_percent(const StatsObject* obj);
float stats_object_get_carry_min_health(const StatsObject* obj);
float stats_object_get_attack_radius(const StatsObject* obj);
float stats_object_get_stamp_radius(const StatsObject* obj);
int stats_object_get_num_of_tools_can_carry(const StatsObject* obj);
float stats_object_get_upgrade_time(const StatsObject* obj);
float stats_object_get_function_coef(const StatsObject* obj);
int stats_get_upgrade_cost_ore(int objType, int objID, unsigned int objLevel);
int stats_get_upgrade_cost_studs(int objType, int objID, unsigned int objLevel);
int stats_find_tool_from_task_type(unsigned int taskType, int* outToolType);
float stats_object_get_flocks_height(const StatsObject* obj);
float stats_object_get_flocks_randomness(const StatsObject* obj);
float stats_object_get_flocks_turn(const StatsObject* obj);
float stats_object_get_flocks_tightness(const StatsObject* obj);
float stats_object_get_flocks_speed(const StatsObject* obj);
unsigned int stats_object_get_flocks_size(const StatsObject* obj);
float stats_object_get_flocks_goal_update(const StatsObject* obj);
float stats_object_get_flocks_attack_time(const StatsObject* obj);
float stats_object_get_awareness_range(const StatsObject* obj);
float stats_object_get_pusher_dist(const StatsObject* obj);
float stats_object_get_pusher_damage(const StatsObject* obj);
float stats_object_get_laser_damage(const StatsObject* obj);
float stats_object_get_freezer_damage(const StatsObject* obj);
float stats_object_get_freezer_time(const StatsObject* obj);
void stats_object_debug_toggle_self_powered(StatsObject* obj);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_ENTITY_STATS_H */
