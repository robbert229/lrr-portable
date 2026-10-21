// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.hpp"
#include "gods98/block.hpp"
#include "gods98/resource.hpp"
#include "level.hpp"
#include "savegame.hpp"
#include "util.hpp"

#if defined(LEGORRR_BACKEND_WINAPI)
#  define SAVE_SEP '\\'  /* the corpus's "%s\%i.sav" DOS separators */
#else
#  define SAVE_SEP '/'   /* host-native on the POSIX port */
#endif

#if defined(LEGORRR_BACKEND_WINAPI)
#  include <direct.h>
#  define LRR_MKDIR(p) _mkdir(p)
#else
#  include <sys/stat.h>
#  define LRR_MKDIR(p) mkdir(p, 0755)
#endif

/* Reward/Timer default when the key is absent (finding 08): 76.9375. */
#define LVL_REWARD_TIMER_DEFAULT 76.9375f

/* The `.sav` open modes (corpus constants): DAT_004a2770 ("rb") and
 * DAT_004a2794 ("wb"). */
static const char* MODE_RB = "rb";
static const char* MODE_WB = "wb";

void save_init(GameContext* ctx)
{
    if (ctx == NULL) {
        return;
    }
    ctx->save_slot = -1;
    snprintf(ctx->saves_dir, sizeof(ctx->saves_dir), "Saves");
    ctx->current_record = 0;
    ctx->active_slot = -1;
    ctx->record_count = 0;
    ctx->records = NULL;
    ctx->checkpoint_blob = NULL;
    ctx->osf_count = 0;
    ctx->osf_capacity = 0;
    ctx->osf_dirty = 0;
    ctx->osf_shadow_count = 0;
    ctx->osf_shadow_dirty = 0;
}

int save_slot_get(const GameContext* ctx)
{
    return ctx != NULL ? ctx->save_slot : -1;
}

void save_slot_set(GameContext* ctx, int slot)
{
    if (ctx != NULL) {
        ctx->save_slot = slot;
    }
}

void save_slot_path(GameContext* ctx, char* out, size_t out_sz, int slot,
                    const char* ext)
{
    const char* dir = (ctx != NULL && ctx->saves_dir[0] != '\0')
                      ? ctx->saves_dir : "Saves";
    snprintf(out, out_sz, "%s%c%d.%s", dir, SAVE_SEP, slot, ext);
}

/* Release the active slot records (used between slots / on teardown). */
static void save_release_records(GameContext* ctx)
{
    free(ctx->records);
    ctx->records = NULL;
    ctx->record_count = 0;
    ctx->active_slot = -1;
}

/* Fresh empty slot context — corpus SavWriteSlot's bootstrap: count 5,
 * zero-filled records (finding 08: "bootstraps slot 5 with a zero-filled
 * record array"). */
static int save_bootstrap_records(GameContext* ctx, int count)
{
    if (count < 0) {
        return 0;
    }
    if (count == 0) {
        count = SAVE_DEFAULT_RECORDS;
    }
    uint8_t* rec = (uint8_t*)calloc((size_t)count, SAVE_RECORD_SIZE);
    if (rec == NULL) {
        return 0;
    }
    save_release_records(ctx);
    ctx->record_count = count;
    ctx->records = rec;
    return 1;
}

