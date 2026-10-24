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
    float  pan_accum;  /* +0x9c pointer-pan accumulator (CameraPointerPanUpdate) */
    float  _gap_a5;    /* corpus dword gap after the accumulator slot */
    float  _gap_a9;    /* ditto */
    float  _gap_ad;    /* ditto */
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
/* DAT_005560a0 / DAT_005560a8 — the pointer-pan cap (CameraAcceleration)
 * and per-move scale (CameraSpeed). No ported code feeds them before the
 * view-config setter below runs, so they start zero (the pan is inert
 * until configured — documented deviation). */
extern float g_view_pan_cap;
extern float g_view_pan_scale;

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

/* --- offset-point rect family (corpus FUN_0045b590 / FUN_00463c00 /
 * FUN_00463c90; findings 241/363/446) --------------------------------------
 * All three are the same half-open containment test over a bounds-quad
 * rect {x_min, y_min, x_max, y_max}: the two float offsets truncate to int
 * (the corpus's two __ftol calls) and subtract from the integer point
 * before the compare — `x_min <= float(x - ox) < x_max` and the same for
 * y. The port's cast matches the documented __ftol ≡ C++ truncation. */

/* Direct bounds quad (FUN_0045b590 / FUN_00463c00; the corpus's two call
 * shapes carry identical arithmetic). */
int rect_contains_offset_point(const float rect_bounds[4], float off_x,
                               float off_y, int x, int y);
int rect_contains_point_by_converted_coords(const float rect_bounds[4],
                                            float off_x, float off_y,
                                            int x, int y);

/* Bounds read through the pointer stored in the corpus record's +0x14 slot
 * (FUN_00463c90): the port passes that slot resolved as a pointer to the
 * rect pointer. NULL slot or NULL rect answers 0 — the corpus dereferences
 * unguarded (documented deviation). */
int rect_contains_point_from_object_bounds(const float* const* bounds_at_14,
                                           float off_x, float off_y,
                                           int x, int y);

/* MenuPointerRectangleHitTest (FUN_004112e0, finding 23/240): the
 * integer widget-rectangle test — inside when left <= x <= left+width
 * and top <= y <= top+height, all four edges closed. The corpus's menu
 * machinery (unported) feeds it the stored pointer position. */
int menu_pointer_rectangle_hit_test(int x, int y, int rect_x, int rect_y,
                                    int rect_w, int rect_h);

/* MenuOverlayListCount (FUN_00412170, findings 23/240): count the
 * overlay-list chain — the head pointer slot at record +0x40, nodes
 * linked through the slot at node +0x18. Deviation: the corpus's
 * 32-bit pointer slots are read native-size (the port's model of
 * corpus pointer fields); NULL head answers 0. */
int menu_overlay_list_count(const void* menu);
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

/* --- 4x4 matrix primitives + polygon parity test (corpus FUN_00479ea0 /
 * FUN_00479ce0 / FUN_00479b80; findings 309/388) ---------------------------
 * Row-major float storage; corpus x87 math is byte-equivalent float here. */

/* Zero the matrix (FUN_00479ea0: 16 stores). */
void matrix4_set_zero(float* out);

/* Multiply: corpus takes (out, right, left) and accumulates each output
 * cell as a four-term dot product, i.e. out = left × right, row-major
 * (FUN_00479ce0). */
void matrix4_multiply(float* out, const float* right, const float* left);

/* Even-odd horizontal-ray crossing parity (FUN_00479b80). Edge k runs
 * verts[k] → next[k]; the corpus reads the edge end through a pointer pair
 * that strides both arrays in lockstep, so any caller whose `next` is the
 * vertex array shifted one vertex (the typical ring form) behaves 1:1.
 * No zero-span/boundary handling beyond the corpus's, including the
 * potentially zero-span divide. */
int point_in_polygon_2d(const float point[2], const float* verts,
                        const float* next, int count);

/* --- camera-handle primitives (record-backed) ----------------------------- */
void cam_offset(CameraHandle* c, float x, float y, float z); /* FUN_004755e0 */
void cam_get_pos(CameraHandle* c, float out[3]);            /* FUN_004754c0 */
void cam_set_pos(CameraHandle* c, float x, float y, float z);/* FUN_00475430 */
void cam_get_facing(CameraHandle* c, float out[3]);         /* FUN_00475500 */
void cam_set_facing(CameraHandle* c, const float* r);       /* FUN_00475470 */

/* TerrainGridBorderInsetGet (FUN_00435f28): the tile-count edge clamp —
 * g_view_grid_edge is DAT_005560ac with its ported writer, so this is
 * the faithful getter. */
int view_grid_edge_get(void);

