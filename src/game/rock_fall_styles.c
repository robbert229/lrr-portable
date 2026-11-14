// SPDX-License-Identifier: LGPL-3.0-or-later
/* The effects unit, original translation unit GAME-012 (finding 505),
 * rebuilt in original shape (ADR 0005; finding 589): text
 * 0040bdc0-0040cdc0, .rdata 0049e104-0049e10c, literals 004a1dcc-004a1e80,
 * .bss 004c7180-004c7df8.
 *
 * The unit runs the game's one-shot scene effects: the rock-fall styles
 * from Lego.cfg's RockFallStyles block (three types of four instances in
 * each of four styles), the boulder explosions and smash paths, the two
 * electric fence beams of 30 instances, the explosion of up to four
 * objects, and the ten MiscObjects effects (lazer, pusher and freezer hits,
 * path dust, four lava erosion smokes, the bird scarer and the upgrade
 * effect). Each effect clones a loaded container, plays it once and removes
 * or hides it when its animation ends.
 *
 * The port's implementation stays in the port tail rock_fall_styles_port.cpp,
 * unannotated. In the matching build rock_fall_styles_link_names.h gives 15
 * of this unit's functions the port's names, so every caller reaches them;
 * natively the port tail serves those names and nothing calls these. */
#include "rock_fall_styles_link_names.h"

#include <string.h>

#include "crt/vc5names.h"
#include "rock_fall_styles_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct Effect_Vector3F {
    float x, y, z;
} Effect_Vector3F;

typedef struct Effect_Point2F {
    float x, y;
} Effect_Point2F;

/* The start of the original's LegoObject (legoobject_unit.h's Lego_Object):
 * a boulder's model container is its `other` pointer. */
typedef struct Effect_Object {
    int type;                         /* +0x00 */
    int id;                           /* +0x04 */
    char* customName;                 /* +0x08 */
    void* vehicle;                    /* +0x0c */
    void* miniFigure;                 /* +0x10 */
    void* rockMonster;                /* +0x14 */
    void* building;                   /* +0x18 */
    void* other;                      /* +0x1c */
} Effect_Object;

/* Lego_Level's BoulderAnimation (lego_unit.h's f_274): 2 is the ice set. */
typedef struct Effect_Level {
    unsigned char unknown_000[0x274];
    int boulderAnimation;             /* +0x274 */
} Effect_Level;

#define EFFECT_ROCKFALL_MAXSTYLES 4
#define EFFECT_ROCKFALL_NUMTYPES 3
#define EFFECT_ROCKFALL_MAXINSTANCES 4
#define EFFECT_FENCEBEAM_NUMTYPES 2
#define EFFECT_FENCEBEAM_MAXINSTANCES 30
#define EFFECT_MISC_NUMTYPES 10
#define EFFECT_MISC_MAXINSTANCES 10

/* One rock-fall type of one style (0xa4 bytes). */
typedef struct Effect_RockFall {
    void* cont;                                     /* +0x00 loaded, hidden */
    void* conts[EFFECT_ROCKFALL_MAXINSTANCES];      /* +0x04 clones */
    unsigned int blockX[EFFECT_ROCKFALL_MAXINSTANCES]; /* +0x14 */
    unsigned int blockY[EFFECT_ROCKFALL_MAXINSTANCES]; /* +0x24 */
    int finished[EFFECT_ROCKFALL_MAXINSTANCES];     /* +0x34 */
    const char* partName;                           /* +0x44 */
    unsigned int partCount;                         /* +0x48 */
    unsigned int unknown_4c[18];                    /* +0x4c */
    unsigned int partsUsed[EFFECT_ROCKFALL_MAXINSTANCES]; /* +0x94 bit per part */
} Effect_RockFall;

/* One electric fence beam type (0xf4 bytes). */
typedef struct Effect_FenceBeam {
    void* cont;                                     /* +0x00 */
    void* conts[EFFECT_FENCEBEAM_MAXINSTANCES];     /* +0x04 */
    int finished[EFFECT_FENCEBEAM_MAXINSTANCES];    /* +0x7c */
} Effect_FenceBeam;

