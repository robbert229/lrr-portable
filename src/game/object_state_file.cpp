// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-030 GAME-030: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-030.csv. */
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
#include <math.h>
#include <float.h>
#include <stdint.h>
#include <ctype.h>
#include "ai.hpp"
#include "building_scene.hpp"
#include "actor_scene.hpp"
#include "type_config_ports.hpp"
#include "entity_pose_delta.hpp"
#include "entity_motion_ports.hpp"
#include "activity_timeout.hpp"
#include "entity_motion_probe.hpp"
#include "port_loop_ports.hpp"
#include "simulation_ui_ports.hpp"
#include "type_motion_scale.hpp"
#include "entity_quad_collision.hpp"
#include "spawn.hpp"
#include "weapon.hpp"
#include "effects.hpp"
#include "terrain_grid.hpp"
#include "cmd.hpp"
#include "select.hpp"
#include "gods98/model.hpp"
#include "gods98/view.hpp"
#include "gods98/sound.hpp"
#include "gods98/message.hpp"
#include "gods98/string.hpp"
#include "upgrades.hpp"
#include "entity_for_object_state_file.hpp"
#include "gods98/media.hpp"
#include "savegame_for_object_state_file.hpp"

/* Dedicated live-table globals preserve the original no-argument reset ABI.
 * OsfRead borrows its GameContext owner into these words before invoking the
 * mapped reset body, then synchronizes the resulting ownership back. */
// GLOBAL: LEGORR 0x004ffe50
static GameOsfEntry* s_osf_live_table;
// GLOBAL: LEGORR 0x004ffe54
static int s_osf_live_count;
// GLOBAL: LEGORR 0x004ffe58
static int s_osf_live_capacity;

// PORT-ONLY: the port's context-based ObjectRecall_StoreMiniFigure; the matching build has the original-shape 0x00459190 at the end of this file.
void osf_add(GameContext* ctx, int32_t id, int32_t ref, const char* name)
{
    if (ctx == NULL) {
        return;
    }
    if (ctx->osf_count >= ctx->osf_capacity) {
        int new_capacity = ctx->osf_capacity + 10;
        GameOsfEntry* grown = (GameOsfEntry*)realloc(
            ctx->osf, (size_t)new_capacity * sizeof(GameOsfEntry));
        if (grown == NULL) return;
        ctx->osf = grown;
        ctx->osf_capacity = new_capacity;
    }
    GameOsfEntry* e = &ctx->osf[ctx->osf_count++];
    e->id = id;
    e->ref = ref;
    /* The corpus clears the whole 12-byte slot before its strncpy(0xb),
     * leaving the final byte zero even when the name fills the copy window. */
    const uint32_t zero = 0u;
    __builtin_memcpy(e->name + 0, &zero, sizeof zero);
    __builtin_memcpy(e->name + 4, &zero, sizeof zero);
    __builtin_memcpy(e->name + 8, &zero, sizeof zero);
    if (name != NULL) {
        crt_string_copy_n(e->name, name, 11u); /* corpus calls 0x48e770 */
    }
    ctx->osf_dirty = 1;
}

/* PopEntityStateOverride (FUN_00459240): only type-2 entities consume the
 * global LIFO; restore +0x3f0 and the saved name, then apply +0x330. */
// PORT-ONLY: the port's ObjectRecall_RecallMiniFigure on its own LIFO; the matching build has the original-shape 0x00459240 at the end of this file.
int entity_state_override_pop(EntitySlot* entity)
{
    EntityStateOverrideEntry entry;
    if (entity == NULL || entity->type != 2 ||
        s_entity_state_override_count == 0u) {
        return 0;
    }
    entry = s_entity_state_override_entries[--s_entity_state_override_count];
    entity->flags_3f0 = entry.flags_3f0;
    if (s_entity_state_override_rename != NULL) {
        s_entity_state_override_rename(entity, entry.name,
                                       s_entity_state_override_context);
    }
    else {
        size_t length = 0;
        while (length < sizeof(entry.name) && entry.name[length] != '\0') {
            entity->name[length] = entry.name[length];
            ++length;
        }
        entity->name[length] = '\0';
    }
    if (s_entity_state_override_value_330_apply != NULL) {
        s_entity_state_override_value_330_apply(
            entity, entry.value_330, s_entity_state_override_context);
    }
    else {
        entity->indexed_record_slot_330 = (int32_t)entry.value_330;
    }
    return 1;
}

