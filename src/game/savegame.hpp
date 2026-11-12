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
#include "gods98/string.hpp"

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
/* Bind runtime saves to the configured game data root after resource startup. */
int save_bind_resource_directory(GameContext* ctx);
/* Bind/query the one active GameContext that stands in for the corpus's
 * process-global save-slot context. */
void save_context_bind(GameContext* ctx);
GameContext* save_current_context_get(void);

int  save_slot_get(const GameContext* ctx);  /* typed SavGetSlot adapter */
void save_slot_set(GameContext* ctx, int slot);  /* typed SavSetSlot adapter */
extern "C" int  save_slot_get_target(void);              /* SavGetSlot @ 0x417e90 */
extern "C" void save_slot_set_target(int slot);          /* SavSetSlot @ 0x417ea0 */

/* SaveSlotSelectionClear (FUN_00415cf0, finding 408): drops the active
 * slot selection back to the corpus's closed sentinel (-1). The corpus
 * writes DAT_005574d0 blindly; NULL answers nothing (documented). */
void save_slot_selection_clear(GameContext* ctx);
extern "C" void save_slot_selection_clear_target(void);  /* @ 0x415cf0 */

/* Typed SavSlotCtx adapter and its one-argument target ABI entry. */
void* sav_slot_ctx(GameContext* ctx, int slot);
extern "C" void* sav_slot_ctx_target(int slot);          /* SavSlotCtx @ 0x417e50 */

/* Open `Saves\<slot>.sav` into the active-slot context. `create_if_missing`
 * mirrors SavReadSlot's `param_3`: 0 writes a fresh empty slot when the file
 * is absent or inconsistent. Returns 1 on success. */
int save_read_slot(GameContext* ctx, int slot, int create_if_missing);
/* SavReadSlot's retail argument order: (slot, slot-context, mode). A supplied
 * per-slot context receives the retail header while record allocations live
 * in a host-width sidecar; NULL retains the typed active-GameContext path. */
extern "C" int save_read_slot_target(uint32_t slot, void* slot_context, int mode);

/* Write the active-slot context to `Saves\<slot>.sav` (+ `.osf`). */
int save_write_slot(GameContext* ctx, int slot);

/* SavRefreshSlots (FUN_00417df0): walk the six slots so every slot file
 * exists. With the engine feature word's bit 0x400 raised, each slot is
 * written fresh before being read back; otherwise each is read (and
 * re-read once when the first attempt fails). Six retail headers are
 * represented by 0xb8-byte blocks and host-width record sidecars. */
void sav_refresh_slots(GameContext* ctx); /* FUN_00417df0 */
void sav_refresh_slots_target(void);

/* SavCommit: write the current active slot to disk. */
extern "C" void save_commit(GameContext* ctx);
void save_return_diagnostic_log_set(void* log);

/* Write the `.dat` slot thumbnail: the framebuffer as an uncompressed
 * 24-bit BMP at `Saves\<slot>.dat`. The corpus dumps it on save confirm
 * (FUN_00413b80 -> FUN_0047e440 -> FUN_0047c8f0) and re-reads it when the
 * save menu opens (FUN_00413a60). `fb` is the 32-bit surface in surface
 * byte order (B,G,R,A), `pitch` in bytes. */
int save_write_thumbnail(GameContext* ctx, int slot, const void* fb,
                         int pitch, int w, int h);

/* Dirty-mark record `index` (|1, plus |2 for an empty-name record). */
extern "C" void save_mark_record(GameContext* ctx, int index);

/* Typed owner adapter: grow the port's record table as needed, bind this
 * GameContext as the current save owner, then enter the retail ABI body. */
int  save_write_record(GameContext* ctx, int index, const uint8_t* blob);
/* SavWriteRecord (FUN_00417f40): the retail two-argument entry reads the
 * active save context, gates on the record count, and copies 0x318c bytes at
 * record base + index * 0x3190 + 4. */
extern "C" int  save_write_record_target(int index, const uint8_t* blob);

/* Return record `index`'s flags word pointer (SavRecordPtr + flags). */
uint32_t* save_record_flags(GameContext* ctx, int index);

/* --- checkpoint blob ------------------------------------------------ */
/* Build the 0x318c-byte snapshot from the task table + reward group
 * (CheckpointBlobBuild). Replaces ctx->checkpoint_blob; task Importance and
 * Quota are refreshed in the copied records and +0x1ce8 points to that
 * snapshot's task table. */
void checkpoint_blob_build(GameContext* ctx);
void checkpoint_blob_free(GameContext* ctx);

