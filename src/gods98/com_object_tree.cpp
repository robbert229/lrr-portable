// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-025 G98-025: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/G98-025.csv. */
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "view.hpp"
#include "record.hpp"
#include "scene_resources.hpp"
#include "scene_pool.hpp"
#include "surface.hpp"
#include "sound.hpp"
#include "texture.hpp"
#include "heap.hpp"
#include "string.hpp"
#include "resource.hpp"
#include "crt/crt.hpp"
#include "model.hpp"
#include "../game/ai.hpp"
#include "../game/entity.hpp"
#include "../game/level.hpp"
#include "view_for_com_object_tree.hpp"
#include "resource_for_com_object_tree.hpp"

typedef void (VIEW_COM_CALL *DdiTimedInterfaceTimeCall)(void*, float);
typedef uintptr_t (VIEW_COM_CALL *DdiTimedInterfaceRefresh)(void*, void**);
typedef void (VIEW_COM_CALL *DdiTimedInterfaceApply)(void*, uintptr_t,
                                                     void**);
typedef void (VIEW_COM_CALL *DdiTimedInterfaceRecordCall)(void*, uint32_t,
                                                          void**);
typedef void (VIEW_COM_CALL *DdiTimedInterfaceValueCall)(void*, uintptr_t);
typedef void (VIEW_COM_CALL *DdiTimedInterfaceRelease)(void*);

typedef void (VIEW_COM_CALL *ObjectHierarchyChildrenGet)(void*, uint32_t*,
                                                          void***);
typedef void (VIEW_COM_CALL *ObjectHierarchyChildApply)(void*, void*);
typedef uint32_t (VIEW_COM_CALL *ObjectHierarchyCountGet)(void*);
typedef void (VIEW_COM_CALL *ObjectHierarchyChildGet)(void*, uint32_t,
                                                       void**);
typedef int32_t (VIEW_COM_CALL *ObjectHierarchyQueryInterface)(
    void*, const void*, void**);
typedef void* (VIEW_COM_CALL *ObjectHierarchyInterfaceGet)(void*);
typedef void (VIEW_COM_CALL *ObjectHierarchyRelatedGet)(void*, void**);
typedef uint32_t (VIEW_COM_CALL *ObjectHierarchyRelease)(void*);

typedef struct ObjectHierarchyGuid {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
} ObjectHierarchyGuid;

typedef struct ResourceTreeArrayWriter {
    unsigned char reserved[0x14];
    void** entries;
    uint32_t count;
    uint32_t reserved_tail;
} ResourceTreeArrayWriter;

/* DAT_0049fb48 is the observed IID {59163DE0-6D43-11CF-AC4A-0000C03825A1}.
 * Its interface name remains unknown, but its bytes and QueryInterface use
 * are established by docs/dat/annotations.csv and the retail call sites. */
static const ObjectHierarchyGuid s_object_hierarchy_resource_iid = {
    0x59163de0u, 0x6d43u, 0x11cfu,
    { 0xacu, 0x4au, 0x00u, 0x00u, 0xc0u, 0x38u, 0x25u, 0xa1u }
};

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static int resource_tree_callback_array_append(void* object, void* context);
static int resource_tree_callback_count_increment(void* object, void* context);
static void scene_owner_link_interface_release(void* object);
static void object_hierarchy_release(void* object);
static void** object_hierarchy_vtable_get(void* object);
static void object_hierarchy_children_get(void* object, uint32_t* count, void*** items_out);

/* The manager's +0x34 creates an IDirect3DRMMaterial. The reference then
 * applies two RGB triples through material slots +0x34 and +0x30. */
