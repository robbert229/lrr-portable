// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-002 AITask.c port tail (ADR 0006 decision 4, finding 570). The
 * original-shape unit is aitask.c; this file keeps the port's command-record
 * implementation (CmdRec, its pool, lists and hooks) under the port's names,
 * unannotated, so native callers and tests are unchanged. The functions were
 * relocated verbatim by tools/relocate_unit.py (body hashes:
 * docs/coverage/relocations/GAME-002.csv) and are kept in original address
 * order; "Port model of 0x..." names the original each one models. */
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
#include "gods98/string.hpp"
#include "cmd_shared.hpp"
#include "cmd_for_message_unit.hpp"
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include "gods98/texture.hpp"
#include "building_scene.hpp"
#include "actor_scene.hpp"
#include "mission.hpp"
#include "ai.h"
#include <stdint.h>
#include "type_config_ports.hpp"
#include "entity_pose_delta.hpp"
#include "entity_motion_ports.hpp"
#include "activity_timeout.hpp"
#include "entity_motion_probe.hpp"
#include "simulation_ui_ports.hpp"
#include "type_motion_scale.hpp"
#include "entity_quad_collision.hpp"
#include "weapon.hpp"
#if defined(LEGORRR_BACKEND_MINIWIN)
#include "creature_scene.hpp"
#endif
#include "gods98/model.hpp"
#include "gods98/resource.hpp"
#include "gods98/record.hpp"
#include "upgrades.hpp"
#include "entity_for_legoobject.hpp"
#include "entity_for_lego.hpp"
#include "entity_from_legoobject.hpp"
#include <limits.h>
#include "game.hpp"
#include "gods98/simtick.hpp"
#include "gods98/surface.hpp"
#include "gods98/camera.hpp"
#include "objective.hpp"
#include "gods98/flic.hpp"
#include "scene_render.hpp"
#include "tutorial_hud_ports.hpp"
#include "game/render.hpp"
#include "simtick_shared.hpp"
#include "cmd_for_aitask.hpp"
#include "ai_for_aitask.hpp"
#include "level_for_aitask.hpp"
#include "gods98/simtick_for_aitask.hpp"
/* AITask.c's priority tables live in the unit's state (aitask.c); the
 * frozen headers' s_behaviour/s_priority_state declarations are not
 * defined anywhere since finding 570. */
extern "C" int* aitask_port_priority_values(void);
extern "C" int* aitask_port_priority_states(void);
#define s_behaviour (aitask_port_priority_values())
#define s_priority_state (aitask_port_priority_states())
#if defined(_MSC_VER) && defined(_M_IX86)
/* The frozen header defines these only for other compilers: the port's
 * naked-asm registry initialiser was dropped by finding 570, so the C body
 * now builds under MSVC too. */
#define CMD_TASK_NAME_SET(index, value) \
    (((const char* volatile*)s_task_names)[index] = value)
#define CMD_PRIORITY_NAME_SET(index, value) \
    (((const char* volatile*)s_priority_names)[index] = value)
#endif

struct TerrainMap;
extern "C" void terrain_map_block_highlight_clear(TerrainMap* map, int bx, int by);

#define CMD_STRIDE sizeof(CmdRec)

/* The callback writes independent 50-entry arrays at +0x335c and +0x3428.
 * Assignment and timer scans consume them with separate swap-with-last
 * rules. The pool builder below only supports direct scan entry calls. */
#define PENDING_CANDIDATE_CAPACITY 50

struct CoordinatePairCallbackPayload {
    const void* coordinate_pair;
    int extra;
};
/* Port state for 0x004b31c8 (GAME-002). */
CmdRec* s_blocks[CMD_POOL_MAX_BLOCKS] = { 0 };
/* The corpus table ends at the free-list head: HandleArrayReleaseAndReset
 * advances four bytes at a time and compares esi with 0x4b31f8. Keep this
 * declaration immediately after the 12 table entries. */
/* Port state for 0x004b31f8 (GAME-002). */
void*   s_free       = NULL;
/* Port state for 0x004b31fc (GAME-002). */
int     s_pool_level = 0;

/* Port state for 0x004b3200 (GAME-002). */
const char* s_task_names[31];
/* Port state for 0x004b327c (GAME-002). */
const char* s_priority_names[27];
/* Port state for 0x004b32e8 (GAME-002). */
/* in aitask.c's state (finding 570); see the accessors above */
/* Port state for 0x004b3354 (GAME-002). */
CmdRec* s_global_head = NULL; /* DAT_004b3354 */

/* Port state for 0x004b335c (GAME-002). */
EntitySlot* s_active_list[ACTIVE_ENTITY_LIST_MAX];
/* Port state for 0x004b3424 (GAME-002). */
int s_active_list_used;

/* Port state for 0x004b34f0 (GAME-002). */
unsigned s_pending_timer_count;

/* --- registry accessors / match marker (see cmd.hpp) -------------------------
 * DAT_004b7ff4: the per-priority state table, zeroed by the (unported)
 * registry initializer loop — zero-init static, 27 entries. The preceding
 * command-count matrix occupies 0x12c0 dwords and ends at this table. */
/* Port state for 0x004b34f4 (GAME-002). */
uint32_t cmd_pending_command_counts[CMD_PENDING_COMMAND_COUNT_TABLE_SIZE];
/* Port state for 0x004b7ff4 (GAME-002). */
/* in aitask.c's state (finding 570); see the accessors above */
/* Port state for 0x004b8060 (GAME-002). */
int   s_global_enabled = 0; /* DAT_004b8060: zero-init; cmd_init sets bit 0 */
static EntitySlot* s_pending_assignment_candidates[PENDING_CANDIDATE_CAPACITY];
static EntitySlot* s_pending_timer_candidates[PENDING_CANDIDATE_CAPACITY];

/* CmdTickGlobal builds these two retail arrays during its preceding filtered
 * entity walk. Direct callers of the scans still get a live-pool fallback. */
static int s_pending_candidates_from_entity_walk;

static const char s_cmd_task_goto[] = "Goto";
static const char s_cmd_task_follow[] = "Follow";
static const char s_cmd_task_follow_attack[] = "FollowAttack";
static const char s_cmd_task_collect[] = "Collect";
static const char s_cmd_task_gather[] = "Gather";
static const char s_cmd_task_deposit[] = "Deposit";
static const char s_cmd_task_dump[] = "Dump";
static const char s_cmd_task_request[] = "Request";
static const char s_cmd_task_dig[] = "Dig";
static const char s_cmd_task_dynamite[] = "Dynamite";
static const char s_cmd_task_repair[] = "Repair";
static const char s_cmd_task_reinforce[] = "Reinforce";
static const char s_cmd_task_clear[] = "Clear";
static const char s_cmd_task_wait[] = "Wait";
static const char s_cmd_task_animation_wait[] = "AnimationWait";
static const char s_cmd_task_elec_fence[] = "ElecFence";
static const char s_cmd_task_eat[] = "Eat";
static const char s_cmd_task_goto_eat[] = "GotoEat";
static const char s_cmd_task_find_driver[] = "FindDriver";
static const char s_cmd_task_get_tool[] = "GetTool";
static const char s_cmd_task_bird_scarer[] = "BirdScarer";
static const char s_cmd_task_upgrade[] = "Upgrade";
static const char s_cmd_task_build_path[] = "BuildPath";
static const char s_cmd_task_train[] = "Train";
static const char s_cmd_task_depart[] = "Depart";
static const char s_cmd_task_attack_path[] = "AttackPath";
static const char s_cmd_task_attack_rock_monster[] = "AttackRockMonster";
static const char s_cmd_task_recharge[] = "Recharge";
static const char s_cmd_task_dock[] = "Dock";
static const char s_cmd_task_attack_object[] = "AttackObject";
static const char s_cmd_task_find_load[] = "FindLoad";
static const char s_cmd_priority_crystal[] = "Crystal";
static const char s_cmd_priority_ore[] = "Ore";
static const char s_cmd_priority_default_collect[] = "DefaultCollect";
static const char s_cmd_priority_destruction[] = "Destruction";
static const char s_cmd_priority_construction[] = "Construction";
static const char s_cmd_priority_request[] = "Request";
static const char s_cmd_priority_reinforce[] = "Reinforce";
static const char s_cmd_priority_repair[] = "Repair";
static const char s_cmd_priority_clearing[] = "Clearing";
static const char s_cmd_priority_storage[] = "Storage";
static const char s_cmd_priority_refining[] = "Refining";
static const char s_cmd_priority_health_low[] = "HealthLow";
static const char s_cmd_priority_get_in[] = "GetIn";
static const char s_cmd_priority_upgrade[] = "Upgrade";
static const char s_cmd_priority_build_path[] = "BuildPath";
static const char s_cmd_priority_attack_rock_monster[] = "AttackRockMonster";
static const char s_cmd_priority_barrier[] = "Barrier";
static const char s_cmd_priority_find_load[] = "FindLoad";
static const char s_cmd_priority_recharge[] = "Recharge";
static const char s_cmd_priority_upgrade_building[] = "UpgradeBuilding";
static const char s_cmd_priority_gather[] = "Gather";
static const char s_cmd_priority_steal[] = "Steal";
static const char s_cmd_priority_punch[] = "Punch";
static const char s_cmd_priority_depart[] = "Depart";
static const char s_cmd_priority_attack_path[] = "AttackPath";
static const char s_cmd_priority_attack_object[] = "AttackObject";
static const char s_cmd_priority_congregate[] = "Congregate";

namespace unported_ledger_stubs {
int ledger_stub_00443710_AdjacentPositionCandidateFind(
    EntitySlot* entity, const int candidate[2], int output[2]);
int entity_capacity_or_state_predicate(const EntitySlot* entity);
int entity_cell_step_command_issue_with_jitter(
    EntitySlot* entity, const int target_xy[2], const float input_xy[2],
    int action_mode, int mark_pending);

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
int ledger_stub_004403d0_LevelDebugCellRetargetDispatch(
    EntitySlot* entity, EntitySlot* target,
    const EntityDebugCellRetargetHooks* hooks);
int ledger_stub_00448eb0_ProximityStepCommandIssueWithJitter(
    EntitySlot* entity, EntitySlot* target, int marker);
void* ledger_stub_00438ae0_SpatialObjectQueryCenterMask02(
    const EntitySlot* entity, const int cell_xy[2], uint32_t filter_a,
    uint32_t filter_b);
} // namespace unported_ledger_stubs

extern "C" float random_float_range(float minimum, float maximum);

namespace unported_ledger_stubs {
int ledger_stub_00404fc0_PlaceCommandEligibilityCheck(
    const ObjectTaskEligibilityRecord* command, EntitySlot* entity,
    float screen_xy[2], float world_xy[2], uint32_t* out_flag,
    int require_command_gate, int relaxed_entity_gate);
void ledger_stub_00404ed0_CheckObjectCandidate(const EntitySlot* candidate);

void* ledger_stub_00438e50_SpatialObjectQueryCenterMask100(
    const EntitySlot* entity, const float center[2], uint32_t extra);
} // namespace unported_ledger_stubs

