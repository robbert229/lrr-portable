// SPDX-License-Identifier: LGPL-3.0-or-later
/* Mesh.c, original translation unit G98-015 (finding 505), rebuilt in
 * original shape (ADR 0005; finding 625): text 004805b0-00483c80, the
 * literal pool 0049e9e0-0049e9f8 and the .bss block 005343c4-005405dc.
 *
 * gods98's immediate-mode meshes. A Mesh is a Direct3DRM user visual on a
 * frame whose render callback (Mesh_RenderCallback) draws its groups with
 * the Direct3D device's DrawIndexedPrimitive. Mesh_Load reads a LightWave
 * object (Lwo.c) into one group per surface, with the surface's textures,
 * colours and render flags. Meshes come from a pool allocated in blocks of
 * 1, 2, 4, ... records (meshGlobs.listSet), with a free list linked through
 * the last word. Meshes drawn with alpha blending are queued on the
 * post-render list and drawn after the scene (Mesh_PostRenderAll, from
 * Viewport_Render).
 *
 * The port's implementation stays in the port tail gods98/mesh_port.cpp,
 * unannotated. In the matching build mesh_link_names.h gives six of these
 * functions the port's names, and the C callers' boundary headers
 * (mesh_unit.h, screen_effect_quads_imports.h, lego_unit_imports.h,
 * viewports_imports.h) point them at the others; natively the port tail
 * serves the callers and nothing calls these. */
#include "mesh_link_names.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"

/* The original's Standard.h includes the Windows and DirectX headers; the
 * unit calls miniwin's DirectX interfaces. */
#include <miniwin/d3drm.h>

#include "mesh_api.h"
#include "mesh_imports.h"
#include "lwo.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

#define MESH_MAXLISTS 20
#define MESH_TEXTURELISTSIZE 2048
#define MESH_MAXTEXTURESTAGESTATES 50
#define MESH_MAXTEXTURESEQUENCE 100
#define MESH_DEFLISTSIZE 10
#define MESH_LISTINCREASE 150 /* percent */

/* Group flags (+0x6c). */
#define MESH_GROUP_FLAG_HIDDEN 0x00000001
#define MESH_GROUP_FLAG_ALPHAENABLE 0x00080000
#define MESH_GROUP_FLAG_COLOURNOTINTERPOLATED 0x00100000
#define MESH_GROUP_FLAG_TEXTURECOLOURONLY 0x00100000
#define MESH_GROUP_FLAG_HASCOLOURKEY 0x00400000
#define MESH_GROUP_FLAG_ALPHAHIDDEN 0x10000000

/* Render flags. */
#define MESH_RENDERFLAGS_ALPHAMASK 0x00001c00
#define MESH_RENDERFLAGS_ALPHATRANS 0x00000400
#define MESH_RENDERFLAGS_ALPHASA1 0x00000800
#define MESH_RENDERFLAGS_ALPHASA0 0x00001000
#define MESH_RENDERFLAGS_LWOALPHA 0x00002000
#define MESH_RENDERFLAGS_LWONORM 0x00003000
#define MESH_RENDERFLAGS_ADDITIVE 0x00002800

/* LightWave surface flags (FLAG and TFLG). */
#define MESH_SFLAG_COLOURHIGHLIGHT 0x00000008
#define MESH_SFLAG_ADDITIVE 0x00000200
#define MESH_SFLAG_DOUBLESIDED 0x00000100
#define MESH_SFLAG_PIXELBLENDING 0x00000080
#define MESH_TFLAG_SEQUENCE 0x00000080
#define MESH_TFLAG_PIXELBLENDING 0x00000020

struct Mesh_Vector3F {
    float x;
    float y;
    float z;
};

#include "mesh_maths.h"

struct Mesh_Point2F {
    float x;
    float y;
};

typedef struct Mesh_Colour4F {
    float r;
    float g;
    float b;
    float a;
} Mesh_Colour4F;

/* D3DMATRIX. */
struct Mesh_Matrix {
    float m[4][4];
};

/* A group's vertex (0x20 bytes, D3DFVF_VERTEX). */
typedef struct Mesh_Vertex {
    Mesh_Vector3F position; /* +0x00 */
    Mesh_Vector3F normal;   /* +0x0c */
    float tu;               /* +0x18 */
    float tv;               /* +0x1c */
} Mesh_Vertex;

/* Container.c's vertex (0x24 bytes). */
struct Mesh_ContVertex {
    Mesh_Vector3F position; /* +0x00 */
    Mesh_Vector3F normal;   /* +0x0c */
    float tu;               /* +0x18 */
    float tv;               /* +0x1c */
    unsigned long colour;   /* +0x20 */
};

/* Viewports.c's Viewport: the Direct3DRM viewport at +0x10 and the
 * rendering flag at +0x18. */
struct Mesh_Viewport {
    float xoffset;          /* +0x00 */
    float yoffset;          /* +0x04 */
    float width;            /* +0x08 */
    float height;           /* +0x0c */
    LPDIRECT3DRMVIEWPORT2 lpVP; /* +0x10 */
    float smoothFOV;        /* +0x14 */
    int rendering;          /* +0x18 */
};

/* Container.c's Container: its flags at +0x14. */
typedef struct Mesh_Container {
    unsigned long fields_00[5];
    unsigned long flags;    /* +0x14 */
} Mesh_Container;

#define MESH_CONTAINER_FLAG_HIDDEN 0x00000080

/* A texture (0xc bytes). */
struct Mesh_Texture {
    LPDIRECTDRAWSURFACE4 surface; /* +0x00 */
    LPDIRECT3DTEXTURE2 texture;   /* +0x04 */
    int colourKey;          /* +0x08 */
};

/* A loaded texture's name (0xc bytes). */
struct Mesh_TextureReference {
    LPDIRECTDRAWSURFACE4 surface; /* +0x00 */
    char* path;                   /* +0x04 */
    int trans;              /* +0x08 */
};

/* A LightWave surface's material (0x3c bytes). */
struct Mesh_LightWave_Surface {
    Mesh_Texture** textureSeq;  /* +0x00 */
    Mesh_Texture* texture;      /* +0x04 */
    unsigned long numInTexSeq;  /* +0x08 */
    Mesh_Colour4F colour;       /* +0x0c */
    float diffuse;              /* +0x1c */
    float emissive;             /* +0x20 */
    float specular;             /* +0x24 */
    float power;                /* +0x28 */
    float transparency;         /* +0x2c */
    unsigned long flags;        /* +0x30 */
    unsigned long texFlags;     /* +0x34 */
    long texSeqOffset;          /* +0x38 */
};

/* A group (0x70 bytes). */
struct Mesh_Group {
    unsigned long faceDataSize;            /* +0x00 */
    unsigned long vertexCount;             /* +0x04 */
    unsigned short* faceData;              /* +0x08 */
    Mesh_Vertex* vertices;                 /* +0x0c */
    D3DMATERIAL material;                  /* +0x10 */
    LPDIRECT3DTEXTURE2 imText;             /* +0x60 */
    unsigned long renderFlags;             /* +0x64 */
    Mesh_LightWave_Surface* lightWaveSurfaceInfo; /* +0x68 */
    unsigned long flags;                   /* +0x6c */
};


/* A mesh (0x34 bytes). */
struct Mesh {
    unsigned long groupCount;              /* +0x00 */
    unsigned long listSize;                /* +0x04 */
    Mesh_Group* groupList;                 /* +0x08 */
    LPDIRECT3DRMUSERVISUAL uv;             /* +0x0c */
    Mesh_RenderCallbackFunc renderCallback; /* +0x10 */
    void* renderCallbackData;              /* +0x14 */
    unsigned long renderFlags;             /* +0x18 */
    Mesh_LightWave_Surface* lightWaveSurf; /* +0x1c */
    unsigned long numOfRefs;               /* +0x20 */
    Mesh* clonedFrom;                      /* +0x24 */
    LPDIRECT3DRMFRAME3 frameCreatedOn;     /* +0x28 */
    unsigned long flags;                   /* +0x2c */
    Mesh* nextFree;                        /* +0x30 */
};

/* A post-render entry (0x48 bytes). */
typedef struct Mesh_PostRenderInfo {
    Mesh* mesh;                            /* +0x00 */
    Mesh_Matrix matWorld;                  /* +0x04 */
    struct Mesh_PostRenderInfo* next;      /* +0x44 */
} Mesh_PostRenderInfo;

/* A texture stage state's value before the meshes changed it. */
typedef struct Mesh_TextureStateChangeData {
    unsigned long origValue;
    int changed;
} Mesh_TextureStateChangeData;

typedef struct Mesh_Globs {
    unsigned int unknown_0000;   /* 005343c0: never read; the original's state is 4-aligned at 005343c4 */
    Mesh_PostRenderInfo* postRenderList;                             /* +0x0000 */
    Mesh_TextureStateChangeData stateData[MESH_MAXTEXTURESTAGESTATES]; /* +0x0004 */
    char* sharedTextureDir;                                          /* +0x0194 */
    Mesh_TextureReference textureList[MESH_TEXTURELISTSIZE];         /* +0x0198 */
    Mesh_TextureReference textureListShared[MESH_TEXTURELISTSIZE];   /* +0x6198 */
    unsigned long textureCount;                                      /* +0xc198 */
    unsigned long textureCountShared;                                /* +0xc19c */
    DWORD oldTextureRM;                                              /* +0xc1a0 */
    DWORD oldMatIM;                                                  /* +0xc1a4 */
    LPDIRECT3DTEXTURE2 oldTextureIM;                                 /* +0xc1a8 */
    DWORD currTextureRM;                                             /* +0xc1ac */
    DWORD currMatIM;                                                 /* +0xc1b0 */
    LPDIRECT3DTEXTURE2 currTextureIM;                                /* +0xc1b4 */
    D3DMATERIALHANDLE matHandle;                                     /* +0xc1b8 */
    LPDIRECT3DMATERIAL3 mat;                                         /* +0xc1bc */
    Mesh* listSet[MESH_MAXLISTS];                                    /* +0xc1c0 */
    Mesh* freeList;                                                  /* +0xc210 */
    unsigned long listCount;                                         /* +0xc214 */
} Mesh_Globs;

#define lpD3DRM() ((LPDIRECT3DRM3)s_main_d3drm)
#define lpIMDevice() ((LPDIRECT3DDEVICE3)s_main_im_device)

/* External, as the original's: with a static struct MSVC 5.0 keeps the
 * tested free-list head in a register across mesh_add_list, where the
 * original reloads it (Mesh_ObtainFromList; findings 669 and 677). The zero
 * initialiser keeps it in this object's .bss rather than the common region
 * (finding 554). */