// FUNCTION: LEGORR 0x004894c0
void* scene_material_create(float color0, float color1, float color2,
                            float color3, float color4, float color5,
                            float power)
{
#if defined(_MSC_VER)
#define SCENE_MATERIAL_STDCALL __stdcall
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#define SCENE_MATERIAL_STDCALL __attribute__((stdcall))
#else
#define SCENE_MATERIAL_STDCALL
#endif
    typedef int32_t (SCENE_MATERIAL_STDCALL *MaterialCreateMethod)(
        void*, float, void**);
    typedef int32_t (SCENE_MATERIAL_STDCALL *MaterialColorMethod)(
        void*, float, float, float);
    void** manager_vtable;
    void** material_vtable;
    MaterialCreateMethod create_material;
    MaterialColorMethod apply_color;
    void* material = NULL;
    if (s_scene_material_manager == NULL) return NULL;
    manager_vtable = *(void***)s_scene_material_manager;
    if (manager_vtable == NULL || manager_vtable[0x34 / 4] == NULL)
        return NULL;
    create_material = (MaterialCreateMethod)manager_vtable[0x34 / 4];
    if (create_material(s_scene_material_manager, power, &material) != 0 ||
        material == NULL) return NULL;
    material_vtable = *(void***)material;
    if (material_vtable == NULL) return material;
    if (material_vtable[0x34 / 4] != NULL) {
        apply_color = (MaterialColorMethod)material_vtable[0x34 / 4];
        (void)apply_color(material, color0, color1, color2);
    }
    if (material_vtable[0x30 / 4] != NULL) {
        apply_color = (MaterialColorMethod)material_vtable[0x30 / 4];
        (void)apply_color(material, color3, color4, color5);
    }
#undef SCENE_MATERIAL_STDCALL
    return material;
}

/* ResourceTreeCallbackContextCreateForEnumerator (FUN_00489520). The
 * first argument is present at retail call sites but unused; the second is
 * retained in the context's first interface field and the third is the
 * enumerated object whose hierarchy is indexed. */
// FUNCTION: LEGORR 0x00489520
ResourceTreeCallbackContext* resource_tree_callback_context_create_for_enumerator(
    void* unused, void* owner, void* enumerator)
{
    ResourceTreeCallbackContext* context_record;
    ResourceTreeReferenceFn add_ref;
    ResourceTreeArrayWriter writer;
    void** vtable;
    (void)unused;
    context_record = (ResourceTreeCallbackContext*)engine_heap_alloc(
        (long)sizeof(ResourceTreeCallbackContext));
    context_record->kind = 0;
    context_record->first_handle = owner;
    context_record->child_chain = NULL;
    context_record->shared_mode = 0;
    context_record->second_handle = enumerator;
    vtable = *(void***)enumerator;
    add_ref = (ResourceTreeReferenceFn)vtable[1];
    (void)add_ref(enumerator);
    context_record->handle_count = 0u;
    context_record->owner = enumerator;
    (void)resource_tree_callback_count(enumerator, 0u,
                                      resource_tree_callback_count_increment,
                                      &context_record->handle_count);
    context_record->handles = (void**)engine_heap_alloc(
        (long)(context_record->handle_count * sizeof(void*)));
    writer.entries = context_record->handles;
    writer.count = 0u;
    (void)resource_tree_callback_count(enumerator, 0u,
                                      resource_tree_callback_array_append,
                                      &writer);
    return context_record;
}

/* ResourceTreeCallbackContextCreateForResource (FUN_004895c0): the alternate
 * mode stores its owner as a child-chain pointer and marks shared mode. */
