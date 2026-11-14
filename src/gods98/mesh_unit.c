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

/* The original's Standard.h includes the Windows and DirectX headers. MSVC
 * 5.0's DirectX headers are DirectX 3's, without the interfaces this unit
 * calls, so it declares its own (below) and includes only <windows.h>, in
 * the matching build only, as config.c does. Of the include sets tried
 * (finding 625), this one, with mesh_api.h's three enums, puts the most
 * functions in the symbol counter's phase that matches (finding 538). */
#if defined(_MSC_VER) && _MSC_VER == 1100
#include <windows.h>
#endif

#include "mesh_api.h"
#include "mesh_imports.h"
#include "lwo.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#define MESH_STDCALL __stdcall
#else
#define MESH_STDCALL
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

typedef struct Mesh_IID {
    unsigned long data1;
    unsigned short data2;
    unsigned short data3;
    unsigned char data4[8];
} Mesh_IID;

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

/* D3DMATERIAL (0x50 bytes). */
struct Mesh_D3DMaterial {
    unsigned long dwSize;     /* +0x00 */
    Mesh_Colour4F diffuse;    /* +0x04 */
    Mesh_Colour4F ambient;    /* +0x14 */
    Mesh_Colour4F specular;   /* +0x24 */
    Mesh_Colour4F emissive;   /* +0x34 */
    float power;              /* +0x44 */
    unsigned long hTexture;   /* +0x48 */
    unsigned long dwRampSize; /* +0x4c */
};

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

typedef struct Mesh_Unknown Mesh_Unknown;     /* IUnknown */
typedef struct Mesh_D3D Mesh_D3D;             /* IDirect3D3 */
typedef struct Mesh_IMDevice Mesh_IMDevice;   /* IDirect3DDevice3 */
typedef struct Mesh_IMDevice2 Mesh_IMDevice2; /* IDirect3DDevice2 */
typedef struct Mesh_Material Mesh_Material;   /* IDirect3DMaterial3 */
typedef struct Mesh_IMViewport Mesh_IMViewport; /* IDirect3DViewport3 */
typedef struct Mesh_D3DRM Mesh_D3DRM;         /* IDirect3DRM3 */
typedef struct Mesh_UserVisual Mesh_UserVisual; /* IDirect3DRMUserVisual */
typedef struct Mesh_RMDevice Mesh_RMDevice;   /* IDirect3DRMDevice3 */

typedef struct Mesh_UnknownVtbl {
    long (MESH_STDCALL* QueryInterface)(Mesh_Unknown*, const Mesh_IID*,
                                        void**);                /* +0x00 */
    unsigned long (MESH_STDCALL* AddRef)(Mesh_Unknown*);         /* +0x04 */
    unsigned long (MESH_STDCALL* Release)(Mesh_Unknown*);        /* +0x08 */
} Mesh_UnknownVtbl;
struct Mesh_Unknown {
    const Mesh_UnknownVtbl* lpVtbl;
};

typedef struct Mesh_D3DVtbl {
    void* slots_00[2];
    unsigned long (MESH_STDCALL* Release)(Mesh_D3D*);            /* +0x08 */
    void* slots_0c[2];
    long (MESH_STDCALL* CreateMaterial)(Mesh_D3D*, Mesh_Material**,
                                        Mesh_Unknown*);         /* +0x14 */
} Mesh_D3DVtbl;
struct Mesh_D3D {
    const Mesh_D3DVtbl* lpVtbl;
};

typedef struct Mesh_MaterialVtbl {
    void* slots_00[2];
    unsigned long (MESH_STDCALL* Release)(Mesh_Material*);       /* +0x08 */
    long (MESH_STDCALL* SetMaterial)(Mesh_Material*,
                                     Mesh_D3DMaterial*);        /* +0x0c */
    void* slots_10[1];
    long (MESH_STDCALL* GetHandle)(Mesh_Material*, Mesh_IMDevice*,
                                   unsigned long*);             /* +0x14 */
} Mesh_MaterialVtbl;
struct Mesh_Material {
    const Mesh_MaterialVtbl* lpVtbl;
};

typedef struct Mesh_Texture2Vtbl {
    void* slots_00[2];
    unsigned long (MESH_STDCALL* Release)(Mesh_Texture2*);       /* +0x08 */
    long (MESH_STDCALL* GetHandle)(Mesh_Texture2*, Mesh_IMDevice2*,
                                   unsigned long*);             /* +0x0c */
} Mesh_Texture2Vtbl;
struct Mesh_Texture2 {
    const Mesh_Texture2Vtbl* lpVtbl;
};

typedef struct Mesh_IMDevice2Vtbl {
    void* slots_00[2];
    unsigned long (MESH_STDCALL* Release)(Mesh_IMDevice2*);      /* +0x08 */
} Mesh_IMDevice2Vtbl;
struct Mesh_IMDevice2 {
    const Mesh_IMDevice2Vtbl* lpVtbl;
};

typedef struct Mesh_SurfaceVtbl {
    long (MESH_STDCALL* QueryInterface)(Mesh_Surface*, const Mesh_IID*,
                                        void**);                /* +0x00 */
    unsigned long (MESH_STDCALL* AddRef)(Mesh_Surface*);         /* +0x04 */
} Mesh_SurfaceVtbl;
struct Mesh_Surface {
    const Mesh_SurfaceVtbl* lpVtbl;
};

typedef struct Mesh_IMViewportVtbl {
    long (MESH_STDCALL* QueryInterface)(Mesh_IMViewport*, const Mesh_IID*,
                                        void**);                /* +0x00 */
    void* slots_04[1];
    unsigned long (MESH_STDCALL* Release)(Mesh_IMViewport*);     /* +0x08 */
} Mesh_IMViewportVtbl;
struct Mesh_IMViewport {
    const Mesh_IMViewportVtbl* lpVtbl;
};

