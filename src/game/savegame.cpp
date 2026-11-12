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
#include "savegame_for_frontend_unit.hpp"
#include "savegame_from_frontend_unit.hpp"

#if defined(LEGORRR_BACKEND_WINAPI)
#  define SAVE_SEP '\\'  /* the corpus's "%s\%i.sav" DOS separators */
#else
#  define SAVE_SEP '/'   /* host-native on the POSIX port */
#endif

/* The `.sav` open modes (corpus constants): DAT_004a2770 ("rb") and
 * DAT_004a2794 ("wb"). */
const char* MODE_RB = "rb";
const char* MODE_WB = "wb";
void* s_sav_tree_root[2]; /* the two corpus tree-root globals' role */
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
void** s_sav_visited_outgame_items;
// GLOBAL: LEGORR 0x00556ff4
uint32_t* s_sav_visited_outgame_words;
// GLOBAL: LEGORR 0x00556fdc
void** s_sav_visited_ingame_items;
// GLOBAL: LEGORR 0x00556fe0
uint32_t* s_sav_visited_ingame_words;
// GLOBAL: LEGORR 0x005574d0
int s_save_current_slot = -1;
/* The six 0xb8-byte per-slot contexts from DAT_00557080..DAT_005574d0. */
// GLOBAL: LEGORR 0x00557080
uint8_t s_sav_slot_context_blocks[SAVE_SLOTS * 0xb8];
/* Wider-host pointers cannot fit the retail context's 32-bit +0x34 slot.
 * Keep each scanned slot's record allocation beside its 0xb8-byte header. */
uint8_t* s_sav_slot_records[SAVE_SLOTS];
int32_t s_sav_slot_record_counts[SAVE_SLOTS];
void save_slot_sidecar_release(int slot);
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

void sav_tree_owned_visit_flags_clear(int which)
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

int save_tree_level_count(int which, uint32_t* count_out)
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

int save_current_level_counts(uint32_t* campaign_count,
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
void save_release_records(GameContext* ctx)
{
    free(ctx->records);
    ctx->records = NULL;
    ctx->record_count = 0;
    ctx->active_slot = -1;
}

void save_slot_sidecar_release(int slot)
{
    if (slot < 0 || slot >= SAVE_SLOTS) return;
    free(s_sav_slot_records[slot]);
    s_sav_slot_records[slot] = NULL;
    s_sav_slot_record_counts[slot] = 0;
}

/* Fresh empty slot context — corpus SavWriteSlot's bootstrap: count 5,
 * zero-filled records (finding 08: "bootstraps slot 5 with a zero-filled
 * record array"). */
int save_bootstrap_records(GameContext* ctx, int count)
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

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void save_set_current_record(GameContext* ctx, int index)
{
    if (ctx != NULL) {
        ctx->current_record = index;
    }
}
#endif /* !LEGORRR_BACKEND_WINAPI */

uint32_t* save_record_flags(GameContext* ctx, int index)
{
    if (ctx == NULL || ctx->records == NULL || index < 0
        || index >= ctx->record_count) {
        return NULL;
    }
    return (uint32_t*)(ctx->records + (size_t)index * SAVE_RECORD_SIZE);
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

int osf_read_path(GameContext* ctx, int slot)
{
    char path[280];
    save_slot_path(ctx, path, sizeof(path), slot, "osf");
    return osf_read(ctx, path);
}

/* --- the save-slot band (2026-10-24; findings 21/408) ---------------------- */

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
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
#endif /* !LEGORRR_BACKEND_WINAPI */

/* SaveStateEventContextSet (FUN_00418110) — zero-init module static per
 * the unowned-global convention (the corpus's word is DAT_0055781c). */
// GLOBAL: LEGORR 0x0055781c
uint32_t s_save_state_event_context = 0u;

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

/* SavRecordPtr (FUN_00417f90) — see savegame.hpp. */
void* sav_record_ptr(GameContext* ctx, int index)
{
    if (ctx == NULL || index < 0) return NULL;
    save_context_bind(ctx);
    return sav_record_ptr_target(index);
}

/* --- the generic list window band (see savegame.hpp) ----------------------
 * Each wide-key accessor carries its scan inline like the corpus. The test-
 * pinned null window/key guards remain; the corpus otherwise walks the entry
 * and value arrays blindly. Its two-byte compare exits when either byte of
 * the matching unit is zero (including the high-byte-zero ASCII case). */

int list_find_name(const SavListWindow* win, const char* name)
{
    if (win == NULL || name == NULL) return -1;
    return list_find_name_target(win, name);
}

/* SaveMenuRefresh (FUN_004150b0) — see savegame.hpp. The corpus reads
 * its slot-context chain, the context's +0x34 stride-window word, and
 * the two tree roots all blind; the port models each with an
 * own-storage hook (zero-defaulted, so an unprimed refresh just
 * guards through both walks). */
static void**  s_sav_menu_windows[2]; /* the chain + 0x10 word's slot role */
static void*   s_sav_slot_stride_window; /* the context + 0x34 word's role */

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void sav_menu_window_install(int which, void** slot)
{
    if (which == 0 || which == 1) {
        s_sav_menu_windows[which] = slot;
    }
}
#endif /* !LEGORRR_BACKEND_WINAPI */

void** sav_menu_window_peek(int which)
{
    return (which == 0 || which == 1) ? s_sav_menu_windows[which] : NULL;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void sav_slot_stride_window_install(void* base)
{
    s_sav_slot_stride_window = base;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

void* sav_slot_stride_window_peek(void)
{
    return s_sav_slot_stride_window;
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

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
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
#endif /* !LEGORRR_BACKEND_WINAPI */

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

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void save_menu_refresh(GameContext* ctx, int slot)
{
    if (ctx == NULL) return;
    save_context_bind(ctx);
    save_menu_refresh_target(0u, slot);
    ctx->save_slot = slot;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

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
