// SPDX-License-Identifier: LGPL-3.0-or-later
/* Block-text (Lego*) scanner + path query — finding 12.
 *
 * See block.hpp for the corpus mapping. The two-pass scan mirrors the
 * corpus: pass 1 clears ';' comments and converts ' \t\r\n' to NUL
 * (FUN_00478e60 first loop), pass 2 walks tokens building records
 * (second loop's flat pool + depth bookkeeping, expressed here as a tree).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "block.hpp"
#include "heap.hpp"
#include "resource.hpp"

typedef struct BlockRec {
    char* name;                 /* token pointer into BlockFile::buffer */
    char* value;                /* NULL = none; container value is "{" */
    struct BlockRec* child;     /* first child */
    struct BlockRec* sibling;   /* next sibling under the same parent */
    struct BlockRec* parent;
} BlockRec;

typedef struct StackEnt {
    BlockRec* container;        /* NULL = implicit file root */
    BlockRec* last_child;
} StackEnt;

struct BlockFile {
    char* buffer;               /* owned copy: all tokens point in */
    size_t len;
    BlockRec* roots;            /* depth-0 records, sibling chain */
    BlockRec** all;             /* document order, for count/frees */
    size_t nrec, all_cap;
};

static int s_lower(int c) { return (c >= 'a' && c <= 'z') ? c - 0x20 : c; }

static int s_strcasecmp(const char* a, const char* b)
{
    for (;;) {
        int ca = s_lower((unsigned char)*a++), cb = s_lower((unsigned char)*b++);
        if (ca != cb) {
            return ca - cb;
        }
        if (ca == 0) {
            return 0;
        }
    }
}

/* strnicmp (ASCII-folded): compares exactly n bytes of `a` against the
 * first n bytes of `b`, stopping early only on a NUL in `a`. */
static int s_strnicasecmp(const char* a, const char* b, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        int ca = s_lower((unsigned char)a[i]);
        int cb = s_lower((unsigned char)b[i]);
        if (ca != cb) {
            return ca - cb;
        }
        if (ca == 0) {
            return 0;
        }
    }
    return 0;
}

/* --- construction ------------------------------------------------------- */

static BlockRec* rec_new(BlockFile* bf)
{
    BlockRec* r = (BlockRec*)calloc(1, sizeof(BlockRec));
    if (r == NULL) {
        return NULL;
    }
    if (bf->nrec == bf->all_cap) {
        size_t cap = bf->all_cap ? bf->all_cap * 2 : 64;
        BlockRec** grown = (BlockRec**)realloc(bf->all, cap * sizeof(*grown));
        if (grown == NULL) {
            free(r);
            return NULL;
        }
        bf->all = grown;
        bf->all_cap = cap;
    }
    bf->all[bf->nrec++] = r;
    return r;
}

/* Attach under the current container and advance last-child. */
static void rec_attach(BlockFile* bf, StackEnt* top, BlockRec* r)
{
    r->parent = top->container;
    if (top->container == NULL) {
        BlockRec** tail = &bf->roots;
        while (*tail) {
            tail = &(*tail)->sibling;
        }
        *tail = r;
    } else if (top->container->child == NULL) {
        top->container->child = r;
    } else {
        top->last_child->sibling = r;
    }
    top->last_child = r;
}

