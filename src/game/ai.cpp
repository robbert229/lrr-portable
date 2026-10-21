// SPDX-License-Identifier: LGPL-3.0-or-later
/* AI execution receivers — port of the FUN_0043b270 entity-tick callback
 * (entry 0x43c810), decompile-verified 2026-10-21 (finding 10 closed). See
 * ai.hpp for the anatomy; branch comments carry corpus addresses. */
#include <stdio.h>
#include <string.h>

#include "ai.hpp"
#include "cmd.hpp"
#include "mission.hpp"

/* The command record's mutable `now` doubles as the task countdown for
 * task commands (the link primitives seed it with the 1500.0 "now"
 * timestamp; the executor resets it to AI_TASK_DURATION on first run). */
#define AI_TOMBSTONE 1000.0f

/* Flag bits the callback branches on (corpus +0x3ec / +0x3e4 words). */
#define ENT_FLAG3EC_ACTIVITY_DONE  0x00200000u  /* activity ended (0x43c845) */
#define ENT_FLAG3EC_TARGET_LINK    0x00040000u  /* acquire link pending (0x43c8b2) */
#define ENT_FLAG3EC_LINKED         0x00080000u  /* link acquired (writes +0x354) */
#define ENT_FLAG3EC_IDLE_REGISTER  0x00000040u  /* type-1 idle check (0x43c94d) */
#define ENT_FLAG3EC_VULNERABLE     0x00000020u  /* cheat-gated flag (0x43c90b) */
#define ENT_FLAG3E4_WAKE_TIMER     0x08000000u  /* +0x368 runs, tick early-return */

static const void* rec_payload(const void* rec) { return cmd_rec_payload(rec); }

/* DAT_004decc4 activity-name table, per the initializer at 0x436c2f. */
static const char* const s_activity[AI_ACTIVITY_COUNT] = {
    "Activity_Stand", "Activity_Route", "Activity_RouteRubble",
    "Activity_RunPanic", "Activity_Drill", "Activity_Teleport",
    "Activity_Walk", "Activity_Reinforce", "Activity_Reverse",
    "Activity_TurnLeft", "Activity_TurnRight", "Activity_CantDo",
    "Activity_Emerge", "Activity_Enter", "Activity_EnterRein",
    "Activity_Collect", "Activity_Gather", "Activity_Carry",
    "Activity_CarryRubble", "Activity_Throw", "Activity_CarryTurnLeft",
    "Activity_CarryTurnRight", "Activity_CarryStand", "Activity_HitLeft",
    "Activity_HitRight", "Activity_HitFront", "Activity_HitBack",
    "Activity_HitHard", "Activity_Dynamite", "Activity_Deposit",
    "Activity_Clear", "Activity_Place", "Activity_Repair",
    "Activity_Slip", "Activity_Rest", "Activity_Eat",
    "Activity_Stamp", "Activity_ThrowMan", "Activity_ThrownByRockMonster",
    "Activity_GetUp", "Activity_BuildPath", "Activity_Upgrade",
    "Activity_Explode", "Activity_Unpowered", "Activity_FireLaser",
    "Activity_Freezed", "Activity_FreezeStart", "Activity_FreezeMelt",
    "Activity_Recharge", "Activity_WakeUp", "Activity_Train",
    "Activity_FloatOn", "Activity_FloatOff", "Activity_Opening",
    "Activity_Closing", "Activity_Open",
    "Activity_Waiting1", "Activity_Waiting2", "Activity_Waiting3",
    "Activity_Waiting4", "Activity_Waiting5", "Activity_Waiting6",
    "Activity_Waiting7", "Activity_Waiting8", "Activity_Waiting9",
    "Activity_Waiting10", "Activity_Waiting11", "Activity_Waiting12",
    "Activity_Waiting13", "Activity_Waiting14", "Activity_Waiting15",
    "Activity_Waiting16", "Activity_Waiting17", "Activity_Waiting18",
    "Activity_Waiting19", "Activity_Waiting20", "Activity_Waiting21"
};

const char* ai_activity_name(int id)
{
    return (id >= 0 && id < AI_ACTIVITY_COUNT) ? s_activity[id] : NULL;
}

