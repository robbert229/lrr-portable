// SPDX-License-Identifier: LGPL-3.0-or-later
#include <math.h>
#include <string.h>

#include "view.hpp"
#include "record.hpp"

/* Reconstruction-owned globals for corpus constants. */
float g_view_follow_scale = 1.0f;   /* DAT_005560a4 (grid pick follow scale) */
int   g_view_grid_edge   = 0;       /* DAT_005560ac (tile-count edge clamp) */
int   g_view_test_blend  = 0;       /* test hook for the mode-3 EMA target */

/* ---- pure vector helpers ------------------------------------------------
 * Corpus mapping (finding 280; the corpus bodies use x87 float10, the port
 * computes the byte-equivalent single-precision results):
 *   FUN_00401720 vec_scale, FUN_00401500 vec_add,  FUN_004016c0 vec_sub,
 *   FUN_00401470 vec_cross, FUN_00401750 vec_normalize,
 *   FUN_00401420 vec_dist,  FUN_004016f0 vec_dot,  FUN_004013a0 vec_len,
 *   FUN_00401240 vec2_dist, FUN_00406730 vec2_dist_alt,
 *   FUN_0047c0c0 vec_exact_equal. */

void vec_scale(float* dst, const float* src, float s) /* FUN_00401720 */
{
    dst[0] = s * src[0];
    dst[1] = s * src[1];
    dst[2] = s * src[2];
}

void vec_add(float* dst, const float* a, const float* b) /* FUN_00401500 */
{
    dst[0] = a[0] + b[0];
    dst[1] = a[1] + b[1];
    dst[2] = a[2] + b[2];
}

void vec_sub(float* dst, const float* a, const float* b) /* FUN_004016c0 */
{
    dst[0] = a[0] - b[0];
    dst[1] = a[1] - b[1];
    dst[2] = a[2] - b[2];
}

void vec_cross(float* dst, const float* b, const float* c) /* FUN_00401470 */
{
    dst[0] = b[1] * c[2] - b[2] * c[1];
    dst[1] = b[2] * c[0] - b[0] * c[2];
    dst[2] = b[0] * c[1] - b[1] * c[0];
}

void vec_normalize(float* v) /* FUN_00401750 (zero length unguarded, as corpus) */
{
    float s = 1.0f / sqrtf(v[0] * v[0] + v[2] * v[2] + v[1] * v[1]);
    v[0] = s * v[0];
    v[1] = s * v[1];
    v[2] = s * v[2];
}

float vec_dist(const float* a, const float* b) /* FUN_00401420 */
{
    float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

float vec_dot(const float* a, const float* b) /* FUN_004016f0 */
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

float vec_len(const float* a) /* FUN_004013a0 */
{
    return sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
}

float vec2_dist(const float* a, const float* b) /* FUN_00401240 */
{
    float dx = a[0] - b[0];
    float dy = a[1] - b[1];
    return sqrtf(dx * dx + dy * dy);
}

float vec2_dist_alt(const float* a, const float* b) /* FUN_00406730 */
{
    /* Corpus variant: dy term first, x term through an extra round-trip —
     * identical single-precision result. */
    float dy = a[1] - b[1];
    float dx = a[0] - b[0];
    return sqrtf(dy * dy + dx * dx);
}

int vec_exact_equal(const float* a, const float* b) /* FUN_0047c0c0 */
{
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}

/* ---- geometry helpers (finding 363/323) --------------------------------
 * Pure math leaves, corpus-exact operations: the corpus computes on x87
 * long doubles, the port on single precision (documented deviation). */

/* PointDistanceWithinRadius (FUN_0045b8a0): true when the point
 * (x1,y1)-(x2,y2) distance is under `radius`. */
int point_distance_within_radius(float x1, float y1, float x2, float y2,
                                 float radius)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrtf(dx * dx + dy * dy) < radius;
}

/* RectContainsPointHalfOpen (FUN_0045e660): rect is the corpus record's
 * float quad {x, y, w, h} at +0x4…+0x10 — half-open in both axes. */
int rect_contains_point_half_open(const float rect_xywh[4], int x, int y)
{
    if (((rect_xywh[0] <= (float)x) && ((float)x < rect_xywh[2] + rect_xywh[0]))
        && ((rect_xywh[1] <= (float)y)
            && ((float)y < rect_xywh[3] + rect_xywh[1]))) {
        return 1;
    }
    return 0;
}

/* ---- offset-point rect family (findings 241/363/446) --------------------- */

/* Shared body of the three corpus leaves: the __ftol-truncated float
 * offsets subtract from the point, then the quad bounds test the shifted
 * point half-open — x_min <= fx < x_max, y_min <= fy < y_max. */
static int rect_contains_converted(const float rect[4], float off_x,
                                   float off_y, int x, int y)
{
    const float fx = (float)(x - (int)off_x);
    const float fy = (float)(y - (int)off_y);
    return (rect[0] <= fx && fy < rect[3]
            && rect[1] <= fy && fx < rect[2])
               ? 1 : 0;
}

/* RectContainsOffsetPoint (FUN_0045b590): the category-cell hit walk's
 * containment test — bounds quad plus the float offset pair. */