/* CheckpointStep — the reward-screen "save" flow: map the current level to
 * its record, mark it when the level's objective-result word is "achieved"
 * (+0x27c == 1), copy the blob in, bootstrap slot 5 when none was active,
 * then commit + reload slot metadata. */
void checkpoint_step(GameContext* ctx);
void checkpoint_step_target(void); /* CheckpointStep @ 0x00462270 */
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
void save_directory_ensure_exists(const char* path); /* FUN_0047f6a0 */

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
int save_slot_preview(GameContext* ctx, int slot, const uint8_t** records, int* count);
int save_slot_records_view(int slot, const uint8_t** records, int* count);
int save_slot_state(GameContext* ctx, int slot, int* out_filled,
                    int* out_completed, int* out_resume);

/* Restore a saved checkpoint (the inverse of checkpoint_blob_build): retain
 * the entire blob in ctx, repair its task-table self pointer, then restore
 * the task table and Reward/Enable+Modifier into live state. Live entity
 * state stays fresh — the corpus's model is a per-level checkpoint, not an
 * entity dump (finding 08). */
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
extern "C" void     save_state_event_context_set(uint32_t value);
uint32_t save_state_event_context_peek(void);

/* Typed SavBuildCtx adapter and the no-argument target ABI entry. */
void* sav_build_ctx(GameContext* ctx);
extern "C" void* sav_build_ctx_target(void);             /* SavBuildCtx @ 0x417e70 */

/* SavRecordPtr (FUN_00417f90): the record address for `index` —
 * base + index * 0x3190 + 4 once the slot context is open and the index
 * is under the count. NULL when the corpus's guard answers 0. */
void* sav_record_ptr(GameContext* ctx, int index);
extern "C" void* sav_record_ptr_target(int index);       /* SavRecordPtr @ 0x417f90 */

/* SelectedLevelStartObjectUpdate (FUN_00460200): when the level block is
 * bound, reads the checkpoint blob through the ported getter (the corpus
 * discards the value; the apparent argument is calling-convention residue —
 * the getter answers the blob word itself). */
void selected_level_start_object_update(GameContext* ctx); /* FUN_00460200 */

/* SelectedLevelStart (FUN_00460220) publishes the active type-4 name list
 * and Pilot grid-record count after the current level/checkpoint gate. */
typedef struct EntityType4IndexNameList {
    /* Retail DAT_005529c4 is exactly 100 index dwords followed by the
     * count and terrain-grid count. Keep those words first for the checkpoint
     * blob copy; names are the port-side lookup aid appended after them. */
    uint32_t indices[100];
    uint32_t count;
    uint32_t grid_count;
    const char* names[100];
} EntityType4IndexNameList;
void start_level_selected(GameContext* ctx); /* FUN_00460220 */
const EntityType4IndexNameList* start_level_entity_type4_list_get(void);
uint32_t start_level_pilot_grid_count_get(void);

/* --- generic list window band (2026-10-25) --------------------------------
 * The 0x416exx/0x4170xx band works over caller-supplied list-header
 * windows: header slots {count, entry-key pointer array, value array A,
 * value array B} (the corpus header's own +8 slot is untouched by this
 * band). Two key views ride the same window: the fold find reads entry
 * slots as byte-string names through the corpus's case-insensitive
 * compare, and the four wide-key halves compare entry slots as
 * little-endian 16-bit units — corpus quirk, reproduced: a zero byte in
 * EITHER position of an otherwise-matching unit terminates the key
 * (`test cl,cl / je` on the low byte at 0x416ef3 and on the high byte
 * at 0x416f05), so an ASCII key whose unit carries a zero high byte
 * matches as a prefix. The add/has halves ride value array B; the
 * set/get halves ride value array A. Deviations: the port guards the
 * window and key pointers (the corpus dereferences them blind); entry,
 * value, and key strides are blind like the corpus; entries, values,
 * and the key slots ride widened slots (the corpus strides dword
 * fields). */
typedef struct SavListWindow {
    int32_t  count;      /* corpus header + 0 */
    void**   entries;    /* corpus + 4 (widened) */
    void**   values_a;   /* corpus + 12 (widened): set/get values */
    void**   values_b;   /* corpus + 16 (widened): add/has values */
} SavListWindow;

/* Fold-find the name slot; answers the entry index or -1. */
int list_find_name(const SavListWindow* win, const char* name);
                                                /* FUN_00416e20 */

/* Wide-key scan halves: add stores the value into array B on a match;
 * has answers the paired array-B value (NULL on a miss); set/get ride
 * array A. */
void  list_add_name(SavListWindow* win, const void* wide_key, void* value);
                                                /* FUN_00416ed0 */
void* list_has_item(const SavListWindow* win, const void* wide_key);
                                                /* FUN_00416f40 */