// GLOBAL: LEGORR 0x005343c0
Mesh_Globs meshGlobs = { 0 };

static int mesh_render_callback(LPDIRECT3DRMUSERVISUAL lpD3DRMUV,
                                void* lpArg,
                                D3DRMUSERVISUALREASON lpD3DRMUVreason,
                                IDirect3DRMDevice* lpD3DRMDev,
                                IDirect3DRMViewport* lpView);

// FUNCTION: LEGORR 0x004805b0
int mesh_initialise(const char* sharedTextureDir)
{
    unsigned long loop;

    meshGlobs.postRenderList = NULL;
    mesh_clear_post_render_list();

    if (sharedTextureDir) {
        unsigned long len = strlen(sharedTextureDir);
        if (sharedTextureDir[len - 1] != '\\') len++;
        meshGlobs.sharedTextureDir = (char*)malloc(len + 1);
        strcpy(meshGlobs.sharedTextureDir, sharedTextureDir);
        meshGlobs.sharedTextureDir[len - 1] = '\\';
        meshGlobs.sharedTextureDir[len] = '\0';
    }

    for (loop = 0; loop < MESH_MAXLISTS; loop++) {
        meshGlobs.listSet[loop] = NULL;
    }
    meshGlobs.freeList = NULL;
    meshGlobs.listCount = 0;

    mesh_create_global_material();

    return 1;
}

