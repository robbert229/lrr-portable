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
#include "model_for_lws.hpp"
#include "model_from_lws.hpp"

ModelResourcePair
    s_model_resource_primary[MODEL_RESOURCE_CACHE_CAPACITY];
ModelResourcePair
    s_model_resource_secondary[MODEL_RESOURCE_CACHE_CAPACITY];
/* DAT_005446b8: the fallback prefix the resolver retries with; installed by
 * the port through model_resource_fallback_prefix_set. */
const char* s_model_resource_fallback_prefix;
ModelResourceInstanceHook s_model_resource_instance_hook;
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

/* --- growable byte buffer --------------------------------------------------- */

/* --- parsing context --------------------------------------------------------- */

/* --- helpers ----------------------------------------------------------------- */

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

/* --- POLS --------------------------------------------------------------------- */

/* --- SURF ---------------------------------------------------------------------- */

/* --- model build ----------------------------------------------------------------- */

/* --- parse entry -------------------------------------------------------------------- */

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


} // namespace unported_ledger_stubs
