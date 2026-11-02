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
 * length at [4], payload from +0x12). Type-1 and type-2 payload codecs are
 * implemented, though the shipped WAD set does not exercise them (its flags
 * are 1 and stored length equals original length).
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
typedef int (*ResourceDriveTypeQueryFn)(char drive_letter, void* context);
typedef int (*ResourcePathExistsFn)(const char* path, void* context);
void resource_cd_probe_hooks_set(ResourceDriveTypeQueryFn drive_type,
                                 ResourcePathExistsFn path_exists,
                                 void* context);
int resource_find_cd_key_drive(void); /* FUN_0047f4f0 */
const char* resource_path_resolve_against_base(const char* path); /* FUN_00480220 */

/* Marker-file probes (finding 01): cd.key = CD present, delme.dat = data
 * installed (CDMissing / DataMissing dialogs in the original). 1 = present. */
int resource_cd_present(void);
int resource_data_present(void);

/* Startup WAD mount services supplied by the host. The retail routine gets
 * localized strings through its registry-backed loader and shows modal
 * MessageBoxes. Process termination is optional: without an exit callback,
 * the startup entry records the status and returns to its caller. */
typedef int (*ResourceStartupTextLookupFn)(
    void* context, const char* registry_key, const char* text_key,
    char* text_out, size_t text_capacity);
typedef int (*ResourceStartupMessageBoxFn)(
    void* context, const char* text, const char* caption, unsigned style);
typedef void (*ResourceStartupExitFn)(void* context, int exit_code);
typedef struct ResourceStartupOps {
    void* context;
    ResourceStartupTextLookupFn text_lookup;
    ResourceStartupMessageBoxFn message_box;
    ResourceStartupExitFn exit_process;
} ResourceStartupOps;
void resource_startup_ops_set(const ResourceStartupOps* ops);
#define RESOURCE_STARTUP_OK 0
#define RESOURCE_STARTUP_PATH_ERROR 1
#define RESOURCE_STARTUP_MISSING_DATA 2
#define RESOURCE_STARTUP_CD_CANCELLED 3
#define RESOURCE_STARTUP_DEPENDENCY_MISSING 4
int resource_startup_result_get(void);

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
/* WadMountStartup's retail ABI: exe-derived prefix, insist-on-CD flag, and
 * registry key used by the localized CDMissing/DataMissing strings. The
 * explicit-root `resource_mount` remains the modern test/host entry. */
void resource_mount_startup_target(const char* prefix, int require_cd,
                                   const char* registry_key);
/* Calls the retail-shaped entry and returns RESOURCE_STARTUP_*. */
int resource_mount_startup(const char* prefix, int require_cd,
                           const char* registry_key);
/* Startup command-line policy supplied by the platform entry point. */
void resource_startup_require_cd_set(int require_cd);
int resource_startup_require_cd_get(void);
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

/* Search the mounted archives from newest to oldest, or one selected archive
 * when `wad_idx` is nonnegative. */
int wad_entry_find_across_mounted_archives(const char* name, int wad_idx);

/* TOC introspection used by tests/validators. */
uint32_t resource_wad_entry_count(int wad_idx);
uint32_t resource_wad_entry_stored(int wad_idx, int entry_idx);

/* Archive-relative name of one TOC entry (or NULL). */
const char* resource_wad_entry_name(int wad_idx, int entry_idx);

/* Open a resource by name. mode selects the branch policy (see defines).
 * Returns a handle or NULL on failure. */
Resource* resource_open(const char* name, int mode);
/* Independent seekable view over a loose file or WAD payload. Expanded
 * raw-copy entries are read directly; implemented WAD codecs 1/2 are
 * expanded into owned memory so the resulting logical resource is seekable. */
typedef struct ResourceStream ResourceStream;
ResourceStream* resource_stream_open(const char* name, int mode,
                                     uint32_t* size_out);
int resource_stream_read_at(ResourceStream* stream, uint32_t offset,
                            void* bytes, uint32_t length);
void resource_stream_close(ResourceStream* stream);
typedef struct MapOpenOps {
    void* context;
    int (*slot_allocate)(void* context, uint32_t size);
    void* (*slot_get)(void* context, uint32_t slot);
    void (*slot_release)(void* context, uint32_t slot);
    void (*progress)(void* context, const char* path, uint32_t size);
} MapOpenOps;
int map_open2(const char* path, const MapOpenOps* ops,
              int32_t* size_out); /* FUN_00480170 */
typedef struct ResourceFileTokenList {
    uint32_t count;
    char tokens[512][256];
} ResourceFileTokenList;
/* Root used by the retail-shaped one-argument token/tree entrypoints. The
 * loader initializes it from resource_mount; hosts may set the same owner
 * root explicitly when they don't mount numbered archives. */
