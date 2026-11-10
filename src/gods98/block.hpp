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
 *   FUN_004790b0  path value ptr  — BlockFindPath then record+8, NULL when
 *                                   missing (the port's block_get_strptr;
 *                                   the 1-arg record accessor is port-local)
 *   FUN_00478ff0  BlockSubgroup   — path + must have children
 *   FUN_00479050  value string    — heap copy; 0 when missing
 *   FUN_004790d0  bool getter     — FUN_00477710 vocab, default 2
 *   atof @ 0x0048d980             — leading-space skip, DBCS-aware CRT parse
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
extern "C" BlockRec* block_find(BlockFile* bf, const char* path);

/* Like block_find but requires the record to have children (corpus
 * BlockSubgroup gate `record.depth < parent.depth`). Returns NULL if found
 * without children. */
extern "C" BlockRec* block_subgroup(BlockFile* bf, const char* path);

/* Record accessors. A container (`name {`) reads back value "{", matching
 * the corpus record+8. */
extern "C" const char* block_name(const BlockRec* r);
extern "C" const char* block_value(const BlockRec* r);     /* NULL when no value token */
extern "C" BlockRec*   block_first_child(const BlockRec* r);
BlockRec*   block_next_sibling(const BlockRec* r);

/* SortedEntryNextEqualKey (FUN_00479020, findings 230/403): follow the
 * sorted record chain and answer the next record whose depth (+0xc)
 * equals the starting record's depth — "equal key" is the depth word,
 * and the walk stops the moment the chain gets shallower. The corpus
 * reads the followed entry's depth before checking it (NULL next
 * crashes — guarded here); in the port's tree model the walk is exactly
 * the next record under the same parent, so this is next-sibling. */
extern "C" BlockRec* block_next_equal_key(const BlockRec* r);
int         block_has_children(const BlockRec* r);
size_t      block_record_count(const BlockFile* bf);
/* Records in document (creation) order — iteration for validators/tools. */
BlockRec*   block_record_at(const BlockFile* bf, size_t index);

/* --- typed getters (missing path ⇒ dflt) -------------------------------- */

int   block_get_str(BlockFile* bf, const char* path, char* buf, size_t sz);
extern "C" const char* block_get_strptr(BlockFile* bf, const char* path); /* NULL ok */
char* block_get_str_copy(BlockFile* bf, const char* path); /* FUN_00479050 */
int   block_get_int(BlockFile* bf, const char* path, int dflt);
float block_get_float(BlockFile* bf, const char* path, float dflt);

/* ConfigBooleanParse (FUN_00477710): TRUE|YES|ON → 1, FALSE|NO|OFF → 0,
 * unparseable → 2. */
extern "C" int   block_get_bool(const char* value);
/* Path convenience corresponding to ConfigBooleanGetDefault2 (0x4790d0);
 * missing or unparseable values answer 2. */
int   block_get_bool(BlockFile* bf, const char* path);

/* ConfigRgbVectorRead's retail colon separator at DAT_004a1cd4. */
extern const char s_config_rgb_separator[];

/* ConfigRgbVectorRead (FUN_00479170): reads the path's value string,
 * splits it on commas the way FUN_00477440 applies the DAT_004a1cd4
 * separator (empty tokens kept, the final character never tested as a
 * separator start), and requires exactly three tokens. The three tokens
 * are integer-parsed (FUN_0048db70 role) and land scaled by 1/255; the
 * corpus multiplies in x87 float10 — the port hands back plain floats
 * (deviation) and splits a stack copy because the port's value string
 * is the tree storage (documented deviation). */
int config_rgb_vector_read(BlockFile* bf, const char* path,
                           float* r, float* g, float* b);

/* ReadAngleSettingRadians (FUN_00479110): the path's float value (the
 * corpus's missing-record half defaults to its empty-string byte, so a
 * miss parses 0.0 either way) scales degrees to radians by
 * 1/360 * 2π. A zero — or missing — value answers zero, matching the
 * corpus's gate that skips the multiply; the x87 float10 product lands
 * as plain float (deviation). */
float config_angle_radians_get(BlockFile* bf, const char* path);

/* RegistryPathSplit (FUN_0048b330, finding 402): the backslash-path
 * component copier the registry key builders loop over — copies bytes
 * up to (not including) the next backslash or NUL into `out`, skips one
 * separator, and returns the rest of the path. Same split idiom as the
 * config nested paths above. NULL path answers NULL, out NULL copies
 * nothing (documented deviation). */
const char* registry_path_split(const char* path, char* out);

/* BlockTextTokenPoolDestroy (FUN_00478e20): frees every nonzero pool slot
 * through the engine heap, then clears the head cursor and the resident
 * flag. The corpus leaves the freed slot pointers dangling; the port
 * NULLs them (anti-dangling deviation). The pool's corpus storage — 32
 * slots at DAT_00506498, grow watermark DAT_0050651c, free-list head
 * DAT_00506518, resident flag DAT_00506520 — is module-local here with
 * the hooks below (the pool init entry point is not ported);
 * DAT_00506518's second corpus role as crt.cpp's record-list head is a
 * documented aliasing the port does not model across translation units. */
void  block_text_token_pool_destroy(void);
void  block_text_token_pool_init(void);               /* FUN_00478df0 */
void  block_text_token_pool_grow(void);              /* FUN_00479490 */
void  block_text_token_pool_slot_set(int slot, void* token);
void* block_text_token_pool_slot_peek(int slot);
void  block_text_token_pool_head_set(void* head);
void* block_text_token_pool_head_peek(void);
int   block_text_token_pool_resident_peek(void);
int   block_text_token_pool_watermark_peek(void);    /* port hook */

#endif /* LEGORRR_BLOCK_HPP */
