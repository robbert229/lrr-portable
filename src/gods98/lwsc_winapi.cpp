// SPDX-License-Identifier: LGPL-3.0-or-later
/* Authored pose-query backend: actual RM3/Frame3 + actual pooled DDi owners.
 * No renderer callbacks or global scene/property hook replacement.
 * Pool contracts: scene_pool.cpp 0x472950/0x4759f0; normal destruction and
 * property cleanup: view.cpp 0x472a50 and scene_runtime_frame_release.
 * Query normalization: view.cpp 0x4754c0/0x475900. */
#include "lwsc_winapi.hpp"

#if defined(LWSC_WINAPI_POSE_BACKEND)
#include "scene_pool.hpp"
#include "view.hpp"
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#define LWSC_WINAPI_MAX_CONTEXTS 64u
#define LWSC_WINAPI_MAX_FRAMES 16384u

typedef char LwscWinapiPointerSizeCheck[sizeof(void*) == 4u ? 1 : -1];

/* IID at retail 0x49f988, also recorded in resource.cpp's actual child QI.
 * Verify this exact interface BEFORE using Frame3 offsets; no arbitrary
 * Model/ResourceTree/host record is ever interpreted as a frame. */
static const GUID s_lwsc_frame3_iid = {
    0xff6b7f70, 0xa40e, 0x11d1,
    { 0x91, 0xf9, 0x00, 0x00, 0xf8, 0x75, 0x8e, 0x66 }
};

typedef HRESULT (WINAPI *LwscQueryInterfaceFn)(void*, const GUID*, void**);
typedef HRESULT (WINAPI *LwscParentGetFn)(void*, void**);
typedef HRESULT (WINAPI *LwscDeleteChildFn)(void*, void*);
typedef HRESULT (WINAPI *LwscNameSetFn)(void*, const char*);
typedef HRESULT (WINAPI *LwscMatrixSetFn)(void*, unsigned int, const float*);
typedef HRESULT (WINAPI *LwscMatrixGetFn)(void*, void*, float*);

typedef struct LwscWinapiFrame {
    void* scene_object;             /* actual return of pool allocate */
    lego::DDiSceneObjectDescriptor channels; /* three separately retained QIs */
    LwscFrameRef* published;        /* producer slot, never a DDi object */
    struct LwscWinapiFrame* parent;
    struct LwscWinapiFrame* next;
    unsigned int children;
    int retired;                   /* callback arrived before child releases */
    int destroy_failed;            /* normal owner did not retire its record */
} LwscWinapiFrame;

struct LwscWinapiContext {
    void* application_scene;        /* actual initialized root, never a map */
    lego::DDiSceneObjectDescriptor application_channels;
    LwscFrameRef parent;
    HMODULE module;                 /* pin of the already-loaded D3DRM */
    LwscWinapiFrame* frames;
    unsigned int frame_count;
    int poisoned;
    char error[160];
    LwscWinapiContext* next;
};

static LwscWinapiContext* s_lwsc_contexts;
static unsigned int s_lwsc_context_count;

static void lwsc_winapi_error_set(LwscWinapiContext* context, const char* message)
{
    strncpy(context->error, message, sizeof(context->error) - 1u);
    context->error[sizeof(context->error) - 1u] = '\0';
}

static int lwsc_winapi_error(LwscWinapiContext* context, const char* message)
{
    lwsc_winapi_error_set(context, message);
    return 0;
}

static int lwsc_winapi_poison(LwscWinapiContext* context, const char* message)
{
    context->poisoned = 1;
    return lwsc_winapi_error(context, message);
}

static int lwsc_winapi_context_known(const LwscWinapiContext* context)
{
    const LwscWinapiContext* cursor;
    for (cursor = s_lwsc_contexts; cursor != NULL; cursor = cursor->next)
        if (cursor == context) return 1;
    return 0;
}

static int lwsc_winapi_finite(float value)
{
    return value == value && value <= FLT_MAX && value >= -FLT_MAX;
}

static int lwsc_winapi_matrix_valid(const float matrix[16])
{
    unsigned int i;
    if (matrix == NULL) return 0;
    for (i = 0u; i < 16u; ++i) if (!lwsc_winapi_finite(matrix[i])) return 0;
    return matrix[3] == 0.0f && matrix[7] == 0.0f && matrix[11] == 0.0f &&
           matrix[15] == 1.0f;
}

