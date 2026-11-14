// SPDX-License-Identifier: LGPL-3.0-or-later
/* Construction.c, original translation unit GAME-052 (finding 603), rebuilt
 * in original shape (ADR 0005): text 00408c80-00409d40, .rdata
 * 0049e090-0049e09c, .bss 004b8a20-004b8a54.
 *
 * The unit keeps the level's construction zones: one Lego_ConstructionZone
 * per path or building that the player has laid out and that waits for its
 * resources. Construction_Zone_StartPath and Construction_Zone_StartBuilding
 * create a zone; the building's barriers and resources are requested from
 * AITask.c; each resource placed counts towards completion; and
 * Construction_UpdateAll, from Lego.c's world update, requests a path's
 * resources and completes a zone once everything has been placed and
 * nothing stands on its origin block. Construction_Zone_CompleteBuilding
 * creates the building and turns its shape's blocks into building floor.
 *
 * The port's implementation stays in the port tail placement_port.cpp,
 * unannotated. In the matching build construction_link_names.h gives twelve
 * of these functions the port's names, so every caller reaches them;
 * natively the port serves those names and nothing calls these. */
#include "construction_link_names.h"

#include <string.h>

#include "crt/vc5names.h"
#include "construction.h"
#include "construction_imports.h"
/* legoGlobs' fields (lego_globs.h, finding 653). lego_unit.h, included
 * above, pays for its symbols (finding 538). */
#include "lego_globs.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

/* LegoObject_Type values the unit tests. */
#define CONSTRUCTION_OBJECT_BUILDING 4
#define CONSTRUCTION_OBJECT_CRYSTAL 6
#define CONSTRUCTION_OBJECT_ORE 7
#define CONSTRUCTION_OBJECT_BARRIER 9
#define CONSTRUCTION_OBJECT_PATH 15

/* LegoObject flags3 0x800000: the object is being removed; flags4 0x100:
 * the object is a placed construction resource. */
#define CONSTRUCTION_OBJECT3_REMOVING 0x800000u
#define CONSTRUCTION_OBJECT4_PLACED 0x100u

/* Lego_Block flags1: 0x8000 the building's entrance block, 0x100000 the
 * block belongs to a construction zone. Lego_Block +0x10 is the zone. */
#define CONSTRUCTION_BLOCK1_ENTRANCE 0x8000u
#define CONSTRUCTION_BLOCK1_ZONE 0x100000u

typedef struct Construction_Globs {
    unsigned int nextHandle;                    /* +0x00 004b8a20 */
    Lego_ConstructionZone* constructList;       /* +0x04 004b8a24 */
    unsigned char reserved_08[0x14];            /* +0x08 004b8a28: no code reference */
    const char* buildingBaseName[5];            /* +0x1c 004b8a3c */
    unsigned int buildingBaseCount;             /* +0x30 004b8a50 */
} Construction_Globs;                           /* 0x34 bytes */

// GLOBAL: LEGORR 0x004b8a20
static Construction_Globs constructionGlobs;

#define CONSTRUCTION_BLOCK(level, bx, by) \
    (LEGO_LEVEL_PTR(Lego_Block, (level)->blocks)[(by) * (level)->width + (bx)])
#define CONSTRUCTION_BLOCK_ZONE(level, bx, by) \
    (*(Lego_ConstructionZone**)&CONSTRUCTION_BLOCK(level, bx, by).f_10[0])

// FUNCTION: LEGORR 0x00408c80
unsigned int construction_get_building_base(const char* name)
{
    unsigned int i;

    for (i = 0; i < constructionGlobs.buildingBaseCount; i++) {
        if (strcmp(constructionGlobs.buildingBaseName[i], name) == 0)
            return i;
    }
    return constructionGlobs.buildingBaseCount;
}

// FUNCTION: LEGORR 0x00408ce0
void construction_zone_place_resource(unsigned int handle,
                                      struct Lego_Object* placedObject)
{
    Lego_ConstructionZone* zone =
        construction_zone_find_by_handle_or_at_block(NULL, &handle);

