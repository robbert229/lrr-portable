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
 * C-compatible (ADR 0005): C in the matching build, C++ elsewhere. The
 * frames, frame arrays, visuals, animation sets, materials and
 * IDirect3DRM3 are called through their COM method tables, declared here
 * with the slots the unit uses. */
#include "animclone_link_names.h"

#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "animclone.h"
#include "lws.h"
#include "mesh_unit.h"
#include "main.h"

#if defined(_WIN32)
#define ANIMCLONE_STDCALL __stdcall
#else
#define ANIMCLONE_STDCALL
#endif

/* The most visuals a frame of a cloned hierarchy carries. */
#define ANIMCLONE_MAXVISUALS 4

/* D3DRMCOMBINE_REPLACE. */
#define ANIMCLONE_COMBINE_REPLACE 0

typedef struct AnimCloneIID {
    unsigned long data1;
    unsigned short data2;
    unsigned short data3;
    unsigned char data4[8];
} AnimCloneIID;

typedef struct AnimCloneFrame AnimCloneFrame;          /* IDirect3DRMFrame3 */
typedef struct AnimCloneFrame1 AnimCloneFrame1;        /* IDirect3DRMFrame */
typedef struct AnimCloneFrameArray AnimCloneFrameArray; /* IDirect3DRMFrameArray */
typedef struct AnimCloneAnimSet AnimCloneAnimSet;      /* IDirect3DRMAnimationSet2 */
typedef struct AnimCloneUnknown AnimCloneUnknown;      /* IUnknown */
typedef struct AnimCloneUserVisual AnimCloneUserVisual; /* IDirect3DRMUserVisual */
typedef struct AnimCloneMaterial AnimCloneMaterial;    /* IDirect3DRMMaterial2 */
typedef struct AnimCloneD3DRM AnimCloneD3DRM;          /* IDirect3DRM3 */

typedef struct AnimCloneFrameVtbl {
    long (ANIMCLONE_STDCALL* QueryInterface)(AnimCloneFrame*,
                                             const AnimCloneIID*, void**);
    unsigned long (ANIMCLONE_STDCALL* AddRef)(AnimCloneFrame*);   /* +0x04 */
    unsigned long (ANIMCLONE_STDCALL* Release)(AnimCloneFrame*);  /* +0x08 */
    void* slots_0c[5];
    long (ANIMCLONE_STDCALL* SetName)(AnimCloneFrame*, const char*); /* +0x20 */
    long (ANIMCLONE_STDCALL* GetName)(AnimCloneFrame*, unsigned long*,
                                      char*);                     /* +0x24 */
    void* slots_28[4];
    long (ANIMCLONE_STDCALL* AddTransform)(AnimCloneFrame*, int,
                                           float (*)[4]);         /* +0x38 */
    void* slots_3c[3];
    long (ANIMCLONE_STDCALL* AddVisual)(AnimCloneFrame*,
                                        AnimCloneUnknown*);       /* +0x48 */
    long (ANIMCLONE_STDCALL* GetChildren)(AnimCloneFrame*,
                                          AnimCloneFrameArray**); /* +0x4c */
    void* slots_50[3];
    long (ANIMCLONE_STDCALL* GetParent)(AnimCloneFrame*,
                                        AnimCloneFrame**);        /* +0x5c */
    void* slots_60[5];
    long (ANIMCLONE_STDCALL* GetTransform)(AnimCloneFrame*, AnimCloneFrame*,
                                           float (*)[4]);         /* +0x74 */
    void* slots_78[2];
    long (ANIMCLONE_STDCALL* GetVisuals)(AnimCloneFrame*, unsigned long*,
                                         AnimCloneUnknown**);     /* +0x80 */
} AnimCloneFrameVtbl;
struct AnimCloneFrame {
    const AnimCloneFrameVtbl* lpVtbl;
};

