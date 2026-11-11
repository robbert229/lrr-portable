// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — LWO model loader + .uv sidecar (see model.hpp).
 *
 * Ported against the rrlwo.py reference (corpus-validated over all shipped
 * models): chunk walk, POLS origin auto-detect, SRFS names, SURF records
 * anchored on SRFS names (including the renderer's color, texture, flag,
 * mapping, and material-value fields), and the finding 19 `.uv` grammar. */
#include "model.hpp"
#include "crt/crt.hpp"
#include "heap.hpp"
#include "resource.hpp"
#include "string.hpp"
#include "surface.hpp"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#if defined(_MSC_VER)
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

/* The reference keeps two adjacent 8-byte-stride path/record pair tables,
 * 0x1f40 bytes apart: 1000 entries each, scanned case-insensitively through
 * the shared pair-table helpers (FUN_00488730/FUN_00488790).  The primary
 * table (DAT_00540838, count DAT_005446bc) keys on the joined request path;
 * the secondary table (DAT_00542778, count DAT_005446c0) keys on the raw
 * path and is consulted under the fallback prefix (DAT_005446b8). */
#define MODEL_RESOURCE_CACHE_CAPACITY 1000u
typedef struct ModelResourcePair {
    char* key;
    void* value;
} ModelResourcePair;
static ModelResourcePair
    s_model_resource_primary[MODEL_RESOURCE_CACHE_CAPACITY];
// GLOBAL: LEGORR 0x005446bc
static int s_model_resource_primary_count;
static ModelResourcePair
    s_model_resource_secondary[MODEL_RESOURCE_CACHE_CAPACITY];
// GLOBAL: LEGORR 0x005446c0
static int s_model_resource_secondary_count;
/* DAT_005446b8: the fallback prefix the resolver retries with; installed by
 * the port through model_resource_fallback_prefix_set. */
static const char* s_model_resource_fallback_prefix;
/* Portable notification for a parsed-model cache hit. It is not the
 * FUN_00480900 source record: that routine expects a 13-dword resource
 * template with per-entry 0x70-byte renderer records, absent from Model. */
typedef void (*ModelResourceInstanceHook)(void* record, void* flags);
static ModelResourceInstanceHook s_model_resource_instance_hook;
void model_resource_instance_hook_set(ModelResourceInstanceHook hook)
{
    s_model_resource_instance_hook = hook;
}
void model_resource_fallback_prefix_set(const char* prefix)
{
    s_model_resource_fallback_prefix = prefix;
}
const char* model_resource_fallback_prefix_get(void)
{
    return s_model_resource_fallback_prefix;
}

/* FUNCTION: LEGORR 0x00481fa0 */
void mesh_vertex_coordinate_remap(int vertex_count, const float* positions_xyz,
                                 float* coordinates_xy, float scale_x,
                                 float scale_y, float scale_z, float offset_x,
                                 float offset_y, float offset_z, uint32_t flags)
{
    int i;
    if (positions_xyz == NULL || coordinates_xy == NULL) return;
    for (i = 0; i < vertex_count; ++i) {
        const float x = positions_xyz[i * 3];
        const float y = positions_xyz[i * 3 + 1];
        const float z = positions_xyz[i * 3 + 2];
        coordinates_xy[i * 2] =
            ((flags & 1u) != 0u ? z - offset_z : x - offset_x) /
                ((flags & 1u) != 0u ? scale_z : scale_x) + 0.5f;
        coordinates_xy[i * 2 + 1] =
            ((flags & 2u) != 0u ? z - offset_z : y - offset_y) /
                ((flags & 2u) != 0u ? scale_z : scale_y) + 0.5f;
    }
}

#if !(defined(_MSC_VER) && (_MSC_VER <= 1100)) \
    && !(defined(__cplusplus) && (__cplusplus >= 201103L))
#error "C++11"
#endif
static_assert(sizeof(float) == 4, "model loader reads IEEE-754 binary32");

/* --- big-endian readers ---------------------------------------------------- */

static uint16_t be16(const uint8_t* p) { return (uint16_t)(((uint16_t)p[0] << 8) | p[1]); }
static uint32_t be32(const uint8_t* p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}
static float bef32(const uint8_t* p)
{
    uint32_t u = be32(p);
    float f;
    memcpy(&f, &u, sizeof f);
    return f;
}

/* LwoReadBigEndianFloat (corpus FUN_0048c1d0): consume four stream bytes
 * at the cursor, byte-reverse them into `out`, advance the cursor, answer
 * 1. The corpus's LwoPoints/LwoSurfaceSettings readers call it for every
 * big-endian float field; byte-reversal is exactly be32 + host store. */
// FUNCTION: LEGORR 0x0048c1d0
int lwo_read_big_endian_float(const uint8_t** cursor, uint8_t out[4])
{
    out[3] = **cursor;
    ++*cursor;
    out[2] = **cursor;
    ++*cursor;
    out[1] = **cursor;
    ++*cursor;
    out[0] = **cursor;
    ++*cursor;
    return 1;
}

/* --- growable byte buffer --------------------------------------------------- */

typedef struct Buf {
    uint8_t* d;
    size_t   len, cap;
} Buf;

static int buf_reserve(Buf* b, size_t need)
{
    if (need <= b->cap)
        return 1;
    size_t ncap = b->cap ? b->cap * 2 : 64;
    while (ncap < need)
        ncap *= 2;
    uint8_t* p = (uint8_t*)realloc(b->d, ncap);
    if (!p)
        return 0;
    b->d = p;
    b->cap = ncap;
    return 1;
}
static int buf_append(Buf* b, const void* src, size_t n)
{
    if (!buf_reserve(b, b->len + n))
        return 0;
    if (n)
        memcpy(b->d + b->len, src, n);
    b->len += n;
    return 1;
}
static void buf_free(Buf* b)
{
    free(b->d);
    b->d = NULL;
    b->len = b->cap = 0;
}

/* --- parsing context --------------------------------------------------------- */

typedef struct Chunk {
    char           tag[5]; /* 4 bytes, trailing NULs stripped (latin1 rstrip) */
    const uint8_t* pay;
    size_t         size; /* clamped to the buffer tail */
} Chunk;

typedef struct SurfRec {
    char*  name;   /* borrowed from sname[]: the SRFS name the anchor matched */
    size_t blocks_cap;
    char   (*tags)[5];
    size_t*  off;
    size_t*  size;
    size_t   nblocks;
    Buf    blocks_data; /* payloads, appended in file order */
} SurfRec;

/* The retail POLS handler grows a three-word polygon table.  The portable
 * public Model adds UV and winding fields, so this compact parse record keeps
 * the stream result intact until that view is built. */
typedef struct PolyRec {
    uint32_t count;
    uint32_t surface;
    uint32_t ring_pointer;
} PolyRec;

typedef struct Parse {
    const uint8_t* d;
    size_t         n;

    float*  verts; /* host-order PNTS coordinates, replaced by later chunks */
    size_t  nverts;
    PolyRec* polys;
    size_t   npolys;
    int16_t** poly_rings; /* host-width sidecar for the retail pointer word */
    int     saw_zero;
    size_t  pols_used_len; /* first POLS chunk only (python one()) */

    char**  sname;
    size_t  nsname, scap;

    SurfRec* recs;
    size_t   nrecs, rcap;
} Parse;

