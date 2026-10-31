// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gods98/block.hpp"
#include "gods98/message.hpp"
#include "crt/crt.hpp"
#include "entity.hpp"
#include "level.hpp"
#include "mission.hpp"
#include "objective.hpp"
#include "savegame.hpp"
#include "ui_list.hpp"
#include "effects.hpp"
#include "../gods98/sound.hpp"
#include "util.hpp"

/* The ten named task slots (TaskBlockInit local list). */
static const char* TASK_NAMES[GAME_TASK_COUNT] = {
    "Crystals", "Ore", "Diggable", "Constructions", "Caverns",
    "Figures", "RockMonsters", "Oxygen", "Timer", "Score",
};

#define MISS_CRYSTAL_QUOTA 26 /* crystal display budget (CrystalQuotaSetup) */

/* The reference entry points carry no context pointers. The game/runtime
 * owner must bind its active reward state and engine callbacks before calling
 * those ABI adapters. */
static RewardScreenState* s_reward_screen_state;
static const RewardScreenOps* s_reward_screen_ops;
static RewardScreenState s_reward_screen_default_state;
static void reward_task_flics_close_all(RewardScreenState* state);

/* Path helper: level-scoped `Reward [<sub>] [<name>]` key, resolved via
 * util_level_cfg_path (Levels container first, flat fallback). */
static void rw_path(char* out, size_t out_sz, BlockFile* cfg,
                    const char* level, const char* sub, const char* name)
{
    char suffix[400]; /* "Reward" + optional " <sub>" + optional " <name>" */
    size_t pos = 0;
    pos += snprintf(suffix + pos, sizeof(suffix) - pos, "Reward");
    if (sub != NULL) {
        pos += snprintf(suffix + pos, sizeof(suffix) - pos, " %s", sub);
    }
    if (name != NULL) {
        pos += snprintf(suffix + pos, sizeof(suffix) - pos, " %s", name);
    }
    (void)pos;
    if (util_level_cfg_path(cfg, level, suffix, out, out_sz) == NULL) {
        out[0] = '\0';
    }
}

static const char* rw_str(BlockFile* cfg, const char* root, const char* level,
                          const char* sub, const char* name)
{
    (void)root;
    char path[512];
    rw_path(path, sizeof(path), cfg, level, sub, name);
    return block_get_strptr(cfg, path);
}

static int rw_int(BlockFile* cfg, const char* root, const char* level,
                  const char* sub, const char* name, int dflt)
{
    (void)root;
    char path[512];
    rw_path(path, sizeof(path), cfg, level, sub, name);
    return block_get_int(cfg, path, dflt);
}

static float rw_float(BlockFile* cfg, const char* root, const char* level,
                      const char* sub, const char* name, float dflt)
{
    (void)root;
    char path[512];
    rw_path(path, sizeof(path), cfg, level, sub, name);
    return block_get_float(cfg, path, dflt);
}

/* Corpus FUN_004790d0 delegates to ConfigBooleanGet @ 0x477710. */
static int rw_typed(BlockFile* cfg, const char* root, const char* level,
                    const char* name)
{
    (void)root;
    char path[512];
    rw_path(path, sizeof(path), cfg, level, NULL, name);
    return block_get_bool(cfg, path);
}

/* Store a short token string at a task payload offset (bounded write). */
static void task_put_str(GameTask* t, int off, const char* s, int cap)
{
    char* dst = (char*)t->data + off;
    dst[0] = '\0';
    if (s == NULL || cap <= 1) {
        return;
    }
    size_t copy_length = strlen(s);
    if (copy_length >= (size_t)cap) copy_length = (size_t)cap - 1u;
    strncpy(dst, s, copy_length);
    dst[copy_length] = '\0';
}

static void task_put_int(GameTask* t, int off, int32_t v)
{
    __builtin_memcpy(t->data + off, &v, 4);
}

static int task_get_int(const GameTask* t, int off)
{
    int32_t v;
    __builtin_memcpy(&v, t->data + off, 4);
    return (int)v;
}

static void task_add_flag(GameTask* t, uint32_t bits)
{
    task_set_flags(t, task_flags(t) | bits);
}

/* Helper used by the field loaders: resolve a pipe-delimited
 * `file|n|n…` config token to its fields (corpus FUN_00477440). */
static int task_fields(const char* token, char out[UTIL_QUERY_MAX_FIELDS][256])
{
    return util_query_fields(token, out, sizeof(out[0]));
}

/* StrReplace (FUN_00413f00): replace one byte throughout a NUL-terminated
 * string and return the original pointer. */
// FUNCTION: LEGORR 0x00413f00
static char* mission_replace_char(char* text, char from, char to)
{
    char* const original = text;
    while (*text != '\0') {
        if (*text == from) {
            *text = to;
        }
        ++text;
    }
    return original;
}

/* ------------------------------------------------------------------ *
 *  Field loaders (corpus order in TaskBlockInit).
 * ------------------------------------------------------------------ */

/* FUN_0045f3e0 — `Reward Images.<name>`: coords at +0x57/0x58, flag |2. */
// FUNCTION: LEGORR 0x0045f3e0
static void task_loader_images(BlockFile* cfg, const char* root,
                               const char* level, GameTask* t)
{
    const char* v = rw_str(cfg, root, level, "Images", task_name(t));
    if (v == NULL) {
        return;
    }
    char f[UTIL_QUERY_MAX_FIELDS][256];
    if (task_fields(v, f) == 0) {
        return;
    }
    task_put_int(t, TASK_OFF_IMG_XY, (int32_t)atoi(f[1]));
    task_put_int(t, TASK_OFF_IMG_XY + 4, (int32_t)atoi(f[2]));
    task_add_flag(t, TASK_FLAG_IMG);
}

/* FUN_0045f530 — `Reward Text.<name>` (key string lives at corpus
 * DAT_004a26b0 = "Text"): field 0 -> display text at +0x54 with '_'
 * mapped to ' ' over the first 0x20 bytes (corpus FUN_00413f00),
 * fields 1/2 -> text-draw x/y at +0x154/+0x158, flag |= 0x31. */
// FUNCTION: LEGORR 0x0045f530
static void task_loader_text(BlockFile* cfg, const char* root,
                             const char* level, GameTask* t)
{
    const char* v = rw_str(cfg, root, level, "Text", task_name(t));
    if (v == NULL) {
        return;
    }
    char f[UTIL_QUERY_MAX_FIELDS][256];
    if (task_fields(v, f) == 0) {
        return;
    }
    task_put_str(t, TASK_OFF_TEXT, f[0], 0x100);
    mission_replace_char((char*)t->data + TASK_OFF_TEXT, '_', ' ');
    task_put_int(t, TASK_OFF_TEXT_XY, (int32_t)atoi(f[1]));
    task_put_int(t, TASK_OFF_TEXT_XY + 4, (int32_t)atoi(f[2]));
    task_add_flag(t, TASK_FLAG_MASK);
}