typedef struct AnimCloneFrame1Vtbl {
    long (ANIMCLONE_STDCALL* QueryInterface)(AnimCloneFrame1*,
                                             const AnimCloneIID*, void**);
    unsigned long (ANIMCLONE_STDCALL* AddRef)(AnimCloneFrame1*);
    unsigned long (ANIMCLONE_STDCALL* Release)(AnimCloneFrame1*); /* +0x08 */
} AnimCloneFrame1Vtbl;
struct AnimCloneFrame1 {
    const AnimCloneFrame1Vtbl* lpVtbl;
};

typedef struct AnimCloneFrameArrayVtbl {
    long (ANIMCLONE_STDCALL* QueryInterface)(AnimCloneFrameArray*,
                                             const AnimCloneIID*, void**);
    unsigned long (ANIMCLONE_STDCALL* AddRef)(AnimCloneFrameArray*);
    unsigned long (ANIMCLONE_STDCALL* Release)(AnimCloneFrameArray*); /* +0x08 */
    unsigned long (ANIMCLONE_STDCALL* GetSize)(AnimCloneFrameArray*); /* +0x0c */
    long (ANIMCLONE_STDCALL* GetElement)(AnimCloneFrameArray*, unsigned long,
                                         AnimCloneFrame1**);      /* +0x10 */
} AnimCloneFrameArrayVtbl;
struct AnimCloneFrameArray {
    const AnimCloneFrameArrayVtbl* lpVtbl;
};

typedef struct AnimCloneAnimSetVtbl {
    long (ANIMCLONE_STDCALL* QueryInterface)(AnimCloneAnimSet*,
                                             const AnimCloneIID*, void**);
    unsigned long (ANIMCLONE_STDCALL* AddRef)(AnimCloneAnimSet*);
    unsigned long (ANIMCLONE_STDCALL* Release)(AnimCloneAnimSet*); /* +0x08 */
    void* slots_0c[11];
    long (ANIMCLONE_STDCALL* SetTime)(AnimCloneAnimSet*, float);  /* +0x38 */
} AnimCloneAnimSetVtbl;
struct AnimCloneAnimSet {
    const AnimCloneAnimSetVtbl* lpVtbl;
};

typedef struct AnimCloneUnknownVtbl {
    long (ANIMCLONE_STDCALL* QueryInterface)(AnimCloneUnknown*,
                                             const AnimCloneIID*, void**);
    unsigned long (ANIMCLONE_STDCALL* AddRef)(AnimCloneUnknown*);
    unsigned long (ANIMCLONE_STDCALL* Release)(AnimCloneUnknown*);
} AnimCloneUnknownVtbl;
struct AnimCloneUnknown {
    const AnimCloneUnknownVtbl* lpVtbl;
};

typedef struct AnimCloneUserVisualVtbl {
    long (ANIMCLONE_STDCALL* QueryInterface)(AnimCloneUserVisual*,
                                             const AnimCloneIID*, void**);
    unsigned long (ANIMCLONE_STDCALL* AddRef)(AnimCloneUserVisual*);
    unsigned long (ANIMCLONE_STDCALL* Release)(AnimCloneUserVisual*);
    void* slots_0c[4];
    unsigned long (ANIMCLONE_STDCALL* GetAppData)(AnimCloneUserVisual*); /* +0x1c */
} AnimCloneUserVisualVtbl;
struct AnimCloneUserVisual {
    const AnimCloneUserVisualVtbl* lpVtbl;
};

typedef struct AnimCloneMaterialVtbl {
    long (ANIMCLONE_STDCALL* QueryInterface)(AnimCloneMaterial*,
                                             const AnimCloneIID*, void**);
    unsigned long (ANIMCLONE_STDCALL* AddRef)(AnimCloneMaterial*);
    unsigned long (ANIMCLONE_STDCALL* Release)(AnimCloneMaterial*);
    void* slots_0c[9];
    long (ANIMCLONE_STDCALL* SetSpecular)(AnimCloneMaterial*, float, float,
                                          float);                 /* +0x30 */
    long (ANIMCLONE_STDCALL* SetEmissive)(AnimCloneMaterial*, float, float,
                                          float);                 /* +0x34 */
} AnimCloneMaterialVtbl;
struct AnimCloneMaterial {
    const AnimCloneMaterialVtbl* lpVtbl;
};

