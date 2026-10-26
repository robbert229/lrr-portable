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

/* --- matrix/ray math leaves (2026-10-24) ----------------------------------
 * Pure single-precision arithmetic over row-major float[16] storage; no
 * corpus state. The corpus rotation setters evaluate x87 `fcos`/`fsin` on
 * the angle promoted to extended precision; the port evaluates cos/sin in
 * double and rounds each result into the float cell — the documented x87
 * deviation (cf. x87_round_to_integral), which the float store hides at
 * nearly every angle. */

/* out = origin + t · dir, corpus component order (FUN_00479ab0). */
void point_along_ray(float* out, const float* origin, const float* dir,
                     float t);

/* ViewRayIntersectionTest (FUN_004798e0): normalizes both directions,
 * intersects ray (p, normalized dir_a) with the plane through q with the
 * normalized dir_b as normal (through ray_plane_intersection_parameter),
 * and writes out = p + t · normalized dir_a, answering 1; parallel rays
 * answer 0 with `out` untouched. */
int view_ray_intersection_test(float* out, const float* p,
                               const float* dir_a, const float* q,
                               const float* dir_b);

/* Matrix4SetIdentity (FUN_00479e70): 1 on the diagonal, 0 elsewhere. */
void matrix4_set_identity(float* out);

/* MatrixIdentityAndTranslationSet (FUN_00479e40): the identity with the
 * translation triad written into row-major row 3. */
void matrix4_identity_translation_set(float* out, const float* translation);

/* Matrix4Copy (FUN_00479eb0): 16 dword copies. */
void matrix4_copy(float* out, const float* src);

/* Rotation setters: each writes the identity, then overwrites the
 * two-row/two-column rotation block with (cos, −sin/sin, cos) at the
 * corpus's slots:
 *   - matrix4_set_rotation_x: m[5]=c, m[6]=−s, m[9]=s, m[10]=c (rows 1–2);
 *   - matrix4_set_rotation_y: m[0]=c, m[2]=s, m[8]=−s, m[10]=c (rows 0/2);
 *   - matrix4_set_rotation_z: m[0]=c, m[1]=−s, m[4]=s, m[5]=c (rows 0–1).
 * The corpus reference names call FUN_00479d50 "Matrix4SetRotationZ" as
 * well as FUN_00479df0; the row/column blocks distinguish the three and
 * are what the port names. */
void matrix4_set_rotation_x(float* out, float angle); /* FUN_00479d50 */
void matrix4_set_rotation_y(float* out, float angle); /* FUN_00479da0 */
void matrix4_set_rotation_z(float* out, float angle); /* FUN_00479df0 */

/* BuildPerspectiveProjectionMatrix (FUN_0040bd60): zeroes the matrix,
 * sets m[10] = m[15] = 1, then the three quotients m[0] = p4/p1,
 * m[5] = p3/p2, m[1] = (p5/p4) · (p4/p1) — corpus expression as written. */
void perspective_projection_matrix_set(float* out, float p1, float p2,
                                       float p3, float p4, float p5);

/* RandomReachableGridPointSelect (FUN_00450560): iterate the ray
 * (p, dir) down through the z-plane — starting at the plane point
 * (0, 0, -50) with normal (0, 0, -1) — `rounds` times, letting the
 * terrain height of the hit (the FUN_0044f940 hook; the port reads the
 * voxel surface byte) ride back into the plane point's z, so every
 * iteration from the second descends the plane to the previous height.
 * The final hit's grid cell then lands in gx/gy through the FUN_0044f6d0
 * hook. The corpus loop tests != 0, so a negative `rounds` runs until
 * it wraps; with rounds == 0 the corpus grid-picks from uninitialized
 * stack, so the port reads whatever `out` the caller left. */
void random_reachable_grid_point_select(EngineContext* ctx, const float* p,
                                        const float* dir, float* out,
                                        int* gx, int* gy, int rounds);

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

/* SelectedGlobalPointTransformAndReturnX (FUN_0046a420, finding 426): picks
 * the follow view in-game (DAT_005560e4) or the editor view otherwise
 * (DAT_005560ec — the engine context's mode word and view windows), reads
 * the camera position with the terrain-height replacement through the
 * ported pair, and returns the frame array's first word (x). The corpus's
 * level-terrain-handle read rides in the call as the ported callee's
 * world-handle argument (folded into the bound level block per deviation). */
float selected_global_point_transform_and_return_x(EngineContext* ctx);
                                                /* FUN_0046a420 */

/* RendererNodeRelativePositionSet (FUN_00436ac9, findings 435/372): the
 * renderer node's +0x28 camera handle rides the ported mode-1 side of the
 * camera teleport/offset dispatch with x/y and a zero z. Deviation: the
 * corpus strides the handle as a dword; the port reads it as the module's
 * widened pointer slot. */
void renderer_node_relative_position_set(const void* node, float x, float y);
                                                /* FUN_00436ac9 */

/* --- scene-root hook band (2026-10-24) -------------------------------------
 * The corpus's scene object argument normalization (the front-end of the
 * ported camera-handle primitives) answers the root slot from its global
 * root object. The corpus's only writer is the renderer-side scene
 * initializer (unported), so the root rides a module hook like the other
 * DAT one-liners; install/peek drive the fallback under test. */
void scene_root_install(void* root);            /* port hook */
void* scene_root_peek(void);                    /* port hook */

/* SceneRootObjectGet (FUN_00472940): the corpus's root-object read. */
void* scene_root_object_get(void);              /* FUN_00472940 */