// FUNCTION: LEGORR 0x00480650
int mesh_create_global_material(void)
{
    LPDIRECT3D3 d3d;
    D3DMATERIAL material = {
        sizeof(D3DMATERIAL),
        { 1.0f, 1.0f, 1.0f, 1.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        0.0f, 0, 0
    };

    lpIMDevice()->GetDirect3D(&d3d);

    if (d3d->CreateMaterial(&meshGlobs.mat, NULL) != D3D_OK) {
        if (d3d) d3d->Release();
        return 0;
    }
    if (meshGlobs.mat->GetHandle(lpIMDevice(), &meshGlobs.matHandle) != D3D_OK) {
        if (d3d) d3d->Release();
        return 0;
    }

    mesh_set_material(&material);
    if (d3d) d3d->Release();

    return 1;

}

// FUNCTION: LEGORR 0x00480780
int mesh_set_material(LPD3DMATERIAL newMaterial)
{
    return meshGlobs.mat->SetMaterial(newMaterial) == D3D_OK;
}

// FUNCTION: LEGORR 0x004807a0
Mesh* mesh_obtain_from_list(void)
{
    Mesh* newMesh;

    if (meshGlobs.freeList == NULL) mesh_add_list();

    newMesh = meshGlobs.freeList;
    meshGlobs.freeList = newMesh->nextFree;
    memset(newMesh, 0, sizeof(Mesh));
    newMesh->nextFree = newMesh;

    return newMesh;
}

// FUNCTION: LEGORR 0x004807d0
void mesh_return_to_list(Mesh* deadMesh)
{

    deadMesh->nextFree = meshGlobs.freeList;
    meshGlobs.freeList = deadMesh;
}

// FUNCTION: LEGORR 0x004807f0
void mesh_add_list(void)
{
    Mesh* list;
    unsigned long loop, count;

    count = 0x00000001 << meshGlobs.listCount;

    if (list = meshGlobs.listSet[meshGlobs.listCount] =
            (Mesh*)malloc(sizeof(Mesh) * count)) {

        meshGlobs.listCount++;

        for (loop = 1; loop < count; loop++) {
            list[loop - 1].nextFree = &list[loop];
        }
        list[count - 1].nextFree = meshGlobs.freeList;
        meshGlobs.freeList = list;
    }
}

// FUNCTION: LEGORR 0x00480870
Mesh* mesh_create_on_frame(LPDIRECT3DRMFRAME3 frame,
                           Mesh_RenderCallbackFunc renderFunc,
                           unsigned long renderFlags, void* data,
                           unsigned long type)
{
    Mesh* mesh = mesh_obtain_from_list();

    container_get_root();

    lpD3DRM()->CreateUserVisual(mesh_render_callback, mesh, &mesh->uv);
    frame->AddVisual(mesh->uv);
    mesh->uv->SetAppData(mesh);

    mesh->frameCreatedOn = frame;
    mesh->renderCallback = renderFunc;
    mesh->renderCallbackData = data;
    mesh->renderFlags = renderFlags;

    if (type == Mesh_Type_PostEffect) {
        mesh->flags |= MESH_FLAG_POSTEFFECT;
    } else if (type == Mesh_Type_LightWaveObject) {
        mesh->flags |= MESH_FLAG_TRANSFORM_PARENTPOS;
    }

    mesh->numOfRefs = 1;

    return mesh;
}

// FUNCTION: LEGORR 0x00480900
void* mesh_clone(void* srcMesh, void* frame)
{
    Mesh* newMesh = mesh_obtain_from_list();
    Mesh* mesh = (Mesh*)srcMesh;
    LPDIRECT3DRMFRAME3 lpFrame = (LPDIRECT3DRMFRAME3)frame;
    unsigned long loop;


    if (mesh->clonedFrom) mesh = mesh->clonedFrom;

    *newMesh = *mesh;
    newMesh->clonedFrom = mesh;
    newMesh->listSize = newMesh->groupCount;
    newMesh->groupList = (Mesh_Group*)malloc(sizeof(Mesh_Group) * newMesh->groupCount);
    memcpy(newMesh->groupList, mesh->groupList, sizeof(Mesh_Group) * newMesh->groupCount);

    for (loop = 0; loop < newMesh->groupCount; loop++) {
        mesh_create_group_material(newMesh, loop);
        mesh_set_group_material(newMesh, loop, &mesh->groupList[loop].material);
    }

    lpD3DRM()->CreateUserVisual(mesh_render_callback, newMesh, &newMesh->uv);
    lpFrame->AddVisual(newMesh->uv);
    newMesh->uv->SetAppData(newMesh);

    newMesh->frameCreatedOn = lpFrame;
    mesh->numOfRefs++;
    mesh->flags |= MESH_FLAG_HASBEENCLONED;
    newMesh->numOfRefs = 0;

    return newMesh;
}

// FUNCTION: LEGORR 0x004809e0
Mesh* mesh_load(const char* fname, LPDIRECT3DRMFRAME3 frame, int noTextures)
{
    APPOBJ* lightWaveObject;
    char path[260];
    char* s;
    char* t;
    Mesh* mesh;

    strcpy(path, fname);
    for (s = t = path; *s != '\0'; s++) {
        if (*s == '\\') t = s;
    }
    t[t != path] = '\0';

    if (lwo_app_obj_load(fname, &lightWaveObject, 0)) {
        mesh = mesh_create_on_frame(frame, NULL, MESH_RENDERFLAGS_LWONORM,
                                           NULL, Mesh_Type_LightWaveObject);
        mesh_parse_lwo(path, mesh, lightWaveObject, noTextures);
        lwo_app_obj_free(lightWaveObject);
        return mesh;
    }

    return NULL;
}

/* Mesh_ParseLWO follows Gods98's own shape (as OpenLRR's Mesh.cpp records
 * it): one surfArraySize/surfFDArraySize offset per face, aoVerts read as
 * floats at index * 3, and post-increments inside the face-data subscripts
 * (finding 661). Which loop counters are shared is part of the original's
 * register allocation: the vertex loop and the group loop share one, the
 * face-normal loop has its own. The group loop walks its arrays by pointer;
 * the original's indexed form (&surfVertices[loop * size]) compiles to the
 * same strength-reduced code but needs five fewer locals, which moves the
 * symbol counter for every later function in the unit (finding 538). */
// FUNCTION: LEGORR 0x00480ac0
int mesh_parse_lwo(const char* basePath, Mesh* mesh,
                   APPOBJ* lightWaveObject, int noTextures)
{
    const unsigned long faceDataQuad[] = { 0, 1, 2, 0, 2, 3 };
    /* Unused in the original too (OpenLRR keeps it as "UNUSED"); it emits
     * no code and counts one symbol (finding 661). */
    float textCoords[][2] = { { 0, 1 }, { 0, 0 }, { 1, 0 }, { 1, 1 } };
    Mesh_Vector3F* surfVertices;
    Mesh_Vector3F** surfVerticesNorm;
    unsigned long* surfVertexCount;
    unsigned long* surfVertexRef;
    unsigned long* surfFaceData;
    unsigned long* surfFaceDataCount;
    Mesh_Vector3F* faceNorm;
    Mesh_Vector3F* vertexNorm;
    Mesh_Point2F* vertexUVs = NULL;
    Mesh_Point2F* surfTextCoords;
    unsigned long size = (lightWaveObject->aoSize.lwVertCount * 2);
    unsigned long fDSize = (lightWaveObject->aoSize.lwPolyCount * 6);
    unsigned long surfArraySize, surfFDArraySize;
    unsigned long face, vertex, loop;
    unsigned long groupID, renderFlags;
    unsigned long* faceDataPtr;
    Mesh_Vector3F* surfVerticesPtr;
    Mesh_Vector3F** surfNormalsPtr;
    Mesh_Point2F* textCoordsPtr;
    unsigned long* surfVertexCountPtr;
    unsigned long vertexIndex;

    /* GET TEXTURE AND TEXTURE COORDINATES FOR VERTICES IF UVIEW FILE
     * EXISTS. Called first because it may alter the LightWave object. */
    if (lightWaveObject->aoFileUV) {
        vertexUVs = (Mesh_Point2F*)malloc(sizeof(Mesh_Point2F) * size * 20);
        mesh_uview_mesh(lightWaveObject, vertexUVs);
        size = (lightWaveObject->aoSize.lwVertCount * 2);
        surfTextCoords = (Mesh_Point2F*)malloc(sizeof(Mesh_Point2F) * lightWaveObject->aoSize.lwSurfaceCount * size);
    } else {
        surfTextCoords = (Mesh_Point2F*)malloc(sizeof(Mesh_Point2F) * size);
    }

    /* ALLOCATE MEMORY FOR OBJECT */
    surfArraySize = (lightWaveObject->aoSize.lwSurfaceCount * size);

    surfVertices = (Mesh_Vector3F*)malloc(sizeof(Mesh_Vector3F) * surfArraySize);
    surfVerticesNorm = (Mesh_Vector3F**)malloc(sizeof(Mesh_Vector3F*) * surfArraySize);
    surfVertexCount = (unsigned long*)malloc(sizeof(unsigned long) * lightWaveObject->aoSize.lwSurfaceCount);
    surfVertexRef = (unsigned long*)malloc(sizeof(unsigned long) * surfArraySize);
    surfFaceData = (unsigned long*)malloc(sizeof(unsigned long) * (lightWaveObject->aoSize.lwSurfaceCount * fDSize));
    surfFaceDataCount = (unsigned long*)malloc(sizeof(unsigned long) * lightWaveObject->aoSize.lwSurfaceCount);
    faceNorm = (Mesh_Vector3F*)malloc(sizeof(Mesh_Vector3F) * lightWaveObject->aoSize.lwPolyCount);
    vertexNorm = (Mesh_Vector3F*)malloc(sizeof(Mesh_Vector3F) * size);

    memset(surfVertexCount, 0, sizeof(unsigned long) * lightWaveObject->aoSize.lwSurfaceCount);
    memset(surfVertexRef, -1, sizeof(unsigned long) * surfArraySize);
    memset(surfFaceDataCount, 0, sizeof(unsigned long) * lightWaveObject->aoSize.lwSurfaceCount);
    memset(vertexNorm, 0, sizeof(Mesh_Vector3F) * size);

    mesh->lightWaveSurf = (Mesh_LightWave_Surface*)malloc(sizeof(Mesh_LightWave_Surface) * lightWaveObject->aoSize.lwSurfaceCount);
    memset(mesh->lightWaveSurf, 0, sizeof(Mesh_LightWave_Surface) * lightWaveObject->aoSize.lwSurfaceCount);

    /* STORE FACE NORMALS FOR GOURAUD SHADING - ASSUMES ALL POLYS ARE
     * PLANAR */
    for (face = 0; face < lightWaveObject->aoSize.lwPolyCount; face++) {
        Mesh_Vector3F pos[3];
        for (vertex = 0; vertex < 3; vertex++) {
            vertexIndex = lightWaveObject->aoPoly[face].plyData[vertex] * 3;

            pos[vertex].x = lightWaveObject->aoVerts[vertexIndex];
            pos[vertex].y = lightWaveObject->aoVerts[vertexIndex + 1];
            pos[vertex].z = lightWaveObject->aoVerts[vertexIndex + 2];
        }
        surface_triangle_normal_calculate((float*)&faceNorm[face], (float*)&pos[0],
                                          (float*)&pos[1], (float*)&pos[2]);
    }

    /* READ SURFACE INFORMATION */
    mesh_get_surf_info(basePath, lightWaveObject, mesh->lightWaveSurf, noTextures);

    /* READ VERTEX INFORMATION */
    for (loop = 0; loop < lightWaveObject->aoSize.lwPolyCount; loop++) {

        surfArraySize = lightWaveObject->aoPoly[loop].srfIndex * size;
        surfFDArraySize = lightWaveObject->aoPoly[loop].srfIndex * fDSize;

        if (lightWaveObject->aoPoly[loop].plyCount == 3) {
            for (vertex = 0; vertex < 3; vertex++) {
                vertexIndex = lightWaveObject->aoPoly[loop].plyData[vertex] * 3;

                if (surfVertexRef[surfArraySize + lightWaveObject->aoPoly[loop].plyData[vertex]] == -1) {

                    /* STORE NEW VERTEX IN SURFACE LIST */
                    surfVertices[surfArraySize + surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex]].x =
                        lightWaveObject->aoVerts[vertexIndex];
                    surfVertices[surfArraySize + surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex]].y =
                        lightWaveObject->aoVerts[vertexIndex + 1];
                    surfVertices[surfArraySize + surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex]].z =
                        lightWaveObject->aoVerts[vertexIndex + 2];

                    if (lightWaveObject->aoFileUV) {
                        surfTextCoords[surfArraySize + surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex]] =
                            vertexUVs[lightWaveObject->aoPoly[loop].plyData[vertex]];
                    }

                    /* ADD VERTEX FACE NORMAL TO CURRENT VERTEX NORMAL */
                    mesh_vector3d_add(&vertexNorm[lightWaveObject->aoPoly[loop].plyData[vertex]],
                                      &vertexNorm[lightWaveObject->aoPoly[loop].plyData[vertex]],
                                      &faceNorm[loop]);
                    mesh_vector3d_normalise(&vertexNorm[lightWaveObject->aoPoly[loop].plyData[vertex]]);
                    surfVerticesNorm[surfArraySize + surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex]] =
                        &vertexNorm[lightWaveObject->aoPoly[loop].plyData[vertex]];

                    /* STORE NEW VERTEX IN FACE DATA */
                    surfFaceData[surfFDArraySize + surfFaceDataCount[lightWaveObject->aoPoly[loop].srfIndex]++] =
                        surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex];

                    /* STORE NEW VERTEX REFERENCE IN VERTEX LIST */
                    surfVertexRef[surfArraySize + lightWaveObject->aoPoly[loop].plyData[vertex]] =
                        surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex]++;
                } else {
                    /* ADD VERTEX FACE NORMAL TO CURRENT VERTEX NORMAL */
                    mesh_vector3d_add(&vertexNorm[lightWaveObject->aoPoly[loop].plyData[vertex]],
                                      &vertexNorm[lightWaveObject->aoPoly[loop].plyData[vertex]],
                                      &faceNorm[loop]);
                    mesh_vector3d_normalise(&vertexNorm[lightWaveObject->aoPoly[loop].plyData[vertex]]);

                    /* GET VERTEX REFERENCE AND COPY TO FACE DATA */
                    surfFaceData[surfFDArraySize + surfFaceDataCount[lightWaveObject->aoPoly[loop].srfIndex]++] =
                        surfVertexRef[surfArraySize + lightWaveObject->aoPoly[loop].plyData[vertex]];
                }
            }
        } else {
            surfArraySize = lightWaveObject->aoPoly[loop].srfIndex * size;

            for (vertex = 0; vertex < 6; vertex++) {

                if (surfVertexRef[surfArraySize + lightWaveObject->aoPoly[loop].plyData[faceDataQuad[vertex]]] == -1) {

                    vertexIndex = lightWaveObject->aoPoly[loop].plyData[faceDataQuad[vertex]] * 3;

                    /* STORE NEW VERTEX IN SURFACE LIST */
                    surfVertices[surfArraySize + surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex]].x =
                        lightWaveObject->aoVerts[vertexIndex];
                    surfVertices[surfArraySize + surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex]].y =
                        lightWaveObject->aoVerts[vertexIndex + 1];
                    surfVertices[surfArraySize + surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex]].z =
                        lightWaveObject->aoVerts[vertexIndex + 2];

                    if (lightWaveObject->aoFileUV) {
                        surfTextCoords[surfArraySize + surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex]] =
                            vertexUVs[lightWaveObject->aoPoly[loop].plyData[faceDataQuad[vertex]]];
                    }

                    /* ADD VERTEX FACE NORMAL TO CURRENT VERTEX NORMAL */
                    mesh_vector3d_add(&vertexNorm[lightWaveObject->aoPoly[loop].plyData[faceDataQuad[vertex]]],
                                      &vertexNorm[lightWaveObject->aoPoly[loop].plyData[faceDataQuad[vertex]]],
                                      &faceNorm[loop]);
                    mesh_vector3d_normalise(&vertexNorm[lightWaveObject->aoPoly[loop].plyData[faceDataQuad[vertex]]]);
                    surfVerticesNorm[surfArraySize + surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex]] =
                        &vertexNorm[lightWaveObject->aoPoly[loop].plyData[faceDataQuad[vertex]]];

                    /* STORE NEW VERTEX IN FACE DATA */
                    surfFaceData[surfFDArraySize + surfFaceDataCount[lightWaveObject->aoPoly[loop].srfIndex]++] =
                        surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex];

                    /* STORE NEW VERTEX REFERENCE IN VERTEX LIST */
                    surfVertexRef[surfArraySize + lightWaveObject->aoPoly[loop].plyData[faceDataQuad[vertex]]] =
                        surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex]++;
                } else {
                    /* ADD VERTEX FACE NORMAL TO CURRENT VERTEX NORMAL */
                    mesh_vector3d_add(&vertexNorm[lightWaveObject->aoPoly[loop].plyData[faceDataQuad[vertex]]],
                                      &vertexNorm[lightWaveObject->aoPoly[loop].plyData[faceDataQuad[vertex]]],
                                      &faceNorm[loop]);
                    mesh_vector3d_normalise(&vertexNorm[lightWaveObject->aoPoly[loop].plyData[faceDataQuad[vertex]]]);

                    /* GET VERTEX REFERENCE AND COPY TO FACE DATA */
                    surfFaceData[surfFDArraySize + surfFaceDataCount[lightWaveObject->aoPoly[loop].srfIndex]++] =
                        surfVertexRef[surfArraySize + lightWaveObject->aoPoly[loop].plyData[faceDataQuad[vertex]]];
                }
            }
        }
    }

    surfNormalsPtr = surfVerticesNorm;
    surfVertexCountPtr = surfVertexCount;
    textCoordsPtr = surfTextCoords;
    faceDataPtr = surfFaceData;
    surfVerticesPtr = surfVertices;
    for (loop = 0; loop < lightWaveObject->aoSize.lwSurfaceCount; loop++) {
        Mesh_LightWave_Surface* lwSurf;

        groupID = mesh_add_group(mesh, surfFaceDataCount[loop], surfFaceDataCount[loop] / 3, 3, faceDataPtr);

        if (lightWaveObject->aoFileUV) {
            mesh_set_vertices_vnt(mesh, groupID, 0, *surfVertexCountPtr, surfVerticesPtr, surfNormalsPtr, textCoordsPtr);
        } else {
            LWSURFACE* lwo = &lightWaveObject->aoSurface[loop];
            mesh_get_texture_uvs_wrap(*surfVertexCountPtr, surfVerticesPtr, surfTextCoords,
                                      lwo->srfTSIZ[0], lwo->srfTSIZ[1], lwo->srfTSIZ[2],
                                      lwo->srfTCTR[0], lwo->srfTCTR[1], lwo->srfTCTR[2],
                                      lwo->srfTexFlags);
            mesh_set_vertices_vnt(mesh, groupID, 0, *surfVertexCountPtr, surfVerticesPtr, surfNormalsPtr, surfTextCoords);
        }

        lwSurf = &mesh->lightWaveSurf[loop];
        if (lwSurf->flags & MESH_SFLAG_ADDITIVE) {
            renderFlags = MESH_RENDERFLAGS_ADDITIVE;
            mesh_alter_group_render_flags(mesh, groupID, renderFlags);
        } else {
            renderFlags = MESH_RENDERFLAGS_LWONORM;
        }
        if (mesh->lightWaveSurf[loop].flags & MESH_SFLAG_DOUBLESIDED) {
            renderFlags |= 0x04000000;
            mesh_alter_group_render_flags(mesh, groupID, renderFlags);
        }
        if (!(mesh->lightWaveSurf[loop].texFlags & MESH_TFLAG_PIXELBLENDING)) {
            renderFlags |= 0x00800000;
            mesh_alter_group_render_flags(mesh, groupID, renderFlags);
        }

        if (lightWaveObject->aoSurface[loop].srfTexFlags & MESH_TFLAG_SEQUENCE) {
            if (mesh->lightWaveSurf[loop].textureSeq) {
                mesh_set_group_texture(mesh, groupID, mesh->lightWaveSurf[loop].textureSeq[0]);
            }
        } else if (mesh->lightWaveSurf[loop].texture) {
            mesh_set_group_texture(mesh, groupID, mesh->lightWaveSurf[loop].texture);
        }

        mesh_set_group_colour(mesh, groupID, mesh->lightWaveSurf[loop].colour.r * mesh->lightWaveSurf[loop].diffuse,
                              mesh->lightWaveSurf[loop].colour.g * mesh->lightWaveSurf[loop].diffuse,
                              mesh->lightWaveSurf[loop].colour.b * mesh->lightWaveSurf[loop].diffuse, Mesh_Colour_Diffuse);
        mesh_set_group_colour(mesh, groupID, mesh->lightWaveSurf[loop].colour.r * mesh->lightWaveSurf[loop].emissive,
                              mesh->lightWaveSurf[loop].colour.g * mesh->lightWaveSurf[loop].emissive,
                              mesh->lightWaveSurf[loop].colour.b * mesh->lightWaveSurf[loop].emissive, Mesh_Colour_Emissive);
        if (mesh->lightWaveSurf[loop].flags & MESH_SFLAG_COLOURHIGHLIGHT) {
            mesh_set_group_colour(mesh, groupID, mesh->lightWaveSurf[loop].colour.r * mesh->lightWaveSurf[loop].specular,
                                  mesh->lightWaveSurf[loop].colour.g * mesh->lightWaveSurf[loop].specular,
                                  mesh->lightWaveSurf[loop].colour.b * mesh->lightWaveSurf[loop].specular, Mesh_Colour_Specular);
        } else {
            mesh_set_group_colour(mesh, groupID, mesh->lightWaveSurf[loop].specular, mesh->lightWaveSurf[loop].specular,
                                  mesh->lightWaveSurf[loop].specular, Mesh_Colour_Specular);
        }
        mesh_set_group_material_values(mesh, groupID, mesh->lightWaveSurf[loop].power * 10.0f, Mesh_Colour_Power);
        mesh_set_group_material_values(mesh, groupID, 1.0f - mesh->lightWaveSurf[loop].transparency, Mesh_Colour_Alpha);

        mesh->groupList[groupID].lightWaveSurfaceInfo = &mesh->lightWaveSurf[loop];

        textCoordsPtr += size;
        surfNormalsPtr += size;
        faceDataPtr += fDSize;
        surfVerticesPtr += size;
        surfVertexCountPtr++;
    }

    free(surfVertices);
    free(surfVerticesNorm);
    free(surfTextCoords);
    free(surfVertexCount);
    free(surfVertexRef);
    free(surfFaceData);
    free(surfFaceDataCount);
    free(faceNorm);
    free(vertexNorm);
    if (vertexUVs) free(vertexUVs);

    return 1;
}

