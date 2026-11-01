// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "cmd.hpp"
#include "ai.hpp"
#include "spawn.hpp"
#include "select.hpp"
#include "level.hpp"
#include "terrain_grid.hpp"
#include "frontend.hpp"
#include "util.hpp"
#include "gods98/engine.hpp"
#include "gods98/view.hpp"
#include "gods98/block.hpp"
#include "entity.hpp"
#include "crt/crt.hpp"
#include "gods98/heap.hpp"
#include "gods98/media.hpp"
#include "gods98/message.hpp"
#include "gods98/sound.hpp"
#include "port_loop_ports.hpp"

namespace unported_ledger_stubs {
int entity_capacity_or_state_predicate(const EntitySlot* entity);
int ledger_stub_0043fc20_RecordPairPoseCopyAndActivityRaise(
    EntitySlot* entity);
int ledger_stub_00447210_EntityAdjacentPlacementCellSearch(
    EntitySlot* entity, EntitySlot* target);
int entity_cell_step_command_issue_with_jitter(
    EntitySlot* entity, const int target_xy[2], const float input_xy[2],
    int action_mode, int mark_pending);
int ledger_stub_004446e40_EntityNearestCellRouteCommit(
    EntitySlot* entity, unsigned int goal_x, unsigned int goal_y, int mode);
int ledger_stub_00448800_EntityMoveToCellCoordinates(
    EntitySlot* entity, const int cell[2]);
int ledger_stub_00448880_RecordGatedStepCommandIssue(
    EntitySlot* entity, const int cell[2]);
int ledger_stub_00448a60_EntityNeighborCellMoveCommandHandle(
    EntitySlot* entity, const int cell[2]);
int ledger_stub_00449010_EntityKind6RecordApproachRouteIssue(
    EntitySlot* entity);
int ledger_stub_004490a0_ProximityStepCommandIssueWithLink(
    EntitySlot* entity, EntitySlot* target, int mode, int set_global_gate);
int ledger_stub_00449240_EntityWalkableTargetCellRouteIssue(
    EntitySlot* entity, const int cell[2]);
int ledger_stub_004492b0_CellStepCommandIssueWithTargetMark(
    EntitySlot* entity, const int cell[2]);
int ledger_stub_00449430_EntityTargetCellRouteAndAssociate(
    EntitySlot* entity, EntitySlot* target);
int ledger_stub_00449520_EntityTargetCellRouteMarker14(
    EntitySlot* entity, EntitySlot* target);
int ledger_stub_00449610_EntityCoordinateRouteAndMark(
    EntitySlot* entity, const int cell[2]);
int ledger_stub_00449700_RouteStepCommandIssue(EntitySlot* entity);
int ledger_stub_00446e40_EntityNearestCellRouteCommit(
    EntitySlot* entity, unsigned int goal_x, unsigned int goal_y, int mode);
int ledger_stub_00449ac0_ProximityStepCommandIssueClearingGate(
    EntitySlot* entity, EntitySlot* target);
int ledger_stub_0044b500_ObjectPositionStepCommandIssueAndLink(
    EntitySlot* entity, const float target_position[2], uint32_t flags);
int ledger_stub_004403d0_LevelDebugCellRetargetDispatch(
    EntitySlot* entity, EntitySlot* target,
    const EntityDebugCellRetargetHooks* hooks);
int ledger_stub_00448eb0_ProximityStepCommandIssueWithJitter(
    EntitySlot* entity, EntitySlot* target, int marker);
void* ledger_stub_00438ae0_SpatialObjectQueryCenterMask02(
    const EntitySlot* entity, const int cell_xy[2], uint32_t filter_a,
    uint32_t filter_b);
}

static CommandKindRecordHooks s_command_kind_record_hooks;
extern float random_float_range(float minimum, float maximum);
#include "gods98/string.hpp"

/* ------------------------------------------------------------------ *
 *  Command record (finding 10 §"Command record" — corpus stride 0x68).
 *  Corpus 0x403190 has "mov [esi+0x38], eax", "mov [esi+0x58], 0", and
 *  state read/write instructions at +0x5c; keep those named fields at their
 *  corpus offsets. The port-only global-list link uses the intervening +0x50
 *  word; private staging values follow the corpus +0x60 chain link.
 *  CMD_STRIDE is sizeof(CmdRec), so the
 *  pool allocation includes the private tail rather than overlapping the
 *  corpus-visible fields (the x86 port record is 0x6c bytes).
 * ------------------------------------------------------------------ */

typedef struct CmdRec {
    int32_t  type;       /* +0x00 record type (0 move/2 target/3 task/5 gated) */
    int32_t  param;      /* +0x04 (flag for move, ext args)                   */
    float    x;          /* +0x08 move coords                                 */
    float    y;          /* +0x0c                                             */
    void*    payload;    /* +0x10 entity / target pointer                      */
    float    now;        /* +0x14 timeGetTime on alloc, 1500.0 on link          */
    int32_t  param2;     /* +0x18 (task param from the queue)                  */
    int32_t  behaviour;  /* +0x1c behaviour id = behaviour[kind]                */
    int32_t  cell_act;   /* +0x20 cell-command action id (8/0xb/0xd)          */
    int32_t  cell_target;/* +0x24 cell-command referenced entity (-1 = unset)   */
    int32_t  cell_count; /* +0x28 cell-command entry count                      */
    int32_t  zone;       /* +0x2c zone-command id (kind-7; the corpus writer
                          * of this word is unported — zero-init)             */
    uint8_t  pad_30[8];  /* +0x30..+0x37 (keeps the corpus +0x38 slot)          */
    void*    arg38;      /* +0x38 raw word/payload (type 0x13 selector)        */
    uint32_t field_3c;   /* +0x3c task-kind selector (when supplied)          */
    void*    buffer;     /* +0x40 owned buffer (freed on cmd_free)             */
    uint32_t count;      /* +0x44 the buffer's entry count                     */
    void*    entity_ref; /* +0x48 pending entity reference (the corpus
                           * writers are the spawn-dispatch band, unported —
                           * zero until one installs a value)                   */
    int32_t  kind;       /* +0x4c kind index                                   */
    void*    global_next; /* +0x50 port-only global command-list link           */
    void*    event_payload; /* +0x54 event payload (the 0xc queue's word)       */
    void*    ref;        /* +0x58 referenced record (the 0x13 "ref" pair)      */
    uint32_t state;      /* +0x5c state flags (CMD_STATE_*)                    */
    void*    next;       /* +0x60 entity-chain link; port reuses it when free  */
    int32_t  grant_act;  /* port-only staged activity (deferred install 0x43da62) */
    int32_t  extra;      /* port-only task selector (type-8 tasks set 8)        */
} CmdRec;

static CmdEntityChainExternalOps s_cmd_entity_chain_external_ops;

void cmd_entity_chain_external_ops_set(const CmdEntityChainExternalOps* ops)
{
    if (ops == NULL) memset(&s_cmd_entity_chain_external_ops, 0,
                            sizeof(s_cmd_entity_chain_external_ops));
    else s_cmd_entity_chain_external_ops = *ops;
}

static int cmd_entity_chain_kind_dispatch(EntitySlot* entity, CmdRec* record)
{
    const int cell[2] = { (int)record->x, (int)record->y };
    EntitySlot* target = (EntitySlot*)record->payload;
    int result = 1;

    switch (record->type) {
    case 3:
        result = entity_gated_step_command_issue(entity, target);
        break;
    case 4:
        result = lego::entity_conditional_type_five_spawn(entity);
        break;
    case 5:
        result = entity_deposit_activity_gate(entity, target);
        break;
    case 6:
    case 7:
        result = unported_ledger_stubs::entity_cell_step_command_issue_with_jitter(
            entity, cell, (const float*)((const char*)record + 0x30), 1,
            (record->state & 0x800u) != 0u);
        break;
    case 8:
        if ((record->state & 8u) != 0u &&
            (entity_stats_word_get(entity) & 1u) == 0u) {
            result = 1;
        }
        else {
            result = unported_ledger_stubs::
                ledger_stub_00446e40_EntityNearestCellRouteCommit(
                    entity, (unsigned int)cell[0], (unsigned int)cell[1],
                    (int)(record->state & 8u));
        }
        break;
    case 9:
        result = unported_ledger_stubs::
            ledger_stub_00448800_EntityMoveToCellCoordinates(entity, cell);
        break;
    case 10:
        result = unported_ledger_stubs::
            ledger_stub_004490a0_ProximityStepCommandIssueWithLink(
                entity, target, 1,
                (record->state & 0x00200000u) != 0u);
        break;
    case 11:
        result = unported_ledger_stubs::
            ledger_stub_00449240_EntityWalkableTargetCellRouteIssue(entity,
                                                                    cell);
        break;
    case 12:
        result = unported_ledger_stubs::
            ledger_stub_004492b0_CellStepCommandIssueWithTargetMark(
                entity, cell);
        break;
    case 13:
    case 14:
        result = 1;
        break;
    case 18:
        result = unported_ledger_stubs::
            ledger_stub_004403d0_LevelDebugCellRetargetDispatch(
                entity, target, NULL);
        break;
    case 15:
        result = unported_ledger_stubs::
            ledger_stub_00448a60_EntityNeighborCellMoveCommandHandle(
                entity, cell);
        break;
    case 16:
    case 17:
    case 20:
        if (record->type == 20)
            result = unported_ledger_stubs::
                ledger_stub_00448880_RecordGatedStepCommandIssue(entity, cell);
        else
            result = unported_ledger_stubs::
                ledger_stub_00449ac0_ProximityStepCommandIssueClearingGate(
                    entity, target);
        break;
    case 19:
        /* 0x44b500 consumes record +0x38 as a task record, resolves the
         * entity's scene position through 0x438df0, then builds a route. */
        result = s_cmd_entity_chain_external_ops.object_position_step != NULL
            ? s_cmd_entity_chain_external_ops.object_position_step(
                entity, record->arg38,
                s_cmd_entity_chain_external_ops.context) : -1;
        break;
    case 21:
        /* 0x444690 consumes the current waypoint and task-record payload;
         * its DDiScene/task operation remains behind this typed owner seam. */
        result = s_cmd_entity_chain_external_ops.waypoint_task_step != NULL
            ? s_cmd_entity_chain_external_ops.waypoint_task_step(
                entity, record->payload, (uint32_t)record->cell_count,
                s_cmd_entity_chain_external_ops.context) : -1;
        break;
    case 22:
        result = entity_set_activity_place(entity);
        break;
    case 23:
        result = unported_ledger_stubs::
            ledger_stub_00448eb0_ProximityStepCommandIssueWithJitter(
                entity, target, 0);
        break;
    case 24:
        result = unported_ledger_stubs::
            ledger_stub_00449700_RouteStepCommandIssue(entity);
        break;
    case 25:
        result = unported_ledger_stubs::
            ledger_stub_00449610_EntityCoordinateRouteAndMark(entity, cell);
        break;
    case 26:
        result = unported_ledger_stubs::
            ledger_stub_00449430_EntityTargetCellRouteAndAssociate(
                entity, target);
        break;
    case 27:
        result = unported_ledger_stubs::
            ledger_stub_00449010_EntityKind6RecordApproachRouteIssue(entity);
        break;
    case 28:
        result = entity_route_marker_setup(entity);
        break;
    case 29:
        result = unported_ledger_stubs::
            ledger_stub_00449520_EntityTargetCellRouteMarker14(
                entity, target);
        break;
    case 30:
        result = entity_route_to_explicit_target(entity, target);
        break;
    default:
        result = 1;
        break;
    }
    return result;
}


#define CMD_STRIDE sizeof(CmdRec)

static CmdRec* as_rec(void* p) { return (CmdRec*)p; }
static const CmdRec* cas_rec(const void* p) { return (const CmdRec*)p; }

int cmd_rec_type(const void* rec) { return cas_rec(rec)->type; }
const void* cmd_rec_payload(const void* rec) { return cas_rec(rec)->payload; }
int cmd_rec_kind(const void* rec) { return cas_rec(rec)->kind; }
int cmd_rec_behaviour(const void* rec) { return cas_rec(rec)->behaviour; }
int cmd_rec_activity(const void* rec) { return cas_rec(rec)->grant_act; }
uint32_t cmd_rec_value_38(const void* rec)
{
    return (uint32_t)(uintptr_t)cas_rec(rec)->arg38;
}
void cmd_rec_set_activity(void* rec, int act) { as_rec(rec)->grant_act = act; }
uint32_t cmd_rec_state(const void* rec) { return cas_rec(rec)->state; }
void* cmd_rec_next(const void* rec) { return cas_rec(rec)->next; }

float cmd_rec_move_x(const void* rec) { return cas_rec(rec)->x; }
float cmd_rec_move_y(const void* rec) { return cas_rec(rec)->y; }
void cmd_rec_move_xy_set(void* rec, float x, float y)
{
    if (rec != NULL) {
        as_rec(rec)->x = x;
        as_rec(rec)->y = y;
    }
}
float cmd_rec_now(const void* rec) { return cas_rec(rec)->now; }
void cmd_rec_set_now(void* rec, float v) { as_rec(rec)->now = v; }
void cmd_rec_set_next(void* rec, void* next) { as_rec(rec)->next = next; }

/* ------------------------------------------------------------------ *
 *  Pool (CmdPoolGrow/CmdAlloc/CmdFree).
 * ------------------------------------------------------------------ */

#define CMD_POOL_MAX_BLOCKS 12
/* Pool bookkeeping is the corpus's DAT_004b31c8 band: CmdPoolGrow stores
 * each block at [level*4+0x4b31c8] and the level at DAT_004b31fc;
 * CmdAlloc/CmdFree pop and push the freelist through DAT_004b31f8
 * (records link at +0x64). All three are zero-init in the corpus.
 * On the 64-bit host test build, keep the pointer-sized `next` link because
 * the private +0x64 word is only 32 bits; the 32-bit target uses grant_act. */
#if defined(_M_IX86) || defined(__i386__)
#define CMD_POOL_LINK_GET(rec) \
    ((CmdRec*)(uintptr_t)(uint32_t)(rec)->grant_act)
#define CMD_POOL_LINK_SET(rec, value) \
    ((rec)->grant_act = (int32_t)(uintptr_t)(value))
#else
#define CMD_POOL_LINK_GET(rec) ((rec)->next)
#define CMD_POOL_LINK_SET(rec, value) ((rec)->next = (value))
#endif
#if defined(LEGORRR_BACKEND_WINAPI)
/* Layout control (the frontend.cpp/audio.cpp data_seg precedent): the
 * release loop's bound s_blocks + CMD_POOL_MAX_BLOCKS materializes as an
 * absolute operand Reccmp must resolve through s_free's annotation at
 * 0x4b31f8, so s_free has to own the dword right after the twelve table
 * entries. The default .bss/.data split let s_upgrade_names land there
 * instead, so the trio rides one zero-initialized named segment in
 * declaration order. */
#pragma data_seg(".data$cmdpool")
#endif
// GLOBAL: LEGORR 0x004b31c8
static CmdRec* s_blocks[CMD_POOL_MAX_BLOCKS] = { 0 };
/* The corpus table ends at the free-list head: HandleArrayReleaseAndReset
 * advances four bytes at a time and compares esi with 0x4b31f8. Keep this
 * declaration immediately after the 12 table entries. */
// GLOBAL: LEGORR 0x004b31f8
static void*   s_free       = NULL;
// GLOBAL: LEGORR 0x004b31fc
static int     s_pool_level = 0;
#if defined(LEGORRR_BACKEND_WINAPI)
#pragma data_seg()
#endif
/* CmdAlloc writes a self-pointer to retail +0x64; CmdFree replaces it with a
 * freelist link. The host record reuses that dword for other staging, so keep
 * live membership in an index-parallel sidecar instead of inferring it from
 * the freelist or treating the record as an EntitySlot. */
static uint8_t* s_block_live[CMD_POOL_MAX_BLOCKS] = { 0 };

static void cmd_pool_record_live_set(CmdRec* record, int live)
{
    int block_index;
    const uintptr_t address = (uintptr_t)record;
    for (block_index = 0; block_index < s_pool_level; ++block_index) {
        CmdRec* const block = s_blocks[block_index];
        const uint32_t capacity = 1u << (block_index & 0x1f);
        const uintptr_t start = (uintptr_t)block;
        const uintptr_t end = start +
            (uintptr_t)capacity * (uintptr_t)CMD_STRIDE;
        if (block != NULL && address >= start && address < end) {
            const uintptr_t offset = address - start;
            if (offset % (uintptr_t)CMD_STRIDE == 0u &&
                s_block_live[block_index] != NULL) {
                s_block_live[block_index][offset / CMD_STRIDE] =
                    live != 0 ? 1u : 0u;
            }
            return;
        }
    }
}

// GLOBAL: LEGORR 0x004b3354
static CmdRec* s_global_head = NULL; /* DAT_004b3354 */
static CmdRec* s_global_tail = NULL; /* DAT_004b3358: independent pending-list head */
static GameContext* s_ctx = NULL;

namespace unported_ledger_stubs {
int ledger_stub_00404fc0_PlaceCommandEligibilityCheck(
    const ObjectTaskEligibilityRecord* command, EntitySlot* entity,
    float screen_xy[2], float world_xy[2], uint32_t* out_flag,
    int require_command_gate, int relaxed_entity_gate);
void ledger_stub_00404ed0_CheckObjectCandidate(const EntitySlot* candidate);
int ledger_stub_0044b4c0_TypeTwoEntityTargetResolveAndApply(
    EntitySlot* entity, uint32_t code);
void* ledger_stub_00438e50_SpatialObjectQueryCenterMask100(
    const EntitySlot* entity, const float center[2], uint32_t extra);
}

static unported_ledger_stubs::ObjectTaskEligibilityOps s_place_eligibility_ops;
static EntityRayHitSweepOps s_place_ray_hit_ops;
static int pending_command_cell_walkable(GameContext* context,
                                         const EntitySlot* entity,
                                         int x, int y, int mode);

typedef struct CmdRayCandidateWalk {
    GameContext* game;
    EntityRayHitSweepState* state;
} CmdRayCandidateWalk;

static int cmd_place_ray_object_position_get(void*, void* object,
                                            float position[3])
{
    return entity_world_center_position_read((const EntitySlot*)object,
                                             position);
}

static int cmd_place_ray_see_through_walls_enabled(void*)
{
    return s_ctx != NULL && s_ctx->cfg != NULL &&
           block_get_bool(s_ctx->cfg, "SeeThroughWalls") == 1;
}

static int cmd_place_ray_grid_dimensions_get(void*, uint32_t* width,
                                             uint32_t* height)
{
    TerrainSurfaceGrid* grid = path_record_surface_grid_get();
    if (grid == NULL || width == NULL || height == NULL ||
        grid->surface_vectors == NULL) return 0;
    *width = grid->width;
    *height = grid->height;
    return grid->width != 0u && grid->height != 0u;
}

static int cmd_place_ray_world_to_cell(void*, const float position[3],
                                       uint32_t* x, uint32_t* y)
{
    int cell_x, cell_y;
    if (s_ctx == NULL || s_ctx->level == NULL || position == NULL ||
        x == NULL || y == NULL ||
        !terrain_world_xy_to_cell(s_ctx->level->ptr_terrain, position[0],
                                  position[1], &cell_x, &cell_y) ||
        cell_x < 0 || cell_y < 0) return 0;
    *x = (uint32_t)cell_x;
    *y = (uint32_t)cell_y;
    return 1;
}

static int cmd_place_ray_cell_intersection(void*, uint32_t x, uint32_t y,
                                           const float start[3],
                                           const float direction[3],
                                           float hit[3])
{
    TerrainSurfaceGrid* grid = path_record_surface_grid_get();
    return grid != NULL && x <= 0x7fffffffu && y <= 0x7fffffffu
        ? terrain_cell_ray_hit_test(grid, (int)x, (int)y, start, direction,
                                    hit)
        : 0;
}

/* LAB_00471370: the scene callback receives each live entity and the ray
 * record. It skips the ray source, projects the entity center onto the
 * horizontal ray, tests the Stats radius at +0x6c, then checks the ray's Z
 * against the terrain-to-Stats-height interval (Stats +0x74). The callback
 * always returns zero so the pool walker examines every candidate. The ray
 * uses the game's logical [X,Y,height] basis; terrain geometry stores the
 * same horizontal axes at vertex components 0/2 and height at component 1. */
// FUNCTION: LEGORR 0x00471370
static int cmd_place_ray_candidate_check(EntitySlot* entity, void* payload)
{
    CmdRayCandidateWalk* walk = (CmdRayCandidateWalk*)payload;
    EntityRayHitSweepState* state;
    float candidate_xy[2];
    float dx, dy, dz;
    float horizontal_length;
    float along;
    float projected_x, projected_y;
    float perp_x, perp_y;
    float radius;
    float height;
    float ground;
    float entry_distance;
    float entry_fraction;
    if (walk == NULL || (state = walk->state) == NULL || entity == NULL ||
        entity == state->ignored_object || walk->game == NULL ||
        walk->game->level == NULL ||
        walk->game->level->ptr_terrain == NULL || state->start == NULL ||
        state->direction == NULL) return 0;

    dx = state->direction[0];
    dy = state->direction[1];
    dz = state->direction[2];
    horizontal_length = (float)sqrt(dx * dx + dy * dy);
    if (horizontal_length <= 0.00001f) return 0;

    entity_transform_position_get(entity, candidate_xy);
    along = (candidate_xy[0] - state->start[0]) * dx / horizontal_length +
            (candidate_xy[1] - state->start[1]) * dy / horizontal_length;
    projected_x = state->start[0] + along * dx / horizontal_length;
    projected_y = state->start[1] + along * dy / horizontal_length;
    perp_x = candidate_xy[0] - projected_x;
    perp_y = candidate_xy[1] - projected_y;
    radius = entity_stats_float_at6c(entity);
    if (radius <= 0.0f || perp_x * perp_x + perp_y * perp_y >=
        radius * radius) return 0;

    entry_distance = along - (float)sqrt(
        radius * radius - perp_x * perp_x - perp_y * perp_y);
    /* A segment that starts inside the candidate's radius reports an
     * immediate hit, after the same vertical interval check at t = 0. */
    entry_fraction = entry_distance > 0.0f
        ? entry_distance / horizontal_length : 0.0f;
    if (entry_fraction >= state->max_distance)
        return 0;

    height = entity_stats_float_at74(entity);
    ground = terrain_surface_height_interpolate(
        walk->game->level->ptr_terrain, candidate_xy[0], candidate_xy[1]);
    /* 0x44f940 returns the terrain quad's vertical component directly.
     * Scene rays store that same vertical value in component +2; components
     * 0/1 are the horizontal pair corresponding to terrain X/Z. */
    if (height <= 0.0f ||
        state->start[2] + dz * entry_fraction > ground + height ||
        state->start[2] + dz * entry_fraction < ground) return 0;

    state->hit_object = entity;
    state->max_distance = entry_fraction;
    state->hit = 1;
    return 0;
}

static void cmd_place_ray_scene_trace_prepare(
    void* context, EntityRayHitSweepState* state)
{
    CmdRayCandidateWalk walk;
    walk.game = (GameContext*)context;
    walk.state = state;
    if (walk.game != NULL && state != NULL)
        (void)entity_pool_find_by_callback(walk.game,
                                           cmd_place_ray_candidate_check,
                                           &walk, 1);
}

static void cmd_place_ray_hit_ops_bind(void)
{
    memset(&s_place_ray_hit_ops, 0, sizeof(s_place_ray_hit_ops));
    s_place_ray_hit_ops.context = s_ctx;
    s_place_ray_hit_ops.object_position_get =
        cmd_place_ray_object_position_get;
    s_place_ray_hit_ops.see_through_walls_enabled =
        cmd_place_ray_see_through_walls_enabled;
    s_place_ray_hit_ops.scene_trace_prepare =
        cmd_place_ray_scene_trace_prepare;
    s_place_ray_hit_ops.grid_dimensions_get =
        cmd_place_ray_grid_dimensions_get;
    s_place_ray_hit_ops.world_to_cell = cmd_place_ray_world_to_cell;
    s_place_ray_hit_ops.cell_intersection =
        cmd_place_ray_cell_intersection;
    /* The entity pool is the live host-side scene candidate list; terrain
     * triangles remain the second pass in the retail trace. */
    ray_hit_cell_sweep_ops_set(&s_place_ray_hit_ops);
}

static int cmd_place_entity_view_get(
    void*, const EntitySlot* entity,
    unported_ledger_stubs::ObjectTaskEligibilityEntityView* view)
{
    const EntitySlot* nested;
    if (entity == NULL || view == NULL) return 0;
    memset(view, 0, sizeof(*view));
    view->type = entity->type;
    view->flags_3e8 = entity->flags_3e8;
    view->flags_f8 = entity->flags_3e0;
    view->flags_f9 = entity->flags_3e4;
    view->flags_fb = entity->flags_3ec;
    view->flags_fc = entity->flags_3f0;
    view->field_bc = entity->cmd_head != NULL;
    view->field_db = entity->owner_36c != NULL;
    view->field_d0 = entity->field_340;
    view->field_d1 = entity->progress_344;
    view->field_f5 = entity->activity_counter_3d4;
    view->activity_flags = entity_stats_word_get(entity);
    nested = (const EntitySlot*)entity->carry_link_308;
    view->nested_type_2fc = nested != NULL ? nested->type : 0;
    /* CheckObjectCandidate reads retail +0xcc, mirrored by the indexed
     * selector sidecar at +0x330. Activity-list capacity is a different
     * Stats field and must not be substituted here. */
    view->task_owner_cc = (uint32_t)entity->indexed_record_slot_330;
    return 1;
}

static int cmd_place_target_position_get(void*, const EntitySlot* entity,
                                         float* x, float* y)
{
    float position[2];
    if (entity == NULL || x == NULL || y == NULL) return 0;
    entity_transform_position_get(entity, position);
    *x = position[0];
    *y = position[1];
    return 1;
}

static int cmd_place_target_cell_position_get(void*, const EntitySlot* entity,
                                              float* x, float* y)
{
    int cell[2] = { -1, -1 };
    if (entity == NULL || x == NULL || y == NULL || s_ctx == NULL ||
        s_ctx->level == NULL) return 0;
    entity_position_to_terrain_cell(entity, s_ctx->level, &cell[0], &cell[1]);
    if (!level_coord_pair_within_bounds(s_ctx->level, cell[0], cell[1]))
        return 0;
    *x = (float)cell[0];
    *y = (float)cell[1];
    return 1;
}

static void* cmd_place_visual_resource_get(const EntitySlot* entity)
{
    EntitySlot* mutable_entity = (EntitySlot*)entity;
    void* resource;
    if (mutable_entity == NULL || mutable_entity->type < 1 ||
        mutable_entity->type > 4) return NULL;
    resource = mutable_entity->visual_resource_handles[
        mutable_entity->type - 1];
    if (resource != NULL) return resource;
    /* The type-config owners retain native scene pointers even when the
     * legacy renderer-record handle has not been installed. Those are the
     * same loaded scene objects consumed by the type-specific resource
     * helpers, so they can answer the position-only command branches. */
    if (mutable_entity->type == 1 &&
        entity_type1_scene_resource_bind(mutable_entity)) {
        resource = mutable_entity->scene_resource_type1.scene_resource;
        if (resource != NULL) return resource;
    }
    if ((mutable_entity->type == 2 || mutable_entity->type == 3) &&
        entity_activity_runner_type_config_bind(mutable_entity)) {
        EntityActivityRunnerSidecar* runner = mutable_entity->type == 2
            ? &mutable_entity->activity_runner_type2
            : &mutable_entity->activity_runner_type3;
        if (runner->child_scene != NULL) return runner->child_scene;
    }
    entity_visual_resource_get_or_create(mutable_entity);
    return mutable_entity->visual_resource_handles[
        mutable_entity->type - 1];
}

static int cmd_place_target_kind_resolve(void*, const EntitySlot* entity)
{
    return cmd_place_visual_resource_get(entity) != NULL;
}

static int cmd_place_target_route_position_get(void*,
                                               const EntitySlot* entity,
                                               float* x, float* y)
{
    float position[3];
    void* resource;
    if (x == NULL || y == NULL) return 0;
    resource = cmd_place_visual_resource_get(entity);
    if (resource == NULL ||
        !ddiscene_object_position_get(resource, NULL, position)) return 0;
    *x = position[0];
    *y = position[1];
    return 1;
}

static int cmd_place_target_activity_position_get(void*,
                                                  const EntitySlot* entity,
                                                  float* x, float* y)
{
    int cell[2];
    if (entity == NULL || x == NULL || y == NULL ||
        !entity_record_flag20_query(entity, cell)) return 0;
    *x = (float)cell[0];
    *y = (float)cell[1];
    return 1;
}

static int cmd_place_target_nested_scene_pose_get(const EntitySlot* entity,
                                                  float position[3])
{
    const EntitySlot* nested;
    void* scene_object;
    if (entity == NULL || position == NULL ||
        (entity->flags_3ec & 0x40u) == 0u || entity->link_2fc == 0u)
        return 0;
    nested = (const EntitySlot*)entity->link_2fc;
    if (nested->type != 4 ||
        nested->attached_direction_record_type4 == NULL) return 0;
    /* FUN_00404fc0 resolves nested type-4 field +0x18 through
     * SceneObjectSecondaryHandleGetOrCreate (FUN_004087e0), then reads that
     * resolved object's scene position. */
    scene_object = scene_object_secondary_handle_get_or_create(
        nested->attached_direction_record_type4);
    return scene_object != NULL &&
           ddiscene_object_position_get(scene_object, NULL, position);
}

static int cmd_place_position_to_world(void*, int x, int y,
                                       float* world_x, float* world_y)
{
    float position[2];
    if (s_ctx == NULL || s_ctx->level == NULL || world_x == NULL ||
        world_y == NULL || !terrain_cell_world_center_xy_get(
            s_ctx->level->ptr_terrain, x, y, position)) return 0;
    *world_x = position[0];
    *world_y = position[1];
    return 1;
}

static int cmd_place_terrain_walkable(void*, const EntitySlot* entity,
                                      int x, int y, int mode)
{
    return pending_command_cell_walkable(s_ctx, entity, x, y, mode);
}

static int pending_command_activity_gate(const EntitySlot* entity, int kind);

static int cmd_place_command_kind_gate(void*, const EntitySlot* entity,
                                      int kind)
{
    return pending_command_activity_gate(entity, kind);
}

static int cmd_place_activity_flags(void*, const EntitySlot* entity)
{
    return (int)entity_stats_word_get(entity);
}

static int cmd_place_target_type4_activity_get(void*,
                                              const EntitySlot* entity,
                                              int* value_out)
{
    if (entity == NULL || value_out == NULL) return 0;
    *value_out = (entity_capability_flags_get(entity) & 2u) != 0u;
    return 1;
}

static int cmd_place_eligibility_gate(void*, const EntitySlot* entity, int relaxed)
{
    return entity_shared_eligibility_check(entity, relaxed);
}

static int cmd_place_entity_task_gate(void*, const EntitySlot* entity,
                                      const EntitySlot*)
{
    return scene_object_has_task_type_4_5_7(entity);
}

static int cmd_place_kind1_activity_gate(void*, const EntitySlot* entity)
{
    /* FUN_00404fc0's type-1 kind-3 predicate calls FUN_00469bb0, which is
     * the indexed Stats record's +0xec deposit count. */
    return entity_stats_word_at_ec(entity) != 0u;
}

static int cmd_place_object_radius_gate(void*, const EntitySlot* entity,
                                       const EntitySlot* target)
{
    /* FUN_00472130 first checks FUN_0043c440's activity class (4/5/7),
     * which differs from the cheat-gate helper FUN_0044c530. */
    return ai_activity_probe(entity) != 0 &&
           entity_centers_within_object_radius(entity, target);
}

static int cmd_place_entity_can_target_kind(void*, const EntitySlot* entity,
                                            int kind)
{
    return unported_ledger_stubs::
        ledger_stub_0044b4c0_TypeTwoEntityTargetResolveAndApply(
            (EntitySlot*)entity, (uint32_t)kind);
}

