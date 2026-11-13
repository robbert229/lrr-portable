// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "cmd.hpp"
#include "ai.hpp"
#include "effects.hpp"
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
int ledger_stub_004446e40_EntityNearestCellRouteCommit(
    EntitySlot* entity, unsigned int goal_x, unsigned int goal_y, int mode);
}

CommandKindRecordHooks s_command_kind_record_hooks;
#include "gods98/string.hpp"
#include "cmd_shared.hpp"
extern "C" void* s_upgrade_records; /* lego_unit.c (C) reads it */
#include "cmd_for_lego.hpp"
#include "cmd_for_weapons.hpp"
#include "cmd_from_weapons.hpp"
#include "cmd_for_interface.hpp"
#include "cmd_for_legoobject.hpp"
#include "cmd_for_message_unit.hpp"
#include "cmd_from_message_unit.hpp"
#include "cmd_for_aitask.hpp"
extern "C" void cmd_registry_init_target(void);
#include "cmd_from_aitask.hpp"
/* AITask.c's priority tables live in the unit's state (aitask.c); the
 * frozen headers' s_behaviour/s_priority_state declarations are not
 * defined anywhere since finding 570. */
extern "C" int* aitask_port_priority_values(void);
extern "C" int* aitask_port_priority_states(void);
#define s_behaviour (aitask_port_priority_values())
#define s_priority_state (aitask_port_priority_states())

/* ------------------------------------------------------------------ *
 *  Command record (finding 10 §"Command record" — corpus stride 0x68).
 *  Corpus 0x403190 has "mov [esi+0x38], eax", "mov [esi+0x58], 0", and
 *  state read/write instructions at +0x5c; keep those named fields at their
 *  corpus offsets. The port-only global-list link uses the intervening +0x50
 *  word; private staging values follow the corpus +0x60 chain link.
 *  CMD_STRIDE is sizeof(CmdRec), so the
 *  pool allocation includes the private tail rather than overlapping the
 *  corpus-visible fields (the x86 port record is 0x88 bytes including the
 *  private GetTool movement identity).
 * ------------------------------------------------------------------ */

CmdEntityChainExternalOps s_cmd_entity_chain_external_ops;

void cmd_entity_chain_external_ops_set(const CmdEntityChainExternalOps* ops)
{
    if (ops == NULL) memset(&s_cmd_entity_chain_external_ops, 0,
                            sizeof(s_cmd_entity_chain_external_ops));
    else s_cmd_entity_chain_external_ops = *ops;
}

CmdRec* as_rec(void* p) { return (CmdRec*)p; }
const CmdRec* cas_rec(const void* p) { return (const CmdRec*)p; }

int cmd_rec_type(const void* rec) { return cas_rec(rec)->type; }
const void* cmd_rec_payload(const void* rec) { return cas_rec(rec)->payload; }
int cmd_rec_kind(const void* rec) { return cas_rec(rec)->kind; }
#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
int cmd_rec_behaviour(const void* rec) { return cas_rec(rec)->behaviour; }
#endif /* !LEGORRR_BACKEND_WINAPI */
int cmd_rec_activity(const void* rec) { return cas_rec(rec)->grant_act; }
uint32_t cmd_rec_value_38(const void* rec)
{
    return (uint32_t)(uintptr_t)cas_rec(rec)->arg38;
}
#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void cmd_rec_set_activity(void* rec, int act) { as_rec(rec)->grant_act = act; }
#endif /* !LEGORRR_BACKEND_WINAPI */
uint32_t cmd_rec_state(const void* rec) { return cas_rec(rec)->state; }
void* cmd_rec_next(const void* rec) { return cas_rec(rec)->next; }

