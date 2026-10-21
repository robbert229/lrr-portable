// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — LWO model loader + .uv sidecar.
 *
 * Reconstruction of the model pipeline the corpus calls FUN_0048d7c0 (create)
 * / FUN_0048d2c0 (FORM walker) with handlers FUN_0048c210 (PNTS),
 * FUN_0048c3e0 (POLS), FUN_0048c690 (SRFS) and FUN_0048c820 (SURF), plus the
 * `.uv` sidecar parser FUN_00481c50 (findings 11 §2-3, §5b and 19 §3).
 *
 * Format (LightWave 4.0 generation, LWO not LW0): `FORM <u32 size> LWOB`,
 * then 4-byte-tag chunks {tag, u32 BE size, payload, pad to even} while the
 * chunk table stays inside `8 + FORM size`. Everything multi-byte is big
 * endian. The engine ignores everything outside PNTS/POLS/SRFS/SURF (CRVS,
 * VMAP, TEXT, ...) — our loader does too; SURF sub-blocks keep only the
 * vocabulary the engine binds: COLR (material colour), TIMG (texture path)
 * and TSIZ (planar-map scale), everything else parsed over.
 *
 * Semantics mirrored (finding 11 §3 + finding 19, corpus-validated by the
 * rrlwo.py converter over all 1,132 shipped models):
 *   PNTS  f32 BE triples, concatenated across chunks             vertices
 *   POLS  per poly: u16 count, count x i16 indices, u16 surface.
 *         Negative index = the polygon is wound backwards -> `flipped`.
 *         Index origin auto-detected per file: zero anywhere -> 0-based,
 *         else 1-based (1-based forbids zero; 4 shipped files are 0-based).
 *         Surface = signed u16 - 1, clamped at 0.
 *   SRFS  NUL-terminated names, even-padded records              surfaces
 *   SURF  records anchored on the SRFS names (`name\0` at an even offset);
 *         last matching record wins per field, first record per name wins
 *         for record visibility.
 *   .uv   text sidecar next to the .lwo: per-surface texture-path lines
 *         replace that surface's TIMG, and per-polygon-corner "u v" blocks
 *         attach per-corner UVs (v negated as stored). Trailing keyframe
 *         blocks parse-fail into a stop, not an error, exactly like the
 *         engine; a structurally bad sidecar is dropped whole.
 *
 * This is the geometry the entity assembly (`.ae` FILE records) feeds to the
 * scene; binding surfaces to the texture cache and draw assembly belong to
 * the render integration, not here.
 */
#ifndef LEGORRR_MODEL_HPP
#define LEGORRR_MODEL_HPP

#include <stddef.h>
#include <stdint.h>

typedef struct ModelSurface {
    char*  name;      /* SRFS name (may be empty) */
    char*  timg;      /* TIMG path, or NULL; the .uv path line replaces it */
    float  colr[4];   /* COLR rgb / 255 with alpha 1 — valid iff has_colr */
    int    has_colr;
    float  tsiz[2];   /* TSIZ planar scale — valid iff has_tsiz */
    int    has_tsiz;
} ModelSurface;

typedef struct ModelPoly {
    uint16_t* ring;   /* vertex indices, 0-based, ring order */
    uint16_t  count;
    uint16_t  surface;/* surface index (POLS field - 1, clamped to >= 0) */
    int       flipped;/* the ring declared any negative index */
    float*    uv;     /* uv_count pairs (u, -v) per corner from the sidecar */
    size_t    uv_count;
} ModelPoly;

typedef struct Model {
    float*       verts;     /* 3 * n_verts, PNTS verbatim */
    size_t       n_verts;
    ModelPoly*   polys;     /* POLS order */
    size_t       n_polys;
    ModelSurface* surfaces; /* SRFS order */
    size_t       n_surfaces;
    int          uv_applied;/* a .uv sidecar was parsed and applied */
} Model;

/* Open through the resource layer (finding 01 resolution) and parse. When
 * the name ends in ".lwo" a sibling ".uv" is tried too — present-and-good
 * sidecars apply, anything else leaves the model as the LWO states it
 * (the corpus opens the sidecar only when the uv slot is requested).
 * Returns NULL on open/read/parse failure. */
Model* model_open(const char* path);

/* Parse a caller-owned buffer; for tests and already-resident data. */
Model* model_parse(const uint8_t* data, size_t len);

/* Parse and apply a .uv sidecar per finding 19 §3. Returns 1 when applied
 * (possibly only up to the first unreadable tail block), 0 when the file
 * does not match the grammar (model untouched) or on allocation failure. */
int model_apply_uv_text(Model* m, const char* text, size_t len);

void model_free(Model* m);

/* FNV-1a digest over the canonical parsed view (verts bit-for-bit, rings +
 * flip + surface + uv pairs, surface names/colr/timg/tsiz) — used to diff
 * against the python ground truth over the shipped corpus. */
uint32_t model_digest(const Model* m);

#endif /* LEGORRR_MODEL_HPP */