void ai_set_activity(EntitySlot* e, int id, int param)
{
    if (e == NULL || id < 0 || id >= AI_ACTIVITY_COUNT) {
        return; /* corpus indexes the table blindly; clamp the mirror */
    }
    e->activity = id;
    /* Corpus +0x2ec mirrors "last name bookkept by FUN_00447c40"; every
     * corpus activity-set site re-registers through 447c40 in the same
     * step, so the fold lands here in the port (see finding 10). */
    e->activity_last = id;
    e->activity_param = param;
    e->activity_timer = 0.0f;
}

/* FUN_00403f30: if the chain head is not a live idle record (type 0xe),
 * prepend one so the idle poller sees this entity. */
static void attach_idle_record(EntitySlot* e)
{
    if (e->cmd_head == NULL || cmd_rec_type(e->cmd_head) != 0xe) {
        void* rec = cmd_alloc(0xe);
        if (rec != NULL) {
            cmd_rec_set_next(rec, e->cmd_head);
            e->cmd_head = rec;
        }
    }
}

/* FUN_0043a960 half: type-1 idle registration into DAT_004eabcc, max 100
 * (DAT_004ead5c). The tile == '\t' probe needs the path-grid mirror and is
 * TODO(corpus); the capacity rule and table are byte-exact. */
#define AI_IDLE_TABLE_MAX 100
static EntitySlot* s_idle100[AI_IDLE_TABLE_MAX];
static int s_idle100_n;

int ai_idle_count(void) { return s_idle100_n; }

static void idle100_register(EntitySlot* e)
{
    for (int i = 0; i < s_idle100_n; ++i) {
        if (s_idle100[i] == e) {
            return;
        }
    }
    if (s_idle100_n < AI_IDLE_TABLE_MAX) {
        s_idle100[s_idle100_n++] = e;
    }
}

/* Finish the chain-head command on `e`: unlink from both structures, free,
 * advance the chain. */
static int finish_head(GameContext* ctx, EntitySlot* e)
{
    void* rec = e->cmd_head;
    e->cmd_head = cmd_rec_next(rec);
    cmd_global_unlink(rec);
    cmd_free(rec, 0);
    (void)ctx;
    return 1;
}

/* Append a waypoint to the entity path list (the corpus +0x24 buffer the
 * pathfinder fills; port-side pathfinding is TODO(corpus), so move commands
 * seed this list with their single hop). A finished-and-fully-popped list
 * restarts, mirroring the buffer being rebuilt per path command. */
int ai_path_append(EntitySlot* e, float x, float y)
{
    if (e == NULL) {
        return -1;
    }
    if (e->wp_index >= e->wp_count) {
        e->wp_count = 0; /* previous path popped whole: reuse the buffer */
        e->wp_index = 0;
    }
    if (e->wp_count >= AI_WAYPOINTS_MAX) {
        return -1;
    }
    e->wp[e->wp_count].x = x;
    e->wp[e->wp_count].y = y;
    e->wp[e->wp_count].pending = 1;
    return e->wp_count++;
}

/* Move/target step: move `e` toward (tx,ty) by speed*dt; 1 when arrived. */
static int step_toward(EntitySlot* e, float tx, float ty, float dt)
{
    float dx = tx - e->x;
    float dy = ty - e->y;
    float len = (float)__builtin_sqrtf(dx * dx + dy * dy);
    if (len <= 0.001f) {
        e->x = tx;
        e->y = ty;
        return 1;
    }
    float step = AI_MOVE_SPEED_CPS * dt;
    if (step >= len) {
        e->x = tx;
        e->y = ty;
        return 1;
    }
    e->x += (dx / len) * step;
    e->y += (dy / len) * step;
    return 0;
}

/* Move step over the entity path list — the corpus move branch's shape
 * (callback 0x43cb98: walk +0x24 buffer at the +0x2c index; a null list
 * falls back to the command's coordinates at 0x43cbb1). Arrival runs the
 * corpus pop: the entry's pending byte is cleared (0x43cd90), no entry
 * removal. When the list runs out the corpus arrives — +0x3e0 bit 0x8 is
 * cleared (0x43cd66) and the command completes; FUN_00403d10(ent,8,0,1)'s
 * record-list unlink and FUN_00432e00(cell,0)'s tile-occupancy clear are
 * TODO(corpus) (the port does not model the +0xbc record list or tile
 * flags). Returns 1 only when the whole path is walked. */