BlockFile* block_parse(char* text, size_t len)
{
    BlockFile* bf = (BlockFile*)calloc(1, sizeof(BlockFile));
    if (bf == NULL) {
        return NULL;
    }
    bf->buffer = (char*)malloc(len + 1);
    if (bf->buffer == NULL) {
        free(bf);
        return NULL;
    }
    memcpy(bf->buffer, text, len);
    bf->buffer[len] = '\0';
    bf->len = len;

    /* Pass 1: comments + whitespace (corpus FUN_00478e60 loop 1). Only
     * space/tab/CR/LF separate tokens; ';' comments run to end of line. */
    int comment = 0;
    for (size_t i = 0; i < len; i++) {
        char c = bf->buffer[i];
        if (c == ';') {
            comment = 1;
        } else if (c == '\n') {
            comment = 0;
        }
        if (comment || c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            bf->buffer[i] = '\0';
        }
    }

    /* Pass 2: token walk. A record takes one name token then one value
     * token (corpus eager two-slot records); '{' in value position opens a
     * container and takes the record's value; '}' closes, flushing a
     * pending name as a valueless record. */
    StackEnt stack[64];
    size_t depth = 0;
    stack[0].container = NULL;
    stack[0].last_child = NULL;

    char* pending = NULL;
    for (char* t = bf->buffer; t <= bf->buffer + len;) {
        if (*t == '\0') {
            ++t;
            continue;
        }
        char* tok = t;
        while (*t != '\0') {
            ++t;
        }

        if (strcmp(tok, "}") == 0) {
            if (pending != NULL) {
                BlockRec* r = rec_new(bf);
                if (r == NULL) { goto fail; }
                r->name = pending;
                rec_attach(bf, &stack[depth], r);
                pending = NULL;
            }
            if (depth > 0) {
                --depth;
            }
            continue;
        }

        if (pending == NULL) {
            pending = tok;                     /* name position */
        } else {
            BlockRec* r = rec_new(bf);         /* value position */
            if (r == NULL) { goto fail; }
            if (strcmp(tok, "{") == 0) {
                r->name = pending;
                r->value = tok;                /* container: corpus value "{" */
                rec_attach(bf, &stack[depth], r);
                pending = NULL;
                if (depth + 1 < 64) {
                    ++depth;
                    stack[depth].container = r;
                    stack[depth].last_child = NULL;
                } else {
                    fprintf(stderr, "block: nesting overflow, truncating\n");
                }
            } else {
                r->name = pending;
                r->value = tok;
                rec_attach(bf, &stack[depth], r);
                pending = NULL;
            }
        }
    }
    if (pending != NULL) {
        BlockRec* r = rec_new(bf);
        if (r == NULL) { goto fail; }
        r->name = pending;
        rec_attach(bf, &stack[depth], r);
    }
    return bf;

fail:
    bf->len = 0;
    block_close(bf);
    return NULL;
}

BlockFile* block_open(const char* path)
{
    Resource* res = resource_open(path, RESOURCE_AUTO);
    if (res == NULL) {
        return NULL;
    }
    int32_t n = resource_read(res);
    if (n <= 0) {
        resource_free(res);
        return NULL;
    }
    BlockFile* bf = block_parse((char*)res->data, (size_t)n);
    resource_free(res);
    return bf;
}

void block_close(BlockFile* bf)
{
    if (bf == NULL) {
        return;
    }
    for (size_t i = 0; i < bf->nrec; i++) {
        free(bf->all[i]);
    }
    free(bf->all);
    free(bf->buffer);
    free(bf);
}

/* --- path lookup (corpus FUN_004792e0) ----------------------------------- */

/* Match one candidate record against a path segment: ASCII case-insensitive
 * exact match, plus — first segment only, per the FUN_004792e0 wildcard gate
 * (iVar3/iVar4 == 1) — prefix match when the *record name* holds '*'.
 * `seg`/`len` bound the segment (block_find tokenizes without NUL-splitting,
 * so segments can carry `::` and are compared length-bounded). */
static int seg_match_len(const BlockRec* r, const char* seg, size_t len,
                         int first)
{
    if (r == NULL || r->name == NULL) {
        return 0;
    }
    if (first) {
        const char* star = strchr(r->name, '*');
        if (star != NULL) {
            size_t plen = (size_t)(star - r->name);
            return len >= plen
                && s_strnicasecmp(seg, r->name, plen) == 0;
        }
    }
    return strlen(r->name) == len
        && s_strnicasecmp(seg, r->name, len) == 0;
}

/* Path lookup. The corpus builds queries with the exe name as the first
 * token and `::` as the group separator ("LegoRR Main Menu::MainMenuFull
 * MenuCount" — DAT_004aab18 = "::", finding 23), so a segment boundary is
 * either a space or a "::" run; matching is otherwise unchanged. */