void resource_directory_scan_root_set(const char* root);
const char* resource_directory_scan_root_get(void);
void resource_file_token_list_load_and_tree_scan_target(
    const char* token_file); /* FUN_004802d0, one-argument retail shape */

/* Direct3DRM resource-tree callback helpers (FUN_00489520, FUN_004895c0,
 * FUN_00489910). These keep the retail COM slot contract behind the resource
 * module boundary. `unused` is the first stack argument present at the two
 * retail constructor call sites; it is not read by either constructor.
 * The context's handle array contains the enumerated interface pointers.
 * Cleanup follows the scene-owner link path, which expects the caller to
 * install the matching interface-release hook. */
/* Portable resource-tree node used by direct name lookups. The resource
 * pointer is the object interface passed to ResourceNameMatchCallback. */
typedef struct ResourceTreeNode {
    const char* name;
    void* resource;
    struct ResourceTreeNode** children;
    uint32_t child_count;
} ResourceTreeNode;
typedef int (*ResourceTreeCallback)(void* object, void* context);
typedef int (*ResourceTreeNodeCallback)(ResourceTreeNode* node,
                                        void* context);
int resource_tree_callback_walk(ResourceTreeNode* root, int depth,
                                ResourceTreeNodeCallback callback,
                                void* context); /* portable tree helper */
int resource_tree_callback_walk_target(void* object, int depth,
                                       ResourceTreeCallback callback,
                                       void* context); /* 0x00476330 */
typedef struct ResourceNameMatchCallbackContext {
    const char* name;
    uint32_t name_length;
    uint32_t case_sensitive;
    void* matched_object;
    uint32_t match_count;
    int mode;
    uint32_t selected_index;
} ResourceNameMatchCallbackContext;
int resource_name_match_callback_target(
    void* object, void* context); /* 0x00476410 */
void* resource_name_register_or_query(ResourceTreeNode* root,
                                      const char* name, int mode,
                                      uint32_t* match_count_or_index);
                                                /* 0x00473bc0 */
void* resource_name_register_or_query_target(
    void** root, const char* name, int mode,
    uint32_t* match_count_or_index); /* retail 0x00473bc0 ABI */
int nested_record_list_remove_matching_key(void* owner, void* key,
                                           void* record); /* 0x0047b330 */
typedef struct ResourceTreeCallbackContext {
    int kind;
    void* child_chain;
    void* first_handle;
    int shared_mode;
    void* second_handle;
    void** handles;
    uint32_t handle_count;
    void* owner;
} ResourceTreeCallbackContext;
uint32_t resource_tree_callback_count(void* object, int depth,
                                      ResourceTreeCallback callback,
                                      void* context); /* FUN_00489910 */
ResourceTreeCallbackContext* resource_tree_callback_context_create_for_enumerator(
    void* unused, void* owner, void* enumerator); /* FUN_00489520 */
ResourceTreeCallbackContext* resource_tree_callback_context_create_for_resource(
    void* unused, void* owner, void* resource); /* FUN_004895c0 */

typedef struct ResourceFileTokenListOps {
    void* context;
    void (*directory_tree_scan)(void* context, const char* root,
                                const ResourceFileTokenList* tokens);
} ResourceFileTokenListOps;
int resource_file_token_list_load_and_tree_scan(
    const char* token_file, const char* root, ResourceFileTokenList* list,
    const ResourceFileTokenListOps* ops);
int resource_directory_token_list_contains(
    const ResourceFileTokenList* list, const char* token); /* FUN_00480570 */
typedef struct ResourceDirectoryScanOps {
    void* context;
    /* Open a search pattern such as "directory\\*.*". */
    int (*scan_open)(void* context, const char* pattern, void** scan_out);
    int (*scan_next)(void* context, void* scan, char name_out[260],
                     int* is_directory_out);
    void (*scan_close)(void* context, void* scan);
} ResourceDirectoryScanOps;
void resource_directory_tree_scan(const char* root,
                                  const ResourceFileTokenList* tokens,
                                  const ResourceDirectoryScanOps* ops);
void resource_directory_tree_scan_target(
    const char* root); /* FUN_00480390, one-argument retail shape */

/* Read the payload into handle->data (lazy; no-op if already read or NULL).
 * Returns the payload size, or -1 on failure (dormant codec / read error). */
int32_t resource_read(Resource* r);

/* Release the handle and its payload. */
void resource_free(Resource* r);

/* Whole-file loaders (FUN_004800a0/004800c0): allocate the complete opened
 * payload through the engine heap and optionally report its size. The
 * progress callback runs after the resource length is known and before the
 * allocation/read, matching the corpus call point. */
typedef void (*WholeFileProgressHook)(const char* path, uint32_t size,
                                     void* user);
void whole_file_progress_hook_set(WholeFileProgressHook hook, void* user);
void* read_whole_file_with_progress(const char* path, int32_t* size_out,
                                    int binary_mode);