static int cmd_entity_global_record_special_step(EntitySlot* entity,
                                                  CmdRec* record,
                                                  CmdRec* previous);

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void cmd_pool_records_release_all(void);
void cmd_link_head(CmdRec* rec, int kind);
static void cmd_global_list_purge(CmdRec** head);
static void cmd_pool_timer_delta_walk(float* delta);
static void current_level_get(void);
static int cmd_has_entity_task_record(const EntitySlot* entity);
static int task_kind_for(EntitySlot* e);
static void cmd_link_tail(CmdRec* rec);
static int coordinate_pair_callback_adapter(EntitySlot* entity, void* payload);
static int argument_callback_adapter(EntitySlot* entity, void* argument);
static void cmd_cell_record_stage(CmdRec* rec, int x, int y, int act);
static void cmd_pair_stage(CmdRec* rec, const int pair[2]);
static void cmd_chain_append(EntitySlot* e, CmdRec* rec, int as_move);
static int task_callback_entity_adapter(EntitySlot* entity, void* payload);
static int pending_record_callback_adapter(EntitySlot* entity, void* payload);
static void cmd_bind_kind(CmdRec* rec, int kind);
static void cmd_raw_head_insert(CmdRec* rec);
static void cmd_raw_tail_insert(CmdRec* rec);
static int cmd_entity_chain_kind_dispatch(EntitySlot* entity, CmdRec* record);
static int cmd_entity_global_record_assign(EntitySlot* entity);
static void cmd_entity_pending_candidates_append(EntitySlot* entity);
static int cmd_gettool_route_adopt(EntitySlot* entity, CmdRec* head);
namespace unported_ledger_stubs { static int object_task_target_record_matches( const CmdRec* record, const uint32_t key[3]); }
static uint32_t filt_dword(const void* rec, size_t off);
static int cmd_place_target_nested_scene_pose_get(const EntitySlot* entity, float position[3]);
static unsigned pending_candidate_pool_build(GameContext* context, EntitySlot** output, int filter_flagged);
static int pending_record_matches_entity(const CmdRec* record, const EntitySlot* entity);
static void cmd_pool_grow(void);
static void cmd_pool_record_live_set(CmdRec* record, int live);
static void cmd_gettool_reference_release(CmdRec* record);

