// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gods98/block.hpp"
#include "gods98/message.hpp"
#include "entity.hpp"
#include "level.hpp"
#include "mission.hpp"
#include "objective.hpp"
#include "savegame.hpp"
#include "util.hpp"

/* The ten named task slots (TaskBlockInit local list). */
static const char* TASK_NAMES[GAME_TASK_COUNT] = {
    "Crystals", "Ore", "Diggable", "Constructions", "Caverns",
    "Figures", "RockMonsters", "Oxygen", "Timer", "Score",
};

#define MISS_CRYSTAL_QUOTA 26 /* crystal display budget (CrystalQuotaSetup) */

static int strcasecmp_lite(const char* a, const char* b)
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

/* Corpus FUN_004790d0 typed getter (TRUE/YES/ON=1, FALSE/NO/OFF=0, else
 * strtol; missing -> dflt) — RewardInit reads `Display`/`CentreText` via it. */
static int rw_typed(BlockFile* cfg, const char* root, const char* level,
                    const char* name, int dflt)
{
    const char* v = rw_str(cfg, root, level, NULL, name);
    if (v == NULL) {
        return dflt;
    }
    if (strcasecmp_lite(v, "TRUE") == 0 || strcasecmp_lite(v, "YES") == 0
        || strcasecmp_lite(v, "ON") == 0) {
        return 1;
    }
    if (strcasecmp_lite(v, "FALSE") == 0 || strcasecmp_lite(v, "NO") == 0
        || strcasecmp_lite(v, "OFF") == 0) {
        return 0;
    }
    char* end = NULL;
    long r = strtol(v, &end, 10);
    return (end == v) ? 2 : (int)r;
}

/* Store a short token string at a task payload offset (bounded write). */
static void task_put_str(GameTask* t, int off, const char* s, int cap)
{
    char* dst = (char*)t->data + off;
    dst[0] = '\0';
    if (s == NULL) {
        return;
    }
    for (int i = 0; i < cap - 1 && s[i] != '\0'; ++i) {
        dst[i] = s[i];
        dst[i + 1] = '\0';
    }
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

/* StrReplace (FUN_00413f00): replace one byte through the bounded reward
 * text field. The original scans to NUL; the 0x20 cap preserves this port's
 * modeled field boundary. */
// FUNCTION: LEGORR 0x00413f00
static void mission_replace_char(char* text, char from, char to, int limit)
{
    for (int i = 0; i < limit && text[i] != '\0'; ++i) {
        if (text[i] == from) {
            text[i] = to;
        }
    }
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
    task_put_int(t, TASK_OFF_IMG_XY, (int32_t)util_atoi(f[1]));
    task_put_int(t, TASK_OFF_IMG_XY + 4, (int32_t)util_atoi(f[2]));
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
    mission_replace_char((char*)t->data + TASK_OFF_TEXT, '_', ' ', 0x20);
    task_put_int(t, TASK_OFF_TEXT_XY, (int32_t)util_atoi(f[1]));
    task_put_int(t, TASK_OFF_TEXT_XY + 4, (int32_t)util_atoi(f[2]));
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
    for (int i = 0; i < 4; ++i) {
        task_put_int(t, TASK_OFF_FLIC_RECT + i * 4, (int32_t)util_atoi(f[1 + i]));
    }
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
    task_put_int(t, TASK_OFF_BOXIMG_XY, (int32_t)util_atoi(f[1]));
    task_put_int(t, TASK_OFF_BOXIMG_XY + 4, (int32_t)util_atoi(f[2]));
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
    int i;
    /* Zero the slots and name them (TaskBlockInit's name pass). */
    for (i = 0; i < GAME_TASK_COUNT; ++i) {
        __builtin_memset(&ctx->tasks[i], 0, GAME_TASK_STRIDE);
        task_set_name(&ctx->tasks[i], TASK_NAMES[i]);
    }
    if (cfg == NULL || level_name == NULL) {
        return;
    }
    /* The six field loaders, in the corpus's TaskBlockInit order. */
    for (i = 0; i < GAME_TASK_COUNT; ++i) {
        GameTask* t = &ctx->tasks[i];
        task_loader_images(cfg, root, level_name, t);
        task_loader_text(cfg, root, level_name, t);
        task_loader_flics(cfg, root, level_name, t);
        task_loader_font(cfg, root, level_name, t);
        task_loader_sound(cfg, root, level_name, t);
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
    if (ctx != NULL) ctx->mission.score += v;
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
    return completed;
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
    if (cfg == NULL || level_name == NULL) {
        return;
    }
    const char* root = util_root_name(cfg);
    /* RewardInit @ 0x45f7f0 keys (corpus engine-region.c 6485-6557). */
    const char* v;
    if ((v = rw_str(cfg, root, level_name, NULL, "Wallpaper")) != NULL) {
        snprintf(rs->wallpaper, sizeof(rs->wallpaper), "%s", v);
    }
    rs->display = rw_typed(cfg, root, level_name, "Display", 0);
    if (rs->display == 2) rs->display = 0;
    rs->centre_text = rw_typed(cfg, root, level_name, "CentreText", 0);
    if (rs->centre_text == 2) rs->centre_text = 0;
    rs->scroll_speed = rw_float(cfg, root, level_name, NULL, "ScrollSpeed", 60.0f);
    rs->vert_spacing = rw_float(cfg, root, level_name, NULL, "VertSpacing", 24.0f);
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
            rs->text_pos[0] = (int)util_atoi(f[0]);
            rs->text_pos[1] = (int)util_atoi(f[1]);
        }
    }
    /* Save/Advance button hit zones (FUN_0045faf0): `file|x|y` triples from
     * `Reward SaveButton` / `Reward AdvanceButton`. Deltas + button
     * *widths* come from the surface cells the corpus reads — the position
     * pair is captured here for RewardZoneCheck. */
    if ((v = rw_str(cfg, root, level_name, NULL, "SaveButton")) != NULL) {
        char f[UTIL_QUERY_MAX_FIELDS][256];
        if (task_fields(v, f) != 0) {
            rs->save_x = (float)util_atoi(f[1]);
            rs->save_y = (float)util_atoi(f[2]);
        }
    }
    if ((v = rw_str(cfg, root, level_name, NULL, "AdvanceButton")) != NULL) {
        char f[UTIL_QUERY_MAX_FIELDS][256];
        if (task_fields(v, f) != 0) {
            rs->advance_x = (float)util_atoi(f[1]);
            rs->advance_y = (float)util_atoi(f[2]);
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
