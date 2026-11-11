// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-031 G98-031: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/G98-031.csv. */
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
#include <math.h>
#include <float.h>
#include <errno.h>
#include "gods98/texture.hpp"
#include <stdarg.h>

#if defined(_MSC_VER)
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

typedef struct Buf {
    uint8_t* d;
    size_t   len, cap;
} Buf;

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

// GLOBAL: LEGORR 0x004af9f4
static const char s_lwo_projection_spherical[] = "SPHERICAL";
// GLOBAL: LEGORR 0x004afa00
static const char s_lwo_projection_cylindrical[] = "CYLINDRICAL";
// GLOBAL: LEGORR 0x004afa0c
static const char s_lwo_projection_planar[] = "PLANAR";

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static float bef32(const uint8_t* p);
static uint16_t be16(const uint8_t* p);
static char* dup_lwo_string(const char* src, size_t len);
static int surfrec_addblock(Parse* P, SurfRec* r, const char* tag, const uint8_t* pay, size_t size);
static int surfrec_reserve(Parse* P);
static uint32_t be32(const uint8_t* p);
static Model* build_model(Parse* P);
static int parse_curves_payload(const uint8_t* data, size_t size);
static void parse_free(Parse* P);
static int tag_equal_ci(const uint8_t* tag, const char* expected);

// FUNCTION: LEGORR 0x0048c040
char* string_case_substring_find(char* haystack, const char* needle,
                                 uint32_t* out_index, size_t* out_length)
{
    if (haystack == NULL || needle == NULL) {
        return NULL; /* pinned deviation: test_string.cpp:145-147 */
    }
    /* Corpus walk: two intrinsic strlens into registers, three entry gates,
     * then a walking haystack pointer beside an inc counter with an unsigned
     * bound and test/jbe gate (the needle is re-read from its stack slot
     * every pass). */
    char* hay = haystack;
    unsigned int hlen = (unsigned int)strlen(haystack);
    unsigned int nlen = (unsigned int)strlen(needle);
    if (hlen == 0u) {
        return NULL; /* pinned: the corpus's strlen-shaped gate */
    }
    if (nlen == 0u) {
        return NULL; /* pinned: the corpus's strlen-shaped gate */
    }
    if (hlen <= 0u) {
        return NULL; /* the corpus's unsigned test/jbe bound before the walk */
    }
    for (unsigned index = 0; index < hlen; ++index) {
        if (strnicmp_lite(hay, needle, (int)nlen) == 0) {
            if (out_index != NULL) {
                *out_index = (uint32_t)index;
            }
            if (out_length != NULL) {
                *out_length = (size_t)nlen;
            }
            return hay;
        }
        ++hay;
    }
    return NULL;
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

/* OwnedResourceTreeRelease (FUN_0048c180) — see crt.hpp. */
// FUNCTION: LEGORR 0x0048c180
void owned_resource_tree_release(void** node, int own_node)
{
    if (node == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    if (node[0] != NULL) {
        engine_heap_free(node[0]);
    }
    if (node[2] != NULL) {
        engine_heap_free(node[2]);
    }
    if (node[1] != NULL) {
        owned_resource_tree_release((void**)node[1], 1);
    }
    if (own_node != 0) {
        engine_heap_free((void*)node);
    }
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

namespace unported_ledger_stubs {
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
