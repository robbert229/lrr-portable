// SPDX-License-Identifier: LGPL-3.0-or-later
/* Erode.c, original translation unit GAME-015 (finding 505), rebuilt in
 * original shape (ADR 0005; finding 599): text 0040e930-0040f0e0, .rdata
 * 0049e124-0049e130, .bss 004c7eb0-004d78c4.
 *
 * The unit runs lava erosion. Lego.c registers the EmergeMap's eroding
 * blocks (Erode_SetBlockErodeRate) and marks each lava block as an active
 * source (Erode_SetActiveBlock). Every ErodeTriggerTime, Erode_Update picks
 * a source at random and starts eroding the first ground block beside it
 * that is neither reserved nor locked. Each eroding block counts down
 * ErodeErodeTime per erosion level, plays the four lava smokes and turns to
 * lava at the end. A block that cannot erode yet is locked for
 * ErodeLockTime.
 *
 * The port's implementation stays in the port tail lava_erosion_port.cpp,
 * unannotated. In the matching build erode_link_names.h gives each of this
 * unit's functions the port's name, so every caller reaches them; natively
 * the port tail serves those names and nothing calls these. */
#include "erode_link_names.h"

#include <stdio.h>
#include <string.h>

#include "crt/vc5names.h"
#include "erode.h"
#include "erode_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

#define EROSION_MAXBLOCKS 2000
#define EROSION_MAXLOCKEDBLOCKS 1000

/* Lego.c's block terrain types. */
#define EROSION_TERRAIN_LAVA 6

/* Lego_Block flags1 and flags2 bits. */
#define EROSION_BLOCK1_FLOOR 0x8u
#define EROSION_BLOCK1_ERODEACTIVE 0x80000000u
#define EROSION_BLOCK2_ERODELOCKED 0x10u

typedef struct Erode_Globs {
    ErodePoint2I activeBlocks[EROSION_MAXBLOCKS];       /* 004c7eb0 */
    unsigned int activeCount;                           /* 004cbd30 */
    ErodePoint2I erodeBlocks[EROSION_MAXBLOCKS];        /* 004cbd34 */
    int erodeUsed[EROSION_MAXBLOCKS];                   /* 004cfbb4 */
    float erodeTimes[EROSION_MAXBLOCKS];                /* 004d1af4 */
    ErodePoint2I lockedBlocks[EROSION_MAXLOCKEDBLOCKS]; /* 004d3a34 */
    float lockedTimes[EROSION_MAXLOCKEDBLOCKS];         /* 004d5974 */
    int lockedUsed[EROSION_MAXLOCKEDBLOCKS];            /* 004d6914 */
    float elapsedTimer;                                 /* 004d78b4 */
    float triggerTime;                                  /* 004d78b8 */
    float erodeTime;                                    /* 004d78bc */
    float lockTime;                                     /* 004d78c0 */
} Erode_Globs;

// GLOBAL: LEGORR 0x004c7eb0
static Erode_Globs erodeGlobs;

#define EROSION_BLOCK(level, bx, by) \
    (LEGO_LEVEL_PTR(Lego_Block, (level)->blocks)[(level)->width * (by) + (bx)])
#define EROSION_MAP(level) LEGO_LEVEL_PTR(struct TerrainMap, (level)->map)

// FUNCTION: LEGORR 0x0040e930
void erode_initialise(float triggerTime, float erodeTime, float lockTime)
{
    erodeGlobs.triggerTime = triggerTime * 25.0f;
    erodeGlobs.erodeTime = (erodeTime * 25.0f) * 0.2f;
    erodeGlobs.lockTime = lockTime * 25.0f;
    erodeGlobs.activeCount = 0;
    memset(erodeGlobs.erodeUsed, 0, sizeof(erodeGlobs.erodeUsed));
    memset(erodeGlobs.lockedUsed, 0, sizeof(erodeGlobs.lockedUsed));
}

