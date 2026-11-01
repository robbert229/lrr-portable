// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "cmd.hpp"
#include "ai.hpp"
#include "select.hpp"
#include "level.hpp"
#include "terrain_grid.hpp"
#include "frontend.hpp"
#include "util.hpp"
#include "gods98/engine.hpp"
#include "entity.hpp"
#include "crt/crt.hpp"
#include "gods98/heap.hpp"
#include "gods98/media.hpp"
#include "gods98/message.hpp"
#include "gods98/sound.hpp"

namespace unported_ledger_stubs {
int entity_capacity_or_state_predicate(const EntitySlot* entity);
int ledger_stub_004470d0_EntityConditionalTypeFiveSpawn(EntitySlot* entity);
int ledger_stub_00447210_EntityAdjacentPlacementCellSearch(
    EntitySlot* entity, EntitySlot* target);
}

static CommandKindRecordHooks s_command_kind_record_hooks;
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
    void*    arg38;      /* +0x38 second payload slot (the 0x13 queue's word)   */
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

#define CMD_STRIDE sizeof(CmdRec)

static CmdRec* as_rec(void* p) { return (CmdRec*)p; }
static const CmdRec* cas_rec(const void* p) { return (const CmdRec*)p; }

int cmd_rec_type(const void* rec) { return cas_rec(rec)->type; }
const void* cmd_rec_payload(const void* rec) { return cas_rec(rec)->payload; }
int cmd_rec_kind(const void* rec) { return cas_rec(rec)->kind; }
int cmd_rec_behaviour(const void* rec) { return cas_rec(rec)->behaviour; }
int cmd_rec_activity(const void* rec) { return cas_rec(rec)->grant_act; }
void cmd_rec_set_activity(void* rec, int act) { as_rec(rec)->grant_act = act; }
uint32_t cmd_rec_state(const void* rec) { return cas_rec(rec)->state; }
void* cmd_rec_next(const void* rec) { return cas_rec(rec)->next; }

float cmd_rec_move_x(const void* rec) { return cas_rec(rec)->x; }
float cmd_rec_move_y(const void* rec) { return cas_rec(rec)->y; }
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

// GLOBAL: LEGORR 0x004b3354
static CmdRec* s_global_head = NULL; /* DAT_004b3354 */
static CmdRec* s_global_tail = NULL; /* DAT_004b3358: independent pending-list head */
static GameContext* s_ctx = NULL;

/* The original keeps two independent 50-entry candidate arrays: the
 * assignment pass at +0x335c and the timer pass at +0x3428. Their callback
 * bodies are anonymous, but the decompile shows the assignment walker is
 * unfiltered and the timer walker excludes +0x3e8 bit 0x80000. Rebuild each
 * view from the live typed pools and keep them separate because each pass
 * consumes entries with its own swap-with-last rule. */
#define PENDING_CANDIDATE_CAPACITY 50
static EntitySlot* s_pending_assignment_candidates[PENDING_CANDIDATE_CAPACITY];
static EntitySlot* s_pending_timer_candidates[PENDING_CANDIDATE_CAPACITY];
static unsigned s_pending_assignment_count;
static unsigned s_pending_timer_count;

static GameContext* pending_scan_context_get(void)
{
    return s_ctx != NULL ? s_ctx : entity_pool_callback_context_get();
}

