// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — block-text config (the `Lego*` format, finding 12).
 *
 * The config tree itself is the original unit Config.c (gods98/config.h,
 * G98-006): block_open loads a file into a flat list of records and
 * block_find looks items up by `::`-separated paths. This header adds the
 * port-only conveniences built on those records, and two config-adjacent
 * helpers from other original units: ConfigBooleanParse (FUN_00477710) and
 * RegistryPathSplit (FUN_0048b330). */
#ifndef LEGORRR_BLOCK_HPP
#define LEGORRR_BLOCK_HPP

#include <stddef.h>
#include <stdint.h>

#include "config.h"

/* Record accessors. NULL records read back NULL. A container (`name {`)
 * reads back the value "{". */
const char* block_name(const BlockRec* r);
const char* block_value(const BlockRec* r);
/* The record's first item, or NULL when it has none. */
BlockRec*   block_first_child(const BlockRec* r);
/* block_next_equal_key without the original's crash at the end of a
 * file's top level: NULL for a NULL record and never the empty record that
 * ends the file. */
BlockRec*   block_next_sibling(const BlockRec* r);
int         block_has_children(const BlockRec* r);
/* Records with a key, in document order — iteration for validators and
 * tools. */
size_t      block_record_count(const BlockFile* bf);
BlockRec*   block_record_at(const BlockFile* bf, size_t index);

/* --- typed getters (missing path ⇒ dflt) -------------------------------- */

int   block_get_str(BlockFile* bf, const char* path, char* buf, size_t sz);
int   block_get_int(BlockFile* bf, const char* path, int dflt);
float block_get_float(BlockFile* bf, const char* path, float dflt);

/* ConfigBooleanParse (FUN_00477710): TRUE|YES|ON → 1, FALSE|NO|OFF → 0,
 * unparseable → 2. */
extern "C" int   block_get_bool(const char* value);

/* RegistryPathSplit (FUN_0048b330, finding 402): the backslash-path
 * component copier the registry key builders loop over — copies bytes
 * up to (not including) the next backslash or NUL into `out`, skips one
 * separator, and returns the rest of the path. NULL path answers NULL, out
 * NULL copies nothing (documented deviation). */
const char* registry_path_split(const char* path, char* out);

#endif /* LEGORRR_BLOCK_HPP */
