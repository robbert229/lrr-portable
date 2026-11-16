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

#include <miniwin/d3drm.h>

#define VIEWPORT_MAXLISTS 32
#define VIEWPORT_FLAG_INITIALISED 0x00000001

typedef struct VpArea2F {
    float x;
    float y;
    float width;
    float height;
} VpArea2F;

/* Container.c's Container: its master frame is the first word. */
typedef struct Container {
    LPDIRECT3DRMFRAME3 masterFrame;
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
    LPDIRECT3DRMVIEWPORT2 lpVP; /* +0x10 */
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

#define lpD3DRM() ((LPDIRECT3DRM3)s_main_d3drm)
#define lpDevice() ((LPDIRECT3DRMDEVICE3)s_main_device)

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

/* Main.c's mainGlobs fields under the port's names (finding 679). */
#include "gods98/main_globs.h"

// FUNCTION: LEGORR 0x00476dc0
Viewport* viewport_create(float xPos, float yPos, float width, float height,
                          Container* camera)
{
    unsigned long devWidth, devHeight;

    devWidth = lpDevice()->GetWidth();
    devHeight = lpDevice()->GetHeight();

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

    devWidth = lpDevice()->GetWidth();
    devHeight = lpDevice()->GetHeight();

    if (xPos < 0) xPos += devWidth;
    if (yPos < 0) yPos += devHeight;

    if (xPos + width <= devWidth && yPos + height <= devHeight) {
        if (viewportGlobs.freeList == NULL) viewport_add_list();

        newViewport = viewportGlobs.freeList;
        viewportGlobs.freeList = newViewport->nextFree;
        newViewport->nextFree = NULL;

        if (lpD3DRM()->CreateViewport(lpDevice(), camera->masterFrame, xPos,
                                      yPos, width, height,
                                      &newViewport->lpVP) == D3DRM_OK) {
            newViewport->lpVP->SetAppData(newViewport);
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
    if (width) *width = vp->lpVP->GetWidth();
    if (height) *height = vp->lpVP->GetHeight();
}

// FUNCTION: LEGORR 0x00476f50
void viewport_set_camera(Viewport* vp, Container* cont)
{
    vp->lpVP->SetCamera(cont->masterFrame);
}

// FUNCTION: LEGORR 0x00476f70
Container* viewport_get_camera(Viewport* vp)
{
    LPDIRECT3DRMFRAME3 frame;
    Container* cont = NULL;
    VpAppData* appData;

    if (vp->lpVP->GetCamera(&frame) == D3DRM_OK) {
        if (appData = (VpAppData*)frame->GetAppData()) {
            cont = appData->ownerContainer;
        }
        frame->Release();
    }

    return cont;
}

// FUNCTION: LEGORR 0x00476fb0
void viewport_set_back_clip(Viewport* vp, float dist)
{
    vp->lpVP->SetBack(dist);
}

// FUNCTION: LEGORR 0x00476fd0
float viewport_get_back_clip(Viewport* vp)
{
    return vp->lpVP->GetBack();
}

// FUNCTION: LEGORR 0x00476ff0
float viewport_get_front_clip(Viewport* vp)
{
    return vp->lpVP->GetFront();
}

// FUNCTION: LEGORR 0x00477010
void viewport_clear(Viewport* vp, int full)
{
    if (full) {
        LPDIRECT3DRMFRAME3 scene;
        unsigned long colour;
        VpArea2F area; /* a struct, so its floats stay in the frame */
        LPDIRECT3DVIEWPORT imViewport1;
        LPDIRECT3DVIEWPORT3 imViewport;

        scene = viewport_get_scene(vp);
        colour = 0;
        area.x = (float)vp->lpVP->GetX();
        area.y = (float)vp->lpVP->GetY();
        area.width = (float)vp->lpVP->GetWidth();
        area.height = (float)vp->lpVP->GetHeight();
        if (scene) colour = scene->GetSceneBackground();

        {
            /* In its own block, so it shares the width's conversion slot. */
            D3DRECT rect;

            rect.x1 = (long)area.x;
            rect.y1 = (long)area.y;
            rect.x2 = (long)(area.x + area.width);
            rect.y2 = (long)(area.y + area.height);

            vp->lpVP->GetDirect3DViewport(&imViewport1);
            imViewport1->QueryInterface(IID_IDirect3DViewport3,
                                        (void**)&imViewport);
            imViewport1->Release();
            imViewport->Clear2(1, &rect, 3, colour, 1.0f, 0);
            imViewport->Release();
        }
    } else {
        vp->lpVP->Clear(D3DRMCLEAR_ALL);
    }
}

// FUNCTION: LEGORR 0x00477150
void viewport_render(Viewport* vp, Container* root, float delta)
{
    HRESULT r;

    vp->rendering = 1;

    if (0.0f != vp->smoothFOV) {
        float smooth, sum, fov; /* smooth first: rule A (finding 538) */

        fov = vp->lpVP->GetField();
        smooth = (1.0f / delta) * 4.0f;
        sum = (smooth * fov) + vp->smoothFOV;
        fov = (1.0f / (smooth - -1.0f)) * sum;
        vp->lpVP->SetField(fov);
        if (fov == vp->smoothFOV) vp->smoothFOV = 0.0f;
    }

    if ((r = vp->lpVP->Render(root->masterFrame)) == D3DRM_OK) {
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
    dead->lpVP->Release();
}

// FUNCTION: LEGORR 0x00477240
void viewport_smooth_set_field(Viewport* vp, float fov)
{
    vp->smoothFOV = fov;
}

// FUNCTION: LEGORR 0x00477250
void viewport_set_field(Viewport* vp, float fov)
{
    vp->lpVP->SetField(fov);
    vp->smoothFOV = 0.0f;
}

// FUNCTION: LEGORR 0x00477270
float viewport_get_field(Viewport* vp)
{
    return vp->lpVP->GetField();
}

// FUNCTION: LEGORR 0x00477290
void viewport_inverse_transform(Viewport* vp, Vector4F* dest,
                                const Vector3F* src)
{
    vp->lpVP->InverseTransform((D3DVECTOR*)dest,
                               (D3DRMVECTOR4D*)const_cast<Vector3F*>(src));
}

// FUNCTION: LEGORR 0x004772b0
void viewport_transform(Viewport* vp, Vector4F* dest, const Vector3F* src)
{
    vp->lpVP->Transform((D3DRMVECTOR4D*)dest,
                        (D3DVECTOR*)const_cast<Vector3F*>(src));
}

// FUNCTION: LEGORR 0x004772d0
void viewport_world_to_screen(Viewport* vp, Point2F* screen,
                              const Vector3F* world)
{
    Vector4F v4d;

    vp->lpVP->Transform((D3DRMVECTOR4D*)&v4d,
                        (D3DVECTOR*)const_cast<Vector3F*>(world));
    screen->x = v4d.x / v4d.w;
    screen->y = v4d.y / v4d.w;
}

// FUNCTION: LEGORR 0x00477310
LPDIRECT3DRMFRAME3 viewport_get_scene(Viewport* vp)
{
    LPDIRECT3DRMFRAME3 camera = NULL;
    LPDIRECT3DRMFRAME3 scene = NULL;

    vp->lpVP->GetCamera(&camera);
    if (camera) {
        camera->Release();
        camera->GetScene(&scene);
        scene->Release();
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
                        tempViewport->lpVP->Release();
                    }
                }
            }
        }
    }
}