/* One MiscObjects effect (0x38 bytes). */
typedef struct Effect_Misc {
    void* conts[EFFECT_MISC_MAXINSTANCES];          /* +0x00 */
    unsigned int count;                             /* +0x28 */
    float timer;                                    /* +0x2c the bird scarer's */
    void* cont;                                     /* +0x30 loaded, hidden */
    unsigned int unknown_34;                        /* +0x34 */
} Effect_Misc;

typedef struct Effect_Globs {
    unsigned int rockFallCompletedTypes[12];        /* 004c7180 */
    unsigned int rockFallCompletedIndexes[12];      /* 004c71b0 */
    Effect_RockFall rockFallEffects[EFFECT_ROCKFALL_MAXSTYLES][EFFECT_ROCKFALL_NUMTYPES]; /* 004c71e0 */
    char* rockFallStyleNames[EFFECT_ROCKFALL_MAXSTYLES]; /* 004c7990 */
    unsigned int rockFallStyleCount;                /* 004c79a0 */
    unsigned int rockFallStyleIndex;                /* 004c79a4 */
    Effect_Misc miscEffects[EFFECT_MISC_NUMTYPES];  /* 004c79a8 */
    void* boulderExplodeConts[4];                   /* 004c7bd8 */
    void* smashPathConts[4];                        /* 004c7be8 */
    Effect_FenceBeam fenceBeams[EFFECT_FENCEBEAM_NUMTYPES]; /* 004c7bf8 */
    void* explosionCont;                            /* 004c7de0 */
    void* explosionConts[4];                        /* 004c7de4 */
    int explosionCount;                             /* 004c7df4 */
} Effect_Globs;

// GLOBAL: LEGORR 0x004c7180
static Effect_Globs effectGlobs;

void effect_remove_all(void);
void effect_spawn_boulder_explode(Effect_Object* obj);
void effect_spawn_boulder_explode_at(Effect_Vector3F* wPos);
void effect_spawn_smash_path(Effect_Object* obj, Effect_Vector3F* wPos);
int effect_get_rock_fall_style(const char* name, unsigned int* style);
void effect_set_rock_fall_style(unsigned int style);
void effect_load_rock_fall_styles_all(BlockFile* config, const char* gameName,
                                      void* root);
int effect_load_rock_fall_style(void* root, const char* filename,
                                unsigned int style, unsigned int type,
                                const char* partName);
int effect_load_electric_fence_beam(void* root, const char* filename,
                                    unsigned int type);
int effect_spawn_rock_fall(unsigned int type, unsigned int bx, unsigned int by,
                           float xPos, float yPos, float zPos,
                           float xDir, float yDir);
int effect_spawn_electric_fence_beam(unsigned int type, float xPos, float yPos,
                                     float zPos, float xDir, float yDir,
                                     float zDir);
unsigned int effect_update_all(float elapsed, unsigned int** completeTypes,
                               unsigned int** completeIndexes);
void effect_update_boulder_explode(float elapsed);
void effect_update_smash_path(float elapsed);
void effect_get_block_pos_rock_fall(unsigned int type, unsigned int index,
                                    unsigned int* bx, unsigned int* by);
void* effect_find_rock_fall_part(unsigned int bx, unsigned int by);
void effect_remove_all_boulder_explode(void);
void effect_remove_all_rock_falls(void);
void effect_load_explosion(void* root, const char* filename);
void effect_spawn_explosion(Effect_Object* obj, Effect_Point2F* pos);
void effect_update_explosions(float elapsed);
Effect_Misc* effect_get_misc_effect_data(unsigned int type);
int effect_load_misc_effect(Effect_Misc* misc, void* root,
                            const char* filename);
void effect_load_misc_objects(BlockFile* config, const char* gameName,
                              void* root);