int rect_contains_offset_point(const float rect_bounds[4], float off_x,
                               float off_y, int x, int y)
{
    return rect_contains_converted(rect_bounds, off_x, off_y, x, y);
}

/* RectContainsPointByConvertedCoords (FUN_00463c00): the region-route
 * walk's copy — the pseudocode is identical to the category walk's. */
int rect_contains_point_by_converted_coords(const float rect_bounds[4],
                                            float off_x, float off_y,
                                            int x, int y)
{
    return rect_contains_converted(rect_bounds, off_x, off_y, x, y);
}

/* RectContainsPointFromObjectBounds (FUN_00463c90): same test, bounds via
 * the pointer stored at the record's +0x14. The corpus dereferences the
 * slot unguarded; the port answers 0 for a NULL slot or rect. */
int rect_contains_point_from_object_bounds(const float* const* bounds_at_14,
                                           float off_x, float off_y,
                                           int x, int y)
{
    if (bounds_at_14 == NULL || *bounds_at_14 == NULL) {
        return 0;
    }
    return rect_contains_converted(*bounds_at_14, off_x, off_y, x, y);
}

/* RayPlaneIntersectionParameter (FUN_00479a30): writes t = dot(q-p, n) /
 * dot(d, n) into `t_out` for ray (p,d) against plane (q,n); answers 1 when
 * not parallel, 0 otherwise. */
int ray_plane_intersection_parameter(float* t_out, const float* p,
                                     const float* d, const float* q,
                                     const float* n)
{
    const float denom = n[0] * d[0] + n[2] * d[2] + n[1] * d[1];
    if (denom != 0.0f) {
        *t_out = ((q[0] - p[0]) * n[0] + (q[1] - p[1]) * n[1]
                  + (q[2] - p[2]) * n[2]) / denom;
        return 1;
    }
    return 0;
}

/* LineIntersection2DCompute (FUN_00479af0): intersects line1
 * (point p1, direction d1) with line2 (point p2, direction d2); writes the
 * hit point into `out` and returns it, or NULL when parallel. */
float* line_intersection_2d_compute(float* out, const float* p1,
                                    const float* d1, const float* p2,
                                    const float* d2)
{
    float cross = -d1[1] * d2[0] + d1[0] * d2[1];
    if (cross != 0.0f) {
        float t = (p1[0] - p2[0]) * -(d2[1] / cross)
                  + (d2[0] / cross) * (p1[1] - p2[1]);
        out[0] = t * d1[0];
        out[1] = t * d1[1];
        out[0] = out[0] + p1[0];
        out[1] = out[1] + p1[1];
        return out;
    }
    return NULL;
}

/* SphereRayIntersectionDiscriminantPositive (FUN_00479c10): positive-
 * discriminant test of ray `dir` against the sphere (center, radius). The
 * corpus's exact discriminant expression is reproduced verbatim. */
int sphere_ray_intersection_discriminant_positive(const float* center,
                                                  float radius2,
                                                  const float* origin,
                                                  const float* dir)
{
    const float wy = origin[1] - center[1];
    const float wz = origin[2] - center[2];
    const float wx = origin[0] - center[0];
    const float b = (dir[2] + dir[2]) * wz + (dir[1] + dir[1]) * wy
                    + (dir[0] + dir[0]) * wx;
    if (0.0f < b * b
        - ((wx * wx + wz * wz + wy * wy) * 4.0f - radius2 * radius2)) {
        return 1;
    }
    return 0;
}

/* ---- 4x4 matrices + polygon parity (finding 309/388) --------------------- */

/* Matrix4SetZero (FUN_00479ea0): the corpus's 16 zero stores. */
void matrix4_set_zero(float* out)
{
    for (int i = 0; i < 16; ++i) {
        out[i] = 0.0f;
    }
}

/* Matrix4Multiply (FUN_00479ce0): the corpus takes (out, right, left),
 * zeroes the output, then accumulates each cell as the four-term dot
 * product out[row][col] = Σ_k left[row][k] · right[k][col] — i.e.
 * out = left × right, row-major. */
void matrix4_multiply(float* out, const float* right, const float* left)
{
    matrix4_set_zero(out);
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            float* dst = out + 4 * row + col;
            for (int k = 0; k < 4; ++k) {
                *dst = left[4 * row + k] * right[4 * k + col] + *dst;
            }
        }
    }
}

/* PointInPolygon2D (FUN_00479b80): even-odd crossing parity. Edge k runs
 * verts[k] → next[k]; the corpus strides its two read pointers in lockstep,
 * so the in-lockstep indexing below reads the same bytes for any caller
 * passing the shifted-ring pair. The boundary semantics, zero-span divide
 * and all, are the corpus's. */
int point_in_polygon_2d(const float point[2], const float* verts,
                        const float* next, int count)
{
    int crossings = 0;
    for (int k = 0; k < count; ++k) {
        const float x0 = verts[2 * k];
        const float y0 = verts[2 * k + 1];
        const float x1 = next[2 * k];
        const float y1 = next[2 * k + 1];
        if (((y0 <= point[1] && point[1] <= y1)
             || (point[1] <= y0 && y1 <= point[1]))
            && point[0] < (x1 - x0) * ((point[1] - y0) / (y1 - y0)) + x0) {
            ++crossings;
        }
    }
    return crossings & 1;
}

