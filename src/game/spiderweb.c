// SPDX-License-Identifier: LGPL-3.0-or-later
/* SpiderWeb.c, original translation unit GAME-042 (finding 505), rebuilt in
 * original shape (ADR 0005; finding 620): text 004661c0-004667e0, the
 * literal 0049e780 and the common block 005520e8-005520ec.
 *
 * The spider webs (spiderweb.h). SpiderWeb_SpawnAt spins a web across a
 * ground block whose two opposite neighbours are walls, and SpiderWeb_Add
 * records a web the object list placed. Every frame SpiderWeb_Update visits
 * each object: a web whose walls have gone is removed, a caught unit tears
 * at its web with the SpiderWeb weapon's damage until the web's health
 * falls below zero, and any other unit is tested against the web on its
 * block.
 *
 * The port's implementation stays in the port tail spiderweb_port.cpp,
 * unannotated, and native callers keep calling it. Lego.c (lego_unit.c)
 * calls this unit's functions; natively nothing reaches them (#552). */
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "spiderweb.h"
#include "spiderweb_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

// GLOBAL: LEGORR 0x005520e8
SpiderWeb_Globs spiderWebGlobs;

#define SPIDERWEB_BLOCK(bx, by) \
    spiderWebGlobs.webBlocks[spiderWebGlobs.level->width * (by) + (bx)]
#define SPIDERWEB_LEVEL_BLOCK(bx, by)                                  \
    LEGO_LEVEL_PTR(Lego_Block, spiderWebGlobs.level->blocks)            \
        [spiderWebGlobs.level->width * (by) + (bx)]

// FUNCTION: LEGORR 0x004661c0
void spiderweb_initialise(Lego_Level* level)
{
    spiderWebGlobs.webBlocks = (SpiderWeb_Block*)malloc(
        level->width * level->height * sizeof(SpiderWeb_Block));
    if (spiderWebGlobs.webBlocks != NULL)
        memset(spiderWebGlobs.webBlocks, 0,
               level->width * level->height * sizeof(SpiderWeb_Block));
    spiderWebGlobs.level = level;
}

// FUNCTION: LEGORR 0x00466210
void spiderweb_shutdown(void)
{
    if (spiderWebGlobs.webBlocks != NULL) {
        free(spiderWebGlobs.webBlocks);
        spiderWebGlobs.webBlocks = NULL;
    }
}

// FUNCTION: LEGORR 0x00466230
void spiderweb_restart(Lego_Level* level)
{
    spiderweb_shutdown();
    spiderweb_initialise(level);
}

