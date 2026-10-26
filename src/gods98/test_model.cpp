// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_model — validation for src/gods98/model.cpp (findings 11 + 19).
 *
 * Synthetic cases always run: FORM walk, PNTS/POLS (1- and 0-based origin
 * auto-detect, negative-index flip, surface clamp, truncation stop), SRFS
 * names, SURF records (anchor scan, sub-block vocabulary, seen-gate, split
 * chunks), and the .uv grammar (header, path override, corner blocks with
 * v negated, tail stop, structural reject).
 *
 * With `--real <dir>` the shipped wads mount and EVERY .lwo entry (1,132) is
 * parsed and paired with its .uv sibling (60) through the resource layer;
 * totals and an aggregate FNV digest cross-check the same view extracted
 * from the shipped bytes with tools/rrlwo.py + tools/rrwad.py, plus spot
 * checks (the 0-based POLS files and an .uv-covered model).
 *
 * Wire-up: meson test 'model' (suite 'model'); real checks via the
 * `test_wads_dir` meson option.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "model.hpp"
#include "resource.hpp"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

/* --- synthetic byte builders -------------------------------------------------
 * LWO is big-endian on disk; floats are written from host values so the bit
 * patterns round-trip regardless of host byte order. */

static unsigned char g_buf[16384];

static void w16(unsigned char** p, unsigned v)
{
    (*p)[0] = (unsigned char)(v >> 8);
    (*p)[1] = (unsigned char)v;
    *p += 2;
}
static void w32(unsigned char** p, unsigned v)
{
    (*p)[0] = (unsigned char)(v >> 24);
    (*p)[1] = (unsigned char)(v >> 16);
    (*p)[2] = (unsigned char)(v >> 8);
    (*p)[3] = (unsigned char)v;
    *p += 4;
}
static void wfl(unsigned char** p, float f)
{
    unsigned char t[4];
    memcpy(t, &f, 4);
    (*p)[0] = t[3];
    (*p)[1] = t[2];
    (*p)[2] = t[1];
    (*p)[3] = t[0];
    *p += 4;
}
static void wtag(unsigned char** p, const char* t)
{
    memcpy(*p, t, 4);
    *p += 4;
}
static void wbytes(unsigned char** p, const void* src, int n)
{
    if (n)
        memcpy(*p, src, n);
    *p += n;
    if (n & 1)
        *(*p)++ = 0;
}
/* tag + u32 BE size + payload, even-padded */
static void wchunk(unsigned char** p, const char* tag, const void* pay, int n)
{
    wtag(p, tag);
    w32(p, (unsigned)n);
    wbytes(p, pay, n);
}
/* SURF sub-block: 4 tag bytes + u16 BE size + payload, even-padded */
static void wsub(unsigned char** p, const char* tag, const void* pay, int n)
{
    wtag(p, tag);
    w16(p, (unsigned)n);
    wbytes(p, pay, n);
}
/* one POLS record: count, signed i16 indices, surface field */
static void wpoly(unsigned char** p, int cnt, const int* idx, int surf)
{
    w16(p, (unsigned)cnt);
    for (int i = 0; i < cnt; i++)
        w16(p, (unsigned)idx[i]);
    w16(p, (unsigned)surf);
}

/* SRFS-style NUL record, even-padded relative to the payload start */
static void wname(unsigned char** p, const unsigned char* base, const char* s)
{
    int n = (int)strlen(s);
    memcpy(*p, s, n + 1);
    *p += n + 1;
    if ((((size_t)*p) - ((size_t)base)) & 1)
        *(*p)++ = 0;
}

static unsigned char* form_begin(void)
{
    unsigned char* p = g_buf;
    wtag(&p, "FORM");
    w32(&p, 0); /* patched by form_end */
    wtag(&p, "LWOB");
    return p;
}
static void form_end(unsigned char* p)
{
    unsigned char* q = g_buf + 4;
    w32(&q, (unsigned)(p - g_buf - 8));
}

static float v3x(int i) { return (float)i; }

/* --- synthetic tests ------------------------------------------------------------ */