/* Serialise the active slot to `<dir>\<slot>.sav` (SavWriteSlot). */
static int sav_write_slot_file(GameContext* ctx)
{
    /* The corpus driver creates the Saves/ directory; the writer mirrors it. */
    LRR_MKDIR(ctx->saves_dir[0] != '\0' ? ctx->saves_dir : "Saves");
    char path[280];
    save_slot_path(ctx, path, sizeof(path), ctx->save_slot, "sav");
    FILE* fp = fopen(path, MODE_WB);
    if (fp == NULL) {
        return 0;
    }
    uint8_t header[SAVE_HEADER_SIZE] = { 0 };
    int32_t count = (int32_t)ctx->record_count;
    __builtin_memcpy(header + 0x18, &count, 4);
    size_t wrote = fwrite(header, 1, SAVE_HEADER_SIZE, fp);
    if (wrote == SAVE_HEADER_SIZE && count > 0) {
        wrote = fwrite(ctx->records, 1, (size_t)count * SAVE_RECORD_SIZE, fp);
    }
    int ok = (wrote != 0);
    fclose(fp);
    if (!ok) {
        return 0;
    }
    /* After a write, refresh the shadow OSF table too (.osf sidecar). */
    (void)osf_write_path(ctx, ctx->save_slot);
    return 1;
}

int save_read_slot(GameContext* ctx, int slot, int create_if_missing)
{
    if (ctx == NULL || slot < 0 || slot >= SAVE_SLOTS) {
        return 0;
    }
    char path[280];
    save_slot_path(ctx, path, sizeof(path), slot, "sav");
    FILE* fp = fopen(path, MODE_RB);
    if (fp == NULL) {
        if (!create_if_missing) {
            save_slot_set(ctx, slot);
            save_bootstrap_records(ctx, SAVE_DEFAULT_RECORDS);
            sav_write_slot_file(ctx);
        }
        return 0;
    }

    uint8_t header[SAVE_HEADER_SIZE];
    size_t got = fread(header, 1, SAVE_HEADER_SIZE, fp);
    if (got != SAVE_HEADER_SIZE) {
        fclose(fp);
        return 0;
    }
    int32_t count;
    __builtin_memcpy(&count, header + 0x18, 4);
    if (count <= 0 || count > 0x1000) {
        fclose(fp);
        if (!create_if_missing) {
            /* Inconsistent slot: the corpus zeroes and returns 0. */
            save_release_records(ctx);
            return 0;
        }
        return 0;
    }
    uint8_t* rec = (uint8_t*)malloc((size_t)count * SAVE_RECORD_SIZE);
    if (rec == NULL) {
        fclose(fp);
        return 0;
    }
    got = fread(rec, 1, (size_t)count * SAVE_RECORD_SIZE, fp);
    fclose(fp);
    if (got != (size_t)count * SAVE_RECORD_SIZE) {
        free(rec);
        return 0;
    }
    save_release_records(ctx);
    ctx->active_slot = slot;
    ctx->record_count = count;
    ctx->records = rec;
    ctx->save_slot = slot;
    return 1;
}

int save_write_slot(GameContext* ctx, int slot)
{
    if (ctx == NULL || slot < 0 || slot >= SAVE_SLOTS) {
        return 0;
    }
    int prev = ctx->save_slot;
    ctx->save_slot = slot;
    if (ctx->records == NULL) {
        save_bootstrap_records(ctx, SAVE_DEFAULT_RECORDS);
    }
    int ok = sav_write_slot_file(ctx);
    ctx->save_slot = prev;
    return ok;
}

void save_commit(GameContext* ctx)
{
    if (ctx == NULL || ctx->save_slot < 0) {
        return;
    }
    if (ctx->records == NULL) {
        save_bootstrap_records(ctx, SAVE_DEFAULT_RECORDS);
    }
    sav_write_slot_file(ctx);
}

void save_set_current_record(GameContext* ctx, int index)
{
    if (ctx != NULL) {
        ctx->current_record = index;
    }
}

uint32_t* save_record_flags(GameContext* ctx, int index)
{
    if (ctx == NULL || ctx->records == NULL || index < 0
        || index >= ctx->record_count) {
        return NULL;
    }
    return (uint32_t*)(ctx->records + (size_t)index * SAVE_RECORD_SIZE);
}

void save_mark_record(GameContext* ctx, int index)
{
    uint32_t* flags = save_record_flags(ctx, index);
    if (flags == NULL) {
        return;
    }
    *flags |= 1u;   /* dirty */
    /* `|2` marks an empty-name/present record (corpus SavMarkRecord). */
    *flags |= 2u;
}