/* ---- matrix/ray math leaves (2026-10-24) ---------------------------------
 * The x87 float10 deviation for the rotation setters is documented in the
 * header; every other body is byte-equivalent single-precision arithmetic.
 */

/* PointAlongRay (FUN_00479ab0): out = origin + t · dir. The corpus reads
 * dir[1]/dir[2] into registers before the stores; the x term is t*dir +
 * origin while y/z add to the origin first — component order mirrored. */
void point_along_ray(float* out, const float* origin, const float* dir,
                     float t)
{
    const float dy = dir[1];
    const float dz = dir[2];
    out[0] = t * dir[0] + origin[0];
    out[1] = origin[1] + dy * t;
    out[2] = origin[2] + dz * t;
}

/* ViewRayIntersectionTest (FUN_004798e0): normalizes both direction
 * vectors (the corpus sums each normalization as z²+y²+x², its SQRT
 * accumulator order), runs the ray (p, dir_a) against the plane through q
 * with normal dir_b, and advances along dir_a when they meet. */
int view_ray_intersection_test(float* out, const float* p,
                               const float* dir_a, const float* q,
                               const float* dir_b)
{
    const float s_a = 1.0f / sqrtf(dir_a[2] * dir_a[2] + dir_a[1] * dir_a[1]
                                   + dir_a[0] * dir_a[0]);
    const float s_b = 1.0f / sqrtf(dir_b[2] * dir_b[2] + dir_b[1] * dir_b[1]
                                   + dir_b[0] * dir_b[0]);
    const float n_a[3] = { s_a * dir_a[0], s_a * dir_a[1], s_a * dir_a[2] };
    const float n_b[3] = { s_b * dir_b[0], s_b * dir_b[1], s_b * dir_b[2] };
    float t;
    if (ray_plane_intersection_parameter(&t, p, n_a, q, n_b) != 0) {
        point_along_ray(out, p, n_a, t);
        return 1;
    }
    return 0;
}

/* Matrix4SetIdentity (FUN_00479e70): row loop, column loop, store 1 when
 * the indices meet (the corpus's dword-0x3f800000 form). */
void matrix4_set_identity(float* out)
{
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            out[4 * row + col] = (row == col) ? 1.0f : 0.0f;
        }
    }
}

/* MatrixIdentityAndTranslationSet (FUN_00479e40): identity, then the
 * translation triad at byte offsets +0x30/+0x34/+0x38 = row 3. */
void matrix4_identity_translation_set(float* out, const float* translation)
{
    matrix4_set_identity(out);
    out[12] = translation[0];
    out[13] = translation[1];
    out[14] = translation[2];
}

/* Matrix4Copy (FUN_00479eb0): the 16 dword copies. */
void matrix4_copy(float* out, const float* src)
{
    for (int i = 0; i < 16; ++i) {
        out[i] = src[i];
    }
}

/* Matrix4SetRotationX (FUN_00479d50; the reference-name table also calls
 * this Z, alongside FUN_00479df0): identity, then the rows 1–2 block with
 * the corpus's signs — m[5]=c, m[6]=−s, m[9]=s, m[10]=c. */
void matrix4_set_rotation_x(float* out, float angle)
{
    const float c = (float)cos(angle);
    const float s = (float)sin(angle);
    matrix4_set_identity(out);
    out[5] = c;
    out[6] = -s;
    out[9] = s;
    out[10] = c;
}

/* Matrix4SetRotationY (FUN_00479da0): identity, then m[0]=c, m[2]=s,
 * m[8]=−s, m[10]=c. */
void matrix4_set_rotation_y(float* out, float angle)
{
    const float c = (float)cos(angle);
    const float s = (float)sin(angle);
    matrix4_set_identity(out);
    out[0] = c;
    out[2] = s;
    out[8] = -s;
    out[10] = c;
}

/* Matrix4SetRotationZ (FUN_00479df0): identity, then m[0]=c, m[1]=−s,
 * m[4]=s, m[5]=c. */
void matrix4_set_rotation_z(float* out, float angle)
{
    const float c = (float)cos(angle);
    const float s = (float)sin(angle);
    matrix4_set_identity(out);
    out[0] = c;
    out[1] = -s;
    out[4] = s;
    out[5] = c;
}

/* BuildPerspectiveProjectionMatrix (FUN_0040bd60): zero the matrix, raise
 * m[10]/m[15], then the three quotients — the corpus's m[1] multiplies
 * p5/p4 by p4/p1, not p5/p1. */
void perspective_projection_matrix_set(float* out, float p1, float p2,
                                       float p3, float p4, float p5)
{
    for (int i = 0; i < 16; ++i) {
        out[i] = 0.0f;
    }
    out[15] = 1.0f;
    out[10] = 1.0f;
    out[0] = p4 / p1;
    out[5] = p3 / p2;
    out[1] = (p5 / p4) * (p4 / p1);
}

