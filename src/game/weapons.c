// SPDX-License-Identifier: LGPL-3.0-or-later
/* Weapons.c, original translation unit GAME-050 (finding 505), rebuilt in
 * original shape (ADR 0005; finding 582): text 0046eb80-004721e0, .rdata
 * 0049e820-0049e878, literals 004aaa3c-004aaaac, .bss 00503870-00505400.
 *
 * The unit loads the WeaponTypes block (per-type damage grids, slow death,
 * recharge, discharge, ammo, range and wall destroy times), runs the
 * projectiles and laser beams that weapons fire, and drives the vehicle and
 * building weapon models (aim, recharge, fire).
 *
 * The port's implementation stays in the port tail weapons_port.cpp,
 * unannotated: it works through hook tables and its own records. Five
 * functions keep the port's names and serve native callers as well, because
 * the port's loader publishes its table into this unit's state
 * (weapons_port_type_table_set, at the end): object_type_name_index_find
 * and the four stats getters (0046f0d0-0046f1a0). The original-layout
 * records below (Weapons_Object, the vehicle and building weapon model) are
 * the original's Win32 layout and are read only in the matching build. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
/* Keeps weapons.h from renaming the port's 0046f270 entry to this unit's. */
#define LEGORRR_GAME_WEAPONS_UNIT
#include "weapons.h"
#include "weapons_imports.h"

/* The unit's functions have C linkage in the native C++ compile too: the
 * port's callers of the shared entries and the port-only accessors expect
 * it. */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct Weapons_Vector3F {
    float x, y, z;
} Weapons_Vector3F;

/* LegoObject (0x40c bytes), the fields this unit reads. */
typedef struct Weapons_Object {
    int type;                         /* +0x000 LegoObject_Type */
    int id;                           /* +0x004 */
    unsigned char pad_008[0x00c - 0x008];
    void* vehicle;                    /* +0x00c VehicleModel */
    unsigned char pad_010[0x018 - 0x010];
    void* building;                   /* +0x018 BuildingModel */
    unsigned char pad_01c[0x330 - 0x01c];
    unsigned int objLevel;            /* +0x330 */
    unsigned char pad_334[0x358 - 0x334];
    float slowDeathTimer;             /* +0x358 */
    int slowDeathWeapon;              /* +0x35c */
    float weaponCoolDown;             /* +0x360 */
    unsigned char pad_364[0x3c4 - 0x364];
    struct Weapons_Object* lazerHitObject; /* +0x3c4 */
    unsigned char pad_3c8[0x3e8 - 0x3c8];
    unsigned int flags3;              /* +0x3e8 */
    unsigned int flags4;              /* +0x3ec */
    unsigned char pad_3f0[0x40c - 0x3f0];
} Weapons_Object;

/* The weapon entry a model slot names (+0x0c: the WeaponTypes name). */
typedef struct Weapons_ModelWeapon {
    unsigned char pad_00[0x0c];
    const char* name;                 /* +0x0c */
} Weapons_ModelWeapon;

/* The weapons part of a vehicle (+0x138) or building (+0xa0) model: up to
 * three weapon slots, each with two fire nulls and an x and a y pivot. */
typedef struct Weapons_Model {
    void* fireNulls[3][2];            /* +0x00 */
    void* xPivots[3];                 /* +0x18 */
    void* yPivots[3];                 /* +0x24 */
    float pitchRange;                 /* +0x30 */
    float f_34[4];                    /* +0x34 */
    Weapons_Vector3F fireDirs[3];     /* +0x44 */
    Weapons_Vector3F targets[3];      /* +0x68 */
    Weapons_ModelWeapon* weaponNames[3]; /* +0x8c */
    float timers[3];                  /* +0x98 */
    unsigned int weaponCount;         /* +0xa4 */
} Weapons_Model;

struct Weapons_Projectile;
struct Weapons_RaySearch;

int weapons_initialise(struct BlockRec* config, const char* gameName); /* 0046eb80 */
float weapon_type_discharge_rate_get_target(int type);        /* 0046f140 */
float weapon_type_range_get_target(int type);                 /* 0046f170 */
float weapon_type_wall_destroy_time_get_target(int type, int wall_type); /* 0046f1a0 */
float weapons_get_damage_for_object(int weaponType, Weapons_Object* obj); /* 0046f1d0 */
void weapons_generic_damage_object(Weapons_Object* obj, int weaponType,
                                   int knownWeapon, float elapsed,
                                   const float* dir);         /* 0046f270 */
void weapons_gun_hit_object(Weapons_Object* obj, float damage, int mode); /* 0046f380 */
void weapons_projectile_hit_check(struct Weapons_Projectile* projectile); /* 0046f3b0 */
void weapons_update(float elapsed);                           /* 0046f550 */
void weapons_projectile_hit_object(Weapons_Object* obj, const float* dir,
                                   Weapons_Vector3F* hitPos, int weaponType,
                                   int knownWeapon);          /* 0046f770 */
void weapons_projectile_update(struct Weapons_Projectile* projectile,
                               float elapsed);                /* 0046f920 */
int weapons_projectile_create(Weapons_Object* owner, Weapons_Vector3F* pos,
                              Weapons_Vector3F* dir, int weaponType,
                              int knownWeapon);               /* 0046faf0 */
int weapons_projectile_create_curved(Weapons_Object* owner, Weapons_Vector3F* pos,
                                     Weapons_Vector3F* target, int weaponType,
                                     int knownWeapon);        /* 0046fc70 */
void weapons_get_known_weapon_object_type(int knownWeapon, int* objType,
                                          unsigned int* objID); /* 0046fef0 */
int weapons_projectile_find_free(void);                       /* 0046ff70 */
int weapons_lazer_find_free(void);                            /* 0046ff90 */
Weapons_Object* weapons_fire_lazer(Weapons_Vector3F* fromPos, Weapons_Vector3F* dir,
                                   Weapons_Object* liveObj, float elapsed,
                                   float coef, int weaponType,
                                   float discharge);          /* 0046ffb0 */
int weapons_object_collision_check(Weapons_Object* obj, float* fromPos,
                                   float* toPos, int checkSide); /* 00470260 */
int weapons_object_quad_intersect(Weapons_Object* obj, float* fromPos,
                                  float* toPos, float* intersection,
                                  int checkSide);             /* 004702b0 */
int weapons_object_center_proximity_check(Weapons_Object* obj, float* fromPos,
                                          float* toPos, int checkSide); /* 00470540 */
int weapons_object_line_side_check(Weapons_Object* obj, float* fromPos,
                                   float* dir);               /* 00470630 */
void weapons_lazer_create(void* cont, Weapons_Vector3F* fromPos,
                          Weapons_Vector3F* toPos);           /* 00470690 */
void weapons_mesh_build_segment(void* mesh, float radius,
                                Weapons_Vector3F* fromPos, Weapons_Vector3F* toPos,
                                float r, float g, float b, float a); /* 00470760 */
Weapons_Model* weapons_get_weapons_model(Weapons_Object* obj); /* 004712c0 */
float weapons_model_timer_get(Weapons_Object* obj, unsigned int slot); /* 004712f0 */
void weapons_model_timer_set(Weapons_Object* obj, float timer,
                             unsigned int slot);              /* 00471310 */
int weapons_z_within_bounds(Weapons_Vector3F* origin, Weapons_Vector3F* dir,
                            float height, float top, float dist); /* 00471330 */
void weapons_ray_block_sweep(struct Weapons_RaySearch* search); /* 00471630 */
int weapons_ray_trace_nearest(Weapons_Vector3F* origin, Weapons_Vector3F* dir,
                              Weapons_Object** hitObject, float* dist,
                              Weapons_Object* ignoreObject);  /* 00471860 */
int weapons_object_pair_ray_hit(Weapons_Object* obj, Weapons_Object* target); /* 004718d0 */
int weapons_see_through_walls_check(Weapons_Object* obj,
                                    Weapons_Object* target);  /* 00471960 */
void* weapons_model_fire_null_get(Weapons_Model* model, unsigned int slot,
                                  unsigned int index);        /* 00471a20 */