/* --- helpers ----------------------------------------------------------------- */

static size_t strnlen_n(const char* s, size_t maxlen)
{
    const char* terminator = (const char*)memchr(s, '\0', maxlen);
    return terminator != NULL ? (size_t)(terminator - s) : maxlen;
}

static int tag_equal_ci(const uint8_t* tag, const char* expected)
{
    for (int i = 0; i < 4; ++i) {
        unsigned char a = tag[i];
        unsigned char b = (unsigned char)expected[i];
        if (a >= 'a' && a <= 'z')
            a = (unsigned char)(a - 'a' + 'A');
        if (b >= 'a' && b <= 'z')
            b = (unsigned char)(b - 'a' + 'A');
        if (a != b)
            return 0;
    }
    return 1;
}

/* strip ASCII whitespace both ends (corpus sidecar/TIMG lines are ASCII) */
static char* dup_strip(const char* src, size_t len)
{
    size_t a = 0, b = len;
    while (a < b && isspace((unsigned char)src[a]))
        a++;
    while (b > a && isspace((unsigned char)src[b - 1]))
        b--;
    char* n = (char*)malloc(b - a + 1);
    if (!n)
        return NULL;
    memcpy(n, src + a, b - a);
    n[b - a] = 0;
    return n;
}

/* LwoStringDuplicate allocates a zero-filled, even-sized copy. */
static char* dup_lwo_string(const char* src, size_t len)
{
    size_t bytes = len + 1u;
    char* copy;
    if (bytes == 0u) return NULL;
    if (bytes & 1u) ++bytes;
    copy = (char*)malloc(bytes);
    if (copy == NULL) return NULL;
    memset(copy, 0, bytes);
    if (len != 0u) memcpy(copy, src, len);
    return copy;
}

static int dup_lwo_assign(char** dst, const char* src, size_t len)
{
    char* copy = dup_lwo_string(src, len);
    if (copy == NULL) return 0;
    free(*dst);
    *dst = copy;
    return 1;
}

static size_t lwo_sequence_marker_find(const char* text, size_t size)
{
    static const char marker[] = "(sequence)";
    const size_t marker_size = sizeof(marker) - 1u;
    size_t offset;
    if (size < marker_size) return size;
    for (offset = 0u; offset <= size - marker_size; ++offset) {
        size_t i;
        for (i = 0u; i < marker_size; ++i) {
            unsigned char a = (unsigned char)text[offset + i];
            unsigned char b = (unsigned char)marker[i];
            if (a >= 'A' && a <= 'Z') a = (unsigned char)(a + ('a' - 'A'));
            if (b >= 'A' && b <= 'Z') b = (unsigned char)(b + ('a' - 'A'));
            if (a != b) break;
        }
        if (i == marker_size) return offset;
    }
    return size;
}

static int dup_assign(char** dst, const char* src, size_t len)
{
    char* n = dup_strip(src, len);
    if (!n)
        return 0;
    free(*dst);
    *dst = n;
    return 1;
}

/* --- SRFS: NUL-terminated names, even-padded records --------------------------- */

// FUNCTION: LEGORR 0x0048c690
static int parse_srfs(Parse* P, const uint8_t* pay, size_t size)
{
    size_t p = 0;
    /* Each retail SRFS dispatch allocates a fresh surface-name table and
     * replaces the caller's pointer/count. Earlier tables are leaked by the
     * binary; release the old names here while preserving the replacement
     * semantics. */
    for (size_t i = 0; i < P->nsname; ++i)
        free(P->sname[i]);
    free(P->sname);
    P->sname = NULL;
    P->nsname = 0;
    P->scap = 0;
    while (p < size) {
        const uint8_t* e = (const uint8_t*)memchr(pay + p, 0, size - p);
        if (!e)
            return 0;
        /* LwoSurfaceNames duplicates through the NUL, without trimming. */
        size_t length = (size_t)(e - (pay + p));
        char* nm = dup_lwo_string((const char*)(pay + p), length);
        if (!nm)
            return 0;
        if (P->nsname >= ((size_t)-1) / sizeof(*P->sname) - 1u) {
            free(nm);
            return 0;
        }
        char** names = (char**)malloc((P->nsname + 1u) * sizeof(*names));
        if (names == NULL) { free(nm); return 0; }
        if (P->nsname != 0u)
            memcpy(names, P->sname, P->nsname * sizeof(*names));
        names[P->nsname] = nm;
        free(P->sname);
        P->sname = names;
        P->nsname++;
        P->scap = P->nsname;
        p = ((size_t)(e - pay) + 2) & ~(size_t)1;
        if (p > size)
            return 0;
    }
    return 1;
}

/* --- POLS --------------------------------------------------------------------- */

/* LwoPolygons (FUN_0048c3e0) stores one 3-word record per polygon:
 * {u32 vertex count, u32 surface index, pointer word}. The table is expanded
 * by allocating a new (count + 1) 3-word record block, copying the old
 * records, then freeing the old block. Host ring pointers live in a sidecar.
 * The on-disk ring and surface words are
 * big-endian; surface is decremented with 16-bit wrap. */
// FUNCTION: LEGORR 0x0048c3e0
static int parse_pols(Parse* P, const uint8_t* d, size_t size)
{
    size_t p = 0;
    /* The FORM walker dispatches every POLS chunk to the same three output
     * fields. Retail starts its polygon index at zero for each dispatch, so a
     * later POLS chunk replaces the previous polygon table and count instead
     * of appending to it. Release the previous portable table before parsing
     * this chunk; the retail code leaks the superseded allocation. */
    for (size_t i = 0; i < P->npolys; ++i)
        free(P->poly_rings[i]);
    free(P->polys);
    free(P->poly_rings);
    P->polys = NULL;
    P->poly_rings = NULL;
    P->npolys = 0;
    P->saw_zero = 0;
    while (p < size) {
        /* The retail reader works from a size + 0x10 scratch buffer and does
         * not validate each read.  Keep the equivalent well-formed record
         * handling while refusing to read beyond this caller-owned buffer. */
        if (size - p < 2)
            return 0;
        size_t cnt = (uint16_t)((uint16_t)d[p] * 0x100u + d[p + 1]);
        p += 2;
        if (cnt > (size - p) / 2 || size - p - cnt * 2 < 2)
            return 0;
        if (P->npolys >= ((size_t)-1) / sizeof(*P->poly_rings) - 1)
            return 0;
        PolyRec* grown = (PolyRec*)malloc((P->npolys + 1u) * sizeof(*grown));
        int16_t** ring_sidecar = (int16_t**)malloc(
            (P->npolys + 1u) * sizeof(*ring_sidecar));
        if (grown == NULL || ring_sidecar == NULL) {
            free(grown);
            free(ring_sidecar);
            return 0;
        }
        if (P->npolys != 0u) {
            memcpy(grown, P->polys, P->npolys * sizeof(*grown));
            memcpy(ring_sidecar, P->poly_rings,
                   P->npolys * sizeof(*ring_sidecar));
        }
        free(P->polys);
        free(P->poly_rings);
        P->polys = grown;
        P->poly_rings = ring_sidecar;

        PolyRec* poly = &P->polys[P->npolys];
        int16_t* ring = (int16_t*)malloc(cnt * sizeof(*ring));
        poly->count = (uint32_t)cnt;
        poly->surface = (uint32_t)(uint16_t)(
            (uint16_t)((uint16_t)d[p + 2 * cnt] * 0x100u +
                       d[p + 2 * cnt + 1]) - 1u);
        poly->ring_pointer = (uint32_t)(uintptr_t)ring;
        P->poly_rings[P->npolys] = ring;
        if (ring == NULL && cnt != 0u)
            return 0;
        for (size_t k = 0; k < cnt; k++) {
            ring[k] = (int16_t)((uint16_t)d[p + 2 * k] * 0x100u +
                                d[p + 2 * k + 1]);
            if (ring[k] == 0)
                P->saw_zero = 1;
        }
        p += 2 * cnt;
        p += 2;
        P->npolys++;
    }
    return 1;
}

