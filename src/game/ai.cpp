// SPDX-License-Identifier: LGPL-3.0-or-later
/* AI execution receivers — port of the FUN_0043b270 entity-tick callback
 * (entry 0x43c810), decompile-verified 2026-10-21 (finding 10 closed). See
 * ai.hpp for the anatomy; branch comments carry corpus addresses. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ai.hpp"
#include "cmd.hpp"
#include "entity.hpp"
#include "level.hpp"
#include "mission.hpp"
#include "select.hpp"
#include "gods98/block.hpp"

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
#define ENT_FLAG3EC_ACTIVITY_OK    0x00000004u  /* +0x3ec bit 0x4: the list
                                                 * permits a 4/5/7 activity;
                                                 * written by FUN_0044b370 */
#define ENT_FLAG3E4_WAKE_TIMER     0x08000000u  /* +0x368 runs, tick early-return */
#define ENT_FLAG3E4_ACTIVITY_GRANT 0x00000040u  /* +0x3e4 bit 0x40: deferred
                                                 * command-grant install
                                                 * (callback 0x43da62) */

static const void* rec_payload(const void* rec) { return cmd_rec_payload(rec); }

/* DAT_004decc4 activity-name table, per the initializer at 0x436c2f. */
static const char* const s_activity_initial[AI_ACTIVITY_COUNT] = {
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

static const char* s_activity[AI_ACTIVITY_COUNT];
static int s_activity_initialized;

/* ActivityNameTableInit (FUN_00436c20): the corpus first clears the large
 * name-map backing store and then installs the 77 activity labels. The port
 * owns only the activity-name slice, so this initializes that same slice and
 * leaves unrelated tables to their own owners. */
// FUNCTION: LEGORR 0x00436C20
void ai_activity_name_table_initialize(void)
{
    int i;
    for (i = 0; i < AI_ACTIVITY_COUNT; ++i) {
        s_activity[i] = s_activity_initial[i];
    }
    s_activity_initialized = 1;
}

const char* ai_activity_name(int id)
{
    if (!s_activity_initialized) {
        ai_activity_name_table_initialize();
    }
    return (id >= 0 && id < AI_ACTIVITY_COUNT) ? s_activity[id] : NULL;
}

// FUNCTION: LEGORR 0x004409e0
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

static void activity_register_bookkeeping(EntitySlot* e);

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

/* EntityKind6TickDownMarkerSpawn (FUN_004489a0): the original uses the
 * type-13 object factory, applies the entity pose and attaches the
 * `TickDown` scene action, then raises the marker flag. The reconstruction
 * has no scene-action resource owner yet, so the type-13 entity and flag
 * are modeled while the visual action remains absent. */
// FUNCTION: LEGORR 0x004489a0
int entity_kind6_tick_down_marker_spawn(GameContext* ctx, EntitySlot* e)
{
    if (ctx == NULL || e == NULL || ai_activity_scan(e, 6) == 0) {
        return 0;
    }
    int index = entity_create(ctx, 13, "TickDown", e->x, e->y, e->z,
                              e->rot);
    if (index < 0) {
        return 0;
    }
    EntitySlot* marker = (EntitySlot*)entity_at(ctx, 13, index);
    if (marker == NULL) {
        return 0;
    }
    marker->flags_3e8 |= 0x10000u;
    return 1;
}

/* RemoveObjectIfClass6 (FUN_0041f840) performs the activity-6 scan and
 * raises the tick-down marker when available; the corpus returns zero
 * regardless of whether the marker was created. */
// FUNCTION: LEGORR 0x0041f840
int entity_remove_object_if_class6(GameContext* ctx, EntitySlot* e)
{
    if (e != NULL && ai_activity_scan(e, 6) != 0) {
        entity_kind6_tick_down_marker_spawn(ctx, e);
    }
    return 0;
}

#define BUBBLE_ACTION_NAME_COUNT 39
#define BUBBLE_ACTION_NAME_CAPACITY 64
static char s_bubble_action_names[BUBBLE_ACTION_NAME_COUNT]
                                  [BUBBLE_ACTION_NAME_CAPACITY];
// GLOBAL: LEGORR 0x00557864
static uint32_t s_bubble_texture_handles[BUBBLE_ACTION_NAME_COUNT];
static BubbleTextureResourceLoadFn s_bubble_texture_load;
static BubbleTextureInitializeFn s_bubble_texture_initialize;
static void* s_bubble_texture_context;
static uint32_t s_bubble_config_global_flags;

int bubble_action_name_register(int index, const char* name)
{
    if (index < 0 || index >= BUBBLE_ACTION_NAME_COUNT || name == NULL) {
        return 0;
    }
    size_t n = strlen(name);
    if (n >= BUBBLE_ACTION_NAME_CAPACITY) {
        n = BUBBLE_ACTION_NAME_CAPACITY - 1u;
    }
    memcpy(s_bubble_action_names[index], name, n);
    s_bubble_action_names[index][n] = '\0';
    return 1;
}

/* BubbleActionNameToIndex (FUN_00407300): the source scans 39 configured
 * strings with case-insensitive comparison. The config loader is still a
 * separate stub, so unregistered rows are skipped instead of dereferenced. */
// FUNCTION: LEGORR 0x00407300
int bubble_action_name_to_index(const char* name)
{
    if (name == NULL) {
        return -1;
    }
    for (int i = 0; i < BUBBLE_ACTION_NAME_COUNT; ++i) {
        const char* candidate = s_bubble_action_names[i];
        if (candidate[0] == '\0') {
            continue;
        }
        const unsigned char* a = (const unsigned char*)candidate;
        const unsigned char* b = (const unsigned char*)name;
        while (*a != 0u && *b != 0u) {
            int ca = *a;
            int cb = *b;
            if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
            if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';
            if (ca != cb) break;
            ++a;
            ++b;
        }
        if (*a == *b) {
            return i;
        }
    }
    return -1;
}

void bubble_texture_load_hooks_set(BubbleTextureResourceLoadFn load,
                                   BubbleTextureInitializeFn initialize,
                                   void* context)
{
    s_bubble_texture_load = load;
    s_bubble_texture_initialize = initialize;
    s_bubble_texture_context = context;
}

void bubble_config_global_flags_set(uint32_t flags)
{
    s_bubble_config_global_flags = flags;
}

/* BubbleConfigTextureLoad (FUN_00407240): walk Bubbles entries, filter
 * names with the global 0x02000000 gate, map the action to its fixed slot,
 * and retain the loaded resource handle before initializing it. */
// FUNCTION: LEGORR 0x00407240
void bubble_config_texture_load(BlockFile* cfg)
{
    BlockRec* group = block_subgroup(cfg, "Bubbles");
    BlockRec* entry = block_first_child(group);
    while (entry != NULL) {
        const char* name = block_name(entry);
        const char* resource_name = block_value(entry);
        int enabled = 1;
        if (name != NULL && name[0] == '!') {
            if ((s_bubble_config_global_flags & 0x02000000u) != 0u) {
                enabled = 0;
            }
            else {
                ++name;
            }
        }
        if (enabled && name != NULL && resource_name != NULL
            && s_bubble_texture_load != NULL) {
            const int index = bubble_action_name_to_index(name);
            if (index >= 0 && index < BUBBLE_ACTION_NAME_COUNT) {
                const uint32_t handle = s_bubble_texture_load(
                    s_bubble_texture_context, resource_name, 0, 0);
                s_bubble_texture_handles[index] = handle;
                if (handle != 0u && s_bubble_texture_initialize != NULL) {
                    s_bubble_texture_initialize(s_bubble_texture_context,
                                                handle);
                }
            }
        }
        entry = block_next_sibling(entry);
    }
}

uint32_t bubble_texture_handle_peek(int index)
{
    return (index >= 0 && index < BUBBLE_ACTION_NAME_COUNT)
        ? s_bubble_texture_handles[index] : 0u;
}

/* FUN_00403f30: if the chain head is not a live idle record (type 0xe),
 * prepend one so the idle poller sees this entity. */
// FUNCTION: LEGORR 0x00403f30
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

/* FUN_0043a960's idle table, DAT_004eabcc, max 100 (DAT_004ead5c). */
#define AI_IDLE_TABLE_MAX 100
// GLOBAL: LEGORR 0x004eabcc
static EntitySlot* s_idle100[AI_IDLE_TABLE_MAX];
// GLOBAL: LEGORR 0x004ead5c
static int s_idle100_n;

int ai_idle_count(void) { return s_idle100_n; }

static const LevelState* grid_of(const GameContext* ctx);

/* Port-side view of the terrain speed table. The source binary's indexed
 * component getter (FUN_00469cc0) is mirrored by
 * entity_stats_float_at24_indexed; this higher-level context adapter adds
 * the port's bounds and fallback policy. */
float ai_stats_speed(const GameContext* ctx, int tile_type)
{
    const float def = 1.0f / AI_MOVE_SPEED_CPS;
    if (ctx == NULL || tile_type < 0 || tile_type >= AI_STATS_TERRAIN_TYPES ||
        ctx->stats_speed_sec[tile_type] <= 0.0f) {
        return def;
    }
    return ctx->stats_speed_sec[tile_type];
}

/* --- permitted-activity list (+0x370 ids / +0x384 count) -------------------
 * Corpus shape, pinned from the wake/riding decompile bundles:
 *   FUN_0044b480 scan -> FUN_0043c440 (4/5/7) -> FUN_0044c530 (type-2 gate)
 *   FUN_0044b370 setter — append/replace/recycle, callers 0x437eb6 (spawn
 *   dispatch, type-2 arm) and 0x43da62 (deferred command install). */

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

/* FUN_0044c530: "can do" — only type-2 entities answer, and only from
 * their permitted-activity list. */
// FUNCTION: LEGORR 0x0044c530
int ai_can_do(const EntitySlot* e)
{
    return (e != NULL && e->type == 2) ? (ai_activity_probe(e) != 0) : 0;
}

/* The 0x438150 spawn-dispatch switch (corpus FUN_00437d00 default arm):
 * type − 8 indexes the byte selector at 0x4382ac — the shipped bytes are
 * {00 01 04 04 02 02 02 04 03 03 03 03} for types 8..0x13 — and jumps
 * through the table at 0x438298. Arms 0 (type 8) and 1 (type 9) pin the
 * +0x334 stats block to DAT_004a98f0 / DAT_004a9a40, selectors 2 and 3
 * (types 0xc, 0xd, 0xe, 0x10, 0x11, 0x12, 0x13) to DAT_004a9b90, and
 * selector 4 falls back to the instance-stats attacher FUN_00469c10.
 * These static templates are the minion "task states": 336 bytes each
 * (stride 0x150), all-zero in the file apart from the leading dword 0x30,
 * filled at runtime (config-driven, TODO(corpus) — the port models their
 * activity capacity per entity, see EntitySlot.activity_cap). */
int ai_minion_state_for(int type)
{
    static const uint8_t selector[12] = {
        0x00, 0x01, 0x04, 0x04, 0x02, 0x02, 0x02, 0x04, 0x03, 0x03, 0x03, 0x03,
    };
    int key = type - 8;
    if (key < 0 || key > 11) {
        return AI_TASK_STATE_NONE;
    }
    switch (selector[key]) {
    case 0x00: return AI_TASK_STATE_MINION_1; /* 0x438163 -> DAT_004a98f0 */
    case 0x01: return AI_TASK_STATE_MINION_2; /* 0x43816f -> DAT_004a9a40 */
    case 0x02:                                /* 0x43817b -> DAT_004a9b90 */
    case 0x03:
        return AI_TASK_STATE_MINION_3;
    default:  return AI_TASK_STATE_NONE;      /* 0x438187: instance stats */
    }
}

/* FUN_0046a060: capacity = *(*(ent+0x334) + 0xcc). The port models the
 * stats block's +0xcc as EntitySlot.activity_cap (set by the minion
 * task-state spawn; 0 = unset) falling back to the game-wide default. */
// FUNCTION: LEGORR 0x0046a060
int ai_activity_cap(const GameContext* ctx, const EntitySlot* e)
{
    int cap = (e != NULL && e->activity_cap > 0)
                  ? e->activity_cap
                  : ((ctx != NULL) ? ctx->stats_activity_cap : 0);
    if (cap > AI_ACTIVITY_SLOTS) {
        cap = AI_ACTIVITY_SLOTS; /* port bound (corpus stores inline) */
    }
    return cap;
}

// FUNCTION: LEGORR 0x0044b350
static int activity_is_special(int v) /* FUN_0044b350's {4,5,7} predicate */
{
    return v == AI_ACT_DRILL || v == AI_ACT_TELEPORT || v == AI_ACT_REINFORCE;
}

/* FUN_0044b370: install one activity grant. Corpus order (the whole body,
 * see ai.hpp): cap gate, +0x3f4 = 0 (field not ported — no reader in the
 * ported surface, TODO(corpus)), +0x3ec bit 0x4 refresh, special-entry
 * replace, FIFO append/overwrite. The corpus compares count != cap with
 * cap = FUN_0046a060(ent) = *(state+0xcc) (the stats-block instance fill is
 * config-driven; the port models it as the per-entity activity_cap, and
 * defaults through ctx->stats_activity_cap clamped to the inline span). */
// FUNCTION: LEGORR 0x0044b370
void ai_activity_grant(GameContext* ctx, EntitySlot* e, int value)
{
    if (e == NULL) {
        return;
    }
    int cap = ai_activity_cap(ctx, e);
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

/* Partial EntityLavaCrustActivityRegister (FUN_0043a960): the corpus's own
 * gate order — the type-1 test (0x43a968) first, then the tile probe that
 * decides whether the entity joins the capped idle table. Deviations: a
 * missing level grid permits registration, the original stats-bit gate
 * (0x43a976) is not modeled here, the port deduplicates table entries, and
 * a NULL entity returns instead of faulting on the corpus's type test. */
// FUNCTION: LEGORR 0x0043a960
static void ai_lava_crust_activity_register(GameContext* ctx, EntitySlot* e)
{
    if (e == NULL || e->type != 1) {
        return; /* the corpus's type gate, in place at 0x43a968 */
    }
    const LevelState* grid = grid_of(ctx);
    if (grid == NULL ||
        level_tile_type(grid, level_cell_of(e->x),
                        level_cell_of(e->y)) == TILE_TYPE_IDLE) {
        idle100_register(e);
    }
}

/* --- the object registry windows (see ai.hpp) ------------------------------ */
typedef struct {
    void*    record;
    uint32_t timer;
} AiRegSlot;

#define AI_REG_FLAGGED_SLOTS 20 /* DAT_005579e4 window */
#define AI_REG_TYPE4_SLOTS   20 /* DAT_00557b24 window */
#define AI_REG_TYPE2_SLOTS   50 /* DAT_00557bc4 window */
#define AI_REG_FLAGGED_SEED  0x42160000u
#define AI_REG_TYPE2_SEED    0x4a095440u

static AiRegSlot s_reg_flagged[AI_REG_FLAGGED_SLOTS];
static AiRegSlot s_reg_type4[AI_REG_TYPE4_SLOTS];
static AiRegSlot s_reg_type2[AI_REG_TYPE2_SLOTS];

// FUNCTION: LEGORR 0x004073a0
void entity_pointer_registries_remove(EntitySlot* record)
{
    int i;
    for (i = 0; i < AI_REG_FLAGGED_SLOTS; ++i) {
        if (s_reg_flagged[i].record == (void*)record) {
            s_reg_flagged[i].record = NULL;
        }
    }
    idle_queue_clear(record); /* window B rides the cmd idle table */
    for (i = 0; i < AI_REG_TYPE4_SLOTS; ++i) {
        if (s_reg_type4[i].record == (void*)record) {
            s_reg_type4[i].record = NULL;
        }
    }
    for (i = 0; i < AI_REG_TYPE2_SLOTS; ++i) {
        if (s_reg_type2[i].record == (void*)record) {
            s_reg_type2[i].record = NULL;
        }
    }
}

// FUNCTION: LEGORR 0x00407410
void type4_object_registry_insert(EntitySlot* record)
{
    if (record == NULL || record->etype != 4) {
        return; /* deviation: the corpus strides the type word blindly */
    }
    int i;
    for (i = 0; i < AI_REG_TYPE4_SLOTS; ++i) {
        if (s_reg_type4[i].record == (void*)record) {
            return; /* the corpus's no-duplicate rule */
        }
    }
    for (i = 0; i < AI_REG_TYPE4_SLOTS; ++i) {
        if (s_reg_type4[i].record == NULL) {
            s_reg_type4[i].record = (void*)record; /* timer word rides on */
            return;
        }
    }
    /* deviation: the corpus's bounded empty scan simply drops the record on
     * a full table; the port's scan drops it too. */
}

// FUNCTION: LEGORR 0x004074b0
void type2_timed_registry_insert(EntitySlot* record)
{
    if (record == NULL || record->etype != 2) {
        return; /* deviation: the corpus strides the type word blindly */
    }
    int i;
    for (i = 0; i < AI_REG_TYPE2_SLOTS; ++i) {
        if (s_reg_type2[i].record == (void*)record) {
            s_reg_type2[i].timer = AI_REG_TYPE2_SEED; /* re-seed the hit */
            return;
        }
    }
    for (i = 0; i < AI_REG_TYPE2_SLOTS; ++i) {
        if (s_reg_type2[i].record == NULL) {
            s_reg_type2[i].record = (void*)record;
            s_reg_type2[i].timer = AI_REG_TYPE2_SEED;
            return;
        }
    }
    /* the corpus drops the record on the full table */
}

// FUNCTION: LEGORR 0x00407510
void set_type2_object_record_value(EntitySlot* record, uint32_t value)
{
    if (record == NULL || record->etype != 2) {
        return; /* deviation: the corpus strides the type word blindly */
    }
    for (int i = 0; i < AI_REG_TYPE2_SLOTS; ++i) {
        if (s_reg_type2[i].record != NULL) {
            s_reg_type2[i].timer = value;
        }
    }
}

// FUNCTION: LEGORR 0x00407540
void flagged_object_timer_registry_insert(EntitySlot* record)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    if ((entity_capability_flags_get(record) & 0x80u) == 0u) {
        return;
    }
    int i;
    for (i = 0; i < AI_REG_FLAGGED_SLOTS; ++i) {
        if (s_reg_flagged[i].record == (void*)record) {
            s_reg_flagged[i].timer = AI_REG_FLAGGED_SEED; /* re-seed the hit */
            return;
        }
    }
    for (i = 0; i < AI_REG_FLAGGED_SLOTS; ++i) {
        if (s_reg_flagged[i].record == NULL) {
            s_reg_flagged[i].record = (void*)record;
            s_reg_flagged[i].timer = AI_REG_FLAGGED_SEED;
            return;
        }
    }
    /* the corpus drops the record on the full table */
}

static AiRegSlot* s_reg_window(int window, int* count)
{
    if (window == 0) {
        *count = AI_REG_FLAGGED_SLOTS;
        return s_reg_flagged;
    }
    if (window == 1) {
        *count = AI_REG_TYPE4_SLOTS;
        return s_reg_type4;
    }
    *count = AI_REG_TYPE2_SLOTS;
    return s_reg_type2;
}

void ai_object_registry_slot_peek(int window, int slot, void** record,
                                  uint32_t* timer)
{
    int count = 0;
    AiRegSlot* w = s_reg_window(window, &count);
    if (slot < 0 || slot >= count) {
        return;
    }
    if (record != NULL) {
        *record = w[slot].record;
    }
    if (timer != NULL) {
        *timer = w[slot].timer;
    }
}

void ai_object_registry_slot_set(int window, int slot, void* record,
                                 uint32_t timer)
{
    int count = 0;
    AiRegSlot* w = s_reg_window(window, &count);
    if (slot < 0 || slot >= count) {
        return;
    }
    w[slot].record = record;
    w[slot].timer = timer;
}

int ai_object_registry_window_count(int window)
{
    int count = 0;
    (void)s_reg_window(window, &count);
    return count;
}

void ai_object_registries_reset(void)
{
    memset(s_reg_flagged, 0, sizeof s_reg_flagged);
    memset(s_reg_type4, 0, sizeof s_reg_type4);
    memset(s_reg_type2, 0, sizeof s_reg_type2);
}

/* EntityQueueForRockfallProcessingIfEligible (FUN_0043a9e0): a pure forwarder
 * in the corpus (0x43a9e5: push e / call FUN_0043a960), which owns every
 * gate; the port forwards blind — no level is bound here, so the tile probe
 * takes its documented no-grid path. */
// FUNCTION: LEGORR 0x0043a9e0
void entity_queue_for_rockfall_processing_if_eligible(EntitySlot* e)
{
    ai_lava_crust_activity_register(NULL, e);
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

/* Append a waypoint to the entity path list (the corpus +0x24 buffer is
 * heap-grown while the path is built — the appender at 0x44693b reallocs
 * `count*20` bytes and writes {x, y, 0.5f} per walked cell). This mirrors
 * one append; the builder is ai_path_build. */
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

/* ------------------------------------------------------------------ *
 *  Wave-flood pathfinder core — the port of the corpus system behind
 *  the level+4 grid handle: the seed FUN_00430ee0 and the traceback /
 *  spread probe FUN_00430530 (decompile evidence:
 *  docs/evidence/legorr-pathfinder-build.bundle; finding 10).
 *
 *  Corpus state lives in the handle's voxel records (stride 0x48): the
 *  wave-state byte record+0 (0 = unwalked, 1 = flooded/walked, 5 =
 *  traceback-stamped) and the record+8 flag word, which carries the
 *  wave flag 0x40000 the seed raises over its northwest 2x2 footprint
 *  including the seed itself (FUN_00430ee0, record 12's offset table)
 *  and the 8 bit FUN_00430530 gates rejected cells on (`flags & 8`).
 *  The port-level equivalent of that handle is the per-build wave state
 *  below: the same state bytes and flag bits over the port grid, created
 *  by the build's install step and released by its finish reset, so the
 *  flood work state is scoped per path build and never leaks between
 *  commands. The only map-side authority it consults is the ported
 *  queries level_walkable (FUN_00432770) and level_tile_occupied
 *  (FUN_00432e00 read): an unwalkable or entity-reserved cell takes the
 *  blocked bit 8 and the wave refuses it, exactly as the corpus gate.
 *
 *  The corpus traceback roots are the routing-destination tiles — cells
 *  whose record+2 type is 8 or 10 (FUN_00430530's '\b'/'\n' branches).
 *  The port routes to a cell, not a destination tile, so the seeded
 *  target cell takes that role: traceback starts there and walks back
 *  through the wave-marked cells to the start cell. The wave pump and
 *  value-propagation queue behind the handle (FUN_0044efb0/FUN_0044f380)
 *  is modelled by the spread FIFO below; the corpus's message/flag
 *  side effects inside FUN_00430530 (FUN_00419b80, FUN_00431450, the
 *  +0x334-gated branches) belong to the unported state objects and stay
 *  TODO(corpus) like the rest of that family.
 * ------------------------------------------------------------------ */
#define WAVE_STATE_WALKED    1u        /* flooded: record+0 = 1          */
#define WAVE_STATE_TRACEBACK 5u        /* record+0 = 5 (traceback pass)  */
#define WAVE_FLAG_BLOCKED    0x00000008u /* FUN_00430530 flags & 8 gate  */
#define WAVE_FLAG_WAVE       0x00040000u /* FUN_00430ee0 wave flag       */

typedef struct WaveFlood {
    const LevelState* grid;
    int w, h;
    uint8_t*  cell;   /* wave-state byte per cell          */
    uint32_t* flags;  /* per-cell flag overlay (8 / 0x40000) */
    int*      dist;   /* wave steps from target (traceback gradient) */
    int*      queue;  /* spread FIFO; reused as the traceback stack   */
    uint8_t*  scan;   /* per-cell traceback scan cursor    */
    int       marks;  /* cells carrying the wave flag (diagnostic)    */
    int       goal;   /* seeded target cell index          */
    int       self;   /* the mover's own cell: its reservation is exempt
                         from the occupancy gate (the walker marks its
                         current cell every tick, 0x43deed) */
} WaveFlood;

/* The four spread directions the flood advances through. */
static const int s_wave_off[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };

/* The seed's flag footprint (record 12's local_20 table): a northwest
 * 2x2 block including the seed cell — (-1,-1), (0,-1), (0,0), (-1,0). */
static const int s_seed_off[4][2] = { { -1, -1 }, { 0, -1 }, { 0, 0 }, { -1, 0 } };

static int s_wave_marks; /* wave-flag count from the last path build */

int ai_wave_flood_count(void) { return s_wave_marks; }

static void wave_mark(WaveFlood* wf, int cell)
{
    if ((wf->flags[cell] & WAVE_FLAG_WAVE) == 0) {
        wf->flags[cell] |= WAVE_FLAG_WAVE;
        ++wf->marks;
    }
}

/* The blocked gate: an unwalkable or entity-reserved cell takes the
 * corpus's flags & 8 rejection bit; the ported queries are the only
 * state authority the wave consults. The mover's own cell is exempt
 * from the occupancy check: the walker raises that reservation on its
 * current cell every tick (callback 0x43deed), so rejecting it would
 * strand every retarget mid-move (PR #87 feedback). */
static int wave_gate_blocked(WaveFlood* wf, int x, int y, int cell)
{
    if (cell == wf->self) {
        return !level_walkable(wf->grid, x, y);
    }
    if (!level_walkable(wf->grid, x, y) || level_tile_occupied(wf->grid, x, y)) {
        wf->flags[cell] |= WAVE_FLAG_BLOCKED;
        return 1;
    }
    return 0;
}

/* FUN_00430ee0 seed: validate the target — the corpus drops row/column 0
 * and everything at or past its w-2/h-2 margin (unsigned), and its
 * record+0 == 0 gate is per-build state, fresh here — then stamps the
 * seed cell walked (1) and raises the wave flag 0x40000 over record 12's
 * footprint offsets, a northwest 2x2 block including the seed itself.
 * Returns 0 (and installs no state) when the target is off-handle or
 * blocked by the ported queries. */
// FUNCTION: LEGORR 0x00430ee0
static int wave_seed(WaveFlood* wf, int tx, int ty)
{
    if (tx == 0 || ty == 0 ||
        (unsigned)tx >= (unsigned)wf->w - 2u ||
        (unsigned)ty >= (unsigned)wf->h - 2u) {
        return 0;
    }
    int goal = ty * wf->w + tx;
    if (wave_gate_blocked(wf, tx, ty, goal) != 0) {
        return 0;
    }
    wf->cell[goal] = WAVE_STATE_WALKED;
    wf->goal = goal;
    for (int i = 0; i < 4; ++i) {
        int nx = tx + s_seed_off[i][0], ny = ty + s_seed_off[i][1];
        if (nx >= 0 && ny >= 0 && nx < wf->w && ny < wf->h) {
            wave_mark(wf, ny * wf->w + nx);
        }
    }
    return 1;
}

/* The wave spread: the corpus pumps the wave flag through the handle's
 * queue (FUN_0044efb0); the port models the same four-direction spread
 * with a FIFO from the seed cell. Entering a cell stamps it walked (1)
 * and raises the wave flag 0x40000 — the mark of a walked cell — while
 * blocked cells take flag 8 and the spread refuses them. The spread
 * stops once the start cell floods; that is the corpus's traceback root
 * condition (FUN_00430530 returning nonzero onto the mover). */
static void wave_spread(WaveFlood* wf, int start)
{
    int w = wf->w, h = wf->h;
    int head = 0, tail = 1;
    wf->queue[0] = wf->goal;
    while (head < tail && wf->cell[start] == 0) {
        int cell = wf->queue[head++];
        int cx = cell % w, cy = cell / w;
        for (int i = 0; i < 4 && wf->cell[start] == 0; ++i) {
            int nx = cx + s_wave_off[i][0], ny = cy + s_wave_off[i][1];
            if (nx < 0 || ny < 0 || nx >= w || ny >= h) {
                continue;
            }
            int ncell = ny * w + nx;
            if (wf->cell[ncell] != 0) {
                continue;
            }
            if (wave_gate_blocked(wf, nx, ny, ncell) != 0) {
                continue;
            }
            wf->cell[ncell] = WAVE_STATE_WALKED;
            wave_mark(wf, ncell);
            wf->dist[ncell] = wf->dist[cell] + 1;
            wf->queue[tail++] = ncell;
        }
    }
}

/* FUN_00430530 traceback: rooted at the flooded target cell, it walks
 * back over the wave's cells to the start cell, stamping every walked
 * cell with record+0 = 5 and refusing cells carrying the blocked flag 8.
 * The port follows the wave-distance gradient with backtracking, so
 * dead-end branches get stamped 5 too — the corpus stamps its explored
 * neighbours 5 in exactly the same "tried and refused" pass. Writes the
 * goal->start sequence into the reused stack (wf->queue) and returns the
 * cell count before the start cell, or 0 when the start never flooded
 * (unreachable or blocked target). */
// FUNCTION: LEGORR 0x00430530
static int wave_traceback(WaveFlood* wf, int start)
{
    if (wf->cell[start] != WAVE_STATE_WALKED) {
        return 0;
    }
    int depth = 0;
    wf->queue[0] = wf->goal;
    wf->cell[wf->goal] = WAVE_STATE_TRACEBACK;
    while (depth >= 0) {
        if (depth >= wf->w * wf->h) {
            return 0; /* safety: a walk-back longer than the grid is broken */
        }
        int cell = wf->queue[depth];
        if (cell == start) {
            return depth;
        }
        int cx = cell % wf->w, cy = cell / wf->w;
        int advanced = 0;
        for (int i = wf->scan[cell]; i < 4; ++i) {
            wf->scan[cell] = (uint8_t)(i + 1);
            int nx = cx + s_wave_off[i][0], ny = cy + s_wave_off[i][1];
            if (nx < 0 || ny < 0 || nx >= wf->w || ny >= wf->h) {
                continue;
            }
            int ncell = ny * wf->w + nx;
            if (wf->cell[ncell] != WAVE_STATE_WALKED) {
                continue; /* flooded-only: never unwalked, never traceback */
            }
            if ((wf->flags[ncell] & WAVE_FLAG_BLOCKED) != 0) {
                continue; /* the flags & 8 gate */
            }
            if (wf->dist[ncell] != wf->dist[cell] + 1) {
                continue; /* follow the wave gradient out from the target */
            }
            wf->queue[++depth] = ncell;
            wf->cell[ncell] = WAVE_STATE_TRACEBACK;
            advanced = 1;
            break;
        }
        if (advanced == 0) {
            --depth; /* backtrack: this branch is a dead end */
        }
    }
    return 0;
}

/* Build a command's path into the entity buffer through the wave-flood
 * core. Corpus shape: the command dispatch installs the list per command
 * (0x4418d0: buffer set, count/index and +0x28c/+0x290 reset, mover flag
 * raised), the builder appends one entry per walked cell (0x44693b's
 * per-cell {x, y, 0.5f} writes — the port's pending=1 mirror), and the
 * command end releases the list (0x441a50). Port-side the finish reset
 * also releases the wave state, so no flood work survives a build.
 * Returns the entry count, or 0 when the build yields no path (no grid,
 * off-handle or blocked target, unreachable, equal cells, longer than
 * AI_WAYPOINTS_MAX) and the caller seeds the one-hop fallback. */
int ai_path_build(GameContext* ctx, EntitySlot* e, float tx, float ty)
{
    s_wave_marks = 0; /* the build owns the whole flood state: start blank */
    const LevelState* grid = grid_of(ctx);
    if (e == NULL || grid == NULL) {
        return 0; /* no grid handle: the one-hop fallback owns this move */
    }
    int w = level_width(grid);
    int h = level_height(grid);
    int sx = level_cell_of(e->x);
    int sy = level_cell_of(e->y);
    int gx = level_cell_of(tx);
    int gy = level_cell_of(ty);
    if (sx < 0 || sy < 0 || gx < 0 || gy < 0 || sx >= w || sy >= h ||
        gx >= w || gy >= h) {
        return 0;
    }

    WaveFlood wf;
    memset(&wf, 0, sizeof(wf));
    wf.grid = grid;
    wf.w = w;
    wf.h = h;
    wf.self = sy * w + sx; /* the mover's own cell: exempt from its own
                              reservation, the gate still sees the map */
    int size = w * h;
    wf.cell = (uint8_t*)calloc((size_t)size, 1);
    wf.flags = (uint32_t*)calloc((size_t)size, sizeof(uint32_t));
    wf.dist = (int*)calloc((size_t)size, sizeof(int));
    wf.queue = (int*)calloc((size_t)size, sizeof(int));
    wf.scan = (uint8_t*)calloc((size_t)size, 1);

    int len = 0;
    if (wf.cell != NULL && wf.flags != NULL && wf.dist != NULL &&
        wf.queue != NULL && wf.scan != NULL) {
        if (wave_seed(&wf, gx, gy) != 0) {
            int start = sy * w + sx;
            if (wf.cell[start] == 0) {
                wave_spread(&wf, start); /* equal cells need no wave */
            }
            len = wave_traceback(&wf, start);
        }
        /* The traceback stack is wf->queue; cells [0, len) run
         * target -> start-adjacent (the start itself is excluded: the
         * entity is already there). */
        s_wave_marks = wf.marks;
    }

    int rc = 0;
    if (len > 0 && len <= AI_WAYPOINTS_MAX) {
        e->wp_count = 0; /* corpus install: fresh list per command (0x4418d0) */
        e->wp_index = 0;
        for (int i = len - 1; i >= 0; --i) {
            int cell = wf.queue[i];
            ai_path_append(e, (float)(cell % w), (float)(cell / w));
        }
        rc = len;
    }
    free(wf.cell); /* finish reset (0x441a50): the flood state is released */
    free(wf.flags);
    free(wf.dist);
    free(wf.queue);
    free(wf.scan);
    return rc;
}

/* The bound level's voxel grid, or NULL when no map is loaded (the corpus
 * tick cannot walk an empty grid, so a sub-margin grid counts as absent and
 * the port-side fallback paths run instead). */
static const LevelState* grid_of(const GameContext* ctx)
{
    if (ctx == NULL || ctx->level == NULL || level_width(ctx->level) < 3 ||
        level_height(ctx->level) < 3) {
        return NULL;
    }
    return ctx->level;
}

/* Cells/second the entity moves at *now*: the speed of the terrain under
 * foot, read the corpus way (move branch 0x43cc35–0x43cc5c: position →
 * cell via FUN_0048e060, cell → type via FUN_004319b0, type → stats entry
 * via FUN_00469cc0, where the corpus accumulates dt/entry per cell).
 * Without a bound grid the corpus cannot read terrain and the port default
 * applies. Corpus runner scaling (+0x3e8 bit 0x2000000 multiplies the
 * entry by the constant at 0x49e3b4 — value not recorded in the bundles)
 * is TODO(corpus). */
static float underfoot_cps(const GameContext* ctx, const LevelState* grid,
                           float x, float y)
{
    if (grid == NULL) {
        return AI_MOVE_SPEED_CPS;
    }
    int type = level_tile_type(grid, level_cell_of(x), level_cell_of(y));
    return 1.0f / ai_stats_speed(ctx, type);
}

/* Move/target step: move `e` toward (tx,ty) by speed*dt, where speed is
 * the terrain-under-foot rate (see underfoot_cps); 1 when arrived. */
static int step_toward(GameContext* ctx, EntitySlot* e, float tx, float ty,
                       float dt)
{
    float dx = tx - e->x;
    float dy = ty - e->y;
    float len = (float)__builtin_sqrtf(dx * dx + dy * dy);
    if (len <= 0.001f) {
        e->x = tx;
        e->y = ty;
        return 1;
    }
    float step = underfoot_cps(ctx, grid_of(ctx), e->x, e->y) * dt;
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
 * falls back to the command's coordinates at 0x43cbb1). With a grid bound,
 * the mover-state sync marks the entity's own cell occupied every tick
 * (callback 0x43deed) and the walker only steps while its *current* cell
 * is walkable (FUN_00432770, checked via 0x43cbe8; unwalkable = no step,
 * the command stays live). The step rate is the terrain-under-foot speed
 * (FUN_00469cc0, see underfoot_cps — 0x43cc35's probe on the same branch).
 * Arrival runs the corpus pop: the entry's pending
 * byte is cleared (0x43cd90), no entry removal. When the list runs out the
 * corpus arrives — +0x3e0 bit 0x8 is cleared (0x43cd66), the arrival cell's
 * occupancy flag is dropped (FUN_00432e00(cell, 0) at 0x43cd72), and the
 * command completes; FUN_00403d10(ent,8,0,1)'s record-list unlink stays
 * TODO(corpus) (the port has no +0xbc record list). Returns 1 only when
 * the whole path is walked. */
static int step_path(GameContext* ctx, EntitySlot* e, float fallback_x,
                     float fallback_y, float dt)
{
    const LevelState* grid = grid_of(ctx);
    if (grid != NULL && (e->flags_3e0 & ENT_FLAG3E0_MOVING) != 0) {
        int cx = level_cell_of(e->x), cy = level_cell_of(e->y);
        level_tile_mark((LevelState*)grid, cx, cy, 1);
        if (!level_walkable(grid, cx, cy)) {
            return 0; /* off-grid or blocked tile: no step this frame */
        }
    }
    int done;
    if (e->wp_count == 0) {
        done = step_toward(ctx, e, fallback_x, fallback_y, dt);
    } else {
        /* Path list: each entry is one cell crossed at the speed of the
         * terrain under foot (corpus 0x43cc35's read + 0x434570's per-cell
         * progress: a cell completes every speed seconds, so `dt` can
         * complete several cells on fast terrain and the overflow carries
         * into the next entries). Every completion pops in place. */
        float rem = dt;
        for (;;) {
            while (e->wp_index < e->wp_count &&
                   e->wp[e->wp_index].pending == 0) {
                ++e->wp_index; /* skip popped entries */
            }
            if (e->wp_index >= e->wp_count) {
                done = 1; /* path exhausted: arrived */
                break;
            }
            float wx = e->wp[e->wp_index].x, wy = e->wp[e->wp_index].y;
            float dx = wx - e->x, dy = wy - e->y;
            float len = (float)__builtin_sqrtf(dx * dx + dy * dy);
            if (len <= 0.001f) {
                e->wp[e->wp_index].pending = 0; /* already there: pop */
                ++e->wp_index;
                continue;
            }
            float cps = underfoot_cps(ctx, grid, e->x, e->y);
            float step = cps * rem;
            if (step >= len) {
                e->x = wx;
                e->y = wy;
                rem -= len / cps; /* time actually consumed */
                e->wp[e->wp_index].pending = 0; /* pop = flag clear (0x43cd90) */
                ++e->wp_index;
                if (e->wp_index >= e->wp_count) {
                    done = 1;
                    break;
                }
            } else {
                e->x += (dx / len) * step;
                e->y += (dy / len) * step;
                done = 0;
                break; /* still walking the current entry */
            }
        }
    }
    if (done && grid != NULL) {
        level_tile_mark((LevelState*)grid, level_cell_of(e->x),
                        level_cell_of(e->y), 0); /* arrival clear (0x43cd72) */
    }
    return done;
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

/* AttachedEntityWake (FUN_0044c110): resolve the parent's +0x364 attached
 * target, then apply its wake timer and wake-request flag. */
// FUNCTION: LEGORR 0x0044c110
static void ai_attached_entity_wake(EntitySlot* parent)
{
    EntitySlot* target = (EntitySlot*)parent->wake_target;
    if ((target->flags_3e0 & 0x00200000u) == 0) {
        target->field_340 = 100.0f;
        target->flags_3e8 |= 0x00800000u;
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
     * down and RETURN — no further behaviour runs this frame. Once the
     * timer has expired, FUN_0044c110 wakes the attached target: unless
     * it is in the attached special state (+0x3e0 bit 0x200000), it takes
     * +0x340 = 100.0 and sets +0x3e8 bit 0x800000. The corpus's +0x364
     * pointer is filled by the attach-pair writer (0x44c0e2, not ported),
     * so the port mirrors the field as-is. */
    if ((e->flags_3e4 & ENT_FLAG3E4_WAKE_TIMER) != 0) {
        if (!(e->wake_timer > 0.0f) && e->wake_target != NULL) {
            ai_attached_entity_wake(e);
        }
        e->wake_timer -= dt;
        return 1;
    }

    /* 0x43c8b2 — target acquisition: +0x3ec bit 0x40000 asks FUN_0044a200
     * for the work object and then links it. The source corpus's FUN_00469fc0
     * is a stats-record +0xc8 getter (Finding 457), not a linkability test;
     * the corpus link helper itself remains unported. The simplified port
     * mirrors the acquired target directly in +0x354 and sets bit 0x80000. */
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
     * the bit is cleared. FUN_0044c530 = "type 2 and FUN_0043c440 is
     * non-zero"; FUN_0043c440 asks FUN_0044b480's scan over the entity's
     * real permitted-activity list (+0x370 slots, count +0x384) for
     * activities 4 (drill), 5 (teleport) or 7 (reinforce) — list-driven,
     * not the single current-activity field. */
    if ((ctx->cheat & 1u) != 0 && ai_can_do(e)) {
        e->flags_3ec |= ENT_FLAG3EC_VULNERABLE;
    } else {
        e->flags_3ec &= ~ENT_FLAG3EC_VULNERABLE;
    }

    /* 0x43c94d — type-1 idle registration: flag bit 0x40 + empty work and
     * link state -> FUN_0043a960. The extracted helper owns its tile probe
     * and capped idle-table insertion. */
    if (e->type == 1 && (e->flags_3ec & ENT_FLAG3EC_IDLE_REGISTER) != 0 &&
        e->cmd_head == NULL && e->link_target == NULL) {
        e->flags_3ec &= ~ENT_FLAG3EC_IDLE_REGISTER;
        ai_lava_crust_activity_register(ctx, e);
    }

    /* 0x43da62 — deferred command grant: +0x3e4 bit 0x40 marks "the command
     * at the chain head grants an activity". The branch consumes the flag,
     * installs the record's +0x38 activity onto the entity's permitted-
     * activity list through the FUN_0044b370 setter (append, or in-place
     * replace when the grant is a special 4/5/7), drops the grant records
     * (FUN_00403d10(ent, 0x13, 0, 1) unlinks every chain record of type
     * 0x13 with no per-type side effects — cmd_chain_drop), and RETURNS:
     * both corpus paths end at 0x43ed80. The corpus writer of the +0x3e4
     * grant bit is NOT the 0x438150 switch (it writes only +0x334) — the
     * raiser is a computed-value store among the +0x3e4 write census
     * (finding 10 §"Spawn-dispatch task-state switch"), still TODO. The
     * ported list is where all these grants land. */
    if ((e->flags_3e4 & ENT_FLAG3E4_ACTIVITY_GRANT) != 0) {
        e->flags_3e4 &= ~ENT_FLAG3E4_ACTIVITY_GRANT;
        if (e->cmd_head != NULL) {
            ai_activity_grant(ctx, e, cmd_rec_activity(e->cmd_head));
        }
        cmd_chain_drop(e, 0x13); /* corpus 0x43da92: FUN_00403d10(ent, 0x13, 0, 1) */
        return 1;
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
                    if (!step_path(ctx, e, cmd_rec_move_x(head),
                                   cmd_rec_move_y(head), dt)) {
                        break; /* still moving */
                    }
                    e->flags_3e0 &= ~ENT_FLAG3E0_MOVING;
                    done += finish_head(ctx, e);
                } else if (type == 2) {
                    /* Target: step toward the payload entity. */
                    const EntitySlot* tgt = (const EntitySlot*)rec_payload(head);
                    if (tgt == NULL || step_toward(ctx, e, tgt->x, tgt->y, dt)) {
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

/* SceneActivityUpdateDispatch (FUN_00447c40): ai_set_activity already
 * stores the current activity in both mirrored fields at the existing
 * callsites, so the common call returns early. Preserve the change path for
 * callers that update the current id directly. */
// FUNCTION: LEGORR 0x00447c40
static void activity_register_bookkeeping(EntitySlot* e)
{
    if (e == NULL || e->activity == e->activity_last) return;
    attach_idle_record(e);
    e->activity_last = e->activity;
    e->activity_timer = 0.0f;
}

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

/* --- the entity activity scene-state leaf (see ai.hpp) ---------------------
 * DAT_004dee14's observed halves ride here as a zero-init static; the
 * corpus reads its global level word through the threaded GameContext. */
static uint32_t s_entity_state_flags;

// FUNCTION: LEGORR 0x00439990
void entity_activity_scene_state_update(GameContext* ctx)
{
    if ((s_entity_state_flags & 0x10u) != 0u) {
        s_entity_state_flags |= 8u;
        return;
    }
    s_entity_state_flags |= 4u;
    global_object_field98_clear(ctx != NULL ? ctx->level : NULL);
}

uint32_t entity_state_flags_peek(void) { return s_entity_state_flags; }
void     entity_state_flags_set(uint32_t value) { s_entity_state_flags = value; }

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

/* --- the entity activity gate band (see ai.hpp) ---------------------------- */

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
        attach_idle_record(e);       /* FUN_00403f30 */
    }
}

/* --- random activity attempt (2026-10-24) --------------------------------- */

/* VC5 rand.obj owns the target RNG word in the classic x86 build; other
 * builds keep a module-local LCG model for this subsystem. */
#if !defined(_MSC_VER) || (_MSC_VER >= 1200) || !defined(_M_IX86)
static uint32_t s_activity_rand_state = 1;
#endif

static int activity_random_draw(void)
{
#if defined(_MSC_VER) && (_MSC_VER < 1200) && defined(_M_IX86)
    return rand();
#else
    s_activity_rand_state = s_activity_rand_state * 214013u + 2531011u;
    return (int)((s_activity_rand_state >> 16) & 0x7fffu);
#endif
}

/* TryRandomEntityActionFromCodeRange (FUN_00443670): up to five draws
 * land in the activity-code band 0x38..0x4c — random_next's 15-bit word
 * mapped through % 0x15 + 0x38 — and each draw installs its code through
 * the activity-set fold (FUN_004409e0 with FUN_00447c40's registration)
 * and asks the dispatch whether the activity took. Deviation: the
 * corpus's dispatch return is the per-type runner result (the scene
 * layer, unported); the deterministic clause the port takes as success
 * is the dispatch's early return — 0 when the current name still equals
 * the last bookkept name, so the attempt succeeds exactly when the code
 * differs from what was last registered. On success the entity raises
 * its +0x3e0 bit 0x8000 and answers 1; five same-name attempts answer 0.
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
        const short roll = (short)activity_random_draw();
        const int code = (int)roll % 0x15 + 0x38;
        const int prev = e->activity_last;
        ai_set_activity(e, code, 0); /* FUN_004409e0; 0x447c40 folds here */
        if (e->activity_last != prev) {
            e->flags_3e0 |= 0x8000u;
            return 1;
        }
    }
    return 0;
}

/* --- the entity counter + link band (see ai.hpp) --------------------------- */

// FUNCTION: LEGORR 0x0043a8f0
void simulation_update_counter_add(GameContext* ctx, int count)
{
    if (ctx == NULL) {
        return; /* deviation: the corpus strides its global level word */
    }
    for (; count != 0; count = count - 1) {
        global_object_field8c_increment(ctx->level);
    }
    entity_activity_scene_state_update(ctx);
}

// FUNCTION: LEGORR 0x00438580
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

// FUNCTION: LEGORR 0x004440a0
void linked_entity_flag_and_command_update(EntitySlot* e)
{
    if (e == NULL) {
        return; /* deviation: the corpus strides blindly */
    }
    EntitySlot* child = (EntitySlot*)(uintptr_t)e->link_2fc;
    if (child == NULL || child->type != 4) {
        return;
    }
    e->flags_3ec |= 0x40u;
    child->flags_3ec |= 0x40u;
    cmd_chain_drop(e, 0x1c);
}

/* --- the idle-tile update band (2026-10-25) -------------------------------- */

/* port hook: clear the idle-registration table (unowned). */
void ai_idle_table_reset(void)
{
    for (int i = 0; i < s_idle100_n; ++i) {
        s_idle100[i] = NULL;
    }
    s_idle100_n = 0;
}

/* TrackedEntityPointerArrayRemove (FUN_0043a930) — see ai.hpp. The corpus
 * strides the DAT_004eabcc window up to its count word blindly; the port
 * rides the s_idle100 model's used window (the corpus keeps walking past
 * a hit, pinned). */
// FUNCTION: LEGORR 0x0043a930
void ai_idle_table_pointer_remove(void* pointer)
{
    for (int i = 0; i < s_idle100_n; ++i) {
        if (s_idle100[i] == pointer) {
            s_idle100[i] = NULL;
        }
    }
}

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
            e->link_2fc = (uintptr_t)link;          /* entity +0x2fc */
            linked_entity_flag_and_command_update(e); /* FUN_004440a0 */
            s_idle100[i] = NULL;                    /* unregister */
        }
    }
}

/* EntityActivityTerrainEligibilityTransition (FUN_00449980) — see ai.hpp.
 * The corpus's two coordinate arguments arrive through the x87 stack and
 * land as __ftol'd integers; the port takes them truncated (the crt
 * precedent). Deviations: guarded entity/level words where the corpus
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
    const int ix = (int)x;
    const int iy = (int)y;
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

/* ActiveEntityListContainsCode (FUN_004031e0): a permitted-activity scan
 * (FUN_0044b480) over the selection when it is non-empty, else over the
 * corpus's global active-entity list. The selection branch caches the
 * count (the corpus's ebx), walks the record array with a pointer and an
 * unsigned counter (add reg,4 / inc / cmp-jb) and passes entries BLIND to
 * the scan — the corpus has no NULL-entry test there, the scan itself
 * answers 0; the fallback branch skips NULL entries (the corpus's
 * test/je) and re-reads the count every iteration. The selection branch
 * answers on the first hit and never falls back to the list
 * (pinned branch isolation).
 * Deviation: the global list (DAT_004b335c entries / DAT_004b3424 count)
 * has no ported owner — the activation band lands through hooks — so it
 * rides zero-init own storage behind install/count hooks with the
 * 50-entry window the corpus region's stride suggests (the corpus strides
 * its bounds blindly); the corpus strides a NULL selection entry into the
 * activity scan, the port's scan answers 0 for it (guarded). */
#define ACTIVE_ENTITY_LIST_MAX 50 /* corpus region DAT_004b335c..3424 */
// GLOBAL: LEGORR 0x004b335c
static EntitySlot* s_active_list[ACTIVE_ENTITY_LIST_MAX];
// GLOBAL: LEGORR 0x004b3424
static int s_active_count;

void active_entity_list_install(const EntitySlot* const* entries, int count)
{
    if (entries == NULL || count < 0) {
        count = 0;
    }
    if (count > ACTIVE_ENTITY_LIST_MAX) {
        count = ACTIVE_ENTITY_LIST_MAX; /* deviation: window guard */
    }
    int i;
    for (i = 0; i < count; ++i) {
        s_active_list[i] = (EntitySlot*)entries[i];
    }
    for (i = count; i < ACTIVE_ENTITY_LIST_MAX; ++i) {
        s_active_list[i] = NULL;
    }
    s_active_count = count;
}

int active_entity_list_count(void)
{
    return s_active_count;
}

// FUNCTION: LEGORR 0x004031e0
int active_entity_list_contains_code(GameContext* ctx, int code)
{
    unsigned count;
    EntitySlot* const* records;
    EntitySlot* const* walk;
    unsigned i;

    if (selection_has_any(ctx) == 0) {
        /* the fallback re-reads the count (the corpus's DAT_004b3424
         * load) on every iteration and skips NULL entries (test/je). */
        for (i = 0; i < (unsigned) s_active_count; ++i) {
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