// FUNCTION: LEGORR 0x0040e990
int erode_get_free_activity(unsigned int* index)
{
    unsigned int i;

    for (i = 0; i < EROSION_MAXBLOCKS; i++) {
        if (!erodeGlobs.erodeUsed[i]) {
            *index = i;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040e9c0
float erode_get_block_erode_rate(ErodePoint2I* blockPos)
{
    Lego_Level* level = legoGlobs_currLevel;
    return (6 - LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos->y + blockPos->x].f_06) *
           erodeGlobs.erodeTime * 25.0f;
}
// FUNCTION: LEGORR 0x0040ea10
void erode_set_block_erode_rate(ErodePoint2I* blockPos, unsigned int erodeSpeed)
{
    Lego_Level* level = legoGlobs_currLevel;
    unsigned int index;

    if (erode_get_free_activity(&index)) {
        EROSION_BLOCK(level, blockPos->x, blockPos->y).f_07 =
            (unsigned char)erodeSpeed;
        erodeGlobs.erodeBlocks[index].x = blockPos->x;
        erodeGlobs.erodeBlocks[index].y = blockPos->y;
        erodeGlobs.erodeTimes[index] =
            erode_get_block_erode_rate(blockPos) / (erodeSpeed + 1);
        erodeGlobs.erodeUsed[index] = 1;
        erode_set_active_block(blockPos, 1);
    }
}

// FUNCTION: LEGORR 0x0040eab0
void erode_update(float elapsedGame)
{
    Lego_Level* level = legoGlobs_currLevel;
    int start = rng_thunk_draw15();
    unsigned int index, i;
    ErodePoint2I adjPos;
    ErodeVector3F wPos, dir;
    char before, after;

    erodeGlobs.elapsedTimer += elapsedGame;
    if (erodeGlobs.elapsedTimer > erodeGlobs.triggerTime) {
        if (erode_get_free_activity(&index)) {
            for (i = 0; i < erodeGlobs.activeCount; i++) {
                if (erode_find_adjacent_block(
                        &erodeGlobs.activeBlocks[(start + i) %
                                                 erodeGlobs.activeCount],
                        &adjPos)) {
                    if (!level_cell_occupied_at_current(&adjPos.x) ||
                        (EROSION_BLOCK(level, adjPos.x, adjPos.y).flags2 &
                         EROSION_BLOCK2_ERODELOCKED)) {
                        EROSION_BLOCK(level, adjPos.x, adjPos.y).flags2 &=
                            ~EROSION_BLOCK2_ERODELOCKED;
                        erodeGlobs.erodeBlocks[index] = adjPos;
                        erodeGlobs.erodeTimes[index] =
                            erode_get_block_erode_rate(
                                &erodeGlobs.erodeBlocks[index]);
                        erodeGlobs.erodeUsed[index] = 1;
                        EROSION_BLOCK(level, adjPos.x, adjPos.y).f_07 = 0;
                        erode_set_active_block(&adjPos, 1);
                        info_send(0x1e, NULL, NULL,
                                  (InfoPoint2I*)&adjPos);
                        break;
                    }
                    erode_add_locked_block(&adjPos);
                    EROSION_BLOCK(level, adjPos.x, adjPos.y).flags2 |=
                        EROSION_BLOCK2_ERODELOCKED;
                }
            }
        }
        erodeGlobs.elapsedTimer = 0.0f;
    }

    for (i = 0; i < EROSION_MAXBLOCKS; i++) {
        if (erodeGlobs.erodeUsed[i]) {
            ErodePoint2I* pos = &erodeGlobs.erodeBlocks[i];
            before = (char)((erodeGlobs.erodeTimes[i] /
                             erode_get_block_erode_rate(
                                 pos)) * 4.0f);
            erodeGlobs.erodeTimes[i] -= elapsedGame;
            after = (char)((erodeGlobs.erodeTimes[i] /
                            erode_get_block_erode_rate(
                                pos)) * 4.0f);
            if (after != before) {
                EROSION_BLOCK(level, pos->x,
                              pos->y).f_07 =
                    (unsigned char)(4 - after);
                terrain_map_block_to_world(EROSION_MAP(level),
                                           pos->x,
                                           pos->y,
                                           &wPos.x, &wPos.y);
                wPos.z = terrain_map_world_z_get(EROSION_MAP(level), wPos.x,
                                                 wPos.y);
                d3drm_vector_random(&dir.x);
                dir.z = 0.0f;
                switch (after) {
                case 0:
                    erodeGlobs.erodeUsed[i] = 0;
                    EROSION_BLOCK(level, pos->x,
                                  pos->y).f_02 =
                        EROSION_TERRAIN_LAVA;
                    lava_effect_select_and_place(4, &wPos.x, &dir.x);
                    break;
                case 1:
                    lava_effect_select_and_place(5, &wPos.x, &dir.x);
                    break;
                case 2:
                    lava_effect_select_and_place(6, &wPos.x, &dir.x);
                    break;
                case 3:
                    lava_effect_select_and_place(7, &wPos.x, &dir.x);
                    break;
                }
                lego_cell_init_pass(level, pos->x,
                                    pos->y, 0);
            }
        }
    }

    for (i = 0; i < EROSION_MAXLOCKEDBLOCKS; i++) {
        if (erodeGlobs.lockedUsed[i]) {
            erodeGlobs.lockedTimes[i] -= elapsedGame;
            if (erodeGlobs.lockedTimes[i] < 0.0f)
                erodeGlobs.lockedUsed[i] = 0;
        }
    }
}
// FUNCTION: LEGORR 0x0040ee00
void erode_add_locked_block(ErodePoint2I* blockPos)
{
    unsigned int i;

    for (i = 0; i < EROSION_MAXLOCKEDBLOCKS; i++) {
        if (!erodeGlobs.lockedUsed[i]) {
            erodeGlobs.lockedBlocks[i] = *blockPos;
            erodeGlobs.lockedTimes[i] = erodeGlobs.lockTime;
            erodeGlobs.lockedUsed[i] = 1;
            return;
        }
    }
}

// FUNCTION: LEGORR 0x0040ee50
void erode_set_active_block(ErodePoint2I* blockPos, int on)
{
    unsigned int i;
    ErodePoint2I* active;
    Lego_Level* level = legoGlobs_currLevel;
    if (on) {
        if (!(EROSION_BLOCK(level, blockPos->x, blockPos->y).flags1 &
              EROSION_BLOCK1_ERODEACTIVE) &&
            erodeGlobs.activeCount < EROSION_MAXBLOCKS) {
            erodeGlobs.activeBlocks[erodeGlobs.activeCount] = *blockPos;
            erodeGlobs.activeCount++;
            map_cell_tracking_flags_clear_and_refresh_current(&blockPos->x);
            EROSION_BLOCK(level, blockPos->x, blockPos->y).flags1 |=
                EROSION_BLOCK1_ERODEACTIVE;
        }
    } else {
        i = 0;
        if (i < erodeGlobs.activeCount) {
            active = erodeGlobs.activeBlocks;
            do {
                if (blockPos->x == active->x && blockPos->y == active->y) {
                    erodeGlobs.activeCount--;
                    erodeGlobs.activeBlocks[i] =
                        erodeGlobs.activeBlocks[erodeGlobs.activeCount];
                    EROSION_BLOCK(level, blockPos->x, blockPos->y).flags1 &=
                        ~EROSION_BLOCK1_ERODEACTIVE;
                    break;
                }
                i++;
                active++;
            } while (i < erodeGlobs.activeCount);
        }
        i = 0;
        active = erodeGlobs.erodeBlocks;
        do {
            if (blockPos->x == active->x && blockPos->y == active->y) {
                erodeGlobs.erodeUsed[i] = 0;
                break;
            }
            active++;
            i++;
        } while (active < &erodeGlobs.erodeBlocks[EROSION_MAXBLOCKS]);
        erode_add_locked_block(blockPos);
    }
}
// FUNCTION: LEGORR 0x0040efb0
int erode_is_block_locked(ErodePoint2I* blockPos)
{
    ErodePoint2I* locked = erodeGlobs.lockedBlocks;
    int* used = erodeGlobs.lockedUsed;

    while (!*used || blockPos->x != locked->x || blockPos->y != locked->y) {
        used++;
        locked++;
        if (used >= &erodeGlobs.lockedUsed[EROSION_MAXLOCKEDBLOCKS])
            return 0;
    }
    return 1;
}
// FUNCTION: LEGORR 0x0040f000
int erode_find_adjacent_block(ErodePoint2I* blockPos, ErodePoint2I* adjPos)
{
    ErodePoint2I DIRECTIONS[4] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
    Lego_Level* level = legoGlobs_currLevel;
    Lego_Block* block;
    unsigned int i;

    if (EROSION_BLOCK(level, blockPos->x, blockPos->y).f_02 !=
        EROSION_TERRAIN_LAVA)
        return 0;
    for (i = 0; i < 4; i++) {
        adjPos->x = blockPos->x + DIRECTIONS[i].x;
        adjPos->y = blockPos->y + DIRECTIONS[i].y;
        block = &EROSION_BLOCK(level, adjPos->x, adjPos->y);
        if (!(block->flags1 & EROSION_BLOCK1_ERODEACTIVE) &&
            (block->flags1 & EROSION_BLOCK1_FLOOR) && block->f_06) {
            if (!erode_is_block_locked(adjPos))
                return 1;
        }
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
