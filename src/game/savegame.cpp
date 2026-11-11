// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.hpp"
#include "entity.hpp"
#include "gods98/block.hpp"
#include "crt/crt.hpp"
#include "gods98/engine.hpp"
#include "gods98/heap.hpp"
#include "gods98/resource.hpp"
#include "gods98/record.hpp"
#include "frontend.hpp"
#include "level.hpp"
#include "mission.hpp"
#include "savegame.hpp"
#include "options.hpp"
#include "util.hpp"
#include "savegame_shared.hpp"
#include "savegame_for_reward.hpp"
#include "savegame_from_reward.hpp"

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

/* The `.sav` open modes (corpus constants): DAT_004a2770 ("rb") and
 * DAT_004a2794 ("wb"). */
const char* MODE_RB = "rb";
const char* MODE_WB = "wb";
static void* s_sav_tree_root[2]; /* the two corpus tree-root globals' role */
static void** s_sav_owned_tree_nodes[2];
static uint32_t s_sav_owned_tree_node_count[2];
GameContext* s_save_current_context;
static FILE* s_return_diag_log;

void save_return_diagnostic_log_set(void* log)
{
    s_return_diag_log = (FILE*)log;
}
// GLOBAL: LEGORR 0x00556fd0
int s_active_object_ingame;
// GLOBAL: LEGORR 0x00556fe4
int s_active_object_outgame;
// GLOBAL: LEGORR 0x00556ff0
static void** s_sav_visited_outgame_items;
// GLOBAL: LEGORR 0x00556ff4
static uint32_t* s_sav_visited_outgame_words;
// GLOBAL: LEGORR 0x00556fdc
static void** s_sav_visited_ingame_items;
// GLOBAL: LEGORR 0x00556fe0
static uint32_t* s_sav_visited_ingame_words;
// GLOBAL: LEGORR 0x005574d0
static int s_save_current_slot = -1;
/* The six 0xb8-byte per-slot contexts from DAT_00557080..DAT_005574d0. */
// GLOBAL: LEGORR 0x00557080
static uint8_t s_sav_slot_context_blocks[SAVE_SLOTS * 0xb8];
/* Wider-host pointers cannot fit the retail context's 32-bit +0x34 slot.
 * Keep each scanned slot's record allocation beside its 0xb8-byte header. */
static uint8_t* s_sav_slot_records[SAVE_SLOTS];
static int32_t s_sav_slot_record_counts[SAVE_SLOTS];
static void save_slot_sidecar_release(int slot);
typedef struct SavVisitedBand {
    void** items;
    uint32_t* words;
    uint32_t count;
    int root_bound;
} SavVisitedBand;
static SavVisitedBand s_sav_visited_bands[2];

static void sav_visited_band_alias_sync(int which)
{
    if (which == 0) {
        s_sav_visited_outgame_items = s_sav_visited_bands[0].items;
        s_sav_visited_outgame_words = s_sav_visited_bands[0].words;
    }
    else if (which == 1) {
        s_sav_visited_ingame_items = s_sav_visited_bands[1].items;
        s_sav_visited_ingame_words = s_sav_visited_bands[1].words;
    }
}

static void sav_visited_band_release(int which);
static int sav_visited_band_install(int which, void* const* items,
                                    uint32_t count);

int save_write_record_target(int index, const uint8_t* blob);
void* sav_build_ctx_target(void);
int save_slot_get_target(void);
void save_slot_set_target(int slot);
void save_slot_selection_clear_target(void);
void* sav_slot_ctx_target(int slot);
void* sav_record_ptr_target(int index);

void save_context_bind(GameContext* ctx)
{
    if (ctx != NULL) {
        s_save_current_context = ctx;
        s_save_current_slot = ctx->save_slot;
    }
}

GameContext* save_current_context_get(void)
{
    return s_save_current_context;
}

static void sav_tree_owned_visit_flags_clear(int which)
{
    uint32_t i;
    if (which < 0 || which > 1) return;
    for (i = 0u; i < s_sav_owned_tree_node_count[which]; ++i) {
        SavTreeNode* node = (SavTreeNode*)s_sav_owned_tree_nodes[which][i];
        if (node != NULL) node->visited = 0u;
    }
}

static void sav_tree_owned_nodes_release(int which)
{
    uint32_t i;
    if (which < 0 || which > 1) return;
    for (i = 0u; i < s_sav_owned_tree_node_count[which]; ++i) {
        SavTreeNode* node = (SavTreeNode*)s_sav_owned_tree_nodes[which][i];
        if (node != NULL) {
            engine_heap_free(node->children);
            engine_heap_free(node);
        }
    }
    free(s_sav_owned_tree_nodes[which]);
    s_sav_owned_tree_nodes[which] = NULL;
    s_sav_owned_tree_node_count[which] = 0u;
    s_sav_tree_root[which] = NULL;
    sav_visited_band_release(which);
    active_object_list_count_set(which, 0);
}

static int save_tree_count_visit(void* node, void* user)
{
    uint32_t* count = (uint32_t*)user;
    (void)node;
    ++*count;
    return 0;
}

static int save_tree_level_count(int which, uint32_t* count_out)
{
    uint32_t count = 0u;
    if (which < 0 || which > 1 || count_out == NULL ||
        s_sav_tree_root[which] == NULL) return 0;
    sav_tree_owned_visit_flags_clear(which);
    (void)sav_tree_walk(s_sav_tree_root[which], save_tree_count_visit,
                        &count);
    sav_clear_visited();
    sav_tree_owned_visit_flags_clear(which);
    *count_out = count;
    return 1;
}

static int save_current_level_counts(uint32_t* campaign_count,
                                     uint32_t* tutorial_count)
{
    int campaign_found;
    if (campaign_count == NULL || tutorial_count == NULL) return 0;
    campaign_found = save_tree_level_count(0, campaign_count);
    if (!campaign_found) *campaign_count = 0u;
    if (!save_tree_level_count(1, tutorial_count))
        *tutorial_count = 0u;
    return campaign_found;
}

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
    for (int slot = 0; slot < SAVE_SLOTS; ++slot)
        save_slot_sidecar_release(slot);
    memset(s_sav_slot_context_blocks, 0, sizeof(s_sav_slot_context_blocks));
    s_save_current_context = ctx;
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
    ctx->osf = NULL;
    ctx->osf_shadow = NULL;
    ctx->osf_shadow_count = 0;
    ctx->osf_shadow_dirty = 0;
    s_save_current_slot = ctx->save_slot;
}

/* SavReadSlot passes "Saves" through GameFileOpenDispatch; bind the same
 * configured data root for the typed FILE-backed save/OSF/thumbnail paths. */
int save_bind_resource_directory(GameContext* ctx)
{
    const char* path;
    size_t length;
    if (ctx == NULL) return 0;
    path = resource_path_resolve_against_base("Saves");
    if (path == NULL || path[0] == '\0') return 0;
    length = strlen(path);
    if (length >= sizeof(ctx->saves_dir)) return 0;
    memcpy(ctx->saves_dir, path, length + 1u);
    return 1;
}

