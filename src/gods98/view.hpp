// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — view context tick (corpus ViewContextTick @ 0x435f5c,
 * finding 04 §ViewContextTick). Ticked three times per frame (the three view
 * objects DAT_005560e4 / e8 / ec; finding 03).
 *
 * Modes: 1 = follow (edge-locked camera), 2 = scripted/lagged
 * (k = |d|/(|d|+A) interpolation), 3 = free/cruise (velocity-EMA, dirty flag
 * bit 0x10). Requested by the sim-fills tranche 2.
 *
 * The view's *camera handles* (corpus FUN_004754c0/00475500/00475430/
 * 00475470/004755e0) are modeled as position/facing records (the native port
 * has no 3D-world projection yet); the corpus functions are implemented
 * against the record. World-query boundaries (grid pick FUN_0044f6d0,
 * terrain height FUN_0044f940, target blend FUN_0044a070) stay stubs with
 * corpus addresses until level/world data lands.
 */
#ifndef LEGORRR_VIEW_HPP
#define LEGORRR_VIEW_HPP

#include "engine.hpp"

/* A camera handle: the corpus's camera objects hold a position + facing. */
typedef struct CameraHandle {
    int   inuse;
    float pos[3];
    float rot[3];
} CameraHandle;

/* View object — the corpus's flat view array read as named fields. */
typedef struct ViewContext {
    int    mode;        /* [0]  1 follow / 2 scripted-lag / 3 free */
    int    follow_id;   /* [1]  followed/velocity state target */
    int    a2;          /* [2] */
    int    a3;          /* [3] */
    int    a4;          /* [4] */
    int    a5;          /* [5] */
    float  offset[3];   /* [6..8] follow offsets */
    CameraHandle cam_world;  /* [9]   main camera */
    CameraHandle cam_follow; /* [10]  follow camera */
    CameraHandle cam_facing; /* [0xb] facing camera */
    int    cam_mode;   /* [0xc] */
    float  follow_span;/* [0x24] */
    float  step;       /* [0x28] */
    int    timer_num;  /* [0x29] */
    int    timer_acc;  /* [0x2a] */
    float  velocity[3];/* [0x2b..2d] */
    uint8_t flags;     /* [0x2e] bit 0x10 = dirty */
} ViewContext;

/* Per-frame view update: ViewContextTick(view, state, dt, scale). */
void view_tick(ViewContext* v, EngineContext* ctx, float dt, float scale);

/* The corpus ticks the three view objects per frame (finding 03). */
void view_views_tick(EngineContext* ctx, float dt, float scale);

/* Reconstruction-owned globals (DAT_005560a4 grid follow scale,
 * DAT_005560ac tile-count edge clamp). */
extern float g_view_follow_scale;
extern int   g_view_grid_edge;

/* Test hook: when nonzero, the mode-3 velocity-EMA target is injected. */
extern int g_view_test_blend;

/* --- pure vector helpers (corpus FUN_00401720 / FUN_00401500 /
 * FUN_004016c0 / FUN_00401470 / FUN_00401750 / FUN_00401420 / FUN_004016f0 /
 * FUN_004013a0 / FUN_00401240 / FUN_00406730 / FUN_0047c0c0; finding 280) --
 * 3D unless noted; x87 float10 corpus math is byte-equivalent float here. */
void vec_scale(float* dst, const float* src, float s);
void vec_add(float* dst, const float* a, const float* b);
void vec_sub(float* dst, const float* a, const float* b);
void vec_cross(float* dst, const float* b, const float* c);
void vec_normalize(float* v); /* zero length unguarded, as corpus */
float vec_dist(const float* a, const float* b);
float vec_dot(const float* a, const float* b);
float vec_len(const float* a);
float vec2_dist(const float* a, const float* b);   /* 2D */
float vec2_dist_alt(const float* a, const float* b); /* 2D corpus variant */
int   vec_exact_equal(const float* a, const float* b);

/* --- geometry helpers (corpus FUN_0045b8a0 / FUN_0045e660 / FUN_00479a30 /
 * FUN_00479af0 / FUN_00479c10; findings 363/323) --------------------------- */
/* True when distance (x1,y1)-(x2,y2) < radius (FUN_0045b8a0). */
int point_distance_within_radius(float x1, float y1, float x2, float y2,
                                 float radius);
/* Half-open rect test on the record's {x,y,w,h} float quad (FUN_0045e660). */
int rect_contains_point_half_open(const float rect_xywh[4], int x, int y);
/* t = dot(q-p, n) / dot(n, d) into t_out for ray (p,d) × plane (q,n);
 * answers 1 when not parallel, 0 otherwise (FUN_00479a30). */
int ray_plane_intersection_parameter(float* t_out, const float* p,
                                     const float* d, const float* q,
                                     const float* n);
/* Line1 (p1,d1) × line2 (p2,d2): writes the hit into out, returns it;
 * NULL when parallel (FUN_00479af0). */
float* line_intersection_2d_compute(float* out, const float* p1,
                                    const float* d1, const float* p2,
                                    const float* d2);
/* Positive-discriminant ray-sphere test, corpus expression verbatim;
 * `radius2` is the corpus's squared-radius parameter (FUN_00479c10). */
int sphere_ray_intersection_discriminant_positive(const float* center,
                                                  float radius2,
                                                  const float* origin,
                                                  const float* dir);

/* --- camera-handle primitives (record-backed) ----------------------------- */
void cam_offset(CameraHandle* c, float x, float y, float z); /* FUN_004755e0 */
void cam_get_pos(CameraHandle* c, float out[3]);            /* FUN_004754c0 */
void cam_set_pos(CameraHandle* c, float x, float y, float z);/* FUN_00475430 */
void cam_get_facing(CameraHandle* c, float out[3]);         /* FUN_00475500 */
void cam_set_facing(CameraHandle* c, const float* r);       /* FUN_00475470 */

#endif /* LEGORRR_VIEW_HPP */