void* read_whole_file(const char* path, int32_t* size_out); /* FUN_004800a0 */

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

/* RenderResourcePairReleaseAndClear (FUN_00476d10): release both owned
 * values in the pair after obtaining the second object's property slot. */
void render_resource_pair_release_and_clear(void** pair);

/* ScenePropertyOwnedBufferRelease (FUN_004762f0): fetch the property's
 * owned buffer through interface slot 0x1c and release it. */
void scene_property_owned_buffer_release(void* property);

/* ResourceInterfaceCallSlot40AndClearField14 (FUN_00477250): call the
 * interface stored at wrapper +0x10, then clear wrapper +0x14. */
typedef struct ResourceInterfaceWrapper {
    unsigned char reserved[0x10];
    void* interface_object;
    uint32_t field14;
} ResourceInterfaceWrapper;
void resource_interface_call_slot40_and_clear_field14(
    ResourceInterfaceWrapper* wrapper, uint32_t value);

/* RenderResourceTrackAndRetain (FUN_00477220): prepend the object to the
 * tracked-resource chain and retain its interface at +0x10. */
void render_resource_track_and_retain(void* resource);

/* ResourceInterfaceCallSlot88 (FUN_00477270): invoke interface vtable slot
 * 0x88 on the object held by the wrapper at +0x10. */
void resource_interface_call_slot88(void* wrapper);

/* --- wad payload band (2026-10-25; finding 01 codec story) ----------------
 * RNC-header decode dispatch, both compressed codecs, its expand wrapper,
 * the type-1 Huffman table clear, the file-layer handle pair, and the
 * wad-entry stream-slot opener. */
#define WAD_DECODE_MAGIC_FAIL 0xffff /* corpus's not-RNC verdict */
#define WAD_DECODE_TYPE_FAIL  0xfffe /* corpus's unknown-type verdict */

/* Verify the "RNC" magic + type gate and copy/delegate the payload. The raw
 * type-0 path returns zero; type-1/2 return the output pointer's high word,
 * whose low 16 bits are zero (the retail caller tests AX). Invalid magic and
 * unknown types return the two 0xffxx failure values above. */
int wad_payload_decode_dispatch(const void* header, void* dest); /* FUN_0049c840 */

/* Probe (out_payload NULL) or materialize an expanded payload; answers the
 * decoded size, 0 on any failure. Buffer owned via engine_heap_free. */
long wad_expand_payload(const void* header, void** out_payload); /* FUN_0049c7c0 */

/* Zero 16-byte table entries: 0, word 0xffff, 0, word 0. */
void wad_huffman_table_clear(void* table, uint8_t count);         /* FUN_0049cf50 */

/* Kind 1: flagged handle record, no stream buffer. Kind 0: carries the
 * 12-byte stream buffer behind; releases the record if it fails. */
typedef struct GameFileHandle {
    int32_t flag;   /* corpus record dword at +0 */
    void*   stream; /* corpus record dword at +4 (widened, LP64 deviation) */
} GameFileHandle;
void* game_file_handle_allocate(int kind);                       /* FUN_0047fed0 */
int game_file_read_byte(GameFileHandle* handle);                  /* FUN_0047fc20 */
int wad_file_read(void* destination, int element_size, int element_count,
                  GameFileHandle* handle);                         /* FUN_0047f980 */
int wad_file_seek(GameFileHandle* handle, int offset, int origin); /* FUN_0047f850 */
int wad_file_cursor_get(GameFileHandle* handle);                   /* FUN_0047fb10 */
void game_file_handle_release(GameFileHandle* handle);            /* FUN_0047ff50 */
int game_file_close(GameFileHandle* handle);                      /* FUN_0047fac0 */
/* Resolve and open through the reference WAD/loose-file route selector. */
GameFileHandle* game_file_open_dispatch(const char* path,
                                        const char* mode);          /* FUN_0047f6e0 */
/* Open a loose-file GameFileHandle after resolving the game's data root.
 * WAD-only read routes are not representable by this FILE-backed adapter. */
GameFileHandle* game_file_open_loose_path(const char* path,
                                          const char* mode);
uint32_t game_file_write(const void* data, uint32_t size,
                         uint32_t count, GameFileHandle* handle); /* FUN_0047fa50 */
int game_file_print_format(GameFileHandle* handle, const char* format, ...);
                                                                 /* FUN_0047fdb0 */
void resource_debug_output_format(const char* format, ...);        /* FUN_0047f600 */
int game_file_open_route_select(const char* path,
                                const char* mode);                 /* FUN_0047fe30 */
char* game_file_mounted_path_prefix_rewrite(const char* path);     /* FUN_0047ffc0 */
char* game_file_read_line(char* buffer, int capacity,
                          GameFileHandle* handle);                /* FUN_0047fce0 */