/* ---- camera-handle primitives (record-backed) --------------------------- */

void cam_offset(CameraHandle* c, float x, float y, float z)
{
    if (c == NULL) {
        return;
    }
    c->inuse = 1;
    c->pos[0] += x;
    c->pos[1] += y;
    c->pos[2] += z;
}

void cam_get_pos(CameraHandle* c, float out[3])
{
    if (c != NULL && out != NULL) {
        out[0] = c->pos[0];
        out[1] = c->pos[1];
        out[2] = c->pos[2];
    }
}

void cam_set_pos(CameraHandle* c, float x, float y, float z)
{
    if (c == NULL) {
        return;
    }
    c->inuse = 1;
    c->pos[0] = x;
    c->pos[1] = y;
    c->pos[2] = z;
}

void cam_get_facing(CameraHandle* c, float out[3])
{
    if (c != NULL && out != NULL) {
        out[0] = c->rot[0];
        out[1] = c->rot[1];
        out[2] = c->rot[2];
    }
}

void cam_set_facing(CameraHandle* c, const float* r)
{
    if (c == NULL || r == NULL) {
        return;
    }
    c->inuse = 1;
    c->rot[0] = r[0];
    c->rot[1] = r[1];
    c->rot[2] = r[2];
}

/* ---- world/stub boundaries ---------------------------------------------- */
/*  FUN_0044f6d0 grid pick   -> floor floor of the camera position.
 *  FUN_0044f940 terrain height-> 0 (no level/world data yet).
 *  FUN_00431570/FUN_0044fe70 camera angle -> 0.
 *  FUN_0044a200 object lookup -> 0 (no object layer yet; mode 2 idles).
 *  FUN_0044a070 velocity blend-> returns 0 (targeted EMA deferred).
 *  FUN_0041f940 / FUN_004369f4 / FUN_004798b0 camera steps -> no-ops/1. */

static int grid_pick(EngineContext* ctx, const float* pos, int* gx, int* gy)
{
    /* with a bound level, cells come from the real voxel grid; terrain uses
     * the voxel surface byte (+0x04). */
    if (ctx->level_block != NULL) {
        int w = engine_level_width(ctx);
        int h = engine_level_height(ctx);
        int x = (int)pos[0];
        int y = (int)pos[1];
        if (w > 0 && h > 0 && x >= 0 && y >= 0 && x < w && y < h) {
            *gx = x;
            *gy = y;
            return 1;
        }
        return 0;
    }
    *gx = (int)pos[0];
    *gy = (int)pos[1];
    return 1;
}
static float terrain_height(EngineContext* ctx, float x, float y)
{
    if (ctx->level_block != NULL) {
        return (float)engine_voxel_byte(ctx, (int)x, (int)y, 0x04);
    }
    return 0.0f;
}

/* RandomReachableGridPointSelect (FUN_00450560): the ray-descending
 * grid-point pick — see view.hpp for the plane geometry. Both float*
 * direction arguments ride the corpus's forwarded-dword convention:
 * the point and ray direction are vectors, and Ghidra typed them as
 * plain dwords because FUN_004798e0 only forwards them. The plane-point
 * z quirk is faithful: FUN_004798e0 answers into `out` while the height
 * store lands on the plane quad's z-slot, so the corpus effectively
 * intersects against successively lower planes. */
void random_reachable_grid_point_select(EngineContext* ctx, const float* p,
                                        const float* dir, float* out,
                                        int* gx, int* gy, int rounds)
{
    float plane[3] = { 0.0f, 0.0f, -50.0f };
    const float normal[3] = { 0.0f, 0.0f, -1.0f };
    for (int i = rounds; i != 0; --i) {
        view_ray_intersection_test(out, p, dir, plane, normal);
        plane[2] = terrain_height(ctx, out[0], out[1]);
    }
    (void)grid_pick(ctx, out, gx, gy); /* FUN_0044f6d0 hook */
}

static void cam_angle()
{
    /* TODO(view): FUN_00431570/FUN_0044fe70 camera angle. */
}
static int object_lookup(int /*id*/)
{
    return 0; /* TODO(view): FUN_0044a200 object layer. */
}
static int target_blend(int /*id*/, int /*mode*/, float* blend,
                        float* facing)
{
    if (blend != NULL) {
        blend[0] = blend[1] = blend[2] = 0.0f;
    }
    if (facing != NULL) {
        facing[0] = facing[1] = facing[2] = 0.0f;
    }
    return g_view_test_blend; /* 0 = deferred (TODO: FUN_0044a070) */
}

/* ---- the ported tick ---------------------------------------------------- */

static void view_tail(ViewContext* v, float scale)
{
    /* Corpus tail: when the motion timer has expired, velocity snaps to 0;
     * otherwise it decays toward the step size and the camera is shifted by
     * the negative velocity (glide-to-stop). */
    if (v->timer_num <= v->timer_acc) {
        v->velocity[0] = v->velocity[1] = v->velocity[2] = 0.0f;
    } else {
        float decay = ((float)(v->timer_num - v->timer_acc) / (float)v->timer_num)
                    * v->step;
        /* corpus FUN_004798b0 ramp (D3DRM math helper) modeled as 1.0 */
        vec_scale(v->velocity, v->velocity, decay);
        v->timer_acc += (int)scale;
        cam_offset(&v->cam_world, -v->velocity[0], -v->velocity[1],
                   -v->velocity[2]);
    }
}