BlockRec* block_find(BlockFile* bf, const char* path)
{
    if (bf == NULL || path == NULL || *path == '\0') {
        return NULL;
    }

    BlockRec* cur = NULL;
    int first = 1;
    const char* p = path;

    for (;;) {
        /* skip separators: spaces and "::" runs (both are boundaries) */
        while (*p == ' ' || (*p == ':' && p[1] == ':')) {
            if (*p == ':') {
                p += 2;
            } else {
                ++p;
            }
        }
        if (*p == '\0') {
            break;
        }

        const char* s = p;
        while (*p != '\0' && *p != ' '
               && !(*p == ':' && p[1] == ':')) {
            ++p;
        }
        size_t len = (size_t)(p - s);

        /* descend: candidates = children of cur (or roots for the first
         * segment), document order, first match wins */
        BlockRec* cand = (cur == NULL) ? bf->roots : cur->child;
        BlockRec* hit = NULL;
        while (cand != NULL) {
            if (seg_match_len(cand, s, len, first)) {
                hit = cand;
                break;
            }
            cand = cand->sibling;
        }
        if (hit == NULL) {
            cur = NULL;
            break;
        }
        cur = hit;
        first = 0;
    }

    return cur;
}

BlockRec* block_subgroup(BlockFile* bf, const char* path)
{
    BlockRec* r = block_find(bf, path);
    if (r != NULL && r->child == NULL) {
        return NULL;          /* corpus gate: record.depth < parent.depth */
    }
    return r;
}

/* --- accessors ------------------------------------------------------------ */

const char* block_name(const BlockRec* r)  { return r ? r->name : NULL; }
const char* block_value(const BlockRec* r) { return r ? r->value : NULL; }

BlockRec* block_first_child(const BlockRec* r) { return r ? r->child : NULL; }
BlockRec* block_next_sibling(const BlockRec* r) { return r ? r->sibling : NULL; }

/* SortedEntryNextEqualKey (FUN_00479020): next record at the same depth
 * is next-sibling in the tree model (see header; corpus next-pointer
 * read guarded). */
BlockRec* block_next_equal_key(const BlockRec* r)
{
    return block_next_sibling(r);
}
int block_has_children(const BlockRec* r) { return r && r->child != NULL; }

size_t block_record_count(const BlockFile* bf) { return bf ? bf->nrec : 0; }

BlockRec* block_record_at(const BlockFile* bf, size_t index)
{
    if (bf == NULL || index >= bf->nrec) {
        return NULL;
    }
    return bf->all[index];
}

/* --- typed getters --------------------------------------------------------- */

int block_get_str(BlockFile* bf, const char* path, char* buf, size_t sz)
{
    BlockRec* r = block_find(bf, path);
    if (r == NULL || r->value == NULL) {
        return 0;
    }
    snprintf(buf, sz, "%s", r->value);
    return 1;
}

const char* block_get_strptr(BlockFile* bf, const char* path)
{
    BlockRec* r = block_find(bf, path);
    return (r == NULL || r->value == NULL) ? NULL : r->value;
}

int block_get_int(BlockFile* bf, const char* path, int dflt)
{
    BlockRec* r = block_find(bf, path);
    if (r == NULL || r->value == NULL) {
        return dflt;
    }
    char* end = NULL;
    long v = strtol(r->value, &end, 10);
    return (end == r->value) ? dflt : (int)v;
}

float block_get_float(BlockFile* bf, const char* path, float dflt)
{
    BlockRec* r = block_find(bf, path);
    if (r == NULL || r->value == NULL) {
        return dflt;
    }
    char* end = NULL;
    float v = strtof(r->value, &end);
    return (end == r->value) ? dflt : v;
}

/* Corpus FUN_00477710: YES/TRUE/ON → 1; NO/FALSE/OFF → 0; else 2.
 * Missing path is also 2 (FUN_004790d0's init value). */
