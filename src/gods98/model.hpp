// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — LWO model loader + .uv sidecar.
 *
 * Reconstruction of the model pipeline the corpus calls FUN_0048d7c0 (create)
 * / FUN_0048d2c0 (FORM walker) with handlers FUN_0048c210 (PNTS),
 * FUN_0048c360 (CRVS), FUN_0048c3e0 (POLS), FUN_0048c690 (SRFS) and
 * FUN_0048c820 (SURF), plus the
 * `.uv` sidecar parser FUN_00481c50 (findings 11 §2-3, §5b and 19 §3).
 *
 * Format (LightWave 4.0 generation, LWO not LW0): `FORM <u32 size> LWOB`,
 * then 4-byte-tag chunks {tag, u32 BE size, payload, pad to even} while the
 * chunk table stays inside `8 + FORM size`. Everything multi-byte is big
 * endian. The engine scans then discards CRVS records; other unsupported
 * chunks (VMAP, TEXT, ...) are skipped. The portable SURF view retains the
 * renderer fields COLR/TIMG/CTEX/TTEX, TFLG/FLAG, TSIZ/TCTR, and six scalar
 * material values. Other SURF blocks are safely skipped.
 *
 * Semantics mirrored (finding 11 §3 + finding 19, corpus-validated by the
 * rrlwo.py converter over all 1,132 shipped models):
 *   PNTS  f32 BE triples; the latest chunk supplies vertices (the retail
 *         loader replaces its current point array for each PNTS chunk)
 *   POLS  per poly: u16 count, count x i16 indices, u16 surface.
 *         Negative index = the polygon is wound backwards -> `flipped`.
 *         Index origin auto-detected per file: zero anywhere -> 0-based,
 *         else 1-based (1-based forbids zero; 4 shipped files are 0-based).
 *         Surface = u16 - 1, stored with 16-bit wraparound.
 *   SRFS  NUL-terminated names, even-padded records              surfaces
 *   SURF  one named record per chunk (`name\0` followed by even-padded
 *         subchunks); the portable view retains color, image/projection,
 *         texture flags, size/center, and six scalar material values. When
 *         names repeat, the first matching SURF record supplies settings.
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
    uint8_t colr_bytes[4]; /* original RGBA bytes, including file alpha */
    int    has_colr;
    uint16_t texture_flags; /* TFLG bit field; chunks accumulate bits */
    uint16_t material_flags; /* FLAG bit field */
    int    has_texture_flags;
    int    has_material_flags;
    int    projection_mode; /* TTEX: PLANAR=0, CYLINDRICAL=1, SPHERICAL=2 */
    int    has_projection_mode;
    int    animated_image_sequence; /* TIMG name contains `(sequence)` */
    char*  texture_definition; /* CTEX text, if present */
    float  tsiz[3];   /* TSIZ planar scale — valid iff has_tsiz */
    int    has_tsiz;
    float  tctr[3];   /* TCTR texture center — valid iff has_tctr */
    int    has_tctr;
    float  luminosity; /* LUMI, normalized from u16 / 256 */
    float  diffuse;    /* DIFF, normalized from u16 / 256 */
    float  specular;   /* SPEC, normalized from u16 / 256 */
    float  reflection; /* REFL, normalized from u16 / 256 */
    float  glossiness; /* GLOS, normalized from u16 / 256 */
    float  transparency; /* TRAN, normalized from u16 / 256 */
    uint8_t material_values_mask; /* bit order follows fields above */
} ModelSurface;

typedef struct ModelPoly {
    uint16_t* ring;   /* vertex indices, 0-based, ring order */
    uint16_t  count;
    uint16_t  surface;/* surface index (POLS field - 1, with 16-bit wrap) */
    int       flipped;/* the ring declared any negative index */
    float*    uv;     /* uv_count pairs (u, -v) per corner from the sidecar */
    size_t    uv_count;
} ModelPoly;

typedef struct ModelTextureBinding ModelTextureBinding;
typedef struct Model Model;
/* Host-width source record created with LwoModelEntryLoad. It keeps the
 * parsed asset and its model-relative texture root together for render
 * owners that enqueue this model once per actual scene instance. */
typedef struct ModelRenderSourceRecord {
    Model* model;
    char path_prefix[260];
} ModelRenderSourceRecord;

struct Model {
    float*       verts;     /* 3 * n_verts, PNTS verbatim */
    size_t       n_verts;
    ModelPoly*   polys;     /* POLS order */
    size_t       n_polys;
    ModelSurface* surfaces; /* SRFS order */
    size_t       n_surfaces;
    int          uv_applied;/* a .uv sidecar was parsed and applied */
    /* Host-side material bindings populated by LwoModelEntryLoad. Renderer
     * submission remains owned by MaterialPass, but keeping the bindings
     * here gives later host render paths the same loaded texture lifetime. */
    ModelTextureBinding* material_bindings;
    size_t       material_binding_count;
    ModelRenderSourceRecord* render_source;
};