// FUNCTION: LEGORR 0x00481820
void mesh_get_surf_info(const char* basePath, APPOBJ* lightWaveObject,
                        Mesh_LightWave_Surface* lightWaveSurf,
                        int noTextures)
{
    unsigned long loop, count;
    unsigned long texNum, tnumlen;
    char path[260], drive[4], dir[256], fname[256], ext[256];
    char baseName[256], textName[256];

    for (loop = 0; loop < lightWaveObject->aoSize.lwSurfaceCount; loop++) {
        if (lightWaveObject->aoSurface[loop].srfPath && !noTextures) {
            _splitpath(lightWaveObject->aoSurface[loop].srfPath, drive, dir, fname, ext);

            if (lightWaveObject->aoSurface[loop].srfTexFlags & MESH_TFLAG_SEQUENCE) {
                if (mesh_get_texture_seq_info(fname, baseName, &texNum, &tnumlen)) {
                    /* Gods98's tempNum: only its commented-out search used
                     * it. It emits no code and counts one symbol (finding
                     * 661). */
                    unsigned long tempNum = texNum;
                    lightWaveSurf[loop].textureSeq = (Mesh_Texture**)malloc(sizeof(Mesh_Texture*) * MESH_MAXTEXTURESEQUENCE);
                    for (count = 0; count < MESH_MAXTEXTURESEQUENCE; count++) {
                        mesh_get_next_in_sequence(baseName, textName, &texNum, tnumlen);
                        sprintf(path, "%s%s", textName, ext);
                        lightWaveSurf[loop].textureSeq[count] = (Mesh_Texture*)mesh_load_texture(basePath, path, NULL, NULL);
                        if (lightWaveSurf[loop].textureSeq[count] == NULL) break;
                    }
                    lightWaveSurf[loop].numInTexSeq = count;
                    lightWaveSurf[loop].textureSeq = (Mesh_Texture**)realloc(lightWaveSurf[loop].textureSeq, sizeof(Mesh_Texture*) * count);
                }
            } else {
                sprintf(path, "%s%s", fname, ext);
                lightWaveSurf[loop].texture = (Mesh_Texture*)mesh_load_texture(basePath, path, NULL, NULL);
            }

            if (lightWaveObject->aoSurface[loop].srfFlags & MESH_SFLAG_PIXELBLENDING) {
                lightWaveSurf[loop].colour.r = lightWaveObject->aoSurface[loop].srfCol[0] / 256.0f;
                lightWaveSurf[loop].colour.g = lightWaveObject->aoSurface[loop].srfCol[1] / 256.0f;
                lightWaveSurf[loop].colour.b = lightWaveObject->aoSurface[loop].srfCol[2] / 256.0f;
            } else {
                lightWaveSurf[loop].colour.r = 1.0f;
                lightWaveSurf[loop].colour.g = 1.0f;
                lightWaveSurf[loop].colour.b = 1.0f;
            }
        } else {
            lightWaveSurf[loop].colour.r = lightWaveObject->aoSurface[loop].srfCol[0] / 256.0f;
            lightWaveSurf[loop].colour.g = lightWaveObject->aoSurface[loop].srfCol[1] / 256.0f;
            lightWaveSurf[loop].colour.b = lightWaveObject->aoSurface[loop].srfCol[2] / 256.0f;
        }

        lightWaveSurf[loop].emissive = lightWaveObject->aoSurface[loop].srfLuminous;
        lightWaveSurf[loop].transparency = lightWaveObject->aoSurface[loop].srfTransparent;
        lightWaveSurf[loop].diffuse = lightWaveObject->aoSurface[loop].srfDiffuse;
        lightWaveSurf[loop].specular = lightWaveObject->aoSurface[loop].srfSpecular;
        lightWaveSurf[loop].power = lightWaveObject->aoSurface[loop].srfGlossy;
        lightWaveSurf[loop].flags = lightWaveObject->aoSurface[loop].srfFlags;
        lightWaveSurf[loop].texFlags = lightWaveObject->aoSurface[loop].srfTexFlags;
    }
}

// FUNCTION: LEGORR 0x00481ac0
int mesh_get_texture_seq_info(const char* tname, char* tfname,
                              unsigned long* tstart, unsigned long* tnumlen)
{
    unsigned char c;
    unsigned long n = 1, k, num = 0;
    unsigned long tnamelen = strlen(tname);

    c = tname[tnamelen - 1];
    if (c >= '0' && c <= '9') {
        for (k = tnamelen - 1; k > 0; k--) {
            c = tname[k];
            if (c < '0' || c > '9') break;
            num += (c - '0') * n;
            n *= 10;
        }
        if (k) {
            strcpy(tfname, tname);
            tfname[k + 1] = '\0';
            *tnumlen = tnamelen - k - 1;
            *tstart = num;
        } else return 0;
    } else return 0;

    /* The original's success path has no return statement: it ends with
     * the tnumlen pointer in eax (finding 677). */
}

// FUNCTION: LEGORR 0x00481b80
void mesh_get_next_in_sequence(const char* baseName, char* nextTextName,
                               unsigned long* texNum, unsigned long tnumlen)
{
    char tempName[16];
    unsigned long loop;

    strcpy(nextTextName, baseName);
    sprintf(tempName, "%d", *texNum);
    loop = tnumlen - strlen(tempName);
    while (loop >= 1) {
        // STRING: LEGORR 0x004ab108
        strcat(nextTextName, "0");
        loop--;
    }

    (*texNum)++;
    strcat(nextTextName, tempName);
}