/* Read only the documented descriptor prefix of a KNOWN actual pool record.
 * There is no locally fabricated retail record layout, kind or refcount. */
static int lwsc_winapi_channels_read(void* scene,
                                     lego::DDiSceneObjectDescriptor* channels)
{
    if (scene == NULL || !lego::ddiscene_object_pool_owns(scene)) return 0;
    memcpy(channels, scene, sizeof(*channels));
    return 1;
}

static int lwsc_winapi_owner_matches(void* frame, void* scene)
{
    DDiSceneAnimationHookRecord* property = ddiscene_animation_hook_record_get(frame);
    return property != NULL && property->scene_object == frame &&
        (uintptr_t)(uint32_t)property->callback == (uintptr_t)scene;
}

static int lwsc_winapi_record_matches(void* scene,
    const lego::DDiSceneObjectDescriptor* retained)
{
    lego::DDiSceneObjectDescriptor actual;
    unsigned int i;
    if (!lwsc_winapi_channels_read(scene, &actual)) return 0;
    /* Compare all pointers before touching properties: returned/free/reused
     * records have cleared/different channels. The retained COM refs prevent
     * old interface addresses being recycled while this entry is alive. */
    for (i = 0u; i < 3u; ++i)
        if (actual.channels[i] == NULL || actual.channels[i] != retained->channels[i]) return 0;
    for (i = 0u; i < 3u; ++i)
        if (!lwsc_winapi_owner_matches(retained->channels[i], scene)) return 0;
    return 1;
}

static void lwsc_winapi_channels_release(lego::DDiSceneObjectDescriptor* channels)
{
    unsigned int i;
    for (i = 3u; i > 0u; --i) {
        scene_winapi_interface_release(channels->channels[i - 1u]);
        channels->channels[i - 1u] = NULL;
    }
}

static int lwsc_winapi_channels_retain(void* scene,
                                      lego::DDiSceneObjectDescriptor* retained)
{
    lego::DDiSceneObjectDescriptor actual;
    unsigned int i;
    memset(retained, 0, sizeof(*retained));
    if (!lwsc_winapi_channels_read(scene, &actual)) return 0;
    for (i = 0u; i < 3u; ++i) {
        void* frame = actual.channels[i];
        void* verified = NULL;
        void** vtable;
        HRESULT result;
        if (frame == NULL) goto failed;
        /* Only IUnknown::QueryInterface is called before Frame3 attestation. */
        vtable = *(void***)frame;
        if (vtable == NULL || vtable[0] == NULL) goto failed;
        result = ((LwscQueryInterfaceFn)vtable[0])(frame, &s_lwsc_frame3_iid, &verified);
        if (result != 0 || verified != frame) {
            scene_winapi_interface_release(verified);
            goto failed;
        }
        retained->channels[i] = verified;
        if (!lwsc_winapi_owner_matches(verified, scene)) goto failed;
    }
    return 1;
failed:
    lwsc_winapi_channels_release(retained);
    return 0;
}

static int lwsc_winapi_root_current(LwscWinapiContext* context)
{
    if (scene_root_object_get() != context->application_scene ||
        !lwsc_winapi_record_matches(context->application_scene, &context->application_channels))
        return lwsc_winapi_poison(context, "application scene/pool lifetime ended or owner changed");
    return 1;
}

static int lwsc_winapi_ready(LwscWinapiContext* context)
{
    return lwsc_winapi_context_known(context) && !context->poisoned &&
           lwsc_winapi_root_current(context);
}

static LwscWinapiFrame* lwsc_winapi_find(LwscWinapiContext* context,
                                        const LwscFrameRef* frame)
{
    LwscWinapiFrame* entry;
    if (frame == NULL) return NULL;
    for (entry = context->frames; entry != NULL; entry = entry->next) {
        if (!entry->retired && entry->published == frame &&
            frame->interface_pointer == entry->channels.channels[0]) return entry;
    }
    return NULL;
}