void effect_update_misc_effect(Effect_Misc* misc, float elapsed);
void effect_update_misc_effects_all(float elapsed);
int effect_spawn_particle(unsigned int type, Effect_Vector3F* wPos,
                          Effect_Vector3F* dir);

// FUNCTION: LEGORR 0x0040bdc0
void effect_remove_all(void)
{
    unsigned int* dummy;

    effect_update_all(10000.0f, &dummy, &dummy);
}

// FUNCTION: LEGORR 0x0040bde0
void effect_spawn_boulder_explode(Effect_Object* obj)
{
    Effect_Vector3F wPos;

    ddiscene_object_position_get(obj->other, NULL, &wPos.x);
    effect_spawn_boulder_explode_at(&wPos);
}

/* legoGlobs' fields (lego_globs.h, finding 653), included before their
 * first use so that the functions above keep their symbol counter
 * (finding 538). */
#include "lego_globs.h"

// FUNCTION: LEGORR 0x0040be10
void effect_spawn_boulder_explode_at(Effect_Vector3F* wPos)
{
    unsigned int i;

    for (i = 0; i < 4; i++) {
        if (effectGlobs.boulderExplodeConts[i] == NULL) {
            if (((Effect_Level*)level_current_owner_get())->boulderAnimation == 2)
                effectGlobs.boulderExplodeConts[i] = entity_visual_resource_load(legoGlobs_contBoulderExplodeIce);
            else
                effectGlobs.boulderExplodeConts[i] = entity_visual_resource_load(legoGlobs_contBoulderExplode);
            ddiscene_frame_update_and_callbacks(effectGlobs.boulderExplodeConts[i], 0.0f);
            ddiscene_object_position_set(effectGlobs.boulderExplodeConts[i], NULL,
                                         wPos->x, wPos->y, wPos->z);
            ddiscene_object_pose_method_dispatch(effectGlobs.boulderExplodeConts[i], NULL,
                                                 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f);
            return;
        }
    }
}

// FUNCTION: LEGORR 0x0040beb0
void effect_spawn_smash_path(Effect_Object* obj, Effect_Vector3F* pos)
{
    Effect_Vector3F wPos;
    unsigned int i;

    if (obj != NULL) {
        void* cont = entity_type_component_get(obj);
        ddiscene_object_position_get(cont, NULL, &wPos.x);
    } else {
        wPos = *pos;
    }
    for (i = 0; i < 4; i++) {
        if (effectGlobs.smashPathConts[i] == NULL) {
            effectGlobs.smashPathConts[i] = entity_visual_resource_load(legoGlobs_contSmashPath);
            ddiscene_frame_update_and_callbacks(effectGlobs.smashPathConts[i], 0.0f);
            ddiscene_object_position_set(effectGlobs.smashPathConts[i], NULL,
                                         wPos.x, wPos.y, wPos.z);
            ddiscene_object_pose_method_dispatch(effectGlobs.smashPathConts[i], NULL,
                                                 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f);
        }
    }
}