int save_slot_get(const GameContext* ctx)
{
    if (ctx == NULL) return -1;
    save_context_bind((GameContext*)ctx);
    return ctx->save_slot;
}

void save_slot_set(GameContext* ctx, int slot)
{
    if (ctx != NULL) {
        save_context_bind(ctx);
        save_slot_set_target(slot);
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

static void save_slot_sidecar_release(int slot)
{
    if (slot < 0 || slot >= SAVE_SLOTS) return;
    free(s_sav_slot_records[slot]);
    s_sav_slot_records[slot] = NULL;
    s_sav_slot_record_counts[slot] = 0;
}

/* SavWriteSlot(slot, 0) creates an empty file sized for the current campaign
 * tree. This path does not change the process-global active slot or its
 * GameContext record allocation. */
static int save_slot_write_fresh_file(GameContext* ctx, int slot,
                                      uint32_t record_count)
{
    char path[280];
    uint8_t header[SAVE_HEADER_SIZE];
    uint8_t* records = NULL;
    size_t wrote;
    GameOptions defaults;
    int ok = 0;
    if (ctx == NULL || slot < 0 || slot >= SAVE_SLOTS ||
        record_count > 0x1000u) return 0;
    save_directory_ensure_exists(ctx->saves_dir[0] != '\0'
                                     ? ctx->saves_dir : "Saves");
    save_slot_path(ctx, path, sizeof(path), slot, "sav");
    memset(header, 0, sizeof(header));
    options_defaults(&defaults);
    /* SavWriteSlot's fresh-context defaults, distinct from menu fallback. */
    defaults.sfx_volume = OPT_RETAIL_FRESH_SFX;
    defaults.brightness = OPT_RETAIL_FRESH_BRIGHTNESS;
    __builtin_memcpy(header + 0x04, &defaults.music_volume, 4);
    __builtin_memcpy(header + 0x08, &defaults.sfx_volume, 4);
    __builtin_memcpy(header + 0x0c, &defaults.game_speed, 4);
    __builtin_memcpy(header + 0x10, &defaults.brightness, 4);
    __builtin_memcpy(header + 0x18, &record_count, 4);
    if (record_count != 0u) {
        if ((size_t)record_count > ((size_t)-1) / SAVE_RECORD_SIZE)
            return 0;
        records = (uint8_t*)calloc((size_t)record_count, SAVE_RECORD_SIZE);
        if (records == NULL) return 0;
    }
    FILE* fp = fopen(path, MODE_WB);
    if (fp == NULL) {
        free(records);
        return 0;
    }
    wrote = fwrite(header, 1, sizeof(header), fp);
    ok = wrote == sizeof(header);
    if (ok && record_count != 0u) {
        wrote = fwrite(records, 1,
                       (size_t)record_count * SAVE_RECORD_SIZE, fp);
        ok = wrote == (size_t)record_count * SAVE_RECORD_SIZE;
    }
    fclose(fp);
    free(records);
    return ok;
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
    ctx->saved_music_volume = ctx->options.music_volume;
    ctx->saved_sfx_volume = ctx->options.sfx_volume;
    ctx->saved_scroll_index = ctx->options.game_speed;
    ctx->saved_brightness = ctx->options.brightness;
    return 1;
}

/* Serialise the active slot to `<dir>\<slot>.sav` (SavWriteSlot). */
// FUNCTION: LEGORR 0x00417bd0
static int sav_write_slot_file(GameContext* ctx)
{
    /* The typed save entry points also serve callers outside the menu's
     * close callback. Snapshot their effective settings before serializing. */
    ctx->saved_music_volume = ctx->options.music_volume;
    options_write_to_slot(ctx);
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
    /* The retail 0xb0-byte header is the slot context's raw prefix. The
     * tracked option words occupy +0x04/+0x08/+0x0c/+0x10; +0x20 is ordinary
     * context data, not a port-specific options extension. */
    __builtin_memcpy(header + 0x04, &ctx->saved_music_volume, 4);
    __builtin_memcpy(header + 0x08, &ctx->saved_sfx_volume, 4);
    __builtin_memcpy(header + 0x0c, &ctx->saved_scroll_index, 4);
    __builtin_memcpy(header + 0x10, &ctx->saved_brightness, 4);
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

/* SavReadSlot's retail ABI is (slot, slot-context, mode). The portable
 * slot block is not layout-compatible with the retail 0xb8-byte context;
 * use the GameContext bound by save_init or the typed adapter. */
// FUNCTION: LEGORR 0x00417a90
int save_read_slot_target(uint32_t slot, void* slot_context, int mode)
{
    GameContext* ctx = s_save_current_context;
    const int per_slot_context = slot_context != NULL &&
                                 slot_context != (void*)ctx;
    uint32_t tree_counts[2] = { 0u, 0u };
    int have_tree_counts = 0;
    if (ctx == NULL || slot >= SAVE_SLOTS) {
        return 0;
    }
    save_directory_ensure_exists(ctx->saves_dir[0] != '\0'
                                     ? ctx->saves_dir : "Saves");
    char path[280];
    save_slot_path(ctx, path, sizeof(path), slot, "sav");
    FILE* fp = fopen(path, MODE_RB);
    if (fp == NULL) {
        if (mode == 0) {
            uint32_t count = SAVE_DEFAULT_RECORDS;
            (void)save_tree_level_count(0, &count);
            if (per_slot_context) {
                (void)save_slot_write_fresh_file(ctx, (int)slot, count);
            } else {
                /* Preserve the typed port's historical empty-slot bootstrap
                 * used by checkpoint and direct-launch callers. */
                save_slot_set_target((int)slot);
                ctx->save_slot = (int)slot;
                if (save_bootstrap_records(ctx, SAVE_DEFAULT_RECORDS)) {
                    ctx->active_slot = (int)slot;
                    ctx->saved_music_volume = OPT_DFLT_MUSIC_VOLUME;
                    ctx->saved_sfx_volume = OPT_RETAIL_FRESH_SFX;
                    ctx->saved_scroll_index = OPT_DFLT_GAME_SPEED;
                    ctx->saved_brightness = OPT_RETAIL_FRESH_BRIGHTNESS;
                    (void)save_slot_write_fresh_file(ctx, (int)slot,
                                                     SAVE_DEFAULT_RECORDS);
                }
            }
        }
        return 0;
    }

    uint8_t header[SAVE_HEADER_SIZE];
    size_t got = fread(header, 1, SAVE_HEADER_SIZE, fp);
    if (got != SAVE_HEADER_SIZE) {
        fclose(fp);
        return 0;
    }
    /* GOPS identifies the earlier portable format unambiguously. Its
     * +0x10 word was music, not retail brightness; normalize the numeric
     * words before either the typed or per-slot reader consumes them. */
    {
        GameOptions legacy_options;
        options_defaults(&legacy_options);
        if (options_load_header(&legacy_options, header + 0x20)) {
            __builtin_memcpy(header + 0x04, &legacy_options.music_volume, 4);
            __builtin_memcpy(header + 0x08, &legacy_options.sfx_volume, 4);
            __builtin_memcpy(header + 0x0c, &legacy_options.game_speed, 4);
            __builtin_memcpy(header + 0x10, &legacy_options.brightness, 4);
        }
    }
    int32_t count;
    __builtin_memcpy(&count, header + 0x18, 4);
    have_tree_counts = save_current_level_counts(&tree_counts[0],
                                                 &tree_counts[1]);
    if (per_slot_context) {
        memcpy(slot_context, header, SAVE_HEADER_SIZE);
    }
    if (mode == 0 && have_tree_counts &&
        (uint32_t)count != tree_counts[0]) {
        /* Native SaveWriteSlot initializes this slot, then SavReadSlot clears
         * exactly the supplied 0xb8-byte slot context and returns a miss. */
        (void)save_slot_write_fresh_file(ctx, (int)slot, tree_counts[0]);
        if (per_slot_context) {
            save_slot_sidecar_release((int)slot);
            memset(slot_context, 0, 0xb8);
        } else {
            save_release_records(ctx);
        }
        fclose(fp);
        return 0;
    }

    if (count <= 0 || count > 0x1000) {
        fclose(fp);
        if (mode == 0 && per_slot_context) {
            save_slot_sidecar_release((int)slot);
            memset(slot_context, 0, 0xb8);
        } else if (mode == 0) {
            save_release_records(ctx);
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
    if (per_slot_context) {
        uint32_t retail_record_pointer = (uint32_t)(uintptr_t)rec;
        save_slot_sidecar_release((int)slot);
        s_sav_slot_records[slot] = rec;
        s_sav_slot_record_counts[slot] = count;
        __builtin_memcpy((uint8_t*)slot_context + 0x34,
                         &retail_record_pointer, 4);
        return 1;
    }

    /* Retail stores the three option words directly in the slot-context
     * header. Accept the former GOPS extension only for saves written by
     * earlier portable builds. */
    __builtin_memcpy(&ctx->saved_music_volume, header + 0x04, 4);
    __builtin_memcpy(&ctx->saved_sfx_volume, header + 0x08, 4);
    __builtin_memcpy(&ctx->saved_scroll_index, header + 0x0c, 4);
    __builtin_memcpy(&ctx->saved_brightness, header + 0x10, 4);
    GameOptions legacy_options;
    options_defaults(&legacy_options);
    if (options_load_header(&legacy_options, header + 0x20)) {
        ctx->options = legacy_options;
    } else {
        /* The retail header contains numeric settings, not the live
         * Advanced cycles. A checkpoint's default-slot reload must retain
         * those cycles, just as an ordinary slot read does. */
        ctx->options.music_volume = ctx->saved_music_volume;
        ctx->options.sfx_volume = ctx->saved_sfx_volume;
        ctx->options.game_speed = ctx->saved_scroll_index;
        ctx->options.brightness = ctx->saved_brightness;
    }
    save_release_records(ctx);
    ctx->active_slot = slot;
    ctx->record_count = count;
    ctx->records = rec;
    save_slot_set_target(slot);
    ctx->save_slot = slot;
    return 1;
}

int save_read_slot(GameContext* ctx, int slot, int create_if_missing)
{
    if (ctx == NULL || slot < 0 || slot >= SAVE_SLOTS) return 0;
    save_context_bind(ctx);
    return save_read_slot_target((uint32_t)slot, NULL, create_if_missing);
}

int save_write_slot(GameContext* ctx, int slot)
{
    if (ctx == NULL || slot < 0 || slot >= SAVE_SLOTS) {
        return 0;
    }
    save_context_bind(ctx);
    int prev = ctx->save_slot;
    save_slot_set_target(slot);
    ctx->save_slot = slot;
    if (ctx->records == NULL) {
        save_bootstrap_records(ctx, SAVE_DEFAULT_RECORDS);
    }
    int ok = sav_write_slot_file(ctx);
    save_slot_set_target(prev);
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
    (void)sav_write_slot_file(ctx);

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

/* Typed owner adapter: the target entry below keeps the corpus's two-argument
 * global-context ABI, while normal port callers still grow the live table. */
int save_write_record(GameContext* ctx, int index, const uint8_t* blob)
{
    if (ctx == NULL || index < 0 || blob == NULL
        || !save_records_grow(ctx, index)) {
        return 0;
    }
    save_context_bind(ctx);
    return save_write_record_target(index, blob);
}

/* ------------------------------------------------------------------ *
 *  Checkpoint blob (CheckpointBlobBuild @ 0x45fda0).
 * ------------------------------------------------------------------ */

void checkpoint_blob_apply(GameContext* ctx, const uint8_t* blob)
{
    if (ctx == NULL || blob == NULL) {
        return;
    }
    if (ctx->checkpoint_blob == NULL)
        checkpoint_blob_build(ctx);
    if (ctx->checkpoint_blob == NULL)
        return;
    memmove(ctx->checkpoint_blob, blob, SAVE_BLOB_SIZE);
    {
        uint32_t self = (uint32_t)(uintptr_t)(ctx->checkpoint_blob + 0x1CECu);
        __builtin_memcpy(ctx->checkpoint_blob + 0x1CE8, &self, 4);
    }
    int32_t enable = 0, mod = 0;
    __builtin_memcpy(&enable, ctx->checkpoint_blob + 0x00, 4);
    __builtin_memcpy(&mod, ctx->checkpoint_blob + 0x1C0, 4);
    ctx->reward.enable = enable;
    ctx->reward.modifier = mod;
    __builtin_memcpy(ctx->tasks, ctx->checkpoint_blob + 0x1CEC,
                     (size_t)GAME_TASK_COUNT * (size_t)GAME_TASK_STRIDE);
}

int save_slot_records_view(int slot, const uint8_t** records, int* count)
{
    if (slot < 0 || slot >= SAVE_SLOTS || records == NULL || count == NULL ||
        s_sav_slot_records[slot] == NULL) return 0;
    *records = s_sav_slot_records[slot];
    *count = s_sav_slot_record_counts[slot];
    return 1;
}

/* Read a menu preview without the retail mode-0 missing/mismatch rewrite. */
int save_slot_preview(GameContext* ctx, int slot, const uint8_t** records,
                      int* count)
{
    uint32_t campaign_count = 0u, tutorial_count = 0u;
    uint8_t* slot_context;
    if (ctx == NULL || slot < 0 || slot >= SAVE_SLOTS ||
        records == NULL || count == NULL) return 0;
    save_context_bind(ctx);
    slot_context = s_sav_slot_context_blocks + (size_t)slot * 0xb8u;
    if (!save_read_slot_target((uint32_t)slot, slot_context, 1)) return 0;
    if (!save_slot_records_view(slot, records, count)) return 0;
    if (save_current_level_counts(&campaign_count, &tutorial_count) &&
        (uint32_t)*count != campaign_count) return 0;
    return 1;
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
    save_context_bind(ctx);
    uint8_t* slot_context = s_sav_slot_context_blocks +
                            (size_t)slot * 0xb8u;
    if (!save_read_slot_target((uint32_t)slot, slot_context, 0)) {
        return 0;
    }
    uint8_t* slot_records = s_sav_slot_records[slot];
    int recs = s_sav_slot_record_counts[slot];
    int filled = 0, completed = 0, resume = -1;
    for (int i = 0; slot_records != NULL && i < recs && i < 256; i++) {
        uint32_t flags = 0u;
        const uint8_t* record = slot_records + (size_t)i * SAVE_RECORD_SIZE;
        __builtin_memcpy(&flags, record, sizeof(flags));
        const uint8_t* blob = record + sizeof(flags);
        int present = (flags & 0x3u) != 0u;
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
            if ((flags & 1u) != 0u) {
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
    if (ctx == NULL) return;
    save_context_bind(ctx);
    checkpoint_step_target();
}

/* ------------------------------------------------------------------ *
 *  Completed-objective table (`.osf`).
 * ------------------------------------------------------------------ */

int osf_write_path(GameContext* ctx, int slot)
{
    char path[280];
    save_slot_path(ctx, path, sizeof(path), slot, "osf");
    return osf_write(ctx, path);
}

/* GameFileLengthGet (FUN_0047fca0): query the stream length and restore its
 * original position, matching the original's save/seek-end/read-position /
 * seek-back sequence. */
// FUNCTION: LEGORR 0x0047fca0
long save_stream_length(FILE* fp)
{
    long position = ftell(fp);
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, position, SEEK_SET);
    return length;
}

int osf_read_path(GameContext* ctx, int slot)
{
    char path[280];
    save_slot_path(ctx, path, sizeof(path), slot, "osf");
    return osf_read(ctx, path);
}

/* --- the save-slot band (2026-10-24; findings 21/408) ---------------------- */

void save_slot_selection_clear(GameContext* ctx)
{
    if (ctx != NULL) {
        save_context_bind(ctx);
        save_slot_selection_clear_target();
        ctx->save_slot = -1;
    }
}

void* sav_slot_ctx(GameContext* ctx, int slot)
{
    if (ctx == NULL) {
        return NULL; /* deviation: the corpus reads its global blindly */
    }
    save_context_bind(ctx);
    return sav_slot_ctx_target(slot);
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

/* Typed SavBuildCtx adapter. */
void* sav_build_ctx(GameContext* ctx)
{
    if (ctx == NULL) {
        return NULL;
    }
    save_context_bind(ctx);
    return sav_build_ctx_target();
}

/* SavBuildCtx @ 0x00417e70 reads the process-global slot and returns its
 * context block. The portable owner is bound by save_init and typed APIs. */
// FUNCTION: LEGORR 0x00417e70
void* sav_build_ctx_target(void)
{
    if (s_save_current_slot != -1)
        return sav_slot_ctx_target(s_save_current_slot);
    return NULL;
}

/* SavSlotCtx @ 0x00417e50 receives the slot index and returns its context.
 * The portable module has one active GameContext in place of six retail
 * context records, so valid indices resolve to that bound owner. */
// FUNCTION: LEGORR 0x00417e50
void* sav_slot_ctx_target(int slot)
{
    if (slot >= 0 && slot < SAVE_SLOTS && s_save_current_context != NULL)
        return s_save_current_context;
    return NULL;
}

/* SavGetSlot @ 0x00417e90 is the no-argument read of the selected slot. */
// FUNCTION: LEGORR 0x00417e90
int save_slot_get_target(void)
{
    return s_save_current_slot;
}

/* SavSetSlot @ 0x00417ea0 writes the global slot and the bound context view. */
// FUNCTION: LEGORR 0x00417ea0
void save_slot_set_target(int slot)
{
    s_save_current_slot = slot;
}

/* SaveSlotSelectionClear @ 0x00415cf0 is the same process-global reset. */
// FUNCTION: LEGORR 0x00415cf0
void save_slot_selection_clear_target(void)
{
    s_save_current_slot = -1;
}

/* SavWriteRecord @ 0x00417f40 takes only the record index and blob pointer;
 * the current save-slot context is process-global in the corpus. The typed
 * save_write_record adapter above grows the portable record table before
 * entering this target-shaped body. */
// FUNCTION: LEGORR 0x00417f40
int save_write_record_target(int index, const uint8_t* blob)
{
    GameContext* slot_ctx = (GameContext*)sav_build_ctx_target();
    uint32_t* destination;
    const uint32_t* source = (const uint32_t*)blob;
    uint32_t count;
    if (slot_ctx != NULL && index < slot_ctx->record_count) {
        destination = (uint32_t*)(slot_ctx->records
            + index * SAVE_RECORD_SIZE + 4);
        for (count = 0xC63u; count != 0u; --count)
            *destination++ = *source++;
        return 1;
    }
    return 0;
}

/* SavRecordPtr (FUN_00417f90) — see savegame.hpp. */
void* sav_record_ptr(GameContext* ctx, int index)
{
    if (ctx == NULL || index < 0) return NULL;
    save_context_bind(ctx);
    return sav_record_ptr_target(index);
}

/* SavRecordPtr @ 0x00417f90 receives one index and reads the bound context. */
// FUNCTION: LEGORR 0x00417f90
void* sav_record_ptr_target(int index)
{
    GameContext* slot_ctx = (GameContext*)sav_build_ctx_target();
    if (slot_ctx != NULL && index < slot_ctx->record_count)
        return slot_ctx->records + index * SAVE_RECORD_SIZE + 4;
    return NULL;
}

/* --- the generic list window band (see savegame.hpp) ----------------------
 * Each wide-key accessor carries its scan inline like the corpus. The test-
 * pinned null window/key guards remain; the corpus otherwise walks the entry
 * and value arrays blindly. Its two-byte compare exits when either byte of
 * the matching unit is zero (including the high-byte-zero ASCII case). */

#if defined(_MSC_VER) && defined(_M_IX86)
/* The mapped ListFindName body follows the unchecked retail list walk. */
// FUNCTION: LEGORR 0x00416e20
static __declspec(naked) int list_find_name_target(
    const SavListWindow* win, const char* name)
{
    __asm {
        push ebx
        push esi
        push edi
        mov edi, dword ptr [esp + 10h]
        xor esi, esi
        cmp dword ptr [edi], 0
        jle list_find_name_not_found
        mov ebx, dword ptr [esp + 14h]
    list_find_name_loop:
        mov eax, dword ptr [edi + 4]
        mov ecx, dword ptr [eax + esi*4]
        push ecx
        push ebx
        call _strcmpi
        add esp, 8
        test eax, eax
        je list_find_name_found
        mov eax, dword ptr [edi]
        inc esi
        cmp esi, eax
        jl list_find_name_loop
        or eax, 0ffffffffh
        pop edi
        pop esi
        pop ebx
        ret
    list_find_name_found:
        mov eax, esi
        pop edi
        pop esi
        pop ebx
        ret
    list_find_name_not_found:
        pop edi
        pop esi
        or eax, 0ffffffffh
        pop ebx
        ret
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
    }
}
#else
static int list_find_name_target(const SavListWindow* win, const char* name)
{
    int i;
    if (win->count <= 0) return -1;
    for (i = 0; i < win->count; ++i) {
        if (strcasecmp_lite(name, (const char*)win->entries[i]) == 0)
            return i;
    }
    return -1;
}
#endif

int list_find_name(const SavListWindow* win, const char* name)
{
    if (win == NULL || name == NULL) return -1;
    return list_find_name_target(win, name);
}

// FUNCTION: LEGORR 0x00416ed0
void list_add_name(SavListWindow* win, const void* wide_key, void* value)
{
    if (win == NULL || wide_key == NULL) {
        return; /* tested port guard; the corpus dereferences both */
    }
    const int count = win->count;
    if (count <= 0) {
        return;
    }
    void** entry = win->entries;
    const unsigned char* probe = (const unsigned char*)wide_key;
    int index = 0;
    while (index < count) {
        const unsigned char* key = (const unsigned char*)*entry;
        const unsigned char* other = probe;
        int compare;
        for (;;) {
            unsigned char ch = *key;
            if (ch != *other) {
                compare = ch < *other ? -1 : 1;
                break;
            }
            if (ch == 0u) {
                compare = 0;
                break;
            }
            ch = key[1];
            if (ch != other[1]) {
                compare = ch < other[1] ? -1 : 1;
                break;
            }
            key += 2;
            other += 2;
            if (ch == 0u) {
                compare = 0;
                break;
            }
        }
        if (compare == 0) {
            win->values_b[index] = value; /* corpus: blind store through +16 */
            return;
        }
        ++index;
        ++entry;
    }
}

// FUNCTION: LEGORR 0x00416f40
void* list_has_item(const SavListWindow* win, const void* wide_key)
{
    if (win == NULL || wide_key == NULL) {
        return NULL;
    }
    const int count = win->count;
    if (count <= 0) {
        return NULL;
    }
    void* const* entry = win->entries;
    const unsigned char* probe = (const unsigned char*)wide_key;
    int index = 0;
    while (index < count) {
        const unsigned char* key = (const unsigned char*)*entry;
        const unsigned char* other = probe;
        int compare;
        for (;;) {
            unsigned char ch = *key;
            if (ch != *other) {
                compare = ch < *other ? -1 : 1;
                break;
            }
            if (ch == 0u) {
                compare = 0;
                break;
            }
            ch = key[1];
            if (ch != other[1]) {
                compare = ch < other[1] ? -1 : 1;
                break;
            }
            key += 2;
            other += 2;
            if (ch == 0u) {
                compare = 0;
                break;
            }
        }
        if (compare == 0) {
            return win->values_b[index];
        }
        ++index;
        ++entry;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00416fb0
void list_set_node(SavListWindow* win, const void* wide_key, void* value)
{
    if (win == NULL || wide_key == NULL) {
        return; /* tested port guard; the corpus dereferences both */
    }
    const int count = win->count;
    if (count <= 0) {
        return;
    }
    void** entry = win->entries;
    const unsigned char* probe = (const unsigned char*)wide_key;
    int index = 0;
    while (index < count) {
        const unsigned char* key = (const unsigned char*)*entry;
        const unsigned char* other = probe;
        int compare;
        for (;;) {
            unsigned char ch = *key;
            if (ch != *other) {
                compare = ch < *other ? -1 : 1;
                break;
            }
            if (ch == 0u) {
                compare = 0;
                break;
            }
            ch = key[1];
            if (ch != other[1]) {
                compare = ch < other[1] ? -1 : 1;
                break;
            }
            key += 2;
            other += 2;
            if (ch == 0u) {
                compare = 0;
                break;
            }
        }
        if (compare == 0) {
            win->values_a[index] = value; /* corpus: blind store through +12 */
            return;
        }
        ++index;
        ++entry;
    }
}

// FUNCTION: LEGORR 0x00417020
void* list_get_node(const SavListWindow* win, const void* wide_key)
{
    if (win == NULL || wide_key == NULL) {
        return NULL;
    }
    const int count = win->count;
    if (count <= 0) {
        return NULL;
    }
    void* const* entry = win->entries;
    const unsigned char* probe = (const unsigned char*)wide_key;
    int index = 0;
    while (index < count) {
        const unsigned char* key = (const unsigned char*)*entry;
        const unsigned char* other = probe;
        int compare;
        for (;;) {
            unsigned char ch = *key;
            if (ch != *other) {
                compare = ch < *other ? -1 : 1;
                break;
            }
            if (ch == 0u) {
                compare = 0;
                break;
            }
            ch = key[1];
            if (ch != other[1]) {
                compare = ch < other[1] ? -1 : 1;
                break;
            }
            key += 2;
            other += 2;
            if (ch == 0u) {
                compare = 0;
                break;
            }
        }
        if (compare == 0) {
            return win->values_a[index];
        }
        ++index;
        ++entry;
    }
    return NULL;
}

/* LevelNodeGet (FUN_00417090): recursively materialize the LevelLinks tree.
 * The native routine gets its config from globals and caches both an
 * in-progress marker and the node in the two registry value arrays; this
 * adapter takes the config explicitly and uses the same cache slots. */
// FUNCTION: LEGORR 0x00417090
SavTreeNode* level_node_get(SavListWindow* win, BlockFile* cfg,
                            const char* level_name)
{
    if (win == NULL || cfg == NULL || level_name == NULL) {
        return NULL;
    }
    if (win->values_a == NULL || win->values_b == NULL) {
        return NULL;
    }
    if (list_has_item(win, level_name) != NULL) {
        return (SavTreeNode*)list_get_node(win, level_name);
    }

    SavTreeNode* node = (SavTreeNode*)engine_heap_alloc(sizeof(*node));
    list_add_name(win, level_name, (void*)1); /* break LevelLinks cycles */
    list_set_node(win, level_name, node);
    if (node == NULL) {
        return NULL;
    }
    memset(node, 0, sizeof(*node));
    {
        const int index = list_find_name_target(win, level_name);
        if (index < 0) {
            engine_heap_free(node);
            return NULL;
        }
        node->window = (uint32_t)index;
    }

    const char* group = strstr(level_name, "::");
    group = group != NULL ? group + 2 : level_name;
    char path[256];
    const char* root = util_root_name(cfg);
    snprintf(path, sizeof(path), "%s::Levels::%s::LevelLinks", root, group);
    const char* links = block_get_strptr(cfg, path);
    if (links == NULL) {
        snprintf(path, sizeof(path), "%s::%s::LevelLinks", root, group);
        links = block_get_strptr(cfg, path);
    }
    if (links == NULL) {
        return node;
    }

    char names[15][FE_LEVEL_NAME_MAX];
    uint32_t count = 0;
    const char* p = links;
    while (*p != '\0' && count < 15) {
        p += strspn(p, ", \t");
        if (*p == '\0') {
            break;
        }
        const char* end = p + strcspn(p, ",");
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
        list_set_node(win, level_name, NULL);
        engine_heap_free(node);
        return NULL;
    }

    SavTreeNode** children = (SavTreeNode**)engine_heap_alloc(
        (long)(count * sizeof(*children)));
    if (children == NULL) {
        list_set_node(win, level_name, NULL);
        list_add_name(win, level_name, NULL);
        engine_heap_free(node);
        return NULL;
    }
    memset(children, 0, count * sizeof(*children));
    node->children = children;
    node->count = count;
    for (uint32_t i = 0; i < count; ++i) {
        children[i] = level_node_get(win, cfg, names[i]);
    }
    return node;
}

/* SavTreeWalk (FUN_004171c0) — see savegame.hpp. */
// FUNCTION: LEGORR 0x004171c0
/* On x86, retain the corpus's push-user/push-node/flag-store/call order.
 * The callback-null guard is kept because test_savegame.cpp pins it. */
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) int sav_tree_walk(void* node, sav_tree_visit visit,
                                    void* user)
{
    __asm {
        push ebx
        push ebp
        push esi
        mov esi, dword ptr [esp + 10h]
        test esi, esi
        push edi
        je sav_tree_walk_empty
        mov eax, dword ptr [esi + 10h]
        test eax, eax
        jne sav_tree_walk_empty
        mov ebx, dword ptr [esp + 1ch]
        mov ebp, dword ptr [esp + 18h]
        test ebp, ebp
        je sav_tree_walk_no_callback
        push ebx
        push esi
        mov dword ptr [esi + 10h], 1
        call ebp
        add esp, 8
        test eax, eax
        je sav_tree_walk_children
        mov eax, 1
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    sav_tree_walk_no_callback:
        mov dword ptr [esi + 10h], 1
    sav_tree_walk_children:
        mov eax, dword ptr [esi + 4]
        test eax, eax
        je sav_tree_walk_empty
        mov eax, dword ptr [esi + 8]
        xor edi, edi
        test eax, eax
        jbe sav_tree_walk_empty
    sav_tree_walk_loop:
        mov eax, dword ptr [esi + 4]
        push ebx
        push ebp
        mov ecx, dword ptr [eax + edi * 4]
        push ecx
        call sav_tree_walk
        add esp, 0ch
        test eax, eax
        jne sav_tree_walk_child_found
        mov eax, dword ptr [esi + 8]
        inc edi
        cmp edi, eax
        jb sav_tree_walk_loop
        xor eax, eax
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    sav_tree_walk_child_found:
        mov eax, 1
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    sav_tree_walk_empty:
        pop edi
        pop esi
        pop ebp
        xor eax, eax
        pop ebx
        ret
    }
}
#else
int sav_tree_walk(void* node, sav_tree_visit visit, void* user)
{
    SavTreeNode* n = (SavTreeNode*)node;
    if (n == NULL) {
        return 0;
    }
    /* SavTreeNode preserves the corpus's reserved +0x0c word so the walk
     * flag remains at +0x10 on x86; the pointer slot widens on the host. */
    if (n->visited != 0u) {
        return 0;
    }
    int visit_result = 0;
    if (visit != NULL) {
        n->visited = 1u;
        visit_result = visit(node, user);
    } else {
        n->visited = 1u; /* test-pinned guard; corpus calls callback blind */
    }
    if (visit_result != 0) {
        return 1;
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
#endif

/* SavTreeWalk2 (FUN_004172f0) — see savegame.hpp. */
// FUNCTION: LEGORR 0x004172f0
void sav_tree_walk2(void* node, SavTree2State* state, int carry,
                    const EngineContext* eng)
{
    SavTreeNode* n = (SavTreeNode*)node;
    if (n == NULL || n->visited != 0u) {
        return;
    }
    const int extended = 1 < engine_app_mode_get()
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
    if (node == s_sav_tree_root[0]) sav_tree_owned_visit_flags_clear(0);
    if (node == s_sav_tree_root[1]) sav_tree_owned_visit_flags_clear(1);
    sav_tree_walk2(node, &st, 1, eng);
    sav_clear_visited();
    if (node == s_sav_tree_root[0]) sav_tree_owned_visit_flags_clear(0);
    if (node == s_sav_tree_root[1]) sav_tree_owned_visit_flags_clear(1);
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

// FUNCTION: LEGORR 0x00417250
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
    if (sav_tree_walk(root, sav_tree_nth_visit, &st) != 0) {
        sav_clear_visited(); /* corpus call site 1: the hit path */
        return st.found;
    }
    sav_clear_visited(); /* corpus call site 2: the miss path */
    return NULL;
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

static void* sav_current_slot_record_window(void)
{
    int slot;
    GameContext* ctx;
    void* installed = sav_slot_stride_window_peek();
    if (installed != NULL) return installed;
    slot = save_slot_get_target();
    if (slot < 0 || slot >= SAVE_SLOTS) return NULL;
    if (s_sav_slot_records[slot] != NULL)
        return s_sav_slot_records[slot];
    ctx = save_current_context_get();
    if (ctx != NULL && ctx->active_slot == slot && ctx->records != NULL)
        return ctx->records;
    return NULL;
}

/* Build the dynamic table for roots installed without an ownership list.
 * sav_tree_walk marks each unique node before invoking this callback. */
typedef struct SavVisitedCollectState {
    void*** items;
    uint32_t* count;
    uint32_t* capacity;
    int* failed;
} SavVisitedCollectState;

static int sav_visited_collect_visit(void* value, void* user)
{
    SavVisitedCollectState* st = (SavVisitedCollectState*)user;
    if (*st->count == *st->capacity) {
        uint32_t next = *st->capacity == 0u ? 8u : *st->capacity * 2u;
        void** grown;
        if (next < *st->capacity ||
            (size_t)next > ((size_t)-1) / sizeof(**st->items)) {
            *st->failed = 1;
            ((SavTreeNode*)value)->visited = 0u;
            return 1;
        }
        grown = (void**)realloc(*st->items, (size_t)next * sizeof(**st->items));
        if (grown == NULL) {
            *st->failed = 1;
            ((SavTreeNode*)value)->visited = 0u;
            return 1;
        }
        *st->items = grown;
        *st->capacity = next;
    }
    (*st->items)[(*st->count)++] = value;
    return 0;
}

static int sav_visited_band_install(int which, void* const* items,
                                    uint32_t count)
{
    SavVisitedBand next;
    uint32_t i;
    if (which < 0 || which > 1 ||
        (count != 0u && items == NULL) ||
        (size_t)count > ((size_t)-1) / sizeof(*next.items) ||
        (size_t)count > ((size_t)-1) / sizeof(*next.words)) return 0;
    next.items = NULL;
    next.words = NULL;
    next.count = 0u;
    next.root_bound = 1;
    if (count != 0u) {
        next.items = (void**)malloc((size_t)count * sizeof(*next.items));
        next.words = (uint32_t*)malloc((size_t)count * sizeof(*next.words));
        if (next.items == NULL || next.words == NULL) {
            free(next.items);
            free(next.words);
            return 0;
        }
        for (i = 0u; i < count; ++i) {
            /* LevelChainBuild zeroes both dynamic bands. The executable
             * contains no stores that seed the record-pointer array. */
            next.items[i] = NULL;
            next.words[i] = 0u;
        }
        next.count = count;
    }
    sav_visited_band_release(which);
    s_sav_visited_bands[which] = next;
    sav_visited_band_alias_sync(which);
    return 1;
}

static void sav_visited_band_release(int which)
{
    if (which < 0 || which > 1) return;
    free(s_sav_visited_bands[which].items);
    free(s_sav_visited_bands[which].words);
    s_sav_visited_bands[which].items = NULL;
    s_sav_visited_bands[which].words = NULL;
    s_sav_visited_bands[which].count = 0u;
    s_sav_visited_bands[which].root_bound = 0;
    sav_visited_band_alias_sync(which);
}

/* SavClearVisited (FUN_00416e70): clear each installed record marker and its
 * paired per-index word, out-of-game band first and in-game band second. The
 * two active counts are set from the same root sizes used to allocate these
 * sidecar arrays. */
// FUNCTION: LEGORR 0x00416e70
void sav_clear_visited(void)
{
    int iVar2;

    iVar2 = 0;
    if (0 < s_active_object_outgame) {
        do {
            void* record = s_sav_visited_outgame_items[iVar2];
            if (record != NULL) {
                ((uint32_t*)record)[4] = 0u;
                s_sav_visited_outgame_words[iVar2] = 0u;
            }
            iVar2 = iVar2 + 1;
        } while (iVar2 < s_active_object_outgame);
    }
    iVar2 = 0;
    if (0 < s_active_object_ingame) {
        do {
            void* record = s_sav_visited_ingame_items[iVar2];
            if (record != NULL) {
                ((uint32_t*)record)[4] = 0u;
                s_sav_visited_ingame_words[iVar2] = 0u;
            }
            iVar2 = iVar2 + 1;
        } while (iVar2 < s_active_object_ingame);
    }
}

void sav_tree_root_install(int which, void* node)
{
    void** items = NULL;
    uint32_t count = 0u;
    uint32_t capacity = 0u;
    int failed = 0;
    SavVisitedCollectState collector;
    if (which < 0 || which > 1) return;
    if (node != NULL) {
        collector.items = &items;
        collector.count = &count;
        collector.capacity = &capacity;
        collector.failed = &failed;
        (void)sav_tree_walk(node, sav_visited_collect_visit, &collector);
        for (uint32_t i = 0u; i < count; ++i) {
            ((SavTreeNode*)items[i])->visited = 0u;
        }
        if (failed) {
            free(items);
            items = NULL;
            count = 0u;
        }
    }
    sav_tree_owned_nodes_release(which);
    s_sav_tree_root[which] = node;
    if (sav_visited_band_install(which, items, count) &&
        count <= 0x7fffffffu) {
        active_object_list_count_set(which, (int)count);
    }
    free(items);
}

static int sav_visited_band_reserve(int which, uint32_t count)
{
    SavVisitedBand* band;
    void** items;
    uint32_t* words;
    uint32_t i;
    if (which < 0 || which > 1) return 0;
    band = &s_sav_visited_bands[which];
    if (count <= band->count) return 1;
    if ((size_t)count > ((size_t)-1) / sizeof(*items) ||
        (size_t)count > ((size_t)-1) / sizeof(*words)) return 0;
    items = (void**)malloc((size_t)count * sizeof(*items));
    words = (uint32_t*)malloc((size_t)count * sizeof(*words));
    if (items == NULL || words == NULL) {
        free(items);
        free(words);
        return 0;
    }
    for (i = 0u; i < band->count; ++i) {
        items[i] = band->items[i];
        words[i] = band->words[i];
    }
    for (; i < count; ++i) {
        items[i] = NULL;
        words[i] = 0u;
    }
    free(band->items);
    free(band->words);
    band->items = items;
    band->words = words;
    band->count = count;
    sav_visited_band_alias_sync(which);
    return 1;
}

void sav_tree_visited_entry_set(int which, uint32_t index, void* record,
                                uint32_t word)
{
    uint32_t count;
    if (which < 0 || which > 1 || index == 0xffffffffu) return;
    if ((s_sav_visited_bands[which].root_bound &&
         index >= s_sav_visited_bands[which].count) ||
        (!s_sav_visited_bands[which].root_bound &&
         index > s_sav_visited_bands[which].count)) return;
    count = index + 1u;
    if (!s_sav_visited_bands[which].root_bound &&
        index == s_sav_visited_bands[which].count &&
        !sav_visited_band_reserve(which, count)) return;
    s_sav_visited_bands[which].items[index] = record;
    s_sav_visited_bands[which].words[index] = word;
}

void sav_tree_visited_entry_peek(int which, uint32_t index, void** record,
                                 uint32_t* word)
{
    SavVisitedBand* band;
    if (which < 0 || which > 1) return;
    band = &s_sav_visited_bands[which];
    if (index >= band->count) return;
    if (record != NULL) *record = band->items[index];
    if (word != NULL) *word = band->words[index];
}

void* sav_tree_root_peek(int which)
{
    return (which == 0 || which == 1) ? s_sav_tree_root[which] : NULL;
}

int sav_tree_root_install_owned(int which, void* node,
                                void* const* nodes, uint32_t node_count)
{
    void** owned = NULL;
    uint32_t owned_count = 0u;
    uint32_t i;
    if (which < 0 || which > 1 || (node_count != 0u && nodes == NULL))
        return 0;
    if (node_count != 0u) {
        if ((size_t)node_count > ((size_t)-1) / sizeof(*owned)) return 0;
        owned = (void**)malloc((size_t)node_count * sizeof(*owned));
        if (owned == NULL) return 0;
        for (i = 0u; i < node_count; ++i) {
            if (nodes[i] != NULL && (uintptr_t)nodes[i] > 1u)
                owned[owned_count++] = nodes[i];
        }
    }
    sav_tree_owned_nodes_release(which);
    s_sav_tree_root[which] = node;
    s_sav_owned_tree_nodes[which] = owned;
    s_sav_owned_tree_node_count[which] = owned_count;
    if (sav_visited_band_install(which, nodes, node_count) &&
        node_count <= 0x7fffffffu) {
        active_object_list_count_set(which, (int)node_count);
    }
    return 1;
}

void sav_tree_roots_clear(void)
{
    sav_tree_owned_nodes_release(0);
    sav_tree_owned_nodes_release(1);
}

void save_menu_refresh(GameContext* ctx, int slot)
{
    if (ctx == NULL) return;
    save_context_bind(ctx);
    save_menu_refresh_target(0u, slot);
    ctx->save_slot = slot;
}

/* SaveMenuRefresh @ 0x004150b0 takes the old context word and new slot; the
 * first word is unused by the recovered body, while save context is global. */
// FUNCTION: LEGORR 0x004150b0
void save_menu_refresh_target(uint32_t unused, int slot)
{
    void* rec;
    void* base = NULL;
    uint32_t flag = 1u;
    (void)unused;
    save_slot_set_target(slot);
    rec = sav_build_ctx_target();
    if (rec != NULL) {
        flag = 0u;
        base = sav_current_slot_record_window(); /* corpus: rec + 0x34 */
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
void show_level_overlay_for_current_selection(GameContext* ctx)
{
    if (ctx == NULL) return;
    save_context_bind(ctx);
    show_level_overlay_target();
}

/* ShowLevelOverlayForCurrentSelection @ 0x00418000 is a no-argument global
 * entry. It commits the active slot before rebuilding the save views. */
// FUNCTION: LEGORR 0x00418000
void show_level_overlay_target(void)
{
    GameContext* ctx = save_current_context_get();
    const int slot = save_slot_get_target();
    if (ctx == NULL || slot < 0 || slot >= SAVE_SLOTS) return;
    (void)save_write_slot(ctx, slot); /* FUN_00417bd0 */
    save_menu_refresh_target(0u, slot);
}

/* --- the checkpoint progress fold band (see savegame.hpp) ------------------ */

/* Explicit-context adapter for portable callers and isolated state tests.
 * The retail target itself is the no-argument current-owner entry above. */
int checkpoint_blob_task_progress_recalculate(GameContext* ctx)
{
    GameContext* previous_context;
    int previous_slot;
    int result;
    if (ctx == NULL) return 0;
    previous_context = s_save_current_context;
    previous_slot = s_save_current_slot;
    save_context_bind(ctx);
    result = checkpoint_blob_task_progress_recalculate_target();
    s_save_current_context = previous_context;
    s_save_current_slot = previous_slot;
    return result;
}

/* SavRefreshSlots (2026-10-25) — FUN_00417df0. Walk the six slot
 * contexts so every slot file exists. The corpus samples the engine
 * feature word once before the walk: with bit 0x400 raised it writes
 * each slot fresh before reading it back; otherwise it reads (and
 * re-reads once when the first attempt fails). Deviations: the port's
 * single active-slot context stands in for the corpus's six
 * DAT_00557080 blocks (the SavSlotCtx precedent), the null context is
 * guarded, and the corpus reads its context-pointer block blind. */
void sav_refresh_slots(GameContext* ctx)
{
    if (ctx == NULL) return;
    save_context_bind(ctx);
    sav_refresh_slots_target();
}

// FUNCTION: LEGORR 0x00417df0
void sav_refresh_slots_target(void)
{
    GameContext* ctx = s_save_current_context;
    uint8_t* slot_context = s_sav_slot_context_blocks;
    uint8_t* const slot_context_end =
        s_sav_slot_context_blocks + sizeof(s_sav_slot_context_blocks);
    int slot = 0;
    if (ctx == NULL) return;
    const int write_first =
        (engine_feature_word_peek() & 0x400u) != 0u;
    do {
        if (!write_first) {
            if (save_read_slot_target((uint32_t)slot, slot_context, 0) != 0)
                goto next_slot;
        } else {
            uint32_t record_count = SAVE_DEFAULT_RECORDS;
            (void)save_tree_level_count(0, &record_count);
            (void)save_slot_write_fresh_file(ctx, slot, record_count);
        }
        (void)save_read_slot_target((uint32_t)slot, slot_context, 0);
next_slot:
        slot_context += 0xb8;
        ++slot;
    } while (slot_context < slot_context_end);
}

namespace unported_ledger_stubs {

/* The corpus globals at DAT_005529c4 and DAT_00555118 hold the selected
 * type-4 name list and Pilot grid count. Keep their state module-owned, as
 * GameContext is the port's owner for the active level and checkpoint. */
static EntityType4IndexNameList s_start_level_type4_list;
static uint32_t s_start_level_pilot_count;

} // namespace unported_ledger_stubs

void start_level_selected(GameContext* ctx)
{
    if (ctx == NULL || ctx->level == NULL || checkpoint_blob_get(ctx) == NULL) {
        return;
    }
    unported_ledger_stubs::ledger_stub_00460290_EntityType4IndexListBuild(
        &unported_ledger_stubs::s_start_level_type4_list);
    unported_ledger_stubs::s_start_level_pilot_count =
        entity_type_grid_record_count_get("Pilot", 0);
}

const EntityType4IndexNameList* start_level_entity_type4_list_get(void)
{
    return &unported_ledger_stubs::s_start_level_type4_list;
}

uint32_t start_level_pilot_grid_count_get(void)
{
    return unported_ledger_stubs::s_start_level_pilot_count;
}

namespace unported_ledger_stubs {

/* Adapter signature for the no-argument retail entry. */
void ledger_stub_00460220_StartLevelSelected(GameContext* ctx)
{
    start_level_selected(ctx);
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// FUNCTION: LEGORR 0x00418040
int ledger_stub_00418040_CampaignChainSnapshotCreate(
    const GameContext* ctx, void* snapshot, size_t capacity)
{
    unsigned char* bytes = (unsigned char*)snapshot;
    size_t records_size;
    uint32_t record_count;
    uint32_t records_offset;
    uint32_t header_word;
    uint32_t word_index;
    if (ctx == NULL || bytes == NULL ||
        sav_build_ctx((GameContext*)ctx) == NULL || ctx->record_count < 0)
        return 0;
    records_size = (size_t)ctx->record_count * SAVE_RECORD_SIZE;
    if ((size_t)ctx->record_count != 0u
        && records_size / (size_t)ctx->record_count != SAVE_RECORD_SIZE)
        return 0;
    if (capacity < 0xb8u + records_size
        || (records_size != 0u && ctx->records == NULL)) return 0;
    /* Copy the active 46-dword slot-context window, matching the decompile's
     * first 0xB8-byte copy. The active GameContext owns the live record count
     * and record bytes; encode their in-buffer location in the copied +0x34
     * pointer slot because the portable snapshot is a caller-owned buffer. */
    for (word_index = 0u; word_index < 0x2eu; ++word_index) {
        header_word = options_slot_window_dword_peek((int)word_index);
        memcpy(bytes + word_index * sizeof(header_word), &header_word,
               sizeof(header_word));
    }
    record_count = (uint32_t)ctx->record_count;
    records_offset = records_size != 0u ? 0xb8u : 0u;
    memcpy(bytes + 0x18, &record_count, sizeof(record_count));
    memcpy(bytes + 0x34, &records_offset, sizeof(records_offset));
    if (records_size != 0u)
        memcpy(bytes + 0xb8u, ctx->records, records_size);
    return 1;
}

/* The reference receives only the destination pointer; the active context
 * supplies both the slot window and record table. The portable snapshot is
 * contiguous and encodes its record offset instead of owning a separate
 * allocation as the original does. */
void campaign_chain_snapshot_create_target(void* snapshot)
{
    GameContext* ctx = save_current_context_get();
    size_t capacity;
    if (ctx == NULL || snapshot == NULL || ctx->record_count < 0) return;
    if ((size_t)ctx->record_count > ((size_t)-1 - 0xb8u) / SAVE_RECORD_SIZE)
        return;
    capacity = 0xb8u + (size_t)ctx->record_count * SAVE_RECORD_SIZE;
    (void)unported_ledger_stubs::
        ledger_stub_00418040_CampaignChainSnapshotCreate(ctx, snapshot,
                                                        capacity);
}

} // namespace unported_ledger_stubs
