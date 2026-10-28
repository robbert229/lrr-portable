// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.hpp"
#include "gods98/block.hpp"
#include "gods98/crt.hpp"
#include "gods98/engine.hpp"
#include "gods98/heap.hpp"
#include "gods98/resource.hpp"
#include "frontend.hpp"
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

/* SaveDirectoryEnsureExists (FUN_0047f6a0): the engine's directory-create
 * wrapper. The port receives the resolved save path directly rather than
 * prefixing the corpus's shared `Data` directory. */
// FUNCTION: LEGORR 0x0047f6a0
void save_directory_ensure_exists(const char* path)
{
    if (path != NULL && path[0] != '\0') {
        LRR_MKDIR(path);
    }
}

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

// FUNCTION: LEGORR 0x00417e90
int save_slot_get(const GameContext* ctx)
{
    return ctx != NULL ? ctx->save_slot : -1;
}

// FUNCTION: LEGORR 0x00417ea0
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
// FUNCTION: LEGORR 0x00417bd0
static int sav_write_slot_file(GameContext* ctx)
{
    save_directory_ensure_exists(ctx->saves_dir[0] != '\0'
                                     ? ctx->saves_dir : "Saves");
    char path[280];
    save_slot_path(ctx, path, sizeof(path), ctx->save_slot, "sav");
    FILE* fp = fopen(path, MODE_WB);
    if (fp == NULL) {
        return 0;
    }
    uint8_t header[SAVE_HEADER_SIZE] = { 0 };
    int32_t count = (int32_t)ctx->record_count;
    __builtin_memcpy(header + 0x18, &count, 4);
    /* Options/Advanced stamp (issue #85): the corpus persists the slot's
     * speed/volume/brightness words alongside the records (SavReadSlot ->
     * OptionsApplyFromSlot); the port stamps an option block at +0x20. */
    options_save_header(&ctx->options, header + 0x20);
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

// FUNCTION: LEGORR 0x00417a90
int save_read_slot(GameContext* ctx, int slot, int create_if_missing)
{
    if (ctx == NULL || slot < 0 || slot >= SAVE_SLOTS) {
        return 0;
    }
    save_directory_ensure_exists(ctx->saves_dir[0] != '\0'
                                     ? ctx->saves_dir : "Saves");
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
    /* Option block (issue #85): restore it when present, else fall back to
     * the documented defaults (an old/absent stamp loads a fresh options
     * context, OptionsLoadOrDefault-style). */
    if (!options_load_header(&ctx->options, header + 0x20)) {
        options_defaults(&ctx->options);
    }
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

/* SavCommit (FUN_00417fc0): the corpus commits the selected slot by
 * writing the open slot context — DAT_005574d0's slot through the
 * port's active-slot model, bootstrap-and-write faithful. */
// FUNCTION: LEGORR 0x00417fc0
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

/* Saves\<slot>.dat — the `.dat` slot thumbnail (FUN_00413b80's save-confirm
 * dump via FUN_0047e440). FUN_0047c8f0's layout, field for field: `BM`,
 * file size = data + 54, reserved 0, data offset 54, DIB header 40, size,
 * positive height (bottom-up), planes 1, 24 bpp, BI_RGB, image size 0,
 * XPel/YPel 1, palette counts 0; pixels BGR, rows padded to four bytes.
 * The port's surface is B,G,R,A in memory (see fe_blit_glyph), so the
 * component map is a straight byte copy. */
// FUNCTION: LEGORR 0x0047c8f0
int save_write_thumbnail(GameContext* ctx, int slot, const void* fb,
                         int pitch, int w, int h)
{
    if (ctx == NULL || fb == NULL || slot < 0 || slot >= SAVE_SLOTS
        || w <= 0 || h <= 0 || w > 640 || pitch < w * 4) {
        return 0;
    }
    const int stride = (w * 3 + 3) & ~3;
    const uint32_t data_off = 54u;
    const uint32_t file_size = data_off + (uint32_t)stride * (uint32_t)h;

    uint8_t hdr[54];
    memset(hdr, 0, sizeof(hdr));
    hdr[0] = 'B';
    hdr[1] = 'M';
    memcpy(hdr + 2, &file_size, 4);
    const uint32_t dib = 40u;
    const uint32_t off32 = data_off;
    const int32_t w32 = w, h32 = h;
    const uint32_t zero = 0;
    const uint16_t planes = 1, bpp = 24;
    memcpy(hdr + 10, &off32, 4);
    memcpy(hdr + 14, &dib, 4);
    memcpy(hdr + 18, &w32, 4);
    memcpy(hdr + 22, &h32, 4);
    memcpy(hdr + 26, &planes, 2);
    memcpy(hdr + 28, &bpp, 2);
    memcpy(hdr + 30, &zero, 4);            /* BI_RGB */
    memcpy(hdr + 34, &zero, 4);            /* image size 0 */
    const uint32_t ppm = 1;
    memcpy(hdr + 38, &ppm, 4);
    memcpy(hdr + 42, &ppm, 4);

    save_directory_ensure_exists(ctx->saves_dir[0] != '\0'
                                     ? ctx->saves_dir : "Saves");
    char path[280];
    save_slot_path(ctx, path, sizeof(path), slot, "dat");
    FILE* fp = fopen(path, MODE_WB);
    if (fp == NULL) {
        return 0;
    }
    int ok = fwrite(hdr, 1, sizeof(hdr), fp) == sizeof(hdr);
    for (int y = 0; y < h && ok; ++y) {
        /* Bottom-up rows: the surface's row 0 (screen top) lands in the
         * file's last row (FUN_0047c8f0's `h - 1 - y` write offset). */
        const uint8_t* src = (const uint8_t*)fb
                           + (size_t)(h - 1 - y) * (size_t)pitch;
        uint8_t line[1920];               /* (640 * 3) row cap */
        for (int x = 0; x < w; ++x) {
            memcpy(line + x * 3, src + (size_t)x * 4, 3);
        }
        if (stride > w * 3) {
            memset(line + w * 3, 0, (size_t)stride - (size_t)w * 3);
        }
        ok = fwrite(line, 1, (size_t)stride, fp) == (size_t)stride;
    }
    fclose(fp);
    return ok;
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

// FUNCTION: LEGORR 0x00417eb0
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

/* SavWriteRecord (FUN_00417f40) — see savegame.hpp. */
// FUNCTION: LEGORR 0x00417f40
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

// FUNCTION: LEGORR 0x0045fda0
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

    /* Tutorial hard-code (CheckpointBlobBuild tail, corpus 0x460066-0x46008c):
     * when the level name matches "Levels::Level03", record_base + 0x868 —
     * i.e. task slot 4 ("Caverns") quota, record_base + 4*0x210 + 0x28 — is
     * forced to 1 as a dword write. */
    const char* base = level_name != NULL ? strrchr(level_name, ':') : NULL;
    if (base == NULL && level_name != NULL) {
        base = strrchr(level_name, '\\');
    }
    if (base != NULL) {
        ++base;
    } else {
        base = level_name;
    }
    /* FUN_0048ff10 is a case-folded strcmp, so match "Level03" the same way. */
    static const char LVL03[] = "Level03";
    int l03 = base != NULL && strcasecmp_lite(base, LVL03) == 0;
    if (l03) {
        int32_t one = 1;
        __builtin_memcpy(blob + 0x1CEC + 0x868, &one, 4);
    }

    free(ctx->checkpoint_blob);
    ctx->checkpoint_blob = blob;
}

/* CheckpointBlobFree (FUN_004601e0, finding 08): frees the blob. The
 * corpus frees its global and LEAVES the dangling pointer; the port
 * nulls it (documented deviation — port-owned storage). */
// FUNCTION: LEGORR 0x004601e0
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
    save_directory_ensure_exists(ctx->saves_dir[0] != '\0'
                                     ? ctx->saves_dir : "Saves");
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

// FUNCTION: LEGORR 0x00462270
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

// FUNCTION: LEGORR 0x00459190
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

// FUNCTION: LEGORR 0x004592e0
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

// FUNCTION: LEGORR 0x004593e0
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

/* The corpus drops its live OSF table and resets its count/capacity before
 * reading another sidecar. The port's fixed table has no allocation to free;
 * its count/capacity words are the corresponding state. */
// FUNCTION: LEGORR 0x004592a0
static void osf_read_state_reset(GameContext* ctx)
{
    if (ctx == NULL) {
        return;
    }
    ctx->osf_count = 0;
    ctx->osf_capacity = 0;
}

/* GameFileLengthGet (FUN_0047fca0): query the stream length and restore its
 * original position, matching the original's save/seek-end/read-position /
 * seek-back sequence. */
// FUNCTION: LEGORR 0x0047fca0
static long save_stream_length(FILE* fp)
{
    long position = ftell(fp);
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, position, SEEK_SET);
    return length;
}

/* Load-side `.osf` restore — OsfRead @ 0x459470, entered from LevelLoad @
 * 0x429890 (finding 08 §".osf load-side restore"). Mirrors FUN_00459470:
 * clear the live table first (OsfClear @ 0x4592a0), open `path` "rb",
 * validate the `'RO'` magic (`0x534f5252`) and the exact
 * `file_size == 8 + count*0x14` shape (the count is u32, read with the
 * header), then repopulate the live table and snapshot it into the shadow
 * (OsfSnapshot @ 0x4592e0) with the dirty flag cleared. A missing or
 * corrupt sidecar degrades to an empty table — the corpus's failure mode,
 * no crash. The return mirrors FUN_00459470's `uVar3`: 1 whenever the
 * file opened (valid or not), 0 when it was absent; the *installed state*
 * (osf_count == 0 on a bad sidecar) is what distinguishes them. `count`
 * is additionally capped at the port's fixed-array GAME_OSF_MAX (the
 * corpus reallocs its table; the port's array cannot grow past that). */
// FUNCTION: LEGORR 0x00459470
int osf_read(GameContext* ctx, const char* path)
{
    if (ctx == NULL || path == NULL) {
        return 0;
    }
    /* The port also clears its shadow here (the corpus frees/reallocs its
     * shadow on the next snapshot; clearing is the fixed-array equivalent,
     * leaving nothing shadowed or stale from an earlier slot). */
    osf_read_state_reset(ctx);
    ctx->osf_shadow_count = 0;
    ctx->osf_shadow_dirty = 0;
    FILE* fp = fopen(path, MODE_RB);
    if (fp == NULL) {
        return 0;
    }
    long int fsz = save_stream_length(fp);
    uint8_t hdr[8];
    size_t got = fread(hdr, 1, 8, fp);
    int ok = (got == 8);
    uint32_t magic = 0, count_le = 0;
    if (ok) {
        __builtin_memcpy(&magic, hdr + 0, 4);    /* 'RO' magic (LE u32)  */
        __builtin_memcpy(&count_le, hdr + 4, 4); /* entry count (u32)    */
        ok = (magic == 0x534F5252u)
          && ((int64_t)fsz - 8 == (int64_t)count_le * 0x14)
          && count_le <= GAME_OSF_MAX;
    }
    if (ok) {
        size_t n = (size_t)count_le * 0x14;      /* count * 20 bytes */
        if (fread(ctx->osf, 1, n, fp) == n) {
            ctx->osf_count = (int)count_le;
            ctx->osf_capacity = (int)count_le;
            osf_snapshot(ctx);                   /* live -> shadow     */
            ctx->osf_dirty = 0;                  /* DAT_004ffe64 = 0   */
            ctx->osf_shadow_dirty = 0;
        }
        /* else: corrupt (short read) -> table stays empty */
    }
    fclose(fp);
    return 1;   /* uVar3 — the file opened */
}

int osf_read_path(GameContext* ctx, int slot)
{
    char path[280];
    save_slot_path(ctx, path, sizeof(path), slot, "osf");
    return osf_read(ctx, path);
}

/* --- the save-slot band (2026-10-24; findings 21/408) ---------------------- */

// FUNCTION: LEGORR 0x00415cf0
void save_slot_selection_clear(GameContext* ctx)
{
    if (ctx != NULL) {
        ctx->save_slot = -1; /* the corpus's 0xffffffff sentinel */
    }
}

// FUNCTION: LEGORR 0x00417e50
void* sav_slot_ctx(GameContext* ctx, int slot)
{
    if (ctx == NULL) {
        return NULL; /* deviation: the corpus reads its global blindly */
    }
    if (slot < 0 || slot >= SAVE_SLOTS) {
        return NULL; /* the corpus's bounds test, faithful */
    }
    /* Deviation: the port keeps one active-slot context (the corpus builds
     * the per-slot contexts inside the unported SavReadSlot/SavBuildCtx
     * flow), so the valid index answers the port's context block. */
    return ctx;
}

/* --- the save-side DAT accessor band (see savegame.hpp) --------------------- */
// FUNCTION: LEGORR 0x004593d0
int osf_read_dirty_flag_get(const GameContext* ctx)
{
    return ctx != NULL ? ctx->osf_dirty : 0;
}

// FUNCTION: LEGORR 0x004601f0
void* checkpoint_blob_get(const GameContext* ctx)
{
    return ctx != NULL ? ctx->checkpoint_blob : NULL;
}

/* SaveStateEventContextSet (FUN_00418110) — zero-init module static per
 * the unowned-global convention (the corpus's word is DAT_0055781c). */
// GLOBAL: LEGORR 0x0055781c
static uint32_t s_save_state_event_context = 0u;

// FUNCTION: LEGORR 0x00418110
void save_state_event_context_set(uint32_t value)
{
    s_save_state_event_context = value;
}

uint32_t save_state_event_context_peek(void)
{
    return s_save_state_event_context;
}

/* SavBuildCtx (FUN_00417e70): slot-open probe through the port's models. */
// FUNCTION: LEGORR 0x00417e70
void* sav_build_ctx(GameContext* ctx)
{
    if (save_slot_get(ctx) == -1) {
        return NULL;
    }
    return sav_slot_ctx(ctx, save_slot_get(ctx));
}

/* SavRecordPtr (FUN_00417f90) — see savegame.hpp. */
// FUNCTION: LEGORR 0x00417f90
void* sav_record_ptr(GameContext* ctx, int index)
{
    if (sav_build_ctx(ctx) == NULL || index < 0
        || index >= ctx->record_count) {
        return NULL; /* the corpus's zero answer */
    }
    return (char*)ctx->records + (size_t)index * SAVE_RECORD_SIZE + 4;
}

/* SelectedLevelStartObjectUpdate (FUN_00460200) — see savegame.hpp. */
// FUNCTION: LEGORR 0x00460200
void selected_level_start_object_update(GameContext* ctx)
{
    if (ctx == NULL || ctx->level == NULL) {
        return; /* corpus: no level bound, nothing reads */
    }
    (void)checkpoint_blob_get(ctx); /* the corpus keeps no result */
}

/* --- the generic list window band (see savegame.hpp) ----------------------
 * Shared wide-key scan: each entry slot's key rides as little-endian
 * 16-bit units; a unit whose LOW bytes agree on zero terminates the key
 * (pinned: the terminating unit's high byte is never compared). */
static int list_wide_key_match(const void* node_key, const void* probe_key)
{
    const unsigned char* a = (const unsigned char*)node_key;
    const unsigned char* b = (const unsigned char*)probe_key;
    for (size_t off = 0;; off += 2) {
        if (a[off] != b[off]) {
            return 0;
        }
        if (a[off] == 0u) {
            return 1; /* low-byte terminator */
        }
        if (a[off + 1] != b[off + 1]) {
            return 0;
        }
    }
}

static int list_wide_scan(const SavListWindow* win, const void* wide_key)
{
    if (win == NULL || win->count < 1 || wide_key == NULL) {
        return -1; /* deviation: the corpus strides the window blindly */
    }
    for (int i = 0; i < win->count; ++i) {
        const void* entry = (win->entries != NULL) ? win->entries[i] : NULL;
        if (entry != NULL && list_wide_key_match(entry, wide_key) != 0) {
            return i;
        }
    }
    return -1;
}

// FUNCTION: LEGORR 0x00416e20
int list_find_name(const SavListWindow* win, const char* name)
{
    if (win == NULL || name == NULL) {
        return -1; /* deviation: the corpus strides the window and the key
                    * blindly; these guards are pinned by test_savegame
                    * ("the window and key guards hold") */
    }
    if (win->count <= 0) { /* the corpus's single cmp [count],0 / jle */
        return -1;
    }
    for (int i = 0; i < win->count; ++i) {
        /* corpus: the entries base and count reload each pass and the
         * entry rides into strcasecmp_lite untested */
        if (strcasecmp_lite((const char*)win->entries[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

// FUNCTION: LEGORR 0x00416ed0
void list_add_name(SavListWindow* win, const void* wide_key, void* value)
{
    const int i = list_wide_scan(win, wide_key);
    if (i >= 0 && win->values_b != NULL) {
        win->values_b[i] = value; /* corpus: blind store through + 16 */
    }
}

// FUNCTION: LEGORR 0x00416f40
void* list_has_item(const SavListWindow* win, const void* wide_key)
{
    const int i = list_wide_scan(win, wide_key);
    if (i < 0 || win->values_b == NULL) {
        return NULL;
    }
    return win->values_b[i];
}

// FUNCTION: LEGORR 0x00416fb0
void list_set_node(SavListWindow* win, const void* wide_key, void* value)
{
    const int i = list_wide_scan(win, wide_key);
    if (i >= 0 && win->values_a != NULL) {
        win->values_a[i] = value; /* corpus: blind store through + 12 */
    }
}

// FUNCTION: LEGORR 0x00417020
void* list_get_node(const SavListWindow* win, const void* wide_key)
{
    const int i = list_wide_scan(win, wide_key);
    if (i < 0 || win->values_a == NULL) {
        return NULL;
    }
    return win->values_a[i];
}

/* LevelNodeGet (FUN_00417090): recursively materialize the LevelLinks tree.
 * The native routine gets its config from globals and caches both an
 * in-progress marker and the node in the two registry value arrays; this
 * adapter takes the config explicitly and uses the same cache slots. */
static SavTreeNode* level_node_get_inner(SavListWindow* win, BlockFile* cfg,
                                         const char* level_name, int depth)
{
    if (win == NULL || cfg == NULL || level_name == NULL || depth >= 128) {
        return NULL;
    }
    const int index = list_find_name(win, level_name);
    if (index < 0 || win->values_a == NULL || win->values_b == NULL) {
        return NULL;
    }
    const void* registry_key = win->entries[index];
    if (list_has_item(win, registry_key) != NULL) {
        return (SavTreeNode*)list_get_node(win, registry_key);
    }

    SavTreeNode* node = (SavTreeNode*)engine_heap_alloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    memset(node, 0, sizeof(*node));
    node->window = (uint32_t)index;
    list_add_name(win, registry_key, (void*)1); /* break LevelLinks cycles */
    list_set_node(win, registry_key, node);

    const char* group = strstr(level_name, "::");
    group = group != NULL ? group + 2 : level_name;
    char path[256];
    const char* root = util_root_name(cfg);
    snprintf(path, sizeof(path), "%s Levels %s LevelLinks", root, group);
    const char* links = block_get_strptr(cfg, path);
    if (links == NULL) {
        snprintf(path, sizeof(path), "%s %s LevelLinks", root, group);
        links = block_get_strptr(cfg, path);
    }
    if (links == NULL || links[0] == '\0') {
        return node;
    }

    char names[15][FE_LEVEL_NAME_MAX];
    uint32_t count = 0;
    const char* p = links;
    while (*p != '\0' && count < 15) {
        while (*p == ',' || *p == ' ' || *p == '\t') {
            ++p;
        }
        if (*p == '\0') {
            break;
        }
        const char* end = p;
        while (*end != '\0' && *end != ',') {
            ++end;
        }
        size_t len = (size_t)(end - p);
        while (len > 0 && (p[len - 1] == ' ' || p[len - 1] == '\t')) {
            --len;
        }
        if (len >= FE_LEVEL_NAME_MAX) {
            len = FE_LEVEL_NAME_MAX - 1;
        }
        if (len != 0) {
            memcpy(names[count], p, len);
            names[count][len] = '\0';
            ++count;
        }
        p = *end == ',' ? end + 1 : end;
    }
    if (count == 0) {
        return node;
    }

    SavTreeNode** children = (SavTreeNode**)engine_heap_alloc(
        (long)(count * sizeof(*children)));
    if (children == NULL) {
        list_set_node(win, registry_key, NULL);
        list_add_name(win, registry_key, NULL);
        engine_heap_free(node);
        return NULL;
    }
    memset(children, 0, count * sizeof(*children));
    node->children = children;
    node->count = count;
    for (uint32_t i = 0; i < count; ++i) {
        children[i] = level_node_get_inner(win, cfg, names[i], depth + 1);
    }
    return node;
}

// FUNCTION: LEGORR 0x00417090
SavTreeNode* level_node_get(SavListWindow* win, BlockFile* cfg,
                            const char* level_name)
{
    return level_node_get_inner(win, cfg, level_name, 0);
}

/* SavTreeWalk (FUN_004171c0) — see savegame.hpp. */
// FUNCTION: LEGORR 0x004171c0
int sav_tree_walk(void* node, sav_tree_visit visit, void* user)
{
    SavTreeNode* n = (SavTreeNode*)node;
    if (n == NULL || n->visited != 0u) {
        return 0;
    }
    n->visited = 1u; /* the corpus's flag store, blind in the original */
    if (visit != NULL && visit(node, user) != 0) {
        return 1; /* deviation: the corpus strides the callback blindly */
    }
    if (n->children == NULL) {
        return 0;
    }
    /* the corpus re-reads the children slot every pass (widened slots) */
    for (uint32_t i = 0; i < n->count; ++i) {
        if (sav_tree_walk(((void**)n->children)[i], visit, user) != 0) {
            return 1;
        }
    }
    return 0;
}

/* SavTreeWalk2 (FUN_004172f0) — see savegame.hpp. */
// FUNCTION: LEGORR 0x004172f0
void sav_tree_walk2(void* node, SavTree2State* state, int carry,
                    const EngineContext* eng)
{
    SavTreeNode* n = (SavTreeNode*)node;
    if (n == NULL || n->visited != 0u) {
        return;
    }
    const int extended = 1 < engine_app_mode_get(eng)
                      || (engine_feature_word_peek() & 0x20000u) != 0u;
    if (extended) {
        state->flag = 0u;
    }
    n->visited = 1u;
    uint32_t need;
    if (state->window_base == NULL || extended) {
        need = state->flag == 0u ? 1u : 0u;
    } else {
        const char* w = (const char*)state->window_base
                      + (ptrdiff_t)((int32_t)n->window) * 0x3190;
        uint32_t word = 0u;
        __builtin_memcpy(&word, w, sizeof word);
        need = word & 1u;
    }
    if (state->blocks != NULL && *state->blocks != NULL) {
        char* blk = (char*)*state->blocks + (size_t)state->index * 0x20;
        uint32_t flags = 0u;
        uint32_t probe = 0u;
        __builtin_memcpy(&flags, blk, sizeof flags);
        __builtin_memcpy(&probe, blk + 0x1c, sizeof probe);
        flags = (need == 0u && carry == 0 && probe == 0u)
                    ? (flags & ~4u)
                    : (flags | 4u);
        __builtin_memcpy(blk, &flags, sizeof flags);
    }
    state->index = state->index + 1u; /* the corpus advances blindly */
    if (n->children != NULL) {
        void** kids = (void**)n->children; /* widened slots */
        for (uint32_t i = 0; i < n->count; ++i) {
            sav_tree_walk2(kids[i], state, need != 0u ? 1 : 0, eng);
        }
    }
}

/* SavTreeLoad (FUN_004173e0) — see savegame.hpp. */
// FUNCTION: LEGORR 0x004173e0
void sav_tree_load(void* node, void* window_base, void** blocks,
                   uint32_t window, uint32_t flag,
                   const EngineContext* eng)
{
    SavTree2State st; /* the corpus's five stack dwords */
    st.window_base = window_base;
    st.window = window;
    st.blocks = blocks;
    st.index = 0u;
    st.flag = flag;
    sav_tree_walk2(node, &st, 1, eng);
    sav_clear_visited();
}

/* RegistryValueLookupWithLock (FUN_00417280) — see savegame.hpp. The
 * user block is the corpus's four stack dwords: found node, the probe
 * index, the visit counter, and the hit-index write. */
typedef struct SavTreeNthState {
    void*    found;
    uint32_t want;
    uint32_t seen;
    uint32_t found_at;
} SavTreeNthState;

static int sav_tree_nth_visit(void* node, void* user)
{
    SavTreeNthState* st = (SavTreeNthState*)user;
    if (st->want == st->seen) {
        st->found_at = st->seen;
        st->found = node;
        return 1;
    }
    st->seen = st->seen + 1u;
    return 0;
}

// FUNCTION: LEGORR 0x00417280
void* registry_value_lookup(void* root, uint32_t index)
{
    SavTreeNthState st = { NULL, index, 0u, 0u };
    const int hit = sav_tree_walk(root, sav_tree_nth_visit, &st);
    sav_clear_visited();
    return hit != 0 ? st.found : NULL;
}

// FUNCTION: LEGORR 0x004172d0
uint32_t registry_value_index_lookup_or_not_found(void* root, uint32_t index)
{
    void* node = registry_value_lookup(root, index);
    if (node != NULL) {
        uint32_t word = 0u;
        __builtin_memcpy(&word, node, sizeof word); /* corpus reads blind */
        return word;
    }
    return 0xffffffffu;
}

/* SaveMenuRefresh (FUN_004150b0) — see savegame.hpp. The corpus reads
 * its slot-context chain, the context's +0x34 stride-window word, and
 * the two tree roots all blind; the port models each with an
 * own-storage hook (zero-defaulted, so an unprimed refresh just
 * guards through both walks). */
static void**  s_sav_menu_windows[2]; /* the chain + 0x10 word's slot role */
static void*   s_sav_slot_stride_window; /* the context + 0x34 word's role */
static void*   s_sav_tree_root[2]; /* the two corpus tree-root globals' role */

void sav_menu_window_install(int which, void** slot)
{
    if (which == 0 || which == 1) {
        s_sav_menu_windows[which] = slot;
    }
}

void** sav_menu_window_peek(int which)
{
    return (which == 0 || which == 1) ? s_sav_menu_windows[which] : NULL;
}

void sav_slot_stride_window_install(void* base)
{
    s_sav_slot_stride_window = base;
}

void* sav_slot_stride_window_peek(void)
{
    return s_sav_slot_stride_window;
}

void sav_tree_root_install(int which, void* node)
{
    if (which == 0 || which == 1) {
        s_sav_tree_root[which] = node;
    }
}

void* sav_tree_root_peek(int which)
{
    return (which == 0 || which == 1) ? s_sav_tree_root[which] : NULL;
}

// FUNCTION: LEGORR 0x004150b0
void save_menu_refresh(GameContext* ctx, int slot)
{
    save_slot_set(ctx, slot); /* FUN_00417ea0 */
    void* rec = sav_build_ctx(ctx); /* FUN_00417e70 */
    void* base = NULL;
    uint32_t flag = 1u;
    if (rec != NULL) {
        flag = 0u;
        base = sav_slot_stride_window_peek(); /* corpus: rec + 0x34 */
    }
    sav_tree_load(sav_tree_root_peek(0), base, sav_menu_window_peek(0),
                  0u, flag, NULL);
    sav_tree_load(sav_tree_root_peek(1), NULL, sav_menu_window_peek(1),
                  0u, 0u, NULL);
}

/* ShowLevelOverlayForCurrentSelection (FUN_00418000): commit the currently
 * selected save slot, then rebuild the campaign/tutorial save-tree views.
 * The frame loop renders the selected level summary from its FrontEnd row;
 * this function handles the save and tree-refresh side effects that preceded
 * the original descriptor display. */
// FUNCTION: LEGORR 0x00418000
void show_level_overlay_for_current_selection(GameContext* ctx)
{
    const int slot = save_slot_get(ctx); /* FUN_00417e90 */
    if (ctx == NULL || slot < 0 || slot >= SAVE_SLOTS) {
        return;
    }
    (void)save_write_slot(ctx, slot); /* FUN_00417bd0 */
    save_menu_refresh(ctx, slot);     /* FUN_004173e0 pair */
}

/* --- the checkpoint progress fold band (see savegame.hpp) ------------------ */

static float blob_float_get(const char* blob, size_t off)
{
    float v = 0.0f;
    __builtin_memcpy(&v, blob + off, sizeof v);
    return v;
}

static void blob_float_put(char* blob, size_t off, float v)
{
    __builtin_memcpy(blob + off, &v, sizeof v);
}

/* Corpus fold shape: percent = 100 / den * num rides through only when
 * the denominator reads non-zero, then clamps after the (invisible)
 * first store. A zero denominator answers 100.0 whatever num holds. */
static float blob_percent_fold(const char* blob, size_t den, size_t num)
{
    float v = 100.0f;
    const float d = blob_float_get(blob, den);
    if (d != 0.0f) {
        v = (100.0f / d) * blob_float_get(blob, num);
    }
    if (100.0f < v) {
        v = 100.0f;
    }
    return v;
}

/* Corpus count-up: every round adds 100.0 to the +0x2fac sum (the
 * corpus's subtracted negative — an addition) and the window's last
 * folded percent to the +0x2fb0 carry; the guard float is re-read every
 * round and the counter rides as a converted uint. */
static void blob_count_up(char* blob, size_t guard_off, size_t add_off)
{
    for (uint32_t n = 0u; (float)n < blob_float_get(blob, guard_off); ++n) {
        blob_float_put(blob, 0x2fac, blob_float_get(blob, 0x2fac) + 100.0f);
        blob_float_put(blob, 0x2fb0,
                       blob_float_get(blob, add_off)
                           + blob_float_get(blob, 0x2fb0));
    }
}

// FUNCTION: LEGORR 0x00460950
int checkpoint_blob_task_progress_recalculate(GameContext* ctx)
{
    if (ctx == NULL || ctx->level == NULL) {
        return 0; /* corpus: the CurrentLevelGet guard; deviation: threaded */
    }
    char* blob = (char*)checkpoint_blob_get(ctx);
    if (blob == NULL) {
        return 0; /* corpus: the blob getter guard */
    }
    blob_float_put(blob, 0x2fb0, 0.0f);
    blob_float_put(blob, 0x2fac, 0.0f);
    blob_float_put(blob, 0x2fb4, 0.0f);
    blob_float_put(blob, 0x1d24, blob_percent_fold(blob, 0x1d1c, 0x1d20));
    blob_float_put(blob, 0x1f34, blob_percent_fold(blob, 0x1f2c, 0x1f30));
    blob_float_put(blob, 0x2144, blob_percent_fold(blob, 0x213c, 0x2140));
    blob_float_put(blob, 0x1b0c, blob_percent_fold(blob, 0x1b04, 0x1b08));
    blob_float_put(blob, 0x2b94, blob_percent_fold(blob, 0x2b8c, 0x2b90));
    blob_float_put(blob, 0x2354, blob_percent_fold(blob, 0x234c, 0x2350));
    blob_float_put(blob, 0x2774, blob_percent_fold(blob, 0x276c, 0x2770));
    blob_float_put(blob, 0x2984, blob_percent_fold(blob, 0x297c, 0x2980));
    blob_count_up(blob, 0x2760, 0x2774);
    blob_count_up(blob, 0x1d10, 0x1d24);
    blob_count_up(blob, 0x2b80, 0x2b94);
    blob_count_up(blob, 0x2340, 0x2354);
    blob_count_up(blob, 0x2550, 0x2564); /* the raw-window twin */
    if (0.0f < blob_float_get(blob, 0x2d90)) {
        /* Corpus shape: this loop keeps both accumulators in registers
         * and writes them back only when it ends. */
        float sum = blob_float_get(blob, 0x2fac);
        float carry = blob_float_get(blob, 0x2fb0);
        const float each = 100.0f - blob_float_get(blob, 0x2da4);
        for (uint32_t n = 0u;
             (float)n < blob_float_get(blob, 0x2d90); ++n) {
            sum = sum + 100.0f;
            carry = each + carry;
        }
        blob_float_put(blob, 0x2fb0, carry);
        blob_float_put(blob, 0x2fac, sum);
    }
    blob_count_up(blob, 0x2970, 0x2984);
    if (blob_float_get(blob, 0x2fac) < blob_float_get(blob, 0x2fb0)) {
        blob_float_put(blob, 0x2fb0, blob_float_get(blob, 0x2fac));
    }
    if (blob_float_get(blob, 0x2fb0) < 0.0f) {
        blob_float_put(blob, 0x2fb0, 0.0f);
    }
    float v = blob_percent_fold(blob, 0x2fac, 0x2fb0);
    blob_float_put(blob, 0x2fb4, v);
    if (100.0f < v) {
        v = 100.0f;
    }
    blob_float_put(blob, 0x2fb4, v);
    uint32_t bonus = 0u; /* the corpus's converted uint word */
    __builtin_memcpy(&bonus, blob + 0x1c0, sizeof bonus);
    v = (float)bonus + blob_float_get(blob, 0x2fb4);
    blob_float_put(blob, 0x2fb4, v);
    if (100.0f < v) {
        v = 100.0f;
    }
    blob_float_put(blob, 0x2fb4, v);
    return 1;
}

/* SavRefreshSlots (2026-10-25) — FUN_00417df0. Walk the six slot
 * contexts so every slot file exists. With the engine feature word's
 * bit 0x400 raised the corpus writes the slot fresh before reading it
 * back; otherwise it reads (and re-reads once when the first attempt
 * fails). Deviations: the port's single active-slot context stands in
 * for the corpus's six DAT_00557080 blocks (the SavSlotCtx precedent),
 * the null context is guarded, and the corpus reads its context-pointer
 * block blind. */
// FUNCTION: LEGORR 0x00417df0
void sav_refresh_slots(GameContext* ctx)
{
    if (ctx == NULL) {
        return; /* deviation: the corpus strides blindly */
    }
    for (int slot = 0; slot < SAVE_SLOTS; ++slot) {
        const int write_first =
            (engine_feature_word_peek() & 0x400u) != 0u;
        if (write_first) {
            (void)save_write_slot(ctx, slot);
            (void)save_read_slot(ctx, slot, 0);
        } else if (save_read_slot(ctx, slot, 0) == 0) {
            (void)save_read_slot(ctx, slot, 0);
        }
    }
}
