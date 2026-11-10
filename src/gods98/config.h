// SPDX-License-Identifier: LGPL-3.0-or-later
/* Config.c (original translation unit G98-006, finding 505): the gods98
 * config tree. It loads a block-text file (finding 12) into a flat list of
 * 32-byte records and looks items up by `::`-separated paths.
 *
 * C-compatible header (ADR 0005). Config.c is compiled as C in the matching
 * build and as C++ in the native port. The port-only conveniences built on
 * these records (block_parse, the record accessors and the typed getters)
 * are in gods98/block.hpp. */
#ifndef LEGORRR_GODS98_CONFIG_H
#define LEGORRR_GODS98_CONFIG_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* One config record (0x20 bytes in the original). A file is a list of them
 * in document order: each holds one `key value` pair and its brace depth.
 * A container reads back the value "{" and its items follow at depth + 1.
 * The list ends with an empty record (no key) at the depth after the last
 * closing brace. */
typedef struct BlockRec {
    char* file_text;            /* +00: the loaded text, root record only */
    char* key;                  /* +04 */
    char* value;                /* +08 */
    unsigned int depth;         /* +0c */
    unsigned int unused;        /* +10 */
    struct BlockRec* next;      /* +14 */
    struct BlockRec* prev;      /* +18 */
    struct BlockRec* next_free; /* +1c: pool link; the record itself in use */
} BlockRec;

/* A loaded file is its root (first) record. */
typedef BlockRec BlockFile;

/* Config_Initialise (FUN_00478df0): clear the record pool. */
void block_text_token_pool_init(void);

/* Config_Shutdown (FUN_00478e20): free every pool block. */
void block_text_token_pool_destroy(void);

/* Config_Load (FUN_00478e60): load and tokenise a file through the resource
 * layer. Returns its root record, or NULL when the file cannot be read. */
BlockFile* block_open(const char* path);

/* Config_BuildStringID (FUN_00478f50): join the NULL-terminated strings with
 * "::" into one static buffer and return it. */
const char* string_path_join(const char* root, ...);

/* Config_FindArray (FUN_00478ff0): the first item of the container at
 * `path`, or NULL when it is missing or empty. */
BlockRec* block_subgroup(BlockFile* root, const char* path);

/* Config_GetNextItem (FUN_00479020): the next record at the same depth
 * before the container ends, or NULL. */
BlockRec* block_next_equal_key(const BlockRec* start);

/* Config_GetStringValue (FUN_00479050): a heap copy of the value at `path`,
 * or NULL. The caller frees it. */
char* block_get_str_copy(BlockFile* root, const char* path);

/* Config_GetTempStringValue (FUN_004790b0): the value at `path`, or NULL. */
const char* block_get_strptr(BlockFile* root, const char* path);

/* Config_GetBoolValue (FUN_004790d0): the value at `path` read as a
 * boolean: 1 for TRUE, YES or ON, 0 for FALSE, NO or OFF, and 2 when it is
 * missing or anything else. */
int block_get_bool_path(BlockFile* root, const char* path);

/* Config_GetAngle (FUN_00479110): the value at `path` in degrees, returned
 * in radians. A missing value reads as 0. */
float config_angle_radians_get(BlockFile* root, const char* path);

/* Config_GetRGBValue (FUN_00479170): read `r:g:b` (0-255 each) as three
 * floats in 0-1. Returns 1 when the value has exactly three parts. */
int config_rgb_vector_read(BlockFile* root, const char* path,
                           float* r, float* g, float* b);

/* Config_Free (FUN_00479240): free a loaded file and return its records to
 * the pool. */
void block_close(BlockFile* root);

/* Config_Create (FUN_00479270): take a record from the pool and append it
 * after `prev` (NULL starts a new list). */
BlockRec* block_record_create(BlockRec* prev);

/* Config_Remove (FUN_004792c0): return a record to the pool. */
void block_record_remove(BlockRec* dead);

/* Config_FindItem (FUN_004792e0): the record at `path`, a list of keys
 * separated by "::" from the root. A root key ending in `*` matches any
 * first key that starts with the text before the `*`; when it does, the
 * last matching record wins, otherwise the first. Keys compare without
 * case. */
BlockRec* block_find(BlockFile* root, const char* path);

/* Config_AddList (FUN_00479490): add a pool block twice the size of the
 * last one. */
void block_text_token_pool_grow(void);

/* Port-only additions (not in the original unit). */

/* Tokenise a copy of `text` the way block_open does a file. */
BlockFile* block_parse(const char* text, size_t length);

/* The pool state, for tests. */
void* block_text_token_pool_slot_peek(int slot);
void* block_text_token_pool_head_peek(void);
int block_text_token_pool_resident_peek(void);
int block_text_token_pool_watermark_peek(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_CONFIG_H */