typedef struct Mesh_IMDeviceVtbl {
    long (MESH_STDCALL* QueryInterface)(Mesh_IMDevice*, const Mesh_IID*,
                                        void**);                /* +0x00 */
    void* slots_04[1];
    unsigned long (MESH_STDCALL* Release)(Mesh_IMDevice*);       /* +0x08 */
    void* slots_0c[8];
    long (MESH_STDCALL* GetDirect3D)(Mesh_IMDevice*, Mesh_D3D**); /* +0x2c */
    long (MESH_STDCALL* SetCurrentViewport)(Mesh_IMDevice*,
                                            Mesh_IMViewport*);  /* +0x30 */
    void* slots_34[8];
    long (MESH_STDCALL* GetRenderState)(Mesh_IMDevice*, unsigned long,
                                        unsigned long*);        /* +0x54 */
    long (MESH_STDCALL* SetRenderState)(Mesh_IMDevice*, unsigned long,
                                        unsigned long);         /* +0x58 */
    long (MESH_STDCALL* GetLightState)(Mesh_IMDevice*, unsigned long,
                                       unsigned long*);         /* +0x5c */
    long (MESH_STDCALL* SetLightState)(Mesh_IMDevice*, unsigned long,
                                       unsigned long);          /* +0x60 */
    long (MESH_STDCALL* SetTransform)(Mesh_IMDevice*, unsigned long,
                                      Mesh_Matrix*);            /* +0x64 */
    long (MESH_STDCALL* GetTransform)(Mesh_IMDevice*, unsigned long,
                                      Mesh_Matrix*);            /* +0x68 */
    void* slots_6c[2];
    long (MESH_STDCALL* DrawIndexedPrimitive)(Mesh_IMDevice*, unsigned long,
                                              unsigned long, void*,
                                              unsigned long,
                                              unsigned short*,
                                              unsigned long,
                                              unsigned long);   /* +0x74 */
    void* slots_78[7];
    long (MESH_STDCALL* GetTexture)(Mesh_IMDevice*, unsigned long,
                                    Mesh_Texture2**);           /* +0x94 */
    long (MESH_STDCALL* SetTexture)(Mesh_IMDevice*, unsigned long,
                                    Mesh_Texture2*);            /* +0x98 */
    long (MESH_STDCALL* GetTextureStageState)(Mesh_IMDevice*, unsigned long,
                                              unsigned long,
                                              unsigned long*);  /* +0x9c */
    long (MESH_STDCALL* SetTextureStageState)(Mesh_IMDevice*, unsigned long,
                                              unsigned long,
                                              unsigned long);   /* +0xa0 */
} Mesh_IMDeviceVtbl;
struct Mesh_IMDevice {
    const Mesh_IMDeviceVtbl* lpVtbl;
};

typedef int (*Mesh_UserVisualCallback)(Mesh_UserVisual* lpD3DRMUV,
                                       void* lpArg, int lpD3DRMUVreason,
                                       Mesh_RMDevice* lpD3DRMDev,
                                       Mesh_RMViewport* lpView);

typedef struct Mesh_D3DRMVtbl {
    void* slots_00[22];
    long (MESH_STDCALL* CreateUserVisual)(Mesh_D3DRM*,
                                          Mesh_UserVisualCallback, void*,
                                          Mesh_UserVisual**);   /* +0x58 */
} Mesh_D3DRMVtbl;
struct Mesh_D3DRM {
    const Mesh_D3DRMVtbl* lpVtbl;
};

typedef struct Mesh_FrameVtbl {
    void* slots_00[18];
    long (MESH_STDCALL* AddVisual)(Mesh_Frame*, Mesh_Unknown*);   /* +0x48 */
    void* slots_4c[7];
    long (MESH_STDCALL* GetScene)(Mesh_Frame*, Mesh_Frame**);     /* +0x68 */
    void* slots_6c[8];
    long (MESH_STDCALL* LookAt)(Mesh_Frame*, Mesh_Frame*, Mesh_Frame*,
                                unsigned long);                 /* +0x8c */
    void* slots_90[4];
    long (MESH_STDCALL* DeleteVisual)(Mesh_Frame*, Mesh_Unknown*); /* +0xa0 */
} Mesh_FrameVtbl;
struct Mesh_Frame {
    const Mesh_FrameVtbl* lpVtbl;
};

typedef struct Mesh_UserVisualVtbl {
    void* slots_00[2];
    unsigned long (MESH_STDCALL* Release)(Mesh_UserVisual*);     /* +0x08 */
    void* slots_0c[3];
    long (MESH_STDCALL* SetAppData)(Mesh_UserVisual*, void*);    /* +0x18 */
} Mesh_UserVisualVtbl;
struct Mesh_UserVisual {
    const Mesh_UserVisualVtbl* lpVtbl;
};

typedef struct Mesh_RMViewportVtbl {
    long (MESH_STDCALL* QueryInterface)(Mesh_RMViewport*, const Mesh_IID*,
                                        void**);                /* +0x00 */
    void* slots_04[1];
    unsigned long (MESH_STDCALL* Release)(Mesh_RMViewport*);     /* +0x08 */
    void* slots_0c[4];
    void* (MESH_STDCALL* GetAppData)(Mesh_RMViewport*);          /* +0x1c */
    void* slots_20[17];
    long (MESH_STDCALL* GetCamera)(Mesh_RMViewport*, Mesh_Frame**); /* +0x64 */
    void* slots_68[12];
    long (MESH_STDCALL* GetDirect3DViewport)(Mesh_RMViewport*,
                                             Mesh_IMViewport**); /* +0x98 */
} Mesh_RMViewportVtbl;
struct Mesh_RMViewport {
    const Mesh_RMViewportVtbl* lpVtbl;
};

/* Viewports.c's Viewport: the Direct3DRM viewport at +0x10 and the
 * rendering flag at +0x18. */
struct Mesh_Viewport {
    float xoffset;          /* +0x00 */
    float yoffset;          /* +0x04 */
    float width;            /* +0x08 */
    float height;           /* +0x0c */
    Mesh_RMViewport* lpVP;  /* +0x10 */
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
    Mesh_Surface* surface;  /* +0x00 */
    Mesh_Texture2* texture; /* +0x04 */
    int colourKey;          /* +0x08 */
};

