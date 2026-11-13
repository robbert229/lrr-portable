// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit G98-003, Viewports.c (text 00476d50-00477440;
 * .rdata 0049e8ac-0049e8b8; .data 004aaba4; commons 0076ace0-0076ad6c).
 * Finding 593 (issue #624).
 *
 * gods98's viewports: a pool of 0x20-byte Viewport records, each wrapping
 * an IDirect3DRMViewport2 at +0x10, allocated in blocks of 1, 2, 4, ...
 * records (viewportGlobs.listSet), with a free list linked through +0x1c.
 * Viewport_Render eases the field of view towards a target set by
 * Viewport_SmoothSetField.
 *
 * Compiled as C in the matching build and as C++ in the native port. In the
 * matching build viewports_link_names.h gives most functions the port's
 * names, so their callers reach them without an edit; natively the port tail
 * (viewports_port.cpp) keeps serving those names. */
#include "viewports_link_names.h"

#include <stdlib.h>
#include "crt/vc5names.h"
#include "viewports.h"
#include "viewports_imports.h"
#include "dxerror.h"

#if defined(_WIN32)
#define VP_STDCALL __stdcall
#else
#define VP_STDCALL
#endif

#define VIEWPORT_MAXLISTS 32
#define VIEWPORT_FLAG_INITIALISED 0x00000001

typedef struct VpIID {
    unsigned long data1;
    unsigned short data2;
    unsigned short data3;
    unsigned char data4[8];
} VpIID;

typedef struct VpRect {
    long x1;
    long y1;
    long x2;
    long y2;
} VpRect;

typedef struct VpArea2F {
    float x;
    float y;
    float width;
    float height;
} VpArea2F;

typedef struct VpFrame VpFrame;           /* IDirect3DRMFrame3 */
typedef struct VpRMViewport VpRMViewport; /* IDirect3DRMViewport2 */
typedef struct VpIMViewport VpIMViewport; /* IDirect3DViewport3 */
typedef struct VpDevice VpDevice;         /* IDirect3DRMDevice3 */
typedef struct VpD3DRM VpD3DRM;           /* IDirect3DRM3 */

typedef struct VpFrameVtbl {
    void* slots_00[2];
    unsigned long (VP_STDCALL* Release)(VpFrame*);              /* +0x08 */
    void* slots_0c[4];
    unsigned long (VP_STDCALL* GetAppData)(VpFrame*);           /* +0x1c */
    void* slots_20[18];
    long (VP_STDCALL* GetScene)(VpFrame*, VpFrame**);           /* +0x68 */
    void* slots_6c[14];
    unsigned long (VP_STDCALL* GetSceneBackground)(VpFrame*);   /* +0xa4 */
} VpFrameVtbl;

struct VpFrame {
    const VpFrameVtbl* lpVtbl;
};

typedef struct VpRMViewportVtbl {
    void* slots_00[2];
    unsigned long (VP_STDCALL* Release)(VpRMViewport*);         /* +0x08 */
    void* slots_0c[3];
    long (VP_STDCALL* SetAppData)(VpRMViewport*, unsigned long); /* +0x18 */
    void* slots_1c[5];
    long (VP_STDCALL* Clear)(VpRMViewport*, unsigned long);     /* +0x30 */
    long (VP_STDCALL* Render)(VpRMViewport*, VpFrame*);         /* +0x34 */
    void* slots_38[1];
    long (VP_STDCALL* SetBack)(VpRMViewport*, float);           /* +0x3c */
    long (VP_STDCALL* SetField)(VpRMViewport*, float);          /* +0x40 */
    void* slots_44[1];
    long (VP_STDCALL* SetCamera)(VpRMViewport*, VpFrame*);      /* +0x48 */
    void* slots_4c[1];
    long (VP_STDCALL* Transform)(VpRMViewport*, float*,
                                 const float*);                 /* +0x50 */
    long (VP_STDCALL* InverseTransform)(VpRMViewport*, float*,
                                        const float*);          /* +0x54 */
    void* slots_58[3];
    long (VP_STDCALL* GetCamera)(VpRMViewport*, VpFrame**);     /* +0x64 */
    void* slots_68[4];
    long (VP_STDCALL* GetX)(VpRMViewport*);                     /* +0x78 */
    long (VP_STDCALL* GetY)(VpRMViewport*);                     /* +0x7c */
    unsigned long (VP_STDCALL* GetWidth)(VpRMViewport*);        /* +0x80 */
    unsigned long (VP_STDCALL* GetHeight)(VpRMViewport*);       /* +0x84 */
    float (VP_STDCALL* GetField)(VpRMViewport*);                /* +0x88 */
    float (VP_STDCALL* GetBack)(VpRMViewport*);                 /* +0x8c */
    float (VP_STDCALL* GetFront)(VpRMViewport*);                /* +0x90 */
    void* slots_94[1];
    long (VP_STDCALL* GetDirect3DViewport)(VpRMViewport*,
                                           VpIMViewport**);     /* +0x98 */
} VpRMViewportVtbl;

struct VpRMViewport {
    const VpRMViewportVtbl* lpVtbl;
};

typedef struct VpIMViewportVtbl {
    long (VP_STDCALL* QueryInterface)(VpIMViewport*, const VpIID*,
                                      void**);                  /* +0x00 */
    void* slots_04[1];
    unsigned long (VP_STDCALL* Release)(VpIMViewport*);         /* +0x08 */
    void* slots_0c[17];
    long (VP_STDCALL* Clear2)(VpIMViewport*, unsigned long, VpRect*,
                              unsigned long, unsigned long, float,
                              unsigned long);                   /* +0x50 */
} VpIMViewportVtbl;

struct VpIMViewport {
    const VpIMViewportVtbl* lpVtbl;
};

typedef struct VpDeviceVtbl {
    void* slots_00[26];
    unsigned long (VP_STDCALL* GetHeight)(VpDevice*);           /* +0x68 */
    unsigned long (VP_STDCALL* GetWidth)(VpDevice*);            /* +0x6c */
} VpDeviceVtbl;

struct VpDevice {
    const VpDeviceVtbl* lpVtbl;
};

typedef struct VpD3DRMVtbl {
    void* slots_00[20];
    long (VP_STDCALL* CreateViewport)(VpD3DRM*, VpDevice*, VpFrame*,
                                      unsigned long, unsigned long,
                                      unsigned long, unsigned long,
                                      VpRMViewport**);          /* +0x50 */
} VpD3DRMVtbl;

struct VpD3DRM {
    const VpD3DRMVtbl* lpVtbl;
};

/* Container.c's Container: its master frame is the first word. */
typedef struct Container {
    VpFrame* masterFrame;
} Container;

/* Container.c's frame app data: its owning container is the first word. */
typedef struct VpAppData {
    Container* ownerContainer;
} VpAppData;

typedef struct Viewport {
    float xoffset;            /* +0x00 */
    float yoffset;            /* +0x04 */
    float width;              /* +0x08 */
    float height;             /* +0x0c */
    VpRMViewport* lpVP;       /* +0x10 */
    float smoothFOV;          /* +0x14 */
    int rendering;            /* +0x18 */
    struct Viewport* nextFree; /* +0x1c */
} Viewport;

typedef struct Viewport_Globs {
    Viewport* listSet[VIEWPORT_MAXLISTS]; /* +0x00 */
    Viewport* freeList;                   /* +0x80 */
    unsigned long listCount;              /* +0x84 */
    unsigned long flags;                  /* +0x88 */
} Viewport_Globs;

typedef struct Vector3F {
    float x;
    float y;
    float z;
} Vector3F;

typedef struct Vector4F {
    float x;
    float y;
    float z;
    float w;
} Vector4F;

typedef struct Point2F {
    float x;
    float y;
} Point2F;

#define lpD3DRM() ((VpD3DRM*)s_main_d3drm)
#define lpDevice() ((VpDevice*)s_main_device)

/* IID_IDirect3DViewport3, shared with Mesh.c (G98-015). */
// GLOBAL: LEGORR 0x0049f778
static const VpIID IID_IDirect3DViewport3 = {
    0xb0ab3b61, 0x33d7, 0x11d1,
    { 0xa9, 0x81, 0x00, 0xc0, 0x4f, 0xd7, 0xb1, 0x74 }
};

/* The unit's state is one common symbol, outside every unit's attributed
 * .bss (finding 505), so this is a tentative definition. */
// GLOBAL: LEGORR 0x0076ace0
Viewport_Globs viewportGlobs;

// FUNCTION: LEGORR 0x00476d50
void viewport_initialise(void)
{
    unsigned long loop;

    for (loop = 0; loop < VIEWPORT_MAXLISTS; loop++) {
        viewportGlobs.listSet[loop] = NULL;
    }
    viewportGlobs.freeList = NULL;
    viewportGlobs.listCount = 0;
    viewportGlobs.flags = VIEWPORT_FLAG_INITIALISED;
}

// FUNCTION: LEGORR 0x00476d80
void viewport_shutdown(void)
{
    unsigned long loop;

    viewport_remove_all();
    for (loop = 0; loop < VIEWPORT_MAXLISTS; loop++) {
        if (viewportGlobs.listSet[loop]) free(viewportGlobs.listSet[loop]);
    }
    viewportGlobs.freeList = NULL;
    viewportGlobs.flags = 0;
}

// FUNCTION: LEGORR 0x00476dc0
Viewport* viewport_create(float xPos, float yPos, float width, float height,
                          Container* camera)
{
    unsigned long devWidth, devHeight;

    devWidth = lpDevice()->lpVtbl->GetWidth(lpDevice());
    devHeight = lpDevice()->lpVtbl->GetHeight(lpDevice());

    return viewport_create_pixel((long)(xPos * devWidth),
                                 (long)(yPos * devHeight),
                                 (unsigned long)(width * devWidth),
                                 (unsigned long)(height * devHeight), camera);
}

// FUNCTION: LEGORR 0x00476e50
Viewport* viewport_create_pixel(long xPos, long yPos, unsigned long width,
                                unsigned long height, Container* camera)
{
    unsigned long devWidth, devHeight;
    Viewport* newViewport;

    devWidth = lpDevice()->lpVtbl->GetWidth(lpDevice());
    devHeight = lpDevice()->lpVtbl->GetHeight(lpDevice());

    if (xPos < 0) xPos += devWidth;
    if (yPos < 0) yPos += devHeight;

    if (xPos + width <= devWidth && yPos + height <= devHeight) {
        if (viewportGlobs.freeList == NULL) viewport_add_list();

        newViewport = viewportGlobs.freeList;
        viewportGlobs.freeList = newViewport->nextFree;
        newViewport->nextFree = NULL;

        if (lpD3DRM()->lpVtbl->CreateViewport(lpD3DRM(), lpDevice(),
                                              camera->masterFrame, xPos, yPos,
                                              width, height,
                                              &newViewport->lpVP) == 0) {
            newViewport->lpVP->lpVtbl->SetAppData(newViewport->lpVP,
                                                  (unsigned long)newViewport);
            newViewport->smoothFOV = 0.0f;
            return newViewport;
        }
    }

    return NULL;
}

// FUNCTION: LEGORR 0x00476f10
void viewport_get_size(Viewport* vp, unsigned long* width,
                       unsigned long* height)
{
    if (width) *width = vp->lpVP->lpVtbl->GetWidth(vp->lpVP);
    if (height) *height = vp->lpVP->lpVtbl->GetHeight(vp->lpVP);
}

// FUNCTION: LEGORR 0x00476f50
void viewport_set_camera(Viewport* vp, Container* cont)
{
    vp->lpVP->lpVtbl->SetCamera(vp->lpVP, cont->masterFrame);
}

// FUNCTION: LEGORR 0x00476f70
Container* viewport_get_camera(Viewport* vp)
{
    VpFrame* frame;
    Container* cont = NULL;
    VpAppData* appData;

    if (vp->lpVP->lpVtbl->GetCamera(vp->lpVP, &frame) == 0) {
        if (appData = (VpAppData*)frame->lpVtbl->GetAppData(frame)) {
            cont = appData->ownerContainer;
        }
        frame->lpVtbl->Release(frame);
    }

    return cont;
}

// FUNCTION: LEGORR 0x00476fb0
void viewport_set_back_clip(Viewport* vp, float dist)
{
    vp->lpVP->lpVtbl->SetBack(vp->lpVP, dist);
}

// FUNCTION: LEGORR 0x00476fd0
float viewport_get_back_clip(Viewport* vp)
{
    return vp->lpVP->lpVtbl->GetBack(vp->lpVP);
}

// FUNCTION: LEGORR 0x00476ff0
float viewport_get_front_clip(Viewport* vp)
{
    return vp->lpVP->lpVtbl->GetFront(vp->lpVP);
}

// FUNCTION: LEGORR 0x00477010
void viewport_clear(Viewport* vp, int full)
{
    if (full) {
        VpFrame* scene;
        unsigned long colour;
        VpArea2F area; /* a struct, so its floats stay in the frame */
        VpIMViewport* imViewport1;
        VpIMViewport* imViewport;

        scene = viewport_get_scene(vp);
        colour = 0;
        area.x = (float)vp->lpVP->lpVtbl->GetX(vp->lpVP);
        area.y = (float)vp->lpVP->lpVtbl->GetY(vp->lpVP);
        area.width = (float)vp->lpVP->lpVtbl->GetWidth(vp->lpVP);
        area.height = (float)vp->lpVP->lpVtbl->GetHeight(vp->lpVP);
        if (scene) colour = scene->lpVtbl->GetSceneBackground(scene);

        {
            /* In its own block, so it shares the width's conversion slot. */
            VpRect rect;

            rect.x1 = (long)area.x;
            rect.y1 = (long)area.y;
            rect.x2 = (long)(area.x + area.width);
            rect.y2 = (long)(area.y + area.height);

            vp->lpVP->lpVtbl->GetDirect3DViewport(vp->lpVP, &imViewport1);
            imViewport1->lpVtbl->QueryInterface(imViewport1,
                                                &IID_IDirect3DViewport3,
                                                (void**)&imViewport);
            imViewport1->lpVtbl->Release(imViewport1);
            imViewport->lpVtbl->Clear2(imViewport, 1, &rect, 3, colour, 1.0f,
                                       0);
            imViewport->lpVtbl->Release(imViewport);
        }
    } else {
        vp->lpVP->lpVtbl->Clear(vp->lpVP, 7);
    }
}

// FUNCTION: LEGORR 0x00477150
void viewport_render(Viewport* vp, Container* root, float delta)
{
    long r;

    vp->rendering = 1;

    if (0.0f != vp->smoothFOV) {
        float smooth, sum, fov; /* smooth first: rule A (finding 538) */

        fov = vp->lpVP->lpVtbl->GetField(vp->lpVP);
        smooth = (1.0f / delta) * 4.0f;
        sum = (smooth * fov) + vp->smoothFOV;
        fov = (1.0f / (smooth - -1.0f)) * sum;
        vp->lpVP->lpVtbl->SetField(vp->lpVP, fov);
        if (fov == vp->smoothFOV) vp->smoothFOV = 0.0f;
    }

    if ((r = vp->lpVP->lpVtbl->Render(vp->lpVP, root->masterFrame)) == 0) {
        model_render_pass_dispatch(vp);
    } else {
        directx_result_diagnostic_dispatch(r, DXERROR_MODULE_D3DRM,
            // STRING: LEGORR 0x004aaba4
            "C:\\Dev\\SourceSafe\\gods98_dx6\\gods98\\src\\Viewports.c", 286);
    }

    vp->rendering = 0;
}

// FUNCTION: LEGORR 0x00477220
void viewport_remove(Viewport* dead)
{
    dead->nextFree = viewportGlobs.freeList;
    viewportGlobs.freeList = dead;
    dead->lpVP->lpVtbl->Release(dead->lpVP);
}

// FUNCTION: LEGORR 0x00477240
void viewport_smooth_set_field(Viewport* vp, float fov)
{
    vp->smoothFOV = fov;
}

// FUNCTION: LEGORR 0x00477250
void viewport_set_field(Viewport* vp, float fov)
{
    vp->lpVP->lpVtbl->SetField(vp->lpVP, fov);
    vp->smoothFOV = 0.0f;
}

// FUNCTION: LEGORR 0x00477270
float viewport_get_field(Viewport* vp)
{
    return vp->lpVP->lpVtbl->GetField(vp->lpVP);
}

// FUNCTION: LEGORR 0x00477290
void viewport_inverse_transform(Viewport* vp, Vector4F* dest,
                                const Vector3F* src)
{
    vp->lpVP->lpVtbl->InverseTransform(vp->lpVP, (float*)dest,
                                       (const float*)src);
}

// FUNCTION: LEGORR 0x004772b0
void viewport_transform(Viewport* vp, Vector4F* dest, const Vector3F* src)
{
    vp->lpVP->lpVtbl->Transform(vp->lpVP, (float*)dest, (const float*)src);
}

// FUNCTION: LEGORR 0x004772d0
void viewport_world_to_screen(Viewport* vp, Point2F* screen,
                              const Vector3F* world)
{
    Vector4F v4d;

    vp->lpVP->lpVtbl->Transform(vp->lpVP, (float*)&v4d, (const float*)world);
    screen->x = v4d.x / v4d.w;
    screen->y = v4d.y / v4d.w;
}

// FUNCTION: LEGORR 0x00477310
VpFrame* viewport_get_scene(Viewport* vp)
{
    VpFrame* camera = NULL;
    VpFrame* scene = NULL;

    vp->lpVP->lpVtbl->GetCamera(vp->lpVP, &camera);
    if (camera) {
        camera->lpVtbl->Release(camera);
        camera->lpVtbl->GetScene(camera, &scene);
        scene->lpVtbl->Release(scene);
    }

    return scene;
}

// FUNCTION: LEGORR 0x00477370
void viewport_add_list(void)
{
    Viewport* list;
    unsigned long loop, count;

    count = 0x00000001 << viewportGlobs.listCount;

    if (list = viewportGlobs.listSet[viewportGlobs.listCount] =
            (Viewport*)malloc(sizeof(Viewport) * count)) {
        viewportGlobs.listCount++;

        for (loop = 1; loop < count; loop++) {
            list[loop - 1].nextFree = &list[loop];
        }
        list[count - 1].nextFree = viewportGlobs.freeList;
        viewportGlobs.freeList = list;
    }
}

// FUNCTION: LEGORR 0x004773e0
void viewport_remove_all(void)
{
    unsigned long list, count, loop;
    Viewport* tempViewport;

    for (list = 0; list < viewportGlobs.listCount; list++) {
        if (viewportGlobs.listSet[list]) {
            count = 0x00000001 << list;
            for (loop = 0; loop < count; loop++) {
                if (tempViewport = &viewportGlobs.listSet[list][loop]) {
                    if (tempViewport->nextFree == tempViewport) {
                        tempViewport->lpVP->lpVtbl->Release(tempViewport->lpVP);
                    }
                }
            }
        }
    }
}