// FUNCTION: LEGORR 0x004895c0
ResourceTreeCallbackContext* resource_tree_callback_context_create_for_resource(
    void* unused, void* owner, void* resource)
{
    ResourceTreeCallbackContext* context_record;
    ResourceTreeReferenceFn add_ref;
    ResourceTreeArrayWriter writer;
    void** vtable;
    (void)unused;
    context_record = (ResourceTreeCallbackContext*)engine_heap_alloc(
        (long)sizeof(ResourceTreeCallbackContext));
    context_record->kind = 0;
    context_record->first_handle = NULL;
    context_record->child_chain = owner;
    context_record->shared_mode = 1;
    context_record->second_handle = resource;
    vtable = *(void***)resource;
    add_ref = (ResourceTreeReferenceFn)vtable[1];
    (void)add_ref(resource);
    context_record->handle_count = 0u;
    context_record->owner = resource;
    (void)resource_tree_callback_count(resource, 0u,
                                      resource_tree_callback_count_increment,
                                      &context_record->handle_count);
    context_record->handles = (void**)engine_heap_alloc(
        (long)(context_record->handle_count * sizeof(void*)));
    writer.entries = context_record->handles;
    writer.count = 0u;
    (void)resource_tree_callback_count(resource, 0u,
                                      resource_tree_callback_array_append,
                                      &writer);
    return context_record;
}

/* The `words[3] != 0` record form is a shallow record clone with a retained
 * interface in word 1. The zero form builds a fresh scene-animation record;
 * its QueryInterface and animation-list clone calls are COM-specific and
 * stay behind the typed full_record_clone seam. */
// FUNCTION: LEGORR 0x00489660
SceneActFrameRecord* scene_act_frame_record_clone_or_create(
    const SceneActFrameRecord* source, uint32_t clone_argument,
    uintptr_t* output_value)
{
    SceneActFrameRecord* copy;
    if (source == NULL) return NULL;
    copy = (SceneActFrameRecord*)engine_heap_alloc(
        (long)sizeof(SceneActFrameRecord));
    if (copy == NULL) return NULL;
    if (source->words[3] != 0u) {
        memcpy(copy, source, sizeof(*copy));
        copy->words[0] = 0u;
        if (source->words[1] != 0u) {
            if (s_scene_act_frame_record_clone_hooks.interface_retain == NULL) {
                engine_heap_free(copy);
                return NULL;
            }
            copy->words[1] = (uintptr_t)
                s_scene_act_frame_record_clone_hooks.interface_retain(
                    (void*)source->words[1],
                    s_scene_act_frame_record_clone_hooks.context);
            if (copy->words[1] == 0u) {
                engine_heap_free(copy);
                return NULL;
            }
        }
    }
    else {
        memset(copy, 0, sizeof(*copy));
        if (s_scene_act_frame_record_clone_hooks.full_record_clone == NULL ||
            !s_scene_act_frame_record_clone_hooks.full_record_clone(
                source, clone_argument, copy,
                s_scene_act_frame_record_clone_hooks.context)) {
            engine_heap_free(copy);
            return NULL;
        }
    }
    if (output_value != NULL) *output_value = source->words[7];
    return copy;
}

// FUNCTION: LEGORR 0x00489750
void scene_object_owner_link_release(SceneObjectOwnerLinkRecord* record)
{
    uint32_t i;
    if (record == NULL) return;
    if (record->kind == 0) {
        if (record->shared_mode == 0) {
            scene_owner_link_interface_release(record->first_handle);
            scene_owner_link_interface_release(record->second_handle);
        }
        else if (s_owner_link_child_release != NULL) {
            s_owner_link_child_release(record->child_chain);
        }
    }
    if (record->shared_mode == 0) {
        for (i = 0; i < record->handle_count; ++i) {
            if (record->handles != NULL)
                scene_owner_link_interface_release(record->handles[i]);
        }
        engine_heap_free(record->handles);
    }
    engine_heap_free(record);
}
// FUNCTION: LEGORR 0x004897e0
/* DDiSceneTimedHookDispatch (FUN_004897e0): root interfaces receive only the
 * requested time. Nested child interfaces use the x86 ABI's +0x5c out-slot,
 * pass its return value and a local out-slot to +0x74, release the interface
 * produced by +0x5c, then run the record-child +0x38 callback. */
