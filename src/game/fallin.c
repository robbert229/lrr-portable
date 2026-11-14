// SPDX-License-Identifier: LGPL-3.0-or-later
/* Fallin.c, original translation unit GAME-051 (finding 599), rebuilt in
 * original shape (ADR 0005): text 0040f0e0-0040f6c0, .rdata
 * 0049e138-0049e13c, .data 004a1ee4, .bss 004d78c4.
 *
 * The unit runs landslides and cave-ins. Every 750 to 799 game ticks (30 to
 * 32 seconds) Fallin_Update tries up to 100 random blocks. A wall that can
 * fall in, and has an open, unreserved neighbour, collapses towards its open
 * sides: the rock-fall effect plays, the block is marked, and the landslide
 * message (6) is sent. A block that has fallen in as often as the level's
 * NumberOfLandSlidesTillCaveIn (Fallin_Initialise; Lego.c's default is 3)
 * caves in its eight neighbours too.
 *
 * The port's implementation stays in the port tail lava_erosion_port.cpp,
 * unannotated. In the matching build fallin_link_names.h gives each of this
 * unit's functions the port's name, so every caller reaches them; natively
 * the port tail serves those names and nothing calls these. */
#include "fallin_link_names.h"

#include "fallin.h"
#include "fallin_imports.h"

#include <stddef.h>
/* legoGlobs' fields (lego_globs.h, finding 653). lego_unit.h, included
 * above, pays for its symbols (finding 538). */
#include "lego_globs.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

/* Lego.c's block terrain types that never fall in: lava, type 7 and water
 * (lego_unit.c's water test reads 9). */
#define FALLIN_TERRAIN_LAVA 6
#define FALLIN_TERRAIN_07 7
#define FALLIN_TERRAIN_WATER 9

/* The two Lego_Block flags1 bits a landslide sets. */
#define FALLIN_BLOCK1_800 0x800u
#define FALLIN_BLOCK1_1000 0x1000u

/* NumberOfLandSlidesTillCaveIn: Lego_Block +0x24 counts a block's
 * landslides. */
// GLOBAL: LEGORR 0x004a1ee4
static unsigned int fallinCaveInLimit = 4;

// GLOBAL: LEGORR 0x004d78c4
static float fallinTimer;

#define FALLIN_BLOCK(level, bx, by) \
    (LEGO_LEVEL_PTR(Lego_Block, (level)->blocks)[(by) * (level)->width + (bx)])
#define FALLIN_MAP(level) LEGO_LEVEL_PTR(struct TerrainMap, (level)->map)

// FUNCTION: LEGORR 0x0040f0e0
void fallin_update(float elapsedGame)
{
    struct TerrainMap* map = FALLIN_LEGO_GET_MAP();
    FallinPoint2I blockPos;
    unsigned int i;

    fallinTimer -= elapsedGame;
    if (fallinTimer <= 0.0f) {
        for (i = 0; i < 100; i++) {
            blockPos.x = rng_thunk_draw15() % map->gridWidth;
            blockPos.y = rng_thunk_draw15() % map->gridHeight;
            if (fallin_try_generate_land_slide(&blockPos, 1)) {
                info_send(6, NULL, NULL, (struct InfoPoint2I*)&blockPos);
                break;
            }
        }
        fallinTimer = (float)(rng_thunk_draw15() % 50) - -750.0f;
    }
}