char* game_file_read_line_dispatch(char* buffer, int capacity,
                                   GameFileHandle* handle);       /* FUN_0047fd40 */
char* game_file_read_crlf_line(char* buffer, int capacity,
                               GameFileHandle* handle);           /* FUN_00480050 */

/* The stream-slot opener's three corpus dwords, front to back. */
typedef struct WadStreamSlots {
    const Resource* handle; /* slot[0]: opened entry (NULL = missing) */
    const void*     stream; /* slot[1]: zeroed by the opener */
    uint32_t        word2;  /* slot[2]: zeroed by the opener */
} WadStreamSlots;
int wad_entry_read_into_stream_slot(WadStreamSlots* slots,
                                    const char* name);           /* FUN_0047fea0 */

/* FixedRecordSlotAllocate (FUN_0048be00): clear and claim the first free
 * nine-dword file-resource record. Its occupancy word is at +4; the
 * separate sixteen-slot WAD resource table above is a different owner. */
int file_resource_record_slot_allocate(void); /* FUN_0048be00 */

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
 * code dwords with the count riding behind it — the codes arrive as the
 * corpus's cdecl varargs (one stack slot each) — and the map initializer
 * registers the fixed eighteen groups. The band word behind each pair is
 * shared with the entity query's id-pair reads and the dispatch's loop
 * bound, so one merged window model (the handle set's) owns it: the
 * install/peek pair below reads and writes that word. */
void resource_input_key_group_register(int group, int count, ...);
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
void resource_handle_list_append(ResourceHandleList* list,
                                 const char* value);              /* FUN_00410aa0 */

/* Rock-fall style names (FUN_0040bf70): lookup the installed config-name
 * table case-insensitively. The parser side is a separate scene-resource
 * owner, so the native table is seeded through this install hook. */
int rock_fall_style_name_lookup(const char* name, uint32_t* index);
void rock_fall_style_names_install(const char* const* names, uint32_t count);
typedef void (*RockFallStyleRegisterHook)(const char* token, uint32_t style,
                                          uint32_t slot, const char* name,
                                          void* context);
typedef struct RockFallStyleConfigEntry {
    const char* name;
    const char* resources;
} RockFallStyleConfigEntry;
void rock_fall_styles_config_load(const RockFallStyleConfigEntry* entries,
                                 uint32_t count,
                                 RockFallStyleRegisterHook register_hook,
                                 void* context); /* FUN_0040bfd0 */

typedef int (*SceneResourceAdvanceHook)(void* handle, float delta,
                                        void* context);
typedef void (*SceneResourceReleaseHook)(void* handle, void* context);
typedef void (*SceneTimeoutEffectHook)(void* handle, void* context);
typedef struct SceneTimeoutEntrySet {
    void* entries[10];
    uint32_t count;
} SceneTimeoutEntrySet;
void scene_resource_lifecycle_hooks_install(
    SceneResourceAdvanceHook advance, SceneResourceReleaseHook release,
    SceneTimeoutEffectHook bird_scarer_effect, void* context);
void scene_light_pool_slot_install(int secondary, int index, void* handle);
void scene_timeout_entry_install(int set_index, int entry_index, void* handle);
void scene_timeout_set_count_install(int set_index, uint32_t count);
typedef void (*SceneTimeoutSetCallback)(float timeout, void* output_a,
                                        void* output_b);
void scene_timeout_set_callback_set(SceneTimeoutSetCallback callback);
void scene_timeout_set_10000(void); /* FUN_0040bdc0 */
typedef void (*GlobalResourceReleaseFn)(void* handle, void* user);
void global_resource_handle_set(void* handle);
void global_resource_release_callback_set(GlobalResourceReleaseFn callback,
                                          void* user);
typedef void (*GlobalResourceCleanupFn)(void* user);
void global_resource_cleanup_callback_set(GlobalResourceCleanupFn callback,
                                          void* user);
void global_resource_handle_release(void); /* FUN_0044e0a0 */
void* panel_event_entry_append(void* panel, const char* name, uint32_t payload,
                               uint32_t value_a, uint32_t value_b,
                               uint32_t flags); /* FUN_004629d0 */
typedef void* (*PanelResourceLoadFn)(const char* name, void* user);
void panel_resource_loader_set(PanelResourceLoadFn callback, void* user);
void scroll_rect_payload_record_create(int slot, const float rect[4],
                                       int count, uint32_t value,
                                       const char* resource_name);
void* scroll_rect_payload_record_get(int slot);
typedef uint32_t (*ObjectListCountFn)(void* list, void* user);
typedef void (*ObjectListSetFn)(void* list, uint32_t index, void* value,
                                void* user);
void object_list_field_callbacks_set(ObjectListCountFn count,
                                     ObjectListSetFn set, void* user);
void object_list_field_assign_all(void* record, void* value); /* 0x00440bf0 */
typedef void* (*ResourceListCreateFn)(void* library, void* key, void* user);
typedef void (*ResourceListUpdateFn)(void* handle, void* object,
                                     void* update_value, void* user);
void resource_list_update_hooks_set(ResourceListCreateFn create,
                                    ResourceListUpdateFn update, void* user);
void resource_list_update_matching_key(void* list, void* library,
                                      void* update_value,
                                      uint32_t key); /* 0x00451bc0 */
typedef void (*OwnedObjectReleaseFn)(void* object, void* user);
void owned_object_release_hooks_set(OwnedObjectReleaseFn object_release,
                                    OwnedObjectReleaseFn scene_destroy,
                                    OwnedObjectReleaseFn node_free,
                                    void* user);
void owned_object_reference_chain_release(void* head); /* 0x00451c50 */
int string_table_entry_install(const char* name);
void string_table_entry_set_current(const char* name); /* 0x0044dde0 */
const char* string_table_entry_current(void);
void message_string_table_initialize(void); /* 0x00451cd0 */
const char* message_string_table_get(uint32_t index);
typedef void (*FileLoadingProgressDrawFn)(float progress, const float rect[4],
                                          int draw_base, void* user);
void file_loading_progress_state_install(int enabled, int total, int mode,
                                         const float rect[4],
                                         FileLoadingProgressDrawFn draw,
                                         void* user);
void file_loading_progress_update(uint32_t unused, float amount); /* 0x0044dec0 */
typedef void* (*IndexedSurfaceCreateFn)(void* owner, int mode, void* user);
typedef void* (*IndexedTextureLoadFn)(const char* name, void* user);
typedef void (*IndexedSurfaceReleaseFn)(void* handle, void* user);
typedef void (*IndexedSurfaceTextureSetFn)(void* surface, uint32_t index,
                                           void* texture, void* user);
void indexed_surface_resource_hooks_set(IndexedSurfaceCreateFn create,
                                        IndexedTextureLoadFn load_texture,
                                        IndexedSurfaceReleaseFn release,
                                        IndexedSurfaceTextureSetFn set_texture,
                                        void* user);
int indexed_surface_resource_allocate(void* owner, int width, int height);
int animated_texture_replace(const char* name); /* 0x00462e60 */
void graphics_overlay_resources_release(void); /* 0x00462ed0 */
typedef void* (*VisualModelResolveFn)(const char* path, int variant,
                                      void* user);
void visual_model_resolver_set(VisualModelResolveFn resolver, void* user);
void* visual_model_record_create(void* parent, const char* name,
                                 const char* path_a, const char* path_b,
                                 int flags); /* 0x004519b0 */
void scene_timeout_entry_sweep(SceneTimeoutEntrySet* set, float delta);
void scene_light_pool_primary_expire(float delta); /* FUN_0040c4d0 */
void scene_light_pool_secondary_expire(float delta); /* FUN_0040c520 */
void scene_resource_singleton_pool_release(void); /* FUN_0040c690 */
void terrain_marker_sets_clear(float delta); /* FUN_0040cc40 */
typedef struct SceneResourceSlotModel {
    unsigned char reserved[0x28];
    uint32_t count;
    uint32_t selected;
    void* handle;
} SceneResourceSlotModel;
SceneResourceSlotModel* resource_slot_address_by_index(int index);
void resource_slot_model_install(int index, void* handle, uint32_t count);
int resource_handle_load_with_fallback(int slot, const char* primary,
                                      const char* fallback);
typedef struct MiscEffectResourceEntry {
    const char* primary;
    const char* fallback;
} MiscEffectResourceEntry;
int misc_objects_effect_slots_bind(const MiscEffectResourceEntry* entries,
                                   uint32_t count); /* FUN_0040c990 */
typedef void* (*NumberedBitmapLoadHook)(const char* path, void* context);
void numbered_bitmap_load_hook_install(NumberedBitmapLoadHook hook,
                                        void* context);
void preload_numbered_bitmaps(const char* directory, const char* prefix);
void* numbered_bitmap_slot_peek(int index);
typedef float (*SceneAnimationDurationHook)(void* handle, uint32_t mode,
                                            void* context);
typedef uint32_t (*SceneAnimationLastFrameHook)(void* handle, void* context);
typedef struct SceneAnimationDurationState {
    void* primary;
    void* secondary;
    uint8_t flags;
} SceneAnimationDurationState;
void scene_animation_duration_hooks_install(
    SceneAnimationDurationHook duration,
    SceneAnimationLastFrameHook last_frame, void* context);
float animation_duration_for_loop_count(
    const SceneAnimationDurationState* state, uint32_t primary_mode,
    uint32_t secondary_mode, uint32_t loop_count); /* FUN_0046d1c0 */

typedef struct SceneResourceAnimationRecord {
    void* primary_resource;
    void* secondary_resource;
    const char* slot_b4_name;
    const char* slot_b8_name;
    const char* slot_c0_name;
    void* cached_b4;
    void* cached_b8;
    void* cached_c0;
    void* activity_resource;
    void* activity_cache[16];
    void* attached_resource;
    void* attached_cache[16];
} SceneResourceAnimationRecord;
typedef void* (*SceneResourceNamedResolveHook)(void* resource,
                                               const char* name,
                                               uint32_t index,
                                               void* context);
typedef void (*SceneObjectResourceReleaseHook)(void* resource,
                                               void* context);
void scene_resource_record_hooks_install(
    SceneResourceNamedResolveHook resolve,
    SceneObjectResourceReleaseHook release, void* context);
void* scene_resource_named_entry_resolve_with_fallback(
    SceneResourceAnimationRecord* record, const char* name, uint32_t index);
int scene_activity_change_dispatch(SceneResourceAnimationRecord* record,
                                   int index); /* FUN_0046da50 */
void* scene_resource_cached_slot_b4_get(
    SceneResourceAnimationRecord* record); /* FUN_0046da90 */
void* scene_resource_cached_slot_b8_get(
    SceneResourceAnimationRecord* record); /* FUN_0046dac0 */
void* scene_resource_cached_slot_c0_get(
    SceneResourceAnimationRecord* record); /* FUN_0046daf0 */
void* attached_entity_slot_object_get(SceneResourceAnimationRecord* record,
                                      int index); /* FUN_0046db20 */
void scene_object_resource_release(void* record);
                                                /* FUN_0046db70 */
typedef struct ScenePairedAnimationState {
    void* primary_resource;
    void* secondary_resource;
    void* primary_track;
    void* secondary_track;
    void* attached_models[16];
    void* activity_cache[16];
    void* attached_cache[16];
    uint32_t attached_count;
    void* scene_slots[6];
    uint32_t scene_slot_count;
    uint32_t activity_cache_count;
    uint32_t flags;
    const char* slot_base_name;
    void* cached_b4;
    void* cached_b8;
    void* cached_c0;
    void* owned_memory[4];
} ScenePairedAnimationState;
typedef void (*SceneAnimationTrackApplyHook)(void* track, void* resource,
                                             int reverse, void* context);
typedef void (*SceneResourceOwnerApplyHook)(void* resource, void* owner,
                                            void* context);
typedef void (*SceneAttachedResourceReleaseHook)(void* resource,
                                                 void* context);
void scene_animation_track_hook_install(SceneAnimationTrackApplyHook hook,
                                        void* context);
void scene_resource_owner_hook_install(SceneResourceOwnerApplyHook hook,
                                       SceneAttachedResourceReleaseHook release,
                                       void* context);
void paired_resource_state_set(ScenePairedAnimationState* state,
                               int enabled); /* FUN_0046ced0 */
void scene_entity_animation_tracks_set_enabled(
    ScenePairedAnimationState* state, int enabled); /* FUN_0046cf20 */
void scene_assign_owner_from_model(ScenePairedAnimationState* state,
                                   void* owner); /* FUN_0046d140 */
void scene_object_attached_resource_release(void* record); /* FUN_0046d1a0 */
typedef void (*SceneOwnedMemoryReleaseHook)(void* memory, void* context);
void scene_payload_memory_release_hook_install(
    SceneOwnedMemoryReleaseHook release, void* context);
void entity_payload_type1_release(ScenePairedAnimationState* state);
                                                /* FUN_0046ce10 */
int scene_record_template_instantiate(
    const ScenePairedAnimationState* source,
    ScenePairedAnimationState* destination); /* FUN_0046cff0 */
void scene_resource_draw_at_origin(SceneResourceAnimationRecord* record,
                                   float x, float y, float z); /* FUN_0046d350 */
typedef uint32_t (*SceneAnimationChannelUpdateHook)(void* resource,
                                                    uint32_t channel,
                                                    void* context);
typedef int (*SceneAnimationChannelQueryHook)(void* resource,
                                             uint32_t channel,
                                             void* context);
typedef void (*SceneAnimationVisibilityHook)(void* resource, int visible,
                                             void* context);
void scene_animation_channel_hooks_install(
    SceneAnimationChannelUpdateHook update,
    SceneAnimationChannelQueryHook query,
    SceneAnimationVisibilityHook visibility, void* context);
uint32_t scene_activity_paired_channel_update(
    ScenePairedAnimationState* state, uint32_t channel, int visible);
void scene_resource_slots_rebuild(ScenePairedAnimationState* state,
                                  const char* slot_base_name); /* FUN_0046d260 */
typedef struct SceneChannelEntry {
    void* record;
    struct SceneChannelEntry* next;
} SceneChannelEntry;
typedef struct SceneChannelSlotsState {
    SceneChannelEntry* entries;
    void* slots[3][5];
    uint32_t registered_count;
    uint32_t mode;
} SceneChannelSlotsState;
typedef uint32_t (*SceneChannelEntryProcessHook)(
    void* record, uint32_t mode, void** output_slots, uint32_t slot_capacity,
    void* context);
void scene_channel_entry_process_hook_install(
    SceneChannelEntryProcessHook hook, void* context);
void scene_channel_slots_rebuild(SceneChannelSlotsState* state,
                                 int mode); /* FUN_0046c510 */
typedef struct SceneIndexedRecord {
    int value;
    void* payload;
} SceneIndexedRecord;
typedef struct SceneIndexedRecordNode {
    SceneIndexedRecord* record;
    uint32_t source_index;
    struct SceneIndexedRecordNode* next;
} SceneIndexedRecordNode;
typedef struct SceneIndexedRecordList {
    SceneIndexedRecord* records;
    uint32_t count;
    int selected_value;
    SceneIndexedRecordNode* head;
} SceneIndexedRecordList;
void scene_record_indexed_entries_rebuild_and_refresh(
    SceneIndexedRecordList* list, SceneChannelSlotsState* channels,
    int value); /* FUN_0046cf90 */
typedef void (*SceneIndexedRecordNodeReleaseHook)(SceneIndexedRecordNode* node,
                                                  void* context);
void scene_indexed_record_release_hook_install(
    SceneIndexedRecordNodeReleaseHook release, void* context);
void scene_indexed_record_list_rebuild_for_value(
    SceneIndexedRecordList* list, int value); /* FUN_0046c340 */
typedef int (*SceneEffectPoseHook)(void* handle, float px, float py, float pz,
                                   float rx, float ry, float rz,
                                   void* context);
void scene_effect_pose_hook_install(SceneEffectPoseHook hook, void* context);
int lava_effect_select_and_place(int slot, const float* position,
                                 const float* rotation); /* FUN_0040cce0 */
typedef int (*SceneObjectPositionHook)(void* object, float position[3],
                                       void* context);
void scene_marker_templates_install(void* light_default, void* light_variant,
                                    void* marker, int scene_variant);
void scene_object_position_hook_install(SceneObjectPositionHook hook,
                                        void* context);
typedef int (*TerrainHeightAtPointHook)(float x, float y, float* height,
                                        void* context);
void terrain_point_template_install(void* handle);
void terrain_height_hook_install(TerrainHeightAtPointHook hook,
                                void* context);
void terrain_point_object_create(void* object, const float* xy); /* FUN_0040c750 */
void scene_light_create_at_position(const float position[3]);
void scene_marker_create_at_position_or_object(void* object,
                                               const float position[3]);
void scene_marker_create_at_object_position(void* object);
int global_resource_handle_load_and_enable(const char* path);
Resource* global_resource_handle_peek(void); /* port state peek */
void global_resource_handle_release(void); /* port lifecycle hook */

/* ResourceAvailabilitySweep (FUN_0040c3a0): records configured scene slots
 * whose asynchronous loads have completed. The completion callback is the
 * native seam for DDiSceneAdvanceByTime/SceneRenderLockToggle. */
typedef int (*SceneAvailabilityAdvanceHook)(void* handle, float delta,
                                            void* context);
typedef void (*SceneAvailabilityEnableHook)(void* handle, void* context);
void scene_availability_hooks_install(SceneAvailabilityAdvanceHook advance,
                                      SceneAvailabilityEnableHook enable,
                                      void* context);
void scene_availability_style_select(int style);
void scene_availability_style_slot_install(int style, int group, int slot,
                                           void* handle, int configured);
void scene_availability_transient_slot_install(int group, int slot,
                                               void* handle, int configured);
void scene_cell_table_pair_install(int style, int group, int slot,
                                   uint32_t first, uint32_t second);
void scene_cell_table_pair_read(int group, int slot, uint32_t* first,
                                uint32_t* second); /* FUN_0040c570 */
typedef void* (*SceneResourceNameFormatHook)(void* handle, uint32_t name_base,
                                             uint32_t index, void* context);
typedef void (*SceneResourceNameApplyHook)(void* handle, void* name,
                                           void* context);
void scene_resource_name_hooks_install(SceneResourceNameFormatHook format,
                                       SceneResourceNameApplyHook apply,
                                       void* context);
void scene_resource_name_slot_install(int style, int group, int slot,
                                      uint32_t handle_key,
                                      uint32_t secondary_key, void* handle,
                                      uint32_t name_base, uint32_t name_count);
int scene_resource_slot_register_name(uint32_t handle_key,
                                      uint32_t secondary_key); /* FUN_0040c5b0 */
typedef void* (*SceneResourceParentLoadHook)(void* scene, const char* path,
                                             void* context);
typedef void* (*SceneResourceChildCreateHook)(void* parent, void* context);
typedef void (*SceneResourceLockHook)(void* handle, int enabled,
                                      void* context);
void scene_resource_instance_hooks_install(SceneResourceParentLoadHook load,
                                           SceneResourceChildCreateHook clone,
                                           SceneResourceLockHook lock,
                                           void* context);
int indexed_resource_handle_set_create(void* scene, const char* path,
                                       int group); /* FUN_0040c1b0 */
typedef void* (*SceneActLoadHook)(void* scene, const char* path,
                                  const char* type, void* context);
typedef void (*SceneActNameRegisterHook)(void* parent, const char* name,
                                         uint32_t style, uint32_t group,
                                         void* context);
void scene_act_hooks_install(SceneActLoadHook load,
                             SceneActNameRegisterHook register_name,
                             void* context);
int scene_act_slot_load_and_register(void* scene, const char* path,
                                     int style, int group,
                                     const char* name); /* FUN_0040c0d0 */
typedef int (*SceneSlotPoseHook)(void* handle, int group, int slot,
                                 float x, float y, float z,
                                 float heading, float pitch, void* context);
void scene_slot_pose_hook_install(SceneSlotPoseHook hook, void* context);
int scene_slot_pose_scale_register(int group, uint32_t first, uint32_t second,
                                   float x, float y, float z,
                                   float heading, float pitch); /* FUN_0040c230 */
void rock_fall_style_scene_resources_release(void); /* FUN_0040c6c0 */
void terrain_point_object_install(int slot, void* handle);
void terrain_point_objects_remove_matching(float delta); /* FUN_0040c830 */
typedef int (*TransientEffectConfigureHook)(void* handle, int group,
                                            float x, float y, float size,
                                            float heading, float dx, float dy,
                                            void* context);
void transient_effect_configure_hook_install(
    TransientEffectConfigureHook hook, void* context);
int transient_effect_render_slot_configure(int group, float x, float y,
                                           float size, float heading,
                                           float dx, float dy); /* FUN_0040c2f0 */
int scene_resource_availability_sweep(float delta, uint32_t** groups_out,
                                      uint32_t** slots_out);

namespace unported_ledger_stubs {
typedef struct MiscObjectResourceHooks {
    void* user;
    void* (*load)(void* config, const char* group, const char* name,
                  int resource_kind, int load_flag, void* user);
    void (*enable)(void* handle, void* user);
    void (*post_load)(void* config, void* user);
} MiscObjectResourceHooks;
void misc_object_resource_hooks_set(const MiscObjectResourceHooks* hooks);
void misc_object_resources_load(void* config); /* FUN_00422850 */
void* misc_object_resource_peek(unsigned int slot);
void directx_error_message_format(const char* name, const char* description);
int directx_result_diagnostic_dispatch(int result, int diagnostic_kind,
                                      const char* source_file,
                                      int source_line);
}

/* Host operations for the retail SceneLightsBuildFromConfig sequence.
 * Owner slots identify the scene objects supplied by the renderer; they are
 * not retail pointer values. The callbacks preserve the order of the light
 * creation, interface setter, root-object, and enable calls. */
enum SceneLightOwnerSlot {
    SCENE_LIGHT_OWNER_TOP_SPOT = 0,
    SCENE_LIGHT_OWNER_TRACK_SPOT = 1,
    SCENE_LIGHT_OWNER_AMBIENT = 2,
    SCENE_LIGHT_OWNER_FIRST_PERSON = 3,
    SCENE_LIGHT_OWNER_DEFAULT_ROOT = 4
};
typedef struct SceneLightDefinition {
    const char* config_name;
    int owner_slot;
    void* owner;
    int light_kind;
    float rgb[3];
} SceneLightDefinition;
typedef struct SceneLightBuildOps {
    void* user;
    int (*rgb_read)(const char* config_name, float out_rgb[3], void* user);
    void* (*owner_get)(int owner_slot, void* user);
    void* (*light_create)(const SceneLightDefinition* definition, void* user);
    int (*light_parameter_set)(void* light, uint32_t vtable_offset,
                               float value, void* user);
    int (*light_position_set)(void* light, void* owner,
                              const float position[3], void* user);
    int (*light_pose_set)(void* light, void* owner, const float pose[6],
                          void* user);
    int (*light_enable)(void* light, int enabled, void* user);
    void* (*scene_root_get)(void* user);
    void* (*scene_object_allocate)(void* root, void* user);
    int (*scene_object_transform_init)(void* light, void* root,
                                       const float scale[3], void* user);
} SceneLightBuildOps;

#endif /* LEGORRR_RESOURCE_HPP */