float cmd_rec_move_x(const void* rec)
{
    int cell;
    if (cas_rec(rec)->type != 0) return cas_rec(rec)->x;
    memcpy(&cell, &cas_rec(rec)->x, sizeof(cell));
    return (float)cell;
}
float cmd_rec_move_y(const void* rec)
{
    int cell;
    if (cas_rec(rec)->type != 0) return cas_rec(rec)->y;
    memcpy(&cell, &cas_rec(rec)->y, sizeof(cell));
    return (float)cell;
}
void cmd_rec_move_xy_set(void* rec, float x, float y)
{
    if (rec != NULL) {
        if (as_rec(rec)->type == 0) {
            const int cell[2] = { (int)x, (int)y };
            memcpy(&as_rec(rec)->x, cell, sizeof(cell));
            as_rec(rec)->state &= ~CMD_STATE_MOVE_ROUTED;
        }
        else {
            as_rec(rec)->x = x;
            as_rec(rec)->y = y;
        }
    }
}
float cmd_rec_now(const void* rec) { return cas_rec(rec)->now; }
void cmd_rec_set_now(void* rec, float v) { as_rec(rec)->now = v; }
void cmd_rec_set_next(void* rec, void* next) { as_rec(rec)->next = next; }

/* ------------------------------------------------------------------ *
 *  Pool (CmdPoolGrow/CmdAlloc/CmdFree).
 * ------------------------------------------------------------------ */

/* CmdAlloc writes a self-pointer to retail +0x64; CmdFree replaces it with a
 * freelist link. The host record reuses that dword for other staging, so keep
 * live membership in an index-parallel sidecar instead of inferring it from
 * the freelist or treating the record as an EntitySlot. */
uint8_t* s_block_live[CMD_POOL_MAX_BLOCKS] = { 0 };

CmdRec* s_global_tail = NULL; /* DAT_004b3358: independent pending-list head */
GameContext* s_ctx = NULL;

namespace unported_ledger_stubs {
int ledger_stub_0044b4c0_TypeTwoEntityTargetResolveAndApply(
    EntitySlot* entity, uint32_t code);
}

static unported_ledger_stubs::ObjectTaskEligibilityOps s_place_eligibility_ops;
static EntityRayHitSweepOps s_place_ray_hit_ops;
int pending_command_cell_walkable(GameContext* context,
                                         const EntitySlot* entity,
                                         int x, int y, int mode);

static int cmd_place_ray_object_position_get(void*, void* object,
                                            float position[3])
{
    return entity_world_center_position_read((const EntitySlot*)object,
                                             position);
}