/* Legacy model/resource list node (the six-word record used by the
 * 0x451ab0 constructor and 0x451b30 flatten operation). */
#define LRR_MODEL_ENTRY_RECORD_DEFINED 1
typedef struct ModelEntryRecord {
    void* object;
    void* instance;
    char* name;
    int kind;
    uint32_t flags;
    struct ModelEntryRecord* next;
} ModelEntryRecord;

extern "C" ModelEntryRecord* model_primitive_record_create(ModelEntryRecord* previous,
                                                const char* name, int kind);
extern "C" ModelEntryRecord* model_entry_list_flatten(const ModelEntryRecord* head);

/* Open through the resource layer (finding 01 resolution) and parse. `path`
 * may be a base name or an explicit `.lwo` path; the loader opens `<base>.lwo`
 * and then tries the sibling `.uv` sidecar. Present-and-good sidecars apply,
 * anything else leaves the model as the LWO states it.
 * Returns NULL on open/read/parse failure. */
Model* model_open(const char* path);

/* Portable parsed-model adapter for LwoModelEntryLoad: opens the model from
 * the full path. It does not create the retail renderer resource instance. */
Model* lwo_model_entry_load(const char* path, void* flags, void* context);

/* Portable parsed-model adapter: case-insensitive cache over the joined
 * "%s%s" path, with the raw-path secondary table under the fallback
 * prefix. Table hits can invoke a portable notification hook; Model does not
 * contain the renderer template expected by FUN_00480900. */
typedef void (*ModelResourceInstanceHook)(void* record, void* flags);
void model_resource_instance_hook_set(ModelResourceInstanceHook hook);
void model_resource_fallback_prefix_set(const char* prefix);
const char* model_resource_fallback_prefix_get(void);
Model* model_resource_resolve_or_load(const char* prefix, const char* path,
                                      void* flags, void* context);
void model_resource_cache_clear(void);

/* Parse a caller-owned buffer; for tests and already-resident data. */
Model* model_parse(const uint8_t* data, size_t len);

/* Buffered-text adapter for ParseUV; retail reads the .uv stream directly.
 * Returns 1 when applied
 * (possibly only up to the first unreadable tail block), 0 when the file
 * does not match the grammar (model untouched) or on allocation failure. */
int model_apply_uv_text(Model* m, const char* text, size_t len);

void model_free(Model* m);

/* FNV-1a digest over the canonical parsed view (verts bit-for-bit, rings +
 * flip + surface + uv pairs, surface names/colr/timg/tsiz) — used to diff
 * against the python ground truth over the shipped corpus. */
uint32_t model_digest(const Model* m);

/* LwoReadBigEndianFloat (corpus FUN_0048c1d0): consume four bytes at the
 * cursor in stream (big-endian) order, hand back the bytes byte-reversed,
 * advance the cursor, answer 1. Interpreting the output as little-endian
 * recovers the big-endian float the corpus's PNTS/SURF readers wanted. The
 * port's model path parses through the rrlwo converter (finding 11), so the
 * reader ships as a standalone primitive. */
int lwo_read_big_endian_float(const uint8_t** cursor, uint8_t out[4]);
void mesh_vertex_coordinate_remap(int vertex_count, const float* positions_xyz,
                                 float* coordinates_xy, float scale_x,
                                 float scale_y, float scale_z, float offset_x,
                                 float offset_y, float offset_z, uint32_t flags);
                                                      /* FUN_00481fa0 */

/* LwoStringDuplicate (FUN_0048c0c0): the corpus's dup allocates strlen + 1
 * rounded UP TO THE NEXT EVEN size through the engine heap, zeroes the
 * whole block (dwords then bytes), then copies strlen + 1 bytes — the
 * corpus's own loop bounds, assembly-pinned. Deviations: the null string
 * is guarded, and the copy rides the rounded size so the block stays a
 * valid C string. LwoTextureProjectionModeParse (FUN_0048c120): a fold
 * compare (direct strnicmp_lite carrier call, as the corpus) against
 * the binary's PLANAR / CYLINDRICAL / SPHERICAL table; the first prefix
 * match answers the table index, anything else answers 0 — PLANAR. */
char* lwo_string_duplicate(const char* text);        /* FUN_0048c0c0 */
int   lwo_texture_projection_mode_parse(const char* name);
                                                      /* FUN_0048c120 */

#endif /* LEGORRR_MODEL_HPP */