/* A loaded texture's name (0xc bytes). */
struct Mesh_TextureReference {
    Mesh_Surface* surface;  /* +0x00 */
    char* path;             /* +0x04 */
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
    Mesh_D3DMaterial material;             /* +0x10 */
    Mesh_Texture2* imText;                 /* +0x60 */
    unsigned long renderFlags;             /* +0x64 */
    Mesh_LightWave_Surface* lightWaveSurfaceInfo; /* +0x68 */
    unsigned long flags;                   /* +0x6c */
};


/* A mesh (0x34 bytes). */
struct Mesh {
    unsigned long groupCount;              /* +0x00 */
    unsigned long listSize;                /* +0x04 */
    Mesh_Group* groupList;                 /* +0x08 */
    Mesh_UserVisual* uv;                   /* +0x0c */
    Mesh_RenderCallbackFunc renderCallback; /* +0x10 */
    void* renderCallbackData;              /* +0x14 */
    unsigned long renderFlags;             /* +0x18 */
    Mesh_LightWave_Surface* lightWaveSurf; /* +0x1c */
    unsigned long numOfRefs;               /* +0x20 */
    Mesh* clonedFrom;                      /* +0x24 */
    Mesh_Frame* frameCreatedOn;            /* +0x28 */
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
    Mesh_PostRenderInfo* postRenderList;                             /* +0x0000 */
    Mesh_TextureStateChangeData stateData[MESH_MAXTEXTURESTAGESTATES]; /* +0x0004 */
    char* sharedTextureDir;                                          /* +0x0194 */
    Mesh_TextureReference textureList[MESH_TEXTURELISTSIZE];         /* +0x0198 */
    Mesh_TextureReference textureListShared[MESH_TEXTURELISTSIZE];   /* +0x6198 */
    unsigned long textureCount;                                      /* +0xc198 */
    unsigned long textureCountShared;                                /* +0xc19c */
    unsigned long oldTextureRM;                                      /* +0xc1a0 */
    unsigned long oldMatIM;                                          /* +0xc1a4 */
    Mesh_Texture2* oldTextureIM;                                     /* +0xc1a8 */
    unsigned long currTextureRM;                                     /* +0xc1ac */
    unsigned long currMatIM;                                         /* +0xc1b0 */
    Mesh_Texture2* currTextureIM;                                    /* +0xc1b4 */
    unsigned long matHandle;                                         /* +0xc1b8 */
    Mesh_Material* mat;                                              /* +0xc1bc */
    Mesh* listSet[MESH_MAXLISTS];                                    /* +0xc1c0 */
    Mesh* freeList;                                                  /* +0xc210 */
    unsigned long listCount;                                         /* +0xc214 */
} Mesh_Globs;

#define lpD3DRM() ((Mesh_D3DRM*)s_main_d3drm)
#define lpIMDevice() ((Mesh_IMDevice*)s_main_im_device)

/* IID_IDirect3DViewport3: Viewports.c's copy (viewports.c), which the two
 * units share. */
extern const Mesh_IID IID_IDirect3DViewport3;

/* IID_IDirect3DTexture2, IID_IDirect3DDevice2 and IID_IDirect3DRMViewport2.
 * The original links them from dxguid.lib; only this unit uses them. */
// GLOBAL: LEGORR 0x0049f6c8
static const Mesh_IID IID_IDirect3DDevice2 = {
    0x93281501, 0x8cf8, 0x11d0,
    { 0x89, 0xab, 0x00, 0xa0, 0xc9, 0x05, 0x41, 0x29 }
};

// GLOBAL: LEGORR 0x0049f6f8
static const Mesh_IID IID_IDirect3DTexture2 = {
    0x93281502, 0x8cf8, 0x11d0,
    { 0x89, 0xab, 0x00, 0xa0, 0xc9, 0x05, 0x41, 0x29 }
};

// GLOBAL: LEGORR 0x0049f958
static const Mesh_IID IID_IDirect3DRMViewport2 = {
    0x4a1b1be6, 0xbfed, 0x11d1,
    { 0x8e, 0xd8, 0x00, 0xa0, 0xc9, 0x67, 0xa4, 0x82 }
};

// GLOBAL: LEGORR 0x005343c4
static Mesh_Globs meshGlobs;

static int mesh_render_callback(Mesh_UserVisual* lpD3DRMUV, void* lpArg,
                                int lpD3DRMUVreason,
                                Mesh_RMDevice* lpD3DRMDev,
                                Mesh_RMViewport* lpView);

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
    Mesh_D3D* d3d;
    Mesh_D3DMaterial material = {
        sizeof(Mesh_D3DMaterial),
        { 1.0f, 1.0f, 1.0f, 1.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        0.0f, 0, 0
    };

    lpIMDevice()->lpVtbl->GetDirect3D(lpIMDevice(), &d3d);

    if (d3d->lpVtbl->CreateMaterial(d3d, &meshGlobs.mat, NULL) != 0) {
        if (d3d) d3d->lpVtbl->Release(d3d);
        return 0;
    }
    if (meshGlobs.mat->lpVtbl->GetHandle(meshGlobs.mat, lpIMDevice(),
                                         &meshGlobs.matHandle) != 0) {
        if (d3d) d3d->lpVtbl->Release(d3d);
        return 0;
    }

    mesh_set_material(&material);
    if (d3d) d3d->lpVtbl->Release(d3d);

    return 1;

}

