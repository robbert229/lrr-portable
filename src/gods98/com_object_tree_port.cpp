// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-025 AnimClone.c's port tail (finding 616). animclone.c is the unit,
 * rebuilt in original shape; this file keeps, unannotated, the port's models
 * of the three functions that port code calls by the port's names:
 * scene_material_create (Material_Create, 0x004894c0; terrain_map.c and
 * view.cpp), scene_object_owner_link_release (AnimClone_Remove, 0x00489750)
 * and ddiscene_timed_hook_dispatch (AnimClone_SetTime, 0x004897e0;
 * container_unit.cpp and view.cpp). Natively they serve the port's scene
 * records through view.cpp's hooks. In the matching build animclone.c serves
 * those names (animclone_link_names.h), so they are compiled out there
 * (LRR_ANIMCLONE_PORT_NAMES). Relocated from com_object_tree.cpp
 * (tools/relocate_unit.py, ADR 0006); body hashes:
 * docs/coverage/relocations/G98-025.csv. The port's other models of the
 * unit had no caller and are deleted. */
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

typedef void (VIEW_COM_CALL *DdiTimedInterfaceTimeCall)(void*, float);
typedef uintptr_t (VIEW_COM_CALL *DdiTimedInterfaceRefresh)(void*, void**);
typedef void (VIEW_COM_CALL *DdiTimedInterfaceApply)(void*, uintptr_t,
                                                     void**);
typedef void (VIEW_COM_CALL *DdiTimedInterfaceRecordCall)(void*, uint32_t,
                                                          void**);
typedef void (VIEW_COM_CALL *DdiTimedInterfaceValueCall)(void*, uintptr_t);
typedef void (VIEW_COM_CALL *DdiTimedInterfaceRelease)(void*);

/* animclone.c serves the port's names of these functions in the matching
 * build (animclone_link_names.h): every build but MSVC 5.0's. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define LRR_ANIMCLONE_PORT_NAMES 0
#else
#define LRR_ANIMCLONE_PORT_NAMES 1
#endif

#if LRR_ANIMCLONE_PORT_NAMES /* animclone.c serves these names in the matching build */
static void scene_owner_link_interface_release(void* object);

/* The manager's +0x34 creates an IDirect3DRMMaterial. The reference then
 * applies two RGB triples through material slots +0x34 and +0x30. */
/* Port model of 0x004894c0 */
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

/* Port model of 0x00489750 */
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
/* Port model of 0x004897e0 */
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

static void scene_owner_link_interface_release(void* object)
{
    if (s_owner_link_interface_release != NULL)
        s_owner_link_interface_release(object);
    else
        ddiscene_payload_interface_release(object);
}
#endif /* LRR_ANIMCLONE_PORT_NAMES */