// FUNCTION: LEGORR 0x004592a0
static void osf_read_state_reset(void)
{
    s_osf_live_count = 0;
    s_osf_live_capacity = 0;
    if (s_osf_live_table != NULL) engine_heap_free(s_osf_live_table);
    s_osf_live_table = NULL;
}
// PORT-ONLY: the port's context-based ObjectRecall_Snapshot; the matching build has the original-shape 0x004592e0 at the end of this file.
void osf_snapshot(GameContext* ctx)
{
    if (ctx == NULL || ctx->osf_count == 0) {
        return;
    }
    GameOsfEntry* snapshot = (GameOsfEntry*)malloc(
        (size_t)ctx->osf_count * sizeof(GameOsfEntry));
    if (snapshot == NULL) return;
    __builtin_memcpy(snapshot, ctx->osf,
                     (size_t)ctx->osf_count * sizeof(GameOsfEntry));
    free(ctx->osf_shadow);
    ctx->osf_shadow = snapshot;
    ctx->osf_shadow_count = ctx->osf_count;
    ctx->osf_shadow_dirty = 1;
}

/* --- the save-side DAT accessor band (see savegame.hpp) --------------------- */
// PORT-ONLY: the port's context-based osf_read_dirty_flag_get; the matching build has the original-shape 0x004593d0 at the end of this file.
int osf_read_dirty_flag_get(const GameContext* ctx)
{
    return ctx != NULL ? ctx->osf_dirty : 0;
}

/* The port's .osf writer, on the game context. */
// PORT-ONLY: the port's context-based ObjectRecall_Save; the matching build has the original-shape 0x004593e0 at the end of this file.
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
 * count is bounded by the exact file length and allocation-size limits. */
// PORT-ONLY: the port's context-based osf_read; the matching build has the original-shape 0x00459470 at the end of this file.
int osf_read(GameContext* ctx, const char* path)
{
    if (ctx == NULL || path == NULL) {
        return 0;
    }
    /* OsfReadStateReset clears only the live table. The corpus leaves the
     * restart snapshot intact when the file is absent or invalid; a valid
     * read replaces it through OsfSnapshot below. */
    s_osf_live_table = ctx->osf;
    s_osf_live_count = ctx->osf_count;
    s_osf_live_capacity = ctx->osf_capacity;
    osf_read_state_reset();
    ctx->osf = s_osf_live_table;
    ctx->osf_count = s_osf_live_count;
    ctx->osf_capacity = s_osf_live_capacity;
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
          && count_le <= 0x7fffffffu
          && (size_t)count_le <= ((size_t)-1) / sizeof(GameOsfEntry);
    }
    if (ok) {
        size_t n = (size_t)count_le * 0x14;      /* count * 20 bytes */
        GameOsfEntry* loaded = count_le != 0
            ? (GameOsfEntry*)malloc(n) : NULL;
        if ((count_le == 0 || loaded != NULL) &&
            (count_le == 0 || fread(loaded, 1, n, fp) == n)) {
            ctx->osf = loaded;
            ctx->osf_count = (int)count_le;
            ctx->osf_capacity = (int)count_le;
            osf_snapshot(ctx);                   /* live -> shadow     */
            ctx->osf_dirty = 0;                  /* DAT_004ffe64 = 0   */
            ctx->osf_shadow_dirty = 0;
        }
        else free(loaded);
        /* else: corrupt (short read) -> table stays empty */
    }
    fclose(fp);
    return 1;   /* uVar3 — the file opened */
}