typedef struct AnimCloneD3DRMVtbl {
    long (ANIMCLONE_STDCALL* QueryInterface)(AnimCloneD3DRM*,
                                             const AnimCloneIID*, void**);
    unsigned long (ANIMCLONE_STDCALL* AddRef)(AnimCloneD3DRM*);
    unsigned long (ANIMCLONE_STDCALL* Release)(AnimCloneD3DRM*);
    void* slots_0c[1];
    long (ANIMCLONE_STDCALL* CreateFrame)(AnimCloneD3DRM*, AnimCloneFrame*,
                                          AnimCloneFrame**);      /* +0x10 */
    void* slots_14[8];
    long (ANIMCLONE_STDCALL* CreateMaterial)(AnimCloneD3DRM*, float,
                                             AnimCloneMaterial**); /* +0x34 */
} AnimCloneD3DRMVtbl;
struct AnimCloneD3DRM {
    const AnimCloneD3DRMVtbl* lpVtbl;
};

/* Main.c's lpD3DRM() (mainGlobs.lpD3DRM, 00505914). */
#define lpD3DRM() ((AnimCloneD3DRM*)s_main_d3drm)

struct AnimClone {
    struct AnimClone* clonedFrom;        /* +0x00 */
    struct LwsInfo* scene;               /* +0x04 */
    AnimCloneAnimSet* animSet;           /* +0x08 */
    int lws;                             /* +0x0c */
    AnimCloneFrame* root;                /* +0x10 */
    AnimCloneFrame** partArray;          /* +0x14 */
    unsigned long partCount;             /* +0x18 */
    unsigned long frameCount;            /* +0x1c */
};

/* IID_IDirect3DRMFrame3 (0049f988), resource.cpp's copy, and
 * IID_IDirect3DRMUserVisual (0049fb48), lws.c's. The original has one copy
 * of each, from the DirectX GUID library. */
#ifdef __cplusplus
extern "C" {
#endif
extern const unsigned char s_resource_tree_child_iid[16];
extern const AnimCloneIID s_iid_user_visual;
#ifdef __cplusplus
}
#endif

static int anim_clone_frame_count_callback(AnimCloneFrame* frame,
                                           void* data);
static int anim_clone_setup_frame_array_callback(AnimCloneFrame* frame,
                                                 void* data);

// FUNCTION: LEGORR 0x004894c0
void* material_create(float emissiveRed, float emissiveGreen,
                      float emissiveBlue, float specularRed,
                      float specularGreen, float specularBlue, float power)
{
    AnimCloneMaterial* material;

    if (lpD3DRM()->lpVtbl->CreateMaterial(lpD3DRM(), power, &material) == 0) {
        material->lpVtbl->SetEmissive(material, emissiveRed, emissiveGreen,
                                      emissiveBlue);
        material->lpVtbl->SetSpecular(material, specularRed, specularGreen,
                                      specularBlue);
        return material;
    }

    return NULL;
}

// FUNCTION: LEGORR 0x00489520
AnimClone* anim_clone_register(AnimCloneAnimSet* animSet,
                               AnimCloneFrame* root,
                               unsigned long frameCount)
{
    AnimClone* orig = (AnimClone*)malloc(sizeof(AnimClone));
    AnimClone data;

    orig->clonedFrom = NULL;
    orig->animSet = animSet;
    orig->scene = NULL;
    orig->lws = 0;
    orig->root = root;
    root->lpVtbl->AddRef(root);
    orig->partCount = 0;
    orig->frameCount = frameCount;

    anim_clone_walk_tree(root, 0, anim_clone_frame_count_callback,
                         &orig->partCount);
    orig->partArray = (AnimCloneFrame**)malloc(sizeof(AnimCloneFrame*) *
                                               orig->partCount);
    data.partArray = orig->partArray;
    data.partCount = 0;
    anim_clone_walk_tree(root, 0, anim_clone_setup_frame_array_callback,
                         &data);

    return orig;
}

