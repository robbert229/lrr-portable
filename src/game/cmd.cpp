// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmd.hpp"
#include "ai.hpp"
#include "select.hpp"
#include "level.hpp"
#include "frontend.hpp"
#include "util.hpp"
#include "entity.hpp"
#include "gods98/heap.hpp"
#include "gods98/media.hpp"

/* ------------------------------------------------------------------ *
 *  Command record (finding 10 §"Command record" — corpus stride 0x68).
 *  The struct is a semantic model, not a byte clone: the corpus packs
 *  next(+0x60) and free-list link(+0x64) into consecutive 4-byte words and
 *  payload(+0x10)/buffer(+0x40) as 32-bit pointers; here the record is a
 *  stable pool object. A separate port-only global link keeps another
 *  entity's commands from extending this entity's chain. Field names carry
 *  the corpus offsets.
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
    int32_t  grant_act;  /* +0x38 activity the command grants: the tick's
                          * deferred install (callback 0x43da62) appends it
                          * to the entity's permitted-activity list and
                          * drops type-0x13 grant records (cmd_chain_drop).
                          * The corpus writer of the record's +0x38 — the
                          * task-state granter — is config/runtime-driven
                          * (finding 10 §"Spawn-dispatch task-state switch";
                          * the shipped binary creates type-0x13 "ref"
                          * records via CmdAlloc(0x13) — verified site
                          * 0x403108 inside FUN_004030e0 — but pins no
                          * +0x38 grant/flag pair);
                          * port-side writers stage grants through
                          * cmd_rec_set_activity. */
    int32_t  extra;      /* +0x3c (type-8 tasks set 8)                         */
    void*    buffer;     /* +0x40 owned buffer (freed on cmd_free)             */
    uint32_t count;      /* +0x44 the buffer's entry count                     */

    int32_t  kind;       /* +0x4c kind index                                   */
    void*    ref;        /* +0x58 referenced record (the 0x13 "ref" pair)      */
    uint32_t state;      /* +0x5c state flags (CMD_STATE_*)                    */
    void*    next;       /* +0x60 chain link / free-list link when free        */
    void*    global_next; /* port-only global command-list link               */
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
static CmdRec* s_blocks[CMD_POOL_MAX_BLOCKS];
static int     s_pool_level = 0;
static void*   s_free       = NULL;

static CmdRec* s_global_head = NULL; /* DAT_004b3354 */
static CmdRec* s_global_tail = NULL; /* DAT_004b3358 */
static int   s_global_enabled = 1; /* DAT_004b8060 */

static void cmd_pool_grow(void)
{
    int level = s_pool_level;
    if (level >= CMD_POOL_MAX_BLOCKS) {
        return;
    }
    uint32_t n = 1u << (level & 0x1f);
    CmdRec* block = (CmdRec*)calloc(n, CMD_STRIDE);
    if (block == NULL) {
        return;
    }
    s_blocks[level] = block;
    ++s_pool_level;
    if (n > 1) {
        for (uint32_t i = 0; i + 1 < n; ++i) {
            block[i].next = &block[i + 1];
        }
    }
    block[n - 1].next = s_free;
    s_free = block;
}

void* cmd_alloc(int type)
{
    if (s_free == NULL) {
        cmd_pool_grow();
    }
    if (s_free == NULL) {
        return NULL;
    }
    CmdRec* rec = (CmdRec*)s_free;
    s_free = rec->next;
    memset(rec, 0, CMD_STRIDE);
    rec->next = NULL;
    rec->type = type;
    return rec;
}

void cmd_free(void* rec, int cancel)
{
    if (rec == NULL) {
        return;
    }
    if (cancel == 0) {
        cmd_cancel_ref(rec); /* corpus: unless the argument suppresses it */
    }
    CmdRec* r = as_rec(rec);
    if (r->buffer != NULL && (r->state & CMD_STATE_BUFOUT) == 0) {
        free(r->buffer);
        r->buffer = NULL;
    }
    r->next = s_free;
    s_free = r;
}

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
static int8_t s_behaviour[27]; /* DAT_004b32e8 by kind */

static int cmd_strcasecmp(const char* a, const char* b)
{
    for (;;) {
        int ca = (unsigned char)*a++;
        int cb = (unsigned char)*b++;
        if (ca >= 'a' && ca <= 'z') ca -= 0x20;
        if (cb >= 'a' && cb <= 'z') cb -= 0x20;
        if (ca != cb) return ca - cb;
        if (ca == 0) return 0;
    }
}