static void test_basic(void)
{
    unsigned char* p = form_begin();
    unsigned char* start = p;
    wtag(&p, "PNTS");
    w32(&p, 36);
    for (int i = 1; i <= 9; i++)
        wfl(&p, v3x(i));
    form_end(p);
    (void)start;

    Model* m = model_parse(g_buf, (size_t)(p - g_buf));
    CHECK(m && m->n_verts == 3, "basic: 3 verts parsed");
    if (m) {
        CHECK(m->verts[0] == 1.0f && m->verts[8] == 9.0f, "basic: vert values");
        CHECK(m->n_polys == 0 && m->n_surfaces == 0, "basic: empty tables");
        model_free(m);
    }

    CHECK(model_parse((const unsigned char*)"OOPS........", 12) == NULL, "basic: not FORM");
    const unsigned char junk[] = {'F', 'O', 'R', 'M', 0, 0, 0, 10, 'X', 'Y',
                                  'Z', 'Z', 0,  0, 0, 0, 0, 0};
    CHECK(model_parse(junk, sizeof junk) == NULL, "basic: FORM but not LWOB");
}

static void test_pols(void)
{
    const float v[] = {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0};

    /* 1-based: quad (1..4) + reversed triangle, surfaces 1 and 2 */
    unsigned char* p = form_begin();
    wtag(&p, "PNTS");
    w32(&p, 48);
    for (int i = 0; i < 12; i++)
        wfl(&p, v[i]);
    unsigned char pols[64];
    unsigned char* q = pols;
    int quad[4] = {1, 2, 3, 4};
    wpoly(&q, 4, quad, 1);
    int tri[3] = {1, -3, 2};
    wpoly(&q, 3, tri, 2);
    wchunk(&p, "POLS", pols, (int)(q - pols));
    unsigned char sr[16] = {'b', 'o', 'x', 0, 0, 0};
    wchunk(&p, "SRFS", sr, 6);
    form_end(p);

    Model* m = model_parse(g_buf, (size_t)(p - g_buf));
    CHECK(m != NULL, "pols: model parses");
    if (m) {
        CHECK(m->n_polys == 2, "pols: two polys");
        if (m->n_polys == 2) {
            const ModelPoly* p0 = &m->polys[0];
            CHECK(p0->count == 4 && p0->ring[0] == 0 && p0->ring[3] == 3
                      && p0->surface == 0 && !p0->flipped,
                  "pols: 1-based quad; surface 1 -> slot 0");
            const ModelPoly* p1 = &m->polys[1];
            CHECK(p1->count == 3 && p1->ring[0] == 0 && p1->ring[1] == 2
                      && p1->ring[2] == 1 && p1->surface == 1 && p1->flipped,
                  "pols: negative index flips; abs then origin");
        }
        model_free(m);
    }

    /* 0-based file: a zero index anywhere switches the origin */
    p = form_begin();
    wtag(&p, "PNTS");
    w32(&p, 36);
    for (int i = 0; i < 9; i++)
        wfl(&p, v[i]);
    q = pols;
    int z[] = {0, 1, 2};
    wpoly(&q, 3, z, 1);
    wchunk(&p, "POLS", pols, (int)(q - pols));
    form_end(p);
    m = model_parse(g_buf, (size_t)(p - g_buf));
    CHECK(m && m->n_polys == 1 && m->polys[0].ring[0] == 0
              && m->polys[0].ring[2] == 2,
          "pols: 0-based auto-detect");
    if (m)
        model_free(m);

    /* truncated record stops the scan; surface 0 clamps to slot 0 */
    p = form_begin();
    wtag(&p, "PNTS");
    w32(&p, 36);
    for (int i = 0; i < 9; i++)
        wfl(&p, v[i]);
    q = pols;
    w16(&q, 3);
    w16(&q, 1);
    w16(&q, 2); /* only 2 of 3 indices, no surface field: drop */
    wchunk(&p, "POLS", pols, (int)(q - pols));
    form_end(p);
    m = model_parse(g_buf, (size_t)(p - g_buf));
    CHECK(m && m->n_polys == 0, "pols: truncated record drops");
    if (m)
        model_free(m);
}