// FUNCTION: LEGORR 0x004895c0
AnimClone* anim_clone_register_lws(struct LwsInfo* scene,
                                   AnimCloneFrame* root,
                                   unsigned long frameCount)
{
    AnimClone* orig = (AnimClone*)malloc(sizeof(AnimClone));
    AnimClone data;

    orig->clonedFrom = NULL;
    orig->animSet = NULL;
    orig->scene = scene;
    orig->lws = 1;
    orig->root = root;
    root->lpVtbl->AddRef(root);
    orig->partCount = 0;
    orig->frameCount = frameCount;

    anim_clone_walk_tree(root, 0, anim_clone_frame_count_callback,
                         &orig->partCount);
    orig->partArray = (AnimCloneFrame**)malloc(sizeof(AnimCloneFrame*) *
                                               orig->partCount);
    data.partArray = orig->partArray;
    data.partCount = 0;
    anim_clone_walk_tree(root, 0, anim_clone_setup_frame_array_callback,
                         &data);

    return orig;
}

// FUNCTION: LEGORR 0x00489660
AnimClone* anim_clone_make(AnimClone* orig, AnimCloneFrame* parent,
                           unsigned long* frameCount)
{
    AnimClone* clone = (AnimClone*)malloc(sizeof(AnimClone));
    AnimClone data;

    if (orig->lws) {
        *clone = *orig;
        clone->scene = lws_clone(orig->scene, (struct LwsFrame*)parent);
        clone->clonedFrom = NULL;
        if (frameCount) *frameCount = orig->frameCount;
    } else {
        memset(clone, 0, sizeof(AnimClone));
        orig->animSet->lpVtbl->SetTime(orig->animSet, 0.0f);
        clone->animSet = NULL;
        clone->clonedFrom = orig;
        lpD3DRM()->lpVtbl->CreateFrame(lpD3DRM(), parent, &clone->root);
        clone->partArray = (AnimCloneFrame**)malloc(sizeof(AnimCloneFrame*) *
                                                    orig->partCount);
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
                dead->animSet->lpVtbl->Release(dead->animSet);
                dead->root->lpVtbl->Release(dead->root);
            }
        }

        if (!dead->lws) {
            for (loop = 0; loop < dead->partCount; loop++) {
                dead->partArray[loop]->lpVtbl->Release(dead->partArray[loop]);
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
        else orig->animSet->lpVtbl->SetTime(orig->animSet, time);

        for (loop = 0; loop < clone->partCount; loop++) {
            AnimCloneFrame* parent;
            float mat[4][4];

            orig->partArray[loop]->lpVtbl->GetParent(orig->partArray[loop],
                                                     &parent);
            orig->partArray[loop]->lpVtbl->GetTransform(orig->partArray[loop],
                                                        parent, mat);
            parent->lpVtbl->Release(parent);
            clone->partArray[loop]->lpVtbl->AddTransform(
                clone->partArray[loop], ANIMCLONE_COMBINE_REPLACE, mat);
        }

        if (oldTime) {
            if (orig->lws) lws_time_set(orig->scene, *oldTime);
            else orig->animSet->lpVtbl->SetTime(orig->animSet, *oldTime);
        }
    } else {
        if (clone->lws) lws_time_set(clone->scene, time);
        else clone->animSet->lpVtbl->SetTime(clone->animSet, time);
    }
}

// FUNCTION: LEGORR 0x004898e0
static int anim_clone_frame_count_callback(AnimCloneFrame* frame, void* data)
{
    unsigned long* count = (unsigned long*)data;

    (*count)++;

    return 0;
}

// FUNCTION: LEGORR 0x004898f0
static int anim_clone_setup_frame_array_callback(AnimCloneFrame* frame,
                                                 void* data)
{
    AnimClone* clone = (AnimClone*)data;

    clone->partArray[clone->partCount] = frame;
    clone->partCount++;

    return 0;
}