static int cmd_place_target_activity_gate(void*, const EntitySlot* entity)
{
    return s_ctx != NULL && ai_activity_cap(s_ctx, entity) != 0;
}

static int cmd_place_target_visibility_check(void*, const EntitySlot* entity,
                                             int activity)
{
    /* FUN_00404fc0's kind-0x13 arm rejects when the activity scan finds a
     * matching active record. */
    return ai_activity_scan(entity, activity) == 0;
}

static EntitySlot* cmd_place_kind13_target_resolve(void*,
                                                   const EntitySlot* entity,
                                                   uint32_t activity)
{
    float position[2];
    (void)activity; /* Activity filtering is performed by visibility above. */
    if (entity == NULL) return NULL;
    entity_transform_position_get(entity, position);
    return entity_lookup_at_world_position(position);
}

static int cmd_place_ray_gate(void*, const EntitySlot* entity,
                              const EntitySlot* target)
{
    return see_through_walls_ray_check((void*)entity, (void*)target);
}

static int cmd_place_kind3_target_gate(void*, const EntitySlot* entity,
                                       const EntitySlot* target, int extra)
{
    return entity_accepts_target_for_task_check(entity, target, extra);
}

static int cmd_place_special_gate(void*, const EntitySlot* entity)
{
    return ai_can_do(entity);
}

static int cmd_place_task_coordinate_check(void*, int x, int y)
{
    (void)x;
    (void)y;
    /* FUN_00432550 (called for kind 0x16) unconditionally returns 1. */
    return 1;
}

static float cmd_place_simulation_time_get(void*)
{
    return s_ctx != NULL && s_ctx->level != NULL
        ? engine_float_field260_get(s_ctx->level) : 0.0f;
}

static void cmd_place_eligibility_ops_bind(void)
{
    memset(&s_place_eligibility_ops, 0, sizeof(s_place_eligibility_ops));
    s_place_eligibility_ops.context = s_ctx;
    s_place_eligibility_ops.activity_scale =
        s_ctx != NULL && s_ctx->cfg != NULL
        ? block_get_float(s_ctx->cfg, "Main MinEnergyForEat", 0.0f)
        : 0.0f;
    s_place_eligibility_ops.entity_view_get = cmd_place_entity_view_get;
    s_place_eligibility_ops.global_feature_word =
        s_ctx != NULL ? &s_ctx->cheat : NULL;
    s_place_eligibility_ops.entity_special_gate = cmd_place_special_gate;
    s_place_eligibility_ops.kind1_activity_gate =
        cmd_place_kind1_activity_gate;
    s_place_eligibility_ops.entity_can_target_kind =
        cmd_place_entity_can_target_kind;
    s_place_eligibility_ops.kind3_target_gate = cmd_place_kind3_target_gate;
    s_place_eligibility_ops.entity_target_gate = cmd_place_entity_task_gate;
    s_place_eligibility_ops.entity_radius_gate = cmd_place_object_radius_gate;
    s_place_eligibility_ops.entity_ray_gate = cmd_place_ray_gate;
    s_place_eligibility_ops.entity_eligible = cmd_place_eligibility_gate;
    s_place_eligibility_ops.target_activity_flags = cmd_place_activity_flags;
    s_place_eligibility_ops.target_type4_activity_get =
        cmd_place_target_type4_activity_get;
    s_place_eligibility_ops.target_activity_gate =
        cmd_place_target_activity_gate;
    s_place_eligibility_ops.target_visibility_check =
        cmd_place_target_visibility_check;
    s_place_eligibility_ops.kind13_target_resolve =
        cmd_place_kind13_target_resolve;
    s_place_eligibility_ops.target_kind_resolve =
        cmd_place_target_kind_resolve;
    s_place_eligibility_ops.target_position_get =
        cmd_place_target_position_get;
    s_place_eligibility_ops.position_to_world = cmd_place_position_to_world;
    s_place_eligibility_ops.terrain_walkable = cmd_place_terrain_walkable;
    s_place_eligibility_ops.command_kind_gate =
        cmd_place_command_kind_gate;
    s_place_eligibility_ops.target_route_position_get =
        cmd_place_target_route_position_get;
    s_place_eligibility_ops.target_activity_position_get =
        cmd_place_target_activity_position_get;
    s_place_eligibility_ops.target_world_position_get =
        cmd_place_target_cell_position_get;
    s_place_eligibility_ops.task_coordinate_check =
        cmd_place_task_coordinate_check;
    s_place_eligibility_ops.simulation_time_get =
        cmd_place_simulation_time_get;
    unported_ledger_stubs::object_task_eligibility_ops_bind(
        &s_place_eligibility_ops);
    cmd_place_ray_hit_ops_bind();
}

int cmd_place_command_eligibility_check(void* record, EntitySlot* entity,
                                        float position_xy[2],
                                        float cell_xy[2], uint32_t* out_flag,
                                        int require_command_gate,
                                        int relaxed_entity_gate)
{
    CmdRec* source = as_rec(record);
    unported_ledger_stubs::ObjectTaskEligibilityRecord command;
    if (source == NULL) return 0;
    memset(&command, 0, sizeof(command));
    command.kind = source->type;
    command.target = (EntitySlot*)source->payload;
    /* The retail predicate reads these words as integers for cell commands;
     * they are not float world coordinates despite sharing +8/+0xc with
     * move records. */
    cmd_rec_cell_xy_get(source, &command.x, &command.y);
    command.flags_5c = source->state;
    command.field_38 = (uint32_t)(uintptr_t)source->arg38;
    command.field_3c = source->field_3c;
    command.field_5c = source->state;
    command.field_17 = source->state;
    command.field_e = (uint32_t)(uintptr_t)source->arg38;
    command.field_f = source->field_3c;
    command.task_scalar_18 = *(float*)&source->param2;
    return unported_ledger_stubs::ledger_stub_00404fc0_PlaceCommandEligibilityCheck(
        &command, entity, position_xy, cell_xy, out_flag,
        require_command_gate, relaxed_entity_gate);
}

/* The callback writes independent 50-entry arrays at +0x335c and +0x3428.
 * Assignment and timer scans consume them with separate swap-with-last
 * rules. The pool builder below only supports direct scan entry calls. */
#define PENDING_CANDIDATE_CAPACITY 50
static EntitySlot* s_pending_assignment_candidates[PENDING_CANDIDATE_CAPACITY];
static EntitySlot* s_pending_timer_candidates[PENDING_CANDIDATE_CAPACITY];
static unsigned s_pending_assignment_count;
// GLOBAL: LEGORR 0x004b34f0
static unsigned s_pending_timer_count;
/* CmdTickGlobal builds these two retail arrays during its preceding filtered
 * entity walk. Direct callers of the scans still get a live-pool fallback. */
static int s_pending_candidates_from_entity_walk;

static GameContext* pending_scan_context_get(void)
{
    return s_ctx != NULL ? s_ctx : entity_pool_callback_context_get();
}

static unsigned pending_candidate_pool_build(GameContext* context,
                                              EntitySlot** output,
                                              int filter_flagged)
{
    unsigned count = 0;
    int index;
    if (context == NULL || output == NULL) return 0;
    if (filter_flagged != 0) {
        const int candidate_count = entity_count(context, 3);
        for (index = 0; index < candidate_count &&
                            count < PENDING_CANDIDATE_CAPACITY; ++index) {
            EntitySlot* entity = (EntitySlot*)entity_at(context, 3, index);
            /* The gameplay builder adds timer candidates only from the
             * type-3, no-head arm of LAB_00404280. The surrounding filtered
             * entity walk excludes +0x3e8 bit 0x80000; the callback then
             * excludes +0x3e8 bit 0x80000000. Mirror both gates for direct
             * calls of the timer scan when CmdTickGlobal did not build its
             * candidate array. */
            if (entity == NULL || entity->type != 3 ||
                entity->ddiscene_owner_token == 0u ||
                entity->cmd_head != NULL ||
                (entity->flags_3e8 & (0x00080000u | 0x80000000u)) != 0u ||
                (entity->flags_3e8 & 0x00400000u) == 0u ||
                entity->field_340 <= 0.0f ||
                entity_matches_current_selection_or_owner(entity) != 0 ||
                entity->attach_304 != NULL ||
                (entity->flags_3e4 & 0x08004000u) != 0u ||
                (entity->flags_3e0 & 0x00800000u) != 0u ||
                entity->command_chain_idle_elapsed_338 >= 250.0f ||
                entity_flag400_and_health_threshold_check(entity) != 0)
                continue;
            output[count++] = entity;
        }
        return count;
    }
    {
        int active_count = active_entity_list_count();
        if (active_count > PENDING_CANDIDATE_CAPACITY)
            active_count = PENDING_CANDIDATE_CAPACITY;
        for (index = 0; index < active_count; ++index) {
            const EntitySlot* entity = active_entity_list_entry_get(index);
            if (entity != NULL) output[count++] = (EntitySlot*)entity;
        }
    }
    return count;
}

static int pending_record_matches_entity(const CmdRec* record,
                                         const EntitySlot* entity)
{
    if (record == NULL || entity == NULL) return 0;
    if ((record->state & 0x100u) != 0u) {
        return cmd_array_find_value_index((const int*)record->buffer,
                                          record->count,
                                          (int)(uintptr_t)entity, NULL);
    }
    /* FUN_00452610(entity, 0): the +0x3e8 entity flag word, bit 0x200000. */
    return (entity->flags_3e8 & 0x200000u) != 0u;
}

static int cmd_entity_global_record_special_step(EntitySlot* entity,
                                                  CmdRec* record,
                                                  CmdRec* previous);

/* LAB_00404280's DAT_004b3354 subwalk. The head-list is owned by CmdRec and
 * uses the same host pointer sidecar as the other command-list operations.
 * This is the first assignment lane: it only considers non-expired global
 * records, applies the retail membership form, then checks Place eligibility
 * and exact cell/adjacency before attaching the first matching record. */
static int cmd_entity_global_record_assign(EntitySlot* entity)
{
    GameContext* context = pending_scan_context_get();
    CmdRec* previous = NULL;
    CmdRec* record = s_global_head;
    while (record != NULL) {
        CmdRec* next = as_rec(record->global_next);
        int eligible = 1;
        if ((record->state & 0x80000u) != 0u) {
            eligible = 0;
        }
        else if ((record->state & 0x100000u) == 0u &&
                 (entity_flag400_and_health_threshold_check(entity) != 0 ||
                  (record->state & 0x400u) != 0u)) {
            eligible = 0;
        }
        if (eligible && (record->state & 0x100u) == 0u &&
            selection_contains_entity_target(NULL, entity, NULL) != 0) {
            eligible = 0;
        }
        if (eligible && (record->state & 0x100u) != 0u &&
            !cmd_array_find_value_index((const int*)record->buffer,
                                        record->count,
                                        (int)(uintptr_t)entity, NULL)) {
            eligible = 0;
        }
        if (eligible) {
            float eligibility_cell[2] = { 0.0f, 0.0f };
            uint32_t out_flag = 0u;
            int actor_cell[2] = { -1, -1 };
            int target_x, target_y;
            if (!cmd_place_command_eligibility_check(
                    record, entity, NULL, eligibility_cell, &out_flag, 1, 1) ||
                out_flag != 0u) {
                eligible = 0;
            }
            else if ((record->state & 0x100000u) == 0u) {
                entity_position_to_terrain_cell(entity,
                    context != NULL ? context->level : NULL,
                    &actor_cell[0], &actor_cell[1]);
                target_x = crt_float_to_long(eligibility_cell[0]);
                target_y = crt_float_to_long(eligibility_cell[1]);
                if (actor_cell[0] < 0 || actor_cell[1] < 0) {
                    eligible = 0;
                }
                else if (record->type == 8 &&
                         (record->state & 8u) == 0u) {
                    if (!((actor_cell[0] == target_x &&
                           abs(actor_cell[1] - target_y) == 1) ||
                          (actor_cell[1] == target_y &&
                           abs(actor_cell[0] - target_x) == 1)))
                        eligible = 0;
                }
                else if (record->type == 10 || record->type == 11 ||
                         record->type == 23 || record->type == 21) {
                    if (!((actor_cell[0] == target_x &&
                           abs(actor_cell[1] - target_y) == 1) ||
                          (actor_cell[1] == target_y &&
                           abs(actor_cell[0] - target_x) == 1)))
                        eligible = 0;
                }
                else if (actor_cell[0] != target_x ||
                         actor_cell[1] != target_y) {
                    eligible = 0;
                }
            }
        }
        if (eligible) {
            entity_record_attach_and_relink(record, previous, entity);
            return 1;
        }
        {
            const int special = cmd_entity_global_record_special_step(
                entity, record, previous);
            if (special != 0) return special;
        }
        previous = record;
        record = next;
    }
    return 0;
}

/* The flag-0x400 half of LAB_00404280 handles carried-object task records
 * and checks whether the entity can reach their encoded cell. */
static int cmd_entity_global_record_special_step(EntitySlot* entity,
                                                  CmdRec* record,
                                                  CmdRec* previous)
{
    EntitySlot* carried = (EntitySlot*)entity->carry_link_308;
    int route_lane = 0;
    int route_failed = 0;
    int goal[2];

    if ((record->state & 0x400u) == 0u) return 0;
    if (entity_flag400_and_health_threshold_check(entity) != 0 &&
        (entity->flags_3e8 & 0x200000u) == 0u && entity->type != 3) {
        if (carried != NULL &&
            (carried->flags_3e8 & 0x80000000u) != 0u) {
            if (record->type == 0x1b) {
                entity_record_attach_and_relink(record, previous, entity);
                return 1;
            }
        }
        else if (carried != NULL &&
                 carried->indexed_record_slot_330 == record->cell_count &&
                 (record->cell_target == -1 ||
                  carried->etype == record->cell_target) &&
                 carried->type == record->type) {
            switch (record->type) {
            case 7:
            case 9:
            case 15:
            case 20:
                route_lane = 1;
                break;
            case 5:
                if (record->payload != NULL &&
                    entity_shared_eligibility_check(
                        (EntitySlot*)record->payload, 0) != 0) {
                    if (entity_deposit_activity_gate(
                            entity, (EntitySlot*)record->payload) != 0) {
                        entity_record_attach_and_relink(record, previous,
                                                        entity);
                        return 1;
                    }
                    route_failed = 1;
                }
                break;
            default:
                break;
            }
        }
    }

    if (route_lane) {
        LevelState* level = entity->owner_ctx != NULL
            ? entity->owner_ctx->level : NULL;
        int start[2] = { -1, -1 };
        int* path_x = NULL;
        int* path_y = NULL;
        uint32_t path_count = 0u;
        int found;
        (void)cmd_rec_cell_xy_get(record, &goal[0], &goal[1]);
        if (record->type == 9) {
            int neighbor[2] = { -1, -1 };
            if (level == NULL || !first_free_neighbor_cell_pick(
                    entity, level, goal, neighbor, 1)) {
                route_failed = 1;
            }
            else {
                goal[0] = neighbor[0];
                goal[1] = neighbor[1];
            }
        }
        entity_position_to_terrain_cell(entity, level, &start[0], &start[1]);
        if (!route_failed && level != NULL && start[0] >= 0 && start[1] >= 0) {
            found = terrain_neighbor_path_check(
                entity, (uint32_t)start[0], (uint32_t)start[1],
                (uint32_t)goal[0], (uint32_t)goal[1], &path_x, &path_y,
                &path_count, NULL, NULL);
            if (path_x != NULL) engine_heap_free(path_x);
            if (path_y != NULL) engine_heap_free(path_y);
            if (!found) route_failed = 1;
        }
        else if (!route_failed) {
            route_failed = 1;
        }
    }
    if (route_failed) record->param2 = (int32_t)0x42960000u;

    if ((record->state & 0x100u) != 0u &&
        cmd_array_find_value_index((const int*)record->buffer,
                                   record->count,
                                   (int)(uintptr_t)entity, NULL))
        return 2; /* stop record walk, then run unmatched-candidate gating */
    return 0;
}

static void cmd_entity_pending_candidates_append(EntitySlot* entity)
{
    if (entity == NULL ||
        entity_flag400_and_health_threshold_check(entity) != 0)
        return;
    if (s_pending_assignment_count < PENDING_CANDIDATE_CAPACITY) {
        s_pending_assignment_candidates[s_pending_assignment_count++] = entity;
    }
    if (entity->type == 3 &&
        (entity->flags_3e8 & 0x80000000u) == 0u &&
        s_pending_timer_count < PENDING_CANDIDATE_CAPACITY) {
        s_pending_timer_candidates[s_pending_timer_count++] = entity;
    }
}

/* FUN_0044b4c0: non-type-2 entities pass the command-activity gate. For a
 * type-2 entity, only command kinds present in the grouped code table require
 * the corresponding permitted activity. */
static int pending_command_activity_gate(const EntitySlot* entity, int kind)
{
    int activity_group;
    if (entity == NULL || entity->type != 2) return 1;
    if (!entity_grouped_code_table_find_value_index((uint32_t)kind,
                                                     &activity_group))
        return 1;
    return ai_activity_scan(entity, activity_group);
}

/* FUN_00432770, called by PlaceCommandEligibilityCheck for kind 8. The
 * simple mode checks the open-cell flags and excluded type bytes. The
 * alternate mode also checks the actor's stats/capability gates and the
 * four neighboring state/type bytes. LevelState owns the same 0x48-byte
 * voxel records, so this branch does not need renderer callbacks. */
static int pending_command_cell_walkable(GameContext* context,
                                         const EntitySlot* entity,
                                         int x, int y, int mode)
{
    const LevelState* level;
    const uint8_t* cell;
    uint32_t flags;
    int type;
    int i;
    static const int state_dx[4] = { 0, 1, 0, -1 };
    static const int state_dy[4] = { -1, 0, 1, 0 };
    static const int type_dx[4] = { -1, 0, 0, -1 };
    static const int type_dy[4] = { -1, -1, 0, 0 };

    if (context == NULL || context->level == NULL || entity == NULL)
        return 0;
    level = context->level;
    if ((entity_stats_word_get(entity) & 0x20000u) != 0u &&
        entity->owner_36c == NULL) return 0;
    if (mode != 0 && (entity_stats_word_get(entity) & 1u) == 0u)
        return 0;
    /* PlaceCommandEligibilityCheck passes param_5 == 1 to FUN_00432770. */
    if ((entity->flags_3e0 & 0x400u) != 0u) return 0;
    if ((entity->flags_3e8 & 2u) == 0u) return 0;
    if ((unsigned)x >= (unsigned)level_width(level) - 2u ||
        (unsigned)y >= (unsigned)level_height(level) - 2u) return 0;
    cell = level_voxel(level, x, y);
    if (cell == NULL) return 0;
    type = (int)(signed char)cell[2];
    if (entity_stats_float_at24_indexed(entity, type) == 0.0f) return 0;

    if (mode == 0) {
        memcpy(&flags, cell + 8, sizeof(flags));
        return (flags & 0x10u) != 0u && (flags & 0x40u) == 0u &&
               type != 1 && type != 7 && type != 11;
    }

    if (cell[0] != 0u) return 0;
    for (i = 0; i != 4; ++i) {
        const uint8_t* neighbor = level_voxel(level, x + state_dx[i],
                                              y + state_dy[i]);
        if (neighbor != NULL && neighbor[0] != 0u) break;
    }
    if (i == 4) return 0;
    for (i = 0; i != 4; ++i) {
        const uint8_t* neighbor = level_voxel(level, x + type_dx[i],
                                              y + type_dy[i]);
        if (neighbor != NULL && neighbor[2] != 1u && neighbor[2] != 7u)
            return 1;
    }
    return 0;
}
// GLOBAL: LEGORR 0x004b8060
static int   s_global_enabled = 0; /* DAT_004b8060: zero-init; cmd_init sets bit 0 */
#define PRIORITY_ENTRY_COUNT 27
// GLOBAL: LEGORR 0x004b7ff4
static int s_priority_state[PRIORITY_ENTRY_COUNT];
static CmdPurgeCellPredicate s_cmd_purge_cell_usable;
static CmdPurgeCellPredicate s_cmd_purge_neighbor_eligible;
static CmdPurgeCompare s_cmd_purge_compare;
static void* s_cmd_purge_context;

// FUNCTION: LEGORR 0x004064c0
static void cmd_pool_grow(void)
{
    int level = s_pool_level;
    if (level >= CMD_POOL_MAX_BLOCKS) {
        return;
    }
    uint32_t n = 1u << (level & 0x1f);
    CmdRec* block = (CmdRec*)malloc((size_t)n * CMD_STRIDE);
    uint8_t* live;
    if (block == NULL) {
        return;
    }
    live = (uint8_t*)calloc((size_t)n, sizeof(*live));
    if (live == NULL) {
        free(block);
        return;
    }
    s_blocks[level] = block;
    s_block_live[level] = live;
    ++s_pool_level;
    if (n > 1) {
        for (uint32_t i = 0; i + 1 < n; ++i) {
            CMD_POOL_LINK_SET(&block[i], &block[i + 1]);
        }
    }
    CMD_POOL_LINK_SET(&block[n - 1], s_free);
    s_free = block;
}

// FUNCTION: LEGORR 0x00406440
void* cmd_alloc(int type)
{
    if (s_free == NULL) {
        cmd_pool_grow();
    }
    if (s_free == NULL) {
        return NULL;
    }
    CmdRec* rec = (CmdRec*)s_free;
    s_free = CMD_POOL_LINK_GET(rec);
    memset(rec, 0, CMD_STRIDE);
    rec->next = NULL;
    rec->type = type;
    cmd_pool_record_live_set(rec, 1);
    return rec;
}

// FUNCTION: LEGORR 0x00406480
void cmd_free(void* rec, int cancel)
{
    /* corpus: no NULL-record path — the gate is the second argument
     * (test [esp+8] / jne) and the record is dereferenced blindly. */
    if (cancel == 0) {
        cmd_cancel_ref(rec); /* corpus: unless the argument suppresses it */
    }
    /* Keep the corpus's direct record dereference here; routing through
     * as_rec adds a call in this hot pool leaf. */
    CmdRec* r = (CmdRec*)rec;
    if (r->buffer != NULL && (r->state & CMD_STATE_BUFOUT) == 0) {
        engine_heap_free(r->buffer); /* corpus: push eax / call 0x48db80 */
        /* corpus: the pointer is never cleared — cmd_alloc zeroes the
         * whole record, so the next owner starts from NULL. */
    }
    cmd_pool_record_live_set(r, 0);
    /* corpus: load ds:0x4b31f8, link it at [rec+0x64], store the record
     * back as that head — s_free is this freelist head. */
    CMD_POOL_LINK_SET(r, s_free);
    s_free = r;
}
#undef CMD_POOL_LINK_GET
#undef CMD_POOL_LINK_SET

/* ------------------------------------------------------------------ *
 *  Registry (CmdRegistryInit / CmdKindByName): 31 AI task types and
 *  27 AI priorities, in the *address order* CmdRegistryInit writes them.
 * ------------------------------------------------------------------ */

static const char* const TASK_NAMES[31] = {
    /* slot = (addr - 0x4b3200)/4 */
    "Goto", "Follow", "FollowAttack", "Collect", "Gather", "Deposit",
    "Dump", "Request", "Dig", "Dynamite", "Repair", "Reinforce", "Clear",
    "Wait", "AnimationWait", "ElecFence", "Eat", "GotoEat", "FindDriver",
    "GetTool", "BirdScarer", "Upgrade", "BuildPath", "Train", "Depart",
    "AttackPath", "AttackRockMonster", "Recharge", "Dock", "AttackObject",
    "FindLoad",
};
static const char* const PRIO_NAMES[27] = {
    /* slot = (addr - 0x4b327c)/4 */
    "Crystal", "Ore", "DefaultCollect", "Destruction", "Construction",
    "Request", "Reinforce", "Repair", "Clearing", "Storage", "Refining",
    "HealthLow", "GetIn", "Upgrade", "BuildPath", "AttackRockMonster",
    "Barrier", "FindLoad", "Recharge", "UpgradeBuilding", "Gather", "Steal",
    "Punch", "Depart", "AttackPath", "AttackObject", "Congregate",
};
// GLOBAL: LEGORR 0x004b32e8
static int32_t s_behaviour[27]; /* DAT_004b32e8 by kind, DWORD stride */

// FUNCTION: LEGORR 0x00401cf0
void cmd_init(void)
{
    int i;
    for (i = 0; i < CMD_POOL_MAX_BLOCKS; ++i) {
        if (s_blocks[i] != NULL) free(s_blocks[i]);
        if (s_block_live[i] != NULL) free(s_block_live[i]);
        s_block_live[i] = NULL;
        s_blocks[i] = NULL;
    }
    s_pool_level = 0;
    s_free = NULL;
    s_global_head = NULL;
    s_global_tail = NULL;
    s_pending_assignment_count = 0;
    s_pending_timer_count = 0;
    s_global_enabled = 1;

    for (i = 0; i < 27; ++i) {
        s_behaviour[i] = 0x31;
    }
    s_behaviour[0]  = 0x37;
    s_behaviour[3]  = 0x28;
    s_behaviour[4]  = 0x3c;
    s_behaviour[5]  = 0x14;
    s_behaviour[6]  = 0x46;
    s_behaviour[8]  = 0x14;
    s_behaviour[9]  = 5;
    s_behaviour[10] = 0xf;
    s_behaviour[15] = 0x38;
    s_behaviour[19] = 99;
    s_behaviour[20] = 0x31;
    s_behaviour[21] = 0x5a;
    s_behaviour[22] = 0x31;
    s_behaviour[23] = 0;
    s_behaviour[24] = 0x14;
    s_behaviour[25] = 0x1e;
    s_behaviour[26] = 99;
}

// FUNCTION: LEGORR 0x00401cb0
int cmd_kind_by_name(const char* name, int* out)
{
    /* stride-4 name-table walk, as the corpus: unsigned end compare (the
     * loop-back is jb) with the index counter beside the pointer (edi) */
    const char* const* entry = PRIO_NAMES;
    const char* const* end = PRIO_NAMES + 27;   /* 0x4b32e8 - 0x4b327c */
    int i = 0;
    for (; entry < end; ++entry, ++i) {
        /* cdecl stack order: push the table entry, then the query name. */
        if (strcasecmp_lite(name, *entry) == 0) {
            *out = i;
            return 1;
        }
    }
    return 0;
}

int cmd_kind_behaviour(int kind)
{
    if (kind < 0 || kind >= 27) {
        return 0x31;
    }
    return s_behaviour[kind];
}

const char* cmd_task_name(int slot)
{
    return (slot >= 0 && slot < 31) ? TASK_NAMES[slot] : NULL;
}
const char* cmd_priority_name(int slot)
{
    return (slot >= 0 && slot < 27) ? PRIO_NAMES[slot] : NULL;
}

/* ------------------------------------------------------------------ *
 *  Link / chain primitives.
 * ------------------------------------------------------------------ */

static void cmd_bind_kind(CmdRec* rec, int kind)
{
    if ((rec->state & 0x30000u) == 0) {
        rec->behaviour = cmd_kind_behaviour(kind);
    } else {
        rec->behaviour = 99;
    }
    rec->kind = kind;
}

// FUNCTION: LEGORR 0x00404030
static void cmd_link_head(CmdRec* rec, int kind)
{
    rec->global_next = s_global_head;
    s_global_head = rec;
    rec->now = 1500.0f;
    rec->state = (rec->state & ~1u) | CMD_STATE_PENDING;
    cmd_bind_kind(rec, kind);
}

// FUNCTION: LEGORR 0x00404070
static void cmd_link_tail(CmdRec* rec)
{
    rec->global_next = s_global_tail;
    s_global_tail = rec;
    rec->now = 1500.0f;
    rec->state &= ~(CMD_STATE_PENDING | 0x10u);
}

// FUNCTION: LEGORR 0x00404190
void* cmd_chain_walk(const EntitySlot* e)
{
    if (e == NULL || e->cmd_head == NULL) {
        return NULL;
    }
    CmdRec* cur = (CmdRec*)e->cmd_head;
    for (;;) {
        if (cur->next == NULL) {
            return cur;
        }
        cur = (CmdRec*)cur->next;
    }
}

int cmd_chain_contains_kind(const EntitySlot* e, int kind)
{
    const CmdRec* cur;
    if (e == NULL) return 0;
    cur = (const CmdRec*)e->cmd_head;
    while (cur != NULL) {
        if (cur->type == kind) return 1;
        cur = (const CmdRec*)cur->next;
    }
    return 0;
}

static void idle_register(EntitySlot* e);

/* The 0x408d70/0x408ce0 helpers share the live path-descriptor list owned by
 * level.cpp. The descriptor key is its serial dword at +0x08; its +0x2c list
 * contains EntitySlot pointers, mirrored by the native sidecar accessors. */