static void test_srfs_surf(void)
{
    const float v[] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
    unsigned char* p = form_begin();
    wtag(&p, "PNTS");
    w32(&p, 36);
    for (int i = 0; i < 9; i++)
        wfl(&p, v[i]);
    unsigned char pols[32];
    unsigned char* q = pols;
    int tri[] = {1, 2, 3};
    wpoly(&q, 3, tri, 2); /* points at surface slot 1 = "floor" */
    wchunk(&p, "POLS", pols, (int)(q - pols));

    /* SRFS: "wall  " (trailing spaces strip) then "floor", both padded */
    unsigned char sb[256];
    q = sb;
    wname(&q, sb, "wall  ");
    wname(&q, sb, "floor");
    wchunk(&p, "SRFS", sb, (int)(q - sb));

    /* SURF chunk: both records in one chunk */
    q = sb;
    memcpy(q, "wall\0", 5);
    q += 6;
    unsigned char colr[4] = {128, 64, 0, 255};
    wsub(&q, "COLR", colr, 4);
    {
        unsigned char ts[8] = {0x40, 0x00, 0x00, 0x00, 0x40, 0x80, 0x00, 0x00};
        wsub(&q, "TSIZ", ts, 8);
    }
    wsub(&q, "TIMG", "brick.bmp\0", 10);
    memcpy(q, "floor\0", 6);
    q += 6;
    {
        unsigned char c3[3] = {255, 128, 0};
        wsub(&q, "COLR", c3, 3);
    }
    wchunk(&p, "SURF", sb, (int)(q - sb));

    /* second SURF chunk repeats "wall" — seen-gate: must NOT overwrite */
    q = pols;
    memcpy(q, "wall\0", 5);
    q += 6;
    wsub(&q, "TIMG", "wrong.bmp\0", 11);
    wchunk(&p, "SURF", pols, (int)(q - pols));
    form_end(p);

    Model* m = model_parse(g_buf, (size_t)(p - g_buf));
    CHECK(m != NULL, "surf: model parses");
    if (m) {
        CHECK(m->n_surfaces == 2, "surf: two surfaces");
        if (m->n_surfaces == 2) {
            CHECK(strcmp(m->surfaces[0].name, "wall") == 0, "surf: SRFS name stripped");
            CHECK(m->surfaces[0].has_colr && m->surfaces[0].colr[0] == 128 / 255.0f
                      && m->surfaces[0].colr[3] == 1.0f,
                  "surf: COLR /255, alpha 1");
            CHECK(m->surfaces[0].has_tsiz && m->surfaces[0].tsiz[0] == 2.0f
                      && m->surfaces[0].tsiz[1] == 4.0f,
                  "surf: TSIZ pair");
            CHECK(m->surfaces[0].timg && strcmp(m->surfaces[0].timg, "brick.bmp") == 0,
                  "surf: TIMG cuts at NUL");
            CHECK(m->surfaces[1].has_colr && m->surfaces[1].colr[0] == 1.0f
                      && m->surfaces[1].colr[1] > 0.5f,
                  "surf: 3-byte COLR read");
            CHECK(m->surfaces[1].timg == NULL, "surf: TIMG absent stays NULL");
            CHECK(m->polys[0].surface == 1, "surf: poly points at surface 1");
        }
        model_free(m);
    }
}

static const char uv_good[] =
    "2\r\n"
    "2\r\n"
    "wall\r\n"
    "floor\r\n"
    "brick.bmp\r\n"
    "panel.bmp\r\n"
    "1\r\n"
    "0 3\r\n"
    "0.5 0.25 0\r\n"
    "-1 2 0\r\n"
    "0 0 0\r\n"
    "1\r\n"
    "0 0 0 0 0 0\r\n"; /* keyframe tail: stops the scan, keeps the blocks */