    if (zone) {
        if (placedObject) {
            placedObject->flags4 |= CONSTRUCTION_OBJECT4_PLACED;
            zone->placedObjects[zone->placedObjectCount++] = placedObject;
            zone->placedCount++;
        }
        if (zone->requestCount == zone->placedCount) {
            zone->progress = 125.0f;
            zone->flags |= CONSTRUCTION_FLAG_READY;
        }
    } else {
        if (placedObject->type == CONSTRUCTION_OBJECT_CRYSTAL)
            cmd_produce(0x16, placedObject, 0, NULL);
        else
            cmd_produce(0x1c, placedObject, 0, NULL);
    }
}

// FUNCTION: LEGORR 0x00408d70
int construction_zone_needs_more_of_resource(unsigned int handle,
                                             int objType, int objID)
{
    Lego_ConstructionZone* zone =
        construction_zone_find_by_handle_or_at_block(NULL, &handle);
    unsigned int placed;
    unsigned int needed;
    int costType;

    if (zone) {
        placed = construction_zone_count_of_resource_placed(zone, objType, objID);
        costType = (zone->flags & CONSTRUCTION_FLAG_LAYPATH)
            ? CONSTRUCTION_OBJECT_PATH : CONSTRUCTION_OBJECT_BUILDING;
        if (objType == CONSTRUCTION_OBJECT_CRYSTAL)
            needed = entity_indexed_record_word_at_b4_get(costType, zone->objID, 0);
        else if (objType == CONSTRUCTION_OBJECT_ORE) {
            if (objID == 0)
                needed = entity_indexed_record_word_at_b0_get(costType, zone->objID, 0);
            else
                needed = stats_table_word_at_b8(costType, zone->objID, 0);
        } else
            return 1;
        if (placed < needed)
            return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00408e10
unsigned int construction_zone_count_of_resource_placed(
    Lego_ConstructionZone* zone, int objType, int objID)
{
    unsigned int count = 0;
    unsigned int i;

    for (i = 0; i < zone->placedCount; i++) {
        if (objType == zone->placedObjects[i]->type &&
            objID == zone->placedObjects[i]->id)
            count++;
    }
    return count;
}

// FUNCTION: LEGORR 0x00408e50
void construction_zone_request_resource(int* blockPos, int objType,
                                        int objID, unsigned int objLevel,
                                        unsigned int count)
{
    Lego_ConstructionZone* zone =
        construction_zone_find_by_handle_or_at_block(blockPos, NULL);
    unsigned int i;

    if (objType == CONSTRUCTION_OBJECT_ORE && objID == 1)
        zone->flags |= CONSTRUCTION_FLAG_USESTUDS;
    for (i = 0; i < count; i++) {
        zone->requestCount++;
        aitask_cmd_command_kind_record_issue(objType, objID, objLevel, blockPos,
                                             zone->handle, NULL, 0);
    }
}

// FUNCTION: LEGORR 0x00408ec0
void construction_spawn_barriers(int* blockPos, int* shapePoints,
                                 unsigned int shapeCount)
{
    Lego_ConstructionZone* zone =
        construction_zone_find_by_handle_or_at_block(blockPos, NULL);
    ConstructionPoint2I* shape = (ConstructionPoint2I*)shapePoints;
    float offset = 8.8f / terrain_context_scale_get(CONSTRUCTION_LEGO_GET_MAP());
    ConstructionPoint2I DIRECTIONS[4] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
    float BARRIERS[4][2];
    ConstructionPoint2I pos;
    unsigned int i;
    unsigned int j;
    unsigned int k;
    int found;

    BARRIERS[0][0] = 0.5f;
    BARRIERS[0][1] = 1.0f - offset;
    BARRIERS[1][0] = 1.0f - offset;
    BARRIERS[1][1] = 0.5f;
    BARRIERS[2][0] = 0.5f;
    BARRIERS[2][1] = offset;
    BARRIERS[3][0] = offset;
    BARRIERS[3][1] = 0.5f;

    for (i = 0; i < shapeCount; i++) {
        if (i != shapeCount - 1 && shape[i].x == shape[i + 1].x &&
            shape[i].y == shape[i + 1].y) {
            i++;
        } else {
            for (j = 0; j < 4; j++) {
                found = 0;
                pos.x = DIRECTIONS[j].x + shape[i].x;
                pos.y = shape[i].y + DIRECTIONS[j].y;
                for (k = 0; k < shapeCount; k++) {
                    if (k != shapeCount - 1 && shape[k].x == shape[k + 1].x &&
                        shape[k].y == shape[k + 1].y) {
                        k++;
                    } else if (shape[k].x == pos.x && shape[k].y == pos.y) {
                        found = 1;
                    }
                }
                if (!found) {
                    aitask_cmd_command_kind_record_issue(
                        CONSTRUCTION_OBJECT_BARRIER, 0, 0, &shape[i].x,
                        zone->handle, BARRIERS[j], 1);
                    zone->requestCount++;
                }
            }
        }
    }
}

// FUNCTION: LEGORR 0x004090a0
int construction_zone_exists_at_block(int* blockPos)
{
    return construction_zone_find_by_handle_or_at_block(blockPos, NULL) != NULL;
}

// FUNCTION: LEGORR 0x004090c0
Lego_ConstructionZone* construction_zone_find_by_handle_or_at_block(
    int* blockPos, unsigned int* handle)
{
    Lego_ConstructionZone* zone;

    for (zone = constructionGlobs.constructList; zone; zone = zone->next) {
        if (blockPos) {
            if (blockPos[0] == zone->origin.x && blockPos[1] == zone->origin.y)
                return zone;
        } else if (handle && *handle == zone->handle) {
            return zone;
        }
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00409110
void construction_zone_complete_path(int* blockPos)
{
    Lego_ConstructionZone* zone =
        construction_zone_find_by_handle_or_at_block(blockPos, NULL);

    if (zone) {
        cancel_action_for_matching_coordinates((const uint32_t*)blockPos);
        construction_zone_remove_barriers(zone);
        construction_zone_free(zone);
    }
}

// FUNCTION: LEGORR 0x00409150
void construction_zone_cancel_path(int* blockPos)
{
    Lego_ConstructionZone* zone =
        construction_zone_find_by_handle_or_at_block(blockPos, NULL);
    unsigned int i;
    Lego_Object* obj;

    terrain_cell_flag40000000_clear_or_set_current(blockPos, 0);
    for (i = 0; i < zone->placedObjectCount; i++) {
        obj = zone->placedObjects[i];
        obj->flags4 &= ~CONSTRUCTION_OBJECT4_PLACED;
        aitask_cmd_create_task((struct EntitySlot*)obj, 0);
    }
    cancel_action_for_matching_coordinates((const uint32_t*)blockPos);
    aitask_cmd_cancel_by_coords(zone->handle, 0, 0, 0);
    construction_zone_free(zone);
}

// FUNCTION: LEGORR 0x004091e0
void construction_update_all(float elapsedGame)
{
    Lego_ConstructionZone* zone;
    Lego_ConstructionZone* next;

    for (zone = constructionGlobs.constructList; zone; zone = next) {
        next = zone->next;
        if ((zone->flags & CONSTRUCTION_FLAG_LAYPATH) &&
            !(zone->flags & CONSTRUCTION_FLAG_REQUESTED)) {
            construction_zone_request_path_resources(zone);
        } else if (zone->flags & CONSTRUCTION_FLAG_READY) {
            if (zone->flags & CONSTRUCTION_FLAG_LAYPATH) {
                aitask_bobbin_clear_delay(&zone->origin.x);
                zone->flags &= ~CONSTRUCTION_FLAG_READY;
            } else if (construction_zone_origin_is_clear(zone)) {
                construction_zone_remove_barriers(zone);
                construction_zone_complete_building(zone, 1);
                construction_zone_free(zone);
            }
        }
    }
}

static int construction_zone_origin_object_callback(Lego_Object* liveObj,
                                                     void* data);

// FUNCTION: LEGORR 0x00409270
int construction_zone_origin_is_clear(Lego_ConstructionZone* zone)
{
    return !legoobject_run_through_lists_skip_upgrade_parts(
        construction_zone_origin_object_callback, zone);
}

/* LegoObject_RunThroughLists callback: TRUE when liveObj stands on the
 * zone's origin block and is not one of its placed resources. */
// FUNCTION: LEGORR 0x00409290
static int construction_zone_origin_object_callback(Lego_Object* liveObj,
                                                     void* data)
{
    Lego_ConstructionZone* zone = (Lego_ConstructionZone*)data;
    ConstructionPoint2I blockPos;
    unsigned int i;

    legoobject_position_to_terrain_cell(liveObj, &blockPos.x, &blockPos.y);
    if (zone->origin.x == blockPos.x && zone->origin.y == blockPos.y) {
        for (i = 0; i < zone->placedObjectCount; i++) {
            if (liveObj == zone->placedObjects[i])
                break;
        }
        if (i == zone->placedObjectCount)
            return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00409300
int construction_zone_start_path(int* blockPos)
{
    Lego_ConstructionZone* zone;

    if (!level_cell_occupied_at_current(blockPos)) {
        zone = construction_zone_create(blockPos);
        terrain_cell_flag40000000_clear_or_set_current(blockPos, 1);
        zone->flags |= CONSTRUCTION_FLAG_LAYPATH;
        cancel_action_for_matching_coordinates((const uint32_t*)blockPos);
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00409350
Lego_ConstructionZone* construction_zone_create(int* blockPos)
{
    Lego_ConstructionZone* zone =
        (Lego_ConstructionZone*)malloc(sizeof(Lego_ConstructionZone));

    memset(zone, 0, sizeof(Lego_ConstructionZone));
    zone->placedCount = 0;
    zone->requestCount = 0;
    zone->handle = constructionGlobs.nextHandle++;
    zone->origin.x = blockPos[0];
    zone->origin.y = blockPos[1];
    zone->next = constructionGlobs.constructList;
    constructionGlobs.constructList = zone;
    return zone;
}

// FUNCTION: LEGORR 0x004093b0
void construction_flood_path_blocks(int* blockPos)
{
    ConstructionPoint2I DIRECTIONS[4] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
    unsigned int i;

    for (i = 0; i < 4; i++) {
        DIRECTIONS[i].x += blockPos[0];
        DIRECTIONS[i].y += blockPos[1];
        if (level_cell_occupied_at_current(&DIRECTIONS[i].x) ||
            terrain_cell_occupied_or_flag_current(DIRECTIONS[i].x, DIRECTIONS[i].y, 1)) {
            if (!lego_terrain_cell_secondary_flag_100(&DIRECTIONS[i].x)) {
                lego_terrain_cell_flag100_set_and_track(&DIRECTIONS[i].x);
                construction_flood_path_blocks(&DIRECTIONS[i].x);
            }
        }
    }
}

// FUNCTION: LEGORR 0x00409450
int construction_find_resource_object(int* blockPos, int resourceAmount)
{
    int result = construction_find_resource_object_recurse(blockPos, resourceAmount);

    terrain_cell_visited_flags_clear_all_current();
    return result;
}

// FUNCTION: LEGORR 0x00409470
int construction_find_resource_object_recurse(int* blockPos,
                                              int resourceAmount)
{
    ConstructionPoint2I DIRECTIONS[4] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
    Lego_Object* obj;
    unsigned int i;

    for (i = 0; i < 4; i++) {
        DIRECTIONS[i].x += blockPos[0];
        DIRECTIONS[i].y += blockPos[1];
        if (terrain_cell_secondary_bit0_current(&DIRECTIONS[i].x) &&
            (obj = legoobject_at_grid_position_find(&DIRECTIONS[i].x)) != NULL &&
            legoobject_available_resource_spend(obj, resourceAmount))
            return 1;
        if (level_cell_occupied_at_current(&DIRECTIONS[i].x) ||
            terrain_cell_occupied_or_flag_current(DIRECTIONS[i].x, DIRECTIONS[i].y, 1)) {
            if (!lego_terrain_cell_visited_flag(&DIRECTIONS[i].x)) {
                lego_terrain_cell_visited_flag_set_and_track(&DIRECTIONS[i].x);
                if (construction_find_resource_object_recurse(&DIRECTIONS[i].x, resourceAmount))
                    return 1;
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00409550
void construction_zone_request_path_resources(Lego_ConstructionZone* zone)
{
    Lego_Object* obj;
    unsigned int crystals;
    unsigned int ore;
    unsigned int studs;

    if (CONSTRUCTION_BLOCK_IS_PATH(&zone->origin)) {
        obj = legoobject_query_center_mask02(NULL, &zone->origin.x, 7, 0);
        crystals = entity_indexed_record_word_at_b4_get(CONSTRUCTION_OBJECT_PATH, 0, 0);
        ore = entity_indexed_record_word_at_b0_get(CONSTRUCTION_OBJECT_PATH, 0, 0);
        studs = stats_table_word_at_b8(CONSTRUCTION_OBJECT_PATH, 0, 0);
        construction_zone_request_resource(&zone->origin.x,
                                           CONSTRUCTION_OBJECT_CRYSTAL, 0, 0,
                                           crystals);
        if (obj && studs)
            construction_zone_request_resource(&zone->origin.x,
                                               CONSTRUCTION_OBJECT_ORE, 1, 0,
                                               studs);
        else
            construction_zone_request_resource(&zone->origin.x,
                                               CONSTRUCTION_OBJECT_ORE, 0, 0,
                                               ore);
        zone->flags |= CONSTRUCTION_FLAG_REQUESTED;
    }
}

// FUNCTION: LEGORR 0x00409600
void construction_zone_cancel_building(int* blockPos)
{
    Lego_Level* level = legoGlobs_currLevel;
    Lego_ConstructionZone* zone =
        construction_zone_find_by_handle_or_at_block(blockPos, NULL);
    unsigned int i;

    if (zone && zone->requestCount != zone->placedCount) {
        for (i = 0; i < zone->shapeCount; i++) {
            if (i != zone->shapeCount - 1 &&
                zone->shapePoints[i].x == zone->shapePoints[i + 1].x &&
                zone->shapePoints[i].y == zone->shapePoints[i + 1].y) {
                CONSTRUCTION_BLOCK(level, zone->shapePoints[i].x,
                                   zone->shapePoints[i].y).flags1 &=
                    ~CONSTRUCTION_BLOCK1_ENTRANCE;
            }
            CONSTRUCTION_BLOCK(level, zone->shapePoints[i].x,
                               zone->shapePoints[i].y).flags1 &=
                ~CONSTRUCTION_BLOCK1_ZONE;
            CONSTRUCTION_BLOCK_ZONE(level, zone->shapePoints[i].x,
                                    zone->shapePoints[i].y) = NULL;
            lego_cell_init_pass(level, zone->shapePoints[i].x,
                                zone->shapePoints[i].y, 0);
            cancel_action_for_matching_coordinates(
                (const uint32_t*)&zone->shapePoints[i]);
        }
        for (i = 0; i < zone->placedObjectCount; i++) {
            if (zone->placedObjects[i]->type == CONSTRUCTION_OBJECT_BARRIER) {
                zone->placedObjects[i]->flags3 |= CONSTRUCTION_OBJECT3_REMOVING;
                zone->placedObjects[i]->health = -1.0f;
            } else {
                zone->placedObjects[i]->flags4 &= ~CONSTRUCTION_OBJECT4_PLACED;
                aitask_cmd_create_task((struct EntitySlot*)zone->placedObjects[i], 0);
            }
        }
        aitask_cmd_cancel_by_coords(zone->handle, 0, 0, 0);
        construction_zone_free(zone);
    }
}

// FUNCTION: LEGORR 0x00409790
unsigned int construction_zone_start_building(int objID, int* blockPos,
                                              unsigned int direction,
                                              int* shapePoints,
                                              unsigned int shapeCount)
{
    Lego_Level* level = legoGlobs_currLevel;
    Lego_ConstructionZone* zone = construction_zone_create(blockPos);
    ConstructionPoint2I* shape = (ConstructionPoint2I*)shapePoints;
    unsigned int i;
    int entrance;

    zone->objID = objID;
    zone->direction = direction;
    zone->shapePoints = (ConstructionPoint2I*)malloc(
        shapeCount * sizeof(ConstructionPoint2I));
    memcpy(zone->shapePoints, shape, shapeCount * sizeof(ConstructionPoint2I));
    zone->shapeCount = shapeCount;
    terrain_map_shape_flatten(CONSTRUCTION_LEGO_GET_MAP(),
                              (const TerrainMapPoint2I*)zone->shapePoints,
                              shapeCount, 4.0f);

    for (i = 0; i < shapeCount; i++) {
        if (i != shapeCount - 1 && shape[i].x == shape[i + 1].x &&
            shape[i].y == shape[i + 1].y) {
            entrance = 1;
            CONSTRUCTION_BLOCK(level, shape[i].x, shape[i].y).flags1 |=
                CONSTRUCTION_BLOCK1_ENTRANCE;
            CONSTRUCTION_BLOCK_ZONE(level, shape[i].x, shape[i].y) = zone;
        } else {
            entrance = 0;
        }
        CONSTRUCTION_BLOCK(level, shape[i].x, shape[i].y).flags1 |=
            CONSTRUCTION_BLOCK1_ZONE;
        CONSTRUCTION_BLOCK_ZONE(level, shape[i].x, shape[i].y) = zone;
        lego_cell_init_pass(level, shape[i].x, shape[i].y, 0);
        construction_update_surrounding_blocks(&shape[i].x);
        cancel_action_for_matching_coordinates((const uint32_t*)&shape[i]);
        if (entrance)
            i++;
    }
    return zone->handle;
}

// FUNCTION: LEGORR 0x00409940
void construction_update_surrounding_blocks(int* blockPos)
{
    ConstructionPoint2I DIRECTIONS[8] = {
        { -1, -1 }, { 0, -1 }, { 1, -1 }, { 1, 0 },
        { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }
    };
    unsigned int i;

    for (i = 0; i < 8; i++) {
        terrain_cell_mutation_progress(g_active_level_retail,
                                       blockPos[0] + DIRECTIONS[i].x,
                                       DIRECTIONS[i].y + blockPos[1]);
    }
}

// FUNCTION: LEGORR 0x004099d0
void construction_remove_all(void)
{
    Lego_ConstructionZone* zone = constructionGlobs.constructList;
    Lego_ConstructionZone* next;

    while (zone) {
        next = zone->next;
        construction_zone_free(zone);
        zone = next;
    }
}

// FUNCTION: LEGORR 0x004099f0
void construction_zone_free(Lego_ConstructionZone* dead)
{
    Lego_ConstructionZone* last = NULL;
    Lego_ConstructionZone* zone;

    for (zone = constructionGlobs.constructList; zone; zone = zone->next) {
        if (zone == dead) {
            if (last)
                last->next = zone->next;
            else
                constructionGlobs.constructList = zone->next;
            break;
        }
        last = zone;
    }
    free(dead->shapePoints);
    free(dead);
}

// FUNCTION: LEGORR 0x00409a40
void construction_zone_remove_barriers(Lego_ConstructionZone* zone)
{
    Lego_Object* obj;
    unsigned int i;

    for (i = 0; i < zone->placedObjectCount; i++) {
        obj = zone->placedObjects[i];
        obj->flags3 |= CONSTRUCTION_OBJECT3_REMOVING;
        obj->health = -1.0f;
    }
}

// FUNCTION: LEGORR 0x00409a90
void* construction_place_building(int objID, int* blockPos,
                                  unsigned int direction, int* shapePoints,
                                  unsigned int shapeCount, int teleportDown)
{
    Lego_ConstructionZone zone;

    memset(&zone, 0, sizeof(zone));
    zone.direction = direction;
    zone.origin.x = blockPos[0];
    zone.origin.y = blockPos[1];
    zone.objID = objID;
    zone.shapePoints = (ConstructionPoint2I*)shapePoints;
    zone.shapeCount = shapeCount;
    terrain_map_shape_flatten(CONSTRUCTION_LEGO_GET_MAP(),
                              (const TerrainMapPoint2I*)shapePoints,
                              shapeCount, 4.0f);
    construction_update_surrounding_blocks(blockPos);
    return construction_zone_complete_building(&zone, teleportDown);
}

// FUNCTION: LEGORR 0x00409b30
struct Lego_Object* construction_zone_complete_building(
    Lego_ConstructionZone* zone, int teleportDown)
{
    unsigned int i = 0;
    float heading = (float)zone->direction * 1.5707964f;
    float xPos;
    float yPos;
    Lego_Object* obj;

    terrain_map_block_to_world(CONSTRUCTION_LEGO_GET_MAP(), zone->origin.x,
                               zone->origin.y, &xPos, &yPos);
    obj = legoobject_create_in_world(&legoGlobs_buildingData[zone->objID],
                                     CONSTRUCTION_OBJECT_BUILDING, zone->objID,
                                     0, xPos, yPos, heading);
    if (teleportDown) {
        legoobject_dispatch_event5_and_set_flag(obj);
        info_send(8, NULL, obj, NULL);
    }
    for (i = 0; i < zone->shapeCount; i++) {
        if (i != zone->shapeCount - 1 &&
            zone->shapePoints[i].x == zone->shapePoints[i + 1].x &&
            zone->shapePoints[i].y == zone->shapePoints[i + 1].y) {
            lego_terrain_cell_flags_set_mask20108000(zone->shapePoints[i].x,
                                                     zone->shapePoints[i].y);
            legoobject_update_and_unregister_at_tile(obj, &zone->shapePoints[i].x);
            i++;
        } else {
            if (entity_stats_word_get((struct EntitySlot*)obj) & 0x100000) {
                lego_terrain_cell_flags_set_and_refresh(&zone->shapePoints[i].x);
            } else {
                cancel_action_for_matching_coordinates(
                    (const uint32_t*)&zone->shapePoints[i]);
                lego_terrain_cell_flags_set_mask100400(zone->shapePoints[i].x,
                                                       zone->shapePoints[i].y);
                if (entity_state_flags_get((struct EntitySlot*)obj) & 0x200)
                    terrain_cell_secondary_bit0_set_current(&zone->shapePoints[i].x);
                construction_update_cardinal_blocks(&zone->shapePoints[i].x);
            }
            electric_fence_block_surfaces_update(zone->shapePoints[i].x,
                                                 zone->shapePoints[i].y);
        }
    }
    return obj;
}

// FUNCTION: LEGORR 0x00409cd0
void construction_update_cardinal_blocks(int* blockPos)
{
    ConstructionPoint2I DIRECTIONS[4] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
    unsigned int i;

    for (i = 0; i < 4; i++) {
        DIRECTIONS[i].x += blockPos[0];
        DIRECTIONS[i].y += blockPos[1];
        aitask_terrain_cell_refresh_dispatch_current(&DIRECTIONS[i].x, 0);
    }
}

#ifdef __cplusplus
}
#endif
