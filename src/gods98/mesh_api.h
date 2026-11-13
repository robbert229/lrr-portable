// SPDX-License-Identifier: LGPL-3.0-or-later
/* Mesh.c (original translation unit G98-015; finding 625): gods98's
 * immediate-mode meshes, rebuilt in original shape as gods98/mesh_unit.c.
 *
 * C-compatible header (ADR 0005). Shared C++ headers do not include this
 * file, so its declarations move no other unit's symbol numbering (finding
 * 538). The names are the unit's own, after OpenLRR's Mesh_* functions;
 * the comments give the original addresses. The types are opaque here:
 * mesh_unit.c defines their layouts. Mesh_Clone and Mesh_Remove take void
 * pointers, as gods98/mesh_unit.h declares them for the C callers.
 *
 * gods98/mesh_unit.h, the boundary header that lws.c, animclone.c, sfx.c
 * and weapons.c include, points those callers at these names in the
 * matching build; natively the port tail gods98/mesh_port.cpp serves the
 * port's names. */
#ifndef LEGORRR_GODS98_MESH_API_H
#define LEGORRR_GODS98_MESH_API_H

#ifdef __cplusplus
extern "C" {
#endif

struct APPOBJ;

/* Mesh_SetGroupColour and Mesh_SetGroupMaterialValues. */
typedef enum Mesh_Colour {
    Mesh_Colour_Diffuse,
    Mesh_Colour_Ambient,
    Mesh_Colour_Specular,
    Mesh_Colour_Emissive,
    Mesh_Colour_Alpha,
    Mesh_Colour_Power
} Mesh_Colour;

/* Mesh_CreateOnFrame. */
typedef enum Mesh_Type {
    Mesh_Type_Norm,
    Mesh_Type_PostEffect,
    Mesh_Type_LightWaveObject
} Mesh_Type;

/* Mesh::flags. */
typedef enum Mesh_Flags {
    MESH_FLAG_HIDDEN = 0x00000001,
    MESH_FLAG_POSTEFFECT = 0x00000002,
    MESH_FLAG_TRANSFORM_PARENTPOS = 0x00000004,
    MESH_FLAG_HASBEENCLONED = 0x00200000,
    MESH_FLAG_FACECAMERA = 0x01000000,
    MESH_FLAG_FACECAMERADONE = 0x02000000
} Mesh_Flags;

typedef struct Mesh Mesh;
typedef struct Mesh_Group Mesh_Group;
typedef struct Mesh_Texture Mesh_Texture;
typedef struct Mesh_TextureReference Mesh_TextureReference;
typedef struct Mesh_LightWave_Surface Mesh_LightWave_Surface;
typedef struct Mesh_D3DMaterial Mesh_D3DMaterial;   /* D3DMATERIAL */
typedef struct Mesh_Matrix Mesh_Matrix;             /* D3DMATRIX */
typedef struct Mesh_Vector3F Mesh_Vector3F;
typedef struct Mesh_Point2F Mesh_Point2F;
typedef struct Mesh_ContVertex Mesh_ContVertex;     /* Container.c's vertex */
typedef struct Mesh_Viewport Mesh_Viewport;         /* Viewports.c's Viewport */
typedef struct Mesh_Frame Mesh_Frame;               /* IDirect3DRMFrame3 */
typedef struct Mesh_RMViewport Mesh_RMViewport;     /* IDirect3DRMViewport */
typedef struct Mesh_Surface Mesh_Surface;           /* IDirectDrawSurface4 */
typedef struct Mesh_Texture2 Mesh_Texture2;         /* IDirect3DTexture2 */

/* A mesh's render callback: Mesh_SetMeshRenderDesc calls it before the
 * mesh's render states are set. */
typedef void (*Mesh_RenderCallbackFunc)(Mesh* mesh, void* data,
                                        Mesh_Viewport* vp);

int mesh_initialise(const char* sharedTextureDir);  /* 004805b0 */
int mesh_create_global_material(void);  /* 00480650 */
int mesh_set_material(Mesh_D3DMaterial* newMaterial);  /* 00480780 */
Mesh* mesh_obtain_from_list(void);  /* 004807a0 */
void mesh_return_to_list(Mesh* deadMesh);  /* 004807d0 */
void mesh_add_list(void);  /* 004807f0 */
Mesh* mesh_create_on_frame(Mesh_Frame* frame,
                           Mesh_RenderCallbackFunc renderFunc,
                           unsigned long renderFlags, void* data,
                           unsigned long type);  /* 00480870 */
void* mesh_clone(void* srcMesh, void* frame);  /* 00480900 */
Mesh* mesh_load(const char* fname, Mesh_Frame* frame, int noTextures);  /* 004809e0 */
int mesh_parse_lwo(const char* basePath, Mesh* mesh,
                   struct APPOBJ* lightWaveObject, int noTextures);  /* 00480ac0 */
void mesh_get_surf_info(const char* basePath, struct APPOBJ* lightWaveObject,
                        Mesh_LightWave_Surface* lightWaveSurf,
                        int noTextures);  /* 00481820 */
int mesh_get_texture_seq_info(const char* tname, char* tfname,
                              unsigned long* tstart, unsigned long* tnumlen);  /* 00481ac0 */
void mesh_get_next_in_sequence(const char* baseName, char* nextTextName,
                               unsigned long* texNum, unsigned long tnumlen);  /* 00481b80 */
void mesh_uview_mesh(struct APPOBJ* lightWaveObject, Mesh_Point2F* textCoords);  /* 00481c50 */
void mesh_get_texture_uvs_wrap(unsigned long vertexCount,
                               Mesh_Vector3F* vertices,
                               Mesh_Point2F* textCoords, float sx, float sy, float sz,
                               float px, float py, float pz,
                               unsigned long flags);  /* 00481fa0 */
int mesh_set_texture_time2(Mesh* mesh, float frame);  /* 00482040 */
void mesh_remove(void* meshPtr, void* frame);  /* 004820d0 */
void mesh_get_group(Mesh* mesh, unsigned long groupID,
                    unsigned long* vertexCount, unsigned long* faceCount,
                    unsigned long* vPerFace, unsigned long* faceDataSize,
                    unsigned long* faceData);  /* 004821a0 */
unsigned long mesh_get_group_count(Mesh* mesh);  /* 00482210 */
long mesh_add_group(Mesh* mesh, unsigned long vertexCount,
                    unsigned long faceCount, unsigned long vPerFace,
                    unsigned long* faceData);  /* 00482220 */
void mesh_alter_group_render_flags(Mesh* mesh, unsigned long groupID,
                                   unsigned long newFlags);  /* 00482350 */
void mesh_scale(Mesh* mesh, float x, float y, float z);  /* 00482370 */
void mesh_set_vertices(Mesh* mesh, unsigned long groupID,
                       unsigned long index, unsigned long count,
                       Mesh_ContVertex* vertices);  /* 004823e0 */
void mesh_get_vertices(Mesh* mesh, unsigned long groupID,
                       unsigned long index, unsigned long count,
                       Mesh_ContVertex* vertices);  /* 00482470 */
void mesh_set_vertices_point_normal_at(Mesh* mesh, unsigned long groupID,
                                       unsigned long index,
                                       unsigned long count,
                                       Mesh_Vector3F* vertices,
                                       Mesh_Vector3F* position,
                                       Mesh_Point2F* textCoords);  /* 00482500 */
void mesh_set_vertices_same_normal(Mesh* mesh, unsigned long groupID,
                                   unsigned long index, unsigned long count,
                                   Mesh_Vector3F* vertices,
                                   Mesh_Vector3F* normal,
                                   Mesh_Point2F* textCoords);  /* 00482620 */
void mesh_set_vertices_vnt(Mesh* mesh, unsigned long groupID,
                           unsigned long index, unsigned long count,
                           Mesh_Vector3F* vertices, Mesh_Vector3F** normal,
                           Mesh_Point2F* textCoords);  /* 004826c0 */
int mesh_is_group_hidden(Mesh* mesh, unsigned long groupID);  /* 00482780 */
void mesh_hide_group(Mesh* mesh, unsigned long groupID, int hide);  /* 004827a0 */
void mesh_hide(Mesh* mesh, int hide);  /* 004827d0 */
void mesh_set_mesh_render_desc(Mesh* mesh, Mesh_Viewport* vp,
                               Mesh_Matrix* matWorld, int alphaBlend);  /* 00482ac0 */
void mesh_set_render_desc(unsigned long flags, Mesh_Matrix* matWorld,
                          int alphaBlend);  /* 00482b50 */
void mesh_set_alpha_render(unsigned long src, unsigned long dest);  /* 00482cb0 */
void mesh_add_to_post_render_list(Mesh* mesh, Mesh_Matrix* matWorld);  /* 00482ce0 */
void mesh_clear_post_render_list(void);  /* 00482d30 */
void mesh_post_render_all(Mesh_Viewport* vp);  /* 00482d60 */
Mesh_Texture* mesh_load_texture(const char* baseDir, const char* name,
                        unsigned long* width, unsigned long* height);  /* 00482e70 */
Mesh_TextureReference* mesh_search_texture_list(
    Mesh_TextureReference* textureList, unsigned long count,
    const char* path);  /* 00483030 */
void mesh_add_texture_reference(Mesh_TextureReference* textureList,
                                unsigned long* count, const char* path,
                                Mesh_Surface* surface, int trans);  /* 00483080 */
void mesh_set_group_texture(Mesh* mesh, unsigned long groupID,
                            Mesh_Texture* texture);  /* 004830c0 */
void mesh_remove_group_texture(Mesh* mesh, unsigned long groupID);  /* 00483140 */
int mesh_create_group_material(Mesh* mesh, unsigned long groupID);  /* 00483170 */
int mesh_set_group_material(Mesh* mesh, unsigned long groupID,
                            Mesh_D3DMaterial* material);  /* 00483240 */
int mesh_set_group_colour(Mesh* mesh, unsigned long groupID, float r,
                          float g, float b, unsigned long type);  /* 00483270 */
Mesh_D3DMaterial* mesh_get_group_material(Mesh* mesh, unsigned long groupID);  /* 00483400 */
int mesh_set_group_material_values(Mesh* mesh, unsigned long groupID,
                                   float value, unsigned long type);  /* 00483420 */
void mesh_set_identity_matrix(Mesh_Matrix* m);  /* 00483540 */
int mesh_set_current_viewport(Mesh_RMViewport* lpView);  /* 00483580 */
int mesh_set_current_gods_viewport(Mesh_Viewport* vp);  /* 00483600 */
int mesh_set_transform(unsigned long state, Mesh_Matrix* mat);  /* 00483690 */
int mesh_set_texture_stage_state(unsigned long type, unsigned long value);  /* 00483810 */
void mesh_store_texture_and_mat(void);  /* 004838b0 */
void mesh_restore_texture_and_mat(void);  /* 00483940 */
int mesh_render_mesh(Mesh* mesh, Mesh_Matrix* matWorld, int alphaBlend);  /* 004839c0 */
int mesh_can_render_group(Mesh_Group* group);  /* 00483a70 */
int mesh_render_group(Mesh* mesh, Mesh_Group* group, Mesh_Matrix* matWorld,
                      int alphaBlend);  /* 00483a90 */
int mesh_set_group_render_desc(Mesh* mesh, Mesh_Group* group,
                               Mesh_Matrix* matWorld, int alphaBlend);  /* 00483b00 */
int mesh_render_triangle_list(unsigned long matHandle, Mesh_Texture2* texture,
                              unsigned long renderFlags, void* vertices,
                              unsigned long vertexCount,
                              unsigned short* faceData,
                              unsigned long indexCount);  /* 00483b70 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_MESH_API_H */
