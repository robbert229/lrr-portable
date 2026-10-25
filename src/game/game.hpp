// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer (C++).
 *
 * The game code (entities, missions, command queue, selection — findings
 * 03/06/10) lived outside the gods98 engine. Decompiled game functions land
 * here as they are produced. This milestone ports the *mission plumbing*:
 *
 *   level.h     level state + `.map` loading pipeline (finding 07)
 *   objective.h objective/advisor config + state machine (finding 22)
 *   savegame.h  `.sav`/`.osf` persistence + checkpoint blob (finding 08)
 *
 * GameContext is the reconstruction's home for the corpus globals these
 * modules touch — DAT_005560c4 (config DB), DAT_005560d4 (level block),
 * DAT_00556ed0 (cheat/feature word), DAT_004ffbc0 (objective word) and the
 * save/objective tables — so no file-scope mutable state leaks across tests.
 * The engine owns the frame tick; the game layer owns the mission state.
 *
 * Discipline (ADR 0003/0004): plain C-shaped code, no OS-facing headers
 * below the seam (src/platform/), no heavy STL.
 */
#ifndef LEGORRR_GAME_HPP
#define LEGORRR_GAME_HPP

#include <cstdint>

#include "gods98/block.hpp"
#include "level.hpp"
#include "options.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* Stub identity string — replaced by real init when decompiled. */
const char* game_layer_name(void);

/* The 10 named mission-task slots (finding 09 §"Task record").
 * CheckpointBlobBuild copies these into the blob verbatim (stride 0x210),
 * so the save module's checkpoint carries task progress state. */
#define GAME_TASK_COUNT 10
#define GAME_TASK_STRIDE 0x210

typedef struct GameTask {
    /* Raw record, laid out at the corpus's task offsets (finding 09):
     * +0x00 flag byte (&1 completed, &2 active), +0x04 name, +0x24
     * importance (float), +0x28 quota (int). Progress (reconstruction-side)
     * lives at +0x2c so the checkpoint blob's verbatim copy stays
     * layout-faithful. */
    uint8_t data[GAME_TASK_STRIDE];
} GameTask;

/* Typed accessors over the raw task record. */
static inline uint32_t task_flags(const GameTask* t)
{
    uint32_t v; __builtin_memcpy(&v, t->data + 0x00, 4); return v;
}
static inline void task_set_flags(GameTask* t, uint32_t v)
{
    __builtin_memcpy(t->data + 0x00, &v, 4);
}
static inline const char* task_name(const GameTask* t)
{
    return (const char*)t->data + 0x04;
}
static inline void task_set_name(GameTask* t, const char* n)
{
    char* dst = (char*)t->data + 0x04;
    dst[0] = '\0';
    if (n != NULL) {
        for (int i = 0; i < 63 && n[i] != '\0'; ++i) {
            dst[i] = n[i];
            dst[i + 1] = '\0';
        }
    }
}
static inline float task_importance(const GameTask* t)
{
    float v; __builtin_memcpy(&v, t->data + 0x24, 4); return v;
}
static inline void task_set_importance(GameTask* t, float v)
{
    __builtin_memcpy(t->data + 0x24, &v, 4);
}
static inline int32_t task_quota(const GameTask* t)
{
    int32_t v; __builtin_memcpy(&v, t->data + 0x28, 4); return v;
}
static inline void task_set_quota(GameTask* t, int32_t v)
{
    __builtin_memcpy(t->data + 0x28, &v, 4);
}
static inline int32_t task_value(const GameTask* t)
{
    int32_t v; __builtin_memcpy(&v, t->data + 0x2c, 4); return v;
}
static inline void task_set_value(GameTask* t, int32_t v)
{
    __builtin_memcpy(t->data + 0x2c, &v, 4);
}
static_assert(GAME_TASK_COUNT * GAME_TASK_STRIDE <= 0x318c,
              "task block fits the checkpoint blob's 0x318c budget");