void* weapons_model_x_pivot_get(Weapons_Model* model, unsigned int slot); /* 00471a40 */
void* weapons_model_y_pivot_get(Weapons_Model* model, unsigned int slot); /* 00471a50 */
void weapons_model_aim(Weapons_Model* model, Weapons_Vector3F* target,
                       float elapsed, int setTarget, unsigned int slot); /* 00471a60 */
void weapons_model_fire_dir_get(Weapons_Model* model, Weapons_Vector3F* dir,
                                unsigned int slot);           /* 00471c70 */
int weapons_object_center_get(Weapons_Object* obj, Weapons_Vector3F* center); /* 00471ca0 */
int weapons_object_can_be_hit(Weapons_Object* obj);           /* 00471ce0 */
int weapons_model_fire_update(Weapons_Object* obj, float elapsed,
                              unsigned int slot);             /* 00471d20 */
int weapons_model_aim_update(Weapons_Object* obj, float elapsed,
                             unsigned int slot);              /* 00471f00 */
int weapons_object_update_weapons(Weapons_Object* obj, float elapsed); /* 00471fc0 */
int weapons_object_in_range(Weapons_Object* obj, Weapons_Object* target); /* 00472080 */
int weapons_object_in_touch_range(Weapons_Object* obj,
                                  Weapons_Object* target);    /* 00472130 */

/* Port-only: the port's loader shares the unit's table. */
void weapons_port_type_table_set(unsigned int count, char** names, void* stats);
void** weapons_port_stats_slot(void);

#ifdef __cplusplus
}
#endif

/* The callees' declarations take the port's EntitySlot and the GAME-023
 * unit's Lego_Object; the unit passes its own view of the same record. */
#define WEAPONS_ENT(obj) ((struct EntitySlot*)(obj))
#define WEAPONS_LOBJ(obj) ((Lego_Object*)(obj))

#include "weapons_maths.h"

/* Weapons_Stats (0x4b68 bytes): one WeaponTypes entry. The damage grid is
 * indexed by the target's object type, object index and object level. */
typedef struct Weapons_Stats {
    float objectDamage[20][15][16]; /* +0x0000 */
    int slowDeath;                  /* +0x4b00 */
    float slowDeathDamage;          /* +0x4b04 */
    float slowDeathDuration;        /* +0x4b08 */
    float rechargeTime;             /* +0x4b0c */
    float defaultDamage;            /* +0x4b10 */
    float dischargeRate;            /* +0x4b14 */
    int ammo;                       /* +0x4b18 */
    float weaponRange;              /* +0x4b1c */
    float wallDestroyTimes[18];     /* +0x4b20 */
} Weapons_Stats;

/* Weapons_Lazer (0x10 bytes): a timed laser beam, two meshes. */
typedef struct Weapons_Lazer {
    void* innerMesh;                /* +0x00 */
    void* outerMesh;                /* +0x04 */
    void* container;                /* +0x08 the firing object's container */
    float timer;                    /* +0x0c */
} Weapons_Lazer;

/* Weapons_Projectile (0x2b0 bytes): a flying projectile, straight or along
 * a curve in the vertical plane of its direction. */
typedef struct Weapons_Projectile {
    int curved;                     /* +0x000 */
    BezierCurve curve;              /* +0x004 */
    float curveLength;              /* +0x260 */
    float curveDistance;            /* +0x264 */
    Weapons_Vector3F startPos;      /* +0x268 */
    Weapons_Vector3F position;      /* +0x274 */
    Weapons_Vector3F lastPosition;  /* +0x280 */
    float speed;                    /* +0x28c */
    Weapons_Vector3F direction;     /* +0x290 */
    int field_29c;                  /* +0x29c */
    Weapons_Object* owner;          /* +0x2a0 */
    Weapons_Object* object;         /* +0x2a4 the projectile's own object */
    int weaponType;                 /* +0x2a8 */
    int knownWeapon;                /* +0x2ac */
} Weapons_Projectile;

/* Weapons.c's .bss, 00503870-00505400: one struct, laid out as the
 * original. */
typedef struct Weapons_Globs {
    unsigned int weaponCount;               /* 00503870 */
    char** weaponNameList;                  /* 00503874 */
    Weapons_Stats* weaponStatsList;         /* 00503878 */
    Weapons_Lazer lazerList[10];            /* 0050387c */
    Weapons_Projectile projectileList[10];  /* 0050391c */
    BlockFile* config;                      /* 005053fc */
} Weapons_Globs;  /* 0x1b90 bytes */

// GLOBAL: LEGORR 0x00503870
static Weapons_Globs weaponGlobs;

/* The ray search record of the trace functions (0x18 bytes). */
typedef struct Weapons_RaySearch {
    Weapons_Object* hitObject;      /* +0x00 */
    float hitDist;                  /* +0x04 */
    Weapons_Vector3F* origin;       /* +0x08 */
    Weapons_Vector3F* direction;    /* +0x0c */
    int hit;                        /* +0x10 */
    Weapons_Object* ignoreObject;   /* +0x14 */
} Weapons_RaySearch;

static int weapons_callback_update_object(Weapons_Object* obj, float* elapsed);
static int weapons_callback_ray_candidate(Weapons_Object* obj,
                                          Weapons_RaySearch* search);

/* Config.c's pooled ":" (004a1cd4): the original's literal resolves to it in
 * every unit (finding 581), so Weapons.c reads it by name (finding 630). */
#ifdef __cplusplus
extern "C" {
#endif
extern const char s_config_rgb_separator[];
#ifdef __cplusplus
}
#endif