// FUNCTION: LEGORR 0x00481c50
void mesh_uview_mesh(APPOBJ* lightWaveObject, Mesh_Point2F* textCoords)
{
    char line[1024], lineSplit[1024];
    char* argv[5];
    unsigned long* vertexUsed;
    long poly = -2;
    long lineCount = 0;
    struct GameFileHandle* uvFile = (struct GameFileHandle*)lightWaveObject->aoFileUV;
    long surface = -1;
    long polyIndex;
    long polyVert = -1;
    unsigned long newVertCount = 0;

    vertexUsed = (unsigned long*)malloc(sizeof(unsigned long) * lightWaveObject->aoSize.lwVertCount * 2);
    memset(vertexUsed, 0, sizeof(unsigned long) * lightWaveObject->aoSize.lwVertCount * 2);

    while (file_get_line(line, 1024, uvFile)) {
        strcpy(lineSplit, line);
        if (whitespace_tokenize(lineSplit, argv)) {
            if (lineCount == 2) surface = 0;

            if (surface != -1) {
                if (surface >= (long)lightWaveObject->aoSize.lwSurfaceCount * 2) {
                    surface = -1;
                    poly = -1;
                } else {
                    if (surface >= (long)lightWaveObject->aoSize.lwSurfaceCount) {
                        lightWaveObject->aoSurface[surface - lightWaveObject->aoSize.lwSurfaceCount].srfPath = string_duplicate(argv[0]);
                    }
                    surface++;
                }
            }

            if (poly != -2) {
                if (poly == -1) {
                    poly = 0;
                } else {
                    if (poly >= (long)lightWaveObject->aoSize.lwPolyCount) return;

                    if (polyVert == -1 || polyVert >= (long)lightWaveObject->aoPoly[polyIndex].plyCount) {
                        if (poly < (long)lightWaveObject->aoSize.lwPolyCount - 1) {
                            polyIndex = atoi(argv[0]);
                            if (polyVert != -1) poly++;
                            polyVert = 0;
                        }
                    } else {
                        int changed = 0;
                        float u, v;

                        u = (float)atof(argv[0]);
                        v = (float)-atof(argv[1]);

                        if (textCoords[lightWaveObject->aoPoly[polyIndex].plyData[polyVert]].x != u ||
                            textCoords[lightWaveObject->aoPoly[polyIndex].plyData[polyVert]].y != v) changed = 1;

                        if (!vertexUsed[lightWaveObject->aoPoly[polyIndex].plyData[polyVert]]) {
                            textCoords[lightWaveObject->aoPoly[polyIndex].plyData[polyVert]].x = u;
                            textCoords[lightWaveObject->aoPoly[polyIndex].plyData[polyVert]].y = v;
                            vertexUsed[lightWaveObject->aoPoly[polyIndex].plyData[polyVert++]] = 1;
                        } else {
                            if (changed) {
                                if (newVertCount == 0) {
                                    lightWaveObject->aoVerts = (float*)realloc(lightWaveObject->aoVerts, sizeof(Mesh_Vector3F) * (lightWaveObject->aoSize.lwVertCount + 11));
                                }
                                {
                                    unsigned long index = lightWaveObject->aoPoly[polyIndex].plyData[polyVert];
                                    unsigned long newIndex = lightWaveObject->aoSize.lwVertCount;
                                    lightWaveObject->aoVerts[(newIndex * 3) + 0] = lightWaveObject->aoVerts[(index * 3) + 0];
                                    lightWaveObject->aoVerts[(newIndex * 3) + 1] = lightWaveObject->aoVerts[(index * 3) + 1];
                                    lightWaveObject->aoVerts[(newIndex * 3) + 2] = lightWaveObject->aoVerts[(index * 3) + 2];
                                }
                                lightWaveObject->aoPoly[polyIndex].plyData[polyVert] = (unsigned short)lightWaveObject->aoSize.lwVertCount;
                                textCoords[lightWaveObject->aoPoly[polyIndex].plyData[polyVert]].x = u;
                                textCoords[lightWaveObject->aoPoly[polyIndex].plyData[polyVert]].y = v;
                                lightWaveObject->aoSize.lwVertCount++;
                                newVertCount++;
                                if (newVertCount > 9) newVertCount = 0;
                            }
                            polyVert++;
                        }
                    }
                }
            }

            lineCount++;
        }
    }

    free(vertexUsed);
}

// FUNCTION: LEGORR 0x00481fa0
void mesh_get_texture_uvs_wrap(unsigned long vertexCount,
                               Mesh_Vector3F* vertices,
                               Mesh_Point2F* textCoords, float sx, float sy, float sz,
                               float px, float py, float pz,
                               unsigned long flags)
{
    unsigned long loop;
    float x, y, z;

    for (loop = 0; loop < vertexCount; loop++) {
        x = vertices[loop].x - px;
        y = vertices[loop].y - py;
        z = vertices[loop].z - pz;

        if (flags & 1) textCoords[loop].x = (z / sz) - -0.5f;
        else textCoords[loop].x = (x / sx) - -0.5f;

        if (flags & 2) textCoords[loop].y = (z / sz) - -0.5f;
        else textCoords[loop].y = (y / sy) - -0.5f;
    }
}

// FUNCTION: LEGORR 0x00482040
int mesh_set_texture_time2(Mesh* mesh, float frame)
{
    unsigned long groupID;
    long addFrame = (long)(frame - (float)fmod(frame, 1.0));

    if (mesh->flags & MESH_FLAG_TRANSFORM_PARENTPOS) {
        for (groupID = 0; groupID < mesh->groupCount; groupID++) {
            Mesh_Group* group = &mesh->groupList[groupID];
            if ((group->lightWaveSurfaceInfo->texFlags & MESH_TFLAG_SEQUENCE) && group->lightWaveSurfaceInfo->numInTexSeq) {
                long texNum = (group->lightWaveSurfaceInfo->texSeqOffset + addFrame) % (long)group->lightWaveSurfaceInfo->numInTexSeq;
                if (texNum < 0) texNum = 0;
                mesh_set_group_texture(mesh, groupID, group->lightWaveSurfaceInfo->textureSeq[texNum]);
            }
        }
        return 1;
    }

    return 0;
}

// FUNCTION: LEGORR 0x004820d0
void mesh_remove(void* meshPtr, void* frame)
{
    Mesh* mesh = (Mesh*)meshPtr;
    LPDIRECT3DRMFRAME3 lpFrame = (LPDIRECT3DRMFRAME3)frame;
    unsigned long groupID;

    lpFrame->DeleteVisual(mesh->uv);

    if (mesh->clonedFrom) {
        Mesh* clonedFrom = mesh->clonedFrom;
        if (mesh->uv) {
            mesh->uv->Release();
            mesh->uv = NULL;
        }
        free(mesh->groupList);
        mesh_return_to_list(mesh);
        mesh = clonedFrom;
    }

    if (--mesh->numOfRefs == 0) {
        if (mesh->uv) {
            mesh->uv->Release();
            mesh->uv = NULL;
        }
        for (groupID = 0; groupID < mesh->groupCount; groupID++) {
            Mesh_Group* group = &mesh->groupList[groupID];
            mesh_remove_group_texture(mesh, groupID);
            free(group->faceData);
            free(group->vertices);
        }
        if (mesh->lightWaveSurf) free(mesh->lightWaveSurf);
        free(mesh->groupList);
        mesh_return_to_list(mesh);
    }
}

// FUNCTION: LEGORR 0x004821a0
void mesh_get_group(Mesh* mesh, unsigned long groupID,
                    unsigned long* vertexCount, unsigned long* faceCount,
                    unsigned long* vPerFace, unsigned long* faceDataSize,
                    unsigned long* faceData)
{
    Mesh_Group* group = &mesh->groupList[groupID];
    unsigned long loop;

    if (vertexCount) *vertexCount = group->vertexCount;
    if (vPerFace) *vPerFace = 3;
    if (faceDataSize) *faceDataSize = group->faceDataSize;
    if (faceData) {
        for (loop = 0; loop < group->faceDataSize; loop++) {
            *faceData++ = group->faceData[loop];
        }
    }
}

// FUNCTION: LEGORR 0x00482210
unsigned long mesh_get_group_count(Mesh* mesh)
{

    return mesh->groupCount;
}

// FUNCTION: LEGORR 0x00482220
long mesh_add_group(Mesh* mesh, unsigned long vertexCount,
                    unsigned long faceCount, unsigned long vPerFace,
                    unsigned long* faceData)
{
    Mesh_Group* group;
    unsigned long loop, size;

    if (mesh->groupList == NULL) {
        mesh->listSize = MESH_DEFLISTSIZE;
        mesh->groupCount = 1;
        mesh->groupList = (Mesh_Group*)malloc(sizeof(Mesh_Group) * MESH_DEFLISTSIZE);
    } else if (mesh->groupCount++ == mesh->listSize) {
        Mesh_Group* newList;
        unsigned long newSize = (mesh->listSize * MESH_LISTINCREASE) / 100;
        if (newList = (Mesh_Group*)realloc(mesh->groupList, sizeof(Mesh_Group) * newSize)) {
            mesh->groupList = newList;
            mesh->listSize = newSize;
        } else return -1;
    }

    group = &mesh->groupList[mesh->groupCount - 1];
    memset(group, 0, sizeof(Mesh_Group));

    size = faceCount * vPerFace;
    group->faceData = (unsigned short*)malloc(sizeof(unsigned short) * size);
    group->faceDataSize = size;
    for (loop = 0; loop < group->faceDataSize; loop++) {
        group->faceData[loop] = (unsigned short)faceData[loop];
    }

    group->vertices = (Mesh_Vertex*)malloc(sizeof(Mesh_Vertex) * vertexCount);
    memset(group->vertices, 0, sizeof(Mesh_Vertex) * vertexCount);
    group->vertexCount = vertexCount;
    group->flags = 0;

    mesh_create_group_material(mesh, mesh->groupCount - 1);

    return mesh->groupCount - 1;
}

// FUNCTION: LEGORR 0x00482350
void mesh_alter_group_render_flags(Mesh* mesh, unsigned long groupID,
                                   unsigned long newFlags)
{
    Mesh_Group* group = &mesh->groupList[groupID];

    group->renderFlags = newFlags;
}

// FUNCTION: LEGORR 0x00482370
void mesh_scale(Mesh* mesh, float x, float y, float z)
{
    unsigned long groupID, loop;

    for (groupID = 0; groupID < mesh->groupCount; groupID++) {
        Mesh_Group* group = &mesh->groupList[groupID];
        for (loop = 0; loop < group->vertexCount; loop++) {
            group->vertices[loop].position.x *= x;
            group->vertices[loop].position.y *= y;
            group->vertices[loop].position.z *= z;
        }
    }
}

// FUNCTION: LEGORR 0x004823e0
void mesh_set_vertices(Mesh* mesh, unsigned long groupID,
                       unsigned long index, unsigned long count,
                       Mesh_ContVertex* vertices)
{
    Mesh_Group* group = &mesh->groupList[groupID];
    unsigned long loop;

    for (loop = 0; loop < count; loop++) {
        group->vertices[index + loop].position = vertices[loop].position;
        group->vertices[index + loop].normal = vertices[loop].normal;
        group->vertices[index + loop].tu = vertices[loop].tu;
        group->vertices[index + loop].tv = vertices[loop].tv;
    }
}

// FUNCTION: LEGORR 0x00482470
void mesh_get_vertices(Mesh* mesh, unsigned long groupID,
                       unsigned long index, unsigned long count,
                       Mesh_ContVertex* vertices)
{
    Mesh_Group* group = &mesh->groupList[groupID];
    unsigned long loop;

    for (loop = 0; loop < count; loop++) {
        vertices[loop].position = group->vertices[index + loop].position;
        vertices[loop].normal = group->vertices[index + loop].normal;
        vertices[loop].tu = group->vertices[index + loop].tu;
        vertices[loop].tv = group->vertices[index + loop].tv;
    }
}