/* Original-ABI adapters (ADR 0005 step 6) for GAME-021's lego_unit.c: the
 * original FUN_004593d0 and FUN_00459470 take no context; the port's
 * entries above take the game context. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (osf_read_dirty_flag_get, 0x004593d0)
extern "C" int osf_read_dirty_flag_get_original(void)
{
    return osf_read_dirty_flag_get(entity_pool_callback_context_get());
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (osf_read, 0x00459470)
extern "C" int osf_read_original(const char* path)
{
    return osf_read(entity_pool_callback_context_get(), path);
}

#if defined(_MSC_VER) && _MSC_VER < 1200
/* The matching build: ObjectRecall.c's functions in the original's shape
 * and address order, on the live recall table above (004ffe50-004ffe58)
 * and the shadow table below (#562, finding 642; #749, finding 648). The
 * port's context-based models above stay the native ones. Callers reach
 * them by link-name aliases: FrontEnd.c's Front_Save_WriteSaveFile
 * (004593e0, frontend_unit_link_names.h), Lego.c's lego_level_load and
 * 0042e020 (004593d0, 00459470, 00459240, lego_unit_imports.h),
 * LegoObject.c (00459190, 00459240, legoobject_unit_imports.h) and
 * Objective.c's 00458be0 (004592e0, 00459360, objective_unit_imports.h).
 * FrontEnd.c's call of 00459360 still goes to its adapter.
 *
 * The block follows the port's osf_read, not 00459190's address order:
 * placed earlier, it moves MSVC's symbol counter for osf_read (rule B,
 * finding 558). The object's fields are read at their original offsets:
 * the port's EntitySlot keeps an inline name at +0x8, where LegoObject has
 * a char*. Stats.c's 00469c10 has its link name from
 * entity_stats_link_names.h and File_Length (0047fca0) its gods98/files.h
 * name; File_Open, File_Read, File_Write and File_Close (0047f6e0,
 * 0047f980, 0047fa50, 0047fac0) have their resource.hpp names. */
#define OSF_OBJ_WORD(obj, off) (*(int*)((char*)(obj) + (off)))
#define OSF_OBJ_NAME(obj) (*(char**)((char*)(obj) + 0x8))
extern "C" int entity_building_index_set(void* obj, unsigned int newLevel);
extern "C" void legoobject_rename(void* obj, const char* name);
extern "C" int file_length(GameFileHandle* f);

// GLOBAL: LEGORR 0x004ffe5c
static GameOsfEntry* s_osf_shadow_table;
// GLOBAL: LEGORR 0x004ffe60
static int s_osf_shadow_count;
// GLOBAL: LEGORR 0x004ffe64
static int s_osf_shadow_dirty;

/* ObjectRecall_StoreMiniFigure: append a minifigure's ability flags
 * (+0x3f0), level (+0x330) and custom name to the live recall table,
 * growing it by ten 0x14-byte entries. */
// FUNCTION: LEGORR 0x00459190
extern "C" void object_recall_store_minifigure(void* liveObj)
{
    GameOsfEntry* recallObj;

    if (OSF_OBJ_WORD(liveObj, 0x0) == 2 &&
        (OSF_OBJ_WORD(liveObj, 0x3f0) || OSF_OBJ_WORD(liveObj, 0x330) ||
         OSF_OBJ_NAME(liveObj))) {
        if (s_osf_live_count == s_osf_live_capacity) {
            s_osf_live_capacity += 10;
            s_osf_live_table = (GameOsfEntry*)realloc(
                s_osf_live_table, s_osf_live_capacity * sizeof(GameOsfEntry));
        }
        recallObj = &s_osf_live_table[s_osf_live_count++];
        recallObj->id = OSF_OBJ_WORD(liveObj, 0x3f0);
        recallObj->ref = OSF_OBJ_WORD(liveObj, 0x330);
        memset(recallObj->name, 0, sizeof(recallObj->name));
        if (OSF_OBJ_NAME(liveObj))
            strncpy(recallObj->name, OSF_OBJ_NAME(liveObj), 11);
    }
}

/* ObjectRecall_RecallMiniFigure: a minifigure takes the last live entry's
 * ability flags, level and custom name. */