// FUNCTION: LEGORR 0x00489910
int anim_clone_walk_tree(AnimCloneFrame* frame, unsigned long level,
                         AnimCloneWalkTreeCallback Callback, void* data)
{
    AnimCloneFrameArray* children;
    unsigned long loop, count;
    int finished = 0;

    if (Callback(frame, data)) return 1;

    if (frame->lpVtbl->GetChildren(frame, &children) == 0) {
        count = children->lpVtbl->GetSize(children);
        for (loop = 0; loop < count; loop++) {
            AnimCloneFrame1* child1;
            AnimCloneFrame* child;

            children->lpVtbl->GetElement(children, loop, &child1);
            child1->lpVtbl->QueryInterface(child1,
                                           (const AnimCloneIID*)s_resource_tree_child_iid,
                                           (void**)&child);
            child1->lpVtbl->Release(child1);
            if (anim_clone_walk_tree(child, level + 1, Callback, data)) {
                finished = 1;
                child->lpVtbl->Release(child);
                break;
            }
            child->lpVtbl->Release(child);
        }
        children->lpVtbl->Release(children);
    }

    return finished;
}

// FUNCTION: LEGORR 0x004899f0
void anim_clone_create_copy(AnimCloneFrame* orig, AnimCloneFrame* clone,
                            int lws)
{
    AnimCloneFrameArray* children;
    AnimCloneFrame* parent;
    unsigned long count, loop;
    unsigned long size;
    char* name;
    float mat[4][4];

    if (lws) anim_clone_clone_lwo_meshes(orig, clone);
    else anim_clone_reference_visuals(orig, clone);

    orig->lpVtbl->GetName(orig, &size, NULL);
    if (size) {
        name = (char*)malloc(size);
        orig->lpVtbl->GetName(orig, &size, name);
        clone->lpVtbl->SetName(clone, name);
        free(name);
    }

    orig->lpVtbl->GetParent(orig, &parent);
    orig->lpVtbl->GetTransform(orig, parent, mat);
    parent->lpVtbl->Release(parent);
    clone->lpVtbl->AddTransform(clone, ANIMCLONE_COMBINE_REPLACE, mat);

    if (orig->lpVtbl->GetChildren(orig, &children) == 0) {
        count = children->lpVtbl->GetSize(children);
        for (loop = 0; loop < count; loop++) {
            AnimCloneFrame1* child1;
            AnimCloneFrame* child;
            AnimCloneFrame* newFrame;

            children->lpVtbl->GetElement(children, loop, &child1);
            child1->lpVtbl->QueryInterface(child1,
                                           (const AnimCloneIID*)s_resource_tree_child_iid,
                                           (void**)&child);
            child1->lpVtbl->Release(child1);
            lpD3DRM()->lpVtbl->CreateFrame(lpD3DRM(), clone, &newFrame);
            anim_clone_create_copy(child, newFrame, lws);
            newFrame->lpVtbl->Release(newFrame);
            child->lpVtbl->Release(child);
        }
        children->lpVtbl->Release(children);
    }
}

// FUNCTION: LEGORR 0x00489b30
void anim_clone_clone_lwo_meshes(AnimCloneFrame* orig, AnimCloneFrame* clone)
{
    AnimCloneUnknown* visual[ANIMCLONE_MAXVISUALS];
    AnimCloneUserVisual* userVisual;
    unsigned long count, loop;

    orig->lpVtbl->GetVisuals(orig, &count, NULL);
    if (count) {
        orig->lpVtbl->GetVisuals(orig, &count, visual);
        for (loop = 0; loop < count; loop++) {
            if (visual[loop]->lpVtbl->QueryInterface(visual[loop],
                                                     &s_iid_user_visual,
                                                     (void**)&userVisual) == 0) {
                void* mesh = (void*)userVisual->lpVtbl->GetAppData(userVisual);
                mesh_clone(mesh, clone);
            }
        }
    }
}

// FUNCTION: LEGORR 0x00489bc0
void anim_clone_reference_visuals(AnimCloneFrame* orig, AnimCloneFrame* clone)
{
    AnimCloneUnknown* visual[ANIMCLONE_MAXVISUALS];
    unsigned long count, loop;

    orig->lpVtbl->GetVisuals(orig, &count, NULL);
    if (count) {
        orig->lpVtbl->GetVisuals(orig, &count, visual);
        for (loop = 0; loop < count; loop++) {
            clone->lpVtbl->AddVisual(clone, visual[loop]);
        }
    }
}