/* --- camera-selection cluster (2026-10-24) ---------------------------------
 * CameraViewContextSet / SelectionRecordPointSet /
 * CameraGetPositionWithTerrainHeight / CameraPointerPanUpdate
 * (FUN_00435e0e / FUN_00436b0b / FUN_00436b33 / FUN_00436b87, findings
 * 102/339/378/424/426): the follow-view camera leaves. The setter stores
 * the four view-config globals (cap, follow scale, pan scale, edge). The
 * point set writes the follow camera's x/y for follow-mode views; the
 * position read copies the follow camera and replaces its z with the
 * terrain height (the corpus's world-handle argument maps to the engine
 * context's bound level block — documented deviation). The pan update
 * normalizes the move vector, advances the per-view accumulator by
 * amount * pan scale, caps it, scales the vector by accumulator * amount,
 * and accumulates it into the follow offsets (the corpus's +0x18 field).
 * Zero-length vectors normalize as-is (the corpus's unguarded leaf). */
void camera_view_context_set(float cap, float follow_scale,
                             float pan_scale, int edge);
void selection_record_point_set(ViewContext* v, float x, float y);
int  camera_get_position_with_terrain_height(ViewContext* v,
                                             EngineContext* ctx,
                                             float out[3]);
void camera_pointer_pan_update(ViewContext* v, float vec[3], float amount);

/* --- the camera-controller bounds band (2026-10-24; finding 280) ------------
 * The corpus's camera-key controller record (offsets into the record its
 * per-view callers pass): bounds stores write the lo/hi floats and raise
 * a gate bit in the flag byte at +0xb8; the distance setter is gated on
 * the bit-4 gate unless bit 8 overrides; zoom adds to the stored value.
 * The record itself is not modeled in the port (its owners are the
 * per-view camera objects) — these take a raw record pointer and write
 * fixed offsets, the port's record-module style, entry-guarded where the
 * corpus dereferences blindly (documented deviations). */
void camera_tilt_bounds_set(void* camera, float lo, float hi);
                                        /* FUN_00436831 */
void camera_scalar_bounds_set(void* camera, float lo, float hi);
                                        /* FUN_00436921 */
void camera_distance_bounds_set(void* camera, float lo, float hi);
                                        /* FUN_00436a18 */
void camera_distance_set(void* camera, float value); /* FUN_00436a4a */
void camera_zoom_update(void* camera, float delta);  /* FUN_00436aa5 */

/* CursorScreenRayContextReset (FUN_00435f16): the corpus's cursor ray
 * handler drops the pan accumulator when the cursor sits inside the
 * screen-edge dead zone — the reset half, ported against the modeled
 * ViewContext slot. NULL skips (deviation: blind store). */
void cursor_screen_ray_context_reset(ViewContext* v);

/* CurrentLevelTerrainGridPick (FUN_00433be0): the corpus's per-level pick
 * wrapper — the world pointer from the state root's +4 field, x/y floats
 * in, grid indices out. The port's grid_pick models the pick (with the
 * stub-boundary note above); this is the wrapper shape, and NULL index
 * outputs skip their stores the way the corpus's bounds check does. */
void current_level_terrain_grid_pick(EngineContext* ctx, float x, float y,
                                     int* gx, int* gy);

/* GridOperationArgumentAdapter (FUN_00442540): the corpus's grid-op
 * argument rotator — the terrain-height call takes (level, x, y) while
 * grid code hands (x, y, level); the float result is discarded (void).
 * NULL contexts skip (deviation). */
void grid_operation_argument_adapter(float x, float y, EngineContext* ctx);

/* SetGlobalScalarToOne (FUN_0040fec0): the corpus draws the 0…1 ramp and
 * drops it; the port models that ramp as 1.0, so the call's observable
 * effect is nothing — faithful. */
void set_global_scalar_to_one(void);

/* --- the view math band (2026-10-24; findings 280/256) -----------------------
 * FloatNegateAbsoluteValue (FUN_0044d250): both corpus branches flip the
 * sign — the port keeps the single in-place negate.
 * Vector2SetLength (FUN_00406760): scale the vector to `length`, the
 * corpus's sqrt-over-length division; a zero length skips (deviation).
 * CameraZoomStepAdjust (FUN_0045bf20): direction 0 zooms by -3x the
 * step, 1 by +3x, anything else is inert; the corpus's view-record
 * global is threaded (deviation). */
void float_negate_absolute_value(float* value);
void vector2_set_length(float v[2], float length);
void camera_zoom_step_adjust(void* camera, int direction, float step);

/* SelectionRecordPointGet (FUN_00436ae9): the follow-mode readback — the
 * selection record's camera position through the corpus's move-or-pose read
 * (cam_get_pos). Non-follow records answer nothing (the corpus's gate). */
void selection_record_point_get(ViewContext* v, float out[3]); /* FUN_00436ae9 */

#endif /* LEGORRR_VIEW_HPP */