static unsigned pending_candidate_pool_build(GameContext* context,
                                              EntitySlot** output,
                                              int filter_flagged)
{
    unsigned count = 0;
    int type;
    int index;
    if (context == NULL || output == NULL) return 0;
    /* Entity types 5..19 share one special-object table in GameContext. */
    for (type = 1; type <= 5 && count < PENDING_CANDIDATE_CAPACITY; ++type) {
        const int entity_count_for_type = entity_count(context, type);
        for (index = 0; index < entity_count_for_type &&
                            count < PENDING_CANDIDATE_CAPACITY; ++index) {
            EntitySlot* entity = (EntitySlot*)entity_at(context, type, index);
            if (entity != NULL && entity->type != 0 &&
                (filter_flagged == 0 ||
                 (entity->flags_3e8 & 0x80000u) == 0u))
                output[count++] = entity;
        }
    }
    {
        const unsigned live_count = count;
        while (count < PENDING_CANDIDATE_CAPACITY) output[count++] = NULL;
        count = live_count;
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
    if ((entity->flags_3e8 & 2u) == 0u) return 0;
    if ((unsigned)x >= (unsigned)level_width(level) - 2u ||
        (unsigned)y >= (unsigned)level_height(level) - 2u) return 0;
    cell = level_voxel(level, x, y);
    if (cell == NULL) return 0;
    type = (int)(signed char)cell[2];
    if (ai_stats_speed(context, type) == 0.0f) return 0;

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
    if (block == NULL) {
        return;
    }
    s_blocks[level] = block;
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

static void idle_register(EntitySlot* e);

/* FUN_00403d10 (generic drop): unlink and free every chain record whose
 * type (+0x00) equals `type`, mirroring the corpus loop over the entity's
 * +0xbc record list. Like the corpus, records with per-type side effects
 * (its types 7/0xc/0x17/3 branches) are not included here — the port only
 * needs the plain drop FUN_00403d10(ent, 0x13, 0, 1) performs on the
 * deferred-grant path. */
// FUNCTION: LEGORR 0x00403d10
int cmd_chain_drop(EntitySlot* e, int type)
{
    if (e == NULL || e->cmd_head == NULL) {
        return 0;
    }
    int dropped = 0;
    CmdRec* prev = NULL;
    CmdRec* cur = as_rec(e->cmd_head);
    while (cur != NULL) {
        CmdRec* next = as_rec(cur->next);
        if (cur->type == type) {
            cmd_global_unlink(cur);
            if (prev != NULL) {
                prev->next = next;
            } else {
                e->cmd_head = next;
            }
            cmd_free(cur, 0);
            ++dropped;
        } else {
            prev = cur;
        }
        cur = next;
    }
    return dropped;
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
/* PendingCommandTimerFireScan (FUN_00405950).  The decompile walks the
 * tail-linked list and the separate DAT_004b3428 candidate pool.  The pool
 * is reconstructed from the live typed entity tables because its original
 * anonymous callback has no port owner yet.  Preserve the observed timer,
 * per-kind flag, record unlink, entity-head install/copy, and swap-removal
 * effects for branches whose predicates are represented by existing APIs. */
void pending_command_timer_fire_scan(void)
{
    GameContext* context = pending_scan_context_get();
    CmdRec* previous = NULL;
    CmdRec* record = s_global_tail;
    unsigned count = pending_candidate_pool_build(context,
                                                   s_pending_timer_candidates,
                                                   1);
    s_pending_timer_count = count;
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
 * array (0x452650). Coordinate/target kinds 0, 8, 0xb, 0xc, 0x12, 0x16,
 * 0x17 and 10 use live entity and terrain fields; compatible candidates are
 * selected by distance and attached through the common transfer/copy helper.
 *
 * The cheat feature gate uses ai_can_do; kind 8 uses the full terrain and
 * entity gates from FUN_00432770. Kinds 3, 0x11, 0x13, 0x15, 0x1a, and
 * 0x1e still need target/config state that has no production owner. Kind
 * 0x12's scene-resource pose alternative is unavailable, while its direct
 * capability-0x20 and stats-flag position branches use live entity/level
 * state.
 * The Ops path requires raw entity views, an installed global feature word,
 * target/resource/visibility/route resolvers, scene position/ray operations,
 * and (for kind 0x11) the configured energy threshold. No production caller
 * binds ObjectTaskEligibilityOps. Kinds 8/0xb/10/0x17 validate distant
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
    unsigned count = pending_candidate_pool_build(
        context, s_pending_assignment_candidates, 0);
    s_pending_assignment_count = count;
    while (record != NULL) {
        CmdRec* next = as_rec(record->global_next);
        if ((*(float*)&record->param2 <= 0.0f) &&
            (record->state & 0x80400u) == 0u) {
            unsigned i;
            float nearest = 99999.0f;
            unsigned selected_index = s_pending_assignment_count;
            EntitySlot* selected = NULL;
            int target_cell_x, target_cell_y;
            int selected_cell_x = 0, selected_cell_y = 0;
            int selected_requires_route = 0;
            float target_world[2];
            int target_world_valid = 0;
            const int coordinate_kind = record->type == 0 ||
                                        record->type == 8 ||
                                        record->type == 0x0b ||
                                        record->type == 0x0c ||
                                        record->type == 0x16 ||
                                        record->type == 0x17 ||
                                        record->type == 10 ||
                                        record->type == 0x12;
            int has_direct_placement = 0;
            if (coordinate_kind && context != NULL && context->level != NULL) {
                if ((record->type == 0x17 || record->type == 10) &&
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
                    else if (entity_record_flag20_query(target, target_xy) &&
                             level_coord_pair_within_bounds(
                                 context->level, target_xy[0], target_xy[1])) {
                        /* FUN_00404fc0's kind-0x12 fallback calls
                         * FUN_00439e40 to derive an adjacent task cell. The
                         * scene-resource pose branch preceding it is not
                         * representable by EntitySlot, so retain the direct
                         * capability-0x20 path exposed by that helper. */
                        target_cell_x = target_xy[0];
                        target_cell_y = target_xy[1];
                        has_direct_placement = terrain_cell_world_center_xy_get(
                            context->level->ptr_terrain, target_cell_x,
                            target_cell_y, target_world);
                        target_world_valid = has_direct_placement;
                    }
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
                float position[2];
                int cell_x, cell_y;
                float dx, dy, distance;
                int requires_route = 0;
                if (!pending_record_matches_entity(record, entity) ||
                    ((entity_stats_word_get(entity) & 0x20000u) != 0u &&
                     entity->owner_36c == NULL) ||
                    (entity->flags_3e0 & 0x800000u) != 0u ||
                    (entity->flags_3ec & 0x400u) != 0u ||
                    ((context->cheat & 1u) != 0u &&
                     record->type != 0x1a && ai_can_do(entity) != 0))
                    continue;
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
                    record->type == 0x17) {
                    const int dx = cell_x - target_cell_x;
                    const int dy = cell_y - target_cell_y;
                    requires_route =
                        !(((dx == 1 || dx == -1) && dy == 0) ||
                          (dx == 0 && (dy == 1 || dy == -1)));
                }
                if (requires_route) {
                    int goal_xy[2];
                    int route_goal_xy[2];
                    int* path_x = NULL;
                    int* path_y = NULL;
                    uint32_t path_count = 0;
                    goal_xy[0] = target_cell_x;
                    goal_xy[1] = target_cell_y;
                    if (!first_free_neighbor_cell_pick(
                            entity, context->level, goal_xy,
                            route_goal_xy, 1))
                        continue;
                    if (!terrain_neighbor_path_check(
                            entity, (uint32_t)cell_x, (uint32_t)cell_y,
                            (uint32_t)route_goal_xy[0],
                            (uint32_t)route_goal_xy[1],
                            &path_x, &path_y, &path_count, NULL, NULL)) {
                        record->param2 = (int32_t)0x42960000u;
                        continue;
                    }
                    if (path_x != NULL) free(path_x);
                    if (path_y != NULL) free(path_y);
                }
                if (target_world_valid) {
                    dx = position[0] - target_world[0];
                    dy = position[1] - target_world[1];
                }
                else {
                    dx = (float)(cell_x - target_cell_x);
                    dy = (float)(cell_y - target_cell_y);
                }
                distance = (float)sqrt((double)(dx * dx + dy * dy));
                if (selected == NULL || distance < nearest) {
                    selected = entity;
                    selected_index = i;
                    nearest = distance;
                    selected_cell_x = cell_x;
                    selected_cell_y = cell_y;
                    selected_requires_route = requires_route;
                }
            }
            if (selected != NULL) {
                if (selected_requires_route) {
                    int goal_xy[2];
                    int route_xy[2];
                    int message_opcode = 0;
                    goal_xy[0] = target_cell_x;
                    goal_xy[1] = target_cell_y;
                    if (!first_free_neighbor_cell_pick(
                            selected, context->level, goal_xy, route_xy, 1)) {
                        --record->behaviour;
                    }
                    else {
                        if (record->type == 8) message_opcode = 0x0d;
                        else if (record->type == 10) message_opcode = 0x0f;
                        else if (record->type == 0x0b) message_opcode = 0x11;
                        else if (record->type == 0x17) message_opcode = 0x32;
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
                if ((record->type == 0x0c || record->type == 0x16) &&
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
                    cmd_produce(record->type == 0x0c ? 0x27 : 0x30,
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

/* Append a record to the entity's chain (entity +0x2f0 analog).
 * Corpus shape (finding 10): the move/target/gated creators chain the
 * record *only* onto the entity chain — CmdCreateMove @ 0x4035c0 walks
 * FUN_00404190 (CmdChainWalk) and writes `tail->next = rec` (or the +0x2f0
 * head), plus the type-2 IdleQueueRegister; they never link onto the global
 * list (DAT_004b3354/4b3358), which is the task-only list (CmdCreateTask
 * links there through CmdLinkHead @ 0x404030). The +0x60 next field is one
 * pointer, so a record must live on exactly one of the two overlays: using
 * it for both at once chains two entities' live commands together through
 * the first command's tail link, giving double ownership of one record.
 * (Exposed by the executor move test: a move issued to a *selection* of two
 * entities creates exactly that conflict.) */

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
void cmd_create_move(EntitySlot* e, float x, float y, int flag)
{
    if (e == NULL) {
        return;
    }
    CmdRec* rec = (CmdRec*)cmd_alloc(0);
    if (rec == NULL) {
        return;
    }
    rec->x = x;
    rec->y = y;
    rec->param = flag;
    /* Corpus move state +0x3e0 bit 0x8 is the creator side of the mover
     * branch, and the dispatch installs the path list per command
     * (0x4418d0): build a real path, falling back to the one-hop seed when
     * there is no grid or no path. */
    e->flags_3e0 |= ENT_FLAG3E0_MOVING;
    e->wp_count = 0;
    e->wp_index = 0;
    if (ai_path_build(s_ctx, e, x, y) == 0) {
        ai_path_append(e, x, y);
    }
    cmd_chain_append(e, rec, 1);
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
    switch (e->etype) {
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
    if (e->etype != 6) {
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
    if (e->etype == 8) {
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
    if (e == NULL || payload == NULL) return NULL;
    rec = (CmdRec*)cmd_alloc(0x15);
    if (rec == NULL) return NULL;
    rec->payload = payload;
    rec->cell_count = parameter;
    cmd_chain_append(e, rec, 0);
    return rec;
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
        first = 0.5f;
        second = 0.5f;
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
}

/* ------------------------------------------------------------------ *
 *  Global tick + purge (scoped; AI execution receivers are stubs).
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

// FUNCTION: LEGORR 0x00402310
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
        if (behavior == 0 || (rec->state & CMD_STATE_CANCEL) != 0u) {
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
                if (s_cmd_purge_cell_usable != NULL &&
                    s_cmd_purge_cell_usable(cell_x, cell_y,
                                            s_cmd_purge_context) != 0) {
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
                    if (s_cmd_purge_neighbor_eligible != NULL &&
                        s_cmd_purge_neighbor_eligible(x, y,
                            s_cmd_purge_context) != 0) {
                        found = 1;
                    }
                }
                if (found) {
                    if (rec->behaviour == 0) {
                        rec->behaviour = 100;
                    }
                } else {
                    rec->behaviour = 0;
                }
            }
        }
    }
    if (count > 1 && s_cmd_purge_compare != NULL) {
        qsort(records, (size_t)count, sizeof(records[0]),
              cmd_purge_compare_adapter);
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
    if (!s_global_enabled) {
        return;
    }
    /* The corpus's per-command processing (the AI task switch that reads
     * +0x4c/+0x1c) is a stub boundary (finding 10 §"Still open"). Keep the
     * structural walk; the global list is drained each tick. */
    for (CmdRec* cur = s_global_head; cur != NULL; ) {
        CmdRec* next = as_rec(cur->global_next);
        (void)cur->state;
        cur->global_next = NULL;
        cur = next;
    }
    s_global_head = NULL;
    for (CmdRec* tail_cur = s_global_tail; tail_cur != NULL; ) {
        CmdRec* next = as_rec(tail_cur->global_next);
        tail_cur->global_next = NULL;
        tail_cur = next;
    }
    s_global_tail = NULL;
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
#define KEYBIND_MAX 8
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
    b->key = key;
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
    /* CmdTickGlobal's two pending-list scans are part of the live command
     * frame path.  Its full callback dispatch has no port equivalent, so the
     * scans rebuild their candidate views from the active GameContext. */
    if (((uint32_t)s_global_enabled & 0x80000000u) == 0u) {
        CmdRec* pending;
        for (pending = s_global_head; pending != NULL;
             pending = as_rec(pending->global_next)) {
            if (pending->type == 10 &&
                (pending->state & 0x200000u) == 0u &&
                pending->payload != NULL &&
                (((EntitySlot*)pending->payload)->field_340 <= 0.0f ||
                 ((EntitySlot*)pending->payload)->field_340 >= 100.0f))
                pending->state |= 0x20u;
        }
        pending_command_entity_assignment_tick();
        pending_command_timer_fire_scan();
    }
    /* 1. Producer block: the held-key pre-pass (SelectionRoot step 1 —
     *    cheat-keybinds; the demo binds keys via cmd_bind_key). */
    if (s_ctx != NULL && s_ctx->input_keyheld != NULL) {
        cmd_keybind_tick(s_ctx->input_keyheld);
    }

    /* 2. Active buffer -> swap (DAT_004fef28). */
    int block = s_ping;
    int count = s_queue_count[block];
    s_queue_count[block] = 0;
    s_ping = !s_ping;

    /* 3. The opcode switch (finding 06/10 §"The queue executor"). */
    for (int i = 0; i < count; ++i) {
        QueueRec* q = &s_queue[block][i];
        EntitySlot* ent = (EntitySlot*)q->a1;
        switch (q->opcode) {
        case 5:
        case 6:
            cmd_create_move(ent, (float)q->a3, (float)q->a4, q->a2);
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
            /* The entry splits a2 into low/high 16-bit direction/mode and
             * passes a3/a4 as coordinates. TerrainStampImpactHandler needs
             * the runtime terrain/effect hook bundle, which is not bound
             * here. */
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
            /* EmergenceRandomSpawnAttempt's port API requires entity/owner
             * indices and level dimensions absent from this queue entry. */
            break;
        case 0x21:
            (void)unported_ledger_stubs::
                ledger_stub_004470d0_EntityConditionalTypeFiveSpawn(ent);
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
 * Comparison deviation: the corpus matches the 32-bit second field, the
 * port matches the stored pointer through the uintptr cast. */
// FUNCTION: LEGORR 0x00451f60
void cmd_queue_cancel_match(int value)
{
    const int block = s_ping;
    int count = s_queue_count[block];
    unsigned i = 0;
    while ((int)i < count) {
        if ((uintptr_t)s_queue[block][i].a1 == (uintptr_t)value) {
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
        if (count_out != NULL) {
            *count_out = 0;
        }
        return NULL; /* deviation: corpus hands the array address anyway */
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
    if (s_ctx == NULL || entity == NULL) {
        return 0;
    }
    const int count = selection_count(s_ctx);
    if (count == 0) {
        return 0;
    }
    EntitySlot** arr = (EntitySlot**)selection_record_array(s_ctx);
    int index = -1;
    for (int i = 0; i < count; ++i) {
        if (arr[i] == (EntitySlot*)entity) {
            index = i;
            break;
        }
    }
    if (index < 0) {
        return 0;
    }
    if (index == 0) {
        /* Corpus posts the HUD context here; the leaves now land as
         * command_context_post_to_hud, but this call site's arguments are
         * lost register residue, so no ported state moves (hook boundary). */
    }
    s_ctx->selection_count = count - 1;
    arr[index] = arr[count - 1];
    ((EntitySlot*)entity)->flags &= ~ENTITY_SELECTED;
    ((EntitySlot*)entity)->flags_3e8 &= ~ENTITY_SELECTED;
    return 1;
}

/* SelectionObjectDetach (FUN_004524b0): cancel queued commands for the
 * object, remove it from the selected set, and clear the current selection
 * context when it refers to that object. The corpus's final context-pair
 * coordinate refresh has no owner in this module; for removal of the first
 * selected object, route the ported HUD reset through the existing post. */
// FUNCTION: LEGORR 0x004524b0
void selection_object_detach(void* entity)
{
    if (entity == NULL) {
        return;
    }
    const int was_first = (selection_first(s_ctx) == (EntitySlot*)entity);
    cmd_queue_cancel_match((int)(uintptr_t)entity);
    (void)selection_entity_remove(entity);
    if (current_selection_context_get() == entity) {
        current_selection_context_set(NULL);
    }
    if (was_first) {
        command_context_post_to_hud(0, NULL);
    }
}

static SelectedEntityDetachPoseHook s_selected_entity_detach_pose_hook;

void selected_entity_detach_pose_hook_set(SelectedEntityDetachPoseHook hook)
{
    s_selected_entity_detach_pose_hook = hook;
}

/* SelectedEntityDetachCallback (FUN_0041f6c0): capability bit 0 chooses
 * between the attachment detach path and the carried-object pose/activity
 * path. The latter is a seam owned by the pose/activity module. */
// FUNCTION: LEGORR 0x0041f6c0
int selected_entity_detach_callback(EntitySlot* entity)
{
    EntitySlot* attached;
    uint32_t capabilities;
    if (entity == NULL) return 0;
    capabilities = entity_capability_flags_get(entity);
    attached = (EntitySlot*)entity->carry_link_308;
    if ((capabilities & 1u) != 0u) {
        if ((entity->flags_3e0 & 0x400u) != 0u && attached != NULL
            && s_selected_entity_detach_pose_hook != NULL) {
            s_selected_entity_detach_pose_hook(attached);
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
        /* Corpus: SelectionClearMarkerAndResetHud — unported marker/HUD
         * seam (finding 77's enqueue+reset) — then reject. */
        return 0;
    }
    if (selection_contains_entity(s_ctx, e) != 0) {
        return 0;
    }
    if (s_ctx->selection_count >= 100) {
        return 0;
    }
    /* Corpus pool refresh: EntityPoolFindByCallbackFiltered is ported but
     * its callback is a corpus code literal (an unported seam). */
    ((EntitySlot**)selection_record_array(s_ctx))[s_ctx->selection_count] = e;
    s_ctx->selection_count = s_ctx->selection_count + 1;
    e->flags_3e8 |= 0x200000u;
    if (e->type != 4
        && (e->flags_3e0 & 0x24002000u) == 0u
        && (e->flags_3e4 & 0x100u) == 0u) {
        /* Activity refresh + action reset: EntityActivityRecordsRefresh
         * / EntityActionSequenceReset — unported entity-layer seams. */
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
        /* Per-type teardown (type 4 / type 0xb / the generic EntityTeardown)
         * is the unported entity-layer teardown family — documented seam. */
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
// FUNCTION: LEGORR 0x004026c0
void entity_deferred_work_schedule(EntitySlot* e)
{
    if (e == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    if (e->attach_304 == NULL && (e->flags_3ec & 0x100u) == 0u) {
        /* EntityPoolCallbackSearch with the fixed callback: the pool
         * registry seam always misses here, so the fallback task is
         * created (corpus shape). */
        cmd_create_task(e);
    }
}

/* --- registry accessors / match marker (see cmd.hpp) -------------------------
 * DAT_004b7ff4: the per-priority state table, zeroed by the (unported)
 * registry initializer loop — zero-init static, 27 entries. The preceding
 * command-count matrix occupies 0x12c0 dwords and ends at this table. */
#define PRIORITY_ENTRY_COUNT 27
// GLOBAL: LEGORR 0x004b34f4
uint32_t cmd_pending_command_counts[CMD_PENDING_COMMAND_COUNT_TABLE_SIZE];
// GLOBAL: LEGORR 0x004b7ff4
static int s_priority_state[PRIORITY_ENTRY_COUNT];
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
    if (rec == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    CmdRec* r = as_rec(rec);
    if ((r->state & 0x30000u) == 0u) {
        const int behaviour = (kind >= 0 && kind < 27)
                                  ? s_behaviour[kind] : 0x31;
        r->kind = kind;
        r->behaviour = behaviour;
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
 * The record array's per-entry callees (the entity action-sequence reset and
 * the attached-object list detach) belong to the unported teardown family, so
 * the loop below is the seam half — the guard, the raised bits, and the
 * snapshot's ownership by the record are the ported observable half. */
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
        void* entry = (array != NULL) ? ((void**)array)[i] : NULL;
        (void)entry;
        /* Corpus: EntityActionSequenceReset(entry, 0) and, with the mode
         * argument set, EntityAttachedObjectListDetach(entry, 0) — the
         * teardown seam; the port changes no entity state here. */
        if (mode != 0) {
            (void)entry;
        }
    }
    (void)mode;
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
    entity_pool_callback_search(pending_record_callback_adapter, payload);
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
    if (level_cell_flag_4(lvl, x, y) == 0u) {
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
    /* Counter update: the spatial-query/action-issue family and the
     * cell-record counter block are unported (documented seam). */
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
    /* Counter update: the seam half above. */
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
    /* Counter update: the seam half above. */
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
 * copies the 26-dword window, restores its free-list self-link at +0x19
 * (the port keeps that bookkeeping in `next` during pooling, so a fresh
 * alloc is clean), flags +0x17 |= 0x200 and zeroes +0x18. */
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
    dst->next = NULL;        /* corpus +0x18 = 0 */
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
 * grid-spawn route accepts dispatches message 6 with the coordinate pair
 * (the route is the quote-only seam precedent — it never lands, so neither
 * the dispatch nor its break fires). Either way the sweep re-arms the timer
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
                /* Grid-spawn route (FUN_0040f190 quote-only seam): never
                 * lands, so the dispatch and the break cannot fire; the
                 * coordinate pair would ride the message 6 call. */
                const int spawn = 0;
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

/* --- command sample stats (2026-10-25) -----------------------------------
 * CommandSampleStatsAccumulate (FUN_0046b660): per-kind sliding-window
 * statistics. A null sample clears the window head and the running total;
 * a sample slides the window when the stored head falls below the sample's
 * +8 word (the new total is (old total − old head) + the new head), then
 * adds the sample's +4 word to the running total and raises the high-water
 * mark. Every call appends the sample pointer to the kind's log and bumps
 * the index. Deviations: the corpus strides its slot block blindly — the
 * port guards the kind (the corpus's stride is 0x27c bytes per kind, of
 * which this function touches dwords 0..24: the log at 0..+19, the index
 * at +0x50, head +0x54, total +0x58, sum +0x5c, high +0x60) and stops the
 * log writes at the slot edge, where the corpus's twenty-first store would
 * stride onto the index word and corrupt it (faithfully pinned: the port
 * keeps counting while it stops storing). */
#define CMD_STATS_KINDS 110u /* corpus window span / slot stride */
#define CMD_STATS_LOG_DWORDS 20u
#define CMD_STATS_SLOT_DWORDS 25u

static uint32_t s_cmd_stats[CMD_STATS_KINDS][CMD_STATS_SLOT_DWORDS];

// FUNCTION: LEGORR 0x0046b660
void command_sample_stats_accumulate(int kind, const void* sample)
{
    if (kind < 0 || (unsigned)kind >= CMD_STATS_KINDS) {
        return;
    }
    uint32_t* slot = s_cmd_stats[kind];
    if (sample == NULL) {
        slot[21] = 0u;                       /* +0x54 head */
        slot[23] = 0u;                       /* +0x5c sum  */
    } else {
        uint32_t v4 = 0u, v8 = 0u;           /* sample+4, sample+8 */
        __builtin_memcpy(&v4, (const char*)sample + 4, sizeof v4);
        __builtin_memcpy(&v8, (const char*)sample + 8, sizeof v8);
        if ((int)slot[21] < (int)v8) {       /* slide the window */
            slot[22] = (uint32_t)((int)slot[22] - (int)slot[21]);
            slot[21] = v8;
            slot[22] = (uint32_t)((int)slot[22] + (int)v8);
        }
        slot[23] = (uint32_t)((int)slot[23] + (int)v4);
        if ((int)slot[24] < (int)slot[23]) { /* high-water mark */
            slot[24] = slot[23];
        }
    }
    const uint32_t idx = slot[20];
    if (idx < CMD_STATS_LOG_DWORDS) {
        slot[idx] = (uint32_t)(uintptr_t)sample;
    }
    slot[20] = idx + 1u;
}

uint32_t command_sample_stats_peek(int kind, int field)
{
    if (kind < 0 || (unsigned)kind >= CMD_STATS_KINDS
        || field < 0 || (unsigned)field >= CMD_STATS_SLOT_DWORDS) {
        return 0u;
    }
    return s_cmd_stats[kind][field];
}