/* The retail CRVS handler consumes records shaped as
 * {u16 index_count, index_count * u16, two trailing u16 values}, then frees
 * its temporary copy without retaining curve data. A truncated final record
 * is bounded here and discarded; the retail scratch buffer's zero tail lets
 * it finish scanning without an out-of-bounds read. */
static int parse_curves_payload(const uint8_t* data, size_t size)
{
    size_t offset = 0u;
    if (data == NULL && size != 0u) return 1;
    while (size - offset >= 2u) {
        size_t index_count = (size_t)be16(data + offset);
        size_t index_bytes;
        offset += 2u;
        if (index_count > ((size_t)-1) / 2u) break;
        index_bytes = index_count * 2u;
        if (index_bytes > size - offset) break;
        offset += index_bytes;
        if (size - offset < 4u) break;
        offset += 4u;
    }
    return 1;
}

/* --- SURF ---------------------------------------------------------------------- */

static int surfrec_reserve(Parse* P)
{
    if (P->nrecs + 1 <= P->rcap)
        return 1;
    size_t ncap = P->rcap ? P->rcap * 2 : 8;
    SurfRec* nr = (SurfRec*)realloc(P->recs, ncap * sizeof *nr);
    if (!nr)
        return 0;
    memset(nr + P->rcap, 0, (ncap - P->rcap) * sizeof *nr);
    P->recs = nr;
    P->rcap = ncap;
    return 1;
}

static int surfrec_addblock(Parse* P, SurfRec* r, const char* tag,
                            const uint8_t* pay, size_t size)
{
    size_t off = r->blocks_data.len;
    if (!buf_append(&r->blocks_data, pay, size))
        return 0;
    /* Keep every subchunk occurrence in file order. Most settings overwrite
     * earlier values, but some retail paths accumulate repeated TFLG bits or
     * remember a sequence marker from any TIMG occurrence. block_find() can
     * still select the last occurrence for ordinary replacement fields. */
    if (r->nblocks == r->blocks_cap) {
        if (r->blocks_cap > ((size_t)-1) / 2 / sizeof *r->tags)
            return 0;
        size_t ncap = r->blocks_cap ? r->blocks_cap * 2 : 8;
        char (*nt)[5] = (char (*)[5])malloc(ncap * sizeof *r->tags);
        size_t* no = (size_t*)malloc(ncap * sizeof *r->off);
        size_t* ns = (size_t*)malloc(ncap * sizeof *r->size);
        if (!nt || !no || !ns) {
            free(nt);
            free(no);
            free(ns);
            return 0;
        }
        if (r->nblocks != 0) {
            memcpy(nt, r->tags, r->nblocks * sizeof *r->tags);
            memcpy(no, r->off, r->nblocks * sizeof *r->off);
            memcpy(ns, r->size, r->nblocks * sizeof *r->size);
        }
        free(r->tags);
        free(r->off);
        free(r->size);
        r->tags = nt;
        r->off = no;
        r->size = ns;
        r->blocks_cap = ncap;
    }
    memcpy(r->tags[r->nblocks], tag, 5);
    r->off[r->nblocks] = off;
    r->size[r->nblocks] = size;
    r->nblocks++;
    return 1;
}

static int block_find(const SurfRec* r, const char* tag, const uint8_t** out, size_t* nout)
{
    int found = 0;
    const uint8_t* found_data = NULL;
    size_t found_size = 0;
    for (size_t i = 0; i < r->nblocks; i++) {
        if (strcasecmp(r->tags[i], tag) == 0) {
            /* LwoSurfaceSettings walks every subchunk and assigns fields as
             * it goes, so a repeated tag replaces the prior value. */
            found_data = r->blocks_data.d + r->off[i];
            found_size = r->size[i];
            found = 1;
        }
    }
    if (found) {
        *out = found_data;
        *nout = found_size;
    }
    return found;
}

/* LwoSurfaceSettings is called once for each SURF chunk. Its payload starts
 * with one NUL-terminated surface name, even-padded, then subchunks of the
 * form {4-byte tag, u16 size, payload, even pad}. */
// FUNCTION: LEGORR 0x0048c820
static int parse_surf_chunk(Parse* P, const uint8_t* d, size_t size)
{
    const uint8_t* terminator;
    size_t name_length, q;
    char* name;
    SurfRec* r;

    if (size == 0)
        return 0;
    terminator = (const uint8_t*)memchr(d, 0, size);
    if (terminator == NULL)
        return 0;
    name_length = (size_t)(terminator - d);
    name = dup_lwo_string((const char*)d, name_length);
    if (name == NULL)
        return 0;
    if (!surfrec_reserve(P)) {
        free(name);
        return 0;
    }
    r = &P->recs[P->nrecs];
    memset(r, 0, sizeof *r);
    r->name = name;

    q = (name_length + 2) & ~(size_t)1;
    if (q > size) {
        ++P->nrecs;
        return 0;
    }
    while (q + 6 <= size) {
        const uint8_t* bt = d + q;
        size_t bsz = be16(d + q + 4);
        char tag[5];
        if (bsz > size - q - 6 ||
            (bsz & 1u) > size - q - 6 - bsz) {
            ++P->nrecs;
            return 0;
        }
        memcpy(tag, bt, 4);
        tag[4] = 0;
        {
            if (!surfrec_addblock(P, r, tag, d + q + 6, bsz)) {
                /* Include the partially populated record in cleanup. */
                ++P->nrecs;
                return 0;
            }
        }
        q += 6 + bsz + (bsz & 1);
    }
    if (q != size) {
        ++P->nrecs;
        return 0;
    }
    ++P->nrecs;
    return 1;
}

/* --- model build ----------------------------------------------------------------- */

