// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit G98-025, AnimClone.c (text 004894c0-00489c30;
 * no .data, .rdata or .bss of its own; findings 505 and 616).
 *
 * An AnimClone is a Direct3DRM frame hierarchy that an animation set or an
 * Lws.c scene drives. AnimClone_Register and AnimClone_RegisterLws record
 * the original's root and a flat array of its frames. AnimClone_Make builds
 * a clone: an Lws scene clones itself (Lws_Clone); an animation-set
 * hierarchy is copied frame by frame (AnimClone_CreateCopy), its visuals
 * shared or, for LightWave meshes, cloned. AnimClone_SetTime poses the
 * original at the time and copies each frame's transform to the clone.
 * Material_Create, which OpenLRR keeps in a file of its own, sits at the
 * start: no data, literal or call separates it from this object (finding
 * 616).
 *
 * The port's model of the unit stays in the port tail,
 * gods98/com_object_tree_port.cpp, unannotated. In the matching build
 * animclone_link_names.h gives three of these functions the port's names,
 * so their callers reach them; natively the port tail serves those names.
 *
 * The frames, frame arrays, visuals, animation sets, materials and
 * IDirect3DRM3 are miniwin's DirectX 6 interfaces (ADR 0007 decision 3). */
#include "animclone_link_names.h"

#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "animclone.h"
#include "lws.h"
#include "mesh_unit.h"
#include "main.h"

#include <miniwin/d3drm.h>

/* The most visuals a frame of a cloned hierarchy carries. */
#define ANIMCLONE_MAXVISUALS 4

/* Main.c's lpD3DRM() (mainGlobs.lpD3DRM, 00505914). */
#define lpD3DRM() ((LPDIRECT3DRM3)s_main_d3drm)

struct AnimClone {
    struct AnimClone* clonedFrom;        /* +0x00 */
    struct LwsInfo* scene;               /* +0x04 */
    LPDIRECT3DRMANIMATIONSET2 animSet;           /* +0x08 */
    int lws;                             /* +0x0c */
    LPDIRECT3DRMFRAME3 root;                /* +0x10 */
    LPDIRECT3DRMFRAME3* partArray;          /* +0x14 */
    unsigned long partCount;             /* +0x18 */
    unsigned long frameCount;            /* +0x1c */
};

static int anim_clone_frame_count_callback(LPDIRECT3DRMFRAME3 frame,
                                           void* data);
static int anim_clone_setup_frame_array_callback(LPDIRECT3DRMFRAME3 frame,
                                                 void* data);

/* Main.c's mainGlobs fields under the port's names (finding 679). */
#include "gods98/main_globs.h"

// FUNCTION: LEGORR 0x004894c0
void* material_create(float emissiveRed, float emissiveGreen,
                      float emissiveBlue, float specularRed,
                      float specularGreen, float specularBlue, float power)
{
    LPDIRECT3DRMMATERIAL2 material;

    if (lpD3DRM()->CreateMaterial(power, &material) == D3DRM_OK) {
        material->SetEmissive(emissiveRed, emissiveGreen, emissiveBlue);
        material->SetSpecular(specularRed, specularGreen, specularBlue);
        return material;
    }

    return NULL;
}

// FUNCTION: LEGORR 0x00489520
AnimClone* anim_clone_register(LPDIRECT3DRMANIMATIONSET2 animSet,
                               LPDIRECT3DRMFRAME3 root,
                               unsigned long frameCount)
{
    AnimClone* orig = (AnimClone*)malloc(sizeof(AnimClone));
    AnimClone data;

    orig->clonedFrom = NULL;
    orig->animSet = animSet;
    orig->scene = NULL;
    orig->lws = 0;
    orig->root = root;
    root->AddRef();
    orig->partCount = 0;
    orig->frameCount = frameCount;

    anim_clone_walk_tree(root, 0, anim_clone_frame_count_callback,
                         &orig->partCount);
    orig->partArray = (LPDIRECT3DRMFRAME3*)malloc(
        sizeof(LPDIRECT3DRMFRAME3) * orig->partCount);
    data.partArray = orig->partArray;
    data.partCount = 0;
    anim_clone_walk_tree(root, 0, anim_clone_setup_frame_array_callback,
                         &data);

    return orig;
}