// FUNCTION: LEGORR 0x00466250
int spiderweb_spawn_at(int bx, int by)
{
    TerrainMapPoint2 wPos;
    float theta;
    Lego_Object* webObj;

    if (!(SPIDERWEB_BLOCK(bx, by).flags & SPIDERWEB_FLAG_WEB)) {
        if (spiderweb_get_angle(bx, by, &theta)) {
            terrain_map_block_to_world(
                LEGO_LEVEL_PTR(TerrainMap, spiderWebGlobs.level->map), bx, by,
                &wPos.x, &wPos.y);
            webObj = legoobject_create_in_world(legoGlobs_contSpiderWeb, 0xc,
                                                0, 0, wPos.x, wPos.y, theta);
            spiderweb_add(bx, by, webObj);
            SPIDERWEB_BLOCK(bx, by).webObj->flags1 |= 0x100;
            scene_action_name_set(SPIDERWEB_BLOCK(bx, by).webObj->other,
                                  "Expand");
            ddiscene_frame_update_and_callbacks(
                SPIDERWEB_BLOCK(bx, by).webObj->other, 0.0f);
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00466380
void spiderweb_add(int bx, int by, Lego_Object* webObj)
{
    SPIDERWEB_BLOCK(bx, by).webObj = webObj;
    SPIDERWEB_BLOCK(bx, by).flags |= SPIDERWEB_FLAG_WEB;
    SPIDERWEB_BLOCK(bx, by).health = 100.0f;
}

/* A web spans a ground block between two walls: east and west (the web
 * faces north, theta = pi/2), else north and south (theta = 0). */
// FUNCTION: LEGORR 0x004663f0
int spiderweb_get_angle(int bx, int by, float* theta)
{
    Lego_Block* block = &SPIDERWEB_LEVEL_BLOCK(bx, by);

    if (block->flags1 & 0x8) {
        if ((block[1].flags1 & 0x10) && (block[-1].flags1 & 0x10)) {
            if (theta != NULL)
                *theta = 1.5707964f;
            return 1;
        }
        if ((SPIDERWEB_LEVEL_BLOCK(bx, by + 1).flags1 & 0x10) &&
            (SPIDERWEB_LEVEL_BLOCK(bx, by - 1).flags1 & 0x10)) {
            if (theta != NULL)
                *theta = 0.0f;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00466490
int spiderweb_check_collision(Lego_Object* liveObj)
{
    int bx, by;
    float wPos[3];
    float faceDir[2];
    float dir[3];

    if (legoobject_position_to_terrain_cell(liveObj, &bx, &by)) {
        if (SPIDERWEB_BLOCK(bx, by).flags & SPIDERWEB_FLAG_WEB) {
            terrain_map_block_to_world(
                LEGO_LEVEL_PTR(TerrainMap, spiderWebGlobs.level->map), bx, by,
                &wPos[0], &wPos[1]);
            legoobject_surface_vector_normalize(SPIDERWEB_BLOCK(bx, by).webObj,
                                                faceDir);
            dir[0] = faceDir[0];
            dir[1] = faceDir[1];
            wPos[2] = 0.0f;
            dir[2] = 0.0f;
            if (weapons_object_collision_check(liveObj, wPos, dir, 0)) {
                liveObj->flags1 |= 0x2000000;
                SPIDERWEB_BLOCK(bx, by).webObj->flags1 |= 0x2000000;
                return 1;
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x004665c0
void spiderweb_update(float elapsedGame)
{
    legoobject_run_through_lists_skip_upgrade_parts(
        (Lego_ObjectCallback)spiderweb_callback_update, &elapsedGame);
}

// FUNCTION: LEGORR 0x004665e0
int spiderweb_callback_update(Lego_Object* liveObj, float* elapsedGame)
{
    int bx, by;
    SpiderWeb_Block* block;

    if (liveObj->type == 0xc) {
        legoobject_position_to_terrain_cell(liveObj, &bx, &by);
        if ((SPIDERWEB_BLOCK(bx, by).flags & SPIDERWEB_FLAG_WEB) &&
            !spiderweb_get_angle(bx, by, NULL)) {
            spiderweb_remove_at(bx, by);
            return 0;
        }
    } else if (liveObj->flags1 & 0x2000000) {
        legoobject_position_to_terrain_cell(liveObj, &bx, &by);
        block = &SPIDERWEB_BLOCK(bx, by);
        if (block->flags & SPIDERWEB_FLAG_WEB) {
            SPIDERWEB_BLOCK(bx, by).health = block->health -
                weapons_get_damage_for_object(
                    object_type_name_index_find("SpiderWeb"), liveObj) *
                *elapsedGame;
            if (!(SPIDERWEB_BLOCK(bx, by).health < 0.0f))
                return 0;
            spiderweb_remove_at(bx, by);
        }
        liveObj->flags1 &= ~0x2000000;
        return 0;
    } else {
        spiderweb_check_collision(liveObj);
    }
    return 0;
}

// FUNCTION: LEGORR 0x00466750
void spiderweb_remove_at(int bx, int by)
{
    SPIDERWEB_BLOCK(bx, by).webObj->flags3 |= 0x800000;
    SPIDERWEB_BLOCK(bx, by).webObj->flags1 &= ~0x2000000;
    SPIDERWEB_BLOCK(bx, by).flags &= ~SPIDERWEB_FLAG_WEB;
}

#ifdef __cplusplus
}
#endif