// FUNCTION: LEGORR 0x00482500
void mesh_set_vertices_point_normal_at(Mesh* mesh, unsigned long groupID,
                                       unsigned long index,
                                       unsigned long count,
                                       Mesh_Vector3F* vertices,
                                       Mesh_Vector3F* position,
                                       Mesh_Point2F* textCoords)
{
    Mesh_Group* group = &mesh->groupList[groupID];
    unsigned long loop;

    for (loop = 0; loop < count; loop++) {
        group->vertices[index + loop].position.x = vertices[loop].x;
        group->vertices[index + loop].position.y = vertices[loop].y;
        group->vertices[index + loop].position.z = vertices[loop].z;
        mesh_vector3d_subtract(&group->vertices[index + loop].normal, position, &group->vertices[index + loop].position);
        mesh_vector3d_normalise(&group->vertices[index + loop].normal);
        group->vertices[index + loop].tu = textCoords[loop].x;
        group->vertices[index + loop].tv = textCoords[loop].y;
    }
}

// FUNCTION: LEGORR 0x00482620
void mesh_set_vertices_same_normal(Mesh* mesh, unsigned long groupID,
                                   unsigned long index, unsigned long count,
                                   Mesh_Vector3F* vertices,
                                   Mesh_Vector3F* normal,
                                   Mesh_Point2F* textCoords)
{
    Mesh_Group* group = &mesh->groupList[groupID];
    unsigned long loop;

    for (loop = 0; loop < count; loop++) {
        group->vertices[index + loop].position.x = vertices[loop].x;
        group->vertices[index + loop].position.y = vertices[loop].y;
        group->vertices[index + loop].position.z = vertices[loop].z;
        group->vertices[index + loop].normal = *normal;
        group->vertices[index + loop].tu = textCoords[loop].x;
        group->vertices[index + loop].tv = textCoords[loop].y;
    }
}

// FUNCTION: LEGORR 0x004826c0
void mesh_set_vertices_vnt(Mesh* mesh, unsigned long groupID,
                           unsigned long index, unsigned long count,
                           Mesh_Vector3F* vertices, Mesh_Vector3F** normal,
                           Mesh_Point2F* textCoords)
{
    Mesh_Group* group = &mesh->groupList[groupID];
    unsigned long loop;

    for (loop = 0; loop < count; loop++) {
        group->vertices[index + loop].position.x = vertices[loop].x;
        group->vertices[index + loop].position.y = vertices[loop].y;
        group->vertices[index + loop].position.z = vertices[loop].z;
        group->vertices[index + loop].normal.x = normal[loop]->x;
        group->vertices[index + loop].normal.y = normal[loop]->y;
        group->vertices[index + loop].normal.z = normal[loop]->z;
        group->vertices[index + loop].tu = textCoords[loop].x;
        group->vertices[index + loop].tv = textCoords[loop].y;
    }
}

// FUNCTION: LEGORR 0x00482780
int mesh_is_group_hidden(Mesh* mesh, unsigned long groupID)
{
    Mesh_Group* group = &mesh->groupList[groupID];

    return group->flags & MESH_GROUP_FLAG_HIDDEN;
}

// FUNCTION: LEGORR 0x004827a0
void mesh_hide_group(Mesh* mesh, unsigned long groupID, int hide)
{
    Mesh_Group* group = &mesh->groupList[groupID];

    if (hide) group->flags |= MESH_GROUP_FLAG_HIDDEN;
    else group->flags &= ~MESH_GROUP_FLAG_HIDDEN;
}

// FUNCTION: LEGORR 0x004827d0
void mesh_hide(Mesh* mesh, int hide)
{

    if (hide) mesh->flags |= MESH_FLAG_HIDDEN;
    else mesh->flags &= ~MESH_FLAG_HIDDEN;
}

// FUNCTION: LEGORR 0x004827f0
static int mesh_render_callback(LPDIRECT3DRMUSERVISUAL lpD3DRMUV,
                                void* lpArg,
                                D3DRMUSERVISUALREASON lpD3DRMUVreason,
                                IDirect3DRMDevice* lpD3DRMDev,
                                IDirect3DRMViewport* lpView)
{
    Mesh* mesh = (Mesh*)lpArg;
    int alphaFound, renderStatesStored;

    if (lpD3DRMUVreason == D3DRMUSERVISUAL_CANSEE) {
        if (lpIMDevice()) {
            Mesh_Container* cont;
            Mesh_Viewport* vp;

            if (mesh->flags & MESH_FLAG_HIDDEN) return 0;
            cont = (Mesh_Container*)container_frame_get_container(mesh->frameCreatedOn);
            if (cont && (cont->flags & MESH_CONTAINER_FLAG_HIDDEN)) return 0;
            if ((mesh->flags & MESH_FLAG_FACECAMERA) && !(mesh->flags & MESH_FLAG_FACECAMERADONE)) {
                vp = (Mesh_Viewport*)lpView->GetAppData();
                if (vp->rendering) {
                    LPDIRECT3DRMFRAME3 scene;
                    LPDIRECT3DRMFRAME3 camera;
                    mesh->frameCreatedOn->GetScene(&scene);
                    vp->lpVP->GetCamera(&camera);
                    mesh->frameCreatedOn->LookAt(camera, scene, D3DRMCONSTRAIN_Z);
                    mesh->flags |= MESH_FLAG_FACECAMERADONE;
                }
            }
            return 1;
        }
    } else if (lpD3DRMUVreason == D3DRMUSERVISUAL_RENDER) {
        if (lpIMDevice()) {
            Mesh_Matrix matWorld;
            LPDIRECT3DRMVIEWPORT2 view;
            Mesh_Viewport* vp;
            unsigned long groupID;
            Mesh_Group* group;

            alphaFound = 0;
            renderStatesStored = 0;
            mesh->flags &= ~MESH_FLAG_FACECAMERADONE;
            lpIMDevice()->GetTransform(D3DTRANSFORMSTATE_WORLD, (LPD3DMATRIX)&matWorld);
            mesh_set_current_viewport(lpView);
            lpView->QueryInterface(IID_IDirect3DRMViewport2, (void**)&view);
            vp = (Mesh_Viewport*)view->GetAppData();

            if (mesh->flags & MESH_FLAG_POSTEFFECT) {
                mesh_add_to_post_render_list(mesh, &matWorld);
            } else if (mesh->renderFlags & MESH_RENDERFLAGS_ALPHAMASK) {
                for (groupID = 0; groupID < mesh->groupCount; groupID++) {
                    group = &mesh->groupList[groupID];
                    if (mesh_can_render_group(group)) {
                        if (group->flags & MESH_GROUP_FLAG_ALPHAENABLE) alphaFound = 1;
                        else if (group->renderFlags == 0) {
                            if (!renderStatesStored) {
                                mesh_store_texture_and_mat();
                                mesh_set_mesh_render_desc(mesh, vp, &matWorld, 0);
                                renderStatesStored = 1;
                            }
                            mesh_render_group(mesh, group, &matWorld, 0);
                        }
                    }
                }
                for (groupID = 0; groupID < mesh->groupCount; groupID++) {
                    group = &mesh->groupList[groupID];
                    if (mesh_can_render_group(group)) {
                        if (group->flags & MESH_GROUP_FLAG_ALPHAENABLE) alphaFound = 1;
                        else if (group->renderFlags) {
                            if (!renderStatesStored) {
                                mesh_store_texture_and_mat();
                                mesh_set_mesh_render_desc(mesh, vp, &matWorld, 0);
                                renderStatesStored = 1;
                            }
                            mesh_render_group(mesh, group, &matWorld, 0);
                        }
                    }
                }
                if (alphaFound) mesh_add_to_post_render_list(mesh, &matWorld);
            } else {
                mesh_store_texture_and_mat();
                mesh_set_mesh_render_desc(mesh, vp, &matWorld, 0);
                renderStatesStored = 1;
                mesh_render_mesh(mesh, &matWorld, 0);
            }

            if (renderStatesStored) {
                main_restore_states();
                mesh_restore_texture_and_mat();
            }

            if (view) view->Release();
        }
        return 1;
    }

    return 0;
}

// FUNCTION: LEGORR 0x00482ac0
void mesh_set_mesh_render_desc(Mesh* mesh, Mesh_Viewport* vp,
                               Mesh_Matrix* matWorld, int alphaBlend)
{

    if (mesh->renderCallback) mesh->renderCallback(mesh, mesh->renderCallbackData, (Mesh_Viewport*)vp);

    main_change_render_state(29, 0);  /* D3DRENDERSTATE_SPECULARENABLE */
    main_change_render_state(9, 2);   /* D3DRENDERSTATE_SHADEMODE, GOURAUD */
    main_change_render_state(26, 1);  /* D3DRENDERSTATE_DITHERENABLE */

    if (!(s_engine_feature_word & 0x20)) {
        mesh_set_texture_stage_state(2, 2);
        mesh_set_texture_stage_state(3, 0);
        mesh_set_texture_stage_state(5, 2);
        mesh_set_texture_stage_state(6, 0);
    }

    mesh_set_render_desc(mesh->renderFlags, matWorld, alphaBlend);
}

// FUNCTION: LEGORR 0x00482b50
void mesh_set_render_desc(unsigned long flags, Mesh_Matrix* matWorld,
                          int alphaBlend)
{
    Mesh_Matrix mat;

    if ((flags & MESH_RENDERFLAGS_ALPHAMASK) && alphaBlend) {
        main_change_render_state(28, 0); /* D3DRENDERSTATE_ZWRITEENABLE */
    }

    if ((flags & MESH_RENDERFLAGS_ALPHAMASK) && !alphaBlend) {
        main_change_render_state(27, 0); /* D3DRENDERSTATE_ALPHABLENDENABLE */
    } else if (flags & MESH_RENDERFLAGS_ALPHATRANS) {
        mesh_set_alpha_render(2, 2);
    } else if (flags & MESH_RENDERFLAGS_ALPHASA1) {
        mesh_set_alpha_render(5, 2);
    } else if (flags & MESH_RENDERFLAGS_ALPHASA0) {
        mesh_set_alpha_render(5, 6);
    } else {
        main_change_render_state(27, 0);
    }

    if (flags & 0x04000000) main_change_render_state(22, 1); /* D3DRENDERSTATE_CULLMODE */
    else main_change_render_state(22, 3);

    if (!(s_engine_feature_word & 0x20)) {
        if (flags & 0x00020000) mesh_set_texture_stage_state(4, 2);
        else if (flags & 0x00040000) mesh_set_texture_stage_state(4, 3);
        else mesh_set_texture_stage_state(4, 4);
    }

    main_change_render_state(7, 1); /* D3DRENDERSTATE_ZENABLE */
    if ((flags & MESH_RENDERFLAGS_ALPHAMASK) && alphaBlend) main_change_render_state(14, 0);
    else main_change_render_state(14, 1);

    if (flags & MESH_RENDERFLAGS_LWOALPHA) {
        mesh_set_transform(1, matWorld);
    } else if (flags & 0x00004000) {
        mesh_set_identity_matrix(&mat);
        mesh_set_transform(1, &mat);
    }

    if (flags & 0x00800000) {
        main_change_render_state(17, 1);
        main_change_render_state(18, 1);
    } else {
        main_change_render_state(17, 2);
        main_change_render_state(18, 4);
    }
}

