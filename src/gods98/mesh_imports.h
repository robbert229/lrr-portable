// SPDX-License-Identifier: LGPL-3.0-or-later
/* Mesh.c (G98-015) imports: C declarations of the callees and globals the
 * unit uses from other modules (ADR 0005 step 6; finding 625). Included only
 * by mesh_unit.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names a definition with C linkage, by the name it has in
 * the matching build; the macros give the unit's code the gods98 names.
 *
 * - Main.c (G98-005): Main_ChangeRenderState (004788d0) and
 *   Main_RestoreStates (00478940) are surface.cpp's, given C linkage in
 *   surface.hpp. mainGlobs.lpD3DRM (00505914) is lws_port.cpp's stand-in
 *   and mainGlobs.imDevice (0050591c) mesh_port.cpp's; mainGlobs.flags
 *   (00505f84) is crt.cpp's.
 * - Container.c (G98-002): Container_GetRoot (00472940) is
 *   scene_root_object_get, the name container.c takes in the matching
 *   build. Container_SearchOwner (00475830) and
 *   Container_LoadTextureSurface (00474050) are container.c's by their own
 *   names there (finding 624); other builds call thin unannotated adapters
 *   at the end of mesh_port.cpp.
 * - Files.c, Utils.c, Maths.c and Lwo.c are converted units. */
#ifndef LEGORRR_GODS98_MESH_IMPORTS_H
#define LEGORRR_GODS98_MESH_IMPORTS_H

#ifdef __cplusplus
extern "C" {
#endif

struct GameFileHandle;

/* Main.c. */
extern void* s_main_d3drm;                                   /* 00505914 */
extern void* s_main_im_device;                               /* 0050591c */
extern unsigned int s_engine_feature_word;                   /* 00505f84 */
void direct3d_render_state_set_cached(unsigned int state,
                                      unsigned int value);   /* 004788d0 */
void d3d_material_state_flush(void);                         /* 00478940 */
#define main_change_render_state direct3d_render_state_set_cached
#define main_restore_states d3d_material_state_flush

/* Container.c. */
void* scene_root_object_get(void);                           /* 00472940 */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* The matching build: container.c (finding 624) defines these two. */
void* Container_LoadTextureSurface(
    const char* fname, int managed, unsigned long* width,
    unsigned long* height, int* trans);                      /* 00474050 */
void* Container_SearchOwner(void* frame);                    /* 00475830 */
#define container_load_texture_surface Container_LoadTextureSurface
#define container_frame_get_container Container_SearchOwner
#else
void* mesh_container_load_texture_surface_adapter(
    const char* fname, int managed, unsigned long* width,
    unsigned long* height, int* trans);                      /* 00474050 */
void* mesh_container_frame_get_container_adapter(void* frame); /* 00475830 */
#define container_load_texture_surface mesh_container_load_texture_surface_adapter
#define container_frame_get_container mesh_container_frame_get_container_adapter
#endif
#define container_get_root scene_root_object_get

/* Utils.c. */
int whitespace_tokenize(char* text, char** fields);         /* 004774b0 */
char* string_duplicate(const char* text);                    /* 00477550 */

/* Maths.c. */
float* surface_triangle_normal_calculate(float* n, const float* p1,
                                         const float* p2,
                                         const float* p3);   /* 00479530 */

/* Files.c. */
char* game_file_read_crlf_line(char* buffer, int capacity,
                               struct GameFileHandle* handle); /* 00480050 */
#define file_get_line game_file_read_crlf_line

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_MESH_IMPORTS_H */