void cmd_init(void)
{
    for (int i = 0; i < CMD_POOL_MAX_BLOCKS; ++i) {
        s_blocks[i] = NULL;
    }
    s_pool_level = 0;
    s_free = NULL;
    s_global_head = NULL;
    s_global_tail = NULL;
    s_global_enabled = 1;

    for (int i = 0; i < 27; ++i) {
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

int cmd_kind_by_name(const char* name, int* out)
{
    if (name == NULL || out == NULL) {
        return 0;
    }
    for (int i = 0; i < 27; ++i) {
        if (cmd_strcasecmp(name, PRIO_NAMES[i]) == 0) {
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

static void cmd_link_head(CmdRec* rec, int kind)
{
    if (s_global_head == NULL) {
        s_global_tail = rec; /* first task establishes the global tail */
    }
    rec->global_next = s_global_head;
    s_global_head = rec;
    rec->now = 1500.0f;
    rec->state = (rec->state & ~1u) | CMD_STATE_PENDING;
    cmd_bind_kind(rec, kind);
}

static void cmd_link_tail(CmdRec* rec)
{
    rec->global_next = NULL;
    if (s_global_tail != NULL) {
        s_global_tail->global_next = rec;
    } else {
        s_global_head = rec;
    }
    s_global_tail = rec;
    rec->now = 1500.0f;
    rec->state &= ~CMD_STATE_PENDING;
}

void* cmd_chain_walk(const EntitySlot* e)
{
    if (e == NULL || e->cmd_head == NULL) {
        return NULL;
    }
    CmdRec* cur = as_rec(e->cmd_head);
    for (;;) {
        if (cur->next == NULL) {
            return cur;
        }
        cur = as_rec(cur->next);
    }
}

static void idle_register(EntitySlot* e);

/* FUN_00403d10 (generic drop): unlink and free every chain record whose
 * type (+0x00) equals `type`, mirroring the corpus loop over the entity's
 * +0xbc record list. Like the corpus, records with per-type side effects
 * (its types 7/0xc/0x17/3 branches) are not included here — the port only
 * needs the plain drop FUN_00403d10(ent, 0x13, 0, 1) performs on the
 * deferred-grant path. */
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

void* cmd_global_next(const void* rec)
{
    return rec != NULL ? cas_rec(rec)->global_next : NULL;
}

void cmd_global_unlink(const void* rec)
{
    if (rec == NULL || s_global_head == NULL) {
        return;
    }
    CmdRec* prev = NULL;
    for (CmdRec* cur = s_global_head; cur != NULL;
         cur = as_rec(cur->global_next)) {
        if ((const void*)cur == rec) {
            if (prev != NULL) {
                prev->global_next = cur->global_next;
            } else {
                s_global_head = as_rec(cur->global_next);
            }
            if (s_global_tail == cur) {
                s_global_tail = prev;
            }
            cur->global_next = NULL;
            return;
        }
        prev = cur;
    }
}

/* PendingRecordEntityReferenceClear (FUN_004063e0) — see cmd.hpp. The
 * corpus strides the global list's +0x60 link and the +0x48 dword
 * blindly; the port rides its native-size link slot and reference slot
 * (documented deviations). */
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

static GameContext* s_ctx = NULL;

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
    /* MinionCompanionCmd: type-6 minions get a second task (kind 0x15)
     * linked at the tail, flagged 0x2000. */
    if (e->etype == 6) {
        CmdRec* rec2 = (CmdRec*)cmd_alloc(3);
        if (rec2 != NULL) {
            rec2->payload = e;
            cmd_bind_kind(rec2, 0x15);
            cmd_link_tail(rec2);
            rec2->state |= CMD_STATE_MINION;
        }
    }
}

/* ------------------------------------------------------------------ *
 *  Global tick + purge (scoped; AI execution receivers are stubs).
 * ------------------------------------------------------------------ */

void cmd_chain_purge(EntitySlot* e)
{
    if (e == NULL) {
        return;
    }
    CmdRec* cur = as_rec(e->cmd_head);
    e->cmd_head = NULL;
    while (cur != NULL) {
        CmdRec* next = as_rec(cur->next);
        cmd_global_unlink(cur);
        cmd_free(cur, 1);
        cur = next;
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
    s_global_tail = NULL;
}

/* ------------------------------------------------------------------ *
 *  Idle queue table (IdleQueueRegister): type-2 entries.
 * ------------------------------------------------------------------ */

#define IDLE_TABLE_MAX 20
static EntitySlot* s_idle[IDLE_TABLE_MAX];

static void idle_register(EntitySlot* e)
{
    if (e == NULL || e->etype != 2) {
        return;
    }
    int i = 0;
    while (i < IDLE_TABLE_MAX && s_idle[i] != e) {
        ++i;
    }
    if (i == IDLE_TABLE_MAX) {
        for (i = 0; i < IDLE_TABLE_MAX; ++i) {
            if (s_idle[i] == NULL) {
                s_idle[i] = e;
                break;
            }
        }
    }
    /* timestamps handled by the (future) idle poller */
}

/* port hook: EntityPointerRegistriesRemove clears idle-queue records from
 * the window this table models; the corpus leaves the timer words. */
void idle_queue_clear(EntitySlot* e)
{
    for (int i = 0; i < IDLE_TABLE_MAX; ++i) {
        if (s_idle[i] == e) {
            s_idle[i] = NULL;
        }
    }
}
/* ------------------------------------------------------------------ *
 *  Queue executor (FUN_004520d0): 5-dword records, ping-pong buffered.
 * ------------------------------------------------------------------ */

#define CMD_QUEUE_CAP 64

typedef struct QueueRec {
    int        opcode;
    const void* a1;    /* arg1: entity ptr for entity ops */
    int        a2;
    int        a3;
    int        a4;
} QueueRec;

static QueueRec s_queue[2][CMD_QUEUE_CAP];
static int      s_queue_count[2];
static int      s_ping; /* active producer block (DAT_004fef28) */

void cmd_context_set(GameContext* ctx)
{
    s_ctx = ctx;
    s_ping = 0;
    for (int i = 0; i < 2; ++i) {
        s_queue_count[i] = 0;
    }
}

void cmd_produce(int op, const void* a1, int a2, int a3, int a4)
{
    int block = s_ping;
    if (s_queue_count[block] >= CMD_QUEUE_CAP) {
        return; /* queue full: silent drop (FUN_00452060) */
    }
    QueueRec* q = &s_queue[block][s_queue_count[block]++];
    q->opcode = op;
    q->a1 = a1;
    q->a2 = a2;
    q->a3 = a3;
    q->a4 = a4;
}

void cmd_enqueue(int opcode, const EntitySlot* e, int param, int p1, int p2)
{
    cmd_produce(opcode, e, param, p1, p2);
}

/* Cheat/debug keybinds (FUN_00452010). */
#define KEYBIND_MAX 8
typedef struct KeyBind {
    int key;
    int op, a1, a2, a3, a4;
} KeyBind;
static KeyBind s_keybinds[KEYBIND_MAX];
static int     s_keybind_count = 0;

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
                        b->a2, b->a3, b->a4);
        }
    }
}

/* Manager-dispatch stubs (the corpus's 0x4526c0/0x452be0/… handlers). */
static void mgr_stub(const char* what, int a)
{
    (void)what;
    (void)a;
}

void cmd_queue_tick(void)
{
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
        case 1:  mgr_stub("op1",  q->a2); break;
        case 3:  mgr_stub("op3",  0);        break;
        case 4:  mgr_stub("op4",  q->a2); break;
        case 9:  mgr_stub("op9",  q->a2); break;
        case 0xb: mgr_stub("opb",  0);       break;
        case 0xc: mgr_stub("opc",  q->a2); break;
        case 0x13: mgr_stub("op13", q->a2); break;
        case 0x15: mgr_stub("op15", q->a2); break;
        case 0x1b: mgr_stub("op1b", q->a2); break;
        case 0x1f: mgr_stub("op1f", 0);       break;
        case 0x21: mgr_stub("op21", q->a2); break;
        case 0x23: mgr_stub("op23", q->a2); break;
        case 0x25: mgr_stub("op25", q->a2); break;
        case 0x2d: selection_state_tick(s_ctx); break;
        case 0x34: mgr_stub("op34", q->a2); break;
        case 0x36: mgr_stub("op36", q->a2); break;
        case 0x40: mgr_stub("op40", 0);       break;
        default:   mgr_stub("unknown", q->opcode); break;
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

int cmd_array_find_value_index(const int* array, unsigned count, int value,
                               unsigned* index_out)
{
    if (array == NULL) {
        return 0; /* deviation: the corpus dereferences blindly */
    }
    for (unsigned i = 0; i < count; ++i) {
        if (array[i] == value) {
            if (index_out != NULL) {
                *index_out = i;
            }
            return 1;
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
 * PRNG: the corpus's shared random word draw (thunk_FUN_0048e170),
 * modelled module-locally the way level.cpp and frontend.cpp do. */
static uint32_t s_cmd_rand_state = 1;

static int cmd_rand15(void)
{
    s_cmd_rand_state = s_cmd_rand_state * 214013u + 2531011u;
    return (int)((s_cmd_rand_state >> 16) & 0x7fffu);
}

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
        /* deviation: the corpus queues a pointer to its stack pair; the
         * port stores the coordinates by value in the record. */
        cmd_produce(0x24, entity, 0, x, y);
    }
}

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
        /* Corpus posts the HUD context (CommandContextPostToHud) — the
         * HUD-chain seam (GlobalStateLowByteClearAndRefresh + the menu /
         * map-coordinate leaves) is unported, so no ported state moves. */
    }
    s_ctx->selection_count = count - 1;
    arr[index] = arr[count - 1];
    ((EntitySlot*)entity)->flags &= ~ENTITY_SELECTED;
    ((EntitySlot*)entity)->flags_3e8 &= ~ENTITY_SELECTED;
    return 1;
}

/* --- selection filter / state monitor (see cmd.hpp) --------------------------
 * The corpus's selection-array append + flag-word set land on the port's
 * modeled selection storage; everything it would hand to the entity /
 * activity / visual layers is documented there as an unported seam. */
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

/* --- entity record schedulers (see cmd.hpp) ----------------------------------
 * Corpus note: the queue leaf's CurrentLevelGet call discards its result
 * (the corpus's level-state touch) — no ported state depends on it. */
void entity_command15_validated_record_queue(EntitySlot* e)
{
    if (e != NULL && entity_flag40000_threshold_compare(e) != 0) {
        CmdRec* rec = (CmdRec*)cmd_alloc(0x1a);
        cmd_link_head(rec, 0xf);
        rec->payload = e;
        rec->state = (rec->state & ~0x10u) | 0x40040u;
    }
}

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
 * registry initializer loop — zero-init static, 27 entries. */
#define PRIORITY_ENTRY_COUNT 27
static int s_priority_state[PRIORITY_ENTRY_COUNT];

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

void cmd_set_kind(void* rec, int kind)
{
    if (rec == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    CmdRec* r = as_rec(rec);
    if ((r->state & 0x30000u) == 0u) {
        r->behaviour = cmd_kind_behaviour(kind);
    } else {
        r->behaviour = 99;
    }
    r->kind = kind;
}

void priority_entry_order_value_set(int kind, int value)
{
    if (kind >= 0 && kind < 27) {
        s_behaviour[kind] = (int8_t)value;
    }
    /* deviation: the corpus writes blindly into the order table. */
}

void priority_entry_state_set(int kind, int value)
{
    if (kind >= 0 && kind < PRIORITY_ENTRY_COUNT) {
        s_priority_state[kind] = value;
    }
}

int priority_entry_state_get(int kind)
{
    if (kind < 0 || kind >= PRIORITY_ENTRY_COUNT) {
        return 0; /* deviation: the corpus reads blindly */
    }
    return s_priority_state[kind];
}

void cmd_rec_state_set(void* rec, uint32_t state)
{
    if (rec != NULL) {
        as_rec(rec)->state = state;
    }
}

/* --- registry flush / wrappers (see cmd.hpp) ---------------------------------
 * The handle array (DAT_004b31c8 band, 12 dwords + companion): no ported
 * feeder (the registry initializer is the seam), so it is modeled as a
 * zero-init static whose release walk is inert. */
static void* s_handle_array[12];
static int   s_handle_array_companion;  /* DAT_004b31f8 */

void cmd_flush_all(int disable_mode)
{
    if (disable_mode != 0) {
        s_global_enabled &= ~2;
        return;
    }
    /* The entity callback sweep (LAB_004021f0) is the unported registry
     * seam; the corpus additionally zeroes the cell-record counter block
     * (0x12c0 entries) — registry data absent from the port. */
    s_global_enabled |= 2;
    s_global_head = NULL;
    s_global_tail = NULL;
    for (int i = 0; i < PRIORITY_ENTRY_COUNT; ++i) {
        s_priority_state[i] = 0;
    }
}

int cmd_global_enabled_peek(void) { return s_global_enabled; }

void* engine_memory_allocate(long size)
{
    (void)engine_heap_alloc(size);
    /* corpus shape: the allocation happens; the pointer is discarded. */
    return NULL;
}

void engine_memory_free(void* ptr)
{
    engine_heap_free(ptr);
}

void handle_array_release_and_reset(void)
{
    for (int i = 0; i < 12; ++i) {
        if (s_handle_array[i] != NULL) {
            engine_heap_free(s_handle_array[i]);
        }
    }
    s_handle_array_companion = 0;
    s_global_enabled = 0;
}

void entity_command14_batch_issue(int count)
{
    const int raise_bit6 = (count == 0);
    const int n = raise_bit6 ? 1 : count;
    for (int i = 0; i < n; ++i) {
        CmdRec* rec = (CmdRec*)cmd_alloc(4);
        if (rec == NULL) {
            continue;
        }
        cmd_set_kind(rec, 0x14);
        if (raise_bit6) {
            rec->state |= 0x40u;
        }
        cmd_link_tail(rec);
    }
}

void level_no_gather_object_setup(int mode)
{
    if (mode == 0) {
        entity_command14_batch_issue(0);
    }
    CmdRec* gather = (CmdRec*)cmd_alloc(0x18);
    if (gather != NULL) {
        cmd_set_kind(gather, 0x17);
        gather->state |= 0x40u;
        /* corpus: the manual push onto the tail pointer only (the head
         * stays put until a head-link touches the list) — ported through
         * the tail link (deviation: timestamp/pending housekeeping). */
        cmd_link_tail(gather);
    }
    CmdRec* other = (CmdRec*)cmd_alloc(0x1b);
    if (other != NULL) {
        cmd_link_head(other, 0);
        other->state = (other->state & ~0x10u) | 0x440u;
    }
}

/* --- cancellation-ref and conditional issuers (see cmd.hpp) ------------------
 * The referenced-record slot (+0x58) is the type-0x13 "ref" pair the
 * shipped binary's spawn-dispatch switch allocates through CmdAlloc(0x13);
 * port-side producers leave it NULL, so both halves answer inert unless a
 * test stages the reference. */
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

void entity_conditional_task_schedule(EntitySlot* e)
{
    if (e == NULL) {
        return; /* deviation: the corpus reads the entity blindly */
    }
    if ((entity_capability_flags_get(e) & 1u) != 0u
        && (e->flags_3e0 & 0x400u) == 0u
        && (e->flags_3ec & 0x10000u) == 0u) {
        CmdRec* rec = (CmdRec*)cmd_alloc(0x1e);
        if (rec == NULL) {
            return;
        }
        rec->payload = e;
        e->flags_3ec |= 0x10000u;
        cmd_link_head(rec, 0x11);
        rec->state = (rec->state & ~0x40u) | 0x100010u;
    }
}

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

void entity_target_command_queue(EntitySlot* e, void* target)
{
    CmdRec* rec = (CmdRec*)cmd_alloc(0x11);
    if (rec == NULL) {
        return;
    }
    rec->payload = target;
    if (e == NULL) {
        return; /* deviation: the corpus writes +0x2f0 blindly */
    }
    CmdRec* tail = as_rec(cmd_chain_walk(e));
    if (tail != NULL) {
        tail->next = rec;
    } else {
        e->cmd_head = rec;
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
        const uint32_t stats = entity_stats_word_get(e);
        if ((stats & 0x2000000u) == 0u) {
            if ((stats & 0x1000000u) != 0u) {
                rec->extra = 2;
            }
        } else {
            rec->extra = ((stats & 0x1000000u) == 0u) ? 4 : 1;
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

void coordinate_pair_callback_dispatch(const void* coord_pair)
{
    /* Seam: pairs the coordinate word with zero and traverses the entity
     * pool with the fixed callback — the pool-registry machinery above. */
    (void)coord_pair;
}

void argument_callback_dispatch(void* arg)
{
    /* Seam: dispatches the argument through the fixed pool callback. */
    (void)arg;
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