static int lwsc_winapi_entry_live(LwscWinapiContext* context, LwscWinapiFrame* entry)
{
    if (!lwsc_winapi_record_matches(entry->scene_object, &entry->channels))
        return lwsc_winapi_poison(context, "registered DDi object/frame ownership changed");
    return 1;
}

/* GetParent returns a temporary real Frame3 COM reference. Check the exact
 * expected physical parent, not a resource name or infrastructure channel1. */
static int lwsc_winapi_parent_is(void* frame, void* expected)
{
    void* parent = NULL;
    void** vtable = *(void***)frame;
    HRESULT result = ((LwscParentGetFn)vtable[0x5c / 4])(frame, &parent);
    int same = result == 0 && parent == expected;
    scene_winapi_interface_release(parent);
    return same;
}

/* The existing frame-release owner has a void API. Check detachment first
 * while we still own verified interfaces, so an actual COM failure cannot
 * be mistaken for completed cleanup. The normal owner then sees no parent
 * and releases its original ref/properties exactly once. */
static int lwsc_winapi_detach(void* frame, void* expected_parent)
{
    void* parent = NULL;
    void** vtable = *(void***)frame;
    HRESULT result = ((LwscParentGetFn)vtable[0x5c / 4])(frame, &parent);
    if (result == 0 && parent != NULL) {
        if (parent != expected_parent) result = E_FAIL;
        else result = ((LwscDeleteChildFn)(*(void***)parent)[0x94 / 4])(parent, frame);
    }
    scene_winapi_interface_release(parent);
    return result == 0;
}

static int lwsc_winapi_ancestry_live(LwscWinapiContext* context, LwscWinapiFrame* entry)
{
    unsigned int count = 0u;
    while (entry != NULL) {
        void* expected = entry->parent != NULL ? entry->parent->channels.channels[0]
                                               : context->application_channels.channels[0];
        if (++count > LWSC_WINAPI_MAX_FRAMES || !lwsc_winapi_entry_live(context, entry) ||
            !lwsc_winapi_parent_is(entry->channels.channels[0], expected))
            return lwsc_winapi_poison(context, "authored frame parent/ancestor chain changed");
        entry = entry->parent;
    }
    return 1;
}

/* The existing runtime owns the selected DLL. Pin THAT same already-loaded
 * module by its exact path/handle, so even an out-of-order runtime shutdown
 * cannot unload native COM Release code while cleanup refs remain. This is
 * not an alternative RM factory or fallback DLL selection. */
static HMODULE lwsc_winapi_module_pin(void)
{
    HMODULE existing = GetModuleHandleA("D3DRM.DLL");
    HMODULE retained;
    char path[MAX_PATH];
    DWORD length;
    if (existing == NULL) return NULL;
    length = GetModuleFileNameA(existing, path, sizeof(path));
    if (length == 0u || length >= sizeof(path)) return NULL;
    retained = LoadLibraryA(path);
    if (retained != existing) {
        if (retained != NULL) FreeLibrary(retained);
        return NULL;
    }
    return retained;
}

LwscWinapiContext* lwsc_winapi_context_create(void* application_ddi_root,
                                            LwscError* error)
{
    LwscWinapiContext* context;
    const char* message = "actual initialized pooled application root required";
    if (error != NULL) memset(error, 0, sizeof(*error));
    if (application_ddi_root == NULL || application_ddi_root != scene_root_object_get() ||
        !lego::ddiscene_object_pool_owns(application_ddi_root)) goto failed;
    if (s_lwsc_context_count >= LWSC_WINAPI_MAX_CONTEXTS) {
        message = "pose backend context limit exceeded"; goto failed;
    }
    context = (LwscWinapiContext*)calloc(1, sizeof(*context));
    if (context == NULL) { message = "pose backend allocation failed"; goto failed; }
    context->application_scene = application_ddi_root;
    context->module = lwsc_winapi_module_pin();
    if (context->module == NULL ||
        !lwsc_winapi_channels_retain(application_ddi_root, &context->application_channels)) {
        if (context->module != NULL) FreeLibrary(context->module);
        free(context);
        message = "real Frame3/property ownership or loaded D3DRM pin unavailable"; goto failed;
    }
    context->parent.interface_pointer = context->application_channels.channels[0];
    context->next = s_lwsc_contexts;
    s_lwsc_contexts = context;
    ++s_lwsc_context_count;
    return context;
failed:
    if (error != NULL) {
        strncpy(error->message, message, sizeof(error->message) - 1u);
        error->message[sizeof(error->message) - 1u] = '\0';
    }
    return NULL;
}