/* FUN_0045f5f0 — `Reward Flics.<name>`: path +0x5b, rect +0x7b..0x7e, |4. */
// FUNCTION: LEGORR 0x0045f5f0
static void task_loader_flics(BlockFile* cfg, const char* root,
                              const char* level, GameTask* t)
{
    const char* v = rw_str(cfg, root, level, "Flics", task_name(t));
    if (v == NULL) {
        return;
    }
    char f[UTIL_QUERY_MAX_FIELDS][256];
    if (task_fields(v, f) == 0) {
        return;
    }
    task_put_str(t, TASK_OFF_FLICPATH2, f[0], 128);
    task_put_int(t, TASK_OFF_FLIC_RECT + 8, (int32_t)atoi(f[1]));
    task_put_int(t, TASK_OFF_FLIC_RECT + 12, (int32_t)atoi(f[2]));
    task_put_int(t, TASK_OFF_FLIC_RECT, (int32_t)atoi(f[3]));
    task_put_int(t, TASK_OFF_FLIC_RECT + 4, (int32_t)atoi(f[4]));
    task_add_flag(t, TASK_FLAG_FLIC);
}

/* FUN_0045f230 — `Reward Fonts.<name>` at +0x204. */
// FUNCTION: LEGORR 0x0045f230
static void task_loader_font(BlockFile* cfg, const char* root,
                             const char* level, GameTask* t)
{
    const char* v = rw_str(cfg, root, level, "Fonts", task_name(t));
    task_put_str(t, TASK_OFF_FONT, v, 64);
}

/* FUN_0045f1f0 — `Reward Sounds.<name>` at +0x208. */
// FUNCTION: LEGORR 0x0045f1f0
static void task_loader_sound(BlockFile* cfg, const char* root,
                              const char* level, GameTask* t)
{
    const char* v = rw_str(cfg, root, level, "Sounds", task_name(t));
    task_put_str(t, TASK_OFF_SOUND, v, 64);
}

/* FUN_0045f290 — `Reward BoxImages.<name>`: coords +0x59/0x5a, |0x40. */
// FUNCTION: LEGORR 0x0045f290
static void task_loader_boximages(BlockFile* cfg, const char* root,
                                  const char* level, GameTask* t)
{
    const char* v = rw_str(cfg, root, level, "BoxImages", task_name(t));
    if (v == NULL) {
        return;
    }
    char f[UTIL_QUERY_MAX_FIELDS][256];
    if (task_fields(v, f) == 0) {
        return;
    }
    task_put_int(t, TASK_OFF_BOXIMG_XY, (int32_t)atoi(f[1]));
    task_put_int(t, TASK_OFF_BOXIMG_XY + 4, (int32_t)atoi(f[2]));
    task_add_flag(t, TASK_FLAG_BOXIMG);
}

/* ------------------------------------------------------------------ *
 *  Entry points (finding 09).
 * ------------------------------------------------------------------ */

const char* mission_task_name_at(GameContext* ctx, int slot)
{
    if (ctx == NULL || slot < 0 || slot >= GAME_TASK_COUNT) {
        return NULL;
    }
    return task_name(&ctx->tasks[slot]);
}

/* Partial TaskBlockInit port: seeds the context-owned task records and runs
 * the six field loaders; the original allocates one session-global block. */
// FUNCTION: LEGORR 0x0045f030
void mission_task_init(GameContext* ctx, BlockFile* cfg, const char* level_name)
{
    if (ctx == NULL) {
        return;
    }
    const char* root = cfg != NULL ? util_root_name(cfg) : "Lego*";
    GameTask* t;
    const char** name;
    /* TaskBlockInit clears its task block before the corpus's name pass. The
     * port keeps the same operation scoped to the context-owned task array. */
    __builtin_memset(ctx->tasks, 0, sizeof(ctx->tasks));
    for (t = ctx->tasks, name = TASK_NAMES;
         t < ctx->tasks + GAME_TASK_COUNT; ++t, ++name) {
        task_set_name(t, *name);
    }
    if (cfg == NULL || level_name == NULL) {
        return;
    }
    /* TaskBlockInit makes one pass per loader (Images, Text, Flics, Fonts,
     * Sounds, BoxImages). Keep that corpus order while targeting ctx's tasks;
     * each pass writes independent task fields, so the resulting records are
     * unchanged from the former per-task grouping. */
    for (t = ctx->tasks; t < ctx->tasks + GAME_TASK_COUNT; ++t) {
        task_loader_images(cfg, root, level_name, t);
    }
    for (t = ctx->tasks; t < ctx->tasks + GAME_TASK_COUNT; ++t) {
        task_loader_text(cfg, root, level_name, t);
    }
    for (t = ctx->tasks; t < ctx->tasks + GAME_TASK_COUNT; ++t) {
        task_loader_flics(cfg, root, level_name, t);
    }
    for (t = ctx->tasks; t < ctx->tasks + GAME_TASK_COUNT; ++t) {
        task_loader_font(cfg, root, level_name, t);
    }
    for (t = ctx->tasks; t < ctx->tasks + GAME_TASK_COUNT; ++t) {
        task_loader_sound(cfg, root, level_name, t);
    }
    for (t = ctx->tasks; t < ctx->tasks + GAME_TASK_COUNT; ++t) {
        task_loader_boximages(cfg, root, level_name, t);
    }
    mission_task_refresh(ctx);
}

/* TaskImportanceRefresh @ 0x4600a0: load one task's Reward Importance
 * value into record +0x24. The global level/config become explicit port
 * context; mission_task_refresh preserves the corpus's per-task ordering. */
// FUNCTION: LEGORR 0x004600a0
static void mission_task_importance_refresh(BlockFile* cfg, const char* root,
                                            const char* level, GameTask* t)
{
    float imp = rw_float(cfg, root, level, "Importance", task_name(t), 0.0f);
    __builtin_memcpy(t->data + TASK_OFF_IMPORTANCE, &imp, 4);
}

/* TaskQuotaRefresh @ 0x460140: load one task's Reward Quota value into
 * record +0x28. */
// FUNCTION: LEGORR 0x00460140
static void mission_task_quota_refresh(BlockFile* cfg, const char* root,
                                       const char* level, GameTask* t)
{
    int32_t quota = (int32_t)rw_int(cfg, root, level, "Quota", task_name(t), 0);
    __builtin_memcpy(t->data + TASK_OFF_QUOTA, &quota, 4);
}

/* CrystalQuotaSetup @ 0x45c040: read the configured crystal quota, using
 * the corpus's zero/over-limit fallback. The context owns the ported value. */
// FUNCTION: LEGORR 0x0045c040
static void mission_crystal_quota_setup(BlockFile* cfg, const char* root,
                                        const char* level, GameContext* ctx)
{
    int quota = rw_int(cfg, root, level, "Quota", "Crystals", 0);
    if (quota > MISS_CRYSTAL_QUOTA || quota == 0) {
        quota = MISS_CRYSTAL_QUOTA;
    }
    ctx->crystal_quota = quota;
}

void mission_task_refresh(GameContext* ctx)
{
    if (ctx == NULL) {
        return;
    }
    const char* level = ctx->level != NULL ? (const char*)ctx->level->ptr_name : NULL;
    if (ctx->cfg == NULL || level == NULL) {
        return;
    }
    const char* root = util_root_name(ctx->cfg);
    for (int i = 0; i < GAME_TASK_COUNT; ++i) {
        GameTask* t = &ctx->tasks[i];
        mission_task_importance_refresh(ctx->cfg, root, level, t);
        mission_task_quota_refresh(ctx->cfg, root, level, t);
        /* Defaults: absence parses as 0; the corpus reads the same way. */
    }
    mission_crystal_quota_setup(ctx->cfg, root, level, ctx);
}

