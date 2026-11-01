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
#include "sound.hpp"
#include "string.hpp"

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

/* ConfigBooleanParse compares these file-backed tokens directly. Their
 * contents and addresses are confirmed by the mapped DAT inventory and
 * decompile references. */
// GLOBAL: LEGORR 0x004aabec
static const char s_config_bool_yes[] = "YES";
// GLOBAL: LEGORR 0x004aabe4
static const char s_config_bool_true[] = "TRUE";
// GLOBAL: LEGORR 0x004a4124
static const char s_config_bool_on[] = "ON";
// GLOBAL: LEGORR 0x004aabe0
static const char s_config_bool_no[] = "NO";
// GLOBAL: LEGORR 0x004aabd8
static const char s_config_bool_false[] = "FALSE";
// GLOBAL: LEGORR 0x004a4120
static const char s_config_bool_off[] = "OFF";
// GLOBAL: LEGORR 0x004a1cd4
static const char s_config_rgb_separator[] = ":";

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
        t = (char*)strchr(t, '\0');

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

// FUNCTION: LEGORR 0x00478e60
BlockFile* block_open(const char* path)
{
    int32_t length = 0;
    char* text = (char*)read_whole_file(path, &length);
    BlockFile* bf;
    if (text == NULL || length < 0) {
        if (text != NULL) {
            engine_heap_free(text);
        }
        return NULL;
    }
    bf = block_parse(text, (size_t)length);
    engine_heap_free(text);
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

/* Path lookup. The corpus builds queries with the exe name as the first
 * token and `::` as the group separator ("LegoRR Main Menu::MainMenuFull
 * MenuCount" — DAT_004aab18 = "::", finding 23), so a segment boundary is
 * either a space or a "::" run; matching is otherwise unchanged. */
// FUNCTION: LEGORR 0x004792e0
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
        p += strspn(p, " ");
        while (*p == ':' && p[1] == ':') {
            p += 2;
            p += strspn(p, " ");
        }
        if (*p == '\0') {
            break;
        }

        const char* s = p;
        const char* space_end = p + strcspn(p, " ");
        const char* group_end = strstr(p, "::");
        if (group_end != NULL && group_end < space_end) {
            p = group_end;
        } else {
            p = space_end;
        }
        size_t len = (size_t)(p - s);

        /* descend: candidates = children of cur (or roots for the first
         * segment), document order, first match wins */
        BlockRec* cand = (cur == NULL) ? bf->roots : cur->child;
        BlockRec* hit = NULL;
        while (cand != NULL) {
            int matches = 0;
            if (cand->name != NULL) {
                const char* star = first ? strchr(cand->name, '*') : NULL;
                if (star != NULL) {
                    size_t prefix_len = (size_t)(star - cand->name);
                    matches = len >= prefix_len
                        && strnicmp_lite(s, cand->name, prefix_len) == 0;
                } else {
                    matches = strlen(cand->name) == len
                        && strnicmp_lite(s, cand->name, len) == 0;
                }
            }
            if (matches) {
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

// FUNCTION: LEGORR 0x00478ff0
BlockRec* block_subgroup(BlockFile* bf, const char* path)
{
    BlockRec* r = block_find(bf, path);
    if (r == NULL || r->child == NULL) {
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
// FUNCTION: LEGORR 0x00479020
BlockRec* block_next_equal_key(const BlockRec* r)
{
    return r != NULL ? r->sibling : NULL;
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

// FUNCTION: LEGORR 0x004790b0
const char* block_get_strptr(BlockFile* bf, const char* path)
{
    BlockRec* r = block_find(bf, path);
    return r == NULL ? NULL : r->value;
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

/* ConfigBooleanParse (FUN_00477710): YES/TRUE/ON → 1; NO/FALSE/OFF → 0;
 * otherwise 2. */
// FUNCTION: LEGORR 0x00477710
int block_get_bool(const char* value)
{
    if (value == NULL) return 2;
    if (strcasecmp_lite(value, s_config_bool_yes) == 0
        || strcasecmp_lite(value, s_config_bool_true) == 0
        || strcasecmp_lite(value, s_config_bool_on) == 0) {
        return 1;
    }
    if (strcasecmp_lite(value, s_config_bool_no) == 0
        || strcasecmp_lite(value, s_config_bool_false) == 0
        || strcasecmp_lite(value, s_config_bool_off) == 0) {
        return 0;
    }
    return 2;
}

/* Path convenience used by the portable callers. Retail's two-argument
 * ConfigBooleanGetDefault2 at 0x4790d0 copies the value, parses it, and frees
 * the copy; the BlockFile tree already owns a stable value string. */
int block_get_bool(BlockFile* bf, const char* path)
{
    const char* value = block_get_strptr(bf, path);
    return value != NULL ? block_get_bool(value) : 2;
}

/* RegistryPathSplit (FUN_0048b330, finding 402): the corpus copies the
 * leading backslash component, terminates both ends, and skips one
 * separator. The registry's 100-byte caller buffer bound is the
 * caller's business (finding 402 notes the helper itself is unbounded —
 * reproduced, caller checks live where the registry lands). */
// FUNCTION: LEGORR 0x0048b330
const char* registry_path_split(const char* path, char* out)
{
    if (path == NULL || out == NULL) {
        return path; /* deviation: pinned by test_block.cpp — corpus strides blind */
    }
    /* Corpus shape: NUL the destination, seed the character, gate entry on
     * it, then a do-while whose back edge is the character's own test —
     * the source advances before the byte is stored. */
    *out = '\0';
    char c = *path;
    if (c != '\0') {
        do {
            if (c == '\\') {
                break;
            }
            ++path;
            *out = c;
            ++out;
            c = *path;
        } while (c != '\0');
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
#if defined(LEGORRR_BACKEND_WINAPI)
/* The destroy loop's bound operand is offset s_token_pool_slots + 80h, so
 * the head cell must sit immediately after the 0x80-byte slot table (the
 * corpus layout at DAT_00506518) for that operand to resolve through
 * s_token_pool_head's own annotation instead of some unrelated neighbor.
 * Pin both into one named data segment, declaration-order adjacent like
 * the frontend menu band; the zero initializers match the zero-filled
 * corpus tail for datacmp. */
#pragma data_seg(".data$tokpool")
#endif
// GLOBAL: LEGORR 0x00506498
static void* s_token_pool_slots[BLOCK_TOKEN_POOL_SLOTS] = { 0 };
// GLOBAL: LEGORR 0x00506518
static void* s_token_pool_head = 0;  /* DAT_00506518 role: free-list head */
#if defined(LEGORRR_BACKEND_WINAPI)
#pragma data_seg()
#endif
// GLOBAL: LEGORR 0x00506520
static int   s_token_pool_resident;  /* _DAT_00506520 role */
// GLOBAL: LEGORR 0x0050651c
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
// FUNCTION: LEGORR 0x00479490
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

// FUNCTION: LEGORR 0x00478e20
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void block_text_token_pool_destroy(void)
{
    __asm {
        push esi
        mov esi, offset s_token_pool_slots
    block_token_pool_destroy_loop:
        mov eax, dword ptr [esi]
        test eax, eax
        je block_token_pool_destroy_next
        push eax
        call free
        add esp, 4
    block_token_pool_destroy_next:
        add esi, 4
        cmp esi, offset s_token_pool_slots + 80h
        jb block_token_pool_destroy_loop
        mov dword ptr [s_token_pool_head], 0
        mov dword ptr [s_token_pool_resident], 0
        pop esi
        ret
    }
}
#else
void block_text_token_pool_destroy(void)
{
    /* Corpus walk: the 32-slot window is walked pointer-wise from
     * DAT_00506498 up to the head cursor word at DAT_00506518
     * (add esi, 0x4 / cmp esi, 0x506518 / jb), freeing each non-null chunk
     * and leaving the slot dangling. Plain pointer operands keep that
     * loop-back unsigned (jb, not jl). */
    void** slot = s_token_pool_slots;
    void** end = s_token_pool_slots + BLOCK_TOKEN_POOL_SLOTS;
    for (; slot < end; ++slot) {
        if (*slot != NULL) {
            engine_heap_free(*slot);
            *slot = NULL; /* deviation: corpus dangles */
        }
    }
    s_token_pool_head = NULL;
    s_token_pool_resident = 0;
}
#endif

/* BlockTextTokenPoolInit (FUN_00478df0): zero the 32-slot window — the
 * corpus's window runs through DAT_00506518, so its head word gets a final
 * explicit store too — clear the watermark counter, then raise the resident
 * flag (`_DAT_00506520 = 1`, the port's resident-count role). Deviation:
 * the port keeps slots and head as separate statics, so the window ends at
 * the slots and the head store stands on its own. */
// FUNCTION: LEGORR 0x00478df0
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
// FUNCTION: LEGORR 0x00479170
int config_rgb_vector_read(BlockFile* bf, const char* path,
                           float* r, float* g, float* b)
{
    const char* v = block_get_strptr(bf, path);
    if (v == NULL) {
        return 0; /* missing string: the corpus stays with its zeroes */
    }
    char buf[256]; /* deviation: the corpus splits the heap copy blindly */
    snprintf(buf, sizeof buf, "%s", v);

    /* FUN_00477440 splits on the mapped colon separator, keeps empty tokens,
     * and never tests the final character. The bounded copy can contain at
     * most 255 tokens, so this array safely holds every split pointer. */
    char* tok[256];
    const int n = sound_variant_split(buf, tok, s_config_rgb_separator);
    if (n != 3) {
        return 0;
    }

    /* VC5 _atoi parses each component before the x87 float scaling. */
    const int vals[3] = { atoi(tok[0]), atoi(tok[1]), atoi(tok[2]) };
    const float scale = 1.0f / 255.0f; /* the corpus's 0.003921569 */
    if (r != NULL) { *r = (float)vals[0] * scale; }
    if (g != NULL) { *g = (float)vals[1] * scale; }
    if (b != NULL) { *b = (float)vals[2] * scale; }
    return 1;
}

/* ReadAngleSettingRadians (FUN_00479110): the corpus tests the path getter,
 * calls it again for a present value, and sends either that string or the
 * empty literal through the CRT `atof` entry before the zero gate and two x87 scales. */
/* These are the corpus's direct x87 operands in ReadAngleSettingRadians.
 * PE bytes: 00 00 00 00, 61 0b 36 3b, db 0f c9 40 respectively. */
// GLOBAL: LEGORR 0x0049e8f8
static const float s_config_angle_zero = 0.0f;
// GLOBAL: LEGORR 0x0049e8fc
static const float s_config_angle_degrees_to_turn = 1.0f / 360.0f;
// GLOBAL: LEGORR 0x0049e900
static const float s_config_angle_turn_radians = 6.2831855f;
// GLOBAL: LEGORR 0x004b8a14
static const char s_config_angle_empty[4] = { 0, 0, 0, 0 };

// FUNCTION: LEGORR 0x00479110
float config_angle_radians_get(BlockFile* bf, const char* path)
{
    const float deg = (float)atof(block_get_strptr(bf, path) != NULL
                                  ? block_get_strptr(bf, path)
                                  : s_config_angle_empty);
    if (deg != s_config_angle_zero) {
        return (deg * s_config_angle_degrees_to_turn) * s_config_angle_turn_radians;
    }
    return s_config_angle_zero;
}