/* SceneNodePositionSet (FUN_00410020): the scene node's position store —
 * the node's position-valid flag at +0xc goes to 1, and the three floats
 * land on the inner object's +0xc/+0x10/+0x14. Deviation: the corpus
 * strides the node's dword handle slot blindly; the port reads the inner
 * through the module's widened pointer slot (stride native) and guards a
 * missing handle. */
void scene_node_position_set(void* node, const float pos[3]);
                                                /* FUN_00410020 */

/* DDiSceneObjectArgumentsNormalize (FUN_00475900): resolves the dispatch
 * pair — the first slot rides out of the first argument's slot, and the
 * second resolves to its own slot when the argument is non-NULL, else to
 * the root object's first slot (the corpus's null-handle fallback).
 * Deviation: the corpus dereferences the first slot and the root blindly;
 * the port answers NULL on the missing-root path. */
void scene_object_arguments_normalize(const void** obj, const void** alt,
                                      void** out_obj, const void** out_alt);
                                                /* FUN_00475900 */

/* Matrix4IdentityInitialize (FUN_00483540): the straight-line shape — the
 * twelve off-diagonal dwords clear, then the four diagonal entries land as
 * the float-1.0 dwords. (The corpus's row-looping sibling stays cited as
 * its own row.) */
void matrix4_identity_initialize(float* out);   /* FUN_00483540 */

/* SurfaceTriangleNormalCalculate (FUN_00479530): the triangle normal as
 * the unit cross product of the edges (p3 − p2) and (p4 − p3). Deviation
 * kept faithful: there is no zero-length guard before the reciprocal, so
 * degenerate input yields the corpus's inf/NaN. */
void surface_triangle_normal_calculate(float* out, const float* p2,
                                       const float* p3, const float* p4);
                                                /* FUN_00479530 */

/* SceneMeshColorAndAlphaSet (FUN_00475020): the scene-side color/alpha
 * pair over an object's mesh record — the object's +0xc inner pointer
 * chains to the indexed record the pair's window model reads (widened
 * pointer slots per convention), then the RGB pair lands through the
 * slot-0 color set and the alpha through the mode-4 opacity composition.
 * Deviations: the corpus strides both link slots blindly — the port
 * guards them, and a missing record leaves both windows untouched. */
void scene_mesh_color_and_alpha_set(void* object, int block, float red,
                                    float green, float blue, float alpha);

/* SceneMeshColorSet (FUN_00474ff0): the color triple without the alpha,
 * mode three of the ported color-vector setter (the block's +0x34 slot).
 * Deviation: the corpus strides both record links blindly. */
void scene_mesh_color_set(void* object, int block, float red, float green,
                          float blue);
                                                /* FUN_00475020 */

/* RotatedPairsScratchFill (FUN_00464110): copies count pair dwords from
 * the source into the module scratch, applies the corpus's signed
 * rotation class (the (param_4 + 0x8000) sign-fold mod-4 selector: 1
 * swaps both negated, 2 negates both, 3 lands (y, -x), else identity —
 * negative folds default), adds the offset pair, and answers the
 * scratch pointer. Deviations: bounded scratch (the corpus writes its
 * DAT region blindly) and count guarded. */
void* rotated_pairs_scratch_fill(const int* offset_pair, const int* source,
                                 int count, int rotation);          /* FUN_00464110 */

/* Observer hook answering the scratch base (test pin). */
void* rotated_pairs_scratch_peek(void);

/* --- curve band (2026-10-25) -------------------------------------------------
 * CubicBezierPointEvaluate (FUN_004065f0): the cubic at t over four two-float
 * control points, written as the corpus's Horner-style expansion (its x half
 * folds A*t first, the y half the t-squared term first — commutative add,
 * bit-identical). Deviation: the corpus strides the output blindly.
 * BezierCurveSamplesBuild (FUN_00406820): builds the sample table — header
 * {count, control pair}, then `count` samples spaced on 1/(count-1) with the
 * 0x31-overflow clamp. PolylineSegmentLengthsAndTotalCalculate (FUN_004067b0):
 * walks the packed point list (float pairs every 8 bytes from +4), stores each
 * segment's length through the +0x198 table, zeroes the +0x194 word, and
 * answers the running total. The corpus strides the record and length window
 * blindly; the port guards them. */
void cubic_bezier_point_evaluate(float out[2], const float* p0, const float* p1,
                                 const float* p2, const float* p3, float t);
void bezier_curve_samples_build(uint32_t* dest, const uint32_t* control,
                                const uint32_t* p3, const uint32_t* p4,
                                const uint32_t* p5, uint32_t count);
float polyline_segment_lengths_total(uint32_t* curve);

/* PathArcLengthInterpolate (FUN_004068c0): walks the arc-length table of a
 * curve record (dword count at +0, point i floats at dwords 2i-1/2i,
 * segment lengths at dword 0x65+segment) and interpolates the point at the
 * travelled distance, answering the segment index. On or past the last
 * segment the endpoint rides through uninterpolated. Deviations: NULL
 * guards on record and output; the length reads reinterpret the stored
 * float bits (the builder's window). */
uint32_t path_arc_length_interpolate(const uint32_t* curve, float travel,
                                     float* out);              /* FUN_004068c0 */

/* ViewPointToScreenGeometryTransform (FUN_0045e940): projects four floats
 * in place through the projection window — screen offsets at +1c/+20 come
 * off first, the scale ratio (+14 / +18) scales x, y, and depth, x lands
 * through the half-size + center pair, y mirrors through its own, and w
 * rides the ratio. Deviations: NULL guards; the ratio is computed once. */
void view_point_to_screen_geometry_transform(const void* view,
                                             float p[4]);     /* FUN_0045e940 */
#endif /* LEGORRR_VIEW_HPP */