void mission_set_crystals(GameContext* ctx, int32_t v)
{
    if (ctx != NULL) ctx->mission.crystals = v;
}
void mission_set_ore(GameContext* ctx, int32_t v)
{
    if (ctx != NULL) ctx->mission.ore = v;
}
void mission_add_score(GameContext* ctx, int32_t v)
{
    if (ctx != NULL) {
        ctx->mission.score += v;
        reward_score_add((float)v);
    }
}

void mission_incident(GameContext* ctx, int id)
{
    (void)ctx;
    message_dispatch(id, 0, 0, NULL);
}

/* Live bridge: write the counters into the level's objective-read slots and
 * advance the task value/score slots; return task bits that completed. */
int mission_update(GameContext* ctx)
{
    if (ctx == NULL || ctx->level == NULL) {
        return 0;
    }
    LevelState* s = ctx->level;
    /* Crystal (task 0) / Ore (task 1) live counts — objective readers. */
    level_set_int(s, OBJ_OFF_CRYSTAL_LIVE, ctx->mission.crystals);
    level_set_int(s, OBJ_OFF_ORE_LIVE, ctx->mission.ore);
    task_set_value(&ctx->tasks[0], ctx->mission.crystals);
    task_set_value(&ctx->tasks[1], ctx->mission.ore);
    task_set_value(&ctx->tasks[3], ctx->mission.constructions);
    task_set_value(&ctx->tasks[5], ctx->mission.score);

    int completed = 0;
    for (int i = 0; i < GAME_TASK_COUNT; ++i) {
        GameTask* t = &ctx->tasks[i];
        int quota = task_get_int(t, TASK_OFF_QUOTA);
        if (quota <= 0) {
            continue;
        }
        int value = task_value(t);
        if (value >= quota && (task_flags(t) & TASK_FLAG_COMPLETED) == 0) {
            task_set_flags(t, task_flags(t) | TASK_FLAG_COMPLETED);
            completed |= (1 << i);
        }
    }
    if (ctx->checkpoint_blob != NULL)
        (void)mission_checkpoint_task_progress_refresh(ctx);
    return completed;
}

static float mission_blob_float_get(const uint8_t* blob, size_t offset)
{
    float value;
    __builtin_memcpy(&value, blob + offset, sizeof(value));
    return value;
}

static void mission_blob_float_set(uint8_t* blob, size_t offset, float value)
{
    __builtin_memcpy(blob + offset, &value, sizeof(value));
}

static void mission_blob_word_set(uint8_t* blob, size_t offset,
                                  uint32_t value)
{
    __builtin_memcpy(blob + offset, &value, sizeof(value));
}

/* Keep explicit-context callers on the same no-argument target path as the
 * retail entry point. Save/restore the temporary owner so callbacks may
 * re-enter the calculation without leaking another GameContext. */
static GameContext* s_checkpoint_refresh_context_override;

int mission_checkpoint_task_progress_refresh(GameContext* ctx)
{
    GameContext* previous = s_checkpoint_refresh_context_override;
    int result;
    s_checkpoint_refresh_context_override = ctx;
    result = mission_checkpoint_task_progress_refresh_bound();
    s_checkpoint_refresh_context_override = previous;
    return result;
}

/* CheckpointBlobTaskProgressRefresh prepares the per-level task snapshot.
 * Map counters stay local to the active GameContext, while the percentage
 * fold and CRT-backed score/stat reads use their existing helpers. The
 * DAT_005529a0 scratch copy and DAT_00555118 still have no ported writer. */