static int step_path(EntitySlot* e, float fallback_x, float fallback_y,
                     float dt)
{
    if (e->wp_count == 0) {
        return step_toward(e, fallback_x, fallback_y, dt);
    }
    while (e->wp_index < e->wp_count && e->wp[e->wp_index].pending == 0) {
        ++e->wp_index; /* skip popped entries */
    }
    if (e->wp_index >= e->wp_count) {
        return 1; /* path exhausted: arrived */
    }
    if (!step_toward(e, e->wp[e->wp_index].x, e->wp[e->wp_index].y, dt)) {
        return 0; /* still walking the current entry */
    }
    e->wp[e->wp_index].pending = 0; /* pop = flag clear (0x43cd90) */
    ++e->wp_index;
    return e->wp_index >= e->wp_count;
}

/* Yield a task's deposit into the mission counters by priority kind, and
 * report the incident on the message bus (finding 22 §1). */
static void task_deposit(GameContext* ctx, int kind)
{
    switch (kind) {
    case 0: /* Crystal */
        mission_set_crystals(ctx, ctx->mission.crystals + AI_TASK_YIELD);
        mission_incident(ctx, 0); /* CrystalFound */
        break;
    case 1: /* Ore */
    case 2: /* DefaultCollect */
        mission_set_ore(ctx, ctx->mission.ore + AI_TASK_YIELD);
        mission_incident(ctx, 1); /* OreSeamFound */
        break;
    case 4: /* Construction */
        ctx->mission.constructions += AI_TASK_YIELD;
        mission_incident(ctx, 8); /* Constructed */
        break;
    default:
        break; /* other priorities have no counter effect in this subset */
    }
}

/* The verified callback branches, in corpus order. Returns 1 when the
 * corpus's callback would have returned early (walker moves on). */
static int callback_branches(GameContext* ctx, EntitySlot* e, float dt)
{
    /* 0x43c845 — activity completion: +0x2e8 zeroed, then FUN_00447c40's
     * bookkeeping (early return when current == last), then flag clear. */
    if ((e->flags_3ec & ENT_FLAG3EC_ACTIVITY_DONE) != 0) {
        int current = AI_ACTIVITY_NONE; /* +0x2e8 was zeroed pre-call */
        if (current != e->activity_last) {
            /* FUN_00403f30: the zeroed name is outside the eight-name
             * exception set, so the idle record attach always runs here. */
            attach_idle_record(e);
            /* per-type runner progress: object layer, TODO(corpus) */
            e->activity_last = AI_ACTIVITY_NONE; /* +0xbb = +0x2e8 (0) */
            e->activity_timer = 0.0f;            /* +0xb5 / +0x2d4 */
        }
        e->activity = AI_ACTIVITY_NONE;
        e->flags_3ec &= ~ENT_FLAG3EC_ACTIVITY_DONE;
    }

    /* 0x43c870 — wake timer: if +0x3e4 bit 0x8000000 is set, tick +0x368
     * down (FUN_0044c110 wakes the attached entity once expired) and
     * RETURN — no further behaviour runs this frame.
     * Attached-entity mirror (+0x364) is TODO(corpus). */
    if ((e->flags_3e4 & ENT_FLAG3E4_WAKE_TIMER) != 0) {
        if (!(e->wake_timer > 0.0f)) {
            /* FUN_0044c110: attached mirror wake. TODO(corpus). */
        }
        e->wake_timer -= dt;
        return 1;
    }

    /* 0x43c8b2 — target acquisition: +0x3ec bit 0x40000 asks FUN_0044a200
     * for the work object and FUN_00469fc0 whether it is linkable; on
     * success FUN_00465050 creates the link (handle lands in +0x354),
     * bit 0x80000 is set, 0x40000 cleared. */
    if ((e->flags_3ec & ENT_FLAG3EC_TARGET_LINK) != 0) {
        const void* obj = (e->cmd_head != NULL)
                              ? rec_payload(e->cmd_head) : NULL;
        if (obj != NULL) {
            e->link_target = (void*)obj; /* +0x354 mirror */
            e->flags_3ec |= ENT_FLAG3EC_LINKED;
        }
        e->flags_3ec &= ~ENT_FLAG3EC_TARGET_LINK;
    }

    /* 0x43c90b — cheat word DAT_00556ed0 bit 0 (testb $0x1): when set, the
     * type-2 "can do" probe FUN_0044c530 decides flag bit 0x20; otherwise
     * the bit is cleared. The riding probe FUN_0043c440 is TODO(corpus),
     * so the mirror uses "has link" as the can-do approximation. */
    if ((ctx->cheat & 1u) != 0 && e->type == 2) {
        if (e->link_target != NULL) {
            e->flags_3ec |= ENT_FLAG3EC_VULNERABLE;
        } else {
            e->flags_3ec &= ~ENT_FLAG3EC_VULNERABLE;
        }
    } else {
        e->flags_3ec &= ~ENT_FLAG3EC_VULNERABLE;
    }

    /* 0x43c94d — type-1 idle registration: flag bit 0x40 + empty work and
     * link state -> FUN_0043a960, which moves the entity into the
     * DAT_004eabcc idle table (cap 100) when it stands on a '\t' tile.
     * Tile probe TODO(corpus); table/cap/flag-clear byte-exact. */
    if (e->type == 1 && (e->flags_3ec & ENT_FLAG3EC_IDLE_REGISTER) != 0 &&
        e->cmd_head == NULL && e->link_target == NULL) {
        e->flags_3ec &= ~ENT_FLAG3EC_IDLE_REGISTER;
        idle100_register(e);
    }
    return 0;
}