// FUNCTION: LEGORR 0x004895c0
AnimClone* anim_clone_register_lws(struct LwsInfo* scene,
                                   LPDIRECT3DRMFRAME3 root,
                                   unsigned long frameCount)
{
    AnimClone* orig = (AnimClone*)malloc(sizeof(AnimClone));
    AnimClone data;

    orig->clonedFrom = NULL;
    orig->animSet = NULL;
    orig->scene = scene;
    orig->lws = 1;
    orig->root = root;
    root->AddRef();
    orig->partCount = 0;
    orig->frameCount = frameCount;

    anim_clone_walk_tree(root, 0, anim_clone_frame_count_callback,
                         &orig->partCount);
    orig->partArray = (LPDIRECT3DRMFRAME3*)malloc(
        sizeof(LPDIRECT3DRMFRAME3) * orig->partCount);
    data.partArray = orig->partArray;
    data.partCount = 0;
    anim_clone_walk_tree(root, 0, anim_clone_setup_frame_array_callback,
                         &data);

    return orig;
}

// FUNCTION: LEGORR 0x00489660
AnimClone* anim_clone_make(AnimClone* orig, LPDIRECT3DRMFRAME3 parent,
                           unsigned long* frameCount)
{
    AnimClone* clone = (AnimClone*)malloc(sizeof(AnimClone));
    AnimClone data;

    if (orig->lws) {
        *clone = *orig;
        clone->scene = lws_clone(orig->scene, parent);
        clone->clonedFrom = NULL;
        if (frameCount) *frameCount = orig->frameCount;
    } else {
        memset(clone, 0, sizeof(AnimClone));
        orig->animSet->SetTime(0.0f);
        clone->animSet = NULL;
        clone->clonedFrom = orig;
        lpD3DRM()->CreateFrame(parent, &clone->root);
        clone->partArray = (LPDIRECT3DRMFRAME3*)malloc(
            sizeof(LPDIRECT3DRMFRAME3) * orig->partCount);
        anim_clone_create_copy(orig->root, clone->root, orig->lws);
        clone->partCount = orig->partCount;
        if (frameCount) *frameCount = orig->frameCount;

        data.partArray = clone->partArray;
        data.partCount = 0;
        anim_clone_walk_tree(clone->root, 0,
                             anim_clone_setup_frame_array_callback, &data);
    }

    return clone;
}

// FUNCTION: LEGORR 0x00489750
void anim_clone_remove(AnimClone* dead)
{
    unsigned long loop;

    if (dead) {
        if (NULL == dead->clonedFrom) {
            if (dead->lws) {
                lws_free(dead->scene);
            } else {
                dead->animSet->Release();
                dead->root->Release();
            }
        }

        if (!dead->lws) {
            for (loop = 0; loop < dead->partCount; loop++) {
                dead->partArray[loop]->Release();
            }
            free(dead->partArray);
        }

        free(dead);
    }
}

// FUNCTION: LEGORR 0x004897d0
int anim_clone_is_lws(AnimClone* clone)
{
    return clone->lws;
}

// FUNCTION: LEGORR 0x004897e0
void anim_clone_set_time(AnimClone* clone, float time, float* oldTime)
{
    AnimClone* orig = clone->clonedFrom;
    unsigned long loop;

    if (orig) {
        if (orig->lws) lws_time_set(orig->scene, time);
        else orig->animSet->SetTime(time);

        for (loop = 0; loop < clone->partCount; loop++) {
            LPDIRECT3DRMFRAME3 parent;
            float mat[4][4];

            orig->partArray[loop]->GetParent(&parent);
            orig->partArray[loop]->GetTransform(parent, mat);
            parent->Release();
            clone->partArray[loop]->AddTransform(D3DRMCOMBINE_REPLACE, mat);
        }

        if (oldTime) {
            if (orig->lws) lws_time_set(orig->scene, *oldTime);
            else orig->animSet->SetTime(*oldTime);
        }
    } else {
        if (clone->lws) lws_time_set(clone->scene, time);
        else clone->animSet->SetTime(time);
    }
}

// FUNCTION: LEGORR 0x004898e0
static int anim_clone_frame_count_callback(LPDIRECT3DRMFRAME3 frame,
                                           void* data)
{
    unsigned long* count = (unsigned long*)data;

    (*count)++;

    return 0;
}

