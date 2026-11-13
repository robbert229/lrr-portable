// SPDX-License-Identifier: LGPL-3.0-or-later
/* Detail.c, original translation unit GAME-011 (finding 505), rebuilt in
 * original shape (ADR 0005; finding 615): text 0040b310-0040bdc0, the
 * .rdata floats 0049e0d0-0049e0ec and the literals 004a1da8-004a1dbc. The
 * unit has no .bss or .data.
 *
 * The block detail meshes. A level's texture set is a grid of
 * "<base><x><y>.bmp" textures; a detail mesh is a pair of LightWave meshes
 * (uncovered and surface), cloned onto each visible block, placed on three
 * of its corners and given the block's texture.
 *
 * The port's implementation stays in the port tail tile_variants_port.cpp,
 * unannotated. In the matching build detail_link_names.h gives eight of
 * this unit's functions the port's names, so every caller reaches them;
 * natively the port tail serves those names. */
#include "detail_link_names.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "detail.h"
#include "detail_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

/* A container mesh vertex (0x24 bytes). */
typedef struct Detail_Vertex {
    Detail_Vector3F position;   /* +0x00 */
    Detail_Vector3F normal;     /* +0x0c */
    float tu;                   /* +0x18 */
    float tv;                   /* +0x1c */
    unsigned int colour;        /* +0x20 */
} Detail_Vertex;

typedef struct Detail_Vector4F {
    float x, y, z, w;
} Detail_Vector4F;

/* Container_Combine_Before. */
#define DETAIL_COMBINE_BEFORE 1
#define DETAIL_COMBINE_AFTER 2

#include "detail_maths.h"

// FUNCTION: LEGORR 0x0040b310
Detail_TextureSet* detail_load_texture_set(const char* textureBaseName,
                                           unsigned int width,
                                           unsigned int height)
{
    Detail_TextureSet* textureSet;
    unsigned int x, y;
    char name[1024];

    if (textureSet = (Detail_TextureSet*)malloc(sizeof(Detail_TextureSet))) {
        if (textureSet->textures = (void**)malloc(sizeof(void*) * width * height)) {
            textureSet->width = width;
            textureSet->height = height;
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    // STRING: LEGORR 0x004a1da8
                    sprintf(name, "%s%0.1x%0.1x.bmp", textureBaseName, x, y);
                    textureSet->textures[(y * width) + x] =
                        detail_container_load_texture2_adapter(name, 1, NULL, NULL);
                }
            }
        } else {
            free(textureSet);
            textureSet = NULL;
        }
    }
    return textureSet;
}

// FUNCTION: LEGORR 0x0040b410
void detail_free_texture_set(Detail_TextureSet* textureSet)
{
    unsigned int x, y;

    for (y = 0; y < textureSet->height; y++) {
        for (x = 0; x < textureSet->width; x++) {
            detail_container_free_texture_adapter(
                textureSet->textures[(y * textureSet->width) + x]);
        }
    }
    free(textureSet->textures);
    free(textureSet);
}

// FUNCTION: LEGORR 0x0040b470
float detail_get_block_screen_area(Detail_Vector3F* vertPoses, void* vp,
                                   const float* screenSize, int noClip)
{
    Detail_Vector3F screenPts[4];
    Detail_Vector4F transPts[4];
    float area = 0.0f;
    float mult = 1.0f;
    unsigned int loop;

    for (loop = 0; loop < 4; loop++) {
        render_interface_transform_point(vp, &transPts[loop], &vertPoses[loop]);
        screenPts[loop].x = transPts[loop].x / transPts[loop].w;
        screenPts[loop].y = transPts[loop].y / transPts[loop].w;
    }

    if (!noClip) {
        for (loop = 0; loop < 4; loop++) {
            if (screenPts[loop].x < 0.0f) mult -= 0.25f;
            else if (screenPts[loop].y < 0.0f) mult -= 0.25f;
            else if (screenPts[loop].x > screenSize[0]) mult -= 0.25f;
            else if (screenPts[loop].y > screenSize[1]) mult -= 0.25f;
            else if (transPts[loop].z > transPts[loop].w ||
                     transPts[loop].z < 0.0f) mult -= 0.25;
        }
    }

    if (mult > 0.0f) {
        area = surface_triangle_area_calculate(&screenPts[2].x, &screenPts[1].x,
                                               &screenPts[0].x, 0);
        area += surface_triangle_area_calculate(&screenPts[0].x, &screenPts[3].x,
                                                &screenPts[2].x, 0);
        area *= mult;
    }
    return area;
}