// FUNCTION: LEGORR 0x00482cb0
void mesh_set_alpha_render(unsigned long src, unsigned long dest)
{
    main_change_render_state(27, 1);
    main_change_render_state(19, src);
    main_change_render_state(20, dest);
}

// FUNCTION: LEGORR 0x00482ce0
void mesh_add_to_post_render_list(Mesh* mesh, Mesh_Matrix* matWorld)
{
    Mesh_PostRenderInfo* info = (Mesh_PostRenderInfo*)malloc(sizeof(Mesh_PostRenderInfo));

    memset(info, 0, sizeof(Mesh_PostRenderInfo));
    info->mesh = mesh;
    if (matWorld) info->matWorld = *matWorld;

    info->next = meshGlobs.postRenderList;
    meshGlobs.postRenderList = info;
}

// FUNCTION: LEGORR 0x00482d30
void mesh_clear_post_render_list(void)
{
    Mesh_PostRenderInfo* info = meshGlobs.postRenderList;
    Mesh_PostRenderInfo* next;

    while (info) {
        next = info->next;
        free(info);
        info = next;
    }

    meshGlobs.postRenderList = NULL;
}

// FUNCTION: LEGORR 0x00482d60
void mesh_post_render_all(Mesh_Viewport* vp)
{
    Mesh_PostRenderInfo* info = meshGlobs.postRenderList;
    unsigned long groupID;
    Mesh_Group* group;

    if (info) {
        mesh_set_current_gods_viewport(vp);
        mesh_store_texture_and_mat();

        do {
            if (!(info->mesh->flags & MESH_FLAG_HIDDEN)) {
                mesh_set_mesh_render_desc(info->mesh, vp, &info->matWorld, 1);

                if (info->mesh->flags & MESH_FLAG_POSTEFFECT) {
                    mesh_render_mesh(info->mesh, &info->matWorld, 1);
                } else {
                    for (groupID = 0; groupID < info->mesh->groupCount; groupID++) {
                        group = &info->mesh->groupList[groupID];
                        if (mesh_can_render_group(group) && (group->flags & MESH_GROUP_FLAG_ALPHAENABLE) && group->renderFlags == 0) {
                            mesh_render_group(info->mesh, group, &info->matWorld, 1);
                        }
                    }
                    for (groupID = 0; groupID < info->mesh->groupCount; groupID++) {
                        group = &info->mesh->groupList[groupID];
                        if (mesh_can_render_group(group) && (group->flags & MESH_GROUP_FLAG_ALPHAENABLE) && group->renderFlags) {
                            mesh_render_group(info->mesh, group, &info->matWorld, 1);
                        }
                    }
                }
            }
        } while (info = info->next);

        mesh_clear_post_render_list();
        main_restore_states();
        mesh_restore_texture_and_mat();
    }
}

// FUNCTION: LEGORR 0x00482e70
Mesh_Texture* mesh_load_texture(const char* baseDir, const char* name,
                        unsigned long* width, unsigned long* height)
{
    Mesh_Texture* texture = (Mesh_Texture*)malloc(sizeof(Mesh_Texture));
    LPDIRECTDRAWSURFACE4 surface;
    Mesh_TextureReference* ref;
    int trans;
    char path[260];

    if (baseDir) sprintf(path, "%s%s", baseDir, name);
    else strcpy(path, name);

    if (ref = (Mesh_TextureReference*)mesh_search_texture_list(meshGlobs.textureList, meshGlobs.textureCount, path)) {
        surface = ref->surface;
        if (surface) surface->AddRef();
        trans = ref->trans;
    } else if (surface = (LPDIRECTDRAWSURFACE4)container_load_texture_surface(path, 1, width, height, &trans)) {
        mesh_add_texture_reference(meshGlobs.textureList, &meshGlobs.textureCount, path, surface, trans);
    } else if (meshGlobs.sharedTextureDir) {
        sprintf(path, "%s%s", meshGlobs.sharedTextureDir, name);
        if (ref = (Mesh_TextureReference*)mesh_search_texture_list(meshGlobs.textureListShared, meshGlobs.textureCountShared, path)) {
            surface = ref->surface;
            if (surface) surface->AddRef();
            trans = ref->trans;
        } else if (surface = (LPDIRECTDRAWSURFACE4)container_load_texture_surface(path, 1, width, height, &trans)) {
            mesh_add_texture_reference(meshGlobs.textureListShared, &meshGlobs.textureCountShared, path, surface, trans);
        }
    }

    if (surface) {
        texture->texture = NULL;
        texture->surface = surface;
        texture->colourKey = trans;
    } else {
        free(texture);
        texture = NULL;
    }
    return texture;
}

// FUNCTION: LEGORR 0x00483030
Mesh_TextureReference* mesh_search_texture_list(
    Mesh_TextureReference* textureList, unsigned long count,
    const char* path)
{
    unsigned long loop;

    for (loop = 0; loop < count; loop++) {
        if (_strcmpi(path, textureList[loop].path) == 0) return &textureList[loop];
    }

    return NULL;
}

// FUNCTION: LEGORR 0x00483080
void mesh_add_texture_reference(Mesh_TextureReference* textureList,
                                unsigned long* count, const char* path,
                                LPDIRECTDRAWSURFACE4 surface, int trans)
{

    textureList[*count].path = string_duplicate(path);
    textureList[*count].surface = surface;
    textureList[*count].trans = trans;
    (*count)++;
}

// FUNCTION: LEGORR 0x004830c0
void mesh_set_group_texture(Mesh* mesh, unsigned long groupID,
                            Mesh_Texture* texture)
{

    if (texture) {
        if (texture->surface) {
            texture->surface->QueryInterface(IID_IDirect3DTexture2,
                                             (void**)&mesh->groupList[groupID].imText);
        }
        if (texture->colourKey) mesh->groupList[groupID].flags |= MESH_GROUP_FLAG_HASCOLOURKEY;
        else mesh->groupList[groupID].flags &= ~MESH_GROUP_FLAG_HASCOLOURKEY;
    }
}

// FUNCTION: LEGORR 0x00483140
void mesh_remove_group_texture(Mesh* mesh, unsigned long groupID)
{
    Mesh_Group* group = &mesh->groupList[groupID];

    if (group->imText) {
        group->imText->Release();
        group->imText = NULL;
    }
}