/* Per-level Reward group snapshot (finding 09 §"Init / teardown"):
 * the values CheckpointBlobBuild reads/writes. */
typedef struct GameReward {
    int     enable;      /* tri-state, 2 -> 0 on read */
    int     modifier;
    float   timer;       /* default 76.9375 */
    int     display_text;
    int     display_images;
    int     display_flics;
} GameReward;

/* Live mission-progress counters (finding 09 §"RewardScoreAdd"; the corpus
 * keeps them in the DAT_005557xx score cluster). These feed the objective
 * checker (level +0x8c/+0xa0 live counts) and the task value slots. */
typedef struct GameMission {
    int32_t crystals;       /* live crystal count               */
    int32_t ore;            /* live ore count                   */
    int32_t score;          /* mission score                    */
    int32_t constructions;  /* constructions (ConstructionObjective) */
    int32_t blocks;         /* walls/blocks built (BlockObjective)     */
} GameMission;

/* Reward (post-mission) display config (finding 09 — RewardInit keys). The
 * font/image *handles* are the render lane; the config strings and the
 * Save/Advance button hit-zones are kept for the save trigger. */
typedef struct RewardScreen {
    char  wallpaper[128];
    int   display;          /* `Display` tri-state, 2 -> 0 */
    int   centre_text;
    float scroll_speed;
    float vert_spacing;
    char  font_base[64];    /* DAT_00555b3c/config name */
    char  font_title[64];   /* DAT_00555b44 */
    char  font_back[64];    /* DAT_00555b4c */
    char  complete_text[128];
    char  failed_text[128];
    char  quit_text[128];
    int   text_pos[2];      /* `TextPos` token pair */
    /* SaveButton / AdvanceButton hit zones (corpus DAT_00555b60..b98). */
    float save_x, save_y;
    float advance_x, advance_y;
    /* Button sizes — the corpus derives these from the widget surface
     * (image dims, FUN_0047dbf0); defaults cover the demo until the
     * surface lane exists. */
    float save_w, save_h;
    float advance_w, advance_h;
} RewardScreen;

/* Entity slot model (finding 10 §"EntityPointer"): the corpus keeps four
 * typed arrays with fixed strides (type 1 vehicle 0x1ec, 2/3 minifigure
 * 0x74, 4 building/object 0x14c — exposed via entity_stride). The
 * reconstruction stores a compact record per slot — enough for placement
 * data, the counts the objective checks consume, and the command-chain
 * head (entity +0x2f0) the command subsystem hangs off. `type`/`etype`
 * carry the corpus entity-type code (1..9; placement kinds 1..4 map to the
 * four tables, other codes share the last table). */
#define ENTITY_TYPES 5 /* 4 typed tables + the special-object table (5..19) */
#define ENTITY_CAP 256
/* Selection table bound (finding 06: adds cap at 100 entries). */
#define SELECTION_MAX 100
/* Selected flag word on EntitySlot.flags (corpus entity +0xfa & 0x200000). */
#define ENTITY_SELECTED 0x200000u
/* Path-list entry (corpus entity +0x24 buffer, stride 5 dwords/entry):
 * x/y are entry[0]/entry[1] read at callback 0x43cba5. The corpus "pending"
 * byte flag sits at entry+8 (callback 0x43cd83 reads list+idx*20-12 relative
 * to +0x28); arrival clears that byte — the pop is flag-clear, not removal. */
#define AI_WAYPOINTS_MAX 16 /* port bound; the corpus list is heap-allocated */
/* Permitted-activity list bound (corpus entity +0x370 id slots / +0x384 count):
 * the inline span +0x370..+0x384 is five dwords, so an entity holds at most
 * five permitted activity ids before the setter (FUN_0044b370) recycles
 * entries — the port models the stats-block capacity as
 * GameContext.stats_activity_cap (see below). */
#define AI_ACTIVITY_SLOTS 5
/* Terrain count of the stats speed table (port bound — the corpus table
 * sits inside the +0x334 stats block with an unpinned size, indexed by the
 * signed tile-type byte; the port guards out-of-range reads). */