int ai_tick(GameContext* ctx, float dt)
{
    if (ctx == NULL || dt <= 0.0f) {
        return 0;
    }
    int done = 0;
    /* 1. Entity chains: the callback's flag branches, then the port-side
     *    application of chain heads (move/target stepping). Corpus order:
     *    the walker visits type blocks in table order, entities ascending. */
    for (int t = 0; t < ENTITY_TYPES; ++t) {
        for (int i = 0; i < ctx->entity_count[t]; ++i) {
            EntitySlot* e = &ctx->entity[t][i];
            if (e->type == 0) {
                continue; /* walker liveness *ent+0x408==ent mirror */
            }
            if (callback_branches(ctx, e, dt)) {
                continue; /* corpus early return (wake-timer state) */
            }
            e->activity_timer += dt;
            while (e->cmd_head != NULL) {
                void* head = e->cmd_head;
                int type = cmd_rec_type(head);
                if (type == 0) {
                    /* Move: walk the entity path list seeded by the creator.
                     * +0x3e0 bit 0x8 is the corpus mover-state marker — the
                     * arrival path clears it (callback 0x43cd66); the tick's
                     * exact gate condition is TODO(corpus), so it is
                     * mirrored, not enforced here. */
                    if (!step_path(e, cmd_rec_move_x(head),
                                   cmd_rec_move_y(head), dt)) {
                        break; /* still moving */
                    }
                    e->flags_3e0 &= ~ENT_FLAG3E0_MOVING;
                    done += finish_head(ctx, e);
                } else if (type == 2) {
                    /* Target: step toward the payload entity. */
                    const EntitySlot* tgt = (const EntitySlot*)rec_payload(head);
                    if (tgt == NULL || step_toward(e, tgt->x, tgt->y, dt)) {
                        done += finish_head(ctx, e);
                    } else {
                        break;
                    }
                } else if (type == 0xe) {
                    break; /* idle record: poller territory (finding 10) */
                } else {
                    /* Unknown record types: drop them (the task type-3 runs
                     * in the global pass below, matching CmdLinkHead). */
                    done += finish_head(ctx, e);
                }
            }
        }
    }
    /* 2. Global command list: task records (CmdLinkHead) run their yield.
     *    Move/target records in the global list are the same objects as the
     *    chain heads handled above — already unlinked when completed. */
    {
        void* rec = cmd_global_head();
        while (rec != NULL) {
            void* next = cmd_rec_next(rec);
            int type = cmd_rec_type(rec);
            if (type == 3) {
                float now = cmd_rec_now(rec);
                if (now <= 0.0f || now >= AI_TOMBSTONE) {
                    now = AI_TASK_DURATION; /* first run: seed the timer */
                }
                now -= dt;
                if (now <= 0.0f) {
                    task_deposit(ctx, cmd_rec_kind(rec));
                    cmd_global_unlink(rec);
                    cmd_free(rec, 0);
                    ++done;
                } else {
                    cmd_rec_set_now(rec, now);
                }
            }
            rec = next;
        }
    }
    return done;
}

int ai_commands_active(const GameContext* ctx)
{
    int n = 0;
    if (ctx == NULL) {
        return 0;
    }
    for (int t = 0; t < ENTITY_TYPES; ++t) {
        for (int i = 0; i < ctx->entity_count[t]; ++i) {
            const EntitySlot* e = &ctx->entity[t][i];
            for (void* rec = e->cmd_head; rec != NULL; rec = cmd_rec_next(rec)) {
                ++n;
            }
        }
    }
    return n;
}