static int uv_model_build(unsigned char** out_end)
{
    const float v[] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
    unsigned char* p = form_begin();
    wtag(&p, "PNTS");
    w32(&p, 36);
    for (int i = 0; i < 9; i++)
        wfl(&p, v[i]);
    unsigned char pols[16];
    unsigned char* q = pols;
    int tri[] = {1, 2, 3};
    wpoly(&q, 3, tri, 1);
    wchunk(&p, "POLS", pols, (int)(q - pols));
    q = pols;
    wname(&q, pols, "wall");
    wname(&q, pols, "floor");
    wchunk(&p, "SRFS", pols, (int)(q - pols));
    form_end(p);
    *out_end = p;
    return 1;
}

static void test_uv(void)
{
    unsigned char* end;
    uv_model_build(&end);
    size_t len = (size_t)(end - g_buf);

    Model* m = model_parse(g_buf, len);
    CHECK(m != NULL, "uv: model parses");
    if (!m)
        return;
    CHECK(model_apply_uv_text(m, uv_good, sizeof uv_good - 1) == 1,
          "uv: good sidecar applies");
    CHECK(m->uv_applied == 1, "uv: applied flag");
    CHECK(m->polys[0].uv_count == 3, "uv: three corners");
    if (m->polys[0].uv_count == 3) {
        CHECK(m->polys[0].uv[0] == 0.5f && m->polys[0].uv[1] == -0.25f, "uv: v negated");
        CHECK(m->polys[0].uv[2] == -1.0f && m->polys[0].uv[3] == -2.0f,
              "uv: second corner negated");
    }
    CHECK(m->surfaces[0].timg && strcmp(m->surfaces[0].timg, "brick.bmp") == 0,
          "uv: path replaces surface 0 TIMG");
    CHECK(m->surfaces[1].timg && strcmp(m->surfaces[1].timg, "panel.bmp") == 0,
          "uv: path replaces surface 1 TIMG");
    model_free(m);

    /* structural failure (corner with one token) drops the sidecar wholesale */
    m = model_parse(g_buf, len);
    const char bad[] = "2\r\n2\r\nwall\r\nfloor\r\nbrick.bmp\r\npanel.bmp\r\n1\r\n"
                       "0 2\r\n0.5 0.25 0\r\n3.5\r\n";
    CHECK(model_apply_uv_text(m, bad, sizeof bad - 1) == 0, "uv: broken corner rejects");
    CHECK(m->uv_applied == 0 && m->polys[0].uv == NULL && m->surfaces[0].timg == NULL,
          "uv: rejected sidecar leaves model untouched");
    model_free(m);

    /* header inconsistent with the line budget is not a .uv file */
    m = model_parse(g_buf, len);
    const char shortf[] = "1\r\n2\r\nwall\r\n";
    CHECK(model_apply_uv_text(m, shortf, sizeof shortf - 1) == 0, "uv: too short rejects");
    model_free(m);

    /* blank lines never reach the state machine */
    m = model_parse(g_buf, len);
    const char blanks[] = "1\r\n\r\n2\r\n\nwall\r\nfloor\r\nbrick.bmp\r\npanel.bmp\r\n1\r\n"
                          "  \r\n0 2\r\n1 1 0\r\n2 2 0\r\n";
    CHECK(model_apply_uv_text(m, blanks, sizeof blanks - 1) == 1, "uv: blanks dropped");
    CHECK(m->polys[0].uv_count == 2, "uv: corners parsed after blank drop");
    model_free(m);

    /* empty path line: the surface keeps whatever the LWO says */
    m = model_parse(g_buf, len);
    const char emptypath[] = "2\r\n2\r\nwall\r\nfloor\r\nbrick.bmp\r\n\r\n1\r\n"
                             "0 1\r\n1 1 0\r\n";
    /* line 6 empty is dropped as blank => grammar shifts; engine would
     * misalign too — just confirm our parser behaves as parse_uv: header
     * budget rejects (7 kept lines < 2+2*2+2) */
    CHECK(model_apply_uv_text(m, emptypath, sizeof emptypath - 1) == 0,
          "uv: blank path line shifts header (rejected like the reference)");
    model_free(m);
}