static int cmd_place_ray_see_through_walls_enabled(void*)
{
    return s_ctx != NULL && s_ctx->cfg != NULL &&
           block_get_bool_path(s_ctx->cfg,
               string_path_join(util_root_name(s_ctx->cfg), "SeeThroughWalls",
                                (const char*)NULL)) == 1;
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

int cmd_place_target_route_position_get(void*,
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

int cmd_place_position_to_world(void*, int x, int y,
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

int pending_command_activity_gate(const EntitySlot* entity, int kind);

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

static int cmd_place_kind3_entity_target_relation(
    void*, const EntitySlot* entity, const EntitySlot* target)
{
    /* FUN_00404fc0's kind-3 branch calls FUN_00439b80(entity, target)
     * after the type/name lookup. This is the mask-2 spatial eligibility
     * check, distinct from the 4/5/7 activity gate used by kind 0x1a. */
    return entity_task_target_cell_eligibility_check(entity, target);
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
    /* FUN_00472130 compares the object centers using the first entity's
     * collision radius. The preceding FUN_004493e0 call is the separate
     * activity-4/5/7 gate; do not add FUN_0043c440's activity probe here. */
    return entity_centers_within_object_radius(entity, target);
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
    return s_ctx != NULL && ai_activity_cap_for_context(s_ctx, entity) != 0;
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
        ? block_get_float(s_ctx->cfg, "Main::MinEnergyForEat", 0.0f)
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
    s_place_eligibility_ops.entity_target_relation =
        cmd_place_kind3_entity_target_relation;
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
    const int result = unported_ledger_stubs::ledger_stub_00404fc0_PlaceCommandEligibilityCheck(
        &command, entity, position_xy, cell_xy, out_flag,
        require_command_gate, relaxed_entity_gate);
    return result;
}

unsigned s_pending_assignment_count;

GameContext* pending_scan_context_get(void)
{
    return s_ctx != NULL ? s_ctx : entity_pool_callback_context_get();
}

/* FUN_0044b4c0: non-type-2 entities pass the command-activity gate. For a
 * type-2 entity, only command kinds present in the grouped code table require
 * the corresponding permitted activity. */
int pending_command_activity_gate(const EntitySlot* entity, int kind)
{
    int activity_group;
    if (entity == NULL || entity->type != 2) return 1;
    if (!entity_grouped_code_table_find_value_index((uint32_t)kind,
                                                     &activity_group))
        return 1;
    const int result = ai_activity_scan(entity, activity_group);
    return result;
}

/* FUN_00432770, called by PlaceCommandEligibilityCheck for kind 8. The
 * simple mode checks the open-cell flags and excluded type bytes. The
 * alternate mode also checks the actor's stats/capability gates and the
 * four neighboring state/type bytes. LevelState owns the same 0x48-byte
 * voxel records, so this branch does not need renderer callbacks. */
int pending_command_cell_walkable(GameContext* context,
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
        entity->owner_36c == NULL)
        return 0;
    if (mode != 0 && (entity_stats_word_get(entity) & 1u) == 0u)
        return 0;
    /* PlaceCommandEligibilityCheck passes param_5 == 1 to FUN_00432770. */
    if ((entity->flags_3e0 & 0x400u) != 0u)
        return 0;
    if ((entity->flags_3e8 & 2u) == 0u)
        return 0;
    if ((unsigned)x >= (unsigned)level_width(level) - 2u ||
        (unsigned)y >= (unsigned)level_height(level) - 2u)
        return 0;
    cell = level_voxel(level, x, y);
    if (cell == NULL)
        return 0;
    type = (int)(signed char)cell[2];
    {
        const float drill_time = entity_stats_float_at24_indexed(entity, type);
        const int rejected = drill_time == 0.0f;
        if (rejected) return 0;
    }

    if (mode == 0) {
        memcpy(&flags, cell + 8, sizeof(flags));
        const int result = (flags & 0x10u) != 0u && (flags & 0x40u) == 0u &&
               type != 1 && type != 7 && type != 11;
        return result;
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
CmdPurgeCellPredicate s_cmd_purge_cell_usable;
CmdPurgeCellPredicate s_cmd_purge_neighbor_eligible;
CmdPurgeCompare s_cmd_purge_compare;
void* s_cmd_purge_context;

int cmd_gettool_store_live(const CmdGetToolRoute* route)
{
    return route->actor != NULL && route->store != NULL &&
        route->store->pool_link_408 == (uintptr_t)route->store &&
        route->store->type == 4 &&
        route->store->owner_ctx == route->actor->owner_ctx;
}

/* No reset here: the caller may already be releasing the waypoint buffer. */
static void cmd_gettool_owner_forget(CmdRec* record)
{
    EntitySlot* actor = record->gettool.actor;
    EntitySlot* store = record->gettool.store;
    if (actor == NULL) return;
    memset(&record->gettool, 0, sizeof(record->gettool));
    record->state &= ~CMD_STATE_ISSUED;
    if (actor->cmd_head == record) {
        actor->flags_3e4 &= ~0x40u;
        if (actor->link_2fc == store) actor->link_2fc = NULL;
    }
}

void cmd_gettool_route_reset(EntitySlot* entity)
{
    CmdRec* head;
    if (entity == NULL) return;
    head = as_rec(entity->cmd_head);
    if (head != NULL && head->type == 19 && head->gettool.actor == entity)
        cmd_gettool_owner_forget(head);
}

void cmd_gettool_record_release(CmdRec* record)
{
    CmdGetToolRoute route;
    EntitySlot* actor;
    int owns_path;
    int saved_enabled;
    if (record->type != 19) return;
    route = record->gettool;
    actor = route.actor;
    owns_path = actor != NULL && actor->cmd_head == record &&
        route.phase == 1 && route.count == actor->wp_count &&
        route.waypoints == ENTITY_WAYPOINTS(actor);
    cmd_gettool_owner_forget(record); /* clear before reset can reenter */
    if (!owns_path) return;
    saved_enabled = s_global_enabled;
    s_global_enabled |= 4; /* preserve the chain during this path release */
    entity_action_sequence_reset(actor, 0);
    s_global_enabled = saved_enabled;
}

void cmd_gettool_entity_removing(EntitySlot* entity)
{
    int block_index;
    if (entity == NULL) return;
    for (block_index = 0; block_index < s_pool_level; ++block_index) {
        CmdRec* block = s_blocks[block_index];
        uint32_t index;
        const uint32_t count = 1u << (block_index & 0x1fu);
        if (block == NULL || s_block_live[block_index] == NULL) continue;
        for (index = 0u; index < count; ++index) {
            CmdRec* record = &block[index];
            if (s_block_live[block_index][index] != 0u &&
                record->type == 19 &&
                (record->gettool.actor == entity ||
                 record->gettool.store == entity))
                cmd_gettool_record_release(record);
        }
    }
}

void cmd_gettool_all_owners_release(void)
{
    int block_index;
    /* Revoke GetTool movement identities while all command blocks exist. */
    for (block_index = 0; block_index < s_pool_level; ++block_index) {
        unsigned int index;
        const unsigned int count = 1u << (block_index & 0x1fu);
        if (s_blocks[block_index] == NULL ||
            s_block_live[block_index] == NULL) continue;
        for (index = 0u; index < count; ++index) {
            if (s_block_live[block_index][index] != 0u)
                cmd_gettool_record_release(&s_blocks[block_index][index]);
        }
    }
}

int cmd_gettool_route_moving(EntitySlot* entity)
{
    CmdRec* head;
    float endpoint[2];
    if (entity == NULL) return 0;
    head = as_rec(entity->cmd_head);
    if (head == NULL || head->type != 19 || head->gettool.phase != 1)
        return 0;
    if (head->gettool.actor == entity &&
        (head->state & (CMD_STATE_ISSUED | CMD_STATE_DONE)) ==
            CMD_STATE_ISSUED && cmd_gettool_store_live(&head->gettool) &&
        entity->link_2fc == head->gettool.store &&
        (entity->flags_3e0 & 1u) != 0u &&
        entity->wp_count == head->gettool.count &&
        ENTITY_WAYPOINTS(entity) == head->gettool.waypoints &&
        entity->wp_index >= 0 && entity->wp_index <= entity->wp_count &&
        ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind == 0x0du &&
        entity_route_waypoint_world_get(entity,
            (unsigned int)(entity->wp_count - 1), endpoint) &&
        endpoint[0] == head->gettool.endpoint[0] &&
        endpoint[1] == head->gettool.endpoint[1]) return 1;
    cmd_gettool_record_release(head);
    return 0;
}

int cmd_gettool_route_arrived(EntitySlot* entity)
{
    CmdRec* head;
    if (entity == NULL) return 0;
    head = as_rec(entity->cmd_head);
    if (head == NULL || head->type != 19 || head->gettool.phase != 2)
        return 0;
    if (head->gettool.actor == entity &&
        (head->state & CMD_STATE_DONE) == 0u &&
        cmd_gettool_store_live(&head->gettool)) return 1;
    cmd_gettool_record_release(head);
    return 0;
}

int cmd_gettool_route_finish(EntitySlot* entity)
{
    CmdRec* head;
    CmdGetToolRoute route;
    float position[2];
    if (!cmd_gettool_route_moving(entity) ||
        entity->wp_index != entity->wp_count) return 0;
    head = as_rec(entity->cmd_head);
    route = head->gettool;
    entity_transform_position_get(entity, position);
    if (!(fabs((double)(position[0] - route.endpoint[0])) <= 0.001 &&
          fabs((double)(position[1] - route.endpoint[1])) <= 0.001)) return 0;
    /* Same ordinary mover, actual LAST weighted point, no second time budget.
     * Mode1 frees the path without requeuing19. Restore only terminal identity
     * after reset invalidates the old moving identity. The callback owns grant. */
    entity_action_sequence_reset(entity, 1);
    route.phase = 2;
    route.waypoints = NULL;
    route.count = 0;
    head->gettool = route;
    head->state |= CMD_STATE_ISSUED;
    entity->flags_3e4 |= 0x40u;
    return 1;
}

#undef CMD_POOL_LINK_GET
#undef CMD_POOL_LINK_SET

/* ------------------------------------------------------------------ *
 *  Registry (CmdRegistryInit / CmdKindByName): 31 AI task types and
 *  27 AI priorities, in the *address order* CmdRegistryInit writes them.
 * ------------------------------------------------------------------ */

static void cmd_port_state_reset(void)
{
    int i;
    cmd_gettool_all_owners_release();

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
}

#if !defined(_MSC_VER) || !defined(_M_IX86)
#undef CMD_TASK_NAME_SET
#undef CMD_PRIORITY_NAME_SET
#endif

void cmd_init(void)
{
    cmd_port_state_reset();
    cmd_registry_init_target();
}

int cmd_kind_behaviour(int kind)
{
    if (kind < 0 || kind >= 27) {
        return 0x31;
    }
    return s_behaviour[kind];
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
const char* cmd_task_name(int slot)
{
    return (slot >= 0 && slot < 31) ? s_task_names[slot] : NULL;
}
const char* cmd_priority_name(int slot)
{
    return (slot >= 0 && slot < 27) ? s_priority_names[slot] : NULL;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* ------------------------------------------------------------------ *
 *  Link / chain primitives.
 * ------------------------------------------------------------------ */

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

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void* cmd_global_pending_head(void)
{
    return s_global_tail;
}

void* cmd_global_next(const void* rec)
{
    return rec != NULL ? cas_rec(rec)->next : NULL;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

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
                    previous->next = current->next;
                else
                    *roots[root_index] = as_rec(current->next);
                current->next = NULL;
                return;
            }
            previous = current;
            current = as_rec(current->next);
        }
    }
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
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

/* Append a record to the entity's chain. This helper is used by other
 * command constructors; CmdCreateMove has its own retail-shaped path below. */

/* ------------------------------------------------------------------ *
 *  Creators (CmdCreateMove/Target/Gated/Task + MinionCompanionCmd).
 * ------------------------------------------------------------------ */

void cmd_create_move(EntitySlot* e, float x, float y, int flag)
{
    const int cell_xy[2] = { (int)x, (int)y };
    float position_xy[2];
    memcpy(position_xy, cell_xy, sizeof(cell_xy));
    cmd_create_move(e, position_xy, flag);
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* Task kind derived from the entity type (FUN_00402720's bVar3 switch). */

/* An admitted list must still terminate at this record's integer cell.
 * Retarget/reset owners invalidate admission; a malformed list never counts
 * as arrival merely because its cursor reached the end. */
static int cmd_move_route_endpoint_matches(EntitySlot* entity, CmdRec* head)
{
    int target[2];
    const AiWaypoint* last;
    if (entity->wp_count <= 0 || entity->wp_count > 4096 ||
        (entity->wp_dynamic == NULL ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    (void)cmd_rec_cell_xy_get(head, &target[0], &target[1]);
    last = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    return last->x == (float)target[0] && last->y == (float)target[1];
}

int cmd_move_route_prepare(EntitySlot* entity, LevelState* level)
{
    CmdRec* head;
    int target[2];
    int saved_enabled;
    int installed;
    if (entity == NULL || level == NULL || level->ptr_terrain == NULL)
        return 0;
    head = as_rec(entity->cmd_head);
    if (head == NULL || head->type != 0 ||
        (head->state & CMD_STATE_DONE) != 0u) return 0;
    if ((head->state & CMD_STATE_MOVE_ROUTED) != 0u && entity->wp_count > 0)
        return cmd_move_route_endpoint_matches(entity, head);
    /* A reset by an entity-state owner invalidates admission. Replan from
     * the current world pose; never fall back to a straight line through walls. */
    head->state &= ~CMD_STATE_MOVE_ROUTED;
    (void)cmd_rec_cell_xy_get(head, &target[0], &target[1]);
    saved_enabled = s_global_enabled;
    /* Route installation resets the old action sequence. Its queue-flush
     * tail must not sweep this head (or the next queued move) while admitted
     * from the earlier game-layer AI pass. The scheduler uses this same bit. */
    s_global_enabled |= 4;
    installed = object_path_submit_to_grid_target(entity, level, target, NULL);
    s_global_enabled = saved_enabled;
    if (!installed) return 0;
    head->state |= CMD_STATE_MOVE_ROUTED;
    ai_set_activity(entity, AI_ACT_ROUTE, 0);
    return 1;
}

int cmd_move_route_finish(EntitySlot* entity)
{
    CmdRec* head;
    int saved_enabled;
    if (entity == NULL) return 0;
    head = as_rec(entity->cmd_head);
    if (head == NULL || head->type != 0) return 0;
    if ((head->state & CMD_STATE_DONE) == 0u) {
        float position[2];
        float endpoint[2];
        int cell[2], target[2];
        GameContext* context = entity->owner_ctx != NULL ? entity->owner_ctx
            : entity_pool_callback_context_get();
        /* Exhaustion is necessary, but not sufficient. Check the commanded
         * endpoint and the actual scene/world pose before retiring the head.
         * Explicitly canceled DONE heads retain their existing cleanup path. */
        if ((head->state & CMD_STATE_MOVE_ROUTED) == 0u ||
            entity->wp_index != entity->wp_count ||
            !cmd_move_route_endpoint_matches(entity, head) ||
            !entity_route_waypoint_world_get(entity,
                (unsigned int)(entity->wp_count - 1), endpoint)) return 0;
        entity_transform_position_get(entity, position);
        (void)cmd_rec_cell_xy_get(head, &target[0], &target[1]);
        if (context == NULL || context->level == NULL ||
            !terrain_world_xy_to_cell(context->level->ptr_terrain,
                position[0], position[1], &cell[0], &cell[1]) ||
            cell[0] != target[0] || cell[1] != target[1]) return 0;
        if (!(fabs((double)(position[0] - endpoint[0])) <= 0.001 &&
              fabs((double)(position[1] - endpoint[1])) <= 0.001)) return 0;
    }
    saved_enabled = s_global_enabled;
    s_global_enabled |= 4;
    head->state |= CMD_STATE_DONE;
    (void)entity_command_chain_head_remove(entity);
    entity_action_sequence_reset(entity, 0);
    s_global_enabled = saved_enabled;
    return 1;
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

int cmd_purge_compare_adapter(const void* left, const void* right)
{
    if (s_cmd_purge_compare == NULL) {
        return 0;
    }
    return s_cmd_purge_compare(left, right, s_cmd_purge_context);
}

int cmd_purge_compare_default(const void* left, const void* right)
{
    const CmdRec* a = *(const CmdRec* const*)left;
    const CmdRec* b = *(const CmdRec* const*)right;
    const uint32_t a_state = a->state & 0x100u;
    const uint32_t b_state = b->state & 0x100u;
    int a_behaviour = a->behaviour;
    int b_behaviour = b->behaviour;
    if (a_state != 0u && b_state == 0u) return -1;
    if (a_state != 0u && b_state != 0u) {
        /* The original's comparator (0x403f80) orders flag-0x100 records by
         * the unsigned word at +0x50, not by a link. The frozen CmdRec names
         * that word global_next; since finding 570 merged the global-list
         * link into +0x60, nothing in the port writes it (its original
         * writer is unrecovered), so it reads as zero here. */
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

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
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

void cmd_tick_global(void)
{
    cmd_tick_global_delta(0.0f);
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* ------------------------------------------------------------------ *
 *  Idle queue table (IdleQueueRegister): type-2 entries.
 * ------------------------------------------------------------------ */

/* The port's copy of Bubbles.c's bubbleTable (0x00557a84, bubbles.c). */
IdleQueueRecord s_idle[IDLE_TABLE_MAX];

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

void cmd_context_set(GameContext* ctx)
{
    s_ctx = ctx;
    selection_active_context_bind(ctx);
    if (entity_pool_callback_context_get() != ctx)
        entity_pool_callback_context_set(ctx);
    cmd_place_eligibility_ops_bind();
    s_ping = 0;
    for (int i = 0; i < 2; ++i) {
        s_queued_count[i] = 0;
    }
}

void cmd_enqueue(int opcode, const EntitySlot* e, int param, int p1, int p2)
{
    int pair[2];
    pair[0] = p1;
    pair[1] = p2;
    cmd_produce(opcode, e, param, pair);
}

KeyBind s_keybinds[KEYBIND_MAX];
int     s_keybind_count = 0;

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

int cmd_queue_active_count_peek(void)
{
    return s_queued_count[s_ping];
}

int selection_entity_remove(void* entity)
{
    int result;
    int i;
    if (s_ctx == NULL || entity == NULL) {
        return 0;
    }
    selection_active_context_bind(s_ctx);
    result = selection_entity_remove_active((EntitySlot*)entity);
    if (result != 0) {
        s_ctx->selection_count = (int)selection_active_count_get();
        for (i = 0; i < s_ctx->selection_count; ++i) {
            s_ctx->selection[i] = selection_active_records_get()[i];
        }
        ((EntitySlot*)entity)->flags &= ~ENTITY_SELECTED;
    }
    return result;
}

int selection_candidate_filter(void* entity, int mode, int param_3)
{
    if (s_ctx == NULL || entity == NULL) {
        return 0;
    }
    selection_active_context_bind(s_ctx);
    {
        const unsigned int count_before = selection_active_count_get();
        const int result = selection_candidate_filter_active(
            entity, mode, param_3);
        const unsigned int count_after = selection_active_count_get();
        if (count_after > count_before) {
            unsigned int i;
            EntitySlot* added = selection_active_records_get()[count_before];
            s_ctx->selection_count = (int)count_after;
            added->flags |= ENTITY_SELECTED;
            for (i = 0; i < count_after; ++i) {
                s_ctx->selection[i] = selection_active_records_get()[i];
            }
        }
        return result;
    }
}

/* --- entity record schedulers (see cmd.hpp) ---------------------------------- */

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
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

int cmd_global_enabled_peek(void) { return s_global_enabled; }

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
#endif /* !LEGORRR_BACKEND_WINAPI */

CoordinatePairEntityCallback s_coordinate_pair_entity_callback;
ArgumentEntityCallback s_argument_entity_callback;
void* s_callback_dispatch_context;
TaskCallbackEntityFn s_task_callback_entity_fn;
void* s_task_callback_entity_context;
TaskCallbackEntityFn s_pending_record_callback_fn;
void* s_pending_record_callback_context;
EntityTaskQueueFlushHooks s_entity_task_queue_flush_hooks;
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

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
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
#endif /* !LEGORRR_BACKEND_WINAPI */

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

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
int cmd_rec_action_get(const void* rec)
{
    return rec != NULL ? (int)cas_rec(rec)->cell_act : -1;
}

int cmd_rec_target_get(const void* rec)
{
    return rec != NULL ? (int)cas_rec(rec)->cell_target : -2;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

void cmd_tutorial_congregation_set(int x, int y, int enable)
{
    const int pair[2] = { x, y };
    if (enable != 0) coordinate_pair_task_record_queue(pair);
    else task_registry_remove_type0_entry_by_pair(pair);
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void* cmd_rec_event_payload_get(const void* rec)
{
    return rec != NULL ? cas_rec(rec)->event_payload : NULL;
}

int cmd_rec_cell_count_get(const void* rec)
{
    return rec != NULL ? (int)cas_rec(rec)->cell_count : -1;
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
#endif /* !LEGORRR_BACKEND_WINAPI */

void cmd_link_global_head(void* rec, int kind)
{
    cmd_link_head(as_rec(rec), kind);
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
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
#endif /* !LEGORRR_BACKEND_WINAPI */

/* --- the cell-command type-3 issue + task batch append band (2026-10-25) -- */

/* The queue is defined natively only (#700, finding 627). In the matching
 * build legoobject_unit.c holds LegoObject.c's deferred list, nothing
 * reachable adds an entry (deferred_olist_entity_queue_add,
 * legoobject_port.cpp, has no live caller there), so the count stays 0. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
DeferredOListEntry s_deferred_olist[DEFERRED_OLIST_SLOTS];
#endif
uint32_t           s_deferred_olist_next; /* DAT_004ea7a4 */

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
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
#endif /* !LEGORRR_BACKEND_WINAPI */

void deferred_olist_reset(void)
{
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
    for (unsigned int i = 0u;
         i < s_deferred_olist_next && i < DEFERRED_OLIST_SLOTS; ++i) {
        engine_heap_free(s_deferred_olist[i].names[0]);
        engine_heap_free(s_deferred_olist[i].names[1]);
    }
    __builtin_memset(s_deferred_olist, 0, sizeof(s_deferred_olist));
#endif
    s_deferred_olist_next = 0u;
}

unsigned int deferred_olist_entity_queue_take_cell(const int cell_xy[2],
    DeferredOListEntityRecord* records, unsigned int capacity)
{
    unsigned int count = 0u, i = 0u;
    if (cell_xy == NULL || records == NULL) return 0u;
#if defined(_MSC_VER) && _MSC_VER < 1200
    (void)capacity; (void)i; /* no queue in the matching build (#700) */
#else
    while (i < s_deferred_olist_next && i < DEFERRED_OLIST_SLOTS &&
           count < capacity) {
        DeferredOListEntry* entry = &s_deferred_olist[i];
        if ((int)entry->w[0] != cell_xy[0] ||
            (int)entry->w[1] != cell_xy[1]) { ++i; continue; }
        DeferredOListEntityRecord* out = &records[count++];
        out->cell_x = (int)entry->w[0];
        out->cell_y = (int)entry->w[1];
        memcpy(&out->x, &entry->w[2], 4);
        memcpy(&out->y, &entry->w[3], 4);
        memcpy(&out->heading, &entry->w[4], 4);
        out->source_kind = (int)entry->w[5];
        out->entity_type = (int)entry->w[6];
        out->variant = (int)entry->w[7];
        out->record_flags = (int)entry->w[8];
        out->name = entry->names[0];
        out->link_name = entry->names[1];
        --s_deferred_olist_next;
        *entry = s_deferred_olist[s_deferred_olist_next];
        memset(&s_deferred_olist[s_deferred_olist_next], 0, sizeof(*entry));
    }
#endif
    return count;
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
uint32_t s_menu_surface_track; /* DAT_004de1f0 */

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
uint32_t menu_surface_track_peek(void)
{
    return s_menu_surface_track;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

const char* s_upgrade_names[UPGRADE_NAME_TABLE_MAX];
// GLOBAL: LEGORR 0x005561e4
void* s_upgrade_records;
unsigned s_upgrade_count;

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

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void kind_six_sweep_timer_install(float value) { s_sweep_timer = value; }
float kind_six_sweep_timer_peek(void) { return s_sweep_timer; }

uint32_t command_sample_stats_peek(int kind, int field)
{
    if (kind < 0 || (unsigned)kind >= CMD_STATS_KINDS ||
        field < 0 || field > 24) {
        return 0u;
    }
    if (field < (int)CMD_STATS_LOG_DWORDS) {
        TimedEffectRecord* record = timed_effect_record_get((unsigned int)kind);
        return record != NULL
            ? ((const uint32_t*)record)[CMD_STATS_LOG_DWORD + field] : 0u;
    }
    switch (field) {
    case 20:
    case 21:
    case 22:
    case 23:
    case 24: {
        TimedEffectRecord* record = timed_effect_record_get((unsigned int)kind);
        const uint32_t* slot;
        if (record == NULL) return 0u;
        slot = (const uint32_t*)record;
        switch (field) {
        case 20: return slot[CMD_STATS_COUNT_DWORD];
        case 21: return slot[CMD_STATS_HEAD_DWORD];
        case 22: return slot[CMD_STATS_WINDOW_DWORD];
        case 23: return slot[CMD_STATS_TOTAL_DWORD];
        case 24: return slot[CMD_STATS_HIGH_DWORD];
        }
    }
    default: return 0u;
    }
}
#endif /* !LEGORRR_BACKEND_WINAPI */

namespace unported_ledger_stubs {

ObjectTaskEligibilityOps* s_object_task_eligibility_ops;
void object_task_eligibility_ops_bind(ObjectTaskEligibilityOps* ops)
{
    s_object_task_eligibility_ops = ops;
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