/* Port model of 0x00401cb0 (GAME-002). */
int cmd_kind_by_name(const char* name, int* out)
{
    /* stride-4 name-table walk, as the corpus: unsigned end compare (the
     * loop-back is jb) with the index counter beside the pointer (edi) */
    const char* const* entry = s_priority_names;
    const char* const* end = s_priority_names + 27;
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
/* C linkage for GAME-021 lego_unit.c (the frozen header declares it without). */
extern "C" void cmd_registry_init_target(void);
/* Port model of 0x00401cf0 (GAME-002). */
void cmd_registry_init_target(void)
{
    int i;
    for (i = 0; i < CMD_POOL_MAX_BLOCKS; ++i) {
        s_blocks[i] = NULL;
    }

    CMD_TASK_NAME_SET(0, "Goto");
    CMD_TASK_NAME_SET(1, "Follow");
    CMD_TASK_NAME_SET(2, "FollowAttack");
    CMD_TASK_NAME_SET(3, "Collect");
    CMD_TASK_NAME_SET(4, "Gather");
    CMD_TASK_NAME_SET(12, "Clear");
    CMD_TASK_NAME_SET(5, "Deposit");
    CMD_TASK_NAME_SET(6, "Dump");
    CMD_TASK_NAME_SET(7, "Request");
    CMD_TASK_NAME_SET(10, "Repair");
    CMD_TASK_NAME_SET(8, "Dig");
    CMD_TASK_NAME_SET(9, "Dynamite");
    CMD_TASK_NAME_SET(11, "Reinforce");
    CMD_TASK_NAME_SET(13, "Wait");
    CMD_TASK_NAME_SET(14, "AnimationWait");
    CMD_TASK_NAME_SET(15, "ElecFence");
    CMD_TASK_NAME_SET(16, "Eat");
    CMD_TASK_NAME_SET(17, "GotoEat");
    CMD_TASK_NAME_SET(18, "FindDriver");
    CMD_TASK_NAME_SET(19, "GetTool");
    CMD_TASK_NAME_SET(20, "BirdScarer");
    CMD_TASK_NAME_SET(21, "Upgrade");
    CMD_TASK_NAME_SET(22, "BuildPath");
    CMD_TASK_NAME_SET(23, "Train");
    CMD_TASK_NAME_SET(24, "Depart");
    CMD_TASK_NAME_SET(25, "AttackPath");
    CMD_TASK_NAME_SET(26, "AttackRockMonster");
    CMD_TASK_NAME_SET(27, "Recharge");
    CMD_TASK_NAME_SET(28, "Dock");
    CMD_TASK_NAME_SET(29, "AttackObject");
    CMD_TASK_NAME_SET(30, "FindLoad");

    CMD_PRIORITY_NAME_SET(0, "Crystal");
    CMD_PRIORITY_NAME_SET(1, "Ore");
    CMD_PRIORITY_NAME_SET(2, "DefaultCollect");
    CMD_PRIORITY_NAME_SET(3, "Destruction");
    CMD_PRIORITY_NAME_SET(4, "Construction");
    CMD_PRIORITY_NAME_SET(5, "Request");
    CMD_PRIORITY_NAME_SET(6, "Reinforce");
    CMD_PRIORITY_NAME_SET(7, "Repair");
    CMD_PRIORITY_NAME_SET(8, "Clearing");
    CMD_PRIORITY_NAME_SET(9, "Storage");
    CMD_PRIORITY_NAME_SET(10, "Refining");
    CMD_PRIORITY_NAME_SET(11, "HealthLow");
    CMD_PRIORITY_NAME_SET(12, "GetIn");
    CMD_PRIORITY_NAME_SET(13, "Upgrade");
    CMD_PRIORITY_NAME_SET(14, "BuildPath");
    CMD_PRIORITY_NAME_SET(15, "AttackRockMonster");
    CMD_PRIORITY_NAME_SET(16, "Barrier");
    CMD_PRIORITY_NAME_SET(17, "FindLoad");
    CMD_PRIORITY_NAME_SET(18, "Recharge");
    CMD_PRIORITY_NAME_SET(19, "UpgradeBuilding");
    CMD_PRIORITY_NAME_SET(20, "Gather");
    CMD_PRIORITY_NAME_SET(21, "Steal");
    CMD_PRIORITY_NAME_SET(22, "Punch");
    CMD_PRIORITY_NAME_SET(23, "Depart");
    CMD_PRIORITY_NAME_SET(24, "AttackPath");
    CMD_PRIORITY_NAME_SET(26, "Congregate");
    CMD_PRIORITY_NAME_SET(25, "AttackObject");

    for (i = 0; i < 27; ++i) {
        s_behaviour[i] = 0x31;
    }
    s_behaviour[4] = 0x3c;
    s_behaviour[3] = 0x28;
    s_behaviour[0] = 0x37;
    s_behaviour[9] = 5;
    s_behaviour[8] = 0x14;
    s_behaviour[10] = 0xf;
    s_behaviour[5] = 0x14;
    s_behaviour[6] = 0x46;
    s_behaviour[15] = 0x38;
    s_behaviour[19] = 99;
    s_behaviour[21] = 0x5a;
    s_behaviour[22] = 0x31;
    s_behaviour[20] = 0x31;
    s_behaviour[23] = 0;
    s_behaviour[24] = 0x14;
    s_behaviour[26] = 99;
    s_behaviour[25] = 0x1e;
    s_free = NULL;
    s_pool_level = 0;
    s_global_enabled = 1;
}

/* Port model of 0x00402000 (GAME-002). */
void cmd_mark_matching(int type, int payload, const int coords[2])
{
    for (CmdRec* cur = s_global_head; cur != NULL;
         cur = as_rec(cur->next)) {
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
/* Port model of 0x00402060 (GAME-002). */
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

/* TaskTableContentsRelease (FUN_004020c0): frees every non-null pool
 * block pointer. The corpus increments esi by four and compares it with
 * s_free at 0x4b31f8 ("cmp esi, 0x4b31f8"), then clears that head and the
 * scheduler word at 0x4b8060. */
/* Port model of 0x004020c0 (GAME-002). */
void handle_array_release_and_reset(void)
{
    CmdRec** walk = s_blocks;
    CmdRec** const end = s_blocks + CMD_POOL_MAX_BLOCKS;
    cmd_gettool_all_owners_release();
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
/* --- registry flush / wrappers (see cmd.hpp) ---------------------------------
 * The "handle array" the corpus's TaskTableContentsRelease walks IS the
 * cmd pool block array (0x4b31c8 band, bounded by the freelist head at
 * 0x4b31f8 — corpus-verified at 0x004020c0), so the release walks the pool
 * statics directly rather than a duplicate model. */
/* Port model of 0x00402100 (GAME-002). */
void cmd_flush_all(int disable_mode)
{
    if (disable_mode != 0) {
        s_global_enabled &= ~2;
        return;
    }
    /* FUN_00406540(&LAB_004021f0, 0): release each currently allocated
     * command record before resetting the scheduler roots and count tables. */
    cmd_pool_records_release_all();
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

/* Port model of 0x00402170 (GAME-002). */
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
/* Port model of 0x00402220 (GAME-002). */
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
         current = as_rec(current->next)) {
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
/* Port model of 0x00402310 (GAME-002). */
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
        CmdRec* next = as_rec(current->next);
        if ((current->state & CMD_STATE_DONE) != 0u) {
            if (previous == NULL) *head = next;
            else previous->next = next;
            current->next = NULL;
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
        current = as_rec(current->next);
    }
    if (current != NULL) overflow = current;
    if (count > 1) {
        qsort(records, (size_t)count, sizeof(records[0]),
              s_cmd_purge_compare != NULL ? cmd_purge_compare_adapter
                                          : cmd_purge_compare_default);
    }
    if (count != 0) {
        for (i = 0; i + 1 < count; ++i)
            records[i]->next = records[i + 1];
        records[count - 1]->next = overflow;
        *head = records[0];
    }
}

/* Port model of 0x00402510 (GAME-002). */
void terrain_cell_command_create(int x, int y, int flag, int raise20)
{
    CmdRec* rec = NULL;
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
    rec = (CmdRec*)cmd_alloc(8);
    if (rec == NULL) {
        return;
    }
    /* 0x40256c..75 copies only the coordinates. The carried-cell metadata
     * and flag 0x400 belong to kind nine: setting them here excludes this
     * Drill task from ordinary worker assignment. */
    __builtin_memcpy(&rec->x, &x, sizeof(rec->x));
    __builtin_memcpy(&rec->y, &y, sizeof(rec->y));
    if (flag != 0) {
        rec->state |= 8u;
    }
    entity_record_array_apply_zero_state(rec, 1);
    cmd_link_head(rec, 3);
    {
        const uint32_t prior = rec->state;
        rec->state = prior | 0x80u;
        if (raise20 != 0) {
            rec->state = prior | 0x20080u;
        }
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
/* --- selection-record array / entity-kind issuer (see cmd.hpp) ---------------
 * FUN_00402600 snapshots the selected entity pointers onto the command
 * record, resets each entity's action sequence, and optionally detaches its
 * carried object. */
/* Port model of 0x00402600 (GAME-002). */
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
/* Port model of 0x00402670 (GAME-002). */
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

/* Port model of 0x004026c0 (GAME-002). */
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
/* Port model of 0x00402720 (GAME-002). */
void cmd_create_task(EntitySlot* e, int32_t parameter)
{
    if (e == NULL) {
        return;
    }
    CmdRec* rec = (CmdRec*)cmd_alloc(3);
    if (rec == NULL) {
        return;
    }
    rec->payload = e;
    rec->param2 = parameter;
    int kind = task_kind_for(e);
    if (e->type == 8) {
        rec->extra = 8; /* type-8 trains set the extra slot */
    }
    cmd_link_head(rec, kind);
    cmd_minion_companion(e);
}

/* Port model of 0x004027a0 (GAME-002). */
void bobbin_clear_delay(SimtickBobbin* node)
{
    if (node == NULL) return;
    if (s_bobbin_clear_delay_hook != NULL) {
        s_bobbin_clear_delay_hook(node, s_bobbin_action_context);
        return;
    }

    /* BobbinClearDelay (FUN_004027a0) only queues the coordinate command
     * when the keyed cell is clear, has a nonzero speed class, and its
     * entry resources are ready. The retail body then allocates type 0x16,
     * stores the pair in +8/+c, and links it globally with kind 0xe. */
    GameContext* game = entity_pool_callback_context_get();
    int cell_xy[2];
    void* command;
    cell_xy[0] = (int)node->key_pair[0];
    cell_xy[1] = (int)node->key_pair[1];
    if (game == NULL || game->level == NULL ||
        level_cell_occupied_at_current(cell_xy) != 0u ||
        level_cell_speed_class(game->level, (int)node->key_pair[0],
                               (int)node->key_pair[1]) == 0 ||
        ensure_cell_entries_loaded(0xfu, 0, 0) == 0) {
        return;
    }
    command = cmd_alloc(0x16);
    if (command == NULL) return;
    cmd_rec_move_xy_set(command, (float)node->key_pair[0],
                        (float)node->key_pair[1]);
    cmd_link_global_head(command, 0xe);
} /* FUN_004027a0 */

/* Port model of 0x00402800 (GAME-002). */
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
/* MinionCompanionCmd (FUN_00402850): append the extra kind-0x15 task that
 * accompanies a type-6 minion's primary task. */
/* Port model of 0x00402850 (GAME-002). */
void cmd_minion_companion(EntitySlot* e)
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

/* --- task-type-23 category-mask issuer (see cmd.hpp) -------------------------
 * The corpus's entity argument rides every read blindly; the port guards the
 * entity slot because the selector gate reads its stats record. The allocator's
 * result is stored blindly, +0x10 payload before +0x3c extra, the selector
 * riding the interleaved stores straight from the argument. */
/* Port model of 0x00402890 (GAME-002). */
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
/* Port model of 0x00402960 (GAME-002). */
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

/* Port model of 0x00402a40 (GAME-002). */
void coordinate_pair_callback_dispatch(const void* coord_pair)
{
    CoordinatePairCallbackPayload payload;
    payload.coordinate_pair = coord_pair;
    payload.extra = 0;
    entity_pool_callback_search(coordinate_pair_callback_adapter, &payload);
}

/* Port model of 0x00402aa0 (GAME-002). */
void argument_callback_dispatch(void* arg)
{
    entity_pool_callback_search(argument_callback_adapter, arg);
}

/* Retail ABI: coordinate pair plus mode, current level fetched internally.
 * The level-taking helper above remains for portable callers with an
 * explicit context. */
/* Port model of 0x00402ae0 (GAME-002). */
void terrain_cell_refresh_dispatch_current(const int cell_xy[2], int mode)
{
    LevelState* lvl = level_current_owner_get();
    TerrainCellRefreshCallbackPayload payload;
    void* terrain;
#if !defined(_MSC_VER) || !defined(_M_IX86)
    if (lvl == NULL || cell_xy == NULL) return;
#endif
    payload.cell_xy = cell_xy;
    payload.mode = mode;
    (void)entity_pool_callback_search(
        terrain_cell_refresh_callback_adapter, &payload);
#if defined(_MSC_VER) && defined(_M_IX86)
    terrain = (void*)lvl->word[1];
    terrain_cell_surface_type_reset(terrain, cell_xy[0], cell_xy[1]);
#else
    terrain = lvl->ptr_terrain;
    if (terrain != NULL)
        terrain_cell_surface_type_reset(terrain, cell_xy[0], cell_xy[1]);
#endif
}

/* Port model of 0x00402b30 (GAME-002). */
void coordinate_callback_dispatch_current(const void* coord_pair)
{
    CoordinateEntityDispatchPayload payload;
    payload.coordinate_pair = coord_pair;
    payload.extra = 0;
    payload.callback = s_coordinate_direct_callback;
    entity_pool_callback_search(coordinate_entity_dispatch_adapter, &payload);
}

/* Port model of 0x00402b60 (GAME-002). */
void coordinate_callback_dispatch_by_mode(const void* coord_pair, int mode)
{
    CoordinateEntityDispatchPayload payload;
    payload.coordinate_pair = coord_pair;
    payload.extra = 0;
    payload.callback = mode != 0 ? s_coordinate_mode_nonzero_callback
                                 : s_coordinate_mode_zero_callback;
    entity_pool_callback_search(coordinate_entity_dispatch_adapter, &payload);
}

/* --- cancellation-ref and conditional issuers (see cmd.hpp) ------------------
 * The referenced-record slot (+0x58) is the type-0x13 "ref" pair the
 * shipped binary's spawn-dispatch switch allocates through CmdAlloc(0x13);
 * the validated auxiliary-work producer installs a reciprocal pair. Direct
 * numeric-selector jobs leave this slot NULL. */
/* Port model of 0x00402cd0 (GAME-002). */
void cmd_cancel_ref(void* rec)
{
    if (rec == NULL) {
        return; /* deviation: the corpus reads the type word blindly */
    }
    CmdRec* r = as_rec(rec);
    if (r->type == 0x13) {
        if (r->ref != NULL) {
            /* Native402ce2 passes &record to the LIVE COMMAND pool search.
             * LAB402d30 only compares candidate+58 with that argument and
             * returns a predicate; it writes nothing. The result is discarded.
             * There is no entity/route cancellation side effect to synthesize. */
        }
        return; /* pure native search has no observable writes */
    }
    if (r->ref == NULL) {
        return;
    }
    for (CmdRec* cur = s_global_head; cur != NULL;
         cur = as_rec(cur->next)) {
        if (cur->type == 0x13 && cur == (CmdRec*)r->ref) {
            cur->state |= 0x20u; /* the completion/cancel flag */
        }
    }
}

/* Port model of 0x00402d50 (GAME-002). */
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
/* Port model of 0x00402e40 (GAME-002). */
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
/* Port model of 0x00402ec0 (GAME-002). */
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
/* Port model of 0x00402f30 (GAME-002). */
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
/* Port model of 0x00402f70 (GAME-002). */
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

/* Port model of 0x00402fb0 (GAME-002). */
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

/* CellCommandType3Issue (FUN_00403030) — see cmd.hpp. */
/* Port model of 0x00403030 (GAME-002). */
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
/* Port model of 0x004030b0 (GAME-002). */
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

/* ValidatedRecordCloneQueue (FUN_004030e0): retain a command record only
 * when its leading code is present in the grouped code table. The optional
 * payload allocation uses the full count word, while the copy loop masks it
 * to 30 bits, matching the reference's encoded-count handling. */
/* Port model of 0x004030e0 (GAME-002). */
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
/* Port model of 0x00403190 (GAME-002). */
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

/* Port model of 0x004031e0 (GAME-002). */
int active_entity_list_contains_code(GameContext* ctx, int code)
{
    unsigned count;
    EntitySlot* const* records;
    EntitySlot* const* walk;
    unsigned i;

    if (selection_has_any(ctx) == 0) {
        /* the fallback re-reads the count (the corpus's DAT_004b3424
         * load) on every iteration and skips NULL entries (test/je). */
        for (i = 0; i < (unsigned) s_active_list_used; ++i) {
            if (s_active_list[i] != NULL
                && ai_activity_scan(s_active_list[i], code) != 0) {
                return 1;
            }
        }
        return 0;
    }
    /* selection branch: cached count (corpus ebx), unsigned counter and
     * a walk pointer (inc / add reg,4 / cmp-jb); entries go BLIND to
     * the scan — the corpus has no NULL test here and the scan
     * (FUN_0044b480) answers 0 for NULL. */
    count = (unsigned) selection_count(ctx);
    records = selection_record_array(ctx);
    for (i = 0, walk = records; records != NULL && i < count; ++i, ++walk) {
        if (ai_activity_scan(*walk, code) != 0) {
            return 1;
        }
    }
    return 0;
}

/* Queue operation 8 addresses the active selection, not its object word.
 * The coordinate pair remains two raw integer cell words. */
/* Port model of 0x00403270 (GAME-002). */
void selection_drag_area_command_create(EntitySlot* const* entities,
    unsigned int count, const int cell_xy[2], int replace)
{
    unsigned int i;
    for (i = 0u; i < count; ++i) {
        CmdRec* record = as_rec(cmd_alloc(0));
        EntitySlot* entity = entities[i];
        CmdRec* tail;
        if (record == NULL) return;
        memcpy(&record->x, cell_xy, 2 * sizeof(uint32_t));
        record->state = 4u;
        if (((entity_stats_word_get(entity) & 0x20000u) == 0u ||
             entity->owner_36c != NULL) && entity->attach_304 == NULL &&
            (entity->flags_3e8 & 0x80u) != 0u) {
            if (replace != 0) command_queue_sweep_relink(entity);
            tail = as_rec(cmd_chain_walk(entity));
            if (tail != NULL) tail->next = record;
            else entity->cmd_head = record;
        }
    }
}

/* ObjectCommandTaskCreate (FUN_00403320): resolve a task target according to
 * the entity kind, then append a type-0x15 record carrying that target and
 * the caller's parameter. */
/* Port model of 0x00403320 (GAME-002). */
void object_command_task_create(EntitySlot* entity, int parameter)
{
    void* target = NULL;
    CmdRec* rec;
    CmdRec* tail;
    if (entity == NULL) return;
    if (entity->type == 2 && (entity->flags_3e0 & 0x400u) == 0u) {
        float position[2];
        entity_transform_position_get(entity, position);
        target = entity_lookup_at_world_position(position);
    }
    else if (entity->type == 1) {
        target = unported_ledger_stubs::
            ledger_stub_00438bf0_SpatialObjectQueryCenterMask80(entity);
    }
    if (target == NULL) return;

    rec = (CmdRec*)cmd_alloc(0x15);
    if (rec == NULL) return; /* allocation failure is a port-only guard */
    rec->payload = target;
    rec->cell_count = parameter;
    /* FUN_00403320 allocates and fills the new record before sweeping the
     * existing chain, then appends it through FUN_00404190's tail walk. */
    command_queue_sweep_relink(entity);
    tail = as_rec(cmd_chain_walk(entity));
    if (tail != NULL) tail->next = rec;
    else entity->cmd_head = rec;
    rec->next = NULL;
}

/* ObjectAttachedRecordAllocate (FUN_004033c0) — see cmd.hpp. */
/* Port model of 0x004033c0 (GAME-002). */
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
/* Port model of 0x004033f0 (GAME-002). */
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

/* Port model of 0x00403430 (GAME-002). */
void task_registry_remove_type0_entry_by_pair(const int pair[2])
{
    if (pair == NULL) {
        return; /* deviation: the corpus reads the pair blindly */
    }
    CmdRec* prev = NULL;
    for (CmdRec* cur = s_global_tail; cur != NULL;) {
        CmdRec* next = as_rec(cur->next);
        int cx = 0, cy = 0;
        cmd_rec_cell_xy_get(cur, &cx, &cy);
        if (cur->type == 0 && cx == pair[0] && cy == pair[1]) {
            if (prev != NULL) {
                prev->next = next;
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
/* Port model of 0x004034a0 (GAME-002). */
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
/* Port model of 0x004034e0 (GAME-002). */
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
/* Port model of 0x00403520 (GAME-002). */
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

/* Port model of 0x00403560 (GAME-002). */
void task_registry_remove_type10_entries_for_entity(const EntitySlot* e)
{
    CmdRec* prev = NULL;
    for (CmdRec* cur = s_global_tail; cur != NULL;) {
        CmdRec* next = as_rec(cur->next);
        if (cur->type == 10 && cur->payload == (void*)e) {
            if (prev != NULL) {
                prev->next = next;
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
/* Port model of 0x004035c0 (GAME-002). */
void cmd_create_move(EntitySlot* e, const float position_xy[2], int flag)
{
    CmdRec* rec;
    CmdRec* tail;
    if (e == NULL || position_xy == NULL) return;
    rec = (CmdRec*)cmd_alloc(0);
    if (rec == NULL) return;
    memcpy(&rec->x, position_xy, 2 * sizeof(uint32_t));
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

/* Port model of 0x00403610 (GAME-002). */
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

/* EntityTaskBatchAppend (FUN_00403650) — see cmd.hpp. */
/* Port model of 0x00403650 (GAME-002). */
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
/* Port model of 0x004036c0 (GAME-002). */
void cmd_create_target(EntitySlot* e, void* target)
{
    CmdRec* rec;
    CmdRec* tail;
    if (e == NULL) {
        return;
    }
    rec = (CmdRec*)cmd_alloc(2);
    if (rec == NULL) {
        return;
    }
    rec->payload = target;
    rec->now = 0.0f;
    /* CmdCreateTarget writes +0x14 = 0 and links the tail directly; it does
     * not use the global-list link helper that seeds 1500.0f. */
    tail = as_rec(cmd_chain_walk(e));
    if (tail != NULL) {
        tail->next = rec;
    }
    else {
        e->cmd_head = rec;
    }
}

/* CmdCancelByCoords (FUN_00403700) — see cmd.hpp. Strides the global
 * chain through the one +0x60 link, as the original does (finding 570). */
/* Port model of 0x403700 (GAME-002). */
void cmd_cancel_by_coords(int zone, int probe, int x, int y)
{
    for (CmdRec* cur = s_global_head; cur != NULL;
         cur = (CmdRec*)cur->next) {
        if (cur->type == 7 && cur->zone == zone
            && (probe == 0
                || (x == cur->cell_act && y == cur->cell_target))) {
            cur->state |= CMD_STATE_DONE;
        }
    }
}
// CommandKindRecordIssueAndLink (FUN_00403750).
/* Port model of 0x00403750 (GAME-002). */
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

/* TerrainCellRecordCountUpdate (FUN_00403810): query at the cell center,
 * issue the requested entity action when found, and otherwise increment the
 * flattened pending-count table. The spatial callback predicate is still
 * owned by the spatial-query adapter; this routine preserves the exact
 * query/action/fallback order around it. */
/* Port model of 0x00403810 (GAME-002). */
void terrain_cell_record_count_update(const int cell_xy[2], int a, int b,
                                      int c)
{
    EntitySlot* entity;
    entity = (EntitySlot*)
        unported_ledger_stubs::
            ledger_stub_00438a60_SpatialObjectQueryCenterMask01(
                cell_xy, (uint32_t)a, (uint32_t)b, (uint32_t)c);
    if (entity == NULL) {
        if (c != 0 && b == 7) {
            entity = (EntitySlot*)
                unported_ledger_stubs::
                    ledger_stub_00438a60_SpatialObjectQueryCenterMask01(
                        cell_xy, 7u, 0u, (uint32_t)c);
            if (entity != NULL) {
                EntitySlot* count_entity = (EntitySlot*)
                    unported_ledger_stubs::
                        ledger_stub_00438ae0_SpatialObjectQueryCenterMask02(
                            NULL, cell_xy, 7u, 0u);
                uint32_t count = stats_instance_word_at_ec(count_entity);
                if (count == 0u) return;
                do {
                    (void)entity_action_kind_entity_create_and_command_issue(
                        entity, 7, 0, c);
                    --count;
                } while (count != 0u);
                return;
            }
        }
    }
    else if (entity_action_kind_entity_create_and_command_issue(
                 entity, a, b, c) != NULL) {
        return;
    }
    /* DAT_004b34f4 is the 20 x 15 x 16 counter table also swept by
     * CellRecordCountsPruneForEntity. Keep the retail flattening expression;
     * the bounds guard protects the host table when callers provide an
     * invalid selector (the reference writes blindly). */
    if (a >= 0 && a < 20 && b >= 0 && b < 15 && c >= 0 && c < 16)
        ++cmd_pending_command_counts[b * 16 + a * 0xf0 + c];
}

/* Port model of 0x004038c0 (GAME-002). */
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

/* Port model of 0x00403910 (GAME-002). */
void terrain_cell_record_counts_prune(void* entity)
{
    int a, b, c;
    uint32_t* count = cmd_pending_command_counts;
    for (a = 0; a < 20; ++a) {
        for (b = 0; b < 15; ++b) {
            for (c = 0; c < 16; ++c, ++count) {
                uint32_t ordinal = 0;
                uint32_t removed = 0;
                while (ordinal < *count) {
                    if (unported_ledger_stubs::
                            object_task_target_filter_find((uint32_t)a,
                                (uint32_t)b, (uint32_t)c) == NULL ||
                        entity_action_kind_entity_create_and_command_issue(
                            (EntitySlot*)entity, a, b, c) != NULL) {
                        ++removed;
                    }
                    ++ordinal;
                }
                *count -= removed;
            }
        }
    }
}

/* Port model of 0x004039a0 (GAME-002). */
void task_record_create_and_scan_grid(
    void* entity)
{
    void* record;
    /* FUN_004039a0 always creates the timed kind-5 record with
     * (payload, 0, -1, 0), then changes its command kind to 9. */
    record = timed_task_record_initialize_and_link(entity, 0, -1, 0);
    if (record != NULL) cmd_set_kind(record, 9);
    terrain_cell_record_counts_prune(entity);
}

/* Port model of 0x004039d0 (GAME-002). */
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
/* Port model of 0x00403a10 (GAME-002). */
void task_callback_dispatch(void* payload)
{
    entity_pool_callback_search(task_callback_entity_adapter, payload);
}

/* Port model of 0x00403a50 (GAME-002). */
void entity_drain_command_chain(EntitySlot* e)
{
    if (e == NULL) {
        return; /* deviation: the corpus reads the entity blindly */
    }
    if ((s_global_enabled & 2) != 0) {
        return; /* the corpus gate: DAT_004b8060 bit 1 blocks the drain */
    }
    CmdRec* cur = as_rec(e->cmd_head);
    if (cur != NULL) cmd_gettool_record_release(cur);
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
/* Port model of 0x00403af0 (GAME-002). */
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
/* Port model of 0x00403b40 (GAME-002). */
void entity_pending_task_flag_clear(EntitySlot* e)
{
    if (e != NULL && e->cmd_head != NULL) {
        as_rec(e->cmd_head)->state &= ~1u;
    }
}

/* Port model of 0x00403b60 (GAME-002). */
void entity_activity_records_refresh(void* entity)
{
#if !defined(_MSC_VER) || !defined(_M_IX86)
    if (entity == NULL) return;
#endif
    uint32_t flags = 0;
    uint32_t count = 0;
    __builtin_memcpy(&flags, (char*)entity + 0x3e0u, sizeof(flags));
    __builtin_memcpy(&count, (char*)entity + 0x324u, sizeof(count));
    if ((flags & 0x400u) == 0u || (flags & 0x2000u) != 0u
        || count == 0u) return;
    {
        uint32_t i = 0;
        do {
            void* value = NULL;
            __builtin_memcpy(&value, (char*)entity + 0x308u
                             + (size_t)i * sizeof(void*), sizeof(value));
            entity_activity_record_remove_by_type(entity, 5u, value);
            entity_activity_record_remove_by_type(entity, 6u, value);
            entity_activity_record_remove_by_type(entity, 9u, value);
            entity_activity_record_remove_by_type(entity, 15u, value);
            entity_activity_record_remove_by_type(entity, 7u, value);
            entity_activity_record_remove_by_type(entity, 4u, NULL);
            entity_activity_record_remove_by_type(entity, 20u, value);
            ++i;
        } while (i < count);
    }
}
/* The raw record model retains the corpus +0x2f0 head and +0x60 next-link
 * offsets. */
#if defined(_MSC_VER) && defined(_M_IX86)
/* Port model of 0x00403c00 (GAME-002). */
__declspec(naked) void entity_activity_record_remove_by_type(
    void* entity, int type, void* action_context)
{
    __asm {
        mov al, byte ptr [s_global_enabled]
        push ebx
        push ebp
        push esi
        test al, 2
        push edi
        jne removed_head_return
        mov ebp, dword ptr [esp + 14h]
        xor ebx, ebx
        mov esi, dword ptr [ebp + 2f0h]
        test esi, esi
        je removed_head_return
        mov eax, dword ptr [esp + 18h]
    find_record:
        cmp eax, dword ptr [esi]
        je found_record
        mov ebx, esi
        mov esi, dword ptr [esi + 60h]
        test esi, esi
        jne find_record
    not_found_return:
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    found_record:
        mov eax, dword ptr [esi]
        cmp eax, 5
        je task_kind_gate
        cmp eax, 7
        je issue_kind_seven
        cmp eax, 6
        je task_kind_gate
        cmp eax, 3
        je task_kind_gate
        cmp eax, 9
        je task_kind_gate
        cmp eax, 0fh
        je task_kind_gate
        cmp eax, 14h
        jne unlink_record
    task_kind_gate:
        cmp eax, 7
        jne after_kind_seven
    issue_kind_seven:
        mov eax, dword ptr [esi + 5ch]
        mov edx, dword ptr [esi + 2ch]
        and eax, 800h
        lea ecx, [esi + 30h]
        push eax
        push ecx
        mov ecx, dword ptr [esi + 28h]
        lea eax, [esi + 8]
        push edx
        mov edx, dword ptr [esi + 24h]
        push eax
        mov eax, dword ptr [esi + 20h]
        push ecx
        push edx
        push eax
        call cmd_command_kind_record_issue
        add esp, 1ch
    after_kind_seven:
        cmp dword ptr [esi], 9
        jne create_task
        lea edi, [esi + 8]
        push 0
        push edi
        call level_tile_mark_current_target
        mov ecx, dword ptr [esi + 0ch]
        mov edx, dword ptr [edi]
        add esp, 8
        push ecx
        push edx
        call level_current_terrain_get
        push eax
        call terrain_map_block_highlight_clear
        add esp, 0ch
    create_task:
        mov edi, dword ptr [esp + 1ch]
        push 43160000h
        push edi
        call cmd_create_task
        add esp, 8
        push edi
        call cmd_minion_companion
        add esp, 4
    unlink_record:
        test ebx, ebx
        je unlink_head
        mov eax, dword ptr [esi + 60h]
        push 0
        push esi
        mov dword ptr [ebx + 60h], eax
        call cmd_free
        add esp, 8
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    unlink_head:
        mov ecx, dword ptr [esi + 60h]
        push 0
        push esi
        mov dword ptr [ebp + 2f0h], ecx
        call cmd_free
        add esp, 8
    removed_head_return:
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    }
}
#else
void entity_activity_record_remove_by_type(void* entity, int type,
                                           void* action_context)
{
    if (entity == NULL) return;
    if ((s_global_enabled & 2) != 0) return;
    void* current = NULL;
    void* previous = NULL;
    __builtin_memcpy(&current, (char*)entity + 0x2f0u, sizeof(current));
    while (current != NULL) {
        uint32_t record_type = 0;
        void* next = NULL;
        __builtin_memcpy(&record_type, current, sizeof(record_type));
        __builtin_memcpy(&next, (char*)current + 0x60u, sizeof(next));
        if (record_type == (uint32_t)type) {
            if (record_type == 7u) {
                const uint32_t* words = (const uint32_t*)current;
                cmd_command_kind_record_issue(
                    words[8], words[9], words[10], (const int*)&words[2],
                    words[11], (const float*)&words[12],
                    (words[23] & 0x800u) != 0u);
            }
            if (record_type == 9u) {
                const int* cell_xy = (const int*)((char*)current + 0x08u);
                level_tile_mark_current_target(cell_xy, 0);
                terrain_cell_surface_type_reset(
                    level_current_terrain_get(cell_xy[0], cell_xy[1]),
                    cell_xy[0], cell_xy[1]);
            }
            if ((record_type == 3u || record_type == 5u
                 || record_type == 6u || record_type == 7u
                 || record_type == 9u || record_type == 15u
                 || record_type == 20u) && action_context != NULL) {
                cmd_create_task((EntitySlot*)action_context, 0x43160000);
                cmd_minion_companion((EntitySlot*)action_context);
            }
            if (previous == NULL) {
                __builtin_memcpy((char*)entity + 0x2f0u, &next,
                                 sizeof(next));
            } else {
                __builtin_memcpy((char*)previous + 0x60u, &next,
                                 sizeof(next));
            }
            cmd_free(current, 0);
            return;
        }
        previous = current;
        current = next;
    }
}
#endif

/* Port model of 0x00403d10 (GAME-002). */
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
                cmd_gettool_record_release(cur);
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

/* Port model of 0x00403ef0 (GAME-002). */
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

/* FUN_00403f30: when the chain head is absent or is not kind 0xe, allocate
 * an idle record, link the former head through +0x60, and install it. */
/* Port model of 0x00403F30 (GAME-002). */
void ai_idle_chain_record_attach(EntitySlot* entity)
{
    if (entity->cmd_head == NULL || cmd_rec_type(entity->cmd_head) != 0xe) {
        void* rec = cmd_alloc(0xe);
        if (rec != NULL) {
            cmd_rec_set_next(rec, entity->cmd_head);
            entity->cmd_head = rec;
        }
    }
}

/* Port model of 0x00403f60 (GAME-002). */
void task_record_kind14_remove_if_head(EntitySlot* e)
{
    if (e != NULL && e->cmd_head != NULL
        && as_rec(e->cmd_head)->type == 0xe) {
        entity_command_chain_head_remove(e);
    }
}
/* Port model of 0x00404030 (GAME-002). */
void cmd_link_head(CmdRec* rec, int kind)
{
    rec->next = s_global_head;
    s_global_head = rec;
    rec->now = 1500.0f;
    rec->state = (rec->state & ~1u) | CMD_STATE_PENDING;
    cmd_bind_kind(rec, kind);
}

/* Port model of 0x00404070 (GAME-002). */
static void cmd_link_tail(CmdRec* rec)
{
    rec->next = s_global_tail;
    s_global_tail = rec;
    rec->now = 1500.0f;
    rec->state &= ~(CMD_STATE_PENDING | 0x10u);
}

// EntityTaskQueueFlush (FUN_004040a0).
/* Port model of 0x004040a0 (GAME-002). */
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

/* Port model of 0x00404100 (GAME-002). */
void command_queue_sweep_relink(EntitySlot* e)
{
    if (e == NULL) {
        return; /* deviation: the corpus reads the entity blindly */
    }
    CmdRec* keep = NULL;
    CmdRec* cur = as_rec(e->cmd_head);
    if (cur != NULL) cmd_gettool_record_release(cur);
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
/* Port model of 0x00404190 (GAME-002). */
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

/* Port model of 0x004041b0 (GAME-002). */
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
/* Port model of 0x004041e0 (GAME-002). */
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
    cmd_gettool_record_release(head);
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
/* LAB_00404250 is a pool-walker callback: argument one is a CmdRec and
 * argument two points at the caller's frame delta. The retail pool walk
 * tests each record's +0x64 self-link; the port live-slot sidecar represents
 * that same allocated-record set. */
/* Port model of 0x00404250 (GAME-002). */
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

/* LAB_00404280 (called by CmdTickGlobal through FUN_004377b0): the pool
 * callback always returns zero so the filtered live-entity walk visits every
 * slot.  The recovered outer gate and the chain-head expiry transitions are
 * represented here.  Record kinds with renderer/task-specific dispatch stay
 * in their own entity helpers; this callback does not substitute ai_tick. */
/* Port model of 0x00404280 (GAME-002). */
int cmd_entity_command_chain_tick(EntitySlot* entity, void* delta_payload)
{
    const float delta = *(const float*)delta_payload;
    CmdRec* head;
    LevelState* level;
    float actor_position[2];
    float target_position[2];
    int actor_cell[2];
    int target_cell[2];
    int gate_reason = 0;

    if (entity == NULL) return 0;
    s_global_enabled |= 4;

    /* Retail requires the active-type bit, a positive +0x340 value, and a
     * false 0x4295c0 predicate; attached, owned type-2, and the two flag
     * families below are excluded before touching the command chain. */
    if ((entity->flags_3e8 & 0x00400000u) == 0u) gate_reason = 1;
    else if (entity->field_340 <= 0.0f) gate_reason = 2;
    else if (entity_matches_current_selection_or_owner(entity) != 0) gate_reason = 3;
    else if (entity->owner_36c != NULL && entity->type == 2) gate_reason = 4;
    else if (entity->attach_304 != NULL) gate_reason = 5;
    else if ((entity->flags_3e4 & 0x08004000u) != 0u) gate_reason = 6;
    else if ((entity->flags_3e0 & 0x00800000u) != 0u) gate_reason = 7;

    if (gate_reason != 0) {
        s_global_enabled &= ~4;
        return 0;
    }

    head = as_rec(entity->cmd_head);
    if (head == NULL) {
        entity->command_chain_idle_elapsed_338 += delta;
        /* Native0x40490b-0x404995 obtains the current cell before testing
         * accumulated idle time. Equal or unordered operands skip0x443710;
         * ordinary subsequent idle ticks use the real terrain-selected cell.
         * This is the general no-head continuation, not a tutorial route. */
        {
            int current_cell[2] = { 0, 0 };
            int candidate_cell[2] = { 0, 0 };
            GameContext* context = pending_scan_context_get();
            LevelState* current_level = entity->owner_ctx != NULL
                ? entity->owner_ctx->level
                : (context != NULL ? context->level : NULL);
            entity_position_to_terrain_cell(entity, current_level,
                &current_cell[0], &current_cell[1]);
            /* VC5's ordered-less comparison uses x87 C0 alone, which also
             * admits unordered operands. Reject IEEE NaN words explicitly
             * before the native C3 inequality, retaining infinities. */
            uint32_t idle_word;
            uint32_t delta_word;
            memcpy(&idle_word, &entity->command_chain_idle_elapsed_338,
                   sizeof(idle_word));
            memcpy(&delta_word, &delta, sizeof(delta_word));
            if ((idle_word & 0x7fffffffu) <= 0x7f800000u &&
                (delta_word & 0x7fffffffu) <= 0x7f800000u &&
                entity->command_chain_idle_elapsed_338 != delta &&
                (entity->flags_3ec & 0xc0u) == 0u &&
                unported_ledger_stubs::
                    ledger_stub_00443710_AdjacentPositionCandidateFind(
                        entity, current_cell, candidate_cell) != 0) {
                if (candidate_cell[0] != 0 && candidate_cell[1] != 0) {
                    CmdRec* move = (CmdRec*)cmd_alloc(0);
                    if (move != NULL) {
                        entity->cmd_head = move;
                        memcpy(&move->x, &candidate_cell[0], sizeof(int));
                        memcpy(&move->y, &candidate_cell[1], sizeof(int));
                    }
                }
                /* A successful search also exits when either output is0.
                 * Do not use cmd_create_move: native attaches an unissued
                 * allocation directly without its sweep/registry effects. */
                s_global_enabled &= ~4;
                return 0;
            }
        }
        if (entity->command_chain_idle_elapsed_338 > 250.0f) {
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
                if (assignment == 0 || assignment == 2) {
                    unsigned before = s_pending_assignment_count;
                    cmd_entity_pending_candidates_append(entity);
                }
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
            /* ai_tick runs before this scheduler. Share its admission owner
             * and retain the record until world motion actually arrives.
             * A failed/busy route stays pending rather than being consumed. */
            (void)cmd_move_route_prepare(entity, level);
            s_global_enabled &= ~4;
            return 0;
        }
        else if ((head->type == 5 || head->type == 8) &&
                 ((head->state & CMD_STATE_ISSUED) != 0u ||
                  (entity->flags_3e8 & 0x200000u) != 0u)) {
            /* Native40450f/404518 precede both task dispatches. Once issued,
             * their entity callbacks own the existing route/work sequence;
             * reissuing here would reset it on every frame. */
            s_global_enabled &= ~4;
            return 0;
        }
        else if (head->type == 19) {
            /* Native40450f/404518: issued and actor-busy gates. Bit1 is an
             * issuance latch, not arrival. AI alone advances this route. */
            if (cmd_gettool_route_arrived(entity) ||
                cmd_gettool_route_moving(entity) ||
                (head->state & CMD_STATE_ISSUED) != 0u ||
                (entity->flags_3e8 & 0x200000u) != 0u) {
                s_global_enabled &= ~4;
                return 0;
            }
            result = cmd_entity_chain_kind_dispatch(entity, head);
            if (result > 0 && !cmd_gettool_route_adopt(entity, head)) result = 0;
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
/* ObjectPathSubmitToGridTarget (FUN_00404e00): a target already on the
 * actor's cell is submitted as a one-point path; otherwise resolve the
 * cardinal route and pass its points to the entity route installer. */
/* Port model of 0x00404e00 (GAME-002). */
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
    float world_position[2];
    int installed;
    uint32_t i;
    if (entity == NULL || level == NULL || target_xy == NULL ||
        level->ptr_terrain == NULL) return 0;
    entity_transform_position_get(entity, world_position);
    if (!terrain_world_xy_to_cell(level->ptr_terrain,
            world_position[0], world_position[1], &start_xy[0], &start_xy[1]) ||
        !level_coord_pair_within_bounds(level, target_xy[0], target_xy[1]))
        return 0;
    if (start_xy[0] == target_xy[0] && start_xy[1] == target_xy[1]) {
        direct_x = (float)start_xy[0];
        direct_y = (float)start_xy[1];
        return entity_route_setup_and_marker_associate(
            entity, 1u, &direct_x, &direct_y, final_marker_position);
    }
    if (!terrain_neighbor_path_check(entity,
            (uint32_t)start_xy[0], (uint32_t)start_xy[1],
            (uint32_t)target_xy[0], (uint32_t)target_xy[1],
            &route_x, &route_y, &route_count, NULL, NULL)) {
        free(route_x);
        free(route_y);
        return 0;
    }
    /* Distinct-cell routes must contain BOTH endpoints. Reject malformed
     * authority output before it can become an admitted/finishable command. */
    if (route_x == NULL || route_y == NULL ||
        route_count < 2u || route_count > 4096u ||
        route_x[0] != start_xy[0] || route_y[0] != start_xy[1] ||
        route_x[route_count - 1u] != target_xy[0] ||
        route_y[route_count - 1u] != target_xy[1]) {
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
    installed = entity_route_setup_and_marker_associate(
        entity, route_count, points_x, points_y, final_marker_position);
    free(points_x);
    free(points_y);
    free(route_x);
    free(route_y);
    return installed;
}

namespace unported_ledger_stubs {
/* Port model of 0x00404ed0 (GAME-002). */
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
/* Port model of 0x00404F10 (GAME-002). */
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
} // namespace unported_ledger_stubs

/* Port model of 0x00404f50 (GAME-002). */
int object_filter_matches_action_target(const void* filter_rec,
                                        const void* target_rec)
{
    if (filter_rec == NULL || target_rec == NULL) {
        return 0; /* deviation: the corpus strides both records blindly */
    }
    const uint32_t type = filt_dword(filter_rec, 0x00);
    if (type != 5u) {
        if (type != 7u && type != 9u && type != 0xfu) {
            return 0;
        }
        uintptr_t handle = 0;
        __builtin_memcpy(&handle, (const char*)filter_rec + 0x10,
                         sizeof handle);
        if (handle != 0
            && entity_shared_eligibility_check((const EntitySlot*)handle, 0)
                   == 0) {
            return 0;
        }
    }
    if (filt_dword(filter_rec, 0x20) != 0u) {
        const uint32_t selector = filt_dword(filter_rec, 0x24);
        if (filt_dword(target_rec, 0x00) != filt_dword(filter_rec, 0x20)
            || (selector != 0xFFFFFFFFu
                && filt_dword(target_rec, 0x04) != selector)
            || filt_dword(target_rec, 0x08) != filt_dword(filter_rec, 0x28)) {
            return 0;
        }
    }
    return 1;
}

namespace unported_ledger_stubs {
/* Port model of 0x00404fc0 (GAME-002). */
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
            ? block_get_float(context->cfg, "Main::MinEnergyForEat", 0.0f)
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
            ai_activity_cap_for_context(context, entity) == 0 ||
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
        /* FUN_00404fc0 calls FUN_0044b4c0 only in the 8/0/0xb/0xc
         * command group and in the separate kind-10 arm. Kind 0x16 has an
         * unconditional FUN_00432550 coordinate gate instead. */
        if (require_command_gate &&
            (kind == 8 || kind == 0xb || kind == 0xc || kind == 10) &&
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
        /* Kind 10 reaches FUN_004424f0 only inside the target-position
         * output branch: the reference leaves param_4 untouched when
         * param_3 is null. Other coordinate kinds write param_4 directly. */
        if (world_xy != NULL && (kind != 10 || screen_xy != NULL)) {
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
            !ops->entity_target_relation(ops->context, entity, target))
            return 0;
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
        /* Both output writes are nested under the retail param_3 test;
         * param_4 remains untouched when only that output is requested. */
        if (screen_xy != NULL) {
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
} // namespace unported_ledger_stubs

/* Port model of 0x00405950 (GAME-002). */
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
        CmdRec* next = as_rec(record->next);
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
                        else previous->next = next;
                        record->next = NULL;
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

/* Port model of 0x00405c10 (GAME-002). */
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
        CmdRec* next = as_rec(record->next);
        const int command_admitted = (*(float*)&record->param2 <= 0.0f) &&
            (record->state & 0x80400u) == 0u;
        if (command_admitted) {
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
                    /* Kind 0x11's placement predicate returns its eligibility
                     * directly after checking target eligibility, minifigure
                     * type, energy, and task flags; unlike the default arm it
                     * does not set the optional immediate-result word. */
                    if (eligibility_immediate == 0u && record->type != 0x11)
                        continue;
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
                    if (record->entity_ref == NULL) {
                        message_cell_xy[0] = target_cell_x;
                        message_cell_xy[1] = target_cell_y;
                        cmd_produce(record->type == 0x0c ? 0x27 :
                                    record->type == 0x15 ? 0x38 : 0x30,
                                    selected, (int)(uintptr_t)record,
                                    message_cell_xy);
                        if ((record->state & 0x100u) == 0u)
                            record->entity_ref = selected;
                        if (record->type == 0x0c && selected->type == 1) {
                            coordinate_callback_dispatch_by_mode(&record->x, 1);
                            record->param2 = 0;
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

/* EntityRecordAttachAndRelink (FUN_00406360) — see cmd.hpp. The record's
 * global-list unlink and its entity-chain link share the one +0x60 word,
 * as in the original (finding 570). Deviations: NULL records /
 * entities guard where the corpus strides blindly, and the +0x18
 * task-param word takes the corpus's float-bit constant raw. */
/* Port model of 0x00406360 (GAME-002). */
void entity_record_attach_and_relink(void* record, void* previous,
                                    EntitySlot* entity)
{
    CmdRec* rec = (CmdRec*)record;
    if (rec == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    if (entity != NULL && entity->cmd_head != NULL)
        cmd_gettool_record_release(as_rec(entity->cmd_head));
    if ((rec->state & 0x40u) == 0u) {
        if (previous == NULL) {
            s_global_head = as_rec(rec->next);
        } else {
            as_rec(previous)->next = rec->next;
        }
        rec->next = NULL;
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
/* PendingRecordEntityReferenceClear (FUN_004063e0) — see cmd.hpp. The
 * corpus strides the global list's +0x60 link and the +0x48 dword
 * blindly; the port rides its native-size link slot and reference slot
 * (documented deviations). */
/* Port model of 0x004063e0 (GAME-002). */
void pending_record_entity_reference_clear(void* reference)
{
    for (CmdRec* cur = s_global_head; cur != NULL;
         cur = as_rec(cur->next)) {
        if (cur->entity_ref == reference) {
            cur->entity_ref = NULL;
        }
    }
}

/* PendingRecordCopyToEntityAndFlag (FUN_00406400) — see cmd.hpp. The corpus
 * copies the 26-dword record window, then restores the allocator self-link at
 * +0x64, clears the entity-chain link at +0x60, and sets state bit 0x200.
 * The timer at +0x18 is part of the copied window and remains unchanged. */
/* Port model of 0x00406400 (GAME-002). */
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
    memset(&dst->gettool, 0, sizeof(dst->gettool));
    if (dst->type == 19) dst->state &= ~CMD_STATE_ISSUED;
    dst->next = NULL;        /* corpus +0x60 = 0: on no chain or list */
    dst->state |= 0x200u;    /* corpus +0x17 |= 0x200 */
    return dst;
}
/* Port model of 0x00406440 (GAME-002). */
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

/* Port model of 0x00406480 (GAME-002). */
void cmd_free(void* rec, int cancel)
{
    /* corpus: no NULL-record path — the gate is the second argument
     * (test [esp+8] / jne) and the record is dereferenced blindly. */
    cmd_gettool_record_release((CmdRec*)rec);
    if (cancel == 0) {
        cmd_cancel_ref(rec); /* corpus: unless the argument suppresses it */
    }
    /* Keep the corpus's direct record dereference here; routing through
     * as_rec adds a call in this hot pool leaf. */
    CmdRec* r = (CmdRec*)rec;
    cmd_gettool_reference_release(r);
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
/* Port model of 0x004064c0 (GAME-002). */
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

/* 0x004065d0 renderer_global_dword_get and 0x004065e0
 * render_surface_secondary_get are defined in aitask.c (C linkage). */

static int cmd_entity_chain_kind_dispatch(EntitySlot* entity, CmdRec* record)
{
    int cell[2];
    /* 0x4047bc..c7 forwards the Drill record's raw integer words. Its
     * constructor does not store floating-point coordinates in these slots. */
    if (record->type == 8)
        (void)cmd_rec_cell_xy_get(record, &cell[0], &cell[1]);
    else {
        cell[0] = (int)record->x;
        cell[1] = (int)record->y;
    }
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
        /* Numeric selector, not a task pointer. The real leaf keeps the
         * ToolNull-derived world marker; an absent scene producer pends. */
        result = s_cmd_entity_chain_external_ops.object_position_step != NULL
            ? s_cmd_entity_chain_external_ops.object_position_step(
                entity, (uint32_t)(uintptr_t)record->arg38,
                s_cmd_entity_chain_external_ops.context)
            : unported_ledger_stubs::
                ledger_stub_0044b500_ObjectPositionStepCommandIssueAndLink(
                    entity, (uint32_t)(uintptr_t)record->arg38);
        if (result >= 0) entity_reset_auxiliary_state(entity); /* 0x407370 */
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

static int cmd_place_target_nested_scene_pose_get(const EntitySlot* entity,
                                                  float position[3])
{
    const EntitySlot* nested;
    void* scene_object;
    if (entity == NULL || position == NULL ||
        (entity->flags_3ec & 0x40u) == 0u || entity->link_2fc == NULL)
        return 0;
    nested = entity->link_2fc;
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
    /* The assignment loop accepts entities for which FUN_00452610(entity,0)
     * is false, i.e. entities not already marked selected in +0x3e8. */
    return (entity->flags_3e8 & 0x200000u) == 0u;
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
    int carry_allowed;
    int goal[2];

    if ((record->state & 0x400u) == 0u) return 0;
    carry_allowed = entity_flag400_and_health_threshold_check(entity);
    if (carry_allowed != 0 &&
        (entity->flags_3e8 & 0x200000u) == 0u && entity->type != 3) {
        if (carried != NULL &&
            (carried->flags_3e8 & 0x80000000u) != 0u) {
            if (record->type == 0x1b) {
                entity_record_attach_and_relink(record, previous, entity);
                return 1;
            }
        }
        else if (carried != NULL &&
                 (record->cell_act == 0 ||
                  (carried->indexed_record_slot_330 == record->cell_count &&
                   (record->cell_target == -1 ||
                    carried->etype == record->cell_target) &&
                   carried->type == record->cell_act))) {
            switch (record->type) {
            case 7:
            case 9:
            case 15:
            case 20:
                route_lane = 1;
                break;
            case 5:
                /* 404be3: a null-target kind5 uses the cell route lane. */
                if (record->payload == NULL) {
                    route_lane = 1;
                }
                else {
                    int target_allowed = entity_shared_eligibility_check(
                        (EntitySlot*)record->payload, 0);
                    if (target_allowed != 0) {
                        int accepted = entity_deposit_activity_gate(
                            entity, (EntitySlot*)record->payload);
                        if (accepted != 0) {
                            entity_record_attach_and_relink(record, previous,
                                                            entity);
                            return 1;
                        }
                        route_failed = 1;
                    }
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
            else {
                /* 404cd3..404cff frees the search buffers, then joins the
                 * common attach owner; path validation alone is not work. */
                entity_record_attach_and_relink(record, previous, entity);
                return 1;
            }
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
        CmdRec* next = as_rec(record->next);
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

static void cmd_entity_pending_candidates_append(EntitySlot* entity)
{
    if (entity == NULL) return;
    if (entity_flag400_and_health_threshold_check(entity) != 0) {
        return;
    }
    if (s_pending_assignment_count < PENDING_CANDIDATE_CAPACITY) {
        s_pending_assignment_candidates[s_pending_assignment_count++] = entity;
    }
    if (entity->type == 3 &&
        (entity->flags_3e8 & 0x80000000u) == 0u &&
        s_pending_timer_count < PENDING_CANDIDATE_CAPACITY) {
        s_pending_timer_candidates[s_pending_timer_count++] = entity;
    }
}

static int cmd_gettool_route_adopt(EntitySlot* entity, CmdRec* head)
{
    CmdGetToolRoute* route = &head->gettool;
    if (entity->wp_count <= 0 || entity->wp_count > 4096 ||
        (entity->wp_dynamic == NULL ? entity->wp_count > AI_WAYPOINTS_MAX :
            (unsigned int)entity->wp_count > entity->wp_capacity) ||
        ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind != 0x0du ||
        !entity_route_waypoint_world_get(entity,
            (unsigned int)(entity->wp_count - 1), route->endpoint)) return 0;
    route->actor = entity;
    route->store = entity->link_2fc;
    route->waypoints = ENTITY_WAYPOINTS(entity);
    route->count = entity->wp_count;
    route->phase = 1;
    if (!cmd_gettool_store_live(route)) {
        cmd_gettool_record_release(head);
        return 0;
    }
    return 1;
}

/* The reciprocal +0x58 pair may outlive either pool record. Only clear a
 * still-live matching partner; never dereference an arbitrary raw reference. */
static void cmd_gettool_reference_release(CmdRec* record)
{
    int block_index;
    if (record->ref == NULL) return;
    for (block_index = 0; block_index < s_pool_level; ++block_index) {
        CmdRec* block = s_blocks[block_index];
        uint32_t index;
        const uint32_t count = 1u << (block_index & 0x1fu);
        if (block == NULL || s_block_live[block_index] == NULL) continue;
        for (index = 0u; index < count; ++index) {
            CmdRec* peer = &block[index];
            if (s_block_live[block_index][index] != 0u &&
                peer == record->ref && peer->ref == record &&
                (record->type == 19 || peer->type == 19)) peer->ref = NULL;
        }
    }
    if (record->type == 19) record->ref = NULL;
}

static void cmd_bind_kind(CmdRec* rec, int kind)
{
    if ((rec->state & 0x30000u) == 0) {
        rec->behaviour = cmd_kind_behaviour(kind);
    } else {
        rec->behaviour = 99;
    }
    rec->kind = kind;
}

static void cmd_chain_append(EntitySlot* e, CmdRec* rec, int as_move)
{
    CmdRec* tail = as_rec(cmd_chain_walk(e));
    if (tail != NULL) {
        tail->next = rec;
    } else {
        e->cmd_head = rec;
    }
    rec->next = NULL; /* entity-chain tail: never re-link onto the global list */
    if (as_move && e->type == 2) {
        idle_register(e); /* IdleQueueRegister for fresh move commands */
    }
}

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

/* LAB_004021f0, passed to FUN_00406540 by CmdFlushAll: the pool walk visits
 * each allocated command record, frees it with cancel=1, and returns zero so
 * enumeration continues. The live bitmap is the port's representation of
 * the retail record's +0x64 self-pointer test. */
static void cmd_pool_records_release_all(void)
{
    int block_index;
    for (block_index = 0; block_index < s_pool_level; ++block_index) {
        CmdRec* block = s_blocks[block_index];
        uint8_t* live = s_block_live[block_index];
        uint32_t count = 1u << (block_index & 0x1fu);
        uint32_t index;
        if (block == NULL || live == NULL) continue;
        for (index = 0u; index < count; ++index) {
            if (live[index] != 0u) {
                cmd_free(&block[index], 1);
            }
        }
    }
}

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

static int cmd_has_entity_task_record(const EntitySlot* entity)
{
    CmdRec* record;
    for (record = s_global_head; record != NULL;
         record = as_rec(record->next)) {
        if (record->type == 3 &&
            (const void*)record->payload == (const void*)entity) return 1;
    }
    for (record = s_global_tail; record != NULL;
         record = as_rec(record->next)) {
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

/* --- the task-record queue producers (see cmd.hpp) ---------------------------
 * The pair staging uses the corpus's int view of the coordinate slots. */
static void cmd_pair_stage(CmdRec* rec, const int pair[2])
{
    if (pair != NULL) {
        __builtin_memcpy(&rec->x, &pair[0], sizeof(rec->x));
        __builtin_memcpy(&rec->y, &pair[1], sizeof(rec->y));
    }
}

/* --- entity command-chain housekeeping (see cmd.hpp) --------------------------
 * DAT_004b3354 and DAT_004b3358 are separate roots that each use +0x60. */
static void cmd_raw_head_insert(CmdRec* rec)
{
    rec->next = s_global_head;
    s_global_head = rec;
}
static void cmd_raw_tail_insert(CmdRec* rec)
{
    rec->next = s_global_tail;
    s_global_tail = rec;
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
} // namespace unported_ledger_stubs

/* ObjectFilterMatchesActionTarget (FUN_00404f50) — see entity.hpp. The
 * identity pair (+0x20/+0x24/+0x28) stays dword; only the +0x10 entity
 * handle widens for the host pointer it dereferences. */
static uint32_t filt_dword(const void* rec, size_t off)
{
    uint32_t v = 0;
    if (rec != NULL) {
        __builtin_memcpy(&v, (const char*)rec + off, sizeof v);
    }
    return v;
}

/* Original-ABI adapters (ADR 0005 step 6) for GAME-021's lego_unit.c. */

/* FUN_00402510: the original takes the block position as a Point2I; the
 * port's entry takes x and y. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (00402510 -> terrain_cell_command_create)
extern "C" void terrain_cell_command_create_adapter(const int* blockPos, int flag,
                                                    int raise20)
{
    terrain_cell_command_create(blockPos[0], blockPos[1], flag, raise20);
}

/* FUN_00402000: the original's second argument is an object pointer; the
 * port's entry takes it as a word. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (00402000 -> cmd_mark_matching)
extern "C" void cmd_mark_matching_adapter(int type, struct EntitySlot* object,
                                          const int* coords)
{
    cmd_mark_matching(type, (int)(uintptr_t)object, coords);
}

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: the
 * original FUN_00402e40 takes the block position by pointer; the port's
 * entry above takes the two coordinates. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (cell_command_type13_issue 00402e40)
extern "C" void cell_command_type13_issue_original(const int* blockPos)
{
    cell_command_type13_issue(blockPos[0], blockPos[1]);
}

/* ---- GAME-002 group D adapters (ADR 0005 step 6) ----
 * Original-ABI entries for aitask.c's 0x00404fc0. Natively nothing calls
 * them: the port tail keeps its own eligibility check above. */
namespace unported_ledger_stubs {
int ledger_stub_0044b4c0_TypeTwoEntityTargetResolveAndApply(
    EntitySlot* entity, uint32_t code);
}

/* FUN_0043a5f0 (LegoObject_GetActivityContainer-style): the original returns
 * the object's container; the port's entry
 * (entity_visual_resource_get_or_create) returns nothing, and the port has
 * no original container. */
// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (0043a5f0 -> no port container)
extern "C" void* aitask_object_container_get_adapter(struct EntitySlot* object)
{
    (void)object;
    return NULL;
}

/* FUN_0044b4c0: the port's definition has C++ linkage, in a namespace, and
 * no header declaration that legoobject_port.cpp sees. */
// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (0044b4c0 -> ledger_stub_0044b4c0_TypeTwoEntityTargetResolveAndApply)
extern "C" int aitask_object_command_gate_adapter(struct EntitySlot* object, int type)
{
    return unported_ledger_stubs::
        ledger_stub_0044b4c0_TypeTwoEntityTargetResolveAndApply(
            object, (uint32_t)type);
}

/* FUN_00432550: the original takes the block position (and ignores it); the
 * port's entry takes no argument. */
// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (00432550 -> always_accept_place_command_cell)
extern "C" int aitask_place_command_cell_accept_adapter(int* blockPos)
{
    (void)blockPos;
    return always_accept_place_command_cell();
}

/* ---- GAME-002 group E adapters (ADR 0005 step 6) ----
 * Original-ABI entries for aitask.c's 0x00405c10. Natively nothing calls
 * them: the port tail keeps its own assignment tick above. */
/* FUN_00440c30: the original returns whether a route exists; the port's
 * entry is a void namespaced ledger stub, so call the route check it
 * forwards to. */
// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (00440c30 -> terrain_neighbor_path_check)
extern "C" int aitask_grid_route_search_adapter(struct EntitySlot* object,
                                                unsigned int startX,
                                                unsigned int startY,
                                                unsigned int goalX,
                                                unsigned int goalY,
                                                int** outX, int** outY,
                                                unsigned int* outCount)
{
    return terrain_neighbor_path_check(object, startX, startY, goalX, goalY,
                                       outX, outY, outCount, NULL, NULL);
}

/* FUN_00438e50: the port's entry is a namespaced ledger stub. */
// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (00438e50 -> ledger_stub_00438e50_SpatialObjectQueryCenterMask100)
extern "C" struct EntitySlot* aitask_object_find_trainer_adapter(
    struct EntitySlot* object, const float* center, unsigned int mask)
{
    return (struct EntitySlot*)unported_ledger_stubs::
        ledger_stub_00438e50_SpatialObjectQueryCenterMask100(object, center,
                                                             mask);
}

/* ---- GAME-002 group C adapters (ADR 0005 step 6) ----
 * Original-ABI entries for aitask.c's 0x00404280 (the per-entity command
 * chain tick), 0x00404e00 and 0x00404ed0. Natively nothing calls them: the
 * port tail keeps its own chain tick above. Most of the callees' port
 * definitions are namespaced ledger stubs in legoobject_port.cpp. */

/* FUN_0043a120: namespaced port entry; the original passes the command's
 * +0x30 position pair untyped. */
// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (0043a120 -> entity_cell_step_command_issue_with_jitter)
extern "C" int aitask_cell_step_command_issue_adapter(struct EntitySlot* object,
                                                      int* blockPos, void* wPos,
                                                      int actionMode,
                                                      int markPending)
{
    return unported_ledger_stubs::entity_cell_step_command_issue_with_jitter(
        object, blockPos, (const float*)wPos, actionMode, markPending);
}

/* FUN_00441700: the original takes the route as block coordinates; the
 * port's entry takes them as floats. */
// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (00441700 -> entity_route_setup_and_marker_associate)
extern "C" int aitask_route_setup_adapter(struct EntitySlot* object,
                                          unsigned int count, int* routeX,
                                          int* routeY, float* wPos)
{
    float* x = (float*)malloc((count != 0u ? count : 1u) * sizeof(float));
    float* y = (float*)malloc((count != 0u ? count : 1u) * sizeof(float));
    unsigned int i;
    int result = 0;
    if (x != NULL && y != NULL) {
        for (i = 0u; i < count; ++i) {
            x[i] = (float)routeX[i];
            y[i] = (float)routeY[i];
        }
        result = entity_route_setup_and_marker_associate(object, count, x, y,
                                                         wPos);
    }
    free(x);
    free(y);
    return result;
}

/* FUN_00443710: namespaced port entry without the original's fourth
 * argument. */
// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (00443710 -> ledger_stub_00443710_AdjacentPositionCandidateFind)
extern "C" int aitask_adjacent_position_find_adapter(struct EntitySlot* object,
                                                     int* blockPos,
                                                     int* outPos, int mode)
{
    (void)mode;
    return unported_ledger_stubs::
        ledger_stub_00443710_AdjacentPositionCandidateFind(object, blockPos,
                                                           outPos);
}

// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (00446e40 -> ledger_stub_00446e40_EntityNearestCellRouteCommit)
extern "C" int aitask_nearest_cell_route_commit_adapter(struct EntitySlot* object,
                                                        unsigned int x,
                                                        unsigned int y,
                                                        int mode)
{
    return unported_ledger_stubs::
        ledger_stub_00446e40_EntityNearestCellRouteCommit(object, x, y, mode);
}

// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (004470d0 -> lego::entity_conditional_type_five_spawn)
extern "C" int aitask_type_five_spawn_adapter(struct EntitySlot* object)
{
    return lego::entity_conditional_type_five_spawn(object);
}

// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (00448800 -> ledger_stub_00448800_EntityMoveToCellCoordinates)
extern "C" int aitask_move_to_cell_adapter(struct EntitySlot* object,
                                           int* blockPos)
{
    return unported_ledger_stubs::
        ledger_stub_00448800_EntityMoveToCellCoordinates(object, blockPos);
}

// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (00448880 -> ledger_stub_00448880_RecordGatedStepCommandIssue)
extern "C" int aitask_gated_step_record_adapter(struct EntitySlot* object,
                                                int* blockPos)
{
    return unported_ledger_stubs::
        ledger_stub_00448880_RecordGatedStepCommandIssue(object, blockPos);
}

// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (00448a60 -> ledger_stub_00448a60_EntityNeighborCellMoveCommandHandle)
extern "C" int aitask_neighbor_cell_move_adapter(struct EntitySlot* object,
                                                 int* blockPos)
{
    return unported_ledger_stubs::
        ledger_stub_00448a60_EntityNeighborCellMoveCommandHandle(object,
                                                                 blockPos);
}

namespace unported_ledger_stubs {
int ledger_stub_00448c90_CardinalStepCommandIssueAndLink(
    EntitySlot* entity, EntitySlot* target, int link_mode);
}

// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (00448c90 -> ledger_stub_00448c90_CardinalStepCommandIssueAndLink)
extern "C" int aitask_cardinal_step_link_adapter(struct EntitySlot* object,
                                                 struct EntitySlot* target,
                                                 int linkMode)
{
    return unported_ledger_stubs::
        ledger_stub_00448c90_CardinalStepCommandIssueAndLink(object, target,
                                                             linkMode);
}

// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (00448eb0 -> ledger_stub_00448eb0_ProximityStepCommandIssueWithJitter)
extern "C" int aitask_proximity_step_jitter_adapter(struct EntitySlot* object,
                                                    struct EntitySlot* target,
                                                    int marker)
{
    return unported_ledger_stubs::
        ledger_stub_00448eb0_ProximityStepCommandIssueWithJitter(object, target,
                                                                 marker);
}

// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (00449010 -> ledger_stub_00449010_EntityKind6RecordApproachRouteIssue)
extern "C" int aitask_kind6_approach_route_adapter(struct EntitySlot* object)
{
    return unported_ledger_stubs::
        ledger_stub_00449010_EntityKind6RecordApproachRouteIssue(object);
}

// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (004490a0 -> ledger_stub_004490a0_ProximityStepCommandIssueWithLink)
extern "C" int aitask_proximity_step_link_adapter(struct EntitySlot* object,
                                                  struct EntitySlot* target,
                                                  int mode, int setGlobalGate)
{
    return unported_ledger_stubs::
        ledger_stub_004490a0_ProximityStepCommandIssueWithLink(
            object, target, mode, setGlobalGate);
}

// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (00449240 -> ledger_stub_00449240_EntityWalkableTargetCellRouteIssue)
extern "C" int aitask_walkable_cell_route_adapter(struct EntitySlot* object,
                                                  int* blockPos)
{
    return unported_ledger_stubs::
        ledger_stub_00449240_EntityWalkableTargetCellRouteIssue(object,
                                                                blockPos);
}

// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (004492b0 -> ledger_stub_004492b0_CellStepCommandIssueWithTargetMark)
extern "C" int aitask_cell_step_mark_adapter(struct EntitySlot* object,
                                             int* blockPos)
{
    return unported_ledger_stubs::
        ledger_stub_004492b0_CellStepCommandIssueWithTargetMark(object,
                                                                blockPos);
}

// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (00449430 -> ledger_stub_00449430_EntityTargetCellRouteAndAssociate)
extern "C" int aitask_target_cell_route_associate_adapter(
    struct EntitySlot* object, struct EntitySlot* target)
{
    return unported_ledger_stubs::
        ledger_stub_00449430_EntityTargetCellRouteAndAssociate(object, target);
}

// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (00449520 -> ledger_stub_00449520_EntityTargetCellRouteMarker14)
extern "C" int aitask_target_cell_route_marker14_adapter(
    struct EntitySlot* object, struct EntitySlot* target)
{
    return unported_ledger_stubs::
        ledger_stub_00449520_EntityTargetCellRouteMarker14(object, target);
}

// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (00449610 -> ledger_stub_00449610_EntityCoordinateRouteAndMark)
extern "C" int aitask_coordinate_route_mark_adapter(struct EntitySlot* object,
                                                    int* blockPos)
{
    return unported_ledger_stubs::
        ledger_stub_00449610_EntityCoordinateRouteAndMark(object, blockPos);
}

// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (00449700 -> ledger_stub_00449700_RouteStepCommandIssue)
extern "C" int aitask_route_step_adapter(struct EntitySlot* object)
{
    return unported_ledger_stubs::ledger_stub_00449700_RouteStepCommandIssue(
        object);
}

// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (00449ac0 -> ledger_stub_00449ac0_ProximityStepCommandIssueClearingGate)
extern "C" int aitask_proximity_step_clearing_adapter(struct EntitySlot* object,
                                                      struct EntitySlot* target)
{
    return unported_ledger_stubs::
        ledger_stub_00449ac0_ProximityStepCommandIssueClearingGate(object,
                                                                   target);
}

// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (0044a3d0 -> type11_cell_coordinate_nearest_resolve)
extern "C" int aitask_type11_nearest_resolve_adapter(struct EntitySlot* object,
                                                     int* outPos)
{
    return unported_ledger_stubs::type11_cell_coordinate_nearest_resolve(object,
                                                                        outPos);
}

/* FUN_0044b500: the original passes the command's +0x38 word. */
// PORT-ONLY: original-ABI adapter for GAME-002 aitask.c (0044b500 -> ledger_stub_0044b500_ObjectPositionStepCommandIssueAndLink)
extern "C" int aitask_position_step_link_adapter(struct EntitySlot* object,
                                                 void* selector)
{
    return unported_ledger_stubs::
        ledger_stub_0044b500_ObjectPositionStepCommandIssueAndLink(
            object, (uint32_t)(uintptr_t)selector);
}