/* LwoReadBigEndianFloat (corpus FUN_0048c1d0): the four stream bytes land
 * byte-reversed so a little-endian read recovers the big-endian float, the
 * cursor advances four, and the return is always 1. */
static void test_be_float_cursor(void)
{
    /* 3.14159... as big-endian bytes. */
    static const uint8_t stream[] = { 0x40, 0x49, 0x0F, 0xD0 };
    const uint8_t* cursor = stream;
    uint8_t out[4];

    CHECK(lwo_read_big_endian_float(&cursor, out) == 1, "be reader answers 1");
    CHECK(cursor == stream + 4, "the cursor advanced four");

    float f;
    memcpy(&f, out, sizeof f);
    CHECK(f > 3.1415f && f < 3.1416f, "byte reversal recovers the float");

    /* Successive reads keep advancing the cursor. */
    static const uint8_t stream2[] = { 0x3F, 0x80, 0x00, 0x00, 0x41, 0x20,
                                       0x00, 0x00 };
    const uint8_t* c2 = stream2;
    float a, b;
    lwo_read_big_endian_float(&c2, out);
    memcpy(&a, out, 4);
    lwo_read_big_endian_float(&c2, out);
    memcpy(&b, out, 4);
    CHECK(a == 1.0f && b == 10.0f, "two reads give the two big-endian floats");
    CHECK(c2 == stream2 + 8, "the cursor advanced twice");
}

static void test_digest_stability(void)
{
    unsigned char* end;
    uv_model_build(&end);
    Model* a = model_parse(g_buf, (size_t)(end - g_buf));
    Model* b = model_parse(g_buf, (size_t)(end - g_buf));
    CHECK(a && b && model_digest(a) == model_digest(b), "digest: deterministic");
    if (a)
        model_free(a);
    if (b)
        model_free(b);
}

/* --- real-corpus validation ---------------------------------------------------
 * Enumeration: every TOC entry, later mounts shadowing earlier ones (the
 * resource layer rule the python ground truth mirrors), sorted by normalized
 * key; each .lwo parsed, sibling .uv applied. Ground truth extracted from
 * the shipped wads with tools/rrlwo.py + tools/rrwad.py. */

#define REAL_LWO 1132
#define REAL_UV_APPLIED 60
#define REAL_VERTS 36506
#define REAL_POLYS 20119
#define REAL_SURFACES 3396
#define REAL_DIGEST 0x18a8f7e3u

typedef struct {
    char key[256];
    char raw[256];
} Entry;

static Entry* s_ent;
static int s_nent, s_capent;

static int ent_add(const char* key, const char* raw)
{
    for (int i = 0; i < s_nent; i++) {
        if (strcmp(s_ent[i].key, key) == 0) {
            snprintf(s_ent[i].raw, sizeof s_ent[i].raw, "%s", raw);
            return 0; /* higher mount wins */
        }
    }
    if (s_nent == s_capent) {
        int ncap = s_capent ? s_capent * 2 : 256;
        Entry* ne = (Entry*)realloc(s_ent, ncap * sizeof *ne);
        if (!ne)
            return -1;
        s_ent = ne;
        s_capent = ncap;
    }
    snprintf(s_ent[s_nent].key, sizeof s_ent[s_nent].key, "%s", key);
    snprintf(s_ent[s_nent].raw, sizeof s_ent[s_nent].raw, "%s", raw);
    s_nent++;
    return 0;
}

static int ent_cmp(const void* a, const void* b)
{
    return strcmp(((const Entry*)a)->key, ((const Entry*)b)->key);
}

static void fold(char* d, const char* s)
{
    int i = 0;
    for (; s[i] && i < 255; i++) {
        char c = s[i];
        if (c >= 'A' && c <= 'Z')
            c += 0x20;
        if (c == '\\')
            c = '/';
        d[i] = c;
    }
    d[i] = 0;
}

static void dga(unsigned* h, unsigned v)
{
    for (int i = 0; i < 4; i++) {
        *h ^= v & 0xff;
        v >>= 8;
        *h *= 16777619u;
    }
}