static Model* build_model(Parse* P)
{
    Model* m = (Model*)calloc(1, sizeof *m);
    if (!m)
        return NULL;
    m->n_verts = P->nverts;
    m->verts = P->verts;
    P->verts = NULL;
    P->nverts = 0;

    size_t np = P->npolys;
    m->n_polys = np;
    if (np) {
        m->polys = (ModelPoly*)calloc(np, sizeof(ModelPoly));
        if (!m->polys) { model_free(m); return NULL; }
    }
    const int base = P->saw_zero ? 0 : 1;
    for (size_t i = 0; i < np; i++) {
        const PolyRec* raw_poly = &P->polys[i];
        uint16_t cnt = (uint16_t)raw_poly->count;
        ModelPoly* p = &m->polys[i];
        p->count = cnt;
        p->ring = (uint16_t*)malloc((cnt ? cnt : 1) * sizeof(uint16_t));
        if (!p->ring) { model_free(m); return NULL; }
        const int16_t* raw = P->poly_rings[i];
        for (uint16_t k = 0; k < cnt; k++) {
            int v = raw[k] < 0 ? -raw[k] : raw[k];
            v -= base;
            p->ring[k] = (uint16_t)(v < 0 ? 0 : v);
            if (raw[k] < 0)
                p->flipped = 1;
        }
        p->surface = (uint16_t)raw_poly->surface;
    }

    m->n_surfaces = P->nsname;
    if (P->nsname) {
        m->surfaces = (ModelSurface*)calloc(P->nsname, sizeof(ModelSurface));
        if (!m->surfaces) { model_free(m); return NULL; }
        for (size_t i = 0; i < P->nsname; i++) {
            size_t length = strlen(P->sname[i]);
            m->surfaces[i].name = (char*)malloc(length + 1);
            if (m->surfaces[i].name == NULL) {
                model_free(m);
                return NULL;
            }
            memcpy(m->surfaces[i].name, P->sname[i], length + 1);
        }
    }

    /* Each SURF chunk produces a fresh record. The retail loader scans those
     * records from the beginning for each SRFS name and copies the first
     * case-insensitive match; later records with that name are ignored. */
    for (size_t r = 0; r < P->nrecs; r++) {
        const SurfRec* rec = &P->recs[r];
        const uint8_t* bp;
        size_t bs;
        int earlier_same_name = 0;
        for (size_t prior = 0; prior < r; ++prior) {
            if (strcasecmp(P->recs[prior].name, rec->name) == 0) {
                earlier_same_name = 1;
                break;
            }
        }
        if (earlier_same_name)
            continue;
        for (size_t i = 0; i < P->nsname; i++) {
            if (strcasecmp(P->sname[i], rec->name) != 0)
                continue;
            ModelSurface* s = &m->surfaces[i];
            free(s->timg);
            free(s->texture_definition);
            {
                char* surface_name = s->name;
                memset(s, 0, sizeof *s);
                s->name = surface_name;
            }
            if (block_find(rec, "COLR", &bp, &bs) && bs >= 3) {
                for (int c = 0; c < 3; c++)
                    s->colr[c] = (float)((double)bp[c] / 255.0); /* == python c/255 */
                s->colr[3] = 1.0f;
                s->colr_bytes[0] = bp[0];
                s->colr_bytes[1] = bp[1];
                s->colr_bytes[2] = bp[2];
                s->colr_bytes[3] = bs >= 4 ? bp[3] : 255;
                s->has_colr = 1;
            }
            if (block_find(rec, "TIMG", &bp, &bs)) {
                size_t n = strnlen_n((const char*)bp, bs);
                size_t marker = lwo_sequence_marker_find((const char*)bp, n);
                for (size_t bi = 0; bi < rec->nblocks; ++bi) {
                    if (strcasecmp(rec->tags[bi], "TIMG") == 0) {
                        const uint8_t* image_data =
                            rec->blocks_data.d + rec->off[bi];
                        size_t image_size = rec->size[bi];
                        size_t image_length = strnlen_n(
                            (const char*)image_data, image_size);
                        if (lwo_sequence_marker_find(
                                (const char*)image_data, image_length) <
                            image_length) {
                            s->animated_image_sequence = 1;
                        }
                    }
                }
                if (marker < n) {
                    /* 0x48ca7f terminates the byte before the marker. */
                    n = marker ? marker - 1 : 0;
                    s->animated_image_sequence = 1;
                }
                if (!dup_lwo_assign(&s->timg, (const char*)bp, n)) {
                    model_free(m);
                    return NULL;
                }
            }
            {
                int found_texture_flags = 0;
                uint16_t texture_flags = 0;
                for (size_t bi = 0; bi < rec->nblocks; ++bi) {
                    if (strcasecmp(rec->tags[bi], "TFLG") == 0 &&
                        rec->size[bi] >= 2u) {
                        texture_flags |= be16(
                            rec->blocks_data.d + rec->off[bi]);
                        found_texture_flags = 1;
                    }
                }
                if (found_texture_flags) {
                    s->texture_flags |= texture_flags;
                    s->has_texture_flags = 1;
                }
            }
            if (block_find(rec, "FLAG", &bp, &bs) && bs >= 2) {
                s->material_flags = be16(bp);
                s->has_material_flags = 1;
            }
            if (block_find(rec, "TTEX", &bp, &bs) && bs > 0) {
                size_t n = strnlen_n((const char*)bp, bs);
                char* mode = dup_lwo_string((const char*)bp, n);
                if (mode == NULL) { model_free(m); return NULL; }
                s->projection_mode = lwo_texture_projection_mode_parse(mode);
                s->has_projection_mode = 1;
                free(mode);
            }
            if (block_find(rec, "CTEX", &bp, &bs) && bs > 0) {
                size_t n = strnlen_n((const char*)bp, bs);
                if (!dup_lwo_assign(&s->texture_definition,
                                    (const char*)bp, n))
                    { model_free(m); return NULL; }
            }
            if (block_find(rec, "TSIZ", &bp, &bs) && bs >= 8) {
                s->tsiz[0] = bef32(bp);
                s->tsiz[1] = bef32(bp + 4);
                if (bs >= 12)
                    s->tsiz[2] = bef32(bp + 8);
                s->has_tsiz = 1;
            }
            if (block_find(rec, "TCTR", &bp, &bs) && bs >= 12) {
                s->tctr[0] = bef32(bp);
                s->tctr[1] = bef32(bp + 4);
                s->tctr[2] = bef32(bp + 8);
                s->has_tctr = 1;
            }
            {
                static const char* const value_tags[6] = {
                    "LUMI", "DIFF", "SPEC", "REFL", "GLOS", "TRAN"
                };
                float* values[6] = {
                    &s->luminosity, &s->diffuse, &s->specular,
                    &s->reflection, &s->glossiness, &s->transparency
                };
                int value_index;
                for (value_index = 0; value_index < 6; ++value_index) {
                    if (block_find(rec, value_tags[value_index], &bp, &bs) && bs >= 2) {
                        *values[value_index] = (float)be16(bp) / 256.0f;
                        s->material_values_mask |= (uint8_t)(1u << value_index);
                    }
                }
            }
        }
    }
    return m;
}

/* --- parse entry -------------------------------------------------------------------- */

