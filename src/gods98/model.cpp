// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — LWO model loader + .uv sidecar (see model.hpp).
 *
 * Ported against the rrlwo.py reference (corpus-validated over all shipped
 * models): chunk walk, POLS origin auto-detect, SRFS names, SURF records
 * anchored on SRFS names, and the finding 19 `.uv` grammar. */
#include "model.hpp"
#include "heap.hpp"
#include "resource.hpp"
#include "string.hpp"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_MSC_VER)
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

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
    const uint8_t* p = *cursor;
    out[0] = p[3];
    out[1] = p[2];
    out[2] = p[1];
    out[3] = p[0];
    *cursor = p + 4;
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

typedef struct Parse {
    const uint8_t* d;
    size_t         n;

    Buf     chunks_buf; /* native Chunk array */
    size_t  nchunks;

    Buf     vb;   /* raw 12-byte PNTS triples (BE bit patterns as stored) */
    Buf     ring;  /* native i16, raw signed POLS values in order */
    Buf     poff;  /* native uint32 start per poly */
    Buf     pcnt;  /* native uint16 per poly */
    Buf     psurf; /* native uint16 per poly (signed-1 clamped) */
    Buf     pflip; /* native uint8 per poly */
    int     saw_zero;
    size_t  pols_used_len; /* first POLS chunk only (python one()) */

    char**  sname;
    size_t  nsname, scap;

    SurfRec* recs;
    size_t   nrecs, rcap;
} Parse;

/* --- helpers ----------------------------------------------------------------- */

static void* xmemmem(const void* h, size_t hl, const void* nd, size_t nl)
{
    const uint8_t* hp = (const uint8_t*)h;
    const uint8_t* np = (const uint8_t*)nd;
    if (nl == 0 || nl > hl)
        return NULL;
    for (size_t i = 0; i + nl <= hl; i++)
        if (hp[i] == np[0] && memcmp(hp + i, np, nl) == 0)
            return (void*)(hp + i);
    return NULL;
}

static size_t strnlen_n(const char* s, size_t maxlen)
{
    size_t n = 0;
    while (n < maxlen && s[n])
        n++;
    return n;
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
    while (p < size) {
        const uint8_t* e = (const uint8_t*)memchr(pay + p, 0, size - p);
        if (!e)
            break;
        /* python: decode("latin1").strip() */
        char* nm = dup_strip((const char*)(pay + p), (size_t)(e - (pay + p)));
        if (!nm)
            return 0;
        if (P->nsname == P->scap) {
            size_t ncap = P->scap ? P->scap * 2 : 4;
            char** np = (char**)realloc(P->sname, ncap * sizeof *np);
            if (!np) { free(nm); return 0; }
            P->sname = np;
            P->scap = ncap;
        }
        P->sname[P->nsname++] = nm;
        p = ((size_t)(e - pay) + 2) & ~(size_t)1;
    }
    return 1;
}

/* --- POLS --------------------------------------------------------------------- */

/* python polys(): {u16 count, count x i16 indices, u16 surface}; stops on
 * count 0 or truncation; surface = max(signed - 1, 0). */