void view_tick(ViewContext* v, EngineContext* ctx, float dt, float scale)
{
    if (v == NULL) {
        return;
    }
    EngineState* st = ctx != NULL ? ctx->state_block : NULL;
    (void)st;
    cam_offset(&v->cam_world, -v->velocity[0], -v->velocity[1],
               -v->velocity[2]);

    if (v->mode == 1) {
        /* Follow: keep the follow camera at the offset, then clamp it at the
         * world grid edges (corpus: local_8/local_c vs DAT_005560ac). */
        vec_scale(v->offset, v->offset, g_view_follow_scale);
        cam_offset(&v->cam_follow, v->offset[0], v->offset[1], v->offset[2]);
        float pos[3], fac[3];
        cam_get_pos(&v->cam_follow, pos);
        cam_get_facing(&v->cam_follow, fac);
        (void)fac;
        int gx = 0, gy = 0;
        if (!grid_pick(ctx, pos, &gx, &gy)) {
            cam_offset(&v->cam_follow, -v->offset[0], -v->offset[1],
                       -v->offset[2]);
        } else {
            const int edge = g_view_grid_edge;
            if (gx < edge || engine_level_width(ctx) - edge <= gx) {
                cam_offset(&v->cam_follow, -v->offset[0], 0, 0);
            }
            if (gy < edge || engine_level_height(ctx) - edge <= gy) {
                cam_offset(&v->cam_follow, 0, -v->offset[1], 0);
            }
            float h = terrain_height(ctx, pos[0], pos[1]);
            cam_set_pos(&v->cam_facing, pos[0], pos[1], h);
            cam_angle(); /* rotor; sets yaw below in the corpus */
            float yaw = v->follow_span + (-0.0f);
            cam_set_pos(&v->cam_world, 0.0f, 0.0f, -yaw);
        }
        view_tail(v, scale);
        return;
    }

    if (v->mode == 2) {
        /* Scripted / lagged follow: interpolate toward the target with
         * k = |d|/(|d|+A) once an object target exists (object_lookup). */
        int target = object_lookup(v->follow_id);
        if (target == 0) {
            view_tail(v, scale);
            return;
        }
        float goal3[3], facing[3];
        cam_get_pos(&v->cam_world, goal3);
        cam_get_facing(&v->cam_world, facing);
        float dist = vec_len(goal3);
        float a = (float)v->a3;
        float k = a <= 0.0f ? 0.0f : dist / (dist + a); (void)k;
        /* ...full corpus interpolations follow when the object layer lands */
        view_tail(v, scale);
        return;
    }

    if (v->mode == 3) {
        /* Free/cruise: velocity-EMA toward the target + dirty-flag gating. */
        float pos[3], facing[3];
        cam_get_pos(&v->cam_world, pos);
        cam_get_facing(&v->cam_world, facing);
        float blend[3], tface[3];
        /* TODO(view): FUN_0044a070 velocity-EMA; when the target is not
         * dirty (flags & 0x10) the cached target is used closer. */
        if (target_blend(v->follow_id, v->a5, blend, tface) == 0) {
            view_tail(v, scale);
            return;
        }
        float d1 = vec_dist(tface, facing);
        float d2 = vec_dot(tface, facing);
        (void)d1; (void)d2;
        /* --_DAT_0049e378/dt*_e384 modelling using the real dt */
        float frame = dt > 0.0f ? (0.0333333f / dt) * 0.5f : 1.0f;
        vec_scale(tface, tface, frame);
        vec_add(facing, facing, tface);
        cam_set_pos(&v->cam_world, tface[0], tface[1], tface[2]);
        cam_set_facing(&v->cam_world, facing);
        v->flags |= 0x10;
        view_tail(v, scale);
        return;
    }

    view_tail(v, scale);
}

/* The corpus ticks the three view contexts each frame (finding 03 §4). */
void view_views_tick(EngineContext* ctx, float dt, float scale)
{
    if (ctx == NULL) {
        return;
    }
    for (int i = 0; i < 3; ++i) {
        if (ctx->views[i] != NULL) {
            view_tick(ctx->views[i], ctx, dt, scale);
        }
    }
}
/* MenuPointerRectangleHitTest (FUN_004112e0): the corpus's widget test,
 * reproduced clause-for-clause — inclusive lower corners, inclusive
 * upper corners via origin+size (finding 23: the widget rectangle is
 * the 100x20 row band this routine hit-tests). */
int menu_pointer_rectangle_hit_test(int x, int y, int rect_x, int rect_y,
                                    int rect_w, int rect_h)
{
    if ((rect_x <= x) && (x <= rect_w + rect_x)) {
        if ((rect_y <= y) && (y <= rect_h + rect_y)) {
            return 1;
        }
    }
    return 0;
}

