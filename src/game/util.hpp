// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — shared corpus helpers.
 *
 * `util_query_fields` reconstructs the corpus's FUN_00477440 (StrMatch /
 * "resource exists + first-token" helper, findings 07/09/12). Given a
 * config value that is either (a) an inline token list or (b) the name of a
 * resource, it yields up to six whitespace-delimited field strings and a
 * status code:
 *
 *   0  resource could not be opened / no fields
 *   2  resource/value has 1–2 fields        (map header read, timer value)
 *   3  exactly 3 fields                     (objective AVI: path w h)
 *   6  >= 6 fields                          (objective image with colour triple)
 *
 * Callers index the returned fields where the corpus uses its stack-slotted
 * locals (local_5b8/local_4f0 etc. in FUN_00457d40).
 *
 * Integer conversion uses the CRT's `atoi` entry, matching FUN_0048db70;
 * floating conversion uses the CRT's DBCS-aware `atof` entry at 0x0048d980.
 */
#ifndef LEGORRR_GAME_UTIL_HPP
#define LEGORRR_GAME_UTIL_HPP

#include <cstddef>
#include <stdint.h>

#include "gods98/block.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#define UTIL_QUERY_MAX_FIELDS 6
#define BUBBLE_ACTION_STRING_COUNT 39

/* Fill `out[i]` (each up to `out_sz` incl. NUL) with the parsed fields.
 * Returns the status code described above. */
int util_query_fields(const char* path_or_value, char out[UTIL_QUERY_MAX_FIELDS][256],
                      size_t out_sz);

/* WhitespaceTokenize (FUN_004774b0): split a mutable buffer in place and
 * write token pointers in document order. The corpus includes an empty
 * first token when the input begins with whitespace and counts a final
 * empty token after trailing whitespace. */
int whitespace_tokenize(char* text, char** fields); /* FUN_004774b0 */

/* GameStringHashCompute (FUN_00477670): polynomial hash over bytes, with
 * optional whitespace skipping and case folding. */
int game_string_hash_compute(const unsigned char* text,
                             int skip_whitespace,
                             int uppercase); /* FUN_00477670 */

/* StringTrailingDigitsSplit (FUN_00481ac0): copy `text` to `prefix`, split
 * off a nonempty decimal suffix, and return `digit_count` on success. */
int* string_trailing_digits_split(const char* text, char* prefix,
                                  int* value, int* digit_count);

/* CounterFormatStringBuild (FUN_00481b80): append a zero-padded decimal
 * counter to the prefix and then increment the counter. `out` must hold the
 * prefix, padding, and formatted decimal string. */
void counter_format_string_build(const char* prefix, char* out,
                                 int* counter, int width);

typedef struct ParsedEntityHealthValue {
    uint32_t flags;       /* 0x80 integer, 0x100 real, 0x200 invalid */
    uint32_t consumed;
    uint32_t reserved[2];
    double value;         /* corpus result at +0x10 */
} ParsedEntityHealthValue;
const ParsedEntityHealthValue* parse_entity_health_value(
    const char* text, int length, int flags, int mode); /* FUN_004907f0 */

typedef struct MotionUtilityState {
    uint32_t flags;
    float channel[3];
    float lower_limit;
    float upper_limit;
    float source_vector[3];
    float random_min;
    float random_max;
    float scaled_vector[3];
} MotionUtilityState;
typedef float (*MotionRandomRangeFn)(float minimum, float maximum,
                                     void* user);
void motion_utility_state_install(MotionUtilityState* state,
                                  MotionRandomRangeFn random_range,
                                  void* user);
void motion_three_channel_tween_step(float delta); /* 0x0044da50 */
void motion_waypoint_vector_random_scale_apply(void); /* 0x0044d710 */

/* DriftOffsetStepApply (0x0044d0d0) advances the module's randomized drift
 * offset. The state keeps the recovered timer, per-axis amplitude, and
 * horizontal rejection bound explicit; callers provide the original random
 * range seam. */
typedef struct MotionDriftState {
    uint32_t flags;
    float timer;
    float timer_min;
    float timer_max;
    float offset[3];
    float amplitude[3];
    float center_x;
    float max_abs_x;
} MotionDriftState;
void motion_drift_state_install(MotionDriftState* state,
                                MotionRandomRangeFn random_range,
                                void* user);
void motion_drift_offset_step(float delta); /* 0x0044d0d0 */

/* First root record name of a parsed block file, used as the leading path
 * segment for `block_find` paths (the block parser keeps the file's first
 * `name {` record as the root; the game layer reads config relative to it).
 * Falls back to "Lego*" when the file is empty. */