// FUNCTION: LEGORR 0x0048c3e0
static int parse_pols(Parse* P, const uint8_t* d, size_t size)
{
    size_t p = 0;
    while (p + 2 <= size) {
        size_t cnt = be16(d + p);
        p += 2;
        if (cnt == 0 || p + 2 * cnt + 2 > size)
            break;
        uint32_t off = (uint32_t)(P->ring.len / sizeof(int16_t));
        if (!buf_append(&P->poff, &off, sizeof off))
            return 0;
        int flip = 0;
        for (size_t k = 0; k < cnt; k++) {
            int16_t v = (int16_t)be16(d + p + 2 * k);
            if (v == 0)
                P->saw_zero = 1;
            if (v < 0)
                flip = 1;
            if (!buf_append(&P->ring, &v, sizeof v))
                return 0;
        }
        p += 2 * cnt;
        int sv = (int)(int16_t)be16(d + p) - 1;
        if (sv < 0)
            sv = 0;
        p += 2;
        uint16_t cc = (uint16_t)cnt, su = (uint16_t)sv, f8 = 0;
        f8 = (uint16_t)flip;
        if (!buf_append(&P->pcnt, &cc, sizeof cc)) return 0;
        if (!buf_append(&P->psurf, &su, sizeof su)) return 0;
        if (!buf_append(&P->pflip, &f8, 1)) return 0;
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
    for (size_t i = 0; i < r->nblocks; i++) {
        if (strcmp(r->tags[i], tag) == 0) { /* python dict overwrite */
            r->off[i] = off;
            r->size[i] = size;
            return 1;
        }
    }
    size_t ncap = r->blocks_cap ? r->blocks_cap * 2 : 8;
    char (*nt)[5] = (char (*)[5])realloc(r->tags, ncap * sizeof *r->tags);
    if (!nt)
        return 0;
    size_t* no = (size_t*)realloc(r->off, ncap * sizeof *r->off);
    if (!no) { free(nt); return 0; } /* original off is stale-freed below */
    r->tags = nt;
    r->off = no;
    if (r->size)
        r->size = (size_t*)realloc(r->size, ncap * sizeof *r->size);
    else
        r->size = (size_t*)malloc(ncap * sizeof *r->size);
    if (!r->size)
        return 0;
    memcpy(r->tags[r->nblocks], tag, 5);
    r->off[r->nblocks] = off;
    r->size[r->nblocks] = size;
    r->nblocks++;
    r->blocks_cap = ncap;
    return 1;
}

static int block_find(const SurfRec* r, const char* tag, const uint8_t** out, size_t* nout)
{
    for (size_t i = 0; i < r->nblocks; i++) {
        if (strcmp(r->tags[i], tag) == 0) {
            *out = r->blocks_data.d + r->off[i];
            *nout = r->size[i];
            return 1;
        }
    }
    return 0;
}

/* python surf_records(): SRFS-name anchors delimit records inside SURF
 * chunks; between them {4 printable tag bytes, u16 size, payload, pad}. */
// FUNCTION: LEGORR 0x0048c820
static int parse_surf_chunk(Parse* P, const uint8_t* d, size_t size)
{
    size_t* starts = NULL;
    size_t nst = 0, scap = 0;
    int ok = 1;

    for (size_t name_index = 0; name_index < P->nsname && ok; name_index++) {
        size_t nlen = strlen(P->sname[name_index]);
        uint8_t* needle = (uint8_t*)malloc(nlen + 1);
        if (!needle) { ok = 0; break; }
        memcpy(needle, P->sname[name_index], nlen);
        needle[nlen] = 0;
        for (size_t pos = 0; pos + nlen + 1 <= size;) {
            const uint8_t* hit = (const uint8_t*)xmemmem(d + pos, size - pos, needle, nlen + 1);
            if (!hit)
                break;
            size_t e = (size_t)(hit - d);
            if (e % 2 == 0) {
                int dup = 0;
                for (size_t k = 0; k < nst; k++)
                    if (starts[k] == e)
                        dup = 1;
                if (!dup) {
                    if (nst == scap) {
                        size_t nc = scap ? scap * 2 : 16;
                        size_t* ns = (size_t*)realloc(starts, nc * sizeof *starts);
                        if (!ns) { ok = 0; break; }
                        starts = ns;
                        scap = nc;
                    }
                    starts[nst++] = e;
                }
            }
            pos = e + 1;
        }
        free(needle);
    }
    /* insertion sort ascending */
    for (int sort_index =  1; sort_index < nst; sort_index++)
        for (size_t sort_cursor = sort_index;
             sort_cursor > 0 && starts[sort_cursor - 1] > starts[sort_cursor];
             sort_cursor--) {
            size_t t = starts[sort_cursor - 1];
            starts[sort_cursor - 1] = starts[sort_cursor];
            starts[sort_cursor] = t;
        }

    for (size_t k = 0; k < nst && ok; k++) {
        size_t e = starts[k];
        size_t limit = (k + 1 < nst) ? starts[k + 1] : size;
        /* first anchor (SRFS order) matching here */
        size_t slot = P->nsname;
        size_t alen = 0;
        for (size_t anchor_index = 0; anchor_index < P->nsname; anchor_index++) {
            alen = strlen(P->sname[anchor_index]);
            if (memcmp(d + e, P->sname[anchor_index], alen) == 0 && d[e + alen] == 0) {
                slot = anchor_index;
                break;
            }
        }
        if (slot == P->nsname)
            continue;
        /* python `seen`: exact names, first record wins */
        int dup = 0;
        for (int rec_index =  0; rec_index < P->nrecs && !dup; rec_index++)
            if (strcmp(P->recs[rec_index].name, P->sname[slot]) == 0)
                dup = 1;
        if (dup)
            continue;
        if (!surfrec_reserve(P)) { ok = 0; break; }

        SurfRec* r = &P->recs[P->nrecs];
        memset(r, 0, sizeof *r);
        r->name = P->sname[slot];
        size_t q = (e + alen + 2) & ~(size_t)1; /* python (e + len(a) + 1) & ~1 */
        while (q + 6 <= limit) {
            const uint8_t* bt = d + q;
            int printable = 1;
            for (int t = 0; t < 4; t++)
                if (bt[t] < 0x20 || bt[t] > 0x7E)
                    printable = 0;
            if (!printable)
                break;
            size_t bsz = be16(d + q + 4);
            if (bsz > limit - q - 6)
                break;
            /* python key = tag.decode.strip("\x00 ") */
            char tag[5];
            memcpy(tag, bt, 4);
            tag[4] = 0;
            size_t a = 0, b = 4;
            while (a < b && (tag[a] == 0 || tag[a] == ' '))
                a++;
            while (b > a && (tag[b - 1] == 0 || tag[b - 1] == ' '))
                b--;
            size_t tn = b - a;
            char key[5];
            memcpy(key, tag + a, tn);
            key[tn] = 0;
            if (!surfrec_addblock(P, r, key, d + q + 6, bsz)) { ok = 0; break; }
            q += 6 + bsz + (bsz & 1);
        }
        P->nrecs++;
    }
    free(starts);
    return ok;
}

/* --- model build ----------------------------------------------------------------- */

static Model* build_model(Parse* P)
{
    Model* m = (Model*)calloc(1, sizeof *m);
    if (!m)
        return NULL;
    size_t nv = P->vb.len / 12;
    m->n_verts = nv;
    if (nv) {
        m->verts = (float*)malloc(nv * 3 * sizeof(float));
        if (!m->verts) { free(m); return NULL; }
        memcpy(m->verts, P->vb.d, nv * 12); /* BE bytes already bit-correct */
    }

    size_t np = P->pcnt.len / sizeof(uint16_t);
    m->n_polys = np;
    if (np) {
        m->polys = (ModelPoly*)calloc(np, sizeof(ModelPoly));
        if (!m->polys) { free(m); return NULL; }
    }
    const int base = P->saw_zero ? 0 : 1;
    for (size_t i = 0; i < np; i++) {
        uint32_t off;
        memcpy(&off, P->poff.d + i * sizeof(off), sizeof(off));
        uint16_t cnt = ((const uint16_t*)P->pcnt.d)[i];
        ModelPoly* p = &m->polys[i];
        p->count = cnt;
        p->ring = (uint16_t*)malloc((cnt ? cnt : 1) * sizeof(uint16_t));
        if (!p->ring) { model_free(m); return NULL; }
        const int16_t* raw = (const int16_t*)P->ring.d + off;
        for (uint16_t k = 0; k < cnt; k++) {
            int v = raw[k] < 0 ? -raw[k] : raw[k];
            v -= base;
            p->ring[k] = (uint16_t)(v < 0 ? 0 : v);
        }
        p->surface = ((const uint16_t*)P->psurf.d)[i];
        p->flipped = ((const uint8_t*)P->pflip.d)[i] != 0;
    }

    m->n_surfaces = P->nsname;
    if (P->nsname) {
        m->surfaces = (ModelSurface*)calloc(P->nsname, sizeof(ModelSurface));
        if (!m->surfaces) { model_free(m); return NULL; }
        for (size_t i = 0; i < P->nsname; i++)
            m->surfaces[i].name = strdup(P->sname[i]);
    }

    /* SURF records apply to every surface whose SRFS name matches
     * case-insensitively (python: meta keyed by name.lower(), looked up by
     * names[si].lower()); records apply in file order, field-wise last-wins. */
    for (size_t r = 0; r < P->nrecs; r++) {
        const SurfRec* rec = &P->recs[r];
        const uint8_t* bp;
        size_t bs;
        for (size_t i = 0; i < P->nsname; i++) {
            if (strcasecmp(P->sname[i], rec->name) != 0)
                continue;
            ModelSurface* s = &m->surfaces[i];
            if (block_find(rec, "COLR", &bp, &bs) && bs >= 3) {
                for (int c = 0; c < 3; c++)
                    s->colr[c] = (float)((double)bp[c] / 255.0); /* == python c/255 */
                s->colr[3] = 1.0f;
                s->has_colr = 1;
            }
            if (block_find(rec, "TIMG", &bp, &bs) && bs > 0) {
                size_t n = strnlen_n((const char*)bp, bs);
                if (!dup_assign(&s->timg, (const char*)bp, n))
                    { model_free(m); return NULL; }
            }
            if (block_find(rec, "TSIZ", &bp, &bs) && bs >= 8) {
                s->tsiz[0] = bef32(bp);
                s->tsiz[1] = bef32(bp + 4);
                s->has_tsiz = 1;
            }
        }
    }
    return m;
}

/* --- parse entry -------------------------------------------------------------------- */

static void parse_free(Parse* P)
{
    buf_free(&P->chunks_buf);
    buf_free(&P->vb);
    buf_free(&P->ring);
    buf_free(&P->poff);
    buf_free(&P->pcnt);
    buf_free(&P->psurf);
    buf_free(&P->pflip);
    for (size_t name_index = 0; name_index < P->nsname; name_index++)
        free(P->sname[name_index]);
    free(P->sname);
    for (int rec_index =  0; rec_index < P->nrecs; rec_index++) {
        free(P->recs[rec_index].tags);
        free(P->recs[rec_index].off);
        free(P->recs[rec_index].size);
        buf_free(&P->recs[rec_index].blocks_data);
    }
    free(P->recs);
}

static const Chunk* chunk_find_first(const Parse* P, const char* tag)
{
    for (size_t i = 0; i < P->nchunks; i++)
        if (strcmp(((const Chunk*)P->chunks_buf.d + i)->tag, tag) == 0)
            return (const Chunk*)P->chunks_buf.d + i;
    return NULL;
}

/* LwoPoints: convert each PNTS triple from big-endian float bits and append
 * the host-order coordinates to the model's vertex buffer. */
// FUNCTION: LEGORR 0x0048c210
static int parse_points(Parse* P, const Chunk* c)
{
    for (size_t q = 0; q + 12 <= c->size; q += 12) {
        for (int k = 0; k < 3; k++) {
            float f = bef32(c->pay + q + 4 * k);
            if (!buf_append(&P->vb, &f, sizeof f)) {
                return 0;
            }
        }
    }
    return 1;
}

// FUNCTION: LEGORR 0x0048d2c0
Model* model_parse(const uint8_t* data, size_t len)
{
    if (len < 12 || memcmp(data, "FORM", 4) != 0 || memcmp(data + 8, "LWOB", 4) != 0)
        return NULL;
    Parse P;
    memset(&P, 0, sizeof P);
    P.d = data;
    P.n = len;

    size_t end = 8 + (size_t)be32(data + 4);
    if (end > len)
        end = len;
    int ok = 1;
    size_t p = 12;
    while (p + 8 <= end) {
        Chunk c;
        memset(&c, 0, sizeof c);
        memcpy(c.tag, data + p, 4);
        c.tag[4] = 0;
        size_t tn = 4;
        while (tn && c.tag[tn - 1] == 0)
            c.tag[--tn] = 0;
        uint32_t size = be32(data + p + 4);
        const uint8_t* pay = data + p + 8;
        c.pay = pay;
        c.size = (pay + size > data + len) ? (size_t)(data + len - pay) : size;
        if (!buf_append(&P.chunks_buf, &c, sizeof c)) { ok = 0; break; }
        P.nchunks++;
        p += 8 + (size_t)size + (size & 1);
    }
    if (!ok) { parse_free(&P); return NULL; }

    const Chunk* c;
    if ((c = chunk_find_first(&P, "PNTS"))) {
        ok = parse_points(&P, c);
    }
    /* POLS / SRFS: first occurrence each */
    if (ok) {
        const Chunk* pc = chunk_find_first(&P, "POLS");
        if (pc)
            ok = parse_pols(&P, pc->pay, pc->size);
    }
    if (ok) {
        const Chunk* sc = chunk_find_first(&P, "SRFS");
        if (sc)
            ok = parse_srfs(&P, sc->pay, sc->size);
    }
    if (ok) {
        for (size_t i = 0; i < P.nchunks && ok; i++) {
            const Chunk* c = (const Chunk*)P.chunks_buf.d + i;
            if (strcmp(c->tag, "SURF") == 0)
                ok = parse_surf_chunk(&P, c->pay, c->size);
        }
    }
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
    if (p != end && (*p == '+' || *p == '-'))
        p++;
    const char* d = p;
    long v = 0;
    int neg = (s != end && *s == '-');
    for (; p != end && isdigit((unsigned char)*p); p++) {
        v = v * 10 + (*p - '0');
        if (v > 1L << 30)
            return 0; /* would need a real ValueError to matter; never in corpus */
    }
    if (p == d || p != end)
        return 0;
    *out = neg ? -v : v;
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
        while (*p && isspace((unsigned char)*p))
            p++;
        if (!*p)
            break;
        const char* s = p;
        while (*p && !isspace((unsigned char)*p))
            p++;
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
    while (*p && isspace((unsigned char)*p))
        p++;
    const char* s = p;
    while (*p && !isspace((unsigned char)*p))
        p++;
    const char* e = p;
    while (*p && isspace((unsigned char)*p))
        p++;
    if (*p)
        return 0;
    return tok_int(s, e, out);
}

typedef struct UvBlock {
    int    idx;
    size_t count;
    float* uv; /* 2*count pairs (u, -v) */
} UvBlock;

/* ParseUV: parse the .uv text and apply its polygon UV pairs to the model.
 * The caller supplies buffered text instead of the corpus stream handle. */
// FUNCTION: LEGORR 0x00481c50
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
    Resource* r = resource_open(path, RESOURCE_AUTO);
    if (!r)
        return NULL;
    int32_t n = resource_read(r);
    Model* m = (n > 0) ? model_parse(r->data, (size_t)n) : NULL;
    if (m) {
        const char* dot = strrchr(path, '.');
        char uvpath[1024];
        if (dot && strcasecmp(dot, ".lwo") == 0 && strlen(path) + 4 < sizeof uvpath) {
            size_t base = (size_t)(dot - path);
            memcpy(uvpath, path, base);
            uvpath[base] = 0;
            strcat(uvpath, ".uv");
            Resource* u = resource_open(uvpath, RESOURCE_AUTO);
            if (u && resource_read(u) > 0)
                model_apply_uv_text(m, (const char*)u->data, (size_t)u->size);
            resource_free(u);
        }
    }
    resource_free(r);
    return m;
}

void model_free(Model* m)
{
    if (!m)
        return;
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
        }
        free(m->surfaces);
    }
    free(m);
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
    const size_t len1 = strlen(text) + 1u;
    const size_t size = len1 + (len1 & 1u); /* even-grained corpus size */
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

    for (uint32_t i = 0; i < 3u; ++i) {
        const size_t len = strlen(modes[i]); /* the corpus inlines this as
                                              * repnz scas at 0x48c152 */
        if (strnicmp_lite(name, modes[i], (int)len) == 0) {
            return (int)i; /* corpus 0x48c164: je exits the loop on match */
        }
    }
    return 0; /* the corpus's fall-through answers PLANAR */
}