#define AI_STATS_TERRAIN_TYPES 16
typedef struct AiWaypoint {
    float   x, y;
    uint8_t pending; /* 1 = unarrived; arrival clears it (0x43cd90) */
} AiWaypoint;

typedef struct EntitySlot {
    int32_t type;       /* corpus entity-type code (1..9)         */
    int32_t etype;      /* same, stored for the cmd task mapping  */
    char    name[32];
    float   x, y, z;
    float   rot;
    int32_t flags;      /* reconstruction-side flag word          */
    /* --- .ol grammar (decompile-verified, FUN_0042e020/e8b0) ----- */
    char    type_name[24]; /* .ol `type` name (e.g. "SmallDigger") */
    char    driving[32];   /* .ol `driving` target object (or "")  */
    int     health;        /* .ol `health` (default 100)           */
    /* --- model-aware rendering (finding 10/30) ------------------ */
    char    model_path[64]; /* .ol `Model` key (best-effort)       */
    const void* model;      /* parsed LWO Model* (entity_model cache) */
    /* --- command layer (finding 10) --- */
    void*   cmd_head;   /* chain-head command record (entity +0x2f0) */
    uint32_t flags_3e0; /* mirror of entity +0x3e0 (bit 0x400 gates gated cmds) */
    uint32_t flags_3e8; /* mirror of entity +0x3e8 (type-6 high bit → kind) */
    /* --- entity-tick state (callback 0x43c810, finding 10) --------- */
    uint32_t flags_3e4; /* +0x3e4 (bit 0x8000000 = wake-timer running)  */
    uint32_t flags_3ec; /* +0x3ec (activity/behaviour flags)            */
    /* +0x3f0: level-state mask word. The corpus's two readers are this
     * module's selection gate (CurrentLevelStateFlagsContainMask) and the
     * unported sim-interaction update; no ported writer feeds it — a
     * zero-init mirror per the flags_3e0 precedent. */
    uint32_t flags_3f0;           /* +0x3f0 (level-state mask word)         */
    /* --- minion task-state (spawn-dispatch switch 0x438150) --------- *
     * The entity spawn dispatcher (FUN_00437d00) assigns the minion types
     * a +0x334 stats block — its "task state" — through the selector at
     * 0x438150 (ai_minion_state_for). activity_cap mirrors the state's
     * +0xcc dword, which the permitted-activity appender reads
     * (FUN_0046a060) to size each entity's list; the shipped static
     * templates are zero and take their real values from config at runtime
     * (TODO(corpus)), so the port defaults the per-entity cap to the span
     * width when a minion task state is assigned (entity_create). */
    int      task_state;    /* +0x334 role: minion task-state id, or
                             * AI_TASK_STATE_NONE (= -1) for instance stats */
    int      activity_cap;  /* state +0xcc: permitted-activity capacity;
                             * 0 = unset -> ctx->stats_activity_cap          */
    const void* stats_record; /* +0x334 record side (finding 457): the
                               * selected stats record image read by the
                               * entity record accessors in entity.hpp. NULL
                               * = the zero template — the Stats config
                               * loader that fills real records is unported
                               * (TODO(corpus)), so reads default to zero as
                               * the shipped static templates do. */
    int      activity;       /* +0x2e8: activity-table index, -1 = none */
    int      activity_last;  /* +0xbb: last completed activity, -1 none */
    int      activity_param; /* +0x2d8 (activity argument)              */
    float    activity_timer; /* +0x2d4 (run since activity set)         */
    float    wake_timer;     /* +0x368 (counts down; early-returns tick) */
    void*    link_target;    /* +0x354: acquired target-acquire link     */
    void*    wake_target;    /* +0x364: attached wake target (FUN_0044c110;
                               * the corpus fills it via the attach-pair
                               * writer 0x44c0e2, which is not ported)      */
    const void* attach_304;  /* +0x304: attached object the selection
                              * filter follows in place of the candidate   */
    const void* owner_36c;   /* +0x36c: owner pointer read by the
                              * context-selected predicate and the
                              * selection-command dispatch (pointer-sized
                              * mirror of the corpus's 32-bit word)         */
    /* --- carried-object pair (corpus +0x308/+0x324/+0x348) ---------------
     * A type-3 carrier links the object it carries through +0x308; +0x324
     * holds the carry state the detach clears, and +0x348 points at the
     * reference whose dword the detach folds into the level's field 0x9c. */
    void*    carry_link_308;  /* +0x308: carried object link            */
    int32_t  carry_state_324; /* +0x324: carry state word               */
    void*    carry_ref_348;   /* +0x348: carried reference (dword read) */
    float    field_340;      /* +0x340 float; the wake mirror sets 100.0
                              * (its general corpus readers are TODO)       */
    /* --- route-progress advance (corpus +0x344/+0x2fc, finding 448) -----
     * The corpus advances a routing progress float by a fixed step and
     * clamps it to the entity's own +0x340 bound, clearing the +0x2fc
     * link field in passing. */
    float    progress_344;   /* +0x344: routing progress float            */
    uintptr_t link_2fc;    /* +0x2fc: link field the advance clears (corpus
                                 * dword link, widened for the port) */
    /* --- path list (corpus +0x24 buffer / +0x28 count / +0x2c index) --- */
    AiWaypoint wp[AI_WAYPOINTS_MAX];
    int      wp_count;       /* +0x28: valid entries                      */
    int      wp_index;       /* +0x2c: entry being walked                 */
    /* --- permitted-activity list (corpus +0x370 ids / +0x384 count) ----- *
     * Activities the entity is permitted to run, as table indices. The
     * FUN_0044b480 scan and the FUN_0043c440 can-do probe read this list
     * (activities 4/5/7 = drill/teleport/reinforce), and FUN_0044b370 is
     * the appender/recycler that command and dispatch installs run. */
    int32_t  act_list[AI_ACTIVITY_SLOTS]; /* +0x370: valid entries first   */
    int      act_count;                   /* +0x384: valid entries, 0 = no
                                           * permission (probe false)       */
} EntitySlot;