static void* cmd_path_descriptor_find(uint32_t key)
{
    void* record;
    for (record = path_record_last_peek(); record != NULL;
         record = *(void**)((char*)record + 0x24)) {
        uint32_t record_key;
        memcpy(&record_key, (char*)record + 0x08, sizeof(record_key));
        if (record_key == key) return record;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00408CE0
static void cmd_operation_group_append_object(uint32_t key, EntitySlot* entity)
{
    void* descriptor = cmd_path_descriptor_find(key);
    uint32_t expected_count = 0u;
    uint32_t current_count = 0u;
    uint32_t flags = 0u;
    if (descriptor == NULL) {
        if (entity != NULL)
            cmd_produce(entity->type == 6 ? 0x16 : 0x1c,
                        entity, 0, NULL);
        return;
    }

    if (entity != NULL) {
        /* Retail sets +0x3ec bit 0x100 before appending the entity pointer.
         * The sidecar append mirrors the descriptor's +0xcc and +0x20 counts. */
        entity->flags_3ec |= 0x100u;
        if (!path_record_entity_append(descriptor, entity)) return;
    }

    memcpy(&expected_count, (char*)descriptor + 0x1c,
           sizeof(expected_count));
    memcpy(&current_count, (char*)descriptor + 0x20,
           sizeof(current_count));
    if (expected_count == current_count) {
        const float completed_progress = 1.3125f;
        memcpy((char*)descriptor + 0x28, &completed_progress,
               sizeof(completed_progress));
        memcpy(&flags, (char*)descriptor + 0xd0, sizeof(flags));
        flags |= 1u;
        memcpy((char*)descriptor + 0xd0, &flags, sizeof(flags));
    }
}

// FUNCTION: LEGORR 0x00403d10
void entity_command_unlink_by_kind(EntitySlot* e, int type,
                                   const void* extra, int unlink)
{
    if (e == NULL || (s_global_enabled & 2) != 0) return;

    /* Type 3 first reports the matching spatial object, before walking the
     * command chain. The retail extra is an EntitySlot pointer; +0xcc is its
     * indexed-record selector. */
    if (type == 3 && extra != NULL) {
        const EntitySlot* payload = (const EntitySlot*)extra;
        void* object = unported_ledger_stubs::
            ledger_stub_00438ae0_SpatialObjectQueryCenterMask02(
            e, NULL, (uint32_t)payload->type,
            payload->indexed_record_slot_330);
        if (object != NULL) cmd_produce(0x18, e, (int)(uintptr_t)object, NULL);
    }

    CmdRec* prev = NULL;
    CmdRec* cur = as_rec(e->cmd_head);
    while (cur != NULL) {
        CmdRec* next = as_rec(cur->next);
        if (cur->type == type) {
            int append_zone_object = 0;
            const uint32_t zone_key = (uint32_t)cur->zone;
            if (type == 7 && extra != NULL) {
                EntitySlot* object = (EntitySlot*)extra;
                if (terrain_descriptor_coordinate_count_limit_check(
                        (uint32_t)cur->zone, object->type,
                        object->etype)) {
                    append_zone_object = 1;
                }
                else {
                    cmd_cancel_by_coords(cur->zone, 1,
                                         object->type, object->etype);
                }
            }
            if (type == 0x0c) {
                int pair[2];
                LevelState* level = e->owner_ctx != NULL
                    ? e->owner_ctx->level : (s_ctx != NULL ? s_ctx->level : NULL);
                memcpy(&pair[0], &cur->x, sizeof(int));
                memcpy(&pair[1], &cur->y, sizeof(int));
                if (level != NULL) {
                    if (e->type == 1) {
                        uint8_t* cell = level_voxel(level, pair[0], pair[1]);
                        unsigned repeat = 0;
                        if (cell != NULL) {
                            uint32_t flags;
                            memcpy(&flags, cell + 8, sizeof(flags));
                            repeat = flags & 3u;
                        }
                        while (repeat-- != 0u) {
                            terrain_cell_state_update(level, pair);
                            cmd_produce((int)(uintptr_t)cur->event_payload,
                                        NULL, 0, pair);
                        }
                    }
                    terrain_cell_state_update(level, pair);
                    cmd_produce((int)(uintptr_t)cur->event_payload,
                                NULL, 0, pair);
                    level_cell_flag_80000_set(level, pair[0], pair[1], 0);
                    level_cell_flag_10000000_set(level, pair[0], pair[1], 0);
                }
            }
            else if (type == 0x17) {
                entity_linked_activity_event_dispatch(e, cur->field_3c);
                advisor_text_animation_update(0x18, 1, 0);
            }
            if (unlink) {
                cmd_global_unlink(cur);
                if (prev != NULL) prev->next = next;
                else e->cmd_head = next;
                cmd_free(cur, 0);
            }
            if (append_zone_object)
                cmd_operation_group_append_object(zone_key,
                                                  (EntitySlot*)extra);
        } else {
            prev = cur;
        }
        cur = next;
    }
    if (type == 3) pending_record_callback_dispatch((void*)extra);
}

int cmd_chain_drop(EntitySlot* e, int type)
{
    int before = 0;
    const CmdRec* record;
    if (e == NULL) return 0;
    for (record = (const CmdRec*)e->cmd_head; record != NULL;
         record = (const CmdRec*)record->next) {
        if (record->type == type) ++before;
    }
    entity_command_unlink_by_kind(e, type, NULL, 1);
    return before;
}

void* cmd_global_head(void)
{
    return s_global_head;
}

void* cmd_global_pending_head(void)
{
    return s_global_tail;
}

// FUNCTION: LEGORR 0x00405950
/* PendingCommandTimerFireScan (FUN_00405950). The decompile walks the
 * tail-linked list and the separate DAT_004b3428 candidate pool. CmdTickGlobal
 * builds that pool in LAB_00404280; direct calls reconstruct the no-head
 * type-3 subset from the live entity table, including the filtered-walk and
 * callback flag gates. Preserve the observed timer, per-kind predicate,
 * record unlink, entity-head install/copy, and swap-removal effects. */
void pending_command_timer_fire_scan(void)
{
    GameContext* context = pending_scan_context_get();
    CmdRec* previous = NULL;
    CmdRec* record = s_global_tail;
    if (!s_pending_candidates_from_entity_walk) {
        unsigned count = pending_candidate_pool_build(
            context, s_pending_timer_candidates, 1);
        s_pending_timer_count = count;
    }
    while (record != NULL) {
        CmdRec* next = as_rec(record->global_next);
        int removed = 0;
        if ((*(float*)&record->param2) <= 0.0f) {
            unsigned i = 0;
            for (i = 0; i < s_pending_timer_count; ++i) {
                EntitySlot* entity = s_pending_timer_candidates[i];
                int fire = 0;
                if (entity == NULL) continue;
                /* Kind 0x18 is tested after the shared stats-threshold
                 * block in FUN_00405950. Keep it eligible even when the
                 * +0xe8 value has reached the entity's +0x340 limit. */
                if (record->type == 0x18) {
                    fire = entity_type3_flag04000000_test(entity) ||
                           (entity_state_flags_get(entity) & 0x100000u) != 0u;
                }
                else if (!(entity_stats_float_ate8(entity) <
                           entity->field_340)) {
                    continue;
                }
                switch (record->type) {
                case 10:
                    if ((entity_state_flags_get(entity) & 0x80000u) == 0u ||
                        (record->payload != NULL &&
                         entity_shared_eligibility_check(
                             (EntitySlot*)record->payload, 0) != 0 &&
                         (entity_state_flags_get((EntitySlot*)record->payload)
                          & 0x400u) == 0u)) {
                        float entity_position[3];
                        float target_position[3];
                        if (record->payload != NULL) {
                            entity_transform_position_get(entity,
                                                          entity_position);
                            entity_transform_position_get(
                                (EntitySlot*)record->payload, target_position);
                            const float dx = entity_position[0] -
                                             target_position[0];
                            const float dy = entity_position[1] -
                                             target_position[1];
                            const float radius = entity_stats_float_at98(entity);
                            if (sqrt((double)(dx * dx + dy * dy)) < radius) {
                                fire = 1;
                                if ((entity_state_flags_get(entity) &
                                     0x80000u) == 0u)
                                    record->param2 = (int32_t)0x443b8000u;
                            }
                        }
                    }
                    break;
                case 0x19:
                    fire = (entity_state_flags_get(entity) & 0x4000u) != 0u;
                    break;
                case 4:
                    fire = entity_type3_flag04000000_test(entity) &&
                           grid_record_count_difference_sum(4, 0) != 0;
                    break;
                case 3:
                    fire = unported_ledger_stubs::
                        entity_capacity_or_state_predicate(entity);
                    if (fire) record->param2 = (int32_t)0x437a0000u;
                    break;
                case 0:
                    if (entity_type3_flag04000000_test(entity) &&
                        context != NULL && context->level != NULL) {
                        float position[2];
                        int cell_x, cell_y;
                        int record_x, record_y;
                        entity_transform_position_get(entity, position);
                        if (cmd_rec_cell_xy_get(record, &record_x, &record_y) &&
                            terrain_world_xy_to_cell(
                                context->level->ptr_terrain, position[0],
                                position[1], &cell_x, &cell_y) &&
                            (record_x != cell_x || record_y != cell_y))
                            fire = 1;
                    }
                    break;
                case 0x1d:
                    fire = entity_type3_flag04000000_test(entity);
                    if (fire) record->param2 = (int32_t)0x437a0000u;
                    break;
                default:
                    /* Other record kinds have no fire branch in this scan. */
                    break;
                }
                if (fire) {
                    --s_pending_timer_count;
                    s_pending_timer_candidates[i] =
                        s_pending_timer_candidates[s_pending_timer_count];
                    if ((record->state & 0x40u) == 0u) {
                        if (previous == NULL) s_global_tail = next;
                        else previous->global_next = next;
                        record->global_next = NULL;
                        entity->cmd_head = record;
                        removed = 1;
                    }
                    else {
                        entity->cmd_head = cmd_pending_record_copy(record);
                    }
                    break;
                }
            }
        }
        if (!removed) previous = record;
        record = next;
    }
}

// FUNCTION: LEGORR 0x00405c10
/* PendingCommandEntityAssignmentTick (FUN_00405c10). Candidate membership
 * uses the recovered 0x452610 stats mask or the record's explicit pointer
 * array (0x452650). Coordinate/target kinds 0, 3, 8, 0xb, 0xc, 0x12, 0x13,
 * 0x16, 0x17 and 10 use live entity and terrain fields; compatible candidates
 * are selected by distance and attached through the common transfer/copy
 * helper. Default predicate kinds use the returned immediate-assignment flag
 * and select the first compatible candidate without requiring coordinates.
 *
 * The cheat feature gate uses ai_can_do; kind 8 uses the full terrain and
 * entity gates from FUN_00432770. Kinds 0x15 and 0x1e resolve the target's
 * cached visual resource and read its DDiScene pose. Kind 0x1a uses the live
 * radius, ray, target-eligibility, and global-feature APIs below. Kind 0x11 reads its
 * `Main MinEnergyForEat` threshold from the active Lego.cfg. Kind 0x12's
 * nested type-4 pose uses the retained scene sidecar when present, and its
 * capability-0x20 and stats-flag position branches use live entity/level
 * state.
 * PlaceCommandEligibilityCheck's live ops are installed from cmd_context_set.
 * Kind 0x12's alternate scene pose still depends on the renderer resource
 * provider. Kinds 8/0xb/10/0x13/0x17 validate distant
 * candidates through TerrainNeighborPathCheck, then queue their decompiled
 * operation and consume the candidate. Route failure resets the record's
 * timer to the reference 75.0 bits. The displaced-cell messages for kinds
 * 0xc/0x16 use cmd_produce; the kind-0xc callback/timer transition is also
 * represented below. */
void pending_command_entity_assignment_tick(void)
{
    GameContext* context = pending_scan_context_get();
    CmdRec* previous = NULL;
    CmdRec* record = s_global_head;
    float nearest = 99999.0f;
    if (!s_pending_candidates_from_entity_walk) {
        unsigned count = pending_candidate_pool_build(
            context, s_pending_assignment_candidates, 0);
        s_pending_assignment_count = count;
    }
    while (record != NULL) {
        CmdRec* next = as_rec(record->global_next);
        if ((*(float*)&record->param2 <= 0.0f) &&
            (record->state & 0x80400u) == 0u) {
            unsigned i;
            unsigned selected_index = s_pending_assignment_count;
            EntitySlot* selected = NULL;
            int target_cell_x = 0, target_cell_y = 0;
            int selected_cell_x = 0, selected_cell_y = 0;
            int selected_requires_route = 0;
            float target_world[2] = { 0.0f, 0.0f };
            int target_world_valid = 0;
            const int coordinate_kind = record->type == 0 ||
                                        record->type == 3 ||
                                        record->type == 8 ||
                                        record->type == 0x0b ||
                                        record->type == 0x0c ||
                                        record->type == 0x16 ||
                                        record->type == 0x17 ||
                                        record->type == 0x13 ||
                                        record->type == 0x15 ||
                                        record->type == 0x1e ||
                                        record->type == 10 ||
                                        record->type == 0x12 ||
                                        record->type == 0x1a;
            /* Some command kinds have no coordinate result: their
             * PlaceCommandEligibilityCheck default arm sets its result flag
             * to request immediate assignment. They still participate in
             * the candidate walk, before the coordinate-specific cases. */
            int has_direct_placement = !coordinate_kind;
            if (coordinate_kind && context != NULL && context->level != NULL) {
                if (record->type == 3 && record->payload != NULL) {
                    entity_transform_position_get(
                        (EntitySlot*)record->payload, target_world);
                    target_world_valid = 1;
                    has_direct_placement = terrain_world_xy_to_cell(
                        context->level->ptr_terrain, target_world[0],
                        target_world[1], &target_cell_x, &target_cell_y);
                }
                else if ((record->type == 0x17 || record->type == 10) &&
                    record->payload != NULL) {
                    float target_position[2];
                    entity_transform_position_get(
                        (EntitySlot*)record->payload, target_position);
                    target_world[0] = target_position[0];
                    target_world[1] = target_position[1];
                    target_world_valid = 1;
                    has_direct_placement = terrain_world_xy_to_cell(
                        context->level->ptr_terrain, target_position[0],
                        target_position[1], &target_cell_x, &target_cell_y);
                }
                else if ((record->type == 0x15 || record->type == 0x1e) &&
                         record->payload != NULL &&
                         cmd_place_target_route_position_get(
                             NULL, (EntitySlot*)record->payload,
                             &target_world[0], &target_world[1])) {
                    target_world_valid = 1;
                    has_direct_placement = terrain_world_xy_to_cell(
                        context->level->ptr_terrain, target_world[0],
                        target_world[1], &target_cell_x, &target_cell_y);
                }
                else if (record->type == 0x12 && record->payload != NULL) {
                    EntitySlot* target = (EntitySlot*)record->payload;
                    int target_xy[2];
                    if ((entity_stats_word_get(target) & 0x2000000u) != 0u) {
                        float target_position[2];
                        entity_transform_position_get(target, target_position);
                        target_world[0] = target_position[0];
                        target_world[1] = target_position[1];
                        target_world_valid = 1;
                        has_direct_placement = terrain_world_xy_to_cell(
                            context->level->ptr_terrain, target_position[0],
                            target_position[1], &target_cell_x, &target_cell_y);
                    }
                    else {
                        float nested_scene_position[3];
                        if (cmd_place_target_nested_scene_pose_get(
                                target, nested_scene_position)) {
                            target_world[0] = nested_scene_position[0];
                            target_world[1] = nested_scene_position[1];
                            target_world_valid = 1;
                            has_direct_placement = terrain_world_xy_to_cell(
                                context->level->ptr_terrain,
                                nested_scene_position[0],
                                nested_scene_position[1], &target_cell_x,
                                &target_cell_y);
                        }
                        else if (entity_record_flag20_query(target,
                                                            target_xy) &&
                             level_coord_pair_within_bounds(
                                 context->level, target_xy[0], target_xy[1])) {
                            /* FUN_00404fc0 first resolves the nested type-4
                             * scene pose, then falls back to FUN_00439e40's
                             * adjacent task cell. */
                            target_cell_x = target_xy[0];
                            target_cell_y = target_xy[1];
                            has_direct_placement =
                                terrain_cell_world_center_xy_get(
                                    context->level->ptr_terrain,
                                    target_cell_x, target_cell_y,
                                    target_world);
                            target_world_valid = has_direct_placement;
                        }
                    }
                }
                else if (record->type == 0x1a && record->payload != NULL) {
                    entity_transform_position_get(
                        (EntitySlot*)record->payload, target_world);
                    target_world_valid = 1;
                    has_direct_placement = terrain_world_xy_to_cell(
                        context->level->ptr_terrain, target_world[0],
                        target_world[1], &target_cell_x, &target_cell_y);
                }
                else if (record->type == 0x13) {
                    /* Kind 0x13 resolves its target from each candidate's
                     * current world position in PlaceCommandEligibilityCheck;
                     * there is no coordinate pair in the command record. */
                    has_direct_placement = 1;
                }
                else if (record->type != 0x17 && record->type != 10 &&
                    record->type != 0x12 &&
                    cmd_rec_cell_xy_get(record, &target_cell_x,
                                        &target_cell_y) &&
                    level_coord_pair_within_bounds(context->level,
                                                   target_cell_x,
                                                   target_cell_y)) {
                    has_direct_placement = 1;
                    target_world_valid = terrain_cell_world_center_xy_get(
                        context->level->ptr_terrain, target_cell_x,
                        target_cell_y, target_world);
                }
            }
            for (i = 0; has_direct_placement &&
                        i < s_pending_assignment_count; ++i) {
                EntitySlot* entity = s_pending_assignment_candidates[i];
                float eligibility_position[2] = { 0.0f, 0.0f };
                uint32_t eligibility_immediate = 0u;
                float position[2];
                float candidate_target_world[2] = { target_world[0],
                                                     target_world[1] };
                int cell_x, cell_y;
                int candidate_target_cell_x = target_cell_x;
                int candidate_target_cell_y = target_cell_y;
                int candidate_target_world_valid = target_world_valid;
                float dx, dy, distance;
                int requires_route = 0;
                int use_approach_cell = 0;
                if (!pending_record_matches_entity(record, entity) ||
                    ((entity_stats_word_get(entity) & 0x20000u) != 0u &&
                     entity->owner_36c == NULL) ||
                    (entity->flags_3e0 & 0x800000u) != 0u ||
                    (entity->flags_3ec & 0x400u) != 0u ||
                    ((context->cheat & 1u) != 0u &&
                     record->type != 0x1a && ai_can_do(entity) != 0))
                    continue;
                /* The retail assignment loop calls 0x404fc0 for every
                 * candidate before path validation. Its final output flag
                 * marks the default command-kind branch as an immediate
                 * selection, so keep that result separate from the derived
                 * coordinate outputs. */
                if (!cmd_place_command_eligibility_check(
                        record, entity,
                        coordinate_kind ? eligibility_position : NULL,
                        NULL, &eligibility_immediate, 1, 1))
                    continue;
                if (!coordinate_kind) {
                    if (eligibility_immediate == 0u) continue;
                    selected = entity;
                    selected_index = i;
                    selected_cell_x = 0;
                    selected_cell_y = 0;
                    selected_requires_route = 0;
                    break;
                }
                if (record->type == 3) {
                    target_world[0] = eligibility_position[0];
                    target_world[1] = eligibility_position[1];
                    target_world_valid = 1;
                    if (!terrain_world_xy_to_cell(
                            context->level->ptr_terrain,
                            eligibility_position[0], eligibility_position[1],
                            &target_cell_x, &target_cell_y)) continue;
                    candidate_target_world[0] = target_world[0];
                    candidate_target_world[1] = target_world[1];
                    candidate_target_world_valid = target_world_valid;
                    candidate_target_cell_x = target_cell_x;
                    candidate_target_cell_y = target_cell_y;
                }
                if (record->type == 0x1a) {
                    target_world[0] = eligibility_position[0];
                    target_world[1] = eligibility_position[1];
                    target_world_valid = 1;
                    if (!terrain_world_xy_to_cell(
                            context->level->ptr_terrain,
                            eligibility_position[0],
                            eligibility_position[1], &target_cell_x,
                            &target_cell_y))
                        continue;
                    candidate_target_world[0] = target_world[0];
                    candidate_target_world[1] = target_world[1];
                    candidate_target_world_valid = target_world_valid;
                    candidate_target_cell_x = target_cell_x;
                    candidate_target_cell_y = target_cell_y;
                }
                if (record->type == 0x15 || record->type == 0x1e) {
                    target_world[0] = eligibility_position[0];
                    target_world[1] = eligibility_position[1];
                    target_world_valid = 1;
                    candidate_target_world[0] = eligibility_position[0];
                    candidate_target_world[1] = eligibility_position[1];
                    candidate_target_world_valid = 1;
                    if (!terrain_world_xy_to_cell(
                            context->level->ptr_terrain,
                            eligibility_position[0], eligibility_position[1],
                            &candidate_target_cell_x,
                            &candidate_target_cell_y)) continue;
                }
                if (record->type == 0x13) {
                    candidate_target_world[0] = eligibility_position[0];
                    candidate_target_world[1] = eligibility_position[1];
                    if (!terrain_world_xy_to_cell(
                            context->level->ptr_terrain,
                            candidate_target_world[0],
                            candidate_target_world[1],
                            &candidate_target_cell_x,
                            &candidate_target_cell_y))
                        continue;
                    candidate_target_world_valid = 1;
                }
                if (record->type == 0x12) {
                    candidate_target_world[0] = eligibility_position[0];
                    candidate_target_world[1] = eligibility_position[1];
                    candidate_target_world_valid = 1;
                    if (!terrain_world_xy_to_cell(
                            context->level->ptr_terrain,
                            eligibility_position[0], eligibility_position[1],
                            &candidate_target_cell_x,
                            &candidate_target_cell_y)) continue;
                }
                if (record->type == 0x0c &&
                    (entity_stats_word_get(entity) & 0x10000u) == 0u)
                    continue;
                if (record->type == 0x16 &&
                    (entity->type != 2 ||
                     !always_accept_place_command_cell()))
                    continue;
                if ((record->type == 8 || record->type == 0x0b ||
                     record->type == 0x0c || record->type == 10) &&
                    !pending_command_activity_gate(entity, record->type))
                    continue;
                if (record->type == 0x0b && entity->type != 2)
                    continue;
                if (record->type == 8) {
                    if (!pending_command_cell_walkable(
                            context, entity, target_cell_x, target_cell_y,
                            (record->state & 8u) != 0u))
                        continue;
                }
                if (record->type == 10) {
                    if (entity->type != 2 ||
                        ((entity->flags_3f0 & 0x10u) == 0u &&
                         (record->state & 0x200000u) == 0u))
                        continue;
                    if ((entity_state_flags_get(entity) & 0x80000u) != 0u &&
                        (entity_shared_eligibility_check(
                             (EntitySlot*)record->payload, 0) == 0 ||
                         (entity_state_flags_get(
                              (EntitySlot*)record->payload) & 0x400u) != 0u))
                        continue;
                }
                if (record->type == 0x12 &&
                    (entity->type != 2 || entity->cmd_head != NULL ||
                     entity->owner_36c != NULL ||
                     (record->field_3c & entity->flags_3f0) == 0u ||
                     record->payload == NULL))
                    continue;
                if (record->type == 0x17) {
                    const uint32_t actor_flags = entity->flags_3f0;
                    if (entity->type != 2 || entity->cmd_head != NULL ||
                        entity->owner_36c != NULL ||
                        ((actor_flags & 1u) != 0u &&
                         record->field_3c == 1u) ||
                        ((actor_flags & 2u) != 0u &&
                         record->field_3c == 2u) ||
                        ((actor_flags & 4u) != 0u &&
                         record->field_3c == 4u) ||
                        ((actor_flags & 8u) != 0u &&
                         record->field_3c == 8u) ||
                        ((actor_flags & 0x10u) != 0u &&
                         record->field_3c == 0x10u) ||
                        ((actor_flags & 0x20u) != 0u &&
                         record->field_3c == 0x20u) ||
                        entity->activity_counter_3d4 <
                            engine_float_field260_get(context->level))
                        continue;
                }
                entity_transform_position_get(entity, position);
                if (!terrain_world_xy_to_cell(context->level->ptr_terrain,
                                              position[0], position[1],
                                              &cell_x, &cell_y))
                    continue;
                if ((record->type == 8 && (record->state & 8u) == 0u) ||
                    record->type == 0x0b || record->type == 10 ||
                    record->type == 0x17 || record->type == 0x13) {
                    const int dx = cell_x - candidate_target_cell_x;
                    const int dy = cell_y - candidate_target_cell_y;
                    requires_route =
                        !(((dx == 1 || dx == -1) && dy == 0) ||
                          (dx == 0 && (dy == 1 || dy == -1)));
                    use_approach_cell = requires_route;
                }
                if (record->type == 3 &&
                    (cell_x != candidate_target_cell_x ||
                     cell_y != candidate_target_cell_y))
                    requires_route = 1;
                {
                    int route_goal_xy[2];
                    int* path_x = NULL;
                    int* path_y = NULL;
                    uint32_t path_count = 0;
                    int route_found;
                    route_goal_xy[0] = candidate_target_cell_x;
                    route_goal_xy[1] = candidate_target_cell_y;
                    /* Retail probes a route for every candidate after
                     * PlaceCommandEligibilityCheck. A subset of command
                     * kinds first changes the route goal to an adjacent
                     * approach cell when the actor is not already adjacent;
                     * other kinds still require a path to the command cell
                     * before they can be assigned. */
                    if (use_approach_cell) {
                        int goal_xy[2];
                        goal_xy[0] = candidate_target_cell_x;
                        goal_xy[1] = candidate_target_cell_y;
                        if (!first_free_neighbor_cell_pick(
                                entity, context->level, goal_xy,
                                route_goal_xy, 1))
                            continue;
                    }
                    /* FUN_00405c10 calls FUN_00440c30 for every otherwise
                     * eligible candidate, including one already in the
                     * destination cell. That route query also validates
                     * the target cell; skipping the coincident-coordinate
                     * case could therefore assign an unreachable command. */
                    route_found = terrain_neighbor_path_check(
                        entity, (uint32_t)cell_x, (uint32_t)cell_y,
                        (uint32_t)route_goal_xy[0],
                        (uint32_t)route_goal_xy[1],
                        &path_x, &path_y, &path_count, NULL, NULL);
                    free(path_x);
                    free(path_y);
                    if (!route_found) {
                        record->param2 = (int32_t)0x42960000u;
                        continue;
                    }
                }
                if (candidate_target_world_valid) {
                    dx = position[0] - candidate_target_world[0];
                    dy = position[1] - candidate_target_world[1];
                }
                else {
                    dx = (float)(cell_x - candidate_target_cell_x);
                    dy = (float)(cell_y - candidate_target_cell_y);
                }
                distance = (float)sqrt((double)(dx * dx + dy * dy));
                if (eligibility_immediate != 0u) {
                    selected = entity;
                    selected_index = i;
                    nearest = distance;
                    selected_cell_x = cell_x;
                    selected_cell_y = cell_y;
                    selected_requires_route = 0;
                    break;
                }
                if (selected == NULL || distance < nearest) {
                    selected = entity;
                    selected_index = i;
                    nearest = distance;
                    selected_cell_x = cell_x;
                    selected_cell_y = cell_y;
                    selected_requires_route = requires_route;
                    if (record->type == 0x13) {
                        target_cell_x = candidate_target_cell_x;
                        target_cell_y = candidate_target_cell_y;
                        target_world[0] = candidate_target_world[0];
                        target_world[1] = candidate_target_world[1];
                        target_world_valid = candidate_target_world_valid;
                    }
                }
            }
            if (selected == NULL) {
                unsigned fallback_index;
                if ((record->state & 0x1000u) == 0u) {
                    for (fallback_index = 0u;
                         fallback_index < s_pending_assignment_count;
                         ++fallback_index) {
                        EntitySlot* entity =
                            s_pending_assignment_candidates[fallback_index];
                        if (pending_record_matches_entity(record, entity) &&
                            cmd_place_command_eligibility_check(
                                record, entity, NULL, NULL, NULL, 0, 1)) {
                            cmd_validated_record_queue(record);
                            break;
                        }
                    }
                }
                if (record->type != 0x17 && record->field_3c != 0 &&
                    (record->state & 0x8000u) == 0u) {
                    EntitySlot* category_entity = NULL;
                    float category_position[2] = { 0.0f, 0.0f };
                    int category_ready = 0;
                    for (fallback_index = 0u;
                         fallback_index < s_pending_assignment_count;
                         ++fallback_index) {
                        EntitySlot* entity =
                            s_pending_assignment_candidates[fallback_index];
                        if (!pending_record_matches_entity(record, entity) ||
                            !cmd_place_command_eligibility_check(
                                record, entity, category_position, NULL,
                                NULL, 0, 0)) continue;
                        category_entity = entity;
                        if (cmd_place_command_eligibility_check(
                                record, entity, category_position, NULL,
                                NULL, 0, 1)) {
                            category_ready = 1;
                            break;
                        }
                    }
                    if (category_entity != NULL && !category_ready) {
                        EntitySlot* found = (EntitySlot*)
                            unported_ledger_stubs::
                                ledger_stub_00438e50_SpatialObjectQueryCenterMask100(
                                    NULL, category_position,
                                    (uint32_t)record->field_3c);
                        if (found != NULL) {
                            task_type23_create_for_category_mask(
                                found, record->field_3c,
                                record->field_3c != 0x10);
                            record->state |= 0x8000u;
                        }
                    }
                }
            }
            if (selected != NULL) {
                if (selected_requires_route) {
                    int goal_xy[2];
                    int route_xy[2];
                    int message_opcode = 0;
                    goal_xy[0] = target_cell_x;
                    goal_xy[1] = target_cell_y;
                    if (record->type == 3) {
                        route_xy[0] = target_cell_x;
                        route_xy[1] = target_cell_y;
                        if ((record->state & 0x100u) == 0u)
                            record->entity_ref = selected;
                        cmd_produce(0x17, selected,
                                    (int)(uintptr_t)record, route_xy);
                        --s_pending_assignment_count;
                        s_pending_assignment_candidates[selected_index] =
                            s_pending_assignment_candidates[
                                s_pending_assignment_count];
                        previous = record;
                        record = next;
                        continue;
                    }
                    if (!first_free_neighbor_cell_pick(
                            selected, context->level, goal_xy, route_xy, 1)) {
                        --record->behaviour;
                    }
                    else {
                        if (record->type == 8) message_opcode = 0x0d;
                        else if (record->type == 10) message_opcode = 0x0f;
                        else if (record->type == 0x0b) message_opcode = 0x11;
                        else if (record->type == 0x17) message_opcode = 0x32;
                        else if (record->type == 0x13) message_opcode = 0x2c;
                        if (message_opcode != 0) {
                            cmd_produce(message_opcode, selected,
                                        (int)(uintptr_t)record, route_xy);
                            if ((record->state & 0x100u) == 0u)
                                record->entity_ref = selected;
                        }
                    }
                    --s_pending_assignment_count;
                    s_pending_assignment_candidates[selected_index] =
                        s_pending_assignment_candidates[
                            s_pending_assignment_count];
                    previous = record;
                    record = next;
                    continue;
                }
                if ((record->type == 0x0c || record->type == 0x15 ||
                     record->type == 0x16) &&
                    (selected_cell_x != target_cell_x ||
                     selected_cell_y != target_cell_y)) {
                    int message_cell_xy[2];
                    /* The retail path queues a task-specific operation and
                     * consumes this candidate while leaving the command
                     * globally linked. */
                    if ((record->state & 0x100u) == 0u)
                        record->entity_ref = selected;
                    message_cell_xy[0] = target_cell_x;
                    message_cell_xy[1] = target_cell_y;
                    cmd_produce(record->type == 0x0c ? 0x27 :
                                record->type == 0x15 ? 0x38 : 0x30,
                                selected, (int)(uintptr_t)record,
                                message_cell_xy);
                    if (record->type == 0x0c && selected->type == 1) {
                        coordinate_callback_dispatch_by_mode(&record->x, 1);
                        record->param2 = 0;
                    }
                    --s_pending_assignment_count;
                    s_pending_assignment_candidates[selected_index] =
                        s_pending_assignment_candidates[
                            s_pending_assignment_count];
                    previous = record;
                    record = next;
                    continue;
                }
                const int copied = (record->state & 0x40u) != 0u;
                entity_record_attach_and_relink(record, previous, selected);
                --s_pending_assignment_count;
                s_pending_assignment_candidates[selected_index] =
                    s_pending_assignment_candidates[s_pending_assignment_count];
                /* The assignment pass consumes this record from its global
                 * list and the entity-local head now owns it. */
                if (copied) previous = record;
                record = next;
                continue;
            }
        }
        previous = record;
        record = next;
    }
}

void* cmd_global_next(const void* rec)
{
    return rec != NULL ? cas_rec(rec)->global_next : NULL;
}

void cmd_global_unlink(const void* rec)
{
    CmdRec** roots[2];
    int root_index;
    if (rec == NULL) return;
    roots[0] = &s_global_head;
    roots[1] = &s_global_tail;
    for (root_index = 0; root_index < 2; ++root_index) {
        CmdRec* previous = NULL;
        CmdRec* current = *roots[root_index];
        while (current != NULL) {
            if ((const void*)current == rec) {
                if (previous != NULL)
                    previous->global_next = current->global_next;
                else
                    *roots[root_index] = as_rec(current->global_next);
                current->global_next = NULL;
                return;
            }
            previous = current;
            current = as_rec(current->global_next);
        }
    }
}

/* PendingRecordEntityReferenceClear (FUN_004063e0) — see cmd.hpp. The
 * corpus strides the global list's +0x60 link and the +0x48 dword
 * blindly; the port rides its native-size link slot and reference slot
 * (documented deviations). */
// FUNCTION: LEGORR 0x004063e0
void pending_record_entity_reference_clear(void* reference)
{
    for (CmdRec* cur = s_global_head; cur != NULL;
         cur = as_rec(cur->global_next)) {
        if (cur->entity_ref == reference) {
            cur->entity_ref = NULL;
        }
    }
}

void* cmd_rec_entity_ref(const void* rec)
{
    return rec != NULL ? cas_rec(rec)->entity_ref : NULL;
}

void cmd_rec_entity_ref_install(void* rec, void* ref)
{
    if (rec != NULL) {
        as_rec(rec)->entity_ref = ref;
    }
}

/* EntityRecordAttachAndRelink (FUN_00406360) — see cmd.hpp. The record's
 * global-list unlink rides the port's global_next slot (the corpus's one
 * +0x60 word; the chain/global context split is documented in the
 * entity command-chain housekeeping band). Deviations: NULL records /
 * entities guard where the corpus strides blindly, and the +0x18
 * task-param word takes the corpus's float-bit constant raw. */
// FUNCTION: LEGORR 0x00406360
void entity_record_attach_and_relink(void* record, void* previous,
                                    EntitySlot* entity)
{
    CmdRec* rec = (CmdRec*)record;
    if (rec == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    if ((rec->state & 0x40u) == 0u) {
        if (previous == NULL) {
            s_global_head = as_rec(rec->global_next);
        } else {
            as_rec(previous)->global_next = rec->global_next;
        }
        rec->global_next = NULL;
        if (entity != NULL) {
            entity->cmd_head = rec; /* corpus entity +0x2f0 */
        }
    } else {
        void* copy = cmd_pending_record_copy(rec);
        if (entity != NULL) {
            entity->cmd_head = copy; /* the corpus relinks the copy */
        }
        if ((rec->state & 0x40000u) != 0u) {
            rec->param2 = (int32_t)0x41c80000; /* +0x18 raw 25.0 bits */
        }
    }
    pending_record_entity_reference_clear(entity); /* FUN_004063e0 */
    idle_register(entity);                         /* FUN_00407450 */
}

/* Append a record to the entity's chain. This helper is used by other
 * command constructors; CmdCreateMove has its own retail-shaped path below. */

static void cmd_chain_append(EntitySlot* e, CmdRec* rec, int as_move)
{
    CmdRec* tail = as_rec(cmd_chain_walk(e));
    if (tail != NULL) {
        tail->next = rec;
    } else {
        e->cmd_head = rec;
    }
    rec->next = NULL; /* entity-chain tail: never re-link onto the global list */
    rec->now = 1500.0f;
    rec->state &= ~CMD_STATE_PENDING;
    if (as_move && e->etype == 2) {
        idle_register(e); /* IdleQueueRegister for fresh move commands */
    }
}

/* ------------------------------------------------------------------ *
 *  Creators (CmdCreateMove/Target/Gated/Task + MinionCompanionCmd).
 * ------------------------------------------------------------------ */

// FUNCTION: LEGORR 0x004035c0
void cmd_create_move(EntitySlot* e, const float position_xy[2], int flag)
{
    CmdRec* rec;
    CmdRec* tail;
    if (e == NULL || position_xy == NULL) return;
    rec = (CmdRec*)cmd_alloc(0);
    if (rec == NULL) return;
    rec->x = position_xy[0];
    rec->y = position_xy[1];
    rec->param = flag;
    tail = as_rec(cmd_chain_walk(e));
    if (tail != NULL) {
        tail->next = rec;
    }
    else {
        e->cmd_head = rec;
        idle_register(e);
    }
}

void cmd_create_move(EntitySlot* e, float x, float y, int flag)
{
    const float position_xy[2] = { x, y };
    cmd_create_move(e, position_xy, flag);
}

// FUNCTION: LEGORR 0x004036c0
void cmd_create_target(EntitySlot* e, void* target)
{
    if (e == NULL) {
        return;
    }
    CmdRec* rec = (CmdRec*)cmd_alloc(2);
    if (rec == NULL) {
        return;
    }
    rec->payload = target;
    rec->now = 0.0f;
    cmd_chain_append(e, rec, 0);
}

// FUNCTION: LEGORR 0x00403610
void cmd_create_gated(EntitySlot* e, void* target)
{
    if (e == NULL) {
        return;
    }
    /* Gated commands apply only when entity +0x3e0 & 0x400. */
    if ((e->flags_3e0 & 0x400u) == 0) {
        return;
    }
    CmdRec* rec = (CmdRec*)cmd_alloc(5);
    if (rec == NULL) {
        return;
    }
    rec->payload = target;
    cmd_chain_append(e, rec, 0);
}

/* Task kind derived from the entity type (FUN_00402720's bVar3 switch). */

static int task_kind_for(EntitySlot* e)
{
    switch (e->type) {
    case 6:  return (e->flags_3e8 & 0x80000000u) != 0 ? 0x12 : 0;
    case 7:  return 1;
    case 9:  return 0x10;
    case 8:  return 3;
    default: return 2;
    }
}

/* MinionCompanionCmd (FUN_00402850): append the extra kind-0x15 task that
 * accompanies a type-6 minion's primary task. */
// FUNCTION: LEGORR 0x00402850
static void cmd_minion_companion(EntitySlot* e)
{
    /* Corpus parity: strides the entity blindly, uses the record blindly,
     * and routes the kind through the corpus's own cmd_set_kind. */
    if (e->type != 6) {
        return;
    }
    CmdRec* rec = (CmdRec*)cmd_alloc(3);
    rec->payload = e;
    cmd_set_kind(rec, 0x15);
    cmd_link_tail(rec);
    /* Corpus: mov eax,[+0x5c] / or ah,0x20 / mov [+0x5c],eax. */
    rec->state = rec->state | CMD_STATE_MINION;
}

// FUNCTION: LEGORR 0x00402720
void cmd_create_task(EntitySlot* e)
{
    if (e == NULL) {
        return;
    }
    CmdRec* rec = (CmdRec*)cmd_alloc(3);
    if (rec == NULL) {
        return;
    }
    rec->payload = e;
    rec->param2 = 0;
    int kind = task_kind_for(e);
    if (e->type == 8) {
        rec->extra = 8; /* type-8 trains set the extra slot */
    }
    cmd_link_head(rec, kind);
    cmd_minion_companion(e);
}

/* ObjectCommandTaskCreate (FUN_00403320): append a type-0x15 task record
 * carrying the spatial-query result and the caller's parameter. */
void* cmd_entity_task_record_create_and_append(EntitySlot* e, void* payload,
                                               int parameter)
{
    CmdRec* rec;
    CmdRec* tail;
    if (e == NULL || payload == NULL) return NULL;
    rec = (CmdRec*)cmd_alloc(0x15);
    if (rec == NULL) return NULL;
    rec->payload = payload;
    rec->cell_count = parameter;
    /* FUN_00403320 allocates and fills the new record before sweeping the
     * existing chain, then appends it through FUN_00404190's tail walk. */
    command_queue_sweep_relink(e);
    tail = as_rec(cmd_chain_walk(e));
    if (tail != NULL) tail->next = rec;
    else e->cmd_head = rec;
    rec->next = NULL;
    return rec;
}

/* ObjectPathSubmitToGridTarget (FUN_00404e00): a target already on the
 * actor's cell is submitted as a one-point path; otherwise resolve the
 * cardinal route and pass its points to the entity route installer. */
// FUNCTION: LEGORR 0x00404e00
int object_path_submit_to_grid_target(EntitySlot* entity, LevelState* level,
                                      const int target_xy[2],
                                      const float* final_marker_position)
{
    int start_xy[2] = { -1, -1 };
    int* route_x = NULL;
    int* route_y = NULL;
    uint32_t route_count = 0u;
    float direct_x;
    float direct_y;
    float* points_x;
    float* points_y;
    uint32_t i;
    if (entity == NULL || level == NULL || target_xy == NULL ||
        level->ptr_terrain == NULL) return 0;
    entity_position_to_terrain_cell(entity, level, &start_xy[0],
                                    &start_xy[1]);
    if (start_xy[0] < 0 || start_xy[1] < 0) return 0;
    if (start_xy[0] == target_xy[0] && start_xy[1] == target_xy[1]) {
        direct_x = (float)start_xy[0];
        direct_y = (float)start_xy[1];
        (void)entity_route_setup_and_marker_associate(
            entity, 1u, &direct_x, &direct_y, final_marker_position);
        return 1;
    }
    if (!terrain_neighbor_path_check(level->ptr_terrain,
            (uint32_t)start_xy[0], (uint32_t)start_xy[1],
            (uint32_t)target_xy[0], (uint32_t)target_xy[1],
            &route_x, &route_y, &route_count, NULL, NULL)) return 0;
    if (route_count == 0u || route_count > 4096u) {
        free(route_x);
        free(route_y);
        return 0;
    }
    points_x = (float*)malloc((size_t)route_count * sizeof(float));
    points_y = (float*)malloc((size_t)route_count * sizeof(float));
    if (points_x == NULL || points_y == NULL) {
        free(points_x);
        free(points_y);
        free(route_x);
        free(route_y);
        return 0;
    }
    for (i = 0u; i < route_count; ++i) {
        points_x[i] = (float)route_x[i];
        points_y[i] = (float)route_y[i];
    }
    (void)entity_route_setup_and_marker_associate(
        entity, route_count, points_x, points_y, final_marker_position);
    free(points_x);
    free(points_y);
    free(route_x);
    free(route_y);
    return 1;
}

// CommandKindRecordIssueAndLink (FUN_00403750).
// FUNCTION: LEGORR 0x00403750
void cmd_command_kind_record_issue(uint32_t a, uint32_t b, uint32_t c,
                                   const int cell_xy[2], uint32_t zone,
                                   const float* random_pair, int set_flag)
{
    CmdRec* rec;
    float first;
    float second;
    if (cell_xy == NULL) return;
    rec = (CmdRec*)cmd_alloc(7);
    if (rec == NULL) return;
    rec->cell_act = (int32_t)a;
    rec->cell_target = (int32_t)b;
    rec->cell_count = (int32_t)c;
    memcpy(&rec->x, &cell_xy[0], sizeof(cell_xy[0]));
    memcpy(&rec->y, &cell_xy[1], sizeof(cell_xy[1]));
    rec->zone = (int32_t)zone;
    if (random_pair != NULL) {
        first = random_pair[0];
        second = random_pair[1];
    }
    else if (s_command_kind_record_hooks.random_range != NULL) {
        first = s_command_kind_record_hooks.random_range(
            0.4f, 0.6f, s_command_kind_record_hooks.context);
        second = s_command_kind_record_hooks.random_range(
            0.4f, 0.6f, s_command_kind_record_hooks.context);
    }
    else {
        first = random_float_range(0.4f, 0.6f);
        second = random_float_range(0.4f, 0.6f);
    }
    memcpy(&rec->pad_30[0], &first, sizeof(first));
    memcpy(&rec->pad_30[4], &second, sizeof(second));
    rec->param2 = 0;
    if (set_flag != 0) rec->state |= 0x800u;
    cmd_link_head(rec, 5);
    rec->state |= 0x400u;
    if (s_command_kind_record_hooks.terrain_cell_update != NULL) {
        s_command_kind_record_hooks.terrain_cell_update(
            cell_xy, (int)a, (int)b, (int)c,
            s_command_kind_record_hooks.context);
    }
    else {
        terrain_cell_record_count_update(cell_xy, (int)a, (int)b, (int)c);
    }
}

/* ------------------------------------------------------------------ *
 *  Global tick + purge. AI activity and command/task execution live in ai.cpp.
 * ------------------------------------------------------------------ */

void cmd_chain_purge_hooks_install(CmdPurgeCellPredicate cell_usable,
                                   CmdPurgeCellPredicate neighbor_eligible,
                                   CmdPurgeCompare compare, void* context)
{
    s_cmd_purge_cell_usable = cell_usable;
    s_cmd_purge_neighbor_eligible = neighbor_eligible;
    s_cmd_purge_compare = compare;
    s_cmd_purge_context = context;
}

static int cmd_purge_compare_adapter(const void* left, const void* right)
{
    if (s_cmd_purge_compare == NULL) {
        return 0;
    }
    return s_cmd_purge_compare(left, right, s_cmd_purge_context);
}

static int cmd_purge_compare_default(const void* left, const void* right)
{
    const CmdRec* a = *(const CmdRec* const*)left;
    const CmdRec* b = *(const CmdRec* const*)right;
    const uint32_t a_state = a->state & 0x100u;
    const uint32_t b_state = b->state & 0x100u;
    int a_behaviour = a->behaviour;
    int b_behaviour = b->behaviour;
    if (a_state != 0u && b_state == 0u) return -1;
    if (a_state != 0u && b_state != 0u) {
        const uintptr_t a_order = (uintptr_t)a->global_next;
        const uintptr_t b_order = (uintptr_t)b->global_next;
        if (a_order > b_order) return -1;
        if (a_order < b_order) return 1;
    }
    if (a_behaviour < 0) a_behaviour = 0;
    if (b_behaviour < 0) b_behaviour = 0;
    if (a_behaviour > 99) a_behaviour = 99;
    if (b_behaviour > 99) b_behaviour = 99;
    return a_behaviour < b_behaviour ? -1 :
           (a_behaviour > b_behaviour ? 1 : 0);
}

/* Portable helper for callers that own an EntitySlot command chain. Retail
 * CmdChainPurge takes a command-list head pointer; its mapped implementation
 * is cmd_global_list_purge below. */
void cmd_chain_purge(EntitySlot* e)
{
    CmdRec* records[300];
    CmdRec* cur;
    CmdRec* overflow;
    int count;
    int i;
    if (e == NULL) {
        return;
    }
    cur = as_rec(e->cmd_head);
    overflow = NULL;
    while (cur != NULL && (cur->state & CMD_STATE_DONE) != 0u) {
        CmdRec* next = as_rec(cur->next);
        if (e->cmd_head == cur) {
            e->cmd_head = next;
        }
        cmd_global_unlink(cur);
        cmd_free(cur, 0);
        cur = next;
    }
    if (e->cmd_head == NULL) {
        return;
    }
    /* Completed records may also occur after a live command. Keep their
     * neighbours linked exactly as the corpus's first walk does. */
    {
        CmdRec* previous = as_rec(e->cmd_head);
        cur = as_rec(previous->next);
        while (cur != NULL) {
            CmdRec* next = as_rec(cur->next);
            if ((cur->state & CMD_STATE_DONE) != 0u) {
                previous->next = next;
                cmd_global_unlink(cur);
                cmd_free(cur, 0);
            } else {
                previous = cur;
            }
            cur = next;
        }
    }
    count = 0;
    cur = as_rec(e->cmd_head);
    while (cur != NULL) {
        if (count == 300) {
            overflow = cur;
            break;
        }
        records[count++] = cur;
        cur = as_rec(cur->next);
    }
    for (i = 0; i < count; ++i) {
        CmdRec* rec = records[i];
        int behavior = (rec->kind >= 0 && rec->kind < 27)
            ? s_behaviour[rec->kind] : 0;
        if (s_priority_state[rec->kind] == 0 ||
            (rec->state & CMD_STATE_CANCEL) != 0u) {
            rec->state &= ~0x80000u;
            rec->behaviour = ((rec->state & 0x30000u) != 0u)
                ? 99 : behavior;
        } else {
            rec->state |= 0x80000u;
        }
        if (rec->type == 8) {
            if ((rec->state & 8u) == 0u) {
                int cell_x, cell_y;
                memcpy(&cell_x, &rec->x, sizeof(cell_x));
                memcpy(&cell_y, &rec->y, sizeof(cell_y));
                LevelState* level = e->owner_ctx != NULL
                    ? e->owner_ctx->level :
                      (s_ctx != NULL ? s_ctx->level : NULL);
                const int clear_state = s_cmd_purge_cell_usable != NULL
                    ? s_cmd_purge_cell_usable(cell_x, cell_y,
                                              s_cmd_purge_context)
                    : (level != NULL &&
                       level_cell_mask18_clear(level, cell_x, cell_y));
                if (clear_state != 0) {
                    rec->behaviour = 0;
                }
            } else {
                static const int offsets[4][2] = {
                    {0, 0}, {0, -1}, {-1, -1}, {-1, 0}
                };
                int cell_x, cell_y;
                int found = 0;
                int n;
                memcpy(&cell_x, &rec->x, sizeof(cell_x));
                memcpy(&cell_y, &rec->y, sizeof(cell_y));
                for (n = 0; n < 4; ++n) {
                    int x = cell_x + offsets[n][0];
                    int y = cell_y + offsets[n][1];
                    LevelState* level = e->owner_ctx != NULL
                        ? e->owner_ctx->level :
                          (s_ctx != NULL ? s_ctx->level : NULL);
                    const int eligible = s_cmd_purge_neighbor_eligible != NULL
                        ? s_cmd_purge_neighbor_eligible(
                            x, y, s_cmd_purge_context)
                        : (level != NULL &&
                           level_cell_walkable_flag(level, x, y));
                    if (eligible != 0) {
                        found = 1;
                    }
                }
                if (found) {
                    if (rec->behaviour == 0) {
                        rec->param2 = (int32_t)0x42c80000u;
                    }
                } else {
                    rec->behaviour = 0;
                }
            }
        }
    }
    if (count > 1) {
        qsort(records, (size_t)count, sizeof(records[0]),
              s_cmd_purge_compare != NULL ? cmd_purge_compare_adapter
                                          : cmd_purge_compare_default);
    }
    if (count != 0) {
        for (i = 0; i + 1 < count; ++i) {
            records[i]->next = records[i + 1];
        }
        records[count - 1]->next = overflow;
        e->cmd_head = records[0];
    }
}

// FUNCTION: LEGORR 0x00402310
static void cmd_global_list_purge(CmdRec** head)
{
    CmdRec* records[300];
    CmdRec* current;
    CmdRec* previous;
    CmdRec* overflow = NULL;
    int count = 0;
    int i;

    previous = NULL;
    current = *head;
    while (current != NULL) {
        CmdRec* next = as_rec(current->global_next);
        if ((current->state & CMD_STATE_DONE) != 0u) {
            if (previous == NULL) *head = next;
            else previous->global_next = next;
            current->global_next = NULL;
            cmd_free(current, 0);
        } else {
            previous = current;
        }
        current = next;
    }

    current = *head;
    while (current != NULL && count < 300) {
        int behavior;
        records[count++] = current;
        behavior = (current->kind >= 0 && current->kind < 27)
            ? s_behaviour[current->kind] : 0;
        if (s_priority_state[current->kind] == 0 ||
            (current->state & CMD_STATE_CANCEL) != 0u) {
            current->state &= ~0x80000u;
            current->behaviour = ((current->state & 0x30000u) != 0u)
                ? 99 : behavior;
        } else {
            current->state |= 0x80000u;
        }

        if (current->type == 8) {
            int cell_x;
            int cell_y;
            LevelState* level = s_ctx != NULL ? s_ctx->level : NULL;
            memcpy(&cell_x, &current->x, sizeof(cell_x));
            memcpy(&cell_y, &current->y, sizeof(cell_y));
            if ((current->state & 8u) == 0u) {
                const int usable = s_cmd_purge_cell_usable != NULL
                    ? s_cmd_purge_cell_usable(cell_x, cell_y,
                                              s_cmd_purge_context)
                    : (level != NULL &&
                       level_cell_mask18_clear(level, cell_x, cell_y));
                if (usable != 0)
                    current->behaviour = 0;
            } else {
                static const int offsets[4][2] = {
                    {0, 0}, {0, -1}, {-1, -1}, {-1, 0}
                };
                int found = 0;
                int n;
                for (n = 0; n < 4; ++n) {
                    const int eligible = s_cmd_purge_neighbor_eligible != NULL
                        ? s_cmd_purge_neighbor_eligible(
                            cell_x + offsets[n][0], cell_y + offsets[n][1],
                            s_cmd_purge_context)
                        : (level != NULL && level_cell_walkable_flag(
                            level, cell_x + offsets[n][0],
                            cell_y + offsets[n][1]));
                    if (eligible != 0)
                        found = 1;
                }
                if (found) {
                    if (current->behaviour == 0)
                        current->param2 = (int32_t)0x42c80000u;
                } else {
                    current->behaviour = 0;
                }
            }
        }
        current = as_rec(current->global_next);
    }
    if (current != NULL) overflow = current;
    if (count > 1) {
        qsort(records, (size_t)count, sizeof(records[0]),
              s_cmd_purge_compare != NULL ? cmd_purge_compare_adapter
                                          : cmd_purge_compare_default);
    }
    if (count != 0) {
        for (i = 0; i + 1 < count; ++i)
            records[i]->global_next = records[i + 1];
        records[count - 1]->global_next = overflow;
        *head = records[0];
    }
}

/* LAB_00404250 is a pool-walker callback: argument one is a CmdRec and
 * argument two points at the caller's frame delta. The retail pool walk
 * tests each record's +0x64 self-link; the port live-slot sidecar represents
 * that same allocated-record set. */
// FUNCTION: LEGORR 0x00404250
static int cmd_pool_timer_delta_apply(CmdRec* record, void* delta_payload)
{
    const float delta = *(const float*)delta_payload;
    float timer18;
    if ((record->state & 0x80u) == 0u)
        record->now -= delta;
    memcpy(&timer18, &record->param2, sizeof(timer18));
    timer18 -= delta;
    memcpy(&record->param2, &timer18, sizeof(record->param2));
    return 0;
}

/* FUN_00406540 walks the pool and invokes LAB_00404250 only for allocated
 * records. The sidecar preserves the retail +0x64 self-link predicate. */
static void cmd_pool_timer_delta_walk(float* delta)
{
    int block_index;
    for (block_index = 0; block_index < s_pool_level; ++block_index) {
        CmdRec* const block = s_blocks[block_index];
        const uint32_t count = 1u << (block_index & 0x1fu);
        uint32_t index;
        if (block == NULL || s_block_live[block_index] == NULL) continue;
        for (index = 0u; index < count; ++index) {
            if (s_block_live[block_index][index] != 0u)
                (void)cmd_pool_timer_delta_apply(&block[index], delta);
        }
    }
}

/* LAB_00404280 (called by CmdTickGlobal through FUN_004377b0): the pool
 * callback always returns zero so the filtered live-entity walk visits every
 * slot.  The recovered outer gate and the chain-head expiry transitions are
 * represented here.  Record kinds with renderer/task-specific dispatch stay
 * in their own entity helpers; this callback does not substitute ai_tick. */
// FUNCTION: LEGORR 0x00404280
int cmd_entity_command_chain_tick(EntitySlot* entity, void* delta_payload)
{
    const float delta = *(const float*)delta_payload;
    CmdRec* head;
    LevelState* level;
    float actor_position[2];
    float target_position[2];
    int actor_cell[2];
    int target_cell[2];

    if (entity == NULL) return 0;
    s_global_enabled |= 4;

    /* Retail requires the active-type bit, a positive +0x340 value, and a
     * false 0x4295c0 predicate; attached, owned type-2, and the two flag
     * families below are excluded before touching the command chain. */
    if ((entity->flags_3e8 & 0x00400000u) == 0u ||
        entity->field_340 <= 0.0f ||
        entity_matches_current_selection_or_owner(entity) != 0 ||
        (entity->owner_36c != NULL && entity->type == 2) ||
        entity->attach_304 != NULL ||
        (entity->flags_3e4 & 0x08004000u) != 0u ||
        (entity->flags_3e0 & 0x00800000u) != 0u) {
        s_global_enabled &= ~4;
        return 0;
    }

    head = as_rec(entity->cmd_head);
    if (head == NULL) {
        entity->command_chain_idle_elapsed_338 += delta;
        if (entity->command_chain_idle_elapsed_338 >= 250.0f) {
            if (entity->type == 2 &&
                entity_matches_current_selection_or_owner(entity) == 0) {
                (void)entity_random_activity_attempt(entity);
            }
            entity->command_chain_idle_elapsed_338 = 0.0f;
        }
        else {
            /* The no-head arm walks DAT_004b3354 before adding this entity to
             * either pending candidate array. It attaches the first matching
             * global command; only an unmatched entity reaches the arrays. */
            {
                const int assignment =
                    cmd_entity_global_record_assign(entity);
                if (assignment == 0 || assignment == 2)
                    cmd_entity_pending_candidates_append(entity);
            }
        }
        s_global_enabled &= ~4;
        return 0;
    }
    entity->command_chain_idle_elapsed_338 = 0.0f;

    if ((head->state & CMD_STATE_DONE) != 0u) {
        (void)entity_command_chain_head_remove(entity);
        entity_action_sequence_reset(entity, 0);
        s_global_enabled &= ~4;
        return 0;
    }

    /* Type 0x0d is a timed chain marker.  Once its +0x14 timer reaches zero,
     * retail drops the head without running the regular action reset. */
    if (head->type == 0x0d && head->now <= 0.0f) {
        (void)entity_command_chain_head_remove(entity);
        s_global_enabled &= ~4;
        return 0;
    }

    if (head->type == 1) {
        EntitySlot* target = (EntitySlot*)head->payload;
        if (head->now <= 0.0f && target != NULL) {
            level = entity->owner_ctx != NULL ? entity->owner_ctx->level : NULL;
            if (level != NULL && level->ptr_terrain != NULL) {
                entity_position_to_terrain_cell(entity, level,
                                                &actor_cell[0], &actor_cell[1]);
                entity_position_to_terrain_cell(target, level,
                                                &target_cell[0], &target_cell[1]);
                if (actor_cell[0] == target_cell[0] &&
                    actor_cell[1] == target_cell[1]) {
                    head->now = 50.0f;
                }
                else {
                    (void)object_path_submit_to_grid_target(
                        entity, level, target_cell, NULL);
                    head->now = 50.0f;
                }
            }
        }
        s_global_enabled &= ~4;
        return 0;
    }
    else if (head->type == 2) {
        EntitySlot* target = (EntitySlot*)head->payload;
        if (head->now <= 0.0f && target != NULL &&
            entity->field_340 <= entity_stats_float_ate8(target)) {
            head->state |= CMD_STATE_DONE;
        }
        else if (head->now <= 0.0f && target != NULL) {
            entity_transform_position_get(entity, actor_position);
            entity_transform_position_get(target, target_position);
            {
                const float dx = actor_position[0] - target_position[0];
                const float dy = actor_position[1] - target_position[1];
                const float distance = (float)sqrt(dx * dx + dy * dy);
                if (distance <= 100.0f) {
                    head->now = entity_face_target_action_start(
                        entity, target, 2) ? 100.0f : 25.0f;
                }
                else {
                    level = entity->owner_ctx != NULL ?
                        entity->owner_ctx->level : NULL;
                    if (level != NULL && level->ptr_terrain != NULL) {
                        entity_position_to_terrain_cell(
                            entity, level, &actor_cell[0], &actor_cell[1]);
                        entity_position_to_terrain_cell(
                            target, level, &target_cell[0], &target_cell[1]);
                        if (actor_cell[0] != target_cell[0] ||
                            actor_cell[1] != target_cell[1]) {
                            (void)object_path_submit_to_grid_target(
                                entity, level, target_cell, NULL);
                        }
                        head->now = 50.0f;
                    }
                }
            }
        }
        s_global_enabled &= ~4;
        return 0;
    }

    /* The non-timed record-kind switch is the body’s main dispatch table.
     * Kind zero carries explicit cell coordinates; the other cases call the
     * matching route/activity leaf already represented by entity modules. */
    {
        int result;
        if (head->type == 0) {
            level = entity->owner_ctx != NULL ? entity->owner_ctx->level : NULL;
            if (level != NULL && level->ptr_terrain != NULL &&
                cmd_rec_cell_xy_get(head, &target_cell[0], &target_cell[1])) {
                result = object_path_submit_to_grid_target(
                    entity, level, target_cell, NULL);
            }
            else result = 0;
        }
        else if (head->type > 30) result = 1;
        else result = cmd_entity_chain_kind_dispatch(entity, head);

        if (result < 0) {
            /* An absent external owner must not consume its command record. */
            s_global_enabled &= ~4;
            return 0;
        }
        if (result != 0) {
            head->state |= 1u;
        }
        else {
            if (head->type == 8 && (head->state & 8u) != 0u)
                head->param2 = (int32_t)0x42c80000u;
            if ((head->state & 0x10u) != 0u && head->now == 0.0f)
                head->now = 1500.0f;
            (void)entity_command_chain_head_remove(entity);
        }
    }

    /* +0x338 is the idle/path accumulator. The portable entity mirror keeps
     * its source float even when the renderer-owned threshold action cannot
     * be created. */
    s_global_enabled &= ~4;
    return 0;
}

// FUNCTION: LEGORR 0x00402220
void cmd_tick_global_delta(float delta)
{
    int threshold;
    CmdRec* current;
    if (((uint32_t)s_global_enabled & 0x80000000u) != 0u) return;

    threshold = s_behaviour[0];
    if (s_behaviour[1] > threshold) threshold = s_behaviour[1];
    if (s_behaviour[4] > threshold) threshold = s_behaviour[4];
    /* DAT_004b3328 aliases behaviour entry 16 in the recovered data. */
    s_behaviour[16] = threshold + 1;

    cmd_pool_timer_delta_walk(&delta);

    for (current = s_global_head; current != NULL;
         current = as_rec(current->global_next)) {
        EntitySlot* entity;
        if (current->type != 10 ||
            (current->state & 0x200000u) != 0u || current->payload == NULL)
            continue;
        entity = (EntitySlot*)current->payload;
        if (entity->field_340 <= 0.0f || entity->field_340 >= 100.0f)
            current->state |= CMD_STATE_DONE;
    }

    cmd_global_list_purge(&s_global_head);
    cmd_global_list_purge(&s_global_tail);
    s_pending_assignment_count = 0u;
    s_pending_timer_count = 0u;
    s_pending_candidates_from_entity_walk = 1;
    (void)entity_pool_find_by_callback_filtered_current(
        cmd_entity_command_chain_tick, &delta);
    pending_command_entity_assignment_tick();
    pending_command_timer_fire_scan();
    s_pending_candidates_from_entity_walk = 0;
}

void cmd_tick_global(void)
{
    cmd_tick_global_delta(0.0f);
}

/* ------------------------------------------------------------------ *
 *  Idle queue table (IdleQueueRegister): type-2 entries.
 * ------------------------------------------------------------------ */

#define IDLE_TABLE_MAX 20
typedef struct IdleQueueRecord {
    EntitySlot* entity;
    float timer;
} IdleQueueRecord;
// GLOBAL: LEGORR 0x00557a84
static IdleQueueRecord s_idle[IDLE_TABLE_MAX];

// FUNCTION: LEGORR 0x00407450
static void idle_register(EntitySlot* e)
{
    if (e == NULL || e->etype != 2) {
        return;
    }
    int i = 0;
    while (i < IDLE_TABLE_MAX && s_idle[i].entity != e) {
        ++i;
    }
    if (i == IDLE_TABLE_MAX) {
        for (i = 0; i < IDLE_TABLE_MAX; ++i) {
            if (s_idle[i].entity == NULL) {
                s_idle[i].entity = e;
                break;
            }
        }
    }
    if (i < IDLE_TABLE_MAX) {
        s_idle[i].timer = 12.5f; /* corpus stores 0x41480000 at +4 */
    }
}

void idle_queue_register(EntitySlot* e)
{
    idle_register(e);
}

/* port hook: EntityPointerRegistriesRemove clears idle-queue records from
 * the window this table models; the corpus leaves the timer words. */
void idle_queue_clear(EntitySlot* e)
{
    for (int i = 0; i < IDLE_TABLE_MAX; ++i) {
        if (s_idle[i].entity == e) {
            s_idle[i].entity = NULL;
        }
    }
}
/* ------------------------------------------------------------------ *
 *  Queue executor (FUN_004520d0): 5-dword records, ping-pong buffered.
 * ------------------------------------------------------------------ */

#define CMD_QUEUE_CAP 0x800

typedef struct QueueRec {
    int        opcode;
    const void* a1;    /* arg1: entity ptr for entity ops */
    int        a2;
    int        a3;
    int        a4;
} QueueRec;

// GLOBAL: LEGORR 0x004eaf20
static QueueRec s_queue[2][CMD_QUEUE_CAP];
// GLOBAL: LEGORR 0x004fef20
static int      s_queue_count[2];
// GLOBAL: LEGORR 0x004fef28
static int      s_ping; /* active producer block (DAT_004fef28) */

void cmd_context_set(GameContext* ctx)
{
    s_ctx = ctx;
    if (entity_pool_callback_context_get() != ctx)
        entity_pool_callback_context_set(ctx);
    cmd_place_eligibility_ops_bind();
    s_ping = 0;
    for (int i = 0; i < 2; ++i) {
        s_queue_count[i] = 0;
    }
}

// FUNCTION: LEGORR 0x00452060
void cmd_produce(int op, const void* a1, int a2, const int* pair)
{
    int block = s_ping;
    if ((unsigned)s_queue_count[block] >= CMD_QUEUE_CAP) {
        return; /* queue full: silent drop (FUN_00452060) */
    }
    QueueRec* q = &s_queue[block][s_queue_count[block]++];
    /* b63 note: an explicit zero-fill phase + single pair null-test was
     * tried (corpus 0x452092..0x4520ba shape) and measured LOWER — the
     * toolchain's store/register scheduling diverges; the fused writes
     * below are the measured-best shape. */
    q->opcode = op;
    q->a1 = a1;
    q->a2 = a2;
    q->a3 = (pair != NULL) ? pair[0] : 0;
    q->a4 = (pair != NULL) ? pair[1] : 0;
}

void cmd_enqueue(int opcode, const EntitySlot* e, int param, int p1, int p2)
{
    int pair[2];
    pair[0] = p1;
    pair[1] = p2;
    cmd_produce(opcode, e, param, pair);
}

/* Cheat/debug keybinds (FUN_00452010). */
/* The byte key table at 0x004ff0bc has twelve slots before the five-dword
 * records beginning at 0x004ff0c8.  Keep the portable table within that
 * retail storage window. */
#define KEYBIND_MAX 12
typedef struct KeyBind {
    int key;
    int op, a1, a2, a3, a4;
} KeyBind;
static KeyBind s_keybinds[KEYBIND_MAX];
static int     s_keybind_count = 0;

// FUNCTION: LEGORR 0x00452010
void cmd_bind_key(int key, int op, int a1, int a2, int a3, int a4)
{
    if (s_keybind_count >= KEYBIND_MAX) {
        return;
    }
    KeyBind* b = &s_keybinds[s_keybind_count++];
    b->key = key & 0xff;
    b->op = op; b->a1 = a1; b->a2 = a2; b->a3 = a3; b->a4 = a4;
}

void cmd_keybind_tick(int (*keyheld)(int key))
{
    if (keyheld == NULL) {
        return;
    }
    for (int i = 0; i < s_keybind_count; ++i) {
        KeyBind* b = &s_keybinds[i];
        if (keyheld(b->key)) {
            cmd_produce(b->op, (const void*)(uintptr_t)b->a1,
                        b->a2, &b->a3);
        }
    }
}

// FUNCTION: LEGORR 0x004520d0
void cmd_queue_tick(void)
{
    int i;
    /* 1. Producer block: the held-key pre-pass (SelectionRoot step 1 —
     *    cheat-keybinds; the demo binds keys via cmd_bind_key). */
    uint32_t* engine_state = engine_current_state_word();
    if ((engine_state == NULL ||
         (*engine_state & ENGINE_STATE_TIME_FROZEN) == 0u) &&
        s_ctx != NULL && s_ctx->input_keyheld != NULL) {
        cmd_keybind_tick(s_ctx->input_keyheld);
    }

    /* 2. Remap the first dword of every queued record while the old bank
     *    is still active. This is FUN_0045d870 over each 0x14-byte record. */
    int block = s_ping;
    int count = s_queue_count[block];
    for (i = 0; i < count; ++i) {
        command_entity_reference_remap((uint32_t*)&s_queue[block][i]);
    }

    /* 3. Retire the old producer bank, flip the ping-pong selector, and
     *    clear the newly active bank before dispatching the saved count. */
    s_queue_count[block] = 0;
    s_ping = !s_ping;
    s_queue_count[s_ping] = 0;

    /* 4. The opcode switch (finding 06/10 §"The queue executor"). */
    for (i = 0; i < count; ++i) {
        QueueRec* q = &s_queue[block][i];
        EntitySlot* ent = (EntitySlot*)q->a1;
        switch (q->opcode) {
        case 5:
        case 6:
            cmd_create_move(ent, (const float*)&q->a3, q->a2);
            break;
        case 8:
            /* add up to N entities to the selection (FUN_00403270 path) */
            selection_add(s_ctx, ent);
            break;
        case 0x17:
        case 0x1d:
            cmd_create_task(ent);
            break;
        case 0x19:
            cmd_create_gated(ent, (void*)(uintptr_t)q->a2);
            break;
        case 0x2b:
            cmd_create_target(ent, (void*)(uintptr_t)q->a2);
            break;
        case 1:
            selection_candidate_filter_default(ent, q->a2);
            break;
        case 3:
            selection_clear(s_ctx);
            break;
        case 4:
            selection_entity_remove(ent);
            break;
        case 9:
            selection_command_dispatch(q->a2);
            break;
        case 0xb:
            media_min_distance_update_mode(1, NULL, 0);
            break;
        case 0xc:
            sound_alias_play(q->a2, 0);
            break;
        case 10:
            /* GlobalRenderRecordSetFromInput also needs its active view and
             * renderer hooks; the queue entry supplies neither. */
            break;
        case 0x13:
            /* CmdQueueFlush calls TerrainStampImpactHandler(a3, a4,
             * low16(a2), high16(a2)); the helper now reads the active level
             * and terrain owners directly. */
            (void)terrain_stamp_impact_handle_current(
                (uint32_t)q->a3, (uint32_t)q->a4,
                (int)((uint32_t)q->a2 & 0xffffu),
                (int)((uint32_t)q->a2 >> 16));
            break;
        case 0x15:
        case 0x1b:
        {
            int cell_xy[2];
            cell_xy[0] = q->a3;
            cell_xy[1] = q->a4;
            (void)entity_build_placement_command_setup(
                cell_xy, 0, NULL, 1, q->opcode == 0x1b);
            break;
        }
        case 0x1f:
        {
            GameContext* context = pending_scan_context_get();
            LevelState* level = context != NULL ? context->level : NULL;
            int owner_index;
            /* The reference ignores the queue payload here: it takes the
             * type-3 entity index from level +0xb0, uses that same index as
             * the spawned entity's owner, and samples the active terrain's
             * width and height. */
            if (level != NULL) {
                owner_index = (int)active_engine_field_b0_get(level);
                if (owner_index >= 0 &&
                    entity_at(context, 3, owner_index) != NULL) {
                    (void)unported_ledger_stubs::
                        emergence_random_spawn_attempt(owner_index,
                            owner_index, level_width(level),
                            level_height(level));
                }
            }
            break;
        }
        case 0x21:
            (void)lego::entity_conditional_type_five_spawn(ent);
            break;
        case 0x23:
            entity_random_valid_cell_command_create(ent);
            break;
        case 0x25:
            (void)unported_ledger_stubs::
                ledger_stub_00447210_EntityAdjacentPlacementCellSearch(
                    ent, (EntitySlot*)(uintptr_t)q->a2);
            break;
        case 0x2d: selection_state_tick(s_ctx); break;
        case 0x34:
        {
            int cell_xy[2];
            cell_xy[0] = q->a3;
            cell_xy[1] = q->a4;
            entity_build_placement_command_pair_create(cell_xy, 0);
            break;
        }
        case 0x36:
        {
            int cell_xy[2];
            cell_xy[0] = q->a3;
            cell_xy[1] = q->a4;
            entity_build_placement_command_batch_expand(cell_xy);
            break;
        }
        case 0x40:
            (void)selected_entity_state_monitor();
            break;
        default:
            /* SelectionRoot has no default dispatch branch. */
            break;
        }
    }
}

/* --- queue-adjacent leaves (see cmd.hpp) -----------------------------------
 * CommandQueueCancelOpcode: corpus shape — the active bank only,
 * swap-with-last removals, forward index never rechecking the slot a
 * swap landed on (so a swapped-in match stays if it lands behind the
 * cursor; the loop bound is the running count).
 * The comparison uses the same low 32-bit word as the 32-bit corpus even on
 * the native host, where entity pointers can be wider. */
// FUNCTION: LEGORR 0x00451f60
void cmd_queue_cancel_match(int value)
{
    const int block = s_ping;
    int count = s_queue_count[block];
    unsigned i = 0;
    while ((int)i < count) {
        if ((uint32_t)(uintptr_t)s_queue[block][i].a1 == (uint32_t)value) {
            count = count - 1;
            s_queue[block][i] = s_queue[block][count];
            s_queue_count[block] = count;
        }
        i = i + 1;
    }
}
// FUNCTION: LEGORR 0x00452580
const void* active_context_record_get(int* count_out)
{
    const int count = selection_count(s_ctx);
    if (count == 0) {
        if (count_out != NULL) *count_out = 0;
        return NULL;
    }
    if (count_out != NULL) {
        *count_out = count;
    }
    return (const void*)selection_record_array(s_ctx);
}

// FUNCTION: LEGORR 0x00452650
int cmd_array_find_value_index(const int* array, unsigned count, int value,
                               unsigned* index_out)
{
    if (count != 0) { /* deviation: the corpus tests the count first */
        if (array != NULL) { /* pinned anti-crash guard — the corpus
                              * dereferences blindly */
            for (unsigned i = 0; i != count; ++i) {
                if (array[i] == value) {
                    if (index_out != NULL) {
                        *index_out = i;
                    }
                    return 1;
                }
            }
        }
    }
    return 0;
}

int cmd_queue_active_count_peek(void)
{
    return s_queue_count[s_ping];
}

/* --- selection snapshot / dispatch (see cmd.hpp) -----------------------------
 * The owner read takes the EntitySlot's +0x36c mirror (pointer-sized; the
 * corpus reads the flat 32-bit word), and the flag reads take the
 * flags_3e8 mirror's low byte as the corpus's byte read. */
static const void* cmd_entity_owner_at(const EntitySlot* e)
{
    return e->owner_36c;
}
// FUNCTION: LEGORR 0x00452520
int selection_snapshot_create(void** array_out, unsigned* count_out)
{
    const int count = selection_count(s_ctx);
    if (count == 0) {
        return 0;
    }
    if (count_out != NULL) {
        *count_out = (unsigned)count;
    }
    void* copy = engine_heap_alloc((long)count * (long)sizeof(void*));
    if (array_out != NULL) {
        *array_out = copy;
    }
    if (copy != NULL) {
        EntitySlot* const* src = selection_record_array(s_ctx);
        const unsigned n = (unsigned)count & 0x3fffffffu;
        for (unsigned i = 0; i < n; ++i) {
            ((void**)copy)[i] = src[i];
        }
    }
    return 1;
}
// FUNCTION: LEGORR 0x00452d60
int selection_command_dispatch(int param)
{
    const void* sel = media_min_distance_context();
    if (sel != NULL) {
        const EntitySlot* s = (const EntitySlot*)sel;
        if ((s->flags_3e8 & 0x10u) != 0u
            && (s->type == 2 || cmd_entity_owner_at(s) != NULL)) {
            media_min_distance_update_mode(0, (void*)s, param);
            return 1;
        }
    }
    const int count = selection_count(s_ctx);
    for (int i = 0; i < count; ++i) {
        const EntitySlot* e = selection_record_array(s_ctx)[i];
        if (e == NULL) {
            continue; /* deviation: the corpus dereferences blindly */
        }
        if (e->type == 2 && cmd_entity_owner_at(e) != NULL) {
            e = (const EntitySlot*)cmd_entity_owner_at(e);
        } /* (the corpus checks the swapped-in entity's own owner word) */
        if ((e->flags_3e8 & 0x10u) != 0u
            && (e->type == 2 || cmd_entity_owner_at(e) != NULL)) {
            media_min_distance_update_mode(0, (void*)e, param);
            return 1;
        }
    }
    return 0;
}

/* --- random-cell producer / entity remove (see cmd.hpp) ----------------------
 * VC5 rand.obj supplies the shared stream in the classic x86 build; other
 * builds keep this subsystem's local LCG model. */
#if !defined(_MSC_VER) || (_MSC_VER >= 1200) || !defined(_M_IX86)
static uint32_t s_cmd_rand_state = 1;
#endif

static int cmd_rand15(void)
{
#if defined(_MSC_VER) && (_MSC_VER < 1200) && defined(_M_IX86)
    return rand();
#else
    s_cmd_rand_state = s_cmd_rand_state * 214013u + 2531011u;
    return (int)((s_cmd_rand_state >> 16) & 0x7fffu);
#endif
}
// FUNCTION: LEGORR 0x00452430
void entity_random_valid_cell_command_create(void* entity)
{
    const LevelState* lvl = (s_ctx != NULL) ? s_ctx->level : NULL;
    const int w = (lvl != NULL) ? level_width(lvl) : 0;
    const int h = (lvl != NULL) ? level_height(lvl) : 0;
    if (w <= 0 || h <= 0) {
        return; /* deviation: the corpus modulos zero on an absent level */
    }
    int x = 0, y = 0;
    int found = 0;
    for (unsigned tries = 0; tries < 1000u; ++tries) {
        x = cmd_rand15() % w;
        y = cmd_rand15() % h;
        if (level_cell_flag_8_bounded(lvl, x, y) != 0u) {
            found = 1;
            break;
        }
    }
    if (found != 0) {
        /* FUN_00452060 copies the two optional words from a stack pair. */
        int pair[2];
        pair[0] = x;
        pair[1] = y;
        cmd_produce(0x24, entity, 0, pair);
    }
}
// FUNCTION: LEGORR 0x00452c50
int selection_entity_remove(void* entity)
{
    uint32_t index;
    if (s_ctx == NULL || entity == NULL) {
        return 0;
    }
    const int count = selection_count(s_ctx);
    EntitySlot** arr = (EntitySlot**)selection_record_array(s_ctx);
    if (count == 0 || !selection_contains_entity_target(
            s_ctx, (EntitySlot*)entity, &index)) {
        return 0;
    }
    if (index == 0) {
        command_context_post_to_hud(0, NULL);
    }
    s_ctx->selection_count = count - 1;
    arr[index] = arr[count - 1];
    ((EntitySlot*)entity)->flags &= ~ENTITY_SELECTED;
    ((EntitySlot*)entity)->flags_3e8 &= ~ENTITY_SELECTED;
    return 1;
}

/* SelectionObjectDetach (FUN_004524b0): cancel queued commands for the
 * object, remove it from the selected set, and clear the current selection
 * context when it refers to that object. */
// FUNCTION: LEGORR 0x004524b0
void selection_object_detach(void* entity)
{
    if (entity == NULL) {
        return;
    }
    cmd_queue_cancel_match((int)(uintptr_t)entity);
    (void)selection_entity_remove(entity);
    if (media_min_distance_context() == entity) {
        media_min_distance_update_mode(1, NULL, 0);
    }
}

/* SelectedEntityDetachCallback (FUN_0041f6c0): capability bit 0 chooses
 * between the attachment detach path and RecordPairPoseCopyAndActivityRaise
 * for the carried object. */
// FUNCTION: LEGORR 0x0041f6c0
int selected_entity_detach_callback(EntitySlot* entity)
{
    EntitySlot* attached;
    uint32_t capabilities;
    capabilities = entity_capability_flags_get(entity);
    attached = (EntitySlot*)entity->carry_link_308;
    if ((capabilities & 1u) != 0u) {
        if ((entity->flags_3e0 & 0x400u) != 0u && attached != NULL) {
            unported_ledger_stubs::
                ledger_stub_0043fc20_RecordPairPoseCopyAndActivityRaise(
                    attached);
        }
    }
    else if ((entity->flags_3e0 & 0x400u) == 0u || attached == NULL
             || attached->type != 9) {
        unported_ledger_stubs::entity_attached_object_list_detach(entity, 0);
    }
    return 0;
}

/* --- selection filter / state monitor (see cmd.hpp) --------------------------
 * The corpus's selection-array append + flag-word set land on the port's
 * modeled selection storage; everything it would hand to the entity /
 * activity / visual layers is documented there as an unported seam. */
// FUNCTION: LEGORR 0x004526e0
int selection_candidate_filter(void* entity, int mode, int param_3)
{
    if (s_ctx == NULL || entity == NULL) {
        return 0; /* deviation: the corpus dereferences blindly */
    }
    EntitySlot* e = (EntitySlot*)entity;
    if (e->attach_304 != NULL) {
        e = (EntitySlot*)e->attach_304;
    }
    if (selection_candidate_eligibility_check(e) == 0) {
        return 0;
    }
    if (mode == 0
        && selection_candidate_already_selected_or_marked(s_ctx, e) != 0) {
        /* The command and HUD reset are available; this command-layer entry
         * has no EngineContext for the helper's threaded state-word write. */
        selection_clear_marker_and_reset_hud(e);
        return 0;
    }
    /* FUN_00452610(entity, NULL) tests +0x3e8 bit 0x200000, even if the
     * record is not currently present in the selected-pointer array. */
    if ((e->flags_3e8 & 0x200000u) != 0u) {
        return 0;
    }
    /* Corpus pool refresh: EntityPoolFindByCallbackFiltered is ported but
     * its callback is a corpus code literal (an unported seam). */
    if (!selection_add(s_ctx, e)) return 0;
    if (e->type != 4
        && (e->flags_3e0 & 0x24002000u) == 0u
        && (e->flags_3e4 & 0x100u) == 0u) {
        entity_action_sequence_reset(e, 0);
        entity_activity_records_refresh(e);
        if (e->type == 2 && param_3 != 0) {
            /* Type-2 turn issue: the follow camera's facing (portable via
             * the engine's view) plus EntityStateReset / turn-activity
             * issue — unported, and the cmd context carries no view. */
        }
    }
    if ((e->flags_3e8 & 0x80u) != 0u) {
        if ((entity_stats_word_get(e) & 0x20000u) == 0u
            || e->owner_36c != NULL) {
            /* Visual half: object lookup + the sound-route resource
             * update — hook-boundary seams (cited in ai.cpp), no state. */
            return 1;
        }
    }
    return 0;
}

/* SelectionCandidateFilterDefault (FUN_004526c0) is the exact mode-1
 * forwarding wrapper around SelectionCandidateFilter. */
// FUNCTION: LEGORR 0x004526c0
void selection_candidate_filter_default(void* entity, int mode)
{
    (void)selection_candidate_filter(entity, mode, 1);
}

// FUNCTION: LEGORR 0x00452cc0
int selected_entity_state_monitor(void)
{
    const int count = (s_ctx != NULL) ? s_ctx->selection_count : 0;
    /* Faithful corpus shape: the counter is not used inside the loop —
     * every pass inspects the first selection entry. */
    for (int i = 0; i < count; ++i) {
        EntitySlot* head = (count > 0)
            ? (EntitySlot*)selection_record_array(s_ctx)[0]
            : NULL;
        if (head == NULL) {
            continue;
        }
        if ((entity_stats_word_get(head) & 0x20000u) != 0u) {
            EntitySlot* owner = (EntitySlot*)head->owner_36c;
            if (owner != NULL) {
                owner->flags_3e4 &= 0xfffffff7u;
                owner->owner_36c = NULL;
            }
        }
        if (head->type == 4) {
            /* FUN_00409ff0 replaces tracked object pointers before teardown. */
            entity_destroy_tracked_pointer_cleanup(head);
        }
        else {
            if (head->type == 0xb) {
                /* FUN_0040d0a0 removes type-11 objects from the cell chain. */
                terrain_entity_cell_record_remove(head);
            }
            (void)entity_teardown(head);
        }
    }
    return count;
}

/* --- entity record schedulers (see cmd.hpp) ---------------------------------- */

/* CurrentLevelGet (FUN_00431560): the corpus's no-arg level-state touch.
 * The scored port of this address (camera_state_root, gods98/camera.cpp)
 * takes an EngineContext — a callee-signature barrier (b26) — so this
 * file-local no-op carrier reproduces the corpus's blind no-arg call at
 * the entry of 0x402670. NOTE (b46): the carrier must NOT carry the
 * annotation — duplicating a ledger-stub
 * address scores fine, but duplicating a *scored* // FUNCTION: claim
 * steals the ledger match away from camera_state_root. */
static void current_level_get(void)
{
    /* TODO(corpus): the level-state root touch; the corpus discards the
     * result and no ported state depends on it. */
}

// FUNCTION: LEGORR 0x00402670
void entity_command15_validated_record_queue(EntitySlot* e)
{
    current_level_get(); /* corpus: call 0x431560 on entry, result discarded */
    if (e != NULL && entity_flag40000_threshold_compare(e) != 0) {
        CmdRec* rec = (CmdRec*)cmd_alloc(0x1a);
        cmd_link_head(rec, 0xf);
        /* Corpus RMW schedule: mov eax,[rec+0x5c] / and al,0xef /
         * mov [rec+0x10],edi / or eax,0x40040 / mov [rec+0x5c],eax — the
         * payload store lands between the two state ops. Keeping the value
         * live in eax across that store would need a local (a /Od local
         * stack-spills, b45), so the port keeps the plain split; bit 4 is
         * disjoint from 0x40040, so the pair is value-identical to the
         * corpus's single (state & ~0x10) | 0x40040 fold. */
        rec->state = rec->state & ~0x10u; /* +0x5c, pre-payload half */
        rec->payload = e;                 /* +0x10 */
        rec->state = rec->state | 0x40040u; /* +0x5c, post-payload half */
    }
}
static int cmd_has_entity_task_record(const EntitySlot* entity)
{
    CmdRec* record;
    for (record = s_global_head; record != NULL;
         record = as_rec(record->global_next)) {
        if (record->type == 3 &&
            (const void*)record->payload == (const void*)entity) return 1;
    }
    for (record = s_global_tail; record != NULL;
         record = as_rec(record->global_next)) {
        if (record->type == 3 &&
            (const void*)record->payload == (const void*)entity) return 1;
    }
    for (record = as_rec(entity->cmd_head); record != NULL;
         record = as_rec(record->next)) {
        if (record->type == 3 &&
            (const void*)record->payload == (const void*)entity) return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x004026c0
void entity_deferred_work_schedule(EntitySlot* e)
{
    if (e == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    if (e->attach_304 == NULL && (e->flags_3ec & 0x100u) == 0u) {
        /* LAB_00402700 in EntityPoolCallbackSearch matches a live type-3
         * command whose +0x10 payload is this entity. The portable command
         * owner exposes live records through its pending and entity chains. */
        if (!cmd_has_entity_task_record(e)) {
            cmd_create_task(e);
        }
    }
}

/* --- registry accessors / match marker (see cmd.hpp) -------------------------
 * DAT_004b7ff4: the per-priority state table, zeroed by the (unported)
 * registry initializer loop — zero-init static, 27 entries. The preceding
 * command-count matrix occupies 0x12c0 dwords and ends at this table. */
// GLOBAL: LEGORR 0x004b34f4
uint32_t cmd_pending_command_counts[CMD_PENDING_COMMAND_COUNT_TABLE_SIZE];
// FUNCTION: LEGORR 0x00402000
void cmd_mark_matching(int type, int payload, const int coords[2])
{
    for (CmdRec* cur = s_global_head; cur != NULL;
         cur = as_rec(cur->global_next)) {
        if (cur->type == type) {
            /* corpus compares the record's +0x10 word (32-bit) against the
             * argument: the port matches the low-word view. */
            if (payload != 0 && (int32_t)(uintptr_t)cur->payload == payload) {
                cur->state |= 0x10000u;
            }
            if (coords != NULL) {
                int rx, ry;
                __builtin_memcpy(&rx, &cur->x, sizeof(rx));
                __builtin_memcpy(&ry, &cur->y, sizeof(ry));
                if (coords[0] == rx && coords[1] == ry) {
                    cur->state |= 0x10000u;
                }
            }
        }
    }
}
// FUNCTION: LEGORR 0x00402060
void cmd_set_kind(void* rec, int kind)
{
    /* Keep this cast at the call site: the original accesses the record's
     * words directly and has no out-of-line conversion helper. */
    CmdRec* r = (CmdRec*)rec;
    if ((r->state & 0x30000u) == 0u) {
        r->kind = kind;
        r->behaviour = s_behaviour[kind];
    } else {
        r->behaviour = 99;
        r->kind = kind;
    }
}
// FUNCTION: LEGORR 0x00402090
void priority_entry_order_value_set(int kind, int value)
{
    /* Config callers pass the registered 0..26 kind index. The reference
     * writes directly to DAT_004b32e8[kind] without a range check. */
    s_behaviour[kind] = value;
}
// FUNCTION: LEGORR 0x004020a0
void priority_entry_state_set(int kind, int value)
{
    /* Match the corpus's direct indexed write to DAT_004b7ff4[kind]. */
    s_priority_state[kind] = value;
}
// FUNCTION: LEGORR 0x004020b0
int priority_entry_state_get(int kind)
{
    /* Match the corpus's direct indexed read from DAT_004b7ff4[kind]. */
    return s_priority_state[kind];
}

void cmd_rec_state_set(void* rec, uint32_t state)
{
    if (rec != NULL) {
        as_rec(rec)->state = state;
    }
}

int cmd_rec_param2_get(const void* rec)
{
    return rec != NULL ? (int)cas_rec(rec)->param2 : 0;
}

/* --- registry flush / wrappers (see cmd.hpp) ---------------------------------
 * The "handle array" the corpus's TaskTableContentsRelease walks IS the
 * cmd pool block array (0x4b31c8 band, bounded by the freelist head at
 * 0x4b31f8 — corpus-verified at 0x004020c0), so the release walks the pool
 * statics directly rather than a duplicate model. */
// FUNCTION: LEGORR 0x00402100
void cmd_flush_all(int disable_mode)
{
    if (disable_mode != 0) {
        s_global_enabled &= ~2;
        return;
    }
    /* The entity callback sweep (LAB_004021f0) is the unported registry
     * seam. The corpus clears the full pending-command count matrix here. */
    s_global_enabled |= 2;
    s_global_head = NULL;
    s_global_tail = NULL;
    uint32_t* count = cmd_pending_command_counts;
    uint32_t remaining = CMD_PENDING_COMMAND_COUNT_TABLE_SIZE;
    while (remaining != 0u) {
        *count = 0u;
        ++count;
        --remaining;
    }
    for (int i = 0; i < PRIORITY_ENTRY_COUNT; ++i) {
        s_priority_state[i] = 0;
    }
}

int cmd_global_enabled_peek(void) { return s_global_enabled; }
// FUNCTION: LEGORR 0x004021e0
void* engine_memory_allocate(long size)
{
    /* The reference calls VC5's _malloc entry directly. */
#if defined(_MSC_VER) && defined(_M_IX86)
    return malloc((size_t)size);
#else
    return engine_heap_alloc(size);
#endif
}
// FUNCTION: LEGORR 0x00402210
void engine_memory_free(void* ptr)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    free(ptr);
#else
    engine_heap_free(ptr);
#endif
}
/* TaskTableContentsRelease (FUN_004020c0): frees every non-null pool
 * block pointer. The corpus increments esi by four and compares it with
 * s_free at 0x4b31f8 ("cmp esi, 0x4b31f8"), then clears that head and the
 * scheduler word at 0x4b8060. */
// FUNCTION: LEGORR 0x004020c0
void handle_array_release_and_reset(void)
{
    CmdRec** walk = s_blocks;
    CmdRec** const end = s_blocks + CMD_POOL_MAX_BLOCKS;
    do {
        if (*walk != NULL) {
            engine_heap_free(*walk);
        }
        if (s_block_live[walk - s_blocks] != NULL) {
            free(s_block_live[walk - s_blocks]);
            s_block_live[walk - s_blocks] = NULL;
        }
        ++walk;
    } while (walk < end);
    s_free = NULL;
    s_global_enabled = 0;
}
// FUNCTION: LEGORR 0x00402800
/* EntityCommand14BatchIssue (FUN_00402800): the corpus materializes the
 * flag by mutation, not by a sete — edi starts at 0, the count==0
 * fall-through stores 1 into it and reuses it as the loop bound (the
 * test/jne shape) — then strides the loop blindly: cmd_alloc(4) is
 * dereferenced with no NULL check (deviation removed: the port's guard
 * let an exhausted pool skip records the corpus would stride onto).
 * Corpus 0x402817 uses the unsigned `jbe` gate; keep the bound and loop
 * index unsigned so MSVC does not emit the port's signed `jle` for negatives. */
void entity_command14_batch_issue(int count)
{
    unsigned flag = 0;
    unsigned n = (unsigned)count;
    if (n == 0) {
        flag = 1u;
        n = 1;
    }
    for (unsigned i = 0; i < n; ++i) {
        CmdRec* rec = (CmdRec*)cmd_alloc(4);
        /* corpus: no alloc guard, the record is used blindly */
        cmd_set_kind(rec, 0x14);
        if (flag != 0u) {
            rec->state |= 0x40u;
        }
        cmd_link_tail(rec);
    }
}
// FUNCTION: LEGORR 0x00402170
void level_no_gather_object_setup(int mode)
{
    if (mode == 0) {
        entity_command14_batch_issue(0);
    }
    CmdRec* gather = (CmdRec*)cmd_alloc(0x18);
    /* Corpus 0x40218d..0x4021a8 uses the allocation blindly. The only
     * in-tree callers are the two test cases, both after cmd_init(). */
    cmd_set_kind(gather, 0x17);
    gather->state |= 0x40u;
    /* Corpus 0x4021a3..0x4021ad prepends this record to the separate timer
     * list through the +0x60 link; it does not call CmdLinkTail. */
    gather->next = s_global_tail;
    s_global_tail = gather;
    CmdRec* other = (CmdRec*)cmd_alloc(0x1b);
    /* Corpus 0x4021bb..0x4021c0 likewise links the allocation blindly. */
    cmd_link_head(other, 0);
    other->state = (other->state & ~0x10u) | 0x440u;
}

/* --- cancellation-ref and conditional issuers (see cmd.hpp) ------------------
 * The referenced-record slot (+0x58) is the type-0x13 "ref" pair the
 * shipped binary's spawn-dispatch switch allocates through CmdAlloc(0x13);
 * port-side producers leave it NULL, so both halves answer inert unless a
 * test stages the reference. */
// FUNCTION: LEGORR 0x00402cd0
void cmd_cancel_ref(void* rec)
{
    if (rec == NULL) {
        return; /* deviation: the corpus reads the type word blindly */
    }
    CmdRec* r = as_rec(rec);
    if (r->type == 0x13) {
        if (r->ref != NULL) {
            /* Corpus: dispatch the cancel callback over the entity pool
             * (LAB_00402d30) through EntityPoolCallbackSearch — the
             * pool-registry seam, which always misses in the port. */
        }
        return; /* a ref record's cancel is the seam; no marking walk */
    }
    if (r->ref == NULL) {
        return;
    }
    for (CmdRec* cur = s_global_head; cur != NULL;
         cur = as_rec(cur->global_next)) {
        if (cur->type == 0x13 && cur == (CmdRec*)r->ref) {
            cur->state |= 0x20u; /* the completion/cancel flag */
        }
    }
}
// FUNCTION: LEGORR 0x00402ec0
void entity_conditional_task_schedule(EntitySlot* e)
{
    if (e == NULL) {
        return; /* pinned anti-crash guard — the corpus reads the entity
                 * blindly; the test hands in NULL */
    }
    if ((entity_capability_flags_get(e) & 1u) != 0u
        && (e->flags_3e0 & 0x400u) == 0u
        && (e->flags_3ec & 0x10000u) == 0u) {
        CmdRec* rec = (CmdRec*)cmd_alloc(0x1e);
        /* Corpus: no null check on the allocation — the payload store and
         * the flag word ride blind. */
        rec->payload = e;
        e->flags_3ec |= 0x10000u;
        cmd_link_head(rec, 0x11);
        rec->state = (rec->state & ~0x40u) | 0x100010u;
    }
}
// FUNCTION: LEGORR 0x00402f30
void entity_threshold_progress_event_queue(void* payload, int flag)
{
    const int set_flag = (flag != 0);
    CmdRec* rec = (CmdRec*)cmd_alloc(10);
    if (rec == NULL) {
        return;
    }
    rec->payload = payload;
    rec->extra = 0x10;
    if (set_flag) {
        rec->state |= 0x200000u;
    }
    /* corpus kind: (-1 & 0xc) + 7 = 0x13 when the flag is set, else 7. */
    cmd_link_head(rec, set_flag ? 0x13 : 7);
}
// FUNCTION: LEGORR 0x00402f70
void entity_target_command_queue(EntitySlot* e, void* target)
{
    CmdRec* rec = (CmdRec*)cmd_alloc(0x11);
    /* Corpus: no null checks — the payload and chain slots ride blind. */
    rec->payload = target;
    CmdRec* tail = as_rec(cmd_chain_walk(e));
    if (tail != NULL) {
        tail->next = rec; /* corpus [tail+0x60] */
    } else {
        e->cmd_head = rec; /* corpus [e+0x2f0] */
    }
    /* The corpus writes only the chain pointer: no timestamp, no pending
     * clear, no idle registration (the append helper's extras). */
}

void cmd_rec_ref_set(void* rec, void* ref)
{
    if (rec != NULL) {
        as_rec(rec)->ref = ref;
    }
}

void* cmd_rec_ref_get(const void* rec)
{
    return rec != NULL ? cas_rec(rec)->ref : NULL;
}

void cmd_rec_type_set(void* rec, int type)
{
    if (rec != NULL) {
        as_rec(rec)->type = type;
    }
}
/* --- selection-record array / entity-kind issuer (see cmd.hpp) ---------------
 * FUN_00402600 snapshots the selected entity pointers onto the command
 * record, resets each entity's action sequence, and optionally detaches its
 * carried object. */
// FUNCTION: LEGORR 0x00402600
void entity_record_array_apply_zero_state(void* rec, int mode)
{
    if (rec == NULL) {
        return; /* deviation: the corpus writes the record blindly */
    }
    if (!selection_has_any(s_ctx)) {
        return; /* corpus: SelectedEntitySetHasAny gates the whole body */
    }
    CmdRec* r = as_rec(rec);
    r->state |= 0x140u; /* bits 6 and 8 */
    void* array = NULL;
    unsigned count = 0;
    if (selection_snapshot_create(&array, &count) == 0) {
        return;
    }
    r->buffer = array; /* owned: cmd_free releases it (no 0x200 flag) */
    r->count = count;
    for (unsigned i = 0; i < count; ++i) {
        EntitySlot* entry = (array != NULL)
            ? (EntitySlot*)((void**)array)[i] : NULL;
        entity_action_sequence_reset(entry, 0);
        if (mode != 0) {
            unported_ledger_stubs::entity_attached_object_list_detach(entry, 0);
        }
    }
}
// FUNCTION: LEGORR 0x00402960
void entity_kind_command_issue(EntitySlot* e)
{
    if (e == NULL) {
        return; /* deviation: the corpus reads the entity blindly */
    }
    if ((e->flags_3e4 & 0x10u) != 0u) {
        return; /* the latch already ran the command */
    }
    CmdRec* rec = (CmdRec*)cmd_alloc(0x12);
    if (rec == NULL) {
        return;
    }
    rec->payload = e;
    e->flags_3e4 |= 0x10u;
    const uint32_t cap = entity_capability_flags_get(e);
    if ((cap & 0x100u) == 0u) {
        /* Corpus 0x402960 calls FUN_00469e60 separately in each branch. */
        if ((entity_stats_word_get(e) & 0x2000000u) == 0u) {
            if ((entity_stats_word_get(e) & 0x1000000u) != 0u) {
                rec->extra = 2;
            }
        } else {
            if ((entity_stats_word_get(e) & 0x1000000u) != 0u) {
                rec->extra = 1;
            } else {
                rec->extra = 4;
            }
        }
    } else {
        rec->extra = 1;
    }
    if ((cap & 0x40u) != 0u && (e->flags_3e0 & 0x400u) == 0u) {
        rec->state |= 0x100000u;
    }
    /* Corpus argument: the inverted bit-20 answer (~state >> 0x14) & 1. */
    entity_record_array_apply_zero_state(rec, (int)((~rec->state) >> 0x14) & 1u);
    rec->state &= ~0x40u;
    cmd_link_head(rec, 0xc);
}

static CoordinatePairEntityCallback s_coordinate_pair_entity_callback;
static ArgumentEntityCallback s_argument_entity_callback;
static void* s_callback_dispatch_context;
static TaskCallbackEntityFn s_task_callback_entity_fn;
static void* s_task_callback_entity_context;
static TaskCallbackEntityFn s_pending_record_callback_fn;
static void* s_pending_record_callback_context;
static EntityTaskQueueFlushHooks s_entity_task_queue_flush_hooks;
void cmd_callback_dispatch_hooks_install(
    CoordinatePairEntityCallback coordinate_callback,
    ArgumentEntityCallback argument_callback, void* context)
{
    s_coordinate_pair_entity_callback = coordinate_callback;
    s_argument_entity_callback = argument_callback;
    s_callback_dispatch_context = context;
}

void task_callback_dispatch_hook_install(TaskCallbackEntityFn callback,
                                         void* context)
{
    s_task_callback_entity_fn = callback;
    s_task_callback_entity_context = context;
}

void pending_record_callback_dispatch_hook_install(
    TaskCallbackEntityFn callback, void* context)
{
    s_pending_record_callback_fn = callback;
    s_pending_record_callback_context = context;
}

void entity_task_queue_flush_hooks_set(
    const EntityTaskQueueFlushHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_entity_task_queue_flush_hooks, 0,
               sizeof(s_entity_task_queue_flush_hooks));
    }
    else {
        s_entity_task_queue_flush_hooks = *hooks;
    }
}

void cmd_kind_record_hooks_set(const CommandKindRecordHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_command_kind_record_hooks, 0,
               sizeof(s_command_kind_record_hooks));
    }
    else {
        s_command_kind_record_hooks = *hooks;
    }
}

struct CoordinatePairCallbackPayload {
    const void* coordinate_pair;
    int extra;
};

static int coordinate_pair_callback_adapter(EntitySlot* entity, void* payload)
{
    CoordinatePairCallbackPayload* const data =
        (CoordinatePairCallbackPayload*)payload;
    if (data == NULL || s_coordinate_pair_entity_callback == NULL) {
        return 0;
    }
    return s_coordinate_pair_entity_callback(entity, data->coordinate_pair,
                                              data->extra,
                                              s_callback_dispatch_context);
}

static int argument_callback_adapter(EntitySlot* entity, void* argument)
{
    if (s_argument_entity_callback == NULL) {
        return 0;
    }
    return s_argument_entity_callback(entity, argument,
                                      s_callback_dispatch_context);
}

static int task_callback_entity_adapter(EntitySlot* entity, void* payload)
{
    if (s_task_callback_entity_fn == NULL) return 0;
    return s_task_callback_entity_fn(entity, payload,
                                     s_task_callback_entity_context);
}

static int pending_record_callback_adapter(EntitySlot* entity, void* payload)
{
    if (s_pending_record_callback_fn == NULL) return 0;
    return s_pending_record_callback_fn(entity, payload,
                                       s_pending_record_callback_context);
}

// FUNCTION: LEGORR 0x00402a40
void coordinate_pair_callback_dispatch(const void* coord_pair)
{
    CoordinatePairCallbackPayload payload;
    payload.coordinate_pair = coord_pair;
    payload.extra = 0;
    entity_pool_callback_search(coordinate_pair_callback_adapter, &payload);
}

// FUNCTION: LEGORR 0x00402aa0
void argument_callback_dispatch(void* arg)
{
    entity_pool_callback_search(argument_callback_adapter, arg);
}

// FUNCTION: LEGORR 0x00403a10
void task_callback_dispatch(void* payload)
{
    entity_pool_callback_search(task_callback_entity_adapter, payload);
}

// FUNCTION: LEGORR 0x00403ef0
void pending_record_callback_dispatch(void* payload)
{
    int block_index;
    if (s_pending_record_callback_fn != NULL) {
        entity_pool_callback_search(pending_record_callback_adapter, payload);
        return;
    }

    /* FUN_00403ef0 calls the pool walker (0x406540), not the entity-pool
     * walker: LAB_00403f10 matches live type-3 records by +0x10 payload and
     * raises +0x5c bit 0x20. The host tracks live slots separately because
     * its +0x64 word is a typed staging field, not retail's self-link. */
    for (block_index = 0; block_index < s_pool_level; ++block_index) {
        CmdRec* const block = s_blocks[block_index];
        uint32_t record_index;
        const uint32_t capacity = 1u << (block_index & 0x1f);
        if (block == NULL || s_block_live[block_index] == NULL) continue;
        for (record_index = 0; record_index < capacity; ++record_index) {
            CmdRec* const candidate = &block[record_index];
            if (s_block_live[block_index][record_index] != 0u &&
                candidate->type == 3 &&
                candidate->payload == payload) {
                candidate->state |= 0x20u;
            }
        }
    }
}

/* LAB_00402bb0, called by EntityPoolCallbackSearch from
 * TerrainCellRefreshDispatch. The callback record is a 0x68-byte CmdRec:
 * its raw +8/+0xc coordinate words are the host `x`/`y` slots, and +0x5c is
 * `state`. CmdAlloc's retail +0x64 self-link is represented by the live-slot
 * sidecar so this scan never mistakes an EntitySlot or a free record for a
 * callback candidate. */
void cmd_pool_cell_refresh_dispatch(const int cell_xy[2], int mode)
{
    int block_index;
    if (cell_xy == NULL) return;
    for (block_index = 0; block_index < s_pool_level; ++block_index) {
        CmdRec* const block = s_blocks[block_index];
        const uint32_t capacity = 1u << (block_index & 0x1f);
        uint32_t record_index;
        if (block == NULL || s_block_live[block_index] == NULL) continue;
        for (record_index = 0; record_index < capacity; ++record_index) {
            CmdRec* const record = &block[record_index];
            int x, y;
            int type;
            if (s_block_live[block_index][record_index] == 0u) continue;
            type = record->type;
            if (type == 8) {
                const int has_type8_flag = (record->state & 8u) != 0u;
                if ((mode != 0) != has_type8_flag) continue;
            }
            else if (type != 9 && type != 11) {
                continue;
            }
            cmd_rec_cell_xy_get(record, &x, &y);
            if (x != cell_xy[0] || y != cell_xy[1]) continue;
            record->state |= 0x20u;
            level_tile_mark_current(cell_xy, 0);
        }
    }
}

// EntityTaskQueueFlush (FUN_004040a0).
// FUNCTION: LEGORR 0x004040a0
void entity_task_queue_flush(EntitySlot* entity)
{
    EntityTaskQueueFlushHooks* hooks = &s_entity_task_queue_flush_hooks;
    void* head;
    if (entity == NULL || (s_global_enabled & 6) != 0) return;
    if (hooks->remove_activity != NULL) {
        hooks->remove_activity(entity, 0, NULL, hooks->context);
    }
    else {
        entity_activity_record_remove_by_type(entity, 0, NULL);
    }
    head = entity->carry_link_308;
    if ((entity->flags_3e0 & 0x400u) != 0u && head != NULL &&
        cmd_rec_type(head) == 8u) {
        if (hooks->remove_activity != NULL) {
            hooks->remove_activity(entity, 9, head, hooks->context);
        }
        else {
            entity_activity_record_remove_by_type(entity, 9, head);
        }
    }
    if (hooks->queue_relink != NULL) {
        hooks->queue_relink(entity, hooks->context);
    }
    else {
        command_queue_sweep_relink(entity);
    }
    if (hooks->state_recompute != NULL) {
        hooks->state_recompute(entity, hooks->context);
    }
    else {
        pending_record_entity_reference_clear(entity);
    }
}

unsigned cmd_rec_count_get(const void* rec)
{
    return rec != NULL ? cas_rec(rec)->count : 0u;
}

int cmd_rec_extra_get(const void* rec)
{
    if (rec == NULL) {
        return -1;
    }
    return (int)cas_rec(rec)->extra;
}

/* --- terrain cell-command issuers (see cmd.hpp) ------------------------------
 * The pair is staged into the coordinate slots as the corpus's integers (the
 * move records keep floats there), so the getters read the int view. */
static void cmd_cell_record_stage(CmdRec* rec, int x, int y, int act)
{
    __builtin_memcpy(&rec->x, &x, sizeof(rec->x));
    __builtin_memcpy(&rec->y, &y, sizeof(rec->y));
    rec->cell_act = act;
    rec->cell_target = -1;
    rec->cell_count = 0;
    rec->state |= 0x400u;
}
// FUNCTION: LEGORR 0x00402510
void terrain_cell_command_create(int x, int y, int flag, int raise20)
{
    const LevelState* lvl = (s_ctx != NULL) ? s_ctx->level : NULL;
    if (lvl == NULL) {
        return; /* deviation: the corpus dereferences the level blindly */
    }
    if (level_cell_state_is_1(lvl, x, y) != 0) {
        return;
    }
    if (level_cell_state_is_6(lvl, x, y) != 0) {
        return;
    }
    if (level_cell_flag_4_current(x, y) == 0u) {
        return;
    }
    CmdRec* rec = (CmdRec*)cmd_alloc(8);
    if (rec == NULL) {
        return;
    }
    cmd_cell_record_stage(rec, x, y, 0);
    if (flag != 0) {
        rec->state |= 8u;
    }
    entity_record_array_apply_zero_state(rec, 1);
    cmd_link_head(rec, 3);
    const uint32_t prior = rec->state;
    rec->state = prior | 0x80u;
    if (raise20 != 0) {
        rec->state = prior | 0x20080u;
    }
    if (flag == 0) {
        /* The terrain half: the surface-type apply and the cell's mark. */
        LevelState* mlvl = (LevelState*)lvl;
        if (mlvl->ptr_terrain != NULL) {
            terrain_cell_surface_type_set(mlvl->ptr_terrain, x, y, 1);
        }
        level_cell_flag_800000_set(mlvl, x, y, 1);
    }
}
// FUNCTION: LEGORR 0x00402d50
int kind_nine_cell_command_issue(int x, int y, int mode)
{
    LevelState* lvl = (s_ctx != NULL) ? (LevelState*)s_ctx->level : NULL;
    if (lvl == NULL) {
        return 0; /* deviation: the corpus dereferences blindly */
    }
    if (level_tile_type(lvl, x, y) == 1) {
        return 0;
    }
    if (level_cell_walkable_flag(lvl, x, y) == 0) {
        return 0;
    }
    if (level_cell_flag_reserved(lvl, x, y) != 0u) {
        return 0;
    }
    CmdRec* rec = (CmdRec*)cmd_alloc(9);
    if (rec == NULL) {
        return 0;
    }
    cmd_cell_record_stage(rec, x, y, 8);
    if (mode == 0) {
        entity_record_array_apply_zero_state(rec, 1);
    }
    cmd_link_head(rec, 3);
    {
        const int cell_xy[2] = { x, y };
        terrain_cell_record_count_update(cell_xy, 8, 0, 0);
    }
    level_tile_mark(lvl, x, y, 1);
    if (lvl->ptr_terrain != NULL) {
        /* The corpus tail loses the setter's register arguments; the type
         * follows the 0x402510 shape. */
        terrain_cell_surface_type_set(lvl->ptr_terrain, x, y, 1);
    }
    return 1;
}
// FUNCTION: LEGORR 0x00402e40
void cell_command_type13_issue(int x, int y)
{
    const LevelState* lvl = (s_ctx != NULL) ? s_ctx->level : NULL;
    if (lvl == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    if (level_cell_flag_8_bounded(lvl, x, y) == 0u) {
        return;
    }
    CmdRec* rec = (CmdRec*)cmd_alloc(0x14);
    if (rec == NULL) {
        return;
    }
    /* Corpus 0x402e60..0x402e8e writes the pair and cell fields directly. */
    __builtin_memcpy(&rec->x, &x, sizeof(rec->x));
    __builtin_memcpy(&rec->y, &y, sizeof(rec->y));
    rec->cell_act = 0xd;
    rec->cell_target = -1;
    rec->cell_count = 0;
    rec->state |= 0x400u;
    entity_record_array_apply_zero_state(rec, 1);
    cmd_link_head(rec, 3);
    {
        const int cell_xy[2] = { x, y };
        terrain_cell_record_count_update(cell_xy, 0xd, 0, 0);
    }
}
// FUNCTION: LEGORR 0x00402fb0
void cell_command_type11_issue(int x, int y)
{
    CmdRec* rec = (CmdRec*)cmd_alloc(0xf);
    if (rec == NULL) {
        return;
    }
    /* Corpus 0x402fc2..0x402feb writes these fields before its helper calls. */
    __builtin_memcpy(&rec->x, &x, sizeof(rec->x));
    __builtin_memcpy(&rec->y, &y, sizeof(rec->y));
    rec->cell_act = 0xb;
    rec->cell_target = -1;
    rec->cell_count = 0;
    rec->state |= 0x400u;
    entity_record_array_apply_zero_state(rec, 1);
    cmd_link_head(rec, 4);
    {
        const int cell_xy[2] = { x, y };
        terrain_cell_record_count_update(cell_xy, 0xb, 0, 0);
    }
    if (s_ctx != NULL && s_ctx->level != NULL) {
        level_cell_secondary_bit8_set((LevelState*)s_ctx->level, x, y, 1);
    }
}

int cmd_rec_cell_xy_get(const void* rec, int* x, int* y)
{
    if (rec == NULL) {
        return 0;
    }
    const CmdRec* r = cas_rec(rec);
    if (x != NULL) {
        __builtin_memcpy(x, &r->x, sizeof(*x));
    }
    if (y != NULL) {
        __builtin_memcpy(y, &r->y, sizeof(*y));
    }
    return 1;
}

int cmd_rec_action_get(const void* rec)
{
    return rec != NULL ? (int)cas_rec(rec)->cell_act : -1;
}

int cmd_rec_target_get(const void* rec)
{
    return rec != NULL ? (int)cas_rec(rec)->cell_target : -2;
}

/* --- task-type-23 category-mask issuer (see cmd.hpp) -------------------------
 * The corpus's entity argument rides every read blindly; the port guards the
 * entity slot because the selector gate reads its stats record. The allocator's
 * result is stored blindly, +0x10 payload before +0x3c extra, the selector
 * riding the interleaved stores straight from the argument. */
// FUNCTION: LEGORR 0x00402890
void task_type23_create_for_category_mask(EntitySlot* e, int selector,
                                          int issue_records)
{
    if (e == NULL) {
        return; /* deviation: the corpus reads the entity blindly */
    }
    if (entity_flags_contain_requested_single_bit(e, selector) == 0) {
        return;
    }
    CmdRec* rec = (CmdRec*)cmd_alloc(0x17);
    /* Corpus: no null check on the allocation — the +0x10/+0x3c stores ride
     * the cmp blind, interleaved between range test and jump table. */
    rec->payload = e;
    rec->extra = (int32_t)selector;
    int kind;
    switch (selector) {
    case 1: case 2: case 4: kind = 0xc; break;
    case 8:                 kind = 3;   break;
    case 0x10:              kind = 7;   break;
    case 0x20:              kind = 4;   break;
    default:                kind = 0xd; break;
    }
    if (issue_records != 0) {
        entity_record_array_apply_zero_state(rec, 1);
    }
    cmd_link_head(rec, kind);
    rec->state &= ~0x40u; /* the corpus clears bit 6 after the link */
}

/* --- the task-kind filtered search (see cmd.hpp) -----------------------------
 * The corpus's pool walk hands the callback the slot pointer and the
 * argument block; the port models that argument block as the selector word. */
// FUNCTION: LEGORR 0x0043a7c0
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
// FUNCTION: LEGORR 0x00439240
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
// FUNCTION: LEGORR 0x00439200
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

/* --- the task-record queue producers (see cmd.hpp) ---------------------------
 * The pair staging uses the corpus's int view of the coordinate slots. */
static void cmd_pair_stage(CmdRec* rec, const int pair[2])
{
    if (pair != NULL) {
        __builtin_memcpy(&rec->x, &pair[0], sizeof(rec->x));
        __builtin_memcpy(&rec->y, &pair[1], sizeof(rec->y));
    }
}
// FUNCTION: LEGORR 0x004030b0
void coordinate_pair_event_queue(const int pair[2], void* payload)
{
    CmdRec* rec = (CmdRec*)cmd_alloc(0xc);
    if (rec == NULL) {
        return;
    }
    cmd_pair_stage(rec, pair);
    rec->event_payload = payload;
    cmd_link_head(rec, 8);
}
// FUNCTION: LEGORR 0x00403190
int object_event_type2_queue(void* payload)
{
    CmdRec* rec = (CmdRec*)cmd_alloc(0x13);
    /* Corpus: no null check on the allocation — the slots are stored blind. */
    rec->arg38 = payload;
    rec->ref = NULL; /* the corpus stores zero into the reference slot */
    entity_record_array_apply_zero_state(rec, 1);
    rec->state = (rec->state & ~0x40u) | CMD_STATE_PENDING;
    cmd_link_head(rec, 2);
    return 1;
}
// FUNCTION: LEGORR 0x004033f0
void coordinate_pair_task_record_queue(const int pair[2])
{
    CmdRec* rec = (CmdRec*)cmd_alloc(0);
    /* Corpus: no null check on the allocation — the pair rides the
     * coordinate slots blind, read straight from the caller's words. */
    *(int*)&rec->x = pair[0]; /* +0x08 raw dword, corpus inline store */
    *(int*)&rec->y = pair[1]; /* +0x0c raw dword */
    rec->state = (rec->state & ~CMD_STATE_PENDING) | 0x40u;
    cmd_set_kind(rec, 0x1a);
    cmd_link_tail(rec);
}
// FUNCTION: LEGORR 0x004034a0
void scalar_task_record_queue(void* scalar)
{
    CmdRec* rec = (CmdRec*)cmd_alloc(0x1d);
    /* Corpus: no null check on the allocation — the scalar rides the
     * +0x10 payload slot blind (the b33 twin at 0x4034e0 precedent). */
    rec->payload = scalar; /* +0x10 */
    cmd_set_kind(rec, 0x19);
    cmd_link_tail(rec);
    /* Corpus: mov eax,[rec+0x5c] / or al,0x40 / mov [rec+0x5c],eax. The
     * plain load-op-store spelling (not the |= compound) keeps the /Od
     * register round-trip instead of folding to or mem,imm (b45
     * global_flag_bit0_clear lesson). */
    rec->state = rec->state | 0x40u; /* +0x5c */
}
// FUNCTION: LEGORR 0x004034e0
void task_pair_record_enqueue(const int pair[2])
{
    CmdRec* rec = (CmdRec*)cmd_alloc(0x19);
    /* Corpus: no null check on the allocation — the pair rides the
     * coordinate slots blind, read straight from the caller's words. */
    *(int*)&rec->x = pair[0]; /* +0x08 raw dword, corpus inline store */
    *(int*)&rec->y = pair[1]; /* +0x0c raw dword */
    cmd_set_kind(rec, 0x18);
    cmd_link_tail(rec);
}
// FUNCTION: LEGORR 0x00403520
void task_record_kind10_queue(void* payload)
{
    CmdRec* rec = (CmdRec*)cmd_alloc(10);
    /* Corpus: no null check on the allocation — the payload rides the
     * +0x10 slot blind (the b33 twin at 0x4034e0 precedent). */
    rec->payload = payload; /* +0x10 */
    cmd_set_kind(rec, 0x16);
    cmd_link_tail(rec);
    /* Corpus: mov eax,[rec+0x5c] / or al,0x40 / mov [rec+0x5c],eax. The
     * plain load-op-store spelling (not the |= compound) keeps the /Od
     * register round-trip instead of folding to or mem,imm (b45
     * global_flag_bit0_clear lesson). */
    rec->state = rec->state | 0x40u; /* +0x5c */
}
// FUNCTION: LEGORR 0x00403430
void task_registry_remove_type0_entry_by_pair(const int pair[2])
{
    if (pair == NULL) {
        return; /* deviation: the corpus reads the pair blindly */
    }
    CmdRec* prev = NULL;
    for (CmdRec* cur = s_global_tail; cur != NULL;) {
        CmdRec* next = as_rec(cur->global_next);
        int cx = 0, cy = 0;
        cmd_rec_cell_xy_get(cur, &cx, &cy);
        if (cur->type == 0 && cx == pair[0] && cy == pair[1]) {
            if (prev != NULL) {
                prev->global_next = next;
            } else {
                s_global_tail = next;
            }
            cmd_free(cur, 0);
            return; /* corpus: the first match only */
        }
        prev = cur;
        cur = next;
    }
}

void cmd_tutorial_congregation_set(int x, int y, int enable)
{
    const int pair[2] = { x, y };
    if (enable != 0) coordinate_pair_task_record_queue(pair);
    else task_registry_remove_type0_entry_by_pair(pair);
}

// FUNCTION: LEGORR 0x00403560
void task_registry_remove_type10_entries_for_entity(const EntitySlot* e)
{
    CmdRec* prev = NULL;
    for (CmdRec* cur = s_global_tail; cur != NULL;) {
        CmdRec* next = as_rec(cur->global_next);
        if (cur->type == 10 && cur->payload == (void*)e) {
            if (prev != NULL) {
                prev->global_next = next;
            } else {
                s_global_tail = next;
            }
            cmd_free(cur, 0);
            cur = next;
            continue; /* corpus: every match is removed */
        }
        prev = cur;
        cur = next;
    }
}
// FUNCTION: LEGORR 0x004038c0
void* timed_task_record_initialize_and_link(void* payload, int arg1,
                                            int arg2, int arg3)
{
    CmdRec* rec = (CmdRec*)cmd_alloc(5);
    /* Corpus: no null check on the allocation — the slots are stored
     * blind (the b33 sibling at 0x4034e0 precedent). */
    rec->payload = payload;   /* +0x10 */
    rec->now = 0.0f;          /* +0x14: the corpus zeroes the timestamp dword */
    rec->cell_act = arg1;     /* +0x20 */
    rec->cell_target = arg2;  /* +0x24 */
    rec->cell_count = arg3;   /* +0x28 */
    cmd_link_head(rec, 10);
    /* Corpus: mov ecx,[+0x5c] / and ecx,0xffffffef / or ecx,0x4c0 /
     * mov [+0x5c],ecx — the plain round-trip form (b45 lesson). */
    rec->state = (rec->state & ~CMD_STATE_PENDING) | 0x4c0u; /* +0x5c */
    return rec;
}
// FUNCTION: LEGORR 0x004039d0
void task_record_kind11_timed_queue(void* payload)
{
    CmdRec* rec = (CmdRec*)cmd_alloc(0x11);
    /* Corpus: no null check on the allocation — the slots are stored
     * blind (the b33 sibling at 0x4034e0 precedent). */
    rec->payload = payload; /* +0x10 */
    rec->now = 0.0f;        /* +0x14: the corpus zeroes the timestamp dword */
    cmd_link_head(rec, 0);
    /* Corpus: mov ecx,[+0x5c] / and ecx,0xffffffef / or cl,0xc0 /
     * mov [+0x5c],ecx — the plain round-trip form (b45 lesson). */
    rec->state = (rec->state & ~CMD_STATE_PENDING) | 0xc0u; /* +0x5c */
}

void* cmd_rec_event_payload_get(const void* rec)
{
    return rec != NULL ? cas_rec(rec)->event_payload : NULL;
}

int cmd_rec_cell_count_get(const void* rec)
{
    return rec != NULL ? (int)cas_rec(rec)->cell_count : -1;
}

/* --- entity command-chain housekeeping (see cmd.hpp) --------------------------
 * DAT_004b3354 and DAT_004b3358 are separate roots that each use +0x60. */
static void cmd_raw_head_insert(CmdRec* rec)
{
    rec->global_next = s_global_head;
    rec->next = NULL;
    s_global_head = rec;
}
static void cmd_raw_tail_insert(CmdRec* rec)
{
    rec->global_next = s_global_tail;
    rec->next = NULL;
    s_global_tail = rec;
}
// FUNCTION: LEGORR 0x004041e0
void* entity_command_chain_head_remove(EntitySlot* e)
{
    /* The corpus's three tail-chain arms (here 0x40421b, 0x403a85 in
     * entity_drain_command_chain, 0x40413b in command_queue_sweep_relink)
     * all test state bit 13 (test ch,0x20), while the only matching
     * producer, cmd_mark_matching (0x402000), raises bit 16 (mov esi,
     * 0x10000 at 0x402019) and no corpus site ever sets bit 13 — so the
     * arm is dead in the shipped original and marked records fall through
     * to cmd_free.  Kept verbatim; the port's mark bit stays 0x10000. */
    if (e == NULL) {
        return NULL; /* deviation: the corpus reads the entity blindly */
    }
    CmdRec* head = as_rec(e->cmd_head);
    if (head == NULL) {
        return NULL;
    }
    e->cmd_head = head->next;
    head->next = NULL;
    if ((head->state & 0x10u) != 0u && (head->state & 0x20u) == 0u) {
        cmd_raw_head_insert(head); /* the pending-and-undone head moves on */
    } else if ((head->state & 0x2000u) != 0u /* corpus 0x40421b: test ch,0x20 -> state bit 13 */
               && (head->state & 0x20u) == 0u) {
        cmd_raw_tail_insert(head); /* raw onto DAT_004b3358 */
    } else {
        cmd_free(head, 0);
    }
    return e->cmd_head;
}
// FUNCTION: LEGORR 0x00403a50
void entity_drain_command_chain(EntitySlot* e)
{
    if (e == NULL) {
        return; /* deviation: the corpus reads the entity blindly */
    }
    if ((s_global_enabled & 2) != 0) {
        return; /* the corpus gate: DAT_004b8060 bit 1 blocks the drain */
    }
    CmdRec* cur = as_rec(e->cmd_head);
    while (cur != NULL) {
        CmdRec* next = as_rec(cur->next);
        if ((cur->state & 0x10u) != 0u && (cur->state & 0x20u) == 0u) {
            cur->next = NULL;
            cmd_link_head(cur, cur->kind); /* the pending head link (full) */
        } else if ((cur->state & 0x2000u) != 0u /* corpus 0x403a85: test ch,0x20 -> state bit 13 */
                   && (cur->state & 0x20u) == 0u) {
            cur->next = NULL;
            cmd_link_tail(cur); /* the bit-13 tail link (full) */
        } else {
            cmd_free(cur, 0);
        }
        cur = next;
    }
    e->cmd_head = NULL;
    /* The corpus's tail half — EntityPoolCallbackSearch with the
     * LAB_00403ad0 callback (bit 5 onto the records whose +0x10 payload
     * is this entity) — is the DAT_004b31c8 registry seam. */
}
// FUNCTION: LEGORR 0x00403af0
void entity_task_state_update_after_cleanup(EntitySlot* e, int flag)
{
    if (e->cmd_head == NULL) {
        return;
    }
    if (flag == 0) {
        /* Corpus 0x403b2d clears bit zero directly at CmdRec +0x5c. */
        ((CmdRec*)e->cmd_head)->state &= ~1u;
        return;
    }
    if (((CmdRec*)e->cmd_head)->type == 0) {
        entity_command_chain_head_remove(e);
        if (e->type == 3) {
            /* Corpus 0x403b1c..0x403b28 calls FUN_00452060 with a null pair. */
            cmd_produce(7, e, 0, NULL);
        }
    }
}
// FUNCTION: LEGORR 0x00403b40
void entity_pending_task_flag_clear(EntitySlot* e)
{
    if (e != NULL && e->cmd_head != NULL) {
        as_rec(e->cmd_head)->state &= ~1u;
    }
}
// FUNCTION: LEGORR 0x00403f60
void task_record_kind14_remove_if_head(EntitySlot* e)
{
    if (e != NULL && e->cmd_head != NULL
        && as_rec(e->cmd_head)->type == 0xe) {
        entity_command_chain_head_remove(e);
    }
}
// FUNCTION: LEGORR 0x00404100
void command_queue_sweep_relink(EntitySlot* e)
{
    if (e == NULL) {
        return; /* deviation: the corpus reads the entity blindly */
    }
    CmdRec* keep = NULL;
    CmdRec* cur = as_rec(e->cmd_head);
    while (cur != NULL) {
        CmdRec* next = as_rec(cur->next);
        if ((cur->state & 4u) != 0u) {
            cmd_free(cur, 0); /* the flag bit-2 record goes */
        } else if ((cur->state & 0x10u) != 0u && (cur->state & 0x20u) == 0u) {
            cmd_raw_head_insert(cur); /* raw onto the head list */
        } else if ((cur->state & 0x2000u) != 0u /* corpus 0x40413b: test ch,0x20 -> state bit 13 */
                   && (cur->state & 0x20u) == 0u) {
            cmd_raw_tail_insert(cur); /* raw onto DAT_004b3358 */
        } else {
            /* The corpus relinks every keeper through the HEAD keeper's
             * link slot only (iVar3 is assigned once), so an intermediate
             * keeper is orphaned — the corpus quirk, kept. */
            cur->next = NULL;
            if (keep == NULL) {
                keep = cur;
            } else {
                keep->next = cur;
            }
        }
        cur = next;
    }
    e->cmd_head = keep;
}
// FUNCTION: LEGORR 0x004041b0
int entity_task_chain_contains_kind(const EntitySlot* e, int type)
{
    if (e == NULL) {
        return 0;
    }
    for (const CmdRec* cur = cas_rec(e->cmd_head); cur != NULL;
         cur = cas_rec(cur->next)) {
        if (cur->type == type) {
            return 1;
        }
    }
    return 0;
}

/* --- the kind-7 zone word + cancel (see cmd.hpp) ----------------------------- */
int cmd_rec_zone_get(const void* rec)
{
    return rec != NULL ? (int)cas_rec(rec)->zone : 0;
}

void cmd_rec_zone_set(void* rec, int zone)
{
    if (rec != NULL) {
        as_rec(rec)->zone = zone;
    }
}

void cmd_link_global_head(void* rec, int kind)
{
    cmd_link_head(as_rec(rec), kind);
}

/* ValidatedRecordCloneQueue (FUN_004030e0): retain a command record only
 * when its leading code is present in the grouped code table. The optional
 * payload allocation uses the full count word, while the copy loop masks it
 * to 30 bits, matching the reference's encoded-count handling. */
// FUNCTION: LEGORR 0x004030e0
void cmd_validated_record_queue(void* record)
{
    CmdRec* const source = as_rec(record);
    CmdRec* copy;
    int group;
    uint32_t i;
    uint32_t copy_count;
    if (source == NULL
        || entity_grouped_code_table_find_value_index(
               (uint32_t)source->type, &group) == 0) {
        return;
    }
    source->state |= 0x1000u;
    copy = (CmdRec*)cmd_alloc(0x13);
    if (copy == NULL) return;
    if ((source->state & 0x100u) != 0u) {
        copy->buffer = engine_heap_alloc((long)(source->count << 2));
        copy_count = source->count & 0x3fffffffu;
        if (copy->buffer == NULL && copy_count != 0u) {
            cmd_free(copy, 0);
            return;
        }
        {
            uint32_t* destination = (uint32_t*)copy->buffer;
            const uint32_t* input = (const uint32_t*)source->buffer;
            for (i = copy_count; i != 0u; --i) {
                *destination = *input;
                ++destination;
                ++input;
            }
        }
        copy->count = source->count;
        copy->state |= 0x100u;
    }
    copy->state |= 0x10u;
    copy->arg38 = (void*)(unsigned long)(unsigned int)group;
    copy->ref = record;
    source->ref = copy;
    cmd_link_head(copy, source->kind);
}

/* CmdCancelByCoords (FUN_00403700) — see cmd.hpp. The corpus strides the
 * global chain through the +0x60 link; the port's global link lands in
 * global_next (deviation note in the struct header). */
// FUNCTION: LEGORR 0x403700
void cmd_cancel_by_coords(int zone, int probe, int x, int y)
{
    for (CmdRec* cur = s_global_head; cur != NULL;
         cur = (CmdRec*)cur->global_next) {
        if (cur->type == 7 && cur->zone == zone
            && (probe == 0
                || (x == cur->cell_act && y == cur->cell_target))) {
            cur->state |= CMD_STATE_DONE;
        }
    }
}

void cmd_rec_action_set(void* rec, int action)
{
    if (rec != NULL) {
        as_rec(rec)->cell_act = action;
    }
}

void cmd_rec_target_set(void* rec, int target)
{
    if (rec != NULL) {
        as_rec(rec)->cell_target = target;
    }
}

/* ObjectAttachedRecordAllocate (FUN_004033c0) — see cmd.hpp. */
// FUNCTION: LEGORR 0x004033c0
void object_attached_record_allocate(EntitySlot* e)
{
    if (e == NULL) {
        return; /* deviation: the corpus strides blindly */
    }
    void* rec = cmd_alloc(0x1c);
    void* tail = cmd_chain_walk(e);
    if (tail != NULL) {
        /* Corpus: mov [eax+0x60], esi — keep the store direct. */
        ((CmdRec*)tail)->next = rec;
        return; /* corpus: the taken branch ends on its own pop/pop/ret */
    }
    e->cmd_head = rec; /* corpus store at entity +0x2f0 */
}

/* PendingRecordCopyToEntityAndFlag (FUN_00406400) — see cmd.hpp. The corpus
 * copies the 26-dword record window, then restores the allocator self-link at
 * +0x64, clears the entity-chain link at +0x60, and sets state bit 0x200.
 * The timer at +0x18 is part of the copied window and remains unchanged. */
// FUNCTION: LEGORR 0x00406400
void* cmd_pending_record_copy(const void* rec)
{
    if (rec == NULL) {
        return NULL; /* deviation: the corpus strides the record blindly */
    }
    const CmdRec* src = cas_rec(rec);
    CmdRec* dst = (CmdRec*)cmd_alloc(src->type);
    if (dst == NULL) {
        return NULL; /* deviation: the corpus strides the allocation blindly */
    }
    *dst = *src;
    dst->next = NULL;        /* corpus +0x60 = 0 */
    dst->global_next = NULL; /* port-only bookkeeping: unattached */
    dst->state |= 0x200u;    /* corpus +0x17 |= 0x200 */
    return dst;
}

/* SelectedEntitiesAnyPredicateMatch (FUN_00431a30) / AnyObjectCanOccupy-
 * TerrainCell (FUN_00431ab0) — see cmd.hpp. The selection array entries
 * ride as native record pointers; the walkability mode-1 gates are the
 * seam pass, the placement gate reads the record's own flags. */
// FUNCTION: LEGORR 0x00431a30
int selected_entities_any_predicate_match(int cx, int cy)
{
    int count = 0;
    const void* recs = active_context_record_get(&count);
    const unsigned remaining_count = (unsigned)count;
    /* Corpus 0x431a54 tests the count as unsigned; zero is the empty case. */
    if (recs == NULL || remaining_count == 0u) {
        return 0;
    }
    const LevelState* lvl = (s_ctx != NULL)
        ? (const LevelState*)s_ctx->level : NULL;
    const EntitySlot* const* arr = (const EntitySlot* const*)recs;
    for (unsigned i = 0; i < remaining_count; ++i) {
        (void)arr[i]; /* the mode-1 entity gates are the seam pass */
        if (level_walkable(lvl, cx, cy) != 0) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00431ab0
int any_object_can_occupy_terrain_cell(int cx, int cy)
{
    int count = 0;
    const void* recs = active_context_record_get(&count);
    if (recs == NULL || count <= 0) {
        return 0;
    }
    const LevelState* lvl = (s_ctx != NULL)
        ? (const LevelState*)s_ctx->level : NULL;
    const EntitySlot* const* arr = (const EntitySlot* const*)recs;
    /* Corpus 0x431aef..0x431af5 advances the array pointer and uses an
     * unsigned loop bound. Keep the signed nonpositive guard above so a
     * negative port count still answers zero before the unsigned walk. */
    const EntitySlot* const* current = arr;
    for (unsigned i = 0; i < (unsigned)count; ++i, ++current) {
        if (terrain_cell_allows_placement(*current, lvl, cx, cy) != 0) {
            return 1;
        }
    }
    return 0;
}

/* TaskTableContentsRelease (FUN_00462960) — see cmd.hpp. */
// FUNCTION: LEGORR 0x00462960
int task_table_contents_release(void* record)
{
    if (record == NULL) {
        return 1; /* deviation: the corpus strides the handle blindly */
    }
    unsigned char* rec = (unsigned char*)record;
    uint32_t count;
    __builtin_memcpy(&count, rec + 0x1c, sizeof count);
    if (count != 0u) {
        uint32_t i = 0;
        for (;;) {
            void* entries = NULL;
            void* entry = NULL;
            void* sub = NULL;
            __builtin_memcpy(&entries, rec + 0x20, sizeof entries);
            if (entries != NULL) {
                entry = ((void**)entries)[i];
            }
            if (entry != NULL) {
                /* Corpus: the entry's +0x14 slot is read blindly; a NULL
                 * entry or NULL sub-slot answers "nothing to free" here. */
                __builtin_memcpy(&sub, (unsigned char*)entry + 0x14,
                                 sizeof sub);
                if (sub != NULL) {
                    engine_heap_free(sub);
                }
                engine_heap_free(entry);
            }
            ++i;
            __builtin_memcpy(&count, rec + 0x1c, sizeof count);
            if (count <= i) {
                break;
            }
        }
    }
    void* entries = NULL;
    __builtin_memcpy(&entries, rec + 0x20, sizeof entries);
    if (entries != NULL) {
        engine_heap_free(entries);
    }
    engine_heap_free(rec);
    return 1;
}

/* --- the cell-command type-3 issue + task batch append band (2026-10-25) -- */

/* CellCommandType3Issue (FUN_00403030) — see cmd.hpp. */
// FUNCTION: LEGORR 0x00403030
void cell_command_type3_issue(int x, int y)
{
    const LevelState* lvl = (s_ctx != NULL)
        ? (const LevelState*)s_ctx->level : NULL;
    if (lvl == NULL) {
        return; /* deviation: the corpus strides the level word blindly */
    }
    if (level_cell_walkable_flag(lvl, x, y) == 0) {
        return;                                    /* FUN_00432bd0 */
    }
    if (level_cell_flag_20_bounded(lvl, x, y) != 0u) {
        return;                                    /* FUN_00432d90 */
    }
    CmdRec* rec = (CmdRec*)cmd_alloc(0xb);         /* FUN_00406440 */
    if (rec == NULL) {
        return; /* deviation: the corpus strides the allocator's word */
    }
    int cx = x, cy = y;
    __builtin_memcpy(&rec->x, &cx, sizeof(rec->x));   /* +0x08 raw dword */
    __builtin_memcpy(&rec->y, &cy, sizeof(rec->y));   /* +0x0c raw dword */
    entity_record_array_apply_zero_state(rec, 1);     /* FUN_00402600 */
    cmd_link_head(rec, 6);                            /* FUN_00404030 */
    /* Corpus tail: the current level-height read (FUN_00431570) feeds the
     * surface-type setter whose register arguments the corpus tail loses —
     * the type follows the 0x402510 shape, as pinned by the 0x402d50 twin. */
    if (lvl->ptr_terrain != NULL) {
        terrain_cell_surface_type_set(lvl->ptr_terrain, x, y, 1);
    }
}

/* EntityTaskBatchAppend (FUN_00403650) — see cmd.hpp. */
// FUNCTION: LEGORR 0x00403650
void entity_task_batch_append(EntitySlot* const* entities, int count,
                              void* payload)
{
    for (int i = 0; i < count; ++i) {
        void* raw = cmd_alloc(1);                   /* FUN_00406440 */
        if (raw == NULL) {
            return; /* deviation: the corpus strides the allocator's word */
        }
        CmdRec* rec = as_rec(raw);
        rec->payload = payload;                     /* +0x10 */
        rec->now = 0.0f;                            /* +0x14 raw zero dword */
        EntitySlot* e = entities[i];
        if (e == NULL) {
            continue; /* deviation: the corpus strides the entity word */
        }
        command_queue_sweep_relink(e);              /* FUN_00404100 */
        CmdRec* tail = as_rec(cmd_chain_walk(e));   /* FUN_00404190 */
        if (tail == NULL) {
            e->cmd_head = rec;                      /* entity +0x2f0 head */
        } else {
            tail->next = rec;                       /* chain tail +0x60 */
        }
    }
}

/* DeferredOListEntityQueueAdd (FUN_00437c20) — see cmd.hpp. The corpus band
 * is a 200-slot array of 11-dword entries ending at its own counter word;
 * the pick tail (FUN_00431570 feeding the FUN_0044f6d0 grid-pick boundary)
 * only lands its ftol of the record's float pair in entry words 0/1, so
 * the port pins that shape and models the boundary's word read through
 * the level mirror (deviations: the corpus's register-passed tail extras
 * are not modeled, the band's past-capacity writes into the counter word
 * are dropped, NULL records crash the corpus, and NULL name parameters
 * leave stale corpus words where the port keeps zero). */
#define DEFERRED_OLIST_SLOTS 200

struct DeferredOListEntry {
    uint32_t w[9];    /* entry +0x00 .. +0x20 (the 11-dword stride)   */
    char*    names[2]; /* widened mirrors of entry +0x24 / +0x28       */
};

static DeferredOListEntry s_deferred_olist[DEFERRED_OLIST_SLOTS];
static uint32_t           s_deferred_olist_next; /* DAT_004ea7a4 */

// FUNCTION: LEGORR 0x00437c20
void deferred_olist_entity_queue_add(GameContext* ctx, const uint32_t record[2],
                                     int p1, int p2, int p3, int p5, int p6,
                                     const char* name1, const char* name2)
{
    const uint32_t slot = s_deferred_olist_next++;
    if (ctx != NULL && ctx->level != NULL) {
        (void)level_height((const LevelState*)ctx->level); /* boundary read */
    }
    if (record == NULL || slot >= DEFERRED_OLIST_SLOTS) {
        return; /* deviations: pinned above */
    }
    DeferredOListEntry* e = &s_deferred_olist[slot];
    float x, y;
    __builtin_memcpy(&x, &record[0], sizeof(x));
    __builtin_memcpy(&y, &record[1], sizeof(y));
    e->w[0] = (uint32_t)float_to_long(x); /* the pick's cell word pair    */
    e->w[1] = (uint32_t)float_to_long(y);
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

uint32_t deferred_olist_entry_word(int slot, int word)
{
    if (slot < 0 || slot >= DEFERRED_OLIST_SLOTS || word < 0 || word > 8) {
        return 0u;
    }
    return s_deferred_olist[slot].w[word];
}

char* deferred_olist_entry_string(int slot, int which)
{
    if (slot < 0 || slot >= DEFERRED_OLIST_SLOTS || which < 0 || which > 1) {
        return NULL;
    }
    return s_deferred_olist[slot].names[which];
}

uint32_t deferred_olist_counter_peek(void)
{
    return s_deferred_olist_next;
}

void deferred_olist_reset(void)
{
    __builtin_memset(s_deferred_olist, 0, sizeof(s_deferred_olist));
    s_deferred_olist_next = 0u;
}

/* --- HUD post / menu-selection chain (2026-10-25) --------------------------
 * The chain lands on the port's modeled half of the corpus's HUD post: the
 * map-store model in frontend.* owns the coordinate/kind words, and the
 * tracked surface word (DAT_004de1f0) is a module static here. The corpus
 * call sites lose the surface-type packer's register arguments (pinned by
 * the cmd.cpp tail precedent), so the pair shape applies here: the tracked
 * word rides the setter as the type on the tracked half, and the parameter
 * half applies type 4 and tracks the prior value when it is not 4.
 * Deviations: the no-level and NULL-coordinate paths stay inert (the
 * corpus dereferences the current level and coordinate blindly), and the
 * NULL terrain handle reads the zero state through the guarded leaves. */
static uint32_t s_menu_surface_track; /* DAT_004de1f0 */

static int menu_kind_gated(int kind)
{
    return kind == 1 || kind == 2 || kind == 3 || kind == 4
        || kind == 5 || kind == 7;
}

// FUNCTION: LEGORR 0x0041b300
void menu_selection_resource_update(int kind, const uint32_t coord[2])
{
    /* FUN_0041b300 */
    if (s_ctx == NULL || s_ctx->level == NULL) {
        return; /* deviation: no current level, no corpus chain */
    }
    void* terrain = s_ctx->level->ptr_terrain;
    if (menu_kind_gated((int)map_store_kind_peek())) {
        const int x = (int)map_store_last_word(0);
        const int y = (int)map_store_last_word(1);
        if (terrain_cell_surface_type_get(terrain, x, y) == 0) {
            s_menu_surface_track = 0u;
        } else {
            terrain_cell_surface_type_set(terrain, x, y,
                                          (int)s_menu_surface_track);
        }
    }
    if (menu_kind_gated(kind) && coord != NULL) {
        const uint32_t prior = (uint32_t)terrain_cell_surface_type_set(
            terrain, (int)coord[0], (int)coord[1], 4);
        if (prior != 4u) {
            s_menu_surface_track = prior;
        }
    }
}

// FUNCTION: LEGORR 0x0041b2d0
void command_context_post_to_hud(int kind, const uint32_t coord[2])
{
    /* FUN_0041b2d0 */
    global_state_low_byte_clear_and_refresh(); /* FUN_00435a20 */
    menu_selection_resource_update(kind, coord);
    store_selected_map_coordinate(kind, coord); /* FUN_0041ea50 */
}

// FUNCTION: LEGORR 0x0041e750
void selection_clear_request_and_hud_reset(void)
{
    /* FUN_0041e750 enqueues command 3 with a null entity and pair, then posts
     * the HUD context with kind 0 / no coordinate. */
    cmd_produce(3, NULL, 0, NULL); /* FUN_00452060 */
    command_context_post_to_hud(0, NULL);
}

uint32_t menu_surface_track_peek(void)
{
    return s_menu_surface_track;
}

/* --- resource/ore counter pair (2026-10-25) --------------------------------
 * The counter half runs under the corpus's current-level gate; the
 * overlay advance half runs outside it (corpus order), so the guarded
 * no-level path keeps stepping the float. */
// FUNCTION: LEGORR 0x0041f9a0
void level_resource_balance_add_and_sound(int selector)
{
    if (s_ctx != NULL && s_ctx->level != NULL) {
        LevelState* lvl = (LevelState*)s_ctx->level;
        if (selector == 0) {
            level_set_int(lvl, LEVEL_OFF_RES_A,
                          level_int(lvl, LEVEL_OFF_RES_A) + 1);
        } else {
            level_set_int(lvl, LEVEL_OFF_RES_B,
                          level_int(lvl, LEVEL_OFF_RES_B) + 1);
        }
    }
    ui_overlay_animation_advance(1, selector != 0 ? 5 : 1);
}

/* UpgradeResourceDebit (FUN_0041f9e0) — the debit half of the counter pair.
 * Corpus order: the counter store runs under the level gate, then the
 * overlay advance runs outside it. Pin: the debit half steps the advance
 * with mode 0 (the add half is mode 1), and a nonzero selector steps the
 * constant 5 regardless of the amount — selector 0 steps the amount.
 * Deviation: the corpus's DAT-null gate is the port's ctx/level guard. */
// FUNCTION: LEGORR 0x0041f9e0
void upgrade_resource_debit(int selector, int amount)
{
    if (s_ctx != NULL && s_ctx->level != NULL) {
        LevelState* lvl = (LevelState*)s_ctx->level;
        if (selector == 0) {
            level_set_int(lvl, LEVEL_OFF_RES_A,
                          level_int(lvl, LEVEL_OFF_RES_A) - amount);
        } else {
            level_set_int(lvl, LEVEL_OFF_RES_B,
                          level_int(lvl, LEVEL_OFF_RES_B) - amount);
        }
    }
    ui_overlay_animation_advance(0, selector != 0 ? 5 : amount);
}

/* --- the upgrade-name table band (2026-10-25) ------------------------------
 * UpgradeNameEntryFind (FUN_0042cd50): a case-insensitive scan over the
 * loader-owned name table, answering the indexed record's address. The
 * loader (the config seam) has no ported owner, so the three globals — the
 * name-pointer array DAT_005561f8, the record block DAT_005561e4, and the
 * entry count DAT_0055628c — ride zero-init own storage behind an install
 * hook. Deviations: the region span (0x1f8..0x28c, 37 dwords) sizes the
 * window to 36 installable entries (the corpus strides its count bound
 * blindly); NULL names (the searched name or a table entry) skip the
 * compare. A cold table answers NULL like the corpus's zero-count return. */
#define UPGRADE_NAME_TABLE_MAX 36 /* span-derived window guard */
static const char* s_upgrade_names[UPGRADE_NAME_TABLE_MAX];
// GLOBAL: LEGORR 0x005561e4
static void* s_upgrade_records;
static unsigned s_upgrade_count;

void upgrade_name_table_install(void* records, const char* const* names,
                                int count)
{
    int i;
    if (names == NULL || count < 0) {
        count = 0; /* deviation: the corpus strides blindly */
    }
    if (count > UPGRADE_NAME_TABLE_MAX) {
        count = UPGRADE_NAME_TABLE_MAX;
    }
    for (i = 0; i < count; ++i) {
        s_upgrade_names[i] = names != NULL ? names[i] : NULL;
    }
    for (i = count; i < UPGRADE_NAME_TABLE_MAX; ++i) {
        s_upgrade_names[i] = NULL;
    }
    s_upgrade_records = records;
    s_upgrade_count = (unsigned)count;
}

// FUNCTION: LEGORR 0x0042cd50
const void* upgrade_name_entry_find(const char* name)
{
    /* Install clamps negatives to zero. Corpus 0x42cd5b/0x42cd7f uses
     * unsigned count gates and an unsigned index bound. */
    if (name == NULL || s_upgrade_count == 0u || s_upgrade_records == NULL) {
        /* deviation: the corpus strides a dangling record base blindly */
        return NULL;
    }
    for (unsigned i = 0; i < s_upgrade_count; ++i) {
        const char* entry = s_upgrade_names[i];
        /* Corpus 0x42cd6b calls the annotated strcasecmp_lite twin at
         * 0x48ff10; this caller observes equality only. */
        if (entry != NULL && strcasecmp_lite(name, entry) == 0) {
            return (const char*)s_upgrade_records + (size_t)i * 16;
        }
    }
    return NULL;
}

/* --- the kind-six message sweep timer (2026-10-25) -------------------------
 * KindSixRandomMessageDispatchSweep (FUN_0040f0e0): subtract the frame delta
 * from the sweep timer, and once it reaches zero draw up to one hundred
 * random grid pairs modulo the current level's width and height; a pair the
 * erosion eligibility/mutation path accepts dispatches message 6 with the
 * coordinate pair and stops the search. Either way the sweep re-arms the timer
 * from one final draw at (draw % 50) + 750.0 and answers draw / 50. The
 * corpus's flag fold over the three timer comparisons pins the positive-
 * timer return to zero (a finite positive trips no comparison; NaN rides
 * the 1024 bit). Deviations: the corpus strides a missing current-level
 * handle blindly and would take a draw modulo zero — the port skips the
 * cell loop without the level (the re-arm draw still runs, the corpus's
 * re-arm is level-independent) and reads the dims through the word model. */
// GLOBAL: LEGORR 0x004d78c4
static float s_sweep_timer;

void kind_six_sweep_timer_install(float value) { s_sweep_timer = value; }
float kind_six_sweep_timer_peek(void) { return s_sweep_timer; }

// FUNCTION: LEGORR 0x0040f0e0
uint32_t kind_six_random_sweep_dispatch(float delta)
{
    s_sweep_timer = s_sweep_timer - delta;
    const uint32_t neg = (s_sweep_timer < 0.0f) ? 1u : 0u;
    const uint32_t nan = (s_sweep_timer != s_sweep_timer) ? 1u : 0u;
    const uint32_t zero = (s_sweep_timer == 0.0f) ? 1u : 0u;
    uint32_t answer = (neg << 8) | (nan << 10) | (zero << 14);
    if (neg != 0u || zero != 0u) {
        answer = 0;
        const LevelState* lvl =
            (s_ctx != NULL) ? (const LevelState*)s_ctx->level : NULL;
        const int w = (lvl != NULL) ? level_int(lvl, LEVEL_OFF_WIDTH) : 0;
        const int h = (lvl != NULL) ? level_int(lvl, LEVEL_OFF_HEIGHT) : 0;
        if (w > 0 && h > 0) {
            for (uint32_t n = 0; n < 100; ++n) {
                int cell[2];
                cell[0] = (int)(short)rng_thunk_draw15() % w;
                cell[1] = (int)(short)rng_thunk_draw15() % h;
                const int spawn = lava_erosion_cell_eligible_and_mutate(
                    cell, 1);
                if (spawn != 0) {
                    message_dispatch(6, 0, 0, cell);
                    break;
                }
            }
        }
        const uint32_t draw = rng_thunk_draw15();
        answer = (uint32_t)((int)(short)draw / 0x32);
        s_sweep_timer = (float)((int)(short)draw % 0x32) - -750.0f;
    }
    return answer;
}

// FUNCTION: LEGORR 0x0043a910
void simulation_mode_counter_add(int mode, int count)
{
    (void)mode; /* register residue: the corpus ignores it, count only */
    /* Deviation: the corpus loops `count != 0` down by one, so a negative
     * count runs away; the port counts toward zero. */
    for (int i = count; i != 0; i = i > 0 ? i - 1 : i + 1) {
        level_resource_balance_add_and_sound(0);
    }
}

/* --- command sample stats -----------------------------------------------
 * CommandSampleStatsAccumulate (FUN_0046b660) operates on one 0x27c-byte
 * record per type. The record's twenty sample pointers begin at +0x20c;
 * its count/head/window-total/cumulative-total/high-water words live at
 * +0x25c..+0x26c. The initialization loop at 0x46b3xx clears 0x1839 dwords,
 * exactly 39 records of 0x9f dwords each. Keep that physical layout here;
 * command_sample_stats_peek() below exposes the compact 0..24 inspection
 * view used by port callers. The reference blindly writes past the twenty
 * pointer log when its count exceeds twenty. Preserve the count but bound
 * those out-of-record writes in the portable implementation. */
#define CMD_STATS_KINDS 39u
#define CMD_STATS_LOG_DWORD 131u /* +0x20c */
#define CMD_STATS_LOG_DWORDS 20u
#define CMD_STATS_COUNT_DWORD 151u /* +0x25c */
#define CMD_STATS_HEAD_DWORD 152u /* +0x260 */
#define CMD_STATS_WINDOW_DWORD 153u /* +0x264 */
#define CMD_STATS_TOTAL_DWORD 154u /* +0x268 */
#define CMD_STATS_HIGH_DWORD 155u /* +0x26c */
#define CMD_STATS_RECORD_DWORDS 159u /* stride +0x27c */

static uint32_t s_cmd_stats[CMD_STATS_KINDS][CMD_STATS_RECORD_DWORDS];

// FUNCTION: LEGORR 0x0046b660
void command_sample_stats_accumulate(int kind, const void* sample)
{
    if (kind < 0 || (unsigned)kind >= CMD_STATS_KINDS) {
        return;
    }
    uint32_t* slot = s_cmd_stats[kind];
    if (sample == NULL) {
        slot[CMD_STATS_HEAD_DWORD] = 0u;
        slot[CMD_STATS_TOTAL_DWORD] = 0u;
    } else {
        uint32_t v4 = 0u, v8 = 0u;           /* sample+4, sample+8 */
        __builtin_memcpy(&v4, (const char*)sample + 4, sizeof v4);
        __builtin_memcpy(&v8, (const char*)sample + 8, sizeof v8);
        if ((int)slot[CMD_STATS_HEAD_DWORD] < (int)v8) {
            slot[CMD_STATS_WINDOW_DWORD] = (uint32_t)(
                (int)slot[CMD_STATS_WINDOW_DWORD] -
                (int)slot[CMD_STATS_HEAD_DWORD]);
            slot[CMD_STATS_HEAD_DWORD] = v8;
            slot[CMD_STATS_WINDOW_DWORD] = (uint32_t)(
                (int)slot[CMD_STATS_WINDOW_DWORD] + (int)v8);
        }
        slot[CMD_STATS_TOTAL_DWORD] = (uint32_t)(
            (int)slot[CMD_STATS_TOTAL_DWORD] + (int)v4);
        if ((int)slot[CMD_STATS_HIGH_DWORD] <
            (int)slot[CMD_STATS_TOTAL_DWORD]) {
            slot[CMD_STATS_HIGH_DWORD] = slot[CMD_STATS_TOTAL_DWORD];
        }
    }
    const uint32_t idx = slot[CMD_STATS_COUNT_DWORD];
    if (idx < CMD_STATS_LOG_DWORDS) {
        slot[CMD_STATS_LOG_DWORD + idx] = (uint32_t)(uintptr_t)sample;
    }
    slot[CMD_STATS_COUNT_DWORD] = idx + 1u;
}

uint32_t command_sample_stats_peek(int kind, int field)
{
    if (kind < 0 || (unsigned)kind >= CMD_STATS_KINDS ||
        field < 0 || field > 24) {
        return 0u;
    }
    if (field < (int)CMD_STATS_LOG_DWORDS) {
        return s_cmd_stats[kind][CMD_STATS_LOG_DWORD + field];
    }
    switch (field) {
    case 20: return s_cmd_stats[kind][CMD_STATS_COUNT_DWORD];
    case 21: return s_cmd_stats[kind][CMD_STATS_HEAD_DWORD];
    case 22: return s_cmd_stats[kind][CMD_STATS_WINDOW_DWORD];
    case 23: return s_cmd_stats[kind][CMD_STATS_TOTAL_DWORD];
    case 24: return s_cmd_stats[kind][CMD_STATS_HIGH_DWORD];
    default: return 0u;
    }
}

namespace unported_ledger_stubs {

static int object_task_target_record_matches(
    const CmdRec* record, const uint32_t key[3])
{
    if (record->type != 5 && record->type != 7 && record->type != 9 &&
        record->type != 0xf) return 0;
    if (record->type != 5 && record->payload != NULL &&
        !entity_shared_eligibility_check((const EntitySlot*)record->payload,
                                         0)) return 0;
    if (record->cell_act != 0 &&
        ((uint32_t)record->cell_act != key[0] ||
         (record->cell_target != -1 &&
          (uint32_t)record->cell_target != key[1]) ||
         (uint32_t)record->cell_count != key[2])) return 0;
    return 1;
}

// FUNCTION: LEGORR 0x00404F10
int object_task_target_filter_find(
    uint32_t type, uint32_t selector, uint32_t companion)
{
    const uint32_t key[3] = { type, selector, companion };
    int block_index;
    for (block_index = 0; block_index < s_pool_level; ++block_index) {
        CmdRec* block = s_blocks[block_index];
        uint8_t* live = s_block_live[block_index];
        const uint32_t capacity = 1u << (block_index & 0x1f);
        uint32_t slot;
        if (block == NULL || live == NULL) continue;
        for (slot = 0u; slot < capacity; ++slot) {
            if (live[slot] != 0u &&
                object_task_target_record_matches(&block[slot], key))
                return 1;
        }
    }
    return 0;
}

}

namespace unported_ledger_stubs {

static ObjectTaskEligibilityOps* s_object_task_eligibility_ops;
void object_task_eligibility_ops_bind(ObjectTaskEligibilityOps* ops)
{
    s_object_task_eligibility_ops = ops;
}

// FUNCTION: LEGORR 0x00404fc0
int ledger_stub_00404fc0_PlaceCommandEligibilityCheck(
    const ObjectTaskEligibilityRecord* command, EntitySlot* entity,
    float screen_xy[2], float world_xy[2], uint32_t* out_flag,
    int require_command_gate, int relaxed_entity_gate)
{
    ObjectTaskEligibilityOps* ops = s_object_task_eligibility_ops;
    ObjectTaskEligibilityEntityView entity_view;
    ObjectTaskEligibilityEntityView target_view;
    EntitySlot* target;
    int kind, value;
    float x = 0.0f, y = 0.0f;
    float simulation_time;
    int have_xy = 0;
    int transform_screen = 0;
    int world_from_entity = 0;
    if (out_flag != NULL) *out_flag = 0u;
    if (command == NULL || entity == NULL) return 0;

    /* FUN_00404fc0's kind-0x17 branch is fully described by entity and
     * level fields: type 2, reject a matching category flag/value pair, and
     * require the entity's +0x3d4 progress to reach the level's +0x260
     * threshold. Keep this path live even when no renderer ops are bound. */
    if (command->kind == 0x17) {
        GameContext* context = pending_scan_context_get();
        EntitySlot* target = command->target;
        const uint32_t flags = entity->flags_3f0;
        if (command->task_scalar_18 > 0.0f ||
            ((entity_stats_word_get(entity) & 0x20000u) != 0u &&
             entity->owner_36c == NULL) ||
            (entity->flags_3e0 & 0x800000u) != 0u ||
            (entity->flags_3ec & 0x400u) != 0u || entity->type != 2 ||
            (context != NULL && (context->cheat & 1u) != 0u &&
             ai_activity_probe(entity) != 0) ||
            ((flags & 1u) != 0u && command->field_f == 1u) ||
            ((flags & 2u) != 0u && command->field_f == 2u) ||
            ((flags & 4u) != 0u && command->field_f == 4u) ||
            ((flags & 8u) != 0u && command->field_f == 8u) ||
            ((flags & 0x10u) != 0u && command->field_f == 0x10u) ||
            ((flags & 0x20u) != 0u && command->field_f == 0x20u) ||
            context == NULL || context->level == NULL ||
            entity->activity_counter_3d4 <
                engine_float_field260_get(context->level)) return 0;
        if (screen_xy != NULL) {
            if (target == NULL) return 0;
            entity_transform_position_get(target, screen_xy);
        }
        if (world_xy != NULL) {
            int cell_x = 0;
            int cell_y = 0;
            if (target == NULL) return 0;
            entity_position_to_terrain_cell(target, context->level,
                                            &cell_x, &cell_y);
            world_xy[0] = (float)cell_x;
            world_xy[1] = (float)cell_y;
        }
        (void)require_command_gate;
        (void)relaxed_entity_gate;
        return 1;
    }

    /* FUN_00404fc0 kind 0x1a is fully expressible through the live entity,
     * terrain, and ray-sweep adapters. Keep its separate global-feature gate
     * and preserve the reference's zero result-flag behavior. */
    if (command->kind == 0x1a) {
        GameContext* context = pending_scan_context_get();
        EntitySlot* target = command->target;
        int cell_x, cell_y;
        if (target == NULL || command->task_scalar_18 > 0.0f ||
            ((entity_stats_word_get(entity) & 0x20000u) != 0u &&
             entity->owner_36c == NULL) ||
            (entity->flags_3e0 & 0x800000u) != 0u ||
            (entity->flags_3ec & 0x400u) != 0u ||
            !scene_object_has_task_type_4_5_7(entity) ||
            ai_activity_probe(entity) == 0 ||
            !entity_centers_within_object_radius(entity, target) ||
            see_through_walls_ray_check(entity, target) != 0 ||
            entity_shared_eligibility_check(target, 1) == 0 ||
            context == NULL || (context->cheat & 1u) == 0u) return 0;
        if (screen_xy != NULL)
            entity_transform_position_get(target, screen_xy);
        if (world_xy != NULL) {
            if (context->level == NULL) return 0;
            entity_position_to_terrain_cell(target, context->level,
                                            &cell_x, &cell_y);
            world_xy[0] = (float)cell_x;
            world_xy[1] = (float)cell_y;
        }
        return 1;
    }

    /* GameConfigInit reads Main.MinEnergyForEat into DAT_00556458. The
     * portable context owns the same open Lego.cfg, so resolve the live
     * configured scale here instead of depending on an uninstalled ops value. */
    if (command->kind == 0x11) {
        GameContext* context = pending_scan_context_get();
        EntitySlot* target = command->target;
        const float min_energy = context != NULL && context->cfg != NULL
            ? block_get_float(context->cfg, "Main MinEnergyForEat", 0.0f)
            : 0.0f;
        if (target == NULL || command->task_scalar_18 > 0.0f ||
            ((entity_stats_word_get(entity) & 0x20000u) != 0u &&
             entity->owner_36c == NULL) ||
            (entity->flags_3e0 & 0x800000u) != 0u ||
            (entity->flags_3ec & 0x400u) != 0u ||
            (context != NULL && (context->cheat & 1u) != 0u &&
             ai_can_do(entity) != 0) ||
            entity_shared_eligibility_check(target, 0) == 0 ||
            entity->type != 2 || (entity->flags_3e4 & 0x800u) != 0u ||
            entity->progress_344 > entity->field_340 * 0.01f * min_energy ||
            (entity->flags_3e0 & 0x400u) != 0u) return 0;
        (void)screen_xy;
        (void)world_xy;
        (void)require_command_gate;
        (void)relaxed_entity_gate;
        return 1;
    }

    /* FUN_00404fc0's kind-3 arm is also representable from the live entity
     * and task-record APIs. Its optional first output is the target's scene
     * position; its second output is the target's terrain cell. */
    if (command->kind == 3) {
        GameContext* context = pending_scan_context_get();
        EntitySlot* task_target = command->target;
        int target_cell[2] = { 0, 0 };
        if (task_target == NULL ||
            (task_target->type != 8 && command->task_scalar_18 > 0.0f) ||
            ((entity_stats_word_get(entity) & 0x20000u) != 0u &&
             entity->owner_36c == NULL) ||
            (entity->flags_3e0 & 0x800000u) != 0u ||
            (entity->flags_3ec & 0x400u) != 0u ||
            (context != NULL && (context->cheat & 1u) != 0u &&
             ai_can_do(entity) != 0) ||
            (entity->flags_3e8 & 0x40u) == 0u ||
            !entity_accepts_target_for_task_check(entity, task_target, 1) ||
            (entity->type == 1 && entity_stats_word_at_ec(entity) == 0u))
            return 0;
        if (task_target->type == 8 &&
            (entity->flags_3f0 & 8u) == 0u && relaxed_entity_gate)
            return 0;
        if (entity->type == 3 && task_target->type != 6) return 0;
        if (entity->type == 1 &&
            (task_target->type == 9 ||
             (task_target->type == 6 &&
              (task_target->flags_3e8 & 0x80000000u) != 0u) ||
             ((entity->flags_3e0 & 0x400u) != 0u &&
              entity->carry_link_308 != NULL &&
              ((EntitySlot*)entity->carry_link_308)->type == 0xb)))
            return 0;
        /* FUN_00404ed0 is a void helper: it performs the type/name/index
         * filter walk, but FUN_00404fc0 does not use a match result as an
         * eligibility gate. FUN_00439b80 supplies that gate. */
        (void)unported_ledger_stubs::
            ledger_stub_00404ed0_CheckObjectCandidate(
                task_target);
        if (!entity_task_target_cell_eligibility_check(entity, task_target))
            return 0;
        if (screen_xy != NULL)
            entity_transform_position_get(task_target, screen_xy);
        if (world_xy != NULL) {
            if (context == NULL || context->level == NULL) return 0;
            entity_position_to_terrain_cell(task_target, context->level,
                                            &target_cell[0], &target_cell[1]);
            world_xy[0] = (float)target_cell[0];
            world_xy[1] = (float)target_cell[1];
        }
        (void)require_command_gate;
        return 1;
    }

    /* FUN_00404fc0's kind-0x13 branch uses only live minifigure state,
     * grouped activity lookup, and the mask-8 world-position query. It does
     * not depend on the renderer/task adapter used by the other branches. */
    if (command->kind == 0x13) {
        GameContext* context = pending_scan_context_get();
        float actor_position[2];
        EntitySlot* target;
        if (command->task_scalar_18 > 0.0f || entity->type != 2 ||
            ((entity_stats_word_get(entity) & 0x20000u) != 0u &&
             entity->owner_36c == NULL) ||
            (entity->flags_3e0 & 0x800000u) != 0u ||
            (entity->flags_3ec & 0x400u) != 0u ||
            (context != NULL && (context->cheat & 1u) != 0u &&
             ai_can_do(entity) != 0) ||
            ai_activity_cap(context, entity) == 0 ||
            ai_activity_scan(entity, (int)command->field_e) != 0) return 0;
        entity_transform_position_get(entity, actor_position);
        target = entity_lookup_at_world_position(actor_position);
        if (target == NULL) return 0;
        if (screen_xy != NULL)
            entity_transform_position_get(target, screen_xy);
        /* The reference's kind-0x13 arm does not write param_4 or the
         * optional result flag after its initial clear. */
        (void)world_xy;
        (void)require_command_gate;
        (void)relaxed_entity_gate;
        return 1;
    }

    /* FUN_00404fc0 has a complete kind-0x12 path in the checked-in body:
     * its target can supply an explicit scene position, a nested type-4
     * scene pose, or the adjacent cell returned by FUN_00439e40. Keep these
     * cases on the live EntitySlot/LevelState adapters so they also work
     * when the optional eligibility-ops table has not been installed. */
    if (command->kind == 0x12) {
        GameContext* context = pending_scan_context_get();
        EntitySlot* target = command->target;
        int target_cell[2];
        float target_position[3];
        int scene_position = 0;
        if (target == NULL || command->task_scalar_18 > 0.0f ||
            ((entity_stats_word_get(entity) & 0x20000u) != 0u &&
             entity->owner_36c == NULL) ||
            (entity->flags_3e0 & 0x800000u) != 0u ||
            (entity->flags_3ec & 0x400u) != 0u ||
            (context != NULL && (context->cheat & 1u) != 0u &&
             ai_can_do(entity) != 0) || entity->type != 2 ||
            entity->cmd_head != NULL || entity->owner_36c != NULL ||
            ((command->field_f & entity->flags_3f0) == 0u &&
             relaxed_entity_gate)) return 0;

        if ((entity_stats_word_get(target) & 0x2000000u) != 0u) {
            entity_transform_position_get(target, target_position);
            scene_position = 1;
            if (world_xy != NULL) {
                if (context == NULL || context->level == NULL) return 0;
                entity_position_to_terrain_cell(target, context->level,
                                                &target_cell[0],
                                                &target_cell[1]);
                world_xy[0] = (float)target_cell[0];
                world_xy[1] = (float)target_cell[1];
            }
        }
        else if (cmd_place_target_nested_scene_pose_get(
                     target, target_position)) {
            scene_position = 1;
            if (world_xy != NULL) {
                int cell_x, cell_y;
                if (context == NULL || context->level == NULL ||
                    !terrain_world_xy_to_cell(
                        context->level->ptr_terrain, target_position[0],
                        target_position[1], &cell_x, &cell_y)) return 0;
                world_xy[0] = (float)cell_x;
                world_xy[1] = (float)cell_y;
            }
        }
        else if (entity_record_flag20_query(target, target_cell)) {
            if (screen_xy != NULL &&
                !cmd_place_position_to_world(NULL, target_cell[0],
                                             target_cell[1], &screen_xy[0],
                                             &screen_xy[1])) return 0;
            if (world_xy != NULL) {
                world_xy[0] = (float)target_cell[0];
                world_xy[1] = (float)target_cell[1];
            }
        }
        else return 0;

        if (screen_xy != NULL && scene_position) {
            screen_xy[0] = target_position[0];
            screen_xy[1] = target_position[1];
        }
        return 1;
    }

    /* The kind-0x15 branch needs only the candidate type/owner link and the
     * target's live scene pose. The model/resource and DDiScene adapters are
     * already part of the gameplay path, so don't make this branch depend on
     * the optional ObjectTaskEligibilityOps registration. */
    if (command->kind == 0x15) {
        GameContext* context = pending_scan_context_get();
        float target_x, target_y;
        int cell_x, cell_y;
        if (command->target == NULL || command->task_scalar_18 > 0.0f ||
            ((entity_stats_word_get(entity) & 0x20000u) != 0u &&
             entity->owner_36c == NULL) ||
            (entity->flags_3e0 & 0x800000u) != 0u ||
            (entity->flags_3ec & 0x400u) != 0u ||
            (context != NULL && (context->cheat & 1u) != 0u &&
             ai_can_do(entity) != 0) || entity->type != 1 ||
            entity->owner_36c == NULL ||
            !cmd_place_target_route_position_get(
                NULL, command->target, &target_x, &target_y)) return 0;
        if (screen_xy != NULL) {
            screen_xy[0] = target_x;
            screen_xy[1] = target_y;
        }
        if (world_xy != NULL) {
            if (context == NULL || context->level == NULL ||
                !terrain_world_xy_to_cell(context->level->ptr_terrain,
                                          target_x, target_y,
                                          &cell_x, &cell_y)) return 0;
            world_xy[0] = (float)cell_x;
            world_xy[1] = (float)cell_y;
        }
        return 1;
    }

    /* Kind 0x1e uses the same scene-pose lookup as kind 0x15, plus the
     * candidate's Stats capability bit 1 (FUN_00469ea0). */
    if (command->kind == 0x1e) {
        GameContext* context = pending_scan_context_get();
        float target_x, target_y;
        int cell_x, cell_y;
        if (command->target == NULL || command->task_scalar_18 > 0.0f ||
            ((entity_stats_word_get(entity) & 0x20000u) != 0u &&
             entity->owner_36c == NULL) ||
            (entity->flags_3e0 & 0x800000u) != 0u ||
            (entity->flags_3ec & 0x400u) != 0u ||
            (context != NULL && (context->cheat & 1u) != 0u &&
             ai_can_do(entity) != 0) ||
            (entity_capability_flags_get(entity) & 2u) == 0u ||
            !cmd_place_target_route_position_get(
                NULL, command->target, &target_x, &target_y)) return 0;
        if (screen_xy != NULL) {
            screen_xy[0] = target_x;
            screen_xy[1] = target_y;
        }
        if (world_xy != NULL) {
            if (context == NULL || context->level == NULL ||
                !terrain_world_xy_to_cell(context->level->ptr_terrain,
                                          target_x, target_y,
                                          &cell_x, &cell_y)) return 0;
            world_xy[0] = (float)cell_x;
            world_xy[1] = (float)cell_y;
        }
        return 1;
    }

    /* Coordinate, kind-10, and kind-16 branches use only command words and
     * live entity/terrain state. Keep them executable without the optional
     * renderer view adapter. This follows the direct branches in FUN_00404fc0
     * and its FUN_00432770 terrain predicate. */
    if (command->kind == 0 || command->kind == 8 ||
        command->kind == 0x0b || command->kind == 0x0c ||
        command->kind == 10 || command->kind == 0x16) {
        GameContext* context = pending_scan_context_get();
        const int kind = command->kind;
        int cell_x = command->x;
        int cell_y = command->y;
        int eligible = 1;
        if ((kind != 3 || command->target == NULL ||
             command->target->type != 8) &&
            command->task_scalar_18 > 0.0f) return 0;
        if ((entity_stats_word_get(entity) & 0x20000u) != 0u &&
            entity->owner_36c == NULL) return 0;
        if ((entity->flags_3e0 & 0x800000u) != 0u ||
            (entity->flags_3ec & 0x400u) != 0u) return 0;
        if (context != NULL && (context->cheat & 1u) != 0u &&
            kind != 0x1a && ai_can_do(entity) != 0) return 0;

        if (kind == 8) {
            eligible = pending_command_cell_walkable(
                context, entity, cell_x, cell_y,
                (command->field_17 & 8u) != 0u);
            if ((entity->flags_3e8 & 2u) == 0u || eligible == 0 ||
                ((command->field_17 & 8u) != 0u &&
                 (entity_stats_word_get(entity) & 1u) == 0u)) return 0;
        }
        if (require_command_gate && kind != 0 &&
            !pending_command_activity_gate(entity, kind)) return 0;
        if ((kind == 0x0b && entity->type != 2) ||
            (kind == 0x0c &&
             (entity_stats_word_get(entity) & 0x10000u) == 0u)) return 0;
        if (kind == 10 &&
            ((relaxed_entity_gate &&
              (entity->flags_3f0 & 0x10u) == 0u &&
              (command->field_17 & 0x200000u) == 0u) ||
             entity->type != 2)) return 0;
        if (kind == 0x16) {
            if (entity->type != 2) return 0;
            /* FUN_00432550 is the unconditional-success coordinate gate. */
        }
        if (screen_xy != NULL) {
            if (kind == 10) {
                if (command->target == NULL) return 0;
                entity_transform_position_get(command->target, screen_xy);
            }
            else if (context == NULL || context->level == NULL ||
                !terrain_cell_world_center_xy_get(
                    context->level->ptr_terrain, cell_x, cell_y,
                    screen_xy)) return 0;
        }
        if (world_xy != NULL) {
            if (kind == 10) {
                if (command->target == NULL || context == NULL ||
                    context->level == NULL) return 0;
                entity_position_to_terrain_cell(command->target,
                    context->level, &cell_x, &cell_y);
            }
            world_xy[0] = (float)cell_x;
            world_xy[1] = (float)cell_y;
        }
        return 1;
    }

    if (ops == NULL ||
        ops->global_feature_word == NULL ||
        ops->entity_view_get == NULL) {
        GameContext* context = pending_scan_context_get();
        const int kind = command->kind;
        EntitySlot* target = command->target;
        if ((kind != 3 || target == NULL || target->type != 8) &&
            command->task_scalar_18 > 0.0f) return 0;
        if ((entity_stats_word_get(entity) & 0x20000u) != 0u &&
            entity->owner_36c == NULL) return 0;
        if ((entity->flags_3e0 & 0x800000u) != 0u ||
            (entity->flags_3ec & 0x400u) != 0u) return 0;
        if (context != NULL && (context->cheat & 1u) != 0u &&
            kind != 0x1a && ai_can_do(entity) != 0) return 0;
        if (kind == 0x1a || kind == 0x1e || kind == 3 ||
            kind == 0x11 || kind == 0x12 || kind == 0x15 ||
            kind == 0x13 || kind == 0x17) return 0;
        if (out_flag != NULL) *out_flag = 1u;
        return 1;
    }
    target = command->target;
    if (!ops->entity_view_get(ops->context, entity, &entity_view)) return 0;
    memset(&target_view, 0, sizeof(target_view));
    if (target != NULL &&
        !ops->entity_view_get(ops->context, target, &target_view)) return 0;
    kind = command->kind;

    /* Shared command and entity gates. */
    if ((kind != 3 || target_view.type != 8) &&
        command->task_scalar_18 > 0.0f) return 0;
    if ((entity_view.activity_flags & 0x20000u) != 0u &&
        entity_view.field_db == 0) return 0;
    if ((entity_view.flags_f8 & 0x800000u) != 0u ||
        (entity_view.flags_fb & 0x400u) != 0u) return 0;
    if ((*ops->global_feature_word & 1u) != 0u && kind != 0x1a) {
        if (ops->entity_special_gate == NULL ||
            ops->entity_special_gate(ops->context, entity)) return 0;
    }

    if (kind == 0x1a) {
        if (ops->entity_target_gate == NULL ||
            !ops->entity_target_gate(ops->context, entity, target) ||
            ops->entity_radius_gate == NULL ||
            !ops->entity_radius_gate(ops->context, entity, target) ||
            ops->entity_ray_gate == NULL ||
            ops->entity_ray_gate(ops->context, entity, target) ||
            ops->entity_eligible == NULL ||
            !ops->entity_eligible(ops->context, target, 1) ||
            ((*ops->global_feature_word & 1u) == 0u)) return 0;
        if (screen_xy != NULL && (ops->target_position_get == NULL ||
            !ops->target_position_get(ops->context, target,
                                      &screen_xy[0], &screen_xy[1]))) return 0;
        if (world_xy != NULL && (ops->target_world_position_get == NULL ||
            !ops->target_world_position_get(ops->context, target,
                                            &world_xy[0], &world_xy[1]))) return 0;
        return 1;
    }
    if (kind == 0x1e) {
        GameContext* context = pending_scan_context_get();
        if (ops->target_kind_resolve == NULL ||
            ops->target_kind_resolve(ops->context, target) == 0 ||
            ops->target_type4_activity_get == NULL ||
            !ops->target_type4_activity_get(ops->context, entity, &value) ||
            value == 0 ||
            ops->target_route_position_get == NULL ||
            !ops->target_route_position_get(ops->context, target, &x, &y))
            return 0;
        if (screen_xy != NULL) {
            screen_xy[0] = x;
            screen_xy[1] = y;
        }
        if (world_xy != NULL) {
            int cell_x, cell_y;
            if (context == NULL || context->level == NULL ||
                !terrain_world_xy_to_cell(context->level->ptr_terrain,
                                          x, y, &cell_x, &cell_y)) return 0;
            world_xy[0] = (float)cell_x;
            world_xy[1] = (float)cell_y;
        }
        return 1;
    }
    else if (kind == 3) {
        if ((entity_view.flags_3e8 & 0x40u) == 0u ||
            ops->kind3_target_gate == NULL ||
            !ops->kind3_target_gate(ops->context, entity, target, 1)) return 0;
        if (entity_view.type == 1 &&
            (ops->kind1_activity_gate == NULL ||
             !ops->kind1_activity_gate(ops->context, entity))) return 0;
        if (target_view.type == 8 &&
            (entity_view.flags_fc & 8u) == 0u && relaxed_entity_gate) return 0;
        if (entity_view.type == 3 && target_view.type != 6) return 0;
        if (entity_view.type == 1) {
            if (target_view.type == 9 ||
                (target_view.type == 6 &&
                 (target_view.flags_3e8 & 0x80000000u) != 0u) ||
                ((entity_view.flags_f8 & 0x400u) != 0u &&
                 target_view.nested_type_2fc == 0xb)) return 0;
        }
        /* FUN_00404ed0 returns void in the retail ABI; only the following
         * FUN_00439b80 relation check determines eligibility. */
        (void)ledger_stub_00404ed0_CheckObjectCandidate(
            target);
        if (ops->entity_target_relation == NULL ||
            !ops->entity_target_relation(ops->context, entity, 0)) return 0;
        if (screen_xy != NULL &&
            (ops->target_position_get == NULL ||
             !ops->target_position_get(ops->context, target, &x, &y)))
            return 0;
        have_xy = screen_xy != NULL || world_xy != NULL;
        world_from_entity = 1;
    }
    else if (kind == 8 || kind == 0 || kind == 0xb || kind == 0xc) {
        int walkable = 1;
        if (kind == 8) {
            walkable = ops->terrain_walkable != NULL &&
                ops->terrain_walkable(ops->context, entity, command->x,
                                      command->y,
                                      (command->field_17 & 8u) != 0u);
        }
        if (require_command_gate && kind != 0 && kind != 9 &&
            (ops->command_kind_gate == NULL ||
             !ops->command_kind_gate(ops->context, entity, kind))) return 0;
        if ((kind == 0xb && entity_view.type != 2) ||
            (kind == 0xc && (entity_view.activity_flags & 0x10000u) == 0u))
            return 0;
        if (kind == 8 && ((entity_view.flags_3e8 & 2u) == 0u ||
            !walkable || ((command->field_17 & 8u) != 0u &&
            (entity_view.activity_flags & 1u) == 0u))) return 0;
        x = (float)command->x;
        y = (float)command->y;
        have_xy = 1;
        transform_screen = 1;
    }
    else if (kind == 10) {
        if ((require_command_gate &&
             (ops->command_kind_gate == NULL ||
              !ops->command_kind_gate(ops->context, entity, 10))) ||
            (relaxed_entity_gate && (entity_view.flags_fc & 0x10u) == 0u &&
             (command->field_17 & 0x200000u) == 0u) ||
            entity_view.type != 2) return 0;
        if (screen_xy != NULL) {
            if (ops->target_position_get == NULL ||
                !ops->target_position_get(ops->context, target, &x, &y))
                return 0;
        }
        /* The retail path writes the scene position only when param_3 is
         * present, but it still falls through to FUN_004424f0 for param_4
         * when only the cell-pair output is requested. Keep the shared tail
         * active for either optional output. */
        if (screen_xy != NULL || world_xy != NULL) {
            have_xy = 1;
            world_from_entity = 1;
        }
    }
    else if (kind == 0x11) {
        if (ops->entity_eligible == NULL ||
            !ops->entity_eligible(ops->context, target, 0) ||
            entity_view.type != 2 || (entity_view.flags_f9 & 0x800u) != 0u ||
            entity_view.field_d1 > entity_view.field_d0 * 0.01f *
                                   ops->activity_scale ||
            (entity_view.flags_f8 & 0x400u) != 0u) return 0;
        return 1;
    }
    else if (kind == 0x12) {
        if (entity_view.type != 2 || entity_view.field_bc != 0 ||
            entity_view.field_db != 0 ||
            ((command->field_f & entity_view.flags_fc) == 0u &&
             relaxed_entity_gate) || ops->target_activity_flags == NULL)
            return 0;
        value = ops->target_activity_flags != NULL
            ? ops->target_activity_flags(ops->context, target) : 0;
        if ((value & 0x2000000) == 0) {
            float scene_position[3];
            if (cmd_place_target_nested_scene_pose_get(target,
                                                       scene_position)) {
                if (screen_xy != NULL) {
                    screen_xy[0] = scene_position[0];
                    screen_xy[1] = scene_position[1];
                }
                /* Retail writes the optional screen position and optional
                 * cell pair independently. A caller asking only for the
                 * cell output still receives the nested type-4 scene pose. */
                if (world_xy != NULL) {
                    GameContext* context = pending_scan_context_get();
                    int cell_x, cell_y;
                    if (context == NULL || context->level == NULL ||
                        !terrain_world_xy_to_cell(
                            context->level->ptr_terrain,
                            scene_position[0], scene_position[1],
                            &cell_x, &cell_y)) return 0;
                    world_xy[0] = (float)cell_x;
                    world_xy[1] = (float)cell_y;
                }
                return 1;
            }
        }
        if ((value & 0x2000000) == 0) {
            if (ops->target_activity_position_get == NULL ||
                !ops->target_activity_position_get(ops->context, target,
                                                   &x, &y)) return 0;
            have_xy = 1;
            transform_screen = 1;
        }
        else {
            if (screen_xy != NULL &&
                (ops->target_position_get == NULL ||
                 !ops->target_position_get(ops->context, target, &x, &y)))
                return 0;
            have_xy = screen_xy != NULL || world_xy != NULL;
            world_from_entity = 1;
        }
    }
    else if (kind == 0x15) {
        GameContext* context = pending_scan_context_get();
        if (entity_view.type != 1 || entity_view.field_db == 0 ||
            ops->target_kind_resolve == NULL ||
            ops->target_route_position_get == NULL ||
            !ops->target_route_position_get(ops->context, target, &x, &y))
            return 0;
        value = ops->target_kind_resolve(ops->context, target);
        (void)value;
        if (screen_xy != NULL) {
            screen_xy[0] = x;
            screen_xy[1] = y;
        }
        if (world_xy != NULL) {
            int cell_x, cell_y;
            if (context == NULL || context->level == NULL ||
                !terrain_world_xy_to_cell(context->level->ptr_terrain,
                                          x, y, &cell_x, &cell_y)) return 0;
            world_xy[0] = (float)cell_x;
            world_xy[1] = (float)cell_y;
        }
        return 1;
    }
    else if (kind == 0x16) {
        if (entity_view.type != 2 || ops->task_coordinate_check == NULL ||
            !ops->task_coordinate_check(ops->context, command->x, command->y))
            return 0;
        x = (float)command->x;
        y = (float)command->y;
        have_xy = 1;
        transform_screen = 1;
    }
    else if (kind == 0x13) {
        EntitySlot* resolved_target;
        if (entity_view.type != 2 || ops->target_activity_gate == NULL ||
            !ops->target_activity_gate(ops->context, entity) ||
            ops->target_visibility_check == NULL ||
            !ops->target_visibility_check(ops->context, entity,
                                          (int)command->field_e) ||
            ops->kind13_target_resolve == NULL ||
            ops->target_position_get == NULL) return 0;
        resolved_target = ops->kind13_target_resolve(
            ops->context, entity, command->field_e);
        if (resolved_target == NULL) return 0;
        if (screen_xy == NULL) return 1;
        if (!ops->target_position_get(ops->context, resolved_target, &x, &y))
            return 0;
        have_xy = 1;
        world_from_entity = 1;
    }
    else if (kind == 0x17) {
        if (entity_view.type != 2 ||
            ((entity_view.flags_fc & 1u) != 0u && command->field_f == 1u) ||
            ((entity_view.flags_fc & 2u) != 0u && command->field_f == 2u) ||
            ((entity_view.flags_fc & 4u) != 0u && command->field_f == 4u) ||
            ((entity_view.flags_fc & 8u) != 0u && command->field_f == 8u) ||
            ((entity_view.flags_fc & 0x10u) != 0u && command->field_f == 0x10u) ||
            ((entity_view.flags_fc & 0x20u) != 0u && command->field_f == 0x20u) ||
            ops->simulation_time_get == NULL) return 0;
        simulation_time = ops->simulation_time_get(ops->context);
        if (entity_view.field_f5 < simulation_time) return 0;
        if (screen_xy != NULL) {
            if (ops->target_position_get == NULL ||
                !ops->target_position_get(ops->context, target, &x, &y)) return 0;
        }
        have_xy = screen_xy != NULL || world_xy != NULL;
        world_from_entity = 1;
    }
    else if (kind != 0x1a && kind != 0x1e) {
        if (out_flag == NULL) return 1;
        *out_flag = 1u;
        return 1;
    }

    if (have_xy) {
        if (screen_xy != NULL && transform_screen) {
            if (ops->position_to_world == NULL ||
                !ops->position_to_world(ops->context, x, y,
                                        &screen_xy[0], &screen_xy[1])) return 0;
        }
        else if (screen_xy != NULL) {
            screen_xy[0] = x;
            screen_xy[1] = y;
        }
        if (world_xy != NULL && kind != 0x13) {
            if (world_from_entity) {
                if (ops->target_world_position_get == NULL) return 0;
                if (!ops->target_world_position_get(ops->context, target,
                                                    &world_xy[0], &world_xy[1]))
                    return 0;
            }
            else { world_xy[0] = x; world_xy[1] = y; }
        }
    }
    return 1;
}


// FUNCTION: LEGORR 0x00403320
void ledger_stub_00403320_ObjectCommandTaskCreate(EntitySlot* entity,
                                                  int parameter)
{
    void* target = NULL;
    if (entity == NULL) return;
    if (entity->type == 2 && (entity->flags_3e0 & 0x400u) == 0u) {
        float position[2];
        entity_transform_position_get(entity, position);
        target = entity_lookup_at_world_position(position);
    }
    else if (entity->type == 1) {
        target = ledger_stub_00438bf0_SpatialObjectQueryCenterMask80(entity);
    }
    if (target != NULL) {
        cmd_entity_task_record_create_and_append(entity, target, parameter);
    }
}

void ledger_stub_00403750_CommandKindRecordIssueAndLink(
    uint32_t a, uint32_t b, uint32_t c, const int cell_xy[2], uint32_t zone,
    const float* random_pair, int set_flag)
{
    cmd_command_kind_record_issue(a, b, c, cell_xy, zone, random_pair,
                                  set_flag);
}

void ledger_stub_00403810_TerrainCellRecordCountUpdate(
    const int cell_xy[2], int a, int b, int c)
{
    terrain_cell_record_count_update(cell_xy, a, b, c);
}

// ObjectPathSubmitToGridTarget forwards the target ABI to the command path
// implementation, which owns route allocation and entity route setup.
int ledger_stub_00404e00_ObjectPathSubmitToGridTarget(
    EntitySlot* entity, LevelState* level, const int target_xy[2],
    const float* final_marker_position)
{
    return object_path_submit_to_grid_target(entity, level, target_xy,
                                             final_marker_position);
}

// FUNCTION: LEGORR 0x00404ed0
void ledger_stub_00404ed0_CheckObjectCandidate(
    const EntitySlot* candidate)
{
    if (candidate == NULL) return;
    if (candidate->type == 6 &&
        (candidate->flags_3e8 & 0x80000000u) != 0u) {
        (void)type11_cell_coordinate_nearest_resolve(candidate, NULL);
        return;
    }
    (void)object_task_target_filter_find(
        (uint32_t)candidate->type, (uint32_t)candidate->etype,
        (uint32_t)candidate->indexed_record_slot_330);
}

int ledger_stub_00404f10_FindMatchingObjectRecord(
    uint32_t type, uint32_t selector, uint32_t companion)
{
    return object_task_target_filter_find(type, selector, companion);
}

// PendingCommandTimerFireScan
void ledger_stub_00405950_PendingCommandTimerFireScan()
{
    pending_command_timer_fire_scan();
}

// PendingCommandEntityAssignmentTick
void ledger_stub_00405c10_PendingCommandEntityAssignmentTick()
{
    pending_command_entity_assignment_tick();
}


} // namespace unported_ledger_stubs