int block_get_bool(BlockFile* bf, const char* path)
{
    const char* v = block_get_strptr(bf, path);
    if (v == NULL) {
        return 2;
    }
    if (s_strcasecmp(v, "YES") == 0 || s_strcasecmp(v, "TRUE") == 0
        || s_strcasecmp(v, "ON") == 0) {
        return 1;
    }
    if (s_strcasecmp(v, "NO") == 0 || s_strcasecmp(v, "FALSE") == 0
        || s_strcasecmp(v, "OFF") == 0) {
        return 0;
    }
    return 2;
}

/* RegistryPathSplit (FUN_0048b330, finding 402): the corpus copies the
 * leading backslash component, terminates both ends, and skips one
 * separator. The registry's 100-byte caller buffer bound is the
 * caller's business (finding 402 notes the helper itself is unbounded —
 * reproduced, caller checks live where the registry lands). */
const char* registry_path_split(const char* path, char* out)
{
    if (path == NULL || out == NULL) {
        return path;
    }
    *out = '\0';
    char c = *path;
    while (c != '\0' && c != '\\') {
        *out = c;
        ++out;
        ++path;
        c = *path;
    }
    if (*path == '\\') {
        ++path;
    }
    *out = '\0';
    return path;
}

/* BlockTextTokenPool (FUN_00478e20 destroy, FUN_00479490 grow; init
 * unported) — see block.hpp. Corpus storage stays module-local; the slot
 * word array runs DAT_00506498..DAT_00506514 (the destroy loop's end
 * condition is exactly the head cursor word), so the port's 32 slots match
 * the corpus 1:1. Grow chains doubled chunks into the free-list head role
 * of DAT_00506518 on a host-aligned widened node layout (see grow below);
 * the corpus's second role for that word, crt.cpp's record-list head, is
 * documented aliasing the port does not model across translation units. */
#define BLOCK_TOKEN_POOL_SLOTS 32
static void* s_token_pool_slots[BLOCK_TOKEN_POOL_SLOTS];
static void* s_token_pool_head;      /* DAT_00506518 role: free-list head */
static int   s_token_pool_resident;  /* _DAT_00506520 role */
static int   s_token_pool_watermark; /* DAT_0050651c role: grow counter */

/* BlockTextTokenPoolGrow (FUN_00479490): the watermark counter doubles the
 * chunk size (1 << watermark), the chunk is engine-heap storage, and its
 * nodes chain into the free-list head — block i links block i + 1, the last
 * block links the old head, and the chunk base becomes the new head. The
 * corpus parks the (possibly NULL) chunk in slot[watermark] before the
 * success check and strides its slot array blindly — the port bounds the
 * window (deviation) — and the counter never resets (the corpus's destroy
 * leaves DAT_0050651c alone, faithful here). Deviation: corpus nodes are
 * 0x20-byte dwords with the 32-bit link at +0x1c; the port widens the node
 * to eight pointer slots (stride 0x40, link = field 7 at +0x38) so the
 * pointer link stays host-aligned — the crt stream-buffer field-slot model
 * applied here. */
void block_text_token_pool_grow(void)
{
    const uint32_t shift = (uint32_t)((unsigned char)s_token_pool_watermark
                                      & 0x1fu);
    const uint32_t count = 1u << shift;
    const uint32_t node = (uint32_t)(8u * sizeof(void*));
    void* chunk = engine_heap_alloc((long)(count * node));
    const int slot = s_token_pool_watermark;
    if (slot >= 0 && slot < BLOCK_TOKEN_POOL_SLOTS) {
        s_token_pool_slots[slot] = chunk; /* corpus stores NULL on failure */
    }
    if (chunk == NULL) {
        return; /* deviation: the corpus still bumps nothing past here */
    }
    s_token_pool_watermark += 1;
    char* base = (char*)chunk;
    const ptrdiff_t link = 7 * (ptrdiff_t)sizeof(void*);
    for (uint32_t i = 0; i + 1 < count; ++i) {
        *(void**)(base + i * node + link) = base + (i + 1u) * node;
    }
    *(void**)(base + (count - 1u) * node + link) = s_token_pool_head;
    s_token_pool_head = chunk;
}