/* MenuOverlayListCount (FUN_00412170): head slot + record+0x40, next
 * slot + node+0x18, both native-size (pointer-slot deviation; see the
 * header). */
int menu_overlay_list_count(const void* menu)
{
    void* head = NULL;
    if (menu != NULL) {
        __builtin_memcpy(&head, (const char*)menu + 0x40, sizeof(head));
    }
    int count = 0;
    while (head != NULL) {
        void* next = NULL;
        __builtin_memcpy(&next, (char*)head + 0x18, sizeof(next));
        ++count;
        head = next;
    }
    return count;
}

/* TerrainGridBorderInsetGet (FUN_00435f28): DAT_005560ac's getter. */
int view_grid_edge_get(void)
{
    return g_view_grid_edge;
}

/* --- camera-selection cluster (see view.hpp) -------------------------------
 * DAT_005560a0 / DAT_005560a8: zero-initialized until camera_view_context_set
 * configures them (the corpus loads them from the camera config records at
 * startup — that loader seam is unported, so the pan stays inert out of the
 * box, like g_view_grid_edge). */
float g_view_pan_cap   = 0.0f;  /* DAT_005560a0 (CameraAcceleration) */
float g_view_pan_scale = 0.0f;  /* DAT_005560a8 (CameraSpeed) */

void camera_view_context_set(float cap, float follow_scale,
                             float pan_scale, int edge)
{
    g_view_pan_cap = cap;
    g_view_follow_scale = follow_scale;
    g_view_pan_scale = pan_scale;
    g_view_grid_edge = edge;
}

void selection_record_point_set(ViewContext* v, float x, float y)
{
    if (v != NULL && v->mode == 1) {
        /* corpus: the view record's dword [10] camera (cam_follow), z=0 */
        cam_set_pos(&v->cam_follow, x, y, 0.0f);
    }
}

int camera_get_position_with_terrain_height(ViewContext* v,
                                            EngineContext* ctx,
                                            float out[3])
{
    if (v != NULL && ctx != NULL && out != NULL && v->mode == 1) {
        cam_get_pos(&v->cam_follow, out);
        /* corpus: the terrain leaf takes the world handle; the port routes
         * it through the engine context's bound level block (deviation). */
        out[2] = terrain_height(ctx, out[0], out[1]);
    }
    return 1; /* corpus: constant success, untouched output for mode != 1 */
}

