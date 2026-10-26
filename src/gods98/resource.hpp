// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — resource loader scaffold (WAD mount + dispatch).
 *
 * Reconstruction of the file/resource layer the corpus calls FUN_0047f6e0 /
 * FUN_0047f130 / FUN_0048bda0 / FUN_0048bf70 / FUN_0048be40 (finding 01).
 * Handles both branches documented there:
 *
 *   WAD branch — mount `<dir>/<prefix>%i.wad` (i = 0..9), the prefix being
 *       the exe basename minus ".EXE" (the find-20 correction of finding 02):
 *       "LegoRR" -> LegoRR0.wad / LegoRR1.wad. Parse the WWAD header + TOC,
 *       and open entries by case-insensitive name. Later mounts (higher i)
 *       shadow earlier ones when resolving unqualified names.
 *   STD branch — a plain loose file opened from the working directory (the
 *       corpus's `Data\...` path; the drive-letter CD fallback is out of
 *       scope until a mount story exists).
 *
 * Entry payload expansion (finding 01): recorded for entries whose flag word
 * has bit 1 set — a small header (4-byte magic, type byte at [3], byte-swapped
 * length at [4], payload from +0x12). Shipped wads never set the bit
 * (flags == 1 everywhere, stored == original), so types 1/2 (FUN_0049c8e0 /
 * FUN_0049ca60) remain dormant and are reported as not implemented.
 *
 * Discipline (ADR 0003/0004): plain C, stdio only (portable across the
 * native/winapi backends), no DirectX/WinAPI headers below the seam.
 */
#ifndef LEGORRR_RESOURCE_HPP
#define LEGORRR_RESOURCE_HPP

#include <cstdint>
#include <cstddef>

/* --- branch selectors (corpus dispatcher: 0 = WAD, 1 = STD) -------------- */
#define RESOURCE_WAD_BRANCH 0
#define RESOURCE_STD_BRANCH 1

/* open modes */
#define RESOURCE_AUTO     0 /* STD file if present, else WAD lookup */
#define RESOURCE_WAD_ONLY 1 /* archive member only */
#define RESOURCE_STD_ONLY 2 /* loose file only */

/* --- CD/STD fallback state (finding 01, FUN_0047f660) --------------------
 * The corpus's STD branch is a loose file; when neither a relative file nor
 * a wad member answers, the game falls back to the CD drive using the
 * "%c:\%s\%s" builder (one-byte drive letter DAT_005343b4 + data dir).
 * These are inert for a modern disk install but required for fidelity of the
 * winapi reference build's behavior.
 */
void resource_set_cd_drive(char drive);       /* default 'A' (mirrors .rdata) */
void resource_set_cd_data_dir(const char* dir); /* default "Data" */
/* Override the drive-root of the "%c:\%s\%s" builder, e.g. to remap the CD
 * to a mounted path. Empty string (default) uses "<drive>:\". */
void resource_set_cd_root(const char* root);

/* Marker-file probes (finding 01): cd.key = CD present, delme.dat = data
 * installed (CDMissing / DataMissing dialogs in the original). 1 = present. */
int resource_cd_present(void);
int resource_data_present(void);

/* An opened resource. `data` is populated by resource_read() and is owned by
 * the handle (freed by resource_free). `mode` is the open() mode requested. */
typedef struct Resource {
    uint8_t* data;
    int32_t  size;
    int32_t  branch;   /* RESOURCE_WAD_BRANCH or RESOURCE_STD_BRANCH */
    int32_t  wad_idx;  /* mounted wad index, or -1 */
    int32_t  entry_idx;/* TOC entry index, or -1 */
    int32_t  mode;     /* as passed to resource_open */
    char*    name;     /* copy of the requested name */
} Resource;

/* Mount every `<dir>/<prefix>%i.wad` for i = 0..9 that exists and parses.
 * Returns the number of wads mounted (0 => none; error message -> stderr).
 * Only the first successful parse is required to be a valid WWAD. */
int resource_mount(const char* dir, const char* prefix);
/* Entry-derived default prefix used by the main frame loop (LegoRR by default). */
void resource_set_wad_prefix(const char* prefix);
const char* resource_get_wad_prefix(void);

/* Number of wads currently mounted. */
int resource_mounted_count(void);

/* Close and free every mounted wad (state reset, used between test runs). */
void resource_unmount_all(void);

/* Case-insensitive lookup of `name` inside one mounted wad (linear scan over
 * the archive-relative path table, ASCII fold only). Returns the TOC entry
 * index or -1. */
int resource_wad_lookup(int wad_idx, const char* name);

/* TOC introspection used by tests/validators. */
uint32_t resource_wad_entry_count(int wad_idx);
uint32_t resource_wad_entry_stored(int wad_idx, int entry_idx);

/* Archive-relative name of one TOC entry (or NULL). */
const char* resource_wad_entry_name(int wad_idx, int entry_idx);

/* Open a resource by name. mode selects the branch policy (see defines).
 * Returns a handle or NULL on failure. */
Resource* resource_open(const char* name, int mode);

/* Read the payload into handle->data (lazy; no-op if already read or NULL).
 * Returns the payload size, or -1 on failure (dormant codec / read error). */
int32_t resource_read(Resource* r);

/* Release the handle and its payload. */
void resource_free(Resource* r);

