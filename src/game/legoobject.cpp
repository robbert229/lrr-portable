// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-023 LegoObject.c: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-023.csv. */
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "ai.hpp"
#include "entity.hpp"
#include "building_scene.hpp"
#include "actor_scene.hpp"
#include "type_config_ports.hpp"
#include "entity_pose_delta.hpp"
#include "entity_motion_ports.hpp"
#include "activity_timeout.hpp"
#include "entity_motion_probe.hpp"
#include "port_loop_ports.hpp"
#include "simulation_ui_ports.hpp"
#include "type_motion_scale.hpp"
#include "entity_quad_collision.hpp"
#include "spawn.hpp"
#include "weapon.hpp"
#include "effects.hpp"
#include "level.hpp"
#include "terrain_grid.hpp"
#include "cmd.hpp"
#include "select.hpp"
#include "gods98/model.hpp"
#include "gods98/view.hpp"
#include "gods98/engine.hpp"
#include "gods98/resource.hpp"
#include "gods98/sound.hpp"
#include "gods98/record.hpp"
#include "gods98/heap.hpp"
#include "gods98/message.hpp"
#include "gods98/string.hpp"
#include "gods98/block.hpp"
#include "upgrades.hpp"
#include "crt/crt.hpp"
#include "frontend.hpp"
#include "entity_shared.hpp"
/* C linkage for legoobject_unit.c (the frozen header below redeclares it). */
extern "C" int entity_building_index_set(EntitySlot* entity, uint32_t state);
#include "entity_from_entity_stats.hpp"
#include "entity_from_lego.hpp"
#include "util.hpp"
#include "gods98/media.hpp"
#include "entity_for_lego.hpp"
#include "mission.hpp"
#include "ai_from_lego.hpp"
#include "cmd_shared.hpp"
#include "combat_ray_ports.hpp"
#include "simtick.hpp"
#include "gods98/scene_pool.hpp"
#include "gods98/scene_resources.hpp"
#include "gods98/surface.hpp"
#include "gods98/texture.hpp"
#include "bezier.h"
#include <stddef.h>
#include <stdarg.h>
#include <time.h>
#include "frontend.h"
#include "objective.hpp"
#include "terrain_feedback.hpp"
#include "savegame.hpp"
#include "gods98/audio.hpp"
#include "gods98/flic.hpp"
#include "gods98/input.hpp"
#include "priority_config_ports.hpp"
#include "tutorial_hud_ports.hpp"
#include "ui_action_dispatch.hpp"
#include "camera_drag.hpp"
#include "screen_pick.hpp"
#include "pointer_cursor.hpp"
#include "ui_list.hpp"
#include "frontend_for_panel_unit.hpp"
#include <limits.h>
#include "game.hpp"
#include "gods98/camera.hpp"
#include "scene_render.hpp"
#include "render.hpp"
#include "spawn_for_lego.hpp"
#include "camera_for_lego.hpp"
#include "simtick_shared.hpp"
#include "view_shared.hpp"
/* C linkage for legoGlobs' object-name tables, which legoobject_unit.c
 * reads (declared again, with the same types, by the frozen header below). */
extern "C" const char* s_object_name_special[9];
extern "C" const char* s_object_the_name_special[9];
extern "C" const char** s_object_name_kind1;
extern "C" const char** s_object_name_kind2;
extern "C" const char** s_object_name_kind3;
extern "C" const char** s_object_name_kind4;
extern "C" const char** s_object_name_kind10;
extern "C" const char** s_object_the_name_kind1;
extern "C" const char** s_object_the_name_kind2;
extern "C" const char** s_object_the_name_kind3;
extern "C" const char** s_object_the_name_kind4;
extern "C" const char** s_object_the_name_kind10;
#include "entity_for_legoobject.hpp"
/* C linkage for legoobject_unit.c (the frozen header below redeclares it). */
extern "C" const char* s_tool_type_names[AI_TOOL_TYPE_COUNT];
#include "ai_for_legoobject.hpp"
#include "cmd_for_legoobject.hpp"
#include "effects_for_legoobject.hpp"
#include "frontend_for_legoobject.hpp"
#include "level_for_legoobject.hpp"
#include "spawn_for_legoobject.hpp"
#include "terrain_grid_for_legoobject.hpp"
#include "gods98/camera_for_legoobject.hpp"
#include "gods98/simtick_for_legoobject.hpp"
#include "gods98/view_for_legoobject.hpp"
/* C linkage for the state legoobject_unit.c reads (declared again, with
 * the same types, by the frozen header below). */
extern "C" const char* s_activity[AI_ACTIVITY_COUNT];
extern "C" const char* s_ability_type_names[AI_ABILITY_TYPE_COUNT];
extern "C" uint32_t s_activity_resource_slots[AI_ACTIVITY_RESOURCE_SLOT_COUNT];
extern "C" EntitySlot* s_idle100[AI_IDLE_TABLE_MAX];
extern "C" int s_idle100_n;
#include "ai_from_legoobject.hpp"
#if defined(LEGORRR_BACKEND_MINIWIN)
#include "creature_scene.hpp"
#endif

struct EntityGridFindQuery {
    int x;
    int y;
    LevelState* level;
    EntitySlot* found;
};

typedef struct EntitySpawnRequest {
    int type;
    int kind;
} EntitySpawnRequest;

typedef struct EntitySpawnRequestNestedSum {
    float value;
} EntitySpawnRequestNestedSum;

typedef struct EntityTypeTwoPositionedDispatch {
    const EntitySlot* source;
    float inner_range;
    float outer_range;
    int mode;
    float source_position[3];
    int source_position_valid;
} EntityTypeTwoPositionedDispatch;

struct EntityTaskAtGridCoordinateRequest {
    uint32_t x;
    uint32_t y;
    EntitySlot** out_entity;
};

#define POOL_RECORD_STRIDE 0x410  /* corpus 0x40c widened for the link */
#define POOL_RECORD_LINK_OFF 0x408

namespace unported_ledger_stubs {
// AdjacentPositionCandidateFind
typedef struct AdjacentRouteAcceptContext {
    const EntitySlot* entity;
    const LevelState* level;
    int route_cookie;
} AdjacentRouteAcceptContext;

typedef struct NearestOpenCellCandidate {
    int x;
    int y;
    float distance;
} NearestOpenCellCandidate;

// PolyVariantSectionModelBuild
typedef int (*PolyVariantSectionBuildHook)(void* context, void* section,
                                           uint32_t variant);

// EntitySceneObjectTreeAttach
typedef int (*EntitySceneTreeAttachHook)(void* context, EntitySlot* entity,
                                        void* scene_object, void* parent);
} // namespace unported_ledger_stubs

#define ENT_FLAG3EC_ACTIVITY_OK    0x00000004u  /* +0x3ec bit 0x4: the list
                                                 * permits a 4/5/7 activity;
                                                 * written by FUN_0044b370 */

typedef struct TerrainVariantVisitContext {
    void* owner_entity;
} TerrainVariantVisitContext;

/* The corpus's eligibility gate compares the +0x340 bound against this
 * .rdata zero dword (objdump-verified fcomp operand of
 * EntitySharedEligibilityCheck); a named const static keeps the fcomp
 * operand resolvable by Reccmp. */
// GLOBAL: LEGORR 0x0049e3a4
static const float s_eligibility_floor = 0.0f;   /* DAT_0049e3a4, verified */

// GLOBAL: LEGORR 0x004a48d4
const char s_tool_name_birdscarer[] = "LegoObject_ToolType_BirdScarer";
// GLOBAL: LEGORR 0x004a48f4
const char s_tool_name_pushergun[] = "LegoObject_ToolType_PusherGun";
// GLOBAL: LEGORR 0x004a4914
const char s_tool_name_laser[] = "LegoObject_ToolType_Laser";
// GLOBAL: LEGORR 0x004a4930
const char s_tool_name_spanner[] = "LegoObject_ToolType_Spanner";
// GLOBAL: LEGORR 0x004a494c
const char s_tool_name_hammer[] = "LegoObject_ToolType_Hammer";
// GLOBAL: LEGORR 0x004a4968
const char s_tool_name_spade[] = "LegoObject_ToolType_Spade";
// GLOBAL: LEGORR 0x004a4984
const char s_tool_name_drill[] = "LegoObject_ToolType_Drill";
// GLOBAL: LEGORR 0x004a49a0
static const char s_ability_name_scanner[] = "LegoObject_AbilityType_Scanner";
// GLOBAL: LEGORR 0x004a49c0
static const char s_ability_name_repair[] = "LegoObject_AbilityType_Repair";
// GLOBAL: LEGORR 0x004a49e0
static const char s_ability_name_dynamite[] = "LegoObject_AbilityType_Dynamite";
// GLOBAL: LEGORR 0x004a4a00
static const char s_ability_name_driver[] = "LegoObject_AbilityType_Driver";
// GLOBAL: LEGORR 0x004a4a20
static const char s_ability_name_sailor[] = "LegoObject_AbilityType_Sailor";
// GLOBAL: LEGORR 0x004a4a40
static const char s_ability_name_pilot[] = "LegoObject_AbilityType_Pilot";
// GLOBAL: LEGORR 0x004a4a60
static const char s_activity_name_activity_teleportin[] = "Activity_TeleportIn";
// GLOBAL: LEGORR 0x004a4a74
static const char s_activity_name_activity_crumble[] = "Activity_Crumble";
// GLOBAL: LEGORR 0x004a4a88
static const char s_activity_name_activity_waiting21[] = "Activity_Waiting21";
// GLOBAL: LEGORR 0x004a4a9c
static const char s_activity_name_activity_waiting20[] = "Activity_Waiting20";
// GLOBAL: LEGORR 0x004a4ab0
static const char s_activity_name_activity_waiting19[] = "Activity_Waiting19";
// GLOBAL: LEGORR 0x004a4ac4
static const char s_activity_name_activity_waiting18[] = "Activity_Waiting18";
// GLOBAL: LEGORR 0x004a4ad8
static const char s_activity_name_activity_waiting17[] = "Activity_Waiting17";
// GLOBAL: LEGORR 0x004a4aec
static const char s_activity_name_activity_waiting16[] = "Activity_Waiting16";
// GLOBAL: LEGORR 0x004a4b00
static const char s_activity_name_activity_waiting15[] = "Activity_Waiting15";
// GLOBAL: LEGORR 0x004a4b14
static const char s_activity_name_activity_waiting14[] = "Activity_Waiting14";
// GLOBAL: LEGORR 0x004a4b28
static const char s_activity_name_activity_waiting13[] = "Activity_Waiting13";
// GLOBAL: LEGORR 0x004a4b3c
static const char s_activity_name_activity_waiting12[] = "Activity_Waiting12";
// GLOBAL: LEGORR 0x004a4b50
static const char s_activity_name_activity_waiting11[] = "Activity_Waiting11";
// GLOBAL: LEGORR 0x004a4b64
static const char s_activity_name_activity_waiting10[] = "Activity_Waiting10";
// GLOBAL: LEGORR 0x004a4b78
static const char s_activity_name_activity_waiting9[] = "Activity_Waiting9";
// GLOBAL: LEGORR 0x004a4b8c
static const char s_activity_name_activity_waiting8[] = "Activity_Waiting8";
// GLOBAL: LEGORR 0x004a4ba0
static const char s_activity_name_activity_waiting7[] = "Activity_Waiting7";
// GLOBAL: LEGORR 0x004a4bb4
static const char s_activity_name_activity_waiting6[] = "Activity_Waiting6";
// GLOBAL: LEGORR 0x004a4bc8
static const char s_activity_name_activity_waiting5[] = "Activity_Waiting5";
// GLOBAL: LEGORR 0x004a4bdc
static const char s_activity_name_activity_waiting4[] = "Activity_Waiting4";
// GLOBAL: LEGORR 0x004a4bf0
static const char s_activity_name_activity_waiting3[] = "Activity_Waiting3";
// GLOBAL: LEGORR 0x004a4c04
static const char s_activity_name_activity_waiting2[] = "Activity_Waiting2";
// GLOBAL: LEGORR 0x004a4c18
static const char s_activity_name_activity_waiting1[] = "Activity_Waiting1";
// GLOBAL: LEGORR 0x004a4c2c
static const char s_activity_name_activity_open[] = "Activity_Open";
// GLOBAL: LEGORR 0x004a4c3c
static const char s_activity_name_activity_closing[] = "Activity_Closing";
// GLOBAL: LEGORR 0x004a4c50
static const char s_activity_name_activity_opening[] = "Activity_Opening";
// GLOBAL: LEGORR 0x004a4c64
static const char s_activity_name_activity_floatoff[] = "Activity_FloatOff";
// GLOBAL: LEGORR 0x004a4c78
static const char s_activity_name_activity_floaton[] = "Activity_FloatOn";
// GLOBAL: LEGORR 0x004a4c8c
static const char s_activity_name_activity_train[] = "Activity_Train";
// GLOBAL: LEGORR 0x004a4c9c
static const char s_activity_name_activity_wakeup[] = "Activity_WakeUp";
// GLOBAL: LEGORR 0x004a4cac
static const char s_activity_name_activity_recharge[] = "Activity_Recharge";
// GLOBAL: LEGORR 0x004a4cc0
static const char s_activity_name_activity_freezemelt[] = "Activity_FreezeMelt";
// GLOBAL: LEGORR 0x004a4cd4
static const char s_activity_name_activity_freezestart[] = "Activity_FreezeStart";
// GLOBAL: LEGORR 0x004a4cec
static const char s_activity_name_activity_freezed[] = "Activity_Freezed";
// GLOBAL: LEGORR 0x004a4d00
static const char s_activity_name_activity_firelaser[] = "Activity_FireLaser";
// GLOBAL: LEGORR 0x004a4d14
static const char s_activity_name_activity_unpowered[] = "Activity_Unpowered";
// GLOBAL: LEGORR 0x004a4d28
static const char s_activity_name_activity_explode[] = "Activity_Explode";
// GLOBAL: LEGORR 0x004a4d3c
static const char s_activity_name_activity_upgrade[] = "Activity_Upgrade";
// GLOBAL: LEGORR 0x004a4d50
static const char s_activity_name_activity_buildpath[] = "Activity_BuildPath";
// GLOBAL: LEGORR 0x004a4d64
static const char s_activity_name_activity_getup[] = "Activity_GetUp";
// GLOBAL: LEGORR 0x004a4d74
static const char s_activity_name_activity_thrownbyrockmonster[] = "Activity_ThrownByRockMonster";
// GLOBAL: LEGORR 0x004a4d94
static const char s_activity_name_activity_throwman[] = "Activity_ThrowMan";
// GLOBAL: LEGORR 0x004a4da8
static const char s_activity_name_activity_stamp[] = "Activity_Stamp";
// GLOBAL: LEGORR 0x004a4db8
static const char s_activity_name_activity_eat[] = "Activity_Eat";
// GLOBAL: LEGORR 0x004a4dc8
static const char s_activity_name_activity_rest[] = "Activity_Rest";
// GLOBAL: LEGORR 0x004a4dd8
static const char s_activity_name_activity_slip[] = "Activity_Slip";
// GLOBAL: LEGORR 0x004a4de8
static const char s_activity_name_activity_repair[] = "Activity_Repair";
// GLOBAL: LEGORR 0x004a4df8
static const char s_activity_name_activity_place[] = "Activity_Place";
// GLOBAL: LEGORR 0x004a4e08
static const char s_activity_name_activity_clear[] = "Activity_Clear";
// GLOBAL: LEGORR 0x004a4e18
static const char s_activity_name_activity_deposit[] = "Activity_Deposit";
// GLOBAL: LEGORR 0x004a4e2c
static const char s_activity_name_activity_dynamite[] = "Activity_Dynamite";
// GLOBAL: LEGORR 0x004a4e40
static const char s_activity_name_activity_hithard[] = "Activity_HitHard";
// GLOBAL: LEGORR 0x004a4e54
static const char s_activity_name_activity_hitback[] = "Activity_HitBack";
// GLOBAL: LEGORR 0x004a4e68
static const char s_activity_name_activity_hitfront[] = "Activity_HitFront";
// GLOBAL: LEGORR 0x004a4e7c
static const char s_activity_name_activity_hitright[] = "Activity_HitRight";
// GLOBAL: LEGORR 0x004a4e90
static const char s_activity_name_activity_hitleft[] = "Activity_HitLeft";
// GLOBAL: LEGORR 0x004a4ea4
static const char s_activity_name_activity_carrystand[] = "Activity_CarryStand";
// GLOBAL: LEGORR 0x004a4eb8
static const char s_activity_name_activity_carryturnright[] = "Activity_CarryTurnRight";
// GLOBAL: LEGORR 0x004a4ed0
static const char s_activity_name_activity_carryturnleft[] = "Activity_CarryTurnLeft";
// GLOBAL: LEGORR 0x004a4ee8
static const char s_activity_name_activity_throw[] = "Activity_Throw";
// GLOBAL: LEGORR 0x004a4ef8
static const char s_activity_name_activity_carryrubble[] = "Activity_CarryRubble";
// GLOBAL: LEGORR 0x004a4f10
static const char s_activity_name_activity_carry[] = "Activity_Carry";
// GLOBAL: LEGORR 0x004a4f20
static const char s_activity_name_activity_gather[] = "Activity_Gather";
// GLOBAL: LEGORR 0x004a4f30
static const char s_activity_name_activity_collect[] = "Activity_Collect";
// GLOBAL: LEGORR 0x004a4f44
static const char s_activity_name_activity_enterrein[] = "Activity_EnterRein";
// GLOBAL: LEGORR 0x004a4f58
static const char s_activity_name_activity_enter[] = "Activity_Enter";
// GLOBAL: LEGORR 0x004a4f68
static const char s_activity_name_activity_emerge[] = "Activity_Emerge";
// GLOBAL: LEGORR 0x004a4f78
static const char s_activity_name_activity_cantdo[] = "Activity_CantDo";
// GLOBAL: LEGORR 0x004a4f88
static const char s_activity_name_activity_turnright[] = "Activity_TurnRight";
// GLOBAL: LEGORR 0x004a4f9c
static const char s_activity_name_activity_turnleft[] = "Activity_TurnLeft";
// GLOBAL: LEGORR 0x004a4fb0
static const char s_activity_name_activity_reverse[] = "Activity_Reverse";
// GLOBAL: LEGORR 0x004a4fc4
static const char s_activity_name_activity_reinforce[] = "Activity_Reinforce";
// GLOBAL: LEGORR 0x004a4fd8
static const char s_activity_name_activity_walk[] = "Activity_Walk";
// GLOBAL: LEGORR 0x004a4fe8
static const char s_activity_name_activity_teleport[] = "Activity_Teleport";
// GLOBAL: LEGORR 0x004a4ffc
static const char s_activity_name_activity_drill[] = "Activity_Drill";
// GLOBAL: LEGORR 0x004a500c
static const char s_activity_name_activity_runpanic[] = "Activity_RunPanic";
// GLOBAL: LEGORR 0x004a5020
static const char s_activity_name_activity_routerubble[] = "Activity_RouteRubble";
// GLOBAL: LEGORR 0x004a5038
static const char s_activity_name_activity_route[] = "Activity_Route";

/* The unit (legoobject_unit.c) reads this state with C linkage; the
 * frozen *_from_legoobject.hpp headers are pre-declared to match where
 * they are included. */
extern "C" {
// GLOBAL: LEGORR 0x004de790
void* s_pool_chunks[POOL_CHUNK_COUNT_SLOTS];
// GLOBAL: LEGORR 0x004de810
void* s_pool_record_head;

// GLOBAL: LEGORR 0x004de814
int s_terrain_sfx_grid[TERRAIN_SFX_GRID_CELLS]; /* DAT_004de814 role */

// GLOBAL: LEGORR 0x004decc4
const char* s_activity[AI_ACTIVITY_COUNT] = {
    s_activity_name_activity_stand, s_activity_name_activity_route, s_activity_name_activity_routerubble,
    s_activity_name_activity_runpanic, s_activity_name_activity_drill, s_activity_name_activity_teleport,
    s_activity_name_activity_walk, s_activity_name_activity_reinforce, s_activity_name_activity_reverse,
    s_activity_name_activity_turnleft, s_activity_name_activity_turnright, s_activity_name_activity_cantdo,
    s_activity_name_activity_emerge, s_activity_name_activity_enter, s_activity_name_activity_enterrein,
    s_activity_name_activity_collect, s_activity_name_activity_gather, s_activity_name_activity_carry,
    s_activity_name_activity_carryrubble, s_activity_name_activity_throw, s_activity_name_activity_carryturnleft,
    s_activity_name_activity_carryturnright, s_activity_name_activity_carrystand, s_activity_name_activity_hitleft,
    s_activity_name_activity_hitright, s_activity_name_activity_hitfront, s_activity_name_activity_hitback,
    s_activity_name_activity_hithard, s_activity_name_activity_dynamite, s_activity_name_activity_deposit,
    s_activity_name_activity_clear, s_activity_name_activity_place, s_activity_name_activity_repair, s_activity_name_activity_slip,
    s_activity_name_activity_rest, s_activity_name_activity_eat, s_activity_name_activity_stamp, s_activity_name_activity_throwman,
    s_activity_name_activity_thrownbyrockmonster, s_activity_name_activity_getup, s_activity_name_activity_buildpath,
    s_activity_name_activity_upgrade, s_activity_name_activity_explode, s_activity_name_activity_unpowered,
    s_activity_name_activity_firelaser, s_activity_name_activity_freezed, s_activity_name_activity_freezestart,
    s_activity_name_activity_freezemelt, s_activity_name_activity_recharge, s_activity_name_activity_wakeup,
    s_activity_name_activity_train, s_activity_name_activity_floaton, s_activity_name_activity_floatoff,
    s_activity_name_activity_opening, s_activity_name_activity_closing, s_activity_name_activity_open,
    s_activity_name_activity_waiting1, s_activity_name_activity_waiting2, s_activity_name_activity_waiting3,
    s_activity_name_activity_waiting4, s_activity_name_activity_waiting5, s_activity_name_activity_waiting6,
    s_activity_name_activity_waiting7, s_activity_name_activity_waiting8, s_activity_name_activity_waiting9,
    s_activity_name_activity_waiting10, s_activity_name_activity_waiting11, s_activity_name_activity_waiting12,
    s_activity_name_activity_waiting13, s_activity_name_activity_waiting14, s_activity_name_activity_waiting15,
    s_activity_name_activity_waiting16, s_activity_name_activity_waiting17, s_activity_name_activity_waiting18,
    s_activity_name_activity_waiting19, s_activity_name_activity_waiting20, s_activity_name_activity_waiting21,
    s_activity_name_activity_crumble, s_activity_name_activity_teleportin
};

} /* extern "C" */

// GLOBAL: LEGORR 0x004dee00
float* s_terrain_neighbor_path_cost;
/* DAT_004dee04 / DAT_004dee08: the retail path-search functions share and
 * reuse one zeroed dword per map cell. */
// GLOBAL: LEGORR 0x004dee04
static uint32_t* s_terrain_path_depth;
// GLOBAL: LEGORR 0x004dee08
static uint32_t s_terrain_path_depth_count;

extern "C" {
// GLOBAL: LEGORR 0x004dee10
uint32_t s_pool_chunk_count;

/* --- the entity activity scene-state leaf (see ai.hpp) ---------------------
 * This shared zero-init word owns the ported DAT_004dee14 bits. The corpus
 * reads its level pointer through the threaded GameContext. */
// GLOBAL: LEGORR 0x004dee14
uint32_t s_entity_state_flags;
// GLOBAL: LEGORR 0x004dee18
uint32_t s_activity_resource_slots[AI_ACTIVITY_RESOURCE_SLOT_COUNT] = {
    0u, 0u, 1u, 1u, 4u, 4u, 4u, 4u, 3u, 2u, 4u
};

// GLOBAL: LEGORR 0x004dee44
uint32_t s_grid_count_second[GRID_COUNT_TABLE_DWORDS];

// GLOBAL: LEGORR 0x004e2684
uint32_t s_task_counter_nonzero;

// GLOBAL: LEGORR 0x004e3944
uint32_t s_grid_count_first[GRID_COUNT_TABLE_DWORDS];

// GLOBAL: LEGORR 0x004e7184
uint32_t s_task_counter_zero;

/* The corpus keeps a bounded twenty-entry pool of the predug coordinates
 * encountered while loading the layer. */
// GLOBAL: LEGORR 0x004e844c
int s_predug_coordinates[20][2];

// GLOBAL: LEGORR 0x004e84ec
int s_type11_cell_coordinates[10][2];

// GLOBAL: LEGORR 0x004e853c
unsigned int s_predug_coordinate_count;

// GLOBAL: LEGORR 0x004e8540
unsigned int s_type11_cell_coordinate_count;

/* The six-dword render snapshot is a separate zero-filled global block in
 * the corpus (DAT_004ea7ac). Its individual values are unresolved; keep the
 * storage opaque and expose only the exact copy/clear operations. */
// GLOBAL: LEGORR 0x004ea7ac
uint32_t s_render_config_snapshot[6];

// GLOBAL: LEGORR 0x004eabcc
EntitySlot* s_idle100[AI_IDLE_TABLE_MAX];
// GLOBAL: LEGORR 0x004ead5c
int s_idle100_n;
// GLOBAL: LEGORR 0x004ead60
const char* s_ability_type_names[AI_ABILITY_TYPE_COUNT] = {
    s_ability_name_pilot, s_ability_name_sailor,
    s_ability_name_driver, s_ability_name_dynamite,
    s_ability_name_repair, s_ability_name_scanner
};

/* GlobalAnimationStateSet (FUN_004383a0) — see entity.hpp. */
// GLOBAL: LEGORR 0x004eadc4
uint32_t s_global_animation_state; /* DAT_004eadc4 */
// GLOBAL: LEGORR 0x004eadd0
EntityAuxiliaryReleaseRecord* s_entity_auxiliary_release_records;
} /* extern "C" */
static EntitySlot* s_entity_teardown_free_head;

static uint32_t s_grid_object_record_words[200];

static EntitySpawnRequest s_entity_spawn_requests[9];
static uint32_t s_entity_spawn_request_count;

static float s_scene_timer = 0.0f;

static SimtickCellCoord* s_scene_pending = NULL;

static uint32_t s_scene_pending_capacity = 0;

static void* entity_spatial_query_execute_current(
    const SpatialObjectQueryRequest* request);

namespace unported_ledger_stubs {
void ledger_stub_00439370_EntityBuildPlacementCommandSetup(
    const int cell_xy[2], int entity_id, const float world_xy[2],
    int update_state);
void ledger_stub_004394b0_EntityBuildPlacementAlternateCommandSetup(
    const int cell_xy[2], int entity_id, const float world_xy[2],
    int update_state);

void ledger_stub_00446610_EntityMovementTickScaleAndDeltaGet(
    void* entity, float delta, float* movement_delta,
    float* movement_scale);
float ledger_stub_00447b30_EntityActivityTick(EntitySlot* entity, float delta);
void ledger_stub_004477d0_PathStepLookaheadEntryWrite(EntitySlot* entity);

// EntityNearestConnectedTerrainCellRouteSelect
static int entity_route_to_cell_commit(EntitySlot* entity,
                                       int target_x, int target_y,
                                       unsigned int marker);

// LevelDebugCellRetargetDispatch
int ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
    EntitySlot* entity, unsigned int goal_x, unsigned int goal_y,
    int include_diagonals, unsigned int* out_candidate, int strict);
// EntityAdjacentPlacementCellSearch
int ledger_stub_004473b0_RouteToCellCommandIssue(
    EntitySlot* entity, int target_x, int target_y, EntitySlot* target);

void ledger_stub_00447ea0_EntityTypeBehaviorUpdate(
    EntitySlot* entity, float delta, const EntityTypeBehaviorHooks* hooks);

int ledger_stub_00449880_BlockedCellApproachWeightWrite(
    EntitySlot* entity, unsigned int cell_x, unsigned int cell_y,
    float amount);

int ledger_stub_00449880_BlockedCellApproachWeightWrite(
    EntitySlot* entity, unsigned int cell_x, unsigned int cell_y, float amount);

int ledger_stub_00449880_BlockedCellApproachWeightWrite(
    EntitySlot* entity, unsigned int cell_x, unsigned int cell_y,
    float amount);
// EntityNeighborCellMoveCommandHandle
static unsigned int entity_route_cardinal_direction(int source_x, int source_y,
                                                    int target_x, int target_y);
static float entity_route_weight_random(float minimum, float maximum);

// CardinalStepCommandIssueAndLink
int ledger_stub_00448eb0_ProximityStepCommandIssueWithJitter(
    EntitySlot* entity, EntitySlot* target, int marker);

// EntityKind6RecordApproachRouteIssue
int type11_cell_coordinate_nearest_resolve(const EntitySlot* target,
                                                 int cell_xy_out[2]);

int ledger_stub_0044a5d0_NearestOpenCellCoordSearch(
    EntitySlot* entity, int* output_x, int* output_y,
    int require_minimum_distance, int allow_maskc0, int allow_flag20);
int ledger_stub_0044a4f0_NearestCellCoordinateWorldResolve(
    const EntitySlot* target, int cell_xy_out[2]);
} // namespace unported_ledger_stubs

int activity_register_bookkeeping(EntitySlot* e);

namespace unported_ledger_stubs {
void ledger_stub_0044afb0_ConfiguredTinyMonsterSpawnAndTeardown(
    EntitySlot* source);
void ledger_stub_0044af90_EntityAttachedSpawnRecordProcess(EntitySlot* owner);
int ledger_stub_0044a5d0_NearestOpenCellCoordSearch(
    EntitySlot* entity, int* output_x, int* output_y,
    int require_minimum_distance, int allow_maskc0, int allow_flag20);
int ledger_stub_00449880_BlockedCellApproachWeightWrite(
    EntitySlot* entity, unsigned int cell_x, unsigned int cell_y,
    float amount);
int ledger_stub_004473b0_RouteToCellCommandIssue(
    EntitySlot* entity, int target_x, int target_y, EntitySlot* target);
void entity_attached_object_list_detach(EntitySlot* entity, int reset_mode);
} // namespace unported_ledger_stubs

extern "C" float random_float_range(float minimum, float maximum);

extern "C" short rng_thunk_draw15(void);
extern "C" float random_float_range(float minimum, float maximum);

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static int queued_grid_animation_anchor_cell_get(void* anchor, float position[2]);
static EntitySlot* queued_grid_animation_spatial_find( const float position[2], int type, int filter_a, int filter_b);
static int entity_linked_route_clear_callback(EntitySlot* candidate, void* payload);
int entity_pool_find_by_callback_filtered_current( int (*callback)(EntitySlot*, void*), void* payload);
static void entity_action_consume_counter_sidecar_release(EntitySlot* owner);
int entity_pool_find_by_callback_current( int (*callback)(EntitySlot*, void*), void* payload, int filter_flagged);
static int deferred_olist_string_equal(const char* first, const char* second);
static int entity_grid_position_match(EntitySlot* entity, void* payload);
static GameContext* building_upgrade_context_get(EntitySlot* building);
static int building_upgrade_live_payment_select(EntitySlot* building, uint32_t* resource_out, uint32_t* amount_out);
int entity_route_marker_candidate_visit(EntitySlot* candidate, void* payload);
namespace unported_ledger_stubs { static void spatial_query_center_from_cell( SpatialObjectQueryRequest* request, const int cell_xy[2]); }
namespace unported_ledger_stubs { static void spatial_query_request_init(SpatialObjectQueryRequest* request, uint32_t mask); }
namespace unported_ledger_stubs { static void spatial_query_center_from_entity( SpatialObjectQueryRequest* request, const EntitySlot* entity); }
static int entity_route_marker_query_match( EntitySlot* candidate, const EntityRouteMarkerSearchRequest* request);
static int entity_display_name_config_index(const EntitySlot* entity);
static int entity_unload_adjacent_cell_find_on_level( const EntitySlot* entity, const LevelState* level, int out_xy[2]);
static void entity_carried_links_clear(EntitySlot* entity);
static EntitySlot* entity_carried_slot_get(EntitySlot* entity, int index);
static void entity_carried_slot_set(EntitySlot* entity, int index, EntitySlot* carried);
static int entity_deposit_position_get(EntitySlot* entity, float position[3]);
static void idle100_register(EntitySlot* e);
static int terrain_variant_entity_visit_adapter(EntitySlot* candidate, void* context);
namespace unported_ledger_stubs { static int emergence_cell_world_position_get(int cell_x, int cell_y, float out_xy[2]); }
namespace unported_ledger_stubs { static EntitySlot* emergence_entity_create(void* base_entity, int owner_index, float x, float y, float heading); }
namespace unported_ledger_stubs { static GameContext* emergence_game_context_get(const void* base_entity); }
namespace unported_ledger_stubs { static LevelState* emergence_level_get(const void* base_entity); }
static void scene_cell_previous_state_track(EngineState* st);
static void scene_cell_update_queue_flush(EngineState* st);
static int scene_entity_callback(EntitySlot* entity, void* payload);
static int entity_pool_teardown_callback(EntitySlot* entity, void* unused);
static int entity_task_at_grid_coordinate_candidate( EntitySlot* entity, void* payload);
static int entity_type_two_positioned_candidate(EntitySlot* candidate, void* payload);
static void objective_sidecar_activity_set(EntitySlot* entity, int activity);
static void objective_sidecar_owner_detach(GameContext* ctx, EntitySlot* entity);
static void entity_kind_four_adjacent_expand(LevelState* level, int x, int y);
static int entity_kind_four_candidate_find(GameContext* context, LevelState* level, const int start_xy[2], int amount);
static int entity_spawn_request_pair_matches(const EntitySpawnRequest* request, const EntitySlot* requester, GameContext* context);
static int entity_linked_reference_matches(EntitySlot* candidate, void* target);
namespace unported_ledger_stubs { void ledger_stub_00444460_FlaggedVectorStepCommandIssue( EntitySlot* entity, const float vector[2]); }
static int terrain_path_cell_accept(void* entity, uint32_t x, uint32_t y);
static int terrain_path_grid_size_get(void* entity, uint32_t* width, uint32_t* height);
static int terrain_path_route_accept(void* entity, uint32_t x, uint32_t y, int direction);
static int terrain_path_half_cost_accept(void* entity, uint32_t x, uint32_t y, int direction);
static LevelState* terrain_path_level_resolve(void* argument, EntitySlot** actor_out);
namespace unported_ledger_stubs { static int adjacent_route_candidate_stop(void*, const uint32_t cell[2], void* context); }
namespace unported_ledger_stubs { static EntityPathMotionRuntime* entity_path_motion_runtime_get( EntitySlot* entity, int create); }
namespace unported_ledger_stubs { int entity_step_movement_vector_resolve(EntitySlot* entity, int advance, int use_saved_vector, int steering_mode); }
namespace unported_ledger_stubs { static int entity_work_entry_grid_pick(float x, float y, int* cx, int* cy); }
static int entity_type_four_linked_type_five_reset(EntitySlot* candidate, void* payload);
static int activity_skips_idle_chain(int activity);
namespace unported_ledger_stubs { static float entity_type_behavior_animation_advance( EntitySlot* entity, float delta, const EntityTypeBehaviorHooks* hooks); }
namespace unported_ledger_stubs { static void entity_type_behavior_animation_play( EntitySlot* entity, const EntityTypeBehaviorHooks* hooks, const char* animation); }
namespace unported_ledger_stubs { static void entity_type_behavior_cell_flag_80000_set(EntitySlot* entity, int set); }
namespace unported_ledger_stubs { static int entity_route_path_install(EntitySlot* entity, LevelState* level, int target_x, int target_y, const float* marker_position); }
namespace unported_ledger_stubs { static int entity_route_live_position_to_cell(const EntitySlot* entity, LevelState* level, int* cell_x, int* cell_y); }
static int render_entity_callback(EntitySlot* entity, void* payload);
namespace unported_ledger_stubs { static int nearest_open_cell_candidate_compare(const void* first, const void* second); }
namespace lego { namespace { static int global_scatter_find(int mode); } }

/* ActivityNameTableInit (FUN_00436c20) resets state represented by the
 * cleared global range, then installs activity, ability, and tool names. */
// FUNCTION: LEGORR 0x00436C20
void ai_activity_name_table_initialize(void)
{
    /* The retail initializer writes these same table values on every call.
     * Volatile lvalues preserve those observable stores without making the
     * lookup paths volatile or changing their runtime values. */
    const char* volatile* const activity_names =
        (const char* volatile*)s_activity;
    const char* volatile* const ability_names =
        (const char* volatile*)s_ability_type_names;
    const char* volatile* const tool_names =
        (const char* volatile*)s_tool_type_names;
    volatile uint32_t* const activity_slots =
        (volatile uint32_t*)s_activity_resource_slots;
    /* The reference clears 0x318f dwords beginning at DAT_004de790, through
     * 0x004eadc8. The port mirrors of every owned slice in that range must be
     * reset along with the retail words. */
    /* The retail 0x318f-dword clear at 0x4de790 also spans both parallel
     * grid-record count tables (0x4dee44 and 0x4e3944). They live in the
     * Level module's sidecar storage, so reset them through their owner. */
    grid_record_count_tables_clear();
    /* DAT_004dee00 lies inside the cleared range. Release its sidecar-backed
     * allocation through its owner so the cleared pointer cannot go stale. */
    terrain_neighbor_path_cost_release();
    /* The same clear covers the queued-animation fractional accumulator at
     * 0x4ea7a8 and the six-dword render snapshot at 0x4ea7ac. These are owned
     * by the entity and level modules, respectively. */
    queued_grid_animation_accumulator_clear();
    render_config_six_field_copy_in_or_clear(NULL);
    /* DAT_004eabcc and its count word at DAT_004ead5c are also in the clear
     * span; clear the widened idle-registration sidecar through its owner. */
    ai_idle_table_reset();
    activity_names[0] = s_activity_name_activity_stand;
    activity_names[1] = s_activity_name_activity_route;
    activity_names[2] = s_activity_name_activity_routerubble;
    activity_names[3] = s_activity_name_activity_runpanic;
    activity_names[4] = s_activity_name_activity_drill;
    activity_names[5] = s_activity_name_activity_teleport;
    activity_names[6] = s_activity_name_activity_walk;
    activity_names[7] = s_activity_name_activity_reinforce;
    activity_names[8] = s_activity_name_activity_reverse;
    activity_names[9] = s_activity_name_activity_turnleft;
    activity_names[10] = s_activity_name_activity_turnright;
    activity_names[11] = s_activity_name_activity_cantdo;
    activity_names[12] = s_activity_name_activity_emerge;
    activity_names[13] = s_activity_name_activity_enter;
    activity_names[14] = s_activity_name_activity_enterrein;
    activity_names[15] = s_activity_name_activity_collect;
    activity_names[16] = s_activity_name_activity_gather;
    activity_names[17] = s_activity_name_activity_carry;
    activity_names[18] = s_activity_name_activity_carryrubble;
    activity_names[19] = s_activity_name_activity_throw;
    activity_names[20] = s_activity_name_activity_carryturnleft;
    activity_names[21] = s_activity_name_activity_carryturnright;
    activity_names[22] = s_activity_name_activity_carrystand;
    activity_names[23] = s_activity_name_activity_hitleft;
    activity_names[24] = s_activity_name_activity_hitright;
    activity_names[25] = s_activity_name_activity_hitfront;
    activity_names[26] = s_activity_name_activity_hitback;
    activity_names[27] = s_activity_name_activity_hithard;
    activity_names[28] = s_activity_name_activity_dynamite;
    activity_names[29] = s_activity_name_activity_deposit;
    activity_names[30] = s_activity_name_activity_clear;
    activity_names[31] = s_activity_name_activity_place;
    activity_names[32] = s_activity_name_activity_repair;
    activity_names[33] = s_activity_name_activity_slip;
    activity_names[34] = s_activity_name_activity_rest;
    activity_names[35] = s_activity_name_activity_eat;
    activity_names[36] = s_activity_name_activity_stamp;
    activity_names[37] = s_activity_name_activity_throwman;
    activity_names[38] = s_activity_name_activity_thrownbyrockmonster;
    activity_names[39] = s_activity_name_activity_getup;
    activity_names[40] = s_activity_name_activity_buildpath;
    activity_names[41] = s_activity_name_activity_upgrade;
    activity_names[42] = s_activity_name_activity_explode;
    activity_names[43] = s_activity_name_activity_unpowered;
    activity_names[44] = s_activity_name_activity_firelaser;
    activity_names[45] = s_activity_name_activity_freezed;
    activity_names[46] = s_activity_name_activity_freezestart;
    activity_names[47] = s_activity_name_activity_freezemelt;
    activity_names[48] = s_activity_name_activity_recharge;
    activity_names[49] = s_activity_name_activity_wakeup;
    activity_names[50] = s_activity_name_activity_train;
    activity_names[51] = s_activity_name_activity_floaton;
    activity_names[52] = s_activity_name_activity_floatoff;
    activity_names[53] = s_activity_name_activity_opening;
    activity_names[54] = s_activity_name_activity_closing;
    activity_names[55] = s_activity_name_activity_open;
    activity_names[56] = s_activity_name_activity_waiting1;
    activity_names[57] = s_activity_name_activity_waiting2;
    activity_names[58] = s_activity_name_activity_waiting3;
    activity_names[59] = s_activity_name_activity_waiting4;
    activity_names[60] = s_activity_name_activity_waiting5;
    activity_names[61] = s_activity_name_activity_waiting6;
    activity_names[62] = s_activity_name_activity_waiting7;
    activity_names[63] = s_activity_name_activity_waiting8;
    activity_names[64] = s_activity_name_activity_waiting9;
    activity_names[65] = s_activity_name_activity_waiting10;
    activity_names[66] = s_activity_name_activity_waiting11;
    activity_names[67] = s_activity_name_activity_waiting12;
    activity_names[68] = s_activity_name_activity_waiting13;
    activity_names[69] = s_activity_name_activity_waiting14;
    activity_names[70] = s_activity_name_activity_waiting15;
    activity_names[71] = s_activity_name_activity_waiting16;
    activity_names[72] = s_activity_name_activity_waiting17;
    activity_names[73] = s_activity_name_activity_waiting18;
    activity_names[74] = s_activity_name_activity_waiting19;
    activity_names[75] = s_activity_name_activity_waiting20;
    activity_names[76] = s_activity_name_activity_waiting21;
    activity_names[77] = s_activity_name_activity_crumble;
    activity_names[78] = s_activity_name_activity_teleportin;

    ability_names[0] = s_ability_name_pilot;
    ability_names[1] = s_ability_name_sailor;
    ability_names[2] = s_ability_name_driver;
    ability_names[3] = s_ability_name_dynamite;
    ability_names[4] = s_ability_name_repair;
    ability_names[5] = s_ability_name_scanner;

    tool_names[0] = s_tool_name_drill;
    tool_names[1] = s_tool_name_spade;
    tool_names[2] = s_tool_name_hammer;
    tool_names[3] = s_tool_name_spanner;
    tool_names[4] = s_tool_name_laser;
    tool_names[5] = s_tool_name_pushergun;
    tool_names[6] = s_tool_name_birdscarer;
    tool_names[7] = s_tool_name_freezergun + 1;

    activity_slots[0] = 0;
    activity_slots[1] = 0;
    activity_slots[2] = 1;
    activity_slots[3] = 1;
    activity_slots[8] = 3;
    activity_slots[9] = 2;
    activity_slots[10] = 4;
    activity_slots[7] = 4;
    activity_slots[4] = 4;
    activity_slots[5] = 4;
    activity_slots[6] = 4;

    pooled_record_pool_initialize();
    entity_iteration_guard_flags_set(1u);
}

/* EntityPoolResourcesRelease (FUN_00437050) runs the reference's filtered
 * entity teardown walk before releasing tracked record chunks and clearing
 * the pool/lifecycle state. DAT_004dee00 is the terrain-grid cost map; retail
 * frees it without nulling the global pointer, while the portable owner
 * clears it to make repeated teardown safe. */
// FUNCTION: LEGORR 0x00437050
void pooled_record_pool_resources_release(void)
{
    uint32_t i;
    entity_iteration_callback_guard();
    for (i = 0; i < POOL_CHUNK_COUNT_SLOTS; ++i) {
        if (s_pool_chunks[i] != NULL) {
            engine_heap_free(s_pool_chunks[i]);
            s_pool_chunks[i] = NULL;
        }
    }
    s_pool_record_head = NULL;
    s_pool_chunk_count = 0;
    /* The retail body clears DAT_004dee14 before freeing the cost map, then
     * repeats its bit-0 clear afterward. */
    s_entity_iteration_guard_flags = 0u;
    terrain_neighbor_path_cost_release();
    s_entity_iteration_guard_flags &= ~1u;
}

// FUNCTION: LEGORR 0x004370b0
void render_config_six_field_copy_out(uint32_t out[6])
{
    if (out == NULL) {
        return; /* deviation: the corpus copies through the pointer blindly */
    }
    const uint32_t* source = s_render_config_snapshot;
    int remaining = 6;
    do {
        *out++ = *source++;
        --remaining;
    } while (remaining != 0);
}

// FUNCTION: LEGORR 0x004370d0
void render_config_six_field_copy_in_or_clear(const uint32_t in[6])
{
    if (in != NULL) {
        uint32_t* destination = s_render_config_snapshot;
        int remaining = 6;
        do {
            *destination++ = *in++;
            --remaining;
        } while (remaining != 0);
        return;
    }
    uint32_t* destination = s_render_config_snapshot;
    int remaining = 6;
    do {
        *destination++ = 0;
        --remaining;
    } while (remaining != 0);
}

// FUNCTION: LEGORR 0x00437100
int grid_record_count_difference_sum(int type, int skip_flagged_rows)
{
    int total = 0;
    unsigned row;
    unsigned slot;

    row = 0;
    do {
        slot = 0;
        do {
            if (skip_flagged_rows == 0
                || (stats_table_word_at_144(type, (int)row) & 8u) == 0u) {
                total += grid_record_count_difference_get(type, (int)row,
                                                           (int)slot, 1);
            }
            ++slot;
        } while (slot < 16);
        ++row;
    } while (row < 15u);
    return total;
}

/* ToolTipIconsConfigLoad (FUN_00437150): load matching images from the
 * ToolTipIcons config section into the two named icon banks and special
 * Blank/Cursor slots. */
// FUNCTION: LEGORR 0x00437150
void tooltip_icons_config_load(void* config)
{
    BlockRec* section = block_subgroup((BlockFile*)config,
        string_path_join(util_root_name((BlockFile*)config), "ToolTipIcons",
                         (const char*)NULL));
    BlockRec* entry;
    if (section == NULL) {
        if (s_tooltip_cursor_handle != NULL) {
            texture_release((TextureSurface*)s_tooltip_cursor_handle);
        }
        s_tooltip_cursor_handle = NULL;
        return;
    }
    entry = section;
    while (entry != NULL) {
        const char* name = block_name(entry);
        const char* value = block_value(entry);
        unsigned int i;
        int matched = 0;
        for (i = 0; i < 11u; ++i) {
            if (ui_panel_string_equal_case_insensitive(
                    name, s_tooltip_primary_names[i])) {
                s_tooltip_primary_handles[i] = image_surface_load(value, 0u, 0u);
                matched = 1;
                break;
            }
        }
        if (!matched) {
            for (i = 0; i < 6u; ++i) {
                if (ui_panel_string_equal_case_insensitive(
                        name, s_tooltip_secondary_names[i])) {
                    s_tooltip_secondary_handles[i] = image_surface_load(
                        value, 0u, 0u);
                    matched = 1;
                    break;
                }
            }
        }
        if (!matched && ui_panel_string_equal_case_insensitive(name, "Blank"))
            s_tooltip_blank_handle = image_surface_load(value, 0u, 0u);
        else if (!matched && ui_panel_string_equal_case_insensitive(
                                 name, "Cursor"))
            s_tooltip_cursor_handle = image_surface_load(value, 0u, 0u);
        entry = block_next_sibling(entry);
    }
}

// FUNCTION: LEGORR 0x004372a0
void spawn_coordinate_pool_clear(void)
{
    unsigned int i;
    entity_iteration_callback_guard();
    grid_record_count_tables_clear();
    grid_object_records_release();
    level_predug_coordinate_pool_clear();
    camera_predug_spawn_state_clear();
    camera_spawn_coordinate_hook_set(NULL, NULL);
    for (i = 0; i < 10u; ++i) {
        s_type11_cell_coordinates[i][0] = 0;
        s_type11_cell_coordinates[i][1] = 0;
    }
    s_type11_cell_coordinate_count = 0u;
    queued_grid_animation_accumulator_clear();
    ai_idle_table_reset();
    entity_global_animation_state_set(0u);
}

/* ProcessQueuedGridAnimations (FUN_00437300): accumulate scaled fractional
 * work, then consume one queued type-six animation for each whole unit. */
// FUNCTION: LEGORR 0x00437300
void process_queued_grid_animations(void* anchor, int mode, float delta)
{
    QueuedGridAnimationHooks* const hooks = &s_queued_grid_animation_hooks;
    float position[2];
    float scaled_delta;
    if (mode == -1) {
        scaled_delta = delta;
    }
    else if (hooks->mode_scale_get != NULL) {
        scaled_delta = hooks->mode_scale_get(mode, hooks->context) * delta;
    }
    else {
        scaled_delta = lego::weapon_type_discharge_rate_get(mode) * delta;
    }
    s_queued_grid_animation_fraction += scaled_delta;
    if (hooks->anchor_position_get != NULL) {
        if (!hooks->anchor_position_get(anchor, position, hooks->context))
            return;
    }
    else if (!queued_grid_animation_anchor_cell_get(anchor, position)) return;
    while (s_queued_grid_animation_fraction >= 1.0f) {
        EntitySlot* candidate = hooks->entity_find != NULL
            ? hooks->entity_find(position, 6, 0, 0, hooks->context)
            : queued_grid_animation_spatial_find(position, 6, 0, 0);
        EntitySlot* animation_entity;
        if (candidate == NULL) break;
        animation_entity = hooks->animation_entity_get != NULL
            ? hooks->animation_entity_get(candidate, 6, 0, 0, hooks->context)
            : entity_action_kind_entity_create_and_command_issue(
                  candidate, 6, 0, 0);
        if (animation_entity == NULL) break;
        animation_entity->flags_3e8 |= 0x80000000u;
        if (hooks->work_queue_update != NULL) {
            hooks->work_queue_update(animation_entity, 0, hooks->context);
        }
        else {
            rover_work_queue_update(animation_entity, 0);
        }
        s_queued_grid_animation_fraction -= 1.0f;
    }
}

/* EntityActivityToggle (FUN_004373d0) — see ai.hpp. The corpus ends every
 * path — both arms and both flag-test skips (the returns at 0x43740a and the
 * shared tail at 0x437431) — on the FUN_00447c40 bookkeeping call, so the
 * port calls the file-local carrier there for code shape; the flag folds
 * stay the dword spelling of the corpus's byte ops on the +0x3ec word. */
// FUNCTION: LEGORR 0x004373d0
void entity_activity_toggle(EntitySlot* e, int on)
{
    if (e == NULL) {
        return; /* deviation: the corpus strides the entity blindly (pinned) */
    }
    if (on == 0) {
        if ((e->flags_3ec & 0x2000u) != 0u) {
            e->flags_3ec = (e->flags_3ec & ~0x2000u) | 0x4000u;
            ai_set_activity(e, 0x36, 0); /* FUN_004409e0 */
        }
    }
    else if ((e->flags_3ec & 0x2000u) == 0u) {
        e->flags_3ec |= 0x1000u;
        ai_set_activity(e, 0x35, 0); /* FUN_004409e0 */
    }
    activity_register_bookkeeping(e); /* FUN_00447c40, corpus tail call */
}

// FUNCTION: LEGORR 0x00437440
void grid_record_count_tables_clear(void)
{
    for (size_t k = 0; k < GRID_COUNT_TABLE_DWORDS; ++k) {
        s_grid_count_first[k] = 0u;
    }
    for (size_t second_index = 0; second_index < GRID_COUNT_TABLE_DWORDS;
         ++second_index) {
        s_grid_count_second[second_index] = 0u;
    }
}

// FUNCTION: LEGORR 0x00437460
int grid_record_count_difference_get(int row, int column, int slot,
                                     int subtract_first_table)
{
    const int64_t idx = grid_count_index(row, column, slot);
    if (idx < 0) {
        return 0;
    }
    const int v = (int)s_grid_count_second[(size_t)idx];
    return (subtract_first_table != 0)
        ? v - (int)s_grid_count_first[(size_t)idx]
        : v;
}

// FUNCTION: LEGORR 0x004374a0
uint32_t grid_record_count_get(int row, int column, int slot)
{
    const int64_t idx = grid_count_index(row, column, slot);
    return (idx < 0) ? 0u : s_grid_count_first[(size_t)idx];
}

/* TaskCounterIncrementByBranch (FUN_004374d0). */
// FUNCTION: LEGORR 0x004374d0
void entity_task_counter_increment_by_branch(int nonzero)
{
    if (nonzero != 0) {
        ++s_task_counter_nonzero;
    } else {
        ++s_task_counter_zero;
    }
}

/* EntityCallbackDispatchUnlessTypeFive (FUN_004374f0): type five is a
 * non-dispatchable source; otherwise clear every live candidate route that
 * points back to it, then notify the optional host observer. */
// FUNCTION: LEGORR 0x004374F0
void entity_callback_dispatch_unless_type_five(EntitySlot* entity)
{
    if (entity == NULL || entity->type == 5) return;
    entity_pool_find_by_callback_filtered_current(
        entity_linked_route_clear_callback, entity);
    if (s_entity_callback_dispatch_hook != NULL) {
        s_entity_callback_dispatch_hook(entity,
                                        s_entity_callback_dispatch_context);
    }
}
// FUNCTION: LEGORR 0x00437510
static int entity_linked_route_clear_callback(EntitySlot* candidate,
                                              void* payload)
{
    EntitySlot* source = (EntitySlot*)payload;
    if (candidate != NULL && source != NULL &&
        candidate->link_2fc == source) {
        entity_action_sequence_reset(candidate, 0);
        candidate->link_2fc = NULL;
    }
    /* LAB_00437510 always continues so every matching link is cleared. */
    return 0;
}

/* EntityTeardown (FUN_00437540). The legacy routine's engine calls are
 * represented by typed stage callbacks; record flags, command-chain cleanup,
 * special back-reference clearing, and free-list ownership stay local. */
/* Port model of 0x00437540 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00437540, retires to the tail in GAME-023 part 3
int entity_teardown(EntitySlot* entity)
{
    uint32_t flags;
    uint32_t state_flags;
    int type;
    if (entity == NULL) return 0;
    /* Cancel every live GetTool consumer before any actor/building scene or
     * pool slot can disappear, including actors skipped by filtered walks. */
    cmd_gettool_entity_removing(entity);
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    if (entity->type == 4) building_scene_entity_release(entity);
    if (entity->type == 2 || entity->type == 3) actor_scene_entity_release(entity);
#elif defined(LEGORRR_BACKEND_MINIWIN)
    /* EntityTeardown (0x437540): Creature_Remove, then free the record. */
    creature_scene_entity_release(entity);
#endif
    type = entity->type;
    flags = (uint32_t)entity->flags_3ec;
    state_flags = s_entity_teardown_hooks.state_flags_get != NULL ?
        s_entity_teardown_hooks.state_flags_get(entity,
            s_entity_teardown_hooks.context) : entity_stats_word_get(entity);
    if (s_entity_teardown_hooks.counter_increment != NULL) {
        unsigned int slot = ((unsigned int)entity->etype +
                             (unsigned int)type * 15U) * 16U;
        s_entity_teardown_hooks.counter_increment(slot,
            s_entity_teardown_hooks.context);
    }
    if ((flags & 0x100000U) != 0 &&
        s_entity_teardown_hooks.resource_release != NULL) {
        void* resource = s_entity_teardown_hooks.resource_get != NULL ?
            s_entity_teardown_hooks.resource_get(entity, 0xd4u,
                s_entity_teardown_hooks.context) : NULL;
        s_entity_teardown_hooks.resource_release(resource,
            s_entity_teardown_hooks.context);
    }
    if ((flags & 0x80000U) != 0 &&
        s_entity_teardown_hooks.resource_release != NULL) {
        void* resource = s_entity_teardown_hooks.resource_get != NULL ?
            s_entity_teardown_hooks.resource_get(entity, 0xd5u,
                s_entity_teardown_hooks.context) : NULL;
        s_entity_teardown_hooks.resource_release(resource,
            s_entity_teardown_hooks.context);
    }
    if (s_entity_teardown_hooks.operation != NULL)
        s_entity_teardown_hooks.operation(entity, ENTITY_TEARDOWN_CALLBACK,
            s_entity_teardown_hooks.context);
    if (type == 3 && s_entity_teardown_hooks.operation != NULL)
        s_entity_teardown_hooks.operation(entity,
            ENTITY_TEARDOWN_TYPE_THREE_PREPARE,
            s_entity_teardown_hooks.context);
    if ((flags & 0x40U) != 0) {
        entity->flags_3ec = (int32_t)(flags & ~0x40U);
        flags &= ~0x40U;
        if (entity->link_2fc != NULL) {
            EntitySlot* linked = entity->link_2fc;
            linked->flags_3ec &= ~0x40u;
        }
    }
    if ((flags & 0x80U) != 0) {
        if (entity->link_2fc != NULL) {
            EntitySlot* linked = entity->link_2fc;
            linked->flags_3ec &= ~0x80u;
        }
        entity->link_2fc = NULL;
        entity->flags_3ec = (int32_t)(flags & ~0x80U);
        flags &= ~0x80U;
    }
    /* EntityTeardown calls TrackedEntityPointerArrayRemove here, before it
     * releases the entity's other registries and payload. */
    ai_idle_table_pointer_remove(entity); /* FUN_0043a930 */
    entity_pointer_registries_remove(entity);
    entity_type_operation_fallback_handle_set(entity, NULL);
    entity_action_sequence_reset(entity, 0);
    entity_type_four_callback_dispatch(entity);
    if (entity->attach_304 != NULL) {
        unported_ledger_stubs::entity_attached_object_list_detach(
            (EntitySlot*)entity->attach_304, 0);
    }
    unported_ledger_stubs::entity_attached_object_list_detach(entity, 0);
    /* Remove the token from the scene before the release callback can return
     * that renderer record to its pool. Keep the entity token valid through
     * the release callback itself. */
    entity_scene_slot_nodes_clear(entity);
    entity_activity_runner_release(entity);
    entity_ddiscene_scene_owner_bind(entity, NULL);
    if (s_entity_teardown_hooks.operation != NULL) {
        s_entity_teardown_hooks.operation(entity, ENTITY_TEARDOWN_DETACH_PARENT,
            s_entity_teardown_hooks.context);
        s_entity_teardown_hooks.operation(entity, ENTITY_TEARDOWN_RELEASE_SCENE,
            s_entity_teardown_hooks.context);
        s_entity_teardown_hooks.operation(entity, ENTITY_TEARDOWN_RELEASE_LINK,
            s_entity_teardown_hooks.context);
        s_entity_teardown_hooks.operation(entity, ENTITY_TEARDOWN_RESET_REFERENCE,
            s_entity_teardown_hooks.context);
        s_entity_teardown_hooks.operation(entity, ENTITY_TEARDOWN_RESET_ACTION,
            s_entity_teardown_hooks.context);
        s_entity_teardown_hooks.operation(entity, ENTITY_TEARDOWN_RELEASE_ACTIVITY,
            s_entity_teardown_hooks.context);
        s_entity_teardown_hooks.operation(entity, ENTITY_TEARDOWN_RELEASE_AI,
            s_entity_teardown_hooks.context);
    }
    entity_drain_command_chain(entity);
    entity_pool_find_with_local_predicate(entity->owner_ctx, entity);
    if ((state_flags & 4U) != 0 || (state_flags & 2U) != 0) {
        if (s_entity_teardown_hooks.operation != NULL)
            s_entity_teardown_hooks.operation(entity,
                ENTITY_TEARDOWN_RELEASE_TYPE_DATA,
                s_entity_teardown_hooks.context);
        else task_callback_dispatch(entity);
    }
    /* EntityTeardown calls ObjectPointerRegistryClearMatch before releasing
     * the type-specific payload. Preserve each OList slot index as a hole. */
    object_pointer_registry_entity_remove(entity);
    if (s_entity_teardown_hooks.type_resource_release != NULL)
        s_entity_teardown_hooks.type_resource_release(entity, type,
            s_entity_teardown_hooks.context);
    if (s_entity_teardown_hooks.operation != NULL)
        s_entity_teardown_hooks.operation(entity, ENTITY_TEARDOWN_FINALIZE,
            s_entity_teardown_hooks.context);
    entity_path_motion_runtime_clear(entity);
    entity_linked_list_entries_remove_by_key(
        (uintptr_t)entity);
    entity_callback_dispatch_unless_type_five(entity);
    entity_action_consume_counter_sidecar_release(entity);
    if (s_entity_teardown_hooks.free_list_link != NULL)
        s_entity_teardown_hooks.free_list_link(entity,
            s_entity_teardown_free_head,
            s_entity_teardown_hooks.context);
    free(entity->wp_dynamic);
    entity->wp_dynamic = NULL;
    entity->wp_capacity = 0;
    entity->wp_count = 0;
    entity->wp_index = 0;
    entity->stats_record = NULL;
    entity->ddiscene_owner_token = 0u;
    entity->motion_inputs_initialized = 0u;
    entity->activity_advance_valid = 0u;
    active_entity_list_entity_remove(entity);
    entity->pool_link_408 = (uintptr_t)s_entity_teardown_free_head;
    s_entity_teardown_free_head = entity;
    return 0;
}
/* EntityPoolFindByCallbackFiltered (FUN_004377b0) fixes the filter on. */
/* Port model of 0x004377b0 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x004377b0, retires to the tail in GAME-023 part 3
int entity_pool_find_by_callback_filtered_current(
    int (*callback)(EntitySlot*, void*), void* payload)
{
    return entity_pool_find_by_callback_current(callback, payload, 1);
}
/* The retail pool walkers use the active context instead of receiving it as
 * an argument. Keep context-taking forms above as portable caller adapters. */
/* Port model of 0x004377d0 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x004377d0, retires to the tail in GAME-023 part 3
int entity_pool_find_by_callback_current(
    int (*callback)(EntitySlot*, void*), void* payload, int filter_flagged)
{
    GameContext* ctx = s_entity_pool_callback_context;
    int table;
    if (ctx == NULL) {
        return 0;
    }
    for (table = 0; table < ENTITY_TYPES; ++table) {
        int count = ctx->entity_count[table];
        int i;
        if (count < 0) {
            count = 0;
        }
        if (count > ENTITY_CAP) {
            count = ENTITY_CAP;
        }
        for (i = 0; i < count; ++i) {
            EntitySlot* entity = &ctx->entity[table][i];
            if (entity->pool_link_408 != (uintptr_t)entity) {
                continue;
            }
            if (filter_flagged != 0 &&
                (entity->flags_3e8 & 0x80000u) != 0u) {
                continue;
            }
            if (callback(entity, payload) != 0) {
                return 1;
            }
        }
    }
    return 0;
}

/* GridObjectRecordsRelease (FUN_004378e0): release both owned handles for
 * each active record, clear the fixed 200-word table, then release its side
 * structure through the installed owner callback. */
/* Port model of 0x004378e0 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x004378e0, retires to the tail in GAME-023 part 3
void grid_object_records_release(void)
{
    unsigned int i;
    for (i = 0; i < s_grid_object_record_count; ++i) {
        if (s_grid_object_records[i].second != NULL)
            engine_heap_free(s_grid_object_records[i].second);
        if (s_grid_object_records[i].first != NULL)
            engine_heap_free(s_grid_object_records[i].first);
        memset(&s_grid_object_records[i], 0,
               sizeof(s_grid_object_records[i]));
    }
    s_grid_object_record_count = 0u;
    memset(s_grid_object_record_words, 0, sizeof(s_grid_object_record_words));
    if (s_grid_object_records_release_tail != NULL) {
        s_grid_object_records_release_tail(
            s_grid_object_records_release_context);
    }
}

/* DeferredOListEntityCreateAndLink (FUN_00437940): realize all queued object
 * records at a cell, initialize each created entity, resolve named links
 * after creation, then release the consumed queue records. */
/* Port model of 0x00437940 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00437940, retires to the tail in GAME-023 part 3
void deferred_olist_entity_create_and_link(const int cell_xy[2])
{
    EntitySlot* created[200];
    DeferredOListEntityRecord active[200];
    DeferredOListEntityRecord pending[200];
    DeferredOListEntityRecord consumed[200];
    unsigned int created_count = 0u;
    unsigned int consumed_count = 0u;
    unsigned int write_index = 0u;
    unsigned int i;
    DeferredOListEntityHooks* const hooks = &s_deferred_olist_hooks;
    if (cell_xy == NULL) return;
    s_deferred_olist_record_count += deferred_olist_entity_queue_take_cell(
        cell_xy, s_deferred_olist_records + s_deferred_olist_record_count,
        200u - s_deferred_olist_record_count);
    for (i = 0; i < s_deferred_olist_record_count; ++i)
        pending[i] = s_deferred_olist_records[i];
    for (i = 0; i < s_deferred_olist_record_count; ++i) {
        DeferredOListEntityRecord* record = &pending[i];
        if (record->cell_x == cell_xy[0] && record->cell_y == cell_xy[1]) {
            EntitySlot* entity = NULL;
            if (record->entity_type == 4 &&
                hooks->special_building_create != NULL) {
                entity = hooks->special_building_create(
                    record, (unsigned int)record->variant, hooks->context);
            }
            else if (hooks->entity_create != NULL) {
                entity = hooks->entity_create(record, hooks->context);
            }
            if (entity != NULL) {
                if (record->entity_type != 4) {
                    entity_world_position_set(entity, record->x, record->y,
                                               record->heading, 1);
                }
                memcpy(&entity->field_340, &record->record_flags, 4);
                if ((record->entity_type == 6 || record->entity_type == 7 ||
                     record->entity_type == 8 || record->entity_type == 9) &&
                    hooks->control_initialize != NULL) {
                    hooks->control_initialize(entity, hooks->context);
                }
                entity->flags_3ec |= 0x200u;
                entity_kind_record_update_dispatch(entity);
                if (entity->type == 2 && hooks->type_two_event != NULL)
                    hooks->type_two_event(entity, hooks->context);
                if (entity->type == 11 &&
                    hooks->type_eleven_finalize != NULL) {
                    hooks->type_eleven_finalize(entity, hooks->context);
                }
                if (entity->type == 11) {
                    /* DeferredOListEntityCreateAndLink calls FUN_0040cf50
                     * for type 0xb after its type-specific initialization. */
                    register_object_at_grid_position(entity);
                }
                created[created_count] = entity;
                active[created_count] = *record;
                ++created_count;
            }
            consumed[consumed_count++] = *record;
            continue;
        }
        s_deferred_olist_records[write_index] = *record;
        ++write_index;
    }
    s_deferred_olist_record_count = write_index;
    for (i = 0; i < created_count; ++i) {
        unsigned int j;
        const char* target_name = active[i].link_name;
        if (target_name == NULL || target_name[0] == '\0') continue;
        for (j = 0; j < created_count; ++j) {
            unsigned int mode;
            EntitySlot* target;
            if (i == j || !deferred_olist_string_equal(
                    target_name, active[j].name)) continue;
            target = created[j];
            created[i]->link_2fc = target;
            created[i]->flags_3e4 |= 4u;
            if ((target->flags_3e0 & 0x100u) != 0u) mode = 1u;
            else if ((target->flags_3e0 & 0x1000000u) == 0u) mode = 4u;
            else if ((target->flags_3e0 & 0x2000000u) == 0u) mode = 2u;
            else mode = 1u;
            if (hooks->link_finalize != NULL) {
                hooks->link_finalize(created[i], target, mode,
                                     hooks->context);
            }
            break;
        }
    }
    if (hooks->record_release != NULL) {
        for (i = 0; i < consumed_count; ++i)
            hooks->record_release(&consumed[i], hooks->context);
    }
}

/* Port model of 0x00437c20 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00437c20, retires to the tail in GAME-023 part 3
void deferred_olist_entity_queue_add(GameContext* ctx, const uint32_t record[2],
                                     int p1, int p2, int p3, int p5, int p6,
                                     const char* name1, const char* name2)
{
    if (s_deferred_olist_next >= DEFERRED_OLIST_SLOTS) return;
    const uint32_t slot = s_deferred_olist_next++;
    if (record == NULL) return;
    DeferredOListEntry* e = &s_deferred_olist[slot];
    float x, y;
    __builtin_memcpy(&x, &record[0], sizeof(x));
    __builtin_memcpy(&y, &record[1], sizeof(y));
    int cell_x = float_to_long(x), cell_y = float_to_long(y);
    if (ctx != NULL && ctx->level != NULL)
        (void)terrain_world_xy_to_cell(ctx->level->ptr_terrain, x, y,
                                      &cell_x, &cell_y);
    e->w[0] = (uint32_t)cell_x;
    e->w[1] = (uint32_t)cell_y;
    e->w[2] = record[0];
    e->w[3] = record[1];
    e->w[4] = (uint32_t)p5; /* entry +0x10 */
    e->w[5] = (uint32_t)p1; /* entry +0x14 */
    e->w[6] = (uint32_t)p2; /* entry +0x18 */
    e->w[7] = (uint32_t)p3; /* entry +0x1c */
    e->w[8] = (uint32_t)p6; /* entry +0x20 */
    if (name1 != NULL) {
        e->names[0] = string_duplicate(name1); /* FUN_00477550 */
    }
    if (name2 != NULL) {
        e->names[1] = string_duplicate(name2); /* FUN_00477550 */
    }
}

/* EntityFlag40000ThresholdCompare (see entity.hpp). */
/* Port model of 0x00437cc0 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00437cc0, retires to the tail in GAME-023 part 3
int entity_flag40000_threshold_compare(const EntitySlot* e)
{
    if ((entity_state_flags_get(e) & 0x40000u) != 0u) {
        if (entity_stats_float_ate8(e) < e->field_340) {
            return 1;
        }
    }
    return 0;
}

/* PooledRecordAllocateAndReset (FUN_004382c0) — see entity.hpp. The
 * corpus pops the free-list head, clears 0x103 dwords, self-links the
 * link slot as its busy mark, and seeds the two 0xbf800000 float words
 * (the corpus's -1.5 pair). Deviations: the port's link slot is
 * native-size (the pool-stride convention), and the grow half answers
 * NULL when the allocator is empty where the corpus strides the word. */
/* Port model of 0x004382c0 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x004382c0, retires to the tail in GAME-023 part 3
void* pooled_record_allocate_and_reset(void)
{
    if (s_pool_record_head == NULL) {
        pooled_record_chunk_allocate_and_link();
    }
    char* record = (char*)s_pool_record_head;
    if (record == NULL) {
        return NULL; /* deviation: empty allocator (corpus strides) */
    }
    void* link = NULL;
    __builtin_memcpy(&link, record + POOL_RECORD_LINK_OFF, sizeof link);
    s_pool_record_head = link;
    memset(record, 0, 0x40cu); /* the corpus's 0x103-dword clear */
    __builtin_memcpy(record + POOL_RECORD_LINK_OFF, &record,
                     sizeof record); /* the busy self-link */
    const uint32_t seeded = 0xbf800000u;
    __builtin_memcpy(record + 0x2f4, &seeded, 4);
    __builtin_memcpy(record + 0x2f8, &seeded, 4);
    return record;
}
/* Port model of 0x00438310 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00438310, retires to the tail in GAME-023 part 3
void pooled_record_chunk_allocate_and_link(void)
{
    const uint32_t n =
        1u << ((uint32_t)(unsigned char)s_pool_chunk_count & 0x1fu);
    void* base = engine_heap_alloc((long)n * POOL_RECORD_STRIDE);
    if (base == NULL) {
        return; /* deviation: the corpus strides the allocator's word */
    }
    if (s_pool_chunk_count < POOL_CHUNK_COUNT_SLOTS) {
        s_pool_chunks[s_pool_chunk_count] = base;
    }
    s_pool_chunk_count = s_pool_chunk_count + 1u;
    char* cur = (char*)base;
    for (uint32_t k = 1; k < n; ++k) {
        void* next = cur + POOL_RECORD_STRIDE;
        __builtin_memcpy(cur + POOL_RECORD_LINK_OFF, &next, sizeof next);
        cur += POOL_RECORD_STRIDE;
    }
    __builtin_memcpy(cur + POOL_RECORD_LINK_OFF, &s_pool_record_head,
                     sizeof(s_pool_record_head));
    s_pool_record_head = base;
}
/* Port model of 0x004383a0 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x004383a0, retires to the tail in GAME-023 part 3
void entity_global_animation_state_set(uint32_t value)
{
    s_global_animation_state = value;
}
/* RoverWorkQueueUpdate (FUN_004383b0): apply the selected global work
 * position to every animation component in the rover's scene model. */
/* Port model of 0x004383b0 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x004383b0, retires to the tail in GAME-023 part 3
void rover_work_queue_update(EntitySlot* entity, int mode)
{
    RoverWorkQueueHooks* const hooks = &s_rover_work_queue_hooks;
    void* model;
    float position[3];
    unsigned int count;
    unsigned int i;
    if (entity == NULL || entity->type != 6 || hooks->model_get == NULL ||
        hooks->animation_count_get == NULL || hooks->position_get == NULL ||
        hooks->animation_position_set == NULL ||
        hooks->animation_update == NULL) return;
    model = hooks->model_get(entity, hooks->context);
    if (model == NULL) return;
    hooks->position_get(mode, position, hooks->context);
    count = hooks->animation_count_get(model, hooks->context);
    if (count > 4096u) count = 4096u;
    for (i = 0; i < count; ++i) {
        hooks->animation_position_set(model, i, position, 1.0f,
                                     hooks->context);
        hooks->animation_update(model, i, position, hooks->context);
    }
}
/* EntityKindRecordUpdateDispatch (FUN_00438460): preserve type-specific
 * update order, action transitions, event flags, and the two shared flag
 * updates while leaving engine-owned side effects behind typed callbacks. */
/* Port model of 0x00438460 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00438460, retires to the tail in GAME-023 part 3
void entity_kind_record_update_dispatch(EntitySlot* entity)
{
    EntityKindRecordUpdateHooks* const hooks =
        &s_entity_kind_record_update_hooks;
    int type;
    if (entity == NULL) return;
    type = entity->type;
    if (type == 4) {
        if (hooks->type_four_update != NULL)
            hooks->type_four_update(entity, hooks->context);
        entity->flags_3e8 |= 0x20u;
        if ((entity_stats_word_get(entity) & 8u) != 0u) {
            /* FUN_00438460's flag-8 branch calls TaskRecordCreateAndScanGrid
             * before its shared refresh. That helper creates command kind 9
             * and performs the production 0x403910 count-prune sweep. */
            task_record_create_and_scan_grid(entity);
            if (hooks->type_four_global_action != NULL)
                hooks->type_four_global_action(hooks->context);
        }
        else if ((entity_stats_word_get(entity) & 2u) != 0u) {
            timed_task_record_initialize_and_link(entity, 7, 0, 0);
            if (hooks->action_dispatch != NULL)
                hooks->action_dispatch(entity, 7, hooks->context);
        }
        else if ((entity_stats_word_get(entity) & 4u) != 0u) {
            timed_task_record_initialize_and_link(entity, 6, 0, 0);
            if (hooks->action_dispatch != NULL)
                hooks->action_dispatch(entity, 6, hooks->context);
        }
        else {
            entity->flags_3e8 &= ~0x20u;
        }
        if (hooks->global_refresh != NULL)
            hooks->global_refresh(hooks->context);
        entity->flags_3ec |= 0x200000u;
    }
    else if (type == 1 && hooks->type_one_record_update != NULL) {
        hooks->type_one_record_update(entity, hooks->context);
    }
    if (type == 1 || type == 2) {
        (void)entity_pool_find_with_local_predicate(entity->owner_ctx, entity);
    }
    if (hooks->event_emit != NULL) {
        hooks->event_emit(entity, type, entity->flags_3ec & 0x200u,
                          hooks->context);
    }
    if (type == 1 || type == 2) {
        scalar_task_record_queue(entity);
    }
    if (type == 4 && hooks->type_four_counter_increment != NULL) {
        hooks->type_four_counter_increment(hooks->context);
    }
    if ((type == 1 || type == 4)) entity->flags_3ec |= 0x40000u;
}

/* Port model of 0x00438580 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00438580, retires to the tail in GAME-023 part 3
void entity_flag80000000_set_or_clear(GameContext* ctx, EntitySlot* e,
                                      int param)
{
    if (e == NULL) {
        return; /* deviation: the corpus strides blindly */
    }
    if (param == 0) {
        e->flags_3e8 |= 0x80000000u;
    } else {
        e->flags_3e8 &= 0x7fffffffu;
    }
    entity_activity_scene_state_update(ctx);
}

// FUNCTION: LEGORR 0x004385b0
int entity_shared_eligibility_check(const EntitySlot* e, int relaxed)
{
    if ((e->flags_3e0 & 0x00700100u) != 0u) {
        return 0;
    }
    if ((e->flags_3e4 & 0x00100000u) != 0u) {
        return 0;
    }
    if (!(e->field_340 >= s_eligibility_floor)) {
        return 0;
    }
    if ((e->flags_3e0 & 0x00020000u) != 0u) {
        return 0;
    }
    if (relaxed != 0) {
        return 1;
    }
    if ((e->flags_3e8 & 0x20000000u) != 0u
        && (e->flags_3e8 & 0x80000000u) == 0u) {
        return 1;
    }
    return 0;
}
/* EntityCreateWithPosition (FUN_00438610): create through the pooled entity
 * allocator, apply the reference position update, then initialize the mode
 * word for pooled kinds six and seven. */
// FUNCTION: LEGORR 0x00438610
EntitySlot* entity_create_with_position(GameContext* context, int type,
                                        const char* name, int value,
                                        float x, float y, float heading)
{
    return entity_create_with_position_subtype(context, type, 0, name, value,
                                               x, y, heading);
}
/* EntityAtGridPositionLookup (FUN_00438670): the callback pool walk returns
 * the first eligible entity whose world position rounds to the requested
 * grid cell. The portable entity tables replace the corpus's raw chunk scan. */
// FUNCTION: LEGORR 0x00438670
EntitySlot* entity_at_grid_position_find(const int cell_xy[2])
{
    EntityGridFindQuery query;
    if (cell_xy == NULL || s_entity_pool_callback_context == NULL ||
        s_entity_pool_callback_context->level == NULL) {
        return NULL;
    }
    query.x = cell_xy[0];
    query.y = cell_xy[1];
    query.level = s_entity_pool_callback_context->level;
    query.found = NULL;
    entity_pool_find_by_callback_filtered_current(
        entity_grid_position_match, &query);
    return query.found;
}

/* EntityAvailableResourceSpend (FUN_00438720) — see level.hpp.
 * The corpus's comparison value is the unconsumed EAX of the void no-op
 * callback; pinning it to 0 matches the corpus call's observable shape. */
// FUNCTION: LEGORR 0x00438720
int entity_available_resource_spend(LevelState* level, int amount,
                                    const void* resource_hint)
{
    if (level != NULL && level_int(level, 0x1f8) != 0) {
        amount = 0;
    }
    const int probe = 0; /* FUN_0041f8e0's void call: pinned residue */
    if (probe < amount) {
        message_dispatch(0xe, 0, 0, resource_hint);
        return 0;
    }
    global_object_field98_add_for_level(level, amount);
    return 1;
}

/* BuildingUpgradeResourceAdmissionCheck (FUN_00438770): reject the terminal
 * upgrade level, then apply the two-resource threshold and return the
 * secondary debit amount when that path is selected. */
// FUNCTION: LEGORR 0x00438770
int building_upgrade_resource_admission_check(EntitySlot* building,
                                               uint32_t* out_secondary_cost)
{
    BuildingUpgradeAdmissionHooks* const hooks =
        &s_building_upgrade_admission_hooks;
    uint32_t first_cost;
    uint32_t second_cost;
    if (out_secondary_cost != NULL) *out_secondary_cost = 0u;
    if (building == NULL) return 0;
    if (hooks->maximum_level_get == NULL || hooks->current_level_get == NULL ||
        hooks->cost_get == NULL || hooks->resource_get == NULL) {
        GameContext* context = building_upgrade_context_get(building);
        uint32_t secondary_resource = 0u;
        uint32_t amount = 0u;
        uint32_t maximum;
        if (context == NULL || context->level == NULL) return 0;
        maximum = building_upgrade_admission_check(
            building->type, building->etype);
        if (maximum <= (uint32_t)building->indexed_record_slot_330 + 1u)
            return 0;
        if (!building_upgrade_live_payment_select(
                building, &secondary_resource, &amount)) return 0;
        if (out_secondary_cost != NULL && secondary_resource != 0u)
            *out_secondary_cost = amount;
        return 1;
    }
    if (hooks->maximum_level_get(building, hooks->context) <=
        hooks->current_level_get(building, hooks->context) + 1u) return 0;
    first_cost = hooks->cost_get(0u, hooks->context);
    second_cost = hooks->cost_get(1u, hooks->context);
    if (hooks->resource_get(0u, hooks->context) < first_cost) {
        if (hooks->resource_get(1u, hooks->context) < second_cost) return 0;
        if (out_secondary_cost != NULL) *out_secondary_cost = second_cost;
        return 1;
    }
    return 1;
}

/* BuildingUpgradeCommit (FUN_004387f0): spend the selected resource, clear
 * the pending request, advance the level in payload/stats, and dispatch the
 * upgrade's world effect. */
// FUNCTION: LEGORR 0x004387f0
void building_upgrade_commit(EntitySlot* building)
{
    BuildingUpgradeCommitHooks* const hooks =
        &s_building_upgrade_commit_hooks;
    uint32_t secondary_cost = 0u;
    unsigned int next_level;
    unsigned int resource;
    uint32_t amount;
    int previous_level;
    GameContext* context;
    int hook_payment;
    if (building == NULL || building->type != 4 ||
        !building_upgrade_resource_admission_check(building,
                                                   &secondary_cost)) {
        return;
    }
    context = building_upgrade_context_get(building);
    hook_payment = s_building_upgrade_admission_hooks.cost_get != NULL &&
        s_building_upgrade_admission_hooks.resource_get != NULL &&
        s_building_upgrade_admission_hooks.current_level_get != NULL &&
        s_building_upgrade_admission_hooks.maximum_level_get != NULL;
    resource = 0u;
    amount = 0u;
    if (hook_payment) {
        amount = s_building_upgrade_admission_hooks.cost_get(
            0u, s_building_upgrade_admission_hooks.context);
        if (s_building_upgrade_admission_hooks.resource_get(
                0u, s_building_upgrade_admission_hooks.context) < amount) {
            resource = 1u;
            amount = s_building_upgrade_admission_hooks.cost_get(
                1u, s_building_upgrade_admission_hooks.context);
        }
    }
    else if (!building_upgrade_live_payment_select(building, &resource,
                                                    &amount)) return;
    previous_level = building->indexed_record_slot_330;
    next_level = (unsigned int)previous_level + 1u;
    if (hooks->resource_spend != NULL)
        hooks->resource_spend(building, resource, amount, hooks->context);
    else if (context != NULL && context->level != NULL)
        upgrade_resource_debit((int)resource, (int)amount);
    building->activity_counter_3d8 = 0.0f;
    if (hooks->upgrade_request_clear != NULL)
        hooks->upgrade_request_clear(building, hooks->context);
    if (hooks->payload_level_set != NULL)
        hooks->payload_level_set(building, next_level, hooks->context);
    else
        entity_payload_state_set(building, (int)next_level);
    if (hooks->stats_level_set != NULL)
        hooks->stats_level_set(building, next_level, hooks->context);
    else
        (void)entity_animation_state_set(building, next_level);
    if (hooks->upgrade_effect_dispatch != NULL)
        hooks->upgrade_effect_dispatch(building, next_level, hooks->context);
    else {
        float position[3];
        if (entity_world_center_position_read(building, position))
            (void)lava_effect_select_and_place(9, position, NULL);
    }
    lego::building_upgrade_state_changed_dispatch(
        building->type, building->etype,
        previous_level, 0);
}
/* EntityActionKindPrerequisiteCheck (FUN_004388b0): require a type-one
 * entity and a valid action, then gate the two resource balances against
 * the corpus's packed primary/secondary action costs. */
// FUNCTION: LEGORR 0x004388B0
int entity_action_kind_prerequisite_check(EntitySlot* entity, int action)
{
    uint32_t values[3] = { 0u, 0u, 0u };
    uint32_t first_resource;
    uint32_t second_resource;
    uint32_t secondary_cost;
    GameContext* context;
    EntityActionPrerequisiteHooks* const hooks =
        &s_entity_action_prerequisite_hooks;
    if (entity == NULL || entity->type != 1) return 0;
    context = entity->owner_ctx;
    if (hooks->action_ready != NULL) {
        if (!hooks->action_ready(entity, action, hooks->context)) return 0;
    }
    else if ((record_flag_bit_test(
                  entity->attached_direction_record_type1,
                  (uint32_t)action, 1) & (1u << ((uint32_t)action & 31u))) == 0u) {
        return 0;
    }
    if (hooks->action_values_get != NULL) {
        if (!hooks->action_values_get(entity, action, values,
                                      hooks->context)) return 0;
    }
    else {
        values[0] = entity_mapped_indexed_record_word_at_fc_get(
            entity->type, entity->etype, action);
        values[1] = entity_mapped_indexed_record_word_at_fc_get(
            entity->type, entity->etype, action);
        values[2] = entity_mapped_indexed_record_word_at_fc_get(
            entity->type, entity->etype, action);
    }
    first_resource = hooks->resource_get != NULL
        ? hooks->resource_get(0u, hooks->context)
        : (context != NULL
            ? upgrade_resource_check(context->level, 0) : 0u);
    if (first_resource < values[2]) {
        second_resource = hooks->resource_get != NULL
            ? hooks->resource_get(1u, hooks->context)
            : (context != NULL
                ? upgrade_resource_check(context->level, 1) : 0u);
        secondary_cost = values[1] / 5u +
                         (values[0] % 5u != 0u ? 1u : 0u);
        if (second_resource < secondary_cost) return 0;
    }
    return 1;
}

/* EntityRouteMarkerCandidateFind (FUN_004389e0): retain the nearest pooled
 * entity within the corpus's 10,000-unit radius that passes LAB_00438fb0's
 * mask-selected predicates. */
// FUNCTION: LEGORR 0x004389E0
EntitySlot* entity_route_marker_candidate_find(EntitySlot* source,
                                               int include_secondary)
{
    EntityRouteMarkerSearchRequest request;
    GameContext* context;
    float position[2];
    if (source == NULL) return NULL;
    entity_transform_position_get(source, position);
    request.match = NULL;
    request.flags = 0x1000u | (include_secondary != 0 ? 0x400u : 0u);
    request.position = position;
    request.best_distance = 10000.0f;
    request.cell_x = 0;
    request.cell_y = 0;
    request.action_argument = 0;
    request.requested_state = 0;
    context = entity_pool_callback_context_get();
    if (context != NULL) {
        entity_pool_find_by_callback_filtered_current(
            entity_route_marker_candidate_visit, &request);
    }
    return request.match;
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x00438a60
void* ledger_stub_00438a60_SpatialObjectQueryCenterMask01(
    const int cell_xy[2], uint32_t filter_a, uint32_t filter_b,
    uint32_t filter_c)
{
    SpatialObjectQueryRequest request;
    if (cell_xy == NULL) return NULL;
    spatial_query_request_init(&request, 1u);
    spatial_query_center_from_cell(&request, cell_xy);
    request.filters[0] = filter_a;
    request.filters[1] = filter_b;
    request.filters[2] = filter_c;
    return entity_spatial_query_execute_current(&request);
}

// FUNCTION: LEGORR 0x00438ae0
void* ledger_stub_00438ae0_SpatialObjectQueryCenterMask02(
    const EntitySlot* entity, const int cell_xy[2], uint32_t filter_a,
    uint32_t filter_b)
{
    SpatialObjectQueryRequest request;
    spatial_query_request_init(&request, 2u);
    if (entity != NULL) spatial_query_center_from_entity(&request, entity);
    else spatial_query_center_from_cell(&request, cell_xy);
    request.filters[0] = filter_a;
    /* Retail locals +0x10 and +0x8 map to context words 0 and 2; word 1
     * stays zero. */
    request.filters[2] = filter_b;
    return entity_spatial_query_execute_current(&request);
}

// FUNCTION: LEGORR 0x00438b80
void* ledger_stub_00438b80_SpatialObjectQueryCenterMaskFromArgument(
    const EntitySlot* entity, uint32_t mask)
{
    SpatialObjectQueryRequest request;
    spatial_query_request_init(&request, mask);
    spatial_query_center_from_entity(&request, entity);
    return entity_spatial_query_execute_current(&request);
}

// FUNCTION: LEGORR 0x00438bf0
void* ledger_stub_00438bf0_SpatialObjectQueryCenterMask80(
    const EntitySlot* entity)
{
    SpatialObjectQueryRequest request;
    spatial_query_request_init(&request, 0x80u);
    spatial_query_center_from_entity(&request, entity);
    return entity_spatial_query_execute_current(&request);
}

// FUNCTION: LEGORR 0x00438c60
void* ledger_stub_00438c60_EntityCandidateSearch(const EntitySlot* entity)
{
    SpatialObjectQueryRequest request;
    spatial_query_request_init(&request, 4u);
    spatial_query_center_from_entity(&request, entity);
    return entity_spatial_query_execute_current(&request);
}

// FUNCTION: LEGORR 0x00438cd0
void* ledger_stub_00438cd0_SpatialObjectQueryCenterMask20(
    const float center[2])
{
    EntityRouteMarkerSearchRequest request;
    float position[2];
    position[0] = center[0];
    position[1] = center[1];
    request.position = position;
    request.flags = 0x20u;
    request.best_distance = 10000.0f;
    request.cell_x = 0;
    request.cell_y = 0;
    request.action_argument = 0;
    request.requested_state = 0;
    request.match = NULL;
    entity_pool_find_by_callback_filtered_current(
        entity_route_marker_candidate_visit, &request);
    return request.match;
}

// FUNCTION: LEGORR 0x00438d30
void* ledger_stub_00438d30_SpatialObjectQueryCenterMask10(
    const float center[2])
{
    EntityRouteMarkerSearchRequest request;
    float position[2];
    position[0] = center[0];
    position[1] = center[1];
    request.position = position;
    request.flags = 0x10u;
    request.best_distance = 10000.0f;
    request.cell_x = 0;
    request.cell_y = 0;
    request.action_argument = 0;
    request.requested_state = 0;
    request.match = NULL;
    entity_pool_find_by_callback_filtered_current(
        entity_route_marker_candidate_visit, &request);
    return request.match;
}

// FUNCTION: LEGORR 0x00438d90
void* ledger_stub_00438d90_SpatialObjectQueryCenterMask200(
    const float center[2])
{
    EntityRouteMarkerSearchRequest request;
    float position[2];
    position[0] = center[0];
    position[1] = center[1];
    request.position = position;
    request.flags = 0x200u;
    request.best_distance = 10000.0f;
    request.cell_x = 0;
    request.cell_y = 0;
    request.action_argument = 0;
    request.requested_state = 0;
    request.match = NULL;
    entity_pool_find_by_callback_filtered_current(
        entity_route_marker_candidate_visit, &request);
    return request.match;
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x00438df0
EntitySlot* entity_lookup_at_world_position(const float world_position[2])
{
    struct EntityLookupRequestRecord {
        uint32_t result_word;
        uint32_t center_x_bits;
        uint32_t center_y_bits;
        uint32_t radius_bits;
        uint32_t mask;
        uint32_t filters[4];
        EntitySlot* native_result;
    } record;
    SpatialObjectQueryRequest query;
    if (world_position == NULL) return NULL;

    /* FUN_00438df0's callback context is exactly nine retail dwords:
     * result, center x/y, radius, mask, and four zero filters. Keep that
     * record shape explicit; the native pointer result remains a sidecar. */
    memset(&record, 0, sizeof(record));
    memcpy(&record.center_x_bits, &world_position[0], sizeof(uint32_t));
    memcpy(&record.center_y_bits, &world_position[1], sizeof(uint32_t));
    record.radius_bits = 0x461c4000u; /* exact retail 10,000.0f */
    record.mask = 8u;
    memcpy(query.center, &record.center_x_bits, sizeof(query.center));
    memcpy(&query.radius, &record.radius_bits, sizeof(query.radius));
    query.mask = record.mask;
    memcpy(query.filters, record.filters, sizeof(query.filters));
    query.center_is_cell = 0;
    /* The reference calls EntityPoolFindByCallbackFiltered directly. Use the
     * same active-pool path as the mask wrappers, rather than depending on an
     * optional level-module spatial-query hook being installed. */
    record.native_result = (EntitySlot*)entity_spatial_query_execute_current(
        &query);
    if (sizeof(void*) == sizeof(uint32_t))
        record.result_word = (uint32_t)(uintptr_t)record.native_result;
    return record.native_result;
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x00438e50
void* ledger_stub_00438e50_SpatialObjectQueryCenterMask100(
    const EntitySlot* entity, const float center[2], uint32_t extra)
{
    SpatialObjectQueryRequest request;
    spatial_query_request_init(&request, 0x100u);
    if (center != NULL) {
        request.center[0] = center[0];
        request.center[1] = center[1];
    }
    spatial_query_center_from_entity(&request, entity);
    /* The extra argument initializes the final callback-context word. */
    request.filters[3] = extra;
    return entity_spatial_query_execute_current(&request);
}
} // namespace unported_ledger_stubs

/* EntityFlagsContainRequestedSingleBit (FUN_00438ed0, findings 327/449).
 * The corpus re-reads the word at each chain rung; the selector arms and
 * the flag bits pair one-to-one, so one read through the switch is the
 * identical answer. */
// FUNCTION: LEGORR 0x00438ed0
int entity_flags_contain_requested_single_bit(const EntitySlot* e,
                                              int selector)
{
    if ((entity_state_flags_get(e) & 0x04u) == 0u || selector != 1) {
        if ((entity_state_flags_get(e) & 0x08u) == 0u || selector != 2) {
            if ((entity_state_flags_get(e) & 0x10u) == 0u || selector != 4) {
                if ((entity_state_flags_get(e) & 0x20u) == 0u || selector != 8) {
                    if ((entity_state_flags_get(e) & 0x40u) == 0u
                        || selector != 0x10) {
                        if ((entity_state_flags_get(e) & 0x80u) == 0u
                            || selector != 0x20) {
                            return 0;
                        }
                    }
                }
            }
        }
    }
    return 1;
}

/* Port model of 0x00438f60 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00438f60, retires to the tail in GAME-023 part 3
int dock_definition_matches_cell(const int cell_xy[2])
{
    int dock_xy[2];
    if (cell_xy == NULL || s_dock_definition_cell_get_hook == NULL) return 0;
    if (!s_dock_definition_cell_get_hook(dock_xy,
                                         s_dock_definition_cell_get_context)) {
        return 0;
    }
    return dock_xy[0] == cell_xy[0] && dock_xy[1] == cell_xy[1];
}

/* Port model of 0x00438fb0 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00438fb0, retires to the tail in GAME-023 part 3
int entity_route_marker_candidate_visit(EntitySlot* candidate,
                                                void* payload)
{
    EntityRouteMarkerSearchRequest* request =
        (EntityRouteMarkerSearchRequest*)payload;
    float candidate_position[2];
    float dx, dy, distance;
    if (request == NULL || candidate == NULL) return 0;
    if (!entity_route_marker_query_match(candidate, request)) return 0;
    if (s_entity_route_marker_candidate_hook != NULL &&
        !s_entity_route_marker_candidate_hook(
            candidate, request->position, request->flags,
            s_entity_route_marker_candidate_context)) return 0;
    entity_transform_position_get(candidate, candidate_position);
    dx = candidate_position[0] - request->position[0];
    dy = candidate_position[1] - request->position[1];
    distance = (float)sqrt((double)(dx * dx + dy * dy));
    if (distance < request->best_distance) {
        request->match = candidate;
        request->best_distance = distance;
    }
    /* LAB_00438fb0 always returns zero: retain the nearest match while the
     * pool walk visits every otherwise eligible entity. */
    return 0;
}

/* Port model of 0x00439200 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439200, retires to the tail in GAME-023 part 3
int task_kind_filtered_search(int selector)
{
    if (selector == 1 || selector == 2 || selector == 4) {
        return 0;
    }
    if (s_ctx == NULL) {
        return 0; /* deviation: the corpus walks the pool blindly */
    }
    for (int t = 0; t < ENTITY_TYPES; ++t) {
        for (int i = 0; i < s_ctx->entity_count[t]; ++i) {
            const EntitySlot* e = entity_at(s_ctx, t + 1, i);
            if (e == NULL || e->type == 0) {
                continue; /* walker liveness: the +0x408 slot mirror */
            }
            if (task_entity_flags_and_cell_task_match(e, selector) != 0) {
                return 1;
            }
        }
    }
    return 0;
}
/* Port model of 0x00439240 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439240, retires to the tail in GAME-023 part 3
int task_entity_flags_and_cell_task_match(const EntitySlot* e, int kind)
{
    /* Corpus: test al,0x8 — the byte test of the stats word return. */
    if (((unsigned char)entity_stats_word_get(e) & 0x8u) != 0u) {
        if (entity_action_kind_supported((EntitySlot*)e, kind, 0) != 0) {
            return 1; /* corpus: mov eax,0x1 ahead of the shared return-0 tail */
        }
    }
    return 0;
}

/* EntityBuildPlacementCommandBatchExpand (FUN_00439280): parse four counts,
 * then expand selectors 0/1 into the two command-setup families. The four
 * values are the outputs of the level's byte-state classifier at 0x42c5b0,
 * so this path uses the live level record instead of requiring a separately
 * installed count callback. */
/* Port model of 0x00439280 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439280, retires to the tail in GAME-023 part 3
void entity_build_placement_command_batch_expand(const int cell_xy[2])
{
    uint32_t counts[4];
    uint32_t i;
    GameContext* context = entity_pool_callback_context_get();
    if (cell_xy == NULL || context == NULL || context->level == NULL ||
        !level_cell_byte_state_classify(context->level, cell_xy[0],
            cell_xy[1], &counts[0], &counts[1], &counts[2], &counts[3])) {
        return;
    }
    for (i = 0; i < counts[0]; ++i) {
        (void)entity_build_placement_command_setup(cell_xy, 0, NULL, 1, 0);
    }
    for (i = 0; i < counts[1]; ++i) {
        (void)entity_build_placement_command_setup(cell_xy, 1, NULL, 1, 0);
    }
    for (i = 0; i < counts[2]; ++i) {
        (void)entity_build_placement_command_setup(cell_xy, 0, NULL, 1, 1);
    }
    for (i = 0; i < counts[3]; ++i) {
        (void)entity_build_placement_command_setup(cell_xy, 1, NULL, 1, 1);
    }
}
/* Port model of 0x00439340 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439340, retires to the tail in GAME-023 part 3
void entity_build_placement_command_pair_create(const int cell_xy[2],
                                                int entity_id)
{
    unported_ledger_stubs::ledger_stub_00439370_EntityBuildPlacementCommandSetup(
        cell_xy, entity_id, NULL, 1);
    unported_ledger_stubs::ledger_stub_004394b0_EntityBuildPlacementAlternateCommandSetup(
        cell_xy, entity_id, NULL, 1);
}

namespace unported_ledger_stubs {
// EntityBuildPlacementCommandSetup
/* Port model of 0x00439370 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439370, retires to the tail in GAME-023 part 3
void ledger_stub_00439370_EntityBuildPlacementCommandSetup(
    const int cell_xy[2], int entity_id, const float world_xy[2],
    int update_state)
{
    EntityBuildPlacementHooks* hooks = &s_entity_build_placement_hooks;
    GameContext* context = s_entity_build_placement_context != NULL
        ? s_entity_build_placement_context
        : entity_pool_callback_context_get();
    EntitySlot* created;
    uint8_t* cell_record;
    float position[2];
    float cell_position[3];

    if (cell_xy == NULL) return;
    if (world_xy != NULL) {
        position[0] = world_xy[0];
        position[1] = world_xy[1];
    }
    else if (hooks->position_choose != NULL) {
        if (!hooks->position_choose(hooks->context, cell_xy, position))
            return;
    }
    else {
        TerrainSurfaceGrid* grid = path_record_surface_grid_get();
        float vertices[12];
        if (context != NULL && context->level != NULL &&
            context->level->ptr_terrain != NULL && grid != NULL &&
            cell_xy[0] >= 0 && cell_xy[1] >= 0 &&
            (uint32_t)cell_xy[0] < grid->width &&
            (uint32_t)cell_xy[1] < grid->height &&
            terrain_cell_surface_vertices_get(grid, (uint32_t)cell_xy[0],
                                               (uint32_t)cell_xy[1],
                                               vertices) >= 0) {
            const float half_cell = terrain_context_scale_get(
                context->level->ptr_terrain);
            const float quarter_cell = terrain_context_scale_get(
                context->level->ptr_terrain);
            const float inset = half_cell * 0.5f - quarter_cell * 0.25f;
            position[0] = random_float_range(vertices[0] + inset,
                                              vertices[6] - inset);
            position[1] = random_float_range(vertices[1] - inset,
                                              vertices[7] + inset);
        }
        else {
            if (!terrain_cell_world_position_get(cell_xy[0], cell_xy[1],
                                                 cell_position)) return;
            position[0] = cell_position[0];
            position[1] = cell_position[1];
        }
    }
    if (hooks->entity_create != NULL)
        created = hooks->entity_create(hooks->context, 6, 0, entity_id,
                                       position[0], position[1]);
    else
        created = entity_create_with_position(context, 6, NULL, entity_id,
                                              position[0], position[1], 0.0f);
    if (created == NULL) return;
    if (hooks->queue_entity != NULL)
        hooks->queue_entity(hooks->context, 0x16, created);
    else
        cmd_enqueue(0x16, created, 0, 0, 0);

    cell_record = context != NULL && context->level != NULL
        ? level_voxel(context->level, cell_xy[0], cell_xy[1]) : NULL;
    if (update_state) {
        if (hooks->event_raise != NULL)
            hooks->event_raise(hooks->context, 0x11, cell_xy);
        else {
            message_dispatch(0, 0, 0, cell_xy);
            advisor_text_animation_update(0x11, 1, 0);
        }
        if (hooks->cell_counter_increment != NULL)
            hooks->cell_counter_increment(hooks->context, cell_xy, 0x2c);
        else if (cell_record != NULL)
            ++*(uint32_t*)(cell_record + 0x2c);
    }
}

// EntityBuildPlacementAlternateCommandSetup
/* Port model of 0x004394b0 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x004394b0, retires to the tail in GAME-023 part 3
void ledger_stub_004394b0_EntityBuildPlacementAlternateCommandSetup(
    const int cell_xy[2], int entity_id, const float world_xy[2],
    int update_state)
{
    EntityBuildPlacementHooks* hooks = &s_entity_build_placement_hooks;
    GameContext* context = s_entity_build_placement_context != NULL
        ? s_entity_build_placement_context
        : entity_pool_callback_context_get();
    EntitySlot* created;
    uint8_t* cell_record;
    float position[2];
    float cell_position[3];

    if (cell_xy == NULL) return;
    if (world_xy != NULL) {
        position[0] = world_xy[0];
        position[1] = world_xy[1];
    }
    else if (hooks->position_choose != NULL) {
        if (!hooks->position_choose(hooks->context, cell_xy, position))
            return;
    }
    else {
        TerrainSurfaceGrid* grid = path_record_surface_grid_get();
        float vertices[12];
        if (context != NULL && context->level != NULL &&
            context->level->ptr_terrain != NULL && grid != NULL &&
            cell_xy[0] >= 0 && cell_xy[1] >= 0 &&
            (uint32_t)cell_xy[0] < grid->width &&
            (uint32_t)cell_xy[1] < grid->height &&
            terrain_cell_surface_vertices_get(grid, (uint32_t)cell_xy[0],
                                               (uint32_t)cell_xy[1],
                                               vertices) >= 0) {
            const float half_cell = terrain_context_scale_get(
                context->level->ptr_terrain);
            const float quarter_cell = terrain_context_scale_get(
                context->level->ptr_terrain);
            const float inset = half_cell * 0.5f - quarter_cell * 0.25f;
            position[0] = random_float_range(vertices[0] + inset,
                                              vertices[6] - inset);
            position[1] = random_float_range(vertices[1] - inset,
                                              vertices[7] + inset);
        }
        else {
            if (!terrain_cell_world_position_get(cell_xy[0], cell_xy[1],
                                                 cell_position)) return;
            position[0] = cell_position[0];
            position[1] = cell_position[1];
        }
    }
    if (hooks->entity_create != NULL)
        created = hooks->entity_create(hooks->context, 7, 0, entity_id,
                                       position[0], position[1]);
    else
        created = entity_create_with_position(context, 7, NULL, entity_id,
                                              position[0], position[1], 0.0f);
    if (created == NULL) return;
    if (hooks->queue_entity != NULL)
        hooks->queue_entity(hooks->context, 0x1c, created);
    else
        cmd_enqueue(0x1c, created, 0, 0, 0);

    if (hooks->event_raise != NULL)
        hooks->event_raise(hooks->context, 0x13, cell_xy);
    else
        advisor_text_animation_update(0x13, 1, 0);
    cell_record = context != NULL && context->level != NULL
        ? level_voxel(context->level, cell_xy[0], cell_xy[1]) : NULL;
    if (update_state) {
        if (hooks->cell_counter_increment != NULL)
            hooks->cell_counter_increment(hooks->context, cell_xy, 0x30);
        else if (cell_record != NULL)
            ++*(uint32_t*)(cell_record + 0x30);
    }
}
} // namespace unported_ledger_stubs

/* EntityDisplayNameLookup (FUN_004395e0) receives an EntitySlot*. Its
 * category-specific index is represented by the config type name in the
 * portable entity record; kind 7's second-word selector is retained as
 * etype, matching the original +4 read. */
/* Port model of 0x004395e0 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x004395e0, retires to the tail in GAME-023 part 3
const char* entity_display_name_lookup(const EntitySlot* entity)
{
    int index;
    if (entity == NULL || entity->type < 0) return NULL;
    if ((entity->type >= 1 && entity->type <= 4) || entity->type == 10) {
        index = entity_display_name_config_index(entity);
        return index >= 0
            ? localized_object_name_get((uint32_t)entity->type, index)
            : NULL;
    }
    if (entity->type == 7) {
        /* The retail table base is DAT_00556190, which is special-name slot
         * 1; the config table's first row is therefore entity selector 0. */
        if (entity->etype < 0 || entity->etype >= 8) return NULL;
        return localized_object_name_get(7u, entity->etype);
    }
    switch (entity->type) {
    case 6:
    case 8:
    case 9:
    case 11:
    case 12:
    case 13:
    case 15:
        return localized_object_name_get((uint32_t)entity->type, 0);
    default:
        return NULL;
    }
}
/* ItemNameByKindLookup (FUN_004396c0): fixed item labels plus the four
 * runtime-configured vehicle/minifigure/creature/building name tables. */
/* Port model of 0x004396c0 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x004396c0, retires to the tail in GAME-023 part 3
const char* item_name_by_kind_lookup(uint32_t kind, int index)
{
    static const char* const special_names[] = {
        "", "", "", "", "", "", s_item_name_power_crystal, "",
        s_item_name_dynamite, s_item_name_barrier, "",
        s_item_name_electric_fence, s_item_name_spider_web,
        s_item_name_ooh_scary
    };
    if (kind >= 1u && kind <= 4u) {
        if (index < 0 || (uint32_t)index >= s_entity_kind_name_counts[kind]) {
            return NULL;
        }
        return s_entity_kind_names[kind] != NULL
            ? s_entity_kind_names[kind] + (size_t)index * 128u : NULL;
    }
    if (kind == 7u) {
        return index == 0 ? s_item_name_ore : s_item_name_processed_ore;
    }
    if (kind == 0xffffffffu) {
        return s_item_name_tv_camera;
    }
    if (kind < sizeof(special_names) / sizeof(special_names[0])
        && special_names[kind][0] != '\0') {
        return special_names[kind];
    }
    return NULL;
}

/* Port model of 0x00439790 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439790, retires to the tail in GAME-023 part 3
const char* localized_object_the_name_get(uint32_t kind, int index)
{
    switch (kind) {
    case 1: return s_object_the_name_kind1[index];
    case 2: return s_object_the_name_kind2[index];
    case 3: return s_object_the_name_kind3[index];
    case 4: return s_object_the_name_kind4[index];
    case 6: return s_object_the_name_special[0];
    case 7: return s_object_the_name_special[index + 1];
    case 8: return s_object_the_name_special[2];
    case 9: return s_object_the_name_special[3];
    case 10: return s_object_the_name_kind10[index];
    case 11: return s_object_the_name_special[5];
    case 12: return s_object_the_name_special[6];
    case 13: return s_object_the_name_special[7];
    case 15: return s_object_the_name_special[8];
    default: return NULL;
    }
}
/* Port model of 0x00439860 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439860, retires to the tail in GAME-023 part 3
const char* localized_object_name_get(uint32_t kind, int index)
{
    switch (kind) {
    case 1: return s_object_name_kind1[index];
    case 2: return s_object_name_kind2[index];
    case 3: return s_object_name_kind3[index];
    case 4: return s_object_name_kind4[index];
    case 6: return s_object_name_special[0];
    case 7: return s_object_name_special[index + 1];
    case 8: return s_object_name_special[2];
    case 9: return s_object_name_special[3];
    case 10: return s_object_name_kind10[index];
    case 11: return s_object_name_special[5];
    case 12: return s_object_name_special[6];
    case 13: return s_object_name_special[7];
    case 15: return s_object_name_special[8];
    default: return NULL;
    }
}

/* CopyTwoDwordPairToOutputs (FUN_00439930) — see record.hpp. */
/* Port model of 0x00439930 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439930, retires to the tail in GAME-023 part 3
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void copy_two_dword_pair_to_outputs(
    const void* record, uint32_t* out_a, uint32_t* out_b)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov edx, dword ptr [esp + 8]
        mov ecx, dword ptr [eax]
        mov dword ptr [edx], ecx
        mov ecx, dword ptr [esp + 0ch]
        mov eax, dword ptr [eax + 4]
        mov dword ptr [ecx], eax
        ret
    }
}
#else
void copy_two_dword_pair_to_outputs(const void* record, uint32_t* out_a,
                                    uint32_t* out_b)
{
    if (record == NULL || out_a == NULL || out_b == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    __builtin_memcpy(out_a, (const char*)record, 4);
    __builtin_memcpy(out_b, (const char*)record + 4, 4);
}
#endif

/* ToolNameStringResourcesLoad (FUN_00439950): load the eleven configured
 * entries from the ToolNames category into the global indexed handle table. */
/* Port model of 0x00439950 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439950, retires to the tail in GAME-023 part 3
void entity_tool_name_strings_load(void* string_loader, void* config)
{
    BlockFile* cfg = (BlockFile*)string_loader;
    const char* config_root = (const char*)config;
    int slot;
    for (slot = 0; slot < 11; ++slot) {
        if (s_entity_tool_name_load_hook != NULL) {
            s_entity_tool_name_handles[slot] = s_entity_tool_name_load_hook(
                string_loader, config, "ToolNames", slot,
                s_entity_tool_name_context);
        }
        else {
            const char* tool_name = ai_tool_type_name_get(slot);
            if (cfg != NULL && config_root != NULL) {
                const char* path = string_path_join(config_root, "ToolNames",
                                                    tool_name, (char*)NULL);
                char* value = block_get_str_copy(cfg, path);
                s_entity_tool_name_handles[slot] = value;
            }
            else {
                s_entity_tool_name_handles[slot] = NULL;
            }
        }
    }
}

/* Port model of 0x00439990 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439990, retires to the tail in GAME-023 part 3
void entity_activity_scene_state_update_retail(void)
{
    if ((s_entity_state_flags & 0x10u) != 0u) {
        s_entity_state_flags |= 8u;
        return;
    }
    s_entity_state_flags |= 4u;
    global_object_field98_clear_current();
}

/* EntityFlag400AndHealthThresholdCheck (FUN_004399c0): require flag 0x400;
 * type-1 entities also pass the corpus's indexed-count and 75-percent gate. */
/* Port model of 0x004399c0 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x004399c0, retires to the tail in GAME-023 part 3
int entity_flag400_and_health_threshold_check(const EntitySlot* entity)
{
    EntityHealthThresholdState state;
    uint32_t value;
    if (entity == NULL || (entity->flags_3e0 & 0x400u) == 0u) {
        return 0;
    }
    if (entity->type != 1) {
        return 1;
    }
    memset(&state, 0, sizeof(state));
    if (s_entity_health_threshold_read_hook != NULL) {
        if (s_entity_health_threshold_read_hook(
                entity, &state, s_entity_health_threshold_context) == 0)
            return 1;
    }
    else {
        /* Exact retail words: entity +0x328, Stats +0xec, entity +0x324,
         * and entity +0x3d4. These already have typed EntitySlot/Stats
         * mirrors; no external threshold provider is required in gameplay. */
        state.upper_limit = entity->field_328;
        /* FUN_00469bb0 indexes the type/row record by entity +0x330; the
         * scalar stats-record mirror omits that instance stride. */
        state.secondary_limit = stats_instance_word_at_ec(entity);
        state.required_value = (uint32_t)entity->carry_state_324;
        state.health_percent = entity->activity_counter_3d4;
    }
    value = state.secondary_limit;
    if (state.upper_limit < value) {
        value = state.upper_limit;
    }
    if (state.required_value < value && state.health_percent < 75.0f) {
        return 0;
    }
    return 1;
}

/* GatedStepCommandIssueWithTargetRecord (FUN_00439a20): resolve both cells,
 * install a one-point route for same-cell targets or the computed path for a
 * different cell, mark the last route step as kind 3, and retain the target. */
/* Port model of 0x00439a20 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439a20, retires to the tail in GAME-023 part 3
int entity_gated_step_command_issue(EntitySlot* entity, EntitySlot* target)
{
    GameContext* context;
    LevelState* level;
    float entity_position[2];
    float target_position[2];
    int* path_x = NULL;
    int* path_y = NULL;
    float* route_x = NULL;
    float* route_y = NULL;
    int entity_cell_x, entity_cell_y;
    int target_cell_x, target_cell_y;
    uint32_t route_count = 0u;
    unsigned int i;
    int route_result;

    if (entity == NULL || target == NULL
        || entity_flag400_and_health_threshold_check(entity) != 0
        || !entity_accepts_target_for_task_check(entity, target, 1)) {
        return 0;
    }
    context = entity->owner_ctx;
    if (context == NULL) context = entity_pool_callback_context_get();
    level = context != NULL ? context->level : NULL;
    if (level == NULL || level->ptr_terrain == NULL) return 0;

    entity_transform_position_get(entity, entity_position);
    entity_transform_position_get(target, target_position);
    if (!terrain_world_xy_to_cell(level->ptr_terrain,
            entity_position[0], entity_position[1],
            &entity_cell_x, &entity_cell_y) ||
        !terrain_world_xy_to_cell(level->ptr_terrain,
            target_position[0], target_position[1],
            &target_cell_x, &target_cell_y)) return 0;

    if (entity_cell_x == target_cell_x && entity_cell_y == target_cell_y) {
        path_x = (int*)malloc(sizeof(*path_x));
        path_y = (int*)malloc(sizeof(*path_y));
        if (path_x == NULL || path_y == NULL) {
            free(path_x);
            free(path_y);
            return 0;
        }
        path_x[0] = target_cell_x;
        path_y[0] = target_cell_y;
        route_count = 1u;
    }
    else {
        if (!terrain_neighbor_path_check(entity,
                (uint32_t)entity_cell_x, (uint32_t)entity_cell_y,
                (uint32_t)target_cell_x, (uint32_t)target_cell_y,
                &path_x, &path_y, &route_count, NULL, NULL)) {
            return 0;
        }
    }
    if (route_count == 0u ||
        (size_t)route_count > ((size_t)-1) / sizeof(*route_x)) {
        free(path_x);
        free(path_y);
        return 0;
    }
    route_x = (float*)malloc((size_t)route_count * sizeof(*route_x));
    route_y = (float*)malloc((size_t)route_count * sizeof(*route_y));
    if (route_x == NULL || route_y == NULL) {
        free(route_x);
        free(route_y);
        free(path_x);
        free(path_y);
        return 0;
    }
    for (i = 0u; i < route_count; ++i) {
        route_x[i] = (float)path_x[i];
        route_y[i] = (float)path_y[i];
    }
    route_result = entity_route_setup_and_marker_associate(
        entity, route_count, route_x, route_y, target_position);
    free(route_x);
    free(route_y);
    free(path_x);
    free(path_y);
    if (!route_result || entity->wp_count <= 0) return 0;
    ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 3u;
    entity->link_2fc = target;
    return 1;
}

/* Port model of 0x00439b80 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439b80, retires to the tail in GAME-023 part 3
int entity_task_target_cell_eligibility_check(
    const EntitySlot* entity, const EntitySlot* target)
{
    SpatialObjectQueryRequest request;
    float position[2];
    void* candidate;
    if (entity == NULL) return 0;
    if (entity->type != 1) return 1;
    if (target == NULL || target->etype != 0) return 0;
    if ((entity_capability_flags_get(entity) & 0x200u) != 0u) return 1;

    /* The reference's 0x438ae0 call is the live mask-2 spatial query. Its
     * arguments are the actor, target type (+0), and target selector
     * (+0x330). Read the host fields directly; EntitySlot is compact and is
     * not a byte-for-byte 0x14c-byte retail record. */
    memset(&request, 0, sizeof(request));
    entity_transform_position_get(entity, position);
    request.center[0] = position[0];
    request.center[1] = position[1];
    request.radius = 10000.0f;
    request.mask = 2u;
    request.filters[0] = (uint32_t)target->type;
    request.filters[1] = (uint32_t)target->indexed_record_slot_330;
    candidate = spatial_object_query_execute(&request);
    return candidate != NULL;
}
/* EntityAcceptsTargetForTaskCheck (FUN_00439bd0, finding 471). */
/* Port model of 0x00439bd0 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439bd0, retires to the tail in GAME-023 part 3
int entity_accepts_target_for_task_check(const EntitySlot* entity,
                                         const EntitySlot* target,
                                         int apply_extra_gate)
{
    uint32_t capabilities;
    if (entity == NULL || target == NULL) return 0;
    if (entity->type == 3) {
        if ((entity->flags_3e8 & 0x08000000u) == 0u) return 0;
    }
    else if ((entity->flags_3e8 & 0x20u) == 0u) {
        return 0;
    }
    if ((entity->flags_3e0 & 1u) != 0u) return 0;
    if (apply_extra_gate != 0
        && entity_flags_mask107c33de_or_1c00203_any_test(entity)) return 0;
    capabilities = entity_capability_flags_get(entity);
    if ((capabilities & 4u) != 0u && target->type != 6) return 0;
    if ((capabilities & 2u) != 0u && target->type != 7) return 0;
    if (entity->carry_state_324 != 0) {
        const EntitySlot* carried = (const EntitySlot*)entity->carry_link_308;
        if (carried == NULL || carried->type != target->type) return 0;
    }
    return 1;
}

/* EntityIndexedFlagSetAndRefresh (FUN_00439c80): type-1 records publish
 * their indexed bit, refresh the owning entity's flags, and raise event 0x19. */
/* Port model of 0x00439c80 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439c80, retires to the tail in GAME-023 part 3
void entity_indexed_flag_set_and_refresh(EntitySlot* entity)
{
    uint32_t bit;
    if (entity == NULL || entity->type != 1) return;
    bit = 1u << (entity->indexed_record_flag_index & 0x1fu);
    entity->indexed_record_flags |= bit;
    (void)entity_animation_state_set(entity, entity->indexed_record_flags);
    if (s_entity_indexed_flag_publish_hook != NULL) {
        s_entity_indexed_flag_publish_hook(entity,
                                           entity->indexed_record_flags,
                                           0x19u,
                                           s_entity_indexed_flag_context);
    }
}
/* Port model of 0x00439cd0 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439cd0, retires to the tail in GAME-023 part 3
int entity_animation_state_set(EntitySlot* entity, uint32_t state)
{
    return entity_building_index_set(entity, state);
}

/* Port model of 0x00439cf0 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439cf0, retires to the tail in GAME-023 part 3
uint32_t entity_stats_flag10_test(const EntitySlot* e)
{
    return entity_stats_word_get(e) & 0x10u;
}

/* Port model of 0x00439d10 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439d10, retires to the tail in GAME-023 part 3
uint32_t entity_stats_flag20_test(const EntitySlot* e)
{
    return entity_stats_word_get(e) & 0x20u;
}

/* Port model of 0x00439d30 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439d30, retires to the tail in GAME-023 part 3
uint32_t entity_stats_flag40_test(const EntitySlot* e)
{
    return entity_stats_word_get(e) & 0x40u;
}
/* Port model of 0x00439d50 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439d50, retires to the tail in GAME-023 part 3
int entity_vehicle_unload_adjacent_cell_find(const EntitySlot* entity,
                                             int out_xy[2])
{
    const GameContext* context;
    if (entity == NULL) return 0;
    context = entity->owner_ctx != NULL ? entity->owner_ctx
                                        : entity_pool_callback_context_get();
    return entity_unload_adjacent_cell_find_on_level(
        entity, context != NULL ? context->level : NULL, out_xy);
}

/* Port model of 0x00439e10 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439e10, retires to the tail in GAME-023 part 3
int entity_vehicle_unload_cell_find_if_eligible(const EntitySlot* entity,
                                                int out_xy[2])
{
    if (entity == NULL
        || (entity_capability_flags_get(entity) & 0x10u) == 0u) {
        return 0;
    }
    return entity_vehicle_unload_adjacent_cell_find(entity, out_xy);
}

/* Port model of 0x00439e40 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439e40, retires to the tail in GAME-023 part 3
int entity_record_flag20_query(const EntitySlot* entity, int out_xy[2])
{
    if (entity == NULL
        || (entity_capability_flags_get(entity) & 0x20u) == 0u) {
        return 0;
    }
    return entity_vehicle_unload_adjacent_cell_find(entity, out_xy);
}

namespace unported_ledger_stubs {
/* EntityAttachedObjectListDetach (FUN_00439e70): detach every represented
 * carried slot, retaining the legacy slot-zero mirror. */
/* Port model of 0x00439e70 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x00439e70, retires to the tail in GAME-023 part 3
void entity_attached_object_list_detach(EntitySlot* entity, int reset_mode)
{
    EntitySlot* carried;
    EntitySlot* linked_entity;
    uint32_t state_flags;
    int carry_index;
    if (entity == NULL) return;
    state_flags = entity->flags_3e0;
    if ((state_flags & 0x400u) == 0u ||
        ((state_flags & 0x2000u) != 0u && reset_mode == 0)) return;
    linked_entity = entity->link_2fc;
    /* The retail consume-and-release branch walks the attached-record array
     * when the +0x2fc entity has stats bit 8 and reset mode is active. Those
     * records are full entity slots; clear their two owner links, unlink kind
     * 5, then let 0x43a850 apply counters and return each slot to its pool. */
    if (reset_mode != 0 && linked_entity != NULL &&
        (entity_stats_word_get(linked_entity) & 8u) != 0u &&
        entity->attached_children != NULL) {
        uint32_t i;
        GameContext* context = linked_entity->owner_ctx;
        if (context == NULL) context = entity_pool_callback_context_get();
        entity_carried_links_clear(entity);
        for (i = 0u; i < entity->attached_child_count; ++i) {
            EntitySlot* record = entity->attached_children[i];
            entity->attached_children[i] = NULL;
            if (record == NULL) continue;
            record->attach_304 = NULL;
            record->link_300 = NULL;
            entity_command_unlink_by_kind(entity, 5, record, 1);
            entity_action_record_consume_and_cleanup(linked_entity, record);
            entity_activity_scene_state_update(context);
        }
        entity->attached_child_count = 0u;
        entity->flags_3e0 &= ~0x400u;
        for (i = 0u; i < (uint32_t)entity->wp_count; ++i) {
            if (ENTITY_WAYPOINTS(entity)[i].step_kind == 5u ||
                ENTITY_WAYPOINTS(entity)[i].step_kind == 7u)
                ENTITY_WAYPOINTS(entity)[i].pending = 0u;
        }
        return;
    }
    if (reset_mode != 0 &&
        (entity_stats_word_get(entity) & 8u) == 0u) {
        (void)cmd_chain_drop(entity, 6);
        (void)cmd_chain_drop(entity, 9);
        (void)cmd_chain_drop(entity, 15);
        (void)cmd_chain_drop(entity, 20);
    }
    for (carry_index = 0; carry_index < 7; ++carry_index) {
        carried = entity_carried_slot_get(entity, carry_index);
        entity_carried_slot_set(entity, carry_index, NULL);
        if (carried == NULL) continue;
        if (carried->attach_304 == entity) {
            carried->attach_304 = NULL;
            carried->attached_scene_slot_index = 0;
        }
        carried->owner_36c = NULL;
        carried->link_2fc = NULL;
        if (reset_mode == 0) {
            if ((state_flags & 0x200u) == 0u) {
                (void)cmd_chain_drop(entity, 5);
                (void)cmd_chain_drop(entity, 6);
                (void)cmd_chain_drop(entity, 9);
                (void)cmd_chain_drop(entity, 15);
                (void)cmd_chain_drop(entity, 7);
                (void)cmd_chain_drop(entity, 4);
                (void)cmd_chain_drop(entity, 20);
            } else {
                entity_command_unlink_by_kind(entity, 3, carried, 1);
            }
        } else {
            (void)cmd_chain_drop(entity, 5);
            if (carried->type != 7 && carried->type != 6)
                carried->flags_3ec |= 0x80000u;
        }
    }
    entity->carry_state_324 = 0;
    entity->flags_3e0 &= ~0x400u;
}
/* Port model of 0x0043a120 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x0043a120, retires to the tail in GAME-023 part 3
int entity_cell_step_command_issue_with_jitter(EntitySlot* entity,
    const int target_xy[2], const float input_xy[2], int action_mode,
    int mark_pending)
{
    int start_x, start_y;
    int* path_x = NULL;
    int* path_y = NULL;
    uint32_t path_count = 0u;
    float* route_x;
    float* route_y;
    float jitter_x = 0.5f;
    float jitter_y = 0.5f;
    int mark_step = mark_pending;
    int route_result;
    uint32_t i;
    if (entity == NULL || target_xy == NULL ||
        (entity->flags_3e0 & 0x400u) == 0u) return 0;
    /* FUN_004424f0 resolves the entity's scene position through the active
     * terrain grid before the path finder is called. EntitySlot::x/y are
     * world coordinates, so rounding them directly only works when the
     * terrain origin and cell scale happen to be the identity transform. */
    if (entity->owner_ctx != NULL && entity->owner_ctx->level != NULL &&
        entity->owner_ctx->level->ptr_terrain != NULL) {
        if (!terrain_world_xy_to_cell(
                entity->owner_ctx->level->ptr_terrain, entity->x, entity->y,
                &start_x, &start_y)) return 0;
    }
    else {
        /* The path hook is the only available fallback when the entity has
         * no live terrain owner. */
        start_x = level_cell_of(entity->x);
        start_y = level_cell_of(entity->y);
    }
    if (start_x == target_xy[0] && start_y == target_xy[1]) {
        path_x = (int*)malloc(sizeof(int));
        path_y = (int*)malloc(sizeof(int));
        if (path_x == NULL || path_y == NULL) {
            free(path_x);
            free(path_y);
            return 0;
        }
        path_x[0] = target_xy[0];
        path_y[0] = target_xy[1];
        path_count = 1u;
    }
    else if (entity->owner_ctx != NULL && entity->owner_ctx->level != NULL) {
        if (!terrain_neighbor_path_check(entity->owner_ctx->level,
                (uint32_t)start_x, (uint32_t)start_y,
                (uint32_t)target_xy[0], (uint32_t)target_xy[1],
                &path_x, &path_y, &path_count, NULL, NULL)) return 0;
    }
    else if (s_cell_step_path_hook != NULL) {
        int fallback_xy[2] = { target_xy[0], target_xy[1] };
        if (!s_cell_step_path_hook(start_x, start_y, target_xy[0],
                                   target_xy[1], fallback_xy,
                                   s_cell_step_hook_user)) return 0;
        path_x = (int*)malloc(sizeof(int));
        path_y = (int*)malloc(sizeof(int));
        if (path_x == NULL || path_y == NULL) {
            free(path_x);
            free(path_y);
            return 0;
        }
        path_x[0] = fallback_xy[0];
        path_y[0] = fallback_xy[1];
        path_count = 1u;
    }
    else return 0;
    /* The retail path finder returns heap-owned coordinate arrays and the
     * route installer supports counts beyond EntitySlot's inline waypoint
     * span. Keep only the allocation-overflow guard here; the old inline
     * maximum rejected valid long paths before the dynamic route store saw
     * them. */
    if (path_count == 0u ||
        (size_t)path_count > ((size_t)-1) / sizeof(float)) {
        free(path_x);
        free(path_y);
        return 0;
    }
    route_x = (float*)malloc((size_t)path_count * sizeof(float));
    route_y = (float*)malloc((size_t)path_count * sizeof(float));
    if (route_x == NULL || route_y == NULL) {
        free(route_x);
        free(route_y);
        free(path_x);
        free(path_y);
        return 0;
    }
    for (i = 0u; i < path_count; ++i) {
        route_x[i] = (float)path_x[i];
        route_y[i] = (float)path_y[i];
    }
    route_result = entity_route_setup_and_marker_associate(
        entity, path_count, route_x, route_y, 0);
    free(route_x);
    free(route_y);
    free(path_x);
    free(path_y);
    if (!route_result || entity->wp_count <= 0) return 0;

    if (entity->type == 1) {
        float value = s_cell_step_random_hook != NULL
            ? s_cell_step_random_hook(0.55f, 0.75f, s_cell_step_hook_user)
            : (float)(int16_t)rng_thunk_draw15() * 3.051851e-05f * 0.20f
                + 0.55f;
        int direction;
        if (s_cell_step_random_hook != NULL) {
            direction = (int)s_cell_step_random_hook(
                0.0f, 3.999f, s_cell_step_hook_user);
        }
        else {
            int16_t draw = (int16_t)rng_thunk_draw15();
            direction = draw < 0 ? -(int)draw : (int)draw;
        }
        mark_step = 1;
        switch (direction & 3) {
        case 0: jitter_x = 0.5f; jitter_y = value; break;
        case 1: jitter_x = value; jitter_y = 0.5f; break;
        case 2: jitter_x = 0.5f; jitter_y = 1.0f - value; break;
        default: jitter_x = 1.0f - value; jitter_y = 0.5f; break;
        }
    } else if (input_xy != NULL) {
        jitter_x = input_xy[0];
        jitter_y = input_xy[1];
    }
    AiWaypoint* step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    step->step_kind = (uint8_t)(action_mode == 0 ? 8 : 9);
    if (input_xy != NULL) {
        step->weight_y = jitter_x;
        step->weight_x = jitter_y;
    }
    if (mark_step != 0) step->pending |= 8u;
    return 1;
}
} // namespace unported_ledger_stubs

/* EntityDepositActivityGate (FUN_0043a300): accepted targets use the real
 * locator, terrain search and route installer. An unavailable scene owner
 * is distinct from an authored missing locator, which uses target origin. */
/* Port model of 0x0043a300 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x0043a300, retires to the tail in GAME-023 part 3
int entity_deposit_activity_gate(EntitySlot* entity, EntitySlot* target)
{
    uint32_t flags;
    GameContext* context;
    LevelState* level;
    float actor_position[3], marker[3], facing[3];
    int start_x, start_y, goal_x, goal_y;
    int locator_status = -1;
    int direction, installed;
    int* path_x = NULL;
    int* path_y = NULL;
    float* route_x = NULL;
    float* route_y = NULL;
    uint32_t count = 0u, i;
    if (entity == NULL || (entity->flags_3e0 & 0x400u) == 0u) return 0;
    if (s_entity_deposit_activity_hooks.activity_flags != NULL)
        flags = s_entity_deposit_activity_hooks.activity_flags(
            s_entity_deposit_activity_hooks.context, entity);
    else flags = entity_stats_word_get(entity);
    if ((flags & (4u | 2u)) != 0u) {
        entity->flags_3e0 |= 0x2000u;
        ai_set_activity(entity, 0x1d, 1);
        (void)activity_register_bookkeeping(entity);
        entity->activity_timer = 1.0f;
        if (s_entity_deposit_activity_hooks.deposit_cancel != NULL)
            s_entity_deposit_activity_hooks.deposit_cancel(
                s_entity_deposit_activity_hooks.context, entity);
        return 1;
    }
    if (target == NULL) return 0;
    if (!entity_accepts_target_for_task_check(
            target, (const EntitySlot*)entity->carry_link_308, 0)
        && (entity_stats_word_get(target) & 8u) == 0u) return 0;
    flags = entity_stats_word_get(target);
    if ((flags & 8u) != 0u || target->type == 4) {
        if (s_entity_deposit_activity_hooks.approach_pose_get != NULL)
            locator_status = s_entity_deposit_activity_hooks.approach_pose_get(
                s_entity_deposit_activity_hooks.context, target,
                (flags & 8u) != 0u, marker, facing);
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
        else locator_status = building_scene_deposit_approach_get(target,
            (flags & 8u) != 0u, marker, facing);
#endif
    }
    else locator_status = 0;
    if (locator_status < 0) return 0;
    if (locator_status == 0) {
        if (!entity_deposit_position_get(target, marker)) return 0;
        direction = (int)((uintptr_t)entity & 3u);
    }
    else {
        if (marker[0] != marker[0] || marker[1] != marker[1] ||
            facing[0] != facing[0] || facing[1] != facing[1] ||
            fabs(marker[0]) > FLT_MAX || fabs(marker[1]) > FLT_MAX ||
            fabs(facing[0]) > FLT_MAX || fabs(facing[1]) > FLT_MAX) return 0;
        if (fabs(facing[0]) > fabs(facing[1]))
            direction = facing[0] > 0.0f ? 3 : 1;
        else direction = facing[1] > 0.0f ? 2 : 0;
    }
    context = entity->owner_ctx != NULL ? entity->owner_ctx
                                        : entity_pool_callback_context_get();
    level = context != NULL ? context->level : NULL;
    if (level == NULL || level->ptr_terrain == NULL ||
        !entity_deposit_position_get(entity, actor_position) ||
        !terrain_world_xy_to_cell(level->ptr_terrain, actor_position[0],
            actor_position[1], &start_x, &start_y) ||
        !terrain_world_xy_to_cell(level->ptr_terrain, marker[0], marker[1],
            &goal_x, &goal_y)) return 0;
    if (start_x == goal_x && start_y == goal_y) {
        float x = (float)goal_x;
        float y = (float)goal_y;
        installed = entity_route_setup_and_marker_associate(
            entity, 1u, &x, &y, marker);
    }
    else {
        if (!terrain_neighbor_path_check(entity, (uint32_t)start_x,
                (uint32_t)start_y, (uint32_t)goal_x, (uint32_t)goal_y,
                &path_x, &path_y, &count, NULL, NULL)) {
            free(path_x); free(path_y);
            return 0;
        }
        if (count == 0u || path_x == NULL || path_y == NULL ||
            (size_t)count > ((size_t)-1) / sizeof(float)) {
            free(path_x); free(path_y);
            return 0;
        }
        route_x = (float*)malloc((size_t)count * sizeof(float));
        route_y = (float*)malloc((size_t)count * sizeof(float));
        if (route_x == NULL || route_y == NULL) {
            free(path_x); free(path_y); free(route_x); free(route_y);
            return 0;
        }
        for (i = 0u; i < count; ++i) {
            route_x[i] = (float)path_x[i];
            route_y[i] = (float)path_y[i];
        }
        installed = entity_route_setup_and_marker_associate(
            entity, count, route_x, route_y, marker);
        free(path_x); free(path_y); free(route_x); free(route_y);
    }
    /* Retail returns success on an installer refusal. The gate writes its
     * target/terminal only on success; the installer owns any changes to
     * the preceding route. Path-search failure returns zero. */
    if (installed && entity->wp_count > 0) {
        AiWaypoint* last = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
        entity->link_2fc = target;
        if ((flags & 8u) != 0u) last->step_kind = 9u;
        else {
            last->step_kind = 7u;
            last->pending |= (uint8_t)(4u | (direction & 3));
        }
    }
    return 1;
}
/* EntityVisualResourceGetOrCreate (FUN_0043a5f0): choose one of the four
 * type-specific cached visual handles and create the model when absent. */
/* Port model of 0x0043a5f0 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x0043a5f0, retires to the tail in GAME-023 part 3
void entity_visual_resource_get_or_create(EntitySlot* entity)
{
    int slot;
    void* resource;
    if (entity == NULL || entity->type < 1 || entity->type > 4) return;
    slot = entity->type - 1;
    resource = entity->visual_resource_handles[slot];
    if (s_entity_visual_resource_present_hook != NULL
        && s_entity_visual_resource_present_hook(entity, resource,
                                                 s_entity_visual_resource_context)) {
        if (entity->scene_slot_nodes == NULL &&
            entity_type_config_scene_slot_nodes_load(entity)) {
            entity_scene_slot_table_rebuild(entity, entity->activity);
        }
        return;
    }
    entity_scene_slot_nodes_clear(entity);
    if (s_entity_visual_resource_create_hook != NULL) {
        s_entity_visual_resource_create_hook(entity,
                                              s_entity_visual_resource_context);
    }
    (void)entity_type_config_scene_slot_nodes_load(entity);
    entity_scene_slot_table_rebuild(entity, entity->activity);
}

/* ActionKindEntityCreateAndCommandIssue (FUN_0043a650). The installed
 * constructor owns resource/template selection; this routine keeps the
 * verified action gate, event order, queue opcode, and kind labels. */
/* Port model of 0x0043a650 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x0043a650, retires to the tail in GAME-023 part 3
EntitySlot* entity_action_kind_entity_create_and_command_issue(
    EntitySlot* source, int kind, int selector, int argument)
{
    EntityActionIssueHooks* hooks = &s_entity_action_issue_hooks;
    EntitySlot* created;
    int opcode;
    if (source == NULL || hooks->create == NULL
        || !entity_action_kind_supported(source, kind, selector)) return NULL;
    switch (kind) {
    case 6:
    case 7:
    case 8:
    case 9:
    case 0xb:
    case 0xd:
        break;
    default:
        return NULL;
    }
    /* Kind 6 refreshes the activity scene and subtracts one from level +0x8c.
     * Kind 7 debits one unit from the selected resource. The adapter remains
     * an override for callers that own the corresponding presentation work. */
    if ((kind == 6 || kind == 7) && hooks->kind_event != NULL) {
        hooks->kind_event(hooks->context, kind, selector);
    }
    else if (kind == 6) {
        GameContext* context = source->owner_ctx;
        if (context == NULL) context = entity_pool_callback_context_get();
        if (context != NULL) {
            entity_activity_scene_state_update(context);
            global_object_field8c_subtract_for_level(context->level, 1);
        }
    }
    else if (kind == 7) {
        upgrade_resource_debit(selector, 1);
    }
    created = hooks->create(hooks->context, source, kind, selector, argument);
    if (created == NULL) return NULL;
    opcode = kind == 7 ? 0x1c : 0x16;
    if (hooks->queue != NULL)
        hooks->queue(hooks->context, opcode, created);
    else
        cmd_enqueue(opcode, created, 0, 0, 0);
    if (kind == 8 && hooks->label_set != NULL) {
        hooks->label_set(hooks->context, created, "Normal");
    }
    else if (kind == 9 && hooks->label_set != NULL) {
        hooks->label_set(hooks->context, created, "Short");
    }
    return created;
}

/* --- the task-kind filtered search (see cmd.hpp) -----------------------------
 * The corpus's pool walk hands the callback the slot pointer and the
 * argument block; the port models that argument block as the selector word. */
/* Port model of 0x0043a7c0 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x0043a7c0, retires to the tail in GAME-023 part 3
int entity_action_kind_supported(EntitySlot* e, int kind, int arg)
{
    if (e == NULL) {
        return 0; /* deviation: the corpus reads the entity blindly */
    }
    if ((entity_stats_word_get(e) & 8u) == 0u) {
        return 0;
    }
    switch (kind) {
    case 6:
        /* Corpus: NoOpCallback(1) — the callable no-op's return is
         * corpus-undefined (finding 381), so the port never passes. */
        return 0;
    case 7:
        return upgrade_resource_check((s_ctx != NULL) ? s_ctx->level
                                                      : NULL, arg)
                   != 0u;
    case 9:
    case 8:
    case 0xb:
    case 0xd:
        return 1;
    default:
        return 0;
    }
}

/* EntityActionRecordConsumeAndCleanup (FUN_0043a850): the second argument is
 * a full entity record. Its +0x330 selector drives the type-6 owner counter;
 * +0x304 is cleared before the record is returned through EntityTeardown. */
/* Port model of 0x0043a850 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x0043a850, retires to the tail in GAME-023 part 3
void entity_action_record_consume_and_cleanup(
    EntitySlot* entity, EntitySlot* record)
{
    EntityActionConsumeHooks* hooks = &s_entity_action_consume_hooks;
    GameContext* context;
    uint32_t* counters = NULL;
    if (entity == NULL || record == NULL) return;
    context = entity_pool_callback_context_get();
    if (context == NULL) context = entity->owner_ctx;
    if (entity->type == 3 && !entity->action_consume_counters_initialized) {
        memset(entity->action_consume_counters, 0,
               sizeof(entity->action_consume_counters));
        entity->action_consume_counters_initialized = 1;
    }
    if (entity->type == 3)
        counters = entity_action_consume_counter_sidecar_get(entity, 1);
    if (record->type == 6) {
        if (entity->type == 3) {
            uint32_t slot = record->indexed_record_slot_330 < 0 ? 6u :
                (uint32_t)record->indexed_record_slot_330;
            if (slot < 6u) {
                if (counters != NULL) {
                    ++counters[slot];
                    entity->action_consume_counters[slot] = counters[slot];
                } else {
                    ++entity->action_consume_counters[slot];
                }
            }
        }
        else if (hooks->event_dispatch != NULL) {
            hooks->event_dispatch(hooks->context, 6, 0);
        }
        else if (context != NULL) {
            global_object_field8c_increment_for_level(context->level);
        }
    }
    else if (record->type == 7 && entity->type != 3
             && hooks->event_dispatch != NULL) {
        hooks->event_dispatch(hooks->context, 7, record->etype);
    }
    else if (record->type == 7 && entity->type != 3) {
        level_resource_balance_add_and_sound(record->etype);
    }
    record->attach_304 = NULL;
    if (hooks->record_release != NULL) {
        hooks->record_release(hooks->context, record);
    }
    else {
        (void)entity_teardown(record);
    }
    if (entity->type != 3 && hooks->entity_records_cleanup != NULL) {
        hooks->entity_records_cleanup(hooks->context, entity);
    }
    else if (entity->type != 3) {
        /* FUN_00403910 is the concrete live implementation of the retail
         * non-creature record cleanup sweep used by this target. */
        terrain_cell_record_counts_prune(entity);
    }
}

/* Port model of 0x0043a8f0 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x0043a8f0, retires to the tail in GAME-023 part 3
void simulation_update_counter_add(GameContext* ctx, int count)
{
    if (ctx == NULL) {
        return; /* deviation: the corpus strides its global level word */
    }
    for (; count != 0; count = count - 1) {
        global_object_field8c_increment_for_level(ctx->level);
    }
    entity_activity_scene_state_update(ctx);
}

/* Port model of 0x0043a910 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x0043a910, retires to the tail in GAME-023 part 3
void simulation_mode_counter_add(int mode, int count)
{
    (void)mode; /* register residue: the corpus ignores it, count only */
    /* Deviation: the corpus loops `count != 0` down by one, so a negative
     * count runs away; the port counts toward zero. */
    for (int i = count; i != 0; i = i > 0 ? i - 1 : i + 1) {
        level_resource_balance_add_and_sound(0);
    }
}

/* TrackedEntityPointerArrayRemove (FUN_0043a930) — see ai.hpp. The corpus
 * strides the DAT_004eabcc window up to its count word blindly; the port
 * rides the s_idle100 model's used window (the corpus keeps walking past
 * a hit, pinned). */
/* Port model of 0x0043a930 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x0043a930, retires to the tail in GAME-023 part 3
void ai_idle_table_pointer_remove(void* pointer)
{
    for (int i = 0; i < s_idle100_n; ++i) {
        if (s_idle100[i] == pointer) {
            s_idle100[i] = NULL;
        }
    }
}
/* EntityLavaCrustActivityRegister (FUN_0043a960): require a type-one entity
 * whose stats +0x144 word lacks bit 0x02000000, then inspect its terrain
 * tile before appending it to the 100-slot high-water table. A NULL entity
 * or missing current level returns safely because the portable runtime has
 * no record or map pointer to inspect in those cases. */
/* Port model of 0x0043a960 (GAME-023); the original-shape function is in legoobject_unit.c. */
// PORT-ONLY: port model of 0x0043a960, retires to the tail in GAME-023 part 3
void ai_lava_crust_activity_register(GameContext* ctx, EntitySlot* e)
{
    const LevelState* grid;
    int cell_x = -1;
    int cell_y = -1;
    if (e == NULL || e->type != 1) {
        return; /* the corpus's type gate, in place at 0x43a968 */
    }
    if ((entity_stats_word_get(e) & 0x02000000u) != 0u) {
        return; /* FUN_00469e60 result at 0x43a976 */
    }
    grid = ctx != NULL ? ctx->level : NULL;
    if (grid == NULL) return;
    entity_position_to_terrain_cell(e, grid, &cell_x, &cell_y);
    if (level_tile_type(grid, cell_x, cell_y) == TILE_TYPE_IDLE)
        idle100_register(e);
}

/* EntityQueueForRockfallProcessingIfEligible (FUN_0043a9e0): a pure forwarder
 * in the corpus (0x43a9e5: push e / call FUN_0043a960). The portable wrapper
 * supplies the active context bound by the entity-pool owner so the callee
 * can perform the same current-level tile test. */
// FUNCTION: LEGORR 0x0043a9e0
void entity_queue_for_rockfall_processing_if_eligible(EntitySlot* e)
{
    ai_lava_crust_activity_register(entity_pool_callback_context_get(), e);
}

/* EntityPairInteractionGate (FUN_0043a9f0): a qualifying carrier pair
 * requests activity 0x24 once per current sentinel and raises the partner
 * interaction bit in the entity's +0x3e4 flags. */
// FUNCTION: LEGORR 0x0043A9F0
void entity_pair_interaction_gate(EntitySlot* entity,
                                  const EntitySlot* partner)
{
    const int* carried_kind;
    if (entity == NULL || partner == NULL) return;
    if ((entity->flags_3e0 & 0x400u) != 0u
        || (entity->flags_3e4 & 1u) != 0u
        || (entity->flags_3e0 & 0x01000100u) != 0u
        || entity->field_340 <= 0.0f
        || (partner->flags_3e0 & 0x400u) == 0u
        || partner->carry_link_308 == NULL) return;
    carried_kind = (const int*)partner->carry_link_308;
    if (*carried_kind != 6 || s_entity_pair_activity_sentinel
        == entity->activity) return;
    ai_set_activity(entity, 0x24, 0);
    (void)activity_register_bookkeeping(entity);
    entity->flags_3e4 |= 0x00200000u;
}

/* ProximityActivityRaise (FUN_0043aab0, finding 471): activate activity
 * 0x24 when a type-3 entity is near a qualifying terrain cell. */
// FUNCTION: LEGORR 0x0043AAB0
void entity_proximity_activity_raise(EntitySlot* entity)
{
    const GameContext* context;
    const LevelState* level;
    float cell_xy[2];
    float block_size;
    float dx, dy;
    int x = 0, y = 0;
    if (entity == NULL || entity->type != 3
        || (entity_capability_flags_get(entity) & 0x4000u) == 0u
        || s_entity_pair_activity_sentinel == entity->activity
        || (entity->flags_3e0 & 0x400u) != 0u
        || !(entity_stats_float_ate8(entity) < entity->field_340)) return;
    context = entity->owner_ctx;
    level = context != NULL ? context->level : NULL;
    if (level == NULL || level->ptr_terrain == NULL) return;
    entity_position_to_terrain_cell(entity, level, &x, &y);
    if (!level_coord_pair_within_bounds(level, x, y)
        || !level_cell_occupied_at(level, x, y)
        || level_cell_flag_8000(level, x, y) != 0u
        || !terrain_cell_world_center_xy_get(level->ptr_terrain, x, y,
                                             cell_xy)) return;
    block_size = terrain_blocksize_get(level->ptr_terrain);
    dx = entity->x - cell_xy[0];
    dy = entity->y - cell_xy[1];
    if (sqrtf(dx * dx + dy * dy) < block_size * 0.33333334f) {
        ai_set_activity(entity, 0x24, 0);
        (void)activity_register_bookkeeping(entity);
        entity->flags_3e4 |= 0x00200000u;
    }
}

/* TerrainCellVariantUpdateQueue (FUN_0043abf0). CPU-visible behavior is
 * owned here; only the DDi marker and unavailable local entity visitor are
 * renderer/entity-owner hooks. */
// FUNCTION: LEGORR 0x0043ABF0
void terrain_cell_variant_update_queue(void* entity)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    int cell_xy[2] = { 0, 0 };
    TerrainVariantVisitContext visit;
    if (level == NULL) level = (LevelState*)map_cell_record_grid_level_get();
    if (entity == NULL || level == NULL) return;

    map_cell_record_grid_activate(level);
    entity_position_to_terrain_cell((const EntitySlot*)entity, level,
                                    &cell_xy[0], &cell_xy[1]);
    if (level_random_draw15() % 5 == 0) {
        random_variant_cell_scatter(cell_xy[0], cell_xy[1], 3, 1);
    }
    if (level_cell_occupied_at(level, cell_xy[0], cell_xy[1]) != 0) {
        if (s_terrain_variant_marker_hook != NULL) {
            s_terrain_variant_marker_hook(
                entity, s_terrain_variant_scene_context);
        }
        terrain_cell_work_flag_clear(level, cell_xy, 0x3fu);
        level_cell_init_pass(level, cell_xy[0], cell_xy[1]);
    }

    /* FUN_0043ac90's per-entity callback is renderer/entity-owner behavior
     * not represented by the terrain module. Preserve the pool walk when a
     * live GameContext exists; the level-only path above remains useful to
     * callers that own the active terrain registry without that context. */
    if (game != NULL) {
        visit.owner_entity = entity;
        (void)entity_pool_find_by_callback_filtered_current(
            terrain_variant_entity_visit_adapter, &visit);
    }
}

/* PredugSpawnCoordinateSelectAndCreate (FUN_0043ad50): choose a configured
 * predug cell or resolve the source entity's cell, add the corpus 12.75-unit
 * radial offset, create the requested entity, and run its setup callback. */
// FUNCTION: LEGORR 0x0043AD50
void* predug_spawn_coordinate_select_and_create(int source_object, int type)
{
    int cell_x;
    int cell_y;
    int selected;
    float angle = 0.0f;
    float world_x;
    float world_y;
    void* entity;
    if (s_predug_spawn_enabled == 0 || s_predug_spawn_count == 0u
        || s_spawn_coordinate_hook == NULL) {
        return NULL;
    }
    if (source_object == 0) {
        selected = s_spawn_random_index_hook != NULL
            ? s_spawn_random_index_hook((int)s_predug_spawn_count,
                                        s_spawn_hook_context)
            : (int)(rng_thunk_draw15() % s_predug_spawn_count);
        if (selected < 0 || (uint32_t)selected >= s_predug_spawn_count) {
            selected = 0;
        }
        cell_x = s_predug_spawn_cells[selected][0];
        cell_y = s_predug_spawn_cells[selected][1];
    } else if (s_spawn_object_cell_hook == NULL
               || s_spawn_object_cell_hook(source_object, &cell_x, &cell_y,
                                            s_spawn_hook_context) == 0) {
        return NULL;
    }
    if (s_spawn_random_angle_hook != NULL) {
        angle = s_spawn_random_angle_hook(s_spawn_hook_context);
    } else {
        angle = random_float_range(0.0f, 6.2831855f);
    }
    if (s_spawn_coordinate_hook(cell_x, cell_y, &world_x, &world_y,
                                s_spawn_coordinate_context) == 0) {
        return NULL;
    }
    world_x += (float)sin((double)angle) * 12.75f;
    world_y += (float)cos((double)angle) * 12.75f;
    if (s_spawn_create_hook == NULL) {
        return NULL;
    }
    entity = s_spawn_create_hook(source_object, type, world_x, world_y, angle,
                                 s_spawn_hook_context);
    if (entity != NULL && s_spawn_initialize_hook != NULL) {
        s_spawn_initialize_hook(entity, s_spawn_hook_context);
    }
    return entity;
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x0043aea0
EntitySlot* emergence_random_spawn_attempt(int entity_index, int owner_index,
                                           int grid_width, int grid_height)
{
    unsigned int attempt;
    GameContext* context = entity_pool_callback_context_get();
    LevelState* level = s_emergence_spawn_hooks.level != NULL
        ? s_emergence_spawn_hooks.level
        : (context != NULL ? context->level : NULL);
    EntitySlot* base_entity;
    if (grid_width <= 0 && level != NULL)
        grid_width = level_width(level);
    if (grid_height <= 0 && level != NULL)
        grid_height = level_height(level);
    if (grid_width <= 0 || grid_height <= 0 || grid_width > 65535 ||
        grid_height > 65535 ||
        (s_emergence_spawn_hooks.spawn_try == NULL &&
         (context == NULL || level == NULL)))
        return NULL;
    base_entity = context != NULL
        ? (EntitySlot*)entity_at(context, 3, entity_index) : NULL;
    if (s_emergence_spawn_hooks.spawn_try == NULL && base_entity == NULL &&
        emergence_type_name_get(owner_index) == NULL)
        return NULL;
    for (attempt = 0; attempt < 1000u; ++attempt) {
        int16_t rx = s_emergence_spawn_hooks.random_u16 != NULL
            ? (int16_t)s_emergence_spawn_hooks.random_u16(
                s_emergence_spawn_hooks.user)
            : (int16_t)rng_thunk_draw15();
        int16_t ry = s_emergence_spawn_hooks.random_u16 != NULL
            ? (int16_t)s_emergence_spawn_hooks.random_u16(
                s_emergence_spawn_hooks.user)
            : (int16_t)rng_thunk_draw15();
        EntitySlot* entity;
        int cell_x = (int)rx % grid_width;
        int cell_y = (int)ry % grid_height;
        if (s_emergence_spawn_hooks.spawn_try != NULL) {
            entity = s_emergence_spawn_hooks.spawn_try(entity_index,
                owner_index, cell_x, cell_y, s_emergence_spawn_hooks.user);
        }
        else {
            entity = creature_spawn_from_emergence(base_entity, 3,
                owner_index, cell_x, cell_y);
        }
        if (entity != NULL) return entity;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0043af30
EntitySlot* creature_spawn_from_emergence(void* base_entity, int mode_token,
                                          int owner_index, int cell_x,
                                          int cell_y)
{
    static const int offsets[4][2] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
    static const float headings[4] = { 0.0f, 1.570796327f,
                                       3.141592654f, 4.712388980f };
    int direction;
    int neighbor_x = 0;
    int neighbor_y = 0;
    float origin[2];
    float neighbor[2];
    EntitySlot* created;
    LevelState* level = emergence_level_get(base_entity);
    GameContext* context = emergence_game_context_get(base_entity);
    if (mode_token != 3 ||
        (level == NULL && s_emergence_spawn_hooks.cell_eligible == NULL) ||
        (s_emergence_spawn_hooks.create == NULL && context == NULL)) {
        return NULL;
    }
    if (level != NULL) {
        if (cell_x < 0 || cell_y < 0 || cell_x >= level_width(level) ||
            cell_y >= level_height(level) ||
            !level_cell_walkable_flag(level, cell_x, cell_y) ||
            level_cell_flag_20_bounded(level, cell_x, cell_y) != 0u ||
            level_cell_mask_c0(level, cell_x, cell_y) != 0u ||
            level_cell_state_is_1(level, cell_x, cell_y) ||
            level_cell_flag_reserved(level, cell_x, cell_y) != 0u) {
            return NULL;
        }
    }
    else if (!s_emergence_spawn_hooks.cell_eligible(
                 cell_x, cell_y, s_emergence_spawn_hooks.user)) {
        return NULL;
    }
    for (direction = 0; direction < 4; ++direction) {
        neighbor_x = cell_x + offsets[direction][0];
        neighbor_y = cell_y + offsets[direction][1];
        if (level != NULL) {
            if (level_cell_flag_8_bounded(level, neighbor_x, neighbor_y)
                    == 0u ||
                 level_cell_mask_180000(level, neighbor_x, neighbor_y) != 0 ||
                 !terrain_cell_route_and_occupancy_check(NULL, level,
                     neighbor_x, neighbor_y, neighbor_x, neighbor_y, 0)) {
                continue;
            }
        }
        else if (!s_emergence_spawn_hooks.cell_eligible(
                     neighbor_x, neighbor_y,
                     s_emergence_spawn_hooks.user)) {
            continue;
        }
        break;
    }
    if (direction >= 4) return NULL;
    if (!emergence_cell_world_position_get(cell_x, cell_y, origin) ||
        !emergence_cell_world_position_get(neighbor_x, neighbor_y, neighbor))
        return NULL;
    origin[0] = (origin[0] * 9.0f + neighbor[0] * 10.0f) / 19.0f;
    origin[1] = (origin[1] * 9.0f + neighbor[1] * 10.0f) / 19.0f;
    created = emergence_entity_create(base_entity, owner_index, origin[0],
                                      origin[1], headings[direction]);
    if (created == NULL) return NULL;
    created->flags_3e0 |= 0x100u;
    created->flags_3e8 &= 0x7fffffffu;
    entity_dispatch_event31_for_pending_flag(created);
    ai_set_activity(created, 0x0c, 0);
    ai_idle_chain_record_attach(created);
    message_dispatch(0x1f, 0u, (uint32_t)(uintptr_t)created, NULL);
    /* The reference does not attach a scene action here. It resolves the
     * type-3 entity's renderer component (legacy +0x14, then handle +4) and
     * submits positional sound alias 10 through 0x465050. EntitySlot only
     * retains the owner scene object, not that component wrapper, so this
     * sound call remains unavailable from the current EntitySlot state. */
    level_tile_mark(level, cell_x, cell_y, 1);
    created->pos_pair_2f4 = (float)cell_x;
    created->pos_pair_2f8 = (float)cell_y;
    {
        static const char* type_names[3] = {
            "RockMonster", "Lavamonster", "Icemonster"
        };
        unsigned int i;
        int type, row;
        for (i = 0u; i < 3u; ++i) {
            if (entity_type_row_for_name(type_names[i], &type, &row) &&
                created->type == type && created->etype == row) {
                message_dispatch((int)i + 2, 0u,
                                (uint32_t)(uintptr_t)created, NULL);
                break;
            }
        }
    }
    message_dispatch(0x27, 0u, (uint32_t)(uintptr_t)created, NULL);
    global_counter_at_00555324_increment();
    if (s_emergence_spawn_hooks.post_create != NULL)
        s_emergence_spawn_hooks.post_create(created, cell_x, cell_y,
                                            s_emergence_spawn_hooks.user);
    return created;
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x0043b270
void simtick_scene_pass(EngineContext* c, float dt)
{
    EngineState* st = c != NULL ? c->state_block : NULL;
    uint32_t scene_word = entity_state_flags_peek();
    if ((scene_word & 4u) != 0) {
        if (s_scene_tracked_count != 0u)
            scene_cell_previous_state_track(st); /* explicit test adapter */
        else
            terrain_cell_previous_state_track_current();
    }
    entity_state_flags_set(scene_word | 0x10u);
    /* FUN_004377d0(callback, &dt, 0): visit every live, unflagged entity and
     * run the recovered per-entity behavior callback. */
    entity_pool_find_by_callback_current(scene_entity_callback,
                                         (void*)&dt, 0);
    ++s_post_n;
    scene_word = entity_state_flags_peek() & ~0x10u;
    entity_state_flags_set(scene_word);
    if ((scene_word & 4u) != 0) {
        if (s_scene_pending_count != 0u)
            scene_cell_update_queue_flush(st); /* explicit test adapter */
        else
            terrain_cell_update_queue_flush_current();
    }
    if ((scene_word & 8u) == 0) {
        entity_state_flags_set(scene_word & ~4u);
    } else {
        entity_activity_scene_state_update(entity_pool_callback_context_get());
        if (s_scene_broadcast != NULL) {
            s_scene_broadcast(0);
        }
        ++s_scene_bcasts;
        entity_state_flags_set(entity_state_flags_peek() & ~8u);
    }
    s_scene_timer -= dt;
    if (s_scene_timer <= 0.0f) {
        s_scene_timer = 125.0f;
        periodic_entity_callback_dispatch((void*)(uintptr_t)1u);
        if (s_scene_broadcast != NULL) {
            s_scene_broadcast(1);
        }
        ++s_scene_bcasts;
    }
    /* FUN_0044bf00's global deferred callback list still needs its renderer
     * record/callback owner; no portable producer is installed for that list. */
}

/* EntityIterationCallbackGuard (FUN_0043b320): the ABI takes no arguments;
 * LAB_0043b350 tears down each entity admitted by the filtered pool walk. */
// FUNCTION: LEGORR 0x0043B320
void entity_iteration_callback_guard(void)
{
    s_entity_iteration_guard_flags |= 2u;
    entity_pool_find_by_callback_filtered_current(
        entity_pool_teardown_callback, NULL);
    s_entity_iteration_guard_flags &= ~2u;
}

/* TaskCountAtGridCoordinateQuery (FUN_0043b360) actually returns whether the
 * callback found an entity and writes that EntitySlot pointer through its
 * output dword. LAB_0043b3b0's eligibility gate and synchronous pool walk are
 * live; its remaining renderer geometry predicate is supplied by the hook. */
// FUNCTION: LEGORR 0x0043B360
int entity_task_at_grid_coordinate_find(uint32_t x, uint32_t y,
                                        EntitySlot** out_entity)
{
    EntityTaskAtGridCoordinateRequest request;
    GameContext* context;
    if (out_entity == NULL) return 0;
    *out_entity = NULL;
    context = s_entity_task_count_context != NULL ?
        s_entity_task_count_context : entity_pool_callback_context_get();
    if (context == NULL) return 0;
    request.x = x;
    request.y = y;
    request.out_entity = out_entity;
    return entity_pool_find_by_callback_filtered(
        context, entity_task_at_grid_coordinate_candidate, &request);
}

/* ClippedRectangleCallbackDispatch (FUN_0043b6c0): form the componentwise
 * rectangle union, visit entities, then enqueue opcode 0x2d unless renderer
 * state bit 8 suppresses the path. */
// FUNCTION: LEGORR 0x0043b6c0
void entity_clipped_rectangle_callback_dispatch(
    void* payload, const float rect_a[2], const float rect_b[2])
{
    float bounds[4];
    EntityClippedRectangleHooks* hooks = &s_entity_clipped_rectangle_hooks;
    if (rect_a == NULL || rect_b == NULL
        || (system_state_flags_get() & 8u) != 0u) return;
    bounds[0] = rect_a[0] < rect_b[0] ? rect_a[0] : rect_b[0];
    bounds[1] = rect_a[1] < rect_b[1] ? rect_a[1] : rect_b[1];
    bounds[2] = rect_a[0] > rect_b[0] ? rect_a[0] : rect_b[0];
    bounds[3] = rect_a[1] > rect_b[1] ? rect_a[1] : rect_b[1];
    if (hooks->visit_entities != NULL) {
        hooks->visit_entities(hooks->context, payload, bounds);
    }
    if (hooks->queue_command != NULL) {
        hooks->queue_command(hooks->context, 0x2d);
    }
    else {
        /* FUN_00452060 is the existing portable command producer for the
         * fixed opcode emitted by the retail tail. */
        cmd_produce(0x2d, NULL, 0, NULL);
    }
}
/* TypeTwoEntityEventDispatch (FUN_0043b820): resolve the supplied entity or
 * its attached owner to a type-2 record, then dispatch through its event hook. */
// FUNCTION: LEGORR 0x0043B820
void entity_type_two_event_dispatch(const EntitySlot* entity,
                                    uint32_t argument_a,
                                    uint32_t argument_b)
{
    const EntitySlot* selected = entity;
    if (selected == NULL) return;
    if (selected->type != 2) {
        selected = (const EntitySlot*)selected->owner_36c;
    }
    if (selected != NULL && selected->type == 2
        && s_entity_type_two_event_hook != NULL) {
        s_entity_type_two_event_hook(selected, argument_b, argument_a,
                                     s_entity_type_two_event_context);
    }
}

/* TypeTwoEntityPositionedCallbackDispatch (FUN_0043b850): select a type-2
 * source (or its linked type-2 owner), capture its scene position when mode
 * is nonzero, then update per-candidate renderer state through the filtered
 * live-entity pool. */
// FUNCTION: LEGORR 0x0043B850
void entity_type_two_positioned_callback_dispatch(
    const EntitySlot* entity, void* scene_object, float inner_range,
    float outer_range, int mode)
{
    const EntitySlot* selected = entity;
    EntityTypeTwoPositionedDispatch dispatch;
    if (selected == NULL) return;
    if (selected->type != 2) {
        const EntitySlot* owner = (const EntitySlot*)selected->owner_36c;
        if (owner != NULL && owner->type == 2) selected = owner;
    }

    memset(&dispatch, 0, sizeof(dispatch));
    dispatch.source = selected;
    dispatch.inner_range = inner_range;
    dispatch.outer_range = outer_range;
    dispatch.mode = mode;
    if (mode != 0) {
        EntityTypeTwoPositionedHooks* hooks =
            &s_entity_type_two_positioned_hooks;
        if (hooks->position_get != NULL) {
            dispatch.source_position_valid = hooks->position_get(
                hooks->context, scene_object, dispatch.source_position) != 0;
        }
        else {
            dispatch.source_position_valid = ddiscene_object_position_get(
                scene_object, NULL, dispatch.source_position) != 0;
        }
    }
    (void)entity_pool_find_by_callback_filtered_current(
        entity_type_two_positioned_candidate, &dispatch);
}
/* EntityFlagsMask107C33DEOr1C00203AnyTest (FUN_0043baf0, finding 319):
 * either flag word intersects its literal mask. */
// FUNCTION: LEGORR 0x0043baf0
int entity_flags_mask107c33de_or_1c00203_any_test(const EntitySlot* e)
{
    if ((e->flags_3e0 & 0x107c33deu) != 0u) {
        return 1;
    }
    return (e->flags_3e4 & 0x01c00203u) != 0u;
}

/* BuildingDepositStateMonitor (FUN_0043bb20): clear stale deposit state
 * when the carried count, capability threshold, or state flags no longer
 * permit it; otherwise request the configured deposit action when supported. */
// FUNCTION: LEGORR 0x0043BB20
void entity_building_deposit_state_monitor(EntitySlot* entity)
{
    uint32_t capabilities;
    uint32_t threshold;
    if (entity == NULL || entity->type != 4) return;
    capabilities = entity_capability_flags_get(entity);
    if ((capabilities & 2u) == 0u) {
        threshold = (uint32_t)entity->carry_state_324;
    }
    else {
        EntityHealthThresholdState state;
        memset(&state, 0, sizeof(state));
        threshold = (uint32_t)entity->field_328;
        if (s_entity_health_threshold_read_hook != NULL
            && s_entity_health_threshold_read_hook(
                entity, &state, s_entity_health_threshold_context)
            && state.upper_limit < threshold) {
            threshold = state.upper_limit;
        }
    }
    if ((uint32_t)entity->carry_state_324 < threshold
        || (entity->flags_3e0 & 0x2000u) != 0u
        || (entity->flags_3e8 & 0x01000000u) != 0u) {
        entity->flags_3e8 &= ~0x01000000u;
        return;
    }
    if ((capabilities & (4u | 2u)) != 0u) {
        /* FUN_0043bb20 calls EntityDepositActivityGate(entity, NULL)
         * directly once the carried-count and state gates pass. Preserve the
         * port's optional integration callback when one is explicitly set. */
        if (s_entity_building_deposit_hook != NULL)
            s_entity_building_deposit_hook(entity,
                                           s_entity_building_deposit_context);
        else
            (void)entity_deposit_activity_gate(entity, NULL);
    }
}

/* EntityDetachCarriedObjectAndResetState (FUN_0043bbc0) — see entity.hpp.
 * The carried half is read through the link as a native slot pointer; the
 * corpus strides its dword words blindly. */
// FUNCTION: LEGORR 0x0043bbc0
void entity_detach_carried_object_and_reset_state(EntitySlot* e,
                                                  LevelState* lvl)
{
    if (e == NULL) {
        return; /* deviation: the corpus dereferences the slot blindly */
    }
    if (e->type != 3) {
        return;
    }
    EntitySlot* carried = (EntitySlot*)e->carry_link_308;
    if (carried != NULL && carried->type == 6) {
        e->carry_link_308 = NULL;
        e->carry_state_324 = 0;
        carried->attach_304 = NULL;
        e->flags_3e0 &= ~0x400u;
        carried->field_340 = -1.0f;
    }
    if (e->carry_ref_348 != NULL) {
        level_state_field9c_add_for_level(lvl, *(const int*)e->carry_ref_348);
    }
    e->field_340 = -1.0f;
    e->flags_3e8 |= 0x800000u;
}

// FUNCTION: LEGORR 0x0043bc40
int objective_sidecar_append_gate(EntitySlot* entity)
{
    GameContext* ctx;
    LevelState* level;
    uint32_t stats_flags;
    int cell_x = 0;
    int cell_y = 0;
    int cell_xy[2];
    int cell_valid = 0;
    EntitySlot* owner;

    if (entity == NULL) return 0;
    ctx = entity->owner_ctx;
    if (ctx == NULL) ctx = entity_pool_callback_context_get();
    level = ctx != NULL ? ctx->level : NULL;

    if (entity->type != 4
        && ((entity->flags_3e4 & 0x08000000u) != 0u
            || (entity->flags_3e0 & 0x00200000u) != 0u)) return 0;
    if (entity->field_340 > 0.0f) return 0;

    stats_flags = entity_stats_word_get(entity);

    /* FUN_0043bc40 first completes a linked +0x3cc child when the source
     * stats word has one of the three documented state groups. EntitySlot's
     * spawn_child_3cc mirror owns that exact link. */
    if ((stats_flags & 0x80000070u) != 0u &&
        entity->spawn_child_3cc != NULL &&
        (entity->spawn_child_3cc->flags_3e0 & 0x00200000u) != 0u) {
        EntitySlot* child = entity->spawn_child_3cc;
        child->field_340 = -1.0f;
        child->flags_3e0 &= ~0x00200000u;
        entity_kind_record_update_dispatch(entity);
        ai_set_activity(child, 0, 1);
        activity_register_bookkeeping(child);
    }

    /* The first linked-activity path reads the pointer at +0x3cc. That field
     * is represented by spawn_child_3cc; it is distinct from carried-object
     * (+0x308) and reciprocal pair (+0x3c0) links. */

    if ((entity->flags_3e8 & 0x00800000u) != 0u) {
        objective_sidecar_owner_detach(ctx, entity);
        if (entity->type == 2 && (entity->flags_3ec & 8u) != 0u
            && ctx != NULL) {
            osf_add(ctx, (int32_t)entity->flags_3f0,
                    (int32_t)entity->indexed_record_slot_330,
                    entity->name);
        }
        /* FUN_0046cf90 is a resource/display callback with no game-side
         * equivalent. The remaining spawn/finalize/teardown operations are
         * represented by existing module APIs. */
        if (entity->type == 1) {
            /* Deliberately no guessed replacement for FUN_0046cf90. */
        }
        lego::entity_kind_one_random_spawn_trigger(entity);
        if (entity->type != 5) {
            if (entity->type == 4)
                entity_destroy_tracked_pointer_cleanup(entity);
            else
                (void)entity_teardown(entity);
            return 1;
        }
        entity_type5_finalize_if_allowed(entity);
        return 1;
    }

    if (entity->type == 3) {
        if ((entity_state_flags_get(entity) & 0x8000u) == 0u) {
            if ((entity_state_flags_get(entity) & 0x00100000u) == 0u)
                return 0;
            entity_state_reset(entity, 0, 1);
            entity->flags_3e0 |= 0x00020000u;
            objective_sidecar_activity_set(entity, 0x0d);
            return 1;
        }
        if ((entity->flags_3e0 & 0x00100000u) != 0u) return 0;
        entity_action_route_state_initialize(entity);
        return 1;
    }

    /* The type-4 path branch reads the dword at +0x390 and the byte at
     * +0x3ec. Retail applies the BuildingTypes cell-line effects only when
     * both veto bits are clear, then marks the one-shot bit before calling
     * 0x409d50. */
    if (entity->type == 4 && (entity->flags_390 & 4u) == 0u &&
        (entity->flags_3ec & 8u) == 0u) {
        if ((entity->flags_3e4 & 0x00100000u) != 0u) return 0;
        entity->flags_3e4 |= 0x00100000u;
        unported_ledger_stubs::entity_cell_path_effects_apply_current(entity);
        objective_sidecar_activity_set(entity, 0x2a);
        task_registry_remove_type10_entries_for_entity(entity);
        return 1;
    }

    if (entity->type != 1 && entity->type != 2 && entity->type != 0xb
        && entity->type != 4 && entity->type != 6) return 0;
    if (entity->type == 6) {
        entity->flags_3e8 |= 0x00800000u;
        return 1;
    }

    if ((entity->flags_3e8 & 0x00040000u) == 0u
        || (entity->flags_3e0 & 0x00400000u) != 0u) return 0;

    entity_position_to_terrain_cell(entity, level, &cell_x, &cell_y);
    cell_valid = level != NULL;
    if ((entity->flags_3ec & 8u) != 0u)
        lego::entity_kind_one_random_spawn_trigger(entity);

    if ((entity->flags_3ec & 0x80u) != 0u) {
        owner = (EntitySlot*)entity->owner_36c;
        if (owner != NULL) owner->flags_3ec &= ~0x80u;
        entity->owner_36c = NULL;
    }

    if (entity->owner_36c != NULL && cell_valid) {
        EntitySlot* carried = (EntitySlot*)entity->carry_link_308;
        owner = (EntitySlot*)entity->owner_36c;
        if ((stats_flags & 1u) != 0u
            && (entity->flags_3e0 & 0x400u) != 0u
            && carried != NULL
            && terrain_cell_route_and_occupancy_check(
                   carried, level, cell_x, cell_y, cell_x, cell_y, 1) == 0) {
            carried->field_340 = -1.0f;
        }
        if ((stats_flags & 0x20000u) == 0u
            || terrain_cell_route_and_occupancy_check(
                   owner, level, cell_x, cell_y, cell_x, cell_y, 1) == 0) {
            objective_sidecar_owner_detach(ctx, entity);
        }
        else {
            owner->field_340 = -1.0f;
        }
    }

    if (entity->type == 0xb)
        terrain_entity_cell_record_remove(entity);

    if (cell_valid) {
        cell_xy[0] = cell_x;
        cell_xy[1] = cell_y;
        if (entity->type == 2 || entity->type == 1 || entity->type == 4)
            message_dispatch(0x26, 0, 0, cell_xy);
        if (entity->type == 2)
            message_dispatch(10, 0, 0, cell_xy);
        else if (entity->type == 1)
            message_dispatch(0xb, 0, 0, cell_xy);
        else if (entity->type == 4)
            message_dispatch(0xc, 0, 0, cell_xy);
    }

    if ((entity->flags_3e4 & 2u) == 0u) {
        entity_state_reset(entity, 0, 1);
        entity->activity_last = entity->activity;
    }
    else {
        entity_state_reset(entity, 1, 1);
    }
    entity->flags_3e0 |= 0x00400000u;
    /* Renderer sidecar allocation, transform copy, and labels are absent
     * from GameContext. The final selection detach is directly represented. */
    (void)selection_entity_remove(entity);
    selection_object_detach(entity);
    return 1;
}

/* EntityStatePairPredicate (FUN_0043c200): the (type, row) stats pair
 * predicate. A zero first read (FUN_00469e00(type1, row1)) answers 1 with
 * no walk (pinned); otherwise the filtered pool walk (the FUN_004377b0
 * path — liveness mirror + the raised bit-0x80000 gate) folds every live
 * entity's +0xDC stats float through the nested add (FUN_0043c280), then
 * both table re-reads join the fold and the answer is 0 when the sum
 * touches or crosses zero (pinned comparison).
 * Deviations: the corpus rides param_2 as the row index and the float
 * accumulator slot in one register — the port keeps them apart, the
 * accumulator starting 0.0 as the corpus resets it before the walk; the
 * corpus folds the three floats in float10, the port in double (exact for
 * float-magnitude triples); the walk composes the corpus's global type
 * tables over the ctx entity blocks (precedent in the tick above); and
 * NULL ctx skips the walk — the table reads still answer. */
// FUNCTION: LEGORR 0x0043c200
int entity_state_pair_predicate(GameContext* ctx, int type1, int row1,
                                int type2, int row2)
{
    if (stats_table_float_at_dc(type1, row1) == 0.0f) {
        return 1; /* pinned: the zero first read short-circuits */
    }
    float fold = 0.0f;
    if (ctx != NULL) {
        for (int t = 0; t < ENTITY_TYPES; ++t) {
            for (int i = 0; i < ctx->entity_count[t]; ++i) {
                EntitySlot* e = &ctx->entity[t][i];
                if (e->type == 0) {
                    continue; /* *ent+0x408 == ent liveness mirror */
                }
                if ((e->flags_3e8 & 0x80000u) != 0u) {
                    continue; /* filtered walk raises that gate */
                }
                entity_nested_float_add_unless_blocked(e, &fold);
            }
        }
    }
    const double sum = (double)stats_table_float_at_dc(type1, row1)
        + (double)stats_table_float_at_dc(type2, row2) + (double)fold;
    return sum <= 0.0 ? 0 : 1;
}
// FUNCTION: LEGORR 0x0043c280
int entity_nested_float_add_unless_blocked(EntitySlot* e, float* value)
{
    if (e == NULL) {
        return 0; /* deviation: the port guards the entity, the corpus strides the value blindly */
    }
    if (e->type == 4 && entity_shared_eligibility_check(e, 0) == 0) {
        return 0;
    }
    *value = entity_stats_float_atdc(e) + *value;
    return 0; /* corpus answers 0 on both paths */
}

/* PeriodicTimer25Advance (FUN_0043c2b0): add the elapsed time, reset after
 * 25 units, then dispatch the filtered live-entity tick callback. */
// FUNCTION: LEGORR 0x0043C2B0
void periodic_timer25_advance(GameContext* ctx, float delta)
{
    int table;
    s_periodic_timer25 += delta;
    if (s_periodic_timer25 > 25.0f) {
        s_periodic_timer25 = 0.0f;
    }
    if (ctx == NULL || s_entity_periodic_tick_hook == NULL) {
        return;
    }
    for (table = 0; table < ENTITY_TYPES; ++table) {
        int count = ctx->entity_count[table];
        int index;
        if (count < 0) count = 0;
        if (count > ENTITY_CAP) count = ENTITY_CAP;
        for (index = 0; index < count; ++index) {
            EntitySlot* entity = &ctx->entity[table][index];
            if (entity->type != 0 && (entity->flags_3e8 & 0x80000u) == 0u) {
                s_entity_periodic_tick_hook(entity, delta,
                                            s_entity_periodic_tick_context);
            }
        }
    }
}

// FUNCTION: LEGORR 0x0043c3e0
/* EntityType26CommandCompletionGate (FUN_0043c3e0): only type-2 entities
 * carrying a kind-0x1a command are considered; distance acceptance followed
 * by a clear ray check completes the gate. */
int entity_type26_command_completion_gate(const EntitySlot* entity)
{
    const void* command;
    const EntitySlot* target;
    if (entity == NULL || entity->type != 2) return 0;
    command = entity->cmd_head;
    if (command == NULL || cmd_rec_type(command) != 0x1a) return 0;
    target = (const EntitySlot*)cmd_rec_payload(command);
    if (target == NULL || !entity_centers_within_configured_radius(entity,
                                                                  target)) {
        return 0;
    }
    return entity_pair_ray_hit_check(entity, target) == 0;
}

/* FUN_0043c440: the can-do activities, checked in corpus order —
 * drill (1), teleport (2), reinforce (3), else 0. */
// FUNCTION: LEGORR 0x0043c440
int ai_activity_probe(const EntitySlot* e)
{
    if (ai_activity_scan(e, AI_ACT_DRILL)) {
        return 1;
    }
    if (ai_activity_scan(e, AI_ACT_TELEPORT)) {
        return 2;
    }
    return ai_activity_scan(e, AI_ACT_REINFORCE) ? 3 : 0;
}

/* TaskTypeAndConditionCheck (FUN_0043c490) — see entity.hpp. */
// FUNCTION: LEGORR 0x0043c490
int task_type_and_condition_check(const void* unused,
                                  const EntitySlot* target, int condition)
{
    (void)unused;
    if (entity_shared_eligibility_check(target, 1) == 0 || condition == 0) {
        return 0;
    }
    return 1;
}
/* EntityFinishState1A (FUN_0043c4c0): finish the linked target's face
 * action when the port can represent that target, clear the finishing bit,
 * and discard the entity's type-0x1a command record. */
// FUNCTION: LEGORR 0x0043c4c0
void entity_finish_state_1a(EntitySlot* entity)
{
    EntitySlot* target;
    if (entity == NULL) return;
    if (entity->link_2fc != NULL) {
        target = entity->link_2fc;
        if (target->type == ENTITY_TYPE_OBJ && entity->wake_timer <= 0.0f)
            (void)entity_face_target_action_start(entity, target, 1);
    }
    entity->flags_3e4 &= ~0x04000000u;
    (void)cmd_chain_drop(entity, 0x1a);
}

/* EntityMarkAndFinishState1A (FUN_0043c530): the corpus only marks a type-2
 * entity when its command head is state 0x1a, then shares the finish path. */
// FUNCTION: LEGORR 0x0043c530
void entity_mark_and_finish_state_1a(EntitySlot* entity)
{
    if (entity == NULL || entity->type != 2 || entity->cmd_head == NULL ||
        cmd_rec_type(entity->cmd_head) != 0x1a) return;
    entity->flags_3e4 |= 0x04000000u;
    entity_finish_state_1a(entity);
}

/* KindFourPostCreateResolve (FUN_0043c570): clear the pending marker and
 * register the kind-four object when its nearby resource search fails;
 * otherwise keep the marker and expand the adjacent-cell state when the
 * observed stats and entity-eligibility gates pass. */
// FUNCTION: LEGORR 0x0043c570
void entity_kind_four_post_create_resolve(EntitySlot* entity)
{
    GameContext* context;
    LevelState* level;
    uint32_t flags;
    int cell_xy[2];
    int have_cell;
    if (entity == NULL || entity->type != 4 ||
        (entity->flags_3e8 & 0x80000000u) != 0u) return;
    level = entity->owner_ctx != NULL ? entity->owner_ctx->level : NULL;
    if (level == NULL) {
        context = entity_pool_callback_context_get();
        level = context != NULL ? context->level : NULL;
    }
    else {
        context = entity->owner_ctx;
    }
    have_cell = 0;
    cell_xy[0] = -1;
    cell_xy[1] = -1;
    if (level != NULL) {
        entity_position_to_terrain_cell(entity, level, &cell_xy[0],
                                        &cell_xy[1]);
        have_cell = level_coord_pair_within_bounds(
            level, cell_xy[0], cell_xy[1]);
    }
    flags = entity_state_flags_get(entity);
    if ((flags & (0x200u | 0x400u)) == 0u && have_cell &&
        !entity_kind_four_candidate_find(context, level, cell_xy,
            (int)stats_instance_word_at_bc(entity))) {
        entity->flags_3e8 &= ~0x20000000u;
        type4_object_registry_insert(entity);
        return;
    }
    entity->flags_3e8 |= 0x20000000u;
    if ((flags & 0x200u) != 0u && have_cell &&
        entity_shared_eligibility_check(entity, 0)) {
        /* FUN_0041f8e0 is empty, so its return-register residue preserves
         * the preceding nonzero eligibility result in the original call. */
        entity_kind_four_adjacent_expand(level, cell_xy[0], cell_xy[1]);
    }
}

namespace unported_ledger_stubs {
/* EntityCapacityOrStatePredicate (FUN_0043c650): gate on stats bit 0x80000,
 * compare the indexed stats-table +0xe0 value against the referenced count,
 * then reject an entity carrying the +0x3e8 high-bit state. */
// FUNCTION: LEGORR 0x0043c650
int entity_capacity_or_state_predicate(const EntitySlot* entity)
{
    uint32_t stats_flags;
    uint32_t limit;
    uint32_t current = 0u;
    if (entity == NULL) return 0;
    stats_flags = entity_stats_word_get(entity);
    if ((stats_flags & 0x80000u) == 0u) return 0;
    limit = stats_instance_word_at_e0(entity);
    if (entity->carry_ref_348 != NULL)
        memcpy(&current, entity->carry_ref_348, sizeof(current));
    if (limit != 0u && entity->carry_ref_348 != NULL && limit <= current)
        return 0;
    return (entity->flags_3e8 & 0x80000000u) == 0u ? 1 : 0;
}
} // namespace unported_ledger_stubs

/* EntityActivityCountersAdvance (FUN_0043c6b0): accumulate the frame delta
 * into the three per-entity counters. Type 2 emits the two configured
 * threshold messages once on each upward crossing; kinds 6, 7, 9, and 0xb
 * schedule deferred work each time the first counter reaches 750. */
// FUNCTION: LEGORR 0x0043c6b0
void entity_activity_counters_advance(EntitySlot* entity, float elapsed)
{
    float old_3d8;
    float old_3d4;
    float threshold;
    const LevelState* level;
    if (entity == NULL) return;
    old_3d8 = entity->activity_counter_3d8;
    old_3d4 = entity->activity_counter_3d4;
    entity->activity_counter_3d8 += elapsed;
    entity->activity_counter_3d4 += elapsed;
    entity->activity_counter_3dc += elapsed;
    level = entity->owner_ctx != NULL ? entity->owner_ctx->level : NULL;
    if (entity->type == 2) {
        threshold = engine_float_field210_get_by_index(level, 2);
        if (threshold != 0.0f
            && threshold <= entity->activity_counter_3d8
            && old_3d8 < threshold) {
            message_dispatch(0x22, 0,
                (uint32_t)(uintptr_t)entity, NULL);
        }
        threshold = engine_float_field260_get(level);
        if (threshold != 0.0f
            && threshold <= entity->activity_counter_3d4
            && old_3d4 < threshold) {
            message_dispatch(0x23, 0,
                (uint32_t)(uintptr_t)entity, NULL);
            return;
        }
    }
    else if ((entity->type == 6 || entity->type == 7 || entity->type == 9
              || (entity->type == 0xb
                  && (entity->flags_3e4 & 0x02000000u) == 0u))
             && 750.0f <= old_3d8 + elapsed && old_3d8 < 750.0f) {
        entity->activity_counter_3d8 = 0.0f;
        entity_deferred_work_schedule(entity);
    }
}

/* LinkedEntityRouteFlagClear (FUN_0043f100, finding 428): the corpus
 * clears bit 0x40 in +0x3ec on the entity and on its optional +0x2fc
 * linked entity. The port stores those words in EntitySlot mirrors; a
 * null-safe guard documents the port's deviation from the unchecked body. */
// FUNCTION: LEGORR 0x0043f100
void entity_linked_route_flag_clear(GameContext* ctx, EntitySlot* e)
{
    (void)ctx; /* the corpus's first parameter is not read */
    if (e == NULL) {
        return;
    }
    e->flags_3ec &= ~0x40u;
    if (e->link_2fc != NULL) {
        EntitySlot* linked = e->link_2fc;
        linked->flags_3ec &= ~0x40u;
    }
}
/* EntitySpawnRequestQueueAdd (FUN_0043f150): append the two request words
 * while the corpus's nine-slot queue has room. The first argument is an
 * unused context-shaped parameter in the original. */
// FUNCTION: LEGORR 0x0043f150
int entity_spawn_request_queue_add(GameContext* ctx, int type,
                                  int request_kind)
{
    (void)ctx;
    if (s_entity_spawn_request_count >= 9u) {
        return 0;
    }
    s_entity_spawn_requests[s_entity_spawn_request_count].type = type;
    s_entity_spawn_requests[s_entity_spawn_request_count].kind = request_kind;
    s_entity_spawn_request_count++;
    return 1;
}

/* EntitySpawnRequestQueueProcess (FUN_0043f190) applies the decompiled
 * eligibility, child-state, request-mask, capacity-residue, and pair-state
 * gates before consuming a request. EntitySlot carries the simulation child
 * link; creation of the retail +0x32c scene wrapper remains renderer-owned. */
// FUNCTION: LEGORR 0x0043f190
void entity_spawn_request_queue_process(EntitySlot* requester)
{
    EntitySlot* existing_child;
    GameContext* context;
    uint32_t requester_flags;
    if (requester == NULL || requester->owner_ctx == NULL
        || s_entity_spawn_request_count == 0u
        || entity_shared_eligibility_check(requester, 0) == 0
        || (requester->flags_3ec & 0xc0u) != 0u) {
        return;
    }
    context = requester->owner_ctx;
    existing_child = requester->spawn_child_3cc;
    if (existing_child != NULL &&
        (entity_stats_word_get(existing_child) & 0x200000u) != 0u) return;
    requester_flags = entity_stats_word_get(requester);
    for (uint32_t i = 0; i < s_entity_spawn_request_count; ++i) {
        const EntitySpawnRequest* request = &s_entity_spawn_requests[i];
        const int type = request->type;
        const char* type_name = NULL;
        uint32_t config_kind = 0u;
        float position[3];
        EntitySlot* spawned;
        if (type < 1 || type > 19 ||
            !entity_spawn_request_pair_matches(request, requester, context))
            continue;
        if (type == 2) config_kind = 1u;
        else if (type == 3) config_kind = 3u;
        else if (type == 4) config_kind = 2u;
        if (config_kind != 0u) {
            const TypeConfigTable* type_table =
                entity_type_config_table_get(config_kind);
            if (type_table != NULL && type_table->names != NULL &&
                (uint32_t)request->kind < type_table->count) {
                type_name = type_table->names[request->kind];
            }
        }
        spawned = entity_create_with_position(context, type, type_name, 0,
                                              0.0f, 0.0f, 0.0f);
        if (spawned == NULL) continue;
        spawned->etype = request->kind;
        (void)entity_building_index_set(spawned, 0u);
        requester->spawn_child_3cc = spawned;
        global_object_field8c_subtract_for_level(context->level,
            (int)stats_table_word_at_b4(type, request->kind, 0));
        entity_activity_scene_state_update(context);
        if ((requester_flags & 0x40u) != 0u) {
            requester->flags_3ec |= 0x40u;
            spawned->flags_3ec |= 0x40u;
        }
        else if ((entity_stats_word_get(spawned) & 0x20000u) != 0u) {
            requester->flags_3ec |= 0x80u;
            spawned->flags_3ec |= 0x80u;
            spawned->link_2fc = requester;
        }
        position[0] = requester->x;
        position[1] = requester->y;
        position[2] = requester->z;
        if (requester->attached_scene_object != NULL) {
            (void)ddiscene_object_position_get(
                requester->attached_scene_object, NULL, position);
        }
        entity_world_position_set(spawned, position[0], position[1],
                                  requester->rot, 1);
        spawned->z = position[2];
        (void)entity_state_override_pop(spawned);
        spawned->flags_3e0 |= 0x200000u;
        ai_set_activity(spawned, 0x4e, 1);
        activity_register_bookkeeping(spawned);
        s_entity_spawn_request_count--;
        while (i < s_entity_spawn_request_count) {
            s_entity_spawn_requests[i] = s_entity_spawn_requests[i + 1u];
            ++i;
        }
        return;
    }
}

/* EntityPoolFindWithLocalPredicate (FUN_0043f560): the original dispatches
 * a local predicate over the filtered entity pool during teardown and
 * state changes. Its LAB body is not included in the decompile corpus; the
 * port models the common reverse-link cleanup for both known entity links. */
// FUNCTION: LEGORR 0x0043f560
int entity_pool_find_with_local_predicate(GameContext* ctx,
                                          EntitySlot* target)
{
    if (target == NULL) {
        return 0;
    }
    return entity_pool_find_by_callback_filtered(
        ctx, entity_linked_reference_matches, target);
}
/* EntityDamageApply (FUN_0043f6a0): the corpus subtracts from the float at
 * +0x340, not the separate `.ol` integer health field. Preserve the visible
 * kind-4 progress event, kind-2 accumulator, and positive damage flags. */
// FUNCTION: LEGORR 0x0043f6a0
void entity_damage_apply(EntitySlot* e, float damage, int emit_effects)
{
    if (e == NULL || entity_shared_eligibility_check(e, 1) == 0
        || e->attach_304 != NULL) {
        return;
    }
    if (e->type == ENTITY_TYPE_BUILDING && damage != 0.0f &&
        e->field_340 == 100.0f) {
        entity_threshold_progress_event_queue(e, 0);
    }
    /* EntityDamageApply tests the kind-3 +0x3e8 bit before comparing the
     * integer conversions of HealthDecayRate (+0x80) and incoming damage.
     * The retail path adds the damage to DAT_00555330 when the latter is
     * larger; the typed stats accessor and CRT conversion preserve that
     * behavior without assuming retail structure offsets in EntitySlot. */
    if (entity_type3_flag04000000_test(e) != 0 &&
        crt_float_to_long(entity_stats_float_at80(e)) <
            crt_float_to_long(damage)) {
        global_float_accumulator_00555330_add(damage);
    }
    if (e->type == ENTITY_TYPE_MINIFIG) {
        global_float_accumulate_00555120(damage);
    }
    e->field_340 = e->field_340 - damage;
    if (emit_effects != 0 && damage > 0.0f) {
        e->flags_3e4 |= 0x3000u;
        flagged_object_timer_registry_insert(e);
    }
}

namespace unported_ledger_stubs {
// EntityEnergyLowTargetRouteCommandTick
// FUNCTION: LEGORR 0x0043f7d0
void ledger_stub_0043f7d0_EntityEnergyLowTargetRouteCommandTick(
    EntitySlot* entity, float delta)
{
    float damage = 0.0f;
    float terrain_damage = 0.0f;
    LevelState* level;
    int cell_x, cell_y;
    if (entity == NULL || entity->type == 10 || entity->type == 5) return;
    level = (entity->owner_ctx != NULL) ? entity->owner_ctx->level : NULL;
    if (entity->field_340 <= 0.0f) {
        entity->field_340 = 0.0f;
    }
    else if ((entity->flags_3e8 & 0x80000000u) == 0u) {
        damage = entity_energy_decay_rate_get(entity) * 0.04f * delta;
        if (damage > 0.0f) entity_damage_apply(entity, damage, 0);
    }
    if (entity->progress_344 <= 0.0f) {
        entity->progress_344 = 0.0f;
    }
    else if ((entity->flags_3e8 & 0x80000000u) == 0u) {
        const float decay = entity_stats_float_atdc(entity) * 0.04f * delta;
        entity->progress_344 -= decay;
        if (entity->progress_344 < 0.0f) entity->progress_344 = 0.0f;
    }

    if (level != NULL) {
        uint8_t* cell;
        entity_position_to_terrain_cell(entity, level, &cell_x, &cell_y);
        cell = level_voxel(level, cell_x, cell_y);
        if (entity->attach_304 == NULL && cell != NULL && cell[2] == 6 &&
            (entity->type == 6 || entity->type == 7)) {
            level_state_field9c_add_for_level(level, 1);
            entity->field_340 = -4.0f;
            entity->flags_3e8 |= 0x00800000u;
        }
        if (entity_cell_type_speed_multiplier_resolve(
                entity, level, cell_x, cell_y, delta, &terrain_damage)) {
            entity_damage_apply(entity, terrain_damage, 1);
        }
    }

    if ((entity_state_flags_get(entity) & 0x00010000u) != 0u &&
        (entity->flags_3e0 & 0x00100100u) == 0u &&
        (entity->flags_3e4 & 0x08000000u) == 0u) {
        (void)entity_fence_spark_terrain_interaction(entity);
    }

    /* FUN_0043f7d0: low-resource type-2 entities seek a nearby candidate,
     * then queue its pointer only after one of the four adjacent destination
     * cells passes the terrain route search. */
    if (entity->type == 2 && entity->owner_ctx != NULL &&
        entity->owner_ctx->cfg != NULL &&
        (entity->owner_ctx->cheat & 0x200000u) == 0u &&
        entity->progress_344 < entity->field_340 * 0.01f *
            block_get_float(entity->owner_ctx->cfg,
                            "Main::MinEnergyForEat", 0.0f) &&
        entity->activity_counter_3d4 > 125.0f) {
        /* The source clears +0x3d4 as part of the timed gate before testing
         * the carried-object link and pending-command predicate. */
        entity->activity_counter_3d4 = 0.0f;
        if (entity->owner_36c == NULL &&
            !cmd_chain_contains_kind(entity, 0x11)) {
            EntitySlot* candidate = (EntitySlot*)
                ledger_stub_00438c60_EntityCandidateSearch(entity);
            if (candidate != NULL && level != NULL) {
                static const int neighbor_x[4] = { 0, 1, 0, -1 };
                static const int neighbor_y[4] = { -1, 0, 1, 0 };
                int start_x, start_y, target_x, target_y;
                unsigned int i;
                entity_position_to_terrain_cell(entity, level,
                                                 &start_x, &start_y);
                entity_position_to_terrain_cell(candidate, level,
                                                 &target_x, &target_y);
                for (i = 0u; i < 4u; ++i) {
                    int* route_x = NULL;
                    int* route_y = NULL;
                    float route_count = 0.0f;
                    if (terrain_neighbor_path_check_current(
                            entity, (uint32_t)start_x, (uint32_t)start_y,
                            (uint32_t)(target_x + neighbor_x[i]),
                            (uint32_t)(target_y + neighbor_y[i]),
                            &route_x, &route_y, &route_count, NULL, NULL)) {
                        free(route_x);
                        free(route_y);
                        entity_target_command_queue(entity, candidate);
                        break;
                    }
                    free(route_x);
                    free(route_y);
                }
            }
        }
    }
    if (entity->field_340 < entity->progress_344)
        entity->progress_344 = entity->field_340;
    (void)entity_damage_hurt_sound_submit(entity, delta, terrain_damage);
    if (entity->activity_param != 0 && entity->activity_timer >= 0.0f)
        entity->activity_timer += delta;
}

// FUNCTION: LEGORR 0x0043fc20
int ledger_stub_0043fc20_RecordPairPoseCopyAndActivityRaise(
    EntitySlot* entity)
{
    EntitySlot* attached;
    EntityScenePose source_pose;
    float position[3];
    LevelState* level;
    int cell_x, cell_y;
    if (entity == NULL || entity->owner_ctx == NULL ||
        s_entity_pair_pose_hooks.pose_read == NULL ||
        s_entity_pair_pose_hooks.pose_apply == NULL) return 0;
    attached = (EntitySlot*)entity->attach_304;
    if (attached == NULL ||
        !entity_world_center_position_read(attached, position)) return 0;
    if (s_entity_pair_pose_hooks.scene_present != NULL &&
        (!s_entity_pair_pose_hooks.scene_present(
             s_entity_pair_pose_hooks.context, entity) ||
         !s_entity_pair_pose_hooks.scene_present(
             s_entity_pair_pose_hooks.context, attached))) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    cell_x = level_cell_of(position[0]);
    cell_y = level_cell_of(position[1]);
    if (!terrain_cell_route_and_occupancy_check(entity, level,
            cell_x, cell_y, cell_x, cell_y, 0)
        || !s_entity_pair_pose_hooks.pose_read(
            s_entity_pair_pose_hooks.context, attached, &source_pose)) return 0;

    entity->attach_304 = NULL;
    attached->carry_link_308 = NULL;
    attached->flags_3e0 &= ~0x400u;
    attached->carry_state_324 = 0;
    s_entity_pair_pose_hooks.pose_apply(s_entity_pair_pose_hooks.context,
                                        entity, &source_pose);
    entity_activity_toggle(attached, 1);
    ai_idle_chain_record_attach(attached);
    ai_set_activity(entity, 0x34, 0);
    (void)activity_register_bookkeeping(entity);
    ai_idle_chain_record_attach(entity);
    entity->link_2fc = attached;
    entity->flags_3ec |= 0x8000u;
    if (s_entity_pair_pose_hooks.activity_raise != NULL)
        s_entity_pair_pose_hooks.activity_raise(
            s_entity_pair_pose_hooks.context, attached, 1);
    return 1;
}
} // namespace unported_ledger_stubs

/* EntityUpdateAndUnregisterAtTile (FUN_00440130) — see ai.hpp. The corpus
 * strides its idle-registration table (DAT_004eabcc / count DAT_004ead5c,
 * the port's s_idle100); a hit stores the link word through the +0x2fc
 * mirror, runs the linked-entity update (FUN_004440a0), and blanks the
 * slot. Deviation: the corpus strides the table blindly; the port scans
 * the NULL-skipping port table. */
// FUNCTION: LEGORR 0x00440130
void entity_update_and_unregister_at_tile(int x, int y, void* link)
{
    for (int i = 0; i < s_idle100_n; ++i) {
        EntitySlot* e = s_idle100[i];
        if (e == NULL) {
            continue;
        }
        if (level_cell_of(e->x) == x && level_cell_of(e->y) == y) {
            e->link_2fc = (EntitySlot*)link;        /* entity +0x2fc */
            linked_entity_flag_and_command_update(e); /* FUN_004440a0 */
            s_idle100[i] = NULL;                    /* unregister */
        }
    }
}

namespace unported_ledger_stubs {
// TerrainCellToWorldPosition
// FUNCTION: LEGORR 0x004401b0
int ledger_stub_004401b0_TerrainCellToWorldPosition(EntitySlot* entity,
                                                     int force)
{
    EntitySlot* owner;
    EntitySlot* linked;
    LevelState* level;
    int cell_x, cell_y;
    int should_transfer = 0;
    int target_pose_valid = 0;
    int linked_pose_valid = 0;
    float target_position[3];
    EntityScenePose target_pose;
    EntityScenePose linked_pose;
    EntityScenePose entity_pose;
    if (entity == NULL || entity->owner_ctx == NULL ||
        (entity_state_flags_get(entity) & 0x20000u) == 0u) return 0;
    owner = (EntitySlot*)entity->owner_36c;
    if (owner == NULL || (owner->flags_3e0 & 1u) != 0u) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;

    /* FUN_004401b0 asks the scene object for its current position before
     * resolving the terrain cell. EntitySlot coordinates are only the
     * fallback when the renderer has no pose provider. */
    if (s_entity_pair_pose_hooks.pose_read != NULL &&
        s_entity_pair_pose_hooks.pose_read(s_entity_pair_pose_hooks.context,
                                           entity, &entity_pose)) {
        target_position[0] = entity_pose.position[0];
        target_position[1] = entity_pose.position[1];
        target_position[2] = entity_pose.position[2];
        cell_x = level_cell_of(target_position[0]);
        cell_y = level_cell_of(target_position[1]);
    }
    else {
        target_position[0] = entity->x;
        target_position[1] = entity->y;
        target_position[2] = entity->z;
        cell_x = level_cell_of(target_position[0]);
        cell_y = level_cell_of(target_position[1]);
    }
    if ((entity_state_flags_get(entity) & 0x02000000u) != 0u &&
        terrain_cell_route_and_occupancy_check(owner, level, cell_x, cell_y,
                                                cell_x, cell_y, 1))
        should_transfer = 1;
    if (!should_transfer && (entity->flags_3ec & 0x40u) == 0u) {
        int unload_cell[2];
        if (force == 0 &&
            entity_vehicle_unload_cell_find_if_eligible(entity, unload_cell)) {
            if (!terrain_cell_world_position_get(unload_cell[0], unload_cell[1],
                                                 target_position)) return 0;
            cell_x = unload_cell[0];
            cell_y = unload_cell[1];
            should_transfer = 1;
        }
        else if ((entity_state_flags_get(entity) & 0x02000000u) == 0u) {
            object_attached_record_allocate(entity);
        }
    }
    else if (!should_transfer && (entity->flags_3ec & 0x40u) != 0u) {
        linked = entity->link_2fc;
        if (linked != NULL && linked->type == 4) {
            if (s_entity_pair_pose_hooks.scene_present != NULL &&
                !s_entity_pair_pose_hooks.scene_present(
                    s_entity_pair_pose_hooks.context, linked)) {
                linked = NULL;
            }
            if (linked != NULL &&
                s_entity_pair_pose_hooks.pose_read != NULL &&
                s_entity_pair_pose_hooks.pose_read(
                    s_entity_pair_pose_hooks.context, linked, &linked_pose)) {
                target_position[0] = linked_pose.position[0];
                target_position[1] = linked_pose.position[1];
                target_position[2] = linked_pose.position[2];
                cell_x = level_cell_of(target_position[0]);
                cell_y = level_cell_of(target_position[1]);
                linked_pose_valid = 1;
                should_transfer = 1;
            }
            else if (linked != NULL &&
                     entity_world_center_position_read(linked,
                                                       target_position)) {
                cell_x = level_cell_of(target_position[0]);
                cell_y = level_cell_of(target_position[1]);
                should_transfer = 1;
            }
        }
    }
    if (!should_transfer) return 0;

    target_position[2] = (float)terrain_surface_height_get(level->ptr_terrain,
                                                            cell_x, cell_y);
    entity_world_position_set(owner, target_position[0], target_position[1],
                              owner->rot, 0);
    owner->z = target_position[2];

    /* The linked-object branch explicitly copies its orientation. Other
     * branches only set the owner's position, preserving its current scene
     * orientation when the renderer can provide it. */
    if (s_entity_pair_pose_hooks.pose_apply != NULL) {
        if (linked_pose_valid) {
            target_pose = linked_pose;
            target_pose_valid = 1;
        }
        else if (s_entity_pair_pose_hooks.pose_read != NULL &&
                 s_entity_pair_pose_hooks.pose_read(
                     s_entity_pair_pose_hooks.context, owner, &target_pose)) {
            target_pose_valid = 1;
        }
        if (target_pose_valid) {
            target_pose.position[0] = target_position[0];
            target_pose.position[1] = target_position[1];
            target_pose.position[2] = target_position[2];
            s_entity_pair_pose_hooks.pose_apply(
                s_entity_pair_pose_hooks.context, owner, &target_pose);
        }
    }

    entity->owner_36c = NULL;
    owner->owner_36c = NULL;
    owner->flags_3e4 &= ~8u;
    ai_set_activity(owner, 0, 0);
    (void)selection_entity_remove(entity);
    return 1;
}

// FUNCTION: LEGORR 0x004403d0
int ledger_stub_004403d0_LevelDebugCellRetargetDispatch(
    EntitySlot* entity, EntitySlot* target,
    const EntityDebugCellRetargetHooks* hooks)
{
    LevelState* level;
    uint32_t target_state;
    uint32_t entity_flags;
    int target_cell[2];
    int target_flag20;
    unsigned int candidate;
    AiWaypoint* step;
    void* head;
    int simple_route_branch;
    if (entity == NULL || target == NULL || entity->type != 2 ||
        entity->owner_ctx == NULL || target->owner_36c != NULL) return 0;
    /* FUN_00403d10 only enters this path with an empty chain or a sole,
     * unlinked kind-0x12 record. The retail link is the record's +0x60
     * word, represented by cmd_rec_next in the portable record. */
    head = entity->cmd_head;
    if (head != NULL &&
        (cmd_rec_type(head) != 0x12 || cmd_rec_next(head) != NULL)) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    entity_flags = entity->flags_3f0;
    target_state = hooks != NULL && hooks->target_state_flags != NULL
        ? hooks->target_state_flags(hooks->context, target)
        : target->flags_3e8;
    target_flag20 = hooks != NULL && hooks->target_flag20_query != NULL
        ? hooks->target_flag20_query(hooks->context, target, 0u)
        : entity_record_flag20_query(target, NULL);
    /* The decompile's outer gate routes the fully flagged pair through its
     * special scene/record path. The terrain-edge route below is only in the
     * other branch. */
    simple_route_branch = (entity_flags & 5u) == 0u ||
        (target_state & 0x02000000u) == 0u ||
        (target_state & 0x00020000u) == 0u ||
        (target->flags_3ec & 0x40u) != 0u;
    if (simple_route_branch && (entity_flags & 2u) != 0u &&
        (target_state & 0x00020000u) != 0u &&
        (target->flags_3ec & 0x40u) == 0u &&
        target_flag20 != 0) {
        entity_position_to_terrain_cell(target, level, &target_cell[0],
                                        &target_cell[1]);
        if (target_cell[0] >= 0 && target_cell[1] >= 0 &&
            ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
                entity, (unsigned int)target_cell[0],
                (unsigned int)target_cell[1], 0, &candidate, 1) &&
            entity->wp_count > 0) {
            step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
            step->weight_x = 0.5f;
            step->weight_y = 0.5f;
            switch (candidate & 3u) {
            case 0u: step->weight_x = 0.1f; break;
            case 1u: step->weight_y = 0.1f; break;
            case 2u: step->weight_x = 0.9f; break;
            case 3u: step->weight_y = 0.9f; break;
            }
            step->pending = (uint8_t)(step->pending | 8u);
            step->step_kind = 12u;
            entity->link_2fc = target;
            return 1;
        }
    }
    return hooks != NULL && hooks->fallback_retarget != NULL
        ? hooks->fallback_retarget(hooks->context, entity, target, level) : 0;
}

// FUNCTION: LEGORR 0x004407b0
void ledger_stub_004407b0_SpatialEntityCallbackQuery(
    EntitySlot* subject, float value_a, float value_b, float value_c)
{
    float position[2];
    float values[3];
    if (subject == NULL) return;
    entity_transform_position_get(subject, position);
    values[0] = value_a;
    values[1] = value_b;
    values[2] = value_c;
    (void)entity_spatial_query_dispatch_kind(subject, position, values, 0);
}
// FUNCTION: LEGORR 0x00440800
int spatial_entity_damage_radius_callback(EntitySlot* candidate,
                                                 void* payload)
{
    SpatialEntityQueryRequest* request = (SpatialEntityQueryRequest*)payload;
    float candidate_position[2];
    float dx, dy, distance;
    if (request == NULL || candidate == NULL) return 0;
    if (candidate == request->subject ||
        (candidate->flags_3e8 & 0x40000u) == 0u) return 0;
    entity_transform_position_get(candidate, candidate_position);
    dx = candidate_position[0] - request->position[0];
    dy = candidate_position[1] - request->position[1];
    distance = (float)sqrt((double)(dx * dx + dy * dy));
    if (distance < request->values[0]) {
        float damage = (1.0f - distance / request->values[0]) *
                       request->values[1];
        entity_damage_apply(candidate, damage, 1);
        if (candidate->type == 3 && distance < request->values[2])
            entity_dispatch_event31_for_pending_flag(candidate);
    }
    return 0;
}

// SpatialEntityCallbackQueryAtOptionalCoordinate
// FUNCTION: LEGORR 0x004408c0
void ledger_stub_004408c0_SpatialEntityCallbackQueryAtOptionalCoordinate(
    EntitySlot* subject, const float* optional_position, float value)
{
    float position[2];
    float values[3];
    if (subject == NULL) return;
    if (optional_position == NULL)
        entity_transform_position_get(subject, position);
    else {
        position[0] = optional_position[0];
        position[1] = optional_position[1];
    }
    values[0] = value;
    /* The reference stack context leaves +0x10/+0x14 unwritten, but
     * LAB_00440920 reads them as vector scales. Use zero-initialized host
     * values for this otherwise undefined retail residue. */
    values[1] = 0.0f;
    values[2] = 0.0f;
    (void)entity_spatial_query_dispatch_kind(subject, position, values, 1);
}
// FUNCTION: LEGORR 0x00440920
int spatial_entity_vector_step_callback(EntitySlot* candidate,
                                               void* payload)
{
    SpatialEntityQueryRequest* request = (SpatialEntityQueryRequest*)payload;
    float candidate_position[2];
    float dx, dy, distance;
    if (request == NULL || candidate == NULL) return 0;
    if (candidate == request->subject ||
        (entity_state_flags_get(candidate) & 1u) == 0u) return 0;
    entity_transform_position_get(candidate, candidate_position);
    dx = candidate_position[0] - request->position[0];
    dy = candidate_position[1] - request->position[1];
    distance = (float)sqrt((double)(dx * dx + dy * dy));
    if (distance < request->values[0]) {
        if (candidate->type == 3)
            entity_dispatch_event31_for_pending_flag(candidate);
        if (distance != 0.0f) {
            float vector[2];
            vector[0] = (dx / distance) * request->values[1];
            vector[1] = (dy / distance) * request->values[2];
            unported_ledger_stubs::ledger_stub_00444460_FlaggedVectorStepCommandIssue(
                candidate, vector);
        }
    }
    return 0;
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x004409e0
void ai_set_activity(EntitySlot* e, int id, int param)
{
    if (e == NULL || id < 0 || id >= AI_ACTIVITY_COUNT) {
        return; /* corpus indexes the table blindly; clamp the mirror */
    }
    e->activity = id;
    ++e->activity_generation;
    e->activity_advance_valid = 0u;
    /* FUN_004409e0 leaves the +0x2ec bookkeeping mirror alone. The paired
     * SceneActivityUpdateDispatch (FUN_00447c40) observes this change, runs
     * its transition work, then updates activity_last. */
    e->activity_param = param;
    e->activity_timer = 0.0f;
}

namespace unported_ledger_stubs {
// AttachedEntityDirectionVectorUpdate
// FUNCTION: LEGORR 0x00440a10
void ledger_stub_00440a10_AttachedEntityDirectionVectorUpdate(
    EntitySlot* entity)
{
    uint32_t i;
    if (entity == NULL || entity->attached_children == NULL) return;
    for (i = 0u; i < entity->attached_child_count; ++i) {
        EntitySlot* child = entity->attached_children[i];
        void* child_scene;
        void* record = NULL;
        void* parent_scene = NULL;
        int resolver_index = 0;
        int resolver_kind = 0;
        float old_position[3];
        float new_position[3];
        float dx, dy, dz, length, inverse_length;
        if (child == NULL || (child->flags_3e8 & 0x2000u) != 0u) continue;
        child_scene = child->attached_scene_object;
        if (child_scene == NULL ||
            !ddiscene_object_position_get(child_scene, NULL, old_position))
            continue;
        switch (entity->type) {
        case 1:
            record = entity->attached_direction_record_type1;
            resolver_kind = 2;
            resolver_index = (int)entity_indexed_record_word_at_f0_get(
                1, entity->etype, entity->indexed_record_slot_330) +
                child->attached_scene_slot_index;
            break;
        case 2:
            if (entity->owner_36c != NULL &&
                ((EntitySlot*)entity->owner_36c)->type == 1) {
                EntitySlot* linked = (EntitySlot*)entity->owner_36c;
                record = linked->attached_direction_record_type1;
                resolver_kind = 2;
                resolver_index = (int)entity_indexed_record_word_at_f0_get(
                    1, linked->etype, linked->indexed_record_slot_330) +
                    child->attached_scene_slot_index;
            }
            else {
                record = entity->attached_direction_record_type2;
                resolver_kind = 0;
            }
            break;
        case 3:
            record = entity->attached_direction_record_type3;
            resolver_kind = 0;
            break;
        case 4:
            record = entity->attached_direction_record_type4;
            resolver_kind = 1;
            resolver_index = child->attached_scene_slot_index;
            break;
        default:
            continue;
        }
        if (record == NULL) continue;
        switch (resolver_kind) {
        case 1:
            parent_scene = scene_object_indexed_handle_get_or_create(
                record, resolver_index);
            break;
        case 2:
            parent_scene = attached_entity_slot_object_get(
                (SceneResourceAnimationRecord*)record, resolver_index);
            break;
        default:
            parent_scene = renderer_cached_handle_48_get_or_create(record);
            break;
        }
        if (parent_scene == NULL) continue;
        (void)ddiscene_object_position_set(child_scene, parent_scene,
                                           0.0f, 0.0f, 0.0f);
        (void)ddiscene_object_pose_method_dispatch(
            child_scene, parent_scene, 0.0f, 0.0f, 1.0f,
            0.0f, 1.0f, 0.0f);
        if (!ddiscene_object_position_get(child_scene, NULL, new_position))
            continue;
        dx = new_position[0] - old_position[0];
        dy = new_position[1] - old_position[1];
        dz = new_position[2] - old_position[2];
        length = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
        inverse_length = 1.0f / length;
        child->attached_direction_length = length;
        child->attached_direction[0] = inverse_length * dx;
        child->attached_direction[1] = inverse_length * dy;
        child->attached_direction[2] = inverse_length * dz;
    }
}
} // namespace unported_ledger_stubs

namespace lego {
/* ObjectListFieldAssignAll (FUN_00440bf0) walks the animated frame container
 * at the type-5 entity's +0x1c field and assigns the TextureSurfaceCreate
 * handle selected by TerrainCellPaletteResourceAtGridPosition. */
// FUNCTION: LEGORR 0x00440BF0
void object_list_field_assign_all(EntitySlot* entity, void* texture_resource)
{
    void* frame_container;
    int count;
    unsigned int frame;
    if (entity == NULL || entity->type != 5) return;
    frame_container = entity->type_operation_fallback_1c;
    count = lego::animated_texture_frame_count(frame_container);
    if (count <= 0 ||
        s_type_five_frame_resource_ops.frame_surface_set == NULL) return;
    for (frame = 0u; frame < (unsigned int)count; ++frame) {
        s_type_five_frame_resource_ops.frame_surface_set(
            s_type_five_frame_resource_ops.context, frame_container, frame,
            texture_resource);
    }
}
} // namespace lego

namespace unported_ledger_stubs {
// GridRouteSearchAdapter
// FUNCTION: LEGORR 0x00440c30
void ledger_stub_00440c30_GridRouteSearchAdapter(
    void* entity, uint32_t start_x, uint32_t start_y,
    uint32_t goal_x, uint32_t goal_y, int** out_x, int** out_y,
    uint32_t* out_count)
{
    /* FUN_00440c30 is a void eight-argument forwarder. The reference passes
     * its final count pointer unchanged and supplies null callback/context. */
    (void)terrain_neighbor_path_check(entity, start_x, start_y, goal_x,
                                      goal_y, out_x, out_y, out_count,
                                      NULL, NULL);
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x00440C70
int terrain_grid_path_search(void* actor, uint32_t start_x,
                             uint32_t start_y, uint32_t goal_x,
                             uint32_t goal_y, int** out_x, int** out_y,
                             uint32_t* out_count, TerrainPathStopHook stop,
                             void* user)
{
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    enum {
        TERRAIN_PATH_FRONTIER_CAPACITY = 100,
        TERRAIN_PATH_FRONTIER_STRIDE = 5
    };
    /* Retail's local_fc0 allocates 500 dwords per side. Each queue record
     * advances five dwords while its first two carry x/y. */
    uint32_t frontier[2][TERRAIN_PATH_FRONTIER_CAPACITY]
                     [TERRAIN_PATH_FRONTIER_STRIDE];
    uint32_t frontier_count[2] = { 0u, 0u };
    uint32_t width, height, cell_count, start, goal = 0u;
    uint32_t side = 0u, depth_value = 1u;
    uint32_t found = 0u;
    int* route_x;
    int* route_y;
    uint32_t length;
    uint32_t current_x, current_y, current_depth;
    int direction_cursor = 0;
    if (out_x == NULL || out_y == NULL || out_count == NULL
        || !terrain_path_grid_size_get(actor, &width, &height)
        || width == 0 || height == 0 || width > 65535u || height > 65535u
        || width > 16777216u / height
        || start_x >= width || start_y >= height) return 0;
    cell_count = width * height;
    if (stop == NULL) {
        if (goal_x >= width || goal_y >= height
            || !terrain_path_cell_accept(actor, goal_x, goal_y)) return 0;
    }
    if (s_terrain_path_depth_count != cell_count) {
        free(s_terrain_path_depth);
        s_terrain_path_depth = NULL;
        s_terrain_path_depth_count = cell_count;
        s_terrain_path_depth = (uint32_t*)malloc(
            (size_t)cell_count * sizeof(uint32_t));
    }
    if (s_terrain_path_depth == NULL) {
        s_terrain_path_depth_count = 0u;
        return 0;
    }
    memset(s_terrain_path_depth, 0,
           (size_t)cell_count * sizeof(uint32_t));
    start = start_y * width + start_x;
    s_terrain_path_depth[start] = 1u;
    frontier[0][0][0] = start_x;
    frontier[0][0][1] = start_y;
    frontier_count[0] = 1u;
    for (;;) {
        uint32_t read_index;
        uint32_t next_side = side ^ 1u;
        uint32_t next_count = 0u;
        ++depth_value;
        for (read_index = 0u; read_index < frontier_count[side];
             ++read_index) {
            uint32_t x = frontier[side][read_index][0];
            uint32_t y = frontier[side][read_index][1];
            int direction;
            for (direction = 0; direction < 4; ++direction) {
                int nx = (int)x + dx[direction];
                int ny = (int)y + dy[direction];
                uint32_t next;
                uint32_t candidate[2];
                if (nx < 0 || ny < 0 || (uint32_t)nx >= width ||
                    (uint32_t)ny >= height) continue;
                next = (uint32_t)ny * width + (uint32_t)nx;
                if (s_terrain_path_depth[next] != 0u ||
                    !terrain_path_route_accept(actor, (uint32_t)nx,
                                               (uint32_t)ny, direction))
                    continue;
                /* Retail's five-dword record window holds 100 coordinates
                 * per side. Do not reproduce its unchecked stack overwrite
                 * if a malformed grid exceeds that frontier size. */
                if (next_count >= TERRAIN_PATH_FRONTIER_CAPACITY) continue;
                frontier[next_side][next_count][0] = (uint32_t)nx;
                frontier[next_side][next_count][1] = (uint32_t)ny;
                ++next_count;
                s_terrain_path_depth[next] = depth_value;
                candidate[0] = (uint32_t)nx;
                candidate[1] = (uint32_t)ny;
                if (stop != NULL) {
                    if (stop(actor, candidate, user)) {
                        goal = next;
                        found = 1u;
                    }
                }
                else if ((uint32_t)nx == goal_x &&
                         (uint32_t)ny == goal_y) {
                    goal = next;
                    found = 1u;
                }
            }
        }
        frontier_count[side] = 0u;
        if (found != 0u) break;
        side = next_side;
        frontier_count[side] = next_count;
        if (frontier_count[side] == 0u) return 0;
    }
    length = depth_value;
    route_x = (int*)malloc((size_t)length * sizeof(int));
    route_y = (int*)malloc((size_t)length * sizeof(int));
    if (route_x == NULL || route_y == NULL) {
        free(route_x);
        free(route_y);
        return 0;
    }
    current_x = goal % width;
    current_y = goal / width;
    route_x[length - 1u] = (int)current_x;
    route_y[length - 1u] = (int)current_y;
    route_x[0] = (int)start_x;
    route_y[0] = (int)start_y;
    current_depth = length - 1u;
    while (current_depth > 1u) {
        int tries;
        for (tries = 0; tries < 4; ++tries) {
            int previous_x = (int)current_x + dx[direction_cursor];
            int previous_y = (int)current_y + dy[direction_cursor];
            if (previous_x >= 0 && previous_y >= 0 &&
                (uint32_t)previous_x < width &&
                (uint32_t)previous_y < height &&
                s_terrain_path_depth[(uint32_t)previous_y * width +
                                     (uint32_t)previous_x] == current_depth) {
                current_x = (uint32_t)previous_x;
                current_y = (uint32_t)previous_y;
                --current_depth;
                route_x[current_depth] = previous_x;
                route_y[current_depth] = previous_y;
                break;
            }
            direction_cursor = (direction_cursor + 1) & 3;
        }
    }
    *out_x = route_x;
    *out_y = route_y;
    *out_count = length;
    return 1;
}

/* TerrainNeighborPathCheck (FUN_004410f0) uses two alternating fixed-size
 * FIFO frontiers and a reusable float cost map. Each accepted relaxation is
 * appended to the opposite frontier, even if that coordinate is already
 * present there; lower costs cause another visit on the next pass. */
// FUNCTION: LEGORR 0x004410F0
int terrain_neighbor_path_check_current(void* actor, uint32_t start_x,
    uint32_t start_y, uint32_t goal_x, uint32_t goal_y, int** out_x,
    int** out_y, float* out_count, TerrainPathStopHook stop, void* user)
{
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    enum { FRONTIER_CAPACITY = 100, FRONTIER_STRIDE = 5 };
    uint32_t frontier[2][FRONTIER_CAPACITY][FRONTIER_STRIDE];
    uint32_t frontier_count[2] = { 1u, 0u };
    LevelState* route_level = terrain_path_level_resolve(actor, NULL);
    uint32_t width, height, cell_count;
    uint32_t side = 0u;
    uint32_t goal_index = 0u;
    uint32_t result_goal_x = goal_x;
    uint32_t result_goal_y = goal_y;
    int status = 0;
    int target_found = 0;
    /* Count route ENTRIES, including the start, not just frontier edges.
     * 0x4410f0 writes start at [0] and goal at [count-1]; its 0x440c70
     * fallback likewise seeds depth 1 and returns 2 for adjacent cells.
     * The decompile's bit-pattern-1 float seed is ambiguous, but zero here
     * made an adjacent goal overwrite itself with the start. */
    float iteration_count = 1.0f;
    float result_count = 0.0f;
    int* route_x;
    int* route_y;
    uint32_t route_length;
    uint32_t current_x, current_y;
    int route_valid = 1;
    if (out_x == NULL || out_y == NULL || out_count == NULL ||
        !terrain_path_grid_size_get(actor, &width, &height) || width == 0u ||
        height == 0u || width > 65535u || height > 65535u ||
        width > 16777216u / height || start_x >= width || start_y >= height)
        return 0;
    if (stop == NULL && (goal_x >= width || goal_y >= height ||
        !terrain_path_cell_accept(actor, goal_x, goal_y))) return 0;
    cell_count = width * height;
    if (s_terrain_neighbor_path_cost_count != cell_count) {
        free(s_terrain_neighbor_path_cost);
        s_terrain_neighbor_path_cost = NULL;
        s_terrain_neighbor_path_cost_count = cell_count;
    }
    if (s_terrain_neighbor_path_cost == NULL) {
        s_terrain_neighbor_path_cost = (float*)malloc(
            (size_t)cell_count * sizeof(float));
    }
    if (s_terrain_neighbor_path_cost == NULL) {
        s_terrain_neighbor_path_cost_count = 0u;
        return 0;
    }
    memset(s_terrain_neighbor_path_cost, 0,
           (size_t)cell_count * sizeof(float));
    s_terrain_neighbor_path_cost[start_y * width + start_x] = 1.0f;
    frontier[0][0][0] = start_x;
    frontier[0][0][1] = start_y;

    /* Retail has no queue-membership bit: a coordinate is enqueued again
     * whenever a later relaxation lowers its current cost. */
    while (status == 0) {
        uint32_t read_index;
        uint32_t next_side = side ^ 1u;
        uint32_t next_count = 0u;
        iteration_count = (float)((int)iteration_count + 1);
        for (read_index = 0u; read_index < frontier_count[side];
             ++read_index) {
            uint32_t x = frontier[side][read_index][0];
            uint32_t y = frontier[side][read_index][1];
            float base_cost = s_terrain_neighbor_path_cost[y * width + x];
            int direction;
            for (direction = 0; direction < 4; ++direction) {
                int nx = (int)x + dx[direction];
                int ny = (int)y + dy[direction];
                uint32_t next;
                float edge_cost = 1.0f;
                float candidate_cost;
                if (nx < 0 || ny < 0 || (uint32_t)nx >= width ||
                    (uint32_t)ny >= height) continue;
                if (terrain_path_half_cost_accept(
                        actor, (uint32_t)nx, (uint32_t)ny, direction))
                    edge_cost = 0.5f;
                if (!terrain_path_route_accept(actor, (uint32_t)nx,
                                               (uint32_t)ny, direction))
                    continue;
                next = (uint32_t)ny * width + (uint32_t)nx;
                candidate_cost = edge_cost + base_cost;
                if (s_terrain_neighbor_path_cost[next] != 0.0f &&
                    candidate_cost >= s_terrain_neighbor_path_cost[next])
                    continue;
                s_terrain_neighbor_path_cost[next] = candidate_cost;
                /* Retail writes unbounded records into a 100-entry side.
                 * Preserve state/stop effects while dropping only an
                 * unrepresentable queue write in the portable build. */
                if (next_count < FRONTIER_CAPACITY) {
                    frontier[next_side][next_count][0] = (uint32_t)nx;
                    frontier[next_side][next_count][1] = (uint32_t)ny;
                    ++next_count;
                }
                if (stop != NULL) {
                    uint32_t candidate[2];
                    candidate[0] = (uint32_t)nx;
                    candidate[1] = (uint32_t)ny;
                    if (stop(actor, candidate, user)) {
                        status = 1;
                        goal_index = next;
                        result_goal_x = (uint32_t)nx;
                        result_goal_y = (uint32_t)ny;
                        result_count = iteration_count;
                    }
                }
                else if ((uint32_t)nx == goal_x &&
                         (uint32_t)ny == goal_y) {
                    target_found = 1;
                    goal_index = next;
                    result_count = iteration_count;
                }
            }
        }
        frontier_count[side] = 0u;
        side = next_side;
        frontier_count[side] = next_count;
        if (frontier_count[side] == 0u) status = -1;
    }
    if (stop == NULL && target_found) status = 1;
    if (status != 1) return 0;

    route_length = (uint32_t)result_count;
    if (route_length == 0u) return 0;
    route_x = (int*)malloc((size_t)route_length * sizeof(int));
    route_y = (int*)malloc((size_t)route_length * sizeof(int));
    if (route_x == NULL || route_y == NULL) {
        free(route_x);
        free(route_y);
        return 0;
    }
    current_x = goal_index % width;
    current_y = goal_index / width;
    route_x[route_length - 1u] = (int)current_x;
    route_y[route_length - 1u] = (int)current_y;
    route_x[0] = (int)start_x;
    route_y[0] = (int)start_y;

    /* Select the smallest positive cost difference below ten. An exact
     * difference of ten is accepted only for an occupied predecessor. */
    if (route_length > 2u) {
        uint32_t index = route_length - 2u;
        for (;;) {
            int found_previous = 0;
            int direction;
            uint32_t current_index = current_y * width + current_x;
            float current_cost = s_terrain_neighbor_path_cost[current_index];
            float best_difference = 10.0f;
            int best_x = 0;
            int best_y = 0;
            for (direction = 0; direction < 4; ++direction) {
                int previous_x = (int)current_x + dx[direction];
                int previous_y = (int)current_y + dy[direction];
                int previous_xy[2];
                uint32_t previous_index;
                float difference;
                if (previous_x < 0 || previous_y < 0 ||
                    (uint32_t)previous_x >= width ||
                    (uint32_t)previous_y >= height) continue;
                previous_index = (uint32_t)previous_y * width +
                                 (uint32_t)previous_x;
                previous_xy[0] = previous_x;
                previous_xy[1] = previous_y;
                /* Unvisited zero-cost cells are not predecessors. */
                if (s_terrain_neighbor_path_cost[previous_index] == 0.0f)
                    continue;
                difference = current_cost -
                    s_terrain_neighbor_path_cost[previous_index];
                if ((difference > 0.0f && difference < best_difference) ||
                    (difference == best_difference &&
                     (route_level != NULL
                        ? level_cell_occupied_at(route_level, previous_x,
                                                 previous_y)
                        : level_cell_occupied_at_current(previous_xy))
                        != 0)) {
                    best_difference = difference;
                    best_x = previous_x;
                    best_y = previous_y;
                    found_previous = 1;
                }
            }
            if (!found_previous) {
                route_valid = 0;
                break;
            }
            current_x = (uint32_t)best_x;
            current_y = (uint32_t)best_y;
            route_x[index] = best_x;
            route_y[index] = best_y;
            if (index == 1u) break;
            --index;
        }
    }

    /* Weighted relaxation can change the number of edges in the best
     * cost trace. Never return holes, a missing endpoint, or a broken edge.
     * Retain the reference's existing 0x440c70 fallback for such traces. */
    if (route_valid) {
        uint32_t index;
        for (index = 1u; index < route_length; ++index) {
            int direction;
            int edge_x = route_x[index] - route_x[index - 1u];
            int edge_y = route_y[index] - route_y[index - 1u];
            if (edge_x == 0 && edge_y == -1) direction = 0;
            else if (edge_x == 1 && edge_y == 0) direction = 1;
            else if (edge_x == 0 && edge_y == 1) direction = 2;
            else if (edge_x == -1 && edge_y == 0) direction = 3;
            else { route_valid = 0; break; }
            if (!terrain_path_route_accept(actor, (uint32_t)route_x[index],
                    (uint32_t)route_y[index], direction)) {
                route_valid = 0;
                break;
            }
        }
    }
    if (!route_valid) {
        uint32_t fallback_count = 0u;
        free(route_x);
        free(route_y);
        if (!terrain_grid_path_search(actor, start_x, start_y,
                result_goal_x, result_goal_y, out_x, out_y,
                &fallback_count, stop, user))
            return 0;
        *out_count = (float)fallback_count;
        return 1;
    }
    *out_x = route_x;
    *out_y = route_y;
    *out_count = result_count;
    return 1;
}

// FUNCTION: LEGORR 0x00441700
int entity_route_setup_and_marker_associate(EntitySlot* entity,
                                            unsigned int count,
                                            const float* x_values,
                                            const float* y_values,
                                            const float* final_marker_position)
{
    AiWaypoint* route;
    GameContext* context;
    LevelState* level;
    const void* terrain;
    unsigned int i;
    int previous_direction = -1;
    int route_marker_failed = 0;
    if (entity == NULL || count > 0x7fffffffu ||
        (count != 0 && (x_values == NULL || y_values == NULL))) return 0;

    /* FUN_0043baf0 is the exact busy/ineligible gate at the top of the
     * retail installer. The route is scratch-owned until every segment has
     * passed, so a failed marker leaves the existing route intact. */
    if (entity_flags_mask107c33de_or_1c00203_any_test(entity)) return 0;

    if ((entity->flags_3ec & 0x40u) != 0u) {
        entity->link_2fc = NULL;
        entity->flags_3ec &= ~0x40u;
    }
    if (count == 0u) {
        route = NULL;
    }
    else {
        if ((size_t)count > ((size_t)-1) / sizeof(AiWaypoint)) return 0;
        route = (AiWaypoint*)malloc((size_t)count * sizeof(AiWaypoint));
        if (route == NULL) return 0;
    }

    context = entity->owner_ctx != NULL ? entity->owner_ctx
                                        : entity_pool_callback_context_get();
    level = context != NULL ? context->level : NULL;
    terrain = level != NULL ? level->ptr_terrain : NULL;
    for (i = 0; i < count; ++i) {
        route[i].x = x_values[i];
        route[i].y = y_values[i];
        route[i].weight_x = 0.5f;
        route[i].weight_y = 0.5f;
        route[i].pending = 0;
        route[i].step_kind = 0;
        route[i].reserved[0] = 0;
        route[i].reserved[1] = 0;
        if (i + 1u == count && final_marker_position != NULL &&
            terrain != NULL) {
            float cell_center[2];
            float blocksize = terrain_blocksize_get(terrain);
            if (blocksize != 0.0f &&
                terrain_cell_world_center_xy_get(terrain,
                    (int)x_values[i], (int)y_values[i], cell_center)) {
                route[i].weight_y =
                    (final_marker_position[0] - cell_center[0]) / blocksize
                    + 0.5f;
                route[i].weight_x =
                    (cell_center[1] - final_marker_position[1]) / blocksize
                    + 0.5f;
            }
        }
        if ((final_marker_position == NULL || i + 1u != count) &&
            (entity->flags_3e8 & 0x1000u) == 0u &&
            (entity_state_flags_get(entity) & 0x20000000u) != 0u) {
            route[i].weight_y = random_float_range(0.3f, 0.7f);
            route[i].weight_x = random_float_range(0.3f, 0.7f);
        }
        if (i + 1u < count) {
            const int direction = cell_neighbor_direction_code(
                (int)x_values[i + 1u], (int)y_values[i + 1u],
                (int)x_values[i], (int)y_values[i]);
            if (direction < 0) {
                route_marker_failed = 1;
                break;
            }
            if (i != 0u && previous_direction != direction && terrain != NULL &&
                terrain_cell_route_and_occupancy_check(
                    entity, level, (int)x_values[i + 1u],
                    (int)y_values[i + 1u], (int)x_values[i],
                    (int)y_values[i], 0) == 2) {
                route[i].pending |= 0x10u;
            }
            previous_direction = direction;
            if (s_entity_route_segment_accept_hook != NULL &&
                s_entity_route_segment_accept_hook(entity, x_values[i],
                    y_values[i], x_values[i + 1u], y_values[i + 1u]) == -1) {
                route_marker_failed = 1;
                break;
            }
        }
    }
    if (route_marker_failed) {
        free(route);
        return 0;
    }

    /* Retail resets/releases the old sequence only after the replacement
     * records have been built. Transfer a long route to the native sidecar;
     * shorter routes stay in EntitySlot's retail-sized inline span. */
    entity_action_sequence_reset(entity, 0);
    if (count > AI_WAYPOINTS_MAX) {
        entity->wp_dynamic = route;
        entity->wp_capacity = count;
        route = NULL;
    }
    else {
        for (i = 0; i < count; ++i) entity->wp[i] = route[i];
    }
    free(route);
    entity->wp_count = (int)count;
    entity->wp_index = 0;
    entity->flags_3e8 &= ~0x400u;
    entity->flags_3e0 |= 1u;
    entity->pos_pair_2f4 = 0.0f;
    entity->pos_pair_2f8 = 0.0f;
    return 1;
}

// FUNCTION: LEGORR 0x00441940
void entity_action_sequence_reset(EntitySlot* entity, int preserve_mode)
{
    int had_waypoints;
    if (entity == NULL) return;
    /* Invalidate the demonstrated GetTool owner BEFORE releasing its path.
     * This also revokes an unconsumed terminal grant on external reset. */
    cmd_gettool_route_reset(entity);
    had_waypoints = entity->wp_count > 0 || entity->wp_dynamic != NULL;
    if (had_waypoints && preserve_mode == 0) {
        unsigned int index = (unsigned int)entity->wp_index;
        const unsigned int count = (unsigned int)entity->wp_count;
        AiWaypoint* waypoints = ENTITY_WAYPOINTS(entity);
        while (index < count) {
            if (waypoints[index].step_kind == 3u) {
                EntitySlot* linked = entity->link_2fc;
                if (linked != NULL) {
                    if (linked->type == 5) {
                        entity_type5_finalize_if_allowed(linked);
                    }
                    else {
                        linked->link_300 = NULL;
                    }
                    entity->link_2fc = NULL;
                }
                break;
            }
            if (waypoints[index].step_kind == 7u) {
                if (entity->link_2fc != NULL) entity->link_2fc = NULL;
                break;
            }
            ++index;
        }

        if (entity->type == 3) {
            EntitySlot* linked = entity->link_2fc;
            if ((entity->flags_3e8 & 0x400u) != 0u && linked != NULL &&
                linked->type == 5) {
                unported_ledger_stubs::entity_attached_object_list_detach(
                    entity, 0);
            }
            cmd_chain_drop(entity, 4);
            cmd_chain_drop(entity, 10);
        }
        if ((entity->flags_3e8 & 8u) != 0u && entity->owner_ctx != NULL &&
            entity->owner_ctx->level != NULL) {
            const int cell_x = crt_float_to_long(entity->x);
            const int cell_y = crt_float_to_long(entity->y);
            level_tile_mark(entity->owner_ctx->level, cell_x, cell_y, 0);
        }
        entity_task_queue_flush(entity);
    }
    /* The mode-1 type-2 branch restores a pose through the legacy component
     * record at +0x10 using saved values at +0x298/+0x29c. EntitySlot has no
     * owner for that record or those saved fields, so that renderer restore
     * cannot be represented by this module. */
    /* The retail path releases its route sequence allocation. The portable
     * route extension owns a separate allocation after the inline waypoints. */
    free(entity->wp_dynamic);
    entity->wp_dynamic = NULL;
    entity->wp_capacity = 0u;
    entity->wp_count = 0;
    entity->wp_index = 0;
    entity_path_motion_runtime_clear(entity);
    entity->flags_3e0 &= 0xff7ffffeu;
    entity->flags_3e8 &= 0xffffbbffu;
    if (entity->link_2fc != NULL) {
        EntitySlot* partner = entity->link_2fc;
        partner->flags_3ec &= ~0x10u;
    }
    if ((entity->flags_3e0 & 2u) != 0) {
        entity->attached_direction[0] = -entity->attached_direction[0];
        entity->attached_direction[1] = -entity->attached_direction[1];
        entity->attached_direction[2] = -entity->attached_direction[2];
        entity->flags_3e0 &= ~2u;
    }
    ai_set_activity(entity, 0, 0);
    if (had_waypoints)
        entity_task_state_update_after_cleanup(entity, preserve_mode);
}

// FUNCTION: LEGORR 0x00441b30
void entity_state_reset(EntitySlot* entity, int clear_mode, int reset_mode)
{
    uint32_t old_flags_3e0;
    uint32_t old_flags_3e4;
    EntitySlot* partner;
    int preserve_link_flags;
    if (entity == NULL) return;
    entity_action_sequence_reset(entity, 0);
    if (reset_mode != 0)
        unported_ledger_stubs::entity_attached_object_list_detach(entity, 0);

    partner = (EntitySlot*)entity->pair_link_3c0;
    if ((entity->flags_3e4 & 2u) != 0u) {
        entity->flags_3e4 &= ~2u;
        ai_set_activity(entity, 0x27, 0);
        entity->flags_3e4 |= 0x100u;
        if (partner != NULL && (partner->flags_3e4 & 1u) != 0u) {
            partner->flags_3e4 &= ~1u;
            ai_set_activity(entity, 0, 0);
        }
    }
    if ((entity->flags_3e4 & 1u) != 0u) {
        entity->flags_3e4 &= ~1u;
        ai_set_activity(entity, 0, 0);
        if (partner != NULL && (partner->flags_3e4 & 2u) != 0u) {
            partner->flags_3e4 &= ~2u;
            ai_set_activity(partner, 0x27, 0);
            partner->flags_3e4 |= 0x100u;
        }
    }

    old_flags_3e4 = entity->flags_3e4;
    entity->flags_3e4 &= 0xfe3fbfdfu;
    old_flags_3e0 = entity->flags_3e0;
    entity->flags_3e0 = old_flags_3e0 & 0x6ffff7ffu;
    if ((old_flags_3e0 & 0x2000u) != 0u) {
        entity->flags_3e0 = old_flags_3e0 & 0x6fffd7ffu;
        cmd_chain_drop(entity, 5);
        entity_simulation_queue_flush(entity);
        if (entity->link_2fc != NULL) {
            EntitySlot* linked = entity->link_2fc;
            linked->flags_3ec &= 0xfeffffffu;
        }
    }
    entity->flags_3e0 &= 0xdffffdffu;
    entity->flags_3e8 &= 0xffffbfffu;
    if ((old_flags_3e4 & 0x200000u) != 0u)
        entity->flags_3e4 &= 0xffdfffffu;
    old_flags_3e4 = entity->flags_3e4;
    entity->flags_3e4 = old_flags_3e4 & 0xfffbffffu;
    if ((old_flags_3e0 & 0x1000000u) != 0u) {
        entity->flags_3e4 = old_flags_3e4 & 0xfff3ffffu;
        cmd_chain_drop(entity, 10);
        entity->flags_3e0 &= 0xfeffffffu;
    }

    if ((entity->flags_3e0 & 0x40000000u) != 0u)
        entity->flags_3e0 &= 0xbfffffffu;
    if ((entity->flags_3e4 & 0x40u) != 0u)
        entity->flags_3e4 &= ~0x40u;
    if ((entity->flags_3e4 & 0x400u) != 0u)
        entity->flags_3e4 &= ~0x400u;

    if ((entity->flags_3e4 & 0x8000u) != 0u) {
        EntitySlot* linked = entity->link_2fc;
        preserve_link_flags = 0;
        if (linked != NULL && (entity_stats_word_get(linked) & 2u) != 0u) {
            if ((linked->flags_3e4 & 0x8000u) != 0u) {
                preserve_link_flags = 1;
            }
            else {
                cmd_chain_drop(entity, 0x15);
                linked->link_300 = NULL;
                entity->link_2fc = NULL;
            }
        }
        if (!preserve_link_flags)
            entity->flags_3e4 &= ~0x8000u;
    }

    if ((entity->flags_3e0 & 0x80000u) != 0u) {
        entity->flags_3e0 &= 0xfff7ffffu;
        entity_type5_finalize_if_allowed(entity);
    }

    old_flags_3e0 = entity->flags_3e0;
    entity->flags_3e0 = old_flags_3e0 & 0xffffefffu;
    if ((old_flags_3e0 & 0x40u) != 0u) {
        int cell_x = level_cell_of(entity->x);
        int cell_y = level_cell_of(entity->y);
        entity->flags_3e0 = (old_flags_3e0 & 0xffffefbfu) | 0x80000000u;
        if (entity->owner_ctx != NULL && entity->owner_ctx->level != NULL)
            level_tile_mark(entity->owner_ctx->level, cell_x, cell_y, 0);
    }

    old_flags_3e0 = entity->flags_3e0;
    entity->flags_3e0 = old_flags_3e0 & 0xfbffffffu;
    if ((entity->flags_3e4 & 4u) != 0u) {
        entity->link_2fc = NULL;
        entity->flags_3e4 &= ~4u;
    }
    entity->flags_3e0 = old_flags_3e0 & 0x7bfa7fffu;
    entity->flags_3e4 &= 0xfffcfcffu;

    if (clear_mode != 0) {
        task_record_kind14_remove_if_head(entity);
        ai_set_activity(entity, 0, 1);
    }
}

// FUNCTION: LEGORR 0x00441ea0
void entity_type5_finalize_if_allowed(EntitySlot* entity)
{
    if (entity == NULL || (s_entity_finalize_mode_flags & 2u) != 0u ||
        entity->type != 5) return;
    if (s_entity_type5_timeout_hook != NULL)
        s_entity_type5_timeout_hook(entity);
    else
        scene_marker_create_at_object_position(
            entity->attached_scene_object);
    if (s_entity_type5_finalize_hook != NULL)
        s_entity_type5_finalize_hook(entity);
    else
        (void)entity_teardown(entity);
}
// FUNCTION: LEGORR 0x00441ed0
int entity_face_target_action_start(EntitySlot* entity, EntitySlot* target,
                                    int mode)
{
    float vector[3];
    float length;
    if (entity == NULL || target == NULL || entity->wake_timer > 0.0f ||
        target->type != ENTITY_TYPE_OBJ) return 0;
    entity_action_sequence_reset(entity, 0);
    vector[0] = target->x - entity->x;
    vector[1] = target->y - entity->y;
    vector[2] = target->z - entity->z;
    length = (float)sqrt(vector[0] * vector[0] + vector[1] * vector[1] +
                         vector[2] * vector[2]);
    vector[0] /= length;
    vector[1] /= length;
    vector[2] /= length;
    entity_velocity_vector_dispatch(entity, 0.0f, vector);
    entity->facing_target_x = target->x;
    entity->facing_target_y = target->y;
    entity->facing_target_z = target->z;
    if (mode == 1) entity->flags_3e4 |= 0x00400000u;
    else if (mode == 2) entity->flags_3e4 |= 0x00800000u;
    else if (mode == 3) entity->flags_3e4 |= 0x01000000u;
    return 1;
}

// FUNCTION: LEGORR 0x004420d0
void kind_effect_slot_init_dispatch(void* entity, int kind)
{
    const char* resource_name;
    uint32_t resource_index;
    float position[3];
    float direction[3];
    float duration;
    EntitySlot* owner = (EntitySlot*)entity;
    GameContext* game;
    LevelState* level;
    if (owner == NULL) return;
    switch (kind) {
    case 1: resource_name = "Lazer"; break;
    case 2: resource_name = "Pusher"; break;
    case 3: resource_name = "Freezer"; break;
    case 4: resource_name = "Lazer"; break;
    default: return;
    }
    game = owner->owner_ctx;
    if (game == NULL) game = entity_pool_callback_context_get();
    level = game != NULL ? game->level : NULL;
    if (s_kind_effect_dispatch_hooks.entity_position_get != NULL) {
        if (!s_kind_effect_dispatch_hooks.entity_position_get(
                s_kind_effect_dispatch_hooks.context, entity, position)) return;
    }
    else if (owner->attached_scene_object != NULL &&
             ddiscene_object_position_get(owner->attached_scene_object, NULL,
                                          position)) {
        /* Use the live scene pose when it has been installed. */
    }
    else {
        position[0] = owner->x;
        position[1] = owner->y;
        position[2] = owner->z;
    }
    if (s_kind_effect_dispatch_hooks.facing_vector_get != NULL) {
        s_kind_effect_dispatch_hooks.facing_vector_get(
            s_kind_effect_dispatch_hooks.context, entity, direction);
    }
    else if (owner->attached_scene_object != NULL &&
             ddiscene_object_facing_get(owner->attached_scene_object, NULL,
                                        direction)) {
        /* The renderer's facing vector is authoritative when available. */
    }
    else {
        direction[0] = (float)cos((double)owner->rot);
        direction[1] = (float)sin((double)owner->rot);
        direction[2] = 0.0f;
    }
    if (s_kind_effect_dispatch_hooks.terrain_height_get != NULL) {
        position[2] = s_kind_effect_dispatch_hooks.terrain_height_get(
            s_kind_effect_dispatch_hooks.context, position[0], position[1]);
    }
    else if (level != NULL && level->ptr_terrain != NULL) {
        position[2] = terrain_surface_height_interpolate(
            level->ptr_terrain, position[0], position[1]);
    }
    else {
        return;
    }
    position[2] -= s_kind_effect_dispatch_hooks.entity_half_height_get != NULL
        ? s_kind_effect_dispatch_hooks.entity_half_height_get(
              s_kind_effect_dispatch_hooks.context, entity)
        : entity_stats_float_at74(owner) * 0.5f;
    resource_index = lego::object_type_name_index_find(resource_name);
    duration = s_kind_effect_dispatch_hooks.resource_duration_get != NULL
        ? s_kind_effect_dispatch_hooks.resource_duration_get(
              s_kind_effect_dispatch_hooks.context, resource_name)
        : lego::weapon_type_recharge_time_get((int)resource_index);
    if (s_kind_effect_dispatch_hooks.effect_spawn != NULL) {
        s_kind_effect_dispatch_hooks.effect_spawn(
            s_kind_effect_dispatch_hooks.context, entity, kind, position,
            direction, duration);
    }
    else if (kind >= 1 && kind <= 3) {
        /* FUN_0046faf0 owns the three direct-emitter modes. Its auxiliary
         * argument is the resolved object-type index used by the spawn
         * record, and the mode is the action kind. Kind 4 uses the separate
         * flight-curve record and needs the endpoint at owner +0x3a8, which
         * EntitySlot does not currently expose. */
        (void)entity_spawn_record_slot_init(owner, position, direction,
                                            resource_index, (uint32_t)kind);
    }
    if (s_kind_effect_dispatch_hooks.cooldown_set != NULL)
        s_kind_effect_dispatch_hooks.cooldown_set(
            s_kind_effect_dispatch_hooks.context, entity, duration);
}

// FUNCTION: LEGORR 0x00442210
void entity_action_route_state_initialize(EntitySlot* entity)
{
    if (entity == NULL) return;
    entity->activity_timer = 0.0f;
    entity->last_action_event = 0x4du;
    entity->last_action_event_flags = 1u;
    if (s_entity_route_state_hook != NULL &&
        s_entity_route_state_hook(entity) != 0) {
        entity->flags_3e0 |= 0x100000u;
    }
    entity->field_340 = -1.0f;
}
// FUNCTION: LEGORR 0x00442260
void entity_transform_position_get_outputs(const EntitySlot* entity,
                                           float* out_x, float* out_y)
{
    float position[3];
    if (entity == NULL || out_x == NULL || out_y == NULL) return;
    /* The reference resolves the kind-specific transform, then reads its
     * slot-zero position.  The attached scene object is the port's live
     * transform for objects whose type component has reached the renderer;
     * the slot coordinates remain the simulation fallback. */
    if (entity->attached_scene_object != NULL &&
        ddiscene_object_position_get(entity->attached_scene_object, NULL,
                                     position)) {
        *out_x = position[0];
        *out_y = position[1];
        return;
    }
    *out_x = entity->x;
    *out_y = entity->y;

}
// FUNCTION: LEGORR 0x004422a0
void entity_surface_vector_normalize(void* surface, float out_xy[2])
{
    float vector[2];
    float inverse_length;
    vector[0] = 0.0f;
    vector[1] = 0.0f;
    if (s_entity_surface_vector_get_hook != NULL)
        s_entity_surface_vector_get_hook(surface, vector);
    inverse_length = 1.0f / (float)sqrt(vector[0] * vector[0] + vector[1] * vector[1]);
    out_xy[0] = inverse_length * vector[0];
    out_xy[1] = inverse_length * vector[1];
}

// FUNCTION: LEGORR 0x00442300
void entity_velocity_vector_dispatch(EntitySlot* entity, float angle,
                                     const float vector_or_null[3])
{
    float x, y, z;
    EntityVelocityApplyHook hook = NULL;
    if (entity == NULL) return;
    if (vector_or_null != NULL) {
        x = vector_or_null[0];
        y = vector_or_null[1];
        z = vector_or_null[2];
    }
    else {
        x = (float)sin(angle);
        y = (float)cos(angle);
        z = 0.0f;
    }
    if (entity->type >= 1 && entity->type <= 4)
        hook = s_entity_velocity_hooks[entity->type - 1];
    else if ((entity->flags_3e8 & 0x20000u) != 0)
        hook = s_entity_velocity_hooks[4];
    if (hook != NULL) hook(entity, x, y, z);
    entity->velocity_x = x;
    entity->velocity_y = y;
    entity->velocity_z = z;
    entity->movement_tick_scale = 0.01f;
}
// FUNCTION: LEGORR 0x00442480
float entity_vector_direction_angle_radians_get(void* surface)
{
    float direction[2];
    float angle;
    entity_surface_vector_normalize(surface, direction);
    angle = (float)atan2(direction[1], direction[0]);
    if (angle < 0.0f) angle += 6.2831855f;
    return angle;
}

/* EntityPositionToTerrainCell (FUN_004424f0): the corpus obtains the
 * entity's scene position, then passes its world x/y pair through the
 * terrain grid picker. EntitySlot::x/y are the portable position values. */
// FUNCTION: LEGORR 0x004424F0
void entity_position_to_terrain_cell(const struct EntitySlot* entity,
                                     const LevelState* s,
                                     int* cell_x, int* cell_y)
{
    if (entity == NULL || s == NULL || cell_x == NULL || cell_y == NULL) {
        return;
    }
    if (!terrain_world_xy_to_cell(s->ptr_terrain, entity->x, entity->y,
                                  cell_x, cell_y)) return;
}

/* GridOperationArgumentAdapter (FUN_00442540) — see view.hpp. */
// FUNCTION: LEGORR 0x00442540
void grid_operation_argument_adapter(float x, float y, EngineContext* ctx)
{
    if (ctx == NULL) {
        return; /* deviation: the corpus reads its own world blindly */
    }
    (void)terrain_height(ctx, x, y);
}

namespace unported_ledger_stubs {
// SubobjectPoseDeltaNormalizeAndDispatch
// FUNCTION: LEGORR 0x004425f0
int ledger_stub_004425f0_SubobjectPoseDeltaNormalizeAndDispatch(
    EntityPoseDeltaState* state, const EntityPoseDeltaHooks* hooks,
    float target_x, float target_y)
{
    return entity_subobject_pose_delta_normalize_and_dispatch(
        state, hooks, target_x, target_y);
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x004428a0
void entity_world_position_set(EntitySlot* entity, float x, float y,
                               float heading, int update_heading)
{
    if (entity == NULL) return;
    /* The retail API passes x/y to the kind-specific pose setter. Those
     * setters obtain vertical height through FUN_00442540 -> 0x44f940; this
     * two-coordinate setter does not convert to a cell or clip to map bounds.
     * The host EntitySlot keeps the same horizontal pair directly. */
    entity->x = x;
    entity->y = y;

    if (entity->attached_scene_object != NULL) {
        float position[3];
        float z = entity->z;
        if (ddiscene_object_position_get(entity->attached_scene_object, NULL,
                                         position)) z = position[2];
        entity->z = z;
        (void)ddiscene_object_position_set(entity->attached_scene_object,
                                           NULL, x, y, z);
    }
    if (update_heading) {
        float direction[3] = { 0.0f, 1.0f, 0.0f };
        float axis[3] = { 0.0f, 0.0f, -1.0f };
        /* FUN_00479510 rotates [0,1,0] around [0,0,-1] by the supplied
         * heading, then the retail body stores the result at +0x2ac..+0x2b4. */
        d3drm_vector_rotate(direction, direction, axis, heading);
        entity->attached_direction[0] = direction[0];
        entity->attached_direction[1] = direction[1];
        entity->attached_direction[2] = direction[2];
        entity->rot = heading;
    }
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    if (entity->type == 4) (void)building_scene_pose_set(entity);
    if (entity->type == 2 || entity->type == 3)
        (void)actor_scene_pose_set(entity);
#elif defined(LEGORRR_BACKEND_MINIWIN)
    creature_scene_pose_apply(entity);
#endif
    /* Corpus writes 0.01 to the +0xae tick scale after every update. */
    entity->movement_tick_scale = 0.01f;
}

namespace unported_ledger_stubs {
// EntityMovementAccumulatorApply
// FUNCTION: LEGORR 0x00442b10
int ledger_stub_00442b10_EntityMovementAccumulatorApply(
    EntityMovementAccumulatorState* state, void* entity, float delta,
    float* output_scale, const EntityMovementAccumulatorHooks* hooks)
{
    return entity_movement_accumulator_apply(state, entity, delta,
                                             output_scale, hooks);
}

// EntityPoseOrientationDispatch
// FUNCTION: LEGORR 0x00442f80
int ledger_stub_00442f80_EntityPoseOrientationDispatch(
    int kind, void* entity, const EntityPoseOrientationHooks* hooks)
{
    return entity_pose_orientation_dispatch(kind, entity, hooks);
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x00443510
void scene_object_pose_position_sync(EntitySlot* entity)
{
    EntityScenePose* source = NULL;
    EntityScenePose* target = NULL;
    int changed = 0;
    unsigned int i;
    if (entity == NULL || s_entity_scene_pose_hooks.poses_get == NULL ||
        !s_entity_scene_pose_hooks.poses_get(s_entity_scene_pose_hooks.context,
                                             entity, &source, &target) ||
        source == NULL || target == NULL) return;
    for (i = 0; i < 3; ++i) {
        if (source->position[i] != target->position[i]) changed = 1;
    }
    if (changed) memcpy(target->position, source->position, sizeof(source->position));
    changed = 0;
    for (i = 0; i < 7; ++i) {
        if (source->orientation[i] != target->orientation[i]) changed = 1;
    }
    if (changed)
        memcpy(target->orientation, source->orientation, sizeof(source->orientation));
}

/* TryRandomEntityActionFromCodeRange (FUN_00443670): up to five draws
 * land in the activity-code band 0x38..0x4c — random_next's 15-bit word
 * mapped through % 0x15 + 0x38 — and each draw installs its code through
 * the activity-set fold (FUN_004409e0 followed by FUN_00447c40) and uses
 * the dispatch's return as its success test. That return depends on the
 * type-2/type-3 scene runner, whose live renderer callbacks are not
 * installed in this backend. On success the entity raises
 * its +0x3e0 bit 0x8000 and answers 1; five failed dispatch attempts answer 0.
 * The gates run before any draw: bit 0x400 of +0x3e0 clear and the
 * mask-word test (FUN_0043baf0) answering 0. */
// FUNCTION: LEGORR 0x00443670
int entity_random_activity_attempt(EntitySlot* e)
{
    if (e == NULL) {
        return 0; /* deviation: the corpus strides blindly */
    }
    if ((e->flags_3e0 & 0x400u) != 0u) {
        return 0;
    }
    if (entity_flags_mask107c33de_or_1c00203_any_test(e)) {
        return 0;
    }
    for (unsigned int attempt = 0; attempt < 5u; ++attempt) {
        const short roll = (short)level_random_draw15();
        const int code = (int)roll % 0x15 + 0x38;
        ai_set_activity(e, code, 0); /* FUN_004409e0; 0x447c40 folds here */
        if (activity_register_bookkeeping(e) != 0) {
            e->flags_3e0 |= 0x8000u;
            return 1;
        }
    }
    return 0;
}

/* EntityType3Flag04000000Test (FUN_004436f0, finding 318): kind 3 with
 * +0x3e8 bit 0x04000000. The corpus reads the flag word as
 * entity[0xfa] (dword-indexed +0x3e8). */
// FUNCTION: LEGORR 0x004436f0
int entity_type3_flag04000000_test(const EntitySlot* e)
{
    return e->type == 3 && (e->flags_3e8 & 0x04000000u) != 0u;
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x00443710
int ledger_stub_00443710_AdjacentPositionCandidateFind(
    EntitySlot* entity, const int candidate[2], int output[2])
{
    GameContext* context;
    AdjacentRouteAcceptContext accept;
    int* route_x = NULL;
    int* route_y = NULL;
    uint32_t route_count = 0u;
    int found;
    if (entity == NULL || candidate == NULL || output == NULL ||
        candidate[0] < 0 || candidate[1] < 0) return 0;
    context = entity->owner_ctx != NULL ? entity->owner_ctx
                                       : entity_pool_callback_context_get();
    if (context == NULL || context->level == NULL) return 0;
    accept.entity = entity;
    accept.level = context->level;
    accept.route_cookie = 0;
    if (entity_route_cell_candidate_accept(entity, context->level,
            candidate[0], candidate[1], accept.route_cookie)) return 0;

    found = terrain_neighbor_path_check(entity,
        (uint32_t)candidate[0], (uint32_t)candidate[1], 0u, 0u,
        &route_x, &route_y, &route_count,
        adjacent_route_candidate_stop, &accept);
    /* The retail code repeats this route query after a failed attempt. */
    if (!found) {
        free(route_x);
        free(route_y);
        route_x = NULL;
        route_y = NULL;
        route_count = 0u;
        found = terrain_neighbor_path_check(entity,
            (uint32_t)candidate[0], (uint32_t)candidate[1], 0u, 0u,
            &route_x, &route_y, &route_count,
            adjacent_route_candidate_stop, &accept);
    }
    if (!found || route_count == 0u) {
        free(route_x);
        free(route_y);
        return 0;
    }
    output[0] = route_x[route_count - 1u];
    output[1] = route_y[route_count - 1u];
    free(route_x);
    free(route_y);
    return 1;
}
} // namespace unported_ledger_stubs

/* FUN_004437f0: pending-flag gate, then the activity-0x31 re-register. */
// FUNCTION: LEGORR 0x004437f0
void entity_dispatch_event31_for_pending_flag(EntitySlot* e)
{
    if (e == NULL) {
        return; /* deviation: the corpus strides blindly */
    }
    if (e->type != 3) {
        return;
    }
    if ((e->flags_3e8 & 0x80000000u) == 0u) {
        return;
    }
    if ((e->flags_3e4 & 0x20000000u) != 0u) {
        return;
    }
    e->flags_3e4 |= 0x20000000u;
    ai_set_activity(e, 0x31, 0); /* FUN_004409e0; 0x447c40 folds here */
    activity_register_bookkeeping(e); /* FUN_00447c40, target call order */
}

// FUNCTION: LEGORR 0x00443840
int entity_route_cell_candidate_accept(const EntitySlot* entity,
                                       const LevelState* level,
                                       int x, int y, int mode)
{
    if (level == NULL) return 0;
    if (mode == 0 && level_cell_walkable_flag(level, x, y)) return 0;
    if (!terrain_cell_route_and_occupancy_check(entity, level, x, y, x, y, 1))
        return 0;
    if (entity_cell_type_speed_multiplier_resolve(entity, level, x, y,
                                                   0.0f, NULL)) return 0;
    if (level_cell_mask_180000(level, x, y)) return 0;
    return 1;
}

namespace unported_ledger_stubs {
// PathStepAdvanceAndArrivalCheck
// FUNCTION: LEGORR 0x004438b0
float entity_path_step_advance_and_arrival_check(EntitySlot* entity,
                                                  float delta)
{
    EntityPathMotionRuntime* state;
    float movement_delta = 0.0f;
    float movement_scale = 1.0f;
    float position[2];
    float path_position[2];
    uint32_t path_flags;
    int resolver_ok = 1;
    if (entity == NULL) return 0.0f;
    if (entity->wp_count <= 0) {
        entity_action_sequence_reset(entity, 0);
        return (float)entity->flags_3e0;
    }
    if ((entity->wp_dynamic == NULL && entity->wp_count > AI_WAYPOINTS_MAX) ||
        (entity->wp_dynamic != NULL &&
         (unsigned int)entity->wp_count > entity->wp_capacity)) return (float)entity->flags_3e0;
    if (entity->wp_index < 0) entity->wp_index = 0;
    if (entity->wp_index >= entity->wp_count) {
        entity_action_sequence_reset(entity, 1);
        return (float)entity->flags_3e0;
    }
    state = entity_path_motion_runtime_get(entity, 1);
    if (state == NULL) {
        return (float)entity->flags_3e0;
    }
    if (state->waypoint_index != entity->wp_index ||
        state->total_length <= 0.0f) {
        state->carry_progress = 0.0f;
        int resolve_result = unported_ledger_stubs::entity_step_movement_vector_resolve(entity, 0,
            (entity->flags_3e8 & 8u) != 0u,
            (entity->flags_3e0 & 0x400u) == 0u);
        resolver_ok = resolve_result;
    }
    if (!resolver_ok || state->waypoint_index != entity->wp_index) {
        return (float)entity->flags_3e0;
    }

    if ((entity->flags_3e0 & 8u) == 0u &&
        ENTITY_WAYPOINTS(entity)[entity->wp_index].step_kind == 1u) {
        float work_position[2];
        int cx, cy;
        if (entity_work_object_position_get(entity, work_position) &&
            entity_work_entry_grid_pick(work_position[0], work_position[1], &cx, &cy) &&
            terrain_cell_walkable_flag_current((uint32_t)cx, (uint32_t)cy) &&
            ENTITY_WAYPOINTS(entity)[entity->wp_index].x == (float)cx &&
            ENTITY_WAYPOINTS(entity)[entity->wp_index].y == (float)cy) {
            int goal_y = (int)entity->pos_pair_2f8;
            int goal_x = (int)entity->pos_pair_2f4;
            if (terrain_cell_walkability_query_current(entity, (uint32_t)goal_x,
                    (uint32_t)goal_y, entity->flags_3e8 & 0x02000000u, 1))
                entity->flags_3e0 |= 0x18u;
            else entity->flags_3e0 |= 0x80000000u;
        }
    }

    unported_ledger_stubs::ledger_stub_00446610_EntityMovementTickScaleAndDeltaGet(
        entity, delta, &movement_delta, &movement_scale);

    (void)movement_scale;
    if ((entity->flags_3e8 & 0x4000u) == 0u)
        state->progress += movement_delta;
    if (state->progress > state->total_length) {
        state->carry_progress = state->progress - state->total_length;
        state->progress = state->total_length;
    }
    else {
        state->carry_progress = 0.0f;
    }
    path_position[0] = state->samples[0][0];
    path_position[1] = state->samples[0][1];
    if (state->sample_count > 1u && state->total_length > 0.0f) {
        float fraction = state->progress / state->total_length;
        path_position[0] += (state->samples[1][0] - state->samples[0][0]) * fraction;
        path_position[1] += (state->samples[1][1] - state->samples[0][1]) * fraction;
    }
    entity_world_position_set(entity, path_position[0], path_position[1],
                              entity->rot, 1);

    path_flags = ENTITY_WAYPOINTS(entity)[entity->wp_index].pending;
    entity_transform_position_get(entity, position);
    if ((entity->flags_3e0 & 8u) == 0u &&
        ENTITY_WAYPOINTS(entity)[entity->wp_index].step_kind == 3u &&
        entity->link_2fc != NULL) {
        float work_position[2];
        float dx, dy, distance;
        if (entity_work_object_position_get(entity->link_2fc,
                                            work_position)) {
            dx = work_position[0] - position[0];
            dy = work_position[1] - position[1];
            distance = (float)sqrt(dx * dx + dy * dy);
            if (distance <= 6.6f) {
                entity->flags_3e0 |= 0x200u;
                entity_action_sequence_reset(entity, 1);
                return (float)entity->flags_3e0;
            }
        }
    }
    if (ENTITY_WAYPOINTS(entity)[entity->wp_index].step_kind == 12u &&
        entity->link_2fc != NULL) {
        EntitySlot* linked = entity->link_2fc;
        float work_position[2];
        float dx, dy;
        if (entity_work_object_position_get(linked, work_position)) {
            dx = work_position[0] - position[0];
            dy = work_position[1] - position[1];
            if ((float)sqrt(dx * dx + dy * dy) <=
                entity_stats_float_at6c(linked)) {
                entity->flags_3e4 |= 4u;
                entity_action_sequence_reset(entity, 1);
                return (float)entity->flags_3e4;
            }
        }
    }
    if (ENTITY_WAYPOINTS(entity)[entity->wp_index].step_kind == 9u &&
        (path_flags & 0x0cu) == 0u) {
        float dx = state->target[0] - position[0];
        float dy = state->target[1] - position[1];
        if ((float)sqrt(dx * dx + dy * dy) <= 6.0f) {
            entity->flags_3e0 |= 0x80000u;
            entity_action_sequence_reset(entity, 1);
            return (float)entity->flags_3e0;
        }
    }
    if (state->progress >= state->total_length) {
        if (entity->wp_index + 1 < entity->wp_count) {
            ++entity->wp_index;
            if ((path_flags & 0x10u) != 0u &&
                ENTITY_WAYPOINTS(entity)[entity->wp_index].step_kind != 1u)
                ++entity->wp_index;
            if (entity->wp_index < entity->wp_count &&
                ENTITY_WAYPOINTS(entity)[entity->wp_index].step_kind == 5u)
                unported_ledger_stubs::ledger_stub_004477d0_PathStepLookaheadEntryWrite(
                    entity);
            state->waypoint_index = -1;
            /* Selecting the last waypoint does not mean its segment has
             * been travelled. Only a skip past the end is terminal here. */
            if (entity->wp_index >= entity->wp_count)
                entity->flags_3e8 |= 0x400u;
        }
        else {
            entity->flags_3e8 |= 0x400u;
        }
    }

    if ((entity->flags_3e8 & 0x400u) != 0u) {
        const unsigned int terminal_index =
            entity->wp_index >= 0 && entity->wp_index < entity->wp_count
                ? (unsigned int)entity->wp_index
                : (unsigned int)(entity->wp_count - 1);
        const AiWaypoint* terminal = &ENTITY_WAYPOINTS(entity)[terminal_index];
        const unsigned int kind = terminal->step_kind;
        switch (kind) {
        case 2u: entity->flags_3e0 |= 0x40u; break;
        case 3u: entity->flags_3e0 |= 0x200u; break;
        case 4u: entity->flags_3e0 |= 0x40000u; break;
        case 5u: entity->flags_3e0 |= 0x800u; break;
        case 6u: entity->flags_3e0 |= 0x1000000u; break;
        case 7u: entity->flags_3e0 |= 0x2000u; break;
        case 8u: entity_action_sequence_reset(entity, 1); break;
        case 9u: entity->flags_3e0 |= 0x80000u; break;
        case 10u: entity->flags_3e0 |= 0x20000u; break;
        case 11u: entity->flags_3e0 |= 0x40000000u; break;
        case 12u: entity->flags_3e4 |= 4u; break;
        case 13u: entity->flags_3e4 |= 0x40u; break;
        case 14u: entity->flags_3e4 |= 0x400u; break;
        case 15u: entity->flags_3e4 |= 0x8000u; break;
        case 16u: entity->flags_3e4 |= 0x200000u; break;
        case 17u: entity->flags_3e4 |= 0x4000000u; break;
        case 18u: entity->flags_3e4 |= 0x10000000u; break;
        case 19u: {
            EntitySlot* linked = entity->link_2fc;
            if (linked != NULL && linked->type == 4) {
                entity->flags_3ec |= 0x40u;
                linked->flags_3ec |= 0x40u;
                cmd_chain_drop(entity, 0x1c);
            }
            break;
        }
        case 20u: cmd_chain_drop(entity, 0x1d); break;
        case 21u: entity->flags_3e4 |= 0x80000000u; break;
        default: break;
        }
        if ((terminal->pending & 0x20u) != 0u)
            entity->flags_3e4 |= 0x80000u;
        entity_action_sequence_reset(entity, 1);
        return (float)entity->flags_3e0;
    }
    return (float)entity->flags_3e0;
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x004440a0
void linked_entity_flag_and_command_update(EntitySlot* e)
{
    if (e == NULL) {
        return; /* deviation: the corpus strides blindly */
    }
    EntitySlot* child = e->link_2fc;
    if (child == NULL || child->type != 4) {
        return;
    }
    e->flags_3ec |= 0x40u;
    child->flags_3ec |= 0x40u;
    cmd_chain_drop(e, 0x1c);
}

namespace unported_ledger_stubs {
// ActivityTimeoutAccumulateAndRaise
// FUNCTION: LEGORR 0x004440f0
void ledger_stub_004440f0_ActivityTimeoutAccumulateAndRaise(
    void* entity, ActivityTimeoutState* state,
    const ActivityTimeoutHooks* hooks, float delta)
{
    activity_timeout_accumulate_and_raise(entity, state, hooks, delta);
}

// EntityRandomCellStepRouteIssue
// FUNCTION: LEGORR 0x00444260
int ledger_stub_00444260_EntityRandomCellStepRouteIssue(EntitySlot* entity)
{
    static const int offsets[8][2] = {
        { -1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 },
        { 1, 0 }, { 1, -1 }, { 0, -1 }, { -1, -1 }
    };
    GameContext* context;
    LevelState* level;
    TerrainSurfaceGrid* surface;
    int start_x, start_y;
    float start_position[2];
    unsigned int attempt;
    if (entity == NULL ||
        entity_flags_mask107c33de_or_1c00203_any_test(entity)) return 0;
    context = entity->owner_ctx != NULL ? entity->owner_ctx
                                       : entity_pool_callback_context_get();
    if (context == NULL || context->level == NULL) return 0;
    level = context->level;
    surface = path_record_surface_grid_get();
    if (surface == NULL || surface->surface_vectors == NULL) return 0;
    entity_transform_position_get(entity, start_position);
    if (!terrain_world_xy_to_cell(level->ptr_terrain, start_position[0],
                                  start_position[1], &start_x, &start_y))
        return 0;
    for (attempt = 0u; attempt < 16u; ++attempt) {
        unsigned int direction =
            (unsigned int)level_random_draw15() % 8u;
        int cell_x = start_x + offsets[direction][0];
        int cell_y = start_y + offsets[direction][1];
        int* path_x = NULL;
        int* path_y = NULL;
        uint32_t path_count = 0u;
        uint32_t i;
        float vertices[12];
        float marker[2];
        float* route_x;
        float* route_y;
        int route_ok;
        if (cell_x < 0 || cell_y < 0 ||
            (uint32_t)cell_x >= surface->width ||
            (uint32_t)cell_y >= surface->height ||
            terrain_cell_route_and_occupancy_check(
                entity, level, cell_x, cell_y, cell_x, cell_y, 0) == 0 ||
            terrain_cell_surface_vertices_get(surface,
                (uint32_t)cell_x, (uint32_t)cell_y, vertices) < 0) {
            continue;
        }
        marker[0] = random_float_range(vertices[0] + 1.0f,
                                       vertices[6] - 1.0f);
        marker[1] = random_float_range(vertices[1] - 1.0f,
                                       vertices[7] + 1.0f);
        if (!terrain_neighbor_path_check(entity, (uint32_t)start_x,
                (uint32_t)start_y, (uint32_t)cell_x, (uint32_t)cell_y,
                &path_x, &path_y, &path_count, NULL, NULL) ||
            path_count == 0u) {
            free(path_x);
            free(path_y);
            continue;
        }
        route_x = (float*)malloc((size_t)path_count * sizeof(float));
        route_y = (float*)malloc((size_t)path_count * sizeof(float));
        if (route_x == NULL || route_y == NULL) {
            free(route_x);
            free(route_y);
            free(path_x);
            free(path_y);
            continue;
        }
        for (i = 0u; i < path_count; ++i) {
            route_x[i] = (float)path_x[i];
            route_y[i] = (float)path_y[i];
        }
        route_ok = entity_route_setup_and_marker_associate(
            entity, path_count, route_x, route_y, marker);
        free(route_x);
        free(route_y);
        free(path_x);
        free(path_y);
        if (route_ok) return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00444460
void ledger_stub_00444460_FlaggedVectorStepCommandIssue(
    EntitySlot* entity, const float vector[2])
{
    GameContext* context;
    LevelState* level;
    int start_cell[2] = { -1, -1 };
    int target_cell[2] = { -1, -1 };
    int* path_x = NULL;
    int* path_y = NULL;
    uint32_t path_count = 0u;
    float* route_x = NULL;
    float* route_y = NULL;
    uint32_t i;
    int route_installed;
    float position[2];
    float scale;
    if (entity == NULL || vector == NULL ||
        (entity->flags_3e0 & 0x800000u) != 0u ||
        !entity_shared_eligibility_check(entity, 1) ||
        entity_matches_current_selection_or_owner(entity) != 0 ||
        entity->owner_36c != NULL) return;
    context = entity->owner_ctx != NULL ? entity->owner_ctx
                                        : entity_pool_callback_context_get();
    if (context == NULL || context->level == NULL) return;
    level = context->level;
    entity_transform_position_get(entity, position);
    if (!terrain_world_xy_to_cell(level->ptr_terrain, position[0], position[1],
                                  &start_cell[0], &start_cell[1])) return;
    if (level_cell_walkable_flag(level, start_cell[0], start_cell[1]) != 0)
        return;

    entity_state_reset(entity, 1, 1);
    entity_transform_position_get(entity, position);
    scale = context->cfg != NULL
        ? block_get_float(context->cfg,
                          "Main::MiniFigureRunAway", 0.0f)
        : 0.0f;
    if (scale == 0.0f) scale = 10.0f;
    position[0] += vector[0] * scale;
    position[1] += vector[1] * scale;
    if (!terrain_world_xy_to_cell(level->ptr_terrain, position[0], position[1],
                                  &target_cell[0], &target_cell[1])) return;

    if (start_cell[0] == target_cell[0] &&
        start_cell[1] == target_cell[1]) {
        path_x = (int*)malloc(sizeof(int));
        path_y = (int*)malloc(sizeof(int));
        if (path_x == NULL || path_y == NULL) {
            free(path_x);
            free(path_y);
            path_x = NULL;
            path_y = NULL;
            goto route_finish;
        }
        path_x[0] = target_cell[0];
        path_y[0] = target_cell[1];
        path_count = 1u;
    }
    else if (!terrain_neighbor_path_check(entity,
            (uint32_t)start_cell[0], (uint32_t)start_cell[1],
            (uint32_t)target_cell[0], (uint32_t)target_cell[1],
            &path_x, &path_y, &path_count, NULL, NULL)) {
        free(path_x);
        free(path_y);
        return;
    }
    if (path_count != 0u &&
        (size_t)path_count <= ((size_t)-1) / sizeof(float)) {
        route_x = (float*)malloc((size_t)path_count * sizeof(float));
        route_y = (float*)malloc((size_t)path_count * sizeof(float));
        if (route_x != NULL && route_y != NULL) {
            for (i = 0u; i < path_count; ++i) {
                route_x[i] = (float)path_x[i];
                route_y[i] = (float)path_y[i];
            }
            route_installed = entity_route_setup_and_marker_associate(
                entity, path_count, route_x, route_y, position);
            if (route_installed && entity->wp_count > 0 &&
                (entity->wp_dynamic == NULL
                    ? entity->wp_count <= AI_WAYPOINTS_MAX
                    : (unsigned int)entity->wp_count <= entity->wp_capacity)) {
                AiWaypoint* final_step = &ENTITY_WAYPOINTS(entity)[
                    entity->wp_count - 1];
                final_step->pending =
                    (uint8_t)(final_step->pending | 0x40u);
            }
        }
    }
    free(route_x);
    free(route_y);
    free(path_x);
    free(path_y);
route_finish:
    entity->flags_3e0 |= 0x800000u;
    idle_queue_register(entity);
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x00444620
void entity_enter_event21_state_once(EntitySlot* entity)
{
    if (entity == NULL || (entity->flags_3e0 & 0x04000000u) != 0u ||
        entity->owner_36c != NULL) return;
    entity_state_reset(entity, 0, 1);
    entity->flags_3e0 |= 0x04000000u;
    ai_set_activity(entity, 0x21, 0);
    (void)activity_register_bookkeeping(entity);
    ai_idle_chain_record_attach(entity);
}

namespace unported_ledger_stubs {
// StepMovementVectorResolve
// FUNCTION: LEGORR 0x00444680
int entity_step_movement_vector_resolve(EntitySlot* entity, int advance,
                                       int use_saved_vector,
                                       int steering_mode)
{
    EntityPathMotionRuntime* state;
    const AiWaypoint* waypoint;
    float current[2];
    float target[2];
    float dx, dy, length;
    unsigned int index;
    if (entity == NULL || entity->wp_count <= 0) return 0;
    state = entity_path_motion_runtime_get(entity, 1);
    if (state == NULL) return 0;
    if (advance) ++entity->wp_index;
    if (entity->wp_index < 0 || entity->wp_index >= entity->wp_count)
        return 0;
    index = (unsigned int)entity->wp_index;
    waypoint = &ENTITY_WAYPOINTS(entity)[index];
    entity_transform_position_get(entity, current);
    if (!entity_path_waypoint_world_get(entity, index, target, 1)) return 0;
    if (use_saved_vector && (waypoint->pending & 8u) != 0u) {
        const void* terrain = entity->owner_ctx != NULL &&
            entity->owner_ctx->level != NULL
            ? entity->owner_ctx->level->ptr_terrain : NULL;
        const float blocksize = terrain != NULL
            ? terrain_blocksize_get(terrain) : 0.0f;
        dx = (waypoint->weight_y - 0.5f) *
             blocksize;
        dy = (0.5f - waypoint->weight_x) *
             blocksize;
        if (dx == 0.0f && dy == 0.0f) {
            dx = target[0] - current[0];
            dy = target[1] - current[1];
        }
    }
    else if ((waypoint->pending & 4u) != 0u) {
        switch (waypoint->pending & 3u) {
        case 0u: dx = 0.0f; dy = 1.0f; break;
        case 1u: dx = 1.0f; dy = 0.0f; break;
        case 2u: dx = 0.0f; dy = -1.0f; break;
        default: dx = -1.0f; dy = 0.0f; break;
        }
    }
    else {
        dx = target[0] - current[0];
        dy = target[1] - current[1];
    }
    length = (float)sqrt(dx * dx + dy * dy);
    if (length == 0.0f) {
        state->vector[0] = 0.0f;
        state->vector[1] = 0.0f;
        state->total_length = 0.0f;
        state->progress = state->carry_progress;
        state->waypoint_index = entity->wp_index;
        state->start[0] = current[0];
        state->start[1] = current[1];
        state->target[0] = current[0];
        state->target[1] = current[1];
        state->sample_count = 1u;
        state->samples[0][0] = current[0];
        state->samples[0][1] = current[1];
        return 1;
    }
    state->vector[0] = dx / length;
    state->vector[1] = dy / length;
    if (steering_mode != 0) {
        float forward_x = entity->attached_direction[0];
        float forward_y = entity->attached_direction[1];
        float forward_length = (float)sqrt(
            forward_x * forward_x + forward_y * forward_y);
        if (forward_length > 0.0f) {
            float dot;
            float cross;
            forward_x /= forward_length;
            forward_y /= forward_length;
            dot = forward_x * state->vector[0] +
                  forward_y * state->vector[1];
            cross = forward_x * state->vector[1] -
                    forward_y * state->vector[0];
            if (dot < 0.70710678f) {
                unsigned int activity = cross < 0.0f ? 9u : 10u;
                if ((entity->flags_3e0 & 0x400u) != 0u) activity += 11u;
                entity->flags_3e0 |= 4u;
                ai_set_activity(entity, (int)activity, 0);
                (void)activity_register_bookkeeping(entity);
            }
        }
    }
    state->start[0] = current[0];
    state->start[1] = current[1];
    state->target[0] = target[0];
    state->target[1] = target[1];
    state->total_length = length;
    state->progress = state->carry_progress;
    state->sample_count = 2u;
    state->samples[0][0] = current[0];
    state->samples[0][1] = current[1];
    state->samples[1][0] = target[0];
    state->samples[1][1] = target[1];
    state->segment_lengths[0] = length;
    state->waypoint_index = entity->wp_index;
    return 1;
}

// EntityTurnToVectorActivityIssue
// FUNCTION: LEGORR 0x00444fb0
void ledger_stub_00444fb0_EntityTurnToVectorActivityIssue(
    EntitySlot* entity, const float target[2])
{
    float current_length;
    float target_length;
    float current_x;
    float current_y;
    float target_x;
    float target_y;
    float dot;
    float angle;
    float cross[3];
    unsigned int activity;
    if (entity == NULL || target == NULL || entity->type != 2) return;
    current_length = (float)sqrt((double)(
        entity->attached_direction[0] * entity->attached_direction[0] +
        entity->attached_direction[1] * entity->attached_direction[1] +
        entity->attached_direction[2] * entity->attached_direction[2]));
    target_length = (float)sqrt((double)(target[0] * target[0] +
                                          target[1] * target[1]));
    if (current_length == 0.0f || target_length == 0.0f) return;
    entity->attached_direction[0] /= current_length;
    entity->attached_direction[1] /= current_length;
    entity->attached_direction[2] /= current_length;
    current_x = entity->attached_direction[0];
    current_y = entity->attached_direction[1];
    target_x = target[0] / target_length;
    target_y = target[1] / target_length;
    dot = current_x * target_x + current_y * target_y;
    if (dot > 1.0f) dot = 1.0f;
    else if (dot < -1.0f) dot = -1.0f;
    angle = (float)acos((double)dot);
    entity_action_sequence_reset(entity, 0);
    if (angle <= 0.7853982f) return;
    if (angle <= 2.3561945f) {
        cross[0] = target_y * entity->attached_direction[2];
        cross[1] = -target_x * entity->attached_direction[2];
        cross[2] = target_x * entity->attached_direction[1] -
                   target_y * entity->attached_direction[0];
        d3drm_vector_rotate(entity->attached_direction,
                            entity->attached_direction, cross,
                            -1.57079637f);
        if (cross[2] < 0.0f) {
            activity = 9u;
        }
        else {
            activity = 10u;
        }
    }
    else {
        entity->attached_direction[0] = -entity->attached_direction[0];
        entity->attached_direction[1] = -entity->attached_direction[1];
        entity->flags_3e0 |= 0x80u;
        activity = 10u;
    }
    if ((entity->flags_3e0 & 0x400u) != 0u) activity += 11u;
    entity->flags_3e0 |= 4u;
    ai_set_activity(entity, (int)activity, 0);
    (void)activity_register_bookkeeping(entity);
}

// EntityPathStepVectorNormalize
// FUNCTION: LEGORR 0x004451e0
void ledger_stub_004451e0_EntityPathStepVectorNormalize(
    EntityPathStepState* state, void* entity,
    const EntityPathStepHooks* hooks)
{
    entity_path_step_vector_normalize(state, entity, hooks);
}

// EntityMoveVectorSurfaceProbe
// FUNCTION: LEGORR 0x004455a0
int ledger_stub_004455a0_EntityMoveVectorSurfaceProbe(
    EntitySlot* entity, const EntityMotionSurfaceProbeHooks* hooks)
{
    return entity_move_vector_surface_probe(entity, hooks);
}

// EntityBehaviorCallbackTick
// FUNCTION: LEGORR 0x004456e0
void ledger_stub_004456e0_EntityBehaviorCallbackTick(
    EntityBehaviorCallbackState* state,
    const EntityBehaviorCallbackHooks* hooks, void* entity)
{
    entity_behavior_callback_tick(state, hooks, entity);
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x00445d70
int entity_flag_gated_callback_dispatch(EntitySlot* entity,
                                        uint32_t argument_a,
                                        uint32_t argument_b,
                                        uint32_t argument_c)
{
    int result = 0;
    if (entity == NULL) return result;
    if ((entity->flags_3e8 & 0x100u) != 0 && s_entity_flag_dispatch_first != NULL)
        s_entity_flag_dispatch_first(entity, argument_a, argument_b,
                                     argument_c, &result);
    if ((entity->flags_3e8 & 0x80u) != 0 && s_entity_flag_dispatch_second != NULL)
        s_entity_flag_dispatch_second(entity, argument_a, argument_b,
                                      argument_c, &result);
    return result;
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x00446610
void ledger_stub_00446610_EntityMovementTickScaleAndDeltaGet(
    void* entity, float delta, float* movement_delta, float* movement_scale)
{
    (void)entity_movement_scale_impl(entity, delta, movement_delta,
                                     movement_scale);
}
} // namespace unported_ledger_stubs

/* AdjacentRouteStepAppend (FUN_004468c0): append a half-cell waypoint only
 * while route-building is active and the requested cell touches the current
 * tail cardinally. Grow the port's inline waypoint span with realloc when the
 * route reaches it, matching the corpus's count-based buffer growth. */
// FUNCTION: LEGORR 0x004468c0
int entity_route_adjacent_step_append(EntitySlot* entity, int cell_x,
                                      int cell_y)
{
    AiWaypoint* waypoint;
    int last_x;
    int last_y;
    if (entity == NULL || (entity->flags_3e0 & 1u) == 0u
        || entity->wp_count <= 0
        || entity->wp_count == 0x7fffffff) {
        return 0;
    }
    waypoint = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    last_x = (int)waypoint->x;
    last_y = (int)waypoint->y;
    if (!((cell_x == last_x &&
           (cell_y == last_y - 1 || cell_y == last_y + 1)) ||
          (cell_y == last_y &&
           (cell_x == last_x - 1 || cell_x == last_x + 1)))) {
        return 0;
    }
    if (!entity_waypoint_storage_reserve(
            entity, (unsigned int)entity->wp_count + 1u)) return 0;
    waypoint = &ENTITY_WAYPOINTS(entity)[entity->wp_count];
    waypoint->x = (float)cell_x;
    waypoint->y = (float)cell_y;
    waypoint->weight_x = 0.5f;
    waypoint->weight_y = 0.5f;
    waypoint->pending = 0;
    waypoint->step_kind = 1;
    waypoint->reserved[0] = 0;
    waypoint->reserved[1] = 0;
    ++entity->wp_count;
    entity->flags_3e8 &= ~0x400u;
    return 1;
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x004469c0
int ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
    EntitySlot* entity, unsigned int goal_x, unsigned int goal_y,
    int include_diagonals, unsigned int* out_candidate, int strict)
{
    static const int cardinal_offsets[4][2] = {
        { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }
    };
    static const int diagonal_offsets[8][2] = {
        { -1, -2 }, { 0, -2 }, { 1, -1 }, { 1, 0 },
        { 0, 1 }, { -1, 1 }, { -2, 0 }, { -2, -1 }
    };
    int* path_x[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    int* path_y[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    uint32_t path_count[8] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    int candidate_x[8];
    int candidate_y[8];
    LevelState* level;
    GameContext* context;
    float goal_center[2];
    float entity_position[2];
    int terrain_start_x;
    int terrain_start_y;
    float blocksize;
    int direct_route_allowed = 1;
    int start_x, start_y;
    unsigned int best_index = 0xffffffffu;
    unsigned int candidate_count;
    unsigned int best_count = 0xffffffffu;
    unsigned int i;
    const int (*offsets)[2];
    if (entity == NULL) return 0;

    context = entity->owner_ctx != NULL ? entity->owner_ctx
                                        : entity_pool_callback_context_get();
    level = context != NULL ? context->level : NULL;
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    entity_transform_position_get(entity, entity_position);
    if (!terrain_world_xy_to_cell(level->ptr_terrain, entity_position[0],
                                  entity_position[1], &terrain_start_x,
                                  &terrain_start_y)) return 0;
    start_x = terrain_start_x;
    start_y = terrain_start_y;
    candidate_count = include_diagonals ? 8u : 4u;
    offsets = include_diagonals ? diagonal_offsets : cardinal_offsets;

    /* When param_6 is clear, the corpus suppresses the direct-current-cell
     * shortcut if the entity is already within one terrain block of the
     * goal cell center. */
    if (strict == 0) {
        if (terrain_cell_world_center_xy_get(level->ptr_terrain,
                (int)goal_x, (int)goal_y, goal_center)) {
            blocksize = terrain_blocksize_get(level->ptr_terrain);
            if (blocksize > 0.0f) {
                const float dx = goal_center[0] - entity_position[0];
                const float dy = goal_center[1] - entity_position[1];
                if ((float)sqrt((double)(dx * dx + dy * dy)) < blocksize)
                    direct_route_allowed = 0;
            }
        }
    }

    for (i = 0; i < candidate_count; ++i) {
        candidate_x[i] = (int)goal_x + offsets[i][0];
        candidate_y[i] = (int)goal_y + offsets[i][1];
        if (candidate_x[i] == start_x && candidate_y[i] == start_y &&
            direct_route_allowed &&
            (include_diagonals != 0 ||
             !level_cell_walkable_flag(level, start_x, start_y))) {
            const float direct_x = (float)start_x;
            const float direct_y = (float)start_y;
            if (!entity_route_setup_and_marker_associate(
                    entity, 1u, &direct_x, &direct_y, NULL)) return 0;
            if (out_candidate != NULL) *out_candidate = i;
            return 1;
        }
    }
    for (i = 0; i < candidate_count; ++i) {
        if (candidate_x[i] == start_x && candidate_y[i] == start_y) continue;
        if (!terrain_neighbor_path_check(entity, (uint32_t)start_x,
            (uint32_t)start_y, (uint32_t)candidate_x[i],
            (uint32_t)candidate_y[i], &path_x[i], &path_y[i],
            &path_count[i], NULL, NULL)) {
            free(path_x[i]);
            free(path_y[i]);
            path_x[i] = NULL;
            path_y[i] = NULL;
            path_count[i] = 0u;
        }
    }

    /* Retail ranks every successful non-walkable route first. If none has a
     * route, it makes a second pass over all successful candidates. Strict
     * less-than comparisons preserve the first candidate on equal lengths. */
    for (i = 0; i < candidate_count; ++i) {
        if (path_count[i] != 0u &&
            !level_cell_walkable_flag(level, candidate_x[i], candidate_y[i]) &&
            path_count[i] < best_count) {
            best_count = path_count[i];
            best_index = i;
        }
    }
    if (best_index == 0xffffffffu) {
        for (i = 0; i < candidate_count; ++i) {
            if (path_count[i] != 0u && path_count[i] < best_count) {
                best_count = path_count[i];
                best_index = i;
            }
        }
    }
    if (best_index == 0xffffffffu) goto cleanup;
    {
        float* route_x;
        float* route_y;
        unsigned int j;
        if ((size_t)best_count > ((size_t)-1) / sizeof(float)) {
            best_index = 0xffffffffu;
            goto cleanup;
        }
        route_x = (float*)malloc((size_t)best_count * sizeof(float));
        route_y = (float*)malloc((size_t)best_count * sizeof(float));
        if (route_x == NULL || route_y == NULL) {
            free(route_x);
            free(route_y);
            best_index = 0xffffffffu;
            goto cleanup;
        }
        for (j = 0; j < best_count; ++j) {
            route_x[j] = (float)path_x[best_index][j];
            route_y[j] = (float)path_y[best_index][j];
        }
        i = entity_route_setup_and_marker_associate(entity, best_count,
                                                     route_x, route_y, NULL);
        free(route_x);
        free(route_y);
        if (i == 0u) best_index = 0xffffffffu;
    }
cleanup:
    for (i = 0; i < candidate_count; ++i) {
        free(path_x[i]);
        free(path_y[i]);
    }
    if (best_index == 0xffffffffu) return 0;
    if (out_candidate != NULL) *out_candidate = best_index;
    return 1;
}

// EntityNearestCellRouteCommit
// FUNCTION: LEGORR 0x00446e40
int ledger_stub_00446e40_EntityNearestCellRouteCommit(
    EntitySlot* entity, unsigned int goal_x, unsigned int goal_y,
    int mode)
{
    static const int appended_offset_x[8] = {
        -1, 0, 0, 0, 0, -1, -1, -1
    };
    static const int appended_offset_y[8] = {
        -1, -1, -1, 0, 0, 0, 0, -1
    };
    static const float waypoint_weight_y[8] = {
        0.9f, 0.1f, 0.5f, 0.5f, 0.1f, 0.9f, 0.5f, 0.5f
    };
    static const float waypoint_weight_x[8] = {
        0.5f, 0.5f, 0.1f, 0.9f, 0.5f, 0.5f, 0.9f, 0.1f
    };
    unsigned int candidate;
    if (entity == NULL ||
        !terrain_cell_walkability_query_current(entity, goal_x, goal_y,
                                                mode, 1) ||
        !ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
            entity, goal_x, goal_y, mode, &candidate, 1)) return 0;
    if (candidate >= (mode != 0 ? 8u : 4u)) return 0;
    if (mode != 0) {
        AiWaypoint* final_step;
        if (!entity_route_adjacent_step_append(entity,
                (int)goal_x + appended_offset_x[candidate],
                (int)goal_y + appended_offset_y[candidate]) ||
            entity->wp_count <= 0 ||
            (entity->wp_dynamic == NULL
                ? entity->wp_count > AI_WAYPOINTS_MAX
                : (unsigned int)entity->wp_count > entity->wp_capacity))
            return 0;
        final_step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
        final_step->weight_y = waypoint_weight_y[candidate];
        final_step->weight_x = waypoint_weight_x[candidate];
    }
    else {
        AiWaypoint* final_step;
        if (!entity_route_adjacent_step_append(entity, (int)goal_x,
                                                (int)goal_y) ||
            entity->wp_count <= 0 ||
            (entity->wp_dynamic == NULL
                ? entity->wp_count > AI_WAYPOINTS_MAX
                : (unsigned int)entity->wp_count > entity->wp_capacity))
            return 0;
        final_step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
        final_step->weight_y = random_float_range(0.3f, 0.7f);
        final_step->weight_x = random_float_range(0.3f, 0.7f);
    }
    entity->pos_pair_2f4 = (float)goal_x;
    entity->pos_pair_2f8 = (float)goal_y;
    if (mode != 0) entity->flags_3e8 |= 0x02000000u;
    else entity->flags_3e8 &= ~0x02000000u;
    return 1;
}

// RegisterRelativeStateUpdateFragment
// FUNCTION: LEGORR 0x0044702d
int ledger_stub_0044702d_RegisterRelativeStateUpdateFragment(
    EntitySlot* entity, float target_x, float target_y)
{
    int result;
    AiWaypoint* step;
    if (entity == NULL) return 0;
    result = entity_route_to_cell_commit(entity, (int)target_x,
                                         (int)target_y, 3u);
    if (!result || entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity))
        return 0;
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    step->weight_x = random_float_range(0.3f, 0.7f);
    step->weight_y = random_float_range(0.3f, 0.7f);
    entity->flags_3e8 &= ~0x02000000u;
    return result;
}
} // namespace unported_ledger_stubs

namespace lego {
/* Spawn the type-five object at the corpus's sentinel position, then let
 * the linked-position routine construct its approach route. */
// FUNCTION: LEGORR 0x004470d0
int entity_conditional_type_five_spawn(EntitySlot* entity)
{
    GameContext* context;
    int destination[2];
    int slot;
    EntitySlot* spawned;
    if (entity == NULL || (entity->flags_3e8 & 0x04000000u) == 0u ||
        entity->owner_ctx == NULL ||
        !(entity_stats_float_at90(entity) < entity->field_340)) return 0;
    context = entity->owner_ctx;
    if (context->level == NULL ||
        !unported_ledger_stubs::
            ledger_stub_0044a5d0_NearestOpenCellCoordSearch(
                entity, &destination[0], &destination[1], 1, 0, 0)) return 0;
    slot = entity_create(context, 5, NULL, 0.0f, 0.0f, 0.0f, 0.0f);
    if (slot < 0) return 0;
    spawned = (EntitySlot*)entity_at(context, 5, slot);
    if (spawned == NULL) return 0;
    entity_world_position_set(spawned, 10000.0f, 10000.0f, 0.0f, 1);
    if (entity_try_create_linked_at_position(
            entity, (float)destination[0], (float)destination[1], spawned)) {
        void* texture_resource =
            terrain_cell_palette_resource_at_grid_position_current(
                destination);
        object_list_field_assign_all(spawned, texture_resource);
        entity->field_33c = 0.0f;
        return 1;
    }
    entity_type5_finalize_if_allowed(spawned);
    return 0;
}
/* The blocked-cell approach helper chooses a connected neighbor and appends
 * its weighted waypoint. This entry then marks that waypoint and establishes
 * the reciprocal object link. */
// FUNCTION: LEGORR 0x004471b0
int entity_try_create_linked_at_position(EntitySlot* entity, float cell_x,
                                         float cell_y, EntitySlot* linked)
{
    if (entity == NULL || linked == NULL ||
        (entity->flags_3e0 & 0x400u) != 0u) return 0;
    if (!unported_ledger_stubs::
            ledger_stub_00449880_BlockedCellApproachWeightWrite(
                entity, (unsigned int)(int)cell_x,
                (unsigned int)(int)cell_y, 4.0f)) return 0;
    if (entity->wp_count <= 0) return 0;
    ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 3u;
    entity->link_2fc = linked;
    linked->link_300 = entity;
    return 1;
}

/* The two RNG draws select a starting neighbor and whether to walk the
 * twelve offsets forward or backward. The second pass drops the terrain-mask
 * filter; exhaustion detaches the entity from its attached-object list. */
// FUNCTION: LEGORR 0x00447210
int entity_adjacent_placement_cell_search(EntitySlot* entity,
                                          EntitySlot* target)
{
    static const int offsets[12][2] = {
        { 2, 0 }, { 0, 2 }, { -2, 0 }, { 0, -2 },
        { 2, -1 }, { 2, 1 }, { 1, 2 }, { -1, 2 },
        { -2, 1 }, { -2, -1 }, { -1, -2 }, { 1, -2 }
    };
    LevelState* level;
    int target_cell[2];
    int16_t random_start;
    int16_t random_direction;
    uint32_t start;
    int direction;
    int pass;
    int i;
    if (entity == NULL || target == NULL || entity->type != 3 ||
        entity->owner_ctx == NULL || entity->owner_ctx->level == NULL)
        return 0;
    level = entity->owner_ctx->level;
    random_start = (int16_t)level_rand_u16();
    start = (uint32_t)((int)random_start % 12);
    random_direction = (int16_t)level_rand_u16();
    /* The reference's sign-corrected expression selects reverse traversal
     * only for a nonnegative odd second draw. Negative odd values do not
     * satisfy its final comparison. */
    direction = (random_direction >= 0 &&
        (((uint16_t)random_direction & 1u) != 0u)) ? -1 : 1;
    entity_position_to_terrain_cell(target, level, &target_cell[0],
                                    &target_cell[1]);
    for (pass = 0; pass < 2; ++pass) {
        for (i = 0; i < 12; ++i) {
            uint32_t sequence = start + (uint32_t)i;
            uint32_t index = direction < 0
                ? (0x18u - sequence) % 12u : sequence % 12u;
            int x;
            int y;
            x = target_cell[0] + offsets[index][0];
            y = target_cell[1] + offsets[index][1];
            if (pass == 0 && level_cell_mask_180000(level, x, y)) continue;
            if (unported_ledger_stubs::
                    ledger_stub_004473b0_RouteToCellCommandIssue(
                        entity, x, y, target)) return 1;
        }
    }
    unported_ledger_stubs::entity_attached_object_list_detach(entity, 0);
    return 0;
}
} // namespace lego

namespace unported_ledger_stubs {
// RouteToCellCommandIssue
// FUNCTION: LEGORR 0x004473b0
int ledger_stub_004473b0_RouteToCellCommandIssue(
    EntitySlot* entity, int target_x, int target_y, EntitySlot* target)
{
    LevelState* level;
    int target_cell[2];
    unsigned int candidate;
    unsigned int direction;
    float edge_weight;
    float step_fraction;
    AiWaypoint* step;
    if (entity == NULL || target == NULL || entity->owner_ctx == NULL ||
        (entity->flags_3e0 & 0x400u) == 0u ||
        entity_flags_mask107c33de_or_1c00203_any_test(entity)) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    entity_position_to_terrain_cell(target, level, &target_cell[0],
                                    &target_cell[1]);
    if (target_cell[0] == target_x && target_cell[1] == target_y) return 0;
    if (!entity_route_to_cell_commit(entity, target_x, target_y, 5u)) return 0;
    if (entity->carry_link_308 != NULL)
        ((EntitySlot*)entity->carry_link_308)->link_2fc = target;
    return 1;
}

// FUNCTION: LEGORR 0x004474f0
void ledger_stub_004474f0_LinkedEntityProximityResponse(EntitySlot* entity)
{
    EntitySlot* linked;
    float entity_position[3];
    float linked_position[3];
    float dx, dy, distance_squared, radius;
    if (entity == NULL) return;
    linked = entity->link_2fc;
    entity->link_2fc = NULL;
    if (linked == NULL ||
        !entity_world_center_position_read(entity, entity_position) ||
        !entity_world_center_position_read(linked, linked_position)) return;
    dx = linked_position[0] - entity_position[0];
    dy = linked_position[1] - entity_position[1];
    distance_squared = dx * dx + dy * dy;
    radius = entity_collision_radius_get(entity) * 2.0f +
             entity_collision_radius_get(linked);
    if (distance_squared < radius * radius) {
        const float direction[2] = { dx, dy };
        ledger_stub_00447950_EntityDirectionalActivityStart(linked,
                                                            direction, 1);
        entity_damage_apply(linked, 10.0f, 1);
    }
}

// EntityProgressAndNegativeDeltaHandler
// FUNCTION: LEGORR 0x004475c0
int ledger_stub_004475c0_EntityProgressAndNegativeDeltaHandler(
    EntitySlot* entity)
{
    EntitySlot* target;
    float delta;
    int complete = 0;
    if (entity == NULL) return 1;
    target = entity->link_2fc;
    if (target == NULL) return 1;
    if ((target->flags_3e4 & 0x00100000u) != 0u) return 0;

    if ((entity_state_flags_get(entity) & 0x00080000u) == 0u) {
        if ((entity->flags_3e8 & 0x00400000u) == 0u) {
            if (entity->type == 3) {
                message_dispatch(5, 0u, (uint32_t)(uintptr_t)target, NULL);
            }
            delta = entity_stats_float_at88(entity);
            if (delta >= 0.0f) {
                target->field_340 += delta;
            }
            else {
                entity_damage_apply(target, -delta, 1);
                target->flags_3e4 |= 0x40000000u;
            }
            if (target->field_340 < 100.0f) {
                entity_threshold_progress_event_queue(target, 0);
            }
            else {
                target->field_340 = 100.0f;
                complete = 1;
            }
        }
        else {
            building_upgrade_commit(target);
            entity->flags_3ec &= ~0x00400000u;
        }
    }
    else {
        if (!entity_shared_eligibility_check(target, 0)) return 1;
        {
            GameContext* context = entity->owner_ctx != NULL
                ? entity->owner_ctx : entity_pool_callback_context_get();
            float drain_time = context != NULL && context->cfg != NULL
                ? block_get_float(context->cfg, "Main::DrainTime", 10.0f)
                : 10.0f;
            if (drain_time == 0.0f) drain_time = 10.0f;
            if (entity->activity_counter_3d8 >= drain_time * 25.0f) {
                process_queued_grid_animations(entity, -1, 1.0f);
                entity->activity_counter_3d8 = 0.0f;
            }
        }
    }
    if (target->field_340 <= 0.0f) complete = 1;
    if (complete) entity->link_2fc = NULL;
    return complete;
}
} // namespace unported_ledger_stubs

/* EntityProgressAdvanceBy25 (FUN_00447730, findings 377/448): the corpus
 * routing-progress step — add 25 (the corpus writes `- -25.0`), clear
 * the link field, clamp to the entity's own +0x340 bound, and report
 * when the bound was reached. Field meanings stay with the routing
 * layer per finding 448. */
// FUNCTION: LEGORR 0x00447730
int entity_progress_advance_by_25(EntitySlot* e)
{
    const float advanced = e->progress_344 + 25.0f;
    e->link_2fc = NULL;
    e->progress_344 = advanced;
    if (e->field_340 < advanced) {
        e->progress_344 = e->field_340;
        return 1;
    }
    return 0;
}

/* EntityClearLinkedObjectState (FUN_00447780) — see entity.hpp. */
// FUNCTION: LEGORR 0x00447780
void entity_clear_linked_object_state(EntitySlot* e)
{
    if (e == NULL) {
        return; /* deviation: the corpus strides the slot blindly */
    }
    if ((e->flags_3e0 & 0x400u) == 0u) {
        return;
    }
    EntitySlot* carried = (EntitySlot*)e->carry_link_308;
    e->flags_3e0 &= ~0x400u;
    e->carry_state_324 = 0;
    e->carry_link_308 = NULL;
    if (carried != NULL) {
        carried->flags_3e8 |= 0x2000u;
    }
    entity_command_unlink_by_kind(e, 4, carried, 1);
}

namespace unported_ledger_stubs {
// PathStepLookaheadEntryWrite
// FUNCTION: LEGORR 0x004477d0
void ledger_stub_004477d0_PathStepLookaheadEntryWrite(EntitySlot* entity)
{
    EntitySlot* linked;
    EntitySlot* target;
    LevelState* level;
    void* terrain;
    AiWaypoint* waypoint;
    float start_world[2];
    float target_world[2];
    float moved_world[2];
    float center[2];
    float blocksize;
    float dx;
    float dy;
    float length;
    int cell_x;
    int cell_y;
    GameContext* context;

    if (entity == NULL || entity->wp_count <= 0
        || (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) {
        return;
    }
    /* The reference reads DAT_005560d4 rather than an entity-owned level.
     * Use the entity's context when present and the active pool context for
     * entities constructed outside the normal pool owner path. */
    context = entity->owner_ctx != NULL ? entity->owner_ctx
                                       : entity_pool_callback_context_get();
    if (context == NULL || context->level == NULL) return;
    level = context->level;
    terrain = level->ptr_terrain;
    if (terrain == NULL) return;
    linked = (EntitySlot*)entity->carry_link_308;
    if (linked == NULL) {
        return;
    }
    target = linked->link_2fc;
    if (target == NULL) {
        return;
    }

    waypoint = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    if (!terrain_cell_world_center_xy_get(terrain, (int)waypoint->x,
                                          (int)waypoint->y, start_world))
        return;
    entity_transform_position_get(target, target_world);
    dx = target_world[0] - start_world[0];
    dy = target_world[1] - start_world[1];
    length = (float)sqrt((double)(dx * dx + dy * dy));
    if (length == 0.0f) return;
    moved_world[0] = start_world[0] + (dx / length) * 0.1f;
    moved_world[1] = start_world[1] + (dy / length) * 0.1f;
    if (!terrain_world_xy_to_cell(terrain, moved_world[0], moved_world[1],
                                  &cell_x, &cell_y) ||
        !terrain_cell_world_center_xy_get(terrain, cell_x, cell_y, center))
        return;
    blocksize = terrain_blocksize_get(terrain);
    if (blocksize == 0.0f) return;
    waypoint->weight_x = (moved_world[0] - center[0]) / blocksize + 0.5f;
    waypoint->weight_y = (moved_world[1] - center[1]) / blocksize + 0.5f;
    waypoint->pending = (uint8_t)(waypoint->pending | 8u);
    linked->pos_pair_2f4 = target_world[0];
    linked->pos_pair_2f8 = target_world[1];
    linked->link_2fc = NULL;
}
} // namespace unported_ledger_stubs

/* TypeFourEntityCallbackDispatch (FUN_00447900): the source type-4 gate
 * selects a filtered pool walk; LAB_00447920 resets linked type-5 action
 * sequences when their attachment record is present. */
// FUNCTION: LEGORR 0x00447900
void entity_type_four_callback_dispatch(EntitySlot* source)
{
    if (source == NULL || source->type != 4) return;
    (void)entity_pool_find_by_callback_filtered_current(
        entity_type_four_linked_type_five_reset, source);
}

/* EntityDirectionalActivityStart (FUN_00447950) chooses an activity from the
 * facing vector's 45-degree cone, applies the activity transition and retry,
 * then runs a one-second tick. Retail reads the direction from entity +0x2ac
 * and +0x2b0 (the attached_direction sidecar). */
// FUNCTION: LEGORR 0x00447950
void entity_directional_activity_start(EntitySlot* entity,
                                       const float direction[2], int mode)
{
    EntitySlot* target;
    float forward_x, forward_y, length, dot, cross;
    int activity;
    int update_result;

    if (entity == NULL) return;
    target = entity;
    if ((entity->flags_3e4 & 2u) != 0u && entity->pair_link_3c0 != 0u)
        target = (EntitySlot*)entity->pair_link_3c0;
    if ((target->flags_3e0 & 0x1000u) != 0u ||
        !entity_shared_eligibility_check(target, 1)) return;

    if (direction == NULL) {
        /* The retail branch computes 0x24 for mode 0 and 0x1b for
         * nonzero mode. Finding 248's 0x25 label conflicts with the
         * shipped PE bytes (NEG/SBB, AND 0xfffffff7, ADD 0x24). */
        activity = mode != 0 ? 0x1b : 0x24;
    }
    else {
        forward_x = target->attached_direction[0];
        forward_y = target->attached_direction[1];
        length = (float)sqrt((double)(forward_x * forward_x +
                                      forward_y * forward_y));
        if (length == 0.0f) return;
        forward_x /= length;
        forward_y /= length;
        dot = forward_y * direction[1] + forward_x * direction[0];
        if (dot <= 0.2928932f) {
            if (dot >= -0.2928932f) {
                cross = forward_y * direction[0] -
                        forward_x * direction[1];
                activity = cross <= 0.0f ? 0x17 : 0x18;
            }
            else {
                activity = 0x19;
            }
        }
        else {
            activity = 0x1a;
        }
    }

    entity_state_reset(target, 0, 1);
    ai_set_activity(target, activity, 1);
    update_result = activity_register_bookkeeping(target);
    if (update_result == 0) {
        ai_set_activity(target, 0, 1);
        (void)activity_register_bookkeeping(target);
    }
    /* The retail tick dispatches through type-specific DDiScene methods.
     * The portable tick API needs those engine-owned callbacks; with no
     * installed tick ops it safely returns the supplied delta unchanged. */
    (void)unported_ledger_stubs::ledger_stub_00447b30_EntityActivityTick(
        target, 1.0f);
    target->flags_3e0 |= 0x1000u;
    if (target->type == 3) target->flags_3e8 &= 0x7fffffffu;
}

/* EntityDispatchEvent5AndSetFlag (FUN_00447b00) — see ai.hpp. */
// FUNCTION: LEGORR 0x00447b00
void entity_dispatch_event5_and_set_flag(EntitySlot* e)
{
    if (e == NULL) {
        return; /* deviation: the corpus strides blindly */
    }
    ai_set_activity(e, 5, 0); /* corpus activity index 5 */
    activity_register_bookkeeping(e); /* corpus site-level re-registration */
    e->flags_3e0 |= 0x200000u;
}

namespace unported_ledger_stubs {
// EntityActivityTick
// FUNCTION: LEGORR 0x00447b30
float ledger_stub_00447b30_EntityActivityTick(
    EntitySlot* entity, float delta)
{
    const EntityActivityTickHooks* hooks = NULL;
    float scaled_delta;
    float movement_delta = 0.0f;
    EntityTypeActivityTickHook tick = NULL;
    float timer_result = 0.0f;
    if (entity == NULL) return delta;
    entity->activity_advance_valid = 0u;
    if (hooks != NULL && hooks->delta_scale != NULL) {
        scaled_delta = hooks->delta_scale(hooks->context, entity, delta);
    }
    else {
        scaled_delta = delta;
        /* FUN_00447b30 always derives the animation delta through
         * FUN_00446610. Use its live EntitySlot/level-backed adapter when no
         * host override supplies that result. */
        ledger_stub_00446610_EntityMovementTickScaleAndDeltaGet(
            entity, delta, &movement_delta, &scaled_delta);
    }
    timer_result = entity->activity_timer;
    if ((entity->flags_3e0 & 0x4000u) == 0u) {
        if (hooks != NULL) {
            switch (entity->type) {
            case 1: tick = hooks->type1_tick; break;
            case 2: tick = hooks->type2_tick; break;
            case 3: tick = hooks->type3_tick; break;
            case 4: tick = hooks->type4_tick; break;
            default: break;
            }
        }
        if (tick != NULL) {
            timer_result = tick(hooks->context, entity, scaled_delta);
            /* FUN_00447b30 writes each type-specific worker's returned
             * timer to entity +0x2d4 before testing the finish gate. */
            entity->activity_timer = timer_result;
        }
        else if ((entity->type == 2 || entity->type == 3)) {
            float overflow = 0.0f;
            int activity = entity->activity;
            uint32_t generation = entity->activity_generation;
            int advanced = 0;
            if (s_actor_activity_advance_hook != NULL)
                advanced = s_actor_activity_advance_hook(
                    s_actor_activity_advance_context, entity, scaled_delta,
                    (uint32_t)entity->activity_param, &overflow);
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
            else advanced = actor_scene_advance(entity, scaled_delta,
                (uint32_t)entity->activity_param, &overflow);
#elif defined(LEGORRR_BACKEND_MINIWIN)
            /* EntityActivityTick (0x447b30): Creature_UpdateAnimation. */
            else advanced = creature_scene_advance(entity, scaled_delta,
                (uint32_t)entity->activity_param, &overflow);
#endif
            if (advanced && generation == entity->activity_generation &&
                activity == entity->activity && overflow == overflow &&
                fabs(overflow) <= FLT_MAX) {
                entity->activity_timer = overflow;
                entity->activity_advance_overflow = overflow;
                entity->activity_advance_activity = activity;
                entity->activity_advance_generation = generation;
                entity->activity_advance_valid = 1u;
            }
            else entity->activity_timer = 0.0f;
            timer_result = entity->activity_timer;
        }
        else if ((entity->flags_3e8 & 0x20000u) != 0u) {
            if (hooks != NULL && hooks->behavior_update != NULL)
                hooks->behavior_update(hooks->context, entity, delta);
            else
                ledger_stub_00447ea0_EntityTypeBehaviorUpdate(
                    entity, delta, NULL);
        }
        else if (entity->type == 10 && hooks != NULL &&
                 hooks->type10_tick != NULL) {
            timer_result = hooks->type10_tick(hooks->context, entity,
                                               scaled_delta);
            entity->activity_timer = timer_result;
        }
    }
    if (timer_result > 0.0f) {
        if (hooks != NULL && hooks->activity_finish != NULL)
            hooks->activity_finish(hooks->context, entity);
        else
            task_record_kind14_remove_if_head(entity);
    }
    return scaled_delta;
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x00447c40
int activity_register_bookkeeping(EntitySlot* e)
{
    int result = 0;
    if (e == NULL || e->activity == e->activity_last) return 0;
    if (!activity_skips_idle_chain(e->activity)) {
        ai_idle_chain_record_attach(e);
    }
    /* FUN_00447c40 dispatches type 2/3 changes through ActivityRunnerUpdate
     * before it commits activity_last. The activity is the table string and
     * +0x2d4's current argument is carried as the callback's raw value word. */
    if (e->type == 2 || e->type == 3) {
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
        result = actor_scene_activity_set(e, ai_activity_name(e->activity),
                                           e->activity_timer);
#elif defined(LEGORRR_BACKEND_MINIWIN)
        result = creature_scene_activity_set(e, ai_activity_name(e->activity),
                                             e->activity_timer);
#else
        uint32_t initial_time;
        memcpy(&initial_time, &e->activity_timer, sizeof(initial_time));
        result = entity_activity_runner_activity_update(
            e, e->type, (void*)ai_activity_name(e->activity),
            (void*)(uintptr_t)initial_time);
#endif
    }
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    if (e->type == 4)
        result = building_scene_activity_set(e, ai_activity_name(e->activity));
#endif
    e->activity_last = e->activity;
    e->activity_timer = 0.0f;
    return result;
}

namespace unported_ledger_stubs {
// EntityTypeBehaviorUpdate
// FUNCTION: LEGORR 0x00447ea0
void ledger_stub_00447ea0_EntityTypeBehaviorUpdate(
    EntitySlot* entity, float delta, const EntityTypeBehaviorHooks* hooks)
{
    int animation_done;
    if (entity == NULL) return;
    switch (entity->type) {
    case 5:
        if ((entity->flags_3e8 & 0x2000u) != 0u &&
            hooks != NULL && hooks->boulder_behavior != NULL)
            hooks->boulder_behavior(hooks->context, entity, delta);
        return;
    case 6:
        if (entity->attach_304 == NULL &&
            entity->attached_scene_object != NULL) {
            render_object_rotate_axis(entity->attached_scene_object, 1u,
                                      0.0f, 1.0f, 0.0f,
                                      delta * 0.01f);
        }
        return;
    case 8:
    case 13:
        if ((entity->flags_3e8 & 0x10000u) == 0u &&
            (entity->flags_3e0 & 0x80000u) != 0u) {
            entity->flags_3e0 &= ~0x80000u;
            entity->flags_3e8 |= 0x10000u;
            entity_type_behavior_animation_play(entity, hooks, "TickDown");
            /* The reference starts the named action at frame zero before it
             * updates the cell and broadcasts message 0x0d. The portable
             * action-name setter is still renderer-owned, but the scene time
             * reset and the message dispatch have direct equivalents. */
            if (entity->attached_scene_object != NULL) {
                (void)ddiscene_frame_update_and_callbacks(
                    entity->attached_scene_object, 0.0f);
            }
            entity_type_behavior_cell_flag_80000_set(entity, 1);
            message_dispatch(0x0d, 0u,
                (uint32_t)(uintptr_t)entity, NULL);
            if (hooks != NULL && hooks->terrain_activity != NULL)
                hooks->terrain_activity(hooks->context, entity);
            return;
        }
        if ((entity->flags_3e8 & 0x10000u) != 0u) {
            animation_done = entity_type_behavior_animation_advance(
                entity, delta, hooks) != 0.0f;
            if (animation_done || entity->field_340 <= 0.0f) {
                entity->flags_3e8 &= ~0x10000u;
                entity->flags_3e8 |= 0x800000u;
                entity->field_340 = -4.0f;
                entity_type_behavior_cell_flag_80000_set(entity, 0);
                if (hooks != NULL && hooks->terrain_activity != NULL)
                    hooks->terrain_activity(hooks->context, entity);
            }
            return;
        }
        (void)entity_type_behavior_animation_advance(entity, delta, hooks);
        return;
    case 11:
        if ((entity->flags_3e0 & 0x80000u) != 0u) {
            LevelState* level = entity->owner_ctx != NULL
                ? entity->owner_ctx->level : NULL;
            int cell_x = -1;
            int cell_y = -1;
            float center[2];
            float heading;
            entity->flags_3e0 &= ~0x80000u;
            register_object_at_grid_position(entity);
            if (level != NULL && level->ptr_terrain != NULL) {
                entity_position_to_terrain_cell(entity, level,
                                                &cell_x, &cell_y);
                if (terrain_cell_world_center_xy_get(level->ptr_terrain,
                        cell_x, cell_y, center)) {
                    heading = entity_vector_direction_angle_radians_get(
                        entity) + 0.7853982f - random_float_range(0.0f, 1.0f);
                    entity_world_position_set(entity, center[0], center[1],
                                              heading, 1);
                    level_cell_secondary_bit8_set(level, cell_x, cell_y, 0);
                }
            }
        }
        return;
    case 9:
        /* The reference first advances the scene action; only a nonzero
         * result permits the expand/teleport state transitions below. */
        if (entity_type_behavior_animation_advance(entity, delta, hooks) ==
            0.0f)
            return;
        if ((entity->flags_3e0 & 0x80000u) != 0u) {
            entity->flags_3e0 &= ~0x80000u;
            entity->flags_3e0 |= 0x100u;
            entity_type_behavior_animation_play(entity, hooks, "Expand");
            return;
        }
        if ((entity->flags_3e0 & 0x100u) != 0u) {
            entity->flags_3e0 &= ~0x100u;
            entity->flags_3e8 |= 0x10000u;
            /* The retail action name at DAT_004a5118 is not recovered. */
            return;
        }
        if ((entity->flags_3e8 & 0x800000u) != 0u) {
            entity->field_340 = 100.0f;
            entity->flags_3e8 &= ~0x800000u;
            entity->flags_3e0 |= 0x200000u;
            entity_type_behavior_animation_play(entity, hooks, "Teleport");
            return;
        }
        if ((entity->flags_3e0 & 0x200000u) != 0u) {
            entity->flags_3e0 &= ~0x200000u;
            entity->field_340 = -4.0f;
            entity->flags_3e8 |= 0x800000u;
        }
        return;
    case 12:
        /* FUN_00447ea0 suppresses delta while this object is in its
         * disintegration state, unless the 0x100 phase bit is set. */
        if (entity_type_behavior_animation_advance(
                entity,
                ((entity->flags_3e0 & 0x2000000u) != 0u &&
                 (entity->flags_3e0 & 0x100u) == 0u) ? 0.0f : delta,
                hooks) == 0.0f) return;
        if ((entity->flags_3e0 & 0x100u) != 0u) {
            entity->flags_3e0 &= ~0x100u;
            entity->flags_3e8 |= 0x10000u;
            entity_type_behavior_animation_play(entity, hooks, "FlapInWind");
            return;
        }
        if ((entity->flags_3e8 & 0x800000u) != 0u) {
            entity->field_340 = 100.0f;
            entity->flags_3e8 &= ~0x800000u;
            entity->flags_3e0 |= 0x200000u;
            entity_type_behavior_animation_play(entity, hooks,
                                                "Disintegrate");
            return;
        }
        if ((entity->flags_3e0 & 0x200000u) != 0u) {
            EntitySlot* owner = (EntitySlot*)entity->wake_target;
            entity->flags_3e0 &= ~0x200000u;
            entity->field_340 = -4.0f;
            entity->flags_3e8 |= 0x800000u;
            if (owner != NULL) owner->flags_3e4 &= ~0x08000000u;
        }
        return;
    case 18:
        /* The type-0x12 sequence advances first. DAT_004a50f4 is the
         * configured "Melt" action token (docs/dat/globals.csv). */
        if (entity_type_behavior_animation_advance(entity, delta, hooks) ==
            0.0f)
            return;
        if ((entity->flags_3e0 & 0x100u) != 0u) {
            entity->flags_3e0 &= ~0x100u;
            entity->flags_3e8 |= 0x10000u;
            entity_type_behavior_animation_play(entity, hooks, "Normal");
            return;
        }
        if ((entity->flags_3e8 & 0x800000u) != 0u) {
            entity->field_340 = 100.0f;
            entity->flags_3e8 &= ~0x800000u;
            entity->flags_3e0 |= 0x200000u;
            entity_type_behavior_animation_play(entity, hooks, "Melt");
            return;
        }
        if ((entity->flags_3e0 & 0x200000u) != 0u) {
            EntitySlot* owner = (EntitySlot*)entity->wake_target;
            entity->flags_3e0 &= ~0x200000u;
            entity->field_340 = -4.0f;
            entity->flags_3e8 |= 0x800000u;
            if (owner != NULL) owner->flags_3e4 &= ~0x08000000u;
        }
        return;
    case 14:
    case 16:
    case 17:
    case 19:
        (void)entity_type_behavior_animation_advance(entity, delta, hooks);
        return;
    default:
        return;
    }
}
} // namespace unported_ledger_stubs

/* EntityClearNestedState8Fields (FUN_004487c0): clear the linked type-8
 * object's +0x2f4 position and run the entity's attached-object detach
 * pass. The separate nested +0xbe field has no EntitySlot mirror yet. */
// FUNCTION: LEGORR 0x004487c0
void entity_clear_nested_state_8_fields(EntitySlot* e)
{
    if (e == NULL || (e->flags_3e0 & 0x400u) == 0u
        || e->carry_link_308 == NULL) {
        return;
    }
    EntitySlot* nested = (EntitySlot*)e->carry_link_308;
    if (nested->type != 8) {
        return;
    }
    nested->pos_pair_2f8 = 0.0f;
    nested->pos_pair_2f4 = 0.0f;
    unported_ledger_stubs::entity_attached_object_list_detach(e, 1);
}

namespace unported_ledger_stubs {
// EntityMoveToCellCoordinates
// FUNCTION: LEGORR 0x00448800
int ledger_stub_00448800_EntityMoveToCellCoordinates(
    EntitySlot* entity, const int cell[2])
{
    const uint32_t* linked_record;
    LevelState* level;
    int x;
    int y;

    if (entity == NULL || cell == NULL || entity->carry_link_308 == NULL) {
        return 0;
    }
    linked_record = (const uint32_t*)entity->carry_link_308;
    if (linked_record[0] != 8u) {
        return 0;
    }
    level = (entity->owner_ctx != NULL) ? entity->owner_ctx->level : NULL;
    x = cell[0];
    y = cell[1];
    if (!entity_move_destination_cell_usable(entity, level, x, y)) {
        return 0;
    }
    if (!ledger_stub_00449880_BlockedCellApproachWeightWrite(
            entity, (unsigned int)x, (unsigned int)y, 7.0f)) {
        return 0;
    }
    if (entity->wp_count > 0 &&
        (entity->wp_dynamic == NULL
            ? entity->wp_count <= AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count <= entity->wp_capacity)) {
        ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 9u;
    }
    entity->pos_pair_2f4 = (float)x;
    entity->pos_pair_2f8 = (float)y;
    return 1;
}

// RecordGatedStepCommandIssue
// FUNCTION: LEGORR 0x00448880
int ledger_stub_00448880_RecordGatedStepCommandIssue(
    EntitySlot* entity, const int cell[2])
{
    const uint32_t* linked_record;
    LevelState* level;
    int x;
    int y;
    int source_x;
    int source_y;
    int* path_x;
    int* path_y;
    uint32_t path_count;
    float* route_x_values;
    float* route_y_values;
    uint32_t i;

    if (entity == NULL || cell == NULL || entity->carry_link_308 == NULL) {
        return 0;
    }
    linked_record = (const uint32_t*)entity->carry_link_308;
    if (linked_record[0] != 0x0du) {
        return 0;
    }
    level = (entity->owner_ctx != NULL) ? entity->owner_ctx->level : NULL;
    x = cell[0];
    y = cell[1];
    if (level == NULL || !level_cell_flag8_check(level, x, y)) {
        return 0;
    }
    source_x = 0;
    source_y = 0;
    route_x_values = NULL;
    route_y_values = NULL;
    entity_position_to_terrain_cell(entity, level, &source_x, &source_y);
    if (source_x == x && source_y == y) {
        float same_cell_x = (float)x;
        float same_cell_y = (float)y;
        /* FUN_00448880 issues a one-cell route on the same-cell branch, then
         * returns zero without changing the step opcode or paired target. */
        (void)entity_route_setup_and_marker_associate(entity, 1u,
            &same_cell_x, &same_cell_y, NULL);
        return 0;
    }
    else {
        path_x = NULL;
        path_y = NULL;
        path_count = 0;
        if (!terrain_neighbor_path_check(level, (uint32_t)source_x,
                (uint32_t)source_y, (uint32_t)x, (uint32_t)y,
                &path_x, &path_y, &path_count, NULL, NULL)) {
            free(path_x);
            free(path_y);
            return 0;
        }
        if (path_count == 0u ||
            (size_t)path_count > ((size_t)-1) / sizeof(float)) {
            free(path_x);
            free(path_y);
            return 0;
        }
        route_x_values = (float*)malloc((size_t)path_count * sizeof(float));
        route_y_values = (float*)malloc((size_t)path_count * sizeof(float));
        if (route_x_values == NULL || route_y_values == NULL) {
            free(route_x_values);
            free(route_y_values);
            free(path_x);
            free(path_y);
            return 0;
        }
        for (i = 0; i < path_count; ++i) {
            route_x_values[i] = (float)path_x[i];
            route_y_values[i] = (float)path_y[i];
        }
        free(path_x);
        free(path_y);
    }
    if (!entity_route_setup_and_marker_associate(entity, path_count,
            route_x_values, route_y_values, 0)) {
        free(route_x_values);
        free(route_y_values);
        return 0;
    }
    free(route_x_values);
    free(route_y_values);
    if (entity->wp_count > 0)
        ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 9u;
    entity->pos_pair_2f4 = (float)x;
    entity->pos_pair_2f8 = (float)y;
    return 1;
}
} // namespace unported_ledger_stubs

/* EntityKind6TickDownMarkerSpawn (FUN_004489a0): the original uses the
 * type-13 object factory, applies the entity pose and attaches the
 * `TickDown` scene action, then raises the marker flag. The reconstruction
 * has no scene-action resource owner yet, so the type-13 entity and flag
 * are modeled while the visual action remains absent. */
// FUNCTION: LEGORR 0x004489a0
int entity_kind6_tick_down_marker_spawn(GameContext* ctx, EntitySlot* e)
{
    LevelState* level;
    float position[2];
    float heading;
    int cell_x = -1;
    int cell_y = -1;
    int index;
    EntitySlot* marker;
    if (ctx == NULL || e == NULL || ai_activity_scan(e, 6) == 0) {
        return 0;
    }
    level = ctx->level;
    if (level == NULL) return 0;
    /* The retail routine reads its pose and cell through 0x42260 and 0x424f0,
     * and takes its heading from 0x42480. Slot x/y/rot are placement mirrors,
     * not authoritative when a scene pose has moved the source. */
    entity_transform_position_get(e, position);
    entity_position_to_terrain_cell(e, level, &cell_x, &cell_y);
    if (!level_coord_pair_within_bounds(level, cell_x, cell_y)) return 0;
    heading = entity_vector_direction_angle_radians_get(e);
    index = entity_create(ctx, 13, "TickDown", position[0], position[1],
                          e->z, heading);
    if (index < 0) {
        return 0;
    }
    marker = (EntitySlot*)entity_at(ctx, 13, index);
    if (marker == NULL) {
        return 0;
    }
    /* FUN_004489a0 marks the source cell after creating the type-13 object. */
    level_cell_flag_80000_set(level, cell_x, cell_y, 1);
    marker->flags_3e8 |= 0x10000u;
    /* The reference also selects the "TickDown" action on the marker's scene
     * object. This backend has scene-action name lookup but no action setter;
     * entity creation preserves the name for the renderer owner to resolve. */
    return 1;
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x00448a60
int ledger_stub_00448a60_EntityNeighborCellMoveCommandHandle(
    EntitySlot* entity, const int target_cell[2])
{
    LevelState* level;
    AiWaypoint* step;
    float weight;
    unsigned int direction;
    int32_t random_value;
    EntitySlot* linked_entity;
    if (entity == NULL || target_cell == NULL || entity->owner_ctx == NULL ||
        entity->carry_link_308 == NULL) return 0;
    linked_entity = (EntitySlot*)entity->carry_link_308;
    if (linked_entity->type != 11) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || !terrain_cell_has_occupied_or_flagged_neighbor(
            level, target_cell[0], target_cell[1], NULL))
        return 0;
    if (!entity_route_path_install(entity, level, target_cell[0],
                                   target_cell[1], NULL) || entity->wp_count <= 0)
        return 0;
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    step->step_kind = 9u;
    if (entity->type == 1) {
        random_value = (int16_t)rng_thunk_draw15();
        if (random_value < 0) random_value = -random_value;
        direction = (unsigned int)random_value & 3u;
        weight = unported_ledger_stubs::entity_cell_step_random_range(
            0.55f, 0.75f);
        switch (direction) {
        case 0u: step->weight_x = 0.5f; step->weight_y = weight; break;
        case 1u: step->weight_y = 0.5f; step->weight_x = weight; break;
        case 2u: step->weight_x = 0.5f;
                 step->weight_y = 1.0f - weight; break;
        default: step->weight_y = 0.5f;
                 step->weight_x = 1.0f - weight; break;
        }
        step->pending = (uint8_t)(step->pending | 8u);
    }
    entity->pos_pair_2f4 = (float)target_cell[0];
    entity->pos_pair_2f8 = (float)target_cell[1];
    return 1;
}
} // namespace unported_ledger_stubs

/* EntitySetActivityPlace (FUN_00448c50) — see ai.hpp. */
// FUNCTION: LEGORR 0x00448c50
int entity_set_activity_place(EntitySlot* e)
{
    if (e == NULL) {
        return 1; /* deviation: the corpus strides blindly, answers 1 */
    }
    ai_set_activity(e, 30, 0); /* corpus activity index 30 */
    activity_register_bookkeeping(e); /* corpus site-level re-registration */
    e->flags_3e4 |= 0x200u;
    return 1;
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x00448c90
int ledger_stub_00448c90_CardinalStepCommandIssueAndLink(
    EntitySlot* entity, EntitySlot* target, int link_mode)
{
    LevelState* level;
    int target_cell[2];
    int source_cell[2];
    AiWaypoint* step;
    unsigned int direction;
    if (entity == NULL || target == NULL || target->type != 4) return 0;
    if (entity->type == 2)
        return ledger_stub_00448eb0_ProximityStepCommandIssueWithJitter(
            entity, target, 1);
    if (entity->type != 1 || entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    target_cell[0] = -1;
    target_cell[1] = -1;
    if (!entity_route_live_position_to_cell(target, level, &target_cell[0],
                                             &target_cell[1])) return 0;
    source_cell[0] = -1;
    source_cell[1] = -1;
    if (!entity_route_live_position_to_cell(entity, level, &source_cell[0],
                                             &source_cell[1])) return 0;
    if (source_cell[0] < 0 || source_cell[1] < 0 ||
        !entity_route_path_install(entity, level, target_cell[0],
                                   target_cell[1], NULL) || entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    direction = entity_route_cardinal_direction(source_cell[0], source_cell[1],
                                                 target_cell[0], target_cell[1]);
    step->step_kind = 15u;
    step->pending = (uint8_t)(step->pending | 4u | direction);
    entity->link_2fc = target;
    target->link_300 = entity;
    entity->command_mode_404 = link_mode;
    return 1;
}

// ProximityStepCommandIssueWithJitter
// FUNCTION: LEGORR 0x00448eb0
int ledger_stub_00448eb0_ProximityStepCommandIssueWithJitter(
    EntitySlot* entity, EntitySlot* target, int marker)
{
    LevelState* level;
    int target_cell[2];
    unsigned int candidate;
    unsigned int direction;
    float edge_weight;
    float step_fraction;
    AiWaypoint* step;
    if (entity == NULL || target == NULL || entity->type != 2 ||
        target->type != 4 || entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    target_cell[0] = -1;
    target_cell[1] = -1;
    if (!entity_route_live_position_to_cell(target, level, &target_cell[0],
                                             &target_cell[1])) return 0;
    if (target_cell[0] < 0 || target_cell[1] < 0) return 0;
    if (!ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
            entity, (unsigned int)target_cell[0],
            (unsigned int)target_cell[1], 0, &candidate, 1) ||
        entity->wp_count <= 0) return 0;
    direction = (candidate - 2u) & 3u;
    edge_weight = unported_ledger_stubs::entity_cell_step_random_range(
        0.3f, 0.9f);
    step_fraction = 4.0f / terrain_context_scale_get(level->ptr_terrain);
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    switch (direction) {
    case 0u: step->weight_y = edge_weight;
            step->weight_x = 1.0f - step_fraction; break;
    case 1u: step->weight_y = 1.0f - step_fraction;
            step->weight_x = edge_weight; break;
    case 2u: step->weight_y = edge_weight;
            step->weight_x = step_fraction; break;
    default: step->weight_y = step_fraction;
            step->weight_x = edge_weight; break;
    }
    step->step_kind = marker ? 15u : 14u;
    step->pending = (uint8_t)(step->pending | 4u | direction);
    entity->link_2fc = target;
    return 1;
}

// FUNCTION: LEGORR 0x00449010
int ledger_stub_00449010_EntityKind6RecordApproachRouteIssue(EntitySlot* entity)
{
    int cell[2];
    EntitySlot* kind_six_record;
    AiWaypoint* step;
    if (entity == NULL || entity->type != 2 ||
        (entity->flags_3e0 & 0x400u) == 0u) return 0;
    kind_six_record = (EntitySlot*)entity->carry_link_308;
    if (kind_six_record == NULL || kind_six_record->type != 6
        || (kind_six_record->flags_3e8 & 0x80000000u) == 0u) return 0;
    if (!unported_ledger_stubs::type11_cell_coordinate_nearest_resolve(entity, cell)) return 0;
    if (!ledger_stub_00449880_BlockedCellApproachWeightWrite(
            entity, (unsigned int)cell[0], (unsigned int)cell[1], 3.0f))
        return 0;
    if (entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    step->step_kind = 18u;
    entity->pos_pair_2f4 = (float)cell[0];
    entity->pos_pair_2f8 = (float)cell[1];
    return 1;
}

// ProximityStepCommandIssueWithLink
// FUNCTION: LEGORR 0x004490a0
int ledger_stub_004490a0_ProximityStepCommandIssueWithLink(
    EntitySlot* entity, EntitySlot* target, int mode, int set_global_gate)
{
    LevelState* level;
    int target_cell[2];
    unsigned int candidate;
    unsigned int direction;
    float edge_weight;
    float step_fraction;
    AiWaypoint* step;
    if (entity == NULL || target == NULL || target->type != 4 ||
        (entity->type != 2 && entity->type != 3) ||
        entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    target_cell[0] = -1;
    target_cell[1] = -1;
    if (!entity_route_live_position_to_cell(target, level, &target_cell[0],
                                             &target_cell[1])) return 0;
    if (target_cell[0] < 0 || target_cell[1] < 0) return 0;
    if (!ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
            entity, (unsigned int)target_cell[0],
            (unsigned int)target_cell[1], 0, &candidate, entity->type != 3) ||
        entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    direction = (candidate - 2u) & 3u;
    if (entity->type == 3 && (entity->flags_3e8 & 0x80000u) == 0u)
        edge_weight = 0.5f;
    else
        edge_weight = entity_route_weight_random(0.2f, 0.8f);
    step_fraction = 4.0f / terrain_context_scale_get(level->ptr_terrain);
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    switch (direction) {
    case 0u: step->weight_y = edge_weight;
            step->weight_x = 1.0f - step_fraction; break;
    case 1u: step->weight_y = 1.0f - step_fraction;
            step->weight_x = edge_weight; break;
    case 2u: step->weight_y = edge_weight;
            step->weight_x = step_fraction; break;
    default: step->weight_y = step_fraction;
            step->weight_x = edge_weight; break;
    }
    step->step_kind = 6u;
    step->pending = (uint8_t)(step->pending | 4u | direction |
                              (mode != 0 ? 0x20u : 0u));
    entity->link_2fc = target;
    if (set_global_gate) entity->flags_3ec |= 0x00400000u;
    return 1;
}

// EntityWalkableTargetCellRouteIssue
// FUNCTION: LEGORR 0x00449240
int ledger_stub_00449240_EntityWalkableTargetCellRouteIssue(
    EntitySlot* entity, const int target_xy[2])
{
    LevelState* level;
    if (entity == NULL || target_xy == NULL || entity->owner_ctx == NULL)
        return 0;
    level = entity->owner_ctx->level;
    if (!terrain_cell_allows_placement(entity, level, target_xy[0],
                                       target_xy[1])) return 0;
    if (!ledger_stub_00449880_BlockedCellApproachWeightWrite(entity,
            (unsigned int)target_xy[0], (unsigned int)target_xy[1], 3.0f) ||
        entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 2u;
    entity->pos_pair_2f4 = (float)target_xy[0];
    entity->pos_pair_2f8 = (float)target_xy[1];
    return 1;
}

// CellStepCommandIssueWithTargetMark
// FUNCTION: LEGORR 0x004492b0
int ledger_stub_004492b0_CellStepCommandIssueWithTargetMark(
    EntitySlot* entity, const int target_xy[2])
{
    LevelState* level;
    int result;
    if (entity == NULL || target_xy == NULL || entity->owner_ctx == NULL)
        return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || level_cell_speed_class(level, target_xy[0],
                                                target_xy[1]) == 0)
        return 1;
    if ((entity_state_flags_get(entity) & 0x10000u) == 0u) return 1;
    result = entity_route_path_install(entity, level, target_xy[0],
                                       target_xy[1], NULL);
    if (!result || entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 4u;
    if (entity->type == 1) {
        level_cell_flag_80000_set(level, target_xy[0], target_xy[1], 1);
        level_cell_flag_10000000_set(level, target_xy[0], target_xy[1], 1);
    }
    return 1;
}
} // namespace unported_ledger_stubs

/* SceneObjectHasTaskType4_5_7 (FUN_004493e0) — see ai.hpp. */
// FUNCTION: LEGORR 0x004493e0
int scene_object_has_task_type_4_5_7(const EntitySlot* e)
{
    if (e == NULL || e->type != 2) {
        return 0; /* deviation: the corpus dereferences blindly */
    }
    if (ai_activity_scan(e, 4) != 0) {
        return 1;
    }
    if (ai_activity_scan(e, 5) != 0) {
        return 1;
    }
    if (ai_activity_scan(e, 7) != 0) {
        return 1;
    }
    return 0;
}

namespace unported_ledger_stubs {
// EntityTargetCellRouteAndAssociate
// FUNCTION: LEGORR 0x00449430
int ledger_stub_00449430_EntityTargetCellRouteAndAssociate(
    EntitySlot* entity, EntitySlot* target)
{
    int cell[2];
    LevelState* level;
    if (entity == NULL || target == NULL || entity->owner_ctx == NULL)
        return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    cell[0] = -1;
    cell[1] = -1;
    if (!entity_route_live_position_to_cell(target, level, &cell[0],
                                             &cell[1])) return 0;
    if (cell[0] < 0 || cell[1] < 0 ||
        !entity_route_path_install(entity, level, cell[0], cell[1], NULL) ||
        entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 0x11u;
    entity->link_2fc = target;
    return 1;
}

// EntityTargetCellRouteMarker14
// FUNCTION: LEGORR 0x00449520
int ledger_stub_00449520_EntityTargetCellRouteMarker14(
    EntitySlot* entity, EntitySlot* target)
{
    int cell[2];
    LevelState* level;
    if (entity == NULL || target == NULL || entity->owner_ctx == NULL)
        return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    entity_position_to_terrain_cell(target, level, &cell[0], &cell[1]);
    return entity_route_to_cell_commit(entity, cell[0], cell[1], 0x14u);
}

// FUNCTION: LEGORR 0x00449610
int ledger_stub_00449610_EntityCoordinateRouteAndMark(
    EntitySlot* entity, const int target_xy[2])
{
    if (entity == NULL || target_xy == NULL || entity->owner_ctx == NULL ||
        entity->owner_ctx->level == NULL) return 0;
    if (!entity_route_path_install(entity, entity->owner_ctx->level,
                                   target_xy[0], target_xy[1], NULL) ||
        entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 0x10u;
    entity->pos_pair_2f4 = (float)target_xy[0];
    entity->pos_pair_2f8 = (float)target_xy[1];
    return 1;
}

// RouteStepCommandIssue
// FUNCTION: LEGORR 0x00449700
int ledger_stub_00449700_RouteStepCommandIssue(EntitySlot* entity)
{
    LevelState* level;
    int destination[2];
    int found;
    if (entity == NULL || entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    if ((entity->flags_3e8 & 0x00100000u) != 0u) {
        found = ledger_stub_0044a4f0_NearestCellCoordinateWorldResolve(
            entity, destination);
        if (!found || !entity_route_path_install(entity, level,
                destination[0], destination[1], NULL) || entity->wp_count <= 0 ||
            (entity->wp_dynamic == NULL
                ? entity->wp_count > AI_WAYPOINTS_MAX
                : (unsigned int)entity->wp_count > entity->wp_capacity))
            return 0;
        ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 10u;
    }
    else {
        found = ledger_stub_0044a5d0_NearestOpenCellCoordSearch(
            entity, &destination[0], &destination[1], 1, 0, 1);
        if (!found || !ledger_stub_00449880_BlockedCellApproachWeightWrite(
                entity, (unsigned int)destination[0],
                (unsigned int)destination[1], 3.0f) ||
            entity->wp_count <= 0 ||
            (entity->wp_dynamic == NULL
                ? entity->wp_count > AI_WAYPOINTS_MAX
                : (unsigned int)entity->wp_count > entity->wp_capacity))
            return 0;
        ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 10u;
    }
    entity->pos_pair_2f4 = (float)destination[0];
    entity->pos_pair_2f8 = (float)destination[1];
    argument_callback_dispatch(entity);
    return 1;
}

// BlockedCellApproachWeightWrite
// FUNCTION: LEGORR 0x00449880
int ledger_stub_00449880_BlockedCellApproachWeightWrite(
    EntitySlot* entity, unsigned int cell_x, unsigned int cell_y, float amount)
{
    LevelState* level;
    unsigned int direction;
    float scaled;
    AiWaypoint* step;
    if (entity == NULL || entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || level->ptr_terrain == NULL ||
        !level_cell_walkable_flag(level, (int)cell_x, (int)cell_y)) return 0;
    if (!ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
            entity, cell_x, cell_y, 0, &direction, 1)) return 0;
    if (entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    scaled = amount / terrain_context_scale_get(level->ptr_terrain);
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    step->weight_x = 0.5f;
    step->weight_y = 0.5f;
    switch (direction & 3u) {
    case 0u: step->weight_x = scaled; break;
    case 1u: step->weight_y = scaled; break;
    case 2u: step->weight_x = 1.0f - scaled; break;
    case 3u: step->weight_y = 1.0f - scaled; break;
    }
    step->pending = (uint8_t)(step->pending | 8u);
    return 1;
}
} // namespace unported_ledger_stubs

/* EntityActivityTerrainEligibilityTransition (FUN_00449980) — see ai.hpp.
 * The corpus's two coordinate arguments arrive through the x87 stack and
 * land as __ftol'd integers; use the CRT helper so non-x86 builds retain
 * the target rounding behavior. Deviations: guarded entity/level words where the corpus
 * strides blindly, and the ctx threading (port convention). */
// FUNCTION: LEGORR 0x00449980
int entity_activity_terrain_eligibility_transition(GameContext* ctx,
                                                   EntitySlot* e,
                                                   float x, float y,
                                                   int* out)
{
    if (out != NULL) {
        *out = 0;
    }
    if (e == NULL || (e->flags_3e0 & 0x40u) == 0u) {
        return 0; /* deviation: the corpus strides the entity word */
    }
    const int ix = crt_float_to_long(x);
    const int iy = crt_float_to_long(y);
    const LevelState* lvl = grid_of(ctx);
    if (lvl == NULL) {
        return 0; /* deviation: the corpus strides the level word */
    }
    if (terrain_cell_allows_placement(e, lvl, ix, iy) == 0) {
        ai_set_activity(e, 11, 1);                      /* FUN_004409e0 */
        e->flags_3e0 = (e->flags_3e0 & ~0x40u) | 0x80000000u;
        level_tile_mark((LevelState*)lvl, ix, iy, 0);    /* FUN_00432e00 */
        return 1;
    }
    if (e->activity_timer <= 0.0f) {
        return 1;
    }
    e->flags_3e0 &= ~0x40u;
    ai_set_activity(e, 0, 0);                           /* FUN_004409e0 */
    terrain_cell_queued_work_transition(ctx, ix, iy);   /* FUN_00431140 */
    const float invalid = -1.0f;
    __builtin_memcpy(&e->pos_pair_2f4, &invalid, sizeof invalid);
    __builtin_memcpy(&e->pos_pair_2f8, &invalid, sizeof invalid);
    if (out != NULL) {
        *out = 1;
    }
    level_tile_mark((LevelState*)lvl, ix, iy, 0);       /* FUN_00432e00 */
    return 1;
}
/* FUN_00449a70: the type-2 half-word, then the bit-0x400-gated 0x23 pass. */
// FUNCTION: LEGORR 0x00449a70
void entity_activity23_state_transition(EntitySlot* e)
{
    if (e == NULL) {
        return; /* deviation: the corpus strides blindly */
    }
    if (e->type == 2) {
        e->flags_3e4 |= 0x800u;
    }
    if ((e->flags_3e0 & 0x400u) == 0u) {
        e->flags_3e0 |= 0x40000000u;
        ai_set_activity(e, 0x23, 0); /* FUN_004409e0; 0x447c40 folds here */
        ai_idle_chain_record_attach(e); /* FUN_00403f30 */
    }
}

namespace unported_ledger_stubs {
// ProximityStepCommandIssueClearingGate
// FUNCTION: LEGORR 0x00449ac0
int ledger_stub_00449ac0_ProximityStepCommandIssueClearingGate(
    EntitySlot* entity, EntitySlot* target)
{
    LevelState* level;
    int target_cell[2];
    unsigned int direction;
    float step_fraction;
    AiWaypoint* step;
    if (entity == NULL || target == NULL || entity->type != 2 ||
        target->type != 4 || entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    target_cell[0] = -1;
    target_cell[1] = -1;
    if (!entity_route_live_position_to_cell(target, level, &target_cell[0],
                                             &target_cell[1])) return 0;
    if (!ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
            entity, (unsigned int)target_cell[0],
            (unsigned int)target_cell[1], 0, &direction, 1)) {
        entity->flags_3e4 &= ~0x00000800u;
        return 0;
    }
    if (entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    step_fraction = 4.0f / terrain_context_scale_get(level->ptr_terrain);
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    switch (direction & 3u) {
    case 0u: step->weight_y = 0.5f;
             step->weight_x = 1.0f - step_fraction; break;
    case 1u: step->weight_y = 1.0f - step_fraction;
             step->weight_x = 0.5f; break;
    case 2u: step->weight_y = 0.5f;
             step->weight_x = step_fraction; break;
    default: step->weight_y = step_fraction;
             step->weight_x = 0.5f; break;
    }
    step->step_kind = 0x0bu;
    step->pending = (uint8_t)(step->pending | 4u | (direction & 3u));
    entity->link_2fc = target;
    return 1;
}
} // namespace unported_ledger_stubs

/* FUN_00449c00: a single ordered render step. */
// FUNCTION: LEGORR 0x00449c00
void simtick_render_step(EngineContext* c)
{
    (void)c;
    entity_pool_find_by_callback_current(render_entity_callback, NULL, 0);
    ++s_post_n;
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x00449cd0
void ledger_stub_00449cd0_SceneObjectStateHandleApplyValue(
    void* entity_record, int set)
{
    void* component = entity_type_component_get(entity_record);
    if (component != NULL) entity_flag_80_set(component, set);
}

// EntityTypeOperationDispatch
// FUNCTION: LEGORR 0x00449f50
void ledger_stub_00449f50_EntityTypeOperationDispatch(
    EntitySlot* entity, uint32_t operation)
{
    if (entity == NULL) return;
    switch (entity->type) {
    case 1:
        if (entity->attached_direction_record_type1 != NULL)
            scene_resource_visibility_set(
                entity->attached_direction_record_type1, operation);
        return;
    case 2:
        if (entity->attached_direction_record_type2 != NULL)
            animation_object_visibility_set(
                entity->attached_direction_record_type2, operation);
        return;
    case 3:
        if (entity->attached_direction_record_type3 != NULL)
            animation_object_visibility_set(
                entity->attached_direction_record_type3, operation);
        return;
    case 4:
        if (entity->attached_direction_record_type4 != NULL)
            scene_object_pair_apply_mode(
                entity->attached_direction_record_type4, operation);
        return;
    case 10:
        if (entity->attached_direction_record_type10 != NULL)
            animation_object_visibility_set(
                entity->attached_direction_record_type10, operation);
        return;
    default:
        if ((entity->flags_3e8 & 0x20000u) != 0u &&
            entity->type_operation_fallback_1c != NULL)
            scene_render_lock_toggle(entity->type_operation_fallback_1c,
                                     (int)operation);
        return;
    }
}
} // namespace unported_ledger_stubs

/* EntityTypeOperationRelease (FUN_00449ff0) returns the type-specific low
 * flag bit used by the outline caller despite its decompiler void label.
 * Kinds 2/3 own their renderer scene in the activity sidecar when the legacy
 * animation-record wrapper is unavailable. */
// FUNCTION: LEGORR 0x00449FF0
uint32_t entity_type_operation_release(const EntitySlot* entity)
{
    if (entity == NULL) return 0u;
    switch (entity->type) {
    case 1:
        return entity->attached_direction_record_type1 != NULL
            ? entity_type_flag_bit1_get(
                entity->attached_direction_record_type1) : 0u;
    case 2:
        if (entity->attached_direction_record_type2 != NULL)
            return animation_object_visibility_get(
                entity->attached_direction_record_type2);
        return entity->activity_runner_type2.child_scene != NULL
            ? scene_object_flag_bit3_get(
                entity->activity_runner_type2.child_scene) : 0u;
    case 3:
        if (entity->attached_direction_record_type3 != NULL)
            return animation_object_visibility_get(
                entity->attached_direction_record_type3);
        return entity->activity_runner_type3.child_scene != NULL
            ? scene_object_flag_bit3_get(
                entity->activity_runner_type3.child_scene) : 0u;
    case 4:
        return entity->attached_direction_record_type4 != NULL
            ? animation_object_visibility_get(
                entity->attached_direction_record_type4) : 0u;
    case 10:
        return entity->attached_direction_record_type10 != NULL
            ? animation_object_visibility_get(
                entity->attached_direction_record_type10) : 0u;
    default:
        if ((entity->flags_3e8 & 0x20000u) == 0u ||
            entity->type_operation_fallback_1c == NULL) return 0u;
        return scene_object_flag_bit3_get(
            entity->type_operation_fallback_1c);
    }
}

/* EntityFollowTargetPoseGet (FUN_0044a070): resolve the indexed scene handle
 * for entity kinds 1-3, read its world position and facing, normalize the
 * facing vector, and reverse it when the type-specific component gate is 1. */
// FUNCTION: LEGORR 0x0044a070
void entity_follow_target_pose_get(EntitySlot* entity, int index,
                                   float position[3], float facing[3])
{
    void* scene_object = NULL;
    uint32_t component_present = 0u;
    float secondary_facing[3];
    float inverse_length;
    if (entity == NULL || position == NULL || facing == NULL) return;
    if (entity->type == 1) {
        scene_object = entity_type1_scene_resource_indexed_get(
            entity, (uint32_t)index, &component_present);
    }
    else if (entity->type == 2 || entity->type == 3) {
        EntityActivityRunnerSidecar* runner;
        scene_object = entity_activity_runner_indexed_get(
            entity, entity->type, (uint32_t)index);
        runner = entity->type == 2 ? &entity->activity_runner_type2
                                   : &entity->activity_runner_type3;
        component_present = runner->component_gate_value == 1u ? 1u : 0u;
    }
    else {
        return;
    }
    if (scene_object == NULL) return;
    (void)ddiscene_object_position_get(scene_object, NULL, position);
    (void)ddiscene_object_facing_pair_get(scene_object, NULL, facing,
                                         secondary_facing);
    inverse_length = 1.0f / sqrtf(facing[0] * facing[0] +
                                 facing[1] * facing[1] +
                                 facing[2] * facing[2]);
    facing[0] = inverse_length * facing[0];
    facing[1] = inverse_length * facing[1];
    facing[2] = inverse_length * facing[2];
    if (component_present != 0u) {
        facing[0] = -facing[0];
        facing[1] = -facing[1];
        facing[2] = -facing[2];
    }
}
/* EntityTypeComponentGet (FUN_0044a200): dispatch through the legacy record's
 * payload slots. This mapped leaf accepts that corpus-layout record directly;
 * the higher-level EntitySlot sidecar is intentionally not treated as a
 * binary-compatible record. */
// FUNCTION: LEGORR 0x0044a200
void* entity_type_component_get(const void* entity_record)
{
    const uint32_t* words = (const uint32_t*)entity_record;
    switch (words[0]) {
    case 1:
        return (void*)(uintptr_t)scene_object_resource_get(
            (const void*)(uintptr_t)words[3]);
    case 2:
        return (void*)(uintptr_t)animation_object_handle_get(
            (const void*)(uintptr_t)words[4]);
    case 3:
        return (void*)(uintptr_t)animation_object_handle_get(
            (const void*)(uintptr_t)words[5]);
    case 4:
        return (void*)(uintptr_t)animation_object_handle_get(
            (const void*)(uintptr_t)words[6]);
    case 10:
        return (void*)(uintptr_t)animation_object_handle_get(
            (const void*)(uintptr_t)words[8]);
    default:
        if ((words[0xfa] & 0x00020000u) != 0u) {
            return (void*)(uintptr_t)words[7];
        }
        return (void*)entity_record;
    }
}

/* EntityWorkObjectPositionGet (FUN_0044a2a0) resolves the type-specific
 * work-object record before asking the scene layer for its position. The
 * sidecar record pointers correspond to the retail entity fields +0x0c,
 * +0x10, and +0x14 used by types 1, 2, and 3. */
// FUNCTION: LEGORR 0x0044a2a0
int entity_work_object_position_get(const EntitySlot* entity,
                                    float out_xy[2])
{
    void* work_object = NULL;
    float position[3];
    if (entity == NULL || out_xy == NULL) return 0;
    switch (entity->type) {
    case 1:
        if (entity->attached_direction_record_type1 != NULL) {
            work_object = scene_resource_cached_slot_b4_get(
                (SceneResourceAnimationRecord*)
                    entity->attached_direction_record_type1);
        }
        break;
    case 2:
    case 3:
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
        /* FUN_0044a2a0 resolves the configured DrillNullName subobject,
         * ordinal zero. Missing markers fail; the actor root is not a tool. */
        work_object = actor_scene_named_node((EntitySlot*)entity,
                                             "DrillNullName", 0u);
#else
        if (entity->type == 2 && entity->attached_direction_record_type2 != NULL)
            work_object = renderer_cached_handle_40_get_or_create(
                entity->attached_direction_record_type2);
        else if (entity->type == 3 && entity->attached_direction_record_type3 != NULL)
            work_object = renderer_cached_handle_40_get_or_create(
                entity->attached_direction_record_type3);
#endif
        break;
    default:
        return 0;
    }
    if (work_object == NULL ||
        !ddiscene_object_position_get(work_object, NULL, position)) return 0;
    out_xy[0] = position[0];
    out_xy[1] = position[1];
    return 1;
}

namespace unported_ledger_stubs {
// ObjectMovementInputAccumulate
// FUNCTION: LEGORR 0x0044a310
void ledger_stub_0044a310_ObjectMovementInputAccumulate(EntitySlot* object,
    int input_y, int input_x, float input_z)
{
    entity_motion_input_accumulate(object, input_y, input_x, input_z);
}

// Type11CellCoordinateAppend
// FUNCTION: LEGORR 0x0044a390
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void type11_cell_coordinate_append(const int* coordinate)
{
    __asm {
        mov ecx, dword ptr [s_type11_cell_coordinate_count]
        cmp ecx, 0ah
        jae type11_append_return
        mov eax, dword ptr [esp + 4]
        mov edx, dword ptr [eax]
        mov dword ptr [ecx*8 + s_type11_cell_coordinates], edx
        mov ecx, dword ptr [s_type11_cell_coordinate_count]
        mov eax, dword ptr [eax + 4]
        mov dword ptr [ecx*8 + s_type11_cell_coordinates + 4], eax
        mov eax, dword ptr [s_type11_cell_coordinate_count]
        inc eax
        mov dword ptr [s_type11_cell_coordinate_count], eax
    type11_append_return:
        ret
    }
}
#else
void type11_cell_coordinate_append(const int* coordinate)
{
    volatile unsigned int* count = &s_type11_cell_coordinate_count;
    volatile int* coordinates = (volatile int*)s_type11_cell_coordinates;
    volatile const int* input = coordinate;
    const unsigned int first = *count;
    unsigned int second;
    if (first >= 10u) return;
    coordinates[first * 2u] = input[0];
    second = *count;
    coordinates[second * 2u + 1u] = input[1];
    ++*count;
}
#endif

// PlaceCellNearestResolve
// FUNCTION: LEGORR 0x0044a3d0
int type11_cell_coordinate_nearest_resolve(const EntitySlot* target,
                                                 int cell_xy_out[2])
{
    float target_xy[2];
    float best_distance = 10000.0f;
    unsigned int i;
    unsigned int best_index = 0xffffffffu;
    if (target == NULL) return 0;
    entity_transform_position_get_outputs(target, &target_xy[0],
                                          &target_xy[1]);
    for (i = 0; i < s_type11_cell_coordinate_count; ++i) {
        float world[3];
        float dx;
        float dy;
        float distance;
        int x = s_type11_cell_coordinates[i][0];
        int y = s_type11_cell_coordinates[i][1];
        if (!terrain_cell_world_position_get(x, y, world)) continue;
        dy = target_xy[1] - world[1];
        dx = target_xy[0] - world[0];
        distance = sqrtf(dy * dy + dx * dx);
        if (distance < best_distance) {
            best_distance = distance;
            best_index = i;
        }
    }
    if (best_index == 0xffffffffu) return 0;
    if (cell_xy_out != NULL) {
        cell_xy_out[0] = s_type11_cell_coordinates[best_index][0];
        cell_xy_out[1] = s_type11_cell_coordinates[best_index][1];
    }
    return 1;
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x0044A4B0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void predug_coordinate_pool_append(const int cell_xy[2])
{
    __asm {
        mov ecx, dword ptr [s_predug_coordinate_count]
        cmp ecx, 14h
        jae predug_append_return
        mov eax, dword ptr [esp + 4]
        mov edx, dword ptr [eax]
        mov dword ptr [ecx*8 + s_predug_coordinates], edx
        mov ecx, dword ptr [s_predug_coordinate_count]
        mov eax, dword ptr [eax + 4]
        mov dword ptr [ecx*8 + s_predug_coordinates + 4], eax
        mov eax, dword ptr [s_predug_coordinate_count]
        inc eax
        mov dword ptr [s_predug_coordinate_count], eax
    predug_append_return:
        ret
    }
}
#else
void predug_coordinate_pool_append(const int cell_xy[2])
{
    volatile unsigned int* count = &s_predug_coordinate_count;
    unsigned int slot = *count;
    if (slot < 20u) {
        s_predug_coordinates[slot][0] = cell_xy[0];
        slot = *count;
        s_predug_coordinates[slot][1] = cell_xy[1];
        *count = slot + 1u;
    }
}
#endif

namespace unported_ledger_stubs {
// NearestCellCoordinateWorldResolve
// FUNCTION: LEGORR 0x0044a4f0
int ledger_stub_0044a4f0_NearestCellCoordinateWorldResolve(
    const EntitySlot* target, int cell_xy_out[2])
{
    float target_xy[2];
    float best_distance = 10000.0f;
    const int* coordinate = &s_predug_coordinates[0][0];
    unsigned int i;
    unsigned int best_index = 0xffffffffu;
    unsigned int count = s_predug_coordinate_count;
    if (target == NULL) return 0;
    entity_transform_position_get_outputs(target, &target_xy[0],
                                          &target_xy[1]);
    if (count != 0u) {
        i = 0u;
        do {
            float world_x;
            float world_y;
            float dx;
            float dy;
            float distance;
            if (map_cell_to_world_center(level_current_terrain_get(
                    coordinate[0], coordinate[1]),
                    (uint32_t)coordinate[0], (uint32_t)coordinate[1],
                    &world_x, &world_y)) {
                dy = target_xy[1] - world_y;
                dx = target_xy[0] - world_x;
                distance = sqrtf(dy * dy + dx * dx);
                if (distance < best_distance) {
                    best_distance = distance;
                    best_index = i;
                }
            }
            ++i;
            coordinate += 2;
        } while (i < count);
    }
    if (best_index == 0xffffffffu) return 0;
    if (cell_xy_out != NULL) {
        cell_xy_out[0] = s_predug_coordinates[best_index][0];
        cell_xy_out[1] = s_predug_coordinates[best_index][1];
    }
    return 1;
}

// NearestOpenCellCoordSearch
// FUNCTION: LEGORR 0x0044a5d0
int ledger_stub_0044a5d0_NearestOpenCellCoordSearch(
    EntitySlot* entity, int* output_x, int* output_y,
    int require_minimum_distance, int allow_maskc0, int allow_flag20)
{
    NearestOpenCellCandidate candidates[100];
    LevelState* level;
    float entity_position[2];
    int center_x = 0, center_y = 0;
    int x, y;
    unsigned int count = 0u;
    float minimum_distance;
    if (entity == NULL || entity->owner_ctx == NULL ||
        output_x == NULL || output_y == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    entity_transform_position_get(entity, entity_position);
    if (!entity_route_live_position_to_cell(entity, level, &center_x,
                                            &center_y)) return 0;
    minimum_distance = terrain_context_scale_get(level->ptr_terrain);
    for (y = center_y - 10; y < center_y + 10; ++y) {
        for (x = center_x - 10; x < center_x + 10; ++x) {
            float cell_position[2];
            float dx, dy, distance;
            if (!level_cell_walkable_flag(level, x, y) ||
                (allow_flag20 == 0 &&
                 level_cell_flag_20_bounded(level, x, y) != 0u) ||
                (allow_maskc0 == 0 && level_cell_mask_c0(level, x, y) != 0u)) {
                continue;
            }
            if (!terrain_cell_world_center_xy_get(level->ptr_terrain,
                                                  x, y, cell_position)) {
                continue;
            }
            dx = cell_position[0] - entity_position[0];
            dy = cell_position[1] - entity_position[1];
            distance = sqrtf(dx * dx + dy * dy);
            if (require_minimum_distance != 0 &&
                minimum_distance > distance) continue;
            if (count < 100u) {
                candidates[count].x = x;
                candidates[count].y = y;
                candidates[count].distance = distance;
                ++count;
            }
        }
    }
    if (count == 0u) return 0;
    crt_array_sort_with_comparator(candidates, (size_t)count,
                                  sizeof(candidates[0]),
                                  nearest_open_cell_candidate_compare);
    *output_x = candidates[0].x;
    *output_y = candidates[0].y;
    return 1;
}

// FUNCTION: LEGORR 0x0044a7d0
int ledger_stub_0044a7d0_PolyVariantSectionModelBuild(
    void* section, uint32_t variant_count, PolyVariantSectionBuildHook build,
    void* context)
{
    uint32_t variant;
    if (section == NULL || build == NULL) return 0;
    for (variant = 0u; variant < variant_count; ++variant)
        if (!build(context, section, variant)) return 0;
    return 1;
}

// FUNCTION: LEGORR 0x0044a910
int ledger_stub_0044a910_BuildingPlacementPreviewUpdate(
    const BuildingPlacementPreviewHooks* hooks)
{
    float position[3];
    uint32_t record_key = 0u;
    uint32_t direction = 0u;
    uint32_t footprint_count = 0u;
    const int* footprint_xy = NULL;
    TerrainSurfaceGrid* surface_grid = NULL;
    int cell_xy[2] = { 0, 0 };
    int emit_boundary_commands = 0;
    int valid;
    if (hooks == NULL || hooks->position_get == NULL ||
        hooks->placement_check == NULL || hooks->preview_set == NULL ||
        !hooks->position_get(hooks->context, position)) return 0;
    valid = hooks->placement_check(hooks->context, position);
    if (valid && hooks->path_selection_get != NULL &&
        hooks->path_selection_get(hooks->context, position, &record_key,
                                  cell_xy, &direction, &footprint_xy,
                                  &footprint_count, &surface_grid,
                                  &emit_boundary_commands) &&
        (footprint_count == 0u || footprint_xy != NULL)) {
        path_record_surface_grid_bind(surface_grid);
        (void)ledger_stub_00409790_CellPathRecordRegisterApply(
            record_key, cell_xy, direction, footprint_xy, footprint_count);
        if (emit_boundary_commands) {
            (void)ledger_stub_00408ec0_KindNineNeighborEmitForFootprintCells(
                cell_xy, footprint_xy, footprint_count);
        }
    }
    hooks->preview_set(hooks->context, position, valid);
    return valid;
}
} // namespace unported_ledger_stubs

/* TerrainObjectTypeSfxGridConfigLoad (FUN_0044acc0) — see level.hpp.
 * Deviations: the corpus strides its zero loop blindly (the port memsets
 * the window); the corpus's two dead preamble loops leave the running
 * pair initialized 20/15, but the resolver zeroes the row half at its
 * entry (0x42e8b8) — only the TYPE half carries across entries, and only
 * into skipped (`continue`) iterations; a resolved name therefore always
 * sees a fresh row (the port's resolver carries the entry zeroing); the corpus
 * strides a missing value string into the alias table (the port's lookup
 * guards NULL) and stores blind past the 300-entry window (the matching
 * build stores blind as well; the native build bounds-checks the store,
 * see the comment at the store below). The entry chain is the subgroup record followed
 * by its equal-key siblings, entry name = the object name (the corpus
 * reads the name blind at its record +4, the BlockRec key), entry value =
 * the sound alias reference. */
// FUNCTION: LEGORR 0x0044acc0
void terrain_object_sfx_grid_config_load(BlockFile* config,
                                         const char* root_path)
{
    memset(s_terrain_sfx_grid, 0, sizeof s_terrain_sfx_grid);
    int obj_x = 20;
    int init_count = 20;
    int obj_y;
    /* The corpus writes the row output on each pass through this 20-count
     * pre-loop. Keep those stores visible to VC5; final outputs remain 20/15. */
    do {
        --init_count;
        *(volatile int*)&obj_y = 15;
    } while (init_count != 0);
    const char* path = string_path_join(root_path, "ObjTtSFXs", NULL);
    for (BlockRec* entry = block_subgroup(config, path);
         entry != NULL; entry = block_next_equal_key(entry)) {
        /* The corpus loads the name blind (mov ecx,[esi+4]), the record
         * key. The chain already proved entry non-NULL. */
        const char* name = entry->key;
        if (entity_type_row_for_name(name, &obj_x,
                                     &obj_y) == 0) {
            continue;
        }
        int sfx = 0;
        /* The entry chain proves non-NULL; read its value blind. */
        const char* value = entry->value;
        if (sound_alias_lookup(value, &sfx) == 0) {
            continue;
        }
        const int cell = obj_y + obj_x * 15;
        /* The out-of-window store (#521, finding 548).
         *
         * What the original does: LegoRR.exe stores `sfx` at
         * `s_terrain_sfx_grid[cell]` with no bounds check. Nothing keeps
         * `cell` inside the 300-entry window: an object row whose type or
         * row index falls outside the 20 x 15 grid writes before or after
         * it. That missing check is the original game's own behaviour, kept
         * on purpose for faithfulness. It is not a bug introduced by the
         * port.
         *
         * Why the matching build keeps it: the MSVC 5.0 matching build must
         * compile this function (0x0044acc0) to the same bytes as the
         * original. A compare-and-branch here would add instructions and
         * break that match. So when the matching compiler sees this code,
         * the guard below is preprocessed out and the store stays as blind
         * as the original's. The token stream and object code of the
         * matching build are unchanged by this guard.
         *
         * What the stray store hits: the native `level` test's data
         * (test_level.cpp) reaches `cell == -15`, a write 60 bytes before
         * `s_terrain_sfx_grid`.
         * - On the data layout before GAME-023's relocation, that address
         *   held nothing that mattered, and the write was harmless.
         * - With GAME-023 (entity.cpp's original unit) relocated, the native
         *   layout changes and the write lands on `s_pool_chunks[28]`.
         *   `pooled_record_storage_clear` later passes that value to
         *   free(), and the `level` test crashes. A hardware watchpoint on
         *   the slot found the store.
         * Which object sits next to the grid depends on the compiler,
         * linker and source order, so on the native build the stray write
         * corrupts whatever is there.
         *
         * Why the guard is native-only: the native port (and the MinGW
         * reference build, which shares this branch) has no byte-for-byte
         * contract, and its data layout is not the original's. There the
         * blind store is memory corruption with no faithful result to
         * preserve, so an out-of-window cell is dropped instead of written.
         * Cells inside the window are stored exactly as before, so
         * well-formed data behaves the same in every build. The owner chose
         * this over changing `test_level`'s data or padding the layout
         * (#521, option 1). The condition is the one the codebase already
         * uses to choose the matching (MSVC x86) path, for example in
         * terrain_object_type_sfx_at_cell_get (0x0044ad80). */
#if !(defined(_MSC_VER) && defined(_M_IX86))
        if (cell < 0 || cell >= TERRAIN_SFX_GRID_CELLS) {
            continue; /* native only: drop the original's out-of-window store */
        }
#endif
        s_terrain_sfx_grid[cell] = sfx; /* the corpus's blind scale store */
    }
}
// FUNCTION: LEGORR 0x0044ad80
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int terrain_object_type_sfx_at_cell_get(
    const void* position_record)
{
    __asm {
        push ecx
        mov edx, dword ptr [esp + 8]
        lea eax, dword ptr [esp]
        lea ecx, dword ptr [esp + 8]
        push eax
        push ecx
        push edx
        call copy_two_dword_pair_to_outputs
        mov eax, dword ptr [esp + 14h]
        mov ecx, dword ptr [esp + 0ch]
        add esp, 0ch
        lea eax, dword ptr [eax + eax*2]
        lea edx, dword ptr [ecx*4]
        lea eax, dword ptr [eax + eax*4]
        mov eax, dword ptr [edx + eax*4 + s_terrain_sfx_grid]
        pop ecx
        ret
    }
}
#else
int terrain_object_type_sfx_at_cell_get(const void* position_record)
{
    uint32_t x = 0, y = 0;
    if (position_record == NULL) {
        return 0; /* deviation: the corpus strides blindly */
    }
    __builtin_memcpy(&x, (const char*)position_record, 4);
    __builtin_memcpy(&y, (const char*)position_record + 4, 4);
    const int cell = (int)(y + x * 15u); /* corpus's second-word-first index */
    if (cell < 0 || cell >= TERRAIN_SFX_GRID_CELLS) {
        return 0;
    }
    return s_terrain_sfx_grid[cell];
}
#endif

namespace lego {
// FUNCTION: LEGORR 0x0044adc0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void scatter_spawn_gate_set(int enabled)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        test eax, eax
        mov eax, dword ptr [s_entity_state_flags]
        je scatter_spawn_gate_clear
        or al, 20h
        mov dword ptr [s_entity_state_flags], eax
        ret
    scatter_spawn_gate_clear:
        and al, 0dfh
        mov dword ptr [s_entity_state_flags], eax
        ret
        nop
        nop
        nop
    }
}
#else
void scatter_spawn_gate_set(int enabled)
{
    if (enabled != 0) s_entity_state_flags |= 0x20u;
    else s_entity_state_flags &= ~0x20u;
}
#endif

// FUNCTION: LEGORR 0x0044ade0
void entity_kind_one_random_spawn_trigger(void* entity)
{
    EntitySlot* source = (EntitySlot*)entity;
    EntitySlot* spawn_owner;
    uint32_t count;
    if (source == NULL || scatter_spawn_gate_get() || source->type != 1 ||
        (source->flags_3ec & 0x00000800u) != 0u) return;
    if (s_trigger_hook != NULL) {
        s_trigger_hook(entity, s_hook_context);
        source->flags_3ec |= 0x00000800u;
        return;
    }
    spawn_owner = source;
    if ((source->flags_3ec & 0x00000040u) != 0u &&
        source->link_2fc != NULL) {
        spawn_owner = source->link_2fc;
    }
    count = stats_table_word_at_b4(1, source->etype, 0);
    random_kind_six_scatter_spawn(spawn_owner, (int)count, 0);
    source->flags_3ec |= 0x00000800u;
}
// FUNCTION: LEGORR 0x0044ae50
void random_kind_six_scatter_spawn(void* entity, int count, int mode)
{
    EntitySlot* source = (EntitySlot*)entity;
    int i;
    if (source == NULL || count <= 0) return;
    if (s_scatter_hook != NULL) {
        s_scatter_hook(entity, count, mode, s_hook_context);
        return;
    }
    if (source->owner_ctx == NULL || source->owner_ctx->level == NULL) return;
    for (i = 0; i < count; ++i) {
        float base_xy[2];
        float spawn_xy[2];
        float offset_x;
        float offset_y;
        int cell_x, cell_y;
        EntitySlot* spawned;
        entity_transform_position_get(source, base_xy);
        if (mode != 0 && source->type == 4) {
            float scene_position[3];
            int have_scene_position = 0;
            if (s_scene_position_get != NULL) {
                float scene_xy[2];
                if (s_scene_position_get(source, scene_xy,
                                         s_scene_position_context) != 0) {
                    base_xy[0] = scene_xy[0];
                    base_xy[1] = scene_xy[1];
                }
            }
            else {
                void* scene_object = scene_object_secondary_handle_get_or_create(
                    source->attached_direction_record_type4);
                have_scene_position = ddiscene_object_position_get(
                    scene_object, NULL, scene_position);
                if (have_scene_position) {
                    base_xy[0] = scene_position[0];
                    base_xy[1] = scene_position[1];
                }
            }
        }
        offset_x = random_float_range(-5.0f, 5.0f);
        offset_y = random_float_range(-5.0f, 5.0f);
        spawn_xy[0] = base_xy[0];
        spawn_xy[1] = base_xy[1];
        if (terrain_world_xy_to_cell(source->owner_ctx->level->ptr_terrain,
                                     base_xy[0] + offset_x,
                                     base_xy[1] + offset_y,
                                     &cell_x, &cell_y) != 0 &&
            level_cell_flag_8_bounded(source->owner_ctx->level,
                                      cell_x, cell_y) != 0u) {
            spawn_xy[0] += offset_x;
            spawn_xy[1] += offset_y;
        }
        spawned = entity_create_with_position(source->owner_ctx, 6, NULL, 0,
                                               spawn_xy[0], spawn_xy[1], 0.0f);
        if (spawned != NULL) cmd_create_task(spawned);
    }
}
} // namespace lego

namespace unported_ledger_stubs {
// EntityAttachedSpawnRecordProcess
// FUNCTION: LEGORR 0x0044af90
void ledger_stub_0044af90_EntityAttachedSpawnRecordProcess(EntitySlot* owner)
{
    if (owner != NULL && owner->carry_ref_348 != NULL) {
        const uint32_t count = *(const uint32_t*)owner->carry_ref_348;
        lego::random_kind_six_scatter_spawn(owner, (int)count, 0);
    }
}

// ConfiguredTinyMonsterSpawnAndTeardown
// FUNCTION: LEGORR 0x0044afb0
void ledger_stub_0044afb0_ConfiguredTinyMonsterSpawnAndTeardown(
    EntitySlot* source)
{
    static const char* const configured_names[3] = {
        "RockMonster", "Lavamonster", "Icemonster"
    };
    int resolved_type, resolved_row, source_type, source_row;
    int selected = -1, i;
    const char* replacement;
    float position[2];
    EntitySlot* monster = NULL;
    unsigned int successes = 0u;
    if (source == NULL || source->owner_ctx == NULL) return;
    if (source->type_name[0] == '\0' ||
        !entity_type_row_for_name(source->type_name, &source_type,
                                  &source_row)) return;
    for (i = 0; i < 3; ++i) {
        if (entity_type_row_for_name(configured_names[i], &resolved_type,
                                     &resolved_row) &&
            source_type == resolved_type && source_row == resolved_row) {
            selected = i;
            break;
        }
    }
    if (selected < 0) return;
    replacement = selected == 2 ? "TinyIM" : "TinyRM";
    if (!entity_type_row_for_name(replacement, &resolved_type, &resolved_row))
        return;
    entity_transform_position_get(source, position);
    while (successes < 3u) {
        monster = entity_create_with_position(
            source->owner_ctx, resolved_type, replacement, 0,
            position[0], position[1],
            (float)((int)(int16_t)level_rand_u16() % 7));
        if (monster == NULL) return;
        monster->field_340 = -1.0f;
        if (!ledger_stub_00449700_RouteStepCommandIssue(monster)) break;
        ++successes;
        source->flags_3e4 |= 0x20u;
    }
    if (successes == 3u) return;
    if (monster != NULL) (void)entity_teardown(monster);
}

// SmallSpiderCellSwarmSpawn
// FUNCTION: LEGORR 0x0044b140
void ledger_stub_0044b140_SmallSpiderCellSwarmSpawn(
    int cell_x, int cell_y, uint32_t variation)
{
    int type, row, i, count = (int)(variation & 3u) + 2;
    float vertices[12];
    GameContext* context = entity_pool_callback_context_get();
    TerrainSurfaceGrid* grid = path_record_surface_grid_get();
    if (context == NULL || context->level == NULL || grid == NULL ||
        cell_x < 0 || cell_y < 0 ||
        (uint32_t)cell_x >= grid->width ||
        (uint32_t)cell_y >= grid->height || grid->surface_vectors == NULL ||
        !entity_type_row_for_name("SmallSpider", &type, &row)) return;
    /* This target only scatters after the cell surface has been resolved. */
    (void)terrain_cell_surface_vertices_get(grid, (uint32_t)cell_x,
                                            (uint32_t)cell_y, vertices);
    (void)row;
    for (i = 0; i < count; ++i) {
        int rotation = (int)(int16_t)level_rand_u16() % 7;
        float x = random_float_range(vertices[0] + 1.0f,
                                     vertices[6] - 1.0f);
        float y = random_float_range(vertices[1] - 1.0f,
                                     vertices[7] + 1.0f);
        EntitySlot* spider = entity_create_with_position(
            context, type, "SmallSpider", 0, x, y, (float)rotation);
        if (spider != NULL) spider->flags_3e0 |= 0x10000u;
    }
}

// PairingActivityRaiseAndMutualLink
// FUNCTION: LEGORR 0x0044b250
void ledger_stub_0044b250_PairingActivityRaiseAndMutualLink(
    EntitySlot* first, EntitySlot* second)
{
    if (first == NULL || second == NULL ||
        !entity_shared_eligibility_check(second, 1) ||
        (second->flags_3e4 & 0x100u) != 0u ||
        (second->flags_3e4 & 2u) != 0u ||
        (first->flags_3e4 & 1u) != 0u ||
        entity_flags_mask107c33de_or_1c00203_any_test(first) ||
        !(entity_stats_float_ate8(first) < first->field_340)) return;
    lego::pairing_activity_prepare(second);
    entity_state_reset(second, 0, 1);
    entity_state_reset(first, 0, 1);
    ai_set_activity(first, 0x25, 1);
    ai_idle_chain_record_attach(first);
    ai_set_activity(second, 0x26, 1);
    ai_idle_chain_record_attach(second);
    first->flags_3e4 |= 1u;
    second->flags_3e4 |= 2u;
    first->pair_link_3c0 = (uintptr_t)second;
    second->pair_link_3c0 = (uintptr_t)first;
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x0044b350
static int activity_is_special(int v) /* FUN_0044b350's {4,5,7} predicate */
{
    return v == AI_ACT_DRILL || v == AI_ACT_TELEPORT || v == AI_ACT_REINFORCE;
}

/* FUN_0044b370: install one activity grant. Corpus order (the whole body,
 * see ai.hpp): cap gate, +0x3f4 = 0 (mirrored as
 * EntitySlot.activity_grant_state_3f4), +0x3ec bit 0x4 refresh, special-entry
 * replace, FIFO append/overwrite. The corpus compares count != cap with
 * cap = FUN_0046a060(ent) = *(state+0xcc), reading the bound Stats row
 * when present and the portable minion/context capacity otherwise. */
// FUNCTION: LEGORR 0x0044b370
void ai_activity_grant(GameContext* ctx, EntitySlot* e, int value)
{
    if (e == NULL) {
        return;
    }
    e->activity_grant_state_3f4 = 0u;
    int cap = ai_activity_cap_for_context(ctx, e);
    if (cap <= 0) {
        return; /* cap-0 entity: the corpus's grant runs nothing (0x44b38d) */
    }
    if (e->act_count > cap) {
        e->act_count = cap; /* guarded port mirror of corpus count/cap */
    }
    const int special = activity_is_special(value);
    if (ai_activity_probe(e) != 0 || special) {
        e->flags_3ec |= ENT_FLAG3EC_ACTIVITY_OK;   /* 0x44b3bf: or $0x4 */
    } else {
        e->flags_3ec &= ~ENT_FLAG3EC_ACTIVITY_OK;  /* 0x44b39c: and $0xfb */
    }
    if (special) {
        /* A special grant replaces the first special entry on the list —
         * the old grant is cleared in place (write at 0x44b463) and the
         * setter returns without touching count. */
        for (int i = 0; i < e->act_count; ++i) {
            if (activity_is_special(e->act_list[i])) {
                e->act_list[i] = value;
                return;
            }
        }
    }
    if (e->act_count != cap) {
        e->act_list[e->act_count++] = value;       /* 0x44b44a: append */
        return;
    }
    /* Full list: drop the oldest (left shift, 0x44b417) and write the last
     * slot (count is kept — the corpus stays at cap). */
    for (int i = 0; i + 1 < e->act_count; ++i) {
        e->act_list[i] = e->act_list[i + 1];
    }
    e->act_list[e->act_count - 1] = value;
}
/* FUN_0044b480: scan the id slots at +0x370 through the +0x384 count;
 * a zero count answers 0 without reading. */
// FUNCTION: LEGORR 0x0044b480
int ai_activity_scan(const EntitySlot* e, int activity)
{
    if (e == NULL || e->act_count <= 0) {
        return 0;
    }
    for (int i = 0; i < e->act_count; ++i) {
        if (e->act_list[i] == activity) {
            return 1;
        }
    }
    return 0;
}

namespace unported_ledger_stubs {
// TypeTwoEntityTargetResolveAndApply
// FUNCTION: LEGORR 0x0044b4c0
int ledger_stub_0044b4c0_TypeTwoEntityTargetResolveAndApply(
    EntitySlot* entity, uint32_t code)
{
    int group;
    /* The reference treats an unmapped code, and every non-minifigure,
     * permissively. A mapped code is a permitted-activity group id. */
    if (entity == NULL || entity->type != 2) return 1;
    if (!entity_grouped_code_table_find_value_index(code, &group)) return 1;
    return ai_activity_scan(entity, group);
}

// ObjectPositionStepCommandIssueAndLink
// FUNCTION: LEGORR 0x0044b500
int ledger_stub_0044b500_ObjectPositionStepCommandIssueAndLink(
    EntitySlot* entity, uint32_t selector)
{
    float entity_position[2];
    float target_position[3];
    int target_xy[2] = { -1, -1 };
    EntitySlot* spatial_candidate;
    void* scene_object;
    LevelState* level;
    AiWaypoint* step;
    if (entity == NULL || entity->type != 2 || entity->owner_ctx == NULL ||
        selector >= AI_ACTIVITY_RESOURCE_SLOT_COUNT) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    if (s_entity_object_position_scene_resolve_hook == NULL) return -1;
    entity_transform_position_get(entity, entity_position);
    spatial_candidate = entity_lookup_at_world_position(entity_position);
    if (spatial_candidate == NULL || spatial_candidate->type != 4 ||
        spatial_candidate->owner_ctx != entity->owner_ctx ||
        spatial_candidate->pool_link_408 != (uintptr_t)spatial_candidate)
        return 0;
    scene_object = s_entity_object_position_scene_resolve_hook(
        s_entity_object_position_scene_resolve_context,
        spatial_candidate, ai_activity_resource_slot_get((int)selector));
    if (scene_object == NULL ||
        !ddiscene_object_position_get(scene_object, NULL, target_position) ||
        !terrain_world_xy_to_cell(level->ptr_terrain, target_position[0],
            target_position[1], &target_xy[0], &target_xy[1]) ||
        !object_path_submit_to_grid_target(entity, level, target_xy,
                                           target_position) ||
        entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    entity->link_2fc = spatial_candidate;
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    step->step_kind = 0x0du;
    return 1;
}

// FUNCTION: LEGORR 0x0044b680
int ledger_stub_0044b680_EntitySceneObjectTreeAttach(
    EntitySlot* entity, void* scene_object, void* parent,
    EntitySceneTreeAttachHook attach, void* context)
{
    if (entity == NULL || scene_object == NULL || attach == NULL) return 0;
    return attach(context, entity, scene_object, parent);
}
} // namespace unported_ledger_stubs

// MatrixApplyRotationAboutZWithOffset (FUN_0044b7a0).
// FUNCTION: LEGORR 0x0044b7a0
void matrix_apply_rotation_about_z_with_offset(void* target,
                                               float matrix[16],
                                               float offset)
{
    float original[16];
    float rotated[16];
    float old_x;
    float old_y;
    float old_z;
    int column;
    if (matrix == NULL) return;
    memcpy(original, matrix, sizeof(original));
    old_x = original[12];
    old_y = original[13];
    old_z = original[14];
    for (column = 0; column < 4; ++column) {
        int base = column * 4;
        rotated[base + 0] = -original[base + 1];
        rotated[base + 1] = original[base + 0];
        rotated[base + 2] = original[base + 2];
        rotated[base + 3] = original[base + 3];
    }
    rotated[12] = old_x;
    rotated[13] = old_z;
    rotated[14] = -old_y - offset;
    memcpy(matrix, rotated, sizeof(rotated));
    if (s_scene_matrix_apply_hook != NULL) {
        s_scene_matrix_apply_hook(s_scene_matrix_apply_context,
                                  target, matrix);
    }
}

// FUNCTION: LEGORR 0x0044bae0
void scene_object_resource_and_callback_state_dispose(void** owner)
{
    void* scene_object;
    void** resource_slot;
    void* resource;
    SceneObjectDisposeHooks* hooks = &s_scene_object_dispose_hooks;
    if (owner == NULL || *owner == NULL) return;
    if (hooks->callback_state_remove != NULL) {
        hooks->callback_state_remove(hooks->context, owner,
                                     0x0044bab0u, 0);
    }
    scene_object = *owner;
    resource_slot = (void**)((char*)scene_object + 0x98);
    resource = *resource_slot;
    if (resource != NULL) {
        if (hooks->scene_node_detach != NULL) {
            hooks->scene_node_detach(hooks->context, resource);
        }
        if (hooks->scene_node_release != NULL) {
            hooks->scene_node_release(hooks->context, resource);
        }
        *resource_slot = NULL;
    }
    if (hooks->owner_record_release != NULL) {
        hooks->owner_record_release(hooks->context, owner);
    }
}

// EntityNodeScaleUpdate (FUN_0044bb30): the accessor call sites pass an extra
// value that the one-argument cdecl readers ignore; only the final scaling
// affects the selected record value.
// FUNCTION: LEGORR 0x0044bb30
void entity_node_scale_update(void* entity_ptr, int doubled)
{
    EntitySlot* entity = (EntitySlot*)entity_ptr;
    float first;
    float second;
    if (entity == NULL || s_entity_node_scale_apply_hook == NULL) return;
    first = entity_stats_float_at128(entity);
    second = entity_stats_float_at12c(entity);
    if (doubled != 0) {
        first = entity_stats_float_at124(entity);
        first = entity_stats_float_at120(entity);
        first += first;
        second = entity_stats_float_at128(entity);
        second = entity_stats_float_at124(entity);
        second = entity_stats_float_at120(entity);
        second *= 4.0f;
    }
    else {
        first = entity_stats_float_at120(entity);
        second = entity_stats_float_at120(entity);
    }
    s_entity_node_scale_apply_hook(s_entity_node_scale_apply_context,
                                   entity, first, second);
}

namespace lego {
// FUNCTION: LEGORR 0x0044bfd0
int configured_monster_spawn_behavior_tick(EntitySlot* entity)
{
    if (entity == NULL || !(0.0f < entity->activity_timer)) return 0;
    entity->flags_3e0 &= ~0x00100000u;
    unported_ledger_stubs::
        ledger_stub_0044afb0_ConfiguredTinyMonsterSpawnAndTeardown(entity);
    unported_ledger_stubs::
        ledger_stub_0044af90_EntityAttachedSpawnRecordProcess(entity);
    entity->flags_3e8 |= 0x00800000u;
    global_counter_at_00555328_increment();
    return 1;
}
} // namespace lego

/* OneShotAttachedEffectStart (FUN_0044c030): a type-3 owner accepts this
 * only once. Default pose and child creation use the live entity/context;
 * the host can still override those operations and supply the named scene
 * action that the portable view API does not expose. */
// FUNCTION: LEGORR 0x0044c030
int entity_one_shot_attached_effect_start(EntitySlot* entity,
                                          float duration)
{
    EntityOneShotAttachedEffectHooks* const hooks =
        &s_one_shot_attached_effect_hooks;
    EntitySlot* child;
    float position[2];
    float angle;
    if (entity == NULL || entity->type != 3
        || (entity->flags_3e4 & 0x08000000u) != 0u) {
        return 0;
    }
    entity->flags_3e4 |= 0x08000000u;
    /* The retail record stores the scaled duration through an integer
     * conversion before it reaches the +0x368 word. */
    entity->wake_timer = (float)(int)(duration * 25.0f);
    if (hooks->scene_pose_get != NULL) {
        if (!hooks->scene_pose_get(hooks->context, entity, position, &angle))
            return 0;
    }
    else {
        /* FUN_00442260 / FUN_00442480 read the owner's scene position and
         * heading. The entity accessors use the attached scene when present
         * and retain the slot's position/heading as the portable fallback. */
        entity_transform_position_get(entity, position);
        angle = entity_vector_direction_angle_radians_get(entity);
    }
    if (hooks->child_create != NULL) {
        child = hooks->child_create(hooks->context, entity, 0x12,
                                    position, angle);
    }
    else if (entity->owner_ctx != NULL) {
        /* The source creates an entity of kind 0x12 using its current level
         * context. This direct path keeps the owner/child linkage live even
         * when the host has not installed a renderer-specific creator. */
        child = entity_create_with_position(entity->owner_ctx, 0x12, NULL, 0,
                                            position[0], position[1], angle);
    }
    else {
        child = NULL;
    }
    if (child == NULL) return 0;
    if (hooks->child_action_start != NULL) {
        hooks->child_action_start(hooks->context, child, "Start");
    }
    /* The retail path immediately advances the new child's scene action at
     * time zero after selecting "Start". The portable scene updater exposes
     * that same operation when the created slot has a bound scene handle. */
    if (child->type_component_handle != NULL) {
        (void)ddiscene_frame_update_and_callbacks(
            child->type_component_handle, 0.0f);
    }
    child->flags_3e0 |= 0x100u;
    entity->wake_target = child;
    child->wake_target = entity;
    return 1;
}

/* AttachedEntityWake (FUN_0044c110): resolve the parent's +0x364 attached
 * target, then apply its wake timer and wake-request flag. */
// FUNCTION: LEGORR 0x0044c110
void ai_attached_entity_wake(EntitySlot* parent)
{
    EntitySlot* target = (EntitySlot*)parent->wake_target;
    if ((target->flags_3e0 & 0x00200000u) == 0) {
        target->field_340 = 100.0f;
        target->flags_3e8 |= 0x00800000u;
    }
}

/* EntityPairScalarFieldsSet (FUN_0044c150): accept kinds one through three,
 * reset their action state, then install the pair/scalar and raise the
 * corresponding +0x3e4 state bit. */
// FUNCTION: LEGORR 0x0044c150
int entity_pair_scalar_fields_set(EntitySlot* entity, const int32_t pair[2],
                                  int32_t scalar)
{
    if (entity == NULL || pair == NULL
        || (entity->type != 1 && entity->type != 2 && entity->type != 3)) {
        return 0;
    }
    entity_state_reset(entity, 0, 1);
    entity->task_pair_3b4[0] = pair[0];
    entity->task_pair_3b4[1] = pair[1];
    entity->task_scalar_3bc = scalar;
    entity->flags_3e4 |= 0x00004000u;
    return 1;
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x0044c1b0
void ledger_stub_0044c1b0_EntityHeadingStepMove(EntitySlot* entity,
                                                 float delta)
{
    LevelState* level;
    float position[2];
    float target[2];
    float direction_x;
    float direction_y;
    float step_x;
    float step_y;
    float magnitude;
    int cell_x;
    int cell_y;
    int allowed;
    int transition;

    if (entity == NULL || entity->owner_ctx == NULL ||
        entity->owner_ctx->level == NULL ||
        entity->owner_ctx->level->ptr_terrain == NULL) return;
    level = entity->owner_ctx->level;
    entity_transform_position_get(entity, position);
    memcpy(&direction_x, &entity->task_pair_3b4[0], sizeof(direction_x));
    memcpy(&direction_y, &entity->task_pair_3b4[1], sizeof(direction_y));
    step_x = direction_x * delta;
    step_y = direction_y * delta;
    target[0] = position[0] + step_x;
    target[1] = position[1] + step_y;
    if (!terrain_world_xy_to_cell(level->ptr_terrain, target[0], target[1],
                                  &cell_x, &cell_y)) {
        allowed = 0;
    }
    else {
        allowed = terrain_cell_route_and_occupancy_check(entity, level,
            cell_x, cell_y, cell_x, cell_y, 1);
    }

    if (allowed) {
        transition = entity_flag_gated_callback_dispatch(entity,
            (uint32_t)(uintptr_t)target, 0u, 0u);
        if (transition == 0) {
            entity_world_position_set(entity, target[0], target[1], 0.0f, 0);
        }
        else {
            allowed = 0;
        }
    }
    if (!allowed) {
        magnitude = (float)sqrt((double)(direction_x * direction_x +
                                          direction_y * direction_y));
        if (magnitude != 0.0f) {
            float normalized_x = direction_x / magnitude;
            float normalized_y = direction_y / magnitude;
            if (normalized_x > 0.5f) {
                normalized_x = 1.0f;
                normalized_y = 0.0f;
            }
            else if (normalized_x < -0.5f) {
                normalized_x = -1.0f;
                normalized_y = 0.0f;
            }
            else if (normalized_y > 0.5f) {
                normalized_x = 0.0f;
                normalized_y = 1.0f;
            }
            else if (normalized_y < -0.5f) {
                normalized_x = 0.0f;
                normalized_y = -1.0f;
            }
            direction_x = normalized_x * magnitude;
            direction_y = normalized_y * magnitude;
            memcpy(&entity->task_pair_3b4[0], &direction_x,
                   sizeof(direction_x));
            memcpy(&entity->task_pair_3b4[1], &direction_y,
                   sizeof(direction_y));
        }
        step_x = direction_x * delta;
        step_y = direction_y * delta;
        target[0] = position[0] + step_x;
        target[1] = position[1] + step_y;
        if (terrain_world_xy_to_cell(level->ptr_terrain, target[0], target[1],
                                     &cell_x, &cell_y)) {
            allowed = terrain_cell_route_and_occupancy_check(entity, level,
                cell_x, cell_y, cell_x, cell_y, 1);
        }
        else {
            allowed = 0;
        }
        if (allowed) {
            transition = entity_flag_gated_callback_dispatch(entity,
                (uint32_t)(uintptr_t)target, 0u, 0u);
            if (transition == 0) {
                entity_world_position_set(entity, target[0], target[1],
                                          0.0f, 0);
            }
            else {
                entity->flags_3e4 &= ~0x4000u;
            }
        }
        else {
            entity->flags_3e4 &= ~0x4000u;
        }
    }
    {
        float remaining;
        memcpy(&remaining, &entity->task_scalar_3bc, sizeof(remaining));
        if (remaining <= 0.0f) entity->flags_3e4 &= ~0x4000u;
        remaining -= (float)sqrt((double)(step_x * step_x + step_y * step_y));
        memcpy(&entity->task_scalar_3bc, &remaining, sizeof(remaining));
    }
}
} // namespace unported_ledger_stubs

/* SelectionClearMarkerAndResetHud (FUN_0044c4a0) uses the active engine
 * state word, corresponding to DAT_00556ecc. */
// FUNCTION: LEGORR 0x0044c4a0
void selection_clear_marker_and_reset_hud(EntitySlot* e)
{
    if ((e->flags_3ec & 2u) != 0u) {
        if ((entity_state_flags_get(e) & 0x20000u) != 0u) {
            if ((entity_stats_word_get(e) & 0x20000u) == 0u
                || e->owner_36c != NULL) {
                uint32_t* state = engine_current_state_word();
                if (state != NULL) *state |= 0x10000000u;
                e->flags_3ec |= 1u;
            }
        }
    }
    selection_clear_request_and_hud_reset(); /* FUN_0041e750 */
}
/* SelectionCandidateFilter (FUN_004526e0) operates on the active selection
 * globals. The command-layer adapter binds and publishes the GameContext. */
/* The reference callback at 0x0044c500 clears the transient selection marker
 * on every filtered-pool entity and clears the engine-state dirty bit when
 * that marker was present. Its body is visible in the reference executable's
 * code at this address, though omitted from the checked-in decompile. */
// FUNCTION: LEGORR 0x0044c500
int selection_candidate_filter_pool_refresh_callback(
    EntitySlot* entity, void* payload)
{
    if ((entity->flags_3ec & 1u) != 0u) {
        uint32_t* state = engine_current_state_word();
        if (state != NULL) {
            *state &= ~0x10000000u;
        }
    }
    entity->flags_3ec &= ~1u;
    (void)payload;
    return 0;
}

/* FUN_0044c530: "can do" — only type-2 entities answer, and only from
 * their permitted-activity list. */
// FUNCTION: LEGORR 0x0044c530
int ai_can_do(const EntitySlot* e)
{
    return (e != NULL && e->type == 2) ? (ai_activity_probe(e) != 0) : 0;
}

namespace lego {
// FUNCTION: LEGORR 0x0044c550
void global_scatter_spawn_enable_and_drain(void)
{
    entity_state_flags_set(entity_state_flags_peek() | 0x40u);
    if (global_scatter_find(0) != 0) return;
    if (s_global_scatter_flag_b == 0) {
        if (global_scatter_find(1) == 0 && s_global_scatter_flag_a != 0) {
            s_global_scatter_flag_a = 0;
            s_global_scatter_flag_b = 0;
            global_scatter_find(1);
            return;
        }
    }
    else {
        s_global_scatter_flag_a = 0;
        s_global_scatter_flag_b = 0;
        global_scatter_find(0);
    }
}
} // namespace lego

static EntitySlot* entity_carried_slot_get(EntitySlot* entity, int index)
{
    if (entity == NULL || index < 0 || index >= 7) return NULL;
    return index == 0 ? (EntitySlot*)entity->carry_link_308
                      : entity->carry_extra_links[index - 1];
}

static void entity_carried_slot_set(EntitySlot* entity, int index,
                                     EntitySlot* carried)
{
    if (index == 0) entity->carry_link_308 = carried;
    else entity->carry_extra_links[index - 1] = carried;
}

static void entity_carried_links_clear(EntitySlot* entity)
{
    int index;
    for (index = 0; index < 7; ++index) {
        EntitySlot* carried = entity_carried_slot_get(entity, index);
        entity_carried_slot_set(entity, index, NULL);
        if (carried != NULL && carried->attach_304 == entity) {
            carried->attach_304 = NULL;
            carried->attached_scene_slot_index = 0;
        }
    }
    entity->carry_state_324 = 0;
}

static void entity_action_consume_counter_sidecar_release(EntitySlot* owner)
{
    EntityActionConsumeCounterSidecar** link =
        &s_entity_action_counter_sidecars;
    while (*link != NULL) {
        EntityActionConsumeCounterSidecar* entry = *link;
        if (entry->owner == owner) {
            *link = entry->next;
            engine_heap_free(entry->values);
            free(entry);
            break;
        }
        link = &entry->next;
    }
    if (owner != NULL && owner->type == 3) {
        memset(owner->action_consume_counters, 0,
               sizeof(owner->action_consume_counters));
        owner->action_consume_counters_initialized = 0u;
    }
}

static int entity_display_name_config_index(const EntitySlot* entity)
{
    uint32_t i;
    int kind;
    if (entity == NULL || entity->type < 1 ||
        (entity->type > 4 && entity->type != 10)) return -1;
    kind = entity->type;
    for (i = 0u; i < s_entity_kind_name_counts[kind]; ++i) {
        if (strcmp(entity->type_name,
                   s_entity_kind_names[kind] + (size_t)i * 128u) == 0)
            return (int)i;
    }
    return -1;
}

static void* entity_spatial_query_execute_current(
    const SpatialObjectQueryRequest* request)
{
    return entity_spatial_object_query_hook(
        entity_pool_callback_context_get(), request);
}
static int entity_route_marker_query_match(
    EntitySlot* candidate, const EntityRouteMarkerSearchRequest* request)
{
    uint32_t mask;
    uint32_t stats;
    uint32_t state;
    int matched = 0;
    int cell[2];
    if (candidate == NULL || request == NULL) return 0;
    mask = request->flags;
    if (!entity_shared_eligibility_check(candidate, (int)(mask & 0x400u)))
        return 0;
    /* The first shared-eligibility call consumes and then removes this bit
     * from the callback's later OR-of-predicates mask. */
    mask &= ~0x400u;
    if ((mask & 0x4000u) != 0u &&
        (candidate->flags_3ec & 0x10u) != 0u) return 0;
    stats = entity_stats_word_get(candidate);
    state = entity_state_flags_get(candidate);

    if (mask == 0u) {
        matched = candidate->type == 4;
    }
    else {
        if ((mask & 0x0001u) != 0u && (stats & 8u) != 0u &&
            entity_action_kind_supported(candidate, request->cell_y,
                                         request->action_argument))
            matched = 1;
        if ((mask & 0x1000u) != 0u) {
            cell[0] = request->cell_x;
            cell[1] = request->cell_y;
            if (dock_definition_matches_cell(cell)) matched = 1;
        }
        if ((mask & 0x2000u) != 0u && (state & 0x600u) == 0u &&
            entity_shared_eligibility_check(candidate, 0)) matched = 1;
        if ((mask & 0x0100u) != 0u &&
            entity_flags_contain_requested_single_bit(
                candidate, request->requested_state)) matched = 1;
        if ((mask & 0x0040u) != 0u && (stats & 8u) == 0u &&
            candidate->type == 4) matched = 1;
        if ((mask & 0x0080u) != 0u && (state & 2u) != 0u &&
            candidate->link_300 == NULL) matched = 1;
        if ((mask & 0x0800u) != 0u &&
            (stats & 0x80000000u) != 0u) matched = 1;
        if ((mask & 0x0010u) != 0u && (stats & 0x10u) != 0u)
            matched = 1;
        if ((mask & 0x0020u) != 0u && (stats & 0x20u) != 0u)
            matched = 1;
        if ((mask & 0x0200u) != 0u && (stats & 0x40u) != 0u)
            matched = 1;
        if ((mask & 0x0002u) != 0u &&
            (((stats & 2u) != 0u && request->cell_x == 7) ||
             ((stats & 4u) != 0u && request->cell_x == 6)))
            matched = 1;
        if ((mask & 0x0004u) != 0u && (stats & 0x4000u) != 0u)
            matched = 1;
        if ((mask & 0x0008u) != 0u && (stats & 0x100000u) != 0u)
            matched = 1;
    }
    return matched;
}

static int queued_grid_animation_anchor_cell_get(void* anchor,
                                                  float position[2])
{
    EntitySlot* entity = (EntitySlot*)anchor;
    int cell_x, cell_y;
    if (entity == NULL || entity->owner_ctx == NULL ||
        entity->owner_ctx->level == NULL || position == NULL) return 0;
    cell_x = level_cell_of(entity->x);
    cell_y = level_cell_of(entity->y);
    entity_position_to_terrain_cell(entity, entity->owner_ctx->level,
                                    &cell_x, &cell_y);
    position[0] = (float)cell_x;
    position[1] = (float)cell_y;
    return 1;
}

static EntitySlot* queued_grid_animation_spatial_find(
    const float position[2], int type, int filter_a, int filter_b)
{
    SpatialObjectQueryRequest request;
    float world_position[3];
    int cell_x, cell_y;
    if (position == NULL) return NULL;
    cell_x = (int)position[0];
    cell_y = (int)position[1];
    request.center[0] = 0.0f;
    request.center[1] = 0.0f;
    request.radius = 10000.0f;
    request.mask = 1u;
    request.filters[0] = (uint32_t)type;
    request.filters[1] = (uint32_t)filter_a;
    request.filters[2] = (uint32_t)filter_b;
    request.filters[3] = 0u;
    request.center_is_cell = 0;
    if (terrain_cell_world_position_get(cell_x, cell_y, world_position)) {
        request.center[0] = world_position[0];
        request.center[1] = world_position[1];
    }
    return (EntitySlot*)spatial_object_query_execute(&request);
}

static int entity_grid_position_match(EntitySlot* entity, void* payload)
{
    EntityGridFindQuery* const query = (EntityGridFindQuery*)payload;
    int cell_x = -1;
    int cell_y = -1;
    if (entity == NULL || query == NULL || query->level == NULL)
        return 0;
    entity_position_to_terrain_cell(entity, query->level, &cell_x, &cell_y);
    if (cell_x == query->x && cell_y == query->y) {
        query->found = entity;
        return 1;
    }
    return 0;
}

static EntitySlot* entity_at_grid_position_find_in_context(
    GameContext* context, LevelState* level, const int cell_xy[2])
{
    EntityGridFindQuery query;
    if (context == NULL || level == NULL || cell_xy == NULL) return NULL;
    query.x = cell_xy[0];
    query.y = cell_xy[1];
    query.level = level;
    query.found = NULL;
    (void)entity_pool_find_by_callback_filtered(
        context, entity_grid_position_match, &query);
    return query.found;
}

static int entity_pool_teardown_callback(EntitySlot* entity, void* unused)
{
    (void)unused;
    return entity_teardown(entity);
}

static int entity_spawn_request_nested_sum_add(EntitySlot* candidate,
                                                void* payload)
{
    EntitySpawnRequestNestedSum* sum =
        (EntitySpawnRequestNestedSum*)payload;
    if (candidate != NULL && candidate->type == 4 &&
        entity_shared_eligibility_check(candidate, 0) != 0) {
        sum->value += entity_stats_float_atdc(candidate);
    }
    return 0;
}

/* EntityStatePairPredicate (FUN_0043c200): the selected type/row is
 * compatible when its +0xdc value, the Barracks +0xdc value, and the
 * eligible type-4 instance sum have a positive total. */
static int entity_spawn_request_state_pair_predicate(int type, int row,
                                                     int other_type,
                                                     int other_row,
                                                     GameContext* context)
{
    EntitySpawnRequestNestedSum sum;
    float candidate_value = stats_table_float_at_dc(type, row);
    float other_value;
    if (candidate_value == 0.0f) return 1;
    sum.value = 0.0f;
    if (context != NULL) {
        (void)entity_pool_find_by_callback_filtered(
            context, entity_spawn_request_nested_sum_add, &sum);
    }
    other_value = stats_table_float_at_dc(other_type, other_row);
    return candidate_value + other_value + sum.value > 0.0f;
}

static int entity_spawn_request_pair_matches(const EntitySpawnRequest* request,
                                              const EntitySlot* requester,
                                              GameContext* context)
{
    uint32_t candidate_flags;
    uint32_t requester_flags;
    int32_t available_slots;
    int barracks_type = 0;
    int barracks_row = 0;
    if (request == NULL || requester == NULL || request->type < 0 ||
        request->type >= STATS_TYPE_TABLE_TYPES || request->kind < 0 ||
        request->kind >= STATS_TYPE_TABLE_ROWS) return 0;
    candidate_flags = stats_table_word_at_148(request->type, request->kind);
    requester_flags = entity_stats_word_get(requester);
    if (!(((candidate_flags & 0x1000u) != 0u &&
           (requester_flags & 0x10u) != 0u) ||
          ((candidate_flags & 0x800u) != 0u &&
           (requester_flags & 0x20u) != 0u) ||
          ((candidate_flags & 0x2000u) != 0u &&
           (requester_flags & 0x40u) != 0u) ||
          ((candidate_flags & 0x400000u) != 0u &&
           (requester_flags & 0x80000000u) != 0u))) return 0;

    /* FUN_0041f8e0 is a void no-op whose caller consumes the unmodified
     * return register. The port pins that residue to zero, matching the
     * established behavior in EntityAvailableResourceSpend. */
    available_slots = (int32_t)stats_table_word_at_b4(
        request->type, request->kind, 0);
    if (available_slots > 0) return 0;

    (void)entity_type_row_for_name("Barracks", &barracks_type,
                                   &barracks_row);
    return entity_spawn_request_state_pair_predicate(
        request->type, request->kind, barracks_type, barracks_row, context);
}

static GameContext* building_upgrade_context_get(EntitySlot* building)
{
    if (building != NULL && building->owner_ctx != NULL)
        return building->owner_ctx;
    return entity_pool_callback_context_get();
}

static uint32_t building_upgrade_config_cost_get(GameContext* context,
                                                  int selector)
{
    const char* key = selector != 0
        ? "Main::BuildingUpgradeCostStuds" : "Main::BuildingUpgradeCostOre";
    const int default_cost = selector != 0 ? 1 : 5;
    if (context == NULL || context->cfg == NULL) return (uint32_t)default_cost;
    return (uint32_t)block_get_int(context->cfg, key, default_cost);
}

static int building_upgrade_live_payment_select(EntitySlot* building,
                                                uint32_t* resource_out,
                                                uint32_t* amount_out)
{
    GameContext* context = building_upgrade_context_get(building);
    uint32_t ore_cost, studs_cost;
    if (resource_out != NULL) *resource_out = 0u;
    if (amount_out != NULL) *amount_out = 0u;
    if (building == NULL || context == NULL || context->level == NULL)
        return 0;
    ore_cost = building_upgrade_config_cost_get(context, 0);
    studs_cost = building_upgrade_config_cost_get(context, 1);
    if (upgrade_resource_check(context->level, 0) >= ore_cost) {
        if (amount_out != NULL) *amount_out = ore_cost;
        return 1;
    }
    if (upgrade_resource_check(context->level, 1) < studs_cost) return 0;
    if (resource_out != NULL) *resource_out = 1u;
    if (amount_out != NULL) *amount_out = studs_cost;
    return 1;
}

static int deferred_olist_string_equal(const char* first, const char* second)
{
    if (first == NULL || second == NULL) return 0;
    while (*first != '\0' && *second != '\0') {
        if (tolower((unsigned char)*first) !=
            tolower((unsigned char)*second)) return 0;
        ++first;
        ++second;
    }
    return *first == '\0' && *second == '\0';
}

static int entity_type_two_positioned_candidate(EntitySlot* candidate,
                                                void* payload)
{
    EntityTypeTwoPositionedDispatch* dispatch =
        (EntityTypeTwoPositionedDispatch*)payload;
    EntityTypeTwoPositionedHooks* hooks = &s_entity_type_two_positioned_hooks;
    int state;
    int next_state;
    int enabled;
    int old_state;

    if (candidate == NULL || candidate == dispatch->source) return 0;
    state = candidate->positioned_render_state_34c;
    old_state = state;
    if (dispatch->mode != 0) {
        float position[3];
        float dx;
        float dy;
        float dz;
        float distance;
        void* component;

        if (state != 0 || !dispatch->source_position_valid) return 0;
        component = candidate->type_component_handle != NULL
            ? candidate->type_component_handle
            : candidate->attached_scene_object;
        if (component == NULL) return 0;
        if (hooks->position_get != NULL) {
            if (hooks->position_get(hooks->context, component, position) == 0)
                return 0;
        }
        else if (ddiscene_object_position_get(component, NULL, position) == 0) {
            return 0;
        }
        dx = position[0] - dispatch->source_position[0];
        dy = position[1] - dispatch->source_position[1];
        dz = position[2] - dispatch->source_position[2];
        distance = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
        if (distance > dispatch->outer_range) next_state = 2;
        else if (distance <= dispatch->inner_range) next_state = 1;
        else return 0;
        enabled = 0;
    }
    else {
        if (state == 0) return 0;
        next_state = 0;
        enabled = 1;
    }

    if (candidate->type >= 1 && candidate->type <= 3 &&
        hooks->render_tracks_set != NULL) {
        hooks->render_tracks_set(hooks->context, candidate,
                                 old_state != 0 ? old_state : next_state,
                                 enabled);
    }
    candidate->positioned_render_state_34c = next_state;
    /* LAB_0043b8d0 always continues the filtered entity-pool walk. */
    return 0;
}

static int entity_task_at_grid_coordinate_candidate(
    EntitySlot* entity, void* payload)
{
    EntityTaskAtGridCoordinateRequest* request =
        (EntityTaskAtGridCoordinateRequest*)payload;
    if (request == NULL || entity == NULL ||
        !entity_shared_eligibility_check(entity, 1)) return 0;

    /* LAB_0043b3b0 has additional renderer/type-specific geometry tests after
     * this shared gate. The portable hook is the only current owner of that
     * scene query; don't substitute the unrelated +0x2f4/+0x2f8 route pair. */
    if (s_entity_task_coordinate_match_hook == NULL ||
        !s_entity_task_coordinate_match_hook(
            s_entity_task_coordinate_match_user, entity,
            request->x, request->y)) return 0;
    *request->out_entity = entity;
    return 1;
}

static int entity_deposit_position_get(EntitySlot* entity, float position[3])
{
    if (entity->attached_scene_object != NULL) {
        if (!ddiscene_object_position_get(entity->attached_scene_object,
                                           NULL, position)) return 0;
    }
    else {
        position[0] = entity->x;
        position[1] = entity->y;
        position[2] = entity->z;
    }
    return position[0] == position[0] && position[1] == position[1] &&
        fabs(position[0]) <= FLT_MAX && fabs(position[1]) <= FLT_MAX;
}

static int entity_kind_four_candidate_search(GameContext* context,
                                            LevelState* level, int x, int y,
                                            int amount)
{
    static const int offsets[4][2] = {
        { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }
    };
    int direction;
    for (direction = 0; direction < 4; ++direction) {
        int cell_xy[2];
        int nx = x + offsets[direction][0];
        int ny = y + offsets[direction][1];
        if (level_cell_secondary_bit0(level, nx, ny) != 0u) {
            EntitySlot* found;
            cell_xy[0] = nx;
            cell_xy[1] = ny;
            found = entity_at_grid_position_find_in_context(
                context, level, cell_xy);
            if (found != NULL && entity_available_resource_spend(
                    level, amount, found)) return 1;
        }
        if (!level_cell_occupied_at(level, nx, ny) &&
            !level_cell_occupied_or_flag(level, nx, ny, 1)) continue;
        if (level_cell_visited_flag(level, nx, ny) != 0u) continue;
        terrain_cell_visited_flag_set_and_track(level, nx, ny);
        if (entity_kind_four_candidate_search(context, level, nx, ny,
                                              amount))
            return 1;
    }
    return 0;
}

static int entity_kind_four_candidate_find(GameContext* context,
                                            LevelState* level,
                                            const int start_xy[2],
                                            int amount)
{
    int found;
    if (level == NULL || start_xy == NULL) return 0;
    found = entity_kind_four_candidate_search(context, level, start_xy[0],
                                               start_xy[1], amount);
    terrain_cell_visited_flags_clear_all(level);
    return found;
}

static void entity_kind_four_adjacent_expand(LevelState* level, int x, int y)
{
    static const int offsets[4][2] = {
        { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }
    };
    int direction;
    for (direction = 0; direction < 4; ++direction) {
        int nx = x + offsets[direction][0];
        int ny = y + offsets[direction][1];
        if (!level_cell_occupied_at(level, nx, ny) &&
            !level_cell_occupied_or_flag(level, nx, ny, 1)) continue;
        if (level_cell_secondary_flag_100(level, nx, ny) != 0u) continue;
        terrain_cell_flag100_set_and_track(level, nx, ny);
        entity_kind_four_adjacent_expand(level, nx, ny);
    }
}

static int entity_unload_adjacent_cell_find_on_level(
    const EntitySlot* entity, const LevelState* level, int out_xy[2])
{
    static const int offsets[4][2] = {
        { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }
    };
    int x = 0, y = 0, i;
    float position[2];
    if (entity == NULL || level == NULL) return 0;
    /* FUN_00439d50 starts from FUN_004424f0's scene-derived position. Use
     * the same live transform when a scene object exists, then apply the
     * terrain's own world-to-cell conversion. The logical EntitySlot x/y
     * values remain the transform fallback inside entity_transform_position_get. */
    entity_transform_position_get(entity, position);
    if (!terrain_world_xy_to_cell(level->ptr_terrain, position[0],
                                  position[1], &x, &y)) return 0;
    if (level_coord_pair_within_bounds(level, x, y) == 0) return 0;
    for (i = 0; i < 4; ++i) {
        int candidate_x = x + offsets[i][0];
        int candidate_y = y + offsets[i][1];
        if (level_cell_flag_8_bounded(level, candidate_x, candidate_y) != 0u
            && terrain_cell_route_and_occupancy_check(
                NULL, level, candidate_x, candidate_y,
                candidate_x, candidate_y, 0) != 0) {
            if (out_xy != NULL) {
                out_xy[0] = candidate_x;
                out_xy[1] = candidate_y;
            }
            return 1;
        }
    }
    return 0;
}

static int entity_type_four_linked_type_five_reset(EntitySlot* candidate,
                                                   void* payload)
{
    EntitySlot* source = (EntitySlot*)payload;
    if (candidate != NULL && candidate->type == 5 &&
        candidate->link_2fc == source &&
        candidate->attach_304 != NULL) {
        entity_action_sequence_reset(candidate, 0);
    }
    /* LAB_00447920 always continues the pool scan. */
    return 0;
}

static int entity_linked_reference_matches(EntitySlot* candidate,
                                           void* target)
{
    int matched = 0;
    if (candidate == NULL || target == NULL) {
        return 0;
    }
    if (candidate->link_2fc == target) {
        candidate->link_2fc = NULL;
        candidate->flags_3ec &= ~0x40u;
        matched = 1;
    }
    {
        int index;
        for (index = 0; index < 7; ++index) {
            if (entity_carried_slot_get(candidate, index) == target) {
                EntitySlot* carried = (EntitySlot*)target;
                int count = candidate->carry_state_324;
                if (count > index && count <= 7) {
                    int next;
                    for (next = index + 1; next < count; ++next) {
                        EntitySlot* moved = entity_carried_slot_get(candidate, next);
                        entity_carried_slot_set(candidate, next - 1, moved);
                        if (moved != NULL && moved->attach_304 == candidate)
                            moved->attached_scene_slot_index = next - 1;
                    }
                    entity_carried_slot_set(candidate, count - 1, NULL);
                    candidate->carry_state_324 = count - 1;
                }
                else entity_carried_slot_set(candidate, index, NULL);
                if (carried->attach_304 == candidate) {
                    carried->attach_304 = NULL;
                    carried->attached_scene_slot_index = 0;
                }
                if (candidate->carry_state_324 == 0 &&
                    candidate->carry_link_308 == NULL)
                    candidate->flags_3e0 &= ~0x400u;
                matched = 1;
                break;
            }
        }
    }
    if (candidate->spawn_child_3cc == target) {
        candidate->spawn_child_3cc = NULL;
        matched = 1;
    }
    return matched;
}

namespace unported_ledger_stubs {
static void spatial_query_request_init(SpatialObjectQueryRequest* request,
                                       uint32_t mask)
{
    memset(request, 0, sizeof(*request));
    request->radius = 10000.0f;
    request->mask = mask;
}

static void spatial_query_center_from_entity(
    SpatialObjectQueryRequest* request, const EntitySlot* entity)
{
    if (entity != NULL) {
        entity_transform_position_get(entity, request->center);
        request->center_is_cell = 0;
    }
}

static void spatial_query_center_from_cell(
    SpatialObjectQueryRequest* request, const int cell_xy[2])
{
    float world_position[3];
    if (request == NULL || cell_xy == NULL) return;
    /* The reference converts the cell to its world-space center. A failed
     * conversion leaves the initialized center at (0, 0), but still runs the
     * entity-pool query. */
    if (terrain_cell_world_position_get(cell_xy[0], cell_xy[1],
                                        world_position)) {
        request->center[0] = world_position[0];
        request->center[1] = world_position[1];
    }
    request->center_is_cell = 0;
}

static int adjacent_route_candidate_stop(void*, const uint32_t cell[2],
                                         void* context)
{
    AdjacentRouteAcceptContext* accept =
        (AdjacentRouteAcceptContext*)context;
    if (accept == NULL || cell == NULL) return 0;
    return entity_route_cell_candidate_accept(accept->entity, accept->level,
        (int)cell[0], (int)cell[1], accept->route_cookie);
}

static EntityPathMotionRuntime* entity_path_motion_runtime_get(
    EntitySlot* entity, int create)
{
    EntityPathMotionRuntime* free_slot = NULL;
    unsigned int i;
    if (entity == NULL) return NULL;
    for (i = 0u; i < ENTITY_TYPES * ENTITY_CAP; ++i) {
        EntityPathMotionRuntime* state = &s_entity_path_motion_runtime[i];
        if (state->owner == entity) return state;
        if (free_slot == NULL && state->owner == NULL) free_slot = state;
    }
    if (!create || free_slot == NULL) return NULL;
    memset(free_slot, 0, sizeof(*free_slot));
    free_slot->owner = entity;
    free_slot->waypoint_index = -1;
    return free_slot;
}

/* Return the real current terrain pick result; absent terrain is failure. */
static int entity_work_entry_grid_pick(float x, float y, int* cx, int* cy)
{
    LevelState* level = level_current_owner_get();
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    if (!terrain_world_xy_to_cell(level->ptr_terrain, x, y, cx, cy)) return 0;
    return *cx < level_width(level) - 1 && *cy < level_height(level) - 1;
}

// EntityCoordinateRouteAndMark
static int entity_route_to_cell_commit(EntitySlot* entity,
                                       int target_x, int target_y,
                                       unsigned int marker)
{
    LevelState* level;
    int start_x, start_y;
    int* path_x = NULL;
    int* path_y = NULL;
    uint32_t path_count = 0;
    float route_x[AI_WAYPOINTS_MAX];
    float route_y[AI_WAYPOINTS_MAX];
    uint32_t i;

    if (entity == NULL || entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || !entity_route_cell_candidate_accept(
            entity, level, target_x, target_y, 1)) return 0;
    entity_position_to_terrain_cell(entity, level, &start_x, &start_y);
    if (start_x == target_x && start_y == target_y) {
        path_x = (int*)malloc(sizeof(int));
        path_y = (int*)malloc(sizeof(int));
        if (path_x == NULL || path_y == NULL) {
            free(path_x);
            free(path_y);
            return 0;
        }
        path_x[0] = target_x;
        path_y[0] = target_y;
        path_count = 1;
    }
    else if (!terrain_neighbor_path_check(level, (uint32_t)start_x,
                                           (uint32_t)start_y,
                                           (uint32_t)target_x,
                                           (uint32_t)target_y,
                                           &path_x, &path_y,
                                           &path_count, NULL, NULL)) {
        if (abs(target_x - start_x) + abs(target_y - start_y) != 1) return 0;
        path_x = (int*)malloc(sizeof(int));
        path_y = (int*)malloc(sizeof(int));
        if (path_x == NULL || path_y == NULL) {
            free(path_x);
            free(path_y);
            return 0;
        }
        path_x[0] = target_x;
        path_y[0] = target_y;
        path_count = 1;
    }
    if (path_count == 0 || path_count > AI_WAYPOINTS_MAX) {
        free(path_x);
        free(path_y);
        return 0;
    }
    for (i = 0; i < path_count; ++i) {
        route_x[i] = (float)path_x[i];
        route_y[i] = (float)path_y[i];
    }
    free(path_x);
    free(path_y);
    if (!entity_route_setup_and_marker_associate(entity, path_count,
                                                  route_x, route_y, 0))
        return 0;
    entity->activity = (int)marker;
    entity->activity_timer = 0.0f;
    entity->pos_pair_2f4 = (float)target_x;
    entity->pos_pair_2f8 = (float)target_y;
    return 1;
}
static void entity_type_behavior_cell_flag_80000_set(EntitySlot* entity,
                                                      int set)
{
    LevelState* level;
    int cell_x = -1;
    int cell_y = -1;
    if (entity == NULL || entity->owner_ctx == NULL) return;
    level = entity->owner_ctx->level;
    if (level == NULL) return;
    entity_position_to_terrain_cell(entity, level, &cell_x, &cell_y);
    level_cell_flag_80000_set(level, cell_x, cell_y, set);
}

static float entity_type_behavior_animation_advance(
    EntitySlot* entity, float delta, const EntityTypeBehaviorHooks* hooks)
{
    if (hooks != NULL && hooks->animation_tick != NULL) {
        return (float)hooks->animation_tick(hooks->context, entity, delta);
    }
    /* FUN_00475120 operates on the scene object stored at entity +0x1c.
     * EntitySlot's attached_scene_object is that runtime owner, and the
     * view module already implements the corresponding advance/remainder
     * behavior. */
    if (entity != NULL && entity->attached_scene_object != NULL) {
        return ddiscene_advance_by_time(entity->attached_scene_object, delta);
    }
    return 0.0f;
}

static void entity_type_behavior_animation_play(
    EntitySlot* entity, const EntityTypeBehaviorHooks* hooks,
    const char* animation)
{
    if (entity == NULL || animation == NULL) return;
    if (hooks != NULL && hooks->animation_play != NULL) {
        hooks->animation_play(hooks->context, entity, animation);
    }
    else if (entity->attached_scene_object != NULL) {
        /* SceneActionNameSet already follows the retail scene object's
         * action interfaces and callback slots. Keep the optional host
         * override, but let the live scene object handle named transitions
         * when no override is installed. */
        (void)scene_action_name_set(entity->attached_scene_object,
                                    animation);
    }
}

static unsigned int entity_route_cardinal_direction(int source_x, int source_y,
                                                    int target_x, int target_y)
{
    int dx = target_x - source_x;
    int dy = target_y - source_y;
    if (abs(dx) <= abs(dy)) return dy <= 0 ? 2u : 0u;
    return dx <= 0 ? 3u : 1u;
}

static float entity_route_weight_random(float minimum, float maximum)
{
    return unported_ledger_stubs::entity_cell_step_random_range(minimum,
                                                                 maximum);
}
/* The retail route issuers all build a one-cell path for a same-cell target,
 * otherwise use TerrainNeighborPathCheck, then pass the returned cells to
 * EntityRouteSetupAndMarkerAssociate. Keep the temporary arrays sized to the
 * actual path instead of imposing the unrelated inline waypoint limit. */
static int entity_route_live_position_to_cell(const EntitySlot* entity,
                                              LevelState* level,
                                              int* cell_x, int* cell_y)
{
    float position[2];
    if (entity == NULL || level == NULL || level->ptr_terrain == NULL ||
        cell_x == NULL || cell_y == NULL) return 0;
    entity_transform_position_get(entity, position);
    return terrain_world_xy_to_cell(level->ptr_terrain, position[0],
                                     position[1], cell_x, cell_y);
}

static int entity_route_path_install(EntitySlot* entity, LevelState* level,
                                     int target_x, int target_y,
                                     const float* marker_position)
{
    int source_x = -1;
    int source_y = -1;
    int* path_x = NULL;
    int* path_y = NULL;
    float* route_x = NULL;
    float* route_y = NULL;
    uint32_t path_count = 0;
    uint32_t i;
    int installed;

    if (entity == NULL || level == NULL) return 0;
    if (!entity_route_live_position_to_cell(entity, level, &source_x,
                                             &source_y)) return 0;
    if (source_x == target_x && source_y == target_y) {
        path_x = (int*)malloc(sizeof(int));
        path_y = (int*)malloc(sizeof(int));
        if (path_x == NULL || path_y == NULL) {
            free(path_x);
            free(path_y);
            return 0;
        }
        path_x[0] = target_x;
        path_y[0] = target_y;
        path_count = 1;
    }
    else if (!terrain_neighbor_path_check(level, (uint32_t)source_x,
            (uint32_t)source_y, (uint32_t)target_x, (uint32_t)target_y,
            &path_x, &path_y, &path_count, NULL, NULL)) {
        free(path_x);
        free(path_y);
        return 0;
    }
    if (path_count == 0u ||
        (size_t)path_count > ((size_t)-1) / sizeof(float)) {
        free(path_x);
        free(path_y);
        return 0;
    }
    route_x = (float*)malloc((size_t)path_count * sizeof(float));
    route_y = (float*)malloc((size_t)path_count * sizeof(float));
    if (route_x == NULL || route_y == NULL) {
        free(route_x);
        free(route_y);
        free(path_x);
        free(path_y);
        return 0;
    }
    for (i = 0; i < path_count; ++i) {
        route_x[i] = (float)path_x[i];
        route_y[i] = (float)path_y[i];
    }
    free(path_x);
    free(path_y);
    installed = entity_route_setup_and_marker_associate(entity, path_count,
        route_x, route_y, marker_position);
    free(route_x);
    free(route_y);
    return installed;
}

static int nearest_open_cell_candidate_compare(const void* first,
                                               const void* second)
{
    const NearestOpenCellCandidate* a =
        (const NearestOpenCellCandidate*)first;
    const NearestOpenCellCandidate* b =
        (const NearestOpenCellCandidate*)second;
    if (a->distance < b->distance) return -1;
    if (a->distance > b->distance) return 1;
    return 0;
}
} // namespace unported_ledger_stubs

static void idle100_register(EntitySlot* e)
{
    if (s_idle100_n < AI_IDLE_TABLE_MAX) {
        s_idle100[s_idle100_n++] = e;
    }
}

/* SceneActivityUpdateDispatch (FUN_00447c40) only attaches the idle command
 * for activities outside this name-table exclusion set. These are the
 * eight direct pointer comparisons in the decompile (activity table slots
 * 0, 1, 4, 6, 8, 12, 17, and 23). */
static int activity_skips_idle_chain(int activity)
{
    switch (activity) {
    case 0:
    case 1:
    case 4:
    case 6:
    case 8:
    case 12:
    case 17:
    case 23:
        return 1;
    default:
        return 0;
    }
}

/* ObjectiveSidecarAppendGate (FUN_0043bc40). The entity's stats accessors,
 * activity, owner, carried-object, selection, task, and OSF state are live in
 * the portable model. Scene-object creation/labels remain renderer-owned. */
static void objective_sidecar_activity_set(EntitySlot* entity, int activity)
{
    ai_set_activity(entity, activity, 0);
    ai_idle_chain_record_attach(entity);
    entity->activity_last = entity->activity;
    entity->activity_timer = 0.0f;
}

static void objective_sidecar_owner_detach(GameContext* ctx,
                                           EntitySlot* entity)
{
    EntitySlot* owner = (EntitySlot*)entity->owner_36c;
    if (owner == NULL) return;
    selection_target_command_update(ctx, entity);
    owner->flags_3e4 &= ~8u;
    owner->owner_36c = NULL;
    entity->owner_36c = NULL;
}

namespace lego {
namespace {
static int global_scatter_find_adapter(EntitySlot* entity, void* payload)
{
    int mode = payload != NULL ? *(int*)payload : 0;
    if (s_global_scatter_find_hook == NULL) return 0;
    return s_global_scatter_find_hook(entity, mode,
                                     s_global_scatter_find_context);
}

static int global_scatter_find(int mode)
{
    return entity_pool_callback_search(global_scatter_find_adapter, &mode);
}
} // namespace
} // namespace lego

namespace unported_ledger_stubs {
static GameContext* emergence_game_context_get(const void* base_entity)
{
    const EntitySlot* base = (const EntitySlot*)base_entity;
    if (base != NULL && base->owner_ctx != NULL) return base->owner_ctx;
    return entity_pool_callback_context_get();
}

static LevelState* emergence_level_get(const void* base_entity)
{
    GameContext* context;
    if (s_emergence_spawn_hooks.level != NULL)
        return s_emergence_spawn_hooks.level;
    context = emergence_game_context_get(base_entity);
    return context != NULL ? context->level : NULL;
}

static int emergence_cell_world_position_get(int cell_x, int cell_y,
                                             float out_xy[2])
{
    float position[3];
    if (s_emergence_spawn_hooks.cell_world_position != NULL)
        return s_emergence_spawn_hooks.cell_world_position(cell_x, cell_y,
            out_xy, s_emergence_spawn_hooks.user);
    if (!terrain_cell_world_position_get(cell_x, cell_y, position)) return 0;
    out_xy[0] = position[0];
    out_xy[1] = position[1];
    return 1;
}

static EntitySlot* emergence_entity_create(void* base_entity,
                                           int owner_index, float x,
                                           float y, float heading)
{
    EntitySlot* created;
    const EntitySlot* base = (const EntitySlot*)base_entity;
    GameContext* context = emergence_game_context_get(base_entity);
    const char* type_name;
    int configured_name;
    if (s_emergence_spawn_hooks.create != NULL)
        return s_emergence_spawn_hooks.create(base_entity, 3, owner_index,
            x, y, heading, s_emergence_spawn_hooks.user);
    if (context == NULL) return NULL;
    type_name = emergence_type_name_get(owner_index);
    configured_name = type_name != NULL;
    if (type_name == NULL && base != NULL) type_name = base->name;
    created = entity_create_with_position(context, 3,
        type_name, owner_index, x, y, heading);
    if (created == NULL) return NULL;
    if (configured_name) {
        /* FUN_00437d00 receives the configured type row and owner index as
         * separate arguments; the port's name-based constructor needs the
         * row index restored explicitly. */
        created->etype = owner_index;
        return created;
    }
    if (base == NULL) return created;

    /* EntityCreateWithPosition owns allocation and world positioning. The
     * emergence caller supplies the type-3 template record, so carry over
     * the row identity and stats image that select the creature variant. */
    created->etype = base->etype;
    memcpy(created->type_name, base->type_name, sizeof(created->type_name));
    created->type_name[sizeof(created->type_name) - 1u] = '\0';
    created->stats_record = base->stats_record;
    created->task_state = base->task_state;
    created->activity_cap = base->activity_cap;
    return created;
}
} // namespace unported_ledger_stubs

static int terrain_variant_entity_visit_adapter(EntitySlot* candidate,
                                                 void* context)
{
    TerrainVariantVisitContext* visit = (TerrainVariantVisitContext*)context;
    if (visit == NULL || s_terrain_variant_visit_hook == NULL) return 0;
    return s_terrain_variant_visit_hook(visit->owner_entity, candidate,
                                        s_terrain_variant_scene_context);
}

/* The retail search's first argument is an optional actor passed through to
 * TerrainCellRouteAndOccupancyCheck. Grid dimensions always come from the
 * active level. Older portable callers also pass LevelState or TerrainState
 * here, so recognize those owners without interpreting their leading words
 * as an EntitySlot. */
static LevelState* terrain_path_level_resolve(void* argument,
                                              EntitySlot** actor_out)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level :
        (LevelState*)map_cell_record_grid_level_get();
    int type;
    if (actor_out != NULL) *actor_out = NULL;
    if (level == NULL) return NULL;
    if (argument == NULL || argument == level ||
        argument == level->ptr_terrain) return level;
    /* Validate actor identity against the active typed tables before reading
     * any EntitySlot fields. This also makes the current-level APIs safe for
     * their historical LevelState/TerrainState arguments. */
    if (game != NULL) {
        for (type = 1; type <= 19; ++type) {
            int index;
            const int count = entity_count(game, type);
            for (index = 0; index < count; ++index) {
                EntitySlot* candidate =
                    (EntitySlot*)entity_at(game, type, index);
                if (candidate == argument) {
                    if (actor_out != NULL) *actor_out = candidate;
                    return level;
                }
            }
        }
        return level;
    }
    /* Without the entity pools an unknown pointer cannot safely be treated
     * as an EntitySlot; NULL and explicit level/terrain arguments still use
     * the active registry's live level. */
    return NULL;
}

static int terrain_path_grid_size_get(void* entity, uint32_t* width,
                                      uint32_t* height)
{
    LevelState* level;
    if (s_terrain_path_grid_size_hook != NULL)
        return s_terrain_path_grid_size_hook(entity, width, height,
                                             s_terrain_path_context);
    level = terrain_path_level_resolve(entity, NULL);
    if (level == NULL || width == NULL ||
        height == NULL) return 0;
    *width = (uint32_t)level_width(level);
    *height = (uint32_t)level_height(level);
    return *width != 0u && *height != 0u;
}

static int terrain_path_route_accept(void* entity, uint32_t x, uint32_t y,
                                     int direction)
{
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    LevelState* level;
    EntitySlot* actor;
    if (s_terrain_path_route_hook != NULL)
        return s_terrain_path_route_hook(entity, x, y, direction,
                                         s_terrain_path_context);
    level = terrain_path_level_resolve(entity, &actor);
    if (level == NULL || direction < 0 ||
        direction >= 4) return 0;
    return terrain_cell_route_and_occupancy_check(
        actor, level, (int)x, (int)y,
        (int)x + dx[direction], (int)y + dy[direction], 0) != 0;
}

static int terrain_path_cell_accept(void* entity, uint32_t x, uint32_t y)
{
    LevelState* level;
    EntitySlot* actor;
    level = terrain_path_level_resolve(entity, &actor);
    if (level != NULL) {
        return terrain_cell_route_and_occupancy_check(
            actor, level, (int)x, (int)y,
            (int)x, (int)y, 0) != 0;
    }
    /* The legacy route hook cannot express the target's same-cell
     * `(x,y)->(x,y)` predicate, so use it only when no live level exists. */
    return s_terrain_path_route_hook != NULL
        ? s_terrain_path_route_hook(entity, x, y, 0,
                                    s_terrain_path_context)
        : 0;
}

static int terrain_path_half_cost_accept(void* entity, uint32_t x,
                                         uint32_t y, int direction)
{
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    LevelState* level;
    int probe_x;
    int probe_y;
    if (s_terrain_path_half_cost_hook != NULL)
        return s_terrain_path_half_cost_hook(entity, x, y, direction,
                                             s_terrain_path_context);
    level = terrain_path_level_resolve(entity, NULL);
    if (level == NULL || direction < 0 ||
        direction >= 4) return 0;
    probe_x = (int)x + dx[direction];
    probe_y = (int)y + dy[direction];
    return level_cell_occupied_at(level, probe_x, probe_y) != 0;
}

static int scene_entity_callback(EntitySlot* entity, void* payload)
{
    float dt = *(float*)payload;
    /* LAB_0043c810 updates live EntitySlot/runner state. Always continue the
     * pool walk here: its nonzero return is reserved for early branch exits
     * inside one entity, not for stopping FUN_004377d0. */
    ai_entity_frame_update(entity, dt);
    if (s_scene_entity_hook != NULL)
        s_scene_entity_hook(entity, dt, s_scene_entity_user);
    return 0;
}

/* FUN_00432200 snapshots the tracked coordinates before the entity walk,
 * mirrors each coordinate into its previous-state slots, and clears cell
 * flag 0x100. The host grid is the same +0xb8 / 0x48-byte cell array used by
 * EngineState. */
static void scene_cell_previous_state_track(EngineState* st)
{
    uint32_t i;
    if (s_scene_tracked_count == 0) return;
    for (i = 0; i < s_scene_tracked_count; ++i) {
        SimtickCellCoord coord = s_scene_tracked[i];
        if (st != NULL && st->grid_cells != NULL && coord.x >= 0 && coord.y >= 0
            && (uint32_t)coord.x < st->grid_w
            && (uint32_t)coord.y < st->grid_h) {
            uint8_t* cell = (uint8_t*)st->grid_cells
                + ((uint32_t)coord.y * st->grid_w + (uint32_t)coord.x) * 0x48u;
            uint32_t flags;
            memcpy(&flags, cell + 0x0c, sizeof(flags));
            flags &= ~0x100u;
            memcpy(cell + 0x0c, &flags, sizeof(flags));
        }
        scene_cell_list_append(&s_scene_pending, &s_scene_pending_count,
                              &s_scene_pending_capacity, coord.x, coord.y);
    }
    s_scene_tracked_count = 0;
}

/* FUN_004321a0 drains the snapshot after the entity callback walk. A cell
 * whose +0x0d byte bit 0 is clear is sent to CellInitPass (FUN_0042f6f0). */
static void scene_cell_update_queue_flush(EngineState* st)
{
    uint32_t i;
    for (i = 0; i < s_scene_pending_count; ++i) {
        SimtickCellCoord coord = s_scene_pending[i];
        if (st == NULL || st->grid_cells == NULL || coord.x < 0 || coord.y < 0
            || (uint32_t)coord.x >= st->grid_w
            || (uint32_t)coord.y >= st->grid_h) continue;
        uint8_t* cell = (uint8_t*)st->grid_cells
            + ((uint32_t)coord.y * st->grid_w + (uint32_t)coord.x) * 0x48u;
        if ((cell[0x0d] & 1u) == 0 && s_scene_cell_init != NULL)
            s_scene_cell_init(st, coord.x, coord.y);
    }
    s_scene_pending_count = 0;
}

static int render_entity_callback(EntitySlot* entity, void* payload)
{
    (void)payload;
    if (s_render_entity_hook != NULL)
        s_render_entity_hook(entity, s_render_entity_user);
    return 0;
}

/* ---- group B adapters ---- */
/* Original-ABI adapters (ADR 0005 step 6) for GAME-023 part 1 group B in
 * legoobject_unit.c. Where the port's entry takes a GameContext or a hook
 * table, or has no entry at all, these give the original's contract.
 * Natively nothing calls them: the unit's functions run only in the
 * matching build. */

/* 00472060 (Weapons.c): the RunThroughLists callback LegoObject_Remove
 * passes, which clears another object's +0x3c4 reference to the removed
 * one. No port entry: does nothing. */
// PORT-ONLY: original-ABI adapter for GAME-023 legoobject_unit.c (callback 0x00472060, no port entry)
extern "C" int legoobject_gb_weapon_target_clear_callback(void* obj, void* data)
{
    (void)obj;
    (void)data;
    return 0;
}

/* 0043f560 takes the object; the port's entry takes its context too. */
// PORT-ONLY: original-ABI adapter for GAME-023 legoobject_unit.c (entity_pool_find_with_local_predicate, 0x0043f560)
extern "C" int legoobject_gb_pool_find_with_local_predicate_adapter(void* obj)
{
    return entity_pool_find_with_local_predicate(
        entity_pool_callback_context_get(), (EntitySlot*)obj);
}

/* 0044be30 takes the object; the port's entry takes a hook table. No hooks
 * are installed for the original's contract: does nothing. */
// PORT-ONLY: original-ABI adapter for GAME-023 legoobject_unit.c (entity_auxiliary_payload_release, 0x0044be30)
extern "C" void legoobject_gb_auxiliary_payload_release_adapter(void* obj)
{
    (void)obj;
}

/* 00409a90 takes (id, position, direction, points, count, notify) and
 * returns the object; the port's entry takes a hook table and returns
 * nothing. Does nothing. */
// PORT-ONLY: original-ABI adapter for GAME-023 legoobject_unit.c (world_object_create_with_transform, 0x00409a90)
extern "C" void* legoobject_gb_world_object_create_adapter(int id, void* position,
                                                           unsigned int direction,
                                                           void* points,
                                                           uint32_t count,
                                                           int notify)
{
    (void)id;
    (void)position;
    (void)direction;
    (void)points;
    (void)count;
    (void)notify;
    return NULL;
}

/* 00454880 is called with the object's type, which it does not read; the
 * port's entry takes no argument. */
// PORT-ONLY: original-ABI adapter for GAME-023 legoobject_unit.c (global_counter_004ff990_increment, 0x00454880)
extern "C" void legoobject_gb_tutorial_counter_increment_adapter(int type)
{
    (void)type;
    global_counter_004ff990_increment();
}

/* ---- group G adapters (GAME-023 part 1, legoobject_unit.c) ---- */

/* 00447b30 (EntityActivityTick) with C linkage for legoobject_unit.c. */
// PORT-ONLY: original-ABI adapter for 0x00447b30, called by legoobject_unit.c
extern "C" float legoobject_activity_tick_adapter(EntitySlot* entity, float delta)
{
    return unported_ledger_stubs::ledger_stub_00447b30_EntityActivityTick(entity, delta);
}

/* ---- group C adapters ---- */
/* Original-ABI adapters (ADR 0005 step 6) for GAME-023 legoobject_unit.c,
 * group C (LegoObject_Create 0x00437d00 and 0x00438460). The port's
 * 0043f560 and 0044b370 take the GameContext too, and its 0044b680 is a
 * ledger stub with another contract. Nothing calls these natively. */

/* 0043f560 takes only the object. */
// PORT-ONLY: original-ABI adapter for GAME-023 legoobject_unit.c (0x0043f560 takes the object only)
extern "C" int legoobject_gc_pool_find_with_local_predicate_adapter(
    EntitySlot* e)
{
    return entity_pool_find_with_local_predicate(
        e != NULL ? e->owner_ctx : NULL, e);
}

/* 0044b370 takes the object and the activity value. */
// PORT-ONLY: original-ABI adapter for GAME-023 legoobject_unit.c (0x0044b370 takes no GameContext)
extern "C" void legoobject_gc_activity_grant_adapter(EntitySlot* e, int value)
{
    ai_activity_grant(e != NULL ? e->owner_ctx : NULL, e, value);
}

/* 0044b680 attaches the object's scene tree; the port's ledger stub takes
 * the scene objects and a hook instead. No port equivalent: nothing. */
// PORT-ONLY: original-ABI adapter for GAME-023 legoobject_unit.c (no port equivalent of 0x0044b680)
extern "C" int legoobject_gc_scene_object_tree_attach_adapter(EntitySlot* e)
{
    (void)e;
    return 0;
}