static void test_real(const char* dir)
{
    int mounted = resource_mount(dir, "LegoRR");
    CHECK(mounted >= 2, "real: both wads mounted");

    for (int w = 0; w < resource_mounted_count(); w++) {
        uint32_t n = resource_wad_entry_count(w);
        for (uint32_t j = 0; j < n; j++) {
            const char* nm = resource_wad_entry_name(w, j);
            if (!nm)
                continue;
            char key[256];
            fold(key, nm);
            if (ent_add(key, nm) != 0)
                return;
        }
    }
    qsort(s_ent, s_nent, sizeof s_ent[0], ent_cmp);

    unsigned h = 2166136261u;
    int models = 0, parsed = 0, failed = 0, uvapplied = 0;
    long verts = 0, polys = 0, surfs = 0;
    for (int i = 0; i < s_nent; i++) {
        size_t kl = strlen(s_ent[i].key);
        if (kl < 4 || strcmp(s_ent[i].key + kl - 4, ".lwo") != 0)
            continue;
        models++;
        Resource* r = resource_open(s_ent[i].raw, RESOURCE_WAD_ONLY);
        if (!r || resource_read(r) <= 0) {
            printf("FAIL: real: open %s\n", s_ent[i].key);
            if (r)
                resource_free(r);
            failed++;
            continue;
        }
        Model* m = model_parse(r->data, (size_t)r->size);
        resource_free(r);
        if (!m) {
            printf("FAIL: real: parse %s\n", s_ent[i].key);
            failed++;
            continue;
        }
        parsed++;

        char sib[256];
        snprintf(sib, sizeof sib, "%s", s_ent[i].raw);
        char* dot = strrchr(sib, '.');
        if (dot) {
            strcpy(dot, ".uv");
            Resource* u = resource_open(sib, RESOURCE_WAD_ONLY);
            if (u) {
                if (resource_read(u) > 0
                    && model_apply_uv_text(m, (const char*)u->data, (size_t)u->size))
                    uvapplied++;
                resource_free(u);
            }
        }

        verts += (long)m->n_verts;
        polys += (long)m->n_polys;
        surfs += (long)m->n_surfaces;
        dga(&h, model_digest(m));
        model_free(m);
    }

    printf("real: models=%d parsed=%d failed=%d uv=%d verts=%ld polys=%ld "
           "surfaces=%ld digest=%08x\n",
           models, parsed, failed, uvapplied, verts, polys, surfs, h);
    CHECK(models == REAL_LWO, "real: 1132 .lwo entries enumerated");
    CHECK(parsed == REAL_LWO, "real: all 1132 parse");
    CHECK(failed == 0, "real: no read or parse failures");
    CHECK(uvapplied == REAL_UV_APPLIED, "real: 60 .uv sidecars applied");
    CHECK(verts == REAL_VERTS, "real: vertex total matches ground truth");
    CHECK(polys == REAL_POLYS, "real: polygon total matches ground truth");
    CHECK(surfs == REAL_SURFACES, "real: surface total matches ground truth");
    CHECK(h == REAL_DIGEST, "real: corpus digest matches python ground truth");
}

