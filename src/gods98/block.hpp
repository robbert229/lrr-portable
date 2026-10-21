// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — block-text config parser (the `Lego*` format).
 *
 * Reconstruction of the single text-data language documented in finding 12:
 * brace-nested `name value` records with path lookup, serving Lego.cfg, .ae
 * entity files, .ptl level scripts and family formats. One scanner + a
 * path-lookup API serve every file type; `Lego*` itself is just the first
 * token, not a signature (finding 12, Summary).
 *
 * Corpus functions covered (finding 12 §3 function map):
 *   FUN_00478e60  BlockOpen       — comment-strip + token scan + depth
 *   FUN_004792e0  BlockFindPath   — space-separated path, case-insensitive
 *   FUN_004790b0  value getter    — record+8 (value token pointer)
 *   FUN_00478ff0  BlockSubgroup   — path + must have children
 *   FUN_00479050  value string    — heap copy; 0 when missing
 *   FUN_004790d0  bool getter     — FUN_00477710 vocab, default 2
 *   FUN_0048d980  float getter    — leading-space skip, strtod-ish
 *
 * Model note: the corpus indexes tokens in-place into a flat pool of 32-byte
 * records {name, value, depth, chain} (FUN_00479270 pool). This build builds
 * the equivalent tree directly: records carry name/value tokens and children
 * in document order, with parent derived from brace nesting. The observable
 * semantics — document-order first-match path lookup with ASCII case folding
 * and `name*` prefix matching, container value "{", comment stripping from
 * ';' to end of line, and the boolean vocabulary {TRUE,YES,ON}→1 /
 * {FALSE,NO,OFF}→0 / else 2 — are preserved (evidence: FUN_00477710 compares
 * against YES/TRUE/ON/NO/FALSE at 0x4aabec/abe4/4124/abe0/abd8 and OFF).
 */
#ifndef LEGORRR_BLOCK_HPP
#define LEGORRR_BLOCK_HPP

#include <stddef.h>
#include <stdint.h>

typedef struct BlockFile BlockFile;
typedef struct BlockRec  BlockRec;

/* Load via the resource layer (WAD + loose-file resolution, finding 01) and
 * parse. Returns NULL on open, read, or parse failure — the corpus accepts
 * any token stream, so parse failure is only an allocation failure. */
BlockFile* block_open(const char* path);

/* Parse a copy of the caller's buffer — the buffer itself is never touched.
 * For tests and for callers that already have the text in memory. */
BlockFile* block_parse(char* text, size_t len);

void block_close(BlockFile* bf);

/* Path lookup over tokens separated by spaces or `::` (the corpus's
 * DAT_004aab18 = "::" group separator — finding 23), like the corpus's
 * "%s%s%s%s%s" builders: `block_find(bf, "Lego* Main Menu::MainMenuFull
 * MenuCount")` descends the record tree. Matches are ASCII case-insensitive;
 * a record name containing '*' matches a token that starts with the text
 * before the '*' (first-token position, per FUN_004792e0). First match in
 * document order wins. Returns NULL when no record matches. */
BlockRec* block_find(BlockFile* bf, const char* path);

/* Like block_find but requires the record to have children (corpus
 * BlockSubgroup gate `record.depth < parent.depth`). Returns NULL if found
 * without children. */
BlockRec* block_subgroup(BlockFile* bf, const char* path);

/* Record accessors. A container (`name {`) reads back value "{", matching
 * the corpus record+8. */
const char* block_name(const BlockRec* r);
const char* block_value(const BlockRec* r);     /* NULL when no value token */
BlockRec*   block_first_child(const BlockRec* r);
BlockRec*   block_next_sibling(const BlockRec* r);
int         block_has_children(const BlockRec* r);
size_t      block_record_count(const BlockFile* bf);
/* Records in document (creation) order — iteration for validators/tools. */
BlockRec*   block_record_at(const BlockFile* bf, size_t index);

/* --- typed getters (missing path ⇒ dflt) -------------------------------- */

int   block_get_str(BlockFile* bf, const char* path, char* buf, size_t sz);
const char* block_get_strptr(BlockFile* bf, const char* path); /* NULL ok */
int   block_get_int(BlockFile* bf, const char* path, int dflt);
float block_get_float(BlockFile* bf, const char* path, float dflt);

/* Corpus bool (FUN_004790d0/77710): TRUE|YES|ON → 1, FALSE|NO|OFF → 0,
 * missing or unparseable → 2. */
int   block_get_bool(BlockFile* bf, const char* path);

#endif /* LEGORRR_BLOCK_HPP */
