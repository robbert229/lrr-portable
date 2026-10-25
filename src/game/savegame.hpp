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

int  save_slot_get(const GameContext* ctx);  /* SavGetSlot FUN_00417e90 */
void save_slot_set(GameContext* ctx, int slot);  /* SavSetSlot FUN_00417ea0 */

/* SaveSlotSelectionClear (FUN_00415cf0, finding 408): drops the active
 * slot selection back to the corpus's closed sentinel (-1). The corpus
 * writes DAT_005574d0 blindly; NULL answers nothing (documented). */
void save_slot_selection_clear(GameContext* ctx);

/* SavSlotCtx (FUN_00417e50): the corpus hands back the slot's 0xB8 context
 * block from the DAT_00557080 array (bounds 0..5). The port models the
 * context block in GameContext, so the corpus's bounds test is reproduced
 * and the block is answered for valid slots (deviation: the port's one
 * active-slot context stands in for the corpus's six). */
void* sav_slot_ctx(GameContext* ctx, int slot);

/* Open `Saves\<slot>.sav` into the active-slot context. `create_if_missing`
 * mirrors SavReadSlot's `param_3`: 0 writes a fresh empty slot when the file
 * is absent or inconsistent. Returns 1 on success. */
int save_read_slot(GameContext* ctx, int slot, int create_if_missing);

/* Write the active-slot context to `Saves\<slot>.sav` (+ `.osf`). */
int save_write_slot(GameContext* ctx, int slot);

/* SavCommit: write the current active slot to disk. */
void save_commit(GameContext* ctx);

/* Write the `.dat` slot thumbnail: the framebuffer as an uncompressed
 * 24-bit BMP at `Saves\<slot>.dat`. The corpus dumps it on save confirm
 * (FUN_00413b80 -> FUN_0047e440 -> FUN_0047c8f0) and re-reads it when the
 * save menu opens (FUN_00413a60). `fb` is the 32-bit surface in surface
 * byte order (B,G,R,A), `pitch` in bytes. */
int save_write_thumbnail(GameContext* ctx, int slot, const void* fb,
                         int pitch, int w, int h);

/* Dirty-mark record `index` (|1, plus |2 for an empty-name record). */
void save_mark_record(GameContext* ctx, int index);

/* SavWriteRecord (FUN_00417f40): copies a 0x318c blob into record
 * `index`'s window (the corpus's record base + index * 0x3190 + 4). The
 * corpus gate is SavBuildCtx's slot context + the record-count word;
 * the port grows the record table to fit (documented deviation).
 * Answers 1 when the blob lands. */
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

/* Load-side `.osf` restore (OsfRead @ 0x459470, entered from LevelLoad @
 * 0x429890): mirror of the corpus's FUN_00459470 — clear the live table
 * first (OsfClear @ 0x4592a0), open `path` "rb", validate the `'RO'`
 * magic (`0x534f5252`) and the exact `file_size == 8 + count*0x14` shape,
 * then repopulate the live table and snapshot it into the shadow
 * (OsfSnapshot @ 0x4592e0) with the dirty flag cleared. A missing or
 * corrupt sidecar degrades to an empty table — the corpus's failure mode,
 * no crash. The return mirrors FUN_00459470's `uVar3`: 1 whenever the
 * file opened (valid or not), 0 when it was absent; the *installed state*
 * (osf_count == 0 on a bad sidecar) is what distinguishes them. */
int  osf_read(GameContext* ctx, const char* path);

/* Restore `Saves\<slot>.osf` (the slot-level wrapper). The corpus calls it
 * from `LevelLoad` only when the in-memory table is clean (dirty == 0) and
 * an active slot is set — the caller (frameloop slot-load) reproduces that
 * gate. */
int  osf_read_path(GameContext* ctx, int slot);

/* Builder for `Saves\<slot>.<ext>` path (native vs winapi separators). */
void save_slot_path(GameContext* ctx, char* out, size_t out_sz, int slot,
                    const char* ext);

/* Current-level record index used by checkpoint_step (ListFindName stand-in
 * until the level tree is walked from config). */
void save_set_current_record(GameContext* ctx, int index);

/* --- slot menu support (finding 08 §"Menu layer" ×
 *       SaveMenuBuild @ 0x415a10) ---------------------------------------- */

/* Inspect one slot without changing the live context's active slot:
 * returns 0 when `Saves\<slot>.sav` does not exist (the slot is absent);
 * otherwise scans its records and writes:
 *   *out_filled    — number of records with a marked/non-empty blob,
 *   *out_completed — number of records whose flags word has |1 (objective
 *                    achieved at checkpoint, SlotSummary @ 0x4152c0),
 *   *out_resume    — the highest such record index (the level to resume),
 *                    or -1 when the slot is empty.
 * The port maps record index -> campaign level (StartLevel + NextLevel). */
int save_slot_state(GameContext* ctx, int slot, int* out_filled,
                    int* out_completed, int* out_resume);

/* Restore a saved checkpoint (the inverse of checkpoint_blob_build): copy
 * the blob's task table back into ctx->tasks and its Reward/Enable+Modifier
 * into ctx->reward. Live level state stays fresh — the corpus's model is a
 * per-level checkpoint, not an entity dump (finding 08). */
void checkpoint_blob_apply(GameContext* ctx, const uint8_t* blob);

#ifdef __cplusplus
}
#endif

/* --- the save-side DAT accessor band (2026-10-24; findings 08/235) ---------
 * OsfReadDirtyFlagGet (FUN_004593d0): the OSF dirty flag the save module
 * already models (DAT_004ffe64); NULL answers 0 (deviation: blind read).
 * CheckpointBlobGet (FUN_004601f0): the checkpoint blob pointer (corpus
 * DAT_00552994), modeled on GameContext; NULL ctx answers NULL. */
int   osf_read_dirty_flag_get(const GameContext* ctx);
void* checkpoint_blob_get(const GameContext* ctx);

/* SaveStateEventContextSet (FUN_00418110, finding 408): the corpus stores
 * the word FrontEndStateChange follows; its corpus feeder is the unported
 * front-end state-change path, so it lands as zero-init storage with a
 * port-side peek. */
void     save_state_event_context_set(uint32_t value);
uint32_t save_state_event_context_peek(void);

/* SavBuildCtx (FUN_00417e70, finding 408): the corpus's active-slot
 * context probe — the slot accessor's block when a slot is open, NULL
 * when the selection is closed. The port's save_slot_get/sav_slot_ctx
 * models both halves, so this composes them. */
void* sav_build_ctx(GameContext* ctx);

/* SavRecordPtr (FUN_00417f90): the record address for `index` —
 * base + index * 0x3190 + 4 once the slot context is open and the index
 * is under the count. NULL when the corpus's guard answers 0. */
void* sav_record_ptr(GameContext* ctx, int index);

/* SelectedLevelStartObjectUpdate (FUN_00460200): when the level block is
 * bound, reads the checkpoint blob through the ported getter (the corpus
 * discards the value; the apparent argument is calling-convention residue —
 * the getter answers the blob word itself). */
void selected_level_start_object_update(GameContext* ctx); /* FUN_00460200 */
#endif /* LEGORRR_GAME_SAVEGAME_HPP */