void block_text_token_pool_destroy(void)
{
    for (int i = 0; i < BLOCK_TOKEN_POOL_SLOTS; ++i) {
        if (s_token_pool_slots[i] != NULL) {
            engine_heap_free(s_token_pool_slots[i]);
            s_token_pool_slots[i] = NULL; /* deviation: corpus dangles */
        }
    }
    s_token_pool_head = NULL;
    s_token_pool_resident = 0;
}

/* BlockTextTokenPoolInit (FUN_00478df0): zero the 32-slot window — the
 * corpus's window runs through DAT_00506518, so its head word gets a final
 * explicit store too — clear the watermark counter, then raise the resident
 * flag (`_DAT_00506520 = 1`, the port's resident-count role). Deviation:
 * the port keeps slots and head as separate statics, so the window ends at
 * the slots and the head store stands on its own. */
void block_text_token_pool_init(void)
{
    memset(s_token_pool_slots, 0, sizeof s_token_pool_slots);
    s_token_pool_head = NULL;
    s_token_pool_watermark = 0;
    s_token_pool_resident = 1;
}

void block_text_token_pool_slot_set(int slot, void* token)
{
    if (slot >= 0 && slot < BLOCK_TOKEN_POOL_SLOTS) {
        s_token_pool_slots[slot] = token;
    }
}

void* block_text_token_pool_slot_peek(int slot)
{
    return (slot >= 0 && slot < BLOCK_TOKEN_POOL_SLOTS)
               ? s_token_pool_slots[slot]
               : NULL;
}

void block_text_token_pool_head_set(void* head)
{
    s_token_pool_head = head;
}

void* block_text_token_pool_head_peek(void)
{
    return s_token_pool_head;
}

int block_text_token_pool_resident_peek(void)
{
    return s_token_pool_resident;
}

int block_text_token_pool_watermark_peek(void)
{
    return s_token_pool_watermark;
}

/* ConfigRgbVectorRead (FUN_00479170) — see block.hpp. The corpus frees its
 * heap copy of the value on every path; the port reads the tree storage
 * (deviation) and splits a stack copy with FUN_00477440's quirks pinned. */
int config_rgb_vector_read(BlockFile* bf, const char* path,
                           float* r, float* g, float* b)
{
    const char* v = block_get_strptr(bf, path);
    if (v == NULL) {
        return 0; /* missing string: the corpus stays with its zeroes */
    }
    char buf[256]; /* deviation: the corpus splits the heap copy blindly */
    snprintf(buf, sizeof buf, "%s", v);

    /* FUN_00477440 over the DAT_004a1cd4 separator, pinned as in sound.*:
     * split in place, keep empty tokens, never test the final character. */
    int n = 0;
    char* tok[3];
    if (buf[0] != '\0') {
        tok[n] = buf;
        ++n;
        for (char* p = buf; p[1] != '\0'; ++p) {
            if (*p == ',') {
                *p = '\0';
                if (n < 3) {
                    tok[n] = p + 1;
                }
                ++n; /* beyond three: counted, untracked */
            }
        }
    }
    if (n != 3) {
        return 0;
    }

    /* FUN_0048db70 integer parse; x87 float10 lands as plain float. */
    char* end = NULL;
    const int vals[3] = { (int)strtol(tok[0], &end, 10),
                          (int)strtol(tok[1], &end, 10),
                          (int)strtol(tok[2], &end, 10) };
    const float scale = 1.0f / 255.0f; /* the corpus's 0.003921569 */
    if (r != NULL) { *r = (float)vals[0] * scale; }
    if (g != NULL) { *g = (float)vals[1] * scale; }
    if (b != NULL) { *b = (float)vals[2] * scale; }
    return 1;
}

/* ReadAngleSettingRadians (FUN_00479110) — see block.hpp. */
float config_angle_radians_get(BlockFile* bf, const char* path)
{
    const float deg = block_get_float(bf, path, 0.0f);
    if (deg == 0.0f) {
        return 0.0f; /* the corpus gates the multiply on value != 0 */
    }
    return deg * (1.0f / 360.0f) * 6.2831855f;
}