/* Completed-objective `.osf` table (finding 08): 20-byte entries — id, ref,
 * then a 12-byte name slot (OsfAdd zeroes it, then strncpy's up to 0xb
 * characters; the file layout is 5×u32, not the "name[11]+u16 pad" the doc
 * table suggests). */
#define GAME_OSF_MAX 64
typedef struct GameOsfEntry {
    int32_t  id;
    int32_t  ref;
    char     name[12];
} GameOsfEntry;
static_assert(sizeof(GameOsfEntry) == 20, "osf entry size");

typedef struct GameContext {
    /* --- corpus globals ------------------------------------------------ */
    BlockFile*  cfg;          /* DAT_005560c4 — Lego.cfg */
    LevelState* level;        /* DAT_005560d4 — current level block */
    uint32_t    cheat;        /* DAT_00556ed0 — level cheat/feature bits */

    /* --- stats speed table (FUN_00469cc0, the `+0x334+0x24` terrain-speed
     * table behind the entity stats block) --------------------------------
     * Corpus: `*(state + 0x24 + terrain_type * 4)` — seconds to cross one
     * cell of that terrain type; the walker accumulates progress as
     * dt/entry and arrives past 1.0 (FUN_00434570 on the move branch,
     * 0x43cc35 reads the cell's type through FUN_004319b0). The stats
     * block itself (type table DAT_00502bd8, stride 336 — instance stats
     * from config, templates zero) is not ported, so the port models the
     * table as this one game-wide array indexed by the tile-type byte
     * (TODO(corpus): per-type instance stats fill). A zero entry steps at
     * AI_MOVE_SPEED_CPS: the corpus gates movement on entry != 0 behind
     * the unported +0x3e8 walker bit, so unset entries need a port-side
     * default either way. */
    float stats_speed_sec[AI_STATS_TERRAIN_TYPES];

    /* --- permitted-activity list capacity (FUN_0046a060) ----------------- *
     * Corpus: the activity setter (FUN_0044b370) sizes and recycles each
     * entity's permitted-activity list from `*(state + 0xcc)` — the dword
     * the capacity probe FUN_0046a060 reads out of the entity's `+0x334`
     * stats block. The instance-stats fill is the documented stats-block
     * TODO (config-driven, values unrecorded), so the port models the
     * capacity game-wide here; game_context_init defaults it to
     * AI_ACTIVITY_SLOTS — the corpus's inline span — so list installs
     * behave like cap == span entities. A zero capacity makes grants a
     * no-op exactly as the corpus's cap-0 entities behave. */
    int stats_activity_cap;

    /* --- save system (DAT_005574d0 + per-slot ctx) ---------------------- */
    int   save_slot;              /* DAT_005574d0 — -1 = none (defaults 5) */
    char  saves_dir[260];         /* "Saves" directory path */
    GameOptions options;          /* DAT_00557030/34/3c — functional Options/
                                   * Advanced values (speed/sfx/music/bright/
                                   * help; issue #85), seeded by the active
                                   * save slot, persisted on commit          */
    int   current_record;         /* checkpoint_step: level -> record index */
    int   active_slot;            /* slot of `records` (ctx +0x18 side) */
    int   record_count;           /* ctx +0x18 */
    uint8_t* records;             /* ctx +0x34 (record array) */
    uint8_t* checkpoint_blob;     /* DAT_00552994 (12 688 B) */

    /* --- completed-objectives table (DAT_004ffe50..) -------------------- */
    GameOsfEntry osf[GAME_OSF_MAX];
    int   osf_count;              /* DAT_004ffe54 */
    int   osf_capacity;           /* DAT_004ffe58 */
    int   osf_dirty;              /* DAT_004ffe64 */
    GameOsfEntry osf_shadow[GAME_OSF_MAX];
    int   osf_shadow_count;       /* DAT_004ffe60 */
    int   osf_shadow_dirty;       /* DAT_004ffe64 (snapshot) */

    /* --- mission task table (DAT_00552990) + reward snapshot ------------ */
    GameTask tasks[GAME_TASK_COUNT]; /* stride 0x210, base self-ref at +0x1CE8 */
    GameReward reward;
    GameMission mission;         /* live progress counters                 */
    RewardScreen reward_screen;  /* Reward display keys (finding 09)        */
    int   crystal_quota;         /* DAT_005007ec (Quota.Crystals, <= 26)   */
    int   reward_active;         /* reward overlay shown (reconstruction)  */

    /* --- entity table (finding 10; object list placements) -------------- */
    EntitySlot   entity[ENTITY_TYPES][ENTITY_CAP]; /* typed entity slots  */
    int          entity_count[ENTITY_TYPES];
    int          entity_placed;  /* objects placed from the .ol this level */

    /* --- .ol TVCamera placement (FUN_0042e020 type -1 branch) ----------- */
    int   ol_camera_set;      /* a TVCamera record was placed (last wins) */
    float ol_camera_x, ol_camera_y, ol_camera_heading;

    /* --- selection manager (finding 06) ----------------------------------- */
    EntitySlot*  selection[SELECTION_MAX]; /* DAT_004fef2c array + count   */
    int          selection_count;           /* DAT_004ff298 */

    /* --- command-driven hooks -------------------------------------------- */
    /* Held-key probe for the executor's cheat-keybind pre-pass
     * (SelectionRoot step 1); set by the frame loop to read the input
     * tables. NULL disables the pre-pass. */
    int (*input_keyheld)(int key);
} GameContext;

/* Reset a GameContext to a known-fresh state (zeroed; cfg untouched). */
void game_context_init(GameContext* ctx);

/* Root identity string for the reconstruction log. */
const char* game_identity(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_HPP */