void  list_set_node(SavListWindow* win, const void* wide_key, void* value);
                                                /* FUN_00416fb0 */
void* list_get_node(const SavListWindow* win, const void* wide_key);
                                                /* FUN_00417020 */

/* LevelNodeGet (FUN_00417090): get or recursively build the LevelLinks node
 * for a name in the level registry. `cfg` is the Lego configuration backing
 * the registry; this explicit argument replaces the corpus global. */
typedef struct BlockRec BlockFile;
struct SavTreeNode;
SavTreeNode* level_node_get(SavListWindow* win, BlockFile* cfg,
                            const char* level_name);  /* FUN_00417090 */

/* SavTreeWalk (FUN_004171c0): visit-each-once tree recursion over
 * node windows — children pointer slot, child count, and the walk flag;
 * the port's node is the widened-slot shape (the corpus strides its
 * dword fields). The callback is the corpus's plain indirect call.
 * Deviation: the port guards the node and child-array pointers; the
 * corpus strides the flag/store edges blindly. Answers 1 when the
 * callback or any subtree reports it. */
typedef struct SavTreeNode {
    uint32_t window;     /* corpus + 0x0: stride-selector word (walk2 read) */
    void*    children;   /* corpus + 4 (widened): child pointer array */
    uint32_t count;      /* corpus + 8 */
    uint32_t reserved;   /* corpus + 0xc: preserved node word */
    uint32_t visited;    /* corpus + 0x10: walk flag */
} SavTreeNode;

typedef int (*sav_tree_visit)(void* node, void* user);

int sav_tree_walk(void* node, sav_tree_visit visit, void* user);
                                                /* FUN_004171c0 */

/* --- the sav-tree state visitor (2026-10-25) --------------------------------
 * SavTreeWalk2 (FUN_004172f0): the stateful sibling of SavTreeWalk
 * (FUN_004171c0). It enters every unvisited node once (the +0x10 walk
 * flag, as the plain walk), computes the "need" word from the shared
 * state: the flag-test lane when the window base is null or the
 * extended-mode gate (app mode > 1, or the feature word's bit 0x20000)
 * raises — the gate also zeroes the flag on entry — otherwise the bit-0
 * read at `node.window * 0x3190 + window_base`. Then it touches the
 * block cursor: clearing bit 2 of the block's +0x0 word only when need
 * and the incoming carry and the block's +0x1c word all read zero,
 * setting it otherwise, and ALWAYS advances the cursor (the corpus
 * advances through a null block pointer too). Children recurse with
 * `need` as the carry.
 * Deviations: the corpus strides the block array and the stride read
 * blind; the port guards the block slot/pointer and reads the window
 * through memcpy lanes. The two corpus tree-root globals are unmodeled,
 * so nodes ride as arguments (the walk-model convention); app mode
 * threads through EngineContext (the corpus strides its global) and the
 * feature word rides the crt.* half-model peek. */
typedef struct SavTree2State {
    void*    window_base; /* corpus + 0x0: stride-0x3190 window base */
    uint32_t window;      /* corpus + 0x4: stored word (the walk never reads it) */
    void**   blocks;      /* corpus + 0x8: block-array pointer slot */
    uint32_t index;       /* corpus + 0xc: visit-order block cursor */
    uint32_t flag;        /* corpus + 0x10: needs-visit flag */
} SavTree2State;

struct EngineContext;

void sav_tree_walk2(void* node, SavTree2State* state, int carry,
                    const EngineContext* eng);  /* FUN_004172f0 */

/* SavTreeLoad (FUN_004173e0): stack-init the state's five dwords the way
 * the corpus builds them (window base, stored word, block slot, zero
 * cursor, flag), walk with carry 1, then SavClearVisited (FUN_00416e70)
 * over the installed visited bands. */
extern "C" void sav_tree_load(void* node, void* window_base, void** blocks,
                   uint32_t window, uint32_t flag,
                   const EngineContext* eng);  /* FUN_004173e0 */

/* --- the sav-tree indexed visit lookups (2026-10-26) -----------------------
 * RegistryValueLookupWithLock (FUN_00417280) walks the tree (FUN_004171c0)
 * with the thunk recovered from the corpus's code section: the callback
 * carries (want, seen) through the walk's user block and answers 1 at the
 * want-th visited node — recording that node's pointer and its index (the
 * write lands the same value both dwords already hold) — or bumping seen
 * and returning 0. Either way the tail runs the installed visited-band
 * clear (FUN_00416e70); the tree's own visit flags stay raised (the band
 * clears the records installed in it, tree nodes included only as that
 * band's records). RegistryValueIndexLookupOrNotFound (FUN_004172d0) reads
 * the found node's leading dword back, or -1.
 * Deviation: the corpus strides the user block's words blindly; the port
 * models the state and guards the found-node read. */