static void lwsc_winapi_release_retired(LwscWinapiContext* context, LwscWinapiFrame* entry);

static int lwsc_winapi_frame_create(const LwscFrameRef* parent,
    LwscFrameRef* result, void* user)
{
    LwscWinapiContext* context = (LwscWinapiContext*)user;
    LwscWinapiFrame* parent_entry = NULL;
    LwscWinapiFrame* entry;
    const lego::DDiSceneObjectDescriptor* descriptor;
    if (!lwsc_winapi_ready(context)) return 0;
    if (result == NULL || result->interface_pointer != NULL)
        return lwsc_winapi_error(context, "frame result must be an empty producer slot");
    if (parent == &context->parent &&
        parent->interface_pointer == context->application_channels.channels[0])
        descriptor = &context->application_channels;
    else {
        parent_entry = lwsc_winapi_find(context, parent);
        if (parent_entry == NULL || !lwsc_winapi_ancestry_live(context, parent_entry))
            return lwsc_winapi_error(context, "foreign/inactive parent frame");
        descriptor = &parent_entry->channels;
    }
    if (context->frame_count >= LWSC_WINAPI_MAX_FRAMES)
        return lwsc_winapi_error(context, "pose backend frame limit exceeded");
    entry = (LwscWinapiFrame*)calloc(1, sizeof(*entry));
    if (entry == NULL) return lwsc_winapi_error(context, "frame registry allocation failed");
    entry->scene_object = lego::ddiscene_object_allocate(descriptor);
    if (entry->scene_object == NULL) {
        free(entry); return lwsc_winapi_error(context, "real DDi allocation/property creation failed");
    }
    /* No frame is substituted. Pool channel0 IS the authored primary frame.
     * Channel1 is its unused identity infrastructure child; channel2 follows
     * the real hidden-parent chain. Both remain owned/released by the pool. */
    if (!lwsc_winapi_channels_retain(entry->scene_object, &entry->channels)) {
        /* The pool's actual allocation/frame owner must unwind interfaces
         * whose Frame3/property attestation failed; never dispatch guessed
         * frame slots on an unverified interface. See the companion's
         * required HRESULT/failure-owner seam for this lower boundary. */
        ddiscene_object_destroy_with_mode(entry->scene_object, 0);
        free(entry);
        return lwsc_winapi_error(context, "allocated DDi Frame3/property validation failed");
    }
    entry->parent = parent_entry;
    if (parent_entry != NULL) ++parent_entry->children;
    entry->next = context->frames;
    context->frames = entry;
    ++context->frame_count;
    if (!lwsc_winapi_parent_is(entry->channels.channels[0], descriptor->channels[0]) ||
        !lwsc_winapi_parent_is(entry->channels.channels[1], entry->channels.channels[0]) ||
        !lwsc_winapi_parent_is(entry->channels.channels[2], descriptor->channels[2])) {
        entry->retired = 1;
        lwsc_winapi_poison(context, "allocated native frame parent validation failed");
        lwsc_winapi_release_retired(context, entry);
        return 0;
    }
    entry->published = result;
    result->interface_pointer = entry->channels.channels[0];
    return 1;
}

/* Release ready retired entries in child-before-parent order. No producer
 * slot is kept after retirement, so deferred releases cannot touch a freed
 * instance array. The normal DDi destructor owns ALL original channel refs,
 * property allocations, parent detachment and the pool transition. */
