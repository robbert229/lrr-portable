// SPDX-License-Identifier: LGPL-3.0-or-later
#include "container_native.hpp"

#if defined(LEGORRR_BACKEND_MINIWIN)

#include "lwsc_miniwin.hpp"
#include "view.hpp"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONTAINER_NATIVE_MAGIC 0x434e4154u /* "CNAT" */
#define CONTAINER_NATIVE_TYPE_ACT 4u

typedef struct ContainerNative {
    /* Retail Container head: masterFrame, activityFrame, hiddenFrame,
     * typeData, type (+0x10), flags, activity callback and data. Only the
     * type word is written; the frames live in `instance`. */
    unsigned char head[0x20];
    uint32_t owner;                    /* +0x20 */
    uint32_t magic;
    LwscAssemblyTemplate* assembly;
    LwscAssemblyInstance* instance;
    float root[16];                    /* the root frame's local matrix */
    int hidden;
    struct ContainerNative* next;
} ContainerNative;

static ContainerNative* s_containers;
static LwscBackendOps s_ops;
static const LwscFrameRef* s_parent;
static char s_part_name[LWSC_NAME_CAPACITY + 16];

static ContainerNative* container_native_get(const void* cont)
{
    ContainerNative* p;
    if (cont == NULL) return NULL;
    for (p = s_containers; p != NULL; p = p->next)
        if (p == cont) return p->magic == CONTAINER_NATIVE_MAGIC ? p : NULL;
    return NULL;
}

static int container_native_backend(void)
{
    if (s_parent != NULL) return 1;
    return lwsc_miniwin_backend_get(&s_ops, &s_parent);
}

