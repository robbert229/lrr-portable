// SPDX-License-Identifier: LGPL-3.0-or-later
#include "lwsc_miniwin.hpp"

#if defined(LEGORRR_BACKEND_MINIWIN)

#include <miniwin/d3drm.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct LwscMiniwinMesh LwscMiniwinMesh;

/* AppData of every frame this backend creates. */
typedef struct LwscMiniwinFrame {
    IDirect3DRMFrame2* frame;
    struct LwscMiniwinFrame* parent;
    struct LwscMiniwinFrame* first_child;
    struct LwscMiniwinFrame* next_sibling;
    LwscMiniwinMesh* meshes;
    float local[16];
    int enabled;
    unsigned int magic;
} LwscMiniwinFrame;

struct LwscMiniwinMesh {
    LwscMiniwinFrame* owner;
    const Model* model;
    float opacity;
    LwscMiniwinMesh* next;
};

#define LWSC_MINIWIN_FRAME_MAGIC 0x4c57464du /* "LWFM" */

static IDirect3DRM* s_rm;
static LwscMiniwinFrame* s_root;
static LwscFrameRef s_root_ref;

static void lwsc_miniwin_identity(float m[16])
{
    memset(m, 0, 16 * sizeof(float));
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

/* out = a * b (row vectors: apply a, then b). out may alias neither. */
static void lwsc_miniwin_multiply(const float a[16], const float b[16],
                                  float out[16])
{
    int r, c;
    for (r = 0; r < 4; ++r) {
        for (c = 0; c < 4; ++c) {
            out[r * 4 + c] = a[r * 4 + 0] * b[0 * 4 + c] +
                             a[r * 4 + 1] * b[1 * 4 + c] +
                             a[r * 4 + 2] * b[2 * 4 + c] +
                             a[r * 4 + 3] * b[3 * 4 + c];
        }
    }
}

static LwscMiniwinFrame* lwsc_miniwin_record(const void* frame)
{
    LwscMiniwinFrame* record;
    if (frame == NULL) return NULL;
    record = (LwscMiniwinFrame*)((IDirect3DRMFrame2*)frame)->GetAppData();
    if (record == NULL || record->magic != LWSC_MINIWIN_FRAME_MAGIC ||
        record->frame != (IDirect3DRMFrame2*)frame) return NULL;
    return record;
}

static LwscMiniwinFrame* lwsc_miniwin_ref_record(const LwscFrameRef* ref)
{
    return ref != NULL ? lwsc_miniwin_record(ref->interface_pointer) : NULL;
}

static LwscMiniwinFrame* lwsc_miniwin_frame_new(LwscMiniwinFrame* parent)
{
    LwscMiniwinFrame* record =
        (LwscMiniwinFrame*)calloc(1u, sizeof(LwscMiniwinFrame));
    IDirect3DRMFrame2* frame = NULL;
    if (record == NULL) return NULL;
    if (s_rm->CreateFrame(parent != NULL ? parent->frame : NULL, &frame) !=
            D3DRM_OK || frame == NULL) {
        free(record);
        return NULL;
    }
    record->frame = frame;
    record->parent = parent;
    record->enabled = 1;
    record->magic = LWSC_MINIWIN_FRAME_MAGIC;
    lwsc_miniwin_identity(record->local);
    frame->SetAppData((LPD3DRM_APPDATA)record);
    if (parent != NULL) {
        record->next_sibling = parent->first_child;
        parent->first_child = record;
    }
    return record;
}

static int lwsc_miniwin_context_ensure(void)
{
    if (s_root != NULL) return 1;
    if (s_rm == NULL && (Direct3DRMCreate(&s_rm) != D3DRM_OK || s_rm == NULL))
        return 0;
    s_root = lwsc_miniwin_frame_new(NULL);
    if (s_root == NULL) return 0;
    s_root_ref.interface_pointer = s_root->frame;
    return 1;
}

static int lwsc_miniwin_frame_create(const LwscFrameRef* parent,
                                     LwscFrameRef* result, void* context)
{
    LwscMiniwinFrame* owner = lwsc_miniwin_ref_record(parent);
    LwscMiniwinFrame* record;
    (void)context;
    if (result == NULL) return 0;
    result->interface_pointer = NULL;
    if (owner == NULL) return 0;
    record = lwsc_miniwin_frame_new(owner);
    if (record == NULL) return 0;
    result->interface_pointer = record->frame;
    return 1;
}

static void lwsc_miniwin_frame_destroy(LwscFrameRef* frame, void* context)
{
    LwscMiniwinFrame* record = lwsc_miniwin_ref_record(frame);
    (void)context;
    if (record == NULL) return;
    /* lwsc destroys children before parents and detaches meshes first. */
    if (record->parent != NULL) {
        LwscMiniwinFrame** link = &record->parent->first_child;
        while (*link != NULL && *link != record) link = &(*link)->next_sibling;
        if (*link == record) *link = record->next_sibling;
        record->parent->frame->DeleteChild(record->frame);
    }
    record->frame->SetAppData(NULL);
    record->frame->Release();
    record->magic = 0u;
    free(record);
    frame->interface_pointer = NULL;
}

static int lwsc_miniwin_frame_name_set(const LwscFrameRef* frame,
                                       const char* name, void* context)
{
    LwscMiniwinFrame* record = lwsc_miniwin_ref_record(frame);
    (void)context;
    return record != NULL && record->frame->SetName(name) == D3DRM_OK;
}

static int lwsc_miniwin_matrix_set(const LwscFrameRef* frame,
                                   const float matrix[16], void* context)
{
    LwscMiniwinFrame* record = lwsc_miniwin_ref_record(frame);
    D3DRMMATRIX4D transform;
    (void)context;
    if (record == NULL || matrix == NULL) return 0;
    memcpy(record->local, matrix, sizeof(record->local));
    memcpy(transform, matrix, sizeof(transform));
    return record->frame->AddTransform(D3DRMCOMBINE_REPLACE, transform) ==
           D3DRM_OK;
}

static int lwsc_miniwin_matrix_get(const LwscFrameRef* frame, float matrix[16],
                                   void* context)
{
    LwscMiniwinFrame* record = lwsc_miniwin_ref_record(frame);
    (void)context;
    if (record == NULL || matrix == NULL) return 0;
    memcpy(matrix, record->local, sizeof(record->local));
    return 1;
}

static int lwsc_miniwin_world_position(const LwscFrameRef* frame,
                                       float xyz[3], void* context)
{
    float world[16];
    (void)context;
    if (xyz == NULL ||
        !lwsc_miniwin_frame_transform_get(
            frame != NULL ? frame->interface_pointer : NULL, NULL, world))
        return 0;
    xyz[0] = world[12];
    xyz[1] = world[13];
    xyz[2] = world[14];
    return 1;
}

static int lwsc_miniwin_activity_enable(const LwscFrameRef* frame,
                                        int enabled, void* context)
{
    LwscMiniwinFrame* record = lwsc_miniwin_ref_record(frame);
    (void)context;
    if (record == NULL) return 0;
    record->enabled = enabled != 0;
    return 1;
}

static int lwsc_miniwin_model_attach(const LwscFrameRef* frame,
                                     const Model* model, const char* path,
                                     unsigned int flags, LwscMeshRef* result,
                                     void* context)
{
    LwscMiniwinFrame* record = lwsc_miniwin_ref_record(frame);
    LwscMiniwinMesh* mesh;
    (void)path;
    (void)context;
    if (result == NULL) return 0;
    result->owner = NULL;
    if (record == NULL || model == NULL) return 0;
    mesh = (LwscMiniwinMesh*)calloc(1u, sizeof(LwscMiniwinMesh));
    if (mesh == NULL) return 0;
    mesh->owner = record;
    mesh->model = model;
    /* UnseenByCamera nodes keep their frame but draw nothing. */
    mesh->opacity = (flags & 4u) != 0u ? 0.0f : 1.0f;
    mesh->next = record->meshes;
    record->meshes = mesh;
    result->owner = mesh;
    return 1;
}

static void lwsc_miniwin_model_detach(LwscMeshRef* mesh, void* context)
{
    LwscMiniwinMesh* attachment;
    LwscMiniwinMesh** link;
    (void)context;
    if (mesh == NULL || mesh->owner == NULL) return;
    attachment = (LwscMiniwinMesh*)mesh->owner;
    link = &attachment->owner->meshes;
    while (*link != NULL && *link != attachment) link = &(*link)->next;
    if (*link == attachment) *link = attachment->next;
    free(attachment);
    mesh->owner = NULL;
}

static int lwsc_miniwin_model_opacity_set(const LwscMeshRef* mesh,
                                          float opacity, void* context)
{
    (void)context;
    if (mesh == NULL || mesh->owner == NULL) return 0;
    ((LwscMiniwinMesh*)mesh->owner)->opacity = opacity;
    return 1;
}

int lwsc_miniwin_backend_get(LwscBackendOps* ops, const LwscFrameRef** parent)
{
    if (ops == NULL || parent == NULL) return 0;
    memset(ops, 0, sizeof(*ops));
    *parent = NULL;
    if (!lwsc_miniwin_context_ensure()) return 0;
    ops->frame_create = lwsc_miniwin_frame_create;
    ops->frame_destroy = lwsc_miniwin_frame_destroy;
    ops->frame_name_set = lwsc_miniwin_frame_name_set;
    ops->matrix_set = lwsc_miniwin_matrix_set;
    ops->matrix_get = lwsc_miniwin_matrix_get;
    ops->world_position = lwsc_miniwin_world_position;
    ops->activity_enable = lwsc_miniwin_activity_enable;
    ops->model_attach = lwsc_miniwin_model_attach;
    ops->model_detach = lwsc_miniwin_model_detach;
    ops->model_opacity_set = lwsc_miniwin_model_opacity_set;
    *parent = &s_root_ref;
    return 1;
}

int lwsc_miniwin_frame_known(const void* frame)
{
    return lwsc_miniwin_record(frame) != NULL;
}

int lwsc_miniwin_frame_transform_get(const void* frame, const void* ancestor,
                                     float matrix[16])
{
    LwscMiniwinFrame* record = lwsc_miniwin_record(frame);
    LwscMiniwinFrame* stop = NULL;
    float product[16];
    float next[16];
    if (record == NULL || matrix == NULL) return 0;
    if (ancestor != NULL) {
        stop = lwsc_miniwin_record(ancestor);
        if (stop == NULL) return 0;
    }
    lwsc_miniwin_identity(product);
    while (record != NULL && record != stop) {
        lwsc_miniwin_multiply(product, record->local, next);
        memcpy(product, next, sizeof(product));
        record = record->parent;
    }
    if (record != stop) return 0;
    memcpy(matrix, product, sizeof(product));
    return 1;
}

static unsigned int lwsc_miniwin_visit(const LwscMiniwinFrame* record,
                                       const float parent_to_ancestor[16],
                                       int include_local,
                                       LwscMiniwinMeshVisitFn visit,
                                       void* context)
{
    const LwscMiniwinFrame* child;
    const LwscMiniwinMesh* mesh;
    float to_ancestor[16];
    unsigned int count = 0u;
    if (!record->enabled) return 0u;
    if (include_local)
        lwsc_miniwin_multiply(record->local, parent_to_ancestor, to_ancestor);
    else
        memcpy(to_ancestor, parent_to_ancestor, sizeof(to_ancestor));
    for (mesh = record->meshes; mesh != NULL; mesh = mesh->next) {
        if (mesh->opacity > 0.0f) {
            visit(context, mesh->model, to_ancestor, mesh->opacity);
            ++count;
        }
    }
    for (child = record->first_child; child != NULL;
         child = child->next_sibling)
        count += lwsc_miniwin_visit(child, to_ancestor, 1, visit, context);
    return count;
}

unsigned int lwsc_miniwin_meshes_visit(const void* top, const void* ancestor,
                                       LwscMiniwinMeshVisitFn visit,
                                       void* context)
{
    LwscMiniwinFrame* record = lwsc_miniwin_record(top);
    float parent_to_ancestor[16];
    if (record == NULL || visit == NULL) return 0u;
    lwsc_miniwin_identity(parent_to_ancestor);
    /* Relative to itself: the top frame's own transform is excluded. */
    if (ancestor == top)
        return lwsc_miniwin_visit(record, parent_to_ancestor, 0, visit,
                                  context);
    if (record->parent != NULL &&
        (ancestor == NULL || record->parent->frame != ancestor) &&
        !lwsc_miniwin_frame_transform_get(record->parent->frame, ancestor,
                                          parent_to_ancestor))
        return 0u;
    return lwsc_miniwin_visit(record, parent_to_ancestor, 1, visit, context);
}

#endif /* LEGORRR_BACKEND_MINIWIN */
