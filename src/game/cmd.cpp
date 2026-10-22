// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmd.hpp"
#include "ai.hpp"
#include "select.hpp"
#include "util.hpp"

/* ------------------------------------------------------------------ *
 *  Command record (finding 10 §"Command record" — corpus stride 0x68).
 *  The struct is a semantic model, not a byte clone: the corpus packs
 *  next(+0x60) and free-list link(+0x64) into consecutive 4-byte words and
 *  payload(+0x10)/buffer(+0x40) as 32-bit pointers; here the record is a
 *  stable pool object and the links share one `next` slot (a live record is
 *  never on the free list at the same time). Field names carry the corpus
 *  offsets.
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
    int32_t  kind;       /* +0x4c kind index                                   */
    uint32_t state;      /* +0x5c state flags (CMD_STATE_*)                    */
    void*    next;       /* +0x60 chain link / free-list link when free        */
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
    rec->next = rec; /* free-list self link while live */
    rec->type = type;
    return rec;
}

void cmd_free(void* rec, int cancel)
{
    (void)cancel; /* cancellation side-effects are a stub (CmdCancelRef) */
    if (rec == NULL) {
        return;
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
        s_global_tail = rec; /* first task: the tail bookkeeping has no record
                               * yet (cmd_link_tail and finish unlink rely on
                               * a consistent tail bound) */
    }
    rec->next = s_global_head;
    s_global_head = rec;
    rec->now = 1500.0f;
    rec->state = (rec->state & ~1u) | CMD_STATE_PENDING;
    cmd_bind_kind(rec, kind);
}

static void cmd_link_tail(CmdRec* rec)
{
    rec->next = NULL;
    if (s_global_tail != NULL) {
        s_global_tail->next = rec;
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
            /* Unlink from the global list FIRST: chain and global share the
             * one next slot, so a chain splice before the scan hides cur
             * from cmd_global_unlink — it would miss, never fix
             * s_global_tail, and the next cmd_link_tail would thread a
             * pooled record off the dangling tail. */
            CmdRec* nxt = next; /* cur->next before unlink/free */
            cmd_global_unlink(cur);
            if (prev != NULL) {
                prev->next = nxt;
            } else {
                e->cmd_head = nxt;
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

void cmd_global_unlink(const void* rec)
{
    if (rec == NULL || s_global_head == NULL) {
        return;
    }
    CmdRec* prev = NULL;
    for (CmdRec* cur = s_global_head; cur != NULL; cur = as_rec(cur->next)) {
        if ((const void*)cur == rec) {
            if (prev != NULL) {
                prev->next = cur->next;
            } else {
                s_global_head = as_rec(cur->next);
            }
            if (s_global_tail == cur) {
                s_global_tail = prev;
            }
            return;
        }
        prev = cur;
    }
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
        CmdRec* next = as_rec(cur->next);
        (void)cur->state;
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
        case 0x2d: mgr_stub("op2d", 0);       break; /* selection filter stub */
        case 0x34: mgr_stub("op34", q->a2); break;
        case 0x36: mgr_stub("op36", q->a2); break;
        case 0x40: mgr_stub("op40", 0);       break;
        default:   mgr_stub("unknown", q->opcode); break;
        }
    }
}