// FUNCTION: LEGORR 0x004898f0
static int anim_clone_setup_frame_array_callback(LPDIRECT3DRMFRAME3 frame,
                                                 void* data)
{
    AnimClone* clone = (AnimClone*)data;

    clone->partArray[clone->partCount] = frame;
    clone->partCount++;

    return 0;
}

// FUNCTION: LEGORR 0x00489910
int anim_clone_walk_tree(LPDIRECT3DRMFRAME3 frame, unsigned long level,
                         AnimCloneWalkTreeCallback Callback, void* data)
{
    LPDIRECT3DRMFRAMEARRAY children;
    unsigned long loop, count;
    int finished = 0;

    if (Callback(frame, data)) return 1;

    if (frame->GetChildren(&children) == D3DRM_OK) {
        count = children->GetSize();
        for (loop = 0; loop < count; loop++) {
            LPDIRECT3DRMFRAME child1;
            LPDIRECT3DRMFRAME3 child;

            children->GetElement(loop, &child1);
            child1->QueryInterface(IID_IDirect3DRMFrame3, (void**)&child);
            child1->Release();
            if (anim_clone_walk_tree(child, level + 1, Callback, data)) {
                finished = 1;
                child->Release();
                break;
            }
            child->Release();
        }
        children->Release();
    }

    return finished;
}

// FUNCTION: LEGORR 0x004899f0
void anim_clone_create_copy(LPDIRECT3DRMFRAME3 orig,
                            LPDIRECT3DRMFRAME3 clone, int lws)
{
    LPDIRECT3DRMFRAMEARRAY children;
    LPDIRECT3DRMFRAME3 parent;
    unsigned long count, loop;
    DWORD size;
    char* name;
    float mat[4][4];

    if (lws) anim_clone_clone_lwo_meshes(orig, clone);
    else anim_clone_reference_visuals(orig, clone);

    orig->GetName(&size, NULL);
    if (size) {
        name = (char*)malloc(size);
        orig->GetName(&size, name);
        clone->SetName(name);
        free(name);
    }

    orig->GetParent(&parent);
    orig->GetTransform(parent, mat);
    parent->Release();
    clone->AddTransform(D3DRMCOMBINE_REPLACE, mat);

    if (orig->GetChildren(&children) == D3DRM_OK) {
        count = children->GetSize();
        for (loop = 0; loop < count; loop++) {
            LPDIRECT3DRMFRAME child1;
            LPDIRECT3DRMFRAME3 child;
            LPDIRECT3DRMFRAME3 newFrame;

            children->GetElement(loop, &child1);
            child1->QueryInterface(IID_IDirect3DRMFrame3, (void**)&child);
            child1->Release();
            lpD3DRM()->CreateFrame(clone, &newFrame);
            anim_clone_create_copy(child, newFrame, lws);
            newFrame->Release();
            child->Release();
        }
        children->Release();
    }
}

// FUNCTION: LEGORR 0x00489b30
void anim_clone_clone_lwo_meshes(LPDIRECT3DRMFRAME3 orig,
                                 LPDIRECT3DRMFRAME3 clone)
{
    IUnknown* visual[ANIMCLONE_MAXVISUALS];
    LPDIRECT3DRMUSERVISUAL userVisual;
    DWORD count, loop;

    orig->GetVisuals(&count, NULL);
    if (count) {
        orig->GetVisuals(&count, visual);
        for (loop = 0; loop < count; loop++) {
            if (visual[loop]->QueryInterface(IID_IDirect3DRMUserVisual,
                                             (void**)&userVisual) == D3DRM_OK) {
                void* mesh = userVisual->GetAppData();
                mesh_clone(mesh, clone);
            }
        }
    }
}

// FUNCTION: LEGORR 0x00489bc0
void anim_clone_reference_visuals(LPDIRECT3DRMFRAME3 orig,
                                  LPDIRECT3DRMFRAME3 clone)
{
    IUnknown* visual[ANIMCLONE_MAXVISUALS];
    DWORD count, loop;

    orig->GetVisuals(&count, NULL);
    if (count) {
        orig->GetVisuals(&count, visual);
        for (loop = 0; loop < count; loop++) {
            clone->AddVisual(visual[loop]);
        }
    }
}
