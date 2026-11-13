// SPDX-License-Identifier: LGPL-3.0-or-later
/* Viewports.c (original translation unit G98-003, finding 593).
 *
 * C-compatible header (ADR 0005). viewports.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include this
 * file: every include and declaration a header adds shifts MSVC's symbol
 * numbering (finding 506). src/game/test_c_boundaries.cpp compiles it beside
 * the C++ declarations of the port's names.
 *
 * In the matching build viewports_link_names.h, which only viewports.c
 * includes, renames most of these to the port's names (see there). */
#ifndef LEGORRR_GODS98_VIEWPORTS_H
#define LEGORRR_GODS98_VIEWPORTS_H

#ifdef __cplusplus
extern "C" {
#endif

struct Viewport;
struct Container;
struct VpFrame;
struct Vector3F;
struct Vector4F;
struct Point2F;

void viewport_initialise(void);                              /* 00476d50 */
void viewport_shutdown(void);                                /* 00476d80 */
struct Viewport* viewport_create(float xPos, float yPos, float width,
                                 float height,
                                 struct Container* camera);  /* 00476dc0 */
struct Viewport* viewport_create_pixel(long xPos, long yPos,
                                       unsigned long width,
                                       unsigned long height,
                                       struct Container* camera); /* 00476e50 */
void viewport_get_size(struct Viewport* vp, unsigned long* width,
                       unsigned long* height);               /* 00476f10 */
void viewport_set_camera(struct Viewport* vp,
                         struct Container* cont);            /* 00476f50 */
struct Container* viewport_get_camera(struct Viewport* vp);  /* 00476f70 */
void viewport_set_back_clip(struct Viewport* vp, float dist); /* 00476fb0 */
float viewport_get_back_clip(struct Viewport* vp);           /* 00476fd0 */
float viewport_get_front_clip(struct Viewport* vp);          /* 00476ff0 */
void viewport_clear(struct Viewport* vp, int full);          /* 00477010 */
void viewport_render(struct Viewport* vp, struct Container* root,
                     float delta);                           /* 00477150 */
void viewport_remove(struct Viewport* dead);                 /* 00477220 */
void viewport_smooth_set_field(struct Viewport* vp, float fov); /* 00477240 */
void viewport_set_field(struct Viewport* vp, float fov);     /* 00477250 */
float viewport_get_field(struct Viewport* vp);               /* 00477270 */
void viewport_inverse_transform(struct Viewport* vp, struct Vector4F* dest,
                                const struct Vector3F* src); /* 00477290 */
void viewport_transform(struct Viewport* vp, struct Vector4F* dest,
                        const struct Vector3F* src);         /* 004772b0 */
void viewport_world_to_screen(struct Viewport* vp, struct Point2F* screen,
                              const struct Vector3F* world); /* 004772d0 */
struct VpFrame* viewport_get_scene(struct Viewport* vp);     /* 00477310 */
void viewport_add_list(void);                                /* 00477370 */
void viewport_remove_all(void);                              /* 004773e0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_VIEWPORTS_H */