static void parse_free(Parse* P)
{
    free(P->verts);
    for (size_t poly_index = 0; poly_index < P->npolys; poly_index++)
        free(P->poly_rings[poly_index]);
    free(P->polys);
    free(P->poly_rings);
    for (size_t name_index = 0; name_index < P->nsname; name_index++)
        free(P->sname[name_index]);
    free(P->sname);
    for (int rec_index =  0; rec_index < P->nrecs; rec_index++) {
        free(P->recs[rec_index].name);
        free(P->recs[rec_index].tags);
        free(P->recs[rec_index].off);
        free(P->recs[rec_index].size);
        buf_free(&P->recs[rec_index].blocks_data);
    }
    free(P->recs);
}

/* LwoPoints: replace the current coordinate table with host-order triples.
 * Keeping the handler's output as one allocation mirrors the retail record
 * and avoids building a second copy when the public Model is assembled. */
// FUNCTION: LEGORR 0x0048c210
static int parse_points(Parse* P, const Chunk* c)
{
    if (c->size % 12u != 0u)
        return 0;
    size_t nverts = c->size / 12;
    float* verts = NULL;
    if (nverts != 0) {
        if (nverts > ((size_t)-1) / (3 * sizeof(float)))
            return 0;
        verts = (float*)malloc(nverts * 3 * sizeof(float));
        if (verts == NULL)
            return 0;
        for (size_t i = 0; i < nverts; i++) {
            for (int k = 0; k < 3; k++)
                verts[i * 3 + (size_t)k] = bef32(c->pay + i * 12 + (size_t)k * 4);
        }
    }
    free(P->verts);
    P->verts = verts;
    P->nverts = nverts;
    return 1;
}

// FUNCTION: LEGORR 0x0048d2c0
Model* model_parse(const uint8_t* data, size_t len)
{
    if (data == NULL || len < 12 || !tag_equal_ci(data, "FORM") ||
        !tag_equal_ci(data + 8, "LWOB"))
        return NULL;
    Parse P;
    memset(&P, 0, sizeof P);
    P.d = data;
    P.n = len;

    size_t form_size = (size_t)be32(data + 4);
    if (form_size < 4 || form_size > len - 8)
        return NULL;
    size_t end = 8 + form_size;
    int ok = 1;
    size_t p = 12;
    /* LoadLWO dispatches each chunk as it reads it. Keep that order here:
     * handlers own their allocations immediately, so a later malformed
     * chunk unwinds already parsed state through parse_free(). */
    while (p < end && ok) {
        Chunk c;
        uint32_t size;
        const uint8_t* tag;
        if (end - p < 8) {
            ok = 0;
            break;
        }
        tag = data + p;
        memset(&c, 0, sizeof c);
        memcpy(c.tag, data + p, 4);
        c.tag[4] = 0;
        size = be32(data + p + 4);
        if ((size_t)size > end - p - 8) {
            ok = 0;
            break;
        }
        const uint8_t* pay = data + p + 8;
        c.pay = pay;
        c.size = (size_t)size;
        if (tag_equal_ci(tag, "PNTS")) {
            ok = parse_points(&P, &c);
        }
        else if (tag_equal_ci(tag, "POLS")) {
            ok = parse_pols(&P, c.pay, c.size);
        }
        else if (tag_equal_ci(tag, "CRVS")) {
            ok = parse_curves_payload(c.pay, c.size);
        }
        else if (tag_equal_ci(tag, "SRFS")) {
            ok = parse_srfs(&P, c.pay, c.size);
        }
        else if (tag_equal_ci(tag, "SURF")) {
            ok = parse_surf_chunk(&P, c.pay, c.size);
        }
        p += 8 + (size_t)size;
        if (size & 1) {
            if (p == end) {
                ok = 0;
                break;
            }
            ++p;
        }
    }
    if (p != end)
        ok = 0;
    if (!ok) { parse_free(&P); return NULL; }
    Model* m = build_model(&P);
    parse_free(&P);
    return m;
}

/* --- .uv sidecar (finding 19 §3) ---------------------------------------------------- */

/* python int(): optional sign, digits, token fully consumed */
static int tok_int(const char* s, const char* end, long* out)
{
    const char* p = s;
    if (p != end && (*p == '+' || *p == '-')) {
        p++;
    }
    if (p == end || !isdigit((unsigned char)*p))
        return 0;
    char* parsed_end;
    long value = strtol(s, &parsed_end, 10);
    if (parsed_end != end || value < -(1L << 30) || value > (1L << 30))
        return 0; /* the source parser only accepts this practical range */
    *out = value;
    return 1;
}

static int tok_float(const char* s, const char* end, float* out)
{
    char tmp[128];
    size_t n = (size_t)(end - s);
    if (n == 0 || n >= sizeof tmp)
        return 0;
    memcpy(tmp, s, n);
    tmp[n] = 0;
    char* ep;
    double d = strtod(tmp, &ep);
    if (ep != tmp + n)
        return 0;
    *out = (float)d;
    return 1;
}

/* tokens of a line like python .split(); fills up to maxt pairs */
static int line_tokens(const char* line, const char** ts, const char** te, int maxt)
{
    int n = 0;
    const char* p = line;
    for (;;) {
        p += strspn(p, " \t\n\r\v\f");
        if (!*p)
            break;
        const char* s = p;
        p += strcspn(p, " \t\n\r\v\f");
        if (n < maxt) {
            ts[n] = s;
            te[n] = p;
        }
        n++;
    }
    return n;
}

/* whole-line int() (whitespace-padded single token) */
static int line_int(const char* line, long* out)
{
    const char* p = line;
    p += strspn(p, " \t\n\r\v\f");
    const char* s = p;
    p += strcspn(p, " \t\n\r\v\f");
    const char* e = p;
    p += strspn(p, " \t\n\r\v\f");
    if (*p)
        return 0;
    return tok_int(s, e, out);
}

typedef struct UvBlock {
    int    idx;
    size_t count;
    float* uv; /* 2*count pairs (u, -v) */
} UvBlock;

/* Port adapter for ParseUV: parse buffered .uv text and apply polygon UV
 * pairs. Retail receives the model and stream handles and returns void. */