/* --- fixed-record slot resources (2026-10-24; findings 375/251) ------------
 * The corpus's fixed-record slot table (DAT_005756a0, stride 0x10: resource
 *   word, installed flag, two stream words) feeds the wad memory-stream
 *   band. FixedRecordSlotResourceRelease (FUN_0048bfc0): when the slot is
 *   installed, frees the slot's resource through the engine heap and clears
 *   the flag. FixedRecordSlotResourceGet (FUN_0048bff0): answers the slot's
 *   resource word when installed, zero otherwise.
 * Deviation: the table has no ported owner (its only corpus writer is the
 *   unported stream opener), so it lands as sixteen zero-init slots with
 *   install hooks; the corpus strides the index blindly. */
void fixed_record_slot_resource_release(int slot); /* FUN_0048bfc0 */
void* fixed_record_slot_resource_get(int slot);    /* FUN_0048bff0 */
void fixed_record_slot_install(int slot, void* resource); /* port hook */
int  fixed_record_slot_flag_peek(int slot);        /* port hook */

/* WadMemoryStreamBufferGet (FUN_0048c030): the corpus's stream-buffer
 * wrapper resolves the fixed-record slot half (FUN_0048bff0) and drops the
 * result; the observable effect is the slot reader's. */
void wad_memory_stream_buffer_get(int slot);       /* FUN_0048c030 */

/* --- wad payload band (2026-10-25; finding 01 codec story) ----------------
 * RNC-header decode dispatch, its expand wrapper, the type-1 Huffman table
 * clear, the file-layer handle pair, and the wad-entry stream-slot opener.
 * The codecs themselves stay dormant (no shipped wad sets the expanded
 * flag), documented in resource.cpp. */
#define WAD_DECODE_MAGIC_FAIL 0xffff /* corpus's not-RNC verdict */
#define WAD_DECODE_TYPE_FAIL  0xfffe /* corpus's unknown-type verdict */

/* Verify the "RNC" magic + type gate and copy/delegate the payload.
 * 0 raw copy, 1/2 the dormant codecs' path markers, verdicts otherwise. */
int wad_payload_decode_dispatch(const void* header, void* dest); /* FUN_0049c840 */

/* Probe (out_payload NULL) or materialize an expanded payload; answers the
 * decoded size, 0 on any failure. Buffer owned via engine_heap_free. */
long wad_expand_payload(const void* header, void** out_payload); /* FUN_0049c7c0 */

/* Zero 16-byte table entries: 0, word 0xffff, 0, word 0. */
void wad_huffman_table_clear(void* table, int count);            /* FUN_0049cf50 */

/* Kind 1: flagged handle record, no stream buffer. Kind 0: carries the
 * 12-byte stream buffer behind; releases the record if it fails. */
typedef struct GameFileHandle {
    int32_t flag;   /* corpus record dword at +0 */
    void*   stream; /* corpus record dword at +4 (widened, LP64 deviation) */
} GameFileHandle;
void* game_file_handle_allocate(int kind);                       /* FUN_0047fed0 */

/* The stream-slot opener's three corpus dwords, front to back. */
typedef struct WadStreamSlots {
    const Resource* handle; /* slot[0]: opened entry (NULL = missing) */
    const void*     stream; /* slot[1]: zeroed by the opener */
    uint32_t        word2;  /* slot[2]: zeroed by the opener */
} WadStreamSlots;
int wad_entry_read_into_stream_slot(WadStreamSlots* slots,
                                    const char* name);           /* FUN_0047fea0 */

/* --- resource-handle release band (2026-10-25) ---------------------------
 * The corpus's global handle set (the DAT_004ddc04 stride-8 band plus
 * twelve named globals) and the handle-list record both own no ported
 * writer, so the set lands as own storage behind install/peek hooks; the
 * release frees non-null slots in corpus order through the engine-heap
 * free seam and clears them (deviation: the corpus dangles). */
void resource_handle_set_release(void);                          /* FUN_0041a850 */
void  resource_handle_set_slot_install(int slot, void* res);     /* port hook */
void* resource_handle_set_slot_peek(int slot);                   /* port hook */

/* --- the key-group action-code band (2026-10-25) ---------------------------
 * The corpus writers of the DAT_004ddc04 stride-8 band land here: the
 * group register stages each interaction-action group as a heap array of
 * code dwords with the count riding behind it, and the map initializer
 * registers the fixed eighteen groups. The band word behind each pair is
 * shared with the entity query's id-pair reads and the dispatch's loop
 * bound, so one merged window model (the handle set's) owns it: the
 * install/peek pair below reads and writes that word. */
void resource_input_key_group_register(int group, int count,
                                       const uint32_t* values);
                                                /* FUN_0041a7d0 */
void interaction_action_code_map_initialize(void); /* FUN_0041a660 */

/* Port hooks over the shared band word (the corpus's +4 of each pair):
 * the count the register stages, read by the query/dispatch lanes. */
void     resource_handle_set_band_word_install(int unit, uint32_t value);
uint32_t resource_handle_set_band_word_peek(int unit);   /* port hook */

/* The corpus's handle-list record: the array word, an unread middle word,
 * and the count (LP64-safe widening). */
typedef struct ResourceHandleList {
    void**    handles; /* corpus dword at +0 */
    uintptr_t word1;   /* corpus dword at +4 (unread by the disposer) */
    int32_t   count;   /* corpus dword at +8 */
} ResourceHandleList;
void resource_handle_list_dispose(ResourceHandleList* list);     /* FUN_00410ae0 */

#endif /* LEGORRR_RESOURCE_HPP */