// FUNCTION: LEGORR 0x00459240
extern "C" int object_recall_recall_minifigure(void* liveObj)
{
    GameOsfEntry* recallObj;

    if (OSF_OBJ_WORD(liveObj, 0x0) == 2 && s_osf_live_count) {
        recallObj = &s_osf_live_table[--s_osf_live_count];
        OSF_OBJ_WORD(liveObj, 0x3f0) = recallObj->id;
        entity_building_index_set(liveObj, recallObj->ref);
        legoobject_rename(liveObj, recallObj->name);
        return 1;
    }
    return 0;
}

/* Copy the live recall table to the shadow table and set the shadow's
 * dirty flag. */
// FUNCTION: LEGORR 0x004592e0
extern "C" void object_recall_snapshot(void)
{
    if (s_osf_live_count && s_osf_live_table) {
        if (s_osf_shadow_table)
            free(s_osf_shadow_table);
        s_osf_shadow_table = (GameOsfEntry*)malloc(
            s_osf_live_count * sizeof(GameOsfEntry));
        memcpy(s_osf_shadow_table, s_osf_live_table,
               s_osf_live_count * sizeof(GameOsfEntry));
        s_osf_shadow_count = s_osf_live_count;
        s_osf_shadow_dirty = 1;
    }
}

/* Reset the live recall table and rebuild it from the shadow table. */
// FUNCTION: LEGORR 0x00459360
extern "C" int object_recall_restore(void)
{
    osf_read_state_reset();
    if (s_osf_shadow_table && s_osf_shadow_count) {
        s_osf_live_table = (GameOsfEntry*)malloc(
            s_osf_shadow_count * sizeof(GameOsfEntry));
        memcpy(s_osf_live_table, s_osf_shadow_table,
               s_osf_shadow_count * sizeof(GameOsfEntry));
        s_osf_live_count = s_osf_shadow_count;
        s_osf_live_capacity = s_osf_shadow_count;
        return 1;
    }
    return 0;
}

/* ObjectRecall_IsLoaded. */
// FUNCTION: LEGORR 0x004593d0
extern "C" int object_recall_is_loaded(void)
{
    return s_osf_shadow_dirty;
}

/* ObjectRecall_Save: write the live recall table as `fname` ('RRSO', the
 * count, then 0x14-byte entries), copy it to the shadow table and clear
 * the shadow's dirty flag. */
// FUNCTION: LEGORR 0x004593e0
extern "C" int object_recall_save(const char* fname)
{
    GameFileHandle* file;
    uint32_t header[2];

    if ((file = game_file_open_dispatch(fname, "wb")) != NULL) {
        header[0] = 0x534f5252; /* 'RRSO' */
        header[1] = s_osf_live_count;
        game_file_write(header, 4, 2, file);
        game_file_write(s_osf_live_table, 0x14, s_osf_live_count, file);
        game_file_close(file);
        object_recall_snapshot();
        s_osf_shadow_dirty = 0;
        return 1;
    }
    return 0;
}

/* ObjectRecall_LoadRROSFile: reset the live table, read the 'RRSO' header
 * and, when the file's length matches the count, the entries, then copy
 * them to the shadow table and clear the dirty flag. */
// FUNCTION: LEGORR 0x00459470
extern "C" int object_recall_load_rrosfile(const char* fname)
{
    GameFileHandle* file;
    uint32_t header[2];
    int length;
    int result = 0;

    osf_read_state_reset();
    if ((file = game_file_open_dispatch(fname, "rb")) != NULL) {
        length = file_length(file) - 8;
        wad_file_read(header, 4, 2, file);
        if (header[0] == 0x534f5252 && length == (int)(header[1] * 0x14)) {
            s_osf_live_count = header[1];
            s_osf_live_capacity = header[1];
            s_osf_live_table = (GameOsfEntry*)malloc(header[1] * 0x14);
            wad_file_read(s_osf_live_table, 0x14, s_osf_live_count, file);
            object_recall_snapshot();
            s_osf_shadow_dirty = 0;
        }
        game_file_close(file);
        result = 1;
    }
    return result;
}
#endif