// FUNCTION: LEGORR 0x00483170
int mesh_create_group_material(Mesh* mesh, unsigned long groupID)
{
    D3DMATERIAL material = {
        sizeof(D3DMATERIAL),
        { 1.0f, 1.0f, 1.0f, 1.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        0.0f, 0, 0
    };

    mesh->groupList[groupID].material = material;

    return 1;
}

// FUNCTION: LEGORR 0x00483240
int mesh_set_group_material(Mesh* mesh, unsigned long groupID,
                            LPD3DMATERIAL material)
{

    mesh->groupList[groupID].material = *material;

    return 1;
}

// FUNCTION: LEGORR 0x00483270
int mesh_set_group_colour(Mesh* mesh, unsigned long groupID, float r,
                          float g, float b, unsigned long type)
{
    Mesh_Group* group = &mesh->groupList[groupID];
    LPD3DMATERIAL material = mesh_get_group_material(mesh, groupID);

    if (r < 0.0f) r = 0.0f;
    if (g < 0.0f) g = 0.0f;
    if (b < 0.0f) b = 0.0f;
    if (r > 1.0f) r = 1.0f;
    if (g > 1.0f) g = 1.0f;
    if (b > 1.0f) b = 1.0f;

    if (type == Mesh_Colour_Diffuse) {
        material->diffuse.r = r;
        material->diffuse.g = g;
        material->diffuse.b = b;
        if (r == 1.0f && g == 1.0f && b == 1.0f) group->flags |= MESH_GROUP_FLAG_COLOURNOTINTERPOLATED;
        else group->flags &= ~MESH_GROUP_FLAG_COLOURNOTINTERPOLATED;
    } else if (type == Mesh_Colour_Ambient) {
        material->ambient.r = r;
        material->ambient.g = g;
        material->ambient.b = b;
    } else if (type == Mesh_Colour_Emissive) {
        material->emissive.r = r;
        material->emissive.g = g;
        material->emissive.b = b;
    } else if (type == Mesh_Colour_Specular) {
        material->specular.r = r;
        material->specular.g = g;
        material->specular.b = b;
    }

    return 1;
}

// FUNCTION: LEGORR 0x00483400
LPD3DMATERIAL mesh_get_group_material(Mesh* mesh, unsigned long groupID)
{

    return &mesh->groupList[groupID].material;
}

// FUNCTION: LEGORR 0x00483420
int mesh_set_group_material_values(Mesh* mesh, unsigned long groupID,
                                   float value, unsigned long type)
{
    Mesh_Group* group = &mesh->groupList[groupID];
    LPD3DMATERIAL material = mesh_get_group_material(mesh, groupID);

    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;

    if (type == Mesh_Colour_Alpha) {
        group->flags &= ~(MESH_GROUP_FLAG_ALPHAENABLE | MESH_GROUP_FLAG_ALPHAHIDDEN);
        if (0.0f == value) {
            if (group->renderFlags) group->flags |= MESH_GROUP_FLAG_ALPHAHIDDEN;
            else if (mesh->renderFlags & MESH_RENDERFLAGS_ALPHAMASK) group->flags |= MESH_GROUP_FLAG_ALPHAHIDDEN;
        } else if (1.0f != value) {
            if (group->renderFlags || (mesh->renderFlags & MESH_RENDERFLAGS_ALPHAMASK)) {
                group->flags |= MESH_GROUP_FLAG_ALPHAENABLE;
            }
        }
        if (group->renderFlags) {
            if (group->renderFlags & (MESH_RENDERFLAGS_ALPHATRANS | MESH_RENDERFLAGS_ALPHASA1)) group->flags |= MESH_GROUP_FLAG_ALPHAENABLE;
        } else if (mesh->renderFlags & (MESH_RENDERFLAGS_ALPHATRANS | MESH_RENDERFLAGS_ALPHASA1)) {
            group->flags |= MESH_GROUP_FLAG_ALPHAENABLE;
        }
        material->diffuse.a = value;
    } else if (type == Mesh_Colour_Power) {
        material->power = value;
    }

    return 1;
}

// FUNCTION: LEGORR 0x00483540
void mesh_set_identity_matrix(Mesh_Matrix* m)
{

    m->m[2][0] = m->m[2][1] = m->m[2][3] = m->m[3][0] = m->m[3][1] = m->m[3][2] =
        m->m[0][1] = m->m[0][2] = m->m[0][3] = m->m[1][0] = m->m[1][2] = m->m[1][3] = 0.0f;
    m->m[0][0] = m->m[1][1] = m->m[2][2] = m->m[3][3] = 1.0f;
}

// FUNCTION: LEGORR 0x00483580
int mesh_set_current_viewport(LPDIRECT3DRMVIEWPORT lpView)
{
    LPDIRECT3DVIEWPORT imview1;
    LPDIRECT3DVIEWPORT3 imview;

    if (lpView->GetDirect3DViewport(&imview1) != D3DRM_OK) return 0;

    if (imview1->QueryInterface(IID_IDirect3DViewport3, (void**)&imview) != DD_OK) {
        if (imview1) imview1->Release();
        return 0;
    }
    if (imview1) {
        imview1->Release();
        imview1 = NULL;
    }
    if (lpIMDevice()->SetCurrentViewport(imview) != D3D_OK) {
        if (imview) imview->Release();
        return 0;
    }
    if (imview) imview->Release();

    return 1;
}

// FUNCTION: LEGORR 0x00483600
int mesh_set_current_gods_viewport(Mesh_Viewport* vp)
{
    LPDIRECT3DVIEWPORT imview1;
    LPDIRECT3DVIEWPORT3 imview;

    if (vp->lpVP->GetDirect3DViewport(&imview1) != D3DRM_OK) return 0;

    if (imview1->QueryInterface(IID_IDirect3DViewport3, (void**)&imview) != DD_OK) {
        if (imview1) imview1->Release();
        return 0;
    }
    if (imview1) {
        imview1->Release();
        imview1 = NULL;
    }
    if (lpIMDevice()->SetCurrentViewport(imview) != D3D_OK) {
        if (imview) imview->Release();
        return 0;
    }
    if (imview) imview->Release();

    return 1;
}

// FUNCTION: LEGORR 0x00483690
int mesh_set_transform(unsigned long state, Mesh_Matrix* mat)
{
    Mesh_Matrix oldMatrix;
    Mesh_Matrix newMatrix = *mat;

    lpIMDevice()->GetTransform((D3DTRANSFORMSTATETYPE)state, (LPD3DMATRIX)&oldMatrix);

    if (oldMatrix.m[0][0] != newMatrix.m[0][0] || oldMatrix.m[0][1] != newMatrix.m[0][1] ||
        oldMatrix.m[0][2] != newMatrix.m[0][2] || oldMatrix.m[0][3] != newMatrix.m[0][3] ||
        oldMatrix.m[1][0] != newMatrix.m[1][0] || oldMatrix.m[1][1] != newMatrix.m[1][1] ||
        oldMatrix.m[1][2] != newMatrix.m[1][2] || oldMatrix.m[1][3] != newMatrix.m[1][3] ||
        oldMatrix.m[2][0] != newMatrix.m[2][0] || oldMatrix.m[2][1] != newMatrix.m[2][1] ||
        oldMatrix.m[2][2] != newMatrix.m[2][2] || oldMatrix.m[2][3] != newMatrix.m[2][3] ||
        oldMatrix.m[3][0] != newMatrix.m[3][0] || oldMatrix.m[3][1] != newMatrix.m[3][1] ||
        oldMatrix.m[3][2] != newMatrix.m[3][2] || oldMatrix.m[3][3] != newMatrix.m[3][3]) {
        if (lpIMDevice()->SetTransform((D3DTRANSFORMSTATETYPE)state, (LPD3DMATRIX)mat) != D3D_OK) return 0;
    }

    return 1;
}

// FUNCTION: LEGORR 0x00483810
int mesh_set_texture_stage_state(unsigned long type, unsigned long value)
{
    DWORD oldValue;

    if (lpIMDevice()->GetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)type, &oldValue) != D3D_OK) oldValue = -1;

    if (oldValue != value) {
        if (lpIMDevice()->SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)type, value) != D3D_OK) return 0;
        if (meshGlobs.stateData[type].changed) {
            if (meshGlobs.stateData[type].origValue == oldValue) meshGlobs.stateData[type].changed = 0;
        } else {
            meshGlobs.stateData[type].changed = 1;
            meshGlobs.stateData[type].origValue = oldValue;
        }
    }

    return 1;
}

// FUNCTION: LEGORR 0x004838b0
void mesh_store_texture_and_mat(void)
{
    lpIMDevice()->GetLightState(D3DLIGHTSTATE_MATERIAL, &meshGlobs.oldMatIM);
    meshGlobs.currMatIM = meshGlobs.oldMatIM;

    if (!(s_engine_feature_word & 0x20)) {
        lpIMDevice()->GetTexture(0, &meshGlobs.oldTextureIM);
        meshGlobs.currTextureIM = meshGlobs.oldTextureIM;
        lpIMDevice()->GetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, &meshGlobs.oldTextureRM);
        if (meshGlobs.oldTextureRM) lpIMDevice()->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, 0);
    } else {
        lpIMDevice()->GetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, &meshGlobs.oldTextureRM);
        meshGlobs.currTextureRM = meshGlobs.oldTextureRM;
    }
}

// FUNCTION: LEGORR 0x00483940
void mesh_restore_texture_and_mat(void)
{
    if (meshGlobs.currMatIM != meshGlobs.oldMatIM) {
        lpIMDevice()->SetLightState(D3DLIGHTSTATE_MATERIAL, meshGlobs.oldMatIM);
    }

    if (!(s_engine_feature_word & 0x20)) {
        if (meshGlobs.currTextureIM != meshGlobs.oldTextureIM) {
            lpIMDevice()->SetTexture(0, meshGlobs.oldTextureIM);
        }
        if (meshGlobs.oldTextureRM) lpIMDevice()->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, meshGlobs.oldTextureRM);
    } else {
        lpIMDevice()->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, meshGlobs.oldTextureRM);
    }
}

// FUNCTION: LEGORR 0x004839c0
int mesh_render_mesh(Mesh* mesh, Mesh_Matrix* matWorld, int alphaBlend)
{
    unsigned long groupID;
    Mesh_Group* group;
    int ok = 1;

    for (groupID = 0; groupID < mesh->groupCount; groupID++) {
        group = &mesh->groupList[groupID];
        if (mesh_can_render_group(group) && group->renderFlags == 0) {
            if (!mesh_render_group(mesh, group, matWorld, alphaBlend)) ok = 0;
        }
    }
    for (groupID = 0; groupID < mesh->groupCount; groupID++) {
        group = &mesh->groupList[groupID];
        if (mesh_can_render_group(group) && group->renderFlags) {
            if (!mesh_render_group(mesh, group, matWorld, alphaBlend)) ok = 0;
        }
    }

    return ok;
}

// FUNCTION: LEGORR 0x00483a70
int mesh_can_render_group(Mesh_Group* group)
{

    if (!(group->flags & MESH_GROUP_FLAG_HIDDEN) && !(group->flags & MESH_GROUP_FLAG_ALPHAHIDDEN)) return 1;

    return 0;
}

// FUNCTION: LEGORR 0x00483a90
int mesh_render_group(Mesh* mesh, Mesh_Group* group, Mesh_Matrix* matWorld,
                      int alphaBlend)
{
    int ok = 1;

    mesh_set_group_render_desc(mesh, group, matWorld, alphaBlend);
    if (!mesh_set_material(&group->material)) ok = 0;
    if (!mesh_render_triangle_list(meshGlobs.matHandle, group->imText, 0x112,
                                   group->vertices, group->vertexCount,
                                   group->faceData, group->faceDataSize)) ok = 0;

    return ok;
}

// FUNCTION: LEGORR 0x00483b00
int mesh_set_group_render_desc(Mesh* mesh, Mesh_Group* group,
                               Mesh_Matrix* matWorld, int alphaBlend)
{

    if (!(s_engine_feature_word & 0x20)) {
        if ((group->flags & MESH_GROUP_FLAG_TEXTURECOLOURONLY) && group->imText) {
            mesh_set_texture_stage_state(1, 2);
        } else {
            mesh_set_texture_stage_state(1, 4);
        }
    }

    if (group->flags & MESH_GROUP_FLAG_HASCOLOURKEY) main_change_render_state(41, 1);
    else main_change_render_state(41, 0);

    if (group->renderFlags) {
        mesh_set_render_desc(group->renderFlags, matWorld, alphaBlend);
        return 1;
    }

    return 0;
}

// FUNCTION: LEGORR 0x00483b70
int mesh_render_triangle_list(unsigned long matHandle, LPDIRECT3DTEXTURE2 texture,
                              unsigned long renderFlags, void* vertices,
                              unsigned long vertexCount,
                              unsigned short* faceData,
                              unsigned long indexCount)
{
    LPDIRECT3DDEVICE2 dev2;
    int ok = 1;

    if (!(s_engine_feature_word & 0x20)) {
        if (texture != meshGlobs.currTextureIM) {
            if (lpIMDevice()->SetTexture(0, texture) != D3D_OK) ok = 0;
            else meshGlobs.currTextureIM = texture;
        }
    } else {
        D3DTEXTUREHANDLE handle = 0;
        if (texture) {
            lpIMDevice()->QueryInterface(IID_IDirect3DDevice2, (void**)&dev2);
            texture->GetHandle(dev2, &handle);
            if (dev2) {
                dev2->Release();
                dev2 = NULL;
            }
        }
        if (handle != meshGlobs.currTextureRM) {
            main_change_render_state(1, handle);
            meshGlobs.currTextureRM = handle;
        }
    }

    if (matHandle != meshGlobs.currMatIM) {
        if (lpIMDevice()->SetLightState(D3DLIGHTSTATE_MATERIAL, matHandle) != D3D_OK) ok = 0;
        else meshGlobs.currMatIM = matHandle;
    }

    if (lpIMDevice()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, renderFlags, vertices,
                                           vertexCount, faceData, indexCount, 0) != D3D_OK) ok = 0;

    return ok;
}

#ifdef __cplusplus
}
#endif