void camera_pointer_pan_update(ViewContext* v, float vec[3], float amount)
{
    if (v == NULL || vec == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    vec_normalize(vec);
    v->pan_accum = amount * g_view_pan_scale + v->pan_accum;
    if (g_view_pan_cap < v->pan_accum) {
        v->pan_accum = g_view_pan_cap;
    }
    vec_scale(vec, vec, v->pan_accum * amount);
    vec_add(v->offset, v->offset, vec);
}

/* --- the camera-controller bounds band (see view.hpp) -----------------------
 * Fixed-offset record operations; the corpus's controller record is not
 * modeled as a struct (its owners are the per-view camera objects, and
 * the pose-dispatch half of the keys' logic stays at the unported scene
 * seam). Only the record-field shape these five leaves need is written. */
#define CAM_FLAGS_OFF 0xb8  /* gate-bit byte */

static uint8_t cam_flags(const void* rec, size_t off)
{
    return *(const uint8_t*)((const char*)rec + off);
}

static void cam_or_flags(void* rec, uint8_t mask)
{
    uint8_t f = *(uint8_t*)((char*)rec + CAM_FLAGS_OFF);
    f |= mask;
    *(uint8_t*)((char*)rec + CAM_FLAGS_OFF) = f;
}

void camera_tilt_bounds_set(void* camera, float lo, float hi)
{
    if (camera == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    cam_or_flags(camera, 1u);
    __builtin_memcpy((char*)camera + 0x7c, &lo, sizeof(lo));
    __builtin_memcpy((char*)camera + 0x80, &hi, sizeof(hi));
}

void camera_scalar_bounds_set(void* camera, float lo, float hi)
{
    if (camera == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    cam_or_flags(camera, 2u);
    __builtin_memcpy((char*)camera + 0x88, &lo, sizeof(lo));
    __builtin_memcpy((char*)camera + 0x8c, &hi, sizeof(hi));
}

void camera_distance_bounds_set(void* camera, float lo, float hi)
{
    if (camera == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    cam_or_flags(camera, 4u);
    __builtin_memcpy((char*)camera + 0x94, &lo, sizeof(lo));
    __builtin_memcpy((char*)camera + 0x98, &hi, sizeof(hi));
}

void camera_distance_set(void* camera, float value)
{
    if (camera == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    const uint8_t f = cam_flags(camera, CAM_FLAGS_OFF);
    float lo = 0.0f, hi = 0.0f;
    __builtin_memcpy(&lo, (const char*)camera + 0x94, sizeof(lo));
    __builtin_memcpy(&hi, (const char*)camera + 0x98, sizeof(hi));
    if ((f & 4u) == 0u || (f & 8u) != 0u || (lo <= value && value <= hi)) {
        __builtin_memcpy((char*)camera + 0x90, &value, sizeof(value));
    }
}

void camera_zoom_update(void* camera, float delta)
{
    if (camera == NULL) {
        return; /* deviation: the corpus reads blindly */
    }
    float cur = 0.0f;
    __builtin_memcpy(&cur, (const char*)camera + 0x90, sizeof(cur));
    camera_distance_set(camera, delta + cur);
}

/* CursorScreenRayContextReset (FUN_00435f16) — see view.hpp. */
void cursor_screen_ray_context_reset(ViewContext* v)
{
    if (v == NULL) {
        return; /* deviation: the corpus stores blindly */
    }
    v->pan_accum = 0.0f; /* the corpus's view-record +0x9c slot */
}

/* CurrentLevelTerrainGridPick (FUN_00433be0) — see view.hpp. */
void current_level_terrain_grid_pick(EngineContext* ctx, float x, float y,
                                     int* gx, int* gy)
{
    const float pos[2] = { x, y };
    int tx = 0, ty = 0;
    int* px = gx != NULL ? gx : &tx;
    int* py = gy != NULL ? gy : &ty;
    (void)grid_pick(ctx, pos, px, py);
}

/* GridOperationArgumentAdapter (FUN_00442540) — see view.hpp. */
void grid_operation_argument_adapter(float x, float y, EngineContext* ctx)
{
    if (ctx == NULL) {
        return; /* deviation: the corpus reads its own world blindly */
    }
    (void)terrain_height(ctx, x, y);
}

/* SetGlobalScalarToOne (FUN_0040fec0) — see view.hpp. */
void set_global_scalar_to_one(void)
{
    /* the corpus's ramp draw is discarded; faithful no-op */
}

/* FloatNegateAbsoluteValue (FUN_0044d250) — see view.hpp. */
void float_negate_absolute_value(float* value)
{
    if (value == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    *value = -*value; /* both corpus branches negate */
}

/* Vector2SetLength (FUN_00406760) — see view.hpp. */
void vector2_set_length(float v[2], float length)
{
    if (v == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    if (length == 0.0f) {
        return; /* deviation: the corpus divides blindly */
    }
    const float s = sqrtf(v[0] * v[0] + v[1] * v[1]) / length;
    v[0] = v[0] / s;
    v[1] = v[1] / s;
}

/* CameraZoomStepAdjust (FUN_0045bf20) — see view.hpp. */
void camera_zoom_step_adjust(void* camera, int direction, float step)
{
    if (direction == 0) {
        camera_zoom_update(camera, step * -3.0f);
    } else if (direction == 1) {
        camera_zoom_update(camera, step * 3.0f);
    }
    /* other directions: the corpus's early exit — inert */
}

/* SelectionRecordPointGet (FUN_00436ae9) — see view.hpp. */
void selection_record_point_get(ViewContext* v, float out[3])
{
    if (v != NULL && out != NULL && v->mode == 1) {
        /* corpus: the view record's dword [10] camera (cam_follow) */
        cam_get_pos(&v->cam_follow, out);
    }
}

/* SelectedGlobalPointTransformAndReturnX (FUN_0046a420) — see view.hpp. */
float selected_global_point_transform_and_return_x(EngineContext* ctx)
{
    if (ctx == NULL) {
        return 0.0f; /* deviation: the corpus strides the globals blindly */
    }
    ViewContext* v = (ctx->context_mode == 1) ? ctx->views[0] : ctx->views[2];
    float frame[3] = { 0.0f, 0.0f, 0.0f };
    (void)camera_get_position_with_terrain_height(v, ctx, frame);
    return frame[0];
}

/* RendererNodeRelativePositionSet (FUN_00436ac9) — see view.hpp. */
void renderer_node_relative_position_set(const void* node, float x, float y)
{
    void* handle = NULL;
    if (node != NULL) {
        __builtin_memcpy(&handle, (const char*)node + 0x28, sizeof handle);
    }
    cam_offset((CameraHandle*)handle, x, y, 0.0f);
}

/* ---- scene-root hook band (2026-10-24) ----------------------------------- *
 * SceneRootObjectGet / DDiSceneObjectArgumentsNormalize — see view.hpp. The
 * corpus's root global (DAT_0076add4) is written only by the unported
 * renderer-side initializer, so the port carries it as a hooked static. */
static void* s_scene_root;

void scene_root_install(void* root) { s_scene_root = root; }
void* scene_root_peek(void) { return s_scene_root; }

/* SceneRootObjectGet (FUN_00472940) — the corpus's root-object read. */
void* scene_root_object_get(void)
{
    return s_scene_root;
}

/* SceneNodePositionSet (FUN_00410020) — see view.hpp. The inner object
 * rides the node's widened slot at +0; the corpus strides a dword. */
void scene_node_position_set(void* node, const float pos[3])
{
    if (node == NULL || pos == NULL) {
        return;
    }
    void* inner = NULL;
    const uint32_t valid = 1u;
    __builtin_memcpy((char*)node + 0x0c, &valid, sizeof valid);
    __builtin_memcpy(&inner, (const char*)node, sizeof inner);
    if (inner == NULL) {
        return; /* deviation: the corpus strides the handle blindly */
    }
    __builtin_memcpy((char*)inner + 0x0c, &pos[0], sizeof pos[0]);
    __builtin_memcpy((char*)inner + 0x10, &pos[1], sizeof pos[1]);
    __builtin_memcpy((char*)inner + 0x14, &pos[2], sizeof pos[2]);
}

/* DDiSceneObjectArgumentsNormalize (FUN_00475900) — resolve the dispatch
 * pair: the object slot rides through, and the second slot falls back to
 * the root object's first slot when the caller passes NULL. */
void scene_object_arguments_normalize(const void** obj, const void** alt,
                                      void** out_obj, const void** out_alt)
{
    if (obj != NULL && out_obj != NULL) {
        *out_obj = (void*)*obj; /* corpus: blind dereference */
    }
    if (out_alt == NULL) {
        return;
    }
    if (alt != NULL) {
        *out_alt = *alt; /* corpus: blind dereference */
        return;
    }
    /* Deviation: a missing root answers NULL; the corpus strides it. */
    const void* root_slot = NULL;
    if (s_scene_root != NULL) {
        __builtin_memcpy((void*)&root_slot, s_scene_root, sizeof root_slot);
    }
    *out_alt = root_slot;
}

/* Matrix4IdentityInitialize (FUN_00483540): the corpus writes the twelve
 * off-diagonal dwords as zero (high to low), then the four diagonal dwords
 * as the float-1.0 constant — the same output, in the corpus's order. */
void matrix4_identity_initialize(float* out)
{
    for (int i = 1; i < 16; ++i) {
        if (i % 5 != 0) {
            out[i] = 0.0f;
        }
    }
    for (int i = 0; i < 4; ++i) {
        out[5 * i] = 1.0f;
    }
}

/* SurfaceTriangleNormalCalculate (FUN_00479530): the two edges
 * e1 = p4 − p3 and e2 = p3 − p2; the three products recombine as the
 * cross product cross(e2, e1), so the normal is that cross normalized.
 * The corpus divides 1.0 by SQRT with no zero-length guard (finding 388
 * pinned); the port keeps the fold exactly. */
void surface_triangle_normal_calculate(float* out, const float* p2,
                                       const float* p3, const float* p4)
{
    const float e1[3] = {p4[0] - p3[0], p4[1] - p3[1], p4[2] - p3[2]};
    const float e2[3] = {p3[0] - p2[0], p3[1] - p2[1], p3[2] - p2[2]};
    const float cx = e2[1] * e1[2] - e2[2] * e1[1];
    const float cy = e2[2] * e1[0] - e2[0] * e1[2];
    const float cz = e2[0] * e1[1] - e2[1] * e1[0];
    const float scale = 1.0f / (float)sqrt(cz * cz + cy * cy + cx * cx);
    out[0] = scale * cx;
    out[1] = scale * cy;
    out[2] = scale * cz;
}

/* SceneMeshColorAndAlphaSet (FUN_00475020) — see view.hpp. */
void scene_mesh_color_and_alpha_set(void* object, int block, float red,
                                    float green, float blue, float alpha)
{
    void* mesh = NULL;
    void* record = NULL;
    if (object != NULL) {
        __builtin_memcpy(&mesh, (const char*)object + 0xc, sizeof mesh);
    }
    if (mesh != NULL) {
        __builtin_memcpy(&record, (const char*)mesh + 0xc, sizeof record);
    }
    if (record == NULL) {
        return; /* deviation: the corpus strides both links blindly */
    }
    render_object_color_vector_set(record, block, red, green, blue, 0);
    screen_effect_item_opacity_set(record, block, alpha, 4);
}

/* --- the rotated-pairs scratch (see view.hpp) ----------------------------- */

#define ROT_PAIR_SLOTS 256 /* deviation: bounded scratch (blind DAT stride) */
static int s_rot_pairs[ROT_PAIR_SLOTS * 2];

void* rotated_pairs_scratch_peek(void) { return s_rot_pairs; }

void* rotated_pairs_scratch_fill(const int* offset_pair, const int* source,
                                 int count, int rotation)
{
    if (count > ROT_PAIR_SLOTS) {
        count = ROT_PAIR_SLOTS; /* deviation: the corpus fills blindly */
    }
    for (int n = 0; n < count; ++n) {
        const int t = rotation + 0x8000; /* the corpus's sign fold */
        const int u = t >> 31;
        const int sel = (((t ^ u) - u & 3) ^ u) - u;
        int x = source[2 * n];
        int y = source[2 * n + 1];
        switch (sel) {
        case 1: { const int h = x; x = -y; y = -h; } break;
        case 2:  x = -x; y = -y; break;
        case 3: { const int h = x; x = y; y = -h; } break;
        default: break; /* identity (negative folds land here) */
        }
        s_rot_pairs[2 * n]     = x + offset_pair[0];
        s_rot_pairs[2 * n + 1] = y + offset_pair[1];
    }
    return s_rot_pairs;
}