static void lwsc_winapi_release_retired(LwscWinapiContext* context, LwscWinapiFrame* entry)
{
    while (entry != NULL && entry->retired && entry->children == 0u && !entry->destroy_failed) {
        LwscWinapiFrame* parent = entry->parent;
        LwscWinapiFrame** link = &context->frames;
        if (lwsc_winapi_record_matches(entry->scene_object, &entry->channels)) {
            const lego::DDiSceneObjectDescriptor* owner = parent != NULL
                ? &parent->channels : &context->application_channels;
            if (!lwsc_winapi_detach(entry->channels.channels[2], owner->channels[2]) ||
                !lwsc_winapi_detach(entry->channels.channels[1], entry->channels.channels[0]) ||
                !lwsc_winapi_detach(entry->channels.channels[0], owner->channels[0])) {
                entry->destroy_failed = 1;
                lwsc_winapi_poison(context, "native detach failed; real DDi refs retained and context remains busy");
                return;
            }
            ddiscene_object_destroy_with_mode(entry->scene_object, 0);
            if (lwsc_winapi_record_matches(entry->scene_object, &entry->channels)) {
                entry->destroy_failed = 1;
                lwsc_winapi_poison(context, "normal DDi owner did not release record; context remains busy");
                return;
            }
        }
        /* If global pool destruction already retired the wrapper, do NOT
         * dereference/free its stale address. Only drop our retained COM QIs.
         * That path is cleanup after a lifecycle violation, not acceptance. */
        while (*link != NULL && *link != entry) link = &(*link)->next;
        if (*link == entry) *link = entry->next;
        lwsc_winapi_channels_release(&entry->channels);
        --context->frame_count;
        if (parent != NULL) --parent->children;
        free(entry);
        entry = parent;
    }
}

static void lwsc_winapi_frame_destroy(LwscFrameRef* frame, void* user)
{
    LwscWinapiContext* context = (LwscWinapiContext*)user;
    LwscWinapiFrame* entry;
    if (!lwsc_winapi_context_known(context)) return;
    entry = lwsc_winapi_find(context, frame);
    if (entry == NULL) {
        lwsc_winapi_error(context, "destroy rejected for foreign/cleared frame ref"); return;
    }
    frame->interface_pointer = NULL;
    entry->published = NULL;
    entry->retired = 1;
    lwsc_winapi_release_retired(context, entry);
}

static LwscWinapiFrame* lwsc_winapi_operation_frame(LwscWinapiContext* context,
                                                    const LwscFrameRef* frame)
{
    LwscWinapiFrame* entry;
    if (!lwsc_winapi_ready(context)) return NULL;
    entry = lwsc_winapi_find(context, frame);
    if (entry == NULL) { lwsc_winapi_error(context, "unknown/foreign/retired authored frame"); return NULL; }
    if (!lwsc_winapi_ancestry_live(context, entry)) return NULL;
    return entry;
}

static int lwsc_winapi_name_set(const LwscFrameRef* frame, const char* name, void* user)
{
    LwscWinapiContext* context = (LwscWinapiContext*)user;
    LwscWinapiFrame* entry = lwsc_winapi_operation_frame(context, frame);
    void* primary;
    if (entry == NULL) return 0;
    if (name == NULL || strlen(name) >= LWSC_NAME_CAPACITY)
        return lwsc_winapi_error(context, "invalid native frame name");
    primary = entry->channels.channels[0];
    if (((LwscNameSetFn)(*(void***)primary)[0x20 / 4])(primary, name) != 0)
        return lwsc_winapi_poison(context, "native frame SetName failed");
    return 1;
}

static int lwsc_winapi_matrix_set(const LwscFrameRef* frame, const float matrix[16], void* user)
{
    LwscWinapiContext* context = (LwscWinapiContext*)user;
    LwscWinapiFrame* entry = lwsc_winapi_operation_frame(context, frame);
    void* primary;
    if (entry == NULL) return 0;
    if (!lwsc_winapi_matrix_valid(matrix)) return lwsc_winapi_error(context, "invalid authored matrix");
    primary = entry->channels.channels[0];
    /* IDirect3DRMFrame3::AddTransform(D3DRMCOMBINE_REPLACE). */
    if (((LwscMatrixSetFn)(*(void***)primary)[0x38 / 4])(primary, 0u, matrix) != 0)
        return lwsc_winapi_poison(context, "native frame replace transform failed");
    return 1;
}

