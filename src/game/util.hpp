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
 * `util_atoi`/`util_atof` are thin wrappers over strtol/strtod — the corpus's
 * FUN_0048db70/FUN_0048d980.
 */
#ifndef LEGORRR_GAME_UTIL_HPP
#define LEGORRR_GAME_UTIL_HPP

#include <cstddef>

#include "gods98/block.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#define UTIL_QUERY_MAX_FIELDS 6

/* Fill `out[i]` (each up to `out_sz` incl. NUL) with the parsed fields.
 * Returns the status code described above. */
int util_query_fields(const char* path_or_value, char out[UTIL_QUERY_MAX_FIELDS][256],
                      size_t out_sz);

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

long  util_atoi(const char* s);
float util_atof(const char* s);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_UTIL_HPP */