/* Grow the record array until `index` is addressable. The corpus bootstraps
 * a 5-record array and lets the sav-tree walk (SavTreeWalk2) widen it as
 * levels enumerate; the port grows the same way when a checkpoint lands
 * past the bootstrap count (the campaign has 24 records, tutorials start
 * at 0, so record_count reaches 24+8). */
static int save_records_grow(GameContext* ctx, int index)
{
    if (ctx->records == NULL || index < ctx->record_count) {
        return ctx->records != NULL;
    }
    int n = ctx->record_count > 0 ? ctx->record_count : SAVE_DEFAULT_RECORDS;
    while (n <= index) {
        n *= 2;
    }
    uint8_t* rec = (uint8_t*)realloc(ctx->records,
                                     (size_t)n * SAVE_RECORD_SIZE);
    if (rec == NULL) {
        return 0;
    }
    memset(rec + (size_t)ctx->record_count * SAVE_RECORD_SIZE, 0,
           (size_t)(n - ctx->record_count) * SAVE_RECORD_SIZE);
    ctx->records = rec;
    ctx->record_count = n;
    return 1;
}

int save_write_record(GameContext* ctx, int index, const uint8_t* blob)
{
    if (ctx == NULL || index < 0 || blob == NULL
        || !save_records_grow(ctx, index)) {
        return 0;
    }
    __builtin_memcpy(ctx->records + (size_t)index * SAVE_RECORD_SIZE + 4,
                     blob, SAVE_BLOB_SIZE);
    return 1;
}

/* ------------------------------------------------------------------ *
 *  Checkpoint blob (CheckpointBlobBuild @ 0x45fda0).
 * ------------------------------------------------------------------ */

static void checkpoint_reward_group(GameContext* ctx, const char* level_name)
{
    if (ctx->cfg == NULL || level_name == NULL) {
        return;
    }
    char path[512];
    /* Reward/Enable tri-state (2 -> 0). Shipped config nests the level
     * under root -> Levels, so resolve each key through the container. */
    if (util_level_cfg_path(ctx->cfg, level_name, "Reward Enable", path,
                            sizeof(path)) != NULL) {
        int en = block_get_bool(ctx->cfg, path);
        if (en == 2) en = 0;
        ctx->reward.enable = en;
    }
    if (util_level_cfg_path(ctx->cfg, level_name, "Reward Modifier", path,
                            sizeof(path)) != NULL) {
        ctx->reward.modifier = block_get_int(ctx->cfg, path, 0);
    }
    if (util_level_cfg_path(ctx->cfg, level_name, "Reward Timer", path,
                            sizeof(path)) != NULL) {
        ctx->reward.timer =
            block_get_float(ctx->cfg, path, LVL_REWARD_TIMER_DEFAULT);
    }
    if (util_level_cfg_path(ctx->cfg, level_name, "Reward DisplayText", path,
                            sizeof(path)) != NULL) {
        int dt = block_get_bool(ctx->cfg, path);
        ctx->reward.display_text = dt == 2 ? 1 : dt;
    }
    if (util_level_cfg_path(ctx->cfg, level_name, "Reward DisplayImages", path,
                            sizeof(path)) != NULL) {
        int di = block_get_bool(ctx->cfg, path);
        ctx->reward.display_images = di == 2 ? 1 : di;
    }
    if (util_level_cfg_path(ctx->cfg, level_name, "Reward DisplayFlics", path,
                            sizeof(path)) != NULL) {
        int df = block_get_bool(ctx->cfg, path);
        ctx->reward.display_flics = df == 2 ? 1 : df;
    }
}