// FUNCTION: LEGORR 0x0046eb80
int weapons_initialise(struct BlockRec* config, const char* gameName)
{
    BlockRec* arrayItem;
    BlockRec* prop;
    unsigned int i;
    unsigned int objType;
    unsigned int objIndex;
    unsigned int objLevel;
    unsigned int j;
    unsigned int levels;
    char* parts[16];
    char buff[128];

    weaponGlobs.config = config;
    weaponGlobs.weaponCount = 0;
    arrayItem = block_subgroup(config,
        // STRING: LEGORR 0x004aaaa0
        string_path_join(gameName, "WeaponTypes", NULL));
    if (arrayItem != NULL) {
        do {
            weaponGlobs.weaponCount++;
            arrayItem = block_next_equal_key(arrayItem);
        } while (arrayItem != NULL);

        weaponGlobs.weaponStatsList = (Weapons_Stats*)malloc(
            weaponGlobs.weaponCount * sizeof(Weapons_Stats));
        if (weaponGlobs.weaponStatsList != NULL) {
            for (i = 0; i < weaponGlobs.weaponCount; i++) {
                for (objType = 0; objType < 20; objType++) {
                    for (objIndex = 0; objIndex < 15; objIndex++) {
                        for (objLevel = 0; objLevel < 16; objLevel++) {
                            weaponGlobs.weaponStatsList[i].objectDamage[objType][objIndex][objLevel] = -1.0f;
                        }
                    }
                }
                weaponGlobs.weaponStatsList[i].slowDeath = 0;
                weaponGlobs.weaponStatsList[i].rechargeTime = 0.0f;
                weaponGlobs.weaponStatsList[i].defaultDamage = 0.0f;
                weaponGlobs.weaponStatsList[i].weaponRange = 150.0f;
                for (j = 0; j < 18; j++) {
                    weaponGlobs.weaponStatsList[i].wallDestroyTimes[j] = 5.0f;
                }
            }

            weaponGlobs.weaponNameList = (char**)malloc(
                weaponGlobs.weaponCount * sizeof(char*));
            if (weaponGlobs.weaponNameList != NULL) {
                i = 0;
                for (arrayItem = block_subgroup(config,
                         string_path_join(gameName, "WeaponTypes", NULL));
                     arrayItem != NULL;
                     arrayItem = block_next_equal_key(arrayItem)) {
                    prop = block_subgroup(config,
                        string_path_join(gameName, "WeaponTypes",
                                         arrayItem->key, NULL));
                    while (prop != NULL) {
                        // STRING: LEGORR 0x004aaa94
                        if (_strcmpi(prop->key, "SlowDeath") == 0) {
                            sound_variant_split(prop->value, parts, s_config_rgb_separator);
                            weaponGlobs.weaponStatsList[i].slowDeath = 1;
                            weaponGlobs.weaponStatsList[i].slowDeathDamage = (float)atof(parts[0]);
                            weaponGlobs.weaponStatsList[i].slowDeathDuration = (float)atof(parts[1]);
                        }
                        // STRING: LEGORR 0x004aaa84
                        else if (_strcmpi(prop->key, "RechargeTime") == 0) {
                            weaponGlobs.weaponStatsList[i].rechargeTime = (float)atof(prop->value);
                        }
                        // STRING: LEGORR 0x004aaa74
                        else if (_strcmpi(prop->key, "DefaultDamage") == 0) {
                            weaponGlobs.weaponStatsList[i].defaultDamage = (float)atof(prop->value);
                        }
                        // STRING: LEGORR 0x004aaa64
                        else if (_strcmpi(prop->key, "DischargeRate") == 0) {
                            weaponGlobs.weaponStatsList[i].dischargeRate = (float)atof(prop->value);
                        }
                        // STRING: LEGORR 0x004aaa58
                        else if (_strcmpi(prop->key, "WeaponRange") == 0) {
                            weaponGlobs.weaponStatsList[i].weaponRange = (float)atof(prop->value);
                        }
                        else {
                            for (j = 0; j < 18; j++) {
                                // STRING: LEGORR 0x004aaa44
                                sprintf(buff, "WallDestroyTime_%s",
                                        legoGlobs_langSurface_name[j] + 17);
                                if (_strcmpi(prop->key, buff) == 0) {
                                    weaponGlobs.weaponStatsList[i].wallDestroyTimes[j] = (float)atof(prop->value);
                                    prop = block_next_equal_key(prop);
                                    break;
                                }
                            }
                            if (j != 18) continue;
                            // STRING: LEGORR 0x004aaa3c
                            if (_strcmpi(prop->key, "Ammo") == 0) {
                                weaponGlobs.weaponStatsList[i].ammo = atoi(prop->value);
                            }
                            else {
                                weaponGlobs.weaponStatsList[i].ammo = 0;
                                if (!lego_entity_type_pair_lookup(prop->key, (int*)&objType,
                                                                  &objIndex, NULL)) continue;
                                levels = building_upgrade_admission_check(objType, objIndex);
                                sound_variant_split(prop->value, parts, s_config_rgb_separator);
                                for (objLevel = 0; objLevel < levels; objLevel++) {
                                    weaponGlobs.weaponStatsList[i].objectDamage[objType][objIndex][objLevel] =
                                        (float)atof(parts[objLevel]);
                                }
                            }
                        }
                        prop = block_next_equal_key(prop);
                    }

                    weaponGlobs.weaponNameList[i] = (char*)malloc(strlen(arrayItem->key) + 1);
                    strcpy(weaponGlobs.weaponNameList[i], arrayItem->key);
                    i++;
                }
                if (arrayItem == NULL) return 1;
                free(weaponGlobs.weaponNameList);
            }
            free(weaponGlobs.weaponStatsList);
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0046f0d0
uint32_t object_type_name_index_find(const char* name)
{
    unsigned int i;
    for (i = 0; i < weaponGlobs.weaponCount; i++) {
        if (_strcmpi(weaponGlobs.weaponNameList[i], name) == 0) return i;
    }
    return weaponGlobs.weaponCount + 1;
}

// FUNCTION: LEGORR 0x0046f110
float weapon_type_recharge_time_get_target(int type)
{
    return weaponGlobs.weaponStatsList[type].rechargeTime;
}

// FUNCTION: LEGORR 0x0046f140
float weapon_type_discharge_rate_get_target(int type)
{
    return weaponGlobs.weaponStatsList[type].dischargeRate;
}

// FUNCTION: LEGORR 0x0046f170
float weapon_type_range_get_target(int type)
{
    return weaponGlobs.weaponStatsList[type].weaponRange;
}

// FUNCTION: LEGORR 0x0046f1a0
float weapon_type_wall_destroy_time_get_target(int type, int wall_type)
{
    return weaponGlobs.weaponStatsList[type].wallDestroyTimes[wall_type];
}

// FUNCTION: LEGORR 0x0046f1d0
float weapons_get_damage_for_object(int weaponType, Weapons_Object* obj)
{
    if (-1.0f == weaponGlobs.weaponStatsList[weaponType].objectDamage[obj->type][obj->id][obj->objLevel]) {
        return weaponGlobs.weaponStatsList[weaponType].defaultDamage;
    }
    return weaponGlobs.weaponStatsList[weaponType].objectDamage[obj->type][obj->id][obj->objLevel];
}

// FUNCTION: LEGORR 0x0046f270
void weapons_generic_damage_object(Weapons_Object* obj, int weaponType,
                                   int knownWeapon, float elapsed,
                                   const float* dir)
{
    float damage = weapons_get_damage_for_object(weaponType, obj);
    if ((obj->flags3 & 0x40000) && legoobject_shared_eligibility_check(WEAPONS_LOBJ(obj), 1)) {
        if (weaponGlobs.weaponStatsList[weaponType].slowDeath) {
            if (knownWeapon) {
                legoobject_damage_apply(WEAPONS_LOBJ(obj), weaponGlobs.weaponStatsList[weaponType].slowDeathDamage * damage, 1, elapsed);
                obj->slowDeathTimer = weaponGlobs.weaponStatsList[weaponType].slowDeathDuration;
                obj->slowDeathWeapon = weaponType;
                entity_directional_activity_start(WEAPONS_ENT(obj), dir, 1);
            }
            else {
                legoobject_damage_apply(WEAPONS_LOBJ(obj), damage * elapsed, 1, elapsed);
            }
        }
        else {
            if (obj->type == 3) global_counter_at_0055532c_increment();
            legoobject_damage_apply(WEAPONS_LOBJ(obj), damage, 1, elapsed);
            if (knownWeapon) entity_directional_activity_start(WEAPONS_ENT(obj), dir, 1);
        }
    }
}

// FUNCTION: LEGORR 0x0046f380
void weapons_gun_hit_object(Weapons_Object* obj, float damage, int mode)
{
    legoobject_damage_apply(WEAPONS_LOBJ(obj), damage, 1, 1.0f);
    entity_directional_activity_start(WEAPONS_ENT(obj), NULL, mode);
}

// FUNCTION: LEGORR 0x0046f3b0
void weapons_projectile_hit_check(Weapons_Projectile* projectile)
{
    Weapons_RaySearch search;
    Weapons_Vector3F dir;
    float range;

    if (projectile->object != NULL) {
        search.hitObject = NULL;
        search.hitDist = 1.0f;
        search.origin = &projectile->lastPosition;
        search.direction = &dir;
        search.hit = 0;
        search.ignoreObject = projectile->owner;
        range = weapon_type_range_get_target(projectile->weaponType);
        if (range != 0.0f &&
            weapons_vector3d_distance(&projectile->startPos, &projectile->position) >= range) {
            legoobject_remove(WEAPONS_LOBJ(projectile->object));
            projectile->object = NULL;
            return;
        }
        weapons_vector3d_subtract(&dir, &projectile->position, &projectile->lastPosition);
        legoobject_run_through_lists_skip_upgrade_parts(
            (Lego_ObjectCallback)weapons_callback_ray_candidate, &search);
        weapons_ray_block_sweep(&search);
        if (search.hit) {
            legoobject_remove(WEAPONS_LOBJ(projectile->object));
            projectile->object = NULL;
            if (search.hitObject != NULL) {
                Weapons_Vector3F hitPos;
                weapons_projectile_hit_object(search.hitObject,
                    (const float*)&projectile->direction,
                    weapons_ray_end_point(&hitPos, search.origin,
                                          search.direction, search.hitDist),
                    projectile->weaponType, projectile->knownWeapon);
            }
        }
    }
}

// FUNCTION: LEGORR 0x0046f550
void weapons_update(float elapsed)
{
    unsigned int i;

    for (i = 0; i < 10; i++) {
        if (weaponGlobs.projectileList[i].object != NULL) {
            weapons_projectile_update(&weaponGlobs.projectileList[i], elapsed);
        }
    }
    legoobject_run_through_lists_skip_upgrade_parts(
        (Lego_ObjectCallback)weapons_callback_update_object, &elapsed);
    for (i = 0; i < 10; i++) {
        weapons_projectile_hit_check(&weaponGlobs.projectileList[i]);
    }
    for (i = 0; i < 10; i++) {
        if (weaponGlobs.lazerList[i].innerMesh != NULL) {
            if (weaponGlobs.lazerList[i].timer < 0.0f) {
                mesh_remove(weaponGlobs.lazerList[i].innerMesh,
                            WEAPONS_CONTAINER_FRAME(weaponGlobs.lazerList[i].container));
                mesh_remove(weaponGlobs.lazerList[i].outerMesh,
                            WEAPONS_CONTAINER_FRAME(weaponGlobs.lazerList[i].container));
                weaponGlobs.lazerList[i].innerMesh = NULL;
            }
            weaponGlobs.lazerList[i].timer -= elapsed;
        }
    }
}

// FUNCTION: LEGORR 0x0046f610
static int weapons_callback_update_object(Weapons_Object* obj, float* elapsed)
{
    Weapons_Vector3F pos;
    unsigned int slot;
    float timer;
    float soundTime;

    if (obj->weaponCoolDown > 0.0f) obj->weaponCoolDown -= *elapsed;
    if (weapons_get_weapons_model(obj) != NULL) {
        for (slot = 0; slot < weapons_get_weapons_model(obj)->weaponCount; slot++) {
            timer = weapons_model_timer_get(obj, slot);
            if (timer > 0.0f) {
                weapons_model_timer_set(obj, weapons_model_timer_get(obj, slot) - *elapsed, slot);
                soundTime = s_scroll_scale * (float)(sound_object_duration_get(0x1d) * 25.0f);
                if (weapons_model_timer_get(obj, slot) < soundTime && timer > soundTime) {
                    ddiscene_object_position_get(entity_type_component_get(obj), NULL, (float*)&pos);
                    sound_submission_route_wrapper(NULL, 0x1d, 0, 0, (const float*)&pos);
                }
            }
        }
    }
    if (obj->slowDeathTimer > 0.0f) {
        weapons_generic_damage_object(obj, obj->slowDeathWeapon, 0, *elapsed, NULL);
        obj->slowDeathTimer -= *elapsed;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0046f770
void weapons_projectile_hit_object(Weapons_Object* obj, const float* dir,
                                   Weapons_Vector3F* hitPos, int weaponType,
                                   int knownWeapon)
{
    float dir2D[2];
    float damage;
    float scale;

    dir2D[0] = dir[0];
    dir2D[1] = dir[1];
    if (legoobject_type3_flag04000000_test(WEAPONS_LOBJ(obj))) scene_light_create_at_position((const float*)hitPos);
    switch (knownWeapon) {
    case 1:
        if (entity_state_flags_get(WEAPONS_ENT(obj)) & 0x2000000) {
            damage = entity_stats_float_at11c(WEAPONS_ENT(obj));
            weapons_gun_hit_object(obj, damage, 1);
        }
        if (hitPos != NULL) lava_effect_select_and_place(0, (const float*)hitPos, NULL);
        break;
    case 2:
        if (entity_state_flags_get(WEAPONS_ENT(obj)) & 0x4000000) {
            scale = entity_stats_float_at114(WEAPONS_ENT(obj));
            damage = entity_stats_float_at118(WEAPONS_ENT(obj));
            dir2D[0] *= 1.5f;
            dir2D[1] *= 1.5f;
            weapons_gun_hit_object(obj, damage, 1);
            entity_pair_scalar_fields_set_original(WEAPONS_ENT(obj), dir2D, scale);
        }
        if (hitPos != NULL) lava_effect_select_and_place(1, (const float*)hitPos, NULL);
        break;
    case 3:
        /* damage holds the effect time here, scale the damage. */
        damage = 10.0f;
        if (entity_state_flags_get(WEAPONS_ENT(obj)) & 0x1000000) {
            scale = entity_stats_float_at10c(WEAPONS_ENT(obj));
            if (scale > 0.0f) damage = scale;
            scale = entity_stats_float_at110(WEAPONS_ENT(obj));
            weapons_gun_hit_object(obj, scale, 0);
            entity_one_shot_attached_effect_start(WEAPONS_ENT(obj), damage);
        }
        if (hitPos != NULL) lava_effect_select_and_place(2, (const float*)hitPos, NULL);
        break;
    default:
        weapons_generic_damage_object(obj, weaponType, 1, 0.0f, dir2D);
        break;
    }
}

// FUNCTION: LEGORR 0x0046f920
void weapons_projectile_update(Weapons_Projectile* projectile, float elapsed)
{
    void* cont;
    Point2F curvePos;

    cont = entity_type_component_get(projectile->object);
    ddiscene_object_position_get(cont, NULL, (float*)&projectile->lastPosition);
    elapsed = projectile->speed * elapsed;
    if (!projectile->curved) {
        weapons_ray_end_point(&projectile->position, &projectile->position,
                              &projectile->direction, elapsed);
        ddiscene_object_position_set(cont, NULL, projectile->position.x,
                                     projectile->position.y, projectile->position.z);
        ddiscene_object_pose_method_dispatch(cont, NULL, projectile->direction.x,
                                             projectile->direction.y,
                                             projectile->direction.z,
                                             0.0f, 0.0f, -1.0f);
    }
    else {
        elapsed += projectile->curveDistance;
        projectile->curveDistance = elapsed;
        path_arc_length_interpolate(&projectile->curve, elapsed, &curvePos);
        projectile->position.x = projectile->direction.x * curvePos.x + projectile->startPos.x;
        projectile->position.y = projectile->startPos.y + projectile->direction.y * curvePos.x;
        projectile->position.z = projectile->startPos.z + curvePos.y;
        ddiscene_object_position_set(cont, NULL, projectile->position.x,
                                     projectile->position.y, projectile->position.z);
        ddiscene_object_pose_method_dispatch(
            cont, NULL, projectile->position.x - projectile->lastPosition.x,
            projectile->position.y - projectile->lastPosition.y,
            (projectile->position.z - projectile->lastPosition.z) * 0.25f,
            0.0f, 0.0f, -1.0f);
    }
}

// FUNCTION: LEGORR 0x0046faf0
int weapons_projectile_create(Weapons_Object* owner, Weapons_Vector3F* pos,
                              Weapons_Vector3F* dir, int weaponType,
                              int knownWeapon)
{
    int objType;
    unsigned int objID;
    void* model;
    int index;
    Weapons_Projectile* projectile;

    if (knownWeapon == 1) legoobject_process_queued_grid_animations(owner, weaponType, 1.0f);
    weapons_get_known_weapon_object_type(knownWeapon, &objType, &objID);
    if (lego_entity_at(objType, objID, &model)) {
        index = weapons_projectile_find_free();
        if (index != -1) {
            projectile = &weaponGlobs.projectileList[index];
            projectile->curved = 0;
            projectile->weaponType = weaponType;
            projectile->knownWeapon = knownWeapon;
            projectile->position = *pos;
            projectile->startPos = *pos;
            projectile->speed = 5.0f;
            projectile->direction = *dir;
            weapons_vector3d_normalise(&projectile->direction);
            projectile->owner = owner;
            projectile->field_29c = 0;
            projectile->object = (Weapons_Object*)legoobject_create_in_world(model, objType, objID, 0,
                                                            pos->x, pos->y, 0.0f);
            weapons_projectile_update(projectile, 0.0f);
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0046fc70
int weapons_projectile_create_curved(Weapons_Object* owner, Weapons_Vector3F* pos,
                                     Weapons_Vector3F* target, int weaponType,
                                     int knownWeapon)
{
    int objType;
    unsigned int objID;
    void* model;
    int index;
    Weapons_Projectile* projectile;
    Point2F p0;
    Point2F p1;
    Point2F p2;
    Point2F p3;
    float dist;

    p0.x = 0.0f;
    p0.y = 0.0f;
    p1.x = 1.0f;
    p1.y = -1.3f;
    p3.x = 0.3f;
    p3.y = 1.0f;
    weapons_get_known_weapon_object_type(knownWeapon, &objType, &objID);
    if (lego_entity_at(objType, objID, &model)) {
        index = weapons_projectile_find_free();
        if (index != -1) {
            projectile = &weaponGlobs.projectileList[index];
            projectile->knownWeapon = knownWeapon;
            projectile->weaponType = weaponType;
            projectile->curved = 1;
            projectile->startPos = *pos;
            projectile->position = *pos;
            weapons_vector3d_subtract(&projectile->direction, target, pos);
            weapons_vector3d_normalise(&projectile->direction);
            projectile->speed = 4.0f;
            p2.y = target->z - pos->z;
            p2.x = dist = weapons_vector3d_distance(pos, target);
            vector2_set_length(&p1, dist * 0.2f);
            vector2_set_length(&p3, dist * 0.33333334f);
            bezier_curve_samples_build(&projectile->curve, &p0, &p1, &p2, &p3, 50);
            projectile->curveLength = polyline_segment_lengths_total(&projectile->curve);
            projectile->curveDistance = 0.0f;
            projectile->owner = owner;
            projectile->field_29c = 0;
            projectile->object = (Weapons_Object*)legoobject_create_in_world(model, objType, objID, 0,
                                                            pos->x, pos->y, 0.0f);
            weapons_projectile_update(projectile, 0.0f);
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0046fef0
void weapons_get_known_weapon_object_type(int knownWeapon, int* objType,
                                          unsigned int* objID)
{
    switch (knownWeapon) {
    case 1:
        *objType = 0x13;
        *objID = 0;
        break;
    case 2:
        *objType = 0x10;
        *objID = 0;
        break;
    case 3:
        *objType = 0x11;
        *objID = 0;
        break;
    case 4:
        *objType = 5;
        *objID = 0;
        break;
    }
}

// FUNCTION: LEGORR 0x0046ff70
int weapons_projectile_find_free(void)
{
    unsigned int i;
    for (i = 0; i < 10; i++) {
        if (weaponGlobs.projectileList[i].object == NULL) return i;
    }
    return -1;
}

// FUNCTION: LEGORR 0x0046ff90
int weapons_lazer_find_free(void)
{
    unsigned int i;
    for (i = 0; i < 10; i++) {
        if (weaponGlobs.lazerList[i].innerMesh == NULL) return i;
    }
    return -1;
}

// FUNCTION: LEGORR 0x0046ffb0
Weapons_Object* weapons_fire_lazer(Weapons_Vector3F* fromPos, Weapons_Vector3F* dir,
                                   Weapons_Object* liveObj, float elapsed,
                                   float coef, int weaponType, float discharge)
{
    float dist;
    Weapons_Object* hitObject;
    Weapons_Vector3F rayDir;
    Weapons_Vector3F endPos;

    dist = 1.0f;
    hitObject = NULL;
    sound_submission_route_wrapper(NULL, 0x1b, 0, 0, (const float*)fromPos);
    weapons_vector3d_scale(&rayDir, dir, coef);
    weapons_ray_trace_nearest(fromPos, &rayDir, &hitObject, &dist, liveObj);
    weapons_vector3d_scale(&rayDir, &rayDir, dist);
    weapons_vector3d_add(&endPos, &rayDir, fromPos);
    weapons_lazer_create(entity_type_component_get(liveObj), fromPos, &endPos);
    if (hitObject == NULL && dist < 1.0f) {
        int blockPos[2];
        unsigned char surface;

        scene_light_create_at_position((const float*)&endPos);
        sound_submission_route_wrapper(NULL, 0x1c, 0, 0, (const float*)&endPos);
        terrain_map_world_to_block(WEAPONS_LEGO_GET_MAP(), endPos.x, endPos.y,
                                   &blockPos[0], &blockPos[1]);
        if (terrain_cell_walkable_flag_current(blockPos[0], blockPos[1]) &&
            !terrain_cell_state_equals_one_current(blockPos)) {
            surface = legoGlobs_currLevel_blocks[legoGlobs_currLevel->width * blockPos[1] + blockPos[0]].f_02;
            if (surface != 0xb) {
                coef = weapon_type_wall_destroy_time_get_target(weaponType, surface);
                if (terrain_cell_drill_work_add_current(blockPos[0], blockPos[1], coef, elapsed)) {
                    lego_terrain_cell_mutate(WEAPONS_LEGO_GET_LEVEL(), blockPos[0], blockPos[1], 0);
                }
                goto done;
            }
        }
        if (level_cell_occupied_at_current(blockPos)) {
            terrain_cell_work_flag_clear_current(blockPos, 0x3f);
            lego_cell_init_pass(legoGlobs_currLevel, blockPos[0], blockPos[1], 0);
        }
    }
    else if (hitObject != NULL) {
        lava_effect_select_and_place(0, (const float*)&endPos, NULL);
        sound_submission_route_wrapper(NULL, 0x1c, 0, 0, (const float*)&endPos);
        if (hitObject != NULL) {
            float dir2D[2];

            if (liveObj != NULL && liveObj->type == 2) {
                dir2D[0] = dir->x;
                dir2D[1] = dir->y;
                entity_directional_activity_start(WEAPONS_ENT(hitObject), dir2D, 1);
            }
            else {
                entity_directional_activity_start(WEAPONS_ENT(hitObject), NULL, 1);
            }
        }
    }
done:
    legoobject_process_queued_grid_animations(liveObj, weaponType, discharge);
    return hitObject;
}

// FUNCTION: LEGORR 0x00470260
int weapons_object_collision_check(Weapons_Object* obj, float* fromPos,
                                   float* toPos, int checkSide)
{
    if (entity_stats_word_get(WEAPONS_ENT(obj)) & 0x100)
        return weapons_object_quad_intersect(obj, fromPos, toPos, NULL, checkSide);
    return weapons_object_center_proximity_check(obj, fromPos, toPos, checkSide);
}

// FUNCTION: LEGORR 0x004702b0
int weapons_object_quad_intersect(Weapons_Object* obj, float* fromPos,
                                  float* toPos, float* intersection,
                                  int checkSide)
{
    void* cont;
    float from[2];
    float to[2];
    float corners[4][3];
    float points[5][2];
    float edge[2];
    float hit[2];
    unsigned int i;
    float* pt;

    cont = entity_type_component_get(obj);
    from[0] = fromPos[0];
    from[1] = fromPos[1];
    to[0] = toPos[0];
    to[1] = toPos[1];
    if (checkSide && weapons_object_line_side_check(obj, from, to)) return 0;
    corners[0][0] = ((float*)entity_stats_field_at_a0_corpus(WEAPONS_ENT(obj)))[0] * -0.5f;
    corners[0][2] = ((float*)entity_stats_field_at_a0_corpus(WEAPONS_ENT(obj)))[1] * 0.5f;
    corners[1][0] = ((float*)entity_stats_field_at_a0_corpus(WEAPONS_ENT(obj)))[0] * 0.5f;
    corners[1][2] = ((float*)entity_stats_field_at_a0_corpus(WEAPONS_ENT(obj)))[1] * 0.5f;
    corners[2][0] = ((float*)entity_stats_field_at_a0_corpus(WEAPONS_ENT(obj)))[0] * 0.5f;
    corners[2][2] = ((float*)entity_stats_field_at_a0_corpus(WEAPONS_ENT(obj)))[1] * -0.5f;
    corners[3][0] = ((float*)entity_stats_field_at_a0_corpus(WEAPONS_ENT(obj)))[0] * -0.5f;
    corners[3][2] = ((float*)entity_stats_field_at_a0_corpus(WEAPONS_ENT(obj)))[1] * -0.5f;
    corners[3][1] = 0.0f;
    corners[2][1] = 0.0f;
    corners[1][1] = 0.0f;
    corners[0][1] = 0.0f;
    for (i = 0; i < 4; i++) {
        scene_object_coordinate_transform_in_place(cont, corners[i], corners[i]);
        points[i][0] = corners[i][0];
        points[i][1] = corners[i][1];
    }
    points[4][0] = points[0][0];
    points[4][1] = points[0][1];
    for (i = 0, pt = points[0]; i < 4; i++, pt += 2) {
        edge[0] = pt[0] - pt[2];
        edge[1] = pt[1] - pt[3];
        if (line_intersection_2d_compute(hit, from, to, pt, edge)) {
            if ((float)fabs(pt[0] - pt[2]) <= (float)fabs(pt[1] - pt[3])) {
                if ((hit[1] > pt[1] && hit[1] < pt[3]) ||
                    (hit[1] < pt[1] && hit[1] > pt[3])) goto found;
            }
            else if ((hit[0] > pt[0] && hit[0] < pt[2]) ||
                     (hit[0] < pt[0] && hit[0] > pt[2])) {
found:
                if (intersection != NULL) {
                    intersection[0] = hit[0];
                    intersection[1] = hit[1];
                }
                return 1;
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00470540
int weapons_object_center_proximity_check(Weapons_Object* obj, float* fromPos,
                                          float* toPos, int checkSide)
{
    float from[2];
    float to[2];
    float end[2];
    float objPos[2];
    float proj[2];
    float dist;

    from[0] = fromPos[0];
    from[1] = fromPos[1];
    to[0] = toPos[0];
    to[1] = toPos[1];
    if (!checkSide || !weapons_object_line_side_check(obj, from, to)) {
        end[0] = to[0] + from[0];
        end[1] = to[1] + from[1];
        legoobject_transform_position_get(WEAPONS_LOBJ(obj), &objPos[0], &objPos[1]);
        point_project_onto_line2d(from, end, objPos, proj);
        dist = (float)sqrt((proj[0] - objPos[0]) * (proj[0] - objPos[0]) +
                          (proj[1] - objPos[1]) * (proj[1] - objPos[1]));
        if (dist < entity_stats_float_at6c(WEAPONS_ENT(obj))) return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00470630
int weapons_object_line_side_check(Weapons_Object* obj, float* fromPos, float* dir)
{
    float objPos[2];
    legoobject_transform_position_get(WEAPONS_LOBJ(obj), &objPos[0], &objPos[1]);
    if ((objPos[0] - fromPos[0]) * dir[0] + (objPos[1] - fromPos[1]) * dir[1] < 0.0f) return 1;
    return 0;
}

// FUNCTION: LEGORR 0x00470690
void weapons_lazer_create(void* cont, Weapons_Vector3F* fromPos,
                          Weapons_Vector3F* toPos)
{
    int index = weapons_lazer_find_free();
    if (index != -1) {
        weaponGlobs.lazerList[index].innerMesh = engine_task_callback_record_allocate_and_initialize(
            WEAPONS_CONTAINER_FRAME(cont), NULL, 0x4400, NULL, 0);
        weaponGlobs.lazerList[index].outerMesh = engine_task_callback_record_allocate_and_initialize(
            WEAPONS_CONTAINER_FRAME(cont), NULL, 0x4400, NULL, 0);
        weapons_mesh_build_segment(weaponGlobs.lazerList[index].innerMesh, 0.3f,
                                   fromPos, toPos, 0.6f, 0.6f, 0.6f, 1.0f);
        weapons_mesh_build_segment(weaponGlobs.lazerList[index].outerMesh, 1.0f,
                                   fromPos, toPos, 0.1f, 0.2f, 0.5f, 1.0f);
        weaponGlobs.lazerList[index].container = cont;
        weaponGlobs.lazerList[index].timer = 5.0f;
    }
}

// FUNCTION: LEGORR 0x00470760
void weapons_mesh_build_segment(void* mesh, float radius,
                                Weapons_Vector3F* fromPos, Weapons_Vector3F* toPos,
                                float r, float g, float b, float a)
{
    Weapons_Vector3F normals[8];
    Weapons_Vector3F vertices[16];
    float textCoords[128] = {
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f
    };
    unsigned int faceData[48] = {
        0, 1, 9, 9, 8, 0, 1, 2, 10, 10, 9, 1, 2, 3, 11, 11, 10, 2, 3, 4, 12,
        12, 11, 3, 4, 5, 13, 13, 12, 4, 5, 6, 14, 14, 13, 5, 6, 7, 15, 15, 14,
        6, 7, 0, 8, 8, 15, 7
    };
    Weapons_Vector3F dir;
    float px;
    float py;
    float ux;
    float uy;
    float vx;
    float vy;
    float vz;
    float dx;
    float dy;
    float dz;
    float ex;
    float ez;
    float len;
    unsigned int i;
    unsigned int group;

    weapons_vector3d_subtract(&dir, toPos, fromPos);
    len = 1.0f / weapons_vector3d_modulus(&dir);
    dir.y = len * dir.y;
    dir.z = len * dir.z;
    if (dir.y == 0.0f && dir.z == 0.0f) {
        py = 1.0f;
        px = 0.0f;
    }
    else if (dir.y == 0.0f) {
        px = 1.0f;
        py = 0.0f;
    }
    else {
        py = (float)sqrt(1.0f / ((dir.z * dir.z) / (dir.y * dir.y) - -1.0f));
        px = -((dir.z / dir.y) * py);
    }
    ux = px * radius;
    uy = py * radius;
    vx = (py * dir.y - px * dir.z) * radius;
    vy = -(py * len * dir.x) * radius;
    vz = px * len * dir.x * radius;
    ex = ux * 0.41421354f;
    dx = vx * 0.41421354f;
    dy = vy * 0.41421354f;
    dz = vz * 0.41421354f;
    ez = uy * 0.41421354f;
    vertices[0].x = dx + 0.0f;  vertices[0].y = ux + dy;  vertices[0].z = uy + dz;
    vertices[1].x = vx + 0.0f;  vertices[1].y = vy + ex;  vertices[1].z = vz + ez;
    vertices[2].x = vx + -0.0f; vertices[2].y = vy + -ex; vertices[2].z = vz + -ez;
    vertices[3].x = dx - 0.0f;  vertices[3].y = dy - ux;  vertices[3].z = dz - uy;
    vertices[4].x = -0.0f - dx; vertices[4].y = -ux - dy; vertices[4].z = -uy - dz;
    vertices[5].x = -0.0f - vx; vertices[5].y = -ex - vy; vertices[5].z = -ez - vz;
    vertices[6].x = 0.0f - vx;  vertices[6].y = ex - vy;  vertices[6].z = ez - vz;
    vertices[7].x = 0.0f - dx;  vertices[7].y = ux - dy;  vertices[7].z = uy - dz;
    for (i = 0; i < 8; i++) {
        normals[i] = vertices[i];
        weapons_vector3d_add(&vertices[i + 8], toPos, &vertices[i]);
        weapons_vector3d_normalise(&normals[i]);
        weapons_vector3d_add(&vertices[i], fromPos, &vertices[i]);
    }
    group = mesh_record_allocate_and_fill((struct MeshRecordPool*)mesh, 16, 16, 3,
                                           (const uint16_t*)faceData);
    render_object_color_vector_set(mesh, group, r, g, b, 3);
    mesh_group_material_values_set(mesh, group, a, 4);
    mesh_segment_vertex_fill(mesh, group, 0, 16, (const float*)vertices,
                             (const uint32_t*)normals, (const uint32_t*)textCoords);
}

// FUNCTION: LEGORR 0x004712c0
Weapons_Model* weapons_get_weapons_model(Weapons_Object* obj)
{
    switch (obj->type) {
    case 1:
        return (Weapons_Model*)((char*)obj->vehicle + 0x138);
    case 4:
        return (Weapons_Model*)((char*)obj->building + 0xa0);
    }
    return NULL;
}

// FUNCTION: LEGORR 0x004712f0
float weapons_model_timer_get(Weapons_Object* obj, unsigned int slot)
{
    return weapons_get_weapons_model(obj)->timers[slot];
}

// FUNCTION: LEGORR 0x00471310
void weapons_model_timer_set(Weapons_Object* obj, float timer, unsigned int slot)
{
    weapons_get_weapons_model(obj)->timers[slot] = timer;
}

// FUNCTION: LEGORR 0x00471330
int weapons_z_within_bounds(Weapons_Vector3F* origin, Weapons_Vector3F* dir,
                            float height, float top, float dist)
{
    float z = dir->z * dist + origin->z;
    if (z <= top && z >= top - height) return 1;
    return 0;
}

// FUNCTION: LEGORR 0x00471370
static int weapons_callback_ray_candidate(Weapons_Object* obj,
                                          Weapons_RaySearch* search)
{
    float origin[2];
    float dir[2];
    float dirLen;
    float objPos[2];
    float proj[2];
    float ground;
    float dist;
    float rx;
    float ry;

    origin[0] = search->origin->x;
    origin[1] = search->origin->y;
    dir[0] = search->direction->x;
    dir[1] = search->direction->y;
    dirLen = (float)sqrt(dir[0] * dir[0] + dir[1] * dir[1]);
    if (obj != search->ignoreObject && (dirLen <= -0.00001f || dirLen >= 0.00001f)) {
        legoobject_transform_position_get(WEAPONS_LOBJ(obj), &objPos[0], &objPos[1]);
        ground = terrain_map_world_z_get(WEAPONS_LEGO_GET_MAP(), objPos[0], objPos[1]);
        point2d_project_onto_direction(origin, dir, objPos, proj);
        if ((float)sqrt((proj[0] - objPos[0]) * (proj[0] - objPos[0]) +
                        (proj[1] - objPos[1]) * (proj[1] - objPos[1])) <=
            entity_stats_float_at6c(WEAPONS_ENT(obj))) {
            rx = proj[0] - origin[0];
            ry = proj[1] - origin[1];
            dist = (float)sqrt(rx * rx + ry * ry);
            if (dist > -0.00001f && dist < 0.00001f) {
                if (weapons_z_within_bounds(search->origin, search->direction,
                                            entity_stats_float_at74(WEAPONS_ENT(obj)), ground, 0.0f)) {
                    search->hitObject = obj;
                    search->hitDist = 0.0f;
                    search->hit = 1;
                    return 0;
                }
            }
            dist = dist / dirLen;
            if (dir[0] > -0.00001f && dir[0] < 0.00001f &&
                rx > -0.00001f && rx < 0.00001f) {
                if (!(dir[1] > -0.00001f && dir[1] < 0.00001f &&
                      ry > -0.00001f && ry < 0.00001f)) {
                    if (dir[1] / ry < 0.0f) dist *= -1.0f;
                }
                else {
                    return 0;
                }
            }
            else if (dir[0] / rx < 0.0f) {
                dist *= -1.0f;
            }
            if (dist > 0.0f && dist < search->hitDist) {
                if (weapons_z_within_bounds(search->origin, search->direction,
                                            entity_stats_float_at74(WEAPONS_ENT(obj)), ground, dist)) {
                    search->hitObject = obj;
                    search->hitDist = dist;
                    search->hit = 1;
                }
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00471630
void weapons_ray_block_sweep(Weapons_RaySearch* search)
{
    Weapons_Vector3F end;
    unsigned int startX;
    unsigned int startY;
    unsigned int endX;
    unsigned int endY;
    unsigned int x;
    unsigned int y;
    unsigned int tmp;
    float dist;

    WEAPONS_LEGO_GET_MAP();
    WEAPONS_LEGO_GET_MAP();
    end.x = search->direction->x + search->origin->x;
    end.y = search->origin->y + search->direction->y;
    end.z = search->origin->z + search->direction->z;
    if (!terrain_map_world_to_block(WEAPONS_LEGO_GET_MAP(), search->origin->x,
                                    search->origin->y, (int*)&startX, (int*)&startY)) {
        if (startX >= WEAPONS_MAP_WIDTH()) startX = WEAPONS_MAP_WIDTH() - 1;
        if (startY >= WEAPONS_MAP_HEIGHT()) startY = WEAPONS_MAP_HEIGHT() - 1;
    }
    if (!terrain_map_world_to_block(WEAPONS_LEGO_GET_MAP(), end.x, end.y,
                                    (int*)&endX, (int*)&endY)) {
        if (endX >= WEAPONS_MAP_WIDTH()) endX = WEAPONS_MAP_WIDTH() - 1;
        if (endY >= WEAPONS_MAP_HEIGHT()) endY = WEAPONS_MAP_HEIGHT() - 1;
    }
    if (startX > endX) {
        tmp = endX;
        endX = startX;
        startX = tmp;
    }
    if (startY > endY) {
        tmp = endY;
        endY = startY;
        startY = tmp;
    }
    for (x = startX; x <= endX; x++) {
        for (y = startY; y <= endY; y++) {
            if (terrain_map_block_ray_intersection(WEAPONS_LEGO_GET_MAP(), x, y,
                    (const TerrainMapVector3*)search->origin,
                    (const TerrainMapVector3*)search->direction,
                    (TerrainMapVector3*)&end)) {
                dist = weapons_vector3d_distance(&end, search->origin) /
                       weapons_vector3d_modulus(search->direction);
                if (dist < search->hitDist) {
                    search->hitDist = dist;
                    search->hit = 1;
                    search->hitObject = NULL;
                }
            }
        }
    }
}

// FUNCTION: LEGORR 0x00471860
int weapons_ray_trace_nearest(Weapons_Vector3F* origin, Weapons_Vector3F* dir,
                              Weapons_Object** hitObject, float* dist,
                              Weapons_Object* ignoreObject)
{
    Weapons_RaySearch search;
    search.hitObject = NULL;
    search.hitDist = *dist;
    search.origin = origin;
    search.direction = dir;
    search.hit = 0;
    search.ignoreObject = ignoreObject;
    legoobject_run_through_lists_skip_upgrade_parts(
        (Lego_ObjectCallback)weapons_callback_ray_candidate, &search);
    weapons_ray_block_sweep(&search);
    *dist = search.hitDist;
    *hitObject = search.hitObject;
    return search.hit;
}

// FUNCTION: LEGORR 0x004718d0
int weapons_object_pair_ray_hit(Weapons_Object* obj, Weapons_Object* target)
{
    float dist;
    Weapons_Vector3F fromPos;
    Weapons_Vector3F toPos;
    Weapons_Vector3F dir;
    Weapons_Object* hitObject;

    dist = 1.0f;
    weapons_object_center_get(obj, &fromPos);
    weapons_object_center_get(target, &toPos);
    weapons_vector3d_subtract(&dir, &toPos, &fromPos);
    if (weapons_ray_trace_nearest(&fromPos, &dir, &hitObject, &dist, obj) &&
        hitObject != target) return 1;
    return 0;
}

// FUNCTION: LEGORR 0x00471960
int weapons_see_through_walls_check(Weapons_Object* obj, Weapons_Object* target)
{
    Weapons_Vector3F fromPos;
    Weapons_Vector3F toPos;
    Weapons_Vector3F dir;
    Weapons_RaySearch search;

    search.hitObject = NULL;
    search.hitDist = 1.0f;
    search.origin = &fromPos;
    search.direction = &dir;
    search.hit = 0;
    search.ignoreObject = NULL;
    if (block_get_bool_path(weaponGlobs.config,
            string_path_join(s_engine_config_root, WEAPONS_LEGO_GET_LEVEL()->name,
                             // STRING: LEGORR 0x004aaaac
                             "SeeThroughWalls", NULL))) {
        return 0;
    }
    weapons_object_center_get(obj, &fromPos);
    weapons_object_center_get(target, &toPos);
    weapons_vector3d_subtract(&dir, &toPos, &fromPos);
    weapons_ray_block_sweep(&search);
    return search.hit;
}

// FUNCTION: LEGORR 0x00471a20
void* weapons_model_fire_null_get(Weapons_Model* model, unsigned int slot,
                                  unsigned int index)
{
    return model->fireNulls[slot][index];
}

// FUNCTION: LEGORR 0x00471a40
void* weapons_model_x_pivot_get(Weapons_Model* model, unsigned int slot)
{
    return model->xPivots[slot];
}

// FUNCTION: LEGORR 0x00471a50
void* weapons_model_y_pivot_get(Weapons_Model* model, unsigned int slot)
{
    return model->yPivots[slot];
}

// FUNCTION: LEGORR 0x00471a60
void weapons_model_aim(Weapons_Model* model, Weapons_Vector3F* target,
                       float elapsed, int setTarget, unsigned int slot)
{
    void* xPivot;
    void* yPivot;
    Weapons_Vector3F pos;
    Weapons_Vector3F dir;
    float len;

    if (setTarget) model->targets[slot] = *target;
    xPivot = weapons_model_x_pivot_get(model, slot);
    if (xPivot != NULL && elapsed > 0.00001f) {
        ddiscene_object_position_get(xPivot, NULL, (float*)&pos);
        elapsed = 5.0f / elapsed;
        weapons_vector3d_subtract(&dir, &model->targets[slot], &pos);
        len = 1.0f / weapons_vector3d_modulus(&dir);
        weapons_vector3d_scale(&model->fireDirs[slot], &model->fireDirs[slot], elapsed);
        model->fireDirs[slot].x = len * dir.x + model->fireDirs[slot].x;
        model->fireDirs[slot].y = len * dir.y + model->fireDirs[slot].y;
        model->fireDirs[slot].z = len * dir.z + model->fireDirs[slot].z;
        weapons_vector3d_normalise(&model->fireDirs[slot]);
        if (model->fireDirs[slot].z > model->pitchRange) {
            model->fireDirs[slot].z = model->pitchRange;
        }
        yPivot = weapons_model_y_pivot_get(model, slot);
        if (yPivot != NULL) {
            ddiscene_object_pose_method_dispatch(yPivot, NULL, model->fireDirs[slot].x,
                                                 model->fireDirs[slot].y, 0.0f,
                                                 0.0f, 0.0f, -1.0f);
        }
        ddiscene_object_pose_method_dispatch(xPivot, NULL, model->fireDirs[slot].x,
                                             model->fireDirs[slot].y,
                                             model->fireDirs[slot].z,
                                             0.0f, 0.0f, -1.0f);
    }
}

// FUNCTION: LEGORR 0x00471c70
void weapons_model_fire_dir_get(Weapons_Model* model, Weapons_Vector3F* dir,
                                unsigned int slot)
{
    *dir = model->fireDirs[slot];
}

// FUNCTION: LEGORR 0x00471ca0
int weapons_object_center_get(Weapons_Object* obj, Weapons_Vector3F* center)
{
    void* cont = entity_type_component_get(obj);
    ddiscene_object_position_get(cont, NULL, (float*)center);
    center->z -= entity_stats_float_at74(WEAPONS_ENT(obj)) * 0.5f;
    return 1;
}

// FUNCTION: LEGORR 0x00471ce0
int weapons_object_can_be_hit(Weapons_Object* obj)
{
    if ((entity_state_flags_get(WEAPONS_ENT(obj)) & 0x100) &&
        legoobject_shared_eligibility_check(WEAPONS_LOBJ(obj), obj->type != 4)) {
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00471d20
int weapons_model_fire_update(Weapons_Object* obj, float elapsed, unsigned int slot)
{
    Weapons_Object* lazerObj;
    Weapons_Object* taskObj;
    float discharge;
    void* fireNull;
    int weaponType;
    float range;
    unsigned int i;
    float dir2D[2];
    Weapons_Vector3F target;
    Weapons_Vector3F dir;
    Weapons_Vector3F fromPos;

    lego_global_triple_copy_out((uint32_t*)&target);
    weapons_model_aim(weapons_get_weapons_model(obj), &target, elapsed, 1, slot);
    if (renderer_word_c_get() && (s_panel_highlight_flags & 0x400) &&
        weapons_model_timer_get(obj, slot) <= 0.0f) {
        if (legoobject_pick_at_screen_position(renderer_word_a_get(), renderer_word_b_get(),
                                              (Lego_Object**)&taskObj) &&
            taskObj == obj) {
            return 1;
        }
        for (i = 0; i < 2; i++) {
            if (Lego_GetCrystalCount(1)) {
                fireNull = weapons_model_fire_null_get(weapons_get_weapons_model(obj), slot, i);
                if (fireNull == NULL) return 1;
                weaponType = object_type_name_index_find(
                    weapons_get_weapons_model(obj)->weaponNames[slot]->name);
                range = weapon_type_range_get_target(weaponType);
                discharge = entity_speed_component_get(WEAPONS_ENT(obj));
                if (discharge == 0.0f) discharge = 1.0f;
                if (i == 0) {
                    weapons_model_timer_set(obj, weapon_type_recharge_time_get_target(weaponType), slot);
                    weapons_model_fire_dir_get(weapons_get_weapons_model(obj), &dir, slot);
                    media_action_feedback_sound_dispatch(0);
                }
                ddiscene_object_position_get(fireNull, NULL, (float*)&fromPos);
                lazerObj = weapons_fire_lazer(&fromPos, &dir, obj, elapsed, range,
                                              weaponType, discharge);
                if (lazerObj != NULL) {
                    dir2D[0] = dir.x;
                    dir2D[1] = dir.y;
                    weapons_generic_damage_object(lazerObj, weaponType, 1, 0.0f, dir2D);
                    obj->lazerHitObject = lazerObj;
                }
            }
        }
    }
    return 1;
}

// FUNCTION: LEGORR 0x00471f00
int weapons_model_aim_update(Weapons_Object* obj, float elapsed, unsigned int slot)
{
    void* cont;
    Weapons_Vector3F target;
    Weapons_Vector3F pos;
    Weapons_Vector3F up;
    Weapons_Vector3F dir;

    cont = entity_type_component_get(obj);
    if (cont != NULL) {
        ddiscene_object_facing_pair_get(cont, NULL, (float*)&dir, (float*)&up);
        ddiscene_object_position_get(cont, NULL, (float*)&pos);
        dir.x *= 10000.0f;
        dir.y *= 10000.0f;
        dir.z *= 10000.0f;
        target.x = dir.x + pos.x;
        target.y = dir.y + pos.y;
        target.z = dir.z + pos.z;
        weapons_model_aim(weapons_get_weapons_model(obj), &target, elapsed, 1, slot);
    }
    return 1;
}

// FUNCTION: LEGORR 0x00471fc0
int weapons_object_update_weapons(Weapons_Object* obj, float elapsed)
{
    unsigned int slot;
    if (obj->flags4 & 1) {
        for (slot = 0; slot < weapons_get_weapons_model(obj)->weaponCount; slot++) {
            weapons_model_fire_update(obj, elapsed, slot);
        }
    }
    else {
        for (slot = 0; slot < weapons_get_weapons_model(obj)->weaponCount; slot++) {
            weapons_model_aim_update(obj, elapsed, slot);
        }
    }
    return 1;
}

// FUNCTION: LEGORR 0x00472080
int weapons_object_in_range(Weapons_Object* obj, Weapons_Object* target)
{
    int weaponType;
    float range;
    Weapons_Vector3F objPos;
    Weapons_Vector3F targetPos;

    weaponType = legoobject_can_do_activity_classify(WEAPONS_LOBJ(obj));
    if (obj != NULL && target != NULL && weaponType != 0) {
        range = weapon_type_range_get_target(weaponType);
        weapons_object_center_get(obj, &objPos);
        weapons_object_center_get(target, &targetPos);
        if (weapons_vector3d_distance(&targetPos, &objPos) < range) return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00472130
int weapons_object_in_touch_range(Weapons_Object* obj, Weapons_Object* target)
{
    int weaponType;
    float range;
    Weapons_Vector3F objPos;
    Weapons_Vector3F targetPos;

    weaponType = legoobject_can_do_activity_classify(WEAPONS_LOBJ(obj));
    if (obj != NULL && target != NULL && weaponType != 0) {
        range = entity_collision_radius_get(WEAPONS_ENT(obj));
        weapons_object_center_get(obj, &objPos);
        weapons_object_center_get(target, &targetPos);
        if (weapons_vector3d_distance(&targetPos, &objPos) < range) return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Port-only: the port's weapon loader shares this unit's table.       */
/* ------------------------------------------------------------------ */

// PORT-ONLY: the port's loader (weapons_port.cpp) publishes its parsed table here, so the shared getters and object_type_name_index_find serve native callers
void weapons_port_type_table_set(unsigned int count, char** names, void* stats)
{
    weaponGlobs.weaponCount = count;
    weaponGlobs.weaponNameList = names;
    weaponGlobs.weaponStatsList = (Weapons_Stats*)stats;
}

// PORT-ONLY: the port's s_weapon_indexed_records is this unit's weaponStatsList (weapon.cpp, weapons_port.cpp)
void** weapons_port_stats_slot(void)
{
    return (void**)&weaponGlobs.weaponStatsList;
}
