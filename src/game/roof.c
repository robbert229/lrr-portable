// SPDX-License-Identifier: LGPL-3.0-or-later
/* Roof.c, original translation unit GAME-038 (finding 505), rebuilt in
 * original shape (ADR 0005; finding 619): text 00462da0-004636b0, the
 * literal pool 0049e6fc-0049e700 and the common block 00552180-00552970.
 *
 * The level's roof. Roof_Initialise makes one mesh container with a grid
 * entry per block. Lego.c gives a block its quad (Roof_SetBlockRoofVertices)
 * when the block becomes a roof block; the quad's group is made then, and
 * the block is marked. Roof_Update gives each corner of a marked block the
 * mean of the plane normals of the four blocks around it. Lego.c hides the
 * blocks under the cursor for one frame (Roof_HideBlock, then
 * Roof_ShowHiddenBlocks) and the whole roof outside the top-down view.
 *
 * The port's implementation stays elsewhere, unannotated: the port tail
 * terrain_overlay_quads_port.cpp and gods98/resource.cpp. In the matching
 * build lego_unit_imports.h points Lego.c at this unit's names; natively
 * the port serves the callers and nothing calls these. */
#include <math.h>
#include <stdlib.h>

#include "crt/vc5names.h"
#include "roof.h"
#include "roof_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

/* A container mesh vertex (0x24 bytes). */
typedef struct Roof_Vertex {
    Roof_Vector3F position;  /* +0x00 */
    Roof_Vector3F normal;    /* +0x0c */
    float tu;                /* +0x18 */
    float tv;                /* +0x1c */
    unsigned int colour;     /* +0x20 */
} Roof_Vertex;

#include "roof_maths.h"

/* Container_Mesh_Type_Normal. */
#define ROOF_MESH_TYPE 2

// GLOBAL: LEGORR 0x00552180
Roof_Globs roofGlobs;

// FUNCTION: LEGORR 0x00462da0
int roof_initialise(void* contRoot, unsigned int width, unsigned int height)
{
    unsigned int loop, count;

    if (roofGlobs.contMesh = container_make_mesh2(contRoot, ROOF_MESH_TYPE)) {
        roofGlobs.texture = NULL;
        roofGlobs.hidden = 0;
        count = width * height;
        if (roofGlobs.grid = (RoofBlock*)malloc(sizeof(RoofBlock) * count)) {
            for (loop = 0; loop < count; loop++) {
                roofGlobs.grid[loop].groupID = 0;
                roofGlobs.grid[loop].flags = ROOF_FLAG_NOGROUP;
            }
            roofGlobs.needUpdate = 1;
            roofGlobs.width = width;
            roofGlobs.height = height;
            roofGlobs.hiddenCount = 0;
            return 1;
        }
        ledger_stub_00472a40_DDiSceneObjectDestroy(roofGlobs.contMesh);
    }
    return 0;
}