void ddiscene_timed_hook_dispatch(void* hook_record, float time,
                                  void* interface_value)
{
    SceneActFrameRecord* record = (SceneActFrameRecord*)hook_record;
    SceneActFrameRecord* shared;
    void** shared_children;
    void** record_children;
    void* updated_interface = interface_value;
    uintptr_t i;
    if (record == NULL) return;
    shared = (SceneActFrameRecord*)record->words[0];
    if (shared == NULL) {
        if (record->words[3] != 0u) {
            unported_ledger_stubs::
                ledger_stub_00487ba0_SharedObjectAnimationAndTimedEventUpdate(
                (void*)record->words[1], time);
        }
        else if (record->words[2] != 0u) {
            void* object = (void*)record->words[2];
            void** vtable = *static_cast<void***>(object);
            if (vtable != NULL && vtable[0x38 / 4] != NULL) {
                DdiTimedInterfaceTimeCall call =
                    reinterpret_cast<DdiTimedInterfaceTimeCall>(
                        vtable[0x38 / 4]);
                call(object, time);
            }
        }
        return;
    }
    if (shared->words[3] != 0u) {
        unported_ledger_stubs::
            ledger_stub_00487ba0_SharedObjectAnimationAndTimedEventUpdate(
            (void*)shared->words[1], time);
    }
    else if (shared->words[2] != 0u) {
        void* object = (void*)shared->words[2];
        void** vtable = *static_cast<void***>(object);
        if (vtable != NULL && vtable[0x38 / 4] != NULL) {
            DdiTimedInterfaceTimeCall call =
                reinterpret_cast<DdiTimedInterfaceTimeCall>(
                    vtable[0x38 / 4]);
            call(object, time);
        }
    }
    shared_children = (void**)shared->words[5];
    record_children = (void**)record->words[5];
    for (i = 0u; i < record->words[6]; ++i) {
        void* child = shared_children != NULL ? shared_children[i] : NULL;
        if (child != NULL) {
            void* child_result = NULL;
            void* released_interface;
            uintptr_t update_value = 0u;
            void** vtable = *static_cast<void***>(child);
            if (vtable != NULL) {
                if (vtable[0x5c / 4] != NULL) {
                    DdiTimedInterfaceRefresh refresh =
                        reinterpret_cast<DdiTimedInterfaceRefresh>(
                            vtable[0x5c / 4]);
                    update_value = refresh(child, &updated_interface);
                }
                if (vtable[0x74 / 4] != NULL) {
                    DdiTimedInterfaceApply apply =
                        reinterpret_cast<DdiTimedInterfaceApply>(
                            vtable[0x74 / 4]);
                    apply(child, update_value, &child_result);
                }
            }
            released_interface = updated_interface;
            if (released_interface != NULL) {
                void** updated_vtable =
                    *static_cast<void***>(released_interface);
                if (updated_vtable != NULL && updated_vtable[2] != NULL) {
                    DdiTimedInterfaceRelease release =
                        reinterpret_cast<DdiTimedInterfaceRelease>(
                            updated_vtable[2]);
                    release(released_interface);
                }
            }
        }
        if (record_children != NULL && record_children[i] != NULL) {
            void* record_child = record_children[i];
            void* child_output = NULL;
            void** vtable = *static_cast<void***>(record_child);
            if (vtable != NULL && vtable[0x38 / 4] != NULL) {
                DdiTimedInterfaceRecordCall call =
                    reinterpret_cast<DdiTimedInterfaceRecordCall>(
                        vtable[0x38 / 4]);
                call(record_child, 0u, &child_output);
            }
        }
    }
    if (updated_interface != NULL) {
        if (shared->words[3] != 0u) {
            uint32_t output_bits = (uint32_t)(uintptr_t)updated_interface;
            float output_value;
            memcpy(&output_value, &output_bits, sizeof(output_value));
            unported_ledger_stubs::
                ledger_stub_00487ba0_SharedObjectAnimationAndTimedEventUpdate(
                (void*)shared->words[1], output_value);
        }
        else if (shared->words[2] != 0u) {
            void* object = (void*)shared->words[2];
            void** vtable = *static_cast<void***>(object);
            if (vtable != NULL && vtable[0x38 / 4] != NULL) {
                DdiTimedInterfaceValueCall call =
                    reinterpret_cast<DdiTimedInterfaceValueCall>(
                        vtable[0x38 / 4]);
                call(object, (uintptr_t)updated_interface);
            }
        }
    }
}