void checkpoint_blob_build(GameContext* ctx)
{
    if (ctx == NULL) {
        return;
    }
    uint8_t* blob = (uint8_t*)calloc(1, SAVE_BLOB_SIZE);
    if (blob == NULL) {
        return;
    }
    const char* level_name = ctx->level != NULL
                             ? (const char*)ctx->level->ptr_name : NULL;
    checkpoint_reward_group(ctx, level_name);

    int32_t enable = (int32_t)ctx->reward.enable;
    int32_t mod = (int32_t)ctx->reward.modifier;
    int32_t ten = GAME_TASK_COUNT;
    __builtin_memcpy(blob + 0x00, &enable, 4);          /* Reward/Enable   */
    __builtin_memcpy(blob + 0x1C0, &mod, 4);            /* Reward/Modifier  */
    __builtin_memcpy(blob + 0x1CE4, &ten, 4);           /* task count       */
    uint32_t self = 0x1CEC;
    __builtin_memcpy(blob + 0x1CE8, &self, 4);          /* low32 self-ref   */
    /* Copy the ten task records (stride 0x210) verbatim. */
    __builtin_memcpy(blob + 0x1CEC, ctx->tasks, GAME_TASK_COUNT * GAME_TASK_STRIDE);

    /* Tutorial hard-code: Level03 forces blob +0x868 = 1. */
    if (level_name != NULL && strcmp(level_name, "Levels\\Level03") == 0) {
        blob[0x868] = 1;
    }

    free(ctx->checkpoint_blob);
    ctx->checkpoint_blob = blob;
}

void checkpoint_blob_free(GameContext* ctx)
{
    if (ctx != NULL) {
        free(ctx->checkpoint_blob);
        ctx->checkpoint_blob = NULL;
    }
}

void checkpoint_blob_apply(GameContext* ctx, const uint8_t* blob)
{
    if (ctx == NULL || blob == NULL) {
        return;
    }
    int32_t enable = 0, mod = 0;
    __builtin_memcpy(&enable, blob + 0x00, 4);
    __builtin_memcpy(&mod, blob + 0x1C0, 4);
    ctx->reward.enable = enable;
    ctx->reward.modifier = mod;
    __builtin_memcpy(ctx->tasks, blob + 0x1CEC,
                     (size_t)GAME_TASK_COUNT * (size_t)GAME_TASK_STRIDE);
}

int save_slot_state(GameContext* ctx, int slot, int* out_filled,
                    int* out_completed, int* out_resume)
{
    if (ctx == NULL || slot < 0 || slot >= SAVE_SLOTS) {
        return 0;
    }
    if (out_filled) { *out_filled = 0; }
    if (out_completed) { *out_completed = 0; }
    if (out_resume) { *out_resume = -1; }
    /* Probe the file directly: save_read_slot(_,_,0) would *create* a fresh
     * slot when the file is absent (finding 08's SavReadSlot bootstrap). */
    char path[280];
    save_slot_path(ctx, path, sizeof(path), slot, "sav");
    FILE* fp = fopen(path, MODE_RB);
    if (fp == NULL) {
        return 0;
    }
    fclose(fp);
    if (!save_read_slot(ctx, slot, 0)) {
        return 0;
    }
    int recs = ctx->record_count > 0 ? ctx->record_count : SAVE_DEFAULT_RECORDS;
    int filled = 0, completed = 0, resume = -1;
    for (int i = 0; i < recs && i < 256; i++) {
        uint32_t* fl = save_record_flags(ctx, i);
        if (fl == NULL) {
            continue;
        }
        const uint8_t* blob = (const uint8_t*)(fl + 1);
        int present = (*fl & 0x3) != 0;
        if (!present) {
            for (size_t b = 0; b < SAVE_BLOB_SIZE; b += 8) {
                if (blob[b] != 0) {
                    present = 1;
                    break;
                }
            }
        }
        if (present) {
            filled++;
            if (*fl & 1) {
                completed++;
                resume = i;
            }
        }
    }
    if (out_filled) { *out_filled = filled; }
    if (out_completed) { *out_completed = completed; }
    if (out_resume) { *out_resume = resume; }
    return 1;
}

