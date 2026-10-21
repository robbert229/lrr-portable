// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — save/load + checkpoints (finding 08).
 *
 * Persistence is *not* an entity dump. A slot stores one fixed-size
 * 12,688-byte checkpoint record per level plus a sidecar of completed
 * objectives:
 *
 *   Saves\<slot>.sav  — 0xB0-byte header + record_count × 0x3190 records
 *   Saves\<slot>.osf  — `'RO'` magic, u32 count, count × 20-byte entries
 *
 * Records: u32 flags (`|1` dirty, `|2` empty-marked) + 0x318c-byte blob
 * (the per-frame checkpoint snapshot: reward enable/modifier + the 10-task
 * mission table, built by checkpoint_blob_build). Slots 0–5, slot 5 is the
 * default; record_count defaults to 5 (the corpus's bootstrap value).
 *
 * Reconstructed from SavReadSlot @ 0x417a90, SavWriteSlot @ 0x417bd0,
 * SavMarkRecord/SavWriteRecord/SavCommit (0x417eb0/40/bd0fc0), the
 * OsfAdd/Snapshot/Write triplet (0x459190/e0/3e0), CheckpointBlobBuild
 * @ 0x45fda0 and CheckpointStep @ 0x462270.
 *
 * The corpus's active-slot context, OSF table and task block are globals
 * (DAT_005574d0, DAT_004ffe50/54, DAT_00552990); here they are carried by
 * GameContext so tests run isolated.
 */
#ifndef LEGORRR_GAME_SAVEGAME_HPP
#define LEGORRR_GAME_SAVEGAME_HPP

#include <cstdint>

#include "game.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#define SAVE_SLOTS       6
#define SAVE_DEFAULT_SLOT 5
#define SAVE_HEADER_SIZE 0xB0
#define SAVE_RECORD_SIZE 0x3190
#define SAVE_BLOB_SIZE   0x318c

/* Default record count written for a fresh slot (corpus bootstrap: count 5,
 * active slot 5). */
#define SAVE_DEFAULT_RECORDS 5

/* OSF entry layout (finding 08): id, ref, 11-char name, 3×u16 pad. */

/* --- slot context --------------------------------------------------- */
void save_init(GameContext* ctx);

int  save_slot_get(const GameContext* ctx);        /* SavGetSlot */
void save_slot_set(GameContext* ctx, int slot);    /* SavSetSlot */

/* Open `Saves\<slot>.sav` into the active-slot context. `create_if_missing`
 * mirrors SavReadSlot's `param_3`: 0 writes a fresh empty slot when the file
 * is absent or inconsistent. Returns 1 on success. */
int save_read_slot(GameContext* ctx, int slot, int create_if_missing);

/* Write the active-slot context to `Saves\<slot>.sav` (+ `.osf`). */
int save_write_slot(GameContext* ctx, int slot);

/* SavCommit: write the current active slot to disk. */
void save_commit(GameContext* ctx);

/* Dirty-mark record `index` (|1, plus |2 for an empty-name record). */
void save_mark_record(GameContext* ctx, int index);

/* Copy a 0x318c blob into record `index` (SavWriteRecord). */
int  save_write_record(GameContext* ctx, int index, const uint8_t* blob);

/* Return record `index`'s flags word pointer (SavRecordPtr + flags). */
uint32_t* save_record_flags(GameContext* ctx, int index);

/* --- checkpoint blob ------------------------------------------------ */
/* Build the 12,684-byte snapshot from the task table + reward group
 * (CheckpointBlobBuild). Replaces ctx->checkpoint_blob; 12,688 B live on
 * the ctx so pre-tick capture / post-tick commit stay frame-consistent. */
void checkpoint_blob_build(GameContext* ctx);
void checkpoint_blob_free(GameContext* ctx);

/* CheckpointStep — the reward-screen "save" flow: map the current level to
 * its record, mark it when the level's objective-result word is "achieved"
 * (+0x27c == 1), copy the blob in, bootstrap slot 5 when none was active,
 * then commit + reload slot metadata. */
void checkpoint_step(GameContext* ctx);
void checkpoint_refresh(GameContext* ctx);

/* --- completed objective table (`.osf`) ----------------------------- */
void osf_add(GameContext* ctx, int32_t id, int32_t ref, const char* name);
void osf_snapshot(GameContext* ctx);      /* clone live -> shadow (kept inline) */
int  osf_write(GameContext* ctx, const char* path); /* write + mark clean */

/* Write one `.osf` table to `path` (loader-side helper). */
int  osf_write_path(GameContext* ctx, int slot);

/* Builder for `Saves\<slot>.<ext>` path (native vs winapi separators). */
void save_slot_path(GameContext* ctx, char* out, size_t out_sz, int slot,
                    const char* ext);

/* Current-level record index used by checkpoint_step (ListFindName stand-in
 * until the level tree is walked from config). */
void save_set_current_record(GameContext* ctx, int index);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_SAVEGAME_HPP */