// FUNCTION: LEGORR 0x004898e0
static int resource_tree_callback_count_increment(void* object, void* context)
{
    uint32_t* count = (uint32_t*)context;
    (void)object;
    ++*count;
    return 0;
}

// FUNCTION: LEGORR 0x004898f0
static int resource_tree_callback_array_append(void* object, void* context)
{
    ResourceTreeArrayWriter* writer = (ResourceTreeArrayWriter*)context;
    writer->entries[writer->count] = object;
    ++writer->count;
    return 0;
}

/* ResourceTreeCallbackCount (FUN_00489910): offer the current interface to
 * the callback first. If it declines, enumerate its child array, query each
 * returned object for the resource-tree interface, and recurse. The child
 * object, queried interface, and array each have distinct COM references and
 * are released at the same points as in the retail routine. */
// FUNCTION: LEGORR 0x00489910
uint32_t resource_tree_callback_count(void* object, int depth,
                                      ResourceTreeCallback callback,
                                      void* context)
{
    void* callback_context = context;
    ResourceTreeCallback callback_function = callback;
    void* current_object = object;
    void** object_vtable;
    void** array_vtable;
    ResourceTreeGetChildrenFn get_children;
    void* child_array;
    uint32_t child_count;
    uint32_t index;
    int found = 0;
    if (callback_function(current_object, callback_context) != 0) return 1u;
    object_vtable = *(void***)current_object;
    get_children = (ResourceTreeGetChildrenFn)object_vtable[0x4c / 4];
    if (get_children(current_object, &child_array) == 0) {
        array_vtable = *(void***)child_array;
        ResourceTreeArraySizeFn get_size;
        get_size = (ResourceTreeArraySizeFn)array_vtable[0x0c / 4];
        child_count = get_size(child_array);
        for (index = 0u; index < child_count; ++index) {
            void* child;
            void* child_interface;
            void** child_vtable;
            void** current_array_vtable = *(void***)child_array;
            ResourceTreeArrayElementFn get_element =
                (ResourceTreeArrayElementFn)current_array_vtable[0x10 / 4];
            ResourceTreeQueryInterfaceFn query_interface;
            ResourceTreeReferenceFn release_child;
            (void)get_element(child_array, index, &child);
            child_vtable = *(void***)child;
            query_interface = (ResourceTreeQueryInterfaceFn)child_vtable[0];
            (void)query_interface(child, s_resource_tree_child_iid,
                                  &child_interface);
            release_child = (ResourceTreeReferenceFn)child_vtable[2];
            (void)release_child(child);
            if (resource_tree_callback_count(child_interface, depth + 1,
                                             callback_function,
                                             callback_context) != 0u) {
                void** child_interface_vtable = *(void***)child_interface;
                ResourceTreeReferenceFn release_interface =
                    (ResourceTreeReferenceFn)child_interface_vtable[2];
                found = 1;
                (void)release_interface(child_interface);
                break;
            }
            {
                void** child_interface_vtable = *(void***)child_interface;
                ResourceTreeReferenceFn release_interface =
                    (ResourceTreeReferenceFn)child_interface_vtable[2];
                (void)release_interface(child_interface);
            }
        }
        array_vtable = *(void***)child_array;
        ResourceTreeReferenceFn release_array =
            (ResourceTreeReferenceFn)array_vtable[2];
        (void)release_array(child_array);
    }
    return (uint32_t)found;
}