int model_apply_uv_text(Model* m, const char* text, size_t len)
{
    /* Split the way parse_uv's python does: CRLF acts as the line break
     * (the \r is the pair's terminator, dropped), blank lines never reach
     * the engine's state machine and are dropped here too. */
    size_t cap = 2;
    for (size_t text_index = 0; text_index < len; text_index++)
        if (text[text_index] == '\n')
            cap++;
    char** lines = (char**)malloc(cap * sizeof *lines);
    if (!lines)
        return 0;
    size_t nlines = 0;
    int split_oom = 0;
    const char* p = text;
    const char* end = text + len;
    while (p < end) {
        const char* nl = (const char*)memchr(p, '\n', (size_t)(end - p));
        const char* le = nl ? nl : end;
        if (le > p && le[-1] == '\r')
            le--; /* part of a CRLF terminator */
        size_t ll = (size_t)(le - p);
        char* ln = (char*)malloc(ll + 1);
        if (!ln) { split_oom = 1; break; }
        memcpy(ln, p, ll);
        ln[ll] = 0;
        int blank = 1;
        for (size_t line_index = 0; line_index < ll; line_index++)
            if (!isspace((unsigned char)ln[line_index]))
                blank = 0;
        if (!blank)
            lines[nlines++] = ln;
        else
            free(ln);
        if (!nl)
            break;
        p = nl + 1;
    }

    int applied = 0;
    if (split_oom)
        goto done;
    if (nlines < 4)
        goto done; /* python: too short to be a .uv file */
    {
        long s = 0, mpoly = 0;
        if (!line_int(lines[1], &s) || s < 1 || nlines < (size_t)(2 + 2 * s + 2))
            goto done; /* missing/inconsistent header count */
        if (!line_int(lines[2 + 2 * (size_t)s], &mpoly))
            goto done; /* missing polygon-count line */

        UvBlock* blocks = NULL;
        size_t nblocks = 0, bcap = 0;
        int err = 0;
        size_t q = 3 + 2 * (size_t)s;
        long expect = 0;
        while (expect < mpoly && q < nlines && !err) {
            const char* ts[4];
            const char* te[4];
            if (line_tokens(lines[q], ts, te, 4) < 2)
                break; /* tail block header: the engine reaches nothing either */
            long bi = 0, cnt = 0;
            if (!tok_int(ts[0], te[0], &bi) || !tok_int(ts[1], te[1], &cnt))
                break;
            if (bi != expect || cnt < 0 || (size_t)(q + 1 + cnt) > nlines)
                break;
            UvBlock nb;
            nb.idx = (int)bi;
            nb.count = (size_t)cnt;
            nb.uv = NULL;
            UvBlock* grown = (UvBlock*)realloc(blocks, (bcap + 1) * sizeof *grown);
            if (!grown) { err = 1; break; }
            blocks = grown;
            bcap++;
            if (cnt > 0) {
                nb.uv = (float*)malloc(2 * (size_t)cnt * sizeof(float));
                if (!nb.uv) { err = 1; break; }
                for (long uv_index = 0; uv_index < cnt; uv_index++) {
                    const char* cs[8];
                    const char* ce[8];
                    if (line_tokens(lines[q + 1 + uv_index], cs, ce, 8) < 2) {
                        free(nb.uv);
                        nb.uv = NULL;
                        err = 1;
                        break;
                    }
                    float u, v;
                    if (!tok_float(cs[0], ce[0], &u) || !tok_float(cs[1], ce[1], &v)) {
                        free(nb.uv);
                        nb.uv = NULL;
                        err = 1;
                        break;
                    }
                    nb.uv[2 * (size_t)uv_index] = u;
                    nb.uv[2 * (size_t)uv_index + 1] = -v; /* v negated (19 §4) */
                }
                if (err)
                    break;
            }
            blocks[nblocks++] = nb;
            expect++;
            q += 1 + (size_t)cnt;
        }

        if (!err) {
            /* path lines replace the surface TIMG (19 §3): the block is
             * s names then s paths, so path i lives at line 2 + s + i */
            for (long surface_index = 0;
                 surface_index < s && (size_t)surface_index < m->n_surfaces;
                 surface_index++) {
                const char* pl = lines[2 + (size_t)s + (size_t)surface_index];
                size_t sl = strlen(pl);
                size_t a = 0, b = sl;
                while (a < b && isspace((unsigned char)pl[a]))
                    a++;
                while (b > a && isspace((unsigned char)pl[b - 1]))
                    b--;
                if (b > a && !dup_assign(&m->surfaces[surface_index].timg, pl + a, b - a)) {
                    err = 1;
                    break;
                }
            }
        }
        if (!err) {
            for (size_t block_index = 0; block_index < nblocks; block_index++) {
                if (blocks[block_index].idx >= 0 &&
                    (size_t)blocks[block_index].idx < m->n_polys) {
                    ModelPoly* p = &m->polys[blocks[block_index].idx];
                    free(p->uv);
                    p->uv = blocks[block_index].uv;
                    p->uv_count = blocks[block_index].count;
                    blocks[block_index].uv = NULL;
                } else {
                    free(blocks[block_index].uv);
                }
            }
            m->uv_applied = 1;
            applied = 1;
        }
        for (size_t block_index = 0; block_index < nblocks; block_index++)
            free(blocks[block_index].uv);
        free(blocks);
    }
done:
    for (int line_index =  0; line_index < nlines; line_index++)
        free(lines[line_index]);
    free(lines);
    return applied;
}

/* --- open / free / digest ------------------------------------------------------------- */

// FUNCTION: LEGORR 0x0048d7c0
Model* model_open(const char* path)
{
    size_t path_length;
    size_t lwo_length;
    char* lwo_path;
    char* uv_path;
    int has_lwo_suffix;
    Resource* r;
    int32_t n;
    Model* m;

    if (path == NULL || path[0] == '\0')
        return NULL;
    path_length = strlen(path);
    has_lwo_suffix = path_length >= 4 &&
        strcasecmp(path + path_length - 4, ".lwo") == 0;
    if (path_length > (size_t)-1 - (has_lwo_suffix ? 1u : 5u))
        return NULL;
    lwo_length = path_length + (has_lwo_suffix ? 0u : 4u);
    lwo_path = (char*)malloc(lwo_length + 1u);
    if (lwo_path == NULL)
        return NULL;
    memcpy(lwo_path, path, path_length);
    if (!has_lwo_suffix)
        memcpy(lwo_path + path_length, ".lwo", 5u);
    else
        lwo_path[path_length] = '\0';

    r = resource_open(lwo_path, RESOURCE_AUTO);
    if (!r)
    {
        free(lwo_path);
        return NULL;
    }
    n = resource_read(r);
    m = (n > 0) ? model_parse(r->data, (size_t)n) : NULL;
    if (m) {
        uv_path = (char*)malloc(lwo_length + 1u);
        if (uv_path != NULL) {
            memcpy(uv_path, lwo_path, lwo_length + 1u);
            memcpy(uv_path + lwo_length - 4u, ".uv", 4u);
            {
                Resource* u = resource_open(uv_path, RESOURCE_AUTO);
                if (u != NULL && resource_read(u) > 0)
                    model_apply_uv_text(m, (const char*)u->data,
                                        (size_t)u->size);
                resource_free(u);
            }
            free(uv_path);
        }
    }
    resource_free(r);
    free(lwo_path);
    return m;
}

// FUNCTION: LEGORR 0x0048d870
void model_free(Model* m)
{
    if (!m)
        return;
    model_render_dispatch_model_remove(m);
    free(m->render_source);
    m->render_source = NULL;
    if (m->material_bindings != NULL) {
        model_texture_bindings_release(m->material_bindings,
                                       (uint32_t)m->material_binding_count);
        free(m->material_bindings);
        m->material_bindings = NULL;
        m->material_binding_count = 0u;
    }
    free(m->verts);
    if (m->polys) {
        for (size_t i = 0; i < m->n_polys; i++) {
            free(m->polys[i].ring);
            free(m->polys[i].uv);
        }
        free(m->polys);
    }
    if (m->surfaces) {
        for (size_t i = 0; i < m->n_surfaces; i++) {
            free(m->surfaces[i].name);
            free(m->surfaces[i].timg);
            free(m->surfaces[i].texture_definition);
        }
        free(m->surfaces);
    }
    free(m);
}