// FUNCTION: LEGORR 0x0040bf70
int effect_get_rock_fall_style(const char* name, unsigned int* style)
{
    unsigned int i;

    for (i = 0; i < effectGlobs.rockFallStyleCount; i++) {
        if (stricmp(effectGlobs.rockFallStyleNames[i], name) == 0) {
            *style = i;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040bfc0
void effect_set_rock_fall_style(unsigned int style)
{
    effectGlobs.rockFallStyleIndex = style;
}

// FUNCTION: LEGORR 0x0040bfd0
void effect_load_rock_fall_styles_all(BlockFile* config, const char* gameName,
                                      void* root)
{
    BlockRec* item;
    char* stringParts[10];
    char str[1024];
    unsigned int count;
    unsigned short i;

    // STRING: LEGORR 0x004a1dcc
    for (item = block_subgroup(config, string_path_join(gameName, "RockFallStyles", 0));
         item != NULL; item = block_next_equal_key(item)) {
        effectGlobs.rockFallStyleNames[effectGlobs.rockFallStyleCount] = string_duplicate(item->key);
        strcpy(str, item->value);
        /* "," (004a13e4) is pooled with GAME-001; advisor.c marks it. */
        count = sound_variant_split(str, stringParts, ",");
        for (i = 1; i < count; i++) {
            effect_load_rock_fall_style(root, stringParts[i], effectGlobs.rockFallStyleCount,
                                        i - 1, stringParts[0]);
        }
        effectGlobs.rockFallStyleCount++;
    }
}

// FUNCTION: LEGORR 0x0040c0d0
int effect_load_rock_fall_style(void* root, const char* filename,
                                unsigned int style, unsigned int type,
                                const char* partName)
{
    Effect_RockFall* rockFall = &effectGlobs.rockFallEffects[style][type];
    const char* name;
    unsigned int i;

    /* "LWS" (004a13e0) is pooled with GAME-001; advisor.c marks it. */
    if ((rockFall->cont = container_load(root, filename, "LWS", 1)) == NULL) {
        // STRING: LEGORR 0x004a1ddc
        rockFall->cont = container_load(root, filename, "ANIM", 1);
    }
    if (rockFall->cont != NULL) {
        scene_render_lock_toggle(rockFall->cont, 1);
        for (i = 0; i < EFFECT_ROCKFALL_MAXINSTANCES; i++) {
            rockFall->conts[i] = entity_visual_resource_load(rockFall->cont);
            rockFall->finished[i] = 1;
            rockFall->partsUsed[i] = 0;
            scene_render_lock_toggle(rockFall->conts[i], 1);
        }
        name = xf_texture_name_format(rockFall->cont, partName, NULL);
        resource_name_register_or_query_target((void**)rockFall->cont, name, 1, &style);
        rockFall->partCount = style;
        rockFall->partName = partName;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040c1b0
int effect_load_electric_fence_beam(void* root, const char* filename,
                                    unsigned int type)
{
    Effect_FenceBeam* beam = &effectGlobs.fenceBeams[type];
    unsigned int i;

    if ((beam->cont = container_load(root, filename, "LWS", 1)) != NULL) {
        scene_render_lock_toggle(beam->cont, 1);
        for (i = 0; i < EFFECT_FENCEBEAM_MAXINSTANCES; i++) {
            beam->conts[i] = entity_visual_resource_load(beam->cont);
            beam->finished[i] = 1;
            scene_render_lock_toggle(beam->conts[i], 1);
        }
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040c230
int effect_spawn_rock_fall(unsigned int type, unsigned int bx, unsigned int by,
                           float xPos, float yPos, float zPos,
                           float xDir, float yDir)
{
    Effect_RockFall* rockFall = &effectGlobs.rockFallEffects[effectGlobs.rockFallStyleIndex][type];
    unsigned int i;

    for (i = 0; i < EFFECT_ROCKFALL_MAXINSTANCES; i++) {
        if (rockFall->finished[i])
            break;
    }
    if (i < EFFECT_ROCKFALL_MAXINSTANCES) {
        scene_render_lock_toggle(rockFall->conts[i], 0);
        ddiscene_frame_update_and_callbacks(rockFall->conts[i], 0.0f);
        ddiscene_object_position_set(rockFall->conts[i], NULL, xPos, yPos, zPos);
        ddiscene_object_pose_method_dispatch(rockFall->conts[i], NULL, xDir, yDir, 0.0f,
                                             0.0f, 0.0f, -1.0f);
        rockFall->blockX[i] = bx;
        rockFall->blockY[i] = by;
        rockFall->finished[i] = 0;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040c2f0
int effect_spawn_electric_fence_beam(unsigned int type, float xPos, float yPos,
                                     float zPos, float xDir, float yDir,
                                     float zDir)
{
    Effect_FenceBeam* beam = &effectGlobs.fenceBeams[type];
    unsigned int i;

    for (i = 0; i < EFFECT_FENCEBEAM_MAXINSTANCES; i++) {
        if (beam->finished[i])
            break;
    }
    if (i < EFFECT_FENCEBEAM_MAXINSTANCES) {
        scene_render_lock_toggle(beam->conts[i], 0);
        ddiscene_frame_update_and_callbacks(beam->conts[i], 0.0f);
        ddiscene_object_position_set(beam->conts[i], NULL, xPos, yPos, zPos);
        ddiscene_object_pose_method_dispatch(beam->conts[i], NULL, xDir, yDir, zDir,
                                             0.0f, 0.0f, -1.0f);
        beam->finished[i] = 0;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040c3a0
unsigned int effect_update_all(float elapsed, unsigned int** completeTypes,
                               unsigned int** completeIndexes)
{
    unsigned int count = 0;
    unsigned int i, j;
    Effect_RockFall* rockFall;
    Effect_FenceBeam* beam;

    effect_update_explosions(elapsed);
    effect_update_boulder_explode(elapsed);
    effect_update_smash_path(elapsed);
    effect_update_misc_effects_all(elapsed);

    for (i = 0; i < EFFECT_ROCKFALL_NUMTYPES; i++) {
        rockFall = &effectGlobs.rockFallEffects[effectGlobs.rockFallStyleIndex][i];
        if (rockFall->cont != NULL) {
            for (j = 0; j < EFFECT_ROCKFALL_MAXINSTANCES; j++) {
                if (!rockFall->finished[j]) {
                    if (ddiscene_advance_by_time_target(rockFall->conts[j], elapsed) > 0.0f) {
                        scene_render_lock_toggle(rockFall->conts[j], 1);
                        rockFall->finished[j] = 1;
                        rockFall->partsUsed[j] = 0;
                        effectGlobs.rockFallCompletedTypes[count] = i;
                        effectGlobs.rockFallCompletedIndexes[count] = j;
                        count++;
                    }
                }
            }
        }
    }
    *completeTypes = effectGlobs.rockFallCompletedTypes;
    *completeIndexes = effectGlobs.rockFallCompletedIndexes;

    for (beam = effectGlobs.fenceBeams; beam < &effectGlobs.fenceBeams[EFFECT_FENCEBEAM_NUMTYPES]; beam++) {
        for (j = 0; j < EFFECT_FENCEBEAM_MAXINSTANCES; j++) {
            if (!beam->finished[j]) {
                if (ddiscene_advance_by_time_target(beam->conts[j], elapsed) > 0.0f) {
                    scene_render_lock_toggle(beam->conts[j], 1);
                    beam->finished[j] = 1;
                }
            }
        }
    }
    return count;
}

// FUNCTION: LEGORR 0x0040c4d0
void effect_update_boulder_explode(float elapsed)
{
    unsigned int i;

    for (i = 0; i < 4; i++) {
        if (effectGlobs.boulderExplodeConts[i] != NULL) {
            if (ddiscene_advance_by_time_target(effectGlobs.boulderExplodeConts[i], elapsed) > 0.0f) {
                ledger_stub_00472a40_DDiSceneObjectDestroy(effectGlobs.boulderExplodeConts[i]);
                effectGlobs.boulderExplodeConts[i] = NULL;
            }
        }
    }
}

// FUNCTION: LEGORR 0x0040c520
void effect_update_smash_path(float elapsed)
{
    unsigned int i;

    for (i = 0; i < 4; i++) {
        if (effectGlobs.smashPathConts[i] != NULL) {
            if (ddiscene_advance_by_time_target(effectGlobs.smashPathConts[i], elapsed) > 0.0f) {
                ledger_stub_00472a40_DDiSceneObjectDestroy(effectGlobs.smashPathConts[i]);
                effectGlobs.smashPathConts[i] = NULL;
            }
        }
    }
}

// FUNCTION: LEGORR 0x0040c570
void effect_get_block_pos_rock_fall(unsigned int type, unsigned int index,
                                    unsigned int* bx, unsigned int* by)
{
    Effect_RockFall* rockFall = &effectGlobs.rockFallEffects[effectGlobs.rockFallStyleIndex][type];

    *bx = rockFall->blockX[index];
    *by = rockFall->blockY[index];
}

/* The part counter reuses `by`, whose address goes to Container_
 * FormatPartName, as the original's stack shows. */
// FUNCTION: LEGORR 0x0040c5b0
void* effect_find_rock_fall_part(unsigned int bx, unsigned int by)
{
    Effect_RockFall* rockFall;
    const char* name;
    unsigned int i, j;

    for (i = 0; i < EFFECT_ROCKFALL_NUMTYPES; i++) {
        rockFall = &effectGlobs.rockFallEffects[effectGlobs.rockFallStyleIndex][i];
        if (rockFall->cont != NULL) {
            for (j = 0; j < EFFECT_ROCKFALL_MAXINSTANCES; j++) {
                if (!rockFall->finished[j] && bx == rockFall->blockX[j] && by == rockFall->blockY[j]) {
                    for (by = 0; by < rockFall->partCount; by++) {
                        if (!(rockFall->partsUsed[j] & (1 << by)))
                            break;
                    }
                    if (by != rockFall->partCount) {
                        rockFall->partsUsed[j] |= 1 << by;
                        name = xf_texture_name_format(rockFall->conts[j], rockFall->partName,
                                                      (const int*)&by);
                        return resource_name_register_or_query_target((void**)rockFall->conts[j],
                                                                      name, 0, NULL);
                    }
                    return NULL;
                }
            }
        }
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0040c690
void effect_remove_all_boulder_explode(void)
{
    unsigned int i;

    for (i = 0; i < 4; i++) {
        if (effectGlobs.boulderExplodeConts[i] != NULL) {
            ledger_stub_00472a40_DDiSceneObjectDestroy(effectGlobs.boulderExplodeConts[i]);
            effectGlobs.boulderExplodeConts[i] = NULL;
        }
    }
}

// FUNCTION: LEGORR 0x0040c6c0
void effect_remove_all_rock_falls(void)
{
    Effect_RockFall* rockFall;
    unsigned int i, j;

    for (i = 0; i < EFFECT_ROCKFALL_NUMTYPES; i++) {
        rockFall = &effectGlobs.rockFallEffects[effectGlobs.rockFallStyleIndex][i];
        if (rockFall->cont != NULL) {
            for (j = 0; j < EFFECT_ROCKFALL_MAXINSTANCES; j++) {
                ledger_stub_00472a40_DDiSceneObjectDestroy(rockFall->conts[j]);
                rockFall->conts[j] = NULL;
            }
        }
    }
}

// FUNCTION: LEGORR 0x0040c720
void effect_load_explosion(void* root, const char* filename)
{
    effectGlobs.explosionCont = container_load(root, filename, "LWS", 1);
    scene_render_lock_toggle(effectGlobs.explosionCont, 1);
}

// FUNCTION: LEGORR 0x0040c750
void effect_spawn_explosion(Effect_Object* obj, Effect_Point2F* pos)
{
    Effect_Vector3F wPos;
    void* cont;

    if (obj != NULL) {
        legoobject_transform_position_get((struct Lego_Object*)obj, &wPos.x, &wPos.y);
    } else {
        wPos.x = pos->x;
        wPos.y = pos->y;
    }
    wPos.z = terrain_map_world_z_get(EFFECT_LEGO_GET_MAP(), wPos.x, wPos.y) - 1.0f;
    if (effectGlobs.explosionCount < sizeof(effectGlobs.explosionConts) / sizeof(effectGlobs.explosionConts[0])) {
        effectGlobs.explosionConts[effectGlobs.explosionCount] = entity_visual_resource_load(effectGlobs.explosionCont);
        cont = effectGlobs.explosionConts[effectGlobs.explosionCount++];
        scene_render_lock_toggle(cont, 0);
        ddiscene_object_position_set(cont, NULL, wPos.x, wPos.y, wPos.z);
        ddiscene_object_pose_method_dispatch(cont, NULL, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f);
        ddiscene_frame_update_and_callbacks(cont, 0.0f);
    }
}

// FUNCTION: LEGORR 0x0040c830
void effect_update_explosions(float elapsed)
{
    void* cont;
    int i;

    for (i = 0; i < effectGlobs.explosionCount; i++) {
        cont = effectGlobs.explosionConts[i];
        if (ddiscene_advance_by_time_target(cont, elapsed) != 0.0f) {
            ledger_stub_00472a40_DDiSceneObjectDestroy(cont);
            effectGlobs.explosionConts[i--] = effectGlobs.explosionConts[--effectGlobs.explosionCount];
        }
    }
}

// FUNCTION: LEGORR 0x0040c8a0
Effect_Misc* effect_get_misc_effect_data(unsigned int type)
{
    switch (type) {
    case 0: return &effectGlobs.miscEffects[0];
    case 1: return &effectGlobs.miscEffects[1];
    case 2: return &effectGlobs.miscEffects[2];
    case 3: return &effectGlobs.miscEffects[3];
    case 4: return &effectGlobs.miscEffects[4];
    case 5: return &effectGlobs.miscEffects[5];
    case 6: return &effectGlobs.miscEffects[6];
    case 7: return &effectGlobs.miscEffects[7];
    case 8: return &effectGlobs.miscEffects[8];
    case 9: return &effectGlobs.miscEffects[9];
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0040c920
int effect_load_misc_effect(Effect_Misc* misc, void* root, const char* filename)
{
    if (filename == NULL)
        return 0;
    if ((misc->cont = container_load(root, filename, "LWS", 1)) == NULL) {
        // STRING: LEGORR 0x004a1d5c
        if ((misc->cont = container_load(root, filename, "LWO", 1)) == NULL)
            return 0;
    }
    scene_render_lock_toggle(misc->cont, 1);
    return 1;
}

// FUNCTION: LEGORR 0x0040c990
void effect_load_misc_objects(BlockFile* config, const char* gameName, void* root)
{
    effect_load_misc_effect(&effectGlobs.miscEffects[0], root,
        block_get_strptr(config, string_path_join(gameName,
            // STRING: LEGORR 0x004a1e74
            "MiscObjects",
            // STRING: LEGORR 0x004a1e80
            "LazerHit", 0)));
    effect_load_misc_effect(&effectGlobs.miscEffects[1], root,
        block_get_strptr(config, string_path_join(gameName, "MiscObjects",
            // STRING: LEGORR 0x004a1e68
            "PusherHit", 0)));
    effect_load_misc_effect(&effectGlobs.miscEffects[2], root,
        block_get_strptr(config, string_path_join(gameName, "MiscObjects",
            // STRING: LEGORR 0x004a1e5c
            "FreezerHit", 0)));
    effect_load_misc_effect(&effectGlobs.miscEffects[3], root,
        block_get_strptr(config, string_path_join(gameName, "MiscObjects",
            // STRING: LEGORR 0x004a1e50
            "PathDust", 0)));
    effect_load_misc_effect(&effectGlobs.miscEffects[4], root,
        block_get_strptr(config, string_path_join(gameName, "MiscObjects",
            // STRING: LEGORR 0x004a1e3c
            "LavaErosionSmoke1", 0)));
    effect_load_misc_effect(&effectGlobs.miscEffects[5], root,
        block_get_strptr(config, string_path_join(gameName, "MiscObjects",
            // STRING: LEGORR 0x004a1e28
            "LavaErosionSmoke2", 0)));
    effect_load_misc_effect(&effectGlobs.miscEffects[6], root,
        block_get_strptr(config, string_path_join(gameName, "MiscObjects",
            // STRING: LEGORR 0x004a1e14
            "LavaErosionSmoke3", 0)));
    effect_load_misc_effect(&effectGlobs.miscEffects[7], root,
        block_get_strptr(config, string_path_join(gameName, "MiscObjects",
            // STRING: LEGORR 0x004a1e00
            "LavaErosionSmoke4", 0)));
    effect_load_misc_effect(&effectGlobs.miscEffects[8], root,
        block_get_strptr(config, string_path_join(gameName, "MiscObjects",
            // STRING: LEGORR 0x004a1df4
            "BirdScarer", 0)));
    effect_load_misc_effect(&effectGlobs.miscEffects[9], root,
        block_get_strptr(config, string_path_join(gameName, "MiscObjects",
            // STRING: LEGORR 0x004a1de4
            "UpgradeEffect", 0)));
}

// FUNCTION: LEGORR 0x0040cb70
void effect_update_misc_effect(Effect_Misc* misc, float elapsed)
{
    Effect_Point2F pos2D;
    Effect_Vector3F wPos;
    void* cont;
    unsigned int i;

    for (i = 0; i < misc->count; i++) {
        cont = misc->conts[i];
        if (misc == &effectGlobs.miscEffects[8] && (misc->timer += elapsed) > 12.5f) {
            ddiscene_object_position_get(cont, NULL, &wPos.x);
            pos2D.x = wPos.x;
            pos2D.y = wPos.y;
            legoobject_spatial_callback_query_at_optional_coordinate(NULL, &pos2D.x,
                                                                     legoGlobs_BirdScarerRadius);
            misc->timer = 0.0f;
        }
        if (ddiscene_advance_by_time_target(cont, elapsed) != 0.0f) {
            ledger_stub_00472a40_DDiSceneObjectDestroy(cont);
            misc->count--;
            if (i == misc->count)
                misc->conts[i] = NULL;
            else
                misc->conts[i] = misc->conts[misc->count];
            i--;
        }
    }
}

// FUNCTION: LEGORR 0x0040cc40
void effect_update_misc_effects_all(float elapsed)
{
    effect_update_misc_effect(&effectGlobs.miscEffects[0], elapsed);
    effect_update_misc_effect(&effectGlobs.miscEffects[1], elapsed);
    effect_update_misc_effect(&effectGlobs.miscEffects[2], elapsed);
    effect_update_misc_effect(&effectGlobs.miscEffects[3], elapsed);
    effect_update_misc_effect(&effectGlobs.miscEffects[4], elapsed);
    effect_update_misc_effect(&effectGlobs.miscEffects[5], elapsed);
    effect_update_misc_effect(&effectGlobs.miscEffects[6], elapsed);
    effect_update_misc_effect(&effectGlobs.miscEffects[7], elapsed);
    effect_update_misc_effect(&effectGlobs.miscEffects[8], elapsed);
    effect_update_misc_effect(&effectGlobs.miscEffects[9], elapsed);
}

// FUNCTION: LEGORR 0x0040cce0
int effect_spawn_particle(unsigned int type, Effect_Vector3F* wPos,
                          Effect_Vector3F* dir)
{
    Effect_Misc* misc = effect_get_misc_effect_data(type);

    if (misc->count < EFFECT_MISC_MAXINSTANCES && misc != NULL && misc->cont != NULL) {
        misc->timer = 0.0f;
        misc->conts[misc->count] = entity_visual_resource_load(misc->cont);
        ddiscene_frame_update_and_callbacks(misc->conts[misc->count], 0.0f);
        ddiscene_object_position_set(misc->conts[misc->count], NULL, wPos->x, wPos->y, wPos->z);
        if (dir != NULL)
            ddiscene_object_pose_method_dispatch(misc->conts[misc->count], NULL,
                                                 dir->x, dir->y, dir->z, 0.0f, 0.0f, -1.0f);
        else
            ddiscene_object_pose_method_dispatch(misc->conts[misc->count], NULL,
                                                 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f);
        misc->count++;
        return 1;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