// FUNCTION: LEGORR 0x00460360
int mission_checkpoint_task_progress_refresh_bound(void)
{
    GameContext* ctx = s_checkpoint_refresh_context_override;
    LevelState* level;
    uint8_t* blob;
    int width;
    int height;
    uint32_t cell_count = 0u;
    float classified_a = 0.0f;
    float classified_b = 0.0f;
    float eligible_cells = 0.0f;
    float eligible_non_solid = 0.0f;
    float marked_cells = 0.0f;
    int y;

    if (ctx == NULL && s_reward_screen_state != NULL) {
        ctx = s_reward_screen_state->game;
    }
    if (ctx == NULL) {
        ctx = s_reward_screen_default_state.game;
    }

    if (ctx == NULL || ctx->level == NULL || ctx->checkpoint_blob == NULL)
        return 0;
    level = ctx->level;
    blob = (uint8_t*)ctx->checkpoint_blob;
    level_set_int(level, OBJ_OFF_CRYSTAL_LIVE, ctx->mission.crystals);
    level_set_int(level, OBJ_OFF_ORE_LIVE, ctx->mission.ore);
    width = level_int(level, LEVEL_OFF_WIDTH);
    height = level_int(level, LEVEL_OFF_HEIGHT);
    if (width <= 0 || height <= 0 || level_voxel(level, 0, 0) == NULL) return 0;

    mission_blob_float_set(blob, 0x1d1c, 0.0f);
    mission_blob_float_set(blob, 0x1f2c, 0.0f);
    mission_blob_word_set(blob, 0x1c8, 0u);
    mission_blob_word_set(blob, 0x1cc, 0u);
    mission_blob_word_set(blob, 0x1d0, 0u);
    mission_blob_float_set(blob, 0x1d14, 0.0f);
    mission_blob_float_set(blob, 0x2140, crt_float_00554adc_peek());
    mission_blob_float_set(blob, 0x2560, crt_float_00554efc_peek());
    mission_blob_float_set(blob, 0x213c, 0.0f);
    mission_blob_float_set(blob, 0x1b04, 0.0f);
    mission_blob_float_set(blob, 0x1b08, 0.0f);
    {
        const uint32_t pilot_count = (uint32_t)
            resolved_entity_operation_dispatch("Pilot", 0, 0);
        mission_blob_word_set(blob, 0x2778, pilot_count);
        mission_blob_float_set(blob, 0x276c, (float)pilot_count);
        mission_blob_float_set(blob, 0x2770, (float)pilot_count);
    }
    mission_blob_word_set(blob, 0x277c, 0u); /* DAT_00555118 has no ported writer */
    mission_blob_float_set(blob, 0x2784, crt_float_00555120_peek());

    {
        const uint32_t count_a = crt_counter_00555324_peek();
        const uint32_t count_b = crt_counter_00555328_peek();
        const uint32_t count_c = crt_counter_0055532c_peek();
        float score_percent = (float)count_a * 100.0f;
        const float score_limit = crt_float_00555330_peek();
        if (score_percent > score_limit) score_percent = score_limit;
        mission_blob_word_set(blob, 0x2988, count_a);
        mission_blob_word_set(blob, 0x298c, count_b);
        mission_blob_word_set(blob, 0x2990, count_c);
        mission_blob_float_set(blob, 0x2994, score_percent);
        mission_blob_float_set(blob, 0x297c, (float)count_a * 100.0f);
        mission_blob_float_set(blob, 0x2980, score_percent);
    }
    {
        const uint32_t score_count = reward_score_count_peek();
        const float score_total = reward_score_total_peek();
        const float denominator = (float)score_count;
        mission_blob_float_set(blob, 0x2da0, denominator);
        mission_blob_float_set(blob, 0x2d9c, score_total / denominator);
    }
    /* The port has no owner for the session-global scratch block at
     * DAT_005529a4 or the 0x66-dword task seed image at DAT_005529c4. Keep
     * their copied regions at the same zero-init values a new context gets. */
    memset(blob + 8, 0, 8u * sizeof(uint32_t));
    memset(blob + 0x28, 0, 0x198u);
    mission_blob_float_set(blob, 0x2344, 0.0f);
    mission_blob_float_set(blob, 0x2354, 0.0f);
    mission_blob_float_set(blob, 0x2554, 0.0f);
    mission_blob_float_set(blob, 0x2564, 0.0f);
    mission_blob_float_set(blob, 0x2d94, 0.0f);
    mission_blob_float_set(blob, 0x2da4, 0.0f);

    /* Keep the named task values tied to live mission counters before the
     * aggregate percentage routine consumes its input/target pairs. */
    task_set_value(&ctx->tasks[0], ctx->mission.crystals);
    task_set_value(&ctx->tasks[1], ctx->mission.ore);
    task_set_value(&ctx->tasks[3], ctx->mission.constructions);
    task_set_value(&ctx->tasks[5], ctx->mission.score);
    __builtin_memcpy(blob + 0x1cec, ctx->tasks,
                     GAME_TASK_COUNT * GAME_TASK_STRIDE);

    /* The reference's 0x2c5b0 classifier reads voxel +5 and routes its
     * {1,3,5,10,25} weight to one of four accumulators. Its caller consumes
     * the first and fourth outputs as the two map totals. */
    for (y = 0; y < height; ++y) {
        int x;
        for (x = 0; x < width; ++x) {
            const uint8_t* voxel = level_voxel(level, x, y);
            uint32_t a = 0u, b = 0u, c = 0u, d = 0u;
            uint32_t flags;
            uint8_t terrain_type;
            if (voxel == NULL) continue;
            ++cell_count;
            level_cell_byte_state_classify(level, x, y, &a, &b, &c, &d);
            classified_a += (float)a;
            classified_b += (float)d;
            terrain_type = voxel[2];
            __builtin_memcpy(&flags, voxel + 8, sizeof(flags));
            if ((flags & 8u) == 0u) {
                if (terrain_type != 1u) {
                    eligible_cells += 1.0f;
                    eligible_non_solid += 1.0f;
                }
                if (terrain_type == 10u) classified_a += 4.0f;
            }
            else if (terrain_type != 7u && terrain_type != 6u) {
                eligible_non_solid += 1.0f;
            }
            if ((flags & 0x61100000u) != 0u) {
                marked_cells += 1.0f;
                eligible_cells += 1.0f;
            }
        }
    }

    mission_blob_word_set(blob, 0x1c8, cell_count);
    mission_blob_float_set(blob, 0x1d1c, classified_a);
    mission_blob_float_set(blob, 0x1f2c,
                           classified_b + eligible_cells * 4.0f);
    mission_blob_float_set(blob, 0x213c, eligible_cells);
    mission_blob_float_set(blob, 0x1b04, eligible_non_solid);
    mission_blob_float_set(blob, 0x1b08, marked_cells);
    mission_blob_float_set(blob, 0x1d20,
        (float)level_int(level, OBJ_OFF_CRYSTAL_LIVE));
    mission_blob_float_set(blob, 0x1f30,
        (float)level_int(level, OBJ_OFF_ORE_LIVE));
    mission_blob_float_set(blob, 0x2b8c, 100.0f);
    mission_blob_float_set(blob, 0x2b90,
        (float)level_int(level, LEVEL_OFF_AIR_SUPPLY));
    mission_blob_float_set(blob, 0x234c,
                           mission_blob_float_get(blob, 0x1cc));
    mission_blob_float_set(blob, 0x2350,
        (mission_blob_float_get(blob, 0x1cc) -
         mission_blob_float_get(blob, 0x1d0)) + 1.0f);
    if (mission_blob_float_get(blob, 0x2344) != 0.0f) {
        mission_blob_float_set(blob, 0x2354,
            (100.0f / mission_blob_float_get(blob, 0x2344)) *
             mission_blob_float_get(blob, 0x2350));
    }
    if (mission_blob_float_get(blob, 0x2554) != 0.0f) {
        mission_blob_float_set(blob, 0x2564,
            (100.0f / mission_blob_float_get(blob, 0x2554)) *
             mission_blob_float_get(blob, 0x2560));
    }

    /* Clamp every task's render percentage, except the Timer record whose
     * +0x38 field is owned by the timer task path in the original. */
    {
        uint32_t offset;
        for (offset = 0u; offset < 0x14a0u; offset += GAME_TASK_STRIDE) {
            float progress;
            if (offset == 0x1080u) continue;
            progress = mission_blob_float_get(blob, 0x1cec + offset + 0x38u);
            if (progress < 0.0f) progress = 0.0f;
            if (progress > 100.0f) progress = 100.0f;
            mission_blob_float_set(blob, 0x1cec + offset + 0x38u, progress);
        }
    }
    if (!checkpoint_blob_task_progress_recalculate(ctx)) return 0;
    (void)reward_stats_panel_draw_bound();
    return 1;

}

/* Partial RewardInit port: reproduces the display/config settings while
 * resource handles remain owned by the platform/render seams. */
// FUNCTION: LEGORR 0x0045f7f0
void mission_reward_init(GameContext* ctx, BlockFile* cfg, const char* level_name)
{
    if (ctx == NULL) {
        return;
    }
    RewardScreen* rs = &ctx->reward_screen;
    __builtin_memset(rs, 0, sizeof(*rs));
    rs->save_w = 56.0f;
    rs->save_h = 20.0f;
    rs->advance_w = 72.0f;
    rs->advance_h = 20.0f;
    /* The reference reward ABI reads process-global state. Keep an owned
     * default state bound to the active GameContext so the checkpoint and
     * pointer-expiry entry points operate on the same live reward data even
     * when a renderer has not installed its drawing callbacks. */
    if (s_reward_screen_state == NULL ||
        s_reward_screen_state == &s_reward_screen_default_state) {
        if (s_reward_screen_state == &s_reward_screen_default_state)
            reward_task_flics_close_all(&s_reward_screen_default_state);
        reward_screen_state_init(&s_reward_screen_default_state, ctx,
                                 NULL, NULL);
        reward_screen_runtime_bind(&s_reward_screen_default_state,
                                   s_reward_screen_ops);
    }
    if (cfg == NULL || level_name == NULL) {
        return;
    }
    const char* root = util_root_name(cfg);
    /* RewardInit @ 0x45f7f0 keys (corpus engine-region.c 6485-6557). */
    const char* v;
    if ((v = rw_str(cfg, root, level_name, NULL, "Wallpaper")) != NULL) {
        snprintf(rs->wallpaper, sizeof(rs->wallpaper), "%s", v);
    }
    rs->display = rw_typed(cfg, root, level_name, "Display");
    if (rs->display == 2) rs->display = 0;
    rs->centre_text = rw_typed(cfg, root, level_name, "CentreText");
    if (rs->centre_text == 2) rs->centre_text = 0;
    v = rw_str(cfg, root, level_name, NULL, "ScrollSpeed");
    rs->scroll_speed = (float)atof(v != NULL ? v : "");
    v = rw_str(cfg, root, level_name, NULL, "VertSpacing");
    rs->vert_spacing = (float)atof(v != NULL ? v : "");
    if ((v = rw_str(cfg, root, level_name, NULL, "TitleFont")) != NULL) {
        snprintf(rs->font_title, sizeof(rs->font_title), "%s", v);
    }
    if ((v = rw_str(cfg, root, level_name, NULL, "BackFont")) != NULL) {
        snprintf(rs->font_back, sizeof(rs->font_back), "%s", v);
    }
    if ((v = rw_str(cfg, root, level_name, NULL, "CompleteText")) != NULL) {
        snprintf(rs->complete_text, sizeof(rs->complete_text), "%s", v);
    }
    if ((v = rw_str(cfg, root, level_name, NULL, "FailedText")) != NULL) {
        snprintf(rs->failed_text, sizeof(rs->failed_text), "%s", v);
    }
    if ((v = rw_str(cfg, root, level_name, NULL, "QuitText")) != NULL) {
        snprintf(rs->quit_text, sizeof(rs->quit_text), "%s", v);
    }
    if ((v = rw_str(cfg, root, level_name, NULL, "TextPos")) != NULL) {
        char f[UTIL_QUERY_MAX_FIELDS][256];
        if (task_fields(v, f) != 0) {
            rs->text_pos[0] = atoi(f[0]);
            rs->text_pos[1] = atoi(f[1]);
        }
    }
    /* Save/Advance button hit zones (FUN_0045faf0): `file|x|y` triples from
     * `Reward SaveButton` / `Reward AdvanceButton`. Deltas + button
     * *widths* come from the surface cells the corpus reads — the position
     * pair is captured here for RewardZoneCheck. */
    if ((v = rw_str(cfg, root, level_name, NULL, "SaveButton")) != NULL) {
        char f[UTIL_QUERY_MAX_FIELDS][256];
        if (task_fields(v, f) != 0) {
            rs->save_x = (float)atoi(f[1]);
            rs->save_y = (float)atoi(f[2]);
        }
    }
    if ((v = rw_str(cfg, root, level_name, NULL, "AdvanceButton")) != NULL) {
        char f[UTIL_QUERY_MAX_FIELDS][256];
        if (task_fields(v, f) != 0) {
            rs->advance_x = (float)atoi(f[1]);
            rs->advance_y = (float)atoi(f[2]);
        }
    }
}