// FUNCTION: LEGORR 0x004899f0
void object_hierarchy_recursive_clone(void* source, void* clone,
                                      uint32_t mode)
{
    void** source_vtable;
    void** clone_vtable;
    uint32_t state_bytes = 0u;
    uint32_t state_value = 0u;
    void* state_buffer = NULL;
    void* related_interface = NULL;
    void* related_result = NULL;
    void* child_list = NULL;
    uint32_t child_count;
    uint32_t index;
    if (source == NULL || clone == NULL) return;
    if (mode != 0u)
        object_interface_array_resource_create(source, mode);
    else
        com_child_items_visit(source);

    source_vtable = object_hierarchy_vtable_get(source);
    clone_vtable = object_hierarchy_vtable_get(clone);
    if (source_vtable == NULL || clone_vtable == NULL) return;

    /* Retail first asks slot +0x24 for a byte count, allocates that exact
     * block, asks the same slot to fill it, and gives it to clone slot +0x20. */
    if (source_vtable[0x24 / 4] != NULL &&
        clone_vtable[0x20 / 4] != NULL) {
        typedef void (VIEW_COM_CALL *StateRead)(void*, void*, void*);
        typedef void (VIEW_COM_CALL *StateApply)(void*, void*);
        ((StateRead)source_vtable[0x24 / 4])(source, &state_bytes, NULL);
        if (state_bytes != 0u && state_bytes <= 0x7fffffffu) {
            state_buffer = engine_heap_alloc((long)state_bytes);
            if (state_buffer != NULL) {
                ((StateRead)source_vtable[0x24 / 4])(source, &state_value,
                                                     state_buffer);
                ((StateApply)clone_vtable[0x20 / 4])(clone, state_buffer);
                engine_heap_free(state_buffer);
            }
        }
    }

    /* Preserve the renderer's interface refresh/apply order and release the
     * temporary interface returned by +0x5c. */
    if (source_vtable[0x5c / 4] != NULL &&
        source_vtable[0x74 / 4] != NULL) {
        typedef void (VIEW_COM_CALL *RelatedApply)(void*, void*, void**);
        ((ObjectHierarchyRelatedGet)source_vtable[0x5c / 4])(
            source, &related_interface);
        ((RelatedApply)source_vtable[0x74 / 4])(source, related_interface,
                                                &related_result);
        object_hierarchy_release(related_interface);
    }
    if (clone_vtable[0x38 / 4] != NULL) {
        typedef void (VIEW_COM_CALL *CloneUpdate)(void*, uint32_t, void**);
        ((CloneUpdate)clone_vtable[0x38 / 4])(clone, 0u, &related_result);
    }

    if (source_vtable[0x4c / 4] == NULL) return;
    {
        typedef int (VIEW_COM_CALL *ChildListGet)(void*, void**);
        if (((ChildListGet)source_vtable[0x4c / 4])(source, &child_list) != 0 ||
            child_list == NULL) return;
    }
    {
        void** list_vtable = object_hierarchy_vtable_get(child_list);
        if (list_vtable == NULL || list_vtable[0x0c / 4] == NULL ||
            list_vtable[0x10 / 4] == NULL) {
            object_hierarchy_release(child_list);
            return;
        }
        child_count = ((ObjectHierarchyCountGet)list_vtable[0x0c / 4])(
            child_list);
        for (index = 0u; index < child_count; ++index) {
            void* child = NULL;
            ((ObjectHierarchyChildGet)list_vtable[0x10 / 4])(
                child_list, index, &child);
            if (child != NULL) {
                if (s_object_hierarchy_resource_hooks.child_clone_register !=
                    NULL) {
                    void* source_child = NULL;
                    void* clone_child = NULL;
                    if (s_object_hierarchy_resource_hooks.child_clone_register(
                            source, clone, child, &source_child, &clone_child,
                            s_object_hierarchy_resource_hooks.context) &&
                        source_child != NULL && clone_child != NULL) {
                        object_hierarchy_recursive_clone(source_child,
                                                         clone_child, mode);
                        object_hierarchy_release(clone_child);
                        object_hierarchy_release(source_child);
                    }
                }
                object_hierarchy_release(child);
            }
        }
        object_hierarchy_release(child_list);
    }
}
// FUNCTION: LEGORR 0x00489b30
void object_interface_array_resource_create(void* object, uint32_t mode)
{
    void** items = NULL;
    uint32_t count = 0u;
    uint32_t index;
    object_hierarchy_children_get(object, &count, NULL);
    if (count == 0u) return;
    object_hierarchy_children_get(object, &count, &items);
    if (items == NULL) return;
    for (index = 0u; index < count; ++index) {
        void* queried = NULL;
        void** item_vtable = object_hierarchy_vtable_get(items[index]);
        if (item_vtable != NULL && item_vtable[0] != NULL) {
            int32_t result = ((ObjectHierarchyQueryInterface)item_vtable[0])(
                items[index], &s_object_hierarchy_resource_iid, &queried);
            if (result == 0 && queried != NULL) {
                void** queried_vtable = object_hierarchy_vtable_get(queried);
                if (queried_vtable != NULL &&
                    queried_vtable[0x1c / 4] != NULL) {
                    void* resource = ((ObjectHierarchyInterfaceGet)
                        queried_vtable[0x1c / 4])(queried);
                    if (resource != NULL &&
                        s_object_hierarchy_resource_hooks.template_resolve !=
                            NULL) {
                        lego::CachedRenderResourceTemplateRecord* source =
                            s_object_hierarchy_resource_hooks.template_resolve(
                                resource, mode,
                                s_object_hierarchy_resource_hooks.context);
                        if (source != NULL)
                            (void)lego::cached_render_resource_instance_create(
                                source);
                    }
                }
            }
        }
        /* The reference retains the queried interface in the resource path;
         * there is no Release call at 0x489b30. */
    }
}
// FUNCTION: LEGORR 0x00489bc0
void com_child_items_visit(void* parent)
{
    void** vtable;
    void** items = NULL;
    uint32_t count = 0u;
    uint32_t index;
    object_hierarchy_children_get(parent, &count, NULL);
    if (count == 0u) return;
    object_hierarchy_children_get(parent, &count, &items);
    if (items == NULL) return;
    vtable = object_hierarchy_vtable_get(parent);
    if (vtable != NULL && vtable[0x48 / 4] != NULL) {
        ObjectHierarchyChildApply apply =
            (ObjectHierarchyChildApply)vtable[0x48 / 4];
        for (index = 0u; index < count; ++index)
            apply(parent, items[index]);
    }
}
static void scene_owner_link_interface_release(void* object)
{
    if (s_owner_link_interface_release != NULL)
        s_owner_link_interface_release(object);
    else
        ddiscene_payload_interface_release(object);
}

static void** object_hierarchy_vtable_get(void* object)
{
    void** vtable = NULL;
    if (object != NULL) memcpy(&vtable, object, sizeof(vtable));
    return vtable;
}

static void object_hierarchy_children_get(void* object, uint32_t* count,
                                          void*** items_out)
{
    void** vtable = object_hierarchy_vtable_get(object);
    if (count == NULL) return;
    if (vtable == NULL || vtable[0x80 / 4] == NULL) {
        *count = 0u;
        if (items_out != NULL) *items_out = NULL;
        return;
    }
    /* Slot +0x80 returns the count and a borrowed child pointer array. The
     * retail callers query with a null array output, then fetch the pointer;
     * they neither allocate a copy nor release the returned entries. */
    ((ObjectHierarchyChildrenGet)vtable[0x80 / 4])(object, count, items_out);
}

static void object_hierarchy_release(void* object)
{
    void** vtable = object_hierarchy_vtable_get(object);
    if (vtable != NULL && vtable[2] != NULL)
        ((ObjectHierarchyRelease)vtable[2])(object);
}