void checkpoint_refresh(GameContext* ctx)
{
    if (ctx == NULL) {
        return;
    }
    checkpoint_blob_build(ctx);
}

void checkpoint_step(GameContext* ctx)
{
    if (ctx == NULL) {
        return;
    }
    int orig = ctx->save_slot;
    int slot = orig == -1 ? SAVE_DEFAULT_SLOT : orig;
    if (ctx->records == NULL || ctx->active_slot != slot) {
        /* Bootstrap the slot context (`SavSetSlot(5)` path in the corpus). */
        save_read_slot(ctx, slot, 0);
    }
    int rec = ctx->current_record;
    /* The corpus marks the record dirty when the level's objective-result
     * word (+0x27c) reads 1 ("achieved"), then copies the blob in. */
    int result = 0;
    if (ctx->level != NULL) {
        result = level_int(ctx->level, LEVEL_OFF_OBJ_RESULT);
    }
    if (ctx->checkpoint_blob == NULL) {
        checkpoint_blob_build(ctx);
    }
    /* Write before marking: the write grows the record array if `rec` is
     * past the bootstrap count, so the mark (a flags-word write) can't
     * miss. The corpus marks first, but both touch the same record — the
     * observable order is the same. */
    save_write_record(ctx, rec, ctx->checkpoint_blob);
    if (result == 1) {
        save_mark_record(ctx, rec);
    }
    if (orig == -1) {
        /* First checkpoint with no active slot: commit the default slot
         * then reload its metadata (the corpus's SavCommit + reload). */
        save_commit(ctx);
        save_read_slot(ctx, SAVE_DEFAULT_SLOT, 0);
    }
}

/* ------------------------------------------------------------------ *
 *  Completed-objective table (`.osf`).
 * ------------------------------------------------------------------ */

void osf_add(GameContext* ctx, int32_t id, int32_t ref, const char* name)
{
    if (ctx == NULL || ctx->osf_count >= GAME_OSF_MAX) {
        return;
    }
    GameOsfEntry* e = &ctx->osf[ctx->osf_count++];
    e->id = id;
    e->ref = ref;
    if (name != NULL) {
        for (int i = 0; i < 11 && name[i] != '\0'; ++i) {
            e->name[i] = name[i];
        }
    }
    ctx->osf_dirty = 1;
}

void osf_snapshot(GameContext* ctx)
{
    if (ctx == NULL || ctx->osf_count == 0) {
        return;
    }
    __builtin_memcpy(ctx->osf_shadow, ctx->osf,
                     (size_t)ctx->osf_count * sizeof(GameOsfEntry));
    ctx->osf_shadow_count = ctx->osf_count;
    ctx->osf_shadow_dirty = 1;
}

int osf_write(GameContext* ctx, const char* path)
{
    if (ctx == NULL || path == NULL) {
        return 0;
    }
    FILE* fp = fopen(path, MODE_WB);
    if (fp == NULL) {
        return 0;
    }
    uint8_t hdr[8];
    const uint32_t magic = 0x534F5252u; /* 'R','O' bytes -> "ROS" u32 */
    const uint32_t count = (uint32_t)ctx->osf_count;
    __builtin_memcpy(hdr + 0, &magic, 4);
    __builtin_memcpy(hdr + 4, &count, 4);
    fwrite(hdr, 1, 8, fp);
    if (count > 0) {
        fwrite(ctx->osf, 1, (size_t)count * 20, fp);
    }
    fclose(fp);
    ctx->osf_dirty = 0;
    osf_snapshot(ctx);
    ctx->osf_shadow_dirty = 0;
    return 1;
}

int osf_write_path(GameContext* ctx, int slot)
{
    char path[280];
    save_slot_path(ctx, path, sizeof(path), slot, "osf");
    return osf_write(ctx, path);
}