/* LwoModelEntryLoad (FUN_004809e0) derives a directory prefix, opens and
 * parses the model, then binds each parsed surface's texture through the
 * existing host resource/cache path. The task-record and renderer-object
 * inputs still have no portable owner. The bounded copy preserves the valid
 * path prefix behavior without reproducing the retail stack overwrite. */
// FUNCTION: LEGORR 0x004809e0
Model* lwo_model_entry_load(const char* path, void* flags, void* context)
{
    char directory[260];
    size_t length;
    size_t last_separator;
    size_t i;
    Model* model;

    (void)flags;    /* task callback flags have no host material equivalent */
    (void)context;  /* renderer object creation remains a backend operation */
    if (path == NULL) return NULL;
    length = strlen(path);
    if (length >= sizeof(directory)) length = sizeof(directory) - 1u;
    memcpy(directory, path, length);
    directory[length] = '\0';
    last_separator = 0u;
    for (i = 0; i < length; ++i)
        if (directory[i] == '\\' || directory[i] == '/')
            last_separator = i + 1u;
    directory[last_separator] = '\0';   /* keeps the backslash, else empties */
    model = model_open(path);
    if (model != NULL) {
        model->render_source = (ModelRenderSourceRecord*)calloc(
            1u, sizeof(*model->render_source));
        if (model->render_source != NULL) {
            model->render_source->model = model;
            memcpy(model->render_source->path_prefix, directory,
                   strlen(directory) + 1u);
        }
    }
    if (model != NULL && model->n_surfaces != 0u &&
        model->n_surfaces <= (size_t)(0x7fffffffL /
                                      sizeof(ModelTextureBinding))) {
        size_t bytes = model->n_surfaces * sizeof(ModelTextureBinding);
        ModelTextureBinding* bindings =
            (ModelTextureBinding*)calloc(1u, bytes);
        if (bindings != NULL) {
            bind_model_textures_for_model(directory, model, bindings, 0);
            model->material_bindings = bindings;
            model->material_binding_count = model->n_surfaces;
        }
        else {
            if (bindings != NULL) {
                model_texture_bindings_release(bindings,
                    (uint32_t)model->n_surfaces);
                free(bindings);
            }
        }
    }
    return model;
}

/* The reference's model resolver joins prefix and path with the raw
 * "%s%s" template (DAT_004a50d4) into a 260-byte stack path before its
 * case-insensitive pair-table lookup; no separator is inserted and a NULL
 * prefix copies the path verbatim.  Keep the same bounded contract without
 * snprintf, which is not part of the VC5 CRT. */
static void model_resource_path_join(char* output, size_t output_size,
                                     const char* prefix, const char* path)
{
    size_t position = 0u;
    const char* source;
    source = prefix;
    while (source != NULL && *source != '\0' &&
           position + 1u < output_size)
        output[position++] = *source++;
    source = path;
    while (*source != '\0' && position + 1u < output_size)
        output[position++] = *source++;
    output[position] = '\0';
}

/* ModelResourceResolveOrLoad (FUN_004885c0): concatenate prefix/path, use
 * the primary case-insensitive cache, load and cache on a miss, then retry
 * through the shared model root. The renderer instance operation remains a
 * backend seam; the portable cache owns parsed Model records. */
// FUNCTION: LEGORR 0x004885c0
Model* model_resource_resolve_or_load(const char* prefix, const char* path,
                                      void* flags, void* context)
{
    char resolved[260];
    Model* record;
    if (path == NULL) return NULL;
    model_resource_path_join(resolved, sizeof(resolved), prefix, path);
    record = (Model*)pair_table_find(s_model_resource_primary,
                                     (uint32_t)s_model_resource_primary_count,
                                     resolved);
    if (record != NULL) {
        if (s_model_resource_instance_hook != NULL)
            s_model_resource_instance_hook(record, flags);
        return record;
    }
    record = lwo_model_entry_load(resolved, flags, context);
    if (record != NULL) {
        /* Deviation: the corpus appends blindly past the 1000-entry
         * window; the port guards it. */
        if (s_model_resource_primary_count <
            (int)MODEL_RESOURCE_CACHE_CAPACITY)
            pair_table_append(s_model_resource_primary,
                              &s_model_resource_primary_count, resolved,
                              record);
        return record;
    }
    if (s_model_resource_fallback_prefix != NULL) {
        model_resource_path_join(resolved, sizeof(resolved),
                                 s_model_resource_fallback_prefix, path);
        record = (Model*)pair_table_find(
            s_model_resource_secondary,
            (uint32_t)s_model_resource_secondary_count, path);
        if (record != NULL) {
            if (s_model_resource_instance_hook != NULL)
                s_model_resource_instance_hook(record, flags);
            return record;
        }
        record = lwo_model_entry_load(resolved, flags, context);
        if (record != NULL) {
            if (s_model_resource_secondary_count <
                (int)MODEL_RESOURCE_CACHE_CAPACITY)
                pair_table_append(s_model_resource_secondary,
                                  &s_model_resource_secondary_count, path,
                                  record);
            return record;
        }
    }
    return NULL;
}

void model_resource_cache_clear(void)
{
    int i;
    /* Queued render entries borrow cached Model pointers until the next
     * DDiSceneDrawEnd pass; drop them before releasing the model records. */
    model_render_dispatch_queue_clear();
    for (i = 0; i < s_model_resource_primary_count; ++i) {
        engine_heap_free(s_model_resource_primary[i].key);
        model_free((Model*)s_model_resource_primary[i].value);
        s_model_resource_primary[i].key = NULL;
        s_model_resource_primary[i].value = NULL;
    }
    s_model_resource_primary_count = 0;
    for (i = 0; i < s_model_resource_secondary_count; ++i) {
        engine_heap_free(s_model_resource_secondary[i].key);
        model_free((Model*)s_model_resource_secondary[i].value);
        s_model_resource_secondary[i].key = NULL;
        s_model_resource_secondary[i].value = NULL;
    }
    s_model_resource_secondary_count = 0;
}

/* --- digest -------------------------------------------------------------------
 * FNV-1a over the canonical parsed view. The python ground truth streams the
 * same byte order (u32 little-endian words via struct.pack("<I"), IEEE bits),
 * which the model test cross-checks against the whole shipped corpus. */

static void dg32(uint32_t* h, uint32_t v)
{
    for (int i = 0; i < 4; i++) {
        *h ^= v & 0xff;
        v >>= 8;
        *h *= 16777619u;
    }
}
static void dgbits(uint32_t* h, float f)
{
    uint32_t u;
    memcpy(&u, &f, sizeof u);
    dg32(h, u);
}