// FUNCTION: LEGORR 0x0040f190
int fallin_try_generate_land_slide(FallinPoint2I* blockPos, int allowCaveIn)
{
    unsigned int i;
    int valid = 1;
    unsigned int fallinDirection = 0;
    int openCount = 0;
    /* The original fetches the map and never reads it. */
    struct TerrainMap* map = FALLIN_LEGO_GET_MAP();
    FallinPoint2I DIRECTIONS[4] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
    FallinPoint2I adjPos;

    if (fallin_can_land_slide_at_block(blockPos)) {
        for (i = 0; i < 4; i++) {
            adjPos.x = blockPos->x + DIRECTIONS[i].x;
            adjPos.y = blockPos->y + DIRECTIONS[i].y;
            if (terrain_cell_walkable_flag_current(adjPos.x, adjPos.y)) {
                if (terrain_cell_state_equals_one_current(&adjPos.x) ||
                    terrain_cell_flag_20_bounded_current(adjPos.x, adjPos.y))
                    valid = 0;
                fallinDirection |= 1 << i;
                if (!terrain_cell_mask_c0_current(adjPos.x, adjPos.y))
                    openCount++;
            }
        }
        if (valid && fallinDirection && openCount) {
            fallin_generate_land_slide(blockPos, fallinDirection, allowCaveIn);
            return 1;
        }
    }
    return 0;
}
// FUNCTION: LEGORR 0x0040f2b0
int fallin_can_land_slide_at_block(FallinPoint2I* blockPos)
{
    Lego_Level* level = legoGlobs_currLevel;
    unsigned char terrain;

    if (terrain_cell_flag_8_bounded_current(blockPos->x, blockPos->y) &&
        !terrain_cell_flag_reserved_current(&blockPos->x)) {
        terrain = FALLIN_BLOCK(level, blockPos->x, blockPos->y).f_02;
        if (terrain != FALLIN_TERRAIN_LAVA && terrain != FALLIN_TERRAIN_WATER &&
            terrain != FALLIN_TERRAIN_07) {
            if (level->safeCaverns &&
                terrain_cell_edge_wall_current(&blockPos->x))
                return 0;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040f330
void fallin_generate_land_slide(FallinPoint2I* blockPos,
                                unsigned int fallinDirection, int allowCaveIn)
{
    FallinPoint2F DIRECTIONS[4] = {
        { 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, -1.0f }, { -1.0f, 0.0f }
    };
    unsigned int direction, type;
    float xPos, yPos;
    FallinVector3F wPos;
    FallinPoint2F* dir;

    while (!terrain_cell_state_update_current(&blockPos->x))
        ;
    /* A mask with no case (0, 15) leaves direction and type unset, as in
     * the original; Fallin_TryGenerateLandSlide never passes one. */
    terrain_cell_work_flag_clear_current(&blockPos->x, 0x2f);

    switch (fallinDirection) {
    case 3:
        direction = 1;
        type = 1;
        break;
    case 6:
        direction = 2;
        type = 1;
        break;
    case 12:
        direction = 3;
        type = 1;
        break;
    case 9:
        direction = 0;
        type = 1;
        break;
    case 7:
    case 8:
        direction = 3;
        type = 0;
        break;
    case 2:
    case 10:
    case 13:
        direction = 1;
        type = 0;
        break;
    case 1:
    case 14:
        direction = 0;
        type = 0;
        break;
    case 4:
    case 5:
    case 11:
        direction = 2;
        type = 0;
        break;
    }

    dir = &DIRECTIONS[direction];
    terrain_map_block_to_world(FALLIN_MAP(legoGlobs_currLevel), blockPos->x,
                               blockPos->y, &xPos, &yPos);
    if (scene_slot_pose_scale_register(
            type, blockPos->x, blockPos->y, xPos, yPos,
            terrain_map_world_z_get(FALLIN_MAP(legoGlobs_currLevel), xPos,
                                    yPos),
            dir->x, dir->y)) {
        wPos.x = xPos;
        wPos.y = yPos;
        wPos.z = terrain_map_world_z_get(FALLIN_MAP(legoGlobs_currLevel), xPos,
                                         yPos);
        FALLIN_BLOCK(legoGlobs_currLevel, blockPos->x, blockPos->y).flags1 |=
            FALLIN_BLOCK1_800;
        FALLIN_BLOCK(legoGlobs_currLevel, blockPos->x, blockPos->y).flags1 |=
            FALLIN_BLOCK1_1000;
        level_tile_mark_current_target(&blockPos->x, 1);
        sound_submission_route_wrapper(NULL, 0x28, 0, 0, &wPos.x);
    }

    if (allowCaveIn) {
        FALLIN_BLOCK(legoGlobs_currLevel, blockPos->x, blockPos->y).f_10[5]++;
        if (FALLIN_BLOCK(legoGlobs_currLevel, blockPos->x,
                         blockPos->y).f_10[5] >= fallinCaveInLimit) {
            fallin_generate_cave_in(blockPos, fallinDirection);
            info_send(6, NULL, NULL, (struct InfoPoint2I*)blockPos);
        }
    }

    lego_cell_init_pass(legoGlobs_currLevel, blockPos->x, blockPos->y, 0);
}
// FUNCTION: LEGORR 0x0040f5e0
void fallin_initialise(unsigned int caveInLimit)
{
    fallinCaveInLimit = caveInLimit;
}

// FUNCTION: LEGORR 0x0040f5f0
void fallin_generate_cave_in(FallinPoint2I* blockPos,
                             unsigned int fallinDirection)
{
    FallinPoint2I DIRECTIONS[8] = {
        { 0, 1 }, { 1, 1 }, { 1, 0 }, { 1, -1 },
        { 0, -1 }, { -1, -1 }, { -1, 0 }, { -1, 1 }
    };
    FallinPoint2I adjPos;
    unsigned int i;

    for (i = 0; i < 8; i++) {
        adjPos.x = blockPos->x + DIRECTIONS[i].x;
        adjPos.y = blockPos->y + DIRECTIONS[i].y;
        if (level_coord_pair_within_bounds_target(
                (const unsigned int*)FALLIN_MAP(legoGlobs_currLevel),
                adjPos.x, adjPos.y)) {
            if (!fallin_try_generate_land_slide(&adjPos, 0)) {
                if (fallin_can_land_slide_at_block(&adjPos))
                    fallin_generate_land_slide(&adjPos, fallinDirection, 0);
            }
        }
    }
}

#ifdef __cplusplus
}
#endif