// FUNCTION: LEGORR 0x00462e60
int roof_set_texture(const char* filename)
{
    unsigned int loop, count;

    if (roofGlobs.texture) detail_container_free_texture_adapter(roofGlobs.texture);
    if (roofGlobs.texture = detail_container_load_texture2_adapter(filename, 1, NULL, NULL)) {
        count = animated_texture_frame_count(roofGlobs.contMesh);
        for (loop = 0; loop < count; loop++) {
            animated_texture_frame_set(roofGlobs.contMesh, loop, roofGlobs.texture);
        }
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00462ed0
void roof_shutdown(void)
{
    if (roofGlobs.grid) free(roofGlobs.grid);
    roofGlobs.grid = NULL;
    if (roofGlobs.contMesh) ledger_stub_00472a40_DDiSceneObjectDestroy(roofGlobs.contMesh);
    roofGlobs.contMesh = NULL;
    detail_container_free_texture_adapter(roofGlobs.texture);
}

// FUNCTION: LEGORR 0x00462f20
void roof_set_block_roof_vertices(unsigned int bx, unsigned int by,
                                  const Roof_Vector3F* pos0,
                                  const Roof_Vector3F* pos1,
                                  const Roof_Vector3F* pos2,
                                  const Roof_Vector3F* pos3)
{
    RoofBlock* block = &roofGlobs.grid[(by * roofGlobs.width) + bx];
    Roof_Vertex vertices[4];
    Roof_Vector3F normal = { 0.0f, 0.0f, 0.0f };
    unsigned int faceData[6];
    int rotate = 0;

    if (block->flags & ROOF_FLAG_NOGROUP) {
        faceData[0] = 0; faceData[1] = 1; faceData[2] = 3;
        faceData[3] = 1; faceData[4] = 2; faceData[5] = 3;
        if ((block->groupID = mesh_record_allocate_or_append(roofGlobs.contMesh, 4, 2, 3, (const unsigned short*)faceData)) != -1) {
            animated_texture_frame_set(roofGlobs.contMesh, block->groupID, roofGlobs.texture);
            container_mesh_perspective_correction_set(roofGlobs.contMesh, block->groupID, 1);
            container_mesh_hide_group(roofGlobs.contMesh, block->groupID, 1);
            block->flags &= ~ROOF_FLAG_NOGROUP;
        }
    }

    roofGlobs.needUpdate = 1;
    block->flags |= ROOF_FLAG_UPDATE;

    /* Split along the diagonal through the lowest corner. */
    if (pos0->z < pos2->z) {
        if (pos0->z > pos1->z || pos0->z > pos3->z) rotate = 1;
    } else {
        if (pos2->z > pos1->z || pos2->z > pos3->z) rotate = 1;
    }

    if (rotate) {
        vertices[3].position = *pos1;
        vertices[2].position = *pos2;
        vertices[1].position = *pos3;
        vertices[0].position = *pos0;
        block->flags |= ROOF_FLAG_ROTATED;
    } else {
        vertices[3].position = *pos0;
        vertices[2].position = *pos1;
        vertices[1].position = *pos2;
        vertices[0].position = *pos3;
        block->flags &= ~ROOF_FLAG_ROTATED;
    }

    vertices[3].normal = normal;
    vertices[2].normal = normal;
    vertices[1].normal = normal;
    vertices[0].normal = normal;
    vertices[3].colour = 0xffffffff;
    vertices[2].colour = 0xffffffff;
    vertices[1].colour = 0xffffffff;
    vertices[0].colour = 0xffffffff;
    vertices[3].tu = 0.0f; vertices[3].tv = 1.0f;
    vertices[2].tu = 1.0f; vertices[2].tv = 1.0f;
    vertices[1].tu = 1.0f; vertices[1].tv = 0.0f;
    vertices[0].tu = 0.0f; vertices[0].tv = 0.0f;

    container_mesh_set_vertices(roofGlobs.contMesh, block->groupID, 0, 4, vertices);
}

// FUNCTION: LEGORR 0x00463230
void roof_update(void)
{
    Roof_Vertex vertices[4];
    Roof_Vector3F normal;
    unsigned int bx, by, loop, corner, sub;
    RoofBlock* block;

    if (roofGlobs.needUpdate) {
        roofGlobs.needUpdate = 0;
        for (by = 0; by < roofGlobs.height; by++) {
            for (bx = 0; bx < roofGlobs.width; bx++) {
                block = &roofGlobs.grid[(by * roofGlobs.width) + bx];
                if (block->flags & ROOF_FLAG_UPDATE) {
                    /* The four blocks around each corner, for each corner. */
                    int offsets[4][4][2] = {
                        { { 0, 0 }, { -1, 0 }, { 0, 1 }, { -1, 1 } },
                        { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } },
                        { { 0, 0 }, { 0, -1 }, { 1, -1 }, { 1, 0 } },
                        { { 0, 0 }, { -1, -1 }, { 0, -1 }, { -1, 0 } },
                    };
                    lwo_layer_polygon_vertex_data_read(roofGlobs.contMesh, block->groupID, 0, 4, vertices);
                    for (loop = 0; loop < 4; loop++) {
                        if (block->flags & ROOF_FLAG_ROTATED) corner = (loop - 1) & 3;
                        else corner = loop;
                        for (sub = 0; sub < 4; sub++) {
                            if (roof_get_block_plane_normal(bx + offsets[corner][sub][0], by + offsets[corner][sub][1], &normal)) {
                                roof_vector3d_add(&vertices[loop].normal, &normal, &vertices[loop].normal);
                            }
                        }
                        roof_vector3d_normalise(&vertices[loop].normal);
                    }
                    container_mesh_set_vertices(roofGlobs.contMesh, block->groupID, 0, 4, vertices);
                    block->flags &= ~ROOF_FLAG_UPDATE;
                }
            }
        }
    }
}

// FUNCTION: LEGORR 0x004634b0
void roof_hide(int hide)
{
    if (roofGlobs.hidden != hide) {
        roofGlobs.hidden = hide;
        if (hide) scene_render_lock_toggle(roofGlobs.contMesh, 1);
        else scene_render_lock_toggle(roofGlobs.contMesh, 0);
    }
}

// FUNCTION: LEGORR 0x004634f0
void roof_hide_block(unsigned int bx, unsigned int by)
{
    RoofBlock* block = &roofGlobs.grid[(by * roofGlobs.width) + bx];

    container_mesh_hide_group(roofGlobs.contMesh, block->groupID, 0);
    roofGlobs.hiddenList[roofGlobs.hiddenCount++] = block;
}

// FUNCTION: LEGORR 0x00463540
void roof_show_hidden_blocks(void)
{
    unsigned int loop;

    for (loop = 0; loop < roofGlobs.hiddenCount; loop++) {
        container_mesh_hide_group(roofGlobs.contMesh, roofGlobs.hiddenList[loop]->groupID, 1);
    }
    roofGlobs.hiddenCount = 0;
}

// FUNCTION: LEGORR 0x00463590
int roof_get_block_plane_normal(unsigned int bx, unsigned int by,
                                Roof_Vector3F* normal)
{
    Roof_Vertex vertices[4];
    Roof_Vector3F first, second;
    RoofBlock* block;

    if (bx < roofGlobs.width && by < roofGlobs.height) {
        block = &roofGlobs.grid[(by * roofGlobs.width) + bx];
        if (!(block->flags & ROOF_FLAG_NOGROUP)) {
            lwo_layer_polygon_vertex_data_read(roofGlobs.contMesh, block->groupID, 0, 4, vertices);
            surface_triangle_normal_calculate(&first.x, &vertices[0].position.x, &vertices[1].position.x, &vertices[2].position.x);
            surface_triangle_normal_calculate(&second.x, &vertices[2].position.x, &vertices[3].position.x, &vertices[0].position.x);
            roof_vector3d_add(normal, &second, &first);
            roof_vector3d_normalise(normal);
            return 1;
        }
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