uint32_t model_digest(const Model* m)
{
    uint32_t h = 2166136261u;
    dg32(&h, (uint32_t)m->n_verts);
    for (size_t vertex_index = 0; vertex_index < m->n_verts; vertex_index++) {
        dgbits(&h, m->verts[vertex_index * 3]);
        dgbits(&h, m->verts[vertex_index * 3 + 1]);
        dgbits(&h, m->verts[vertex_index * 3 + 2]);
    }
    dg32(&h, (uint32_t)m->n_polys);
    for (int poly_index =  0; poly_index < m->n_polys; poly_index++) {
        const ModelPoly* p = &m->polys[poly_index];
        dg32(&h, p->count);
        dg32(&h, p->surface);
        dg32(&h, (uint32_t)(p->flipped != 0));
        for (uint16_t ring_index = 0; ring_index < p->count; ring_index++)
            dg32(&h, (uint32_t)(int16_t)p->ring[ring_index]);
        dg32(&h, (uint32_t)p->uv_count);
        for (int uv_index =  0; uv_index < 2 * p->uv_count; uv_index++)
            dgbits(&h, p->uv[uv_index]);
    }
    dg32(&h, (uint32_t)m->n_surfaces);
    for (int surface_index =  0; surface_index < m->n_surfaces; surface_index++) {
        const ModelSurface* s = &m->surfaces[surface_index];
        uint32_t l = (uint32_t)strlen(s->name ? s->name : "");
        dg32(&h, l);
        for (uint32_t name_index = 0; name_index < l; name_index++) {
            h ^= (uint8_t)s->name[name_index];
            h *= 16777619u;
        }
        dg32(&h, (uint32_t)(s->has_colr != 0));
        for (int c = 0; c < 4; c++)
            dgbits(&h, s->colr[c]);
        dg32(&h, (uint32_t)(s->has_tsiz != 0));
        dgbits(&h, s->tsiz[0]);
        dgbits(&h, s->tsiz[1]);
        l = s->timg ? (uint32_t)strlen(s->timg) : 0;
        dg32(&h, l);
        for (int timg_index =  0; timg_index < l; timg_index++) {
            h ^= (uint8_t)s->timg[timg_index];
            h *= 16777619u;
        }
    }
    return h;
}

/* LwoStringDuplicate (FUN_0048c0c0): the corpus's dup allocates strlen + 1
 * rounded UP TO THE NEXT EVEN size through the engine heap, zeroes the
 * whole block (dwords then bytes), then copies strlen + 1 bytes (the
 * corpus's own loop bounds, assembly-pinned — Ghidra's "tail copy" was the
 * even-size padding). On the corpus heap the allocator's even-grained
 * blocks absorb the padding. LwoTextureProjectionModeParse (FUN_0048c120):
 * a fold compare against the table PLANAR / CYLINDRICAL / SPHERICAL (the
 * corpus calls the strnicmp_lite carrier directly); the first
 * prefix match answers the table index, anything else answers 0 — the
 * PLANAR default. */
// FUNCTION: LEGORR 0x0048c0c0
char* lwo_string_duplicate(const char* text)
{
    if (text == NULL) {
        return NULL; /* deviation: the corpus dereferences blindly */
    }
    /* Keep the corpus's length, terminator, and even-rounding steps separate
     * (0x48c0d2-0x48c0db: dec, inc, then add the low bit). */
    size_t size = strlen(text);
    ++size;
    size += size & 1u;
    char* copy = (char*)engine_heap_alloc((long)size);
    /* Corpus parity: the copy is filled unchecked — sole-caller audit found
     * no test or caller that survives a NULL allocator answer anyway. */
    memset(copy, 0, size);
    strcpy(copy, text);
    return copy;
}

// GLOBAL: LEGORR 0x004afa0c
static const char s_lwo_projection_planar[] = "PLANAR";
// GLOBAL: LEGORR 0x004afa00
static const char s_lwo_projection_cylindrical[] = "CYLINDRICAL";
// GLOBAL: LEGORR 0x004af9f4
static const char s_lwo_projection_spherical[] = "SPHERICAL";

// FUNCTION: LEGORR 0x0048c120
int lwo_texture_projection_mode_parse(const char* name)
{
    /* Corpus entry 0x48c12b-0x48c13b copies these literal addresses into
     * the stack table. */
    const char* modes[3];
    modes[0] = s_lwo_projection_planar;
    modes[1] = s_lwo_projection_cylindrical;
    modes[2] = s_lwo_projection_spherical;

    uint32_t i = 0;
    for (; i < 3u; ++i) {
        const size_t len = strlen(modes[i]); /* the corpus inlines this as
                                              * repnz scas at 0x48c152 */
        if (strnicmp_lite(name, modes[i], (int)len) == 0) break;
    }
    if (i == 3u) i = 0u; /* corpus 0x48c16f-0x48c174 resets exhausted scan */
    return (int)i;
}

namespace unported_ledger_stubs {
// LwoModelEntryLoad (implemented in gods98/model.cpp)
Model* ledger_stub_004809e0_LwoModelEntryLoad(const char* path, void* flags,
                                              void* context)
{
    return lwo_model_entry_load(path, flags, context);
}


// ModelResourceResolveOrLoad (implemented in gods98/model.cpp)
Model* ledger_stub_004885c0_ModelResourceResolveOrLoad(const char* prefix,
                                                       const char* path,
                                                       void* flags,
                                                       void* context)
{
    return model_resource_resolve_or_load(prefix, path, flags, context);
}


// LwoCurves
// FUNCTION: LEGORR 0x0048c360
uint32_t ledger_stub_0048c360_LwoCurves(GameFileHandle* stream,
                                         int byte_count)
{
    uint8_t* data;
    const uint8_t* cursor;
    size_t remaining;
    if (stream == NULL || byte_count < 0 || byte_count > 0x7ffffff0) return 0u;
    data = (uint8_t*)engine_heap_alloc((long)byte_count + 16L);
    if (data == NULL) return 0u;
    memset(data, 0, (size_t)byte_count + 16u);
    if (byte_count != 0) {
        /* FUN_0048c360 ignores WadFileRead's element count. The allocation
         * is pre-zeroed with a 16-byte tail, so a short/truncated read still
         * proceeds through the bounded record walk and returns success. */
        (void)wad_file_read(data, byte_count, 1, stream);
    }
    /* The retail walk consumes {u16 point count, point-count u16 values,
     * then two opaque u16 words}. It neither builds a curve table nor reads
     * the trailing words; they are only skipped before the next record. */
    cursor = data;
    remaining = (size_t)byte_count;
    while (remaining != 0u) {
        size_t point_count;
        size_t point_bytes;
        if (remaining < 2u) break;
        point_count = (size_t)be16(cursor);
        cursor += 2u;
        remaining -= 2u;
        if (point_count > remaining / 2u) break;
        point_bytes = point_count * 2u;
        cursor += point_bytes;
        remaining -= point_bytes;
        if (remaining < 4u) break;
        cursor += 4u;
        remaining -= 4u;
    }
    engine_heap_free(data);
    return 1u;
}


} // namespace unported_ledger_stubs