// FUNCTION: LEGORR 0x00480780
int mesh_set_material(Mesh_D3DMaterial* newMaterial)
{
    return meshGlobs.mat->lpVtbl->SetMaterial(meshGlobs.mat, newMaterial) == 0;
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
Mesh* mesh_create_on_frame(Mesh_Frame* frame,
                           Mesh_RenderCallbackFunc renderFunc,
                           unsigned long renderFlags, void* data,
                           unsigned long type)
{
    Mesh* mesh = mesh_obtain_from_list();

    container_get_root();

    lpD3DRM()->lpVtbl->CreateUserVisual(lpD3DRM(), mesh_render_callback,
                                        mesh, &mesh->uv);
    frame->lpVtbl->AddVisual(frame, (Mesh_Unknown*)mesh->uv);
    mesh->uv->lpVtbl->SetAppData(mesh->uv, mesh);

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
    Mesh_Frame* lpFrame = (Mesh_Frame*)frame;
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

    lpD3DRM()->lpVtbl->CreateUserVisual(lpD3DRM(), mesh_render_callback,
                                        newMesh, &newMesh->uv);
    lpFrame->lpVtbl->AddVisual(lpFrame, (Mesh_Unknown*)newMesh->uv);
    newMesh->uv->lpVtbl->SetAppData(newMesh->uv, newMesh);

    newMesh->frameCreatedOn = lpFrame;
    mesh->numOfRefs++;
    mesh->flags |= MESH_FLAG_HASBEENCLONED;
    newMesh->numOfRefs = 0;

    return newMesh;
}

// FUNCTION: LEGORR 0x004809e0
Mesh* mesh_load(const char* fname, Mesh_Frame* frame, int noTextures)
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

// FUNCTION: LEGORR 0x00480ac0
int mesh_parse_lwo(const char* basePath, Mesh* mesh,
                   APPOBJ* lightWaveObject, int noTextures)
{
    unsigned long loop, loopSurf, loopVert;
    unsigned long maxVertices, maxFaces, surfCount;
    unsigned long triangleFan[6];
    unsigned long* surfVertexCount;
    unsigned long* surfFaceCount;
    unsigned long* vertexIndexList;
    unsigned long* surfFaceData;
    Mesh_Vector3F* surfVertices;
    Mesh_Vector3F** surfNormals;
    Mesh_Vector3F* faceNormals;
    Mesh_Vector3F* vertexNormals;
    Mesh_Point2F* textCoords;
    Mesh_Point2F* textCoordsUV = NULL;
    Mesh_Vector3F polyVerts[3];
    unsigned long* faceDataPtr;
    Mesh_Vector3F* surfVerticesPtr;
    Mesh_Vector3F** surfNormalsPtr;
    Mesh_Point2F* textCoordsPtr;
    unsigned long* surfVertexCountPtr;
    unsigned long groupID, renderFlags;

    maxVertices = lightWaveObject->aoSize.lwVertCount * 2;
    maxFaces = lightWaveObject->aoSize.lwPolyCount * 6;
    triangleFan[0] = 0;
    triangleFan[1] = 1;
    triangleFan[2] = 2;
    triangleFan[3] = 0;
    triangleFan[4] = 2;
    triangleFan[5] = 3;

    if (lightWaveObject->aoFileUV) {
        textCoordsUV = (Mesh_Point2F*)malloc(sizeof(Mesh_Point2F) * maxVertices * 20);
        mesh_uview_mesh(lightWaveObject, textCoordsUV);
        maxVertices = lightWaveObject->aoSize.lwVertCount * 2;
        textCoords = (Mesh_Point2F*)malloc(sizeof(Mesh_Point2F) * maxVertices * lightWaveObject->aoSize.lwSurfaceCount);
    } else {
        textCoords = (Mesh_Point2F*)malloc(sizeof(Mesh_Point2F) * maxVertices);
    }

    surfCount = lightWaveObject->aoSize.lwSurfaceCount;
    surfVertices = (Mesh_Vector3F*)malloc(sizeof(Mesh_Vector3F) * maxVertices * surfCount);
    surfNormals = (Mesh_Vector3F**)malloc(sizeof(Mesh_Vector3F*) * maxVertices * surfCount);
    surfVertexCount = (unsigned long*)malloc(sizeof(unsigned long) * lightWaveObject->aoSize.lwSurfaceCount);
    vertexIndexList = (unsigned long*)malloc(sizeof(unsigned long) * maxVertices * surfCount);
    surfFaceData = (unsigned long*)malloc(sizeof(unsigned long) * maxFaces * lightWaveObject->aoSize.lwSurfaceCount);
    surfFaceCount = (unsigned long*)malloc(sizeof(unsigned long) * lightWaveObject->aoSize.lwSurfaceCount);
    faceNormals = (Mesh_Vector3F*)malloc(sizeof(Mesh_Vector3F) * lightWaveObject->aoSize.lwPolyCount);
    vertexNormals = (Mesh_Vector3F*)malloc(sizeof(Mesh_Vector3F) * maxVertices);

    memset(surfVertexCount, 0, sizeof(unsigned long) * lightWaveObject->aoSize.lwSurfaceCount);
    memset(vertexIndexList, -1, sizeof(unsigned long) * maxVertices * lightWaveObject->aoSize.lwSurfaceCount);
    memset(surfFaceCount, 0, sizeof(unsigned long) * lightWaveObject->aoSize.lwSurfaceCount);
    memset(vertexNormals, 0, sizeof(Mesh_Vector3F) * maxVertices);

    mesh->lightWaveSurf = (Mesh_LightWave_Surface*)malloc(sizeof(Mesh_LightWave_Surface) * lightWaveObject->aoSize.lwSurfaceCount);
    memset(mesh->lightWaveSurf, 0, sizeof(Mesh_LightWave_Surface) * lightWaveObject->aoSize.lwSurfaceCount);

    for (loop = 0; loop < lightWaveObject->aoSize.lwPolyCount; loop++) {
        for (loopVert = 0; loopVert < 3; loopVert++) {
            unsigned short index = lightWaveObject->aoPoly[loop].plyData[loopVert];
            polyVerts[loopVert] = ((Mesh_Vector3F*)lightWaveObject->aoVerts)[index];
        }
        surface_triangle_normal_calculate((float*)&faceNormals[loop], (float*)&polyVerts[0],
                                          (float*)&polyVerts[1], (float*)&polyVerts[2]);
    }

    mesh_get_surf_info(basePath, lightWaveObject, mesh->lightWaveSurf, noTextures);

    for (loop = 0; loop < lightWaveObject->aoSize.lwPolyCount; loop++) {
        unsigned long surf = lightWaveObject->aoPoly[loop].srfIndex;
        unsigned long faceOffset = maxFaces * surf;
        unsigned long vertOffset = maxVertices * surf;

        if (lightWaveObject->aoPoly[loop].plyCount == 3) {
            for (loopVert = 0; loopVert < 3; loopVert++) {
                unsigned short index = lightWaveObject->aoPoly[loop].plyData[loopVert];
                if (vertexIndexList[index + vertOffset] == -1) {
                    surfVertices[surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex] + vertOffset].x = ((Mesh_Vector3F*)lightWaveObject->aoVerts)[index].x;
                    surfVertices[surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex] + vertOffset].y = ((Mesh_Vector3F*)lightWaveObject->aoVerts)[index].y;
                    surfVertices[surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex] + vertOffset].z = ((Mesh_Vector3F*)lightWaveObject->aoVerts)[index].z;
                    if (lightWaveObject->aoFileUV) {
                        textCoords[surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex] + vertOffset].x = textCoordsUV[lightWaveObject->aoPoly[loop].plyData[loopVert]].x;
                        textCoords[surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex] + vertOffset].y = textCoordsUV[lightWaveObject->aoPoly[loop].plyData[loopVert]].y;
                    }
                    index = lightWaveObject->aoPoly[loop].plyData[loopVert];
                    mesh_vector3d_add(&vertexNormals[index], &faceNormals[loop], &vertexNormals[index]);
                    mesh_vector3d_normalise(&vertexNormals[lightWaveObject->aoPoly[loop].plyData[loopVert]]);
                    surfNormals[surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex] + vertOffset] =
                        &vertexNormals[lightWaveObject->aoPoly[loop].plyData[loopVert]];
                    surfFaceData[surfFaceCount[lightWaveObject->aoPoly[loop].srfIndex] + faceOffset] =
                        surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex];
                    surfFaceCount[lightWaveObject->aoPoly[loop].srfIndex]++;
                    vertexIndexList[lightWaveObject->aoPoly[loop].plyData[loopVert] + vertOffset] =
                        surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex];
                    surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex]++;
                } else {
                    mesh_vector3d_add(&vertexNormals[index], &faceNormals[loop], &vertexNormals[index]);
                    mesh_vector3d_normalise(&vertexNormals[lightWaveObject->aoPoly[loop].plyData[loopVert]]);
                    surfFaceData[surfFaceCount[lightWaveObject->aoPoly[loop].srfIndex] + faceOffset] =
                        vertexIndexList[lightWaveObject->aoPoly[loop].plyData[loopVert] + vertOffset];
                    surfFaceCount[lightWaveObject->aoPoly[loop].srfIndex]++;
                }
            }
        } else {
            for (loopVert = 0; loopVert < 6; loopVert++) {
                unsigned short index = lightWaveObject->aoPoly[loop].plyData[triangleFan[loopVert]];
                if (vertexIndexList[vertOffset + index] == -1) {
                    surfVertices[surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex] + vertOffset].x = ((Mesh_Vector3F*)lightWaveObject->aoVerts)[index].x;
                    surfVertices[surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex] + vertOffset].y = ((Mesh_Vector3F*)lightWaveObject->aoVerts)[index].y;
                    surfVertices[surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex] + vertOffset].z = ((Mesh_Vector3F*)lightWaveObject->aoVerts)[index].z;
                    if (lightWaveObject->aoFileUV) {
                        textCoords[surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex] + vertOffset].x = textCoordsUV[lightWaveObject->aoPoly[loop].plyData[triangleFan[loopVert]]].x;
                        textCoords[surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex] + vertOffset].y = textCoordsUV[lightWaveObject->aoPoly[loop].plyData[triangleFan[loopVert]]].y;
                    }
                    index = lightWaveObject->aoPoly[loop].plyData[triangleFan[loopVert]];
                    mesh_vector3d_add(&vertexNormals[index], &faceNormals[loop], &vertexNormals[index]);
                    mesh_vector3d_normalise(&vertexNormals[lightWaveObject->aoPoly[loop].plyData[triangleFan[loopVert]]]);
                    surfNormals[surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex] + vertOffset] =
                        &vertexNormals[lightWaveObject->aoPoly[loop].plyData[triangleFan[loopVert]]];
                    surfFaceData[surfFaceCount[lightWaveObject->aoPoly[loop].srfIndex] + faceOffset] =
                        surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex];
                    surfFaceCount[lightWaveObject->aoPoly[loop].srfIndex]++;
                    vertexIndexList[lightWaveObject->aoPoly[loop].plyData[triangleFan[loopVert]] + vertOffset] =
                        surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex];
                    surfVertexCount[lightWaveObject->aoPoly[loop].srfIndex]++;
                } else {
                    mesh_vector3d_add(&vertexNormals[index], &faceNormals[loop], &vertexNormals[index]);
                    mesh_vector3d_normalise(&vertexNormals[lightWaveObject->aoPoly[loop].plyData[triangleFan[loopVert]]]);
                    surfFaceData[surfFaceCount[lightWaveObject->aoPoly[loop].srfIndex] + faceOffset] =
                        vertexIndexList[lightWaveObject->aoPoly[loop].plyData[triangleFan[loopVert]] + vertOffset];
                    surfFaceCount[lightWaveObject->aoPoly[loop].srfIndex]++;
                }
            }
        }
    }

    faceDataPtr = surfFaceData;
    surfVerticesPtr = surfVertices;
    surfNormalsPtr = surfNormals;
    textCoordsPtr = textCoords;
    surfVertexCountPtr = surfVertexCount;
    for (loopSurf = 0; loopSurf < lightWaveObject->aoSize.lwSurfaceCount; loopSurf++) {
        Mesh_LightWave_Surface* lwSurf;

        groupID = mesh_add_group(mesh, surfFaceCount[loopSurf], surfFaceCount[loopSurf] / 3, 3, faceDataPtr);

        if (lightWaveObject->aoFileUV) {
            mesh_set_vertices_vnt(mesh, groupID, 0, *surfVertexCountPtr, surfVerticesPtr, surfNormalsPtr, textCoordsPtr);
        } else {
            LWSURFACE* lwo = &lightWaveObject->aoSurface[loopSurf];
            mesh_get_texture_uvs_wrap(*surfVertexCountPtr, surfVerticesPtr, textCoords,
                                      lwo->srfTSIZ[0], lwo->srfTSIZ[1], lwo->srfTSIZ[2],
                                      lwo->srfTCTR[0], lwo->srfTCTR[1], lwo->srfTCTR[2],
                                      lwo->srfTexFlags);
            mesh_set_vertices_vnt(mesh, groupID, 0, *surfVertexCountPtr, surfVerticesPtr, surfNormalsPtr, textCoords);
        }

        lwSurf = &mesh->lightWaveSurf[loopSurf];
        if (lwSurf->flags & MESH_SFLAG_ADDITIVE) {
            mesh_alter_group_render_flags(mesh, groupID, MESH_RENDERFLAGS_ADDITIVE);
            renderFlags = MESH_RENDERFLAGS_ADDITIVE;
        } else {
            renderFlags = MESH_RENDERFLAGS_LWONORM;
        }
        if (mesh->lightWaveSurf[loopSurf].flags & MESH_SFLAG_DOUBLESIDED) {
            renderFlags |= 0x04000000;
            mesh_alter_group_render_flags(mesh, groupID, renderFlags);
        }
        if (!(mesh->lightWaveSurf[loopSurf].texFlags & MESH_TFLAG_PIXELBLENDING)) {
            mesh_alter_group_render_flags(mesh, groupID, renderFlags | 0x00800000);
        }

        if (lightWaveObject->aoSurface[loopSurf].srfTexFlags & MESH_TFLAG_SEQUENCE) {
            if (mesh->lightWaveSurf[loopSurf].textureSeq) {
                mesh_set_group_texture(mesh, groupID, mesh->lightWaveSurf[loopSurf].textureSeq[0]);
            }
        } else if (mesh->lightWaveSurf[loopSurf].texture) {
            mesh_set_group_texture(mesh, groupID, mesh->lightWaveSurf[loopSurf].texture);
        }

        lwSurf = &mesh->lightWaveSurf[loopSurf];
        mesh_set_group_colour(mesh, groupID, lwSurf->colour.r * lwSurf->diffuse,
                              lwSurf->colour.g * lwSurf->diffuse,
                              lwSurf->colour.b * lwSurf->diffuse, Mesh_Colour_Diffuse);
        lwSurf = &mesh->lightWaveSurf[loopSurf];
        mesh_set_group_colour(mesh, groupID, lwSurf->colour.r * lwSurf->emissive,
                              lwSurf->colour.g * lwSurf->emissive,
                              lwSurf->colour.b * lwSurf->emissive, Mesh_Colour_Emissive);
        lwSurf = &mesh->lightWaveSurf[loopSurf];
        if (lwSurf->flags & MESH_SFLAG_COLOURHIGHLIGHT) {
            mesh_set_group_colour(mesh, groupID, lwSurf->colour.r * lwSurf->specular,
                                  lwSurf->colour.g * lwSurf->specular,
                                  lwSurf->colour.b * lwSurf->specular, Mesh_Colour_Specular);
        } else {
            mesh_set_group_colour(mesh, groupID, lwSurf->specular, lwSurf->specular,
                                  lwSurf->specular, Mesh_Colour_Specular);
        }
        mesh_set_group_material_values(mesh, groupID, mesh->lightWaveSurf[loopSurf].power * 10.0f, Mesh_Colour_Power);
        mesh_set_group_material_values(mesh, groupID, 1.0f - mesh->lightWaveSurf[loopSurf].transparency, Mesh_Colour_Alpha);

        mesh->groupList[groupID].lightWaveSurfaceInfo = &mesh->lightWaveSurf[loopSurf];

        textCoordsPtr += maxVertices;
        surfNormalsPtr += maxVertices;
        surfVerticesPtr += maxVertices;
        faceDataPtr += maxFaces;
        surfVertexCountPtr++;
    }

    free(surfVertices);
    free(surfNormals);
    free(textCoords);
    free(surfVertexCount);
    free(vertexIndexList);
    free(surfFaceData);
    free(surfFaceCount);
    free(faceNormals);
    free(vertexNormals);
    if (textCoordsUV) free(textCoordsUV);

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
    unsigned long tnamelen = strlen(tname);
    unsigned long n = 1, k, num = 0;
    unsigned char c;

    c = tname[tnamelen - 1];
    if (c >= '0' && c <= '9') {
        for (k = tnamelen - 1; k; k--) {
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
            Mesh_LightWave_Surface* surf = mesh->groupList[groupID].lightWaveSurfaceInfo;
            if ((surf->texFlags & MESH_TFLAG_SEQUENCE) && surf->numInTexSeq) {
                long texNum = (surf->texSeqOffset + addFrame) % (long)surf->numInTexSeq;
                if (texNum < 0) texNum = 0;
                mesh_set_group_texture(mesh, groupID, surf->textureSeq[texNum]);
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
    Mesh_Frame* lpFrame = (Mesh_Frame*)frame;
    unsigned long groupID;

    lpFrame->lpVtbl->DeleteVisual(lpFrame, (Mesh_Unknown*)mesh->uv);

    if (mesh->clonedFrom) {
        Mesh* clonedFrom = mesh->clonedFrom;
        if (mesh->uv) {
            mesh->uv->lpVtbl->Release(mesh->uv);
            mesh->uv = NULL;
        }
        free(mesh->groupList);
        mesh_return_to_list(mesh);
        mesh = clonedFrom;
    }

    if (--mesh->numOfRefs == 0) {
        if (mesh->uv) {
            mesh->uv->lpVtbl->Release(mesh->uv);
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
static int mesh_render_callback(Mesh_UserVisual* lpD3DRMUV, void* lpArg,
                                int lpD3DRMUVreason,
                                Mesh_RMDevice* lpD3DRMDev,
                                Mesh_RMViewport* lpView)
{
    Mesh* mesh = (Mesh*)lpArg;
    int alphaFound, renderStatesStored;

    if (lpD3DRMUVreason == 0) {
        if (lpIMDevice()) {
            Mesh_Container* cont;
            Mesh_Viewport* vp;

            if (mesh->flags & MESH_FLAG_HIDDEN) return 0;
            cont = (Mesh_Container*)container_frame_get_container(mesh->frameCreatedOn);
            if (cont && (cont->flags & MESH_CONTAINER_FLAG_HIDDEN)) return 0;
            if ((mesh->flags & MESH_FLAG_FACECAMERA) && !(mesh->flags & MESH_FLAG_FACECAMERADONE)) {
                vp = (Mesh_Viewport*)lpView->lpVtbl->GetAppData(lpView);
                if (vp->rendering) {
                    Mesh_Frame* scene;
                    Mesh_Frame* camera;
                    mesh->frameCreatedOn->lpVtbl->GetScene(mesh->frameCreatedOn, &scene);
                    vp->lpVP->lpVtbl->GetCamera(vp->lpVP, &camera);
                    mesh->frameCreatedOn->lpVtbl->LookAt(mesh->frameCreatedOn, camera, scene, 0);
                    mesh->flags |= MESH_FLAG_FACECAMERADONE;
                }
            }
            return 1;
        }
    } else if (lpD3DRMUVreason == 1) {
        if (lpIMDevice()) {
            Mesh_Matrix matWorld;
            Mesh_RMViewport* view;
            Mesh_Viewport* vp;
            unsigned long groupID;
            Mesh_Group* group;

            alphaFound = 0;
            renderStatesStored = 0;
            mesh->flags &= ~MESH_FLAG_FACECAMERADONE;
            lpIMDevice()->lpVtbl->GetTransform(lpIMDevice(), 1, &matWorld);
            mesh_set_current_viewport(lpView);
            lpView->lpVtbl->QueryInterface(lpView, &IID_IDirect3DRMViewport2, (void**)&view);
            vp = (Mesh_Viewport*)view->lpVtbl->GetAppData(view);

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

            if (view) view->lpVtbl->Release(view);
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
    Mesh_Surface* surface;
    Mesh_TextureReference* ref;
    int trans;
    char path[260];

    if (baseDir) sprintf(path, "%s%s", baseDir, name);
    else strcpy(path, name);

    if (ref = (Mesh_TextureReference*)mesh_search_texture_list(meshGlobs.textureList, meshGlobs.textureCount, path)) {
        surface = ref->surface;
        if (surface) surface->lpVtbl->AddRef(surface);
        trans = ref->trans;
    } else if (surface = (Mesh_Surface*)container_load_texture_surface(path, 1, width, height, &trans)) {
        mesh_add_texture_reference(meshGlobs.textureList, &meshGlobs.textureCount, path, surface, trans);
    } else if (meshGlobs.sharedTextureDir) {
        sprintf(path, "%s%s", meshGlobs.sharedTextureDir, name);
        if (ref = (Mesh_TextureReference*)mesh_search_texture_list(meshGlobs.textureListShared, meshGlobs.textureCountShared, path)) {
            surface = ref->surface;
            if (surface) surface->lpVtbl->AddRef(surface);
            trans = ref->trans;
        } else if (surface = (Mesh_Surface*)container_load_texture_surface(path, 1, width, height, &trans)) {
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
                                Mesh_Surface* surface, int trans)
{

    textureList[*count].path = string_duplicate(path);
    textureList[*count].surface = (Mesh_Surface*)surface;
    textureList[*count].trans = trans;
    (*count)++;
}

// FUNCTION: LEGORR 0x004830c0
void mesh_set_group_texture(Mesh* mesh, unsigned long groupID,
                            Mesh_Texture* texture)
{

    if (texture) {
        if (texture->surface) {
            texture->surface->lpVtbl->QueryInterface(texture->surface, &IID_IDirect3DTexture2,
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
        group->imText->lpVtbl->Release(group->imText);
        group->imText = NULL;
    }
}

// FUNCTION: LEGORR 0x00483170
int mesh_create_group_material(Mesh* mesh, unsigned long groupID)
{
    Mesh_D3DMaterial material = {
        sizeof(Mesh_D3DMaterial),
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
                            Mesh_D3DMaterial* material)
{

    mesh->groupList[groupID].material = *material;

    return 1;
}

// FUNCTION: LEGORR 0x00483270
int mesh_set_group_colour(Mesh* mesh, unsigned long groupID, float r,
                          float g, float b, unsigned long type)
{
    Mesh_Group* group = &mesh->groupList[groupID];
    Mesh_D3DMaterial* material = mesh_get_group_material(mesh, groupID);

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
Mesh_D3DMaterial* mesh_get_group_material(Mesh* mesh, unsigned long groupID)
{

    return &mesh->groupList[groupID].material;
}

// FUNCTION: LEGORR 0x00483420
int mesh_set_group_material_values(Mesh* mesh, unsigned long groupID,
                                   float value, unsigned long type)
{
    Mesh_Group* group = &mesh->groupList[groupID];
    Mesh_D3DMaterial* material = mesh_get_group_material(mesh, groupID);

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
int mesh_set_current_viewport(Mesh_RMViewport* lpView)
{
    Mesh_IMViewport* imview1;
    Mesh_IMViewport* imview;

    if (lpView->lpVtbl->GetDirect3DViewport(lpView, &imview1) != 0) return 0;

    if (imview1->lpVtbl->QueryInterface(imview1, &IID_IDirect3DViewport3, (void**)&imview) != 0) {
        if (imview1) imview1->lpVtbl->Release(imview1);
        return 0;
    }
    if (imview1) {
        imview1->lpVtbl->Release(imview1);
        imview1 = NULL;
    }
    if (lpIMDevice()->lpVtbl->SetCurrentViewport(lpIMDevice(), imview) != 0) {
        if (imview) imview->lpVtbl->Release(imview);
        return 0;
    }
    if (imview) imview->lpVtbl->Release(imview);

    return 1;
}

// FUNCTION: LEGORR 0x00483600
int mesh_set_current_gods_viewport(Mesh_Viewport* vp)
{
    Mesh_IMViewport* imview1;
    Mesh_IMViewport* imview;

    if (vp->lpVP->lpVtbl->GetDirect3DViewport(vp->lpVP, &imview1) != 0) return 0;

    if (imview1->lpVtbl->QueryInterface(imview1, &IID_IDirect3DViewport3, (void**)&imview) != 0) {
        if (imview1) imview1->lpVtbl->Release(imview1);
        return 0;
    }
    if (imview1) {
        imview1->lpVtbl->Release(imview1);
        imview1 = NULL;
    }
    if (lpIMDevice()->lpVtbl->SetCurrentViewport(lpIMDevice(), imview) != 0) {
        if (imview) imview->lpVtbl->Release(imview);
        return 0;
    }
    if (imview) imview->lpVtbl->Release(imview);

    return 1;
}

// FUNCTION: LEGORR 0x00483690
int mesh_set_transform(unsigned long state, Mesh_Matrix* mat)
{
    Mesh_Matrix oldMatrix;
    Mesh_Matrix newMatrix = *mat;

    lpIMDevice()->lpVtbl->GetTransform(lpIMDevice(), state, &oldMatrix);

    if (oldMatrix.m[0][0] != newMatrix.m[0][0] || oldMatrix.m[0][1] != newMatrix.m[0][1] ||
        oldMatrix.m[0][2] != newMatrix.m[0][2] || oldMatrix.m[0][3] != newMatrix.m[0][3] ||
        oldMatrix.m[1][0] != newMatrix.m[1][0] || oldMatrix.m[1][1] != newMatrix.m[1][1] ||
        oldMatrix.m[1][2] != newMatrix.m[1][2] || oldMatrix.m[1][3] != newMatrix.m[1][3] ||
        oldMatrix.m[2][0] != newMatrix.m[2][0] || oldMatrix.m[2][1] != newMatrix.m[2][1] ||
        oldMatrix.m[2][2] != newMatrix.m[2][2] || oldMatrix.m[2][3] != newMatrix.m[2][3] ||
        oldMatrix.m[3][0] != newMatrix.m[3][0] || oldMatrix.m[3][1] != newMatrix.m[3][1] ||
        oldMatrix.m[3][2] != newMatrix.m[3][2] || oldMatrix.m[3][3] != newMatrix.m[3][3]) {
        if (lpIMDevice()->lpVtbl->SetTransform(lpIMDevice(), state, mat) != 0) return 0;
    }

    return 1;
}

// FUNCTION: LEGORR 0x00483810
int mesh_set_texture_stage_state(unsigned long type, unsigned long value)
{
    unsigned long oldValue;

    if (lpIMDevice()->lpVtbl->GetTextureStageState(lpIMDevice(), 0, type, &oldValue) != 0) oldValue = -1;

    if (oldValue != value) {
        if (lpIMDevice()->lpVtbl->SetTextureStageState(lpIMDevice(), 0, type, value) != 0) return 0;
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
    lpIMDevice()->lpVtbl->GetLightState(lpIMDevice(), 1, &meshGlobs.oldMatIM);
    meshGlobs.currMatIM = meshGlobs.oldMatIM;

    if (!(s_engine_feature_word & 0x20)) {
        lpIMDevice()->lpVtbl->GetTexture(lpIMDevice(), 0, &meshGlobs.oldTextureIM);
        meshGlobs.currTextureIM = meshGlobs.oldTextureIM;
        lpIMDevice()->lpVtbl->GetRenderState(lpIMDevice(), 1, &meshGlobs.oldTextureRM);
        if (meshGlobs.oldTextureRM) lpIMDevice()->lpVtbl->SetRenderState(lpIMDevice(), 1, 0);
    } else {
        lpIMDevice()->lpVtbl->GetRenderState(lpIMDevice(), 1, &meshGlobs.oldTextureRM);
        meshGlobs.currTextureRM = meshGlobs.oldTextureRM;
    }
}

// FUNCTION: LEGORR 0x00483940
void mesh_restore_texture_and_mat(void)
{
    if (meshGlobs.currMatIM != meshGlobs.oldMatIM) {
        lpIMDevice()->lpVtbl->SetLightState(lpIMDevice(), 1, meshGlobs.oldMatIM);
    }

    if (!(s_engine_feature_word & 0x20)) {
        if (meshGlobs.currTextureIM != meshGlobs.oldTextureIM) {
            lpIMDevice()->lpVtbl->SetTexture(lpIMDevice(), 0, meshGlobs.oldTextureIM);
        }
        if (meshGlobs.oldTextureRM) lpIMDevice()->lpVtbl->SetRenderState(lpIMDevice(), 1, meshGlobs.oldTextureRM);
    } else {
        lpIMDevice()->lpVtbl->SetRenderState(lpIMDevice(), 1, meshGlobs.oldTextureRM);
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
int mesh_render_triangle_list(unsigned long matHandle, Mesh_Texture2* texture,
                              unsigned long renderFlags, void* vertices,
                              unsigned long vertexCount,
                              unsigned short* faceData,
                              unsigned long indexCount)
{
    Mesh_IMDevice2* dev2;
    int ok = 1;

    if (!(s_engine_feature_word & 0x20)) {
        if (texture != meshGlobs.currTextureIM) {
            if (lpIMDevice()->lpVtbl->SetTexture(lpIMDevice(), 0, texture) != 0) ok = 0;
            else meshGlobs.currTextureIM = texture;
        }
    } else {
        unsigned long handle = 0;
        if (texture) {
            lpIMDevice()->lpVtbl->QueryInterface(lpIMDevice(), &IID_IDirect3DDevice2, (void**)&dev2);
            texture->lpVtbl->GetHandle(texture, dev2, &handle);
            if (dev2) {
                dev2->lpVtbl->Release(dev2);
                dev2 = NULL;
            }
        }
        if (handle != meshGlobs.currTextureRM) {
            main_change_render_state(1, handle);
            meshGlobs.currTextureRM = handle;
        }
    }

    if (matHandle != meshGlobs.currMatIM) {
        if (lpIMDevice()->lpVtbl->SetLightState(lpIMDevice(), 1, matHandle) != 0) ok = 0;
        else meshGlobs.currMatIM = matHandle;
    }

    if (lpIMDevice()->lpVtbl->DrawIndexedPrimitive(lpIMDevice(), 4, renderFlags, vertices,
                                                   vertexCount, faceData, indexCount, 0) != 0) ok = 0;

    return ok;
}

#ifdef __cplusplus
}
#endif