static void container_native_identity(float m[16])
{
    memset(m, 0, 16 * sizeof(float));
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static ContainerNative* container_native_wrap(LwscAssemblyTemplate* assembly,
                                              LwscAssemblyInstance* instance)
{
    ContainerNative* p = (ContainerNative*)calloc(1u, sizeof(ContainerNative));
    uint32_t type = CONTAINER_NATIVE_TYPE_ACT;
    if (p == NULL) return NULL;
    memcpy(p->head + 0x10, &type, sizeof(type));
    p->magic = CONTAINER_NATIVE_MAGIC;
    p->assembly = assembly;
    p->instance = instance;
    container_native_identity(p->root);
    p->next = s_containers;
    s_containers = p;
    return p;
}

static void container_native_trigger(void* context)
{
    ddiscene_activity_trigger_dispatch(context);
}

/* SAMPLE callbacks are not dispatched: the sound route reads the master
 * frame as a 32-bit position handle, which native frames are not. */
static void container_native_callbacks(ContainerNative* p,
                                       LwscClockCallbacks* callbacks)
{
    memset(callbacks, 0, sizeof(*callbacks));
    callbacks->context = p;
    callbacks->trigger = container_native_trigger;
}

void* container_native_load_act(const char* entity_path, uint32_t feature,
                                const char* const* name_keys,
                                unsigned int name_key_count)
{
    LwscAssemblyTemplate* assembly;
    LwscAssemblyInstance* instance;
    ContainerNative* p;
    LwscError error;
    float identity[16];
    if (entity_path == NULL || !container_native_backend()) return NULL;
    assembly = lwsc_assembly_open(entity_path, "Lego*", feature, name_keys,
                                  name_key_count, &error);
    if (assembly == NULL) {
        fprintf(stderr, "container: %s: %s\n", entity_path, error.message);
        return NULL;
    }
    container_native_identity(identity);
    instance = lwsc_instance_create(assembly, &s_ops, s_parent, identity,
                                    &error);
    if (instance == NULL) {
        fprintf(stderr, "container: %s: %s\n", entity_path, error.message);
        lwsc_assembly_release(assembly);
        return NULL;
    }
    p = container_native_wrap(assembly, instance);
    if (p == NULL) {
        lwsc_instance_destroy(instance);
        lwsc_assembly_release(assembly);
    }
    return p;
}

int container_native_owns(const void* cont)
{
    return container_native_get(cont) != NULL;
}

const char* container_native_configured_name(const void* cont,
                                             const char* key)
{
    ContainerNative* p = container_native_get(cont);
    return p != NULL ? lwsc_assembly_configured_name(p->assembly, key) : NULL;
}

void* container_native_clone(void* cont)
{
    ContainerNative* source = container_native_get(cont);
    ContainerNative* p;
    LwscAssemblyInstance* instance;
    LwscError error;
    if (source == NULL) return NULL;
    instance = lwsc_instance_clone(source->instance, s_parent, source->root,
                                   &error);
    if (instance == NULL) {
        fprintf(stderr, "container: clone failed: %s\n", error.message);
        return NULL;
    }
    lwsc_assembly_retain(source->assembly);
    p = container_native_wrap(source->assembly, instance);
    if (p == NULL) {
        lwsc_instance_destroy(instance);
        lwsc_assembly_release(source->assembly);
        return NULL;
    }
    memcpy(p->root, source->root, sizeof(p->root));
    return p;
}

void container_native_remove(void* cont)
{
    ContainerNative* p = container_native_get(cont);
    ContainerNative** link = &s_containers;
    if (p == NULL) return;
    while (*link != NULL && *link != p) link = &(*link)->next;
    if (*link == p) *link = p->next;
    lwsc_instance_destroy(p->instance);
    lwsc_assembly_release(p->assembly);
    p->magic = 0u;
    free(p);
}

void container_native_hidden_set(void* cont, int hidden)
{
    ContainerNative* p = container_native_get(cont);
    if (p != NULL) p->hidden = hidden != 0;
}

int container_native_hidden(const void* cont)
{
    ContainerNative* p = container_native_get(cont);
    return p == NULL || p->hidden;
}

uint32_t container_native_activity_set(void* cont, const char* activity)
{
    ContainerNative* p = container_native_get(cont);
    LwscError error;
    if (p == NULL || activity == NULL) return 0u;
    return lwsc_instance_activity_set(p->instance, activity, &error) ? 1u : 0u;
}

float container_native_animation_time_set(void* cont, float time)
{
    ContainerNative* p = container_native_get(cont);
    LwscClockCallbacks callbacks;
    LwscError error;
    float overrun = 0.0f;
    if (p == NULL) return 0.0f;
    container_native_callbacks(p, &callbacks);
    if (!lwsc_instance_clock_set(p->instance, time, &callbacks, &overrun,
                                 &error))
        return 0.0f;
    return overrun;
}

float container_native_animation_forward(void* cont, float delta)
{
    ContainerNative* p = container_native_get(cont);
    LwscClockCallbacks callbacks;
    LwscError error;
    float overrun = 0.0f;
    if (p == NULL) return 0.0f;
    container_native_callbacks(p, &callbacks);
    /* One repeat: the Creature unit folds extra repeats itself. */
    if (!lwsc_instance_advance(p->instance, delta, 1u, &callbacks, &overrun,
                               &error))
        return 0.0f;
    return overrun;
}

uint32_t container_native_animation_frames(void* cont)
{
    ContainerNative* p = container_native_get(cont);
    const LwscSceneTemplate* scene;
    if (p == NULL) return 0u;
    scene = lwsc_assembly_scene(p->assembly,
                                lwsc_instance_activity(p->instance));
    return scene != NULL ? lwsc_scene_last_frame(scene) : 0u;
}

float container_native_animation_time(void* cont)
{
    ContainerNative* p = container_native_get(cont);
    return p != NULL ? lwsc_instance_time(p->instance) : 0.0f;
}

float container_native_trans_coef(void* cont)
{
    ContainerNative* p = container_native_get(cont);
    const EntityAssembly* metadata;
    unsigned int activity;
    if (p == NULL) return 0.0f;
    metadata = lwsc_assembly_metadata(p->assembly);
    activity = lwsc_instance_activity(p->instance);
    if (metadata == NULL || activity >= (unsigned int)metadata->count)
        return 0.0f;
    return metadata->acts[activity].transcoef;
}

static int container_native_root_apply(ContainerNative* p)
{
    LwscError error;
    return lwsc_instance_root_set(p->instance, p->root, &error);
}

int container_native_position_set(void* cont, float x, float y, float z)
{
    ContainerNative* p = container_native_get(cont);
    if (p == NULL) return 0;
    p->root[12] = x;
    p->root[13] = y;
    p->root[14] = z;
    return container_native_root_apply(p);
}

static int container_native_normalize(float v[3])
{
    float length = (float)sqrt((double)(v[0] * v[0] + v[1] * v[1] +
                                        v[2] * v[2]));
    if (!(length > 1e-6f)) return 0;
    v[0] /= length;
    v[1] /= length;
    v[2] /= length;
    return 1;
}

int container_native_orientation_set(void* cont, float dx, float dy,
                                     float dz, float ux, float uy, float uz)
{
    ContainerNative* p = container_native_get(cont);
    float z[3], y[3], x[3], d;
    if (p == NULL) return 0;
    z[0] = dx; z[1] = dy; z[2] = dz;
    if (!container_native_normalize(z)) return 0;
    d = ux * z[0] + uy * z[1] + uz * z[2];
    y[0] = ux - d * z[0];
    y[1] = uy - d * z[1];
    y[2] = uz - d * z[2];
    if (!container_native_normalize(y)) return 0;
    /* D3DRM frames are left-handed: x = y cross z. */
    x[0] = y[1] * z[2] - y[2] * z[1];
    x[1] = y[2] * z[0] - y[0] * z[2];
    x[2] = y[0] * z[1] - y[1] * z[0];
    p->root[0] = x[0]; p->root[1] = x[1]; p->root[2] = x[2];
    p->root[4] = y[0]; p->root[5] = y[1]; p->root[6] = y[2];
    p->root[8] = z[0]; p->root[9] = z[1]; p->root[10] = z[2];
    return container_native_root_apply(p);
}

int container_native_object_known(const void* object)
{
    return container_native_owns(object) || lwsc_miniwin_frame_known(object);
}

int container_native_position_get(const void* object, float xyz[3])
{
    ContainerNative* p = container_native_get(object);
    float world[16];
    if (xyz == NULL) return 0;
    if (p != NULL) {
        xyz[0] = p->root[12];
        xyz[1] = p->root[13];
        xyz[2] = p->root[14];
        return 1;
    }
    if (!lwsc_miniwin_frame_transform_get(object, NULL, world)) return 0;
    xyz[0] = world[12];
    xyz[1] = world[13];
    xyz[2] = world[14];
    return 1;
}

const char* container_native_part_name_format(void* cont, const char* name,
                                              const int* instance)
{
    (void)cont;
    snprintf(s_part_name, sizeof(s_part_name), "%s|%d",
             name != NULL ? name : "", instance != NULL ? *instance : 0);
    return s_part_name;
}

void* container_native_part_find(void* cont, const char* formatted_name)
{
    ContainerNative* p = container_native_get(cont);
    char base[LWSC_NAME_CAPACITY];
    const char* bar;
    const LwscFrameRef* frame;
    size_t length;
    int ordinal;
    if (p == NULL || formatted_name == NULL) return NULL;
    bar = strrchr(formatted_name, '|');
    if (bar == NULL) return NULL;
    length = (size_t)(bar - formatted_name);
    if (length == 0u || length >= sizeof(base)) return NULL;
    memcpy(base, formatted_name, length);
    base[length] = '\0';
    ordinal = atoi(bar + 1);
    if (ordinal < 0) return NULL;
    frame = lwsc_instance_named_node(p->instance, base, (unsigned int)ordinal);
    return frame != NULL ? frame->interface_pointer : NULL;
}

typedef struct ContainerNativeVisit {
    ContainerNativeMeshVisitFn visit;
    void* context;
    float scale;
} ContainerNativeVisit;

/* lwsc folds the assembly Scale into the root frame's matrix; put it back
 * on the root-relative matrices so they keep the authored size. */
static void container_native_visit_scaled(void* context, const Model* model,
                                          const float model_to_root[16],
                                          float opacity)
{
    const ContainerNativeVisit* visit = (const ContainerNativeVisit*)context;
    float scaled[16];
    unsigned int i;
    memcpy(scaled, model_to_root, sizeof(scaled));
    for (i = 0u; i < 15u; ++i) {
        if ((i & 3u) != 3u) scaled[i] *= visit->scale;
    }
    visit->visit(visit->context, model, scaled, opacity);
}

unsigned int container_native_meshes_visit(void* cont,
                                           ContainerNativeMeshVisitFn visit,
                                           void* context)
{
    ContainerNative* p = container_native_get(cont);
    const EntityAssembly* metadata;
    const LwscFrameRef* root;
    ContainerNativeVisit scaled;
    if (p == NULL || p->hidden || visit == NULL) return 0u;
    root = lwsc_instance_root(p->instance);
    if (root == NULL || root->interface_pointer == NULL) return 0u;
    metadata = lwsc_assembly_metadata(p->assembly);
    scaled.visit = visit;
    scaled.context = context;
    scaled.scale = metadata != NULL && metadata->scale != 0.0f
        ? metadata->scale : 1.0f;
    return lwsc_miniwin_meshes_visit(root->interface_pointer,
                                     root->interface_pointer,
                                     container_native_visit_scaled, &scaled);
}

#endif /* LEGORRR_BACKEND_MINIWIN */