int mission_reward_active(const GameContext* ctx)
{
    return ctx != NULL ? ctx->reward_active : 0;
}

void mission_reward_set_active(GameContext* ctx, int on)
{
    if (ctx != NULL) {
        ctx->reward_active = on;
    }
}

/* Rect hit-test helper (the corpus's widget zones +w/h, RewardZoneCheck). */
static int zone_hit(float x, float y, float w, float h, int mx, int my)
{
    return mx >= x && mx < x + w && my >= y && my < y + h;
}

// FUNCTION: LEGORR 0x00461dd0
int mission_reward_zone(GameContext* ctx, int mx, int my,
                        int* hit_save, int* hit_advance)
{
    if (ctx == NULL || hit_save == NULL || hit_advance == NULL) {
        return 0;
    }
    *hit_save = 0;
    *hit_advance = 0;
    if (!ctx->reward_active || ctx->level == NULL) {
        return 0;
    }
    int result = level_int(ctx->level, LEVEL_OFF_OBJ_RESULT);
    if (result == 1) {
        /* achieved: the SaveButton zone is live (RewardZoneCheck branch 1) */
        if (zone_hit(ctx->reward_screen.save_x, ctx->reward_screen.save_y,
                     ctx->reward_screen.save_w, ctx->reward_screen.save_h,
                     mx, my)) {
            *hit_save = 1;
            return 1;
        }
    } else if (zone_hit(ctx->reward_screen.advance_x, ctx->reward_screen.advance_y,
                        ctx->reward_screen.advance_w, ctx->reward_screen.advance_h,
                        mx, my)) {
        *hit_advance = 1;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00462780
int mission_reward_click(GameContext* ctx, int hit_save)
{
    if (ctx == NULL) {
        return 0;
    }
    (void)hit_save;
    if (ctx->checkpoint_blob == NULL) checkpoint_blob_build(ctx);
    (void)mission_checkpoint_task_progress_refresh(ctx);
    /* The corpus's reward-button handler (FUN_00462780) commits the
     * checkpoint through CheckpointStep; the level name maps to the active
     * record. */
    checkpoint_step(ctx);
    ctx->reward_active = 0;
    return 1;
}

/* Advance to the parsed `nextlevel` (finding 07): checkpoint the current
 * progress (the corpus's advance branch calls CheckpointStep too), tear the
 * level down, and load + re-seed the next one. Returns the next level name
 * (owned by GameState, i.e. ctx->level after the swap) or NULL when the
 * chain ends. */
const char* mission_reward_advance(GameContext* ctx)
{
    if (ctx == NULL || ctx->level == NULL || ctx->cfg == NULL) {
        return NULL;
    }
    const char* next = level_next_name(ctx->level);
    if (next == NULL || next[0] == '\0') {
        ctx->reward_active = 0; /* final level: just close the overlay */
        return NULL;
    }
    char name[128];
    snprintf(name, sizeof(name), "%s", next);

    if (ctx->checkpoint_blob == NULL) checkpoint_blob_build(ctx);
    (void)mission_checkpoint_task_progress_refresh(ctx);
    checkpoint_step(ctx); /* save before moving on */
    level_destroy(ctx->level);
    ctx->level = NULL;
    ctx->selection_count = 0;
    ctx->reward_active = 0;
    __builtin_memset(&ctx->mission, 0, sizeof(ctx->mission));
    __builtin_memset(&ctx->tasks, 0, sizeof(ctx->tasks));

    ctx->level = level_load(ctx->cfg, name);
    if (ctx->level != NULL) {
        mission_task_init(ctx, ctx->cfg, name);
        mission_reward_init(ctx, ctx->cfg, name);
        object_list_load(ctx, ctx->cfg, name);
    }
    return ctx->level != NULL ? (const char*)ctx->level->ptr_name : NULL;
}

static GameTask* reward_task_get(RewardScreenState* state, int index)
{
    if (state == NULL || state->game == NULL || index < 0 ||
        index >= GAME_TASK_COUNT) return NULL;
    return &state->game->tasks[index];
}

static void reward_task_flic_reload(RewardScreenState* state, int index)
{
    GameTask* task = reward_task_get(state, index);
    Flic* flic;
    const char* path;
    if (task == NULL || (task_flags(task) & TASK_FLAG_FLIC) == 0u) return;
    if (state->task_flics[index] != NULL) {
        flic_close(state->task_flics[index]);
        state->task_flics[index] = NULL;
    }
    path = (const char*)task->data + TASK_OFF_FLICPATH2;
    if (path[0] == '\0') return;
    flic = flic_open(path);
    state->task_flics[index] = flic;
}

static void reward_task_flics_reload_all(RewardScreenState* state)
{
    int i;
    if (state == NULL) return;
    for (i = 0; i < GAME_TASK_COUNT; ++i)
        reward_task_flic_reload(state, i);
}

static void reward_task_flics_close_all(RewardScreenState* state)
{
    int i;
    if (state == NULL) return;
    for (i = 0; i < GAME_TASK_COUNT; ++i) {
        if (state->task_flics[i] != NULL) {
            flic_close(state->task_flics[i]);
            state->task_flics[i] = NULL;
        }
    }
}

void reward_screen_state_init(RewardScreenState* state, GameContext* game,
                              PointerCursorState* pointer_cursor,
                              const PointerCursorHooks* pointer_hooks)
{
    if (state == NULL) return;
    memset(state, 0, sizeof(*state));
    state->game = game;
    state->pointer_cursor = pointer_cursor;
    state->pointer_cursor_hooks = pointer_hooks;
    state->task_duration = game != NULL ? game->reward.timer : 0.0f;
    state->elapsed = state->task_duration;
}

void reward_screen_runtime_bind(RewardScreenState* state,
                                const RewardScreenOps* ops)
{
    s_reward_screen_state = state;
    s_reward_screen_ops = ops;
}

int reward_screen_tick_bound(void)
{
    return reward_screen_tick();
}

/* RewardStatsTextFormat (FUN_00461070). The checkpoint snapshot stores the
 * numeric values and their rendered forms in the same fixed task records.
 * Keep the offsets literal: several values are outside the contiguous task
 * array, exactly as the reference writes them. */
// FUNCTION: LEGORR 0x00461070
int reward_stats_text_format_bound(void)
{
    static const int value_offsets[] = {
        0x1d3c, 0x1f4c, 0x215c, 0x1b24, 0x1cc,
        0x257c, 0x278c, 0x299c, 0x2bac, 0x2fcc
    };
    static const int text_offsets[] = {
        0x1dc0, 0x1fd0, 0x21e0, 0x1ba8, 0x23f0,
        0x2600, 0x2810, 0x2a20, 0x2c30, 0x3050
    };
    RewardScreenState* state = s_reward_screen_state;
    uint8_t* blob;
    int i;
    uint32_t seconds;
    uint32_t hours;
    uint32_t minutes;
    uint32_t remainder;
    float value;
    int32_t integer;

    if (state == NULL || state->game == NULL ||
        state->game->level == NULL || state->game->checkpoint_blob == NULL)
        return 0;
    blob = state->game->checkpoint_blob;
    for (i = 0; i < 10; ++i) {
        if (i == 4) {
            __builtin_memcpy(&integer, blob + value_offsets[i], 4);
        }
        else {
            __builtin_memcpy(&value, blob + value_offsets[i], 4);
            integer = (int32_t)value;
        }
        __builtin_memcpy(blob + value_offsets[i], &integer, 4);
        sprintf((char*)blob + text_offsets[i], "%d", (int)integer);
    }

    /* The elapsed-time source is a float at +0x2dbc. The reference reduces
     * it modulo 359999 seconds, then emits hours, minutes, and seconds. */
    __builtin_memcpy(&value, blob + 0x2dbc, 4);
    seconds = value > 0.0f ? (uint32_t)value : 0u;
    seconds %= 359999u;
    __builtin_memcpy(blob + 0x2dbc, &seconds, 4);
    hours = seconds / 3600u;
    remainder = seconds % 3600u;
    minutes = remainder / 60u;
    remainder %= 60u;
    sprintf((char*)blob + 0x2e40, "%02lu:%02lu:%02lu",
            (unsigned long)hours, (unsigned long)minutes,
            (unsigned long)remainder);
    return 1;
}

/* RewardStatsPanelDraw (FUN_00460f00) owns layout and text selection in the
 * original. The renderer-specific layout/draw calls are supplied by the
 * bound runtime; keep the level/blob admission gate and formatting here. */
// FUNCTION: LEGORR 0x00460f00
int reward_stats_panel_draw_bound(void)
{
    RewardScreenState* state = s_reward_screen_state;
    if (state == NULL || state->game == NULL || state->game->level == NULL ||
        state->game->checkpoint_blob == NULL ||
        !reward_stats_text_format_bound()) return 0;
    if (s_reward_screen_ops != NULL &&
        s_reward_screen_ops->reward_stats_draw != NULL) {
        s_reward_screen_ops->reward_stats_draw(
            s_reward_screen_ops->context, state->game);
    }
    return 1;
}

/* RewardFlhContextTick (FUN_004624a0): advance one task when the display
 * interval expires, or skip the first interval on the slow/normal path. */
static void reward_screen_task_timer_tick(RewardScreenState* state,
                                          const RewardScreenOps* ops)
{
    int advance = 0;
    int stage_is_initial;
    int display_mode = ops != NULL && ops->display_mode_get != NULL
        ? ops->display_mode_get(ops->context, state->game) : 0;
    stage_is_initial = state->transition_stage == 0;
    if (state->task_duration <= state->elapsed) {
        if (stage_is_initial) advance = 1;
    }
    else if (!stage_is_initial &&
             state->task_duration * 2.0f <= state->elapsed &&
             display_mode == 1 &&
             state->displayed_task_count < GAME_TASK_COUNT) {
        advance = 1;
    }
    if (!advance) return;

    state->elapsed -= state->task_duration;
    if (state->displayed_task_count < GAME_TASK_COUNT)
        ++state->displayed_task_count;
    if (state->displayed_task_count > 0) {
        GameTask* task = reward_task_get(state,
                                          state->displayed_task_count - 1);
        if (task != NULL) {
            const char* sound_name = (const char*)task->data + TASK_OFF_SOUND;
            int sound_alias;
            if (sound_name[0] != '\0' &&
                sound_alias_lookup(sound_name, &sound_alias))
                sound_request_queue_submit(sound_alias, 0);
        }
    }
    if (state->displayed_task_count > 9) {
        if (state->transition_stage == 0) {
            state->displayed_task_count = 9;
            ++state->transition_stage;
            return;
        }
        if (state->transition_stage > 1) {
            if (state->displayed_task_count > 10) {
                state->displayed_task_count = 10;
                ++state->transition_stage;
                return;
            }
            reward_task_flics_reload_all(state);
        }
    }
}

void reward_screen_transition_tick(RewardScreenState* state,
                                   const RewardScreenOps* ops)
{
    int return_to_game = 0;
    int reset_reward = 0;
    if (state == NULL || state->game == NULL || ops == NULL ||
        (state->game->cheat & 0x10u) == 0u ||
        ops->transition_events == NULL ||
        !ops->transition_events(ops->context, &return_to_game,
                                &reset_reward)) return;
    if (return_to_game && ops->return_to_game != NULL)
        ops->return_to_game(ops->context);
    if (!reset_reward) return;
    state->displayed_task_count = 0;
    state->transition_stage = 0;
    state->transition_state = 0;
    state->score_text_gate = 0;
    state->elapsed = state->task_duration;
    sound_buffer_stop_handle(0x29);
    sound_request_queue_submit(0x29, 1);
    reward_task_flics_reload_all(state);
}

// FUNCTION: LEGORR 0x00462390
void reward_screen_transition_tick_abi(int* displayed_task_count,
                                       int* transition_stage,
                                       int* score_text_gate, float* elapsed)
{
    RewardScreenState* state = s_reward_screen_state;
    if (state == NULL || displayed_task_count == NULL ||
        transition_stage == NULL || score_text_gate == NULL ||
        elapsed == NULL) return;
    state->displayed_task_count = *displayed_task_count;
    state->transition_stage = *transition_stage;
    state->score_text_gate = *score_text_gate;
    state->elapsed = *elapsed;
    reward_screen_transition_tick(state, s_reward_screen_ops);
    *displayed_task_count = state->displayed_task_count;
    *transition_stage = state->transition_stage;
    *score_text_gate = state->score_text_gate;
    *elapsed = state->elapsed;
}

// FUNCTION: LEGORR 0x00462700
void reward_pointer_override_expiry_tick(RewardScreenState* state,
                                         float delta,
                                         const RewardScreenOps* ops)
{
    if (state == NULL) return;
    ui_pointer_cursor_override_tick(1.0f);
    if (state->pointer_cursor != NULL) {
        state->pointer_cursor->selected = 0;
        if (ops != NULL && ops->cursor_position_get != NULL) {
            ops->cursor_position_get(ops->context, &state->pointer_x,
                                     &state->pointer_y);
        }
        pointer_cursor_draw(state->pointer_cursor,
                            state->pointer_cursor_hooks,
                            state->pointer_x, state->pointer_y);
    }
    if (ops != NULL && ops->cursor_input_update != NULL)
        ops->cursor_input_update(ops->context);
    state->pointer_override_active = 0;
    if (ops != NULL && ops->pointer_override_clear != NULL)
        ops->pointer_override_clear(ops->context);
    if (ops != NULL && ops->screen_effect_update != NULL)
        ops->screen_effect_update(ops->context, delta);
    if (ops != NULL && ops->pointer_effect_update != NULL)
        ops->pointer_effect_update(ops->context, state->pointer_x,
                                   state->pointer_y, delta * 40.0f);
    timed_effect_runtime_update(renderer_word_a_get(), renderer_word_b_get(),
                                delta * 40.0f);
    if (ops != NULL && ops->frame_pump != NULL)
        ops->frame_pump(ops->context, 1);
}

void reward_pointer_override_expiry_tick_bound(float delta)
{
    reward_pointer_override_expiry_tick(s_reward_screen_state, delta,
                                        s_reward_screen_ops);
}

static void reward_screen_task_visual_draw(RewardScreenState* state,
                                          const RewardScreenOps* ops,
                                          int index, uint32_t mask)
{
    GameTask* task = reward_task_get(state, index);
    Flic* flic;
    if (task == NULL || ops == NULL) return;
    if ((mask == 2u &&
         (state->game->reward.display_images == 0 ||
          (task_flags(task) & TASK_FLAG_IMG) == 0u)) ||
        (mask == 0x40u &&
         (state->game->reward.display_images == 0 ||
          (task_flags(task) & TASK_FLAG_BOXIMG) == 0u)) ||
        (mask == 4u &&
         (state->game->reward.display_flics == 0 ||
          (task_flags(task) & TASK_FLAG_FLIC) == 0u))) return;
    flic = state->task_flics[index];
    if (mask == 4u) {
        if (flic == NULL) {
            reward_task_flic_reload(state, index);
            flic = state->task_flics[index];
        }
        if (flic != NULL) flic_tick_loop(flic, 1);
    }
    if (ops->task_visual_draw == NULL) return;
    ops->task_visual_draw(ops->context, task, index, mask,
                          state->displayed_task_count, state->elapsed, flic);
}

/* RewardDisplayTick (FUN_00461790). `task_number` is one-based in the
 * reward loop (zero is also accepted for its first slot); the input record is
 * the checkpoint copy, which is what the original renderer reads. */
// FUNCTION: LEGORR 0x00461790
void reward_display_task_abi(GameTask* task, uint32_t draw_mask,
                             uint32_t task_number, float elapsed)
{
    RewardScreenState* state = s_reward_screen_state;
    const RewardScreenOps* ops = s_reward_screen_ops;
    int index;
    uint32_t flags;
    Flic* flic;
    const char* path;

    if (state == NULL || state->game == NULL || task == NULL || ops == NULL)
        return;
    index = task_number == 0u ? 0 : (int)task_number - 1;
    if (index < 0 || index >= GAME_TASK_COUNT) return;
    flags = task_flags(task);
    if (draw_mask == 2u &&
        (state->game->reward.display_images == 0 ||
         (flags & TASK_FLAG_IMG) == 0u)) return;
    if (draw_mask == 0x40u &&
        (state->game->reward.display_images == 0 ||
         (flags & TASK_FLAG_BOXIMG) == 0u)) return;
    if (draw_mask == 4u &&
        (state->game->reward.display_flics == 0 ||
         (flags & TASK_FLAG_FLIC) == 0u)) return;
    if ((draw_mask == 1u || draw_mask == 0x10u || draw_mask == 0x20u) &&
        state->game->reward.display_text == 0) return;
    if (draw_mask == 1u && (flags & TASK_FLAG_COMPLETED) == 0u) return;
    if (draw_mask == 0x10u && (flags & TASK_FLAG_TEXT) == 0u) return;
    if (draw_mask == 0x20u && (flags & TASK_FLAG_BOXTEXT) == 0u) return;
    if (draw_mask != 1u && draw_mask != 2u && draw_mask != 4u &&
        draw_mask != 0x10u && draw_mask != 0x20u && draw_mask != 0x40u)
        return;

    if (draw_mask == 1u) {
        if (ops->task_text_draw != NULL)
            ops->task_text_draw(ops->context, task, index,
                                state->displayed_task_count, elapsed);
        return;
    }

    flic = state->task_flics[index];
    if (draw_mask == 4u) {
        if (flic == NULL) {
            path = (const char*)task->data + TASK_OFF_FLICPATH2;
            if (path[0] != '\0') {
                flic = flic_open(path);
                state->task_flics[index] = flic;
            }
        }
        if (flic != NULL) flic_tick_loop(flic, 1);
    }
    if (ops->task_visual_draw != NULL) {
        ops->task_visual_draw(ops->context, task, index, draw_mask,
                              state->displayed_task_count, elapsed, flic);
    }
    if ((draw_mask == 0x10u || draw_mask == 0x20u) &&
        ops->task_text_draw != NULL) {
        ops->task_text_draw(ops->context, task, index,
                            state->displayed_task_count, elapsed);
    }
}

/* RewardScoreTaskTextDraw (FUN_00461c90). The native caller supplies the
 * current displayed-task count and elapsed time; the score record is the
 * final task slot in the active reward context. Its ordinary task text is
 * submitted through RewardDisplayTick before the separate score string. */
// FUNCTION: LEGORR 0x00461c90
int reward_score_task_text_draw(int displayed_task_count, float elapsed)
{
    RewardScreenState* state = s_reward_screen_state;
    const RewardScreenOps* ops = s_reward_screen_ops;
    GameContext* game;
    int drawn = 0;
    if (state == NULL || state->game == NULL || state->game->level == NULL)
        return 0;
    game = state->game;
    if (game->level->objective_result != 1) return 0;

    reward_display_task_abi(&game->tasks[GAME_TASK_COUNT - 1], 1u,
                            (uint32_t)displayed_task_count, elapsed);
    if (ops != NULL && ops->score_text_draw != NULL) {
        drawn = ops->score_text_draw(ops->context, game,
                                     displayed_task_count, elapsed);
    }
    return drawn;
}

/* RewardFlhContextTick (FUN_004624a0) updates the timer/index tuple supplied
 * by the reward loop, and performs task audio and transition cleanup. */
// FUNCTION: LEGORR 0x004624a0
void reward_flic_context_tick_abi(int* displayed_task_count,
                                  int* transition_stage, float* elapsed)
{
    RewardScreenState* state = s_reward_screen_state;
    if (state == NULL || state->game == NULL || displayed_task_count == NULL ||
        transition_stage == NULL || elapsed == NULL) return;
    state->displayed_task_count = *displayed_task_count;
    state->transition_stage = *transition_stage;
    state->elapsed = *elapsed;
    reward_screen_task_timer_tick(state, s_reward_screen_ops);
    *displayed_task_count = state->displayed_task_count;
    *transition_stage = state->transition_stage;
    *elapsed = state->elapsed;
}

// FUNCTION: LEGORR 0x00462ad0
extern "C" int reward_scroll_ticker_advance_and_draw(
    RewardScrollTickerState* state)
{
    uint32_t i;
    if (state == NULL || state->entry_get == NULL || state->entry_draw == NULL)
        return 0;
    for (i = 0; i < state->entry_count; ++i) {
        RewardScrollTickerEntry entry;
        int y;
        int upper;
        int clipped;
        if (!state->entry_get(state->context, i, &entry)) continue;
        y = entry.y;
        if ((state->flags & 1u) != 0u && entry.line_height != 0) {
            while ((float)y < state->minimum_y)
                y += entry.line_height;
            upper = (int)(state->scroll_height + state->minimum_y +
                          state->lower_clip);
            while ((float)y > (float)upper) y -= entry.line_height;
        }
        clipped = (entry.flags & 2u) == 0u &&
            ((float)y > state->minimum_y + state->lower_clip ||
             (float)y < state->minimum_y);
        if (!clipped || (entry.flags & 2u) != 0u)
            state->entry_draw(state->context, &entry, entry.x, y, clipped);
    }
    state->current_y -= state->scroll_speed;
    if ((state->flags & 0x10u) != 0u && state->debug_key_get != NULL) {
        if (state->debug_key_get(state->context, 0u)) state->scroll_speed = 0.0f;
        if (state->debug_key_get(state->context, 1u))
            state->current_y = state->minimum_y;
        if (state->debug_key_get(state->context, 2u))
            state->scroll_speed += 0.01f;
        if (state->debug_key_get(state->context, 3u))
            state->scroll_speed -= 0.01f;
        for (i = 0; i < 10u; ++i) {
            if (state->debug_key_get(state->context, i + 4u)) {
                float magnitude = (float)i * 0.1f;
                state->scroll_speed = state->scroll_speed < 0.0f
                    ? -magnitude : magnitude;
            }
        }
    }
    return 1;
}

// FUNCTION: LEGORR 0x00461410
int reward_screen_tick(void)
{
    RewardScreenState* state = s_reward_screen_state;
    const RewardScreenOps* ops = s_reward_screen_ops;
    GameContext* game;
    uint32_t now;
    int display_mode;
    int i;
    if (state == NULL || ops == NULL || state->game == NULL) return 0;
    game = state->game;
    if (game->level == NULL || game->checkpoint_blob == NULL ||
        ops->save_button_check == NULL || ops->time_get_ms == NULL ||
        ops->fast_forward_pressed == NULL || ops->frame_pump == NULL ||
        !ops->save_button_check(ops->context, game)) return 0;
    now = ops->time_get_ms != NULL ? ops->time_get_ms(ops->context) : 0u;
    state->last_clock_ms = now;
    if (ops->display_mode_get != NULL)
        display_mode = ops->display_mode_get(ops->context, game);
    else display_mode = 0;

    while (state->transition_stage < 4 ||
           state->pointer_override_active ||
           (ops->loop_should_continue != NULL &&
            ops->loop_should_continue(ops->context, state))) {
        uint32_t current_ms = ops->time_get_ms != NULL
            ? ops->time_get_ms(ops->context) : state->last_clock_ms;
        float delta = (float)(uint32_t)(current_ms - state->last_clock_ms)
            * 0.001f;
        int task_limit;
        int fast_forward;
        int score_text_drawn = 0;
        state->last_clock_ms = current_ms;
        state->elapsed += delta;
        reward_screen_task_timer_tick(state, ops);
        fast_forward = ops->fast_forward_pressed != NULL &&
            ops->fast_forward_pressed(ops->context);
        if (fast_forward && state->transition_stage < 3) {
            ++state->transition_stage;
            if (state->transition_stage == 3)
                state->transition_stage = 2;
            if (state->transition_stage < 2 || display_mode != 1)
                state->displayed_task_count = 9;
            else state->displayed_task_count = 10;
        }
        if (state->scroll_ticker != NULL)
            reward_scroll_ticker_advance_and_draw(state->scroll_ticker);
        for (i = 0; i < state->displayed_task_count; ++i)
            reward_screen_task_visual_draw(state, ops, i, 2u);
        task_limit = state->displayed_task_count < 9
            ? state->displayed_task_count : 9;
        for (i = 0; i < task_limit; ++i)
            reward_screen_task_visual_draw(state, ops, i, 0x40u);
        for (i = 0; i < state->displayed_task_count; ++i)
            reward_screen_task_visual_draw(state, ops, i, 4u);

        if ((state->displayed_task_count == 10 ||
             (display_mode != 1 && state->displayed_task_count == 9)) &&
            ops->score_text_gate_ready != NULL &&
            ops->score_text_gate_ready(ops->context, game))
            state->score_text_gate = 1;
        for (i = 0; game->reward.display_text != 0 && i < task_limit; ++i) {
            GameTask* task = reward_task_get(state, i);
            if (task != NULL && ops->task_text_draw != NULL)
                ops->task_text_draw(ops->context, task, i,
                                    state->displayed_task_count,
                                    state->elapsed);
        }
        if (state->score_text_gate && game->reward.display_text != 0) {
            score_text_drawn = reward_score_task_text_draw(
                state->displayed_task_count, state->elapsed);
            if (score_text_drawn && state->transition_stage < 3 &&
                ops->reward_zone_check != NULL)
                ops->reward_zone_check(ops->context, state);
        }
        if (ops->reward_stats_draw != NULL)
            ops->reward_stats_draw(ops->context, game);
        if (state->transition_stage == 3) state->transition_stage = 4;
        reward_screen_transition_tick(state, ops);
        reward_pointer_override_expiry_tick(state, delta, ops);
    }

    if (ops->reward_scroll_cleanup != NULL)
        ops->reward_scroll_cleanup(ops->context);
    if (ops->render_finish != NULL) ops->render_finish(ops->context);
    reward_task_flics_close_all(state);
    if (ops->input_restore != NULL) ops->input_restore(ops->context, 1);
    if (display_mode == 1) {
        if (ops->completion_audio != NULL)
            ops->completion_audio(ops->context, game);
        sound_request_queue_submit(0x2c, 1);
        if (ops->frame_pump != NULL) ops->frame_pump(ops->context, 1);
    }
    return 1;
}

/* Explicit-state adapter for portable callers. The retail entry point above
 * is global and no-argument; save and restore the binding so nested UI work
 * cannot replace the caller's reward state after returning. */
int reward_screen_tick_with_ops(RewardScreenState* state,
                                const RewardScreenOps* ops)
{
    RewardScreenState* const previous_state = s_reward_screen_state;
    const RewardScreenOps* const previous_ops = s_reward_screen_ops;
    int result;
    if (state == NULL) return 0;
    s_reward_screen_state = state;
    s_reward_screen_ops = ops;
    result = reward_screen_tick();
    s_reward_screen_state = previous_state;
    s_reward_screen_ops = previous_ops;
    return result;
}