static int lwsc_winapi_matrix_get(const LwscFrameRef* frame, float matrix[16], void* user)
{
    LwscWinapiContext* context = (LwscWinapiContext*)user;
    LwscWinapiFrame* entry = lwsc_winapi_operation_frame(context, frame);
    void* parent = NULL;
    void* primary;
    void* expected;
    float actual[16];
    HRESULT result;
    if (entry == NULL || matrix == NULL) return 0;
    primary = entry->channels.channels[0];
    expected = entry->parent != NULL ? entry->parent->channels.channels[0]
                                     : context->application_channels.channels[0];
    result = ((LwscParentGetFn)(*(void***)primary)[0x5c / 4])(primary, &parent);
    if (result == 0 && parent == expected)
        result = ((LwscMatrixGetFn)(*(void***)primary)[0x74 / 4])(primary, parent, actual);
    else if (result == 0) result = E_FAIL;
    scene_winapi_interface_release(parent);
    if (result != 0 || !lwsc_winapi_matrix_valid(actual))
        return lwsc_winapi_poison(context, "native frame parent/GetTransform failed");
    memcpy(matrix, actual, sizeof(actual));
    return 1;
}

static int lwsc_winapi_world_position(const LwscFrameRef* frame, float xyz[3], void* user)
{
    LwscWinapiContext* context = (LwscWinapiContext*)user;
    LwscWinapiFrame* entry = lwsc_winapi_operation_frame(context, frame);
    float actual[3];
    if (entry == NULL || xyz == NULL) return 0;
    /* Reuse the actual DDi normalizer/GetPosition on this SAME pooled
     * wrapper, explicitly relative to the retained actual application root.
     * With this root alive it is identical to the consumer's NULL fallback. */
    if (!ddiscene_object_position_get(entry->scene_object, context->application_scene, actual) ||
        !lwsc_winapi_finite(actual[0]) || !lwsc_winapi_finite(actual[1]) ||
        !lwsc_winapi_finite(actual[2]))
        return lwsc_winapi_poison(context, "actual DDi frame GetPosition failed");
    memcpy(xyz, actual, sizeof(actual));
    return 1;
}

int lwsc_winapi_backend_get(LwscWinapiContext* context, LwscBackendOps* ops,
                            const LwscFrameRef** parent)
{
    if (ops != NULL) memset(ops, 0, sizeof(*ops));
    if (parent != NULL) *parent = NULL;
    if (ops == NULL || parent == NULL || !lwsc_winapi_ready(context)) return 0;
    ops->context = context;
    ops->frame_create = lwsc_winapi_frame_create;
    ops->frame_destroy = lwsc_winapi_frame_destroy;
    ops->frame_name_set = lwsc_winapi_name_set;
    ops->matrix_set = lwsc_winapi_matrix_set;
    ops->matrix_get = lwsc_winapi_matrix_get;
    ops->world_position = lwsc_winapi_world_position;
    *parent = &context->parent;
    return 1;
}

void* lwsc_winapi_scene_object(LwscWinapiContext* context, const LwscFrameRef* frame)
{
    LwscWinapiFrame* entry = lwsc_winapi_operation_frame(context, frame);
    return entry != NULL ? entry->scene_object : NULL;
}

int lwsc_winapi_context_destroy(LwscWinapiContext* context)
{
    LwscWinapiContext** link;
    HMODULE module;
    if (context == NULL) return 1;
    if (!lwsc_winapi_context_known(context)) return 0;
    if (context->frame_count != 0u)
        return lwsc_winapi_error(context, "destroy instances/retire DDi consumers before context destruction");
    link = &s_lwsc_contexts;
    while (*link != context) link = &(*link)->next;
    *link = context->next;
    --s_lwsc_context_count;
    module = context->module;
    lwsc_winapi_channels_release(&context->application_channels);
    free(context);
    FreeLibrary(module);
    return 1;
}

unsigned int lwsc_winapi_frame_count(const LwscWinapiContext* context)
{
    return lwsc_winapi_context_known(context) ? context->frame_count : 0u;
}

const char* lwsc_winapi_context_error(const LwscWinapiContext* context)
{
    return lwsc_winapi_context_known(context) ? context->error : "unknown pose backend context";
}

int lwsc_winapi_can_shutdown(void)
{
    return s_lwsc_contexts == NULL;
}

#endif /* LWSC_WINAPI_POSE_BACKEND */