// FUNCTION: LEGORR 0x0040b5f0
Detail_Mesh* detail_load_meshes(void* root, const char* uncoveredName,
                                const char* surfaceName, float blockSize,
                                Detail_TextureSet* textureSet)
{
    Detail_Mesh* mesh;
    unsigned int loop, sub, vertexCount;
    Detail_Vertex* vertices;
    int swapV, swapU;
    float scale;

    if (surfaceName == NULL) surfaceName = uncoveredName;

    if (mesh = (Detail_Mesh*)malloc(sizeof(Detail_Mesh))) {
        // STRING: LEGORR 0x004a1dbc
        if (mesh->cont[0] = container_load(root, uncoveredName, "LWO:NOTEXTURE", 1)) {
            if (!(mesh->cont[1] = container_load(root, surfaceName, "LWO:NOTEXTURE", 1))) {
                mesh->cont[1] = container_load(root, uncoveredName, "LWO:NOTEXTURE", 1);
            }

            scale = blockSize * 0.01f;
            mesh->cloneCount = 0;
            mesh->blockSize = blockSize;
            mesh->textureSet = textureSet;

            for (loop = 0; loop < DETAIL_MESH_COUNT; loop++) {
                detail_container_mesh_scale_adapter(mesh->cont[loop], scale, scale, scale);
                container_mesh_perspective_correction_set(mesh->cont[loop], 0, 1);
                scene_render_lock_toggle(mesh->cont[loop], 1);
                memset(mesh->clones[loop], 0, sizeof(mesh->clones[loop]));
            }

            for (loop = 0; loop < DETAIL_MESH_COUNT; loop++) {
                if (loop == 0) swapV = swapU = 0;
                else swapV = swapU = 1;

                detail_container_mesh_get_group_adapter(mesh->cont[loop], 0, &vertexCount,
                                                        NULL, NULL, NULL, NULL);
                vertices = (Detail_Vertex*)malloc(sizeof(Detail_Vertex) * vertexCount);
                lwo_layer_polygon_vertex_data_read(mesh->cont[loop], 0, 0, vertexCount, vertices);
                for (sub = 0; sub < vertexCount; sub++) {
                    if (swapU) vertices[sub].tu = 1.0f - vertices[sub].tu;
                    if (swapV) vertices[sub].tv = 1.0f - vertices[sub].tv;
                }
                container_mesh_set_vertices(mesh->cont[loop], 0, 0, vertexCount, vertices);
                free(vertices);
            }

            return mesh;
        }
        free(mesh);
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0040b7d0
void detail_free_mesh(Detail_Mesh* mesh)
{
    unsigned int loop;

    if (mesh) {
        detail_reset_all(mesh);
        for (loop = 0; loop < DETAIL_MESH_COUNT; loop++) {
            ledger_stub_00472a40_DDiSceneObjectDestroy(mesh->cont[loop]);
        }
        free(mesh);
    }
}

// FUNCTION: LEGORR 0x0040b810
void detail_reset_all(Detail_Mesh* mesh)
{
    unsigned int loop, sub;

    mesh->cloneCount = 0;
    for (loop = 0; loop < DETAIL_MESH_COUNT; loop++) {
        for (sub = 0; sub < DETAIL_MAXCLONES; sub++) {
            if (mesh->clones[loop][sub]) {
                ledger_stub_00472a40_DDiSceneObjectDestroy(mesh->clones[loop][sub]);
                mesh->clones[loop][sub] = NULL;
            }
        }
    }
}

// FUNCTION: LEGORR 0x0040b850
void* detail_get_texture(Detail_TextureSet* textureSet, unsigned char texture)
{
    if (textureSet) {
        unsigned int x = ((unsigned int)texture & 0xf0) >> 4;
        unsigned int y = (unsigned int)texture & 0x0f;
        return textureSet->textures[(y * textureSet->width) + x];
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0040b880
void detail_add_block(Detail_Mesh* mesh, Detail_Vector3F* vertPoses,
                      unsigned char texture, unsigned char direction,
                      float depth, float alpha)
{
    if (mesh) {
        float blockSize = mesh->blockSize;
        unsigned int vertOrder[4][3] = {
            { 3, 0, 2 }, { 0, 1, 3 }, { 1, 2, 0 }, { 2, 3, 1 }
        };
        void* cont[DETAIL_MESH_COUNT];
        void* tex;
        unsigned int loop;
        unsigned int* order;

        tex = detail_get_texture(mesh->textureSet, texture);
        for (loop = 0; loop < DETAIL_MESH_COUNT; loop++) {
            mesh->clones[loop][mesh->cloneCount] = entity_visual_resource_load(mesh->cont[loop]);
            cont[loop] = mesh->clones[loop][mesh->cloneCount];
        }
        order = vertOrder[direction];
        detail_place_mesh(cont[0], blockSize, &vertPoses[order[0]], &vertPoses[order[1]],
                          &vertPoses[order[2]], depth, alpha, tex, 0);
        order = vertOrder[(direction + 2) % 4];
        detail_place_mesh(cont[1], blockSize, &vertPoses[order[0]], &vertPoses[order[1]],
                          &vertPoses[order[2]], depth, alpha, tex, 0);
        mesh->cloneCount++;
    }
}

// FUNCTION: LEGORR 0x0040ba00
void detail_place_mesh(void* cont, float blockSize, Detail_Vector3F* a,
                       Detail_Vector3F* b, Detail_Vector3F* c, float depth,
                       float alpha, void* texture, unsigned char flags)
{
    unsigned int loop;

    detail_set_mesh_frame(cont, blockSize, a, b, c);
    scene_render_lock_toggle(cont, 0);
    for (loop = 0; loop < (unsigned int)animated_texture_frame_count(cont); loop++) {
        animated_texture_frame_set(cont, loop, texture);
    }
    scene_object_camera_teleport_or_offset(cont, DETAIL_COMBINE_BEFORE, -0.02f, -0.02f, 0.0f);
    scene_frame_scale_set(cont, DETAIL_COMBINE_BEFORE, 1.015f, 1.015f, depth);
    render_object_set_packed_rgba_from_float_channels(cont, alpha, alpha, alpha, 1.0f);

    if (flags & 0x01) render_object_rotate_axis(cont, DETAIL_COMBINE_BEFORE, 1.0f, 0.0f, 0.0f, 1.57079637f);
    if (flags & 0x02) render_object_rotate_axis(cont, DETAIL_COMBINE_BEFORE, 1.0f, 0.0f, 0.0f, -1.57079637f);
    if (flags & 0x04) render_object_rotate_axis(cont, DETAIL_COMBINE_BEFORE, 0.0f, 1.0f, 0.0f, 1.57079637f);
    if (flags & 0x08) render_object_rotate_axis(cont, DETAIL_COMBINE_BEFORE, 0.0f, 1.0f, 0.0f, -1.57079637f);
    if (flags & 0x10) render_object_rotate_axis(cont, DETAIL_COMBINE_BEFORE, 0.0f, 0.0f, 1.0f, 1.57079637f);
    if (flags & 0x20) render_object_rotate_axis(cont, DETAIL_COMBINE_BEFORE, 0.0f, 0.0f, 1.0f, -1.57079637f);
    if (flags & 0x80) scene_object_camera_teleport_or_offset(cont, DETAIL_COMBINE_AFTER, 0.0f, -10.0f, 0.0f);
    if (flags & 0x40) scene_object_camera_teleport_or_offset(cont, DETAIL_COMBINE_AFTER, 0.0f, 10.0f, 0.0f);
}

// FUNCTION: LEGORR 0x0040bb90
void detail_set_mesh_frame(void* cont, float blockSize, Detail_Vector3F* a,
                           Detail_Vector3F* b, Detail_Vector3F* c)
{
    Detail_Vector3F dir, up, tb, tc;
    float mat[4][4];
    float dirScale, upScale;

    detail_vector3d_subtract(&dir, b, a);
    detail_vector3d_subtract(&up, c, b);
    detail_vector3d_cross_product(&up, &up, &dir);
    dirScale = 1.0f / detail_vector3d_modulus(&dir);
    upScale = 1.0f / detail_vector3d_modulus(&up);

    ddiscene_object_pose_method_dispatch(cont, NULL, -(up.x * upScale), -(up.y * upScale),
                                         -(up.z * upScale), dir.x * dirScale,
                                         dir.y * dirScale, dir.z * dirScale);
    ddiscene_object_position_set(cont, NULL, a->x, a->y, a->z);
    detail_container_inverse_transform_adapter(cont, &tb, b);
    detail_container_inverse_transform_adapter(cont, &tc, c);
    detail_matrix_setup(&mat[0][0], blockSize, blockSize, tb.y, tc.x, tc.y);
    legoobject_gt_container_matrix_set_adapter(cont, DETAIL_COMBINE_BEFORE, &mat[0][0]);
}

// FUNCTION: LEGORR 0x0040bd60
void detail_matrix_setup(float* m, float sx, float sy, float y, float x2,
                         float y2)
{
    m[3] = 0.0f;
    m[2] = 0.0f;
    m[7] = 0.0f;
    m[6] = 0.0f;
    m[4] = 0.0f;
    m[11] = 0.0f;
    m[9] = 0.0f;
    m[8] = 0.0f;
    m[14] = 0.0f;
    m[13] = 0.0f;
    m[12] = 0.0f;
    m[15] = 1.0f;
    m[10] = 1.0f;
    m[0] = x2 / sx;
    m[5] = y / sy;
    m[1] = (y2 / x2) * (x2 / sx);
}

#ifdef __cplusplus
}
#endif
