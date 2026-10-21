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
} RewardScreen;

/* Entity slot model (finding 10 §"EntityPointer"): the corpus keeps four
 * typed arrays with fixed strides (type 1 vehicle 0x1ec, 2/3 minifigure
 * 0x74, 4 building/object 0x14c — exposed via entity_stride). The
 * reconstruction stores a compact record per slot — enough for placement
 * data and the counts the objective checks consume; the full stride-length
 * sim record is the future entity layer. */
#define ENTITY_TYPES 4
#define ENTITY_CAP 256
typedef struct EntitySlot {
    int32_t type;       /* 1..4 */
    char    name[32];
    float   x, y, z;
    float   rot;
    int32_t flags;      /* reconstruction-side flag word */
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

    /* --- save system (DAT_005574d0 + per-slot ctx) ---------------------- */
    int   save_slot;              /* DAT_005574d0 — -1 = none (defaults 5) */
    char  saves_dir[260];         /* "Saves" directory path */
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

    /* --- entity table (finding 10; object list placements) -------------- */
    EntitySlot   entity[ENTITY_TYPES][ENTITY_CAP]; /* typed entity slots  */
    int          entity_count[ENTITY_TYPES];
    int          entity_placed;  /* objects placed from the .ol this level */
} GameContext;

/* Reset a GameContext to a known-fresh state (zeroed; cfg untouched). */
void game_context_init(GameContext* ctx);

/* Root identity string for the reconstruction log. */
const char* game_identity(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_HPP */