void* registry_value_lookup(void* root, uint32_t index);
                                                /* FUN_00417280 */
uint32_t registry_value_index_lookup_or_not_found(void* root, uint32_t index);
                                                /* FUN_004172d0 */

/* --- the sav-tree menu refresh (2026-10-26) --------------------------------
 * SaveMenuRefresh (FUN_004150b0): set the open slot (FUN_00417ea0), probe
 * the slot context (FUN_00417e70), then run the two stack-state loads
 * (FUN_004173e0). The in-band walk enters with flag 1 and no stride
 * window when the slot is unopened; otherwise flag 0 and the stride
 * window taken from the context's +0x34 word. The out-of-game walk
 * always enters with window base 0 and flag 0. Each walk's block-slot
 * rides the corresponding slot-context chain word.
 * Deviations: the corpus's chain dereference, the context's +0x34 word,
 * and the two tree roots have no port-side loader, so they ride
 * own-storage install/peek hooks (the visited-band precedent); the
 * corpus's first param is unused, the never-read window-slot addresses
 * land as zero, and the walks thread no EngineContext (the gate reads
 * the port's zeroed app-mode default). */
void save_menu_refresh(GameContext* ctx, int slot);  /* FUN_004150b0 */
void save_menu_refresh_target(uint32_t unused, int slot);
void show_level_overlay_for_current_selection(GameContext* ctx); /* FUN_00418000 */
extern "C" void show_level_overlay_target(void);

/* Install/peek hooks for the corpus reference state the refresh walks.
 * slot is the corpus's port-owned block-slot address (the chain word's
 * role); base is the slot-context stride-window base (+0x34 word role). */
void  sav_menu_window_install(int which, void** slot);  /* port hook */
void** sav_menu_window_peek(int which);
void  sav_slot_stride_window_install(void* base);       /* port hook */
void* sav_slot_stride_window_peek(void);
void  sav_tree_root_install(int which, void* node);     /* port hook */
void* sav_tree_root_peek(int which);
/* Adopt the LevelNodeGet graph and its node allocations. `nodes` contains
 * the unique nodes allocated while building the registry; replacing or
 * clearing the root releases each node and its child array. */
int sav_tree_root_install_owned(int which, void* node,
                                void* const* nodes, uint32_t node_count);
void sav_tree_roots_clear(void);
/* The visited bands are owned by the corresponding tree root and grow to
 * the runtime entry count. Their records are widened host pointers; the
 * retail globals contain 32-bit pointer slots. */
void sav_tree_visited_entry_set(int which, uint32_t index, void* record,
                                uint32_t word);
void sav_tree_visited_entry_peek(int which, uint32_t index, void** record,
                                 uint32_t* word);

/* --- the checkpoint progress fold band (2026-10-25) ------------------------
 * CheckpointBlobTaskProgressRecalculate (FUN_00460950): the blob's
 * progress fold over the stride-0x210 task-window bands: zero the three
 * accumulator words, fold each pair as percent = 100 / den * num — a
 * zero denominator answers 100.0, and the clamp lands AFTER the store,
 * as the corpus stores-clamps-stores (pinned quirk) — then walk the
 * count-up loops: every round adds 100.0 to the +0x2fac sum and the
 * last folded percent to the +0x2fb0 carry. The 0x2d90-gated loop keeps
 * both accumulators in registers and writes them back only when it ends
 * (pinned corpus shape), the 0x2550 loop adds the RAW window (+0x2564),
 * then the carry clamps down to the sum and below zero to 0.0, and the
 * final percent fold (100 / sum * carry) plus the float-converted +0x1c0
 * word land at +0x2fb4, clamped to 100.
 * Deviations: the corpus threads the global current level and the blob
 * word through its two getters; the port threads GameContext (the level
 * guard, then the blob pointer) and runs the fold math in single
 * precision. A missing level or blob answers 0; success answers 1.
 * Corpus offsets kept: windows +0x1b04..+0x2984 (folds), guards
 * +0x1d10..+0x2d90 (loops), accumulators +0x2fac/+0x2fb0/+0x2fb4. */
/* Target ABI reads the current save/level context without arguments. */
int checkpoint_blob_task_progress_recalculate_target(void);
int checkpoint_blob_task_progress_recalculate(GameContext* ctx);
                                                /* explicit-context adapter */

namespace unported_ledger_stubs {
void campaign_chain_snapshot_create_target(void* snapshot);
}

#endif /* LEGORRR_GAME_SAVEGAME_HPP */