const char* util_root_name(BlockFile* bf);

/* Resolve a level-scoped config path against the shipped layout. The shipped
 * `Lego.cfg` nests every level group under the root's `Levels` container, so
 * the path is tried through `root Levels <level> <suffix>` first, falling
 * back to the flat `root <level> <suffix>` (fixture/legacy shape). Returns
 * the resolving path (written into out) or NULL when neither resolves.
 * `suffix` is everything after the level name ("SurfaceMap", "Reward Enable",
 * "Reward Importance", …). */
const char* util_level_cfg_path(BlockFile* cfg, const char* level,
                                const char* suffix, char* out, size_t out_sz);

/* StringTableIndexLookup (FUN_00408c80), over the corpus name-pointer table
 * (DAT_004b8a3c/50). Entries are borrowed and must outlive the installed
 * table. A miss returns the configured count. */
void string_table_install(const char* const* entries, unsigned int count);
unsigned int string_table_index_lookup(const char* value);
void bubble_action_strings_initialize(void); /* FUN_004070b0 */
const char* bubble_action_string_get(unsigned int index);
int case_insensitive_name_table_lookup_at_offset8(const void* const* entries,
                                                   unsigned int count,
                                                   const char* name);
                                                /* FUN_00451fd0 */

/* Minimal fixed-capacity owner for the pending-record five-dword rows. */
void pending_record_table_install(const uint32_t* rows, unsigned int count);
void pending_record_release_hook_set(void (*hook)(uint32_t* row));
unsigned int pending_record_count_get(void);
void pending_record_list_entry_move_to_front(unsigned int index); /* FUN_00419de0 */
void pending_record_list_entry_remove_and_release(unsigned int index); /* FUN_00419e80 */
void pending_record_release_if_present(void); /* FUN_00419e60 */
void pending_record_release_all(void); /* FUN_00419f10 */

void level_identifier_remap_table_install(const uint32_t* pairs,
                                          unsigned int count);
void command_entity_reference_remap(uint32_t* entity_reference); /* FUN_0045d870 */

/* Port-side intrusive incident-list model for EntityLinkedListEntriesRemoveByKey.
 * Nodes and list records are caller-owned; the hooks release removed storage. */
struct EntityLinkNode {
    uint32_t key;
    EntityLinkNode* next;
};
struct EntityLinkList {
    EntityLinkNode* head;
    unsigned int count;
    int kind;
    EntityLinkList* next;
};
void entity_link_lists_install(EntityLinkList** head);
void entity_link_release_hooks_set(void (*node_hook)(EntityLinkNode*),
                                  void (*list_hook)(EntityLinkList*));
void entity_linked_list_entries_remove_by_key(uint32_t key); /* FUN_00419830 */
void terrain_cell_linked_entries_remove(uint32_t key); /* FUN_004198f0 */

/* Resource-table owner matching the width/height/data fields at +0x74/+0x78/+0x80. */
struct RectangularResourceTable {
    unsigned int width;
    unsigned int height;
    void** resources;
};
void rectangular_resource_table_release_hooks_set(void (*resource_hook)(void*),
                                                   void (*array_hook)(void*));
void rectangular_resource_table_destroy(RectangularResourceTable* table); /* FUN_0042b480 */
typedef void (*StringQueueFlushHook)(uint32_t first, uint32_t second,
                                     uint32_t third, const char* text);
void string_queue_flush_hook_set(StringQueueFlushHook hook);
unsigned int string_queue_count_get(void);
void string_queue_record_or_flush(uint32_t first, uint32_t second,
                                  uint32_t third, const char* text,
                                  int record); /* FUN_00410320 */
struct CallbackOperationNode {
    CallbackOperationNode* next;
    void* payload;
};
typedef void (*CallbackOperationVisit)(void* stack_owner,
                                       CallbackOperationNode* node,
                                       int* status);
int callback_operation_run_and_return_status(void* stack_owner,
                                             CallbackOperationNode* first,
                                             CallbackOperationVisit visit); /* FUN_00410280 */
typedef struct RecordCountRenderHooks {
    void* context;
    void (*style_set)(void* context, unsigned int slot, float first,
                      float second, unsigned int mode);
    void (*digit_set)(void* context, unsigned int slot, unsigned int digit);
    void (*slot_hide)(void* context, unsigned int slot, int hidden);
    void (*blank_set)(void* context, unsigned int slot);
} RecordCountRenderHooks;
void record_count_digits_fill(const RecordCountRenderHooks* hooks,
                              uint32_t value); /* FUN_0040a5e0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_UTIL_HPP */