static void test_real_spots(void)
{
    /* the zero-origin POLS files finding 11 §3 lists (as TOC paths): the
     * auto-detect means their rings contain index 0 */
    static const char* zero_based[] = {
        "MiscAnims\\BirdScarer\\1.lwo", "MiscAnims\\Effects\\BEAM.LWO",
        "Creatures\\Spider\\M_SFang2.lwo", "Buildings\\Powerstation\\Connecting_Pipe.lwo",
    };
    for (int i = 0; i < 4; i++) {
        Resource* r = resource_open(zero_based[i], RESOURCE_WAD_ONLY);
        if (!r || resource_read(r) <= 0) {
            printf("FAIL: spot: open %s\n", zero_based[i]);
            if (r)
                resource_free(r);
            continue;
        }
        Model* m = model_parse(r->data, (size_t)r->size);
        CHECK(m != NULL, "spot: 0-based file parses");
        int sawzero = 0;
        if (m) {
            for (size_t p = 0; p < m->n_polys && !sawzero; p++)
                for (uint16_t k = 0; k < m->polys[p].count; k++)
                    if (m->polys[p].ring[k] == 0)
                        sawzero = 1;
            CHECK(sawzero == 1, "spot: 0-based file carries index 0");
            model_free(m);
        }
        resource_free(r);
    }

    /* the .uv story end-to-end: Creatures\LavaMonster\Arml.lwo (ground truth
     * from the wad: 3 surfaces, 10 polys, uv covers all 10; the sidecar paths
     * replace every TIMG — the LWO had none) */
    Model* arm = model_open("Creatures\\LavaMonster\\Arml.lwo");
    CHECK(arm != NULL, "spot: Arml.lwo opens");
    if (arm) {
        CHECK(arm->uv_applied == 1, "spot: Arml sidecar applied");
        CHECK(arm->n_surfaces == 3 && arm->n_polys == 10, "spot: Arml tables");
        CHECK(arm->surfaces[0].timg
                  && strcmp(arm->surfaces[0].timg,
                            "Z:\\Lego\\data\\creatures\\lavamonster\\LavaTextures.bmp")
                         == 0,
              "spot: Arml uv path override");
        CHECK(arm->polys[0].uv_count == 3 && arm->polys[0].uv != NULL,
              "spot: Arml corner uv");
        if (arm->polys[0].uv_count == 3) {
            CHECK(arm->polys[0].uv[0] > 0.6629f && arm->polys[0].uv[0] < 0.663f,
                  "spot: Arml corner u");
            CHECK(arm->polys[0].uv[1] > -0.245741f && arm->polys[0].uv[1] < -0.245739f,
                  "spot: Arml corner v negated");
        }
        model_free(arm);
    }
}

int main(int argc, char** argv)
{
    test_basic();
    test_pols();
    test_srfs_surf();
    test_uv();
    test_be_float_cursor();
    test_digest_stability();
    /* The string helper leaves (2026-10-25): the even-rounded dup and the
     * projection-mode table parse. */
    {
        char* dup = lwo_string_duplicate("ab");
        CHECK(dup != NULL && strcmp(dup, "ab") == 0,
              "the even-length dup copies");
        CHECK(dup != NULL && dup[2] == '\0' && dup[3] == '\0',
              "the even-length block rides NUL to the rounded end");
        free(dup);

        char* odd = lwo_string_duplicate("abc");
        CHECK(odd != NULL && strcmp(odd, "abc") == 0 && odd[3] == '\0'
                  && odd[4] == '\0',
              "the odd-length dup pads even and stays terminated");
        free(odd);

        char* empty = lwo_string_duplicate("");
        CHECK(empty != NULL && empty[0] == '\0',
              "the empty dup allocates the rounded byte block");
        free(empty);

        CHECK(lwo_string_duplicate(NULL) == NULL,
              "the null guard answers null");

        CHECK(lwo_texture_projection_mode_parse("PLANAR") == 0,
              "PLANAR answers 0");
        CHECK(lwo_texture_projection_mode_parse("CYLINDRICAL") == 1,
              "CYLINDRICAL answers 1");
        CHECK(lwo_texture_projection_mode_parse("SPHERICAL") == 2,
              "SPHERICAL answers 2");
        CHECK(lwo_texture_projection_mode_parse("spherical") == 2,
              "the fold reads the name case-blind");
        CHECK(lwo_texture_projection_mode_parse("SPHERICALX") == 2,
              "the prefix match wins");
        CHECK(lwo_texture_projection_mode_parse("cyl") == 0
                  && lwo_texture_projection_mode_parse("other") == 0
                  && lwo_texture_projection_mode_parse(NULL) == 0,
              "the no-match fall-through answers PLANAR");
    }

    if (argc >= 3 && strcmp(argv[1], "--real") == 0) {
        test_real(argv[2]);
        test_real_spots();
    }

    if (s_failures == 0)
        printf("model: all checks passed\n");
    return s_failures ? 1 : 0;
}
